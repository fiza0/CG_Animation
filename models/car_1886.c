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
/* 1886 Benz Patent-Motorwagen                                         */
/*                                                                     */
/* Not a body but a light frame of bent steel tubes on three wire     */
/* wheels: one small front wheel steered by a tiller, two large rear   */
/* wheels driven by belt and chains from a single-cylinder engine     */
/* with a big horizontal flywheel at the back.                        */
/* ------------------------------------------------------------------ */
#define FRONT_X     0.85f
#define REAR_X     -0.85f
#define FRONT_R     0.34f
#define REAR_R      0.62f
#define REAR_TRACK  0.62f
#define FLY_X      -1.22f
#define FLY_Y       0.56f   /* below the crankcase */
#define FLY_R       0.29f
#define FLY_RATE  400.0f      /* flywheel degrees per metre driven */

static void frame(void)
{
    setMaterial(MAT_ACCENT);   /* black-painted steel tube */

    /* Two side rails, bent upwards and inwards to meet at the steering
     * head. A smooth spline through a few points makes the bends. */
    for (int s = -1; s <= 1; s += 2) {
        Vec3 rail[] = {
            v3(-1.10f, 0.62f, s * 0.34f), v3(-0.60f, 0.60f, s * 0.34f),
            v3( 0.00f, 0.58f, s * 0.33f), v3( 0.40f, 0.60f, s * 0.27f),
            v3( 0.64f, 0.74f, s * 0.15f), v3( 0.74f, 0.92f, s * 0.07f),
        };
        sweepTube(rail, COUNT(rail), 30, 0.021f, 8);
    }
    const float cx[3] = { -1.05f, -0.55f, 0.0f }, cy[3] = { 0.62f, 0.60f, 0.58f };
    for (int i = 0; i < 3; i++)                                  /* cross members */
        tube(v3(cx[i], cy[i], -0.34f), v3(cx[i], cy[i], 0.34f), 0.018f, 0.018f, 8);

    /* Rear axle between the hubs. */
    setMaterial(MAT_DARK_METAL);
    tube(v3(REAR_X, REAR_R, -REAR_TRACK + 0.05f), v3(REAR_X, REAR_R, REAR_TRACK - 0.05f),
         0.022f, 0.022f, 10);

    /* Steering head and the fork down to the front axle. */
    tubeCapped(v3(0.74f, 0.86f, 0.0f), v3(0.77f, 1.02f, 0.0f), 0.032f, 0.032f, 12);
    for (int s = -1; s <= 1; s += 2)
        tube(v3(0.75f, 0.90f, s * 0.075f), v3(FRONT_X, FRONT_R, s * 0.075f), 0.016f, 0.014f, 8);
    tube(v3(FRONT_X, FRONT_R, -0.085f), v3(FRONT_X, FRONT_R, 0.085f), 0.012f, 0.012f, 8);
    tube(v3(0.75f, 0.90f, -0.08f), v3(0.75f, 0.90f, 0.08f), 0.014f, 0.014f, 8);
}

/* Tiller: a vertical column in front of the seat with a crank handle on
 * top, linked by a rod to the steering head. */
static void tiller(void)
{
    setMaterial(MAT_DARK_METAL);
    tube(v3(0.50f, 0.64f, 0.0f), v3(0.46f, 1.24f, 0.0f), 0.016f, 0.016f, 8);
    tube(v3(0.49f, 0.66f, -0.27f), v3(0.49f, 0.66f, 0.27f), 0.012f, 0.012f, 8);  /* bracket */
    tube(v3(0.495f, 0.74f, 0.0f), v3(0.75f, 0.92f, 0.0f), 0.010f, 0.010f, 6);    /* link rod */
    tube(v3(0.46f, 1.24f, 0.0f), v3(0.32f, 1.27f, 0.0f), 0.012f, 0.012f, 8);     /* crank arm */
    setMaterial(MAT_WOOD);
    ellipsoid(0.32f, 1.30f, 0.0f, 0.022f, 0.04f, 0.022f);                         /* knob */
    setMaterial(MAT_BRASS);
    tubeCapped(v3(0.465f, 1.20f, 0.0f), v3(0.46f, 1.235f, 0.0f), 0.024f, 0.024f, 12);
}

