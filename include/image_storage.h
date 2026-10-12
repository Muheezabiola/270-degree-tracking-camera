#pragma once

#include <Arduino.h>

// Micro-SD archive of every captured frame.
// Mounting is best-effort: capturing keeps working without a card.
bool imageStorageInit();

bool imageStorageReady();

// Writes one JPEG and fills outName with the file name it was stored
// under (e.g. "IMG_0007.JPG"), so the same name can be reused for the
// upload. Returns false when nothing was written.
bool imageStorageSave(const uint8_t* data, size_t length, char* outName, size_t outNameSize);
