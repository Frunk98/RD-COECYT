#include <mcp_can.h>             // Biblioteca local para comunicación CAN
#include <GP94.h>                // Biblioteca local para la IMU GP94 
#include <SPI.h>                 // Biblioteca para la comunicación SPI
#include <SD.h>                  // Biblioteca para tarjetas SD
#include <WiFi.h>                // Biblioteca para WiFi
#include <PubSubClient.h>        // Biblioteca para MQTT
#include <Adafruit_GFX.h>        // Biblioteca para gráficos de Adafruit
#include <Adafruit_SSD1306.h>    // Biblioteca para la pantalla OLED SSD1306

#define CAN0_INT 33                   // Pin de interrupción CAN
#define RXD2 16                       // RX Serial 1
#define TXD2 17                       // TX Serial 1
#define SCREEN_WIDTH 128              // Ancho de la pantalla
#define SCREEN_HEIGHT 64              // Alto de la pantalla
#define OLED_RESET    -1              // Pin de reset
#define SCREEN_ADDRESS 0x3C           // Dirección I2C para la pantalla
#define CHIP_SELECT 5                 // ChipSelect de la tarjeta SD
MCP_CAN CAN0(4);                      // ChipSelect del MCP2515

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Inicializa la pantalla OLED
HardwareSerial mySerial(2);                                               // Inicializa un segundo puerto serial
GP9 imu(mySerial);                                                        // Inicializa el objeto imu para la comunicación Serial con la GP9
WiFiClient espClient;                                                     // Cliente WiFi
PubSubClient client(espClient);                                           // Cliente MQTT

// Configuración de la red WiFi
const char* ssid = "seifa";           // Nombre de la red
const char* password = "91045843";    // Contraseña de la red

// Configuración del broker MQTT
const char* mqtt_server = "192.168.0.100";    // Dirección del servidor MQTT
const int mqtt_port = 1883;                   // Puerto del servidor MQTT
const char* mqtt_user = "cinvestav";          // Usuario MQTT
const char* mqtt_password = "robotino";       // Contraseña MQTT

// Variable para almacenar el timestamp
String currentTimestamp = "";

// Variables CAN
long unsigned int rxId;                                               // ID del mensaje recibido en el bus CAN
float rpm, pvel, gas;                                                 // Variables para almacenar los datos de OBDII
unsigned char len = 0;                                                // Longitud del mensaje recibido
unsigned char rxBuf[8];                                               // Buffer para almacenar los datos recibidos
byte D_sol[8] = { 0x02, 0x01, 0x00, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };   // Array del mensaje CAN
int S_rxId;                                                           // Variable para almacenar el ID del mensaje CAN

void setup() {
  // Inicialización del puerto serie para la comunicación con el ordenador
  Serial.begin(115200);
  
  // Inicialización del segundo puerto serie para la comunicación con la IMU
  mySerial.begin(115200, SERIAL_8N1, RXD2, TXD2);  
  
  // Inicialización de la pantalla OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Error de inicialización de SSD1306"));
    for (;;); 
  }
  display.display();
  delay(2000); 
  display.clearDisplay();
  
  // Configuración de la conexión WiFi
  setup_wifi();
  
  // Configuración del servidor MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); // Configurar la función de callback para suscribirse a topicos del MQTT
  
  // Inicialización del bus CAN
  while (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK) {
    display.setTextColor(SSD1306_WHITE); 
    display.println("Error al inicializar MCP2515");
    display.display();
    delay(5000); 
  }
  display.setTextColor(SSD1306_WHITE); 
  display.println("MCP2515 Inicializada correctamente");
  display.display();
  delay(2000); 
  CAN0.setMode(MCP_NORMAL);
  pinMode(CAN0_INT, INPUT);
  
  // Inicialización de la tarjeta SD
  if (!SD.begin(CHIP_SELECT)) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextColor(SSD1306_WHITE); 
    display.print("Error al inicializar la tarjeta SD");
    display.display();
    delay(2000); //
    return;
  }
  display.setTextColor(SSD1306_WHITE);
  display.print("Tarjeta SD inicializada");
  display.display();
  delay(2000);
  reconnect();
}

