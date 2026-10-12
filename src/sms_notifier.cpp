#include "sms_notifier.h"

#include <Arduino.h>
#include <HardwareSerial.h>

#include "config.h"

// ============================================================
//  SIM800L alerting over UART1
//
//  The AT sequence is the one the original sketch used. The one
//  difference: every step now waits for the modem's reply (with the
//  original delays as timeouts) instead of sleeping blindly, so a
//  missing SIM or a bad signal is reported instead of looking like a
//  successful send.
// ============================================================

namespace
{
HardwareSerial g_sim800(1);

void drainModem()
{
    while (g_sim800.available() > 0)
    {
        g_sim800.read();
    }
}

bool waitForResponse(const char* token, uint32_t timeoutMs)
{
    const uint32_t deadline = millis() + timeoutMs;
    String response;
    response.reserve(128);

    while (millis() < deadline)
    {
        while (g_sim800.available() > 0)
        {
            response += static_cast<char>(g_sim800.read());
            if (response.indexOf(token) >= 0)
            {
                return true;
            }
        }
    }

    response.replace("\r", " ");
    response.replace("\n", " ");
    Serial.printf("[sms] expected \"%s\" within %lu ms, modem said: %s\n",
                  token, static_cast<unsigned long>(timeoutMs), response.c_str());
    return false;
}

bool sendCommand(const char* command, uint32_t timeoutMs)
{
    drainModem();
    g_sim800.println(command);
    return waitForResponse("OK", timeoutMs);
}
} // namespace

void smsNotifierInit()
{
    // UART1: RX = PIN_SIM800_RX, TX = PIN_SIM800_TX
    g_sim800.begin(SIM800_BAUD, SERIAL_8N1, PIN_SIM800_RX, PIN_SIM800_TX);

    Serial.println("[sms] waiting for SIM800L to register");
    delay(SIM800_BOOT_DELAY_MS);

    if (!sendCommand("AT", SIM800_AT_TIMEOUT_MS))
    {
        Serial.println("[sms] modem did not answer AT - check wiring and the 4 V supply");
    }
}

bool smsNotifierSend(const char* message)
{
    if (!sendCommand("AT", SIM800_AT_TIMEOUT_MS) ||
        !sendCommand("AT+CSCS=\"GSM\"", SIM800_CMD_TIMEOUT_MS) ||   // 7-bit alphabet
        !sendCommand("AT+CMGF=1", SIM800_CMD_TIMEOUT_MS) ||         // SMS text mode
        !sendCommand("AT+CSMP=17,167,0,0", SIM800_CMD_TIMEOUT_MS))  // No class, no expiry
    {
        Serial.println("[sms] modem setup failed, alert not sent");
        return false;
    }

    drainModem();
    g_sim800.print("AT+CMGS=\"");
    g_sim800.print(ALERT_PHONE_NUMBER);
    g_sim800.println("\"");

    if (!waitForResponse(">", SIM800_CMD_TIMEOUT_MS * 3)) // ">" = send the body now
    {
        Serial.println("[sms] modem never offered the message body");
        g_sim800.write(0x1B); // ESC aborts a half-sent message
        return false;
    }

    g_sim800.print(message);
    g_sim800.write(0x1A); // Ctrl+Z sends

    const bool sent = waitForResponse("+CMGS", SIM800_SEND_TIMEOUT_MS);
    Serial.println(sent ? "[sms] alert sent" : "[sms] alert failed");
    return sent;
}
