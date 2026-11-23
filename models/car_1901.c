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
/* 1901 Mercedes 35 HP                                                 */
/*                                                                     */
/* Long, low pressed-steel frame, flat honeycomb radiator at the very  */
/* front, long louvred bonnet, wooden dashboard with brass lamps, two  */
/* rows of high seats, raked steering column, artillery wheels.        */
/* Right-hand drive: the driver sits on the +z side.                   */
/* ------------------------------------------------------------------ */
#define FRONT_X   1.175f
#define REAR_X   -1.175f
#define WHEEL_R   0.46f
#define TRACK     0.70f
#define DRIVER_Z  0.24f

/* Bonnet: flat-topped box with rounded edges, from the dash to the
 * radiator. */
static void bonnetSection(float x, Section *s, const void *ctx)
{
    (void)ctx; (void)x;
    s->yBottom = 0.60f;
    s->yTop = 1.06f;
    s->halfWidth = 0.36f;
    s->topHalfWidth = 0.33f;
    s->roundness = 7.0f;
}
static const Loft bonnet = { bonnetSection, NULL, 0.72f, 1.62f, 24, 28, 1, 1, NULL, NULL };

/* Open body: rounded rear tonneau, a dip between the seat rows, and the
 * front seat well rising to the dash. */
static const Pt2 bodyTop[] = {
    { -1.90f, 0.86f }, { -1.80f, 1.05f }, { -1.55f, 1.12f }, { -1.00f, 1.10f },
    { -0.80f, 1.06f }, { -0.65f, 0.93f }, {  0.30f, 0.92f }, {  0.55f, 0.98f },
    {  0.72f, 1.04f },
};
static const Pt2 bodyHw[] = {
    { -1.90f, 0.40f }, { -1.70f, 0.52f }, { -1.00f, 0.52f }, { -0.60f, 0.47f },
    {  0.72f, 0.48f },
};
static const Pt2 bodyRound[] = { { -1.90f, 3.0f }, { -1.20f, 4.5f }, { 0.72f, 6.0f } };

static void bodySection(float x, Section *s, const void *ctx)
{
    (void)ctx;
    s->yBottom = 0.60f;
    s->yTop = curveY(bodyTop, COUNT(bodyTop), x);
    s->halfWidth = curveY(bodyHw, COUNT(bodyHw), x);
    s->topHalfWidth = 0.94f * s->halfWidth;
    s->roundness = curveY(bodyRound, COUNT(bodyRound), x);
}
static const Loft body = { bodySection, NULL, -1.90f, 0.72f, 56, 28, 1, 1, NULL, NULL };

/* Pressed-steel frame rails, axles, leaf springs, chain drive. */
static void chassis(void)
{
    setMaterial(MAT_ACCENT);
    for (int s = -1; s <= 1; s += 2) {
        roundedBox(0.0f, 0.55f, s * 0.42f, 3.62f, 0.12f, 0.05f, 6.0f);           /* rails   */
        roundedBox(FRONT_X, 0.49f, s * 0.42f, 0.90f, 0.04f, 0.07f, 4.0f);        /* springs */
        roundedBox(REAR_X, 0.49f, s * 0.42f, 0.90f, 0.04f, 0.07f, 4.0f);
    }
    roundedBox(1.72f, 0.55f, 0.0f, 0.06f, 0.10f, 0.84f, 6.0f);                    /* front cross */

    setMaterial(MAT_DARK_METAL);
    tube(v3(FRONT_X, WHEEL_R, -TRACK + 0.08f), v3(FRONT_X, WHEEL_R, TRACK - 0.08f), 0.028f, 0.028f, 10);
    tube(v3(REAR_X, WHEEL_R, -TRACK + 0.08f), v3(REAR_X, WHEEL_R, TRACK - 0.08f), 0.03f, 0.03f, 10);

    /* Chain drive from a countershaft to each rear wheel. */
    for (int s = -1; s <= 1; s += 2) {
        float z = s * 0.55f;
        tubeCapped(v3(-0.75f, 0.50f, z - 0.01f), v3(-0.75f, 0.50f, z + 0.01f), 0.07f, 0.07f, 16);
        tubeCapped(v3(REAR_X, WHEEL_R, z - 0.01f), v3(REAR_X, WHEEL_R, z + 0.01f), 0.13f, 0.13f, 18);
        setMaterial(MAT_MATTE);
        tube(v3(-0.75f, 0.57f, z), v3(REAR_X, WHEEL_R + 0.13f, z), 0.009f, 0.009f, 6);
        tube(v3(-0.75f, 0.43f, z), v3(REAR_X, WHEEL_R - 0.13f, z), 0.009f, 0.009f, 6);
        setMaterial(MAT_DARK_METAL);
    }
}

