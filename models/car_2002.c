#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "../cars.h"
#include "../geom.h"
#include "../materials.h"
#include "../carparts.h"
#include "models.h"

#define FX      1.427f
#define RX     -1.427f
#define WR      0.33f
#define WW      0.225f
#define ARCH    (WR + 0.06f)
#define HW      0.91f
#define SILL    0.29f
#define TRACK   (HW - 0.5f * WW - 0.04f)
#define NOSE_X  2.43f
#define TAIL_X -2.39f

static const Pt2 lowTop[] = {
    { TAIL_X, 0.76f }, { -2.34f, 0.93f }, { -2.15f, 0.99f }, { -1.95f, 1.00f },
    { -1.70f, 0.98f }, { -1.20f, 0.955f }, {  0.50f, 0.925f }, {  1.00f, 0.895f },
    {  1.75f, 0.82f }, {  2.15f, 0.74f }, {  2.34f, 0.66f }, { NOSE_X, 0.56f },
};
static const Pt2 lowBot[] = { { TAIL_X, 0.36f }, { -2.15f, 0.30f }, { -1.80f, SILL },
                              { 1.80f, SILL }, { 2.20f, 0.31f }, { NOSE_X, 0.36f } };
static const Pt2 lowHw[]  = { { TAIL_X, 0.78f }, { -2.15f, 0.88f }, { -1.60f, HW }, { 1.60f, HW },
                              { 2.15f, 0.87f }, { NOSE_X, 0.72f } };

static float beltY(float x) { return curveY(lowTop, COUNT(lowTop), x); }

static void lowerSection(float x, Section *s, const void *ctx)
{
    (void)ctx;
    s->yTop = beltY(x);
    float yb = curveY(lowBot, COUNT(lowBot), x);
    yb = archBottom(x, FX, ARCH, WR, yb);
    yb = archBottom(x, RX, ARCH, WR, yb);
    s->yBottom = yb < s->yTop - 0.06f ? yb : s->yTop - 0.06f;
    s->halfWidth = curveY(lowHw, COUNT(lowHw), x);
    s->topHalfWidth = 0.88f * s->halfWidth;
    s->roundness = 3.4f;
}

static const Pt2 glTop[] = {
    { -1.85f, 1.00f }, { -1.50f, 1.17f }, { -1.15f, 1.33f }, { -0.60f, 1.44f },
    { -0.10f, 1.45f }, {  0.25f, 1.41f }, {  0.60f, 1.20f }, {  0.98f, 0.93f },
};
static const Pt2 glHw[] = { { -1.85f, 0.74f }, { -1.20f, 0.83f }, { 0.98f, 0.82f } };

static void glassSection(float x, Section *s, const void *ctx)
{
    (void)ctx;
    s->yBottom = beltY(x) - 0.12f;
    s->yTop = curveY(glTop, COUNT(glTop), x);
    s->halfWidth = curveY(glHw, COUNT(glHw), x);
    s->topHalfWidth = 0.62f;
    s->roundness = 3.4f;
}

static const Loft lower = { lowerSection, NULL, TAIL_X, NOSE_X, 84, 32, 1, 1, NULL, NULL };
static const Loft glass = { glassSection, NULL, -1.85f, 0.98f, 56, 30, 0, 0, NULL, NULL };

static void setup(ClosedCar *c)
{
    memset(c, 0, sizeof *c);
    c->lower = &lower;
    c->glass = &glass;
    GlassLayout gl = { 0.20f, -1.15f, { -0.30f, 0.0f, 0.0f }, 1, 0.04f, 0.45f, 0.45f };
    c->layout = gl;
    c->beltY = beltY;
    c->axleX[0] = FX;   c->axleX[1] = RX;
    c->axleY[0] = WR;   c->axleY[1] = WR;
    c->archR[0] = ARCH; c->archR[1] = ARCH;
    c->hw = HW;
    c->sill = SILL;
    c->wellDepth = HW - (TRACK - 0.5f * WW - 0.06f);
    c->doorX[0] = 0.80f; c->doorX[1] = -0.30f; c->doorX[2] = -1.12f; c->nDoorX = 3;
    c->doorTopH = 0.96f; c->doorBottomH = 0.20f;
    c->roofMat = MAT_PAINT;
    c->beltMat = MAT_CHROME;
    c->roofTrimMat = MAT_CHROME;
    c->pillarTrimMat = MAT_GLOSS_BLACK;
    CabinSpec cab = { 0.93f, 0.53f, 0.70f, -0.32f, -1.20f, 0.38f, 0.40f, 1, MAT_PLASTIC, 0.19f };
    c->cabin = cab;
}

static void headlampUnit(float s)
{
    Vec3 n, p = loftTop(&lower, 2.33f, s * 0.58f, &n);
    pushFrameFacing(p, n, v3(0.0f, 0.0f, s));
        glTranslatef(0.0f, 0.0f, 0.003f);
        setMaterialScaled(MAT_METAL, 0.6f);
        teardropDisc(0.20f, 0.09f, 0.45f);
        glTranslatef(0.0f, 0.0f, 0.002f);
        setMaterial(MAT_CHROME);
        ellipseDisc(0.082f, 0.072f, 24);
        glPushMatrix(); glTranslatef(-0.13f, -0.005f, 0.0f); ellipseDisc(0.062f, 0.055f, 24); glPopMatrix();
        setMaterial(MAT_LAMP);
        ellipsoid(0.0f, 0.0f, 0.002f, 0.072f, 0.064f, 0.016f);
        ellipsoid(-0.13f, -0.005f, 0.002f, 0.054f, 0.048f, 0.013f);
    glPopMatrix();
}

