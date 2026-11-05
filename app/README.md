# SGIP - Sistema de Gestión Inteligente de Parqueos
## Guía de Diseño y Sistema Visual de la Interfaz Web

Este documento define la arquitectura visual, el sistema de diseño (tokens, paletas, tipografía), los componentes y las especificaciones exactas del plano vectorial 2D implementado en la aplicación web del SGIP. Está orientado a diseñadores de producto (UI/UX), diseñadores gráficos e ingenieros de frontend.

---

## 1. Filosofía de Diseño

El diseño de SGIP responde a una premisa fundamental: **máxima legibilidad operativa con estética minimalista y limpia**.

* **Sin emojis**: Queda estrictamente descartado el uso de emojis en toda la interfaz, textos, componentes y notificaciones.
* **Colores planos y sólidos**: No se utilizan gradientes, mapas de relieve ni texturas fotorealistas. La información se comunica mediante contraste cromático y geometría precisa.
* **Enfoque informativo**: La representación gráfica del parqueo es esquemática y fiel a la orientación física, pero despojada de ruido visual innecesario (sin bordillos simulados, sin asfalto texturizado, sin etiquetas redundantes como "Zona Oeste" o "Zona Este").
* **Identidad propia**: El proyecto responde a las siglas **SGIP** (no parqueame).

---

## 2. Sistema de Colores (Design Tokens)

El sistema cromático está estructurado mediante variables CSS para facilitar la coherencia y el mantenimiento.

### 2.1 Color Primario y Fondos
* **Fondo de página (`--bg-page`)**: `#ffffff` (Blanco puro).
* **Fondo de superficie (`--bg-surface`)**: `#f8fafc` (Gris frío muy claro para el área del canvas).
* **Superficie de tarjetas y navbar (`--bg-card`)**: `#ffffff`.

### 2.2 Color Secundario de Marca
* **Secundario principal (`--color-secondary`)**:
  * HEX: `#20124d`
  * RGB: `rgb(32, 18, 77)`
  * HSL: `hsl(254, 62%, 19%)`
  * Aplicación: Barra de navegación, encabezados, botones activos, iconos principales y contornos de marca.
* **Secundario variante clara (`--color-secondary-light`)**: `#311c75` (Hover en botones primarios).
* **Secundario sutil (`--color-secondary-subtle`)**: `#f0eef8` (Fondos de estados activos y hover en listados).

### 2.3 Estados de Estacionamiento
* **Disponible / Libre**:
  * Relleno principal: `#16a34a` (Verde sólido).
  * Fondo de pastilla / tarjeta: `#dcfce7`.
  * Borde de tarjeta: `#86efac`.
* **Ocupado**:
  * Relleno principal: `#dc2626` (Rojo sólido).
  * Fondo de pastilla / tarjeta: `#fee2e2`.
  * Borde de tarjeta: `#fca5a5`.
* **Baja Confianza / Revisión Óptica**:
  * Relleno principal: `#d97706` (Ámbar / Amarillo de alerta).
  * Fondo de pastilla / tarjeta: `#fef3c7`.
  * Borde de tarjeta: `#fcd34d`.

### 2.4 Asfalto y Delimitación Perimetral
* **Relleno de asfalto (`--color-asphalt`)**: `#556274` (Tono pizarra uniforme, claro y con alto contraste frente a las plazas).
* **Contorno de asfalto (`--color-asphalt-edge`)**: `#3a4554` (Trazo de 0.8px para delimitar el perímetro).

### 2.5 Vegetación y Árboles (Volumen Plano)
El arbolado utiliza de forma estricta una paleta de 3 tonos verdes planos para generar volumen mediante superposición geométrica:
* **Verde Oliva Oscuro**: `#196F3D` (Círculos base y zonas de sombra).
* **Verde Pino Medio**: `#27AE60` (Cuerpo principal de la masa vegetal).
* **Esmeralda Suave**: `#1ABC9C` (Copas superiores y puntos de luz).

---

## 3. Logotipo e Identidad Visual

