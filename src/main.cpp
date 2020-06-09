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
// Function Prototype
//-----------------------------------------------------------------------------
void callback(char* topic, byte* payload, unsigned int length);

/******************************************************************************
 * Packet Handler Commands
 ******************************************************************************/
#define CMD_AutoCapture   01
#define CMD_ManualCapture 02
#define CMD_Range         03

/******************************************************************************
 * MQTT Received Packet Structure for control topic
 ******************************************************************************/
struct 
{
  uint8_t command,
          param1,
          param2;

} MQTT_Packet;

//*****************************************************************************
// Global Configurations
//-----------------------------------------------------------------------------
const char* WIFI_SSID     = "Rohan Hotspot";    /*!< WIFI SSID to connect to*/
const char* WIFI_PASSWORD = "01747910";         /*!< WIFI Password*/

const char* ServerIP   = "142.93.62.203";       /*!< IP Address of the MQTT Broker*/
const char* ClientID   = "Rohan-ESP32CAM";      /*!< Client ID of the device*/
const int   PortServer =  1883;                 /*!< The server port for MQTT Connection*/
const int QoS = 1;                              /*!< MQTT Quality of Service*/

#define DEFAULT_MIN_DIST 25
#define DEFAULT_MAX_DIST 40

volatile int Min_Capture_Dist = DEFAULT_MIN_DIST;             /*!< Minimum Distance (in cm) an object needs to be to capture image*/
volatile int Max_Capture_Dist = DEFAULT_MAX_DIST;             /*!< Maximum Distance (in cm) an object needs to be to capture image*/

const int SampleTime = 500;                     /*!< The Sampling Time of the Sonar module (in ms)*/   
const int ACK_TIMEOUT = 30000;                  /*!< Timeout for no response received after image published (in ms)*/

bool Callback_Requested = false;                /*!< A flag to track if a callback has been requested*/
bool Callback_Handled  = false;                 /*!< A flag to check if the callback has been handled*/

volatile bool Auto_Capture = true;              /*!< A flag to indicate if auto capture mode is enabled. It can be changed via MQTT command*/

//*****************************************************************************
// MQTT Publish and subscribe topics
//-----------------------------------------------------------------------------
const char* MQTT_PUB_TOPIC     = "Image";       /*!< Topic the device is publishing to*/
const char* MQTT_RESULT_TOPIC  = "Result";
const char* MQTT_CONTROL_TOPIC = "Control";

//*****************************************************************************
// Global Objects
//-----------------------------------------------------------------------------
WiFiClient ESPClient;                                                 /*!< A WiFi Client to be used by the PubSub Client*/
PubSubClient MQTTClient(ServerIP, PortServer, callback, ESPClient);   /*!< A PubSub Client for MQTT Connection*/


/*! @brief  Camera captures an image and publishes to a given topic via MQTT
 *
 *  @param  topic   topic to publish the frame buffer
 */
static inline bool Capture_n_Publish()
{
  camera_fb_t *fb = NULL;
  
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

    return true;
  }

  return false;
}

/*! @brief  Camera captures a frame, publishes the image and handles the response
 *
 *  @param  topic   topic to publish the frame buffer
 */
static inline void Capture_n_Recognize ()
{
  LED_On(LED_WHITE);

  if (Capture_n_Publish())
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
static inline void Result_Handler (byte* payload, unsigned int length)
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

static inline void AutoCapture_Handler ()
{
  if (MQTT_Packet.param1 == 0)
  {
    Auto_Capture = false;
    Serial.println("Auto Capture Turned off");
  }

  else if (MQTT_Packet.param1 == 1)
  {
    Auto_Capture = true;
    Serial.println("Auto Capture Turned on");
  }

  else
    Serial.println("Invalid Parameters");
  
}

static inline void ManualCapture_Handler ()
{
  if (MQTT_Packet.param1 == 0)
    Capture_n_Publish();

  else if (MQTT_Packet.param1 == 1)
    Capture_n_Recognize();
  
  else
    Serial.println("Invalid Parameters");
}

static inline void Range_Handler()
{
  //TODO: Validation Check

  if (MQTT_Packet.param1 != 0)
  {
    if (MQTT_Packet.param1 == 99)
    {
      Min_Capture_Dist = DEFAULT_MIN_DIST;
      Serial.println("DEFAULT MIN LOADED");
    }
    
    else if (MQTT_Packet.param1 >= 20 && MQTT_Packet.param1 <= 40)
    {
      Min_Capture_Dist = MQTT_Packet.param1;
      Serial.println("New Min Distance = " + MQTT_Packet.param1);
    }
  }

  if (MQTT_Packet.param2 != 0)
  {
    if (MQTT_Packet.param2 == 99)
    {
      Max_Capture_Dist = DEFAULT_MAX_DIST;
      Serial.println("DEFAULT MAX LOADED");
    }
    
    else if (MQTT_Packet.param2 >= 20 && MQTT_Packet.param2 <= 60)
    {
      Max_Capture_Dist = MQTT_Packet.param2;
      Serial.println("New Max Distance = " + MQTT_Packet.param2);
    }
  } 

}

/*! @brief  Callback function when a message arrives for the subsribed topics
 *
 *  @param  topic   - topic for which message has arrived
 *  @param  payload - the payload of the arrived message
 *  @param  length  - the length of the message in the payload
 */
void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.println("Message Arrived\nTopic: " + String(topic));

  if (!strcmp(topic, MQTT_RESULT_TOPIC))
    Result_Handler(payload, length);

  else if (!strcmp(topic, MQTT_CONTROL_TOPIC))
  {
    // a valid packet size is 6 bytes
    if (length != 6)
    {
      Serial.println("Invalid Packet");
      return;
    }

    Serial.println(std::string((char*) payload, 6).c_str());

    std::string buffer = std::string((char*) payload, 0, 2);
    MQTT_Packet.command = (uint8_t) strtoul(buffer.c_str(), NULL, 10);

    buffer = std::string((char*) payload, 2, 2);
    MQTT_Packet.param1 = (uint8_t) strtoul(buffer.c_str(), NULL, 10);
    
    buffer = std::string((char*) payload, 4, 2);
    MQTT_Packet.param2 = (uint8_t) strtoul(buffer.c_str(), NULL, 10);

    switch (MQTT_Packet.command)
    {
      case CMD_AutoCapture:
        AutoCapture_Handler();
        break;

      case CMD_ManualCapture:
        ManualCapture_Handler();
        break;
      
      case CMD_Range:
        Range_Handler();
        break;
    }
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

    // Subsribe to the topics
    Serial.println("\nSubscribing to Topic: " + String(MQTT_RESULT_TOPIC));

    while (!MQTTClient.subscribe(MQTT_RESULT_TOPIC, QoS));

    Serial.println("Subscribing to Topic: " + String(MQTT_CONTROL_TOPIC));

    while (!MQTTClient.subscribe(MQTT_CONTROL_TOPIC, QoS));

    Serial.println("Subscribed Successfully\n");
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
    Capture_n_Recognize();

  MQTTClient.loop();

  // Sampling time of 500ms
  delay(SampleTime);

  LED_On(LED_AQUA);
}
/*!
** @}
*/