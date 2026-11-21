#include <stddef.h>
#include <stdlib.h>   /* rand */
#include <math.h>     /* sinf, cosf */
#include <string.h>   /* memset */
#include "transition.h"
#include "cars.h"

/* ------------------------------------------------------------------ */
/* Timing (seconds)                                                    */
/* ------------------------------------------------------------------ */
#define DRIVE_TIME       4.0f   /* how long each era is shown          */
#define TRANSITION_TIME  2.5f   /* shortest length of a transition     */

/* Scale swap: shrink, stay hidden briefly, swap, grow back. */
#define SHRINK_END       1.10f
#define SWAP_TIME        1.25f
#define GROW_START       1.40f
#define GROW_END         TRANSITION_TIME

/* ------------------------------------------------------------------ */
/* Tunnel layout (units, roughly metres)                               */
/* ------------------------------------------------------------------ */
#define TUNNEL_LENGTH     9.0f   /* longer than the longest car (5.2)   */
#define TUNNEL_LEAD      11.0f   /* entrance this far ahead of the car  */
#define TUNNEL_INNER_HALF 1.70f  /* half the inside width               */
#define TUNNEL_WALL       0.35f
#define TUNNEL_INNER_H    2.40f
#define TUNNEL_ROOF       0.30f
#define TUNNEL_FORGET   200.0f   /* drop tunnels this far behind the car */
#define MAX_TUNNELS       6
#define PORTAL_THICK      0.22f
#define PORTAL_PULSE      4.0f   /* radians per second of the glow cycle */

/* ------------------------------------------------------------------ */
/* Particles (scale swap only)                                         */
/* ------------------------------------------------------------------ */
#define MAX_PARTICLES    48
#define PARTICLE_LIFE    1.2f
#define GRAVITY          6.0f

typedef enum { PHASE_DRIVE, PHASE_TRANSITION } Phase;
enum { FX_TUNNEL, FX_SCALE };

typedef struct { float x; int alive; } Tunnel;   /* x = entrance */

typedef struct {
    float x, y, z;       /* relative to the car, so the burst moves with it */
    float vx, vy, vz;
    float life;          /* seconds left; <= 0 means unused */
} Particle;

static Phase phase        = PHASE_DRIVE;
static float phaseTime    = 0.0f;
static int   current      = 0;
static int   target       = 0;
static int   swapped      = 0;
static int   effect       = FX_SCALE;    /* chosen with 't'               */
static int   activeEffect = FX_SCALE;    /* effect of the running transition */
static float glowClock    = 0.0f;
static float lastCarX     = 0.0f;
static float startX       = 0.0f;        /* car x when the transition began */
static float swapX        = 0.0f;        /* tunnel: swap when the car gets here */

static Tunnel   tunnels[MAX_TUNNELS];
static Particle particles[MAX_PARTICLES];

/* ------------------------------------------------------------------ */
/* Tunnels and particles                                               */
/* ------------------------------------------------------------------ */
static void spawnTunnel(float x)
{
    /* Reuse a free slot, or else the tunnel furthest behind the car. */
    int slot = 0;
    for (int i = 0; i < MAX_TUNNELS; i++) {
        if (!tunnels[i].alive) { slot = i; break; }
        if (tunnels[i].x < tunnels[slot].x) slot = i;
    }
    tunnels[slot].x = x;
    tunnels[slot].alive = 1;
}

static float frand(void) { return (float)rand() / (float)RAND_MAX; }

/* Throw small light cubes outwards and upwards from the car's centre. */
static void burstParticles(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &particles[i];
        float a = frand() * 2.0f * (float)M_PI;   /* direction around y */
        float h = 1.5f + frand() * 2.5f;          /* sideways speed     */
        p->x = 0.0f;  p->y = 0.6f;  p->z = 0.0f;
        p->vx = cosf(a) * h;
        p->vz = sinf(a) * h;
        p->vy = 1.5f + frand() * 2.5f;
        p->life = PARTICLE_LIFE * (0.7f + 0.3f * frand());
    }
}