### 3.1 Archivos de Logo
* [app/public/logo.svg](file:///home/thegxnster/parqueame/app/public/logo.svg): Logotipo oficial en vectores puros.
  * Fondo: Totalmente transparente (sin lienzo blanco contenedor).
  * Color de trazo y relleno: `#20124d` (color secundario de marca).
  * Proporción de aspecto: 1.61:1 (ancho respecto a alto).
  * Visualización en barra de navegación: `height: 38px; width: auto; max-width: 65px;`.
* [app/public/favicon.svg](file:///home/thegxnster/parqueame/app/public/favicon.svg) y [app/public/favicon.png](file:///home/thegxnster/parqueame/app/public/favicon.png): Icono de pestaña (favicon) centrado en formato cuadrado sin fondo.

### 3.2 Nombre y Subtítulo
* **Nombre de marca**: SGIP
* **Subtítulo institucional**: Sistema de Gestión Inteligente de Parqueos

---

## 4. Tipografía y Jerarquía

* **Fuente principal**: `Plus Jakarta Sans`, con respaldo en `system-ui, -apple-system, sans-serif`.
* **Escala y pesos**:
  * Título de marca (Navbar): `18px`, peso `800` (ExtraBold), `letter-spacing: -0.3px`.
  * Subtítulo de marca: `11px`, peso `500` (Medium).
  * Título de locación (Subheader): `14.5px`, peso `700` (Bold).
  * Contadores métricos de plazas: `12.5px`, peso `800` para cifras numéricas.
  * Botones de filtro: `12px`, peso `700`.
  * Título del drawer lateral: `18px`, peso `800`.
  * Etiquetas de telemetría: `13px`, peso `500` (label) y `700` (valor).
  * Números dentro de cada plaza vectorial: `1.85px` relativo al viewBox, peso `700`, centrado perfecto (`text-anchor: middle; dominant-baseline: central; fill: #ffffff;`).

---

## 5. Estructura de Componentes de la Web

La interfaz se divide en cuatro bloques principales dispuestos verticalmente:

```
+-----------------------------------------------------------------------------------+
| 1. NAVBAR SUPERIOR (64px, Sticky, z-index: 60, Fondo Blanco)                      |
| [Logo SGIP] SGIP                       [Selector de Locaciones Cubiertas v]       |
+-----------------------------------------------------------------------------------+
| 2. SUBHEADER DE MÉTRICAS Y FILTROS (64px, z-index: 50, Fondo Blanco)              |
| Campus Principal UCE (219 Plazas) | 84 Libres | 122 Ocupados | 13 Revisión        |
|                                   [Todos (219)] [Solo Libres] [Solo Ocupados]     |
+-----------------------------------------------------------------------------------+
| 3. CANVAS VIEWPORT INTERACTIVO (Flex: 1, Fondo #f8fafc)                           |
|                                                                                   |
|                      [ MAPA 2D VECTORIAL DEL PARQUEO ]                            |
|             (Asfalto #556274, 219 Plazas con Borde Blanco,                       |
|               Follaje en 3 Tonos, Zoom Simétrico Centrado)       [+]              |
|                                                                  [-] [100%]       |
|                                                                  [R]              |
+-----------------------------------------------------------------------------------+
| 4. DRAWER LATERAL DE DETALLE (Slide-in Modal, Ancho: 380px, z-index: 160)         |
| [X] Espacio L1-A01 | Tarjeta de Estado | Telemetría (GPS, %) | [Ruta en G. Maps]  |
+-----------------------------------------------------------------------------------+
```

### 5.1 Barra de Navegación (Navbar)
* Altura: `64px`, posición `sticky; top: 0; z-index: 60;`.
* Borde inferior de 1px en `#e2e8f0`.
* Selector desplegable de **Locaciones Cubiertas**:
  * **Campus Principal de la UCE (San Pedro de Macorís)**: Única locación activa por defecto.
  * **Multiplaza Higüey**: Próximamente.
  * **Plaza de la Restauración (San Pedro de Macorís)**: Próximamente.
  * **Parqueo de la Basílica de Higüey**: Próximamente.
  * **Jumbo San Pedro**: Próximamente.
  * **Multiplaza La Romana**: Próximamente.
  * **Jumbo La Romana**: Próximamente.
  * **Bávaro City Center**: Próximamente.

### 5.2 Subheader de Estado y Filtros
* Resumen en vivo de plazas: pastillas con borde suave y punto indicador verde, rojo y ámbar.
* Botones de alternancia de filtros:
  * **Todos (219)**: Muestra todas las plazas activas.
  * **Solo Libres**: Atenúa al 18% de opacidad las plazas ocupadas y en revisión.
  * **Solo Ocupados**: Atenúa al 18% de opacidad las plazas libres y en revisión.

### 5.3 Motor de Zoom del Canvas
* **Escala simétrica y uniforme**: Anclada en el centro geométrico del plano `(92, 91)` mediante `transform: translate(92, 91) scale(S) translate(-92, -91)`.
* **Rango de zoom**: De `1.0` ($100\%$) a `2.4` ($240\%$), con incrementos de `0.20`.
* **Cero desplazamiento lateral**: Queda bloqueado el arrastre libre con el ratón hacia los lados para mantener el plano siempre centrado e intuitivo.
* **Desbordamiento vertical superior**: Gracias a la regla `overflow: visible` del SVG y la jerarquía de capas, al hacer zoom el mapa crece uniformemente y su parte superior se oculta con elegancia detrás de la barra superior.
* **Controles flotantes**: Botón Acercar (`+`), Alejar (`-`) y Restablecer (`100%`).

### 5.4 Drawer de Telemetría (Slide-in)
* Al hacer clic sobre cualquier plaza en el mapa, se resalta con un contorno blanco más grueso (`0.9px`) y resplandor sutil.
* Se despliega un panel lateral derecho con:
  * Identificador único (ej. `L1-A01`, `R2-B08`).
  * Denominación de la hilera física y lado de estacionamiento.
  * Nivel de confianza del modelo óptico de detección.
  * Coordenadas GPS con precisión de 6 decimales.
  * Botón directo con enlace a **Google Maps** (`https://www.google.com/maps/dir/?api=1&destination=lat,lng`) para trazar la ruta hacia ese punto exacto.

---

## 6. Especificación del Plano Vectorial 2D

El plano oficial del parqueo se encuentra en dos representaciones:
* [app/assets/maps/representacion_vectorial_mapa_uce.svg](file:///home/thegxnster/parqueame/app/assets/maps/representacion_vectorial_mapa_uce.svg): Vectorial maestro con la geometría de las islas y perímetros.
* [app/public/mapa_parqueo_uce.svg](file:///home/thegxnster/parqueame/app/public/mapa_parqueo_uce.svg): Mapa exclusivo independiente con asfalto, vegetación volumétrica, numeración y plazas coloreadas por estado.

### 6.1 Dimensiones y Regularización de Plazas
Todas las 219 plazas del estacionamiento están matemáticamente estandarizadas:
* **Ancho del rectángulo**: `3.300 mm`
* **Largo del rectángulo**: `7.800 mm`
* **Radio de curvatura de esquinas (`rx`, `ry`)**: `0.550 mm`
* **Contorno / Demarcación**: `stroke: #ffffff; stroke-width: 0.35;`
* **Paso entre plazas contiguas**: `3.540 mm` (distancia centro a centro).

### 6.2 Distribución de Hileras en el Campus UCE
El estacionamiento se divide físicamente en dos grandes sectores separados por un corredor vegetal:
* **Sector Oeste (4 hileras dobles y 1 superior)**:
  * `L1`: Hilera doble (Lado A y Lado B), 9 plazas por lado (18 plazas). Ángulo: `-31.5°`.
  * `L2`: Hilera doble, 9 plazas por lado (18 plazas). Ángulo: `-31.5°`.
  * `L3`: Hilera doble, 9 plazas por lado (18 plazas). Ángulo: `-31.5°`.
  * `L4`: Hilera doble, 9 plazas por lado (18 plazas). Ángulo: `-31.5°`.
  * `LW-top`: Hilera superior simple, 11 plazas. Ángulo: `-30.7°`.
* **Sector Este (4 hileras dobles, 1 superior y 2 auxiliares)**:
  * `LE-top`: Hilera superior simple, 11 plazas. Ángulo: `-30.7°`.
  * `R1`: Hilera doble, 14 plazas por lado (28 plazas). Ángulo: `-31.5°`.
  * `R2`: Hilera doble, 14 plazas por lado (28 plazas). Ángulo: `-31.5°`.
  * `R3`: Hilera doble, 14 plazas por lado (28 plazas). Ángulo: `-31.5°`.
  * `R4`: Hilera doble, 14 plazas por lado (28 plazas). Ángulo: `-25.5°`.
  * `R-bot`: Hilera inferior simple, 10 plazas. Ángulo: `-22.0°`.
  * `R-aux`: Hilera auxiliar sur, 3 plazas. Ángulo: `-21.48°`.
* **Total de Plazas**: **219 plazas numeradas**.

### 6.3 Capas y Orden de Renderizado SVG
1. `<g id="perimeterTrees">`: Follaje exterior (222 círculos en 3 tonos de verde) que enmarcan el contorno del predio.
2. `<path id="perimetro" class="perimeter-asphalt">`: Polígono de asfalto pizarra (`#556274`).
3. `<g id="centralTrees">`: Follaje del corredor central (142 círculos superpuestos) situado directamente sobre el asfalto entre las islas oeste y este.
4. `<g id="parkingLots">`: Las 12 hileras con los rectángulos demarcados en blanco, centros en color de estado y texto numérico.

---

## 7. Archivos de Referencia en el Repositorio

* [app/index.html](file:///home/thegxnster/parqueame/app/index.html): Implementación completa autocontenida de la interfaz de usuario en un solo archivo.
* [app/public/logo.svg](file:///home/thegxnster/parqueame/app/public/logo.svg): Logotipo oficial SVG sin fondo blanco.
* [app/public/favicon.svg](file:///home/thegxnster/parqueame/app/public/favicon.svg): Favicon vectorial SVG.
* [app/public/mapa_parqueo_uce.svg](file:///home/thegxnster/parqueame/app/public/mapa_parqueo_uce.svg): Plano vectorial autónomo completo del estacionamiento con colores y árboles.
* [app/assets/maps/representacion_vectorial_mapa_uce.svg](file:///home/thegxnster/parqueame/app/assets/maps/representacion_vectorial_mapa_uce.svg): Vectorial fuente con las geometrías base de las hileras.
* [app/assets/images/parqueo-uce-visto-desde-arriba.png](file:///home/thegxnster/parqueame/app/assets/images/parqueo-uce-visto-desde-arriba.png): Fotografía satelital original cenital del parqueo de la UCE.
