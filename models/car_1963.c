#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "../cars.h"
#include "../geom.h"
#include "../materials.h"
#include "../carparts.h"
#include "models.h"

#define FX     1.20f
#define RX    -1.20f
#define WR     0.33f
#define WW     0.18f
#define ARCH   (WR + 0.06f)
#define HW     0.88f
#define SILL   0.28f
#define TRACK  (HW - 0.5f * WW - 0.04f)
#define NOSE_X 2.16f
#define TAIL_X -2.08f

static const Pt2 lowTop[] = {
    { TAIL_X, 0.70f }, { -2.02f, 0.80f }, { -1.85f, 0.83f }, { -1.20f, 0.84f },
    {  0.50f, 0.84f }, {  1.20f, 0.83f }, {  1.90f, 0.80f }, {  2.10f, 0.76f },
    { NOSE_X, 0.70f },
};
static const Pt2 lowBot[] = { { TAIL_X, 0.32f }, { -1.80f, SILL }, { 1.80f, SILL }, { NOSE_X, 0.32f } };
static const Pt2 lowHw[]  = { { TAIL_X, 0.80f }, { -1.90f, 0.86f }, { -1.20f, HW }, { 1.20f, HW },
                              { 1.90f, 0.86f }, { NOSE_X, 0.80f } };

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
    s->roundness = 7.0f;
}

static const Pt2 glTop[] = {
    { -1.12f, 0.86f }, { -1.00f, 1.10f }, { -0.92f, 1.24f }, { -0.80f, 1.29f },
    {  0.00f, 1.30f }, {  0.12f, 1.28f }, {  0.30f, 1.05f }, {  0.42f, 0.86f },
};

static void glassSection(float x, Section *s, const void *ctx)
{
    (void)ctx; (void)x;
    s->yBottom = beltY(x) - 0.12f;
    s->yTop = curveY(glTop, COUNT(glTop), x);
    s->halfWidth = 0.80f;
    s->topHalfWidth = 0.74f;
    s->roundness = 8.0f;
}

static const Loft lower = { lowerSection, NULL, TAIL_X, NOSE_X, 80, 32, 1, 1, NULL, NULL };
static const Loft glass = { glassSection, NULL, -1.12f, 0.42f, 44, 32, 0, 0, NULL, NULL };

static void setup(ClosedCar *c)
{
    memset(c, 0, sizeof *c);
    c->lower = &lower;
    c->glass = &glass;
    GlassLayout gl = { 0.12f, -0.90f, { -0.60f, 0.0f, 0.0f }, 1, 0.025f, 0.50f, 0.50f };
    c->layout = gl;
    c->beltY = beltY;
    c->axleX[0] = FX;   c->axleX[1] = RX;
    c->axleY[0] = WR;   c->axleY[1] = WR;
    c->archR[0] = ARCH; c->archR[1] = ARCH;
    c->hw = HW;
    c->sill = SILL;
    c->wellDepth = HW - (TRACK - 0.5f * WW - 0.06f);
    c->doorX[0] = 0.38f; c->doorX[1] = -0.78f; c->nDoorX = 2;
    c->doorTopH = 0.95f; c->doorBottomH = 0.20f;
    c->roofMat = MAT_ACCENT;
    c->beltMat = MAT_CHROME;
    c->roofTrimMat = MAT_CHROME;
    c->pillarTrimMat = MAT_ACCENT;
    CabinSpec cab = { 0.84f, 0.05f, 0.62f, -0.70f, 0.0f, 0.0f, 0.36f, 0, MAT_PAINT, 0.21f };
    c->cabin = cab;
}

static void pagodaRoof(void)
{
    setMaterialScaled(MAT_ACCENT, 0.88f);
    loftTopPatch(&glass, -0.78f, 0.06f, -0.6f, 0.6f, 8, 6, 0.003f);

    Pt2 sec[16];
    for (int i = 0; i < 16; i++) {
        float t = 2.0f * (float)M_PI * i / 16, c = cosf(t), sn = sinf(t);
        sec[i].x = 0.06f * (c  >= 0 ? sqrtf(c)  : -sqrtf(-c));
        sec[i].y = 0.03f * (sn >= 0 ? sqrtf(sn) : -sqrtf(-sn));
    }
    setMaterial(MAT_ACCENT);
    for (int s = -1; s <= 1; s += 2) {
        Vec3 path[6];
        for (int i = 0; i < 6; i++) {
            float x = -0.85f + 0.186f * i;
            Vec3 n, p = loftTop(&glass, x, s * 0.84f, &n);
            path[i] = vAdd(p, vScale(n, 0.012f));
        }
        sweep(path, 6, 24, sec, 16, v3(0, 0, 1), 1);
    }
}

