# DX12 Rendering Pipeline Performance Optimization

**Target:** 60 FPS with 10,000+ creatures on mid-range GPU (GTX 1660 / RX 5600)

## Executive Summary

The current implementation uses basic instanced rendering (2 draw calls for all creatures) but lacks GPU-side culling and LOD, causing performance degradation at high creature counts. This document outlines a comprehensive optimization strategy.

### Current Performance Profile

| Metric | Current (2000 creatures) | Target (10000 creatures) |
|--------|-------------------------|-------------------------|
| Draw Calls | 2 (instanced) | 6 (LOD) + 2 (compute) |
| CPU Instance Update | ~1.2ms | ~0.3ms (persistent mapped) |
| GPU Culling | None (CPU only) | GPU frustum cull |
| LOD System | None | 3 levels |
| AI Compute | Separate (blocking) | Async (overlapped) |

---

## 1. GPU Frustum Culling (Compute Shader Pre-pass)

### Current Problem
All creatures are rendered regardless of camera visibility. With 10,000 creatures at 80 bytes/instance, we upload 800KB/frame even if only 1,000 are visible.

### Solution: FrustumCull.hlsl Compute Shader

**New Shader:** `shaders/hlsl/FrustumCull.hlsl`

```hlsl
// 3 compute shader entry points:
// 1. CSClearCounts - Reset instance counts at frame start
// 2. CSMain - Parallel frustum test + LOD assignment
// 3. CSUpdateDrawArgs - Write indirect draw arguments
```

**Performance Impact:**
- Typical scene: 30-50% culled (camera FOV)
- 10,000 creatures → ~5,000-7,000 rendered
- Compute cost: <0.1ms (256 threads/group, 40 groups)

### Implementation Steps

1. Create bounding sphere buffer (32 bytes/creature)
2. Extract frustum planes from ViewProjection matrix
3. Run CSClearCounts → CSMain → CSUpdateDrawArgs
4. Use output as indirect draw arguments

```cpp
// Frustum plane extraction from ViewProjection matrix
void ExtractFrustumPlanes(const Mat4& vp, Vec4 planes[6]) {
    // Left:   row3 + row0
    // Right:  row3 - row0
    // Bottom: row3 + row1
    // Top:    row3 - row1
    // Near:   row2
    // Far:    row3 - row2
}
```

---

## 2. ExecuteIndirect for Indirect Drawing

### Current Problem
Instance counts are determined on CPU, requiring CPU-GPU sync.

### Solution: ID3D12GraphicsCommandList::ExecuteIndirect

**Buffer Layout:**
```cpp
struct IndirectDrawArgs {  // Matches D3D12_DRAW_INDEXED_ARGUMENTS
    uint32_t IndexCountPerInstance;
    uint32_t InstanceCount;      // Written by GPU compute shader
    uint32_t StartIndexLocation;
    int32_t  BaseVertexLocation;
    uint32_t StartInstanceLocation;
};
```

**Command Signature:**
```cpp
D3D12_INDIRECT_ARGUMENT_DESC args[1];
args[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

D3D12_COMMAND_SIGNATURE_DESC desc = {};
desc.ByteStride = sizeof(IndirectDrawArgs);
desc.NumArgumentDescs = 1;
desc.pArgumentDescs = args;

device->CreateCommandSignature(&desc, nullptr, IID_PPV_ARGS(&m_commandSignature));
```

**Render Loop:**
```cpp
// Instead of:
cmdList->DrawIndexedInstanced(indexCount, instanceCount, 0, 0, 0);

// Use:
cmdList->ExecuteIndirect(
    m_commandSignature.Get(),
    6,  // 6 draw calls (2 types × 3 LODs)
    m_drawArgsBuffer.Get(),
    0,
    nullptr, 0
);
```

**Performance Impact:**
- Eliminates CPU readback of instance counts
- Enables fully GPU-driven rendering
- Reduces CPU frame time by ~0.5ms

---

## 3. Async Compute for AI (Overlap with Rendering)

### Current Problem
SteeringCompute.hlsl runs on graphics queue, blocking rendering.

### Solution: Separate Compute Queue

**Architecture:**
```
Frame N:
  Graphics Queue: [Render Frame N]------------------[Present]
  Compute Queue:  [AI Compute for Frame N+1]--------[Signal]

Frame N+1:
  Graphics Queue: [Wait]---[Render Frame N+1]-------[Present]
  Compute Queue:  [AI Compute for Frame N+2]--------[Signal]
```

**Implementation:**
```cpp
// Create compute command queue
D3D12_COMMAND_QUEUE_DESC computeQueueDesc = {};
computeQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
device->CreateCommandQueue(&computeQueueDesc, IID_PPV_ARGS(&m_computeQueue));

// Separate compute command list
device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE,
                          m_computeAllocator.Get(), nullptr,
                          IID_PPV_ARGS(&m_computeCmdList));

// Fence for synchronization
device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_computeFence));
```

