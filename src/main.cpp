/*!
 ** @file main.cpp
 ** @version 1.0
 ** @brief
 **         Main module.
 **         This module contains user's application code.
 */
/*!
 **  @addtogroup main_module main module documentation
 **  @{
 */
#include <Arduino.h>
#include <WiFi.h>
#include "LED.hpp"
#include "Sonar.hpp"
#include "camera.hpp"

#define MQTT_MAX_PACKET_SIZE 51200              /*!< Maximum MQTT packet size in bytes*/
#define MQTT_KEEPALIVE 3600                     /*!< The keep alive timeout for the MQTT connection in seconds*/
#define MQTT_SOCKET_TIMEOUT 1                   /*!< The socket timeout for the MQTT connection in seconds*/
#include <PubSubClient.h>

const char* WIFI_SSID     = "Rohan Hotspot";    /*!< WIFI SSID to connect to*/
const char* WIFI_PASSWORD = "01747910";         /*!< WIFI Password*/

const char* ServerIP   = "142.93.62.203";       /*!< IP Address of the MQTT Broker*/
const char* ClientID   = "Rohan-ESP32CAM";      /*!< Client ID of the device*/
const int   PortServer =  1883;                 /*!< The server port for MQTT Connection*/

const char* MQTT_SUB_TOPIC = "Result";          /*!< Topic the device is subscribed*/
const char* MQTT_PUB_TOPIC = "Image";           /*!< Topic the device is publishing to*/

const int QoS = 1;                              /*!< MQTT Quality of Service*/

//callback function prototype
void callback(char* topic, byte* payload, unsigned int length);

WiFiClient ESPClient;                                                 /*!< A WiFi Client to be used by the PubSub Client*/
PubSubClient MQTTClient(ServerIP, PortServer, callback, ESPClient);   /*!< A PubSub Client for MQTT Connection*/

const int Min_Capture_Dist = 25;       /*!< Minimum Distance (in cm) an object needs to be to capture image*/
const int Max_Capture_Dist = 40;       /*!< Maximum Distance (in cm) an object needs to be to capture image*/

const int SampleTime = 500;           /*!< The Sampling Time of the Sonar module (in ms)*/   
const int ACK_TIMEOUT = 30000;        /*!< Timeout for no response received after image published (in ms)*/

bool Callback_Requested = false;             /*!< A flag to track if a callback has been requested*/
bool Callback_Handled  = false;              /*!< A flag to check if the callback has been handled*/


/*! @brief  Callback function when a message arrives for the subsribed topics
 *
 *  @param  topic   - topic for which message has arrived
 *  @param  payload - the payload of the arrived message
 *  @param  length  - the length of the message in the payload
 */
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


/*! @brief  Connects to the WiFi
 *
 */
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


/*! @brief  Connects to MQTT broker
 *
 *  @note   Assumes WiFi connection is successful
 */
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


/*! @brief  Initializes all the modules and connects to WiFi and MQTT Broker
 *  
 */
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


/*! @brief  The main loop that runs idefinitely
 *
 *  @note   Assumes setup() has been called
 */
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

/*!
** @}
*/