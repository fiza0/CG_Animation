#ifndef HUD_H
#define HUD_H

#include "scene.h"

/* Draws the 2D text overlay on top of the finished 3D frame.
 * Call last in display(). speed is in units (metres) per second. */
void drawHUD(int winW, int winH, float speed, int paused, int autoOrbit);

#endif
