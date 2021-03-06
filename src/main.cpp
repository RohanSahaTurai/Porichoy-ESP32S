#include <Arduino.h>
#include <WiFi.h>
#include "LED.hpp"
#include "Sonar.hpp"
#include "camera.hpp"

//Maximum packet size = 50 KiB
#define MQTT_MAX_PACKET_SIZE 51200
#include <PubSubClient.h>

const char* WIFI_SSID     = "Rohan Hotspot";
const char* WIFI_PASSWORD = "01747910";

const char* ServerIP   = "test.mosquitto.org";
const char* ClientID   = "Rohan-ESP32CAM";
const int   PortServer =  1883;

const char* MQTT_SUB_TOPIC = "";
const char* MQTT_PUB_TOPIC = "Image";

WiFiClient ESPClient;
PubSubClient MQTTClient(ServerIP, PortServer, ESPClient);

const int CaptureDistance = 40;
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
  
  LED_On(LED_WHITE);

  WiFi_Connect();
  
  LED_On(LED_AQUA);

  MQTT_Connect();
}

/* TODO: WiFi gets disconnected after a while. Fix it */
void loop() 
{
  String buffer;
  camera_fb_t *fb = NULL;

  // Check if the WiFi and MQTT broker is still connected
  if (WiFi.status() != WL_CONNECTED)
    WiFi_Connect();
  if (!MQTTClient.connected())
    MQTT_Connect();

  LED_On(LED_BLUE);

  int distance = Sonar_GetDistance();
  
  buffer = "Distance = " + String(distance) + " cm";
  
  Serial.println(buffer);

  // Check if distance is valid
  if (distance <= CaptureDistance && distance != -1)
  {
      LED_On(LED_YELLOW);

      if (Camera_Capture(&fb))
      {
        const uint8_t* fbPayload = (uint8_t*)fb->buf;
        size_t fbSize = fb->len;

        Serial.println("Size = "+ String(fbSize)+" bytes");

        // Start Publishing
        MQTTClient.beginPublish(MQTT_PUB_TOPIC, fbSize, false);
        // Write the frame buffer content in the packet 
        MQTTClient.write(fbPayload, fbSize);
        // Check if the the publishing is finsied
        if (MQTTClient.endPublish())
           Serial.println("Frame Sent!");
  
        Camera_FreeFrameBuffer(&fb);
      }
  }

  // Sampling time of 500ms
  delay(SampleTime);

  LED_On(LED_WHITE);
}