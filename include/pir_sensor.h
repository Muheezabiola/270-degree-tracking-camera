#pragma once

#include <Arduino.h>

// Raw state of the two PIR sensors.
struct PirReading
{
    bool left;
    bool right;
};

void pirSensorInit();

// Samples both sensors. true means motion is currently present.
PirReading pirSensorRead();
