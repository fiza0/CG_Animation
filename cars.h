#ifndef CARS_H
#define CARS_H

#include "scene.h"
#include "wheels.h"

#define CAR_LANE_Z  -2.0f

typedef struct CarModel CarModel;

struct CarModel {
    const char *year;
    const char *name;
    float length;

    float paint[3], accent[3], leather[3];

    int   numWheels;
    float axleX[2];
    float track[2];
    WheelSpec wheel[2];

    void (*buildBody)(const CarModel *m);
    void (*buildGlass)(const CarModel *m);
    void (*drawAnimated)(const CarModel *m, double distance);

    GLuint     bodyList, glassList;
    WheelLists wheelLists[2];
};

void            initCars(void);
int             carModelCount(void);
const CarModel *getCarModel(int index);

void drawCar(const CarModel *m, double distance);

void drawCarGlass(const CarModel *m);

float carMaxLength(void);
float carWheelRadius(const CarModel *m);

#endif
