#pragma once

#include <Arduino.h>

// ============================================================
//  PROJECT: 270° Tracking Camera
//  Single source of truth for pins, timings and credentials.
//
//  One code base builds both boards; the active node is chosen at
//  build time by the PlatformIO environment, which defines
//  NODE_FLAG_SERVO or NODE_FLAG_CAMERA (see platformio.ini).
//
//  Every module includes this header instead of hard-coding a pin
//  or a timing constant.
// ============================================================

// --- Active node selection (from the build environment) ---
#if defined(NODE_FLAG_SERVO)
#define NODE_IS_SERVO 1
#define NODE_NAME "ServoCaptureNode"
#elif defined(NODE_FLAG_CAMERA)
#define NODE_IS_CAMERA 1
#define NODE_NAME "CameraNode"
#else
#error "No node selected: build with -D NODE_FLAG_SERVO or -D NODE_FLAG_CAMERA (see platformio.ini)."
#endif

// --- Serial ---
#define SERIAL_BAUD 115200

// --- Credentials ---
// Real values live in include/secrets.h, which is git-ignored.
// Copy include/secrets.example.h to include/secrets.h first.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "Missing include/secrets.h - copy include/secrets.example.h to include/secrets.h and fill in your credentials."
#endif

// ============================================================
//  SERVO / CAPTURE NODE CONFIGURATION
//  ESP32 DevKit (board = esp32dev)
//
//  Watches two PIR sensors, aims the pan servo at the side that
//  saw the motion, asks the camera node for a photo over the
//  capture-trigger line, then reports the event by SMS.
// ============================================================
#if defined(NODE_IS_SERVO)

// --- PIR sensors (HIGH while motion is present) ---
// GPIO34/35 are input-only pins, which suits them perfectly.
// The mapping follows the original sketch: the sensor that aims
// the camera left (45°) sits on GPIO35 and the one that aims it
// right (135°) sits on GPIO34. Swap the two defines if your
// sensors are physically wired the other way round.
#define PIN_PIR_LEFT 35
#define PIN_PIR_RIGHT 34

// --- Pan servo ---
#define PIN_SERVO 23
#define SERVO_ANGLE_LEFT 45
#define SERVO_ANGLE_CENTER 90
#define SERVO_ANGLE_RIGHT 135
#define SERVO_MIN_PULSE_US 500
#define SERVO_MAX_PULSE_US 2400
#define SERVO_SETTLE_MS 1000 // Travel time allowed before a capture

// --- Capture-trigger line to the camera node ---
// Idle HIGH. The node holds it LOW for CAPTURE_TRIGGER_HOLD_MS to
// request one photo; the camera node reacts to the falling edge.
#define PIN_CAPTURE_TRIGGER 32
#define CAPTURE_TRIGGER_HOLD_MS 2000

// --- SIM800L GSM modem (UART1) ---
#define PIN_SIM800_RX 16 // ESP32 RX <- SIM800 TX
#define PIN_SIM800_TX 17 // ESP32 TX -> SIM800 RX
#define SIM800_BAUD 9600
#define SIM800_BOOT_DELAY_MS 3000 // Modem network registration after power-up
#define SIM800_AT_TIMEOUT_MS 2000
#define SIM800_CMD_TIMEOUT_MS 1000
#define SIM800_SEND_TIMEOUT_MS 10000

// --- Alerting ---
#define ALERT_SMS_MESSAGE "Motion detected!"
#define ALERT_COOLDOWN_MS 10000 // Ignore motion while a report is in flight

#endif // NODE_IS_SERVO

// ============================================================
//  CAMERA NODE CONFIGURATION
//  AI-Thinker ESP32-CAM (board = esp32cam)
//
//  Waits for a capture request from the servo node, takes one
//  JPEG, archives it on the micro-SD card and pushes it to a
//  Telegram chat. Wi-Fi is optional: without a link the node still
//  captures and archives.
// ============================================================
#if defined(NODE_IS_CAMERA)

// --- Capture-trigger line from the servo node ---
// Wired to the servo node's GPIO32 (PIN_CAPTURE_TRIGGER there).
// This is the pin the original camera sketch read as its trigger. It is
// also UART0 RX, so the console is output-only while the wire is
// attached and the link must be broken to flash this board. GPIO13 is
// free (the card runs in 1-bit mode) if you would rather have it back.
#define PIN_CAPTURE_TRIGGER 3

// --- Illumination LED ---
#define PIN_FLASH_LED 4

// --- Camera module pin map: AI-Thinker ESP32-CAM (OV2640) ---
#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 // Not exposed on this board
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27
#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22
#define CAM_XCLK_FREQ_HZ 20000000

// --- Frame quality, as in the original sketch: VGA at quality 10
//     with two frame buffers when PSRAM is present, QVGA otherwise.
//     Values are FRAMESIZE_* / JPEG quality (0-63, lower = better).
#define CAM_FRAMESIZE_PSRAM 8       // FRAMESIZE_VGA
#define CAM_JPEG_QUALITY_PSRAM 10
#define CAM_FB_COUNT_PSRAM 2
#define CAM_FRAMESIZE_NO_PSRAM 5    // FRAMESIZE_QVGA
#define CAM_JPEG_QUALITY_NO_PSRAM 12
#define CAM_FB_COUNT_NO_PSRAM 1
#define CAPTURE_FLASH_WARMUP_MS 200 // LED settling time before the shot

// --- micro-SD archive ---
// The AI-Thinker slot is wired to SDMMC 4-bit (GPIO 2/4/12/13/14/15).
// 1-bit mode is used so GPIO 4 stays available for the illumination LED.
#define SD_MOUNT_POINT "/sdcard"
#define SD_1BIT_MODE 1

#if !SD_1BIT_MODE
#warning "4-bit SD mode also claims GPIO 4, which PIN_FLASH_LED uses - reassign it in this file."
#endif

#define IMAGE_NAME_PREFIX "IMG_"
#define IMAGE_NAME_DIGITS 4
#define IMAGE_TELEGRAM_FALLBACK "esp32cam.jpg"

// --- Wi-Fi (optional: captures are archived to SD without it) ---
#define WIFI_CONNECT_TIMEOUT_MS 15000
#define WIFI_RETRY_INTERVAL_MS 30000

// --- Telegram bot ---
#define TELEGRAM_HOST "api.telegram.org"
#define TELEGRAM_PORT 443
#define TELEGRAM_BOUNDARY "ESP32CAM"
#define TELEGRAM_RESPONSE_TIMEOUT_MS 10000

#endif // NODE_IS_CAMERA
