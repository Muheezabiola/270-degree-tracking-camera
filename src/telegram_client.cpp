#include "telegram_client.h"

#include <WiFiClientSecure.h>

#include "config.h"
#include "wifi_link.h"

// ============================================================
//  Telegram photo upload
//
//  Same multipart/form-data request the original sketch sent, with the
//  HTTP status line checked so a rejected upload is reported instead of
//  assumed successful.
// ============================================================

namespace
{
WiFiClientSecure g_client;

bool readStatusLine(String& line)
{
    const uint32_t deadline = millis() + TELEGRAM_RESPONSE_TIMEOUT_MS;
    while (millis() < deadline)
    {
        if (g_client.available() > 0)
        {
            line = g_client.readStringUntil('\n');
            return true;
        }
    }
    return false;
}
} // namespace

void telegramClientInit()
{
    // The bot API needs TLS but cannot be certificate-pinned on a device
    // without a clock, which is why the original sketch used setInsecure().
    g_client.setInsecure();
}

bool telegramClientSendPhoto(const uint8_t* jpeg, size_t length, const char* filename)
{
    if (!wifiLinkConnected())
    {
        Serial.println("[telegram] no Wi-Fi link, upload skipped");
        return false;
    }

    if (!g_client.connect(TELEGRAM_HOST, TELEGRAM_PORT))
    {
        Serial.println("[telegram] TLS connection failed");
        return false;
    }

    String head;
    head.reserve(256);
    head += "--" TELEGRAM_BOUNDARY "\r\n";
    head += "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" TELEGRAM_CHAT_ID "\r\n";
    head += "--" TELEGRAM_BOUNDARY "\r\n";
    head += "Content-Disposition: form-data; name=\"photo\"; filename=\"";
    head += filename;
    head += "\"\r\nContent-Type: image/jpeg\r\n\r\n";
    const String tail = "\r\n--" TELEGRAM_BOUNDARY "--\r\n";

    const size_t totalLength = head.length() + length + tail.length();

    g_client.printf("POST /bot%s/sendPhoto HTTP/1.1\r\n", TELEGRAM_BOT_TOKEN);
    g_client.printf("Host: %s\r\n", TELEGRAM_HOST);
    g_client.printf("Content-Type: multipart/form-data; boundary=%s\r\n", TELEGRAM_BOUNDARY);
    g_client.printf("Content-Length: %u\r\n\r\n", static_cast<unsigned>(totalLength));

    g_client.print(head);
    g_client.write(jpeg, length);
    g_client.print(tail);

    String statusLine;
    const bool answered = readStatusLine(statusLine);
    const bool uploaded = answered && statusLine.indexOf(" 200 ") > 0;

    if (answered)
    {
        statusLine.trim();
        Serial.printf("[telegram] %s\n", statusLine.c_str());
    }
    else
    {
        Serial.println("[telegram] no response from the bot API");
    }

    g_client.stop();
    return uploaded;
}
