#ifndef SCENE_H
#define SCENE_H

#ifdef __APPLE__
  #define GL_SILENCE_DEPRECATION
  #include <OpenGL/gl.h>
  #include <OpenGL/glu.h>
  #include <GLUT/glut.h>
#else
  #include <GL/gl.h>
  #include <GL/glu.h>
  #include <GL/glut.h>
#endif

#define ROAD_HALF_LENGTH 300.0f
#define ROAD_HALF_WIDTH    4.0f
#define GROUND_NEAR_Z    400.0f
#define GROUND_FAR_Z    -145.0f

#define SCENE_PERIOD      96.0f
#define SLOTS_PER_PERIOD   8
#define SLOT_LENGTH       12.0f

#define ROW_NEAR_Z         5.0f
#define ROW_FAR_Z         -5.8f
#define SLOT_NEAR_Z        6.5f
#define SLOT_FAR_Z        -9.0f
#define BACK_Z_EVEN      -18.0f
#define BACK_Z_ODD       -24.0f
#define TURBINE_Z        -30.0f

struct EraTheme;
struct RippleState;

void initScene(void);

void applyThemeGlobals(const struct EraTheme *t);

void setupLighting(const struct EraTheme *t);

void drawBackdrop(float followX, const struct EraTheme *cur,
                  const struct EraTheme *prev, const struct RippleState *rip);

void drawScene(float followX, const struct EraTheme *cur,
               const struct EraTheme *prev, const struct RippleState *rip);

void sceneHideNearSide(int hide);

void drawRippleRing(const struct RippleState *rip, const float color[3]);

#endif
