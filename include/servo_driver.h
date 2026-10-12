#pragma once

// Pan servo used to aim the camera at the side that detected motion.
void servoDriverInit();

// Moves the servo to the given angle (degrees). Returns immediately;
// the caller decides how long to let it travel.
void servoDriverWrite(int angle);
