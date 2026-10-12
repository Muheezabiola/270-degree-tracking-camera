#include "camera_driver.h"

#include "config.h"

// ============================================================
//  OV2640 capture on the AI-Thinker ESP32-CAM
//
//  Pin map and frame quality come from config.h. As in the original
//  sketch, the module falls back to QVGA with a single frame buffer
//  when it has no PSRAM.
// ============================================================

namespace
{
camera_config_t buildConfig()
{
    camera_config_t config = {};

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;

    config.pin_pwdn = CAM_PIN_PWDN;
    config.pin_reset = CAM_PIN_RESET;
    config.pin_xclk = CAM_PIN_XCLK;
    config.pin_sccb_sda = CAM_PIN_SIOD;
    config.pin_sccb_scl = CAM_PIN_SIOC;
    config.pin_d0 = CAM_PIN_D0;
    config.pin_d1 = CAM_PIN_D1;
    config.pin_d2 = CAM_PIN_D2;
    config.pin_d3 = CAM_PIN_D3;
    config.pin_d4 = CAM_PIN_D4;
    config.pin_d5 = CAM_PIN_D5;
    config.pin_d6 = CAM_PIN_D6;
    config.pin_d7 = CAM_PIN_D7;
    config.pin_vsync = CAM_PIN_VSYNC;
    config.pin_href = CAM_PIN_HREF;
    config.pin_pclk = CAM_PIN_PCLK;

    config.xclk_freq_hz = CAM_XCLK_FREQ_HZ;
    config.pixel_format = PIXFORMAT_JPEG;

    if (psramFound())
    {
        config.frame_size = static_cast<framesize_t>(CAM_FRAMESIZE_PSRAM);
        config.jpeg_quality = CAM_JPEG_QUALITY_PSRAM;
        config.fb_count = CAM_FB_COUNT_PSRAM;
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.grab_mode = CAMERA_GRAB_LATEST;
    }
    else
    {
        config.frame_size = static_cast<framesize_t>(CAM_FRAMESIZE_NO_PSRAM);
        config.jpeg_quality = CAM_JPEG_QUALITY_NO_PSRAM;
        config.fb_count = CAM_FB_COUNT_NO_PSRAM;
        config.fb_location = CAMERA_FB_IN_DRAM;
    }

    return config;
}
} // namespace

bool cameraDriverInit()
{
    pinMode(PIN_FLASH_LED, OUTPUT);
    digitalWrite(PIN_FLASH_LED, LOW);

    const camera_config_t config = buildConfig();
    const esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("[camera] init failed (0x%04x) - check the ribbon cable and the 5 V supply\n", err);
        return false;
    }

    Serial.printf("[camera] OV2640 ready, psram=%s, frame_size=%d, quality=%d\n",
                  psramFound() ? "yes" : "no",
                  static_cast<int>(config.frame_size),
                  config.jpeg_quality);
    return true;
}

camera_fb_t* cameraDriverCapture()
{
    return esp_camera_fb_get();
}

void cameraDriverRelease(camera_fb_t* frame)
{
    if (frame != nullptr)
    {
        esp_camera_fb_return(frame);
    }
}

void cameraDriverSetLed(bool on)
{
    digitalWrite(PIN_FLASH_LED, on ? HIGH : LOW);
}
