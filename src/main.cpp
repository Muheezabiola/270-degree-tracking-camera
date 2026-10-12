#include <Arduino.h>

#include "config.h"

#if defined(NODE_IS_SERVO)
#include "servo_node.h"
#elif defined(NODE_IS_CAMERA)
#include "camera_node.h"
#endif

// ============================================================
//  270° Tracking Camera — firmware entry point
//
//  Both MCUs are built from this single entry point; the active node
//  is chosen at build time (see config.h and platformio.ini). It stays
//  a thin orchestrator: it hands control to whichever node module the
//  environment selected, so no board-specific detail leaks in here.
//
//  Servo node (ESP32 DevKit)
//    pir_sensor       dual PIR sampling
//    servo_driver     pan servo aiming
//    capture_trigger  asks the camera node for a photo
//    sms_notifier     SIM800L alert over UART1
//    servo_node       the motion -> aim -> capture -> alert flow
//
//  Camera node (AI-Thinker ESP32-CAM)
//    trigger_input    capture requests from the servo node
//    camera_driver    OV2640 capture and the illumination LED
//    image_storage    micro-SD archive of every frame
//    wifi_link        optional Wi-Fi link with retry
//    telegram_client  multipart photo upload
//    camera_node      the request -> capture -> archive -> upload flow
// ============================================================

void setup()
{
    Serial.begin(SERIAL_BAUD);

#if defined(NODE_IS_SERVO)
    servoNodeInit();
#elif defined(NODE_IS_CAMERA)
    cameraNodeInit();
#endif
}

void loop()
{
#if defined(NODE_IS_SERVO)
    servoNodeUpdate();
#elif defined(NODE_IS_CAMERA)
    cameraNodeUpdate();
#endif
}
