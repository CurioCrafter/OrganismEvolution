# Code Review Fixes (March 2026)

This document records the fixes applied after the full code review and why each change was made.

## GPU Steering Compute Path

**Files:** `src/main.cpp`, `src/ai/GPUSteeringCompute.h`, `src/ai/GPUSteeringCompute.cpp`

### What Changed
- Moved GPU steering dispatch onto a dedicated compute command list with an explicit fence for synchronization.
- Added a readback copy step from the UAV output buffer to a readback buffer before mapping.
- Added explicit resource state tracking and transitions for input/output buffers.
- Split timing into dispatch vs. readback to surface GPU work and CPU wait costs separately.

### Why
- The compute work was recorded on a command list that had not begun recording and was later reset by the render pass, so the GPU never saw the dispatch.
- Readback mapping previously happened without a GPU copy or synchronization, producing undefined data.
- Resource state transitions were inconsistent across frames, risking validation errors and undefined behavior.
- Separate timing makes it clear whether stalls are due to GPU compute or readback synchronization.

## Creature Mesh Uploads (DX12)

**Files:** `src/graphics/rendering/CreatureRenderer_DX12.h`, `src/graphics/rendering/CreatureRenderer_DX12.cpp`

### What Changed
- Added a small upload path using a staging buffer + command list + fence.
- Kept vertex/index buffers in the default heap and uploaded once at creation.

### Why
- Default-heap buffers cannot be mapped, so the previous map/unmap path left GPU buffers uninitialized.
- A staging upload preserves GPU read performance while fixing correctness.
- The synchronous fence wait happens only when a new mesh is created, avoiding per-frame overhead.

## Creature Pool Hot-Path Optimization

**Files:** `src/main.cpp`

### What Changed
- Track active pooled creatures via an index list for O(1) removal.
- Release dead creatures by iterating only active entries instead of the entire pool.
- Build active creature lists from active indices instead of scanning every pool slot.

### Why
- The previous implementation did linear scans and duplicate checks, which scaled poorly with large pools.
- The active-index approach removes O(n^2) behavior and reduces per-frame work in dense simulations.

## Food Respawn RNG

**Files:** `src/main.cpp`

### What Changed
- Replaced per-frame `std::random_device`/`std::mt19937` construction with a persistent RNG in `SimulationWorld`.

### Why
- Creating entropy sources every frame is expensive and may block on some platforms.
- A persistent RNG avoids unnecessary overhead while preserving randomness quality for gameplay.
