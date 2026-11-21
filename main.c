#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "scene.h"
#include "cars.h"
#include "transition.h"
#include "hud.h"
#include "eras.h"
#include "props.h"

/* ------------------------------------------------------------------ */
/* Car motion state                                                    */
/* ------------------------------------------------------------------ */
#define MIN_SPEED        4.0f   /* units per second (about 14 km/h)   */
#define MAX_SPEED       30.0f   /* about 108 km/h                     */
#define SPEED_STEP       2.0f
#define AUTO_ORBIT_RATE  8.0f   /* degrees per second for the 'c' key */
#define WRAP_DISTANCE  (20.0f * SCENE_PERIOD)   /* 1920 m, a whole number of periods */

static float  carX      = 0.0f;   /* position along the road            */
static float  carSpeed  = 12.0f;  /* units per second (about 43 km/h)   */
static double distance  = 0.0;    /* total distance driven, for wheels  */
static int    paused    = 0;
static int    autoOrbit = 0;
static int    lastTime  = 0;      /* ms, from glutGet(GLUT_ELAPSED_TIME) */
static int    winW = 1024, winH = 640;

/* ------------------------------------------------------------------ */
/* Orbit camera                                                        */
/* The camera sits on a sphere around a target point and always looks */
/* at it. The target follows the car, so the camera follows too.      */
/* ------------------------------------------------------------------ */
static float camYaw      = 35.0f;   /* degrees around the y axis       */
static float camPitch    = 18.0f;   /* degrees above the ground        */
static float camDistance = 14.0f;   /* distance from the target        */

static void applyCamera(void)
{
    float targetX = carX, targetY = 1.0f, targetZ = CAR_LANE_Z;

    /* Spherical to Cartesian: convert (yaw, pitch, distance) into an
     * (x, y, z) camera position relative to the target. */
    float yaw   = camYaw   * (float)M_PI / 180.0f;
    float pitch = camPitch * (float)M_PI / 180.0f;

    float camX = targetX + camDistance * cosf(pitch) * sinf(yaw);
    float camY = targetY + camDistance * sinf(pitch);
    float camZ = targetZ + camDistance * cosf(pitch) * cosf(yaw);

    gluLookAt(camX, camY, camZ,          /* eye position      */
              targetX, targetY, targetZ, /* point looked at   */
              0.0f, 1.0f, 0.0f);         /* "up" direction    */
}

/* ------------------------------------------------------------------ */
/* The car of the current era, at carX, scaled by the timeline         */
/* ------------------------------------------------------------------ */
static void drawCurrentCar(void)
{
    float s = timelineCarScale();
    if (s < 0.01f) return;   /* fully shrunk: nothing to draw */

    glPushMatrix();
        glTranslatef(carX, 0.0f, CAR_LANE_Z);   /* move the whole car */
        /* Scaling about the car's own origin, which sits on the road,
         * makes it shrink down into its base rather than float away. */
        glScalef(s, s, s);
        drawCar(getCarModel(timelineModelIndex()), distance);
    glPopMatrix();
}

/* ------------------------------------------------------------------ */
/* Animation update                                                    */
/* ------------------------------------------------------------------ */
static void update(float dt)
{
    float step = carSpeed * dt;       /* distance = speed x time */
    carX     += step;
    distance += step;

    /* Floats lose precision as numbers grow, so after a long run the
     * car would start to jitter. Pulling carX back by a multiple of
     * SCENE_PERIOD is invisible, because the scene repeats exactly.
     * Tunnels live in world coordinates, so they move back too. */
    if (carX > WRAP_DISTANCE) {
        carX -= WRAP_DISTANCE;
        timelineShiftWorld(-WRAP_DISTANCE);
    }

    updateTimeline(dt, carX);
    updateProps(dt);     /* turbine rotors */
}

/* Called about 60 times per second. Delta time (dt) is the real time
 * since the previous call, so the car covers the same distance per
 * second whether the machine draws 30 or 120 frames per second. */
