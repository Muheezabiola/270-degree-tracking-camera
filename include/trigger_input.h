#pragma once

#include <Arduino.h>

// Capture-trigger line coming from the servo node.
// Idle HIGH; the servo node holds it LOW to request one photo.
void triggerInputInit();

// Consumes one pending capture request. Requests that arrive while a
// frame is being stored or uploaded are latched by the interrupt and
// picked up on the next call, so none are silently dropped.
bool triggerInputTakeRequest();
