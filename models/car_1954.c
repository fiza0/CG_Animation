#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "../cars.h"
#include "../geom.h"
#include "../materials.h"
#include "../carparts.h"
#include "models.h"

/* ------------------------------------------------------------------ */
/* 1954 300 SL "Gullwing"                                              */
/*                                                                     */
/* Very low, smooth coupe: long bonnet with two power domes, a wide    */
/* low grille with one thick chrome bar, round headlamps faired into   */
/* the wings, eyebrows over the wheel arches, gill vents behind the    */
/* front wheels, gullwing door seams running into the roof.            */
/* ------------------------------------------------------------------ */
#define FX     1.20f
#define RX    -1.20f
#define WR     0.35f
#define WW     0.17f
#define ARCH   (WR + 0.06f)
#define HW     0.89f
#define SILL   0.27f
#define TRACK  (HW - 0.5f * WW - 0.04f)

static const Pt2 lowTop[] = {
    { -2.20f, 0.60f }, { -2.10f, 0.74f }, { -1.90f, 0.82f }, { -1.55f, 0.86f },
    { -1.00f, 0.88f }, {  0.30f, 0.90f }, {  0.80f, 0.88f }, {  1.40f, 0.84f },
    {  1.85f, 0.75f }, {  2.10f, 0.66f }, {  2.20f, 0.58f },
};
static const Pt2 lowBot[] = {
    { -2.20f, 0.34f }, { -1.95f, 0.29f }, { -1.50f, SILL }, { 1.50f, SILL },
    {  1.95f, 0.29f }, {  2.20f, 0.31f },
};
/* Slightly wider over the wheels: the start of the eyebrows. */
static const Pt2 lowHw[] = {
    { -2.20f, 0.62f }, { -2.00f, 0.80f }, { -1.60f, 0.87f }, { -1.20f, HW },
    { -0.60f, 0.87f }, {  0.60f, 0.87f }, {  1.20f, HW },    {  1.75f, 0.84f },
    {  2.05f, 0.74f }, {  2.20f, 0.58f },
};

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
    s->topHalfWidth = 0.86f * s->halfWidth;
    s->roundness = 3.2f;
}

/* Small, rounded glasshouse: steep fastback rear, thin pillars. */
static const Pt2 glTop[] = {
    { -1.45f, 0.86f }, { -1.25f, 1.02f }, { -1.00f, 1.16f }, { -0.60f, 1.27f },
    { -0.10f, 1.30f }, {  0.20f, 1.27f }, {  0.40f, 1.10f }, {  0.55f, 0.90f },
};
static const Pt2 glHw[] = { { -1.45f, 0.70f }, { -0.80f, 0.80f }, { 0.55f, 0.78f } };

static void glassSection(float x, Section *s, const void *ctx)
{
    (void)ctx;
    s->yBottom = beltY(x) - 0.12f;
    s->yTop = curveY(glTop, COUNT(glTop), x);
    s->halfWidth = curveY(glHw, COUNT(glHw), x);
    s->topHalfWidth = 0.62f;
    s->roundness = 3.0f;
}

static const Loft lower = { lowerSection, NULL, -2.20f, 2.20f, 80, 32, 1, 1, NULL, NULL };
static const Loft glass = { glassSection, NULL, -1.45f, 0.55f, 48, 28, 0, 0, NULL, NULL };

static void setup(ClosedCar *c)
{
    memset(c, 0, sizeof *c);
    c->lower = &lower;
    c->glass = &glass;
    GlassLayout gl = { 0.22f, -0.85f, { -0.52f, 0.0f, 0.0f }, 1, 0.035f, 0.45f, 0.45f };
    c->layout = gl;
    c->beltY = beltY;
    c->axleX[0] = FX;   c->axleX[1] = RX;
    c->axleY[0] = WR;   c->axleY[1] = WR;
    c->archR[0] = ARCH; c->archR[1] = ARCH;
    c->hw = HW;
    c->sill = SILL;
    c->wellDepth = HW - (TRACK - 0.5f * WW - 0.06f);
    c->doorX[0] = 0.50f; c->doorX[1] = -0.62f; c->nDoorX = 2;
    c->doorTopH = 0.93f; c->doorBottomH = 0.30f;     /* high sills */
    c->roofMat = MAT_PAINT;
    c->beltMat = MAT_CHROME;
    c->roofTrimMat = MAT_RUBBER;
    c->pillarTrimMat = MAT_PAINT;
    CabinSpec cab = { 0.88f, 0.10f, 0.62f, -0.75f, 0.0f, 0.0f, 0.36f, 0, MAT_PAINT, 0.20f };
    c->cabin = cab;
}

