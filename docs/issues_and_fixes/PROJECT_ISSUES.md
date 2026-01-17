# OrganismEvolution Project Review

Scope: Static repository scan only (no build/run, no external tooling beyond file reads).

## Critical
1) Build configs omit required source files referenced by `src/main.cpp`, so the default build will fail to link for terrain/grass/tree/rendering and camera/frustum symbols. Evidence: `CMakeLists.txt:147`, `src/main.cpp:1452`, `src/main.cpp:1462`, `src/main.cpp:1473`, `src/graphics/rendering/GrassRenderer_DX12.cpp`, `src/graphics/rendering/TreeRenderer_DX12.cpp`, `src/graphics/rendering/TerrainRenderer_DX12.cpp`, `src/graphics/Camera.cpp`, `src/graphics/Frustum.cpp`, `src/environment/Terrain.cpp`, `src/environment/GrassSystem.cpp`, `src/environment/VegetationManager.cpp`.
2) `CMakeLists_DX12.txt` only builds `main.cpp` + engine + ImGui, so `GPUSteeringCompute` and other non-header-only systems used by `main.cpp` are missing and the config will not link. Evidence: `CMakeLists_DX12.txt:111`, `src/ai/GPUSteeringCompute.cpp`.

## High
1) Save/Load writes placeholder values (generation, terrain seed, RNG state) and overwrites per-entity state (health, respawn timer, active flags), so reloaded worlds diverge from what was saved. Evidence: `src/main.cpp:2214`, `src/main.cpp:2216`, `src/main.cpp:2222`, `src/main.cpp:2256`, `src/main.cpp:2279`.
2) Load restores creature IDs but never updates ID counters in the pool/world, so new spawns can reuse existing IDs after a load. Evidence: `src/main.cpp:160`, `src/main.cpp:363`, `src/main.cpp:2352`.
3) Replay snapshots serialize genome and neural weight fields, but `BuildReplayFrame` fills only defaults and never captures actual brain/genome state, so replays are incomplete. Evidence: `src/core/ReplaySystem.h:44`, `src/main.cpp:2407`.
4) ForgeEngine DX12 RHI has stubbed implementations for compute pipelines and render passes, so callers get no-op or incomplete behavior. Evidence: `ForgeEngine/RHI/DX12/DX12Device.cpp:1071`, `ForgeEngine/RHI/DX12/DX12Device.cpp:1384`.

## Medium
1) Binary deserialization reads string/vector sizes directly from file without bounds checks or stream error handling, so corrupt data can trigger huge allocations or undefined state. Evidence: `src/core/Serializer.h:177`, `src/core/Serializer.h:188`.
2) Shader sources exist in two diverging trees (`Runtime/Shaders` and `shaders/hlsl`) while the build copies only `Runtime/Shaders`; edits to the other tree will not ship. Evidence: `CMakeLists.txt:227`, `Runtime/Shaders/Creature.hlsl`, `shaders/hlsl/Creature.hlsl`.
3) Two DX12 device abstractions live side by side (ForgeEngine RHI vs `src/graphics/DX12Device`), selected by compile flags; this increases config drift risk. Evidence: `ForgeEngine/RHI/DX12/DX12Device.cpp`, `src/graphics/DX12Device.h`, `src/ai/GPUSteeringCompute.h`.

## Low / Hygiene
1) Build artifacts are present and `build_dx12/` is not ignored; this bloats the repo and risks stale binaries. Evidence: `.gitignore:1`, `build_dx12/x64/Release/ALL_BUILD/ALL_BUILD.tlog/ALL_BUILD.lastbuildstate`.
2) `imgui.ini` is user-specific state and should not be tracked. Evidence: `imgui.ini`.
3) A file named `nul` exists at repo root; this breaks common tools on Windows (e.g., ripgrep errors) and is painful to handle in Git. Evidence: `nul`.
4) Tests exist on disk but no build/CI wiring is present (`enable_testing`, targets, or `add_subdirectory(tests)`), so they are not run. Evidence: `tests/test_spatial_grid.cpp`, `CMakeLists.txt`.

## Open Questions
- Which shader directory is the source of truth: `Runtime/Shaders` or `shaders/hlsl`?
- Is `CMakeLists_DX12.txt` still used, or can it be removed/updated to match the main build?
- Should Save/Replay be deterministic (capture RNG state, genome, neural weights), or is a lightweight snapshot acceptable?
