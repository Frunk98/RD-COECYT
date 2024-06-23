# Sistema de Evaluación Inteligente de Factores que Afectan las Competencias de Conducción Vehicular para Policías en Formación

![MEHC](https://github.com/Frunk98/RD-COECYT/blob/main/Imagenes/MEHC.jpg)

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


### [VOLANTE_SD_MQTT_LCD_final](https://github.com/Frunk98/RD-COECYT/tree/main/Programas/VOLANTE_SD_MQTT_LCD_final)
