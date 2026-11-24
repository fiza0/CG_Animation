#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "hud.h"
#include "cars.h"
#include "transition.h"

#define STRIP_MARGIN  48
#define STRIP_Y       88
#define LABEL_Y       64

static int textWidth(void *font, const char *s)
{
    int w = 0;
    while (*s) w += glutBitmapWidth(font, *s++);
    return w;
}

static void text(float x, float y, void *font, const char *s,
                 float r, float g, float b)
{
    glColor3f(0.0f, 0.0f, 0.0f);
    glRasterPos2f(x + 1.0f, y - 1.0f);
    for (const char *c = s; *c; c++) glutBitmapCharacter(font, *c);

    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (const char *c = s; *c; c++) glutBitmapCharacter(font, *c);
}

static void rect(float x0, float y0, float x1, float y1, GLenum mode)
{
    glBegin(mode);
        glVertex2f(x0, y0);  glVertex2f(x1, y0);
        glVertex2f(x1, y1);  glVertex2f(x0, y1);
    glEnd();
}

static void drawTitle(int h, float speed, int paused, int autoOrbit,
                      const char *viewName)
{
    char title[96], status[200], view[48] = "";
    const CarModel *cur = getCarModel(timelineModelIndex());

    if (timelineTravelling())
        snprintf(title, sizeof title, "Travelling to %s...",
                 getCarModel(timelineTargetIndex())->year);
    else
        snprintf(title, sizeof title, "%s: %s", cur->year, cur->name);

    if (viewName) snprintf(view, sizeof view, "    View: %s", viewName);
    snprintf(status, sizeof status, "Transition: %s    Speed: %.0f km/h%s%s%s%s",
             timelineEffectName(), speed * 3.6f,
             paused                ? "    PAUSED"     : "",
             autoOrbit             ? "    AUTO ORBIT" : "",
             timelineInspecting()  ? "    INSPECTING" : "",
             view);

    int w = textWidth(GLUT_BITMAP_TIMES_ROMAN_24, title);
    int ws = textWidth(GLUT_BITMAP_HELVETICA_12, status);
    if (ws > w) w = ws;

    glColor4f(0.0f, 0.0f, 0.0f, 0.35f);
    rect(10.0f, h - 72.0f, 34.0f + w, h - 10.0f, GL_QUADS);

    text(22.0f, h - 40.0f, GLUT_BITMAP_TIMES_ROMAN_24, title, 1.0f, 1.0f, 1.0f);
    text(22.0f, h - 62.0f, GLUT_BITMAP_HELVETICA_12, status, 0.85f, 0.88f, 0.92f);
}

static void drawStrip(int w)
{
    int   n    = carModelCount();
    int   cur  = timelineModelIndex();
    float x0   = STRIP_MARGIN, x1 = w - STRIP_MARGIN;
    float step = (n > 1) ? (x1 - x0) / (n - 1) : 0.0f;

    glColor4f(0.0f, 0.0f, 0.0f, 0.35f);
    rect(x0 - 30.0f, LABEL_Y - 12.0f, x1 + 30.0f, STRIP_Y + 16.0f, GL_QUADS);

    glLineWidth(2.0f);
    glColor3f(0.7f, 0.7f, 0.75f);
    glBegin(GL_LINES);
        glVertex2f(x0, STRIP_Y);  glVertex2f(x1, STRIP_Y);
    glEnd();

    for (int i = 0; i < n; i++) {
        float x = x0 + i * step;
        const char *year = getCarModel(i)->year;
        float tx = x - 0.5f * textWidth(GLUT_BITMAP_HELVETICA_12, year);

        if (i == cur) {
            glColor3f(1.0f, 0.80f, 0.20f);
            rect(x - 7.0f, STRIP_Y - 7.0f, x + 7.0f, STRIP_Y + 7.0f, GL_QUADS);
            text(tx, LABEL_Y, GLUT_BITMAP_HELVETICA_12, year, 1.0f, 0.80f, 0.20f);
        } else if (i < cur) {
            glColor3f(0.9f, 0.9f, 0.9f);
            rect(x - 4.0f, STRIP_Y - 4.0f, x + 4.0f, STRIP_Y + 4.0f, GL_QUADS);
            text(tx, LABEL_Y, GLUT_BITMAP_HELVETICA_12, year, 0.9f, 0.9f, 0.9f);
        } else {
            glColor3f(0.75f, 0.75f, 0.78f);
            rect(x - 4.0f, STRIP_Y - 4.0f, x + 4.0f, STRIP_Y + 4.0f, GL_LINE_LOOP);
            text(tx, LABEL_Y, GLUT_BITMAP_HELVETICA_12, year, 0.7f, 0.7f, 0.72f);
        }
    }

    int tgt = timelineTargetIndex();
    if (timelineTravelling() && tgt > cur) {
        float x = x0 + (cur + timelineProgress() * (tgt - cur)) * step;
        glColor3f(0.3f, 0.9f, 1.0f);
        glBegin(GL_QUADS);
            glVertex2f(x, STRIP_Y - 6.0f);  glVertex2f(x + 6.0f, STRIP_Y);
            glVertex2f(x, STRIP_Y + 6.0f);  glVertex2f(x - 6.0f, STRIP_Y);
        glEnd();
    }
}

static void drawControls(int w)
{
    const char *l1 = "Arrows: orbit   +/-: zoom   V: camera view   C: auto orbit   Space: pause   Esc: quit";
    const char *l2 = "N: next era   R: restart   T: transition   [ ]: speed";
    const char *l3 = "M: inspect model   , . : previous / next model";
    void *f = GLUT_BITMAP_HELVETICA_12;

    text(w - 12.0f - textWidth(f, l1), 38.0f, f, l1, 0.95f, 0.95f, 0.95f);
    text(w - 12.0f - textWidth(f, l2), 23.0f, f, l2, 0.95f, 0.95f, 0.95f);
    text(w - 12.0f - textWidth(f, l3),  8.0f, f, l3, 0.95f, 0.95f, 0.95f);
}

void drawHUD(int winW, int winH, float speed, int paused, int autoOrbit,
             const char *viewName)
{
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, winW, 0.0, winH, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    drawTitle(winH, speed, paused, autoOrbit, viewName);
    drawStrip(winW);
    drawControls(winW);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();
}
