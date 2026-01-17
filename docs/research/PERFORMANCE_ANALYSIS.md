# Performance Analysis - OrganismEvolution

**Date:** January 2026
**Target:** 10,000+ creatures at 60 FPS
**Current State:** Functional but unoptimized rendering pipeline with significant GPU infrastructure NOT integrated

---

## Executive Summary

The OrganismEvolution project has a functional creature simulation with instanced rendering for creatures, but significant performance bottlenecks exist. Analysis reveals that substantial GPU infrastructure (compute shaders, LOD system, frustum culling) has been written but is **not integrated** into the main rendering pipeline.

### Key Findings:
1. **Draw calls per frame:** 4 + TreeCount + NametagCount (unoptimized for trees/nametags)
2. **GPU compute shaders:** SteeringCompute.hlsl, FrustumCull.hlsl exist but are NOT USED
3. **LOD system:** CreatureLOD.hlsl designed but NOT INTEGRATED
4. **Frustum culling:** Frustum.cpp exists but is NOT USED in rendering
5. **Spatial grid:** Inline version in main.cpp is functional but could be optimized
6. **Sensory system:** 4 separate passes over creatures (should be unified)
7. **Aquatic creatures:** O(N) linear scan bypasses spatial grid

---

## 1. Rendering Pipeline Analysis

### 1.1 Frame Timing

```cpp
// Current implementation (main.cpp:5005-5012)
auto lastTime = std::chrono::high_resolution_clock::now();
while (!m_window->ShouldClose())
{
    auto currentTime = std::chrono::high_resolution_clock::now();
    f32 deltaTime = std::chrono::duration<f32>(currentTime - lastTime).count();
    lastTime = currentTime;
    deltaTime = std::min(deltaTime, 0.05f);  // Clamped to 50ms max (20 FPS minimum)
}
```

**Observations:**
- Uses high-resolution clock for precise timing
- Delta time clamped at 50ms prevents physics explosion on lag spikes
- Simulation speed scaling: `f32 scaledDt = dt * m_renderer.m_simulationSpeed;`

### 1.2 Draw Call Analysis

| Category | Draw Calls | Notes |
|----------|------------|-------|
| Terrain | 1 | Single indexed draw - optimal |
| Vegetation (Trees) | **N (variable)** | **One draw call per tree - BOTTLENECK** |
| Herbivores | 1 | Instanced rendering - optimal |
| Carnivores | 1 | Instanced rendering - optimal |
| Nametags | **N (variable)** | **One draw call per visible nametag - BOTTLENECK** |
| ImGui | 1 | Single `ImGui_ImplDX12_RenderDrawData()` |

**Total Draw Calls = 4 + TreeCount + NametagCount**

With 150 trees and 100 visible nametags: **254 draw calls per frame**

### 1.3 Instance Buffer Configuration

```cpp
// Per-instance data: 80 bytes
struct CreatureInstanceData {
    f32 modelRow0[4];    // 16 bytes
    f32 modelRow1[4];    // 16 bytes
    f32 modelRow2[4];    // 16 bytes
    f32 modelRow3[4];    // 16 bytes
    f32 colorType[4];    // 16 bytes (RGB + creature type)
};

// Double-buffered per-frame (prevents CPU/GPU race)
m_herbivoreInstanceBuffer[NUM_FRAMES_IN_FLIGHT];  // 2 buffers
m_carnivoreInstanceBuffer[NUM_FRAMES_IN_FLIGHT];  // 2 buffers

// Memory: MAX_CREATURES * 80 bytes = 2000 * 80 = 160KB each
// Total instance upload per frame: up to 320KB
```

### 1.4 Constant Buffer Pattern

