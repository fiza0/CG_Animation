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
#define ROAD_HALF_LENGTH 200.0f   /* road spans x = -200 .. +200        */
#define ROAD_HALF_WIDTH    4.0f   /* road spans z = -4 .. +4 (two lanes) */
#define GROUND_SIZE      400.0f   /* half-size of the grass plane        */

/* Sky colour is shared by the clear colour and the fog, so distant
 * objects fade into the horizon instead of ending abruptly. */
#define SKY_R 0.62f
#define SKY_G 0.80f
#define SKY_B 0.95f

void initScene(void);       /* call once after the window is created */
void setupLighting(void);   /* call every frame, after the camera     */
void drawScene(void);       /* ground, road, markings, roadside items */

#endif