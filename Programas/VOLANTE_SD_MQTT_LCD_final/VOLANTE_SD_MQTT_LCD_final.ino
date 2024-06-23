#include <GP94.h>                // Biblioteca local para la IMU GP94 
#include <SPI.h>                 // Biblioteca para la comunicación SPI
#include <SD.h>                  // Biblioteca para tarjetas SD
#include <WiFi.h>                // Biblioteca para WiFi
#include <PubSubClient.h>        // Biblioteca para MQTT
#include <Adafruit_GFX.h>        // Biblioteca para gráficos de Adafruit
#include <Adafruit_SSD1306.h>    // Biblioteca para la pantalla OLED SSD1306

#define RXD2 16                       // RX Serial 1 
#define TXD2 17                       // TX Serial 1 
#define CHIP_SELECT 5                 // ChipSelect de la tarjeta SD 
#define SCREEN_WIDTH 128              // Ancho de la pantalla
#define SCREEN_HEIGHT 64              // Alto de la pantalla
#define OLED_RESET    -1              // Pin de reset
#define SCREEN_ADDRESS 0x3C           // Dirección I2C para la pantalla

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Inicializa la pantalla OLED
HardwareSerial mySerial(2);                                               // Usamos el segundo puerto serial
GP9 imu(mySerial);                                                        // Inicializa el objeto IMU usando la comunicación Serial 
WiFiClient espClient;                                                     // Cliente WiFi
PubSubClient client(espClient);                                           // Cliente MQTT

// Configuración de la red WiFi
const char* ssid = "";           // Nombre de la red
const char* password = "";    // Contraseña de la red

// Configuración del broker MQTT
const char* mqtt_server = "";    // Dirección del servidor MQTT
const int mqtt_port = 1883;                   // Puerto del servidor MQTT
const char* mqtt_user = "";          // Usuario MQTT
const char* mqtt_password = "";       // Contraseña MQTT


//Constantes MUX
const int selectPin0 = 33;    // Pin de selección S0
const int selectPin1 = 25;    // Pin de selección S1
const int selectPin2 = 26;    // Pin de selección S2
const int zInput = 32;        // Pin de entrada analógica Z
const float VCC = 4.67;       // Voltaje medido de la línea de 5V

// Variable para almacenar el timestamp
String currentTimestamp = "";

void setup() {
  // Inicialización del puerto serie para la comunicación con el ordenador
  Serial.begin(115200);
  
  // Inicialización del segundo puerto serie para la comunicación con la IMU
  mySerial.begin(115200, SERIAL_8N1, RXD2, TXD2);  

  // Inicialización de los pines de selección del MUX
  pinMode(selectPin0, OUTPUT);
  pinMode(selectPin1, OUTPUT);
  pinMode(selectPin2, OUTPUT);
  pinMode(zInput, INPUT);

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
  client.publish("volante/data", message.c_str()); // Publica el mensaje en el topic "volante/data"
}

// Función para imprimir datos en microSD
void printDataSD() {
  if (imu.decode(mySerial.read())) {
    File dataFile = SD.open("/data.txt", FILE_APPEND);
    if (dataFile) {

      // FSR Y0
      digitalWrite(selectPin0, LOW);
      digitalWrite(selectPin1, LOW);
      digitalWrite(selectPin2, LOW);
      int pot1ADC = analogRead(zInput);
      float pot1V = pot1ADC * VCC / 4095.0;

      // FSR Y1
      digitalWrite(selectPin0, HIGH);
      digitalWrite(selectPin1, LOW);
      digitalWrite(selectPin2, LOW);
      int pot2ADC = analogRead(zInput);
      float pot2V = pot2ADC * VCC / 4095.0;

      // FSR Y2
      digitalWrite(selectPin0, LOW);
      digitalWrite(selectPin1, HIGH);
      digitalWrite(selectPin2, LOW);
      int pot3ADC = analogRead(zInput);
      float pot3V = pot3ADC * VCC / 4095.0;

      // FSR Y3
      digitalWrite(selectPin0, HIGH);
      digitalWrite(selectPin1, HIGH);
      digitalWrite(selectPin2, LOW);
      int pot4ADC = analogRead(zInput);
      float pot4V = pot4ADC * VCC / 4095.0;

      // Verifica si el timestamp está vacío
      if (currentTimestamp == "") {
        Serial.println("Error: El timestamp está vacío.");
      }

      // Imprimir timestamp
      dataFile.print(currentTimestamp); dataFile.print(" ");

      // Imprimir datos del MUX
      dataFile.print(pot1V, 6); dataFile.print(",");
      dataFile.print(pot2V, 6); dataFile.print(",");
      dataFile.print(pot3V, 6); dataFile.print(",");
      dataFile.print(pot4V, 6); dataFile.print(",");

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
      dataFile.println(imu.yaw);
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

    // FSR Y0
    digitalWrite(selectPin0, LOW);
    digitalWrite(selectPin1, LOW);
    digitalWrite(selectPin2, LOW);
    int pot1ADC = analogRead(zInput);
    float pot1V = pot1ADC * VCC / 4095.0;

    // FSR Y1
    digitalWrite(selectPin0, HIGH);
    digitalWrite(selectPin1, LOW);
    digitalWrite(selectPin2, LOW);
    int pot2ADC = analogRead(zInput);
    float pot2V = pot2ADC * VCC / 4095.0;

    // FSR Y2
    digitalWrite(selectPin0, LOW);
    digitalWrite(selectPin1, HIGH);
    digitalWrite(selectPin2, LOW);
    int pot3ADC = analogRead(zInput);
    float pot3V = pot3ADC * VCC / 4095.0;

    // FSR Y3
    digitalWrite(selectPin0, HIGH);
    digitalWrite(selectPin1, HIGH);
    digitalWrite(selectPin2, LOW);
    int pot4ADC = analogRead(zInput);
    float pot4V = pot4ADC * VCC / 4095.0;

    // Verifica si el timestamp está vacío
    if (currentTimestamp == "") {
      Serial.println("Error: El timestamp está vacío.");
    }

    // Imprir timestamp e iniciar payload
    String message = currentTimestamp + " ";
    message += String(pot1V, 6) + ",";
    message += String(pot2V, 6) + ",";
    message += String(pot3V, 6) + ",";
    message += String(pot4V, 6) + ",";
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
