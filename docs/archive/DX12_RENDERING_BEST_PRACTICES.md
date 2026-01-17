# DirectX 12 Rendering Best Practices Reference

This document compiles best practices from official Microsoft DirectX 12 samples, DirectXTK12, and professional engine implementations. While our project uses OpenGL, these patterns represent industry-standard rendering techniques that can inform graphics programming decisions.

## Important Note

**Our project (OrganismEvolution) uses OpenGL 3.3**, not DirectX 12. This document serves as a reference for DirectX 12 patterns that could inform future architecture decisions or potential ports.

---

## 1. Microsoft D3D12HelloWorld Samples

### Reference Implementation: PopulateCommandList()

From the official [D3D12HelloTriangle sample](https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloTriangle/D3D12HelloTriangle.cpp):

```cpp
void D3D12HelloTriangle::PopulateCommandList()
{
    // Command list allocators can only be reset when the associated
    // command lists have finished execution on the GPU
    ThrowIfFailed(m_commandAllocator->Reset());

    // Reset command list for re-recording
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

    // Set necessary state
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // CRITICAL: Transition back buffer to render target state
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[m_frameIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Get render target handle
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
        m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
        m_frameIndex,
        m_rtvDescriptorSize);

    // Bind render target
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Clear render target
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // Draw commands...
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_commandList->DrawInstanced(3, 1, 0, 0);

    // Transition back buffer to present state
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[m_frameIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT));

    ThrowIfFailed(m_commandList->Close());
}
```

### Key Ordering Pattern

1. Reset command allocator/list
2. Set pipeline state (PSO, root signature)
3. Set viewport and scissor
4. **Resource barrier: PRESENT -> RENDER_TARGET**
5. **OMSetRenderTargets** (bind RTV and optionally DSV)
6. **ClearRenderTargetView**
7. **ClearDepthStencilView** (if using depth)
8. Draw commands
9. **Resource barrier: RENDER_TARGET -> PRESENT**
10. Close command list

---

## 2. DirectXTK12 DeviceResources Pattern

### Reference: [DirectXTK12 DeviceResources Wiki](https://github.com/microsoft/DirectXTK12/wiki/DeviceResources)

### Core Architecture

```cpp
class DeviceResources {
public:
    // Initialization
    void CreateDeviceResources();
    void CreateWindowSizeDependentResources();

    // Frame management
    void Prepare(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_PRESENT,
                 D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_RENDER_TARGET);
    void Present(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_RENDER_TARGET);
    void WaitForGpu();

    // Accessors
    ID3D12Device* GetDevice() const;
    ID3D12CommandQueue* GetCommandQueue() const;
    ID3D12CommandAllocator* GetCommandAllocator() const;
    ID3D12GraphicsCommandList* GetCommandList() const;
    ID3D12Resource* GetRenderTarget() const;
    ID3D12Resource* GetDepthStencil() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;
};
```

### Initialization Pattern

```cpp
Game::Game() noexcept(false) {
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM,           // backBufferFormat
        DXGI_FORMAT_D32_FLOAT,                 // depthBufferFormat
        2,                                      // backBufferCount
        D3D_FEATURE_LEVEL_11_0                 // minFeatureLevel
    );
    m_deviceResources->RegisterDeviceNotify(this);
}

void Game::Initialize(HWND window, int width, int height) {
    m_deviceResources->SetWindow(window, width, height);
    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();
    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
}
```

### Render Loop Pattern

```cpp
void Game::Render() {
    if (m_timer.GetFrameCount() == 0)
        return;

    // Prepare the command list
    m_deviceResources->Prepare();

    auto commandList = m_deviceResources->GetCommandList();

    // Clear render targets
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->ClearRenderTargetView(rtvDescriptor, Colors::CornflowerBlue, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);

    // Set viewport and scissor
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    // Draw scene here...

    // Present
    m_deviceResources->Present();
}
```

---

## 3. ClearRenderTargetView Best Practices

### Reference: [Microsoft Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-clearrendertargetview)

### Function Signature

```cpp
void ClearRenderTargetView(
    D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView,
    const FLOAT ColorRGBA[4],
    UINT NumRects,
    const D3D12_RECT *pRects
);
```

### Requirements

| Requirement | Details |
|-------------|---------|
| Resource State | Must be `D3D12_RESOURCE_STATE_RENDER_TARGET` |
| Descriptor Type | `D3D12_DESCRIPTOR_HEAP_TYPE_RTV` |
| Float Handling | Denormalized values set to 0 (NaNs preserved) |

### Best Practices

