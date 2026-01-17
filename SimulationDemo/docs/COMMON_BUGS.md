# Common DirectX 12 Bug Patterns

Catalog of common DX12 bugs, their symptoms, root causes, and solutions.

## Table of Contents

1. [Resource State Mismatches](#resource-state-mismatches)
2. [Descriptor Corruption](#descriptor-corruption)
3. [Root Signature Errors](#root-signature-errors)
4. [Synchronization Issues](#synchronization-issues)
5. [Memory Issues](#memory-issues)
6. [Pipeline State Issues](#pipeline-state-issues)
7. [Command List Errors](#command-list-errors)
8. [TDR / Device Removal](#tdr--device-removal)

---

## Resource State Mismatches

### Bug #1: RESOURCE_BARRIER_BEFORE_AFTER_MISMATCH

**Debug Layer Message:**
```
D3D12 ERROR: ID3D12CommandList::ResourceBarrier: Before state (0x...) of
resource (0x...) does not match current state (0x...).
```

**Symptoms:**
- Rendering corruption
- Missing geometry
- Flickering textures
- GPU hang in severe cases

**Root Cause:**
The "before" state in your transition barrier doesn't match the resource's actual current state.

**Common Scenarios:**
1. Forgot to transition back after use
2. State changed on different queue without sync
3. Using promoted state incorrectly
4. Copy queue state vs Direct queue state confusion

**Fix Pattern:**
```cpp
// BAD - Assuming state without tracking
cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
    resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
    D3D12_RESOURCE_STATE_RENDER_TARGET));

// GOOD - Track state properly
void TransitionResource(ID3D12Resource* resource,
                        D3D12_RESOURCE_STATES newState) {
    D3D12_RESOURCE_STATES currentState = resourceStates[resource];
    if (currentState != newState) {
        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
            resource, currentState, newState));
        resourceStates[resource] = newState;
    }
}
```

**Prevention:**
- Use a resource state tracking system
- Always transition resources explicitly
- Be aware of implicit state decay rules
- Consider using Enhanced Barriers API

---

### Bug #2: Resource State Promotion Confusion

**Symptoms:**
- Works sometimes, fails other times
- Different behavior on different GPUs
- Works in debug, fails in release

**Root Cause:**
Buffers and certain textures can be "promoted" to read states implicitly, but this behavior is subtle and GPU-dependent.

**State Promotion Rules:**
- Buffers can be promoted from COMMON to any read state
- Promoted states decay back to COMMON after ExecuteCommandLists
- Only works for buffers and non-simultaneous-access textures

**Fix Pattern:**
```cpp
// Don't rely on promotion for clarity
// Explicitly transition everything
void PrepareForRead(ID3D12Resource* buffer) {
    // Even if promotion would work, be explicit
    TransitionResource(buffer, D3D12_RESOURCE_STATE_COMMON,
                       D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
}
```

---

### Bug #3: Missing UAV Barrier

**Debug Layer Message:**
```
D3D12 WARNING: Possible hazard: Resource is in UAV state and being accessed
without a UAV barrier.
```

**Symptoms:**
- Compute shader results corrupted
- Race conditions between dispatches
- Inconsistent output

**Fix Pattern:**
```cpp
// Between dispatches that write to same UAV
cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(uavResource));

// Or use enhanced barriers
D3D12_BUFFER_BARRIER uavBarrier = {
    .SyncBefore = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
    .SyncAfter = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
    .AccessBefore = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
    .AccessAfter = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
    .pResource = uavResource
};
```

---

## Descriptor Corruption

### Bug #4: Uninitialized Descriptor Usage

**Debug Layer Message:**
```
D3D12 ERROR: ID3D12CommandList::DrawIndexedInstanced: Root Parameter Index [N]
is a descriptor table and all descriptors must be initialized before use.
```

**Symptoms:**
- Black textures
- GPU crash
- Random visual corruption
- Works on some GPUs, crashes on others

**Root Cause:**
Using a descriptor table where some descriptors haven't been written.

**Fix Pattern:**
```cpp
// Initialize ALL descriptors in a table, even unused ones
void InitializeDescriptorTable(DescriptorHeap& heap, UINT start, UINT count) {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = heap.GetCPUHandle(start);
    UINT increment = device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    for (UINT i = 0; i < count; i++) {
        // Create null SRV for unused slots
        D3D12_SHADER_RESOURCE_VIEW_DESC nullSrv = {};
        nullSrv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        nullSrv.ViewType = D3D12_SRV_DIMENSION_TEXTURE2D;
        nullSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        device->CreateShaderResourceView(nullptr, &nullSrv, handle);
        handle.ptr += increment;
    }
}
```

---

### Bug #5: Descriptor Heap Overflow

**Symptoms:**
- Graphics corruption after running for a while
- GPU crash / TDR
- Works initially, then fails

**Root Cause:**
Running out of descriptors in temporary/dynamic descriptor heaps.

**Fix Pattern:**
```cpp
class DescriptorAllocator {
    UINT currentOffset = 0;
    UINT heapSize;

public:
    UINT Allocate(UINT count) {
        if (currentOffset + count > heapSize) {
            // Option 1: Ring buffer with fence tracking
            WaitForOldestFence();
            currentOffset = 0;

            // Option 2: Grow heap (less ideal)
            // GrowHeap();
        }

        UINT offset = currentOffset;
        currentOffset += count;
        return offset;
    }

    void ResetFrame() {
        // Only reset if using per-frame allocation
        currentOffset = 0;
    }
};
```

---

### Bug #6: Stale Descriptor References

**Debug Layer Message:**
```
D3D12 ERROR: Descriptor references a deleted resource.
```

**Symptoms:**
- Crash when using old materials
- Works for a while, then corrupts

**Root Cause:**
Resource was deleted but descriptor still points to it.

**Fix Pattern:**
```cpp
// Use reference counting or explicit lifetime management
class ManagedTexture {
    ComPtr<ID3D12Resource> resource;
    UINT srvIndex;
    std::atomic<int> refCount{1};

public:
    void Release() {
        if (--refCount == 0) {
            // Defer actual deletion until GPU is done
            deletionQueue.push({resource, srvIndex, currentFenceValue});
        }
    }
};

// Process deletion queue when safe
void ProcessDeletions() {
    while (!deletionQueue.empty()) {
        auto& item = deletionQueue.front();
        if (fence->GetCompletedValue() >= item.fenceValue) {
            // Now safe to release
            item.resource.Reset();
            descriptorAllocator.Free(item.srvIndex);
            deletionQueue.pop();
        } else {
            break;
        }
    }
}
```

---

## Root Signature Errors

### Bug #7: Root Signature Mismatch

**Debug Layer Message:**
```
D3D12 ERROR: Root Signature doesn't match Vertex Shader.
```

**Symptoms:**
- PSO creation fails
- Shader bindings don't work

**Root Cause:**
Root signature doesn't match what shader expects.

**Common Issues:**
1. Missing constant buffer binding
2. Wrong register space
3. Descriptor range mismatch
4. Static sampler configuration

**Fix Pattern:**
```cpp
// Match shader declarations
// Shader: cbuffer Constants : register(b0, space0)
//         Texture2D tex : register(t0, space0)

CD3DX12_ROOT_PARAMETER1 params[2];

// b0 - Constant buffer
params[0].InitAsConstantBufferView(0, 0);  // register b0, space 0

// t0 - SRV table
CD3DX12_DESCRIPTOR_RANGE1 srvRange;
srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);  // t0, space 0
params[1].InitAsDescriptorTable(1, &srvRange);
```

---

### Bug #8: Root Constants Size Mismatch

**Symptoms:**
- Shader receives garbage data
- Only first N values correct

**Root Cause:**
Root constant count doesn't match shader struct size.

**Fix Pattern:**
```cpp
// Shader struct
struct Constants {
    float4x4 worldViewProj;  // 16 floats
    float4 color;            // 4 floats
    float time;              // 1 float
    // Padding to 4-float boundary
};  // Total: 21 floats, rounds to 24

// Root signature must match
params[0].InitAsConstants(24, 0, 0);  // 24 DWORDs = 24 floats

// Set correctly
cmdList->SetGraphicsRoot32BitConstants(0, 24, &constants, 0);
```

---

## Synchronization Issues

### Bug #9: COMMAND_ALLOCATOR_SYNC

**Debug Layer Message:**
```
D3D12 ERROR: ID3D12CommandAllocator::Reset: Cannot reset allocator while
commands are still in flight on GPU.
```

**Symptoms:**
- Crash on second frame
- Corruption after a few frames

**Root Cause:**
Resetting command allocator before GPU finishes executing its commands.

**Fix Pattern:**
```cpp
// Use multiple allocators (one per frame in flight)
class CommandAllocatorPool {
    static constexpr int FRAMES_IN_FLIGHT = 3;
    ComPtr<ID3D12CommandAllocator> allocators[FRAMES_IN_FLIGHT];
    UINT64 fenceValues[FRAMES_IN_FLIGHT] = {};
    int currentFrame = 0;

public:
    ID3D12CommandAllocator* GetAllocator() {
        // Wait if GPU hasn't finished with this allocator
        if (fence->GetCompletedValue() < fenceValues[currentFrame]) {
            fence->SetEventOnCompletion(fenceValues[currentFrame], event);
            WaitForSingleObject(event, INFINITE);
        }
        allocators[currentFrame]->Reset();
        return allocators[currentFrame].Get();
    }

    void FinishFrame(ID3D12CommandQueue* queue) {
        fenceValues[currentFrame] = ++currentFenceValue;
        queue->Signal(fence.Get(), currentFenceValue);
        currentFrame = (currentFrame + 1) % FRAMES_IN_FLIGHT;
    }
};
```

---

### Bug #10: OBJECT_ACCESSED_WHILE_STILL_IN_USE

**Debug Layer Message:**
```
D3D12 ERROR: Object accessed while still in use by GPU.
```

**Symptoms:**
- Resource corruption
- GPU crash
- Intermittent failures

**Root Cause:**
Modifying or releasing a resource while GPU is still using it.

**Fix Pattern:**
```cpp
// Defer resource destruction
class DeferredDeleter {
    struct PendingDelete {
        ComPtr<IUnknown> resource;
        UINT64 fenceValue;
    };
    std::queue<PendingDelete> pending;

public:
    void Delete(ComPtr<IUnknown> resource) {
        pending.push({std::move(resource), currentFenceValue});
    }

    void ProcessDeletions(ID3D12Fence* fence) {
        UINT64 completed = fence->GetCompletedValue();
        while (!pending.empty() && pending.front().fenceValue <= completed) {
            pending.pop();  // Release happens here
        }
    }
};
```

---

### Bug #11: Multi-Queue Synchronization

**Symptoms:**
- Copy operations not complete before use
- Compute results not ready for graphics

**Root Cause:**
Missing fence synchronization between queues.

**Fix Pattern:**
```cpp
// After copy on copy queue
copyQueue->Signal(copyFence.Get(), ++copyFenceValue);

// Before using on graphics queue
graphicsQueue->Wait(copyFence.Get(), copyFenceValue);

// Now safe to use copied resource
cmdList->SetGraphicsRootDescriptorTable(0, copiedTextureSRV);
```

---

## Memory Issues

### Bug #12: Map/Unmap While GPU Writing

**Debug Layer Message:**
```
D3D12 WARNING: Resource is mapped while GPU is still writing to it.
```

**Symptoms:**
- Data corruption
- Reading stale data
- Undefined behavior

**Fix Pattern:**
```cpp
// Use UPLOAD heap for CPU->GPU
// Use READBACK heap for GPU->CPU

// For GPU->CPU reads, wait for GPU
void ReadbackBuffer(ID3D12Resource* readbackBuffer, void* dest, size_t size) {
    // Wait for GPU to finish writing
    WaitForGPU();

    void* data;
    D3D12_RANGE readRange = {0, size};
    readbackBuffer->Map(0, &readRange, &data);
    memcpy(dest, data, size);

    D3D12_RANGE writeRange = {0, 0};  // Didn't write
    readbackBuffer->Unmap(0, &writeRange);
}
```

---

### Bug #13: Placed Resource Aliasing

**Symptoms:**
- Texture shows wrong content
- Random corruption

**Root Cause:**
Using aliased resources without proper barriers.

**Fix Pattern:**
```cpp
// When switching between aliased resources
D3D12_RESOURCE_ALIASING_BARRIER aliasBarrier = {
    .pResourceBefore = oldResource,
    .pResourceAfter = newResource
};
cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Aliasing(
    oldResource, newResource));
```

---

## Pipeline State Issues

### Bug #14: PSO Shader Compilation Failure

**Symptoms:**
- PSO creation returns E_FAIL
- No specific error message

**Debug Approach:**
```cpp
ComPtr<ID3DBlob> errorBlob;
HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc,
    D3D_ROOT_SIGNATURE_VERSION_1_0, &signatureBlob, &errorBlob);

if (FAILED(hr)) {
    OutputDebugStringA((char*)errorBlob->GetBufferPointer());
}

// For shader compilation
hr = D3DCompileFromFile(filename, nullptr, nullptr, "main", "vs_5_0",
    D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
    0, &shaderBlob, &errorBlob);

if (FAILED(hr)) {
    OutputDebugStringA((char*)errorBlob->GetBufferPointer());
}
```

---

### Bug #15: Input Layout Mismatch

**Symptoms:**
- Missing vertex attributes
- Vertices in wrong positions
- Black or incorrect colors

**Fix Pattern:**
```cpp
// Shader input
// struct VSInput {
//     float3 position : POSITION;
//     float3 normal : NORMAL;
//     float2 texcoord : TEXCOORD;
// };

// Must match exactly
D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
};
```

---

## Command List Errors

### Bug #16: Command List Not Closed

**Debug Layer Message:**
```
D3D12 ERROR: ID3D12CommandQueue::ExecuteCommandLists: Command list not closed.
```

**Fix:**
```cpp
// Always close before execute
cmdList->Close();
ID3D12CommandList* lists[] = { cmdList.Get() };
queue->ExecuteCommandLists(1, lists);
```

---

### Bug #17: Command List Reset Without Close

**Debug Layer Message:**
```
D3D12 ERROR: ID3D12GraphicsCommandList::Reset: Command list was not closed.
```

**Fix:**
```cpp
// Close before reset
cmdList->Close();  // Even if empty
cmdList->Reset(allocator, pso);
```

---

## TDR / Device Removal

### Bug #18: GPU Timeout (TDR)

**Symptoms:**
- Screen goes black
- Device removed error
- System recovers after few seconds

**Common Causes:**
1. Infinite loop in shader
2. Very large dispatch/draw
3. Resource access violation
4. Deadlock in GPU work

**Investigation Steps:**

1. **Enable DRED:**
```cpp
ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dredSettings;
D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings));
dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
```

2. **Add PIX Markers:**
```cpp
PIXBeginEvent(cmdList, PIX_COLOR(255, 0, 0), "Suspicious Draw");
// ... draw call ...
PIXEndEvent(cmdList);
```

3. **Check DRED After Crash:**
```cpp
void OnDeviceRemoved() {
    HRESULT reason = device->GetDeviceRemovedReason();

    ComPtr<ID3D12DeviceRemovedExtendedData> dred;
    device->QueryInterface(IID_PPV_ARGS(&dred));

    D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT breadcrumbs;
    dred->GetAutoBreadcrumbsOutput(&breadcrumbs);

    // Last completed: breadcrumbs.pHeadAutoBreadcrumbNode
    // Log to file for analysis
}
```

---

### Bug #19: Page Fault

**Symptoms:**
- Immediate crash
- DRED shows page fault info

**Common Causes:**
1. Accessing deleted resource
2. Out-of-bounds access
3. Wrong resource in descriptor

**Investigation:**
```cpp
D3D12_DRED_PAGE_FAULT_OUTPUT pageFault;
dred->GetPageFaultAllocationOutput(&pageFault);

// pageFault.PageFaultVA - Virtual address that faulted
// Check against resource allocations to identify culprit
```

---

## Prevention Strategies

### 1. Always Run with Debug Layer in Development

```cpp
#ifdef _DEBUG
    EnableDebugLayer();
    EnableGPUBasedValidation();
    EnableDRED();
#endif
```

### 2. Fix Warnings, Not Just Errors

Many crashes start as warnings. Don't ignore:
- Resource state warnings
- Descriptor validation warnings
- Synchronization warnings

### 3. Use PIX Validation

Run GPU validation in PIX periodically:
1. Take GPU capture
2. Click "Run GPU Validation"
3. Fix all reported issues

### 4. Implement Resource State Tracking

```cpp
class ResourceStateTracker {
    std::unordered_map<ID3D12Resource*, D3D12_RESOURCE_STATES> states;

public:
    void Transition(ID3D12GraphicsCommandList* cmdList,
                    ID3D12Resource* resource,
                    D3D12_RESOURCE_STATES newState) {
        auto& current = states[resource];
        if (current != newState) {
            cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                resource, current, newState));
            current = newState;
        }
    }
};
```

### 5. Test on Multiple GPU Vendors

Bugs often manifest differently on NVIDIA vs AMD vs Intel. Test on at least two different GPU vendors.

---

## Quick Debug Checklist

When something goes wrong:

- [ ] Check debug layer output first
- [ ] Verify all resources are in correct state
- [ ] Confirm descriptors are initialized
- [ ] Check root signature matches shaders
- [ ] Verify synchronization (fences, barriers)
- [ ] Test with GPU-based validation
- [ ] Capture frame in PIX for analysis
- [ ] Check DRED output if crash/TDR

---

## Additional Resources

- [D3D12 Debug Layer](https://learn.microsoft.com/en-us/windows/win32/direct3d12/understanding-the-d3d12-debug-layer)
- [GPU-Based Validation](https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-d3d12-debug-layer-gpu-based-validation)
- [DRED Documentation](https://microsoft.github.io/DirectX-Specs/d3d/DeviceRemovedExtendedData.html)
- [Enhanced Barriers](https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html)
- [PIX GPU Captures](https://devblogs.microsoft.com/pix/gpu-captures/)