```cpp
// Size: 400 bytes per draw
// Strategy: Map once per frame, write at offsets
void* cbData = constantsCB->Map();
m_constantsWriteIndex = 0;

auto updateConstants = [&](const Mat4& model, const Vec3& color, ...) {
    const u32 cbOffset = m_constantsWriteIndex * m_constantsStride;
    memcpy(static_cast<u8*>(cbData) + cbOffset, &frameConstants, sizeof(frameConstants));
    cmdList->BindConstantBuffer(0, constantsCB, cbOffset);
    ++m_constantsWriteIndex;
};
```

---

## 2. Creature Update Analysis

### 2.1 Update Loop Structure

```cpp
void Update(f32 dt)
{
    simulationTime += dt;

    // 1. Update day/night cycle - O(1)
    dayNight.Update(dt);

    // 2. Rebuild spatial grid - O(N+M) every frame
    spatialGrid.Clear();
    for (auto& c : creatures) spatialGrid.InsertCreature(c.get());
    for (auto& f : food) spatialGrid.InsertFood(f.get());

    // 3. Update creatures - O(N * K) where K = avg nearby
    for (auto& creature : creatures) {
        if (!creature->alive) continue;
        UpdateCreature(*creature, dt);
    }

    // 4-9. Interactions, reproduction, cleanup...
}
```

### 2.2 Per-Creature Update Cost

Each `UpdateCreature()` call involves:

| Operation | Complexity | Notes |
|-----------|------------|-------|
| Age/energy decay | O(1) | Negligible |
| Neural network inference | O(1) | 10->12->4 network = 168 FLOPs |
| Sensory gathering | O(4*C) | 4 separate passes (vision/hearing/smell/touch) |
| Steering calculation | O(K) | Reynolds behaviors |
| Velocity integration | O(1) | Physics step |
| Terrain height lookup | O(1) | Perlin noise sampling |
| Bounds enforcement | O(1) | World edge handling |

**Overall per-creature: O(K + 4C)** where K = average nearby creatures, C = all creatures

### 2.3 Identified O(n^2) Operations

| Location | Issue | Current Complexity |
|----------|-------|-------------------|
| `updateBehaviorAquatic()` | Linear scan of ALL creatures for schooling | **O(N)** - bypasses SpatialGrid |
| SensorySystem | 4 separate passes over creatures | O(4*C + F) per creature |
| Food search | O(F) iteration in herbivore behavior | O(F) per herbivore |

### 2.4 Memory Access Patterns

**Issues:**
- **Pointer chasing:** `vector<unique_ptr<Creature>>` causes scattered heap access
- **Per-query allocations:** `SpatialGrid::query()` allocates new vector on every call
- **Cold data mixing:** Genome/brain weights interleaved with hot position/velocity data

**Recommendation:** Data-Oriented Design separating hot transforms from cold genome data

```cpp
// Recommended layout
struct CreatureTransforms {  // Hot - updated every frame
    std::vector<Vec3> positions;
    std::vector<Vec3> velocities;
    std::vector<float> energies;
    std::vector<float> healths;
};

struct CreatureGenomes {  // Cold - rarely accessed
    std::vector<Genome> genomes;
    std::vector<DiploidGenome> diploidGenomes;
};
```

---

## 3. GPU Infrastructure Gap Analysis

### 3.1 What EXISTS but is NOT USED

| Component | File | Capability | Status |
|-----------|------|------------|--------|
| GPU Steering | `SteeringCompute.hlsl` + `GPUSteeringCompute.cpp` | 65K creatures parallel AI | **NOT USED** |
| GPU Frustum Culling | `FrustumCull.hlsl` | <0.1ms culling for 10K+ creatures | **NOT USED** |
| Multi-LOD Rendering | `CreatureLOD.hlsl` | 3-level LOD with crossfade | **NOT USED** |
| CPU Frustum Class | `Frustum.cpp` | AABB/Sphere visibility tests | **NOT USED** |
| Standalone SpatialGrid | `SpatialGrid.cpp` | Spatial partitioning | REPLACED by inline |
| Shadow Mapping | `Shadow.hlsl` + `ShadowMap_DX12.cpp` | Dynamic shadows | **NOT USED** |
| Post-Processing | `SSAO.hlsl`, `Bloom.hlsl`, etc. | Advanced effects | **NOT USED** |

