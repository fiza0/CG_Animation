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

#define MIN_SPEED        4.0f
#define MAX_SPEED       30.0f
#define SPEED_STEP       2.0f
#define AUTO_ORBIT_RATE  8.0f
#define WRAP_DISTANCE  (20.0f * SCENE_PERIOD)

static float  carX      = 0.0f;
static float  carSpeed  = 12.0f;
static double distance  = 0.0;
static int    paused    = 0;
static int    autoOrbit = 0;
static int    lastTime  = 0;
static int    winW = 1024, winH = 640;

static float camYaw      = 35.0f;
static float camPitch    = 18.0f;
static float camDistance = 14.0f;

typedef struct { const char *name; float yaw, pitch, dist; } CamPreset;
static const CamPreset presets[] = {
    { "side view",          0.0f,  6.0f,  9.0f },
    { "front three-quarter", 55.0f, 14.0f, 9.0f },
    { "rear three-quarter", -55.0f, 14.0f, 9.0f },
    { "low front",          70.0f,  3.0f,  7.0f },
    { "top-down",            0.0f, 85.0f, 11.0f },
};
static int presetIndex = -1;

static void applyCamera(void)
{
    float targetX = carX, targetY = 1.0f, targetZ = CAR_LANE_Z;

    float yaw   = camYaw   * (float)M_PI / 180.0f;
    float pitch = camPitch * (float)M_PI / 180.0f;

    float camX = targetX + camDistance * cosf(pitch) * sinf(yaw);
    float camY = targetY + camDistance * sinf(pitch);
    float camZ = targetZ + camDistance * cosf(pitch) * cosf(yaw);

    gluLookAt(camX, camY, camZ,
              targetX, targetY, targetZ,
              0.0f, 1.0f, 0.0f);
}

static int pushCarTransform(void)
{
    float s = timelineCarScale();
    if (s < 0.01f) return 0;

    glPushMatrix();
    glTranslatef(carX, 0.0f, CAR_LANE_Z);
    glScalef(s, s, s);
    return 1;
}

static void drawCurrentCar(void)
{
    if (!pushCarTransform()) return;
    drawCar(getCarModel(timelineModelIndex()), distance);
    glPopMatrix();
}

static void drawCurrentCarGlass(void)
{
    if (!pushCarTransform()) return;
    drawCarGlass(getCarModel(timelineModelIndex()));
    glPopMatrix();
}

static void update(float dt)
{
    float step = carSpeed * dt;
    carX     += step;
    distance += step;

    if (carX > WRAP_DISTANCE) {
        carX -= WRAP_DISTANCE;
        timelineShiftWorld(-WRAP_DISTANCE);
    }

    updateTimeline(dt, carX);
    updateProps(dt);
}

static void timer(int value)
{
    (void)value;
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - lastTime) / 1000.0f;
    lastTime = now;

    if (dt > 0.1f) dt = 0.1f;
    if (!paused) update(dt);

    if (autoOrbit) camYaw = fmodf(camYaw + AUTO_ORBIT_RATE * dt, 360.0f);

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

static void display(void)
{
    const EraTheme    *cur  = getEraTheme(timelineModelIndex());
    const RippleState *rip  = timelineRipple();
    const EraTheme    *prev = rip->active ? getEraTheme(rip->fromEra) : NULL;

    EraTheme look;
    if (prev) blendThemeGlobals(&look, prev, cur, rippleGlobalFactor(rip));
    else      look = *cur;

    applyThemeGlobals(&look);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    applyCamera();
    setupLighting(&look);
    drawBackdrop(carX, cur, prev, rip);
    drawScene(carX, cur, prev, rip);
    drawTunnels();
    drawCurrentCar();
    drawCurrentCarGlass();
    drawRippleRing(rip, cur->portal);
    drawParticles(carX);
    drawHUD(winW, winH, carSpeed, paused, autoOrbit,
            presetIndex >= 0 ? presets[presetIndex].name : NULL);

    glutSwapBuffers();
}

static void reshape(int w, int h)
{
    if (h == 0) h = 1;
    winW = w;
    winH = h;
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
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
    if (camPitch < 3.0f)  camPitch = 3.0f;
    if (camPitch > 85.0f) camPitch = 85.0f;
}

static void keyboard(unsigned char key, int x, int y)
{
    (void)x; (void)y;
    switch (key) {
        case '+': case '=': camDistance -= 1.0f;     break;
        case '-': case '_': camDistance += 1.0f;     break;
        case ' ':           paused = !paused;        break;
        case 'n': case 'N': timelineNext();          break;
        case 'r': case 'R': timelineRestart();
                            sceneHideNearSide(0);    break;
        case 'm': case 'M':
            timelineSetInspect(!timelineInspecting());
            sceneHideNearSide(timelineInspecting());
            break;
        case ',': case '<': timelineInspectStep(-1); break;
        case '.': case '>': timelineInspectStep(+1); break;
        case 'v': case 'V':
            presetIndex = (presetIndex + 1) % (int)(sizeof presets / sizeof presets[0]);
            camYaw      = presets[presetIndex].yaw;
            camPitch    = presets[presetIndex].pitch;
            camDistance = presets[presetIndex].dist;
            break;
        case 't': case 'T': timelineToggleEffect();  break;
        case 'c': case 'C': autoOrbit = !autoOrbit;  break;
        case '[':           carSpeed -= SPEED_STEP;  break;
        case ']':           carSpeed += SPEED_STEP;  break;
        case 27:            exit(0);
    }
    if (camDistance < 5.0f)  camDistance = 5.0f;
    if (camDistance > 60.0f) camDistance = 60.0f;
    if (carSpeed < MIN_SPEED) carSpeed = MIN_SPEED;
    if (carSpeed > MAX_SPEED) carSpeed = MAX_SPEED;
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Mercedes-Benz: Evolution by Decade");

    initScene();
    initCars();
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
