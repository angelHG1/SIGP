/*
  =============================================================================
  PARQUEAME - Firmware de Cámara para Seeed Studio XIAO ESP32-S3 Sense
  Arquitectura: Cliente HTTP Multipart (Push Directo)
  Alimentación: Escudo de Batería 18650 V8 (Doble Salida 5V/3A y 3V/1A)
  =============================================================================

  Características:
  - DEV_MODE = true:
      * El puerto USB CDC (Serie) se mantiene activo permanentemente.
      * NUNCA entra en Deep Sleep -> no pierde el puerto COM en la PC.
      * Permite reflashear directamente con arduino-cli sin presionar botones 'B' o 'R'.
      * Envía capturas periódicas cada INTERVAL_SECONDS con logs en Serial Monitor.
  
  - DEV_MODE = false (Modo Producción):
      * Captura foto -> Conecta WiFi -> Sube a FastAPI -> Apaga WiFi -> Deep Sleep.
      * Bajo consumo entre disparos alimentado por el escudo de baterías 18650 V8.
  =============================================================================
*/

#include "esp_camera.h"
#include <WiFi.h>

// =============================================================================
// 1. CONFIGURACIÓN DEL USUARIO
// =============================================================================

// Modo de ejecución:
//   true  -> Desarrollo (USB Serie permanente, SIN Deep Sleep, fácil flasheo)
//   false -> Producción (Con Deep Sleep de ultrabajo consumo para baterías)
const bool DEV_MODE = true;

// Credenciales de la red Wi-Fi (debe ser red de 2.4 GHz)
const char* WIFI_SSID = "NOMBRE_DE_TU_WIFI";
const char* WIFI_PASS = "CONTRASENA_DE_TU_WIFI";

// Dirección IP local y puerto de la laptop donde corre el servidor FastAPI
// IMPORTANTE: NO uses localhost ni 127.0.0.1 (debe ser la IP de red local, ej: 192.168.1.50)
const char* SERVER_HOST = "192.168.1.50";
const int   SERVER_PORT = 8000;

// Identificador único de esta cámara (cam_01, cam_02, cam_03, cam_04)
const char* CAMERA_ID = "cam_01";

// Intervalo entre cada fotografía en segundos
const uint32_t INTERVAL_SECONDS = 30;

// =============================================================================
// 2. CONFIGURACIÓN DE TELEMETRÍA (ESCUDO 18650 V8)
// =============================================================================
// El escudo V8 regula su salida a 5V/3A constantes. Para medir el estado real de
// la celda 18650, se debe conectar el pad "B+" / "BAT+" del escudo al pin A0
// (GPIO 1) a través de un divisor resistivo (dos resistencias de 100k ohm).
// Si no tienes soldado este cable, déjalo en 'false' para enviar valores nominales.
const bool  HAS_BATTERY_DIVIDER    = false; 
const int   BATTERY_ADC_PIN        = 1;     // Pin A0 / D0 en XIAO ESP32S3 = GPIO 1
const float VOLTAGE_DIVIDER_RATIO  = 2.0;   // Multiplicador (división entre 2)

// =============================================================================
// 3. MAPEO DE PINES DE CÁMARA (SEEED STUDIO XIAO ESP32S3 SENSE)
// =============================================================================
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39

#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

// Contador de ciclos persistente a través de Deep Sleep
RTC_DATA_ATTR uint32_t bootCount = 0;

// =============================================================================
// 4. LECTURA DE TELEMETRÍA DE BATERÍA
// =============================================================================
struct BatteryInfo {
  float voltage;     // En Voltios (ej. 3.95 V)
  float percentage;  // 0.0% a 100.0%
};

BatteryInfo readBattery() {
  BatteryInfo info;
  if (!HAS_BATTERY_DIVIDER) {
    info.voltage = 4.00;
    info.percentage = 100.0;
    return info;
  }

  uint32_t pin_mv = analogReadMilliVolts(BATTERY_ADC_PIN);
  float battery_mv = (float)pin_mv * VOLTAGE_DIVIDER_RATIO;
  info.voltage = battery_mv / 1000.0;

  // Curva simple de LiPo: 4.20V (100%) a 3.30V (0%)
  float pct = ((info.voltage - 3.30) / (4.20 - 3.30)) * 100.0;
  if (pct > 100.0) pct = 100.0;
  if (pct < 0.0) pct = 0.0;
  info.percentage = pct;
  return info;
}

