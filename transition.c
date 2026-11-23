#include <stddef.h>
#include <stdlib.h>   /* rand */
#include <math.h>     /* sinf, cosf */
#include <string.h>   /* memset */
#include "transition.h"
#include "cars.h"
#include "eras.h"

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
#define TUNNEL_MIN_LENGTH 9.0f   /* never shorter than this             */
#define TUNNEL_MARGIN     3.0f   /* longer than the longest car by this */
#define TUNNEL_LENGTH     tunnelLength()
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

/* The tunnel must hide the longest car completely at the swap, so its
 * length follows the car models. */
static float tunnelLength(void)
{
    float L = carMaxLength() + TUNNEL_MARGIN;
    return L > TUNNEL_MIN_LENGTH ? L : TUNNEL_MIN_LENGTH;
}

typedef enum { PHASE_DRIVE, PHASE_TRANSITION } Phase;
enum { FX_TUNNEL, FX_SCALE };

typedef struct {
    float x;             /* entrance */
    float color[3];      /* portal glow, from the destination era */
    int   alive;
} Tunnel;

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

static int   pendingNext  = 0;           /* 'n' pressed while busy: run it later */
static int   inspecting   = 0;           /* 'm': timeline frozen on one model     */

/* The background ripple, started at the moment of the model swap. */
static RippleState ripple;

static Tunnel   tunnels[MAX_TUNNELS];
static Particle particles[MAX_PARTICLES];

/* ------------------------------------------------------------------ */
/* Tunnels and particles                                               */
/* ------------------------------------------------------------------ */
static void spawnTunnel(float x, const float color[3])
{
    /* Reuse a free slot, or else the tunnel furthest behind the car. */
    int slot = 0;
    for (int i = 0; i < MAX_TUNNELS; i++) {
        if (!tunnels[i].alive) { slot = i; break; }
        if (tunnels[i].x < tunnels[slot].x) slot = i;
    }
    tunnels[slot].x = x;
    tunnels[slot].alive = 1;
    memcpy(tunnels[slot].color, color, sizeof tunnels[slot].color);
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
/* A new transition may only start once the previous ripple has
 * finished, so two background changes never overlap. */
static int canBegin(void)
{
    return phase == PHASE_DRIVE && !ripple.active && !inspecting;
}

static void beginTransition(void)
{
    if (!canBegin()) return;

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
        spawnTunnel(entrance, getEraTheme(target)->portal);
        swapX = entrance + 0.5f * TUNNEL_LENGTH;
    }
}

/* Start the background ripple. The background only learns where and
 * when the swap happened, not which car effect caused it. */
static void startRipple(float x, float z, int from, int to)
{
    ripple.active  = 1;
    ripple.fromEra = from;
    ripple.toEra   = to;
    ripple.cx      = x;
    ripple.cz      = z;
    ripple.time    = 0.0f;
}

static void doSwap(void)
{
    int from = current;
    current = target;
    swapped = 1;

    /* Tunnel: the wave starts from the middle of the tunnel. Scale
     * swap: from the spot where the car vanished. */
    float cx = (activeEffect == FX_TUNNEL) ? swapX : lastCarX;
    startRipple(cx, CAR_LANE_Z, from, current);

    if (activeEffect == FX_SCALE) burstParticles();
}

/* The ripple ends once its fully changed zone has passed everything
 * that could still be seen: the fog limit seen from the farthest
 * camera, measured from wherever the car has got to by now. */
