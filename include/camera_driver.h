#pragma once

#include <Arduino.h>
#include "esp_camera.h"

// Initialises the OV2640 and the illumination LED. Returns false when
// the sensor could not be brought up.
bool cameraDriverInit();

// Grabs one JPEG frame. The caller owns the buffer and must hand it
// back with cameraDriverRelease(). Returns nullptr on failure.
camera_fb_t* cameraDriverCapture();

void cameraDriverRelease(camera_fb_t* frame);

// Illumination / status LED.
void cameraDriverSetLed(bool on);
