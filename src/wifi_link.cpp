#include "wifi_link.h"

#include <WiFi.h>

#include "config.h"

// ============================================================
//  Wi-Fi link
//
//  The original sketch blocked forever until the access point came up.
//  Here the network is optional: a failed connection is logged, the
//  node keeps capturing to the SD card, and reconnection is retried on
//  an interval instead of stalling the capture loop.
// ============================================================

namespace
{
uint32_t g_nextAttemptMs = 0;

void connectOnce()
{
    Serial.printf("[wifi] connecting to \"%s\"\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    const uint32_t deadline = millis() + WIFI_CONNECT_TIMEOUT_MS;
    while (WiFi.status() != WL_CONNECTED && millis() < deadline)
    {
        delay(250);
        Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        g_nextAttemptMs = 0;
        Serial.printf("[wifi] connected, ip=%s rssi=%d dBm\n",
                      WiFi.localIP().toString().c_str(),
                      WiFi.RSSI());
    }
    else
    {
        g_nextAttemptMs = millis() + WIFI_RETRY_INTERVAL_MS;
        Serial.println("[wifi] no link - captures are still archived to SD");
    }
}
} // namespace

void wifiLinkInit()
{
    WiFi.mode(WIFI_STA);
    // Disabling modem sleep keeps the TLS upload from stalling mid-request.
    WiFi.setSleep(false);
    connectOnce();
}

void wifiLinkUpdate()
{
    if (WiFi.status() == WL_CONNECTED || millis() < g_nextAttemptMs)
    {
        return;
    }

    connectOnce();
}

bool wifiLinkConnected()
{
    return WiFi.status() == WL_CONNECTED;
}