static void timer(int value)
{
    (void)value;
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - lastTime) / 1000.0f;
    lastTime = now;

    if (dt > 0.1f) dt = 0.1f;   /* avoid a big jump after a stall */
    if (!paused) update(dt);

    /* The automatic orbit keeps turning while paused, which is handy
     * for looking at a frozen car from every side. */
    if (autoOrbit) camYaw = fmodf(camYaw + AUTO_ORBIT_RATE * dt, 360.0f);

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

/* ------------------------------------------------------------------ */
/* GLUT callbacks                                                      */
/* ------------------------------------------------------------------ */
static void display(void)
{
    /* The era theme follows the car model. While a ripple runs, the
     * previous era is still visible further out, and the global look
     * (sky, fog, light) blends from the old theme to the new one. */
    const EraTheme    *cur  = getEraTheme(timelineModelIndex());
    const RippleState *rip  = timelineRipple();
    const EraTheme    *prev = rip->active ? getEraTheme(rip->fromEra) : NULL;

    EraTheme look;
    if (prev) blendThemeGlobals(&look, prev, cur, rippleGlobalFactor(rip));
    else      look = *cur;

    applyThemeGlobals(&look);   /* sky and fog colour, before clearing */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    applyCamera();                       /* 1. position the camera             */
    setupLighting(&look);                /* 2. sun in world coordinates        */
    drawBackdrop(carX, cur, prev, rip);  /* 3. distant skyline                 */
    drawScene(carX, cur, prev, rip);     /* 4. ground, road, roadside objects  */
    drawTunnels();                       /* 5. time tunnels, also in the world */
    drawCurrentCar();                    /* 6. the car itself                  */
    drawRippleRing(rip, cur->portal);    /* 7. see-through effects last        */
    drawParticles(carX);
    drawHUD(winW, winH, carSpeed, paused, autoOrbit);  /* 8. overlay */

    glutSwapBuffers();
}

static void reshape(int w, int h)
{
    if (h == 0) h = 1;
    winW = w;            /* the HUD needs the window size in pixels */
    winH = h;
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    /* 50 degree vertical field of view; near and far clipping planes at
     * 0.1 and 500 units. Anything outside that range is not drawn. */
    gluPerspective(50.0, (double)w / h, 0.1, 500.0);
    glMatrixMode(GL_MODELVIEW);
}

static void specialKeys(int key, int x, int y)
{
    (void)x; (void)y;
    switch (key) {
        case GLUT_KEY_LEFT:  camYaw   -= 3.0f; break;
        case GLUT_KEY_RIGHT: camYaw   += 3.0f; break;
        case GLUT_KEY_UP:    camPitch += 2.0f; break;
        case GLUT_KEY_DOWN:  camPitch -= 2.0f; break;
    }
    /* Keep the camera above the ground and below straight overhead. */
    if (camPitch < 3.0f)  camPitch = 3.0f;
    if (camPitch > 85.0f) camPitch = 85.0f;
}

static void keyboard(unsigned char key, int x, int y)
{
    (void)x; (void)y;
    switch (key) {
        case '+': case '=': camDistance -= 1.0f;     break;
        case '-': case '_': camDistance += 1.0f;     break;
        case ' ':           paused = !paused;        break;  /* pause / resume  */
        case 'n': case 'N': timelineNext();          break;  /* next era now    */
        case 'r': case 'R': timelineRestart();       break;  /* first era again */
        case 't': case 'T': timelineToggleEffect();  break;  /* tunnel / scale  */
        case 'c': case 'C': autoOrbit = !autoOrbit;  break;  /* camera orbit    */
        case '[':           carSpeed -= SPEED_STEP;  break;
        case ']':           carSpeed += SPEED_STEP;  break;
        case 27:            exit(0);                          /* Esc quits       */
    }
    if (camDistance < 5.0f)  camDistance = 5.0f;
    if (camDistance > 60.0f) camDistance = 60.0f;
    if (carSpeed < MIN_SPEED) carSpeed = MIN_SPEED;
    if (carSpeed > MAX_SPEED) carSpeed = MAX_SPEED;
}

/* ------------------------------------------------------------------ */
int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Mercedes-Benz: Evolution by Decade");

    initScene();
    initCars();       /* tessellate every body into a display list */
    initTimeline();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(specialKeys);
    glutKeyboardFunc(keyboard);

    lastTime = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}