/* Flat honeycomb radiator: brass frame, dark core, a fine grid of
 * lines over the core, and a brass filler cap on top. */
static void radiator(void)
{
    setMaterial(MAT_BRASS);
    roundedBox(1.67f, 0.86f, 0.0f, 0.10f, 0.60f, 0.78f, 6.0f);

    pushFrame(v3(1.722f, 0.86f, 0.0f), v3(1.0f, 0.0f, 0.0f));
        setMaterial(MAT_MATTE);
        quadXY(-0.33f, -0.24f, 0.33f, 0.24f);
        setMaterialScaled(MAT_BRASS, 0.55f);
        glTranslatef(0.0f, 0.0f, 0.002f);
        for (int i = 0; i <= 16; i++) {                  /* horizontal lines */
            float y = -0.24f + 0.03f * i;
            quadXY(-0.33f, y - 0.002f, 0.33f, y + 0.002f);
        }
        for (int i = 0; i <= 22; i++) {                  /* vertical lines */
            float x = -0.33f + 0.03f * i;
            quadXY(x - 0.002f, -0.24f, x + 0.002f, 0.24f);
        }
    glPopMatrix();

    pushFrame(v3(1.67f, 1.16f, 0.0f), v3(0.0f, 1.0f, 0.0f));
        setMaterial(MAT_BRASS);
        Pt2 cap[] = { { 0.05f, 0.0f }, { 0.05f, 0.03f }, { 0.05f, 0.03f },
                      { 0.035f, 0.05f }, { 0.0f, 0.055f } };
        revolve(cap, COUNT(cap), 16);
    glPopMatrix();
}

/* Bonnet: louvres in two rows on each side, brass hinge line on top. */
static void bonnetDetails(void)
{
    for (int s = -1; s <= 1; s += 2) {
        for (int row = 0; row < 2; row++) {
            float h = row ? 0.66f : 0.40f;
            for (int i = 0; i < 9; i++) {
                float x = 0.90f + 0.075f * i;
                Vec3 n, p = loftSide(&bonnet, x, h, (float)s, &n);
                louvre(p, n, 0.035f, 0.10f, MAT_PAINT);
            }
        }
    }
    setMaterial(MAT_BRASS);
    Pt2 hinge[2] = { { 0.74f, 0.0f }, { 1.60f, 0.0f } };
    loftStrip(&bonnet, hinge, 2, 1, 1.0f, 0.015f, 0.003f);
}

/* Mudguards: thin curved wings swept over each wheel, with short
 * running boards between them. */
