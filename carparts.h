#ifndef CARPARTS_H
#define CARPARTS_H

#include "geom.h"
#include "materials.h"

void roundRectDisc(float w, float h, float rad);
void teardropDisc(float a, float b, float taper);
void roundRectLoop(float w, float h, float rad, float tubeR);

void pushFrameFacing(Vec3 p, Vec3 n, Vec3 xWant);

void tubeLoop(const Vec3 *p, int n, float r);
void tubeChain(const Vec3 *p, int n, float r);

void underbody(float x0, float x1, float y, float hw);

void archWell(float axleX, float axleY, float archR, float zIn, float zOut, float sill);

void headlampRound(Vec3 p, Vec3 n, float r, float depth, int housingMat, int ringMat);

void lampRect(Vec3 p, Vec3 n, float w, float h, float rad, int lensMat, int frameMat);

void numberPlate(Vec3 p, Vec3 n, float w, float h);

void sideMirror(Vec3 base, float side, int housingMat, int indicator);

void doorHandle(Vec3 p, Vec3 n, float len, int mat);

void wiper(Vec3 pivot, Vec3 tip, Vec3 n);

void tailpipe(Vec3 p, float r, float len, int mat);

void louvre(Vec3 p, Vec3 n, float w, float h, int slatMat);

void seat(float x, float y, float z, float w, float depth, float backH,
          float rakeDeg, int headrest);

void steeringWheel(Vec3 c, Vec3 axis, float r, int rimMat, int spokes);

void driverFigure(Vec3 hip, Vec3 wheelCentre, float wheelR, int goggles);

typedef struct {
    float wsX;
    float rwX;
    float pillarX[3];
    int   nPillars;
    float pillarHalfW;
    float roofNy;
    float edgeNz;
} GlassLayout;

typedef struct {
    float beltY;
    float dashX;
    float dashHW;
    float seatX;
    float rearSeatX;
    float rearBackH;
    float seatZ;
    int   headrests;
    int   dashMat;
    float wheelR;
} CabinSpec;

typedef struct {
    const Loft  *lower;
    const Loft  *glass;
    GlassLayout  layout;
    float (*beltY)(float x);
    float axleX[2], axleY[2], archR[2];
    float hw, sill;
    float wellDepth;
    float doorX[4];
    int   nDoorX;
    float doorTopH, doorBottomH;
    int   roofMat;
    int   beltMat, roofTrimMat, pillarTrimMat;
    CabinSpec cabin;
} ClosedCar;

void drawClosedCarCommon(const ClosedCar *c);
void drawClosedCarGlass(const ClosedCar *c);

void drawCabin(const CabinSpec *c);

#endif
