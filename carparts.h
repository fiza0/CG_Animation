#ifndef CARPARTS_H
#define CARPARTS_H

#include "geom.h"
#include "materials.h"

/* ------------------------------------------------------------------ */
/* Parts shared by many cars. Everything is drawn in the car's own     */
/* coordinates (origin on the road, between the axles).                */
/* ------------------------------------------------------------------ */

/* Flat shapes in the local xy plane facing +z (use inside pushFrame). */
void roundRectDisc(float w, float h, float rad);
void teardropDisc(float a, float b, float taper);   /* wide end at +x */
void roundRectLoop(float w, float h, float rad, float tubeR);   /* trim ring */

/* pushFrame, then turned half a turn if needed so the local x axis
 * points roughly along xWant (for shapes that are not symmetric). */
void pushFrameFacing(Vec3 p, Vec3 n, Vec3 xWant);

/* Loop of tube segments through points (closed), with joints. */
void tubeLoop(const Vec3 *p, int n, float r);
/* Open chain of tube segments, with joints. */
void tubeChain(const Vec3 *p, int n, float r);

/* Matte panel under the car, facing down. */
void underbody(float x0, float x1, float y, float hw);

/* Wheel well: dark liner half-ring on each side between |z| = zIn and
 * zOut, and a dark block filling the space between the two wells. */
void archWell(float axleX, float axleY, float archR, float zIn, float zOut, float sill);

/* Round headlamp: reflector housing, trim ring and a glowing lens dome.
 * p is the centre of the lens rim, n the direction the lamp faces. */
void headlampRound(Vec3 p, Vec3 n, float r, float depth, int housingMat, int ringMat);

/* Flat lens with a frame around it (lamps, indicators, reflectors). */
void lampRect(Vec3 p, Vec3 n, float w, float h, float rad, int lensMat, int frameMat);

/* Blank number plate (no text). */
void numberPlate(Vec3 p, Vec3 n, float w, float h);

/* Door mirror on a stalk from base, on side +1 or -1. */
void sideMirror(Vec3 base, float side, int housingMat, int indicator);

void doorHandle(Vec3 p, Vec3 n, float len, int mat);

/* Wiper lying on the glass from pivot to tip; n is the glass normal. */
void wiper(Vec3 pivot, Vec3 tip, Vec3 n);

/* Exhaust tailpipe ending at p, pointing backwards (-x). */
void tailpipe(Vec3 p, float r, float len, int mat);

/* Louvre: dark slot with a slanted slat over it, on a surface. */
void louvre(Vec3 p, Vec3 n, float w, float h, int slatMat);

/* Seat: cushion top at y, back pivot at x (back leans towards -x). */
void seat(float x, float y, float z, float w, float depth, float backH,
          float rakeDeg, int headrest);

/* Steering wheel centred at c, facing along axis (towards the driver). */
void steeringWheel(Vec3 c, Vec3 axis, float r, int rimMat, int spokes);

/* Seated driver: hip point and steering wheel centre/radius. */
void driverFigure(Vec3 hip, Vec3 wheelCentre, float wheelR, int goggles);

/* ------------------------------------------------------------------ */
/* Closed cars: a lower body loft plus a glasshouse loft whose quads   */
/* are split into paint (roof, pillars) and glass.                     */
/* ------------------------------------------------------------------ */
typedef struct {
    float wsX;            /* windscreen zone: x above this            */
    float rwX;            /* rear window zone: x below this           */
    float pillarX[3];     /* B and C pillars                          */
    int   nPillars;
    float pillarHalfW;
    float roofNy;         /* side quads facing up more than this: roof */
    float edgeNz;         /* screen quads facing sideways: pillars     */
} GlassLayout;

typedef struct {
    float beltY;          /* reference height for the interior        */
    float dashX;          /* dashboard centre                         */
    float dashHW;
    float seatX;          /* front seat back pivot                    */
    float rearSeatX;      /* rear seat back pivot (0 = none)          */
    float rearBackH;
    float seatZ;          /* |z| of the front seats; driver at -seatZ */
    int   headrests;
    int   dashMat;
    float wheelR;
} CabinSpec;

typedef struct {
    const Loft  *lower;
    const Loft  *glass;
    GlassLayout  layout;
    float (*beltY)(float x);      /* lower body top along x             */
    float axleX[2], axleY[2], archR[2];
    float hw, sill;               /* body half-width, lowest body y      */
    float wellDepth;              /* how far the wheel wells reach in    */
    float doorX[4];               /* door gap positions along x          */
    int   nDoorX;
    float doorTopH, doorBottomH;  /* door gaps, as loftSide h values     */
    int   roofMat;                /* material of roof and pillars        */
    int   beltMat, roofTrimMat, pillarTrimMat;
    CabinSpec cabin;
} ClosedCar;

/* Body, glasshouse (painted parts), wells, underbody, window trim,
 * door gaps, interior. Then each car adds its own features. */
void drawClosedCarCommon(const ClosedCar *c);
/* The see-through parts of the glasshouse, for the glass pass. */
void drawClosedCarGlass(const ClosedCar *c);

void drawCabin(const CabinSpec *c);

#endif