static void updateParticles(float dt)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &particles[i];
        if (p->life <= 0.0f) continue;
        p->vy -= GRAVITY * dt;                    /* simple gravity */
        p->x += p->vx * dt;
        p->y += p->vy * dt;
        p->z += p->vz * dt;
        if (p->y < 0.05f) { p->y = 0.05f; p->vy = -0.4f * p->vy; }  /* bounce */
        p->life -= dt;
    }
}

/* ------------------------------------------------------------------ */
/* State machine                                                       */
/* ------------------------------------------------------------------ */
static void beginTransition(void)
{
    if (phase == PHASE_TRANSITION) return;

    phase        = PHASE_TRANSITION;
    phaseTime    = 0.0f;
    swapped      = 0;
    target       = (current + 1) % carModelCount();
    activeEffect = effect;
    startX       = lastCarX;

    if (activeEffect == FX_TUNNEL) {
        /* Build the tunnel ahead of the car. The car keeps its speed
         * and simply drives in. The model is swapped when the car's
         * centre reaches the middle of the tunnel: since the tunnel is
         * longer than any car, the whole car is hidden at that moment. */
        float entrance = lastCarX + TUNNEL_LEAD;
        spawnTunnel(entrance);
        swapX = entrance + 0.5f * TUNNEL_LENGTH;
    }
}

static void doSwap(void)
{
    current = target;
    swapped = 1;
    if (activeEffect == FX_SCALE) burstParticles();
}

void initTimeline(void)
{
    timelineRestart();
}

void updateTimeline(float dt, float carX)
{
    lastCarX = carX;

    /* The glow only uses sinf(glowClock * PULSE), so wrapping the clock
     * by one full cycle keeps it small without any visible jump. */
    glowClock += dt;
    float cycle = 2.0f * (float)M_PI / PORTAL_PULSE;
    if (glowClock > cycle) glowClock -= cycle;

    updateParticles(dt);

    /* Forget tunnels that are far behind and long out of sight. */
    for (int i = 0; i < MAX_TUNNELS; i++)
        if (tunnels[i].alive && tunnels[i].x + TUNNEL_LENGTH < carX - TUNNEL_FORGET)
            tunnels[i].alive = 0;

    phaseTime += dt;

    if (phase == PHASE_DRIVE) {
        if (phaseTime >= DRIVE_TIME) beginTransition();
        return;
    }

    /* PHASE_TRANSITION */
    if (!swapped) {
        if (activeEffect == FX_TUNNEL && carX >= swapX)      doSwap();
        if (activeEffect == FX_SCALE  && phaseTime >= SWAP_TIME) doSwap();
    }
    if (swapped && phaseTime >= TRANSITION_TIME) {
        phase = PHASE_DRIVE;
        phaseTime = 0.0f;
    }
}

void timelineShiftWorld(float dx)
{
    for (int i = 0; i < MAX_TUNNELS; i++) tunnels[i].x += dx;
    swapX    += dx;
    startX   += dx;
    lastCarX += dx;
}

void timelineNext(void) { beginTransition(); }

void timelineRestart(void)
{
    phase     = PHASE_DRIVE;
    phaseTime = 0.0f;
    current   = 0;
    target    = 0;
    swapped   = 0;
    memset(tunnels,   0, sizeof tunnels);
    memset(particles, 0, sizeof particles);
}

/* The new choice applies from the next transition, so a running
 * transition is never cut in half. */
void timelineToggleEffect(void)
{
    effect = (effect == FX_TUNNEL) ? FX_SCALE : FX_TUNNEL;
}

/* ------------------------------------------------------------------ */
/* Queries                                                             */
/* ------------------------------------------------------------------ */
int timelineModelIndex(void)  { return current; }
int timelineTargetIndex(void) { return target; }
int timelineTravelling(void)  { return phase == PHASE_TRANSITION && !swapped; }

const char *timelineEffectName(void)
{
    return effect == FX_TUNNEL ? "time tunnel" : "scale swap";
}

