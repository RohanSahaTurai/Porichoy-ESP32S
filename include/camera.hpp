#ifndef CAMERA_H
#define CAMERA_H

#include <Arduino.h>
#include <esp_camera.h>
#include <esp32-hal-ledc.h>

// Pin definition for CAMERA_MODEL_AI_THINKER
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

// Prefered frame size
// The frame size will only be activated if PSRAM is detected
// Otherwise, frame size is dropped to VGA by default
const framesize_t FRAME_SIZE = FRAMESIZE_SVGA;

//Flash configuration
#define FLASH_PIN   4
const int FlashBrightness = 10; //0-255, brightest 255

bool Camera_Init();

// Captures a frame
bool Camera_Capture(camera_fb_t **frameBuffer);

// Frees the allocated memory.
// Must be called after the data in the frameBuffer has been processed.
bool Camera_FreeFrameBuffer(camera_fb_t **frameBuffer);

#endif