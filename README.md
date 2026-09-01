# nbody-sim

A gravitational N-body simulator, rendered in real time with raylib.

## What is the N-body problem?

"*The n-body problem is the problem of predicting the individual motions of a group of celestial objects interacting with each other gravitationally. Solving this problem has been motivated by the desire to understand the motions of the Sun, Moon, planets, and visible stars.*" - [Wikipedia (n-body problem)](https://en.wikipedia.org/wiki/N-body_problem)

Two bodies have an exact solution, an orbit shaped like an ellipse. Three or more do not, so instead of solving for future positions directly, this simulator calculates the gravitational pull between every pair of bodies at each small timestep and nudges everything forward, repeating many times a second.

## About nbody-sim

This is a real time 2D gravitational simulator. Each frame, every body's combined gravitational pull on every other body is calculated directly, then integrated forward with semi-implicit (symplectic) Euler integration.

Distance is measured in world units (40 units = 1 AU), mass in solar masses, and time in simulated days, using the real gravitational constant for this unit system (the square of the Gaussian gravitational constant, rescaled for world-unit distances). Orbital periods and relative speeds match real physics. Rendered body radii are stylized for visibility rather than to scale, and a softening term caps the gravitational force at very close range for numerical stability.

Real orbital periods range from 88 days (Mercury) to over 160 years (Neptune), so the interactive display advances simulated time faster than real time (`TIME_SCALE` in `main.c`).

The stars-orbiting-a-black-hole scenario places bodies with uniform-by-area disc sampling and derives each body's orbital velocity from the mass enclosed within its orbit, rather than the central mass alone, with total disc mass scaled to body count. This keeps the scenario stable at high body counts, where orbiting mass is no longer negligible next to the central mass.

## Requirements

- GCC (or another C99 compiler)
- make
- git
- raylib

## Setup (Linux)

### 1. Install build tools

```
sudo apt update
sudo apt install build-essential git
```

### 2. Install raylib dependencies

```
sudo apt install libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev
```

### 3. Install raylib

```
git clone --depth 1 https://github.com/raysan5/raylib.git raylib
cd raylib/src/
make PLATFORM=PLATFORM_DESKTOP
sudo make install
cd ../..
rm -rf raylib
```

This installs raylib into `/usr/local/lib` and `/usr/local/include`, which the Makefile in this repo expects.

### 4. Build and run

```
make
./project
```

## Setup (Windows)

The simplest path is w64devkit, a portable GCC and make environment bundled by the raylib author.

### 1. Install w64devkit

Download the latest release from [w64devkit releases](https://github.com/skeeto/w64devkit/releases), extract it somewhere permanent (e.g. `C:\w64devkit`), and run `w64devkit.exe` to open its terminal. Use this terminal for all the steps below instead of PowerShell or cmd.

### 2. Install raylib

Download the prebuilt raylib release matching your GCC version from [raylib releases](https://github.com/raysan5/raylib/releases), for example `raylib-5.5_win64_mingw-w64.zip`. Extract it and copy `include` and `lib` into the w64devkit install, or reference the paths directly in the Makefile.

Alternatively build from source inside the w64devkit terminal:

```
git clone --depth 1 https://github.com/raysan5/raylib.git raylib
cd raylib/src/
make PLATFORM=PLATFORM_DESKTOP
```

### 3. Build and run

```
make
make run
```

## Controls

- `SPACE`: pause or resume the simulation
- `R`: reset the current scenario
- `1` / `2` / `3`: switch between the built-in scenarios (stars orbiting a black hole, planets orbiting a star, solar system)
- `4`: load the saved custom scenario file (see Command line usage below)
- `S`: save whatever is currently running as the custom scenario file
- Left click and drag: pan the camera
- Scroll wheel: zoom in and out

Hovering the mouse over any body shows its id, mass (in solar masses and Earth masses), position, and velocity. The top-left display also shows the current simulated day count.

## Command line usage

Running `./nbody` with no arguments opens the interactive window as above. A set of flags also let the simulation run headless, with no window at all, for scripted and reproducible runs.

### Scenario files

Any built-in scenario, or whatever is currently running in the interactive window, can be saved to a `.nbs` file, a binary snapshot of every body's exact position, velocity, mass, and id. Loading that file back reproduces the exact same starting conditions, with no randomness and no precision loss from the save and load round trip. These files are a raw dump of memory, so they're tied to the compiler and machine that produced them and shouldn't be copied between different platforms.

### Flags

- `--headless`: run without opening a window
- `--mode=N`: which built-in scenario to generate if no `--scenario` is given (0: stars orbiting a black hole, 1: planets orbiting a star, 2: solar system)
- `--scenario=<path>`: load starting conditions from a saved `.nbs` file instead of generating one
- `--dt=<value>`: fixed timestep for a headless run, in simulated days, independent of real time (default `1/60`)
- `--steps=<n>`: number of fixed timesteps to run before saving (default `3600`)
- `--out=<path>`: where to write the resulting bodies after the run (default `output.nbs`)
- `--compare-a=<path>` and `--compare-b=<path>`: instead of running a simulation, load two result files and report the difference between them
- `--tol=<value>`: largest position difference allowed before `--compare-a`/`--compare-b` reports a failure (default `1e-3`)

### Example: checking that a code change didn't alter the simulation

Useful any time `update_bodies` is modified, whether that's a refactor, an optimization, or a rewrite, and you want to confirm the physics still comes out the same.

Generate a fixed starting scenario once and save it:

```
./nbody --headless --mode=1 --steps=0 --out=scenario.nbs
```

Run it for a fixed number of steps and save the result, before making any changes:

```
./nbody --headless --scenario=scenario.nbs --dt=0.01667 --steps=5000 --out=result_before.nbs
```

Make the change, rebuild, and run the identical command again, writing to a different file:

```
make
./nbody --headless --scenario=scenario.nbs --dt=0.01667 --steps=5000 --out=result_after.nbs
```

Compare the two results:

```
./nbody --compare-a=result_before.nbs --compare-b=result_after.nbs --tol=0.01
```

This reports the largest position and velocity difference between the two runs and whether it's within the given tolerance. Exact equality isn't expected if the change reorders any floating point summation, since floating point addition isn't perfectly associative, but the difference should stay small relative to the tolerance if the change preserves the same physics.

## Cleaning up build files

```
make clean
```

## Notes

If you're on WSL2 without WSLg, you'll need an X server (e.g. VcXsrv) running on the Windows side, with `DISPLAY` set correctly, since raylib opens a window through X11.