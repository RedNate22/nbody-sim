# nbody-sim

A gravitational N-body simulator, rendered in real time with raylib.

## What is the N-body problem?

"*The n-body problem is the problem of predicting the individual motions of a group of celestial objects interacting with each other gravitationally. Solving this problem has been motivated by the desire to understand the motions of the Sun, Moon, planets, and visible stars.*" - [Wikipedia (n-body problem)](https://en.wikipedia.org/wiki/N-body_problem)

Two bodies have an exact solution, an orbit shaped like an ellipse. Three or more do not, so instead of solving for future positions directly, this simulator calculates the gravitational pull between every pair of bodies at each small timestep and nudges everything forward, repeating many times a second.

## About nbody-sim

This is a real time 2D gravitational simulator. Each frame, every body's combined gravitational pull on every other body is calculated directly, then integrated forward with semi-implicit (symplectic) Euler integration.

Distance is measured in world units (40 units = 1 AU), mass in solar masses, and time in simulated days, using the real gravitational constant for this unit system (the square of the Gaussian gravitational constant, rescaled for world-unit distances). Orbital periods and relative speeds match real physics. A softening term also caps the gravitational force at very close range for numerical stability.

Real orbital periods range from 88 days (Mercury) to over 160 years (Neptune), so time is advanced faster than real time, at a pace set independently per scenario.

## Requirements

- GCC (or another C99 compiler)
- make
- git
- raylib

## Setup (Linux)

### 1. Install build tools

```shell
sudo apt update
sudo apt install build-essential git
```

### 2. Install raylib dependencies

```shell
sudo apt install libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev
```

### 3. Install raylib

```shell
git clone --depth 1 https://github.com/raysan5/raylib.git raylib
cd raylib/src/
make PLATFORM=PLATFORM_DESKTOP
sudo make install
cd ../..
rm -rf raylib
```

This installs raylib into `/usr/local/lib` and `/usr/local/include`, which the Makefile in this repo expects.

### 4. Build and run

```shell
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

```shell
git clone --depth 1 https://github.com/raysan5/raylib.git raylib
cd raylib/src/
make PLATFORM=PLATFORM_DESKTOP
```

### 3. Build and run

```shell
make
make run
```

or,

```shell
make
./nbody
```

## Controls

- `SPACE`: pause or resume the simulation
- `R`: reset the current scenario
- `1` / `2` / `3`: switch between the built-in scenarios
- `4`: load the saved custom scenario file
- `S`: save whatever is currently running as the custom scenario file
- Left click and drag: pan the camera
- Scroll wheel: zoom in and out

Hovering the mouse over any body shows its id, mass (in solar masses and Earth masses), position, and velocity. The top-left display also shows the current simulated day count.

### Scenarios

#### Scenario 1 (Stars Orbiting Black Hole)

A single massive body (3000 solar masses, acting as a black hole) at the center, surrounded by enough stars to fill out `MAX_BODIES`. Stars are placed at random angles with radius density falling off as 1/r², so they thin out further from the center, and their combined mass is pinned to 10% of the central mass. Each star's orbital velocity accounts for the mass of every other star closer to the center, not just the central mass, so the disc behaves a bit like a real galactic bulge rather than a simple two-body system.

#### Scenario 2 (Planets Orbiting Star)

A single 1 solar mass star with 15 planets scattered at random angles and random distances (1.25-22.5 AU), each given a random mass between a Mercury-like 1e-7 and a Jupiter-like 1e-3 solar masses. Orbital velocities are circular based on the star's mass alone.

#### Scenario 3 (Solar System)

The Sun plus the 9 real planets (Mercury through Pluto), using real masses and real orbital distances converted to world units. Each planet starts at a random angle rather than its real position, but on a circular orbit at the correct distance, so orbital periods and relative spacing match reality even though the "date" doesn't correspond to anything real.

#### Scenario 4 (Custom Scenario)

This scenario acts as a save/load slot. Press `S` to save whatever is currently running (any of the above, including one previously loaded this way) to `scenario.nbs`, and `4` to load it back. The file remembers which built-in scenario it came from, so the correct timescale is restored along with the bodies.

## Benchmarking (Custom Scenarios)

Running `./nbody` with no arguments opens the interactive window as above. A set of flags also let the simulation run headless, with no window at all, for scripted and reproducible runs.

### Scenario files

Any built-in scenario, or whatever is currently running in the interactive window, can be saved to a `.nbs` file, a binary snapshot of every body's exact position, velocity, mass, and id. The file also records which scenario it was generated from, so loading it back resumes at that scenario's time scale. Loading that file back reproduces the exact same starting conditions, with no randomness and no precision loss from the save and load round trip. These files are a raw dump of memory, so they're tied to the compiler and machine that produced them and shouldn't be copied between different platforms (expect undefined behaviour otherwise).

### Flags

- `--headless`: run without opening a window (default: `false`)
- `--mode=N`: which built-in scenario to generate if no `--scenario` is given, (default: `--mode=0` (Stars Orbiting Black Hole))
- `--bodies=<n>`: total body count to generate for mode 0 (Stars Orbiting Black Hole), up to `MAX_BODIES` (default `1200`); ignored by the other modes, which have a fixed body count
- `--scenario=<path>`: load starting conditions from a saved `.nbs` file instead of generating one
- `--dt=<value>`: fixed timestep for a headless run, in simulated days, independent of real time (default `1/60`)
- `--steps=<n>`: number of fixed timesteps to run before saving (default `3600`)
- `--out=<path>`: where to write the resulting bodies after the run (default `scenario.nbs`)
- `--compare-a=<path>` and `--compare-b=<path>`: instead of running a simulation, load two result files and report the difference between them
- `--tol=<value>`: largest position difference allowed before `--compare-a`/`--compare-b` reports a failure (default `1e-3`)

If `--scenario` AND `--out` aren't specified, then default behaviour will result in saving the scenario to `scenario.nbs` after every run (so long as `--steps` IS specified, otherwise it will run forever and never save).

#### dt

`dt` is how much simulated time passes per physics update, in days. Smaller is more accurate but slower to compute; larger is faster but less accurate, and can become unstable for bodies on fast, tight orbits if pushed too far.

#### steps

`steps` is how many `dt`-sized jumps to run before stopping and saving. Total simulated time covered is `steps * dt` days.

### Example: verifying a physics change

If you wish to experiment with the force and integration calculations, for example switching to a different integrator (semi-implicit Euler to something like Verlet or Runge-Kutta), moving from the current O(n²) brute-force approach to something like Barnes-Hut, or just optimising the existing force calculation, this confirms the new version still produces the same physics as the old one.

Generate a fixed starting scenario once and save it:

```shell
./nbody --headless --mode=0 --steps=0 --out=scenario.nbs
```

Run it for a fixed number of steps and save the result, before making any changes:

```shell
./nbody --headless --scenario=scenario.nbs --dt=1 --steps=3650 --out=result_before.nbs
```

Make the change, rebuild, and run the identical command again, writing to a different file:

```shell
make
./nbody --headless --scenario=scenario.nbs --dt=1 --steps=3650 --out=result_after.nbs
```

Compare the two results:

```shell
./nbody --compare-a=result_before.nbs --compare-b=result_after.nbs --tol=0.001
```

This reports the largest position and velocity difference between the two runs and whether it's within the given tolerance. Exact equality isn't expected if the change reorders any floating point summation, since floating point addition isn't perfectly associative, but the difference should stay small relative to the tolerance if the change preserves the same physics.

### Example: choosing a timestep

Useful when picking a `dt` for a scenario with fast, tight orbits (Mercury, or close binary stars), where too large a timestep will show visible drift or instability rather than just harmless rounding error.

Generate a fixed starting scenario once and save it:

```shell
./nbody --headless --mode=2 --steps=0 --out=scenario.nbs
```

Run it for one simulated year at a trusted, fine timestep, treated as the reference:

```shell
./nbody --headless --scenario=scenario.nbs --dt=0.1 --steps=3650 --out=reference.nbs
```

Run the same simulated year again at a coarser timestep you're considering using:

```shell
./nbody --headless --scenario=scenario.nbs --dt=2 --steps=182 --out=coarse.nbs
```

Note that `steps` changes between the two runs but `dt * steps` stays the same (365 simulated days either way), so both runs cover the same span of simulated time.

Compare the two:

```shell
./nbody --compare-a=reference.nbs --compare-b=coarse.nbs --tol=0.001
```

If the result is a FAIL, `dt=2` is too coarse for this scenario and is introducing real error, not just floating point noise, and a smaller `dt` is needed. If it passes, `dt=2` is fine for this scenario and there's no need to pay for the extra steps a finer timestep would cost.

### Example: sensitivity to initial conditions

The n-body problem is chaotic once there are more than two bodies with any real gravitational influence on each other, meaning two starts that differ by an immeasurably small amount can end up wildly different after enough time. This is a property of the physics.

Save a scenario with several mutually interacting bodies, then run it twice with a barely different `dt`, close enough that the difference stands in for an imperceptibly different starting condition. `steps` is adjusted between the two runs so both cover almost exactly the same span of simulated time (`dt * steps` differs by only 0.0001 days), so the comparison isolates the effect of the tiny `dt` difference rather than just measuring two snapshots taken at slightly different moments:

```shell
./nbody --headless --mode=0 --steps=0 --out=chaotic.nbs
./nbody --headless --scenario=chaotic.nbs --dt=1      --steps=10000 --out=chaotic_a.nbs
./nbody --headless --scenario=chaotic.nbs --dt=1.0001 --steps=9999  --out=chaotic_b.nbs
./nbody --compare-a=chaotic_a.nbs --compare-b=chaotic_b.nbs --tol=1e-3
```

```
max position delta: 599743
max velocity delta: 92.9006
FAIL: exceeded tolerance
```

Now do the same with the solar system scenario, where the bodies are few and widely spaced:

```shell
./nbody --headless --mode=2 --steps=0 --out=solar.nbs
./nbody --headless --scenario=solar.nbs --dt=1      --steps=10000 --out=solar_a.nbs
./nbody --headless --scenario=solar.nbs --dt=1.0001 --steps=9999  --out=solar_b.nbs
./nbody --compare-a=solar_a.nbs --compare-b=solar_b.nbs --tol=1e-3
```

```
max position delta: 1.83307
max velocity delta: 0.0696116
FAIL: exceeded tolerance
```

While both report FAIL at `--tol=1e-3`, the actual scale of the divergence is comparatively very different. The solar system's max position delta comes out around 1.8 world units, which is driven by Mercury and Venus, the fastest moving bodies. These accumulate ordinary phase drift over roughly 113 Mercury orbits.

```
body id 1: position delta 1.04244 exceeds tolerance 0.001 <-- Mercury
body id 2: position delta 1.83307 exceeds tolerance 0.001 <-- Venus
```

Comparing this to the black hole scenario's max position delta comes out around 600,000 world units, which is several orders of magnitude larger than the entire scenario itself. This means the stars have ended up somewhere else entirely, rather than just nudged along their orbit.

```
body id 2: position delta 599743 exceeds tolerance 0.001 <-- some poor, unfortunate body
```

This is a key reason why long-term weather forecasting and long-term solar system forecasting both eventually break down over time -- no amount of precision removes this sensitivity, only delays it.

## Cleaning up build files

```shell
make clean
```

This will not remove any `.nbs` files.