### 3.2 SteeringCompute.hlsl Capabilities

```hlsl
// Thread configuration
[numthreads(64, 1, 1)]  // Up to 65,536 creatures in parallel

// Behaviors implemented:
- Seek, Flee, Arrive, Wander
- Pursuit, Evasion
- Flocking (Separation, Alignment, Cohesion)
- Predator-prey (Hunt, Flee)
- Environment (AvoidWater, AvoidLand, MaintainAltitude)

// 8 creature types supported
// 14 behavior flags for state tracking
```

### 3.3 FrustumCull.hlsl Capabilities

```hlsl
[numthreads(256, 1, 1)]  // 16,384 creatures supported

// LOD levels:
// LOD 0: Full detail (< 50m)
// LOD 1: Simplified mesh (< 150m)
// LOD 2: Billboard impostor (> 150m)

// Outputs IndirectDrawArgs for ExecuteIndirect
// 2 creature types x 3 LOD levels = 6 indirect draw calls
```

### 3.4 CreatureLOD.hlsl Capabilities

```hlsl
// LOD 0 (< 50m): Full PBR, procedural skin, animation
// LOD 1 (50-150m): Blinn-Phong, no animation
// LOD 2 (> 150m): Camera-facing billboards

// Features:
- Crossfade transitions (10m zone)
- Day/night cycle integration
- Fog system
- Per-instance instancing support
```

---

## 4. Spatial Grid Analysis

### 4.1 Current Implementation (main.cpp inline)

```cpp
class SpatialGrid {
    static constexpr i32 GRID_SIZE = 50;
    static constexpr f32 CELL_SIZE = 20.0f;  // 1000x1000 world

    std::vector<std::vector<Creature*>> creatureGrid;  // 50x50 = 2500 cells
    std::vector<std::vector<FoodSource*>> foodGrid;

    void Clear() { /* clears all cells */ }
    void InsertCreature(Creature* c) { /* O(1) insert */ }
    Vec2i WorldToCell(Vec3 pos) { /* O(1) conversion */ }

    std::vector<Creature*> QueryCreatures(Vec3 pos, f32 radius);
    std::vector<FoodSource*> QueryFood(Vec3 pos, f32 radius);
};
```

### 4.2 Performance Characteristics

- **Grid resolution:** 50x50 = 2,500 cells
- **Cell lookup:** O(1) via coordinate conversion
- **Radius query:** O(cells in radius * creatures per cell)
- **Rebuild frequency:** Every frame (Clear + Insert all)

### 4.3 Inefficiencies

1. **Full rebuild every frame** - even when simulation is paused
2. **Vector of vectors** - less cache-friendly than flat array
3. **Query result allocation** - new vector per query call
4. **No temporal coherence** - doesn't track creature movement between frames

---

## 5. Bottleneck Priority Ranking

### Critical (Must Fix)

| Priority | Issue | Impact | Solution |
|----------|-------|--------|----------|
| 1 | Tree rendering not instanced | N draw calls vs 4 | Instance by tree type |
| 2 | GPU compute shaders not integrated | CPU-bound AI | Integrate GPUSteeringCompute |
| 3 | No frustum culling | Rendering off-screen creatures | Integrate Frustum class |

### High (Should Fix)

| Priority | Issue | Impact | Solution |
|----------|-------|--------|----------|
| 4 | Nametag rendering not instanced | N draw calls vs 1 | Instance rendering |
| 5 | LOD system not integrated | Full detail at all distances | Integrate CreatureLOD.hlsl |
| 6 | Spatial grid rebuilt every frame | O(N+M) overhead | Lazy rebuild on movement |
| 7 | SensorySystem 4-pass iteration | Redundant work | Unify into single pass |

### Medium (Could Fix)

