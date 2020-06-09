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

//*****************************************************************************
// Struct for handling MQTT subscribed topics
//-----------------------------------------------------------------------------
typedef struct
{
  std::string topic;
  void (*Handler)(byte*, unsigned int);

}Subscribe_t;


//*****************************************************************************
// Function Prototypes
//-----------------------------------------------------------------------------
void callback(char* topic, byte* payload, unsigned int length);
void Result_Handler (byte* payload, unsigned int length);
void AutoCapture_Handler (byte* payload, unsigned int length);
void ManualCapture_Handler (byte* payload, unsigned int length);
void WhoAmI_Handler (byte* payload, unsigned int length);

//*****************************************************************************
// Global Configurations
//-----------------------------------------------------------------------------
const char* WIFI_SSID     = "Rohan Hotspot";    /*!< WIFI SSID to connect to*/
const char* WIFI_PASSWORD = "01747910";         /*!< WIFI Password*/

const char* ServerIP   = "142.93.62.203";       /*!< IP Address of the MQTT Broker*/
const char* ClientID   = "Rohan-ESP32CAM";      /*!< Client ID of the device*/
const int   PortServer =  1883;                 /*!< The server port for MQTT Connection*/
const int QoS = 1;                              /*!< MQTT Quality of Service*/

const int Min_Capture_Dist = 25;                /*!< Minimum Distance (in cm) an object needs to be to capture image*/
const int Max_Capture_Dist = 40;                /*!< Maximum Distance (in cm) an object needs to be to capture image*/

const int SampleTime = 500;                     /*!< The Sampling Time of the Sonar module (in ms)*/   
const int ACK_TIMEOUT = 30000;                  /*!< Timeout for no response received after image published (in ms)*/

bool Callback_Requested = false;                /*!< A flag to track if a callback has been requested*/
bool Callback_Handled  = false;                 /*!< A flag to check if the callback has been handled*/

volatile bool Auto_Capture = true;              /*!< A flag to indicate if auto capture mode is enabled. It can be changed via MQTT command*/

//*****************************************************************************
// MQTT Publish and subscribe topics
//-----------------------------------------------------------------------------
const char* MQTT_PUB_TOPIC = "Image";           /*!< Topic the device is publishing to*/

#define NB_SUB_TOPICS 4

const Subscribe_t MQTT_SUB_TOPICS[NB_SUB_TOPICS]  =
{
  {
    .topic   = "Result",
    .Handler = &Result_Handler
  },

  {
    .topic   = "Auto_Capture",
    .Handler = &AutoCapture_Handler
  },

  {
    .topic   = "Manual_Capture",
    .Handler = &ManualCapture_Handler
  },

  {
    .topic   = "WhoAmI",
    .Handler = &WhoAmI_Handler
  }
};

//*****************************************************************************
// Global Objects
//-----------------------------------------------------------------------------
WiFiClient ESPClient;                                                 /*!< A WiFi Client to be used by the PubSub Client*/
PubSubClient MQTTClient(ServerIP, PortServer, callback, ESPClient);   /*!< A PubSub Client for MQTT Connection*/


/*! @brief  Camera captures an image and publishes to a given topic via MQTT
 *
 *  @param  topic   topic to publish the frame buffer
 */
static inline bool Capture_n_Publish(const char* topic)
{
  camera_fb_t *fb = NULL;
  
  if (Camera_Capture(&fb))
  {
    const uint8_t* fbPayload = (uint8_t*)fb->buf;
    size_t fbSize = fb->len;

    Serial.println("Size = "+ String(fbSize)+" bytes");

    // Start Publishing
    MQTTClient.beginPublish(topic, fbSize, false);
    // Write the frame buffer content in the packet 
    MQTTClient.write(fbPayload, fbSize);
    // Check if the the publishing is finsied
    if (MQTTClient.endPublish())
        Serial.println("Frame Sent!");
    
    Camera_FreeFrameBuffer(&fb);

    return true;
  }

  return false;
}