/* Wooden seat box, upholstered bench, footboard and dashboard board. */
static void seatAndBoards(void)
{
    setMaterial(MAT_WOOD);
    roundedBox(-0.35f, 0.72f, 0.0f, 0.62f, 0.20f, 0.74f, 8.0f);   /* seat box  */
    boxAt(0.36f, 0.63f, 0.0f, 0.36f, 0.025f, 0.56f);               /* footboard */
    roundedBox(0.58f, 0.78f, 0.0f, 0.03f, 0.30f, 0.56f, 6.0f);     /* dashboard */

    seat(-0.55f, 0.92f, 0.0f, 0.82f, 0.50f, 0.46f, 8.0f, 0);

    setMaterial(MAT_LEATHER);                                      /* arm rolls */
    for (int s = -1; s <= 1; s += 2)
        tube(v3(-0.58f, 1.04f, s * 0.42f), v3(-0.12f, 1.00f, s * 0.42f), 0.028f, 0.028f, 10);
}

/* Engine: horizontal cylinder with brass bands, crankcase, vertical
 * crankshaft for the flywheel, water tank and a brass oil cup. */
static void engine(void)
{
    setMaterial(MAT_DARK_METAL);
    tubeCapped(v3(-1.06f, 0.82f, 0.0f), v3(-0.72f, 0.82f, 0.0f), 0.10f, 0.10f, 16);
    roundedBox(-1.12f, 0.78f, 0.0f, 0.16f, 0.22f, 0.24f, 4.0f);     /* crankcase */
    tube(v3(FLY_X, 0.50f, 0.0f), v3(FLY_X, 0.92f, 0.0f), 0.022f, 0.022f, 10);

    setMaterial(MAT_BRASS);
    for (int i = 0; i < 3; i++) {
        float x = -1.00f + 0.11f * i;
        tube(v3(x, 0.82f, 0.0f), v3(x + 0.02f, 0.82f, 0.0f), 0.106f, 0.106f, 16);
    }
    tubeCapped(v3(-0.90f, 0.92f, 0.0f), v3(-0.90f, 0.98f, 0.0f), 0.025f, 0.02f, 10);

    /* Water tank behind the seat, with brass bands and thin legs. */
    setMaterial(MAT_DARK_METAL);
    tubeCapped(v3(-0.78f, 0.95f, 0.0f), v3(-0.78f, 1.18f, 0.0f), 0.09f, 0.09f, 16);
    setMaterial(MAT_BRASS);
    tube(v3(-0.78f, 0.99f, 0.0f), v3(-0.78f, 1.01f, 0.0f), 0.094f, 0.094f, 16);
    tube(v3(-0.78f, 1.13f, 0.0f), v3(-0.78f, 1.15f, 0.0f), 0.094f, 0.094f, 16);
    setMaterial(MAT_DARK_METAL);
    for (int s = -1; s <= 1; s += 2)
        tube(v3(-0.78f, 0.95f, s * 0.06f), v3(-0.74f, 0.80f, s * 0.12f), 0.008f, 0.008f, 6);
}

/* Drive: a flat belt from the engine pulley to a countershaft under
 * the seat, then a chain on each side to the rear wheels. */
