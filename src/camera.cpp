/*! @file  camera.cpp
 *
 *  @brief Routines for operating the OV2640 camera module
 *
 *  This contains the functions needed for operating the camera module in ESP32-CAM
 *
 *  @author Rohan
 *  @date 2020-06-03
 */
#include <camera.hpp>

// setting PWM properties for flash
static const int FREQ = 500;                /*!< PWM Signal Frequency*/
static const int FLASH_LEDChannel = 4;      /*!< Timer Channel attached with the flash*/
static const int Resolution = 8;            /*!< Resolution of the PWM duty cycle*/


/*! @brief Initializes the camera and the flash 
 *
 *  @return bool - TRUE if initialization is successful
 */
bool Camera_Init()
{
    camera_config_t config = 
    {
        .pin_pwdn = PWDN_GPIO_NUM,
        .pin_reset = RESET_GPIO_NUM,

        .pin_xclk = XCLK_GPIO_NUM,
        .pin_sscb_sda = SIOD_GPIO_NUM,
        .pin_sscb_scl = SIOC_GPIO_NUM,

        .pin_d7 = Y9_GPIO_NUM,
        .pin_d6 = Y8_GPIO_NUM,
        .pin_d5 = Y7_GPIO_NUM,
        .pin_d4 = Y6_GPIO_NUM,
        .pin_d3 = Y5_GPIO_NUM,
        .pin_d2 = Y4_GPIO_NUM,
        .pin_d1 = Y3_GPIO_NUM,
        .pin_d0 = Y2_GPIO_NUM,
        
        .pin_vsync = VSYNC_GPIO_NUM,
        .pin_href = HREF_GPIO_NUM,
        .pin_pclk = PCLK_GPIO_NUM,

        .xclk_freq_hz = 20000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        
        .pixel_format = PIXFORMAT_JPEG,
    };

    //init with high specs to pre-allocate larger buffers
    if(psramFound())
    {
        config.frame_size   = FRAME_SIZE;
        config.jpeg_quality = 10;
        config.fb_count     = 2;
    } 
    else 
    {
        config.frame_size   = FRAMESIZE_VGA;
        config.jpeg_quality = 12;
        config.fb_count     = 1;
    }

    // Init Camera
    esp_err_t error = esp_camera_init(&config);

    if (error != ESP_OK)
    {
        Serial.printf("Camera init failed with error 0x%x\n", error);
        return false;
    }

    // Initialize the flash
    ledcSetup(FLASH_LEDChannel, FREQ, Resolution);
    ledcAttachPin(FLASH_PIN, FLASH_LEDChannel);
    ledcWrite(FLASH_LEDChannel, 0); //turn off flash initially

    Serial.println("Camera Initialized!");

    return true;
}


/*! @brief When the function is called, the camera captures a frame
 *
 *  @param  frameBuffer - pointer to the start of the frameBuffer
 *  @return bool - TRUE if the frame is captured successfully
 */
bool Camera_Capture(camera_fb_t **frameBuffer)
{
    //turn on the flash
    ledcWrite(FLASH_LEDChannel, FlashBrightness);
    delay(300);

    *frameBuffer = esp_camera_fb_get();

    ledcWrite(FLASH_LEDChannel, 0); //turn off the flash

    if (!(*frameBuffer))
    {
        Serial.println("Camera capture failed.");
        return false;
    }

    Serial.println("Frame captured.");
    return true;
}


/*! @brief  Frees the allocated buffer memory. 
 *          Must be called after the data in the frameBuffer has been processed.
 *
 *  @param  frameBuffer - pointer to the start of the frameBuffer
 *  @return bool - TRUE if the deallocation is successful
 */
bool Camera_FreeFrameBuffer(camera_fb_t **frameBuffer)
{
    if (!(*frameBuffer))
        esp_camera_fb_return(*frameBuffer);
    
    return true;
}
