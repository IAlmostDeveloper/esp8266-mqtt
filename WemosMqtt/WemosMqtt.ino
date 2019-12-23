#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define PIN_RED 14
#define PIN_GREEN 13
#define PIN_BLUE 12

const char *ssid = "xxxxxxx"; // Имя роутера
const char *pass = "xxxxxxx"; // Пароль роутера

const char *mqtt_server = "xxxxxxxxxxxxx"; // Имя сервера MQTT
const int mqtt_port = 1883; // Порт для подключения к серверу MQTT
const char *mqtt_user = "xxxxxxxx"; // Логи для подключения к серверу MQTT
const char *mqtt_pass = "xxxxxxxx"; // Пароль для подключения к серверу MQTT
const char* mqtt_topics[] = {"esp", "led", "pwmred", "pwmgreen", "pwmblue"};

#define OLED_RESET 1
Adafruit_SSD1306 display(OLED_RESET);

WiFiClient wclient; 

void setDisplay(){
  display.setCursor(0,0);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();
}

int getLedValue(byte* payload,  unsigned int length){
  int result = 0;
  int multiplier = 1;
  for(int i=length-1;i>=0;i--){
    result += ((char)payload[i] - '0') * multiplier;
    multiplier *= 10;
  }
  return result;
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

void pwmLed(int ledValue, char* topic){
  byte pin = strcmp(topic, "pwmred")==0 ? PIN_RED 
            : strcmp(topic, "pwmgreen")==0 ? PIN_GREEN 
            : strcmp(topic, "pwmblue")==0 ? PIN_BLUE 
            : 0;
  analogWrite(pin, ledValue);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("]");
  setDisplay();
  if(strcmp(topic, "esp")==0)
    printMessage(payload, length);
  else if(strcmp(topic, "led")==0)
    toggleLed(payload);
  else 
    pwmLed(getLedValue(payload, length), topic);
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
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  display.clearDisplay();
  
  Serial.begin(115200);
  delay(10);
  Serial.println("\n");
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