/* Two long bulges on the bonnet, swept along its surface. The ends dip
 * into the bonnet so the bulges rise out of it smoothly. */
static void powerDomes(void)
{
    Pt2 sec[16];
    for (int i = 0; i < 16; i++) {
        float t = 2.0f * (float)M_PI * i / 16;
        sec[i].x = 0.08f * cosf(t);
        sec[i].y = 0.028f * sinf(t);
    }
    setMaterial(MAT_PAINT);
    for (int s = -1; s <= 1; s += 2) {
        Vec3 path[6];
        for (int i = 0; i < 6; i++) {
            float x = 0.85f + 0.17f * i;
            Vec3 n, p = loftTop(&lower, x, s * 0.30f, &n);
            float lift = (i == 0 || i == 5) ? -0.03f : 0.008f;
            path[i] = vAdd(p, vScale(n, lift));
        }
        sweep(path, 6, 30, sec, 16, v3(0, 0, 1), 1);
    }
}

static void frontEnd(void)
{
    /* Wide, low oval grille with one thick horizontal chrome bar. */
    pushFrame(v3(2.202f, 0.45f, 0.0f), v3(1, 0, 0));
        setMaterial(MAT_MATTE);
        ellipseDisc(0.40f, 0.10f, 32);
        setMaterial(MAT_CHROME);
        Vec3 ring[24];
        for (int i = 0; i < 24; i++) {
            float t = 2.0f * (float)M_PI * i / 24;
            ring[i] = v3(0.41f * cosf(t), 0.11f * sinf(t), 0.004f);
        }
        tubeLoop(ring, 24, 0.012f);
        tube(v3(-0.39f, 0.0f, 0.02f), v3(0.39f, 0.0f, 0.02f), 0.022f, 0.022f, 12);
    glPopMatrix();

    for (int s = -1; s <= 1; s += 2) {
        /* Round headlamps faired into the wings, turned mostly forward. */
        Vec3 n, p = loftSide(&lower, 2.08f, 0.80f, (float)s, &n);
        Vec3 aim = vNorm(vAdd(n, v3(1.2f, 0.0f, 0.0f)));
        headlampRound(vAdd(p, vScale(n, 0.01f)), aim, 0.085f, 0.10f, MAT_PAINT, MAT_CHROME);

        p = loftSide(&lower, 2.14f, 0.42f, (float)s, &n);
        lampRect(p, n, 0.06f, 0.03f, 0.012f, MAT_INDICATOR, MAT_CHROME);
    }

    /* Slim chrome bumpers with overriders, front and rear. */
    setMaterial(MAT_CHROME);
    Vec3 fb[5] = { v3(1.98f, 0.36f, -0.80f), v3(2.16f, 0.36f, -0.60f), v3(2.25f, 0.36f, 0.0f),
                   v3(2.16f, 0.36f, 0.60f),  v3(1.98f, 0.36f, 0.80f) };
    sweepTube(fb, 5, 30, 0.022f, 10);
    Vec3 rb[5] = { v3(-1.98f, 0.40f, -0.80f), v3(-2.16f, 0.40f, -0.60f), v3(-2.25f, 0.40f, 0.0f),
                   v3(-2.16f, 0.40f, 0.60f),  v3(-1.98f, 0.40f, 0.80f) };
    sweepTube(rb, 5, 30, 0.022f, 10);
    for (int s = -1; s <= 1; s += 2) {
        roundedBox(2.24f, 0.38f, s * 0.30f, 0.06f, 0.12f, 0.05f, 4.0f);
        roundedBox(-2.24f, 0.42f, s * 0.30f, 0.06f, 0.12f, 0.05f, 4.0f);
    }

    numberPlate(v3(2.26f, 0.29f, 0.0f), v3(1, 0, 0), 0.40f, 0.10f);
    numberPlate(v3(-2.202f, 0.48f, 0.0f), v3(-1, 0, 0), 0.36f, 0.11f);
}