// =============================================================================
// 5. INICIALIZACIÓN DEL SENSOR OV2640
// =============================================================================
bool initCameraHardware() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode    = CAMERA_GRAB_LATEST;
  config.fb_location  = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12; // Calidad JPEG (10 = muy buena, 20 = más compresión)
  config.fb_count     = 2;  // Doble buffer para evitar parpadeos y retrasos
  config.frame_size   = FRAMESIZE_VGA; // 640x480 (Resolución estándar del proyecto)

  if (!psramFound()) {
    Serial.println("[ERROR PSRAM] ¡PSRAM OPI no encontrada! Comprueba el FQBN (PSRAM=opi).");
    config.frame_size  = FRAMESIZE_QVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.fb_count    = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[ERROR CÁMARA] esp_camera_init falló con código: 0x%x\n", err);
    if (err == 0x103) {
      Serial.println("  -> Causa: Falta memoria PSRAM. Verifica que la opción OPI PSRAM esté activada.");
    } else if (err == 0x20002) {
      Serial.println("  -> Causa: Sensor no detectado en bus I2C. Comprueba la cinta flex o acoplamiento del Sense.");
    }
    return false;
  }

  // Ajustes de exposición y contraste iniciales
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, 0);     // -2 a 2
    s->set_contrast(s, 0);       // -2 a 2
    s->set_saturation(s, 0);     // -2 a 2
    s->set_whitebal(s, 1);       // Auto balance de blancos habilitado
    s->set_gain_ctrl(s, 1);      // Control automático de ganancia
    s->set_exposure_ctrl(s, 1);  // Exposición automática
  }

  return true;
}

// =============================================================================
// 6. GESTIÓN DE CONEXIÓN WIFI
// =============================================================================
bool connectWiFi(uint32_t timeoutMs = 15000) {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  Serial.printf("[WIFI] Conectando a '%s'...", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  WiFi.setSleep(false); // Mantener radio WiFi activa con baja latencia

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(400);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WIFI OK] Conectado. IP: %s | RSSI: %d dBm\n", 
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
    return true;
  } else {
    Serial.printf("[WIFI ERROR] No se pudo conectar a '%s'. Verifica la red (solo 2.4 GHz).\n", WIFI_SSID);
    return false;
  }
}

// =============================================================================
// 7. SUBIDA MULTIPART A FASTAPI (/api/v1/upload/{camera_id})
// =============================================================================
bool uploadPhotoToServer(camera_fb_t *fb, const BatteryInfo &battery) {
  if (!fb || !fb->buf || fb->len == 0) {
    Serial.println("[HTTP ERROR] Framebuffer vacío o nulo.");
    return false;
  }

  WiFiClient client;
  client.setTimeout(6000); // 6 segundos de timeout

  Serial.printf("[HTTP] Conectando a http://%s:%d ...\n", SERVER_HOST, SERVER_PORT);
  if (!client.connect(SERVER_HOST, SERVER_PORT)) {
    Serial.println("[HTTP ERROR] Conexión rechazada por el servidor.");
    Serial.println("  -> Revisa que el servidor FastAPI esté corriendo en ese puerto.");
    Serial.println("  -> Revisa que la IP de la laptop sea correcta y el firewall permita el puerto 8000.");
    return false;
  }

  String boundary = "----ParqueameBoundary7MA4YWxkTrZu0gW";
  
  // URL con query params opcionales para telemetría
  String path = "/api/v1/upload/" + String(CAMERA_ID) + 
                "?battery=" + String(battery.percentage, 1) + 
                "&voltage=" + String(battery.voltage, 2);

  String head = "--" + boundary + "\r\n" +
                "Content-Disposition: form-data; name=\"file\"; filename=\"" + CAMERA_ID + ".jpg\"\r\n" +
                "Content-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--" + boundary + "--\r\n";

  size_t totalLen = head.length() + fb->len + tail.length();

  // Enviar cabeceras HTTP
  client.print("POST " + path + " HTTP/1.1\r\n");
  client.print("Host: " + String(SERVER_HOST) + ":" + String(SERVER_PORT) + "\r\n");
  client.print("Content-Type: multipart/form-data; boundary=" + boundary + "\r\n");
  client.print("Content-Length: " + String(totalLen) + "\r\n");
  client.print("Connection: close\r\n\r\n");

  // Enviar cuerpo multipart
  client.print(head);

  // Enviar JPEG en bloques de 1024 bytes
  uint8_t *buf = fb->buf;
  size_t remaining = fb->len;
  while (remaining > 0) {
    size_t chunk = (remaining > 1024) ? 1024 : remaining;
    client.write(buf, chunk);
    buf += chunk;
    remaining -= chunk;
  }
  client.print(tail);

  // Leer respuesta del servidor
  uint32_t tStart = millis();
  bool success = false;
  String statusLine = "";
  while (client.connected() && millis() - tStart < 5000) {
    if (client.available()) {
      String line = client.readStringUntil('\n');
      if (statusLine == "") {
        statusLine = line;
      }
      if (line.indexOf("200 OK") >= 0 || line.indexOf("\"status\": \"success\"") >= 0 || line.indexOf("\"status\":\"success\"") >= 0) {
        success = true;
      }
    }
  }
  client.stop();

  if (success) {
    Serial.printf("[HTTP 200 OK] ¡Fotografía recibida y procesada correctamente! (Tamaño: %u bytes)\n", fb->len);
  } else {
    Serial.printf("[HTTP AVISO] Respuesta recibida: %s\n", statusLine.c_str());
  }

  return success;
}

