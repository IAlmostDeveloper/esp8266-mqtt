#include <SPI.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DHT.h"

unsigned long last_sending_time = 0;

#define TEMPERATURE_SENSOR_PIN 14

const char *ssid = "xxxxxxxxx";
const char *pass = "xxxxxxxxx";

const char *mqtt_server = "xxxxxxxxxxxxxxxx";
const int mqtt_port = 1883;
const char *mqtt_user = "xxxxxxxxx";
const char *mqtt_pass = "xxxxxxxxx";
const char *mqtt_send_topic = "xxxxxxxx";

WiFiClient wclient; 
DHT dht(TEMPERATURE_SENSOR_PIN, DHT11);

void callback(char* topic, byte* payload, unsigned int length);
void sendDataByMqtt();

PubSubClient client(mqtt_server, mqtt_port, callback, wclient);

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("]");
  if(strcmp("requests", topic)==0){
   sendDataByMqtt();
  }
}

void sendDataByMqtt(){
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& obj = jsonBuffer.createObject();
  String data = "";
  byte results[1024];
  obj["temperature"] = dht.readTemperature();
  obj["humidity"] = dht.readHumidity();
  obj.printTo(data);
  for(int i=0;i<data.length();i++){
    results[i] = (byte)data[i];
  }
  client.publish(mqtt_send_topic, results, data.length());
Serial.println(data);
  Serial.println(data.length());
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
    client.subscribe("requests");
   } 
  else
    Serial.println("Could not connect to MQTT server"); 
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 1);
  dht.begin();
  Serial.begin(115200);
  delay(10);
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
      if(millis()-last_sending_time>5000){
        last_sending_time = millis();
        sendDataByMqtt();
      }
    }
  }
}
