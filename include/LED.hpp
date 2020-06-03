/*! @file  LED.hpp
 *
 *  @brief Routines for controlling the RGB LED Module
 *
 *  This contains the functions needed for controlling the RGB LED Module
 *
 *  @author Rohan
 *  @date 2020-06-03
 */
/*!
 * @addtogroup RGB_LED_Module RGB LED Module documentation
 * @{
 *
 *  This contains the functions needed for controlling the RGB LED Module
 *
 */
#ifndef _LED_H_
#define _LED_H_

#include <Arduino.h>
#include <esp32-hal-ledc.h>

// pin definitions of LED colours
#define BLUE_PIN    2                /*!< ESP32CAM Pin to which the RGB LED BLUE pin is connected*/
#define GREEN_PIN   14               /*!< ESP32CAM Pin to which the RGB LED Green pin is connected*/
#define RED_PIN     15               /*!< ESP32CAM Pin to which the RGB LED Red pin is connected*/

/*! @brief RGB components of colours
 *
 */
typedef struct
{
    unsigned int red;
    unsigned int green;
    unsigned int blue;
}TLED;

// available colours by default
extern TLED LED_BLUE, LED_GREEN, LED_RED, 
            LED_YELLOW, LED_PURPLE, LED_AQUA, LED_WHITE;

/*! @brief Initiliazes the RGB Led Module
 *
 *  @return bool - TRUE if initialization is successful
 */
bool LED_Init(void);

/*! @brief  Turns on the colour passed as argument.
 *          First turns off the LED before turning on the requested colour
 *
 *  @return bool - TRUE if LED is turned on
 */
bool LED_On(const TLED colour);

/*! @brief  Turns off the LED
 *
 *  @return bool - TRUE if LED is turned off
 */
bool LED_Off(void);

/*! @brief  Toggles the LED of the requested colour
 *
 *  @return bool - TRUE if toggled successfully
 */
bool LED_Toggle(const TLED colour);

#endif
/*!
 * @}
 */