// =============================================================================
// 8. TAREA PRINCIPAL DE CAPTURA Y SUBIDA
// =============================================================================
void performCaptureAndUpload() {
  bootCount++;
  BatteryInfo batt = readBattery();

  Serial.println("\n-------------------------------------------------------------");
  Serial.printf("[CICLO #%u] Cámara: %s | Modo: %s | Batería: %.2fV (%.1f%%)\n",
                bootCount, CAMERA_ID, DEV_MODE ? "DESARROLLO (Activo)" : "PRODUCCIÓN (Sleep)",
                batt.voltage, batt.percentage);
  Serial.printf("[ESTADO MEMORIA] Free Heap: %d KB | Free PSRAM: %d KB\n",
                ESP.getFreeHeap() / 1024, ESP.getFreePsram() / 1024);

  // 1. Asegurar conexión WiFi
  if (!connectWiFi(10000)) {
    Serial.println("[ERROR] Sin red WiFi disponible para subir fotograma.");
    return;
  }

  // 2. Descartar un frame anterior del buffer si hubiese quedado en cola
  camera_fb_t *fb_stale = esp_camera_fb_get();
  if (fb_stale) {
    esp_camera_fb_return(fb_stale);
  }

  // 3. Capturar frame fresco
  Serial.println("[CÁMARA] Capturando nueva fotografía...");
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("[ERROR] esp_camera_fb_get() devolvió NULL.");
    return;
  }

  Serial.printf("[CÁMARA OK] %u bytes capturados (%d x %d px)\n", fb->len, fb->width, fb->height);

  // 4. Subir por HTTP
  uploadPhotoToServer(fb, batt);

  // 5. Liberar buffer inmediatamente
  esp_camera_fb_return(fb);
  Serial.println("-------------------------------------------------------------");
}

// =============================================================================
// 9. SETUP
// =============================================================================
void setup() {
  // Inicialización de Serial USB nativo
  Serial.begin(115200);
  delay(1500); // Pequeña pausa para sincronizar terminal USB

  Serial.println("\n========================================================");
  Serial.println("  PARQUEAME - Firmware XIAO ESP32-S3 Sense");
  Serial.printf("  [MODO]  %s\n", DEV_MODE ? "DESARROLLO (Puerto COM Permanente)" : "PRODUCCIÓN (Deep Sleep)");
  Serial.printf("  [PSRAM] %s (%d KB libres)\n", psramFound() ? "OK" : "NO DETECTADA", ESP.getFreePsram() / 1024);
  Serial.printf("  [CPU]   %d MHz | Flash: %d MB\n", getCpuFrequencyMhz(), ESP.getFlashChipSize() / (1024 * 1024));
  Serial.println("========================================================\n");

  // Inicializar hardware de cámara
  if (!initCameraHardware()) {
    Serial.println("[FATAL] Fallo al iniciar la cámara. Verifique hardware antes de continuar.");
    return;
  }
  Serial.println("[INICIO] Cámara OV2640 inicializada con éxito.");

  // En modo desarrollo, conectamos WiFi desde el setup
  if (DEV_MODE) {
    connectWiFi();
    Serial.println("\n[DEV_MODE ACTIVO] El puerto COM permanecerá ABIERTO de forma ininterrumpida.");
    Serial.println("[DEV_MODE ACTIVO] Puedes recompilar y subir código en cualquier momento con arduino-cli.");
  }
}

// =============================================================================
// 10. LOOP PRINCIPAL
// =============================================================================
void loop() {
  if (DEV_MODE) {
    // -------------------------------------------------------------------------
    // MODO DESARROLLO:
    // Captura -> Sube -> Espera con delay() / temporizador -> NO SE DUERME
    // El puerto COM se mantiene conectado a Windows para flasheo directo sin botones.
    // -------------------------------------------------------------------------
    static uint32_t lastRun = 0;
    uint32_t now = millis();

    if (now - lastRun >= (INTERVAL_SECONDS * 1000) || lastRun == 0) {
      lastRun = now;
      performCaptureAndUpload();
    }

    delay(100); // Pequeño respiro para la tarea IDLE del RTOS

  } else {
    // -------------------------------------------------------------------------
    // MODO PRODUCCIÓN:
    // Captura -> Sube -> Apaga radio WiFi -> Entra en Deep Sleep
    // -------------------------------------------------------------------------
    performCaptureAndUpload();

    // Desconectar y apagar la radio WiFi para ahorrar energía
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(50);

    // Configurar temporizador de Deep Sleep
    uint64_t sleepUs = (uint64_t)INTERVAL_SECONDS * 1000000ULL;
    Serial.printf("[DEEP SLEEP] Entrando en modo reposo durante %u segundos...\n", INTERVAL_SECONDS);
    Serial.flush();
    esp_sleep_enable_timer_wakeup(sleepUs);
    esp_deep_sleep_start();
  }
}
