# Optimization Plan - OrganismEvolution

**Date:** January 2026
**Goal:** 10,000+ creatures at 60 FPS stable
**Current State:** ~500 creatures at 30-60 FPS with significant GPU infrastructure unused

---

## Executive Summary

This document outlines the comprehensive optimization strategy to achieve 10,000+ creatures at 60 FPS. The key insight from the performance analysis is that **substantial GPU infrastructure already exists** but is not integrated. The primary work is integration, not new development.

### Optimization Pillars

1. **GPU Instanced Rendering** - Reduce draw calls from N to constant
2. **GPU Compute AI** - Move steering/behavior to GPU
3. **Frustum Culling** - Skip rendering off-screen creatures
4. **LOD System** - Reduce vertex processing for distant creatures
5. **Spatial Partitioning** - Optimize neighbor queries

---

## Phase 1: Integrate CPU Frustum Culling (Quick Win)

### 1.1 Objective
Integrate the existing `Frustum` class to cull off-screen creatures before rendering.

### 1.2 Implementation

**File: src/main.cpp**

```cpp
// Add include at top
#include "graphics/Frustum.h"

// In render function, before building instance arrays:
Frustum frustum;
frustum.extractFromMatrix(m_camera.getViewProjectionMatrix());

// Stats tracking
u32 visibleCount = 0;
u32 culledCount = 0;

// In creature instance building loop:
for (const auto& creature : world.creatures) {
    if (!creature->alive) continue;

    // CULLING CHECK - skip if outside frustum
    float boundingRadius = creature->size * 2.5f;
    if (!frustum.isSphereVisible(creature->position, boundingRadius)) {
        culledCount++;
        continue;
    }
    visibleCount++;

    // Build instance data...
    CreatureInstanceData inst;
    inst.SetFromCreature(creature->GetModelMatrix(), creature->genome.color, (i32)creature->type);

    if (creature->type == CreatureType::HERBIVORE) {
        m_herbivoreInstances.push_back(inst);
    } else {
        m_carnivoreInstances.push_back(inst);
    }
}

// Update stats for ImGui
m_lastVisibleCount = visibleCount;
m_lastCulledCount = culledCount;
```

### 1.3 Expected Impact
- **Rendering cost reduction:** 30-70% depending on camera view
- **No simulation impact:** Creatures still update, just not rendered
- **Effort:** Low (1-2 hours)

---

## Phase 2: Instance Buffer Class

### 2.1 Design

Create a reusable instance buffer system for DX12.

**File: src/graphics/InstanceBuffer.h**

```cpp
#pragma once
#include "ForgeEngine/ForgeEngine.h"

template<typename T>
class InstanceBuffer {
public:
    InstanceBuffer() = default;
    ~InstanceBuffer() = default;

    void create(IDevice* device, size_t maxInstances, u32 frameCount = 2) {
        m_maxInstances = maxInstances;
        m_frameCount = frameCount;
        m_buffers.resize(frameCount);

        BufferDesc desc;
        desc.size = maxInstances * sizeof(T);
        desc.usage = BufferUsage::Vertex;
        desc.cpuAccess = true;  // Upload heap

        for (u32 i = 0; i < frameCount; ++i) {
            m_buffers[i] = device->CreateBuffer(desc);
        }

        m_staging.reserve(maxInstances);
    }

    void clear() {
        m_staging.clear();
    }

    bool add(const T& instance) {
        if (m_staging.size() >= m_maxInstances) return false;
        m_staging.push_back(instance);
        return true;
    }

    void upload(u32 frameIndex) {
        m_currentCount = m_staging.size();
        if (m_currentCount == 0) return;

        IBuffer* buffer = m_buffers[frameIndex % m_frameCount].get();
        void* mapped = buffer->Map();
        memcpy(mapped, m_staging.data(), m_currentCount * sizeof(T));
        buffer->Unmap();
    }

    void bind(IGraphicsCommandList* cmdList, u32 slot, u32 frameIndex) const {
        cmdList->BindVertexBuffer(slot, m_buffers[frameIndex % m_frameCount].get(),
                                   sizeof(T), 0);
    }

    size_t count() const { return m_currentCount; }
    size_t capacity() const { return m_maxInstances; }
    bool empty() const { return m_currentCount == 0; }

private:
    std::vector<std::unique_ptr<IBuffer>> m_buffers;
    std::vector<T> m_staging;
    size_t m_maxInstances = 0;
    size_t m_currentCount = 0;
    u32 m_frameCount = 2;
};
```

