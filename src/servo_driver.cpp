#include <ESP32Servo.h>

#include "config.h"
#include "servo_driver.h"

namespace
{
Servo g_servo;
} // namespace

void servoDriverInit()
{
    g_servo.setPeriodHertz(50); // Standard analog servo frame rate
    g_servo.attach(PIN_SERVO, SERVO_MIN_PULSE_US, SERVO_MAX_PULSE_US);
    g_servo.write(SERVO_ANGLE_CENTER);
}

void servoDriverWrite(int angle)
{
    g_servo.write(angle);
}
