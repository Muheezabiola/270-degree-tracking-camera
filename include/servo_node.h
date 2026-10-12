#pragma once

// Servo node entry points: configure the hardware, then run one
// motion -> aim -> capture -> alert cycle per call.
void servoNodeInit();
void servoNodeUpdate();