// Función de callback para recibir el timestamp
boolean timestampReceived = false; // Bandera para verificar si el timestamp ya fue recibido
void callback(char* topic, byte* payload, unsigned int length) {
  currentTimestamp = "";
  for (int i = 0; i < length; i++) {
    currentTimestamp += (char)payload[i];
  }
  if (!timestampReceived) { // Solo imprime el mensaje si el timestamp no ha sido recibido
    display.setTextColor(SSD1306_WHITE);
    display.println("Timestamp recibido: ");
    display.println(currentTimestamp);
    display.display();
    delay(2000); 
    timestampReceived = true; // Timestamprecibido
  }
}

// Configuración de la conexión WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.print("Conectando a ");
  display.print(ssid);
  display.display();
  delay(2000);

  WiFi.begin(ssid, password); // Inicia la conexión WiFi

  while (WiFi.status() != WL_CONNECTED) { // Espera hasta que la conexión se establezca
    delay(500);
    display.println(".");
    display.display();
    delay(2000);
  }
  display.println("WiFi conectado");
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.display();
  delay(2000);
}

// Función para reintentar conexión al broker MQTT si la conexión se pierde
void reconnect() {
  while (!client.connected()) {
    display.setCursor(0, 0);
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.println("Conectando al broker");
    display.print("Server: "); display.println(mqtt_server);
    display.print("Puerto: "); display.println(mqtt_port);
    display.display();
    delay(2000);

    // Intentar conectarse al broker MQTT
    if (client.connect(String("ESP32Client-" + String(ESP.getEfuseMac())).c_str(), mqtt_user, mqtt_password)) {
      client.subscribe("timestamp/update"); // Suscribir al tópico de timestamp

      // Esperar a que el timestamp se reciba antes de continuar
      while (!timestampReceived) {
        client.loop();
        delay(100);
      }
      display.setTextColor(SSD1306_WHITE);
      display.println("Conectado al broker MQTT");
      display.display();
      delay(2000);
    } else {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.print("Error, rc=");
      display.print(client.state()); // Muestra el código de error de la conexión
      display.println(" Intentando de nuevo en 5 segundos");
      display.display();
      delay(5000);
    }
  }
}

// Función para enviar datos al broker MQTT
void sendMqtt(String message) {
  if (!client.connected()) { // Verifica si el cliente MQTT está conectado
    reconnect(); // Si no está conectado, intenta reconectar
  }
  client.publish("obd/data", message.c_str()); // Publica el mensaje en el topic "obd/data"
}

// Función para leer mensajes CAN (Se pueden agregar más, modificando el ID rxBuf[2] == 0x"XX")
void leer() {
  if (!digitalRead(CAN0_INT)) { // Verifica si hay una interrupción del bus CAN
    if (CAN0.readMsgBuf(&rxId, &len, rxBuf) == CAN_OK) { // Lee el mensaje del bus CAN
      if (rxId == 0x7E8) { 
        switch (rxBuf[2]) { 
          case 0x0C:
            rpm = (0.25 * ((rxBuf[3] * 256) + rxBuf[4])); // Calcula RPM 
            break;
          case 0x0D:
            pvel = rxBuf[3]; // Calcula velocidad (km/h)
            break;
          case 0x2F:
            gas = (1.0 / 2.55) * (rxBuf[3]); // Calcula el nivel del tanque de gasolina (litros)
            break;
        }
      }
    }
  }
}

// Función para enviar mensajes por CAN
void enviar() {
  D_sol[2] = S_rxId; // Asigna el ID de mensaje a enviar 
  if (CAN0.sendMsgBuf(0x7DF, 0, 8, D_sol) != CAN_OK) { // Envía el mensaje por el bus CAN
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextColor(SSD1306_WHITE); 
    display.println("Error al enviar el mensaje"); 
    display.display();
    delay(2000);
  }
  leer();
}

