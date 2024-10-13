#include <mcp_can.h>           // Biblioteca para la comunicación CAN 
#include <GP94.h>              // Biblioteca local para la IMU
#include <SPI.h>               // Biblioteca para la comunicación SPI
#include <SD.h>                // Biblioteca para el módulo de memoria MicroSD
#include <WiFi.h>              // Biblioteca para la comunicación WiFi
#include <PubSubClient.h>      // Biblioteca para la comunicación MQTT
#include <Adafruit_GFX.h>      // Biblioteca para gráficos de Adafruit
#include <Adafruit_SSD1306.h>  // Biblioteca para la pantalla OLED SSD1306

// Pin de interrupción del módulo de comunicación CAN
#define CAN0_INT 33           
// Pines de comunicación Serial para la IMU
#define RXD2 16                
#define TXD2 17       
//Medidas de la pantalla OLED
#define SCREEN_WIDTH 128        
#define SCREEN_HEIGHT 64  
// Reset de la pantalla OLED (No utilizado)
#define OLED_RESET -1     
// DIrección I2C de la pantalla OLED
#define SCREEN_ADDRESS 0x3C    
// Chip Select de la memoria MicroSD
#define CHIP_SELECT 5    
// Chip select del módulo de comunicación CAN
MCP_CAN CAN0(4);                

// Declaraciones de perifericos
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  // Pantalla OLED
HardwareSerial mySerial(2);  // Puerto Serial para la IMU
GP9 imu(mySerial);  // Objeto IMU 
WiFiClient espClient;  // Cliente WiFi  
PubSubClient client(espClient);  //Cliente MQTT

// Constantes para el protocolo MQTT
const char* ssid = "";          // Nombre de la red
const char* password = "";      // Contraseña de la red
const char* mqtt_server = "";   // Dirección del servidor MQTT
const int mqtt_port = 1833;     // Puerto del servidor MQTT
const char* mqtt_user = "";     // Usuario MQTT
const char* mqtt_password = ""; // Contraseña MQTT

// Constantes del módulo de comunicación CAN
long unsigned int rxId;
float vel, rpm, acel, chrg, tmp, gas, adj, tiempo;
unsigned char len = 0;
unsigned char rxBuf[8];
byte D_sol[8] = { 0x02, 0x01, 0x00, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };
int S_rxId;