float timelineProgress(void)
{
    if (!timelineTravelling()) return 0.0f;
    float p;
    if (activeEffect == FX_TUNNEL)
        p = (lastCarX - startX) / (swapX - startX);
    else
        p = phaseTime / SWAP_TIME;
    return p < 0.0f ? 0.0f : (p > 1.0f ? 1.0f : p);
}

/* Ease-in (slow start, fast end) while shrinking, ease-out (fast start,
 * gentle landing) while growing. Squaring the time gives both curves. */
float timelineCarScale(void)
{
    if (phase != PHASE_TRANSITION || activeEffect != FX_SCALE) return 1.0f;

    float t = phaseTime;
    if (t < SHRINK_END) {
        float u = t / SHRINK_END;
        return 1.0f - u * u;
    }
    if (t < GROW_START) return 0.0f;
    if (t < GROW_END) {
        float v = (t - GROW_START) / (GROW_END - GROW_START);
        return 1.0f - (1.0f - v) * (1.0f - v);
    }
    return 1.0f;
}

/* ------------------------------------------------------------------ */
/* Drawing                                                             */
/* ------------------------------------------------------------------ */
static void box(float cx, float cy, float cz, float sx, float sy, float sz)
{
    glPushMatrix();
        glTranslatef(cx, cy, cz);
        glScalef(sx, sy, sz);
        glutSolidCube(1.0);
    glPopMatrix();
}

/* Glowing frame around one tunnel opening. Lighting is switched off so
 * the colour is shown at full strength, and the colour drifts between
 * cyan and violet with a sine wave, so it reads as an energy portal. */
static void portal(float x)
{
    float k    = 0.5f + 0.5f * sinf(glowClock * PORTAL_PULSE);
    float half = TUNNEL_INNER_HALF + TUNNEL_WALL;   /* outer half-width */
    float top  = TUNNEL_INNER_H + TUNNEL_ROOF;
    float t    = PORTAL_THICK;
    float zc   = CAR_LANE_Z;

    glDisable(GL_LIGHTING);
    glColor3f(0.2f + 0.5f * k, 0.9f - 0.5f * k, 1.0f);
    box(x, 0.5f * (top + t), zc - half - 0.5f * t, t, top + t, t);   /* posts  */
    box(x, 0.5f * (top + t), zc + half + 0.5f * t, t, top + t, t);
    box(x, top + 0.5f * t,   zc, t, t, 2.0f * (half + t));          /* lintel */
    glEnable(GL_LIGHTING);
}

/* Two walls and a roof made of boxes. The roof hides the car from a
 * high camera, so the swap stays invisible from any orbit angle. */
static void drawTunnel(float x)
{
    float L  = TUNNEL_LENGTH;
    float cx = x + 0.5f * L;
    float zc = CAR_LANE_Z;
    float in = TUNNEL_INNER_HALF, w = TUNNEL_WALL;
    float H  = TUNNEL_INNER_H,    R = TUNNEL_ROOF;

    glColor3f(0.24f, 0.26f, 0.34f);
    box(cx, 0.5f * H, zc - in - 0.5f * w, L, H, w);    /* far wall  */
    box(cx, 0.5f * H, zc + in + 0.5f * w, L, H, w);    /* near wall */
    box(cx, H + 0.5f * R, zc, L, R, 2.0f * (in + w));  /* roof      */

    portal(x);
    portal(x + L);
}

void drawTunnels(void)
{
    for (int i = 0; i < MAX_TUNNELS; i++)
        if (tunnels[i].alive) drawTunnel(tunnels[i].x);
}

/* Particles fade out through alpha blending. They do not write to the
 * depth buffer, so overlapping see-through cubes do not hide each other. */
void drawParticles(float carX)
{
    glPushMatrix();
    glTranslatef(carX, 0.0f, CAR_LANE_Z);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    for (int i = 0; i < MAX_PARTICLES; i++) {
        const Particle *p = &particles[i];
        if (p->life <= 0.0f) continue;
        float a = p->life / PARTICLE_LIFE;
        float s = 0.12f * (0.5f + 0.5f * a);      /* shrink as they fade */
        glColor4f(1.0f, 0.95f, 0.70f, a);
        box(p->x, p->y, p->z, s, s, s);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}
