# CG_Animation: Mercedes-Benz, Evolution by Decade

A 3D animation in C with legacy OpenGL (fixed-function pipeline, GLUT, GLU).
A car drives endlessly down a straight country road. Every few seconds it
shrinks into the road and grows back (or, with `t`, drives through a glowing
"time tunnel") as the next era's Mercedes-Benz model, from the 1886
Patent-Motorwagen to the 2021 EQS.
After the last model the timeline loops back to the start.

All geometry is procedural: no model files, textures or external libraries.

## Build and run (macOS)

Uses the OpenGL and GLUT frameworks that ship with macOS:

```sh
clang main.c scene.c cars.c transition.c hud.c -o car -framework OpenGL -framework GLUT && ./car
```

In VS Code the same command is the default build task (`Cmd+Shift+B`).

## Controls

| Key | Action |
|---|---|
| Arrow keys | Orbit the camera (left/right: around, up/down: higher/lower) |
| `+` / `-` | Zoom in / out |
| Space | Pause / resume |
| `n` | Start the next transition now |
| `r` | Restart from the first era |
| `t` | Toggle the transition style (scale swap, the default, or time tunnel) for the next transition |
| `c` | Toggle a slow automatic camera orbit |
| `[` / `]` | Slower / faster (about 14 to 108 km/h) |
| Esc | Quit |

## Eras

| Year | Model |
|---|---|
| 1886 | Benz Patent-Motorwagen |
| 1901 | Mercedes 35 HP |
| 1936 | 540K |
| 1954 | 300 SL Gullwing |
| 1963 | 230 SL "Pagoda" |
| 1976 | W123 E-Class |
| 1982 | 190E (W201) |
| 1995 | E-Class (W210) |
| 2002 | E-Class (W211) |
| 2013 | S-Class (W222) |
| 2021 | EQS |

The shapes are simplified side silhouettes with approximate dimensions. No
badges or logos are drawn.

## Code structure

| File | Responsibility |
|---|---|
| `main.c` | Window, GLUT callbacks, orbit camera, keyboard, timer and `update(dt)` |
| `scene.c/.h` | Ground, road, trees and lamp posts, lighting and fog |
| `cars.c/.h` | Car model table and the generic car renderer |
| `transition.c/.h` | Timeline state machine, time tunnel, scale swap and particles |
| `hud.c/.h` | 2D text overlay |

## Techniques

**Extruded side profiles.** Each car body is described by its side silhouette:
a list of 15 to 40 (x, y) points. The renderer pushes this outline across the
car's width. The two flat side caps are the outline itself; the walls are one
quad per outline edge, whose normal is the edge direction crossed with the z
axis. Neighbouring walls that meet at a shallow angle share a blended normal so
curves look smooth, while sharp corners keep a crisp edge. Each edge also
carries a style (body, glass, chrome, trim, lamp), which colours that strip of
wall. This is how windscreens, bumpers and lights appear without extra
geometry. Wheel arches are generated automatically: an edge marked as an arch is
replaced by a half circle around the nearest wheel.

**GLU tessellation.** The silhouettes are concave (wheel arches, windscreen
dips), so `GL_POLYGON` or a triangle fan would fill them wrongly. The GLU
tessellator splits each outline into triangles. Side windows are a second,
smaller tessellated outline placed just outside the body so the two surfaces do
not fight in the depth buffer.

**Display lists.** Tessellation runs once at start-up. The resulting body,
fenders and windows of each model are recorded in a display list, so drawing a
body each frame is a single `glCallList`.

**Data-driven models.** One `CarModel` struct per era (profile, window, colours,
wheel layout and an optional `drawDetails` function for era extras such as
seats, a tiller, spare wheels, chrome bumpers or LED light strips). Adding an
era means adding data, not new rendering code.

**Timeline state machine.** `DRIVE` (4 s) shows the current model. `TRANSITION`
(about 2.5 s) runs the effect and swaps to the next model at the right moment.
All durations are named constants at the top of `transition.c`.

**Time tunnel (press `t`).** When a transition starts, a short tunnel (two walls and a roof)
is placed ahead of the car in world coordinates. The car never slows down; it
just drives in. The model is swapped when the car's centre reaches the middle
of the tunnel. Because the tunnel is longer than any car, the whole car is
hidden at that moment, and the roof hides it from high camera angles too. The
entrances have frames drawn without lighting, in a colour that pulses with a
sine wave. The tunnel stays behind as the car drives on and is removed once it
is far out of view.

**Scale swap (default).** The effect shrinks the car into the road with an
ease-in curve, swaps the model while it is invisible, throws out a burst of
fading light particles, and grows the new car back with an ease-out curve.

**Endless road (treadmill).** The road is only 400 units long. The scene is
redrawn around the car in jumps of exactly one repeat period (12 units), so it
looks identical after each jump and seems endless. To keep floats precise, the
car's x is pulled back by a multiple of that period now and then; tunnels in the
world are shifted by the same amount.

**Delta-time motion and rolling wheels.** The car moves `speed x dt` each frame,
where `dt` is the real time since the last frame, so the speed does not depend
on the frame rate. The total distance driven sets each wheel's angle:
`angle = distance / radius`. Each wheel size uses its own radius, so the small
front wheel of the 1886 car turns faster than its large rear wheels.

**HUD.** Drawn last each frame with an orthographic projection the size of the
window and lighting, depth test and fog switched off, then restored.
