# Project Cleanup Status

## Current State Analysis

### Legacy OpenGL Files (MOVED OUT OF src)

| File | Status | Notes |
|------|--------|------|
| deprecated/opengl/legacy_src/platform/Win32Window.* | Legacy Win32 window + WGL | Moved out of src |
| deprecated/opengl/main.cpp | Legacy OpenGL entry | Kept for reference |
| deprecated/opengl/main_enhanced.cpp | Legacy OpenGL variant | Kept for reference |
| deprecated/opengl/Shader.* | Legacy shader system | Kept for reference |
| deprecated/opengl/ShadowMap.* | Legacy shadow map | Kept for reference |
| deprecated/opengl/legacy_src/core/Simulation.* | Legacy OpenGL simulation loop | Moved out of src |
| deprecated/opengl/legacy_src/graphics/rendering/CreatureRenderer.* | Legacy OpenGL creature renderer | Moved out of src |
| deprecated/opengl/legacy_src/ui/DashboardUI.* | Legacy OpenGL ImGui UI | Moved out of src |
| deprecated/opengl/legacy_src/utils/Screenshot.* | Legacy OpenGL screenshot | Moved out of src |
| deprecated/opengl/legacy_src/utils/MeshDebugger.h | Legacy OpenGL debug | Moved out of src |

### Graphics Headers in src (API-agnostic)

| File | Notes |
|------|-------|
| src/graphics/mesh/MeshData.h | CPU-only mesh container (no OpenGL handles) |
| src/graphics/Camera.h | Uses bool instead of GLboolean |
| src/entities/Creature.h | Uses uint32_t placeholder for legacy render handle |

### Files with DirectX 12 (KEEP)

| File | Purpose | Status |
|------|---------|--------|
| src/main.cpp | DX12 entry point (Forge RHI) | Working |
| src/graphics/DX12Device.cpp | Device initialization | Not in current build |
| src/graphics/DX12Device.h | Device class | Not in current build |
| src/graphics/Shader_DX12.h | PSO management | Not in current build |
| src/graphics/ShadowMap_DX12.cpp | Shadow depth pass | Not in current build |
| src/graphics/ShadowMap_DX12.h | Shadow class | Not in current build |
| src/graphics/rendering/CreatureRenderer_DX12.cpp | Instanced creatures | Not in current build |
| src/graphics/rendering/CreatureRenderer_DX12.h | Creature renderer | Not in current build |
| src/ai/GPUSteeringCompute.cpp | Compute shader AI | Not in current build |
| src/ai/GPUSteeringCompute.h | Compute class | Not in current build |
| src/ui/ImGuiManager.cpp | ImGui manager (DX12) | Not in current build |
| src/ui/ImGuiManager.h | ImGui manager | Not in current build |

### HLSL Shaders (KEEP)

| File | Purpose | Status |
|------|---------|--------|
| shaders/hlsl/Terrain.hlsl | Terrain with biomes | Complete |
| shaders/hlsl/Creature.hlsl | Instanced creatures | Complete |
| shaders/hlsl/Nametag.hlsl | Billboard nametags | Complete |
| shaders/hlsl/Vegetation.hlsl | Trees and plants | Complete |
| shaders/hlsl/SteeringCompute.hlsl | GPU AI compute | Complete |

### Pure Logic Files (KEEP UNCHANGED)

These files have no graphics dependencies:

- src/entities/Creature.cpp (except rendering refs)
- src/entities/Genome.cpp
- src/entities/NeuralNetwork.cpp
- src/entities/SteeringBehaviors.cpp
- src/entities/SensorySystem.cpp
- src/entities/genetics/* (all files)
- src/environment/Food.cpp
- src/environment/SeasonManager.cpp
- src/environment/ProducerSystem.cpp
- src/environment/DecomposerSystem.cpp
- src/environment/EcosystemMetrics.cpp
- src/environment/EcosystemManager.cpp
- src/ai/NeuralNetwork.cpp
- src/ai/NEATGenome.cpp
- src/ai/BrainModules.cpp
- src/physics/* (all files)
- src/utils/Random.cpp
- src/utils/PerlinNoise.cpp
- src/utils/SpatialGrid.cpp
- src/utils/CommandProcessor.cpp

## Quick Cleanup Script

```batch
@echo off
REM Move legacy OpenGL files out of src
mkdir deprecated\opengl\legacy_src\core
mkdir deprecated\opengl\legacy_src\graphics\rendering
mkdir deprecated\opengl\legacy_src\platform
mkdir deprecated\opengl\legacy_src\ui
mkdir deprecated\opengl\legacy_src\utils

move src\core\Simulation.cpp deprecated\opengl\legacy_src\core\
move src\core\Simulation.h deprecated\opengl\legacy_src\core\
move src\graphics\rendering\CreatureRenderer.cpp deprecated\opengl\legacy_src\graphics\rendering\
move src\graphics\rendering\CreatureRenderer.h deprecated\opengl\legacy_src\graphics\rendering\
move src\platform\Win32Window.cpp deprecated\opengl\legacy_src\platform\
move src\platform\Win32Window.h deprecated\opengl\legacy_src\platform\
move src\ui\DashboardUI.cpp deprecated\opengl\legacy_src\ui\
move src\ui\DashboardUI.h deprecated\opengl\legacy_src\ui\
move src\utils\Screenshot.cpp deprecated\opengl\legacy_src\utils\
move src\utils\Screenshot.h deprecated\opengl\legacy_src\utils\
move src\utils\MeshDebugger.h deprecated\opengl\legacy_src\utils\

echo Done! Legacy OpenGL sources are under deprecated\opengl\legacy_src.
```

## CMakeLists.txt Status

CMakeLists.txt is already DX12-only and links the required DirectX and Win32 libraries.

## Priority Order

1. **Agent 1** - Complete the migration (builds on current DX12 work)
2. **Test** - Verify DX12 version runs
3. **Delete** - Remove deprecated folder once confirmed working
4. **Update CMake** - Point to new files

## Files Safe to Delete Now

These GLSL shaders are already deleted:
- ~~shaders/vertex.glsl~~
- ~~shaders/fragment.glsl~~
- ~~shaders/creature_vertex.glsl~~
- ~~shaders/creature_fragment.glsl~~
- ~~shaders/tree_vertex.glsl~~
- ~~shaders/tree_fragment.glsl~~
- ~~shaders/nametag_vertex.glsl~~
- ~~shaders/nametag_fragment.glsl~~
- ~~shaders/shadow_vertex.glsl~~
- ~~shaders/shadow_fragment.glsl~~

These OpenGL ImGui backends are deleted:
- ~~external/imgui/imgui_impl_opengl3.*~~
- ~~external/imgui/imgui_impl_glfw.*~~
