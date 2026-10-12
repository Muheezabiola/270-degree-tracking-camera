#include "pir_sensor.h"

#include "config.h"

// HC-SR501 style modules idle LOW and go HIGH while motion is present.
void pirSensorInit()
{
    pinMode(PIN_PIR_LEFT, INPUT);
    pinMode(PIN_PIR_RIGHT, INPUT);
}

PirReading pirSensorRead()
{
    PirReading reading;
    reading.left = digitalRead(PIN_PIR_LEFT) == HIGH;
    reading.right = digitalRead(PIN_PIR_RIGHT) == HIGH;
    return reading;
}