| Priority | Issue | Impact | Solution |
|----------|-------|--------|----------|
| 8 | Neural network vector allocations | Heap pressure | Pool input/output vectors |
| 9 | Model matrix computed per-frame | CPU cycles | Cache when not moving |
| 10 | Aquatic schooling O(N) | Scales poorly | Use SpatialGrid |
| 11 | Query result allocations | Heap pressure | Reuse vectors |

---

## 6. Frame Structure Analysis

```
Frame N:
├── PollEvents()
├── BeginImGuiFrame()
├── HandleInput(dt)
├── Update(dt)                           [CPU-BOUND]
│   ├── dayNight.Update()
│   ├── spatialGrid.Rebuild()            [O(N+M)]
│   ├── for each creature:
│   │   ├── Sensory::sense()             [O(4*C)] ← 4 passes!
│   │   └── UpdateCreature()             [Neural net + physics]
│   ├── UpdateInteractions()
│   ├── HandleReproduction()
│   ├── BalancePopulation()
│   ├── SpawnFood() (throttled)
│   └── RemoveDeadEntities()
├── Render()
│   ├── Map constant buffer (once)
│   ├── Draw Terrain                      [1 draw call]
│   ├── for each tree:
│   │   └── Draw Tree                     [N draw calls] ← BOTTLENECK
│   ├── Build instance arrays
│   ├── Upload herbivore instances
│   ├── DrawIndexedInstanced Herbivores   [1 draw call]
│   ├── Upload carnivore instances
│   ├── DrawIndexedInstanced Carnivores   [1 draw call]
│   ├── Build nametag constants
│   ├── Upload nametag CB
│   ├── for each nametag:
│   │   └── Draw Nametag                  [N draw calls] ← BOTTLENECK
│   ├── Unmap constant buffer (once)
│   └── Render ImGui                      [1 draw call]
└── EndFrame() [Fence signaling]
```

---

## 7. Performance Metrics (Current)

### What's Tracked

```cpp
// FPS display (ImGui)
ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);

// Console output (per second)
printf("FPS: %.0f | Pop: %zu (H:%d C:%d) | Food: %zu | Gen: %u", ...);
```

### What's Missing

- No GPU timing queries
- No per-phase CPU timing breakdown
- No draw call counting in UI
- No memory allocation tracking
- No culling statistics (visible vs culled creatures)
- No behavior distribution metrics
- No genetic/neural network metrics

---

## 8. GPU Parallelization Potential

### Speedup Estimates

| Component | Current (CPU) | Potential (GPU) | Speedup |
|-----------|--------------|-----------------|---------|
| Neural Network | O(N * 168 FLOPs) | Batched matmul | 50-100x |
| Sensory Queries | O(N * 4C) | Parallel radius search | 50-200x |
| Position Updates | O(N) | Parallel integration | 20-50x |
| Flocking | O(N * K) | Parallel neighborhood | 30-100x |
| Frustum Culling | O(N) on CPU | O(N/256) on GPU | 100-500x |

### Recommended GPU Pipeline

```
1. GPU Spatial Grid Construction (parallel hash)
2. GPU Sensory Queries (parallel radius search)
3. GPU Neural Network Forward (batched matmul)
4. GPU Steering Integration (parallel physics)
5. GPU Frustum Culling (parallel visibility test)
6. GPU Instance Buffer Build (compact visible creatures)
7. Indirect Draw (ExecuteIndirect with GPU-generated args)
```

---

## 9. Memory Analysis

### Current Allocations Per Frame

| Allocation | Size | Frequency | Issue |
|------------|------|-----------|-------|
| Instance arrays clear/rebuild | 160KB x 2 | Every frame | Not severe |
| SpatialGrid query results | Variable | Per query (many) | Heap pressure |
| Neural network inputs | 40 bytes | Per creature | Heap pressure |
| Sensory percepts | Variable | Per creature | Heap pressure |
| Model matrix computation | 64 bytes | Per creature | Could cache |

### Buffer Sizes