static void updateRipple(float dt, float carX)
{
    if (!ripple.active) return;
    ripple.time += dt;
    float reach = RIPPLE_VISIBLE + RIPPLE_RAMP + fabsf(carX - ripple.cx);
    if (rippleRadius(&ripple) > reach && ripple.time >= RIPPLE_BLEND_TIME)
        ripple.active = 0;
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
    updateRipple(dt, carX);

    /* Forget tunnels that are far behind and long out of sight. */
    for (int i = 0; i < MAX_TUNNELS; i++)
        if (tunnels[i].alive && tunnels[i].x + TUNNEL_LENGTH < carX - TUNNEL_FORGET)
            tunnels[i].alive = 0;

    if (inspecting) return;      /* the car keeps driving, the era stays */
    phaseTime += dt;

    if (phase == PHASE_DRIVE) {
        /* Waits here if the previous ripple is still running. */
        if (pendingNext || phaseTime >= DRIVE_TIME) {
            if (canBegin()) { pendingNext = 0; beginTransition(); }
        }
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
    swapX     += dx;
    startX    += dx;
    ripple.cx += dx;
    lastCarX += dx;
}

/* 'n': start now if possible, otherwise remember the request (only one,
 * however often the key is pressed) and start as soon as allowed. */
void timelineNext(void)
{
    if (inspecting) return;
    if (canBegin()) beginTransition();
    else            pendingNext = 1;
}

void timelineRestart(void)
{
    phase     = PHASE_DRIVE;
    phaseTime = 0.0f;
    current   = 0;
    target    = 0;
    swapped   = 0;
    pendingNext = 0;
    inspecting  = 0;
    memset(&ripple,   0, sizeof ripple);       /* first era, no ripple */
    memset(tunnels,   0, sizeof tunnels);
    memset(particles, 0, sizeof particles);
}

/* Inspection mode. Entering it settles any running transition: a swap
 * that has not happened yet is cancelled, so the car on screen stays. */
void timelineSetInspect(int on)
{
    inspecting = on ? 1 : 0;
    if (inspecting) {
        phase       = PHASE_DRIVE;
        target      = current;
        swapped     = 0;
        pendingNext = 0;
    }
    phaseTime = 0.0f;
}

int timelineInspecting(void) { return inspecting; }

/* Step to the previous or next model instantly. The background follows
 * at once too, so any running ripple is stopped. */
void timelineInspectStep(int dir)
{
    if (!inspecting) return;
    int n = carModelCount();
    current = ((current + dir) % n + n) % n;
    target  = current;
    ripple.active = 0;
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
const RippleState *timelineRipple(void) { return &ripple; }
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
 * the colour is shown at full strength. The colour is the destination
 * era's (amber for the early years, cool white-blue for today) and a
 * sine wave pulses it towards white, so it reads as an energy portal. */
static void portal(float x, const float c[3])
{
    float k    = 0.5f + 0.5f * sinf(glowClock * PORTAL_PULSE);
    float half = TUNNEL_INNER_HALF + TUNNEL_WALL;   /* outer half-width */
    float top  = TUNNEL_INNER_H + TUNNEL_ROOF;
    float t    = PORTAL_THICK;
    float zc   = CAR_LANE_Z;

    glDisable(GL_LIGHTING);
    float w    = 0.35f * k;                         /* share of white */
    glColor3f(c[0] + (1.0f - c[0]) * w, c[1] + (1.0f - c[1]) * w,
              c[2] + (1.0f - c[2]) * w);
    box(x, 0.5f * (top + t), zc - half - 0.5f * t, t, top + t, t);   /* posts  */
    box(x, 0.5f * (top + t), zc + half + 0.5f * t, t, top + t, t);
    box(x, top + 0.5f * t,   zc, t, t, 2.0f * (half + t));          /* lintel */
    glEnable(GL_LIGHTING);
}

/* Two walls and a roof made of boxes. The roof hides the car from a
 * high camera, so the swap stays invisible from any orbit angle. */
static void drawTunnel(const Tunnel *tn)
{
    float x  = tn->x;
    float L  = TUNNEL_LENGTH;
    float cx = x + 0.5f * L;
    float zc = CAR_LANE_Z;
    float in = TUNNEL_INNER_HALF, w = TUNNEL_WALL;
    float H  = TUNNEL_INNER_H,    R = TUNNEL_ROOF;

    glColor3f(0.24f, 0.26f, 0.34f);
    box(cx, 0.5f * H, zc - in - 0.5f * w, L, H, w);    /* far wall  */
    box(cx, 0.5f * H, zc + in + 0.5f * w, L, H, w);    /* near wall */
    box(cx, H + 0.5f * R, zc, L, R, 2.0f * (in + w));  /* roof      */

    portal(x, tn->color);
    portal(x + L, tn->color);
}

void drawTunnels(void)
{
    for (int i = 0; i < MAX_TUNNELS; i++)
        if (tunnels[i].alive) drawTunnel(&tunnels[i]);
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
