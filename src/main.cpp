#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "LED.hpp"
#include "Sonar.hpp"
#include "camera.hpp"

const char* WIFI_SSID     = "Rohan Hotspot";
const char* WIFI_PASSWORD = "01747910";

const char* ServerIP   = "142.93.62.203";
const char* ClientID   = "Rohan-ESP32CAM";
const int   PortServer =  1883;

const char* MQTT_SUB_TOPIC = "";
const char* MQTT_PUB_TOPIC = "Sonar";

WiFiClient ESPClient;
PubSubClient MQTTClient(ServerIP, PortServer, ESPClient);

const int ActivateLEDDistance = 20;
const int SampleTime = 500;

void WiFi_Connect()
{
  // Set up WiFi Connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  WiFi.setSleep(false);

  Serial.print("WiFi Connecting to ");
  Serial.println(WIFI_SSID);
  
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
}

void MQTT_Connect()
{   
    Serial.print("Connecting to MQTT broker on ");
    Serial.println(ServerIP);

    while (!MQTTClient.connected())
    {
        if (MQTTClient.connect(ClientID))
            Serial.println("MQTT Broker Connected.");
    
        else
        {
            Serial.print("Failed, rc=");
            Serial.print(MQTTClient.state());
            Serial.println(". Try again in 5 seconds");
                
            // Wait 5 seconds before retrying
            delay(5000);            
        }
    }
}

void setup() 
{
  Serial.begin(115200);
  Serial.println("ESP32-S started successfully.");

  LED_Init();

  Sonar_Init();

  while (!Camera_Init())
    delay(1000);

  WiFi_Connect();

  MQTT_Connect();
}

/* TODO: WiFi gets disconnected after a while. Fix it */
void loop() 
{
  char   payload[256];
  String buffer;

  LED_On(LED_BLUE);

  // Check if the WiFi and MQTT broker is still connected
  if (WiFi.status() != WL_CONNECTED)
    WiFi_Connect();
  if (!MQTTClient.connected())
    MQTT_Connect();

  int distance = Sonar_GetDistance();
  
  buffer = "Distance = " + String(distance) + " cm";
  
  Serial.println(buffer);

  // Check if distance is valid
  if (distance <= ActivateLEDDistance && distance != -1)
  {
      LED_On(LED_GREEN);
      Serial.println("Green LED Turned On.");

      buffer.toCharArray(payload, buffer.length()+1);

      if (MQTTClient.publish(MQTT_PUB_TOPIC, payload))
        Serial.println("Message Sent!");
  }

  // Sampling time of 500ms
  delay(SampleTime);
}