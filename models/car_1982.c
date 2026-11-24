#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "../cars.h"
#include "../geom.h"
#include "../materials.h"
#include "../carparts.h"
#include "models.h"

#define FX      1.33f
#define RX     -1.33f
#define WR      0.31f
#define WW      0.19f
#define ARCH    (WR + 0.06f)
#define HW      0.85f
#define SILL    0.28f
#define TRACK   (HW - 0.5f * WW - 0.04f)
#define NOSE_X  2.15f
#define TAIL_X -2.17f

static const Pt2 lowTop[] = {
    { TAIL_X, 0.80f }, { -2.14f, 0.97f }, { -2.06f, 1.00f }, { -1.90f, 0.985f },
    { -1.45f, 0.965f }, { -1.00f, 0.94f }, {  0.00f, 0.915f }, {  0.80f, 0.89f },
    {  1.25f, 0.85f }, {  1.90f, 0.78f }, {  2.10f, 0.72f }, { NOSE_X, 0.66f },
};
static const Pt2 lowBot[] = { { TAIL_X, 0.32f }, { -1.90f, SILL }, { 1.90f, SILL }, { NOSE_X, 0.30f } };
static const Pt2 lowHw[]  = { { TAIL_X, 0.80f }, { -2.00f, 0.84f }, { -1.33f, HW }, { 1.33f, HW },
                              { 2.00f, 0.83f }, { NOSE_X, 0.78f } };

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
    s->topHalfWidth = 0.93f * s->halfWidth;
    s->roundness = 6.5f;
}

static const Pt2 glTop[] = {
    { -1.32f, 0.97f }, { -1.12f, 1.18f }, { -0.95f, 1.31f }, { -0.70f, 1.37f },
    {  0.10f, 1.38f }, {  0.32f, 1.35f }, {  0.58f, 1.13f }, {  0.80f, 0.90f },
};

static void glassSection(float x, Section *s, const void *ctx)
{
    (void)ctx;
    s->yBottom = beltY(x) - 0.12f;
    s->yTop = curveY(glTop, COUNT(glTop), x);
    s->halfWidth = 0.78f;
    s->topHalfWidth = 0.65f;
    s->roundness = 5.0f;
}

static const Loft lower = { lowerSection, NULL, TAIL_X, NOSE_X, 80, 32, 1, 1, NULL, NULL };
static const Loft glass = { glassSection, NULL, -1.32f, 0.80f, 48, 30, 0, 0, NULL, NULL };

static void setup(ClosedCar *c)
{
    memset(c, 0, sizeof *c);
    c->lower = &lower;
    c->glass = &glass;
    GlassLayout gl = { 0.30f, -0.95f, { -0.27f, 0.0f, 0.0f }, 1, 0.04f, 0.45f, 0.45f };
    c->layout = gl;
    c->beltY = beltY;
    c->axleX[0] = FX;   c->axleX[1] = RX;
    c->axleY[0] = WR;   c->axleY[1] = WR;
    c->archR[0] = ARCH; c->archR[1] = ARCH;
    c->hw = HW;
    c->sill = SILL;
    c->wellDepth = HW - (TRACK - 0.5f * WW - 0.06f);
    c->doorX[0] = 0.62f; c->doorX[1] = -0.30f; c->doorX[2] = -1.02f; c->nDoorX = 3;
    c->doorTopH = 0.96f; c->doorBottomH = 0.32f;
    c->roofMat = MAT_PAINT;
    c->beltMat = MAT_PLASTIC;
    c->roofTrimMat = MAT_PLASTIC;
    c->pillarTrimMat = MAT_PLASTIC;
    CabinSpec cab = { 0.92f, 0.38f, 0.66f, -0.47f, -1.02f, 0.38f, 0.38f, 1, MAT_PLASTIC, 0.19f };
    c->cabin = cab;
}

static void frontEnd(void)
{
    for (int s = -1; s <= 1; s += 2) {
        lampRect(v3(NOSE_X, 0.54f, s * 0.55f), v3(1, 0, 0), 0.34f, 0.12f, 0.01f, MAT_LAMP, MAT_PLASTIC);
        Vec3 n, p = loftSide(&lower, 2.10f, 0.66f, (float)s, &n);
        lampRect(p, n, 0.08f, 0.10f, 0.01f, MAT_INDICATOR, MAT_PLASTIC);
    }

    setMaterial(MAT_CHROME);
    roundedBox(2.165f, 0.55f, 0.0f, 0.04f, 0.16f, 0.28f, 8.0f);
    pushFrame(v3(2.187f, 0.55f, 0.0f), v3(1, 0, 0));
        setMaterial(MAT_MATTE);
        roundRectDisc(0.22f, 0.11f, 0.01f);
        setMaterial(MAT_CHROME);
        for (int i = -3; i <= 3; i++)
            tube(v3(i * 0.03f, -0.05f, 0.005f), v3(i * 0.03f, 0.05f, 0.005f), 0.005f, 0.005f, 6);
    glPopMatrix();

    setMaterial(MAT_PAINT);
    roundedBox(2.17f, 0.38f, 0.0f, 0.10f, 0.16f, 1.72f, 6.0f);
    setMaterial(MAT_GREY_TRIM);
    roundedBox(2.175f, 0.38f, 0.0f, 0.11f, 0.08f, 1.74f, 6.0f);
    numberPlate(v3(2.232f, 0.38f, 0.0f), v3(1, 0, 0), 0.46f, 0.11f);
}

