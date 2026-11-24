#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "../cars.h"
#include "../geom.h"
#include "../materials.h"
#include "../carparts.h"
#include "models.h"

#define FX      1.40f
#define RX     -1.40f
#define WR      0.32f
#define WW      0.19f
#define ARCH    (WR + 0.06f)
#define HW      0.89f
#define SILL    0.27f
#define TRACK   (HW - 0.5f * WW - 0.04f)
#define NOSE_X  2.26f
#define TAIL_X -2.28f

static const Pt2 lowTop[] = {
    { TAIL_X, 0.78f }, { -2.23f, 0.92f }, { -1.95f, 0.95f }, { -1.45f, 0.95f },
    { -1.10f, 0.94f }, {  0.90f, 0.93f }, {  1.30f, 0.92f }, {  2.10f, 0.89f },
    {  2.21f, 0.86f }, { NOSE_X, 0.84f },
};
static const Pt2 lowBot[] = { { TAIL_X, 0.30f }, { -2.00f, SILL }, { 2.00f, SILL }, { NOSE_X, 0.29f } };
static const Pt2 lowHw[]  = { { TAIL_X, 0.86f }, { -2.10f, HW }, { 2.10f, HW }, { NOSE_X, 0.86f } };

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
    s->topHalfWidth = 0.95f * s->halfWidth;
    s->roundness = 8.0f;
}

static const Pt2 glTop[] = {
    { -1.36f, 0.95f }, { -1.15f, 1.18f }, { -1.00f, 1.36f }, { -0.85f, 1.42f },
    {  0.25f, 1.44f }, {  0.42f, 1.42f }, {  0.65f, 1.18f }, {  0.88f, 0.95f },
};

static void glassSection(float x, Section *s, const void *ctx)
{
    (void)ctx;
    s->yBottom = beltY(x) - 0.12f;
    s->yTop = curveY(glTop, COUNT(glTop), x);
    s->halfWidth = 0.83f;
    s->topHalfWidth = 0.72f;
    s->roundness = 6.0f;
}

static const Loft lower = { lowerSection, NULL, TAIL_X, NOSE_X, 84, 32, 1, 1, NULL, NULL };
static const Loft glass = { glassSection, NULL, -1.36f, 0.88f, 50, 32, 0, 0, NULL, NULL };

static void setup(ClosedCar *c)
{
    memset(c, 0, sizeof *c);
    c->lower = &lower;
    c->glass = &glass;
    GlassLayout gl = { 0.40f, -0.98f, { -0.22f, -0.80f, 0.0f }, 2, 0.04f, 0.45f, 0.45f };
    c->layout = gl;
    c->beltY = beltY;
    c->axleX[0] = FX;   c->axleX[1] = RX;
    c->axleY[0] = WR;   c->axleY[1] = WR;
    c->archR[0] = ARCH; c->archR[1] = ARCH;
    c->hw = HW;
    c->sill = SILL;
    c->wellDepth = HW - (TRACK - 0.5f * WW - 0.06f);
    c->doorX[0] = 0.70f; c->doorX[1] = -0.20f; c->doorX[2] = -1.05f; c->nDoorX = 3;
    c->doorTopH = 0.96f; c->doorBottomH = 0.18f;
    c->roofMat = MAT_PAINT;
    c->beltMat = MAT_CHROME;
    c->roofTrimMat = MAT_CHROME;
    c->pillarTrimMat = MAT_PLASTIC;
    CabinSpec cab = { 0.93f, 0.46f, 0.70f, -0.40f, -1.02f, 0.38f, 0.40f, 1, MAT_WOOD, 0.20f };
    c->cabin = cab;
}

static void bumper(float x, float y, float dir)
{
    Pt2 blade[16], rubber[16];
    for (int i = 0; i < 16; i++) {
        float t = 2.0f * (float)M_PI * i / 16, c = cosf(t), sn = sinf(t);
        float cc = c >= 0 ? powf(c, 0.4f) : -powf(-c, 0.4f);
        float ss = sn >= 0 ? powf(sn, 0.4f) : -powf(-sn, 0.4f);
        blade[i].x  = 0.05f * cc;   blade[i].y  = 0.07f * ss;
        rubber[i].x = 0.025f * cc;  rubber[i].y = 0.03f * ss;
    }
    Vec3 p[7] = { v3(x - dir * 0.21f, y, -0.90f), v3(x - dir * 0.04f, y, -0.80f),
                  v3(x + dir * 0.01f, y, -0.40f), v3(x + dir * 0.02f, y, 0.0f),
                  v3(x + dir * 0.01f, y, 0.40f),  v3(x - dir * 0.04f, y, 0.80f),
                  v3(x - dir * 0.21f, y, 0.90f) };
    setMaterial(MAT_CHROME);
    sweep(p, 7, 40, blade, 16, v3(1, 0, 0), 1);
    for (int i = 0; i < 7; i++) p[i].x += dir * 0.035f;
    setMaterial(MAT_RUBBER);
    sweep(p, 7, 40, rubber, 16, v3(1, 0, 0), 1);
}

