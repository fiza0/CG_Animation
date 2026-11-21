#ifndef SCENE_H
#define SCENE_H

/* Platform-specific OpenGL headers: macOS keeps them in frameworks. */
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

/* World layout (units are roughly metres).
 * The road runs along the x axis; the car drives in the +x direction.
 * y is up, z is across the road. */
#define ROAD_HALF_LENGTH 300.0f   /* road spans snap - 300 .. snap + 300   */
#define ROAD_HALF_WIDTH    4.0f   /* road spans z = -4 .. +4 (two lanes)   */
#define GROUND_NEAR_Z    400.0f   /* grass reaches this far on the near side */
#define GROUND_FAR_Z    -145.0f   /* ...and stops just before the skyline  */

/* The roadside pattern repeats every SCENE_PERIOD metres. Shifting the
 * whole scene by a multiple of this period is invisible, which is how
 * the road becomes endless. The period is split into slots, and each
 * slot gets its own objects, so the repeat is hard to spot. */
#define SCENE_PERIOD      96.0f
#define SLOTS_PER_PERIOD   8
#define SLOT_LENGTH       12.0f   /* SCENE_PERIOD / SLOTS_PER_PERIOD       */

/* Where props stand across the road (z). Rows repeat in every slot at
 * the slot start; slot objects stand in the middle of the slot. */
#define ROW_NEAR_Z         5.0f
#define ROW_FAR_Z         -5.8f
#define SLOT_NEAR_Z        6.5f
#define SLOT_FAR_Z        -9.0f
#define BACK_Z_EVEN      -18.0f   /* background, alternating by slot       */
#define BACK_Z_ODD       -24.0f
#define TURBINE_Z        -30.0f   /* turbines stand further back           */

/* Defined in eras.h; declared here so scene.h need not include it. */
struct EraTheme;
struct RippleState;

void initScene(void);    /* call once after the window is created */

/* Sky (clear colour), fog colour and fog distances. Call every frame,
 * before glClear, with the blended theme. */
void applyThemeGlobals(const struct EraTheme *t);

/* Sun and ambient light. Call every frame, after the camera. */
void setupLighting(const struct EraTheme *t);

/* Distant skyline, crossfaded during a ripple. Call right after the
 * camera, before drawScene; it clears the depth buffer when done. */
void drawBackdrop(float followX, const struct EraTheme *cur,
                  const struct EraTheme *prev, const struct RippleState *rip);

/* Ground, road and roadside objects, kept centred on followX (the car).
 * prev and rip may be NULL; outside a ripple only cur is drawn. */
void drawScene(float followX, const struct EraTheme *cur,
               const struct EraTheme *prev, const struct RippleState *rip);

/* The glowing wavefront. Call after all opaque geometry. */
void drawRippleRing(const struct RippleState *rip, const float color[3]);

#endif
