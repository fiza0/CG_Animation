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
clang *.c models/*.c -o car -framework OpenGL -framework GLUT && ./car
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
| `m` | Model inspection: stop the timeline on the current car (it keeps driving) and hide the near-side roadside objects. Press again to resume |
| `,` / `.` | Previous / next model while inspecting (instant swap) |
| `v` | Cycle camera presets: side view, front three-quarter, rear three-quarter, low front, top-down. The arrow keys still adjust from there |
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

Dimensions are approximate; silhouette, proportions and signature details are
what make each car recognisable. No emblems, badges, lettering or real number
plates are drawn (plates are blank).

| Car | Signature features |
|---|---|
| 1886 Patent-Motorwagen | Bent-tube frame, three wire wheels (small front, large rear), tiller with crank handle, high bench seat, horizontal flywheel that spins as the car moves, belt and chain drive, water tank |
| 1901 Mercedes 35 HP | Low pressed-steel frame, flat honeycomb radiator, long louvred bonnet, raked column with a big wooden wheel, two rows of high seats, brass carbide lamps and bulb horn, side levers, mudguards, goggled driver, artillery wheels |
| 1936 540K Cabriolet | Very long bonnet with chrome hinge and louvres, three chrome exhaust pipes each side, tall V grille, flowing black wings into running boards, free-standing headlamps, fog lamps and horns, side-mounted spare wheels, V windscreen, folded hood, luggage trunk, cream and black two-tone |
| 1954 300 SL Gullwing | Low coupe, two power domes, wide low grille with one thick bar, faired round headlamps, eyebrows over the arches, gill vents with chrome bars, gullwing seams over the roof, red interior |
| 1963 230 SL Pagoda | Slab sides, tall stacked headlamp units, wide low grille, white hardtop with a dished roof and thin pillars, vertical overriders, upright tail lamps |
| 1976 W123 | Boxy three-box saloon, tall glasshouse with chrome frames and a quarter window, wide headlamp units beside a tall flat-topped grille, chrome bumpers with rubber strips, ribbed tail lamps, ribbed hubcaps |
| 1982 190E | Wedge with a high kicked-up boot, narrow headlamps, small grille, grey side cladding, grey bumper covers, monowiper, 15-hole alloys |
| 1995 W210 | Rounded saloon, twin oval headlamps per side, slanted grille in a body-colour bumper, wrap-around tail lamps, oval handles, 8-hole alloys |
| 2002 W211 | Arched roof, twin ovals under one clear teardrop cover, raked grille, chrome window surround, clear-and-red tail lamps, five-twin-spoke alloys with brakes |
| 2013 S-Class | Long wheelbase, large upright chrome grille, slim lamps with three LED strips, shoulder crease and dropping lower line, wide LED tail lamps, ten-spoke wheels |
| 2021 EQS | One-bow arc, cab-forward stance, gloss-black front panel, light bands across front and rear, flush handles, frameless windows, black lower section, aero wheels, no exhaust |

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
| `cars.c/.h` | Car model table, public interface, display lists, wheel placement |
| `models/car_YYYY.c` | One file per car: its shape data and build functions |
| `geom.c/.h` | Geometry toolkit: splines, lofts, sweeps, tubes, revolves, rounded boxes, surface decals |
| `materials.c/.h` | Material table (specular, shininess, emission) and `setMaterial()` |
| `wheels.c/.h` | Revolved tyres and the nine rim styles |
| `carparts.c/.h` | Parts many cars share: lamps, plates, mirrors, wipers, seats, steering wheels, driver figure, wheel wells, and the closed-car body framework |
| `transition.c/.h` | Timeline state machine, time tunnel, scale swap and particles |
| `hud.c/.h` | 2D text overlay |

## Techniques

### Car modelling

**Splines.** Every profile is described by 8 to 20 control points. Between two
points a cubic curve is used whose slopes come from the neighbouring points
(Catmull-Rom style), so the shape passes through every point and bends smoothly.
The same idea in 3D gives smooth paths for tubes and wings.

**Lofted bodies.** A body is a chain of cross-sections along the car (60 to 90
of them), joined into one skin with quads. Each cross-section is a
superellipse, a rounded rectangle whose roundness goes from an ellipse to an
almost sharp box, between a bottom and a top height, and narrower at the top
than at the bottom (tumblehome). Five spline functions along x give the bottom,
top, half-width, top half-width and roundness, so a body can taper to a nose,
rise over a bonnet or drop into a tail. Raising the bottom into a half-circle
around each axle cuts the wheel arches; a dark liner and a dark block behind
them make a real wheel well. Vertex normals come from two tangents on the
surface (along the car and around the section); their cross product is flipped
if it points inwards, so every normal points out and the curves shade
smoothly.

**Glasshouse.** Closed cars have a second, narrower loft for the cabin. Each of
its quads is sorted by position and normal: the roof and the pillars are drawn
with the body, the rest becomes glass. The windscreen and rear window are the
two ends; where those surfaces turn sideways they become the A and C pillars.

**Sweeps, tubes and revolves.** A cross-section swept along a smooth path makes
wings, mudguards, bumpers, exhaust pipes, eyebrows and light bands. Straight
tubes make frames, spokes, columns and stays. Spinning an (r, z) profile around
an axis makes tyres, lamp bowls, lenses, hubcaps, horns and filler caps; a
repeated point in a profile makes a sharp crease.

**Decals.** Panel gaps, door handles, window seals, chrome strips, cladding,
louvres, lamps and plates are laid on the body surface: the loft can report any
surface point and its normal, and a local frame built from that normal places
flat shapes a few millimetres outside the surface, so they never fight with it
in the depth buffer.

**Materials and specular lighting.** `GL_COLOR_MATERIAL` still sets the
diffuse colour; `setMaterial()` adds a specular highlight and shininess (paint,
chrome, glass, rubber, leather, wood, brass...) and emission for lamp lenses, so
lamps look lit. The sun has a specular colour that follows the era theme, a
local viewer makes highlights slide over curved panels as the camera moves, and
separate specular keeps highlights white on coloured paint. Specular and
emission are reset after each car, so scenery is not affected.

**Glass pass.** Interiors (seats, dashboards, steering wheels, drivers in the
open cars) are solid geometry. Each car's glass is a separate display list,
drawn semi-transparent with blending on and depth writes off, after everything
solid in the frame and before the ripple ring, particles and HUD, so the
interior shows through and the glass never hides anything drawn later.

**Wheels.** Tyres are revolved with rounded shoulders and sidewalls: tall and
thin on the early cars, wide and low on the modern ones (with tread grooves).
Rim styles: laced wire spokes (1886, 1936), wooden artillery spokes with a
brass cap (1901), painted steel with chrome cap and ring (1954, 1963), ribbed
full hubcap (1976), 15-hole (1982) and 8-hole (1995) alloys, five twin spokes
(2002) and ten spokes (2013) with brake disc and calliper, and a nearly closed
aero disc (2021). Each wheel is a display list drawn four (or three) times with
its rotation; the calliper has its own list so it does not turn.

**Display lists.** Each car's static geometry is built once at start-up into one
list (plus one for its glass and one per wheel size), so a frame only replays
them. Only moving parts (the 1886 flywheel) are drawn fresh every frame.

**One file per car.** Each `models/car_YYYY.c` fills in a `CarModel`: name,
length, colours, axles, wheel specs and its build functions. The shared parts
live in `geom.c`, `wheels.c` and `carparts.c`, so a car file only describes what
makes that car itself.

### Animation and scene

**Timeline state machine.** `DRIVE` (4 s) shows the current model. `TRANSITION`
(about 2.5 s) runs the effect and swaps to the next model at the right moment.
All durations are named constants at the top of `transition.c`.

**Time tunnel (press `t`).** When a transition starts, a short tunnel (two walls and a roof)
is placed ahead of the car in world coordinates. The car never slows down; it
just drives in. The model is swapped when the car's centre reaches the middle
of the tunnel. Because the tunnel is longer than any car, the whole car is
hidden at that moment (its length follows the longest car), and the roof hides it from high camera angles too. The
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