/* Eyebrows: a rounded lip following the top of every wheel arch. */
static void eyebrows(void)
{
    setMaterial(MAT_PAINT);
    for (int a = 0; a < 2; a++) {
        float ax = a ? RX : FX;
        for (int s = -1; s <= 1; s += 2) {
            Vec3 path[7];
            for (int i = 0; i < 7; i++) {
                float t = (20.0f + 140.0f * i / 6) * (float)M_PI / 180.0f;
                float x = ax + (ARCH + 0.02f) * cosf(t);
                float y = WR + (ARCH + 0.02f) * sinf(t);
                Vec3 n, p = loftSide(&lower, x, loftH(&lower, x, y), (float)s, &n);
                path[i] = vAdd(p, vScale(n, 0.004f));
            }
            sweepTube(path, 7, 28, 0.02f, 8);
        }
    }
}

static void sides(void)
{
    for (int s = -1; s <= 1; s += 2) {
        /* Gill vents behind the front wheels: dark slot, chrome bars. */
        Vec3 n, p = loftSide(&lower, 0.66f, 0.62f, (float)s, &n);
        pushFrame(p, n);
            setMaterial(MAT_MATTE);
            glTranslatef(0.0f, 0.0f, DECAL_LIFT);
            roundRectDisc(0.26f, 0.12f, 0.03f);
            setMaterial(MAT_CHROME);
            for (int k = -1; k <= 1; k += 2)
                tube(v3(-0.12f, k * 0.03f, 0.008f), v3(0.12f, k * 0.03f, 0.008f), 0.008f, 0.008f, 8);
        glPopMatrix();

        p = loftSide(&lower, -0.40f, 0.80f, (float)s, &n);
        doorHandle(p, n, 0.10f, MAT_CHROME);

        p = loftSide(&lower, 0.40f, 0.95f, (float)s, NULL);
        sideMirror(p, (float)s, MAT_CHROME, 0);

        /* Small vertical tail lamps. */
        p = loftSide(&lower, -2.15f, 0.62f, (float)s, &n);
        lampRect(p, n, 0.05f, 0.13f, 0.02f, MAT_TAIL, MAT_CHROME);

        /* Gullwing door seam over the roof, and down the glasshouse. */
        setMaterial(MAT_MATTE);
        Pt2 seam[2] = { { -0.52f, s * 0.30f }, { 0.15f, s * 0.30f } };
        loftStrip(&glass, seam, 2, 1, 1.0f, 0.005f, 0.002f);
    }

    /* Wipers parked at the base of the windscreen. */
    for (int i = 0; i < 2; i++) {
        float u0 = i ? 0.05f : -0.55f;
        Vec3 n, p = loftTop(&glass, 0.49f, u0, &n);
        Vec3 t = loftTop(&glass, 0.47f, u0 + 0.50f, NULL);
        wiper(p, t, n);
    }

    tailpipe(v3(-2.22f, 0.27f, 0.50f), 0.03f, 0.5f, MAT_CHROME);
}

static void buildBody(const CarModel *m)
{
    (void)m;
    ClosedCar c;
    setup(&c);
    drawClosedCarCommon(&c);
    powerDomes();
    frontEnd();
    eyebrows();
    sides();
}

static void buildGlass(const CarModel *m)
{
    (void)m;
    ClosedCar c;
    setup(&c);
    drawClosedCarGlass(&c);
}

void defineCar1954(CarModel *m)
{
    static const float paint[3]   = { 0.72f, 0.73f, 0.75f };   /* silver       */
    static const float accent[3]  = { 0.10f, 0.10f, 0.10f };
    static const float leather[3] = { 0.55f, 0.08f, 0.08f };   /* red interior */

    m->year = "1954";
    m->name = "300 SL Gullwing";
    m->length = 4.50f;
    memcpy(m->paint, paint, sizeof paint);
    memcpy(m->accent, accent, sizeof accent);
    memcpy(m->leather, leather, sizeof leather);

    m->numWheels = 4;
    m->axleX[0] = FX;     m->axleX[1] = RX;
    m->track[0] = TRACK;  m->track[1] = TRACK;

    WheelSpec w = { WR, WW, 0.20f, RIM_STEEL_CAP, MAT_PAINT, 0 };
    m->wheel[0] = w;
    m->wheel[1] = w;

    m->buildBody    = buildBody;
    m->buildGlass   = buildGlass;
    m->drawAnimated = NULL;
}
