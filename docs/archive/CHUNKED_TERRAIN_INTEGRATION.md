# Chunked Terrain System Integration Guide

This document describes how to integrate the new GPU-accelerated chunked terrain system with the existing OrganismEvolution codebase.

## Overview

The new terrain system provides:
- **2048x2048 world size** (vs 450x450 in current implementation)
- **1024 chunks** (32x32 grid of 64x64 unit chunks)
- **4 LOD levels** for distance-based detail optimization
- **GPU compute shader** for heightmap generation
- **Frustum culling** per chunk
- **LOD stitching** to prevent seams between different detail levels

## New Files Created

### C++ Source Files
- `src/environment/TerrainChunkManager.h/.cpp` - Central coordinator
- `src/environment/TerrainChunk.h/.cpp` - Individual chunk with LOD buffers
- `src/environment/TerrainHeightmapGenerator.h/.cpp` - GPU compute pipeline

### HLSL Shaders
- `shaders/hlsl/HeightmapGenerate.hlsl` - Compute shader for procedural heightmap
- `shaders/hlsl/TerrainChunked.hlsl` - Vertex/pixel shader for chunked terrain rendering

## Architecture Differences

### Current System (main.cpp inline Terrain class)
- Uses Forge Engine RHI abstraction (`IDevice`, `IBuffer`, etc.)
- Single mesh with 300x300 vertices (90,000 vertices)
- CPU-generated Perlin noise heightmap
- No LOD - renders entire terrain at full detail

### New Chunked System
- Uses raw DirectX 12 APIs (`ID3D12Device`, `ID3D12Resource`, etc.)
- 1024 chunks with 4 LOD levels each
- GPU compute shader generates 2048x2048 heightmap
- Distance-based LOD selection with frustum culling

## Integration Options

### Option 1: Wrap Raw DX12 with Forge RHI (Recommended)

Modify TerrainChunkManager to use Forge RHI instead of raw DX12:

```cpp
// Instead of:
ComPtr<ID3D12Resource> m_vertexBuffer;
HRESULT hr = d3dDevice->CreateCommittedResource(...);

// Use:
SharedPtr<IBuffer> m_vertexBuffer;
BufferDesc desc;
desc.size = ...;
desc.usage = BufferUsage::Vertex;
m_vertexBuffer = device->CreateBuffer(desc);
```

This requires adding Forge RHI includes to the terrain files:
```cpp
#include "RHI/RHI.h"
using namespace Forge::RHI;
```

### Option 2: Direct DX12 Access from Forge Device

If Forge exposes the underlying DX12 device:
```cpp
ID3D12Device* d3dDevice = static_cast<ID3D12Device*>(forgeDevice->GetNativeDevice());
ID3D12GraphicsCommandList* cmdList = static_cast<ID3D12GraphicsCommandList*>(cmdBuffer->GetNativeCommandList());
```

### Option 3: Replace main.cpp Terrain Class

Replace the inline `Terrain` class in main.cpp (lines 482-620) with calls to TerrainChunkManager:

```cpp
// In SimulationWorld struct, replace:
Terrain terrain;

// With:
std::unique_ptr<TerrainChunkManager> terrainManager;

// In initialization:
terrainManager = std::make_unique<TerrainChunkManager>();
terrainManager->Initialize(dx12Device, seed);

// In rendering:
terrainManager->Update(cameraPos, frustum);
terrainManager->Render(cmdList, terrainPSO, rootSignature);

// For height queries:
float h = terrainManager->GetHeight(worldX, worldZ);
bool inWater = terrainManager->IsWater(worldX, worldZ);
```

## Integration Steps

### Step 1: Enable Terrain Sources in CMakeLists.txt

```cmake
set(SOURCES
    ${MAIN_SOURCES}
    ${FORGE_ENGINE_SOURCES}
    ${IMGUI_SOURCES}
    ${TERRAIN_SOURCES}  # Uncomment this line
)
```

### Step 2: Add DX12Device Wrapper (if using Option 2)

Create a simple wrapper that provides raw DX12 access:

```cpp
// src/graphics/DX12DeviceWrapper.h
class DX12DeviceWrapper {
public:
    explicit DX12DeviceWrapper(Forge::RHI::IDevice* forgeDevice);

    ID3D12Device* GetDevice() const;
    ID3D12CommandQueue* GetCommandQueue() const;
    ID3D12GraphicsCommandList* GetCommandList() const;

private:
    Forge::RHI::IDevice* m_forgeDevice;
};
```

### Step 3: Update Shader Paths

The TerrainChunked.hlsl shader expects to be loaded from:
```
shaders/hlsl/TerrainChunked.hlsl
```

Update the renderer to load this shader for terrain rendering.

### Step 4: Create Root Signature for Chunked Terrain

The chunked terrain shader requires:
- `b0`: Frame constants (view, projection, lighting)
- `b1`: Chunk constants (world offset, LOD, height scale)
- `t0`: Heightmap texture (R32_FLOAT 2048x2048)
- `s0`: Linear clamp sampler

### Step 5: Integrate Height Queries

Replace all calls to `terrain.GetHeightWorld(x, z)` with `terrainManager->GetHeight(x, z)`:

```cpp
// Before:
float h = world.terrain.GetHeightWorld(x, z);
bool water = world.terrain.IsWater(x, z);

// After:
float h = world.terrainManager->GetHeight(x, z);
bool water = world.terrainManager->IsWater(x, z);
```

## Performance Expectations

| Metric | Current | Chunked System |
|--------|---------|----------------|
| World Size | 450x450 | 2048x2048 |
| Vertices | 90,000 | ~100,000 visible |
| Draw Calls | 1 | ~200-400 |
| LOD Support | No | 4 levels |
| Frustum Culling | No | Per-chunk |
| GPU Memory | ~3 MB | ~75 MB |

## Debugging

### Verify Heightmap Generation

The heightmap generator includes a CPU readback for debugging:
```cpp
if (terrainManager->GetHeightmapTexture()) {
    // Heightmap texture is valid
}

float testHeight = terrainManager->GetHeight(0.0f, 0.0f);
printf("Height at origin: %f\n", testHeight);
```

### Verify Chunk Visibility

```cpp
int visibleChunks = terrainManager->GetVisibleChunkCount();
int totalTriangles = terrainManager->GetTotalTriangles();
printf("Visible: %d chunks, %d triangles\n", visibleChunks, totalTriangles);
```

### Shader Debugging

Enable shader debug output in HeightmapGenerate.hlsl:
```hlsl
// Add debug output to verify generation
if (texCoord.x == 1024 && texCoord.y == 1024) {
    // Center of map - should be highest point (island center)
}
```

## Future Enhancements

1. **Texture Streaming** - Load high-res detail textures on demand
2. **Tessellation** - GPU tessellation for close-up terrain detail
3. **Async Generation** - Generate chunks in background thread
4. **Occlusion Culling** - Skip chunks hidden behind mountains
5. **Water System** - Separate water mesh with reflections/refractions
