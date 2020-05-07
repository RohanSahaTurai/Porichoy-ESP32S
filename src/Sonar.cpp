#include <Sonar.hpp>

bool Sonar_Init()
{
    pinMode(TRIGGER_PIN, OUTPUT);
    digitalWrite(TRIGGER_PIN, LOW);

    pinMode(ECHO_PIN, INPUT);

    return true;
}

int Sonar_GetDistance()
{
    // Send 10µs pulse
    digitalWrite(TRIGGER_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGGER_PIN, LOW);

    // Read pulse duration in microseconds
    // Convert to cm 
    // Speed of sound = 340 m/s; 1/speed = 29.1 µs/cm
    // duration in cm = (duration/2) * speed = duration / 58
    int duration = pulseIn(ECHO_PIN, HIGH) / 58;

    // Maximum range is 400 cm, so anything greater is invalid
    if (duration >= 0 && duration <= 400)
        return duration;
    else
        return -1;
}