# CG_Animation: Mercedes-Benz, Evolution by Decade

A 3D animation in C with legacy OpenGL (fixed-function pipeline, GLUT, GLU).
A car drives endlessly down a straight country road. Every few seconds it
shrinks into the road and grows back (or, with `t`, drives through a glowing
"time tunnel") as the next era's Mercedes-Benz model, from the 1886
Patent-Motorwagen to the 2021 EQS. At the same moment a "time ripple" spreads
out from the car and changes the world around it to match the new era: sky,
light, road surface and markings, roadside objects and the distant skyline.
After the last model the timeline loops back to the start.

All geometry is procedural: no model files, textures or external libraries.

## Build and run (macOS)

Uses the OpenGL and GLUT frameworks that ship with macOS:

```sh
clang main.c scene.c cars.c transition.c hud.c eras.c props.c -o car -framework OpenGL -framework GLUT && ./car
```

In VS Code the same command is the default build task (`Cmd+Shift+B`).

## Controls

| Key | Action |
|---|---|
| Arrow keys | Orbit the camera (left/right: around, up/down: higher/lower) |
| `+` / `-` | Zoom in / out |
| Space | Pause / resume |
| `n` | Start the next transition now (or as soon as the current ripple has finished) |
| `r` | Restart from the first era and its background |
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

## Era themes

Each era also has a background theme (`eras.c`), in the same order as the cars.
Colour grading does most of the work: the early eras are warm, washed out and
foggier, like an old photograph; the 1950s and 1960s are strongly saturated;
the modern eras have a clean blue sky, bright greens and cool white sunlight.
The sun and ambient light colours tint every lit object, so the same tree looks
different in each era.

| Era | Sky / mood | Road | Roadside | Skyline |
|---|---|---|---|---|
| 1886 | Pale sepia, warm sun | Tan dirt track with wheel ruts | Fences, haystacks, cottages, round trees | Rolling hills |
| 1901 | Cooler sepia | Gravel, no markings | Gas lamps, telegraph poles with sagging wires, fences, cottages | Hills with a church spire |
| 1936 | Faded cream | Concrete slabs with joints every 4 m | Electric lamps, stone walls, a petrol pump, telegraph poles | Factory chimneys |
| 1954 | First strong colours | Asphalt, white centre dashes | Lamp posts, billboards, suburban houses | Small town |
| 1963 | Saturated blue | White dashes and edge lines | Flat-roofed houses, billboards, a diner sign | Mid-rise blocks |
| 1976 | Hazy, warm | Grey asphalt, yellow dashes | Sodium lamps with orange heads, apartment blocks | Tower blocks |
| 1982 | Hazy blue | As 1976 | As 1976, plus a road sign and a phone box | Tower blocks |
| 1995 | Clear blue | Darker asphalt, glowing cat's eyes | Crash barrier, white lamps, blue-glass offices | Office towers |
| 2002 | Clear blue | As 1995 | As 1995, plus an overhead sign gantry | Office towers |
| 2013 | Bright, clean | Very dark asphalt, crisp markings | Slim LED lamps, glass towers, wind turbines | Glass skyscrapers |
| 2021 | Crisp, cool light | As 2013, plus a green cycle lane | EV chargers, solar panels, turbines, LED lamps | Glass skyscrapers |

Billboards, signs and buildings use plain coloured panels only: no names or
logos.

## Code structure

| File | Responsibility |
|---|---|
| `main.c` | Window, GLUT callbacks, orbit camera, keyboard, timer and `update(dt)` |
| `scene.c/.h` | Ground and road strips, markings, prop placement, lighting, fog, skyline backdrop and ripple ring |
| `eras.c/.h` | Era theme table, colour blending and ripple progress maths |
| `props.c/.h` | Roadside objects and skylines, compiled into display lists |
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

**Endless road (treadmill).** The road is only 600 m long (300 m each side of
the snap point, so its ends are always beyond the fog). The scene is redrawn
around the car in jumps of exactly one repeat period, so it looks identical
after each jump and seems endless. To keep floats precise, the car's x is
pulled back by a multiple of that period now and then; the tunnels and the
ripple centre are shifted by the same amount.

**Longer scene period.** The original period was 12 m, with the same tree and
lamp in every 12 m. With many kinds of objects per era that repeat would be
obvious, so the period is now 96 m, split into 8 slots of 12 m. Each theme
gives every slot its own near-side, far-side and background object, plus "row"
objects (fences, walls, wires, crash barriers) that repeat in every slot and
join up across slots. Everything that varies (dash spacing, gravel speckles,
turbine phase) depends only on the position within the 96 m period, so the snap
stays invisible.

**Props in display lists.** Every static object (cottage, gas lamp, apartment
block, gantry, and so on) is modelled once from cubes, cylinders, cones,
spheres, discs and quads, standing on y = 0 with its front facing +z, and
recorded into a display list at start-up. Near-side copies are turned by 180
degrees to face the road. Only the turbine rotors are drawn freshly each frame,
because they turn. Glowing parts (lanterns, sodium heads, cat's eyes, charger
lights) are drawn with lighting switched off.

**Skyline backdrop.** A flat silhouette layer about 150 m away on the far side
(hills, chimneys, towers), kept centred on the car so it never slides past,
like a painted backdrop. It is drawn first, without lighting or fog, in a
colour slightly darker and bluer than the sky, then the depth buffer is cleared
so it never hides the real scene.

**Time ripple.** At the instant the car model is swapped (inside the tunnel, or
when the shrinking car reaches zero size), a ripple starts at that point in the
world. The background code only knows "the swap happened at P, at time T"; it
does not care which car effect caused it. From then on:

```
radius   = RIPPLE_SPEED * t                 (120 m/s)
progress = clamp((radius - d) / RIPPLE_RAMP, 0, 1)   (RIPPLE_RAMP = 40 m)
```

where `d` is the distance from the ripple centre: in x and z for objects, and
along x for the road and ground.

- *Objects:* the old era's object is scaled vertically by `1 - progress` and
  the new one by `progress`, both from their base and eased with smoothstep, so
  the old world sinks and the new one rises as the wave passes.
- *Road and ground:* drawn as 12 m strips. Each strip lerps its colour between
  the two themes by its own progress and switches to the new markings once its
  progress passes 0.5, so the new road "unrolls" outwards from the car.
- *Global look:* sky (clear colour), fog colour and distances, sun and ambient
  light blend from the old theme to the new one over 1.5 s with smoothstep.
- *Skyline:* the old and new layers crossfade with alpha blending over the
  same 1.5 s.
- *Wavefront:* a translucent, glowing open cylinder at `radius`, drawn unlit
  with additive blending and without depth writes, fading as it grows. Its
  colour (like the tunnel portal) comes from the destination era: amber for the
  early years, cool white-blue for today.

The ripple ends once its fully changed zone has passed everything that could
still be seen (the fog limit from the farthest camera position). A new
transition cannot start before that; pressing `n` meanwhile queues one request.
The constants are at the top of `eras.h`.

**Delta-time motion and rolling wheels.** The car moves `speed x dt` each frame,
where `dt` is the real time since the last frame, so the speed does not depend
on the frame rate. The total distance driven sets each wheel's angle:
`angle = distance / radius`. Each wheel size uses its own radius, so the small
front wheel of the 1886 car turns faster than its large rear wheels.

**HUD.** Drawn last each frame with an orthographic projection the size of the
window and lighting, depth test and fog switched off, then restored.