static void frontEnd(void)
{
    for (int s = -1; s <= 1; s += 2) {
        float z = s * 0.60f;
        setMaterial(MAT_CHROME);
        roundedBox(2.18f, 0.56f, z, 0.05f, 0.30f, 0.20f, 6.0f);
        headlampRound(v3(2.207f, 0.62f, z), v3(1, 0, 0), 0.075f, 0.04f, MAT_CHROME, MAT_CHROME);
        lampRect(v3(2.205f, 0.51f, z), v3(1, 0, 0), 0.14f, 0.05f, 0.015f, MAT_LAMP, MAT_CHROME);
        lampRect(v3(2.205f, 0.45f, z), v3(1, 0, 0), 0.14f, 0.04f, 0.015f, MAT_INDICATOR, MAT_CHROME);
    }

    pushFrame(v3(NOSE_X + 0.003f, 0.50f, 0.0f), v3(1, 0, 0));
        setMaterial(MAT_MATTE);
        roundRectDisc(0.80f, 0.20f, 0.04f);
        setMaterial(MAT_CHROME);
        glTranslatef(0.0f, 0.0f, 0.01f);
        roundRectLoop(0.82f, 0.22f, 0.05f, 0.012f);
        tube(v3(-0.40f, 0.0f, 0.01f), v3(0.40f, 0.0f, 0.01f), 0.018f, 0.018f, 10);
    glPopMatrix();

    setMaterial(MAT_CHROME);
    Vec3 fb[7] = { v3(2.00f, 0.33f, -0.86f), v3(2.15f, 0.33f, -0.74f), v3(2.20f, 0.33f, -0.40f),
                   v3(2.21f, 0.33f, 0.0f),   v3(2.20f, 0.33f, 0.40f),  v3(2.15f, 0.33f, 0.74f),
                   v3(2.00f, 0.33f, 0.86f) };
    sweepTube(fb, 7, 40, 0.028f, 10);
    Vec3 rb[7] = { v3(-1.92f, 0.34f, -0.86f), v3(-2.07f, 0.34f, -0.74f), v3(-2.12f, 0.34f, -0.40f),
                   v3(-2.13f, 0.34f, 0.0f),   v3(-2.12f, 0.34f, 0.40f),  v3(-2.07f, 0.34f, 0.74f),
                   v3(-1.92f, 0.34f, 0.86f) };
    sweepTube(rb, 7, 40, 0.028f, 10);
    for (int s = -1; s <= 1; s += 2) {
        roundedBox(2.22f, 0.40f, s * 0.30f, 0.05f, 0.20f, 0.06f, 6.0f);
        roundedBox(-2.14f, 0.42f, s * 0.30f, 0.05f, 0.20f, 0.06f, 6.0f);
    }
    numberPlate(v3(2.20f, 0.265f, 0.0f), v3(1, 0, 0), 0.40f, 0.09f);
}

static void tail(void)
{
    for (int s = -1; s <= 1; s += 2) {
        float z = s * 0.62f;
        lampRect(v3(TAIL_X, 0.54f, z), v3(-1, 0, 0), 0.12f, 0.16f, 0.02f, MAT_TAIL, MAT_CHROME);
        lampRect(v3(TAIL_X, 0.66f, z), v3(-1, 0, 0), 0.12f, 0.06f, 0.015f, MAT_INDICATOR, MAT_CHROME);
    }
    numberPlate(v3(TAIL_X - 0.002f, 0.52f, 0.0f), v3(-1, 0, 0), 0.36f, 0.11f);
    tailpipe(v3(-2.10f, 0.27f, 0.50f), 0.028f, 0.5f, MAT_CHROME);
}

static void sides(void)
{
    for (int s = -1; s <= 1; s += 2) {
        setMaterial(MAT_CHROME);
        Pt2 strip[2] = { { -1.95f, 0.55f }, { 1.95f, 0.55f } };
        loftStrip(&lower, strip, 2, 0, (float)s, 0.012f, 0.004f);

        Vec3 n, p = loftSide(&lower, -0.62f, 0.85f, (float)s, &n);
        doorHandle(p, n, 0.10f, MAT_CHROME);
        p = loftSide(&lower, 0.30f, 0.96f, (float)s, NULL);
        sideMirror(p, (float)s, MAT_CHROME, 0);
    }
    for (int i = 0; i < 2; i++) {
        float u0 = i ? 0.05f : -0.60f;
        Vec3 n, p = loftTop(&glass, 0.38f, u0, &n);
        Vec3 t = loftTop(&glass, 0.37f, u0 + 0.52f, NULL);
        wiper(p, t, n);
    }
}

static void buildBody(const CarModel *m)
{
    (void)m;
    ClosedCar c;
    setup(&c);
    drawClosedCarCommon(&c);
    pagodaRoof();
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

void defineCar1963(CarModel *m)
{
    static const float paint[3]   = { 0.62f, 0.76f, 0.88f };
    static const float accent[3]  = { 0.94f, 0.94f, 0.92f };
    static const float leather[3] = { 0.55f, 0.30f, 0.15f };

    m->year = "1963";
    m->name = "230 SL Pagoda";
    m->length = 4.34f;
    memcpy(m->paint, paint, sizeof paint);
    memcpy(m->accent, accent, sizeof accent);
    memcpy(m->leather, leather, sizeof leather);

    m->numWheels = 4;
    m->axleX[0] = FX;     m->axleX[1] = RX;
    m->track[0] = TRACK;  m->track[1] = TRACK;

    WheelSpec w = { WR, WW, 0.18f, RIM_STEEL_CAP, MAT_PAINT, 0 };
    m->wheel[0] = w;
    m->wheel[1] = w;

    m->buildBody    = buildBody;
    m->buildGlass   = buildGlass;
    m->drawAnimated = NULL;
}
