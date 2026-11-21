#ifndef CARS_H
#define CARS_H

#include "scene.h"

/* The car drives along the centre of the far lane. */
#define CAR_LANE_Z  -2.0f

/* ------------------------------------------------------------------ */
/* How a car is described                                              */
/*                                                                     */
/* Every car body is a 2D side silhouette (x along the car, y up) that */
/* is pushed out ("extruded") across the car's width. One generic      */
/* renderer handles all eras; the eras differ only in their data.      */
/* ------------------------------------------------------------------ */

/* Style of the edge that LEAVES a profile point (point i to point i+1).
 * The style colours the strip of side wall built on that edge, which
 * gives windscreens, bumpers and lamps without any extra geometry. */
enum SegStyle {
    SEG_BODY,     /* body colour                                       */
    SEG_GLASS,    /* dark blue-grey: windscreen and rear window        */
    SEG_CHROME,   /* light grey: chrome bumpers and radiator shells    */
    SEG_TRIM,     /* dark grey: plastic bumpers on later cars          */
    SEG_LAMP,     /* pale yellow at the front, red at the rear         */
    SEG_ARCH      /* replaced at start-up by a wheel-arch curve         */
};

enum WheelStyle { WIRE_SPOKES, WOOD_SPOKES, HUBCAP, ALLOY };

/* One silhouette point plus the style of the edge that starts here. */
typedef struct {
    float x, y;
    int   seg;
} ProfilePt;

typedef struct CarModel CarModel;

struct CarModel {
    const char *year;              /* "1954"                              */
    const char *name;              /* "300 SL"                            */

    /* Body silhouette: clockwise seen from +z, x = 0 at the car centre,
     * y = 0 on the road. */
    const ProfilePt *profile;
    int              numPoints;
    float            halfWidth;    /* body spans z = -halfWidth..+halfWidth */
    float            bodyColor[3];

    /* Side window outline (style field unused), may be NULL. A body-
     * coloured pillar of width pillarW is drawn at pillarX (0 = none). */
    const ProfilePt *window;
    int              numWindow;
    float            pillarX, pillarW;

    /* Optional second extrusion for separate fenders and running
     * boards (1936). Drawn on both sides between fenderIn and fenderOut. */
    const ProfilePt *fender;
    int              numFender;
    float            fenderIn, fenderOut;
    float            fenderColor[3];

    /* Wheels: index 0 is the front axle, 1 the rear axle. */
    int   numWheels;               /* 3 for the 1886 car, otherwise 4    */
    float wheelX[2];               /* axle positions along the car       */
    float wheelRadius[2];          /* front and rear can differ          */
    float wheelWidth;
    float wheelTrack;              /* |z| of each wheel's centre         */
    int   wheelStyle;

    void (*drawDetails)(const CarModel *m);   /* era extras, may be NULL */

    GLuint bodyList;               /* display list, built by initCars()  */
};

void            initCars(void);    /* call once, after the GL context exists */
int             carModelCount(void);
const CarModel *getCarModel(int index);

/* Draws a car with its centre on the origin and wheels on y = 0.
 * distance is the total distance driven, which sets the wheel angle. */
void drawCar(const CarModel *m, double distance);

#endif
