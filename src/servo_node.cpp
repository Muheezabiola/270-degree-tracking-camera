#include "config.h"

#include "capture_trigger.h"
#include "pir_sensor.h"
#include "servo_driver.h"
#include "servo_node.h"
#include "sms_notifier.h"

// ============================================================
//  Servo node: motion -> aim -> capture -> alert
//
//  Mirrors the original single-board sketch - the PIR pair decides
//  where to look, the servo sweeps that side, the capture-trigger
//  line asks the camera node for a photo, and one SMS reports the
//  event.
//
//  The only change is that the trailing 10 s delay became a
//  non-blocking cooldown, so the PIR state keeps printing while the
//  node refuses to fire a second burst of alerts.
// ============================================================

namespace
{
uint32_t g_cooldownUntilMs = 0;

// Aims the servo and gives it time to travel before the shot.
void aimAndSettle(int angle)
{
    servoDriverWrite(angle);
    delay(SERVO_SETTLE_MS);
}

// One "look here, take a photo" step.
void captureAt(int angle)
{
    aimAndSettle(angle);
    captureTriggerPulse();
}
} // namespace

void servoNodeInit()
{
    Serial.printf("\n[%s] starting up\n", NODE_NAME);

    pirSensorInit();
    servoDriverInit();
    captureTriggerInit();
    smsNotifierInit();

    Serial.println("[servo] watching for motion");
}

void servoNodeUpdate()
{
    const PirReading reading = pirSensorRead();

    Serial.printf("[pir] left=%d right=%d\n", reading.left ? 1 : 0, reading.right ? 1 : 0);

    if (millis() < g_cooldownUntilMs)
    {
        return;
    }

    if (!reading.left && !reading.right)
    {
        aimAndSettle(SERVO_ANGLE_CENTER);
        return;
    }

    if (reading.left && reading.right)
    {
        // Both sides: sweep left to right and take a photo at each stop.
        captureAt(SERVO_ANGLE_LEFT);
        captureAt(SERVO_ANGLE_CENTER);
        captureAt(SERVO_ANGLE_RIGHT);
    }
    else if (reading.left)
    {
        captureAt(SERVO_ANGLE_LEFT);
    }
    else
    {
        captureAt(SERVO_ANGLE_RIGHT);
    }

    smsNotifierSend(ALERT_SMS_MESSAGE);
    g_cooldownUntilMs = millis() + ALERT_COOLDOWN_MS;
}