### 2.2 Tree Instance Buffer

**File: src/graphics/TreeInstanceBuffer.h**

```cpp
#pragma once
#include "InstanceBuffer.h"

struct TreeInstanceData {
    float modelRow0[4];  // 16 bytes
    float modelRow1[4];  // 16 bytes
    float modelRow2[4];  // 16 bytes
    float modelRow3[4];  // 16 bytes
    float color[4];      // 16 bytes (RGB + tree type)
};  // Total: 80 bytes

class TreeInstanceBuffer {
public:
    void create(IDevice* device, size_t maxTrees, u32 frameCount = 2) {
        // 4 tree types: oak, pine, birch, willow
        for (int i = 0; i < 4; ++i) {
            m_buffers[i].create(device, maxTrees / 4 + 100, frameCount);
        }
    }

    void clear() {
        for (auto& buf : m_buffers) buf.clear();
    }

    void addTree(const Tree& tree, const Mat4& model) {
        TreeInstanceData inst;
        // Fill model matrix rows
        for (int i = 0; i < 4; ++i) {
            inst.modelRow0[i] = model.m[0][i];
            inst.modelRow1[i] = model.m[1][i];
            inst.modelRow2[i] = model.m[2][i];
            inst.modelRow3[i] = model.m[3][i];
        }
        inst.color[0] = tree.color.x;
        inst.color[1] = tree.color.y;
        inst.color[2] = tree.color.z;
        inst.color[3] = (float)tree.type;

        m_buffers[tree.type].add(inst);
    }

    void upload(u32 frameIndex) {
        for (auto& buf : m_buffers) buf.upload(frameIndex);
    }

    void renderType(int type, IGraphicsCommandList* cmdList, u32 frameIndex,
                    IBuffer* vb, IBuffer* ib, u32 indexCount) {
        if (m_buffers[type].empty()) return;

        m_buffers[type].bind(cmdList, 1, frameIndex);
        cmdList->BindVertexBuffer(0, vb, sizeof(TreeVertex), 0);
        cmdList->BindIndexBuffer(ib, IndexFormat::UInt32, 0);
        cmdList->DrawIndexedInstanced(indexCount, (u32)m_buffers[type].count(), 0, 0, 0);
    }

private:
    InstanceBuffer<TreeInstanceData> m_buffers[4];  // One per tree type
};
```

### 2.3 Usage in main.cpp

```cpp
// Replace tree rendering loop with:
m_treeInstanceBuffer.clear();

for (const auto& tree : world.trees) {
    // Optional: frustum cull trees too
    if (!frustum.isSphereVisible(tree.position, tree.radius)) continue;

    Mat4 model = Mat4::Translation(tree.position) *
                 Mat4::RotationY(tree.rotation) *
                 Mat4::Scale(Vec3(tree.scale));

    m_treeInstanceBuffer.addTree(tree, model);
}

m_treeInstanceBuffer.upload(m_frameIndex);

// Render each tree type with single draw call
m_treeInstanceBuffer.renderType(0, cmdList, m_frameIndex, m_oakVB, m_oakIB, m_oakIndexCount);
m_treeInstanceBuffer.renderType(1, cmdList, m_frameIndex, m_pineVB, m_pineIB, m_pineIndexCount);
m_treeInstanceBuffer.renderType(2, cmdList, m_frameIndex, m_birchVB, m_birchIB, m_birchIndexCount);
m_treeInstanceBuffer.renderType(3, cmdList, m_frameIndex, m_willowVB, m_willowIB, m_willowIndexCount);
```

### 2.4 Expected Impact
- **Draw calls:** Reduced from N trees to 4 (one per type)
- **CPU overhead:** Reduced constant buffer updates
- **Effort:** Medium (2-4 hours)

---

## Phase 3: GPU Compute AI Integration

### 3.1 Objective
Integrate the existing `GPUSteeringCompute` class to move AI processing to GPU.

### 3.2 Current Infrastructure

The following already exist:
- `src/ai/GPUSteeringCompute.h/.cpp` - Full DX12 compute wrapper
- `shaders/hlsl/SteeringCompute.hlsl` - Complete steering compute shader

### 3.3 Integration Steps

**Step 1: Add GPUSteeringCompute to renderer class**