void setup() {
  //Inicialización del puerto serial para la IMU
  mySerial.begin(115200, SERIAL_8N1, RXD2, TXD2);
  //Inicialización de la pantalla OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    while (true);
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  //Configuración de WiFi y MQTT
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  //Inicialización del módulo de comunicación CAN MCP2515
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
  //Inicialización de la memoria MicroSD
  if (!SD.begin(CHIP_SELECT)) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextColor(SSD1306_WHITE);
    display.print("Error al inicializar la tarjeta SD");
    display.display();
    delay(2000);
    return;
  }
  display.setTextColor(SSD1306_WHITE);
  display.print("Tarjeta SD inicializada");
  display.display();
  delay(2000);
  //Reconexión en caso de perder la comunicación MQTT
  reconnect();
}
//Configuración de WiFi
void setup_wifi() {
  delay(10);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.print("Conectando a ");
  display.print(ssid);
  display.display();
  delay(2000);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    display.print(".");
    display.display();
    delay(2000);
  }
  display.println("WiFi conectado");
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.display();
  delay(2000);
}
//Reconexión con el broker MQTT
void reconnect() {
  while (!client.connected()) {
    display.setCursor(0, 0);
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.println("Conectando al broker");
    display.print("Server: ");
    display.println(mqtt_server);
    display.print("Puerto: ");
    display.println(mqtt_port);
    display.display();
    delay(2000);
    Serial.println("Intentando conectar al broker MQTT...");
    if (client.connect(String("MEHC_OBD").c_str(), mqtt_user, mqtt_password)) {
      Serial.println("Conectado al broker MQTT");
      display.setTextColor(SSD1306_WHITE);
      display.println("Conectado al broker MQTT");
      display.display();
      delay(2000);
    } else {
      Serial.print("Error de conexión, rc=");
      Serial.print(client.state());
      Serial.println(" Intentando de nuevo en 5 segundos");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.print("Error, rc=");
      display.print(client.state());
      display.println(" Intentando de nuevo en 5 segundos");
      display.display();
      delay(5000);
    }
  }
}
//Función para enviar datos por MQTT
void sendMqtt(String message) {
  if (!client.connected()) {
    reconnect();
  }
  client.publish("MEHC_OBD/data", message.c_str());
}
//Función para leer y convertir los mensajes CAN
void leer() {
  if (!digitalRead(CAN0_INT)) {
    if (CAN0.readMsgBuf(&rxId, &len, rxBuf) == CAN_OK) {
      if (rxId == 0x7E8) {
        switch (rxBuf[2]) {
          case 0x0D:
            vel = rxBuf[3];
            break;
          case 0x0C:
            rpm = 0.25 * ((rxBuf[3] * 256.0) + rxBuf[4]);
            break;
          case 0x49:
            acel = rxBuf[3] / 2.55;
            break;
          case 0x04:
            chrg = rxBuf[3] / 2.55;
            break;
          case 0x05:
            tmp = rxBuf[3] - 40.0;
            break;
          case 0x0E:
            tiempo = (rxBuf[3] / 2.0) - 64.0;
            break;
          case 0x14:
            adj = (rxBuf[4] / 1.28) - 100.0;
            break;
          case 0x2F:
            gas = (1.0 / 2.55) * rxBuf[3];
            break;
        }
      }
    }
  }
}
//Función para enviar los mensajes CAN
void enviar() {
  D_sol[2] = S_rxId;
  if (CAN0.sendMsgBuf(0x7DF, 0, 8, D_sol) != CAN_OK) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextColor(SSD1306_WHITE);
    display.println("Error al enviar el mensaje");
    display.display();
    delay(2000);
  }
  leer();
}
//Función para guardar los datos en la memoria MicroSD
void printDataSD() {
  if (imu.decode(mySerial.read())) {  // Función que decodifica los datos de la IMU
  File dataFile = SD.open("/obd2.svd", FILE_APPEND);  // Crear e inicializar un SVD en la memoria MicroSD
  if (dataFile) {
    // Datos CAN (OBDII)
    S_rxId = 0x0D;
    enviar();
    dataFile.print(vel,6);
    dataFile.print(",");
    S_rxId = 0x0C;
    enviar();
    dataFile.print(rpm,6);
    dataFile.print(",");
    S_rxId = 0x49;
    enviar();
    dataFile.print(acel,6);
    dataFile.print(",");
    S_rxId = 0x04;
    enviar();
    dataFile.print(chrg,6);
    dataFile.print(",");
    S_rxId = 0x05;
    enviar();
    dataFile.print(tmp,6);
    dataFile.print(",");
    S_rxId = 0x0E;
    enviar();
    dataFile.print(tiempo,6);
    dataFile.print(",");
    S_rxId = 0x14;
    enviar();
    dataFile.print(adj,6);
    dataFile.print(",");
    S_rxId = 0x2F;
    enviar();
    dataFile.print(gas,6);
    dataFile.print(",");
    //Datos IMU
    dataFile.print(imu.accel_y, 6);
    dataFile.print(",");
    dataFile.print(imu.accel_z, 6);
    dataFile.print(",");
    dataFile.print(",");
    dataFile.print(imu.gyro_y, 6);
    dataFile.print(",");
    dataFile.print(imu.gyro_z, 6);
    dataFile.print(",");
    dataFile.print(imu.lattitude, 6);
    dataFile.print(",");
    dataFile.print(imu.longitude, 6);
    dataFile.print(",");
    dataFile.print(imu.altitude, 6);
    dataFile.print(",");
    dataFile.println(imu.speed, 6);
    dataFile.println();
    dataFile.close();
    } else {  //En caso de que no
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.println("Error al abrir el archivo de datos");
      display.display();
      delay(2000);
    }
  }
}
//Función para enviar los datos por MQTT
void sendData() {
  if (imu.decode(mySerial.read())) {
    String message = "";
    S_rxId = 0x0D;
    enviar();
    message += String(vel,6) + ",";
    S_rxId = 0x0C;
    enviar();
    message += String(rpm,6) + ",";
    S_rxId = 0x49;
    enviar();
    message += String(acel,6) + ",";
    S_rxId = 0x04;
    enviar();
    message += String(chrg,6) + ",";
    S_rxId = 0x05;
    enviar();
    message += String(tmp,6) + ",";
    S_rxId = 0x0E;
    enviar();
    message += String(tiempo,6) + ",";
    S_rxId = 0x14;
    enviar();
    message += String(adj,6) + ",";
    S_rxId = 0x2F;
    enviar();
    message += String(gas,6) + ",";
    message += String(imu.accel_y, 6) + ",";
    message += String(imu.accel_z, 6) + ",";
    message += String(imu.gyro_y, 6) + ",";
    message += String(imu.gyro_z, 6) + ",";
    message += String(imu.lattitude, 6) + ",";
    message += String(imu.longitude, 6) + ",";
    message += String(imu.altitude, 6) + ",";
    message += String(imu.speed, 6)  + "\n";
    sendMqtt(message);
  }
}
// Bucle para enviar y guardar los datos
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  printDataSD();
  sendData();
}