static void frontEnd(void)
{
    for (int s = -1; s <= 1; s += 2) {
        headlampUnit((float)s);
        headlampRound(v3(2.452f, 0.38f, s * 0.62f), v3(1, 0, 0), 0.04f, 0.03f,
                      MAT_PLASTIC, MAT_PLASTIC);
    }

    Vec3 n, p = loftTop(&lower, 2.40f, 0.0f, &n);
    pushFrame(p, n);
        glTranslatef(0.0f, 0.0f, 0.004f);
        setMaterial(MAT_MATTE);
        roundRectDisc(0.38f, 0.15f, 0.06f);
        setMaterial(MAT_CHROME);
        glTranslatef(0.0f, 0.0f, 0.006f);
        roundRectLoop(0.40f, 0.17f, 0.07f, 0.013f);
        for (int i = -1; i <= 1; i++)
            tube(v3(-0.17f, i * 0.045f, 0.0f), v3(0.17f, i * 0.045f, 0.0f), 0.008f, 0.008f, 6);
    glPopMatrix();

    setMaterial(MAT_PAINT);
    roundedBox(2.38f, 0.40f, 0.0f, 0.14f, 0.18f, 1.72f, 5.0f);
    pushFrame(v3(2.452f, 0.36f, 0.0f), v3(1, 0, 0));
        setMaterial(MAT_MATTE);
        roundRectDisc(0.80f, 0.06f, 0.025f);
    glPopMatrix();
    numberPlate(v3(2.452f, 0.44f, 0.0f), v3(1, 0, 0), 0.46f, 0.10f);
}

static void tail(void)
{
    for (int s = -1; s <= 1; s += 2) {
        lampRect(v3(TAIL_X, 0.64f, s * 0.56f), v3(-1, 0, 0), 0.30f, 0.18f, 0.03f, MAT_TAIL, MAT_PLASTIC);
        lampRect(v3(TAIL_X, 0.64f, s * 0.36f), v3(-1, 0, 0), 0.12f, 0.16f, 0.02f, MAT_PLATE, MAT_PLASTIC);
        Vec3 n, p = loftSide(&lower, -2.34f, loftH(&lower, -2.34f, 0.64f), (float)s, &n);
        lampRect(p, n, 0.16f, 0.15f, 0.03f, MAT_TAIL, MAT_PLASTIC);
    }
    numberPlate(v3(TAIL_X - 0.002f, 0.58f, 0.0f), v3(-1, 0, 0), 0.46f, 0.10f);
    setMaterial(MAT_PAINT);
    roundedBox(-2.35f, 0.42f, 0.0f, 0.12f, 0.18f, 1.70f, 5.0f);
    tailpipe(v3(-2.41f, 0.28f, 0.42f), 0.028f, 0.5f, MAT_CHROME);
    tailpipe(v3(-2.41f, 0.28f, 0.52f), 0.028f, 0.5f, MAT_CHROME);
}

static void sides(void)
{
    for (int s = -1; s <= 1; s += 2) {
        const float hx[2] = { 0.28f, -0.78f };
        for (int i = 0; i < 2; i++) {
            Vec3 n, p = loftSide(&lower, hx[i], 0.80f, (float)s, &n);
            doorHandle(p, n, 0.12f, MAT_PAINT);
        }
        Vec3 p = loftSide(&lower, 0.80f, 0.97f, (float)s, NULL);
        sideMirror(p, (float)s, MAT_PAINT, 1);
    }
    for (int i = 0; i < 2; i++) {
        float u0 = i ? 0.05f : -0.62f;
        Vec3 n, p = loftTop(&glass, 0.93f, u0, &n);
        Vec3 t = loftTop(&glass, 0.91f, u0 + 0.55f, NULL);
        wiper(p, t, n);
    }
}

static void buildBody(const CarModel *m)
{
    (void)m;
    ClosedCar c;
    setup(&c);
    drawClosedCarCommon(&c);
    frontEnd();
    tail();
    sides();
}

static void buildGlass(const CarModel *m)
{
    (void)m;
    ClosedCar c;
    setup(&c);
    drawClosedCarGlass(&c);

    setMaterialScaled(MAT_GLASS, 3.0f);
    for (int s = -1; s <= 1; s += 2) {
        Vec3 n, p = loftTop(&lower, 2.33f, s * 0.58f, &n);
        pushFrameFacing(p, n, v3(0.0f, 0.0f, (float)s));
            glTranslatef(0.0f, 0.0f, 0.028f);
            teardropDisc(0.21f, 0.095f, 0.45f);
        glPopMatrix();
    }
}

void defineCar2002(CarModel *m)
{
    static const float paint[3]   = { 0.74f, 0.75f, 0.77f };
    static const float accent[3]  = { 0.10f, 0.10f, 0.10f };
    static const float leather[3] = { 0.12f, 0.12f, 0.12f };

    m->year = "2002";
    m->name = "E-Class (W211)";
    m->length = 4.84f;
    memcpy(m->paint, paint, sizeof paint);
    memcpy(m->accent, accent, sizeof accent);
    memcpy(m->leather, leather, sizeof leather);

    m->numWheels = 4;
    m->axleX[0] = FX;     m->axleX[1] = RX;
    m->track[0] = TRACK;  m->track[1] = TRACK;

    WheelSpec w = { WR, WW, 0.215f, RIM_5TWIN, MAT_METAL, 1 };
    m->wheel[0] = w;
    m->wheel[1] = w;

    m->buildBody    = buildBody;
    m->buildGlass   = buildGlass;
    m->drawAnimated = NULL;
}
