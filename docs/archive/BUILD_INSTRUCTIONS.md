# Build Instructions

> NOTE: This document contains legacy OpenGL/MSYS2 notes. The current DX12
> build uses MSVC; see `BUILD_AND_RUN.md` and `SETUP_GUIDE.md`.

## Changes Made

### Tree and Grass Rendering (COMPLETED)
1. Added `getMeshForType()` method to [VegetationManager.h](src/environment/VegetationManager.h:39)
2. Implemented mesh lookup in [VegetationManager.cpp](src/environment/VegetationManager.cpp:145-156)
3. Added getter methods to [MeshData.h](src/graphics/mesh/MeshData.h:135-137):
   - `getVAO()` - Returns OpenGL VAO handle
   - `getIndexCount()` - Returns number of indices for drawing
4. Completed [Simulation.cpp:432-497](src/core/Simulation.cpp:432-497) `renderVegetation()` function:
   - Binds appropriate tree mesh based on type (OAK, PINE, WILLOW)
   - Sets tree-specific colors (brown for oak, green for pine, tan for willow)
   - Calls `glDrawElements()` to actually render each tree
   - Renders grass clusters using simple quads

### Creature Movement Investigation
The creature movement code appears correct:
- Creatures start with paused=false [Simulation.cpp:8](src/core/Simulation.cpp:8)
- Neural network processes inputs and outputs speed multiplier
- Velocity is calculated as `genome.speed * speedMultiplier` [Creature.cpp:96](src/entities/Creature.cpp:96)
- Position updated by `position += velocity * deltaTime` [Creature.cpp:106](src/entities/Creature.cpp:106)

**Possible issues:**
1. Neural network might be outputting very low speed multipliers
2. Creatures might be stuck avoiding water and not finding valid movement paths
3. DeltaTime might be incorrect

## To Build and Test

You need to rebuild from MSYS2 MinGW 64-bit terminal:

```bash
cd /c/Users/andre/Desktop/OrganismEvolution
./build.sh
```

Or manually:
```bash
cd /c/Users/andre/Desktop/OrganismEvolution/build
cmake .. -G "MinGW Makefiles"
mingw32-make -j4

# Copy DLLs
cp /c/msys64/mingw64/bin/glfw3.dll .
cp /c/msys64/mingw64/bin/glew32.dll .
cp /c/msys64/mingw64/bin/libgcc_s_seh-1.dll .
cp /c/msys64/mingw64/bin/libstdc++-6.dll .
cp /c/msys64/mingw64/bin/libwinpthread-1.dll .

# Run
./OrganismEvolution.exe
```

## What to Look For After Rebuild

1. **Trees visible**: Should see 4 trees (oak, pine, or willow) with brown/green colors
2. **Grass visible**: Should see ~68 grass clusters as small green quads
3. **Creatures moving**: Creatures should be walking around seeking food

If creatures still aren't moving, we need to debug:
- Add console output to show creature velocities
- Check if creatures are finding food
- Verify neural network is producing non-zero outputs
