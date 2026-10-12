#include "capture_trigger.h"

#include <Arduino.h>

#include "config.h"

void captureTriggerInit()
{
    pinMode(PIN_CAPTURE_TRIGGER, OUTPUT);
    digitalWrite(PIN_CAPTURE_TRIGGER, HIGH); // Idle level: no request
}

void captureTriggerPulse()
{
    digitalWrite(PIN_CAPTURE_TRIGGER, LOW);
    delay(CAPTURE_TRIGGER_HOLD_MS);
    digitalWrite(PIN_CAPTURE_TRIGGER, HIGH);
}
