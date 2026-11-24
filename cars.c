#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "cars.h"
#include "geom.h"
#include "materials.h"
#include "wheels.h"
#include "models/models.h"

typedef void (*DefineFn)(CarModel *m);

static const DefineFn definers[] = {
    defineCar1886, defineCar1901, defineCar1936, defineCar1954,
    defineCar1963, defineCar1976, defineCar1982, defineCar1995,
    defineCar2002, defineCar2013, defineCar2021,
};

#define MODEL_COUNT COUNT(definers)

static CarModel models[MODEL_COUNT];

void initCars(void)
{
    initGeom();

    for (int i = 0; i < MODEL_COUNT; i++) {
        CarModel *m = &models[i];
        memset(m, 0, sizeof *m);
        definers[i](m);

        setCarColors(m->paint, m->accent, m->leather);

        for (int a = 0; a < 2; a++)
            buildWheel(&m->wheel[a], &m->wheelLists[a]);

        m->bodyList = glGenLists(1);
        glNewList(m->bodyList, GL_COMPILE);
            m->buildBody(m);
            resetMaterial();
        glEndList();

        if (m->buildGlass) {
            m->glassList = glGenLists(1);
            glNewList(m->glassList, GL_COMPILE);
                m->buildGlass(m);
                resetMaterial();
            glEndList();
        }
    }
}

int carModelCount(void) { return MODEL_COUNT; }

const CarModel *getCarModel(int index)
{
    if (index < 0 || index >= MODEL_COUNT) index = 0;
    return &models[index];
}

float carMaxLength(void)
{
    float L = 0.0f;
    for (int i = 0; i < MODEL_COUNT; i++)
        if (models[i].length > L) L = models[i].length;
    return L;
}

float carWheelRadius(const CarModel *m) { return m->wheel[1].radius; }

static float wheelAngle(double distance, float radius)
{
    return (float)fmod(distance / radius * 180.0 / M_PI, 360.0);
}

static void placeWheel(const CarModel *m, int axle, float z, int farSide, double distance)
{
    float r = m->wheel[axle].radius;
    glPushMatrix();
        glTranslatef(m->axleX[axle], r, z);
        drawWheel(&m->wheelLists[axle], wheelAngle(distance, r), farSide);
    glPopMatrix();
}

void drawCar(const CarModel *m, double distance)
{
    glCallList(m->bodyList);

    placeWheel(m, 1,  m->track[1], 0, distance);
    placeWheel(m, 1, -m->track[1], 1, distance);
    if (m->numWheels == 3) {
        placeWheel(m, 0, 0.0f, 0, distance);
    } else {
        placeWheel(m, 0,  m->track[0], 0, distance);
        placeWheel(m, 0, -m->track[0], 1, distance);
    }

    if (m->drawAnimated) {
        setCarColors(m->paint, m->accent, m->leather);
        m->drawAnimated(m, distance);
    }
    resetMaterial();
}

void drawCarGlass(const CarModel *m)
{
    if (!m->glassList) return;
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glCallList(m->glassList);
    glPopAttrib();
    resetMaterial();
}
