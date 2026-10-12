#pragma once

// Capture-trigger line towards the camera node.
// The line idles HIGH and is pulled LOW to request one photo.
void captureTriggerInit();

// Requests a single photo: drives the line LOW for
// CAPTURE_TRIGGER_HOLD_MS, then releases it. Blocking by design -
// the hold time is what the camera node watches for.
void captureTriggerPulse();
