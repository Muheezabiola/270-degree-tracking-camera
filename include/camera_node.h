#pragma once

// Camera node entry points: configure the hardware, then service one
// capture request per call.
void cameraNodeInit();
void cameraNodeUpdate();
