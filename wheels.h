#ifndef WHEELS_H
#define WHEELS_H

#include "scene.h"

enum RimStyle {
    RIM_WIRE,
    RIM_ARTILLERY,
    RIM_STEEL_CAP,
    RIM_RIBBED_CAP,
    RIM_15HOLE,
    RIM_8HOLE,
    RIM_5TWIN,
    RIM_10SPOKE,
    RIM_AERO
};

typedef struct {
    float radius;
    float width;
    float rimRadius;
    int   style;
    int   rimMat;
    int   treads;
} WheelSpec;

typedef struct { GLuint spin, fixed; } WheelLists;

void buildWheel(const WheelSpec *w, WheelLists *out);

void drawWheel(const WheelLists *wl, float angleDeg, int farSide);

#endif
