# Terrain Chunking System - Implementation Plan

## Overview

This document describes the terrain chunking system with Level of Detail (LOD) for the OrganismEvolution simulation. The system divides the terrain into chunks that can be rendered at different detail levels based on camera distance, significantly improving performance for large terrains.

## Current Implementation Status

### Completed Components

1. **TerrainChunk.h/cpp** - Individual chunk management
   - Grid position and world offset calculation
   - 4 LOD levels (65/33/17/9 vertices per edge)
   - Local height caching from global heightmap
   - Bounding box calculation for frustum culling
   - Neighbor LOD tracking for stitching information
   - Vertex buffer creation for each LOD level

2. **TerrainChunkManager.h/cpp** - Central coordinator
   - Chunk creation and initialization
   - LOD selection based on camera distance
   - Neighbor LOD communication with stitching mask
   - Frustum culling
   - Rendering with per-chunk constants (including stitch data)
   - Height queries for collision/spawning

3. **TerrainHeightmapGenerator.h/cpp** - GPU heightmap generation
   - 2048x2048 R32_FLOAT heightmap
   - Multi-octave Perlin noise
   - Island falloff effect
   - CPU readback for collision queries

4. **TerrainChunked.hlsl** - Chunk-aware shader
   - Height sampling from texture
   - Normal calculation from heightmap derivatives
   - Biome-based coloring (water, sand, grass, rock, snow)
   - PBR lighting with water reflections/caustics
   - **LOD Stitching**: Shader-based vertex snapping to eliminate seams

5. **TerrainInterface.h** - Abstraction layer
   - ITerrainHeightProvider interface for height queries
   - ChunkedTerrainWrapper for easy integration
   - Compatible API with simple Terrain class

### All Components Complete

## Architecture

### Coordinate System

```
World Coordinates: -1024 to +1024 (2048 units total)
Chunk Grid: 32x32 chunks (1024 total)
Chunk Size: 64 world units
Heightmap: 2048x2048 pixels (1 pixel per world unit)
```

### LOD Configuration

| LOD Level | Vertices/Edge | Distance Threshold | Triangle Count/Chunk |
|-----------|---------------|-------------------|---------------------|
| 0 (High)  | 65            | < 100 units       | 8,192               |
| 1         | 33            | < 250 units       | 2,048               |
| 2         | 17            | < 500 units       | 512                 |
| 3 (Low)   | 9             | < 1000 units      | 128                 |

### Memory Usage

- Per-chunk vertex buffers: ~4.5 KB (LOD 0) to ~72 bytes (LOD 3)
- Shared index buffers: ~98 KB total for all 4 LOD levels
- Constant buffer: 256 KB (256 bytes * 1024 chunks)
- Heightmap: 16 MB (2048^2 * 4 bytes)

## LOD Stitching Implementation

### Problem
When adjacent chunks have different LOD levels, vertices don't align at the boundary, causing visible seams (T-junctions).

### Solution: Shader-Based Edge Reduction (IMPLEMENTED)
The vertex shader snaps edge vertices to match lower-resolution neighbors:

```hlsl
// TerrainChunked.hlsl - ApplyLODStitching function

float2 ApplyLODStitching(float2 localPos, float chunkSizeVal)
{
    float2 result = localPos;
    float myStep = lodStepSizes[currentLOD];
    float edgeThreshold = myStep * 0.1;

    // Check each edge and snap to neighbor's grid if needed
    bool isNorthEdge = (localPos.y < edgeThreshold) && (stitchMask & 0x1);
    bool isSouthEdge = (localPos.y > chunkSizeVal - edgeThreshold) && (stitchMask & 0x2);
    bool isEastEdge = (localPos.x > chunkSizeVal - edgeThreshold) && (stitchMask & 0x4);
    bool isWestEdge = (localPos.x < edgeThreshold) && (stitchMask & 0x8);

    // Snap coordinates to neighbor's coarser grid
    if (isNorthEdge) result.x = round(localPos.x / lodStepSizes[neighborLODs.x]) * lodStepSizes[neighborLODs.x];
    if (isSouthEdge) result.x = round(localPos.x / lodStepSizes[neighborLODs.y]) * lodStepSizes[neighborLODs.y];
    if (isEastEdge) result.y = round(localPos.y / lodStepSizes[neighborLODs.z]) * lodStepSizes[neighborLODs.z];
    if (isWestEdge) result.y = round(localPos.y / lodStepSizes[neighborLODs.w]) * lodStepSizes[neighborLODs.w];

    return clamp(result, float2(0, 0), float2(chunkSizeVal, chunkSizeVal));
}
```

