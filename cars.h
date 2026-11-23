#ifndef CARS_H
#define CARS_H

#include "scene.h"
#include "wheels.h"

/* The car drives along the centre of the far lane. */
#define CAR_LANE_Z  -2.0f

/* ------------------------------------------------------------------ */
/* One car model per era                                               */
/*                                                                     */
/* A car's local origin is on the road (y = 0), halfway between its    */
/* axles in x and centred across its width in z; the front points to  */
/* +x. Each model lives in its own file under models/ and fills in    */
/* this struct; cars.c turns its build functions into display lists.  */
/* ------------------------------------------------------------------ */
typedef struct CarModel CarModel;

struct CarModel {
    const char *year;              /* "1954"                              */
    const char *name;              /* "300 SL Gullwing"                   */
    float length;                  /* overall, nose to tail (m)           */

    float paint[3], accent[3], leather[3];

    int   numWheels;               /* 3 for the 1886 car, otherwise 4     */
    float axleX[2];                /* front, rear                         */
    float track[2];                /* |z| of the wheel centres            */
    WheelSpec wheel[2];            /* front, rear                         */

    /* Static geometry, compiled once into display lists. */
    void (*buildBody)(const CarModel *m);     /* everything opaque        */
    void (*buildGlass)(const CarModel *m);    /* see-through parts, or NULL */
    /* Parts that move with the car (1886 flywheel), drawn every frame. */
    void (*drawAnimated)(const CarModel *m, double distance);

    GLuint     bodyList, glassList;
    WheelLists wheelLists[2];
};

void            initCars(void);    /* call once, after the GL context exists */
int             carModelCount(void);
const CarModel *getCarModel(int index);

/* Opaque parts of the car (body, wheels, moving parts). distance is the
 * total distance driven; it sets the wheel and flywheel angles. */
void drawCar(const CarModel *m, double distance);

/* Glass, drawn later in the frame with blending (see main.c). */
void drawCarGlass(const CarModel *m);

float carMaxLength(void);                    /* longest model           */
float carWheelRadius(const CarModel *m);     /* rear wheel radius       */

#endif