static void tail(void)
{
    for (int s = -1; s <= 1; s += 2) {
        float zc = s * 0.52f;
        lampRect(v3(TAIL_X, 0.66f, zc), v3(-1, 0, 0), 0.38f, 0.18f, 0.02f, MAT_TAIL, MAT_PLASTIC);
        lampRect(v3(TAIL_X - 0.002f, 0.66f, s * 0.66f), v3(-1, 0, 0), 0.09f, 0.16f, 0.012f,
                 MAT_INDICATOR, MAT_INDICATOR);
        setMaterial(MAT_TAIL);
        for (int k = 0; k < 4; k++) {
            float y = 0.60f + 0.04f * k;
            tube(v3(TAIL_X - 0.012f, y, s * 0.34f), v3(TAIL_X - 0.012f, y, s * 0.70f), 0.005f, 0.005f, 6);
        }
    }
    numberPlate(v3(TAIL_X - 0.002f, 0.62f, 0.0f), v3(-1, 0, 0), 0.46f, 0.11f);

    setMaterial(MAT_PAINT);
    roundedBox(-2.19f, 0.40f, 0.0f, 0.10f, 0.16f, 1.70f, 6.0f);
    setMaterial(MAT_GREY_TRIM);
    roundedBox(-2.195f, 0.40f, 0.0f, 0.11f, 0.08f, 1.72f, 6.0f);
    tailpipe(v3(-2.20f, 0.27f, 0.45f), 0.028f, 0.5f, MAT_CHROME);
}

static void cladding(void)
{
    const float xs[3][2] = {
        { TAIL_X + 0.05f, RX - ARCH - 0.02f },
        { RX + ARCH + 0.02f, FX - ARCH - 0.02f },
        { FX + ARCH + 0.02f, NOSE_X - 0.05f },
    };
    setMaterial(MAT_GREY_TRIM);
    for (int s = -1; s <= 1; s += 2)
        for (int i = 0; i < 3; i++)
            loftSidePatch(&lower, xs[i][0], xs[i][1], 0.06f, 0.30f, (float)s, 10, 4, 0.008f);
}

static void sides(void)
{
    for (int s = -1; s <= 1; s += 2) {
        const float hx[2] = { 0.15f, -0.80f };
        for (int i = 0; i < 2; i++) {
            Vec3 n, p = loftSide(&lower, hx[i], 0.80f, (float)s, &n);
            doorHandle(p, n, 0.11f, MAT_CHROME);
        }
        Vec3 p = loftSide(&lower, 0.62f, 0.97f, (float)s, NULL);
        sideMirror(p, (float)s, MAT_PLASTIC, 0);
    }

    Vec3 n, p = loftTop(&glass, 0.75f, 0.05f, &n);
    Vec3 t = loftTop(&glass, 0.73f, -0.85f, NULL);
    wiper(p, t, n);

    setMaterial(MAT_PLASTIC);
    Pt2 lip[2] = { { -2.08f, -0.85f }, { -2.08f, 0.85f } };
    loftStrip(&lower, lip, 2, 1, 1.0f, 0.008f, 0.003f);
}

static void buildBody(const CarModel *m)
{
    (void)m;
    ClosedCar c;
    setup(&c);
    drawClosedCarCommon(&c);
    cladding();
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

void defineCar1982(CarModel *m)
{
    static const float paint[3]   = { 0.10f, 0.14f, 0.30f };
    static const float accent[3]  = { 0.42f, 0.43f, 0.45f };
    static const float leather[3] = { 0.24f, 0.26f, 0.32f };

    m->year = "1982";
    m->name = "190E (W201)";
    m->length = 4.42f;
    memcpy(m->paint, paint, sizeof paint);
    memcpy(m->accent, accent, sizeof accent);
    memcpy(m->leather, leather, sizeof leather);

    m->numWheels = 4;
    m->axleX[0] = FX;     m->axleX[1] = RX;
    m->track[0] = TRACK;  m->track[1] = TRACK;

    WheelSpec w = { WR, WW, 0.19f, RIM_15HOLE, MAT_METAL, 1 };
    m->wheel[0] = w;
    m->wheel[1] = w;

    m->buildBody    = buildBody;
    m->buildGlass   = buildGlass;
    m->drawAnimated = NULL;
}
