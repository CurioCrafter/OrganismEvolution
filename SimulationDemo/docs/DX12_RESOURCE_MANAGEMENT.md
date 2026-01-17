# DirectX 12 Resource Management: Comprehensive Guide

> **Target Project**: ModularGameEngine - Simulation with many creatures, terrain, and dynamic resources

This guide covers professional-grade resource management patterns for DirectX 12 applications.

---

## Table of Contents

1. [Resource Heap Types](#1-resource-heap-types)
2. [Resource Barriers & State Transitions](#2-resource-barriers--state-transitions)
3. [Descriptor Heap Management](#3-descriptor-heap-management)
4. [Memory Allocation Strategies](#4-memory-allocation-strategies)
5. [Buffer Management](#5-buffer-management)
6. [Texture Streaming](#6-texture-streaming)
7. [Best Practices Summary](#7-best-practices-summary)

---

## 1. Resource Heap Types

DirectX 12 provides explicit control over memory placement through heap types. Understanding when to use each type is critical for performance.

### 1.1 DEFAULT Heap (`D3D12_HEAP_TYPE_DEFAULT`)

**Purpose**: GPU-only memory with maximum bandwidth. The GPU can read and write at full speed.

**Characteristics**:
- Fastest GPU access
- No direct CPU access
- Requires staging through UPLOAD heap for CPU data
- Supports all resource state transitions
- On discrete GPUs: Located in dedicated VRAM (L1)
- On integrated GPUs: Located in shared memory (L0) but optimized for GPU access

**When to Use**:
- Static geometry (vertex/index buffers that don't change)
- Render targets and depth buffers
- Textures after initial upload
- UAV resources for compute shaders
- Any resource that the GPU primarily accesses

```cpp
// Creating a resource in DEFAULT heap
D3D12_HEAP_PROPERTIES heapProps = {};
heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

D3D12_RESOURCE_DESC resourceDesc = {};
resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
resourceDesc.Width = bufferSize;
resourceDesc.Height = 1;
resourceDesc.DepthOrArraySize = 1;
resourceDesc.MipLevels = 1;
resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
resourceDesc.SampleDesc.Count = 1;
resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

device->CreateCommittedResource(
    &heapProps,
    D3D12_HEAP_FLAG_NONE,
    &resourceDesc,
    D3D12_RESOURCE_STATE_COPY_DEST,  // Initial state for upload
    nullptr,
    IID_PPV_ARGS(&defaultBuffer)
);
```

### 1.2 UPLOAD Heap (`D3D12_HEAP_TYPE_UPLOAD`)

**Purpose**: CPU-writable staging memory for transferring data to the GPU.

**Characteristics**:
- CPU can write directly via `Map()`
- GPU reads only (write-combined, uncached)
- Must remain in `D3D12_RESOURCE_STATE_GENERIC_READ` state
- Located in system RAM
- Write-combined: sequential writes are efficient, reads are slow

**When to Use**:
- Staging buffers for texture/buffer uploads
- Constant buffers that change every frame
- Dynamic vertex buffers (streaming geometry)
- Any data that needs frequent CPU updates

**Critical Best Practice**: Never read from UPLOAD heap memory on CPU - it's extremely slow due to write-combined nature.

```cpp
// Creating an UPLOAD buffer
D3D12_HEAP_PROPERTIES uploadHeapProps = {};
uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

device->CreateCommittedResource(
    &uploadHeapProps,
    D3D12_HEAP_FLAG_NONE,
    &resourceDesc,
    D3D12_RESOURCE_STATE_GENERIC_READ,  // MUST be this state
    nullptr,
    IID_PPV_ARGS(&uploadBuffer)
);

// Map and write data
void* mappedData;
uploadBuffer->Map(0, nullptr, &mappedData);
memcpy(mappedData, sourceData, dataSize);  // Use memcpy for best performance
uploadBuffer->Unmap(0, nullptr);
```

### 1.3 READBACK Heap (`D3D12_HEAP_TYPE_READBACK`)

**Purpose**: CPU-readable memory for retrieving GPU results.

**Characteristics**:
- CPU can read via `Map()`
- GPU writes only
- Must remain in `D3D12_RESOURCE_STATE_COPY_DEST` state
- Located in system RAM, cached for CPU reads
- Random CPU access is efficient

**When to Use**:
- Reading back compute shader results
- GPU timing queries
- Screenshot capture
- Debug data retrieval
- Occlusion query results

```cpp
// Creating a READBACK buffer
D3D12_HEAP_PROPERTIES readbackHeapProps = {};
readbackHeapProps.Type = D3D12_HEAP_TYPE_READBACK;

device->CreateCommittedResource(
    &readbackHeapProps,
    D3D12_HEAP_FLAG_NONE,
    &resourceDesc,
    D3D12_RESOURCE_STATE_COPY_DEST,  // MUST be this state
    nullptr,
    IID_PPV_ARGS(&readbackBuffer)
);

// After GPU copy completes (sync with fence)
void* mappedData;
D3D12_RANGE readRange = { 0, dataSize };
readbackBuffer->Map(0, &readRange, &mappedData);
// Read data from mappedData
D3D12_RANGE writeRange = { 0, 0 };  // No CPU writes
readbackBuffer->Unmap(0, &writeRange);
```

### 1.4 GPU_UPLOAD Heap (`D3D12_HEAP_TYPE_GPU_UPLOAD`) - ReBAR/SAM

**Purpose**: Direct CPU-to-VRAM writes via Resizable BAR (AMD SAM / NVIDIA ReBAR).

**Availability**: Agility SDK 1.613.0+ (March 2024 for retail, no Developer Mode required)

**Characteristics**:
- CPU writes directly to VRAM
- GPU reads at full VRAM bandwidth
- Write-combined: use sequential access patterns
- PCIe bandwidth limited (~14 GB/s for PCIe 4.0 x16)

**When to Use**:
- Frequently updated data that GPU reads immediately
- Reduces copy overhead vs UPLOAD → DEFAULT pattern
- Best for medium-sized, frequently changing resources

```cpp
// Check for GPU_UPLOAD support
D3D12_FEATURE_DATA_D3D12_OPTIONS16 options16 = {};
device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &options16, sizeof(options16));
if (options16.GPUUploadHeapSupported) {
    D3D12_HEAP_PROPERTIES gpuUploadHeapProps = {};
    gpuUploadHeapProps.Type = D3D12_HEAP_TYPE_GPU_UPLOAD;

    device->CreateCommittedResource(
        &gpuUploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&gpuUploadBuffer)
    );
}
```

### 1.5 Upload-to-DEFAULT Pattern

The standard pattern for initializing GPU resources:

```cpp
// 1. Create staging buffer in UPLOAD heap
ComPtr<ID3D12Resource> stagingBuffer;
CreateCommittedResource(UPLOAD, &stagingBuffer);

// 2. Create destination in DEFAULT heap
ComPtr<ID3D12Resource> destBuffer;
CreateCommittedResource(DEFAULT, &destBuffer);

// 3. Map and write to staging
void* mapped;
stagingBuffer->Map(0, nullptr, &mapped);
memcpy(mapped, sourceData, size);
stagingBuffer->Unmap(0, nullptr);

// 4. Copy staging to destination
commandList->CopyResource(destBuffer.Get(), stagingBuffer.Get());

// 5. Transition destination for use
D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
    destBuffer.Get(),
    D3D12_RESOURCE_STATE_COPY_DEST,
    D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
);
commandList->ResourceBarrier(1, &barrier);
```

---

## 2. Resource Barriers & State Transitions

Resource barriers synchronize GPU operations and manage resource states. Incorrect barriers cause corruption; excessive barriers kill performance.

### 2.1 Barrier Types

#### Transition Barriers
Change resource state (e.g., render target → shader resource).

```cpp
D3D12_RESOURCE_BARRIER barrier = {};
barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
barrier.Transition.pResource = resource;
barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

commandList->ResourceBarrier(1, &barrier);
```

#### UAV Barriers
Synchronize unordered access between dispatches/draws.

```cpp
D3D12_RESOURCE_BARRIER uavBarrier = {};
uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
uavBarrier.UAV.pResource = uavResource;  // or nullptr for all UAVs

commandList->ResourceBarrier(1, &uavBarrier);
```

#### Aliasing Barriers
Synchronize when switching between aliased resources sharing memory.

```cpp
D3D12_RESOURCE_BARRIER aliasingBarrier = {};
aliasingBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
aliasingBarrier.Aliasing.pResourceBefore = resourceA;
aliasingBarrier.Aliasing.pResourceAfter = resourceB;

commandList->ResourceBarrier(1, &aliasingBarrier);
```

### 2.2 Split Barriers

Split barriers allow the GPU to schedule transitions asynchronously, potentially hiding latency.

```cpp
// BEGIN the split barrier - resource cannot be used until END
D3D12_RESOURCE_BARRIER beginBarrier = {};
beginBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
beginBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
beginBarrier.Transition.pResource = resource;
beginBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
beginBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
commandList->ResourceBarrier(1, &beginBarrier);

// ... do other work while transition is in progress ...

// END the split barrier - resource now usable in new state
D3D12_RESOURCE_BARRIER endBarrier = beginBarrier;
endBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
commandList->ResourceBarrier(1, &endBarrier);
```

**When to Use Split Barriers**:
- Large texture decompression/format conversions
- When you can schedule other independent work between begin/end
- Cross-command-list transitions

### 2.3 Batching Barriers

**Always batch multiple barriers into single calls** - this is critical for performance.

```cpp
// GOOD: Batched barriers
std::vector<D3D12_RESOURCE_BARRIER> barriers;
barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
    resourceA, STATE_A_BEFORE, STATE_A_AFTER));
barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
    resourceB, STATE_B_BEFORE, STATE_B_AFTER));
barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
    resourceC, STATE_C_BEFORE, STATE_C_AFTER));

commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

// BAD: Individual barriers (driver overhead, potential stalls)
commandList->ResourceBarrier(1, &barrierA);
commandList->ResourceBarrier(1, &barrierB);
commandList->ResourceBarrier(1, &barrierC);
```

### 2.4 Common Resource States

| State | Usage |
|-------|-------|
| `COMMON` | Initial state, avoid in hot paths |
| `VERTEX_AND_CONSTANT_BUFFER` | Vertex buffer or CBV reads |
| `INDEX_BUFFER` | Index buffer reads |
| `RENDER_TARGET` | Render target writes |
| `UNORDERED_ACCESS` | UAV read/write |
| `DEPTH_WRITE` | Depth buffer writes |
| `DEPTH_READ` | Depth buffer reads (shadow maps) |
| `NON_PIXEL_SHADER_RESOURCE` | VS/GS/HS/DS/CS texture reads |
| `PIXEL_SHADER_RESOURCE` | PS texture reads |
| `COPY_DEST` | Copy destination |
| `COPY_SOURCE` | Copy source |
| `GENERIC_READ` | Required for UPLOAD heap resources |

### 2.5 Anti-Patterns to Avoid

```cpp
// ANTI-PATTERN 1: Unnecessary COMMON state usage
// COMMON causes driver to assume worst-case synchronization
barrier.StateAfter = D3D12_RESOURCE_STATE_COMMON;  // Avoid!

// ANTI-PATTERN 2: Redundant barriers
// Track state yourself; don't transition to current state
if (currentState != desiredState) {
    // Only then issue barrier
}

// ANTI-PATTERN 3: Per-draw barriers on same resource
// Batch all state changes at render pass boundaries
for (auto& object : objects) {
    commandList->ResourceBarrier(...);  // Bad if inside hot loop
    Draw(object);
}
```

### 2.6 Enhanced Barriers (D3D12 Agility SDK)

The new Enhanced Barriers API provides finer-grained control:

```cpp
// Enhanced barrier with explicit sync and access
D3D12_TEXTURE_BARRIER textureBarrier = {};
textureBarrier.SyncBefore = D3D12_BARRIER_SYNC_RENDER_TARGET;
textureBarrier.SyncAfter = D3D12_BARRIER_SYNC_PIXEL_SHADING;
textureBarrier.AccessBefore = D3D12_BARRIER_ACCESS_RENDER_TARGET;
textureBarrier.AccessAfter = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
textureBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
textureBarrier.LayoutAfter = D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
textureBarrier.pResource = texture;
textureBarrier.Subresources.IndexOrFirstMipLevel = 0;
textureBarrier.Subresources.NumMipLevels = 1;
textureBarrier.Subresources.FirstArraySlice = 0;
textureBarrier.Subresources.NumArraySlices = 1;
textureBarrier.Subresources.FirstPlane = 0;
textureBarrier.Subresources.NumPlanes = 1;

D3D12_BARRIER_GROUP barrierGroup = {};
barrierGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
barrierGroup.NumBarriers = 1;
barrierGroup.pTextureBarriers = &textureBarrier;

commandList->Barrier(1, &barrierGroup);
```

---

## 3. Descriptor Heap Management

Descriptors are the bridge between shaders and resources. Efficient management is crucial for scalability.

### 3.1 Descriptor Heap Types

| Type | Shader Visible | Max Count | Usage |
|------|----------------|-----------|-------|
| `CBV_SRV_UAV` | Yes | 1,000,000+ | Constant buffers, textures, UAVs |
| `SAMPLER` | Yes | 2,048 | Texture samplers |
| `RTV` | No | Unlimited | Render target views |
| `DSV` | No | Unlimited | Depth stencil views |

### 3.2 Shader-Visible vs Non-Shader-Visible

**Shader-Visible Heaps**:
- Can be bound to pipeline via `SetDescriptorHeaps()`
- Limited to ONE CBV_SRV_UAV and ONE SAMPLER heap at a time
- Changing bound heaps causes pipeline flush - **avoid mid-frame**

**Non-Shader-Visible (CPU) Heaps**:
- Used for staging descriptors
- Copy to shader-visible heap when needed
- No pipeline impact from allocation

### 3.3 Descriptor Management Architecture

```cpp
class DescriptorHeapManager {
public:
    struct DescriptorAllocation {
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;  // Only valid for shader-visible
        uint32_t index;
    };

private:
    // GPU-visible heap (one per type, bound at frame start)
    ComPtr<ID3D12DescriptorHeap> m_gpuHeap;

    // CPU-staging heaps (for descriptor creation)
    ComPtr<ID3D12DescriptorHeap> m_cpuStagingHeap;

    // Static region: persistent resources (textures, static buffers)
    uint32_t m_staticRegionStart = 0;
    uint32_t m_staticRegionEnd;
    std::queue<uint32_t> m_staticFreeList;

    // Dynamic region: per-frame allocations (ring buffer)
    uint32_t m_dynamicRegionStart;
    uint32_t m_dynamicRegionEnd;
    std::atomic<uint32_t> m_dynamicHead;

    uint32_t m_descriptorSize;

public:
    void Initialize(ID3D12Device* device, uint32_t staticCount, uint32_t dynamicCount) {
        uint32_t totalCount = staticCount + dynamicCount;

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.NumDescriptors = totalCount;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_gpuHeap));

        m_descriptorSize = device->GetDescriptorHandleIncrementSize(heapDesc.Type);
        m_staticRegionEnd = staticCount;
        m_dynamicRegionStart = staticCount;
        m_dynamicRegionEnd = totalCount;
        m_dynamicHead = m_dynamicRegionStart;
    }

    // Allocate persistent descriptor (texture, static buffer)
    DescriptorAllocation AllocateStatic() {
        uint32_t index;
        if (!m_staticFreeList.empty()) {
            index = m_staticFreeList.front();
            m_staticFreeList.pop();
        } else {
            // Linear allocation for initial fill
            static uint32_t nextStatic = 0;
            index = nextStatic++;
        }
        return GetAllocation(index);
    }

    // Allocate temporary descriptor (per-frame constant buffer)
    DescriptorAllocation AllocateDynamic() {
        uint32_t index = m_dynamicHead.fetch_add(1) %
            (m_dynamicRegionEnd - m_dynamicRegionStart) + m_dynamicRegionStart;
        return GetAllocation(index);
    }

    // Reset dynamic region at frame boundary
    void ResetDynamicAllocations() {
        m_dynamicHead = m_dynamicRegionStart;
    }

private:
    DescriptorAllocation GetAllocation(uint32_t index) {
        DescriptorAllocation alloc;
        alloc.index = index;
        alloc.cpuHandle = m_gpuHeap->GetCPUDescriptorHandleForHeapStart();
        alloc.cpuHandle.ptr += index * m_descriptorSize;
        alloc.gpuHandle = m_gpuHeap->GetGPUDescriptorHandleForHeapStart();
        alloc.gpuHandle.ptr += index * m_descriptorSize;
        return alloc;
    }
};
```

### 3.4 Bindless Rendering (SM 6.6+)

Modern bindless rendering eliminates per-draw descriptor table binding.

```cpp
// Minimal root signature for bindless
D3D12_ROOT_SIGNATURE_FLAGS flags =
    D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
    D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

// Root signature with just indices passed as root constants
D3D12_ROOT_PARAMETER1 rootParams[1] = {};
rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
rootParams[0].Constants.ShaderRegister = 0;
rootParams[0].Constants.RegisterSpace = 0;
rootParams[0].Constants.Num32BitValues = 4;  // texture index, sampler index, etc.

// In HLSL (SM 6.6+)
/*
cbuffer RootConstants : register(b0) {
    uint albedoTextureIndex;
    uint normalTextureIndex;
    uint samplerIndex;
};

Texture2D textures[] : register(t0);  // Bindless texture array
SamplerState samplers[] : register(s0);

float4 main(PSInput input) : SV_TARGET {
    Texture2D albedo = ResourceDescriptorHeap[albedoTextureIndex];
    SamplerState samp = SamplerDescriptorHeap[samplerIndex];
    return albedo.Sample(samp, input.uv);
}
*/
```

### 3.5 Descriptor Heap Best Practices

1. **Never change shader-visible heaps mid-frame** - causes pipeline flush
2. **Use free lists for static allocations** - avoid fragmentation
3. **Use ring buffer for dynamic allocations** - O(1) allocation
4. **Pre-create descriptors** - `CreateShaderResourceView` is not free
5. **Consider bindless** - eliminates most descriptor management overhead

---

## 4. Memory Allocation Strategies

### 4.1 Committed vs Placed Resources

**Committed Resources** (`CreateCommittedResource`):
- Each resource gets its own implicit heap
- Simple but wasteful - heaps are 64KB aligned minimum
- Good for: Large, long-lived resources

**Placed Resources** (`CreatePlacedResource`):
- Multiple resources share explicit heaps
- Memory-efficient through sub-allocation
- Requires manual heap management
- Good for: Many small resources, pooling

```cpp
// Placed resource example
D3D12_HEAP_DESC heapDesc = {};
heapDesc.SizeInBytes = 64 * 1024 * 1024;  // 64 MB heap
heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

ComPtr<ID3D12Heap> heap;
device->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap));

// Place multiple buffers in the heap
device->CreatePlacedResource(
    heap.Get(),
    offsetInHeap,
    &resourceDesc,
    D3D12_RESOURCE_STATE_COMMON,
    nullptr,
    IID_PPV_ARGS(&resource)
);
```

### 4.2 D3D12 Memory Allocator (D3D12MA)

AMD's open-source D3D12MA handles allocation complexity for you.

```cpp
#include "D3D12MemAlloc.h"

// Create allocator
D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
allocatorDesc.pDevice = device;
allocatorDesc.pAdapter = adapter;

D3D12MA::Allocator* allocator;
D3D12MA::CreateAllocator(&allocatorDesc, &allocator);

// Allocate resource (automatically uses placed resources when beneficial)
D3D12MA::ALLOCATION_DESC allocationDesc = {};
allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

D3D12MA::Allocation* allocation;
device->CreateResource(
    &allocationDesc,
    &resourceDesc,
    D3D12_RESOURCE_STATE_COPY_DEST,
    nullptr,
    &allocation,
    IID_PPV_ARGS(&resource)
);

// D3D12MA automatically:
// - Pools allocations in large heaps (64 MB default)
// - Falls back to committed resources when over budget
// - Tracks memory usage and budget
```

### 4.3 Memory Aliasing

Share memory between resources that aren't used simultaneously.

```cpp
// Create heap large enough for largest resource
D3D12_RESOURCE_ALLOCATION_INFO info1 = device->GetResourceAllocationInfo(0, 1, &desc1);
D3D12_RESOURCE_ALLOCATION_INFO info2 = device->GetResourceAllocationInfo(0, 1, &desc2);
UINT64 heapSize = max(info1.SizeInBytes, info2.SizeInBytes);

D3D12_HEAP_DESC heapDesc = {};
heapDesc.SizeInBytes = heapSize;
heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;

ComPtr<ID3D12Heap> heap;
device->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap));

// Create aliased resources at same offset
device->CreatePlacedResource(heap.Get(), 0, &desc1, ...);
device->CreatePlacedResource(heap.Get(), 0, &desc2, ...);

// Use aliasing barrier when switching
D3D12_RESOURCE_BARRIER aliasingBarrier = {};
aliasingBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
aliasingBarrier.Aliasing.pResourceBefore = resource1;
aliasingBarrier.Aliasing.pResourceAfter = resource2;
commandList->ResourceBarrier(1, &aliasingBarrier);
```

### 4.4 Memory Budgeting

Query and respect memory budget to avoid OS paging.

```cpp
// Get adapter with budget query support
ComPtr<IDXGIAdapter3> adapter3;
adapter.As(&adapter3);

void CheckMemoryBudget() {
    DXGI_QUERY_VIDEO_MEMORY_INFO localInfo = {};
    DXGI_QUERY_VIDEO_MEMORY_INFO nonLocalInfo = {};

    adapter3->QueryVideoMemoryInfo(
        0,  // Node index
        DXGI_MEMORY_SEGMENT_GROUP_LOCAL,  // VRAM
        &localInfo
    );

    adapter3->QueryVideoMemoryInfo(
        0,
        DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL,  // System RAM
        &nonLocalInfo
    );

    // localInfo.Budget - target maximum for VRAM usage
    // localInfo.CurrentUsage - current VRAM consumption

    if (localInfo.CurrentUsage > localInfo.Budget) {
        // Over budget! Consider:
        // - Evicting lower-priority resources
        // - Using lower-quality assets
        // - Reducing texture resolution
    }
}

// With D3D12MA: Respect budget automatically
D3D12MA::ALLOCATION_DESC allocDesc = {};
allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_WITHIN_BUDGET;
// Allocation fails if it would exceed budget
```

---

## 5. Buffer Management

### 5.1 Constant Buffer Ring Buffer

The ring buffer pattern avoids per-draw state transitions and GPU stalls.

```cpp
class ConstantBufferRingAllocator {
private:
    ComPtr<ID3D12Resource> m_buffer;
    uint8_t* m_mappedPtr;

    size_t m_bufferSize;
    size_t m_head;  // Current write position
    size_t m_frameTails[MAX_FRAMES_IN_FLIGHT];
    uint32_t m_currentFrame;

    static constexpr size_t CB_ALIGNMENT = 256;  // D3D12 CBV alignment

public:
    void Initialize(ID3D12Device* device, size_t size) {
        m_bufferSize = size;
        m_head = 0;

        // Create persistently mapped UPLOAD buffer
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = size;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_buffer)
        );

        // Keep mapped for lifetime
        m_buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedPtr));
    }

    // Called at start of new frame
    void BeginFrame(uint32_t frameIndex) {
        m_currentFrame = frameIndex;

        // Reclaim space from completed frames
        // (Frame N-2 is guaranteed complete if N is starting)
        uint32_t completedFrame = (frameIndex + MAX_FRAMES_IN_FLIGHT - 2) % MAX_FRAMES_IN_FLIGHT;
        // Space from frame start to m_frameTails[completedFrame] is now free
    }

    // Called at end of frame to record tail position
    void EndFrame() {
        m_frameTails[m_currentFrame] = m_head;
    }

    struct Allocation {
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
        void* cpuAddress;
    };

    Allocation Allocate(size_t size) {
        // Align to 256 bytes
        size_t alignedSize = (size + CB_ALIGNMENT - 1) & ~(CB_ALIGNMENT - 1);

        // Wrap around if needed
        if (m_head + alignedSize > m_bufferSize) {
            m_head = 0;
        }

        Allocation alloc;
        alloc.cpuAddress = m_mappedPtr + m_head;
        alloc.gpuAddress = m_buffer->GetGPUVirtualAddress() + m_head;

        m_head += alignedSize;

        return alloc;
    }
};

// Usage
void RenderFrame() {
    cbAllocator.BeginFrame(currentFrameIndex);

    for (auto& object : objects) {
        auto cbAlloc = cbAllocator.Allocate(sizeof(ObjectConstants));
        memcpy(cbAlloc.cpuAddress, &object.constants, sizeof(ObjectConstants));

        commandList->SetGraphicsRootConstantBufferView(0, cbAlloc.gpuAddress);
        commandList->DrawIndexedInstanced(...);
    }

    cbAllocator.EndFrame();
}
```

### 5.2 Upload Buffer Pool

Pool staging buffers for texture/buffer uploads.

```cpp
class UploadBufferPool {
private:
    struct PooledBuffer {
        ComPtr<ID3D12Resource> resource;
        size_t size;
        uint64_t lastUsedFence;
        bool inUse;
    };

    std::vector<PooledBuffer> m_buffers;
    ComPtr<ID3D12Fence> m_fence;

public:
    ID3D12Resource* AcquireBuffer(size_t requiredSize, uint64_t currentFence) {
        // Find suitable free buffer
        for (auto& buffer : m_buffers) {
            if (!buffer.inUse &&
                buffer.size >= requiredSize &&
                buffer.lastUsedFence <= currentFence) {
                buffer.inUse = true;
                return buffer.resource.Get();
            }
        }

        // Create new buffer if none available
        PooledBuffer newBuffer;
        newBuffer.size = std::max(requiredSize, 64 * 1024ULL);  // Min 64KB
        // Create resource...
        newBuffer.inUse = true;
        m_buffers.push_back(std::move(newBuffer));

        return m_buffers.back().resource.Get();
    }

    void ReleaseBuffer(ID3D12Resource* buffer, uint64_t completionFence) {
        for (auto& pooled : m_buffers) {
            if (pooled.resource.Get() == buffer) {
                pooled.inUse = false;
                pooled.lastUsedFence = completionFence;
                break;
            }
        }
    }
};
```

### 5.3 Dynamic Vertex Buffer Streaming

For particle systems, procedural geometry, or animated meshes:

```cpp
class DynamicVertexBuffer {
private:
    ComPtr<ID3D12Resource> m_buffer;
    uint8_t* m_mappedPtr;
    size_t m_capacity;
    size_t m_writeOffset;

public:
    void Initialize(ID3D12Device* device, size_t maxVertices, size_t vertexStride) {
        m_capacity = maxVertices * vertexStride;

        D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_UPLOAD };
        D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(m_capacity);

        device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_buffer)
        );

        m_buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedPtr));
    }

    // Returns GPU address and CPU write pointer for vertices
    D3D12_VERTEX_BUFFER_VIEW AllocateVertices(
        size_t vertexCount,
        size_t vertexStride,
        void** outCpuPtr
    ) {
        size_t requiredSize = vertexCount * vertexStride;

        // Wrap if needed
        if (m_writeOffset + requiredSize > m_capacity) {
            m_writeOffset = 0;
        }

        *outCpuPtr = m_mappedPtr + m_writeOffset;

        D3D12_VERTEX_BUFFER_VIEW vbv;
        vbv.BufferLocation = m_buffer->GetGPUVirtualAddress() + m_writeOffset;
        vbv.SizeInBytes = static_cast<UINT>(requiredSize);
        vbv.StrideInBytes = static_cast<UINT>(vertexStride);

        m_writeOffset += requiredSize;

        return vbv;
    }

    void ResetForFrame() {
        m_writeOffset = 0;
    }
};
```

---

## 6. Texture Streaming

### 6.1 Tiled Resources Overview

Tiled (reserved) resources enable partial residency - only load mip levels that are needed.

**Hardware Requirements**: `D3D12_TILED_RESOURCES_TIER_2` minimum for practical streaming.

```cpp
// Check tiled resource support
D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));

if (options.TiledResourcesTier >= D3D12_TILED_RESOURCES_TIER_2) {
    // Full streaming support available
}
```

### 6.2 Creating Reserved (Tiled) Textures

```cpp
// Create reserved texture (no backing memory yet)
D3D12_RESOURCE_DESC textureDesc = {};
textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
textureDesc.Width = 16384;  // 16K texture
textureDesc.Height = 16384;
textureDesc.DepthOrArraySize = 1;
textureDesc.MipLevels = 0;  // Full mip chain
textureDesc.Format = DXGI_FORMAT_BC7_UNORM;
textureDesc.SampleDesc.Count = 1;
textureDesc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;

device->CreateReservedResource(
    &textureDesc,
    D3D12_RESOURCE_STATE_COMMON,
    nullptr,
    IID_PPV_ARGS(&tiledTexture)
);

// Create heap for tile storage
D3D12_HEAP_DESC heapDesc = {};
heapDesc.SizeInBytes = 256 * 1024 * 1024;  // 256 MB tile pool
heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
heapDesc.Flags = D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES;

device->CreateHeap(&heapDesc, IID_PPV_ARGS(&tileHeap));
```

### 6.3 Mapping Tiles

```cpp
// Map a tile to memory
D3D12_TILED_RESOURCE_COORDINATE coord = {};
coord.X = tileX;
coord.Y = tileY;
coord.Z = 0;
coord.Subresource = mipLevel;

D3D12_TILE_REGION_SIZE regionSize = {};
regionSize.NumTiles = 1;
regionSize.UseBox = FALSE;

D3D12_TILE_RANGE_FLAGS rangeFlags = D3D12_TILE_RANGE_FLAG_NONE;
UINT heapTileOffset = allocatedTileIndex;  // From your tile allocator
UINT rangeTileCount = 1;

commandQueue->UpdateTileMappings(
    tiledTexture.Get(),
    1,
    &coord,
    &regionSize,
    tileHeap.Get(),
    1,
    &rangeFlags,
    &heapTileOffset,
    &rangeTileCount,
    D3D12_TILE_MAPPING_FLAG_NONE
);
```

### 6.4 Sampler Feedback Streaming

Use Sampler Feedback to know which mips are actually sampled.

```cpp
// Create feedback texture (much smaller than source)
D3D12_RESOURCE_DESC feedbackDesc = {};
feedbackDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
feedbackDesc.Width = textureWidth / 64;   // One feedback texel per 64x64 region
feedbackDesc.Height = textureHeight / 64;
feedbackDesc.Format = DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE;
feedbackDesc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;

device->CreateCommittedResource2(
    &defaultHeapProps,
    D3D12_HEAP_FLAG_NONE,
    &feedbackDesc,
    D3D12_RESOURCE_STATE_COMMON,
    nullptr,
    nullptr,
    IID_PPV_ARGS(&feedbackTexture)
);

// Create paired SRV
D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
srvDesc.Format = DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE;
srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
srvDesc.Texture2D.MostDetailedMip = 0;
srvDesc.Texture2D.MipLevels = 1;

device->CreateShaderResourceView(feedbackTexture.Get(), &srvDesc, cpuHandle);

// In shader: write feedback during sampling
/*
Texture2D<float4> diffuseTexture : register(t0);
FeedbackTexture2D<SAMPLER_FEEDBACK_MIN_MIP> feedbackMap : register(u0);

float4 main(PSInput input) : SV_TARGET {
    // This records which mip would be sampled
    feedbackMap.WriteSamplerFeedback(diffuseTexture, linearSampler, input.uv);
    return diffuseTexture.Sample(linearSampler, input.uv);
}
*/
```

### 6.5 Residency Management

```cpp
// Set residency priority for streaming textures
device->SetResidencyPriority(
    1,
    &tiledTexture,
    &D3D12_RESIDENCY_PRIORITY_HIGH  // Keep in VRAM if possible
);

// Eviction callback (optional)
void OnBudgetExceeded() {
    // Sort textures by importance (distance, screen coverage)
    // Evict least important tiles first

    for (auto& texture : sortedByImportance) {
        if (texture.residentTiles > texture.requiredTiles) {
            EvictExcessTiles(texture);
        }
    }
}
```

---

## 7. Best Practices Summary

### Memory Management
- Use **D3D12MA** for automatic heap management and budget awareness
- Prefer **placed resources** for many small allocations
- Use **GPU_UPLOAD** when available for frequently updated data
- Monitor budget via `QueryVideoMemoryInfo()` and react to pressure

### Barriers
- **Batch barriers** - never issue single-barrier calls in loops
- Use **split barriers** for async work opportunities
- Track resource states to avoid redundant transitions
- Consider **Enhanced Barriers** for modern applications

### Descriptors
- **One shader-visible heap** bound for entire frame
- Split into **static** (free list) and **dynamic** (ring buffer) regions
- Consider **bindless** for simplified management (SM 6.6+)

### Buffers
- **Ring buffer** for per-frame constant buffer allocations
- Keep UPLOAD buffers **persistently mapped**
- Use **256-byte alignment** for constant buffers
- Pool upload buffers for texture streaming

### Textures
- Use **tiled resources** for streamable content
- Implement **Sampler Feedback** for optimal mip selection
- Set **residency priorities** for important resources

### Frame Synchronization
- Track **MAX_FRAMES_IN_FLIGHT** (typically 2-3)
- Use **fences** to know when GPU completes frames
- Only reuse memory when corresponding fence signals

---

## References

See [dx12_links.md](references/dx12_links.md) for comprehensive link collection.

### Official Documentation
- [Microsoft D3D12 Programming Guide](https://learn.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide)
- [DirectX Specs GitHub](https://microsoft.github.io/DirectX-Specs/)

### Key Libraries
- [D3D12 Memory Allocator (D3D12MA)](https://gpuopen.com/d3d12-memory-allocator/)
- [DirectX Tool Kit for DX12](https://github.com/microsoft/DirectXTK12)

---

*Document Version: 1.0*
*Last Updated: January 2026*
*Target: DirectX 12 with Agility SDK 1.613.0+*
