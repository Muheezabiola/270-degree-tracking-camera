#include "camera_node.h"

#include <Arduino.h>

#include "camera_driver.h"
#include "config.h"
#include "image_storage.h"
#include "telegram_client.h"
#include "trigger_input.h"
#include "wifi_link.h"

// ============================================================
//  Camera node: request -> capture -> archive -> upload
//
//  The servo node owns the timing: it aims the camera, then pulls the
//  capture-trigger line low. This side simply takes one JPEG per
//  request, keeps a copy on the SD card, and pushes it to Telegram when
//  the network allows.
// ============================================================

namespace
{
void handleCaptureRequest()
{
    const uint32_t startedMs = millis();
    Serial.println("[camera] capture requested by the servo node");

    cameraDriverSetLed(true);
    delay(CAPTURE_FLASH_WARMUP_MS);

    camera_fb_t* frame = cameraDriverCapture();
    if (frame == nullptr)
    {
        Serial.println("[camera] capture failed");
        cameraDriverSetLed(false);
        return;
    }

    const size_t frameLength = frame->len;
    const char* uploadName = IMAGE_TELEGRAM_FALLBACK;

    char fileName[32] = {0};
    if (imageStorageSave(frame->buf, frameLength, fileName, sizeof(fileName)))
    {
        uploadName = fileName;
    }

    telegramClientSendPhoto(frame->buf, frameLength, uploadName);

    cameraDriverRelease(frame);
    cameraDriverSetLed(false);

    Serial.printf("[camera] %u byte frame handled in %lu ms\n",
                  static_cast<unsigned>(frameLength),
                  static_cast<unsigned long>(millis() - startedMs));
}
} // namespace

void cameraNodeInit()
{
    Serial.printf("\n[%s] starting up\n", NODE_NAME);

    // Armed before the camera comes up, so a request that arrives during
    // the (slow) sensor init is latched instead of lost.
    triggerInputInit();
    wifiLinkInit();
    telegramClientInit();
    imageStorageInit();

    if (!cameraDriverInit())
    {
        Serial.println("[camera] restarting to retry the sensor");
        ESP.restart();
    }

    Serial.println("[camera] ready, waiting for capture requests");
}

void cameraNodeUpdate()
{
    wifiLinkUpdate();

    if (triggerInputTakeRequest())
    {
        handleCaptureRequest();
    }
}
