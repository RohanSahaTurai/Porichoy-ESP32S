/*! @file  camera.hpp
 *
 *  @brief Routines for operating the OV2640 camera module
 *
 *  This contains the functions needed for operating the camera module in ESP32-CAM
 *
 *  @author Rohan
 *  @date 2020-06-03
 */
/*!
 * @addtogroup Camera_module Camera Module documentation
 * @{
 *
 *  This contains the functions needed for operating the camera module in ESP32-CAM
 *
 */
#ifndef CAMERA_H
#define CAMERA_H

#include <Arduino.h>
#include <esp_camera.h>
#include <esp32-hal-ledc.h>

#define PWDN_GPIO_NUM     32                        
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// The frame size will only be activated if PSRAM is detected
// Otherwise, frame size is dropped to VGA by default
const framesize_t FRAME_SIZE = FRAMESIZE_SVGA;          /*!< Prefered frame size for image capture*/

//Flash configuration
#define FLASH_PIN   4                                   /*!< The pin to which the internal flash is connected*/
const int FlashBrightness = 10;                         /*!< The flash brightness; Range 0-255, brightest = 255 */


/*! @brief Initializes the camera and the flash 
 *
 *  @return bool - TRUE if initialization is successful
 */
bool Camera_Init();


/*! @brief When the function is called, the camera captures a frame
 *
 *  @param  frameBuffer - pointer to the start of the frameBuffer
 *  @return bool - TRUE if the frame is captured successfully
 */
bool Camera_Capture(camera_fb_t **frameBuffer);


/*! @brief  Frees the allocated buffer memory. 
 *          Must be called after the data in the frameBuffer has been processed.
 *
 *  @param  frameBuffer - pointer to the start of the frameBuffer
 *  @return bool - TRUE if the deallocation is successful
 */
bool Camera_FreeFrameBuffer(camera_fb_t **frameBuffer);

#endif
/*!
 * @}
 */