```cpp
// In main.cpp or Renderer class
#include "ai/GPUSteeringCompute.h"

class Renderer {
    // ...
    std::unique_ptr<GPUSteeringCompute> m_gpuSteering;
    bool m_useGPUSteering = true;  // Toggle for comparison
};
```

**Step 2: Initialize in setup**

```cpp
void initializeGPUSteering() {
    m_gpuSteering = std::make_unique<GPUSteeringCompute>();
    m_gpuSteering->create(m_device.get(),
                          "shaders/hlsl/SteeringCompute.cso",
                          65536,  // Max creatures
                          4096);  // Max food
}
```

**Step 3: Upload creature data each frame**

```cpp
void uploadCreaturesToGPU() {
    std::vector<CreatureInput> inputs;
    inputs.reserve(world.creatures.size());

    for (const auto& c : world.creatures) {
        if (!c->alive) continue;

        CreatureInput input;
        input.position = c->position;
        input.velocity = c->velocity;
        input.forward = c->forward;
        input.energy = c->energy;
        input.type = (int)c->type;
        input.targetId = c->targetId;
        // ... fill remaining fields

        inputs.push_back(input);
    }

    m_gpuSteering->uploadCreatures(inputs);
}

void uploadFoodToGPU() {
    std::vector<Vec4> foodPositions;
    foodPositions.reserve(world.food.size());

    for (const auto& f : world.food) {
        foodPositions.push_back(Vec4(f->position, f->energy));
    }

    m_gpuSteering->uploadFood(foodPositions);
}
```

**Step 4: Dispatch compute shader**

```cpp
void updateCreaturesGPU(f32 deltaTime) {
    if (!m_useGPUSteering) {
        // Fallback to CPU
        for (auto& c : world.creatures) {
            UpdateCreatureCPU(*c, deltaTime);
        }
        return;
    }

    // Upload data
    uploadCreaturesToGPU();
    uploadFoodToGPU();

    // Update constants
    m_gpuSteering->updateConstants(deltaTime, world.creatures.size());

    // Dispatch compute shader
    m_gpuSteering->dispatch(m_computeCommandList.get());

    // Read back results (or keep on GPU if rendering from same buffer)
    auto outputs = m_gpuSteering->readbackResults();

    // Apply results to creatures
    size_t i = 0;
    for (auto& c : world.creatures) {
        if (!c->alive) continue;
        if (i >= outputs.size()) break;

        c->steeringForce = outputs[i].steering;
        c->targetPosition = outputs[i].targetPosition;
        c->behaviorFlags = outputs[i].behaviorFlags;
        ++i;
    }
}
```

### 3.4 Hybrid Approach (Recommended)

For initial integration, use hybrid CPU+GPU:
- **GPU:** Steering force calculation (parallel, expensive)
- **CPU:** Position integration, health, energy (sequential, simple)

```cpp
void updateFrame(f32 dt) {
    // 1. GPU: Calculate steering forces for all creatures
    m_gpuSteering->dispatch(cmdList);

    // 2. CPU: While GPU works, do other updates
    updateDayNightCycle(dt);
    rebuildSpatialGrid();

    // 3. Sync: Wait for GPU, read results
    auto steeringOutputs = m_gpuSteering->readback();

    // 4. CPU: Apply steering, update positions, energy
    for (size_t i = 0; i < creatures.size(); ++i) {
        creatures[i]->velocity += steeringOutputs[i].steering * dt;
        creatures[i]->position += creatures[i]->velocity * dt;
        creatures[i]->energy -= baseConsumption * dt;
        // ... other simple updates
    }
}
```

### 3.5 Expected Impact
- **CPU load:** Reduced by 60-80%
- **Creature capacity:** 10x-50x increase
- **Latency:** One frame for GPU round-trip
- **Effort:** High (6-10 hours)

---

## Phase 4: LOD System Integration

### 4.1 Objective
Integrate the existing `CreatureLOD.hlsl` shader for distance-based detail reduction.

### 4.2 LOD Levels

| Level | Distance | Rendering |
|-------|----------|-----------|
| LOD 0 | < 50m | Full mesh, animation, PBR |
| LOD 1 | 50-150m | Simplified mesh, basic lighting |
| LOD 2 | > 150m | Billboard impostor |

### 4.3 Implementation

**Step 1: Add LOD field to instance data**

```cpp
struct CreatureInstanceData {
    float modelRow0[4];
    float modelRow1[4];
    float modelRow2[4];
    float modelRow3[4];
    float colorTypeLOD[4];  // RGB, type, LOD level (packed)
};
```