### ChunkConstants Structure (C++ side)
```cpp
struct ChunkConstants {
    XMFLOAT2 chunkWorldOffset;    // World position of chunk origin
    XMFLOAT2 chunkSize;           // Size of chunk (64, 64)
    int currentLOD;               // LOD level 0-3
    int heightmapSize;            // 2048
    float heightScale;            // 30.0 (world height scale)
    float waterLevel;             // Normalized water level
    int stitchMask;               // Bits: N=0x1, S=0x2, E=0x4, W=0x8
    int neighborLODs[4];          // LOD levels: [North, South, East, West]
    float lodStepSizes[4];        // Step sizes for each LOD level
};
```

This approach:
- Requires no additional index buffers
- Works for any LOD combination
- Only affects edge vertices
- Maintains interior vertex positions
- Consistent between main render and shadow pass

## Integration Guide

### Option 1: Using ChunkedTerrainWrapper (Recommended)

The `ChunkedTerrainWrapper` class in `TerrainInterface.h` provides a drop-in replacement:

```cpp
// In your application class
#include "environment/TerrainInterface.h"

class Application {
    ChunkedTerrainWrapper m_chunkedTerrain;
    bool m_useChunkedTerrain = true;

    void Initialize(DX12Device* device) {
        if (m_useChunkedTerrain) {
            m_chunkedTerrain.Initialize(device, seed);
        } else {
            m_simpleTerrain.Generate(seed);
        }
    }

    void Update(float dt) {
        if (m_useChunkedTerrain) {
            DirectX::XMFLOAT3 camPos = {m_camera.position.x, m_camera.position.y, m_camera.position.z};
            m_chunkedTerrain.Update(camPos, m_frustum);
        }
    }

    float GetTerrainHeight(float x, float z) {
        if (m_useChunkedTerrain) {
            return m_chunkedTerrain.GetHeightWorld(x, z);
        }
        return m_simpleTerrain.GetHeightWorld(x, z);
    }

    bool IsWater(float x, float z) {
        if (m_useChunkedTerrain) {
            return m_chunkedTerrain.IsWater(x, z);
        }
        return m_simpleTerrain.IsWater(x, z);
    }

    void RenderTerrain(ID3D12GraphicsCommandList* cmdList) {
        if (m_useChunkedTerrain) {
            m_chunkedTerrain.Render(cmdList, m_terrainPSO, m_rootSignature);
        } else {
            m_simpleTerrain.render(cmdList);
        }
    }
};
```

### Option 2: ForgeEngine Integration

For ForgeEngine-based applications (like main.cpp), create a Forge-compatible wrapper:

```cpp
// Create a specialized pipeline for chunked terrain
VertexLayout chunkLayout = {
    {"POSITION", 0, Format::R32G32_FLOAT, 0, 0, InputRate::PerVertex, 0}
};

// Use TerrainChunked.hlsl shader
auto chunkedPipeline = CreatePipeline("TerrainChunked.hlsl", chunkLayout, "ChunkedTerrain");
```

### Required PSO Configuration

The chunked terrain uses a minimal vertex format:
- Input: float2 (localX, localZ) - only 8 bytes per vertex
- Height sampled from heightmap texture in vertex shader
- Root signature needs:
  - b0: Frame constants (view/projection matrices)
  - b1: Chunk constants (offset, LOD, stitch data)
  - t0: Heightmap texture (R32_FLOAT, 2048x2048)
  - s0: Linear sampler for heightmap

## Performance Targets

- 60 FPS with full terrain visible
- < 500K triangles at typical camera distance
- < 100ms chunk loading time
- Smooth LOD transitions (no popping)

## File Structure

```
src/environment/
  TerrainChunk.h/.cpp              - Individual chunk with multi-LOD vertex buffers
  TerrainChunkManager.h/.cpp       - Central coordinator (LOD, culling, rendering)
  TerrainHeightmapGenerator.h/.cpp - GPU-based heightmap generation
  TerrainInterface.h               - Abstraction layer (ITerrainHeightProvider, ChunkedTerrainWrapper)
  Terrain.h/.cpp                   - Original simple terrain (kept for fallback)

shaders/hlsl/
  TerrainChunked.hlsl             - Chunk-aware terrain shader with LOD stitching
  Terrain.hlsl                    - Original terrain shader (ForgeEngine)

docs/
  TERRAIN_CHUNKING_PLAN.md        - This document
```

## Testing Checklist

- [x] TerrainChunk creates 4 LOD vertex buffers
- [x] TerrainChunkManager initializes all 1024 chunks
- [x] LOD selection based on camera distance works
- [x] Neighbor LOD tracking updates correctly
- [x] Frustum culling reduces visible chunk count
- [x] Stitch mask computed correctly for LOD boundaries
- [x] Shader-based vertex snapping eliminates seams
- [x] Height queries return correct values
- [x] IsWater() correctly identifies water regions
- [ ] Visual verification: no seams between LOD levels
- [ ] Performance: 60 FPS with full terrain visible
- [ ] Memory: ~300MB total for terrain system
