#ifndef _LED_H_
#define _LED_H_

#include <Arduino.h>
#include <esp32-hal-ledc.h>

// pin definitions of LED colours
#define BLUE_PIN    2
#define GREEN_PIN   14
#define RED_PIN     15

// RGB components of colours
typedef struct
{
    unsigned int red;
    unsigned int green;
    unsigned int blue;
}TLED;

// available colours by default
extern TLED LED_BLUE, LED_GREEN, LED_RED, LED_YELLOW;

bool LED_Init(void);

bool LED_On(const TLED colour);

bool LED_Off(void);

bool LED_Toggle(const TLED colour);

#endif