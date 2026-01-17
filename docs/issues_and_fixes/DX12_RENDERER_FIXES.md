# DX12 Renderer Fixes

## Overview

This document summarizes recent stability and correctness fixes for the DirectX 12 renderer.
The focus is on per-draw constant buffer safety, resize correctness, depth state, and
uploading static geometry into GPU-friendly memory.

## Changes

1. Per-draw constant buffer offsets
   - Constant buffer writes now use a 256-byte aligned stride per draw.
   - Each draw gets a unique offset to avoid GPU reads seeing overwritten data.
   - Slot count is sized to cover terrain + all trees + instanced creatures.

2. Resize-safe rendering
   - Swapchain and depth buffer are recreated on window resize.
   - Viewport, scissor, and projection use current swapchain dimensions.
   - Resize events are wired through the window event callback.

3. Depth buffer state tracking
   - Depth buffer transitions to DepthWrite once per creation.
   - State is tracked in the renderer and reset on resize.

4. Static buffer uploads to default heap
   - Terrain, creature, and tree meshes are created in default heap.
   - Data is staged via upload buffers and copied with a one-time command list.
   - Per-frame instance buffers remain upload heap for CPU updates.

5. Swapchain RTVs on resize
   - Swapchain RTV descriptor slots are reserved on creation.
   - Resize recreates RTVs in the same reserved slots to avoid heap growth.

6. Shader hot-reload path normalization
   - File watcher paths are normalized to forward slashes.
   - Reload events now match watched files reliably on Windows.

## Files Updated

- `src/main.cpp`
  - Constant buffer ring + per-draw offsets
  - Dynamic viewport/projection and resize hook
  - Default-heap static mesh uploads with staging
  - Depth buffer state tracking
- `ForgeEngine/RHI/DX12/DX12Device.cpp`
  - Swapchain RTV recreation during resize
  - Reserved RTV slots for swapchain backbuffers
- `src/graphics/Shader_DX12.cpp`
  - Normalized file paths in the shader hot-reload watcher

## Notes

- Constant buffer stride is 256-byte aligned and based on `sizeof(Constants)`.
- Resize uses `IDevice::WaitIdle()` before rebuilding swapchain and depth resources.
- RTV descriptor slots for the swapchain are reused on resize to avoid leaks.