static void drivetrain(void)
{
    const float bz = 0.34f;   /* outside the flywheel rim */
    setMaterial(MAT_DARK_METAL);
    tubeCapped(v3(-1.00f, 0.62f, bz - 0.03f), v3(-1.00f, 0.62f, bz + 0.03f), 0.10f, 0.10f, 18);
    tubeCapped(v3(-0.35f, 0.50f, bz - 0.03f), v3(-0.35f, 0.50f, bz + 0.03f), 0.10f, 0.10f, 18);
    tube(v3(-0.35f, 0.50f, -0.52f), v3(-0.35f, 0.50f, 0.52f), 0.018f, 0.018f, 8);

    setMaterial(MAT_LEATHER);                                     /* belt */
    tube(v3(-1.00f, 0.72f, bz), v3(-0.35f, 0.60f, bz), 0.008f, 0.008f, 6);
    tube(v3(-1.00f, 0.52f, bz), v3(-0.35f, 0.40f, bz), 0.008f, 0.008f, 6);

    for (int s = -1; s <= 1; s += 2) {
        float z = s * 0.52f;
        setMaterial(MAT_DARK_METAL);                              /* sprockets */
        tubeCapped(v3(-0.35f, 0.50f, z - 0.01f), v3(-0.35f, 0.50f, z + 0.01f), 0.07f, 0.07f, 16);
        tubeCapped(v3(REAR_X, REAR_R, z - 0.01f), v3(REAR_X, REAR_R, z + 0.01f), 0.10f, 0.10f, 18);
        setMaterial(MAT_MATTE);                                   /* chains */
        tube(v3(-0.35f, 0.57f, z), v3(REAR_X, REAR_R + 0.10f, z), 0.008f, 0.008f, 6);
        tube(v3(-0.35f, 0.43f, z), v3(REAR_X, REAR_R - 0.10f, z), 0.008f, 0.008f, 6);
    }
}

static void buildBody(const CarModel *m)
{
    (void)m;
    frame();
    tiller();
    seatAndBoards();
    engine();
    drivetrain();

    /* Driver sits in the middle of the bench, behind the central tiller,
     * both hands on the crank knob. A tiny "wheel radius" brings the
     * hands together around the knob; feet rest on the footboard. */
    driverFigure(v3(-0.32f, 0.95f, 0.0f), v3(0.32f, 1.30f, 0.0f), 0.06f, 0);
}

/* The flywheel turns with the engine, so it is drawn every frame. Its
 * angle grows with the distance driven, like the wheels. */
static void drawAnimated(const CarModel *m, double distance)
{
    (void)m;
    float angle = (float)fmod(distance * FLY_RATE, 360.0);
    glPushMatrix();
        glTranslatef(FLY_X, FLY_Y, 0.0f);
        glRotatef(angle, 0.0f, 1.0f, 0.0f);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);   /* local xy plane -> horizontal */
        setMaterial(MAT_DARK_METAL);
        glutSolidTorus(0.03, FLY_R, 10, 32);    /* heavy rim */
        for (int i = 0; i < 4; i++) {
            float a = (float)M_PI * 0.5f * i;
            tube(v3(0, 0, 0), v3(FLY_R * cosf(a), FLY_R * sinf(a), 0.0f), 0.014f, 0.014f, 6);
        }
        setMaterial(MAT_BRASS);                 /* bright hub shows the spin */
        ellipsoid(0.0f, 0.0f, 0.0f, 0.04f, 0.04f, 0.03f);
        ellipsoid(0.7f * FLY_R, 0.0f, 0.0f, 0.03f, 0.03f, 0.025f);
    glPopMatrix();
}

void defineCar1886(CarModel *m)
{
    static const float paint[3]   = { 0.06f, 0.06f, 0.06f };
    static const float accent[3]  = { 0.06f, 0.06f, 0.07f };
    static const float leather[3] = { 0.36f, 0.12f, 0.08f };

    m->year = "1886";
    m->name = "Benz Patent-Motorwagen";
    m->length = 2.72f;
    memcpy(m->paint, paint, sizeof paint);
    memcpy(m->accent, accent, sizeof accent);
    memcpy(m->leather, leather, sizeof leather);

    m->numWheels = 3;
    m->axleX[0] = FRONT_X;  m->axleX[1] = REAR_X;
    m->track[0] = 0.0f;     m->track[1] = REAR_TRACK;

    /* Tall, thin wheels with solid rubber tyres. */
    WheelSpec front = { FRONT_R, 0.04f, FRONT_R - 0.025f, RIM_WIRE, MAT_DARK_METAL, 0 };
    WheelSpec rear  = { REAR_R,  0.05f, REAR_R  - 0.030f, RIM_WIRE, MAT_DARK_METAL, 0 };
    m->wheel[0] = front;
    m->wheel[1] = rear;

    m->buildBody    = buildBody;
    m->buildGlass   = NULL;
    m->drawAnimated = drawAnimated;
}
