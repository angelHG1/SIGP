# Guia de Desarrollo y Continuidad del Frontend para SGIP

Este documento tiene como objetivo orientar a cualquier programador que se integre al proyecto SGIP (Sistema de Gestion Inteligente de Parqueos) para comprender la estructura existente, saber con precision que tareas restan por implementar en la aplicacion web, como comunicarse con el servidor backend mediante sus endpoints, como utilizar la especificacion de agentes como skill y como aplicar la metodologia de Desarrollo Guiado por Especificaciones (Spec-Driven Development) para construir las funcionalidades restantes de forma solida y asistida por inteligencia artificial.

---

## 1. Vision General del Proyecto y Analogia Fundamental

Para comprender como encajan todas las piezas de este sistema sin perderse en tecnicismos, resulta util visualizar la analogia de un restaurante de alta cocina:

El servidor backend, desarrollado en Python con FastAPI, cumple la funcion de la cocina. En la cocina no entran los comensales; alli se reciben los suministros (las fotografias enviadas periodicamente por las camaras ESP32 instaladas en los postes del parqueo), el chef (el modelo de vision artificial YOLOv8) analiza cada imagen para determinar que vehiculo esta ocupando que espacio y anota el estado en una pizarra central de inventario.

La interfaz web, ubicada en la carpeta app, cumple el rol del salon comedor. Es el espacio pulcro, comodo y visualmente atractivo donde los usuarios finales observan la disponibilidad en tiempo real mediante un mapa 2D interactivo, interactuan con los espacios para solicitar una ruta o filtran por disponibilidad.

Las llamadas de red mediante la API Fetch de JavaScript representan a los camareros. Cada pocos segundos, el camarero va hasta la ventanilla de la cocina, consulta el estado actual de las mesas y regresa al comedor para actualizar la pizarra publica sin interrumpir la experiencia de los clientes.

---

## 2. Que Queda por Hacer en la Web

La aplicacion web actual (disponible en app/index.html) cuenta con la maqueta visual y la representacion geometrica 2D completa y exacta de las 219 plazas del Campus Principal de la Universidad Central del Este (UCE). El mapa posee las dimensiones estandarizadas, el arbolado con volumen plano, los colores de estado, el selector de sedes y el motor de zoom simetrico centrado. Sin embargo, para convertir esta maqueta interactiva en un producto operativo de produccion conectado al mundo real, restan las siguientes etapas:

### 2.1 Conexion con la API en Tiempo Real (Sincronizacion Viva)
Actualmente, el archivo index.html contiene una base de datos estatica con valores simulados generados al cargar el archivo. Es necesario implementar una rutina en JavaScript que consulte periodicamente el endpoint de estado del backend (GET /api/v1/status) cada dos o tres segundos utilizando `setInterval` o mediante WebSockets, para refrescar el color de las plazas (verde, rojo, amarillo), los contadores numéricos superiores y el nivel de confianza optica.

### 2.2 Mapeo de Identificadores entre Servidor y Plano Vectorial
Las camaras envian datos asociados a identificadores de plaza internos (por ejemplo spot_01, spot_02 dentro del campo visual de cam_01). Se debe construir un archivo de correspondencia o logica de mapeo en JavaScript que asocie cada identificador emitido por la camara con el identificador fisico del plano vectorial (como L1-A01, R2-B08, etc.), de modo que la deteccion de la camara altere exactamente el rectangulo correspondiente en el mapa.

### 2.3 Fotografia de Evidencia en el Panel Lateral
Cuando un conductor hace clic en un espacio, se abre el panel lateral derecho con detalles. Una funcionalidad pendiente de alto valor es incrustar una vista previa de la fotografia real capturada por la camara en ese sector (consumiendo el endpoint GET /api/v1/cameras/{camera_id}/image). Esto permite al usuario verificar visualmente que la plaza esta efectivamente desocupada o comprobar si hay algun obstaculo.

### 2.4 Panel Analitico de Rotacion y Ocupacion
El backend ya dispone de endpoints que calculan el tiempo de permanencia promedio, la tasa de rotacion vehicular y los picos de ocupacion a lo largo del dia. Queda pendiente crear una vista secundaria o pestana de analiticas dentro de la web que consuma los endpoints de analitica y presente graficos sencillos de linea de tiempo y estadisticas para la administracion del campus.

