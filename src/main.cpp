#include <Arduino.h>
#include <WiFi.h>
#include "LED.hpp"
#include "Sonar.hpp"
#include "camera.hpp"

//Maximum packet size = 50 KiB
#define MQTT_MAX_PACKET_SIZE 51200
#define MQTT_KEEPALIVE 3600
#define MQTT_SOCKET_TIMEOUT 1
#include <PubSubClient.h>

const char* WIFI_SSID     = "Rohan Hotspot";
const char* WIFI_PASSWORD = "01747910";

const char* ServerIP   = "142.93.62.203";
const char* ClientID   = "Rohan-ESP32CAM";
const int   PortServer =  1883;

const char* MQTT_SUB_TOPIC = "Result";
const char* MQTT_PUB_TOPIC = "Image";

const int QoS = 1;

//callback function prototype
void callback(char* topic, byte* payload, unsigned int length);

WiFiClient ESPClient;
PubSubClient MQTTClient(ServerIP, PortServer, callback, ESPClient);

const int Min_Capture_Dist = 25;   //25cm
const int Max_Capture_Dist = 40;   //40cm

const int SampleTime = 500;       //500ms
const int ACK_TIMEOUT = 30000;     //30000ms = 30s

bool Callback_Requested = false;
bool Callback_Handled  = false;

// Call back function
void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.println("Message Arrived");

  // Don't process the callback if not requested.
  // This protects from unwanted messages received
  if (!Callback_Requested)
    return;

  // Handle the callback for each topic
  if (!strcmp(topic, MQTT_SUB_TOPIC))
  {
    std::string response((char*)payload, length);

    Serial.println("Response: " + String(response.c_str()));

    if (!response.find("NO MATCH" ))
      LED_On(LED_RED);
    
    else
      LED_On(LED_GREEN);
  }

  // set the flag to indicate callback handled
  Callback_Handled = true;
}

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
    MQTTClient.disconnect();

    MQTTClient.setCallback(callback);
    
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

    // Subscribe to receive acknowledgements
    Serial.println("Subscribing to Topic: " + String(MQTT_SUB_TOPIC));

    while (!MQTTClient.subscribe(MQTT_SUB_TOPIC, QoS))
    {
      Serial.print(".");
      delay(500);
    }

    Serial.println("Suscribed Successfully");

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

  // Check if distance is within the capture range
  if (distance >= Min_Capture_Dist && distance <= Max_Capture_Dist)
  {
      LED_On(LED_WHITE);

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
        
        // Request and poll for a callback
        Callback_Requested = true;
        unsigned long int startTime = millis();

        Camera_FreeFrameBuffer(&fb);

        // Wait for acknowledgement upto the timeout period
        Serial.println("Waiting for response...");
        while(!Callback_Handled)
        {
          MQTTClient.loop();

          //after timeout turn LED Red
          if (millis() - startTime >= ACK_TIMEOUT)
          {
            Serial.println("Timeout...");
            LED_On(LED_RED);
            break;
          }
        }

        // reset the flag
        Callback_Handled = false;
        Callback_Requested = false;

        // delay at least 5 seconds before next frame is captured
        delay(5000);
      }
  }

  MQTTClient.loop();

  // Sampling time of 500ms
  delay(SampleTime);

  LED_On(LED_AQUA);
}