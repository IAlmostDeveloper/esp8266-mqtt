#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define PIN_RED 14
#define PIN_GREEN 13
#define PIN_BLUE 12

const char *ssid = "xxxxxxxxx"; // Имя роутера
const char *pass = "xxxxxxxxx"; // Пароль роутера

const char *mqtt_server = "xxxxxxxxx"; // Имя сервера MQTT
const int mqtt_port = 1883; // Порт для подключения к серверу MQTT
const char *mqtt_user = "xxxxxxxxx"; // Логи для подключения к серверу MQTT
const char *mqtt_pass = "xxxxxxxxx"; // Пароль для подключения к серверу MQTT
const char* mqtt_topics[] = {"esp", "led", "pwmrgb"};

#define OLED_RESET 1
Adafruit_SSD1306 display(OLED_RESET);

WiFiClient wclient; 

void setDisplay(){
  display.setCursor(0,0);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();
}

void printMessage(byte* payload, unsigned int length){
  for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
      display.print((char)payload[i]);
      display.display();
  }
}

void toggleLed(byte* payload){
  if((char)payload[0] == '1' || (char)payload[0] == '0')
    digitalWrite(LED_BUILTIN, (int)((char)payload[0] - '0'));
}

void setRgbLed(byte* payload){
 const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
    DynamicJsonBuffer jsonBuffer(capacity);
    JsonObject& root = jsonBuffer.parseObject(payload);
    if (!root.success()) {
      Serial.println(F("Parsing failed!"));
      return;
    }
    int r = root["R"].as<int>();
    int g = root["G"].as<int>();
    int b = root["B"].as<int>();
    analogWrite(PIN_RED, 1023 - r);
    analogWrite(PIN_GREEN, 1023- g);
    analogWrite(PIN_BLUE, 1023 - b);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("]");
  setDisplay();
  if(strcmp(topic, "pwmrgb")==0)
    setRgbLed(payload);
  if(strcmp(topic, "esp")==0)
    printMessage(payload, length);
  else if(strcmp(topic, "led")==0)
    toggleLed(payload);
  Serial.println();
}

PubSubClient client(mqtt_server, mqtt_port, callback, wclient);

void subscribeToAllTopics(){
  for(int i=0;i<(sizeof(mqtt_topics)/sizeof(*mqtt_topics));i++)
    client.subscribe(mqtt_topics[i]);
}

void connectToWiFi(){
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println("...");
    WiFi.begin(ssid, pass);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) return;
    Serial.println("WiFi connected");
  }
}

void connectToMQTT(){
  String clientId = "ESP8266Client-";
  clientId += String(random(0xffff), HEX);
  if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
    Serial.println("Connected to MQTT server ");
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    subscribeToAllTopics();
   } 
  else
    Serial.println("Could not connect to MQTT server"); 
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
  digitalWrite(PIN_RED,1);
  digitalWrite(PIN_GREEN,1);
  digitalWrite(PIN_BLUE,1);
  digitalWrite(LED_BUILTIN,1);
  
  Serial.begin(115200);
  delay(10);
  Serial.println("\n");
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  display.clearDisplay();
}

void loop() {
   connectToWiFi();
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      Serial.print("Connecting to MQTT server ");
      Serial.print(mqtt_server);
      Serial.println("...");
      connectToMQTT();
    }
    if (client.connected()){
      client.loop();
    }
  }
}