### 2.5 Modularizacion Opcional del Codigo
La arquitectura actual mantiene todo el codigo autocontenido en index.html por razones de portabilidad inmediata. Para un mantenimiento colaborativo a largo plazo, el siguiente paso recomendado es separar los estilos en un archivo styles.css, los modulos interactivos en carpetas src/js/ (mapa, drawer, api, telemetria) y gestionar el empaquetado a traves de Vite.

---

## 3. Catalogo de Endpoints del Servidor Backend

El servidor backend corre por defecto en la direccion local `http://localhost:8000`. Dispone de documentacion interactiva autogenerada accesible directamente desde el navegador en `http://localhost:8000/docs`, donde es posible probar cada peticion con un solo clic.

A continuacion se describen los endpoints principales que la aplicacion web necesita consumir:

### 3.1 Informacion General y Salud del Sistema
* **GET /**: Retorna un saludo con el nombre del proyecto, version y estado general del servicio. Es util para verificar que el servidor esta encendido.
* **GET /api/v1/health**: Proporciona el estado de salud de la infraestructura, incluyendo el tiempo de actividad en segundos, si la aceleracion por tarjeta grafica (CUDA) esta disponible y la cantidad de camaras registradas.

### 3.2 Estado de los Puestos en Vivo
* **GET /api/v1/status**: Es el endpoint principal que la web debe consultar frecuentemente. Retorna un objeto JSON con:
  * `total_spots`: Cantidad global de plazas monitoreadas.
  * `total_occupied`: Plazas ocupadas en el instante actual.
  * `total_free`: Plazas disponibles para estacionar.
  * `timestamp`: Marca de tiempo UNIX de la ultima actualizacion.
  * `cameras`: Diccionario con el desglose de cada camara activa, incluyendo la lista de plazas bajo su cobertura, indicando si cada una esta ocupada (true o false), el nivel de confianza del modelo de inteligencia artificial (valor decimal entre 0.0 y 1.0) y las coordenadas del poligono de deteccion.

### 3.3 Detalle y Captura de Camara Individual
* **GET /api/v1/cameras/{camera_id}/status**: Devuelve la telemetria enfocada de una camara especifica, util cuando el usuario selecciona una plaza y deseamos consultar unicamente la informacion del dispositivo que la vigila.
* **GET /api/v1/cameras/{camera_id}/image**: Retorna la imagen binaria en formato JPEG de la ultima captura procesada por dicha camara. Este endpoint es el que se debe enlazar en el atributo `src` de una etiqueta `img` en el panel de detalle para mostrar la foto real.

### 3.4 Analiticas y Estadisticas Historicas
* **GET /api/v1/analytics/summary?hours=2.0**: Entrega un informe consolidado del periodo indicado (por defecto dos horas). Incluye la tasa de rotacion por plaza, la duracion media de estacionamiento en minutos y el porcentaje medio de ocupacion.
* **GET /api/v1/analytics/timeline?hours=2.0&bucket_minutes=15**: Proporciona una serie temporal dividida en intervalos de quince minutos con el promedio de vehiculos estacionados, ideal para alimentar graficos de barras o lineas en un panel administrativo.

---

## 4. Como Utilizar la Skill (AGENTS.md)

En la raiz del proyecto se encuentra el archivo `AGENTS.md`. Este documento no es una simple lectura para personas, sino una especificacion formal disenada especificamente para actuar como una directiva de comportamiento (skill) para agentes de inteligencia artificial (como Antigravity, Claude, ChatGPT o Cursor).

### 4.1 Cual es el Proposito de la Skill
Cuando un programador solicita a un modelo de inteligencia artificial que realice un cambio en la interfaz web, el modelo corre el riesgo de reinventar el diseno, cambiar los colores institucionales, agregar emojis no deseados, alterar el tamano de los 219 rectangulos de estacionamiento o arruinar la formula matematica que mantiene el zoom perfectamente centrado en las coordenadas (92, 91).

La skill `AGENTS.md` funciona como una constitucion inviolable. Contiene los valores exactos en milimetros de las plazas (3.300 por 7.800 mm con radio de esquina de 0.550 mm), el angulo de giro de cada una de las doce hileras, los codigos hexadecimales exactos de las tres tonalidades de verde de los arboles (#196F3D, #27AE60, #1ABC9C), y la prohibicion estricta de utilizar emojis.

### 4.2 Instrucciones para Invocar la Skill con un Agente
Cada vez que pidas a un agente de inteligencia artificial que cree un nuevo componente, conecte un endpoint o refactorice codigo en la web, debes incluir en tu instruccion una referencia explicita a la skill:

```text
Lee atentamente el archivo AGENTS.md antes de escribir cualquier codigo. Asegurate de respetar al pie de la letra todos los tokens de diseno, la regla de cero emojis, las dimensiones exactas de los rectangulos SVG y el orden de renderizado especificado en la skill.
```

Al hacer esto, el agente asume el contexto formal del sistema y generara codigo que encaja de forma perfecta con lo existente, sin degradar el trabajo previo ni introducir estilos discordantes.

---

## 5. Como Aplicar Spec-Driven Development (Desarrollo Guiado por Especificaciones)

El Desarrollo Guiado por Especificaciones (Spec-Driven Development) es una metodologia de ingenieria de software que transforma radicalmente la forma de trabajar con asistentes de programacion automatizada.

### 5.1 La Analogia de la Construccion Civil
Imagina que deseas construir un puente. Un metodo caotico seria llamar a los obreros y decirles: "Empiecen a poner concreto y hierro hasta que parezca un puente que aguante camiones". Lo mas probable es que la estructura colapse o termine costando el triple.

El metodo de ingenieria civil consiste en redactar primero los planos y las especificaciones tecnicas: resistencia de materiales, calculo de cargas, tolerancias de viento y dimensiones exactas. Cuando los planos estan aprobados y firmados, los constructores simplemente ejecutan las instrucciones al milimetro.

En el desarrollo de software asistido por agentes, el codigo es el concreto y la especificacion escrita es el plano arquitectonico. Si intentas pedirle a un agente que programe directamente sin una especificacion previa, el agente tendra que adivinar detalles criticos y el resultado requerira muchas correcciones.

### 5.2 El Flujo de Trabajo en Cuatro Pasos

#### Paso 1: Redactar la Especificacion Funcional
Antes de tocar el codigo, crea un archivo Markdown dentro de la carpeta de especificaciones del proyecto (por ejemplo, `__specs__/web_polling_integration.md`). En ese documento defines con absoluta claridad:
* Que problema resuelve la funcionalidad.
* Cuales endpoints exactos del backend va a consultar y con que frecuencia.
* Cual es la estructura JSON esperada y como se manejan los errores de conexion.
* Que elementos del DOM o del SVG se veran modificados visualmente cuando cambie el estado.
* Criterios de aceptacion claros y medibles (por ejemplo: "Si la plaza L1-A01 pasa a occupied en el JSON, su rectangulo debe adquirir la clase occupied y el color rojo en menos de 500 milisegundos").

#### Paso 2: Revision de la Especificacion
Lee tu propio documento como si fueras un observador externo. Si encuentras alguna ambiguedad (por ejemplo, "¿que debe mostrar la pantalla si el servidor se apaga?"), resuelvela en el texto antes de programar.

#### Paso 3: Ejecucion Asistida por el Agente
Proporciona al agente tanto la especificacion redactada como la skill `AGENTS.md`. Tu peticion sera tan precisa como:
"Implementa la funcionalidad descrita en `__specs__/web_polling_integration.md`, respetando todas las directivas de diseno y restricciones tecnicas de `AGENTS.md`".

#### Paso 4: Verificacion y Pruebas
Dado que la especificacion contiene criterios de aceptacion explicitos, puedes verificar de inmediato si la solucion funciona como se previo, ejecutando pruebas automaticas o comprobando los escenarios descritos en la guia.

---

## 6. Mini Tutorial de Node.js y Herramientas de Desarrollo con Vite

Para trabajar comodamente en el frontend moderno sin complicaciones innecesarias, utilizaremos Node.js como entorno de ejecucion en tu computadora y Vite como servidor de desarrollo ultrarrapido.

### 6.1 Que son Node.js y npm
Node.js es un motor que permite ejecutar codigo JavaScript directamente en tu sistema operativo, fuera del navegador web. Es el motor que impulsa las herramientas de construccion, empaquetado y servidores locales.

npm (Node Package Manager) es el gestor de paquetes que viene instalado junto con Node.js. Funciona como una gran biblioteca donde puedes descargar herramientas creadas por otros desarrolladores mediante simples comandos en tu terminal.

Para comprobar si tienes instaladas estas herramientas en tu maquina, abre una terminal y escribe:

```bash
node -v
npm -v
```

Si ambos comandos te devuelven un numero de version (por ejemplo `v20.x.x` y `10.x.x`), tu entorno esta listo.

### 6.2 Por que Vite
En los origenes del desarrollo web, cada vez que hacias un cambio en el codigo debias cambiar de ventana y presionar recargar manualmente en el navegador. Vite resuelve esto mediante un mecanismo llamado Hot Module Replacement: levanta un servidor web local en milesimas de segundo y, cada vez que guardas un cambio en tu editor de codigo, el navegador se actualiza instantaneamente en tiempo real sin perder el estado de lo que estabas haciendo.

### 6.3 Puesta en Marcha Paso a Paso

#### Paso 1: Ingresar a la carpeta de la aplicacion
Abre tu terminal en la raiz del proyecto y desplázate al directorio del frontend:

```bash
cd app
```

#### Paso 2: Inicializar el proyecto con Vite
Si deseas crear la configuracion basica de Vite con JavaScript puro desde cero dentro de la carpeta, puedes ejecutar:

```bash
npm create vite@latest . -- --template vanilla
```

Este comando descargara la plantilla minima y configurara los archivos base necesarios.

#### Paso 3: Instalar dependencias
Para asegurarte de que todas las librerias declaradas esten disponibles localmente, ejecuta:

```bash
npm install
```

#### Paso 4: Iniciar el servidor local de desarrollo
Para encender el servidor y empezar a trabajar, escribe:

```bash
npm run dev
```

En la consola veras un mensaje similar al siguiente:

```text
  VITE v5.x.x  ready in 120 ms

  ➜  Local:   http://localhost:5173/
  ➜  Network: use --host to expose
  ➜  press h + enter to show help
```

Abre tu navegador en la direccion `http://localhost:5173`. A partir de ese momento, cualquier modificacion que guardes en los archivos se reflejara en pantalla de forma automatica e instantanea.

#### Paso 5: Compilar para produccion
Cuando el desarrollo este completado y listo para ser desplegado en un servidor web definitivo (como Nginx o Apache), ejecuta:

```bash
npm run build
```

Vite empaquetara, minificara y optimizara todo el codigo en una carpeta llamada `dist`, generando archivos sumamente livianos que cargan a maxima velocidad para los usuarios finales.

---

## 7. Recursos de Aprendizaje Recomendados en YouTube

Para profundizar en los conceptos tecnicos mencionados y adquirir soltura en el flujo de trabajo moderno, se recomienda consultar los siguientes temas y contenidos audiovisuales:

### 7.1 Vite y Fundamentos de Desarrollo Web Moderno
* **Buscar en YouTube**: *"Vite JS tutorial desde cero"* o *"Aprende Vite en 15 minutos"*.
  * Conceptos clave a observar: Como funciona el servidor de desarrollo, como se importan modulos y como se estructura la carpeta `public` para archivos estaticos.
* **Canales recomendados en espanol**: Midudev, Fazt Code, ManzDev.

### 7.2 Consumo de APIs REST con Fetch y JavaScript Asincrono
* **Buscar en YouTube**: *"Como consumir una API con Fetch en JavaScript"* o *"Async Await y Promesas en JavaScript explicacion sencilla"*.
  * Conceptos clave a observar: Como enviar una peticion GET, como convertir la respuesta binaria a formato JSON con `.json()`, como atrapar errores de conexion con `try / catch` y como actualizar el DOM en respuesta a los datos recibidos.

### 7.3 Manipulacion Dinamica de SVG con JavaScript
* **Buscar en YouTube**: *"Manipular SVG con JavaScript tutorial"* o *"Interactive SVG map JavaScript"*.
  * Conceptos clave a observar: Como seleccionar elementos dentro de un `<svg>` con `document.querySelector`, como alternar clases CSS (`classList.toggle`) y como escuchar eventos de clic sobre poligonos y rectangulos vectoriales.

### 7.4 Metodologia Spec-Driven Development e Ingenieria con Asistentes AI
* **Buscar en YouTube**: *"Spec-Driven Development AI coding"* o *"Como programar con agentes de inteligencia artificial buenas practicas"*.
  * Conceptos clave a observar: La importancia de escribir requerimientos detallados y contratos de API antes de solicitar codigo a un modelo generativo, evitando alucinaciones y garantizando consistencia arquitectonica.