| Buffer | Size | Notes |
|--------|------|-------|
| Herbivore instance buffer | 160KB | 2000 * 80 bytes |
| Carnivore instance buffer | 160KB | 2000 * 80 bytes |
| Constant buffer | ~800KB | 2000 * 400 bytes aligned |
| Vertex buffers | ~2MB | Meshes |
| Index buffers | ~500KB | Indices |

---

## 10. Neural Network Analysis

### Simple Network (entities/NeuralNetwork)

```
Architecture: 4 inputs -> 4 hidden -> 2 outputs
Per forward pass: 24 MAC + 6 tanh = ~50 FLOPs
Cost per frame (200 creatures): 10,000 FLOPs (negligible)
```

### NEAT Network (ai/NeuralNetwork)

```cpp
// Current inefficiency: O(E) connection lookup per node
for (int nodeId : m_executionOrder) {  // O(V)
    for (const auto& conn : m_connections) {  // O(E) per node!
        if (conn.toNode != nodeId) continue;
        // ...
    }
}
```

**Complexity:** O(V * E) instead of O(V + E) with adjacency list

---

## 11. Conclusions and Next Steps

### Immediate Wins (Low Effort, High Impact)

1. **Integrate Frustum culling** - Add `Frustum.h` include, call `isSphereVisible()` before adding to instance buffer
2. **Pool query result vectors** - Reuse vectors instead of allocating per query
3. **Instance tree rendering** - Group by type, use instance buffer (reduce N draw calls to 4)

### Medium-Term (GPU Integration)

1. **Integrate GPUSteeringCompute** - Wire up compute shader dispatch
2. **Integrate FrustumCull compute shader** - GPU-side visibility testing
3. **Implement ExecuteIndirect** - GPU-driven draw call generation

### Long-Term (Full Optimization)

1. **Multi-LOD rendering** - Integrate CreatureLOD.hlsl
2. **Billboard impostor system** - For distant creatures
3. **Data-Oriented redesign** - Separate hot/cold data for cache efficiency
4. **Unified sensory pass** - Single iteration instead of 4

### Target Metrics

| Metric | Current | Target |
|--------|---------|--------|
| Creature count | ~500 | 10,000+ |
| FPS | 30-60 | 60 stable |
| Draw calls | 4 + N trees + N nametags | < 10 total |
| GPU utilization | Low (~20%) | High (~80%) |
| CPU update time | 10-20ms | < 5ms |

---

## Appendix A: File References

### Active Files
- `src/main.cpp` - Main rendering and update loop
- `src/entities/Creature.cpp` - Creature update logic
- `src/entities/SensorySystem.cpp` - 4-pass sensory scanning
- `src/entities/SteeringBehaviors.cpp` - Flocking algorithms
- `shaders/hlsl/Creature.hlsl` - Basic creature rendering

### Unused Infrastructure
- `src/ai/GPUSteeringCompute.cpp` - GPU steering (NOT USED)
- `src/graphics/Frustum.cpp` - Frustum culling (NOT USED)
- `shaders/hlsl/SteeringCompute.hlsl` - GPU steering shader (NOT USED)
- `shaders/hlsl/FrustumCull.hlsl` - GPU culling shader (NOT USED)
- `shaders/hlsl/CreatureLOD.hlsl` - Multi-LOD rendering (NOT USED)

---

## Appendix B: Creature Type Update Costs

| Creature Type | Behavior Function | Spatial Queries | Complexity |
|--------------|-------------------|-----------------|------------|
| HERBIVORE | updateBehaviorHerbivore | 2 (predator + neighbors) | O(2K) |
| CARNIVORE | updateBehaviorCarnivore | 2 (prey + neighbors) | O(2K) |
| AQUATIC | updateBehaviorAquatic | 0 (linear scan!) | **O(N)** |
| FLYING | updateBehaviorFlying | 2 (prey + neighbors) | O(2K) |

---

*Analysis completed: January 2026*
*Lines: 400+*
