#include <math.h>
#include <stdlib.h>
#include "scene.h"

/* ------------------------------------------------------------------ */
/* Car motion state                                                    */
/* ------------------------------------------------------------------ */
#define CAR_LANE_Z    -2.0f   /* centre of the far lane                  */
#define WHEEL_RADIUS   0.4f
#define WHEEL_WIDTH    0.3f

static float carX       = 0.0f;   /* position along the road            */
static float carSpeed   = 12.0f;  /* units per second (about 43 km/h)   */
static float wheelAngle = 0.0f;   /* degrees, grows as the car rolls    */
static int   paused     = 0;
static int   lastTime   = 0;      /* ms, from glutGet(GLUT_ELAPSED_TIME) */

static GLUquadric *wheelQuad = NULL;

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
/* Wheel: a tyre (cylinder), two hub caps (discs) and a bar across     */
/* the face. The bar is what makes the rotation visible.               */
/* ------------------------------------------------------------------ */
static void drawWheel(float x, float z)
{
    glPushMatrix();
        glTranslatef(x, WHEEL_RADIUS, z);   /* wheel touches the road */

        /* Rolling forward (+x) is a clockwise turn when seen from +z,
         * which is a NEGATIVE rotation about the z axis (the axle). */
        glRotatef(-wheelAngle, 0.0f, 0.0f, 1.0f);

        glTranslatef(0.0f, 0.0f, -WHEEL_WIDTH / 2.0f); /* centre on axle */

        glColor3f(0.08f, 0.08f, 0.08f);               /* tyre */
        gluCylinder(wheelQuad, WHEEL_RADIUS, WHEEL_RADIUS, WHEEL_WIDTH, 24, 1);

        glColor3f(0.75f, 0.75f, 0.78f);               /* hub caps */
        gluQuadricOrientation(wheelQuad, GLU_INSIDE); /* inner face points -z */
        gluDisk(wheelQuad, 0.0, WHEEL_RADIUS * 0.7, 24, 1);
        gluQuadricOrientation(wheelQuad, GLU_OUTSIDE);
        glTranslatef(0.0f, 0.0f, WHEEL_WIDTH);
        gluDisk(wheelQuad, 0.0, WHEEL_RADIUS * 0.7, 24, 1);

        glColor3f(0.15f, 0.15f, 0.15f);               /* spoke bars */
        glTranslatef(0.0f, 0.0f, -WHEEL_WIDTH / 2.0f);
        glPushMatrix();
            glScalef(WHEEL_RADIUS * 1.3f, 0.07f, WHEEL_WIDTH + 0.02f);
            glutSolidCube(1.0);
        glPopMatrix();
        glScalef(0.07f, WHEEL_RADIUS * 1.3f, WHEEL_WIDTH + 0.02f);
        glutSolidCube(1.0);
    glPopMatrix();
}

/* ------------------------------------------------------------------ */
/* Placeholder car: two boxes and four wheels, drawn at carX           */
/* ------------------------------------------------------------------ */
static void drawPlaceholderCar(void)
{
    glPushMatrix();
        glTranslatef(carX, 0.0f, CAR_LANE_Z);  /* move the whole car */

        glColor3f(0.75f, 0.10f, 0.10f);
        glPushMatrix();                         /* lower body: 4.5 x 0.8 x 1.8 */
            glTranslatef(0.0f, 0.75f, 0.0f);
            glScalef(4.5f, 0.8f, 1.8f);
            glutSolidCube(1.0);
        glPopMatrix();

        glPushMatrix();                         /* cabin: 2.4 x 0.7 x 1.6 */
            glTranslatef(-0.3f, 1.5f, 0.0f);
            glScalef(2.4f, 0.7f, 1.6f);
            glutSolidCube(1.0);
        glPopMatrix();

        /* Wheels poke slightly out of the body's sides (body half-width 0.9). */
        drawWheel( 1.4f,  0.85f);   /* front, near side */
        drawWheel( 1.4f, -0.85f);   /* front, far side  */
        drawWheel(-1.4f,  0.85f);   /* rear,  near side */
        drawWheel(-1.4f, -0.85f);   /* rear,  far side  */
    glPopMatrix();
}

/* ------------------------------------------------------------------ */
/* Animation update                                                    */
/* ------------------------------------------------------------------ */
static void update(float dt)
{
    float distance = carSpeed * dt;       /* distance = speed x time */
    carX += distance;

    /* Rolling without slipping: the arc length the tyre turns through
     * equals the distance travelled, so angle (radians) = distance / r. */
    wheelAngle += (distance / WHEEL_RADIUS) * 180.0f / (float)M_PI;
    if (wheelAngle >= 360.0f) wheelAngle = fmodf(wheelAngle, 360.0f);

    /* Floats lose precision as numbers grow, so after a long run the
     * car would start to jitter. Pulling carX back by a multiple of
     * SCENE_PERIOD is invisible, because the scene repeats exactly. */
    if (carX > 100.0f * SCENE_PERIOD) carX -= 100.0f * SCENE_PERIOD;
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

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

/* ------------------------------------------------------------------ */
/* GLUT callbacks                                                      */
/* ------------------------------------------------------------------ */
static void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    applyCamera();       /* 1. position the camera                  */
    setupLighting();     /* 2. place the sun in world coordinates   */
    drawScene(carX);     /* 3. draw the world around the car        */
    drawPlaceholderCar();

    glutSwapBuffers();
}

static void reshape(int w, int h)
{
    if (h == 0) h = 1;
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
        case '+': case '=': camDistance -= 1.0f; break;
        case '-': case '_': camDistance += 1.0f; break;
        case ' ': paused = !paused;              break;  /* pause / resume */
        case 27:  exit(0);                                /* Esc quits      */
    }
    if (camDistance < 5.0f)  camDistance = 5.0f;
    if (camDistance > 60.0f) camDistance = 60.0f;
}

/* ------------------------------------------------------------------ */
int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1024, 640);
    glutCreateWindow("Mercedes-Benz: Evolution by Decade");

    initScene();
    wheelQuad = gluNewQuadric();
    gluQuadricNormals(wheelQuad, GLU_SMOOTH);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(specialKeys);
    glutKeyboardFunc(keyboard);

    lastTime = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}