1. **Match Clear Value to Resource Creation**
   ```cpp
   // When creating the resource, specify optimized clear value
   D3D12_CLEAR_VALUE clearValue = {};
   clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
   clearValue.Color[0] = 0.0f;
   clearValue.Color[1] = 0.2f;
   clearValue.Color[2] = 0.4f;
   clearValue.Color[3] = 1.0f;

   device->CreateCommittedResource(
       &heapProps,
       D3D12_HEAP_FLAG_NONE,
       &resourceDesc,
       D3D12_RESOURCE_STATE_RENDER_TARGET,
       &clearValue,  // Optimized clear value
       IID_PPV_ARGS(&renderTarget));
   ```

2. **Clear Before OMSetRenderTargets or After**
   Both orderings are valid, but Microsoft samples typically:
   - Call `OMSetRenderTargets` first
   - Then call `ClearRenderTargetView`

3. **Avoid Denormalized Float Values**
   The debug layer issues errors for denormalized input colors.

---

## 4. ClearDepthStencilView Best Practices

### Reference: [Microsoft Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-cleardepthstencilview)

### Function Signature

```cpp
void ClearDepthStencilView(
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView,
    D3D12_CLEAR_FLAGS ClearFlags,
    FLOAT Depth,
    UINT8 Stencil,
    UINT NumRects,
    const D3D12_RECT *pRects
);
```

### Clear Flags

| Flag | Description |
|------|-------------|
| `D3D12_CLEAR_FLAG_DEPTH` | Clear depth buffer only |
| `D3D12_CLEAR_FLAG_STENCIL` | Clear stencil buffer only |
| Both flags combined | Clear both buffers |

### Requirements

| Requirement | Details |
|-------------|---------|
| Resource State | Must be `D3D12_RESOURCE_STATE_DEPTH_WRITE` |
| Depth Value | Clamped between 0.0 and 1.0 |
| Typical Clear Value | 1.0f (furthest from camera) |

### Best Practice Example

```cpp
// Create depth buffer with optimized clear value
D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
depthOptimizedClearValue.DepthStencil.Stencil = 0;

device->CreateCommittedResource(
    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
    D3D12_HEAP_FLAG_NONE,
    &CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_D32_FLOAT,
        width, height,
        1, 0, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
    D3D12_RESOURCE_STATE_DEPTH_WRITE,
    &depthOptimizedClearValue,
    IID_PPV_ARGS(&depthStencilBuffer));

// Later, in render loop
commandList->ClearDepthStencilView(
    dsvHandle,
    D3D12_CLEAR_FLAG_DEPTH,
    1.0f,    // Must match optimized clear value
    0,
    0,
    nullptr);
```

---

## 5. OMSetRenderTargets Best Practices

### Reference: [Microsoft Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-omsetrendertargets)

### Function Signature

```cpp
void OMSetRenderTargets(
    UINT NumRenderTargetDescriptors,
    const D3D12_CPU_DESCRIPTOR_HANDLE *pRenderTargetDescriptors,
    BOOL RTsSingleHandleToDescriptorRange,
    const D3D12_CPU_DESCRIPTOR_HANDLE *pDepthStencilDescriptor
);
```

### RTsSingleHandleToDescriptorRange Parameter

| Value | Description |
|-------|-------------|
| TRUE | Handle points to contiguous range of descriptors (more efficient) |
| FALSE | Handle is first of array of separate descriptor handles (more flexible) |

### Usage Patterns

**Single Render Target with Depth:**
```cpp
CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvHeap->GetCPUDescriptorHandleForHeapStart());

commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
```

**Multiple Render Targets (MRT):**
```cpp
D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[4];
for (int i = 0; i < 4; i++) {
    rtvHandles[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(
        rtvHeap->GetCPUDescriptorHandleForHeapStart(),
        i, rtvDescriptorSize);
}
commandList->OMSetRenderTargets(4, rtvHandles, FALSE, &dsvHandle);
```

### Timing Best Practice

> "PSOs need the Format information of RTV and DSV, thus you'd better defer the OMSetRenderTargets operation to the time when the PSO is set."

---

## 6. Frame Pacing and Synchronization

### Reference: [3D Game Engine Programming Tutorial](https://www.3dgep.com/learning-directx-12-2/)

### Fence-Based Synchronization