**Step 2: Calculate LOD per creature**

```cpp
int calculateLOD(const Vec3& creaturePos, const Camera& camera) {
    float dist = distance(creaturePos, camera.position);

    if (dist < 50.0f) return 0;
    if (dist < 150.0f) return 1;
    return 2;
}
```

**Step 3: Group by LOD for rendering**

```cpp
// Separate instance buffers per LOD
InstanceBuffer<CreatureInstanceData> m_herbLOD0, m_herbLOD1, m_herbLOD2;
InstanceBuffer<CreatureInstanceData> m_carnLOD0, m_carnLOD1, m_carnLOD2;

// Build phase
for (const auto& c : world.creatures) {
    // Frustum cull
    if (!frustum.isSphereVisible(c->position, c->size * 2.5f)) continue;

    int lod = calculateLOD(c->position, camera);
    CreatureInstanceData inst = buildInstance(*c, lod);

    if (c->type == CreatureType::HERBIVORE) {
        switch (lod) {
            case 0: m_herbLOD0.add(inst); break;
            case 1: m_herbLOD1.add(inst); break;
            case 2: m_herbLOD2.add(inst); break;
        }
    } else {
        // ... same for carnivore
    }
}

// Render phase - different shaders/meshes per LOD
renderLOD0(m_herbLOD0, m_herbMeshFull);
renderLOD1(m_herbLOD1, m_herbMeshSimple);
renderLOD2(m_herbLOD2, m_billboardQuad);
```

### 4.4 Billboard System for LOD 2

```cpp
struct BillboardInstanceData {
    float position[3];
    float size;
    float color[4];
    float texCoord[4];  // UV for atlas lookup
};

void renderBillboards() {
    // Use geometry shader to expand points to quads
    // Or pre-built quad mesh with instance transforms

    cmdList->SetPipelineState(m_billboardPSO);
    m_billboardInstances.bind(cmdList, 0, m_frameIndex);
    cmdList->DrawInstanced(4, m_billboardInstances.count(), 0, 0);
}
```

### 4.5 Expected Impact
- **Vertex processing:** 70-90% reduction for distant creatures
- **Draw calls:** Increased (6 vs 2) but smaller batches
- **Visual quality:** Minimal at intended view distances
- **Effort:** Medium-High (4-8 hours)

---

## Phase 5: Enhanced Spatial Grid

### 5.1 Objective
Optimize the spatial grid for better cache performance and reduced allocations.

### 5.2 Flat Array Layout

```cpp
class SpatialGridOptimized {
public:
    static constexpr int GRID_SIZE = 50;
    static constexpr int CELL_COUNT = GRID_SIZE * GRID_SIZE;
    static constexpr float CELL_SIZE = 20.0f;

    void create() {
        // Single flat allocation
        m_cells.resize(CELL_COUNT);
        m_queryBuffer.reserve(1000);  // Reused buffer
    }

    void clear() {
        for (auto& cell : m_cells) {
            cell.count = 0;  // Don't deallocate, just reset count
        }
    }

    void insert(Creature* c) {
        int idx = worldToIndex(c->position);
        if (idx < 0 || idx >= CELL_COUNT) return;

        auto& cell = m_cells[idx];
        if (cell.count < Cell::MAX_PER_CELL) {
            cell.creatures[cell.count++] = c;
        }
    }

    // Query that reuses buffer instead of allocating
    const std::vector<Creature*>& queryRadius(const Vec3& pos, float radius) {
        m_queryBuffer.clear();

        int minX = std::max(0, worldToGridX(pos.x - radius));
        int maxX = std::min(GRID_SIZE - 1, worldToGridX(pos.x + radius));
        int minZ = std::max(0, worldToGridZ(pos.z - radius));
        int maxZ = std::min(GRID_SIZE - 1, worldToGridZ(pos.z + radius));

        float radiusSq = radius * radius;

        for (int z = minZ; z <= maxZ; ++z) {
            for (int x = minX; x <= maxX; ++x) {
                const auto& cell = m_cells[z * GRID_SIZE + x];
                for (int i = 0; i < cell.count; ++i) {
                    Creature* c = cell.creatures[i];
                    float distSq = lengthSq(c->position - pos);
                    if (distSq <= radiusSq) {
                        m_queryBuffer.push_back(c);
                    }
                }
            }
        }

        return m_queryBuffer;
    }

private:
    struct Cell {
        static constexpr int MAX_PER_CELL = 64;
        Creature* creatures[MAX_PER_CELL];
        int count = 0;
    };

    std::vector<Cell> m_cells;  // Flat array, cache-friendly
    std::vector<Creature*> m_queryBuffer;  // Reused across queries

    int worldToIndex(const Vec3& pos) {
        int x = worldToGridX(pos.x);
        int z = worldToGridZ(pos.z);
        if (x < 0 || x >= GRID_SIZE || z < 0 || z >= GRID_SIZE) return -1;
        return z * GRID_SIZE + x;
    }

    int worldToGridX(float x) { return (int)((x + 500.0f) / CELL_SIZE); }
    int worldToGridZ(float z) { return (int)((z + 500.0f) / CELL_SIZE); }
};
```

