#include <GP94.h>                // Biblioteca local para la IMU 
#include <SPI.h>                 // Biblioteca para la comunicación SPI
#include <SD.h>                  // Biblioteca para el módulo de memoria MicroSD
#include <WiFi.h>                // Biblioteca para la comunicación WiFi
#include <PubSubClient.h>        // Biblioteca para la comunicación MQTT
#include <Adafruit_GFX.h>        // Biblioteca para gráficos de Adafruit
#include <Adafruit_SSD1306.h>    // Biblioteca para la pantalla OLED SSD1306

// Pines de comunicación Serial para la IMU
#define RXD2 16                        
#define TXD2 17  
// Chip Select de la memoria MicroSD
#define CHIP_SELECT 5                  
//Medidas de la pantalla OLED
#define SCREEN_WIDTH 128        
#define SCREEN_HEIGHT 64  
// Reset de la pantalla OLED (No utilizado)
#define OLED_RESET -1     
// DIrección I2C de la pantalla OLED
#define SCREEN_ADDRESS 0x3C    

//Declaraciones de perifericos
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


//Constantes MUX
const int selectPin0 = 33;    // Pin de selección S0
const int selectPin1 = 25;    // Pin de selección S1
const int selectPin2 = 26;    // Pin de selección S2
const int zInput = 32;        // Pin de entrada analógica Z


void setup() {
  // Inicialización del puerto serie para depuración
  Serial.begin(115200);
  /Inicialización del puerto serial para la IMU
  mySerial.begin(115200, SERIAL_8N1, RXD2, TXD2);  
  // Inicialización de los pines de selección del MUX
  pinMode(selectPin0, OUTPUT);
  pinMode(selectPin1, OUTPUT);
  pinMode(selectPin2, OUTPUT);
  pinMode(zInput, INPUT);
  // Inicialización de la pantalla OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    while (true);
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  //Configuración de WiFi y MQTT
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
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
  Serial.println();
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
    if (client.connect(String("MEHC_VOLANTE").c_str(), mqtt_user, mqtt_password)) {
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

// Función para enviar datos por MQTT
void sendMqtt(String message) {
  if (!client.connected()) { // Verifica si el cliente MQTT está conectado
    reconnect(); // Si no está conectado, intenta reconectar
  }
  client.publish("MEHC_VOLANTE/data", message.c_str()); // Publica el mensaje en el topic "MEHC_VOLANTE/data"
}

// Función para imprimir datos en microSD
void printDataSD() {
  if (imu.decode(mySerial.read())) {  // Función que decodifica los datos de la IMU
    File dataFile = SD.open("/volante.svd", FILE_APPEND);  // Crear e inicializar un SVD en la memoria MicroSD
    if (dataFile) {
      // FSR Y0
      digitalWrite(selectPin0, LOW);
      digitalWrite(selectPin1, LOW);
      digitalWrite(selectPin2, LOW);
      int pot1ADC = analogRead(zInput);
      // FSR Y1
      digitalWrite(selectPin0, HIGH);
      digitalWrite(selectPin1, LOW);
      digitalWrite(selectPin2, LOW);
      int pot2ADC = analogRead(zInput);
      // FSR Y2
      digitalWrite(selectPin0, LOW);
      digitalWrite(selectPin1, HIGH);
      digitalWrite(selectPin2, LOW);
      int pot3ADC = analogRead(zInput);
      // FSR Y3
      digitalWrite(selectPin0, HIGH);
      digitalWrite(selectPin1, HIGH);
      digitalWrite(selectPin2, LOW);
      int pot4ADC = analogRead(zInput);
      // Datos MUX
      dataFile.print(pot1ADC);
      dataFile.print(",");
      dataFile.print(pot2ADC);
      dataFile.print(",");
      dataFile.print(pot3ADC);
      dataFile.print(",");
      dataFile.print(pot4ADC);
      dataFile.print(",");
      // Datos IMU
       dataFile.print(imu.gyro_x, 6);
       dataFile.print(",");
       dataFile.print(imu.accel_x, 6);
       dataFile.print(",");
       dataFile.print(imu.quat_a / 29789.09091, 6);
       dataFile.print(",");
       dataFile.print(imu.quat_b / 29789.09091, 6);
       dataFile.print(",");
       dataFile.print(imu.quat_c / 29789.09091, 6);
       dataFile.print(",");
       dataFile.print(imu.quat_d / 29789.09091, 6);
       dataFile.println();
      dataFile.close();
    } else {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.print("Error al abrir el archivo de datos");
      display.display();
      delay(2000);
    }
  }
}

// Función para enviar datos por MQTT
void sendData() {
  if (imu.decode(mySerial.read())) {
    String message = "";
    // FSR Y0
    digitalWrite(selectPin0, LOW);
    digitalWrite(selectPin1, LOW);
    digitalWrite(selectPin2, LOW);
    int pot1ADC = analogRead(zInput);
    // FSR Y1
    digitalWrite(selectPin0, HIGH);
    digitalWrite(selectPin1, LOW);
    digitalWrite(selectPin2, LOW);
    int pot2ADC = analogRead(zInput);
    // FSR Y2
    digitalWrite(selectPin0, LOW);
    digitalWrite(selectPin1, HIGH);
    digitalWrite(selectPin2, LOW);
    int pot3ADC = analogRead(zInput);
    // FSR Y3
    digitalWrite(selectPin0, HIGH);
    digitalWrite(selectPin1, HIGH);
    digitalWrite(selectPin2, LOW);
    int pot4ADC = analogRead(zInput);
    //Datos MUX
    message += String(pot1ADC) + ",";
    message += String(pot2ADC) + ",";
    message += String(pot3ADC) + ",";
    message += String(pot4ADC) + ",";
    //Datos IMU
    message += String(imu.gyro_x, 6) + ",";
    message += String(imu.accel_x, 6) + ",";
    message += String(imu.quat_a / 29789.09091, 6) + ",";
    message += String(imu.quat_b / 29789.09091, 6) + ",";
    message += String(imu.quat_c / 29789.09091, 6) + ",";
    message += String(imu.quat_d / 29789.09091, 6) + "\n";
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