**Synchronization Pattern:**
```cpp
// End of render pass - signal graphics fence
m_graphicsQueue->Signal(m_graphicsFence, frameIndex);

// Start of compute pass - wait for previous frame's graphics
m_computeQueue->Wait(m_graphicsFence, frameIndex - 1);

// Execute AI compute
m_computeQueue->ExecuteCommandLists(1, &computeCmdList);
m_computeQueue->Signal(m_computeFence, frameIndex);

// Start of next frame's render - wait for compute
m_graphicsQueue->Wait(m_computeFence, frameIndex);
```

**Performance Impact:**
- AI compute overlaps with previous frame's present
- Effective 20-30% frame time reduction
- SteeringCompute: ~2ms → hidden behind rendering

---

## 4. LOD System (3 Levels)

### Current Problem
All creatures use full-detail mesh (500+ triangles) regardless of distance.

### Solution: Distance-Based LOD with Crossfade

**New Shader:** `shaders/hlsl/CreatureLOD.hlsl`

| LOD Level | Distance | Triangles | Features |
|-----------|----------|-----------|----------|
| LOD 0 | < 50m | ~500 | Full PBR, skin patterns, animation |
| LOD 1 | 50-150m | ~100 | Simplified mesh, Phong lighting |
| LOD 2 | > 150m | 2 (billboard) | Impostor quad, distance fade |

**Mesh Generation:**
```cpp
// LOD 0: Full marching cubes (resolution 16)
MarchingCubes::GenerateMesh(metaballs, vertsLOD0, indicesLOD0, 16, 0.5f);

// LOD 1: Reduced resolution (resolution 8)
MarchingCubes::GenerateMesh(metaballs, vertsLOD1, indicesLOD1, 8, 0.5f);

// LOD 2: Billboard quad
float billboardVerts[] = {
    -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
     0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
};
```

**Crossfade Transitions:**
```hlsl
// In pixel shader
float lodBlend = saturate((distance - LOD_THRESHOLD) / CROSSFADE_RANGE);
float3 color = lerp(fullDetailColor, simplifiedColor, lodBlend);
```

**Performance Impact:**
- Average 60-70% triangle reduction at typical camera distance
- 10,000 creatures: ~5M triangles → ~1.5M triangles
- GPU rasterization: ~4ms → ~1.5ms

---

## 5. Persistent Mapped Buffers

### Current Problem
Every frame: Map() → memcpy → Unmap() for instance data.

### Solution: Write-Combined Persistent Mapping

**Buffer Creation:**
```cpp
D3D12_HEAP_PROPERTIES heapProps = {};
heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;  // CPU-writable, GPU-readable

D3D12_RESOURCE_DESC bufferDesc = {};
bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
bufferDesc.Width = MAX_CREATURES * sizeof(CreatureInstanceData) * NUM_FRAMES;
bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

device->CreateCommittedResource(
    &heapProps, D3D12_HEAP_FLAG_NONE,
    &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
    nullptr, IID_PPV_ARGS(&m_instanceBuffer)
);

// Map ONCE at creation, never unmap
m_instanceBuffer->Map(0, nullptr, &m_persistentMappedPtr);
```

**Frame Update:**
```cpp
// Direct write to mapped memory (no Map/Unmap per frame)
CreatureInstanceData* frameData =
    (CreatureInstanceData*)m_persistentMappedPtr + (frameIndex * MAX_CREATURES);

for (const auto& creature : creatures) {
    frameData->SetFromCreature(creature->GetModelMatrix(),
                               creature->genome.color,
                               (int)creature->type);
    frameData++;
}
```

**Performance Impact:**
- Eliminates Map/Unmap overhead (~0.3ms/frame)
- Better CPU cache utilization
- Write-combined memory optimized for streaming uploads

---

## 6. Pipeline State Caching

### Current Problem
Pipeline states are created at initialization but not optimized for state transitions.

### Solution: Pipeline State Object (PSO) Library

**PSO Caching:**
```cpp
// At initialization: compile and cache all PSOs
struct PSOKey {
    uint64_t vsHash;
    uint64_t psHash;
    uint32_t inputLayoutHash;
    // ... other state
};

std::unordered_map<PSOKey, ComPtr<ID3D12PipelineState>> m_psoCache;

// Create PSO library for disk caching
device->CreatePipelineLibrary(libraryBlob, blobSize,
                               IID_PPV_ARGS(&m_pipelineLibrary));
```

**State Batching:**
```cpp
// Sort draw calls by PSO to minimize state changes
std::sort(drawCalls.begin(), drawCalls.end(),
    [](const DrawCall& a, const DrawCall& b) {
        return a.psoKey < b.psoKey;
    });

// Render with minimal state changes
ID3D12PipelineState* currentPSO = nullptr;
for (const auto& draw : drawCalls) {
    if (draw.pso != currentPSO) {
        cmdList->SetPipelineState(draw.pso);
        currentPSO = draw.pso;
    }
    cmdList->DrawIndexedInstanced(...);
}
```

