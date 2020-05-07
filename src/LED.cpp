#include "LED.hpp"

// setting PWM properties
static const int FREQ = 500;
static const int LEDChannel[3] = {0 , 1, 2};
static const int Resolution = 8;

// RGB colour oponent of each colour
TLED LED_BLUE   = {0, 0, 255},
     LED_RED    = {255, 0, 0},
     LED_GREEN  = {0, 255, 0},
     LED_YELLOW = {255, 255, 51};

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

bool LED_On (const TLED colour)
{
    ledcWrite(LEDChannel[0], colour.red);
    ledcWrite(LEDChannel[1], colour.green);
    ledcWrite(LEDChannel[2], colour.blue);

    return true;
}

bool LED_Off (void)
{
    for (int i = 0; i < 3; i ++)
        ledcWrite(LEDChannel[i], 0);
    
    return true;
}

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