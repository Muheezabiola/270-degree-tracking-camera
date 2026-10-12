#pragma once

// Wi-Fi link for the camera node. Deliberately non-fatal: the node
// captures and archives to SD whether or not the network is there.
void wifiLinkInit();

// Keeps the link alive; reconnects (with a backoff) when it drops.
// Call once per loop.
void wifiLinkUpdate();

bool wifiLinkConnected();