static void frontEnd(void)
{
    for (int s = -1; s <= 1; s += 2) {
        float zc = s * 0.58f;
        setMaterial(MAT_CHROME);
        roundedBox(2.275f, 0.66f, zc, 0.04f, 0.22f, 0.44f, 8.0f);
        lampRect(v3(2.297f, 0.66f, s * 0.62f), v3(1, 0, 0), 0.20f, 0.16f, 0.01f, MAT_LAMP, MAT_PLASTIC);
        lampRect(v3(2.297f, 0.66f, s * 0.43f), v3(1, 0, 0), 0.10f, 0.16f, 0.01f, MAT_LAMP, MAT_PLASTIC);
        lampRect(v3(2.297f, 0.66f, s * 0.765f), v3(1, 0, 0), 0.06f, 0.16f, 0.01f, MAT_INDICATOR, MAT_PLASTIC);
    }

    setMaterial(MAT_CHROME);
    roundedBox(2.285f, 0.68f, 0.0f, 0.06f, 0.40f, 0.40f, 8.0f);
    pushFrame(v3(2.316f, 0.67f, 0.0f), v3(1, 0, 0));
        setMaterial(MAT_MATTE);
        roundRectDisc(0.32f, 0.30f, 0.02f);
        setMaterial(MAT_CHROME);
        for (int i = -5; i <= 5; i++) {
            float r = (i == 0) ? 0.012f : 0.006f;
            tube(v3(i * 0.028f, -0.14f, 0.006f), v3(i * 0.028f, 0.14f, 0.006f), r, r, 6);
        }
    glPopMatrix();

    bumper(2.28f, 0.42f, 1.0f);
    numberPlate(v3(2.365f, 0.42f, 0.0f), v3(1, 0, 0), 0.46f, 0.11f);
}

static void tail(void)
{
    for (int s = -1; s <= 1; s += 2) {
        float zc = s * 0.55f;
        lampRect(v3(TAIL_X, 0.60f, zc), v3(-1, 0, 0), 0.46f, 0.24f, 0.02f, MAT_TAIL, MAT_CHROME);
        lampRect(v3(TAIL_X - 0.002f, 0.60f, s * 0.71f), v3(-1, 0, 0), 0.12f, 0.22f, 0.015f,
                 MAT_INDICATOR, MAT_INDICATOR);
        lampRect(v3(TAIL_X - 0.002f, 0.60f, s * 0.40f), v3(-1, 0, 0), 0.08f, 0.22f, 0.015f,
                 MAT_PLATE, MAT_PLATE);
        setMaterial(MAT_TAIL);
        for (int k = 0; k < 6; k++) {
            float y = 0.50f + 0.04f * k;
            tube(v3(TAIL_X - 0.012f, y, s * 0.34f), v3(TAIL_X - 0.012f, y, s * 0.76f), 0.006f, 0.006f, 6);
        }
    }
    numberPlate(v3(TAIL_X - 0.002f, 0.52f, 0.0f), v3(-1, 0, 0), 0.50f, 0.11f);
    bumper(-2.30f, 0.40f, -1.0f);
    tailpipe(v3(-2.26f, 0.25f, 0.45f), 0.03f, 0.5f, MAT_CHROME);
}

static void sides(void)
{
    for (int s = -1; s <= 1; s += 2) {
        setMaterial(MAT_PLASTIC);
        Pt2 strip[2] = { { -1.95f, 0.42f }, { 2.00f, 0.42f } };
        loftStrip(&lower, strip, 2, 0, (float)s, 0.035f, 0.006f);

        const float hx[2] = { 0.25f, -0.75f };
        for (int i = 0; i < 2; i++) {
            Vec3 n, p = loftSide(&lower, hx[i], 0.80f, (float)s, &n);
            doorHandle(p, n, 0.12f, MAT_CHROME);
        }
        Vec3 p = loftSide(&lower, 0.70f, 0.97f, (float)s, NULL);
        sideMirror(p, (float)s, MAT_CHROME, 0);
    }
    for (int i = 0; i < 2; i++) {
        float u0 = i ? 0.05f : -0.60f;
        Vec3 n, p = loftTop(&glass, 0.84f, u0, &n);
        Vec3 t = loftTop(&glass, 0.82f, u0 + 0.52f, NULL);
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
}

void defineCar1976(CarModel *m)
{
    static const float paint[3]   = { 0.80f, 0.72f, 0.55f };
    static const float accent[3]  = { 0.10f, 0.10f, 0.10f };
    static const float leather[3] = { 0.42f, 0.26f, 0.14f };

    m->year = "1976";
    m->name = "W123 E-Class";
    m->length = 4.74f;
    memcpy(m->paint, paint, sizeof paint);
    memcpy(m->accent, accent, sizeof accent);
    memcpy(m->leather, leather, sizeof leather);

    m->numWheels = 4;
    m->axleX[0] = FX;     m->axleX[1] = RX;
    m->track[0] = TRACK;  m->track[1] = TRACK;

    WheelSpec w = { WR, WW, 0.18f, RIM_RIBBED_CAP, MAT_PAINT, 0 };
    m->wheel[0] = w;
    m->wheel[1] = w;

    m->buildBody    = buildBody;
    m->buildGlass   = buildGlass;
    m->drawAnimated = NULL;
}
