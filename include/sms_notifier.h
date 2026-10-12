#pragma once

// SIM800L alerting. The modem lives on UART1 (pins from config.h).
void smsNotifierInit();

// Sends one SMS to ALERT_PHONE_NUMBER. Returns true only when the
// modem acknowledged the message with +CMGS.
bool smsNotifierSend(const char* message);
