# Build and Run Guide

This guide will help you compile and run the OrganismEvolution simulator.

## Prerequisites

You must have MSYS2 installed with all required development packages. If you haven't set up MSYS2 yet, see [SETUP_GUIDE.md](SETUP_GUIDE.md) for detailed instructions.

### Quick Prerequisites Check

Open **MSYS2 MinGW 64-bit** terminal and verify installations:

```bash
gcc --version      # Should show 13.x or later
cmake --version    # Should show 3.x
git --version      # Should show 2.x
```

If any command fails, install the missing packages:

```bash
pacman -S mingw-w64-x86_64-gcc \
          mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-glfw \
          mingw-w64-x86_64-glew \
          mingw-w64-x86_64-glm \
          mingw-w64-x86_64-mesa \
          git \
          make
```

## Building the Project

### Step 1: Navigate to Project Directory

```bash
cd /c/Users/andre/Desktop/OrganismEvolution
```

(Adjust the path if your project is in a different location)

### Step 2: Create Build Directory

```bash
mkdir build
cd build
```

### Step 3: Generate Build Files with CMake

```bash
cmake .. -G "MinGW Makefiles"
```

Expected output:
```
-- Build type: Release
-- C++ Standard: 17
-- Compiler: GNU 13.x.x
-- Configuring done
-- Generating done
-- Build files have been written to: ...
```

### Step 4: Compile the Project

```bash
cmake --build .
```

This will compile all source files. Expected output:
```
[  4%] Building CXX object CMakeFiles/OrganismEvolution.dir/src/main.cpp.obj
[  8%] Building CXX object CMakeFiles/OrganismEvolution.dir/src/graphics/Shader.cpp.obj
[ 12%] Building CXX object CMakeFiles/OrganismEvolution.dir/src/graphics/Camera.cpp.obj
...
[100%] Linking CXX executable OrganismEvolution.exe
[100%] Built target OrganismEvolution
```

### Step 5: Run the Simulator

```bash
./OrganismEvolution.exe
```

Or simply:

```bash
./OrganismEvolution
```

## What You Should See

When you run the simulator, you should see:

1. **Initialization Messages** in the console:
   ```
   ==================================================
       OrganismEvolution - Evolution Simulator
   ==================================================

   Initializing...
   OpenGL Version: 4.6.0 ...
   GLSL Version: 4.60 ...

   Generating terrain...
   Creating initial population...
   Spawning initial food...
   Simulation initialized! Population: 50
   ```

2. **3D Window** showing:
   - Blue sky background
   - Colored terrain (blue water, sandy beaches, green grass, gray mountains)
   - Small creatures moving around
   - Food items scattered on land

3. **Live Statistics** in console (updates every second):
   ```
   Population:   45 | Generation:   1 | Avg Fitness: 23.45 | FPS:  60
   ```

## Controls

Once the simulation is running:

| Key | Action |
|-----|--------|
| **WASD** | Move camera forward/left/back/right |
| **Mouse** | Look around (must press TAB first to capture mouse) |
| **Space** | Move camera up |
| **Ctrl** | Move camera down |
| **TAB** | Toggle mouse capture on/off |
| **P** | Pause/Resume simulation |
| **+** (Equal key) | Speed up simulation (2x, 4x, 8x) |
| **-** (Minus key) | Slow down simulation (0.5x, 0.25x) |
| **ESC** | Exit simulator |

## Troubleshooting

### "OrganismEvolution.exe not found"

Make sure you're in the `build` directory:
```bash
cd build
ls -la OrganismEvolution.exe
```

### "Failed to open shader file"

The executable needs to be run from the `build` directory so it can find shaders:
```bash
cd build
./OrganismEvolution.exe
```

Verify shaders exist:
```bash
ls -la ../shaders/
```

### OpenGL Errors

If you see OpenGL initialization errors:

1. Update graphics drivers
2. Check OpenGL version:
   ```bash
   glxinfo | grep "OpenGL version"
   ```
   (Requires `mesa-utils` package)

3. Try running with software rendering:
   ```bash
   LIBGL_ALWAYS_SOFTWARE=1 ./OrganismEvolution.exe
   ```

### Compilation Errors

If compilation fails:

1. **Clean build directory:**
   ```bash
   cd build
   rm -rf *
   cmake .. -G "MinGW Makefiles"
   cmake --build .
   ```

2. **Check you're using MinGW 64-bit terminal** (not MSYS2 or UCRT64)

3. **Reinstall development packages:**
   ```bash
   pacman -S --needed mingw-w64-x86_64-toolchain
   ```

### Missing DLL Errors

If you see "missing DLL" errors when running:

```bash
ldd OrganismEvolution.exe
```

This shows required DLLs. Make sure MSYS2's MinGW bin is in your PATH:
```bash
export PATH="/mingw64/bin:$PATH"
```

Or copy required DLLs to the build directory.

### Black Screen / No Rendering

1. Check if terrain is generating:
   - Look for "Generating terrain..." in console output
   - Should see "Simulation initialized! Population: 50"

2. Move camera around with WASD to see terrain from different angles

3. Press TAB to capture mouse, then move mouse to look around

## Performance Tips

### Low FPS (< 30)

1. **Reduce terrain detail** - Edit `src/core/Simulation.cpp`:
   ```cpp
   terrain = std::make_unique<Terrain>(100, 100, 2.0f); // Instead of 150, 150
   ```

2. **Reduce population cap** - Edit `src/core/Simulation.cpp`:
   ```cpp
   if (creatures.size() > 100) { // Instead of 200
   ```

3. **Build in Release mode** (default, but verify):
   ```bash
   cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
   ```

### High CPU Usage

This is normal - the simulation is computationally intensive. To reduce CPU:

1. Use pause feature (P key) when not actively watching
2. Slow down simulation speed (- key)
3. Run with lower priority:
   ```bash
   nice -n 10 ./OrganismEvolution.exe
   ```

## Rebuilding After Code Changes

If you modify the source code:

```bash
cd build
cmake --build .
./OrganismEvolution.exe
```

For major changes, clean rebuild:

```bash
cd build
rm -rf *
cmake .. -G "MinGW Makefiles"
cmake --build .
```

## Debug Mode

To build with debug symbols for troubleshooting:

```bash
cd build
rm -rf *
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

Run with GDB debugger:

```bash
gdb ./OrganismEvolution.exe
```

## Next Steps

Once the simulation is running:

1. **Watch evolution happen** - Observe creatures over multiple generations
2. **Experiment with parameters** - Edit values in source files and rebuild
3. **Add features** - See [PROJECT_PLAN.md](PROJECT_PLAN.md) for ideas

## Getting Help

- Review [SETUP_GUIDE.md](SETUP_GUIDE.md) for environment setup
- Check [README.md](README.md) for project overview
- Open an issue: https://github.com/CurioCrafter/OrganismEvolution/issues

---

**Happy Simulating!** 🧬