```cpp
// Member variables
ID3D12Fence* m_fence;
UINT64 m_fenceValue;
UINT64 m_frameFenceValues[BufferCount];
HANDLE m_fenceEvent;

// Initialization
device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

// Signal fence after executing command list
UINT64 SignalFence(ID3D12CommandQueue* commandQueue, ID3D12Fence* fence, UINT64& fenceValue) {
    UINT64 fenceValueForSignal = ++fenceValue;
    ThrowIfFailed(commandQueue->Signal(fence, fenceValueForSignal));
    return fenceValueForSignal;
}

// Wait for fence value
void WaitForFenceValue(ID3D12Fence* fence, UINT64 fenceValue, HANDLE fenceEvent) {
    if (fence->GetCompletedValue() < fenceValue) {
        ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
        WaitForSingleObject(fenceEvent, INFINITE);
    }
}

// Frame synchronization pattern
void Present() {
    // Present the frame
    m_swapChain->Present(1, 0);

    // Signal fence for current frame
    m_frameFenceValues[m_currentBackBufferIndex] = SignalFence(
        m_commandQueue, m_fence, m_fenceValue);

    // Get next back buffer index
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Wait for previous frame on this buffer to complete
    WaitForFenceValue(m_fence, m_frameFenceValues[m_currentBackBufferIndex], m_fenceEvent);
}
```

### Double vs Triple Buffering

| Buffering | Pros | Cons |
|-----------|------|------|
| Double (2) | Lower latency, less VRAM | Potential stuttering |
| Triple (3) | Smoother frame delivery | Higher latency, more VRAM |

---

## 7. Resource State Transitions

### Reference: [DirectXTK12 Resource Barriers](https://github.com/microsoft/DirectXTK12/wiki/Resource-Barriers)

### Common State Transitions

| Transition | Usage |
|------------|-------|
| PRESENT -> RENDER_TARGET | Beginning of frame |
| RENDER_TARGET -> PRESENT | End of frame, before Present() |
| COPY_DEST -> PIXEL_SHADER_RESOURCE | After texture upload |
| DEPTH_WRITE -> DEPTH_READ | Shadow mapping pass |

### Barrier Example

```cpp
// Begin rendering
CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
    m_renderTargets[m_frameIndex].Get(),
    D3D12_RESOURCE_STATE_PRESENT,
    D3D12_RESOURCE_STATE_RENDER_TARGET);
commandList->ResourceBarrier(1, &barrier);

// ... draw commands ...

// End rendering
barrier = CD3DX12_RESOURCE_BARRIER::Transition(
    m_renderTargets[m_frameIndex].Get(),
    D3D12_RESOURCE_STATE_RENDER_TARGET,
    D3D12_RESOURCE_STATE_PRESENT);
commandList->ResourceBarrier(1, &barrier);
```

---

## 8. Comparison: DirectX 12 vs OpenGL (Our Setup)

### Our Current OpenGL Implementation

```cpp
// main.cpp render loop
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // Clear both buffers
shader.use();
// Set uniforms...
simulation->render(camera);
glfwSwapBuffers(window);
```

### Key Differences

| Aspect | DirectX 12 | OpenGL (Our Setup) |
|--------|------------|-------------------|
| Resource States | Explicit barriers | Implicit/driver managed |
| Command Recording | Deferred, multi-threaded | Immediate mode |
| Synchronization | Explicit fences | Implicit with swap |
| Clear Operations | Separate RTV/DSV clears | Combined glClear() |
| Render Targets | Explicit binding | Implicit framebuffer |

### What We're Doing Correctly (OpenGL equivalent)

1. **Depth testing enabled**: `glEnable(GL_DEPTH_TEST)`
2. **Clear both buffers**: `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)`
3. **Set clear color**: `glClearColor(0.53f, 0.81f, 0.92f, 1.0f)`
4. **VSync via swap**: `glfwSwapBuffers(window)`

### Potential Improvements Based on DX12 Patterns

1. **Explicit frame timing** - Could add fence-like timing for profiling
2. **Batch draw calls** - Already doing with instanced rendering
3. **State caching** - Could reduce redundant state changes
4. **Resource state tracking** - Internal tracking for debugging

---

## Sources

- [D3D12HelloWorld Samples - Microsoft Learn](https://learn.microsoft.com/en-us/samples/microsoft/directx-graphics-samples/d3d12-hello-world-samples-win32/)
- [DirectXTK12 DeviceResources Wiki](https://github.com/microsoft/DirectXTK12/wiki/DeviceResources)
- [ClearRenderTargetView Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-clearrendertargetview)
- [ClearDepthStencilView Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-cleardepthstencilview)
- [OMSetRenderTargets Documentation](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-omsetrendertargets)
- [Learning DirectX 12 - Rendering Tutorial](https://www.3dgep.com/learning-directx-12-2/)
- [Braynzar Soft - DirectX 12 Depth Testing](https://www.braynzarsoft.net/viewtutorial/q16390-directx-12-depth-testing)
- [DirectX Graphics Samples GitHub](https://github.com/Microsoft/DirectX-Graphics-Samples)
- [Secrets of Direct3D 12: RTV and DSV Descriptors](https://asawicki.info/news_1772_secrets_of_direct3d_12_do_rtv_and_dsv_descriptors_make_any_sense)