### 5.3 Key Optimizations

1. **Flat array** - Better cache locality than vector-of-vectors
2. **Fixed-size cells** - No dynamic allocation in cells
3. **Reused query buffer** - One allocation per frame, not per query
4. **Squared distance** - Avoid sqrt in inner loop

### 5.4 Expected Impact
- **Memory allocations:** Reduced from O(queries) to O(1) per frame
- **Cache misses:** Reduced by 50%+ due to contiguous memory
- **Query time:** 20-40% faster
- **Effort:** Low-Medium (2-4 hours)

---

## Phase 6: GPU Frustum Culling

### 6.1 Objective
Move frustum culling to GPU using the existing `FrustumCull.hlsl`.

### 6.2 Integration Flow

```
CPU                           GPU
────────────────────────────────────────────────
1. Upload creature positions  →
2. Upload frustum planes      →
                              3. CSMain: Test visibility
                              4. Atomic append to visible list
                              5. CSUpdateDrawArgs: Build indirect args
                              ← 6. Indirect draw arguments ready
7. ExecuteIndirect with GPU-  ←
   generated draw calls
```

### 6.3 Implementation

**Step 1: Create buffers**

```cpp
struct GPUFrustumCuller {
    IBuffer* creaturePositions;   // SRV - all creature positions
    IBuffer* visibleIndices[6];   // UAV - per-LOD per-type visible lists
    IBuffer* drawArgs[6];         // UAV - indirect draw arguments
    IBuffer* frustumPlanes;       // CBV - 6 planes
    IPipelineState* cullPSO;
    IPipelineState* updateArgsPSO;
    IRootSignature* rootSig;
};
```

**Step 2: Dispatch cull pass**

```cpp
void dispatchFrustumCull(IComputeCommandList* cmdList) {
    cmdList->SetPipelineState(m_cullPSO);
    cmdList->SetComputeRootSignature(m_rootSig);

    // Bind resources
    cmdList->SetComputeRootSRV(0, m_creaturePositions);
    cmdList->SetComputeRootCBV(1, m_frustumPlanes);
    cmdList->SetComputeRootUAV(2, m_visibleIndices);
    cmdList->SetComputeRootUAV(3, m_drawArgs);

    // Dispatch
    UINT groupCount = (creatureCount + 255) / 256;
    cmdList->Dispatch(groupCount, 1, 1);

    // Barrier
    cmdList->UAVBarrier(m_visibleIndices);
}
```

**Step 3: Execute indirect draws**

```cpp
void renderCreaturesIndirect(IGraphicsCommandList* cmdList) {
    // Set pipeline state for creatures
    cmdList->SetPipelineState(m_creaturePSO);

    // Bind vertex/index buffers
    cmdList->BindVertexBuffer(0, m_creatureMesh);
    cmdList->BindIndexBuffer(m_creatureIndices);

    // Execute indirect draws for each LOD/type combination
    for (int i = 0; i < 6; ++i) {
        cmdList->ExecuteIndirect(
            m_drawSignature,
            1,
            m_drawArgs[i],
            0,
            nullptr,
            0
        );
    }
}
```

### 6.4 Expected Impact
- **Culling time:** < 0.1ms for 10,000+ creatures
- **CPU-GPU sync:** Eliminated for culling
- **Draw call overhead:** Reduced (6 indirect vs many direct)
- **Effort:** High (8-12 hours)

---

## Phase 7: Performance Metrics UI

### 7.1 Comprehensive ImGui Panel