// Función para imprimir datos en microSD
void printDataSD(){
  if (imu.decode(mySerial.read())) {
    File dataFile = SD.open("/data.txt", FILE_APPEND);
    if (dataFile) {

      // Verifica si el timestamp está vacío
      if (currentTimestamp == "") {
        Serial.println("Error: El timestamp está vacío.");
      }

      // Imprimir timestamp
      dataFile.print(currentTimestamp); dataFile.print(" ");

      // Imprimir datos OBDII
      S_rxId = 0x0C; // ID de RPM
      enviar();
      dataFile.print(rpm); dataFile.print(",");
      S_rxId = 0x0D; // ID de velocidad
      enviar();
      dataFile.print(pvel); dataFile.print(",");
      S_rxId = 0x2F; // ID de nivel de combustible
      enviar();
      dataFile.print(gas); dataFile.print(",");

      // Imprimir datos IMU
      dataFile.print(imu.gyro_raw_x); dataFile.print(",");
      dataFile.print(imu.gyro_raw_y); dataFile.print(",");
      dataFile.print(imu.gyro_raw_z); dataFile.print(",");
      dataFile.print(imu.accel_raw_x); dataFile.print(",");
      dataFile.print(imu.accel_raw_y); dataFile.print(",");
      dataFile.print(imu.accel_raw_z); dataFile.print(",");
      dataFile.print(imu.gyro_x); dataFile.print(",");
      dataFile.print(imu.gyro_y); dataFile.print(",");
      dataFile.print(imu.gyro_z); dataFile.print(",");
      dataFile.print(imu.accel_x); dataFile.print(",");
      dataFile.print(imu.accel_y); dataFile.print(",");
      dataFile.print(imu.accel_z); dataFile.print(",");
      dataFile.print(imu.quat_a / 29789.09091); dataFile.print(",");
      dataFile.print(imu.quat_b / 29789.09091); dataFile.print(",");
      dataFile.print(imu.quat_c / 29789.09091); dataFile.print(",");
      dataFile.print(imu.quat_d / 29789.09091); dataFile.print(",");
      dataFile.print(imu.roll); dataFile.print(",");
      dataFile.print(imu.pitch); dataFile.print(",");
      dataFile.print(imu.yaw); dataFile.print(",");
      dataFile.print(imu.lattitude, 6); dataFile.print(",");
      dataFile.print(imu.longitude, 6); dataFile.print(",");
      dataFile.println(imu.altitude, 6);
      dataFile.close();
    } else {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.println("Error al abrir el archivo de datos");
      display.display();
      delay(2000);
    }
  }
}

//Función para enviar datos por MQTT
void sendData() {
  if (imu.decode(mySerial.read())) {
    // Verifica si el timestamp está vacío
    if (currentTimestamp == "") {
      Serial.println("Error: El timestamp está vacío.");
    }
    // Imprir timestamp e iniciar payload
    String message = currentTimestamp + " ";
    // Imprimir datos OBDII
    S_rxId = 0x0C; // ID de RPM
    enviar();
    message += String(rpm) + ",";
    S_rxId = 0x0D; // ID de velocidad
    enviar();
    message += String(pvel) + ",";
    S_rxId = 0x2F; // ID de nivel de combustible
    enviar();
    message += String(gas) + ",";
    // Imprimir datos IMU
    message += String(imu.gyro_raw_x) + ",";
    message += String(imu.gyro_raw_y) + ",";
    message += String(imu.gyro_raw_z) + ",";
    message += String(imu.accel_raw_x) + ",";
    message += String(imu.accel_raw_y) + ",";
    message += String(imu.accel_raw_z) + ",";
    message += String(imu.gyro_x) + ",";
    message += String(imu.gyro_y) + ",";
    message += String(imu.gyro_z) + ",";
    message += String(imu.accel_x) + ",";
    message += String(imu.accel_y) + ",";
    message += String(imu.accel_z) + ",";
    message += String(imu.quat_a / 29789.09091) + ",";
    message += String(imu.quat_b / 29789.09091) + ",";
    message += String(imu.quat_c / 29789.09091) + ",";
    message += String(imu.quat_d / 29789.09091) + ",";
    message += String(imu.roll) + ",";
    message += String(imu.pitch) + ",";
    message += String(imu.yaw) + ",";
    message += String(imu.lattitude, 6) + ",";
    message += String(imu.longitude, 6) + ",";
    message += String(imu.altitude, 6);
    //Serial.println(message); //Imprimir el payload enviado por MQTT en serial para debug
    sendMqtt(message);
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  printDataSD();
  sendData();
}
