#include <math.h>
#include <stdlib.h>
#include "scene.h"

/* ------------------------------------------------------------------ */
/* Orbit camera                                                        */
/* The camera sits on a sphere around a target point and always looks */
/* at it. Arrow keys move it around the sphere; + and - zoom.          */
/* ------------------------------------------------------------------ */
static float camYaw      = 35.0f;   /* degrees around the y axis       */
static float camPitch    = 18.0f;   /* degrees above the ground        */
static float camDistance = 14.0f;   /* distance from the target        */
static float targetX = 0.0f, targetY = 1.0f, targetZ = 0.0f;

static void applyCamera(void)
{
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
/* Placeholder car: two boxes, replaced by real models in a later step */
/* ------------------------------------------------------------------ */
static void drawPlaceholderCar(void)
{
    glColor3f(0.75f, 0.10f, 0.10f);

    glPushMatrix();                      /* lower body: 4.5 x 1.0 x 1.8 */
        glTranslatef(0.0f, 0.8f, -2.0f); /* sits in the far lane (z = -2) */
        glScalef(4.5f, 1.0f, 1.8f);
        glutSolidCube(1.0);
    glPopMatrix();

    glPushMatrix();                      /* cabin: 2.4 x 0.8 x 1.6 */
        glTranslatef(-0.3f, 1.7f, -2.0f);
        glScalef(2.4f, 0.8f, 1.6f);
        glutSolidCube(1.0);
    glPopMatrix();
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
    drawScene();         /* 3. draw the world                       */
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
    glutPostRedisplay();
}

static void keyboard(unsigned char key, int x, int y)
{
    (void)x; (void)y;
    switch (key) {
        case '+': case '=': camDistance -= 1.0f; break;
        case '-': case '_': camDistance += 1.0f; break;
        case 27:  exit(0);                        /* Esc quits */
    }
    if (camDistance < 5.0f)  camDistance = 5.0f;
    if (camDistance > 60.0f) camDistance = 60.0f;
    glutPostRedisplay();
}

/* ------------------------------------------------------------------ */
int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1024, 640);
    glutCreateWindow("Mercedes-Benz: Evolution by Decade");

    initScene();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(specialKeys);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}