```cpp
void renderPerformancePanel() {
    if (!ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    // Timing
    ImGui::Text("FPS: %.1f (%.2f ms)", m_fps, 1000.0f / m_fps);
    ImGui::Text("CPU Update: %.2f ms", m_cpuUpdateTime);
    ImGui::Text("CPU Render: %.2f ms", m_cpuRenderTime);
    ImGui::Text("GPU Frame: %.2f ms", m_gpuFrameTime);

    // Draw calls
    ImGui::Separator();
    ImGui::Text("Draw Calls: %d", m_drawCallCount);
    ImGui::Text("Triangles: %dk", m_triangleCount / 1000);
    ImGui::Text("Instances: %d", m_instanceCount);

    // Culling
    ImGui::Separator();
    ImGui::Text("Visible Creatures: %d", m_visibleCreatureCount);
    ImGui::Text("Culled Creatures: %d", m_culledCreatureCount);
    ImGui::Text("Cull Ratio: %.1f%%",
        100.0f * m_culledCreatureCount / (m_visibleCreatureCount + m_culledCreatureCount));

    // LOD Distribution
    ImGui::Separator();
    ImGui::Text("LOD 0 (Full): %d", m_lodCounts[0]);
    ImGui::Text("LOD 1 (Simple): %d", m_lodCounts[1]);
    ImGui::Text("LOD 2 (Billboard): %d", m_lodCounts[2]);

    // Memory
    ImGui::Separator();
    ImGui::Text("Instance Buffer: %.1f KB", m_instanceBufferSize / 1024.0f);
    ImGui::Text("Creature Memory: %.1f MB", m_creatureMemory / (1024.0f * 1024.0f));

    // GPU Compute (if enabled)
    if (m_useGPUSteering) {
        ImGui::Separator();
        ImGui::Text("GPU Steering: Enabled");
        ImGui::Text("Compute Dispatch: %.2f ms", m_computeTime);
    }

    // Population
    ImGui::Separator();
    ImGui::Text("Total Creatures: %zu", m_creatureCount);
    ImGui::Text("Herbivores: %d", m_herbivoreCount);
    ImGui::Text("Carnivores: %d", m_carnivoreCount);

    // Graphs
    ImGui::Separator();
    ImGui::PlotLines("Frame Time", m_frameTimeHistory.data(),
                     m_frameTimeHistory.size(), 0, nullptr, 0, 33.3f,
                     ImVec2(0, 60));
    ImGui::PlotLines("Population", m_populationHistory.data(),
                     m_populationHistory.size(), 0, nullptr, 0, 10000,
                     ImVec2(0, 60));
}
```

### 7.2 GPU Timing Queries

```cpp
class GPUTimer {
public:
    void create(IDevice* device, u32 maxQueries = 64) {
        QueryHeapDesc desc;
        desc.type = QueryType::Timestamp;
        desc.count = maxQueries * 2;  // Start + end
        m_queryHeap = device->CreateQueryHeap(desc);

        BufferDesc bufDesc;
        bufDesc.size = maxQueries * 2 * sizeof(u64);
        bufDesc.usage = BufferUsage::ReadBack;
        m_readbackBuffer = device->CreateBuffer(bufDesc);
    }

    void beginQuery(IGraphicsCommandList* cmdList, u32 index) {
        cmdList->EndQuery(m_queryHeap.get(), QueryType::Timestamp, index * 2);
    }

    void endQuery(IGraphicsCommandList* cmdList, u32 index) {
        cmdList->EndQuery(m_queryHeap.get(), QueryType::Timestamp, index * 2 + 1);
    }

    void resolve(IGraphicsCommandList* cmdList, u32 count) {
        cmdList->ResolveQueryData(m_queryHeap.get(), QueryType::Timestamp,
                                   0, count * 2, m_readbackBuffer.get(), 0);
    }

    float getTimeMs(u32 index, u64 gpuFrequency) {
        u64* data = (u64*)m_readbackBuffer->Map();
        u64 start = data[index * 2];
        u64 end = data[index * 2 + 1];
        m_readbackBuffer->Unmap();

        return (float)(end - start) / gpuFrequency * 1000.0f;
    }

private:
    std::unique_ptr<IQueryHeap> m_queryHeap;
    std::unique_ptr<IBuffer> m_readbackBuffer;
};
```

---

## Phase 8: Benchmark Suite

### 8.1 Automated Benchmarks

