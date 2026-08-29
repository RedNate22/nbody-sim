# nbody-sim

A gravitational N-body simulator, rendered in real time with raylib.

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

## Cleaning up build files

```
make clean
```

## Notes

If you're on WSL2 without WSLg, you'll need an X server (e.g. VcXsrv) running on the Windows side, with `DISPLAY` set correctly, since raylib opens a window through X11.