static void mudguards(void)
{
    /* A flat, slightly rounded section 0.20 m wide and 1.6 cm thick. */
    Pt2 sec[16];
    for (int i = 0; i < 16; i++) {
        float t = 2.0f * (float)M_PI * i / 16, c = cosf(t), sn = sinf(t);
        sec[i].x = 0.10f * (c >= 0 ? sqrtf(c) : -sqrtf(-c));
        sec[i].y = 0.008f * (sn >= 0 ? sqrtf(sn) : -sqrtf(-sn));
    }
    float R = WHEEL_R + 0.08f;

    setMaterial(MAT_ACCENT);
    for (int s = -1; s <= 1; s += 2) {
        float z = s * TRACK;
        Vec3 front[7], rear[7];
        const float fa[6] = { 10.0f, 45.0f, 90.0f, 130.0f, 160.0f, 178.0f };
        for (int i = 0; i < 6; i++) {
            float a = fa[i] * (float)M_PI / 180.0f;
            front[i] = v3(FRONT_X + R * cosf(a), WHEEL_R + R * sinf(a), z);
        }
        front[6] = v3(0.55f, 0.50f, z);
        sweep(front, 7, 40, sec, 16, v3(0, 0, 1), 1);

        rear[0] = v3(-0.55f, 0.50f, z);
        const float ra[6] = { 2.0f, 30.0f, 70.0f, 110.0f, 150.0f, 175.0f };
        for (int i = 0; i < 6; i++) {
            float a = ra[i] * (float)M_PI / 180.0f;
            rear[i + 1] = v3(REAR_X + R * cosf(a), WHEEL_R + R * sinf(a), z);
        }
        sweep(rear, 7, 40, sec, 16, v3(0, 0, 1), 1);

        boxAt(0.0f, 0.50f, z, 1.12f, 0.025f, 0.22f);                     /* running board */
        setMaterial(MAT_DARK_METAL);                                      /* stays */
        tube(v3(FRONT_X + 0.35f, 0.86f, z), v3(FRONT_X + 0.20f, 0.56f, s * 0.44f), 0.008f, 0.008f, 6);
        tube(v3(REAR_X - 0.35f, 0.86f, z), v3(REAR_X - 0.20f, 0.56f, s * 0.44f), 0.008f, 0.008f, 6);
        setMaterial(MAT_ACCENT);
    }
}

/* Wooden dashboard with brass carbide lamps on its sides. */
static void dashboard(void)
{
    setMaterial(MAT_WOOD);
    roundedBox(0.70f, 0.86f, 0.0f, 0.05f, 0.56f, 1.02f, 8.0f);
    for (int s = -1; s <= 1; s += 2) {
        setMaterial(MAT_BRASS);
        tube(v3(0.72f, 0.98f, s * 0.50f), v3(0.78f, 0.98f, s * 0.56f), 0.012f, 0.012f, 8);
        headlampRound(v3(0.84f, 1.02f, s * 0.56f), v3(1.0f, 0.0f, 0.0f),
                      0.075f, 0.16f, MAT_BRASS, MAT_BRASS);
        ellipsoid(0.74f, 1.12f, s * 0.56f, 0.03f, 0.04f, 0.03f);          /* chimney cap */
    }
}

