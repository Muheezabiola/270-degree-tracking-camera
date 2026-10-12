#include "trigger_input.h"

#include "config.h"

// ============================================================
//  Capture requests coming from the servo node
//
//  The line idles HIGH and is pulled LOW to request one photo. An
//  interrupt is used rather than polling because a single request can
//  arrive while the node is busy writing the previous frame to the SD
//  card or uploading it - polling then would miss the edge and lose
//  the shot.
// ============================================================

namespace
{
volatile bool g_requestPending = false;

void IRAM_ATTR onTriggerFalling()
{
    g_requestPending = true;
}
} // namespace

void triggerInputInit()
{
    // INPUT_PULLUP keeps a disconnected wire from looking like a request.
    pinMode(PIN_CAPTURE_TRIGGER, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_CAPTURE_TRIGGER), onTriggerFalling, FALLING);
}

bool triggerInputTakeRequest()
{
    noInterrupts();
    const bool pending = g_requestPending;
    g_requestPending = false;
    interrupts();
    return pending;
}
