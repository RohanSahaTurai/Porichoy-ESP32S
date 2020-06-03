/*! @file  LED.cpp
 *
 *  @brief Routines for controlling the RGB LED Module
 *
 *  This contains the functions needed for controlling the RGB LED Module
 *
 *  @author Rohan
 *  @date 2020-06-03
 */
#include "LED.hpp"

// setting PWM properties
static const int FREQ = 500;
static const int LEDChannel[3] = {1 , 2, 3};
static const int Resolution = 8;
 
TLED LED_BLUE   = {0  , 0  , 255},
     LED_RED    = {255, 0  , 0  },
     LED_GREEN  = {0  , 255, 0  },
     LED_YELLOW = {255, 255, 0  },
     LED_AQUA   = {0  , 255, 255},
     LED_PURPLE = {80 , 0  ,  80},
     LED_WHITE  = {255, 255, 255};


/*! @brief Initiliazes the RGB Led Module
 *
 *  @return bool - TRUE if initialization is successful
 */
bool LED_Init()
{   
    // Setup three channels for three GPIOS
    for (int i = 0; i < 3; i++)
        ledcSetup(LEDChannel[i], FREQ, Resolution);

    ledcAttachPin(RED_PIN, LEDChannel[0]);
    ledcAttachPin(GREEN_PIN, LEDChannel[1]);
    ledcAttachPin(BLUE_PIN, LEDChannel[2]);
    
    return true;
}

/*! @brief  Turns on the colour passed as argument.
 *          First turns off the LED before turning on the requested colour
 *
 *  @return bool - TRUE if LED is turned on
 */
bool LED_On (const TLED colour)
{
    ledcWrite(LEDChannel[0], colour.red);
    ledcWrite(LEDChannel[1], colour.green);
    ledcWrite(LEDChannel[2], colour.blue);

    return true;
}

/*! @brief  Turns off the LED
 *
 *  @return bool - TRUE if LED is turned off
 */
bool LED_Off (void)
{
    for (int i = 0; i < 3; i ++)
        ledcWrite(LEDChannel[i], 0);
    
    return true;
}

/*! @brief  Toggles the LED of the requested colour
 *
 *  @return bool - TRUE if toggled successfully
 */
bool LED_Toggle (const TLED colour)
{
    for (int i = 0; i < 3; i++)
        if (ledcRead(LEDChannel[i]) != 0)
            {
                LED_Off();
                return true;
            }
    
    LED_On(colour);
    
    return true;
}