static inline void Capture_n_Recognize (const char* topic)
{
  LED_On(LED_WHITE);

  if (Capture_n_Publish(MQTT_PUB_TOPIC))
  {        
    // Request and poll for a callback
    Callback_Requested = true;
    unsigned long int startTime = millis();

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

//*****************************************************************************
// Call back functions and handlers
//-----------------------------------------------------------------------------
void Result_Handler (byte* payload, unsigned int length)
{
  // Don't process the callback if not requested.
  // This protects from unwanted messages received
  if (!Callback_Requested)
    return;

  std::string response((char*)payload, length);

  Serial.println("Response: " + String(response.c_str()));

  if (!response.find("NO MATCH"))
    LED_On(LED_RED);

  else
    LED_On(LED_GREEN);

  // set the flag to indicate callback handled
  Callback_Handled = true;
}


void AutoCapture_Handler (byte* payload, unsigned int length)
{
  std::string response((char*)payload, length);

  Serial.println("Message: " + String(response.c_str()));

  // if the length is incorrect, ignore
  if (length != 1)
    return;

  if (response[0] == '0')
  {
    Auto_Capture = false;
    Serial.println("Auto Capture Turned off");
  }
    
  else if (response[0] == '1')
  {
    Auto_Capture = true;
    Serial.println("Auto Capture Turned on");
  }
}


void ManualCapture_Handler (byte* payload, unsigned int length)
{
  std::string response((char*)payload, length);

  Serial.println("Message: " + String(response.c_str()));

  if (!strcmp(response.c_str(), "Capture Now!"))
    Capture_n_Publish(MQTT_PUB_TOPIC);
}

void WhoAmI_Handler (byte* payload, unsigned int length)
{
  std::string response((char*)payload, length);

  Serial.println("Message: " + String(response.c_str()));

  if (!strcmp(response.c_str(), "who Am I?"))
    Capture_n_Recognize(MQTT_PUB_TOPIC);

}


/*! @brief  Callback function when a message arrives for the subsribed topics
 *
 *  @param  topic   - topic for which message has arrived
 *  @param  payload - the payload of the arrived message
 *  @param  length  - the length of the message in the payload
 */
void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.println("Message Arrived");

  for (int i = 0; i < NB_SUB_TOPICS; i++)
    // Handle the callback for each topic
    if (!strcmp(topic, MQTT_SUB_TOPICS[i].topic.c_str()))
    { 
      Serial.println("Topic:" + String(topic));
      
      MQTT_SUB_TOPICS[i].Handler(payload, length);
      break;
    }
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

    // Subscribe to all the topics
    for (int i = 0; i < NB_SUB_TOPICS; i++)
    {
      char* topic = (char*) MQTT_SUB_TOPICS[i].topic.c_str();

      Serial.println("\nSubscribing to Topic: " + String(topic));

      while (!MQTTClient.subscribe(topic, QoS))
      {
        Serial.print(".");
        delay(500);
      }
    }

    Serial.println("\nSuscribed Successfully to " + String(NB_SUB_TOPICS) + " topics");
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
  // Check if the WiFi and MQTT broker is still connected
  if (WiFi.status() != WL_CONNECTED)
    WiFi_Connect();
  if (!MQTTClient.connected())
    MQTT_Connect();

  if (Auto_Capture)
    LED_On(LED_BLUE);
  else
    LED_On(LED_PURPLE);

  int distance = Sonar_GetDistance();
  
  Serial.println("Distance = " + String(distance) + " cm");

  // Check if distance is within the capture range if auto capture is enabled
  if (Auto_Capture && 
      distance >= Min_Capture_Dist && distance <= Max_Capture_Dist)
    Capture_n_Recognize(MQTT_PUB_TOPIC);

  MQTTClient.loop();

  // Sampling time of 500ms
  delay(SampleTime);

  LED_On(LED_AQUA);
}
/*!
** @}
*/