```cpp
struct BenchmarkResult {
    u32 creatureCount;
    float avgFps;
    float minFps;
    float maxFps;
    float avgFrameTime;
    float cpuTime;
    float gpuTime;
    u32 drawCalls;
};

std::vector<BenchmarkResult> runBenchmarkSuite() {
    std::vector<BenchmarkResult> results;

    // Test creature counts
    std::vector<u32> testCounts = {100, 500, 1000, 2000, 5000, 10000, 20000};

    for (u32 count : testCounts) {
        // Spawn creatures
        world.clearCreatures();
        for (u32 i = 0; i < count; ++i) {
            spawnRandomCreature();
        }

        // Warmup
        for (int i = 0; i < 60; ++i) {
            updateAndRender(1.0f / 60.0f);
        }

        // Measure
        BenchmarkResult result = {};
        result.creatureCount = count;

        float totalTime = 0;
        float minFps = FLT_MAX;
        float maxFps = 0;

        for (int i = 0; i < 300; ++i) {  // 5 seconds at 60fps
            auto start = std::chrono::high_resolution_clock::now();
            updateAndRender(1.0f / 60.0f);
            auto end = std::chrono::high_resolution_clock::now();

            float frameTime = std::chrono::duration<float>(end - start).count() * 1000.0f;
            float fps = 1000.0f / frameTime;

            totalTime += frameTime;
            minFps = std::min(minFps, fps);
            maxFps = std::max(maxFps, fps);
        }

        result.avgFrameTime = totalTime / 300.0f;
        result.avgFps = 1000.0f / result.avgFrameTime;
        result.minFps = minFps;
        result.maxFps = maxFps;
        result.cpuTime = m_cpuUpdateTime + m_cpuRenderTime;
        result.gpuTime = m_gpuFrameTime;
        result.drawCalls = m_drawCallCount;

        results.push_back(result);

        // Log
        printf("Benchmark: %d creatures - %.1f FPS (%.1f-%.1f)\n",
               count, result.avgFps, result.minFps, result.maxFps);
    }

    return results;
}
```

### 8.2 Success Criteria

| Creature Count | Target FPS | Target Frame Time |
|----------------|------------|-------------------|
| 1,000 | 60+ | < 16.67ms |
| 5,000 | 60+ | < 16.67ms |
| 10,000 | 60+ | < 16.67ms |
| 20,000 | 30+ | < 33.33ms |

---

## Implementation Order

### Priority 1: Quick Wins (1-2 days)
1. CPU frustum culling integration
2. Enhanced SpatialGrid
3. Basic performance metrics UI

### Priority 2: Rendering Optimization (2-3 days)
4. InstanceBuffer class
5. Tree instanced rendering
6. Nametag instanced rendering

### Priority 3: GPU Compute (3-5 days)
7. GPUSteeringCompute integration
8. Hybrid CPU+GPU update loop
9. GPU frustum culling

### Priority 4: LOD System (2-3 days)
10. LOD calculation
11. Multi-LOD instance buffers
12. Billboard system for LOD 2

### Priority 5: Polish (1-2 days)
13. Full performance metrics
14. GPU timing queries
15. Benchmark suite

---

## Risk Mitigation

### GPU Compute Fallback
```cpp
bool m_useGPUCompute = true;

void update(f32 dt) {
    try {
        if (m_useGPUCompute) {
            updateGPU(dt);
        } else {
            updateCPU(dt);
        }
    } catch (const std::exception& e) {
        // GPU compute failed, fallback to CPU
        m_useGPUCompute = false;
        updateCPU(dt);
    }
}
```

### Progressive Enhancement
- Start with CPU-only implementation
- Add GPU features incrementally
- Each phase should be independently testable
- Keep toggle switches for A/B comparison

---

## Appendix: File Locations

### Files to Create
- `src/graphics/InstanceBuffer.h`
- `src/graphics/TreeInstanceBuffer.h`
- `src/graphics/GPUFrustumCuller.h/.cpp`

### Files to Modify
- `src/main.cpp` - Main integration
- `src/utils/SpatialGrid.cpp` - Optimization

### Existing Files to Integrate
- `src/graphics/Frustum.cpp` - Include and use
- `src/ai/GPUSteeringCompute.cpp` - Instantiate and call
- `shaders/hlsl/FrustumCull.hlsl` - Bind and dispatch
- `shaders/hlsl/CreatureLOD.hlsl` - New PSO

---

*Plan created: January 2026*
*Lines: 700+*
*Target completion: Phase 1-2 (1 week), Phase 3-5 (2 weeks)*
