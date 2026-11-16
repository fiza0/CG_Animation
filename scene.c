#include <stddef.h> 
#include "scene.h"

/* A quadric object is GLU's helper for drawing cylinders, discs and
 * spheres. We create one and reuse it for every tree trunk and pole. */
static GLUquadric *quad = NULL;

/* ------------------------------------------------------------------ */
/* Initialisation: OpenGL state that never changes                    */
/* ------------------------------------------------------------------ */
void initScene(void)
{
    glClearColor(SKY_R, SKY_G, SKY_B, 1.0f);

    glEnable(GL_DEPTH_TEST);   /* nearer surfaces hide farther ones          */
    glEnable(GL_NORMALIZE);    /* keep normals unit-length after glScalef    */
    glShadeModel(GL_SMOOTH);

    /* Lighting on, with glColor driving the material colour. */
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    /* Linear fog: objects blend into the sky between 60 and 180 units. */
    GLfloat fogColor[] = { SKY_R, SKY_G, SKY_B, 1.0f };
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_START, 60.0f);
    glFogf(GL_FOG_END, 180.0f);

    quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);
}

/* ------------------------------------------------------------------ */
/* Lighting: a sun (directional light) plus soft ambient fill          */
/* ------------------------------------------------------------------ */
void setupLighting(void)
{
    /* w = 0 makes this a directional light: parallel rays, like the sun.
     * It must be set AFTER the camera transform so the sun stays fixed
     * in the world instead of following the camera. */
    GLfloat sunDir[]   = { 0.4f, 1.0f, 0.3f, 0.0f };
    GLfloat sunColor[] = { 0.9f, 0.9f, 0.85f, 1.0f };
    GLfloat ambient[]  = { 0.35f, 0.35f, 0.40f, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, sunDir);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  sunColor);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient);
}

/* ------------------------------------------------------------------ */
/* Helper: flat horizontal rectangle at height y, facing up            */
/* ------------------------------------------------------------------ */
static void flatRect(float x0, float z0, float x1, float z1, float y)
{
    glNormal3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
        glVertex3f(x0, y, z0);
        glVertex3f(x0, y, z1);
        glVertex3f(x1, y, z1);
        glVertex3f(x1, y, z0);
    glEnd();
}

/* ------------------------------------------------------------------ */
/* Ground: one large grass plane                                       */
/* ------------------------------------------------------------------ */
static void drawGround(void)
{
    /* Fixed-function lighting is calculated per vertex, so a single huge
     * quad can look evenly lit. That is fine for a flat directional sun. */
    glColor3f(0.30f, 0.55f, 0.25f);
    flatRect(-GROUND_SIZE, -GROUND_SIZE, GROUND_SIZE, GROUND_SIZE, 0.0f);
}

/* ------------------------------------------------------------------ */
/* Road surface and markings                                           */
/* ------------------------------------------------------------------ */
static void drawRoad(void)
{
    /* Each layer is raised slightly above the one below it. Two surfaces
     * at exactly the same height "fight" in the depth buffer and flicker
     * (z-fighting), so: ground y=0, asphalt y=0.01, paint y=0.02. */
    glColor3f(0.22f, 0.22f, 0.24f);
    flatRect(-ROAD_HALF_LENGTH, -ROAD_HALF_WIDTH,
              ROAD_HALF_LENGTH,  ROAD_HALF_WIDTH, 0.01f);

    /* Solid white edge lines. */
    glColor3f(0.95f, 0.95f, 0.95f);
    flatRect(-ROAD_HALF_LENGTH, -ROAD_HALF_WIDTH + 0.2f,
              ROAD_HALF_LENGTH, -ROAD_HALF_WIDTH + 0.35f, 0.02f);
    flatRect(-ROAD_HALF_LENGTH,  ROAD_HALF_WIDTH - 0.35f,
              ROAD_HALF_LENGTH,  ROAD_HALF_WIDTH - 0.2f, 0.02f);

    /* Dashed centre line: 3-unit dash, 3-unit gap. These dashes are what
     * will make the car's motion visible later on. */
    glColor3f(0.95f, 0.85f, 0.20f);
    for (float x = -ROAD_HALF_LENGTH; x < ROAD_HALF_LENGTH; x += 6.0f)
        flatRect(x, -0.08f, x + 3.0f, 0.08f, 0.02f);
}

/* ------------------------------------------------------------------ */
/* Roadside objects built from GLU/GLUT primitives                     */
/* ------------------------------------------------------------------ */

/* GLU cylinders and GLUT cones are built along the +z axis. Rotating
 * by -90 degrees about x stands them upright along +y. */
static void tree(float x, float z)
{
    glPushMatrix();
        glTranslatef(x, 0.0f, z);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

        glColor3f(0.40f, 0.26f, 0.13f);                 /* trunk  */
        gluCylinder(quad, 0.25, 0.20, 2.0, 12, 1);

        glTranslatef(0.0f, 0.0f, 1.5f);                 /* foliage */
        glColor3f(0.13f, 0.40f, 0.15f);
        glutSolidCone(1.4, 3.5, 14, 4);
    glPopMatrix();
}

static void lampPost(float x, float z)
{
    glPushMatrix();
        glTranslatef(x, 0.0f, z);

        glPushMatrix();                                 /* pole */
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            glColor3f(0.35f, 0.35f, 0.38f);
            gluCylinder(quad, 0.08, 0.08, 5.0, 10, 1);
        glPopMatrix();

        glTranslatef(0.0f, 5.0f, 0.0f);                 /* lamp head */
        glColor3f(0.95f, 0.95f, 0.80f);
        glutSolidSphere(0.25, 12, 12);
    glPopMatrix();
}

static void drawRoadside(void)
{
    /* Repeat items at regular intervals along both sides of the road.
     * Offsetting the two sides makes the pattern look less mechanical. */
    for (float x = -ROAD_HALF_LENGTH; x <= ROAD_HALF_LENGTH; x += 12.0f) {
        tree(x,          -ROAD_HALF_WIDTH - 4.0f);      /* far side  */
        tree(x + 6.0f,    ROAD_HALF_WIDTH + 5.5f);      /* near side */
        lampPost(x + 3.0f, ROAD_HALF_WIDTH + 1.0f);
    }
}

/* ------------------------------------------------------------------ */
void drawScene(void)
{
    drawGround();
    drawRoad();
    drawRoadside();
}