/* Seats, steering, levers, bulb horn and the driver. */
static void cockpit(void)
{
    setMaterial(MAT_LEATHER);
    loftTopPatch(&body, -1.55f, -0.85f, -0.85f, 0.85f, 8, 8, 0.004f);
    loftTopPatch(&body, -0.55f, 0.45f, -0.85f, 0.85f, 8, 8, 0.004f);

    seat(-0.40f, 0.99f,  DRIVER_Z, 0.44f, 0.48f, 0.50f, 10.0f, 0);
    seat(-0.40f, 0.99f, -DRIVER_Z, 0.44f, 0.48f, 0.50f, 10.0f, 0);
    seat(-1.52f, 1.16f, 0.0f, 0.95f, 0.45f, 0.42f, 12.0f, 0);

    /* Raked column up to a large wooden wheel. */
    Vec3 wc = v3(0.28f, 1.25f, DRIVER_Z);
    setMaterial(MAT_DARK_METAL);
    tube(v3(0.64f, 0.72f, DRIVER_Z), wc, 0.022f, 0.022f, 8);
    steeringWheel(wc, vNorm(v3(-0.55f, 0.83f, 0.0f)), 0.21f, MAT_WOOD, 4);

    /* Gear and brake levers outside the body on the driver's side. */
    for (int i = 0; i < 2; i++) {
        float x = i ? -0.25f : -0.08f;
        setMaterial(MAT_BRASS);
        tube(v3(x, 0.72f, 0.54f), v3(x + 0.08f, 1.20f, 0.56f), 0.014f, 0.012f, 8);
        ellipsoid(x + 0.08f, 1.22f, 0.56f, 0.025f, 0.025f, 0.025f);
    }
    setMaterial(MAT_DARK_METAL);
    roundedBox(-0.16f, 0.74f, 0.53f, 0.30f, 0.05f, 0.03f, 4.0f);         /* quadrant */

    /* Bulb horn: brass tube ending in a flared bell, rubber bulb. */
    /* It runs below the right-hand lamp to clear its housing. */
    Vec3 hp[] = { v3(0.45f, 0.95f, 0.50f), v3(0.62f, 0.90f, 0.60f), v3(0.95f, 0.84f, 0.62f) };
    setMaterial(MAT_BRASS);
    sweepTube(hp, 3, 16, 0.012f, 8);
    pushFrame(hp[2], v3(1.0f, 0.0f, 0.0f));
        Pt2 bell[] = { { 0.014f, 0.0f }, { 0.03f, 0.08f }, { 0.065f, 0.13f } };
        revolve(bell, COUNT(bell), 16);
    glPopMatrix();
    setMaterial(MAT_RUBBER);
    ellipsoid(0.42f, 0.95f, 0.49f, 0.045f, 0.045f, 0.045f);

    driverFigure(v3(-0.22f, 1.02f, DRIVER_Z), wc, 0.21f, 1);
}

static void rear(void)
{
    /* Small oil tail lamp on the right. */
    setMaterial(MAT_BRASS);
    roundedBox(-1.94f, 0.95f, 0.38f, 0.07f, 0.09f, 0.08f, 4.0f);
    lampRect(v3(-1.975f, 0.95f, 0.38f), v3(-1.0f, 0.0f, 0.0f), 0.045f, 0.045f, 0.02f,
             MAT_TAIL, MAT_BRASS);
    tailpipe(v3(-1.96f, 0.45f, -0.30f), 0.03f, 1.2f, MAT_DARK_METAL);
    underbody(-1.80f, 1.70f, 0.49f, 0.40f);
}

static void buildBody(const CarModel *m)
{
    (void)m;
    setMaterial(MAT_PAINT);
    drawLoft(&body);
    drawLoft(&bonnet);
    chassis();
    radiator();
    bonnetDetails();
    mudguards();
    dashboard();
    cockpit();
    rear();
}

void defineCar1901(CarModel *m)
{
    static const float paint[3]   = { 0.55f, 0.06f, 0.07f };   /* deep red   */
    static const float accent[3]  = { 0.05f, 0.05f, 0.05f };   /* black      */
    static const float leather[3] = { 0.20f, 0.11f, 0.07f };

    m->year = "1901";
    m->name = "Mercedes 35 HP";
    m->length = 3.75f;
    memcpy(m->paint, paint, sizeof paint);
    memcpy(m->accent, accent, sizeof accent);
    memcpy(m->leather, leather, sizeof leather);

    m->numWheels = 4;
    m->axleX[0] = FRONT_X;  m->axleX[1] = REAR_X;
    m->track[0] = TRACK;    m->track[1] = TRACK;

    WheelSpec w = { WHEEL_R, 0.09f, WHEEL_R - 0.06f, RIM_ARTILLERY, MAT_WOOD, 0 };
    m->wheel[0] = w;
    m->wheel[1] = w;

    m->buildBody    = buildBody;
    m->buildGlass   = NULL;
    m->drawAnimated = NULL;
}
