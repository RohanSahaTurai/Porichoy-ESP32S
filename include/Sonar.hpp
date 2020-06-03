/*! @file  Sonar.hpp
 *
 *  @brief Routines for operating the HC_SR04 sonar module
 *
 *  This contains the functions needed for operating the HC_SR04 sonar module
 *
 *  @author Rohan
 *  @date 2020-06-03
 */
/*!
 * @addtogroup Sonar_Module Sonar Module documentation
 * @{
 *
 *  This contains the functions needed for operating the HC_SR04 sonar module
 *
 */
#ifndef SONAR_H
#define SONAR_H

#include <Arduino.h>

#define TRIGGER_PIN 12              /*!< ESP32CAM Pin connected to the HC_SR04 TRIG Pin*/
#define ECHO_PIN    16              /*!< ESP32CAM Pin connected to the HC_SR04 ECHO Pin*/

#define TRIG_TIME_MICROSECS 10      /*!< Delay between trigger and echo read (in microseconds)*/

/*! @brief Initializes the HC_SR04 Sonar module
 *
 *  @return bool - TRUE if initialization is successful
 */
bool Sonar_Init();

/*! @brief Measures the distance to the object near the device
 *
 *  @return int - distance in cm; returns -1 if measurement is invalid
 */
int Sonar_GetDistance();

#endif
/*!
 * @}
 */
