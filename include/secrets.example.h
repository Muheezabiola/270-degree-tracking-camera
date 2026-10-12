#pragma once

// ============================================================
//  CREDENTIALS TEMPLATE
//
//  Copy this file, then fill in the real values:
//
//      copy include\secrets.example.h include\secrets.h
//
//  include/secrets.h is listed in .gitignore and must never be
//  committed or pushed to GitHub.
//
//  Which node needs what:
//    servo_node   -> ALERT_PHONE_NUMBER
//    camera_node  -> WIFI_SSID, WIFI_PASSWORD,
//                    TELEGRAM_BOT_TOKEN, TELEGRAM_CHAT_ID
// ============================================================

// --- Wi-Fi (camera node) ---
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// --- Telegram bot (camera node) ---
// Create a bot with @BotFather, then read the chat id from
// https://api.telegram.org/bot<token>/getUpdates
// Treat the token like a password: rotate it if it ever leaks.
#define TELEGRAM_BOT_TOKEN "123456789:replace_me"
#define TELEGRAM_CHAT_ID "replace_me"

// --- SMS alert destination (servo node) ---
// E.164 format, e.g. "+2348012345678"
#define ALERT_PHONE_NUMBER "+10000000000"
