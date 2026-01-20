# Known Issues and Limitations

This document provides an honest assessment of known bugs, limitations, disabled features, and workarounds.

## Build Warnings

The following compiler warnings are present but do not affect functionality:

| Warning | Location | Description |
|---------|----------|-------------|
| C4267 | Various | size_t to int/unsigned int conversions |
| C4244 | Genome.cpp | float to int/uint8_t conversions |
| C4996 | Various | Unsafe C functions (ctime, sscanf, getenv, strncpy, localtime) |
| C4099 | BiomeSystem.h | class/struct mismatch for IslandData |
| C4005 | AudioManager.cpp | WIN32_LEAN_AND_MEAN macro redefinition |

These are cosmetic warnings and do not cause runtime issues.

## Disabled Features

### UI Dashboard (Deliberately Disabled)
**Status**: Commented out in CMakeLists.txt (lines 118-125)
**Reason**: Complex dependencies on genetics system that aren't fully integrated
**Files**: src/ui/DashboardMetrics.cpp, SimulationDashboard.cpp, NEATVisualizer.cpp, PhylogeneticTreeVisualizer.cpp
**Impact**: No advanced UI visualizations for genetics or family trees

### SimulationOrchestrator (Not Used)
**Status**: Exists but not compiled
**Reason**: Replaced by inline implementation in main.cpp
**Impact**: None - functionality is in main.cpp

## Scaffolded But Not Integrated

These systems have code but aren't connected to the main simulation:

### Audio System
**Files**: src/audio/*.cpp
**Status**: Compiles but produces no sound
**Workaround**: None - wait for future integration

### Advanced Behavior Systems
**Files**: src/entities/behaviors/*.cpp
**Status**: TerritorialBehavior, SocialGroups, PackHunting, MigrationBehavior, ParentalCare
**Issue**: Infrastructure exists but not called from simulation loop
**Workaround**: Creatures use basic steering behaviors only

### Climate Effects on Creatures
**Files**: src/environment/ClimateSystem.cpp, SeasonManager.cpp
**Status**: Systems track temperature/humidity but creatures don't respond
**Workaround**: Visual day/night works, but no gameplay impact

### Natural Disasters
**Files**: src/environment/DisasterSystem.cpp
**Status**: System exists but events are never triggered
**Workaround**: None - disasters don't occur

### Multi-Island Archipelago
**Files**: src/core/MultiIslandManager.h, src/environment/IslandGenerator.cpp
**Status**: Code exists but not integrated into main.cpp
**Workaround**: Single continuous terrain only

### Full NEAT Evolution
**Files**: src/ai/NEATGenome.cpp
**Status**: NEAT genome structure exists but not used for breeding
**Workaround**: Simple genetic inheritance instead of neuroevolution

## Known Bugs

### Performance

1. **GPU Steering Compute may fail to initialize**
   - The compute shader requires Runtime/Shaders/SteeringCompute.hlsl to be in working directory
   - If initialization fails, CPU fallback is used automatically
   - Check console output for "GPU STEERING COMPUTE STATUS" section
   - ImGui panel shows GPU status clearly (green=enabled, red=CPU fallback)
   - Agent 36: GPU steering is now attempted with proper error capture

2. **High creature counts slow simulation**
   - Above ~5000 creatures, FPS drops significantly
   - GPU steering helps but readback is synchronous
   - Workaround: Keep creature count below 5000 for smooth experience

2. **First frame hitch**
   - Initial frame may have noticeable delay while resources load
   - Workaround: Wait 1-2 seconds after launch

### Rendering

1. **Creatures may clip through terrain**
   - Ground clearance calculation isn't perfect
   - Workaround: Usually minor, creatures adjust on next frame

2. **Grass may pop in/out**
   - LOD distance causes visible transitions
   - Workaround: Reduce grass render distance in debug panel

### Simulation

1. **Creatures can spawn underwater**
   - Spawn logic doesn't always check water level
   - Workaround: Use spawn controls in debug panel with appropriate spawn radius

2. **Energy system is simplified**
   - Creatures don't truly "die" from zero energy, they just become inactive
   - Workaround: Pool handles cleanup, appears correct to user

3. **Reproduction is basic**
   - Offspring inherit traits but no fitness pressure
   - Evolution happens but without survival-of-fittest mechanics
   - Workaround: Accept as current limitation

### Save/Load

1. **Large save files with many creatures**
   - 10,000+ creatures creates large save files
   - Loading may take several seconds
   - Workaround: Keep creature count reasonable

2. **Auto-save may cause brief stutter**
   - Serialization happens on main thread
   - Workaround: Disable auto-save if this is problematic

### UI

1. **ImGui windows can overlap**
   - Multiple panels open may overlap initially
   - Workaround: Drag windows to reposition

2. **Follow camera may jitter**
   - Smooth follow interpolation can wobble with fast creatures
   - Workaround: Use free camera for precise viewing

## Platform Limitations

- **Windows only**: DirectX 12 is Windows-exclusive
- **No Linux/Mac support**: Would require Vulkan port
- **No controller support**: Keyboard/mouse only
- **No multi-monitor support**: Renders to single window

## What's Honest vs Marketing

| What We Claim | Reality |
|---------------|---------|
| "Evolution simulator" | Basic trait inheritance, not full GA |
| "Neural network brains" | Code exists, not driving decisions |
| "Ecosystem simulation" | Basic predator-prey only |
| "10,000 creatures" | Possible but with reduced FPS |
| "Procedural animation" | Skeletal code exists, creatures use simple rotation |

## Workarounds Summary

1. **No sound**: Expected - audio system not connected
2. **Slow with many creatures**: Keep below 5000
3. **No advanced evolution**: Use spawn controls to observe basic inheritance
4. **No disasters/climate effects**: Visual-only day/night cycle works
5. **UI dashboard missing**: Use ImGui debug panel (F1) instead

## Reporting New Issues

If you find a bug not listed here:
1. Note the creature count and simulation time
2. Check if it's reproducible
3. Document steps to reproduce
4. Check console output for error messages

---

*Last updated: January 19, 2026*
