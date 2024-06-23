# Sistema de Evaluación Inteligente de Factores que Afectan las Competencias de Conducción Vehicular para Policías en Formación

## Descripción

Este proyecto tiene como objetivo desarrollar un sistema de evaluación objetiva y subjetiva que mida, valore e infiera las competencias de manejo de los Cadetes de la Academia de Policía de Saltillo (AdP-S). El sistema está diseñado para proporcionar una evaluación integral de las habilidades de conducción vehicular, utilizando una combinación de datos objetivos y subjetivos para ofrecer una valoración precisa y útil para los instructores y cadetes.

## Propuesta

Integrar toda la información en algoritmos sustentados en técnicas de Inteligencia Artificial (IA) para resolver una función de costo a la medida de los diversos criterios y métricas aplicables, para evaluar las competencias de manejo y atención vial de los cadetes de la AdP-S.

## Módulo de Evaluación de Habilidades de Conducción (MEHC)

El MEHC evalúa las habilidades de conducción del cadete en la realización de circuitos, pruebas de manejo y situaciones policiales.

### Métricas:

- Parámetros del vehículo: posición, velocidad y aceleración.
- Manipulación del volante: dinámica angular.

### Elementos Principales:

- Centrales inerciales.
- Comunicación OBD-II.
- Algoritmo de IA para las habilidades de conducción del cadete.

## Programas Principales

### Programa para el módulo OBDII
[OBD_SD_MQTT_LCD_final](https://github.com/Frunk98/RD-COECYT/tree/main/Programas/OBD_SD_MQTT_LCD_final)

Este programa incluye varias bibliotecas esenciales para su funcionamiento:

- `mcp_can.h`: Biblioteca local para la comunicación CAN (Controller Area Network), descargable desde [este enlace](https://github.com/Frunk98/RD-COECYT/blob/main/MCP_CAN_lib-master.zip).
- `GP94.h`: Biblioteca local para la Unidad de Medición Inercial (IMU) GP94, descargable desde [este enlace](https://github.com/Frunk98/RD-COECYT/blob/main/GP9-modificados.zip). Esta biblioteca está modificada específicamente para este proyecto. Para agregar más funciones de la IMU, se pueden descargar las bibliotecas originales desde [aquí](https://github.com/Frunk98/RD-COECYT/blob/main/GP9-original.zip).
- `SPI.h`: Biblioteca estándar para la comunicación SPI (Serial Peripheral Interface).
- `SD.h`: Biblioteca estándar para la gestión de tarjetas SD.
- `WiFi.h`: Biblioteca para la conexión WiFi.
- `PubSubClient.h`: Biblioteca para la comunicación MQTT (Message Queuing Telemetry Transport).
- `Adafruit_GFX.h`: Biblioteca para gráficos de Adafruit.
- `Adafruit_SSD1306.h`: Biblioteca para la pantalla OLED SSD1306 de Adafruit.

### Inicialización y Configuración

- `Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);`: Inicializa la pantalla OLED.
- `HardwareSerial mySerial(2);`: Inicializa un segundo puerto serial.
- `GP9 imu(mySerial);`: Inicializa el objeto imu para la comunicación Serial con la GP9.
- `WiFiClient espClient;`: Cliente WiFi.
- `PubSubClient client(espClient);`: Cliente MQTT.

#### Configuración de la red WiFi

- `const char* ssid = "";`: Nombre de la red.
- `const char* password = "";`: Contraseña de la red.

#### Configuración del broker MQTT

- `const char* mqtt_server = "";`: Dirección del servidor MQTT.
- `const int mqtt_port = 1883;`: Puerto del servidor MQTT.
- `const char* mqtt_user = "";`: Usuario MQTT.
- `const char* mqtt_password = "";`: Contraseña MQTT.

### Variables

- `String currentTimestamp = "";`: Variable para almacenar el timestamp.
- Variables CAN:
  - `long unsigned int rxId;`: ID del mensaje recibido en el bus CAN.
  - `float rpm, pvel, gas;`: Variables para almacenar los datos de OBDII.
  - `unsigned char len = 0;`: Longitud del mensaje recibido.
  - `unsigned char rxBuf[8];`: Buffer para almacenar los datos recibidos.
  - `byte D_sol[8] = { 0x02, 0x01, 0x00, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };`: Array del mensaje CAN.
  - `int S_rxId;`: Variable para almacenar el ID del mensaje CAN.
 

### [VOLANTE_SD_MQTT_LCD_final](https://github.com/Frunk98/RD-COECYT/tree/main/Programas/VOLANTE_SD_MQTT_LCD_final)