**Performance Impact:**
- PSO changes: 6 → 3 per frame (sorted by LOD)
- First-frame compilation eliminated (disk cache)
- State change overhead: ~0.1ms/frame savings

---

## 7. Multi-threaded Command List Recording

### Current Problem
Single-threaded command list recording limits CPU parallelism.

### Solution: Per-Thread Command Lists with Bundle Reuse

**Thread Pool Setup:**
```cpp
// Create one command allocator per thread per frame
struct ThreadResources {
    ComPtr<ID3D12CommandAllocator> allocators[NUM_FRAMES];
    ComPtr<ID3D12GraphicsCommandList> cmdList;
};

std::vector<ThreadResources> m_threadResources(numThreads);
```

**Parallel Recording:**
```cpp
// Divide creatures into chunks
const size_t chunkSize = creatures.size() / numThreads;

std::vector<std::future<void>> futures;
for (int t = 0; t < numThreads; t++) {
    futures.push_back(threadPool.submit([&, t]() {
        auto& res = m_threadResources[t];
        res.cmdList->Reset(res.allocators[frameIndex].Get(), m_creaturePSO.Get());

        // Record draw commands for this chunk
        size_t start = t * chunkSize;
        size_t end = (t == numThreads - 1) ? creatures.size() : start + chunkSize;

        RecordCreatureDraws(res.cmdList.Get(), start, end);
        res.cmdList->Close();
    }));
}

// Wait for all threads
for (auto& f : futures) f.wait();

// Execute all command lists
std::vector<ID3D12CommandList*> cmdLists;
for (auto& res : m_threadResources) {
    cmdLists.push_back(res.cmdList.Get());
}
m_commandQueue->ExecuteCommandLists(cmdLists.size(), cmdLists.data());
```

**Performance Impact:**
- 4-core CPU: 2.5x command list recording speedup
- CPU frame time: ~3ms → ~1.2ms
- Better CPU/GPU parallelism

---

## Implementation Priority

| Priority | Optimization | Effort | Impact |
|----------|-------------|--------|--------|
| 1 | GPU Frustum Culling | Medium | High (30-50% fewer draws) |
| 2 | LOD System | Medium | High (60-70% fewer triangles) |
| 3 | Persistent Mapped Buffers | Low | Medium (0.3ms saved) |
| 4 | Async Compute | Medium | High (overlapped AI) |
| 5 | ExecuteIndirect | Low | Medium (GPU-driven) |
| 6 | Multi-threaded Recording | High | Medium (CPU bound only) |
| 7 | PSO Caching | Low | Low (startup time) |

---

## Memory Budget (10,000 creatures)

| Buffer | Size | Notes |
|--------|------|-------|
| Instance Data (LOD 0) | 800 KB | 10K × 80 bytes |
| Instance Data (LOD 1) | 400 KB | Simplified |
| Instance Data (LOD 2) | 160 KB | Billboard |
| Bounding Spheres | 320 KB | 10K × 32 bytes |
| Indirect Args | 120 B | 6 × 20 bytes |
| **Total GPU Memory** | ~1.7 MB | Per-frame (×2 for double buffer) |

---

## Profiling Checkpoints

### GPU Profiling (PIX/RenderDoc)
```cpp
// Add timestamp queries
cmdList->BeginQuery(m_timestampHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0);
// ... render pass ...
cmdList->EndQuery(m_timestampHeap, D3D12_QUERY_TYPE_TIMESTAMP, 1);
```

### CPU Profiling
```cpp
auto start = std::chrono::high_resolution_clock::now();
// ... operation ...
auto elapsed = std::chrono::duration<float, std::milli>(
    std::chrono::high_resolution_clock::now() - start
).count();
```

### Key Metrics to Track
- Frame time (total)
- GPU cull time
- GPU draw time
- CPU instance update time
- Visible instance count
- Triangle count (per LOD)

---

## Shader Files Created

1. **FrustumCull.hlsl** - GPU culling compute shader
   - `CSClearCounts` - Reset counters
   - `CSMain` - Parallel frustum test + LOD
   - `CSUpdateDrawArgs` - Write indirect args

2. **CreatureLOD.hlsl** - Multi-LOD creature rendering
   - `VSMain_LOD0` / `PSMain_LOD0` - Full detail
   - `VSMain_LOD1` / `PSMain_LOD1` - Simplified
   - `VSMain_Billboard` / `PSMain_Billboard` - Impostor

---

## Expected Results

With all optimizations implemented:

| Scenario | Before | After |
|----------|--------|-------|
| 2,000 creatures | 60 FPS | 120+ FPS |
| 10,000 creatures | 15 FPS | 60 FPS |
| 20,000 creatures | 5 FPS | 30 FPS |

**Bottleneck Shift:** After optimization, the bottleneck moves from GPU rendering to CPU simulation (spatial grid updates, neural network evaluation). Further scaling requires SIMD optimization or moving simulation to GPU.
