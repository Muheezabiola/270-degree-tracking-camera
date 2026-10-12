#pragma once

#include <Arduino.h>

// Telegram bot uploads (multipart/form-data over TLS).
void telegramClientInit();

// Posts one JPEG to the configured chat. Returns true on an HTTP 200
// response from the Telegram API.
bool telegramClientSendPhoto(const uint8_t* jpeg, size_t length, const char* filename);
