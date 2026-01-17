# Stub and Placeholder Status

This document tracks all stub functions, placeholders, and partial implementations in the codebase.
**Last updated:** January 2026

## Status Legend
- **IMPLEMENTED**: Previously a stub, now fully functional
- **INTENTIONAL STUB**: Deliberate no-op (e.g., conditional compilation, graceful degradation)
- **PARTIAL**: Has working fallback but lacks full implementation
- **DOCUMENTED**: Stub with clear error messaging when called

---

## Fixed Stubs (Now Implemented)

### BiomeSystem::generateFromIslandData
- **File:** `src/environment/BiomeSystem.cpp`
- **Status:** IMPLEMENTED
- **Details:** Now properly delegates to `generateBiomeMap()` using IslandData's heightmap, dimensions, and seed. Also processes cave entrances, lake shores, and river banks from island data.

### PlanetTheme::applyToBiomeSystem
- **File:** `src/environment/PlanetTheme.cpp`
- **Status:** IMPLEMENTED
- **Details:** Applies terrain palette colors to appropriate biomes (beaches, forests, grasslands, snow, volcanic, etc.)

### SpeciesNamingSystem::importFromJson
- **File:** `src/entities/SpeciesNaming.cpp`
- **Status:** IMPLEMENTED
- **Details:** Basic JSON parser for species data. Validates input, extracts species fields, and fails loudly with assert on malformed data.

---

## Intentional Stubs (By Design)

### Terrain Rendering (Forge Engine Mode)
- **File:** `src/environment/Terrain.cpp`
- **Functions:** `createBuffers()`, `render()`, `renderForShadow()`
- **Status:** INTENTIONAL STUB
- **Details:** When `USE_FORGE_ENGINE` is defined, these are intentionally empty because rendering is handled by `TerrainRendererDX12` in the Forge Engine.

### TerrainChunkManager (Forge Engine Mode)
- **File:** `deprecated/terrain_chunking/TerrainChunkManager.cpp`
- **Status:** DEPRECATED + INTENTIONAL STUB
- **Details:** Entire class is deprecated and moved to `deprecated/` folder. When `USE_FORGE_ENGINE` is defined, stubs allow linking without standalone DX12Device.

### AudioManager (Non-Windows)
- **File:** `src/audio/AudioManager.cpp`
- **Functions:** Platform-specific audio stubs
- **Status:** INTENTIONAL STUB
- **Details:** Non-Windows platforms get stub implementations that return success but produce no audio. This is intentional cross-platform support.

### FungiSystem::render
- **File:** `src/environment/FungiSystem.cpp`
- **Status:** INTENTIONAL STUB
- **Details:** Deliberately empty - fungi rendering is handled by main renderer via `getMushrooms()`. This function exists for API consistency.

---

## Partial Implementations (Working Fallback)

### FishSchoolingManager GPU Pipeline
- **File:** `src/entities/aquatic/FishSchooling.cpp`
- **Functions:** `createComputePipeline()`, `createBuffers()`, `updateGPU()`, `syncFromGPU()`
- **Status:** PARTIAL
- **Details:** GPU compute not implemented. Functions return false and log warning, then fall back to CPU-based schooling simulation. CPU path is fully functional.

### IslandCamera Cinematic Mode
- **File:** `src/graphics/IslandCamera.cpp`
- **Function:** `update()` case `CINEMATIC`
- **Status:** PARTIAL
- **Details:** Falls back to island view mode. To implement: add keyframe-based camera paths with smooth easing.

---

## Remaining TODOs (Non-Critical)

These are minor TODOs that don't affect core functionality:

| File | Line | Description | Priority |
|------|------|-------------|----------|
| `src/animation/SecondaryMotion.cpp` | 417 | Override search to point antennae at target | Low |
| `src/entities/small/SmallCreatures.cpp` | 1522 | Web mechanics for spider prey detection | Low |
| `src/graphics/PostProcess_DX12.cpp` | 878-898 | DXC shader precompilation placeholders | Low |
| `src/entities/genetics/NicheSystem.cpp` | 997 | Resource score placeholder | Low |
| `src/scenarios/DarwinMode.cpp` | 220 | Descendant species tracking placeholder | Low |

---

## How to Handle New Stubs

When adding new stub functions:

1. **If intentional:** Document why (conditional compilation, fallback, etc.)
2. **If temporary:** Add error logging and assert:
   ```cpp
   std::cerr << "ERROR: [FunctionName] is not implemented!" << std::endl;
   assert(false && "Stub function called");
   ```
3. **If partial:** Ensure graceful fallback exists
4. **Update this document** with the new stub entry

---

## Build Verification

Run the following to verify no stub is hit silently during normal operation:

```bash
# Debug build with asserts enabled
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run with typical workload
./build/OrganismEvolution
```

Any unintentional stub hit will trigger an assert or error message.
