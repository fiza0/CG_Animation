#ifndef WHEELS_H
#define WHEELS_H

#include "scene.h"

enum RimStyle {
    RIM_WIRE,        /* 1886, 1936: laced wire spokes             */
    RIM_ARTILLERY,   /* 1901: thick wooden spokes, brass cap      */
    RIM_STEEL_CAP,   /* 1954, 1963: steel disc, chrome cap + ring */
    RIM_RIBBED_CAP,  /* 1976: full chrome cap with radial ribs    */
    RIM_15HOLE,      /* 1982                                      */
    RIM_8HOLE,       /* 1995                                      */
    RIM_5TWIN,       /* 2002: five twin spokes, brake visible     */
    RIM_10SPOKE,     /* 2013                                      */
    RIM_AERO         /* 2021: nearly closed aero disc             */
};

typedef struct {
    float radius;     /* outer tyre radius (m)                    */
    float width;      /* tyre width                               */
    float rimRadius;  /* where the tyre meets the rim             */
    int   style;
    int   rimMat;     /* main rim material (wire and steel rims)  */
    int   treads;     /* draw tread grooves (modern tyres)        */
} WheelSpec;

/* Each wheel has a display list that turns with the wheel and, for
 * styles with a brake calliper, one that stays still. */
typedef struct { GLuint spin, fixed; } WheelLists;

/* Builds the lists. The car's colours must already be set with
 * setCarColors (painted steel wheels use the body colour). */
void buildWheel(const WheelSpec *w, WheelLists *out);

/* Draw at the current origin (the wheel centre), outer face to +z.
 * farSide mirrors it so the outer face points to -z instead. */
void drawWheel(const WheelLists *wl, float angleDeg, int farSide);

#endif
