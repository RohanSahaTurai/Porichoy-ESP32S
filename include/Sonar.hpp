#ifndef SONAR_H
#define SONAR_H

#include <Arduino.h>

#define TRIGGER_PIN 12
#define ECHO_PIN    16

#define TRIG_TIME_MICROSECS 10

bool Sonar_Init();

// returns distance in cm
// return -1 if measurement is invalid
int Sonar_GetDistance();

#endif