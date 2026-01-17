# Validation Patterns for DirectX 12

Comprehensive validation patterns for parameter validation, state validation, resource validation, and performance considerations in DX12 applications.

## Table of Contents

1. [Parameter Validation](#parameter-validation)
2. [State Validation](#state-validation)
3. [Resource Validation](#resource-validation)
4. [Descriptor Validation](#descriptor-validation)
5. [Command List Validation](#command-list-validation)
6. [Synchronization Validation](#synchronization-validation)
7. [Memory Validation](#memory-validation)
8. [Debug Layer Integration](#debug-layer-integration)
9. [Performance Considerations](#performance-considerations)
10. [Validation Macros & Utilities](#validation-macros--utilities)

---

## Parameter Validation

### Basic Parameter Checks

Always validate parameters at API boundaries to catch errors early.

```cpp
// Null pointer validation
#define VALIDATE_PTR(ptr, name)                                         \
    do {                                                                \
        if ((ptr) == nullptr) {                                         \
            LOG_ERROR("Null pointer: {}", name);                        \
            return E_POINTER;                                           \
        }                                                               \
    } while(0)

// Range validation
#define VALIDATE_RANGE(value, min, max, name)                           \
    do {                                                                \
        if ((value) < (min) || (value) > (max)) {                       \
            LOG_ERROR("{} out of range: {} (expected {}-{})",           \
                      name, value, min, max);                           \
            return E_INVALIDARG;                                        \
        }                                                               \
    } while(0)

// Alignment validation
#define VALIDATE_ALIGNMENT(value, alignment, name)                      \
    do {                                                                \
        if (((value) & ((alignment) - 1)) != 0) {                       \
            LOG_ERROR("{} not aligned to {}: {}", name, alignment, value); \
            return E_INVALIDARG;                                        \
        }                                                               \
    } while(0)
```

### Buffer Size Validation

```cpp
HRESULT CreateBuffer(const BufferDesc& desc, ID3D12Resource** ppBuffer) {
    // Validate output pointer
    VALIDATE_PTR(ppBuffer, "ppBuffer");
    *ppBuffer = nullptr;

    // Validate size
    if (desc.size == 0) {
        LOG_ERROR("Buffer size cannot be zero");
        return E_INVALIDARG;
    }

    // Check maximum size (typical limit ~2GB for buffers)
    constexpr UINT64 MAX_BUFFER_SIZE = 2ULL * 1024 * 1024 * 1024;
    if (desc.size > MAX_BUFFER_SIZE) {
        LOG_ERROR("Buffer size {} exceeds maximum {}", desc.size, MAX_BUFFER_SIZE);
        return E_INVALIDARG;
    }

    // Alignment requirements for constant buffers
    if (desc.usage == BufferUsage::ConstantBuffer) {
        if ((desc.size % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) != 0) {
            LOG_WARN("Constant buffer size {} not aligned to {}, rounding up",
                     desc.size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
            // Auto-align if desired, or return error
        }
    }

    // Proceed with creation...
    return CreateBufferInternal(desc, ppBuffer);
}
```

### Format Validation

```cpp
struct FormatInfo {
    DXGI_FORMAT format;
    UINT bytesPerPixel;
    bool isCompressed;
    bool isDepth;
    bool isTypeless;
};

bool IsValidTextureFormat(DXGI_FORMAT format) {
    switch (format) {
        // Valid color formats
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        // Compressed formats
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC7_UNORM:
            return true;

        case DXGI_FORMAT_UNKNOWN:
            return false;

        default:
            // Query device for format support
            return true;  // Assume valid, let D3D12 validate
    }
}

bool IsValidDepthFormat(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return true;
        default:
            return false;
    }
}

HRESULT ValidateTextureDesc(const TextureDesc& desc) {
    // Format validation
    if (!IsValidTextureFormat(desc.format)) {
        LOG_ERROR("Invalid texture format: {}", (int)desc.format);
        return E_INVALIDARG;
    }

    // Dimension validation
    if (desc.width == 0 || desc.height == 0) {
        LOG_ERROR("Invalid texture dimensions: {}x{}", desc.width, desc.height);
        return E_INVALIDARG;
    }

    // Max dimensions (typically 16384 for most GPUs)
    D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
    m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));

    constexpr UINT MAX_TEXTURE_DIM = 16384;
    if (desc.width > MAX_TEXTURE_DIM || desc.height > MAX_TEXTURE_DIM) {
        LOG_ERROR("Texture dimensions exceed maximum: {}x{} > {}",
                  desc.width, desc.height, MAX_TEXTURE_DIM);
        return E_INVALIDARG;
    }

    // Mip level validation
    UINT maxMips = CalculateMaxMipLevels(desc.width, desc.height);
    if (desc.mipLevels > maxMips) {
        LOG_WARN("Requested {} mip levels, maximum is {}. Clamping.",
                 desc.mipLevels, maxMips);
    }

    // Array size validation
    if (desc.arraySize == 0 || desc.arraySize > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) {
        LOG_ERROR("Invalid array size: {}", desc.arraySize);
        return E_INVALIDARG;
    }

    // Sample count validation (for MSAA)
    if (desc.sampleCount > 1) {
        if (!IsPowerOfTwo(desc.sampleCount) || desc.sampleCount > 16) {
            LOG_ERROR("Invalid sample count: {}", desc.sampleCount);
            return E_INVALIDARG;
        }
    }

    return S_OK;
}
```

---

## State Validation

### Resource State Tracking

```cpp
class ResourceStateTracker {
public:
    void SetInitialState(ID3D12Resource* resource, D3D12_RESOURCE_STATES state) {
        m_resourceStates[resource] = state;
    }

    void TransitionResource(ID3D12GraphicsCommandList* cmdList,
                           ID3D12Resource* resource,
                           D3D12_RESOURCE_STATES newState) {
        auto it = m_resourceStates.find(resource);

        if (it == m_resourceStates.end()) {
            LOG_ERROR("Resource not tracked! Cannot determine current state.");
            DEBUG_BREAK();
            return;
        }

        D3D12_RESOURCE_STATES currentState = it->second;

        // Validate transition
        if (!IsValidTransition(currentState, newState)) {
            LOG_ERROR("Invalid state transition: {} -> {}",
                      GetStateName(currentState), GetStateName(newState));
            DEBUG_BREAK();
            return;
        }

        // Skip if already in target state
        if (currentState == newState) {
            LOG_TRACE("Resource already in state {}", GetStateName(newState));
            return;
        }

        // Create and record barrier
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = resource;
        barrier.Transition.StateBefore = currentState;
        barrier.Transition.StateAfter = newState;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        cmdList->ResourceBarrier(1, &barrier);

        // Update tracked state
        it->second = newState;
    }

    D3D12_RESOURCE_STATES GetCurrentState(ID3D12Resource* resource) const {
        auto it = m_resourceStates.find(resource);
        if (it == m_resourceStates.end()) {
            LOG_ERROR("Resource not being tracked!");
            return D3D12_RESOURCE_STATE_COMMON;
        }
        return it->second;
    }

#ifdef _DEBUG
    void ValidateState(ID3D12Resource* resource, D3D12_RESOURCE_STATES expectedState) {
        D3D12_RESOURCE_STATES actual = GetCurrentState(resource);
        if (actual != expectedState) {
            LOG_ERROR("Resource state mismatch! Expected: {}, Actual: {}",
                      GetStateName(expectedState), GetStateName(actual));
            DEBUG_BREAK();
        }
    }
#else
    void ValidateState(ID3D12Resource*, D3D12_RESOURCE_STATES) {}
#endif

private:
    bool IsValidTransition(D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to) {
        // Some states can be combined, some cannot
        // This is a simplified check
        return true;  // Let D3D12 debug layer catch invalid transitions
    }

    const char* GetStateName(D3D12_RESOURCE_STATES state) {
        switch (state) {
            case D3D12_RESOURCE_STATE_COMMON: return "COMMON";
            case D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER: return "VERTEX_AND_CONSTANT_BUFFER";
            case D3D12_RESOURCE_STATE_INDEX_BUFFER: return "INDEX_BUFFER";
            case D3D12_RESOURCE_STATE_RENDER_TARGET: return "RENDER_TARGET";
            case D3D12_RESOURCE_STATE_UNORDERED_ACCESS: return "UNORDERED_ACCESS";
            case D3D12_RESOURCE_STATE_DEPTH_WRITE: return "DEPTH_WRITE";
            case D3D12_RESOURCE_STATE_DEPTH_READ: return "DEPTH_READ";
            case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE: return "NON_PIXEL_SHADER_RESOURCE";
            case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE: return "PIXEL_SHADER_RESOURCE";
            case D3D12_RESOURCE_STATE_COPY_DEST: return "COPY_DEST";
            case D3D12_RESOURCE_STATE_COPY_SOURCE: return "COPY_SOURCE";
            case D3D12_RESOURCE_STATE_PRESENT: return "PRESENT";
            default: return "UNKNOWN";
        }
    }

    std::unordered_map<ID3D12Resource*, D3D12_RESOURCE_STATES> m_resourceStates;
};
```

### Pipeline State Validation

```cpp
class PipelineStateValidator {
public:
    HRESULT ValidatePSODesc(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc) {
        // Must have a root signature
        if (desc.pRootSignature == nullptr) {
            LOG_ERROR("PSO: Root signature is null");
            return E_INVALIDARG;
        }

        // Must have a vertex shader
        if (desc.VS.pShaderBytecode == nullptr || desc.VS.BytecodeLength == 0) {
            LOG_ERROR("PSO: Vertex shader is missing");
            return E_INVALIDARG;
        }

        // If using input layout, validate it
        if (desc.InputLayout.NumElements > 0) {
            HRESULT hr = ValidateInputLayout(desc.InputLayout);
            if (FAILED(hr)) return hr;
        }

        // Validate render target formats
        for (UINT i = 0; i < desc.NumRenderTargets; ++i) {
            if (desc.RTVFormats[i] == DXGI_FORMAT_UNKNOWN) {
                LOG_ERROR("PSO: RTV format {} is UNKNOWN", i);
                return E_INVALIDARG;
            }
        }

        // Validate depth format if depth is enabled
        if (desc.DepthStencilState.DepthEnable) {
            if (!IsValidDepthFormat(desc.DSVFormat)) {
                LOG_ERROR("PSO: Invalid depth format: {}", (int)desc.DSVFormat);
                return E_INVALIDARG;
            }
        }

        // Validate sample count matches across all targets
        if (desc.SampleDesc.Count > 1) {
            // MSAA enabled - ensure it's supported
            if (!IsPowerOfTwo(desc.SampleDesc.Count)) {
                LOG_ERROR("PSO: Sample count must be power of 2: {}",
                          desc.SampleDesc.Count);
                return E_INVALIDARG;
            }
        }

        return S_OK;
    }

private:
    HRESULT ValidateInputLayout(const D3D12_INPUT_LAYOUT_DESC& layout) {
        std::set<std::string> usedSemantics;

        for (UINT i = 0; i < layout.NumElements; ++i) {
            const auto& elem = layout.pInputElementDescs[i];

            // Check semantic name
            if (elem.SemanticName == nullptr || strlen(elem.SemanticName) == 0) {
                LOG_ERROR("Input element {} has empty semantic name", i);
                return E_INVALIDARG;
            }

            // Check for duplicate semantics
            std::string semantic = std::string(elem.SemanticName) +
                                   std::to_string(elem.SemanticIndex);
            if (usedSemantics.count(semantic)) {
                LOG_ERROR("Duplicate semantic: {}", semantic);
                return E_INVALIDARG;
            }
            usedSemantics.insert(semantic);

            // Validate format
            if (elem.Format == DXGI_FORMAT_UNKNOWN) {
                LOG_ERROR("Input element {} has UNKNOWN format", i);
                return E_INVALIDARG;
            }

            // Validate alignment
            if (elem.AlignedByteOffset != D3D12_APPEND_ALIGNED_ELEMENT) {
                if (elem.AlignedByteOffset % 4 != 0) {
                    LOG_WARN("Input element {} has non-4-byte aligned offset: {}",
                             i, elem.AlignedByteOffset);
                }
            }
        }

        return S_OK;
    }
};
```

---

## Resource Validation

### Buffer Validation

```cpp
class BufferValidator {
public:
    HRESULT ValidateVertexBuffer(ID3D12Resource* buffer, UINT stride,
                                 UINT expectedVertexCount) {
#ifdef _DEBUG
        if (buffer == nullptr) {
            LOG_ERROR("Vertex buffer is null");
            return E_POINTER;
        }

        D3D12_RESOURCE_DESC desc = buffer->GetDesc();

        // Must be a buffer
        if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
            LOG_ERROR("Resource is not a buffer");
            return E_INVALIDARG;
        }

        // Check size
        UINT64 expectedSize = (UINT64)stride * expectedVertexCount;
        if (desc.Width < expectedSize) {
            LOG_ERROR("Vertex buffer too small: {} bytes, expected at least {}",
                      desc.Width, expectedSize);
            return E_INVALIDARG;
        }

        // Stride validation
        if (stride == 0) {
            LOG_ERROR("Vertex stride cannot be zero");
            return E_INVALIDARG;
        }

        if (stride > 2048) {
            LOG_WARN("Vertex stride {} is unusually large", stride);
        }
#endif
        return S_OK;
    }

    HRESULT ValidateIndexBuffer(ID3D12Resource* buffer, DXGI_FORMAT format,
                                UINT expectedIndexCount) {
#ifdef _DEBUG
        if (buffer == nullptr) {
            LOG_ERROR("Index buffer is null");
            return E_POINTER;
        }

        // Validate format
        UINT indexSize;
        if (format == DXGI_FORMAT_R16_UINT) {
            indexSize = 2;
        } else if (format == DXGI_FORMAT_R32_UINT) {
            indexSize = 4;
        } else {
            LOG_ERROR("Invalid index buffer format: {}", (int)format);
            return E_INVALIDARG;
        }

        D3D12_RESOURCE_DESC desc = buffer->GetDesc();
        UINT64 expectedSize = (UINT64)indexSize * expectedIndexCount;

        if (desc.Width < expectedSize) {
            LOG_ERROR("Index buffer too small: {} bytes, expected at least {}",
                      desc.Width, expectedSize);
            return E_INVALIDARG;
        }
#endif
        return S_OK;
    }

    HRESULT ValidateConstantBuffer(ID3D12Resource* buffer, size_t expectedSize) {
#ifdef _DEBUG
        if (buffer == nullptr) {
            LOG_ERROR("Constant buffer is null");
            return E_POINTER;
        }

        D3D12_RESOURCE_DESC desc = buffer->GetDesc();

        // Check alignment
        if (desc.Width % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT != 0) {
            LOG_ERROR("Constant buffer not 256-byte aligned: {} bytes", desc.Width);
            return E_INVALIDARG;
        }

        // Check size (must be at least expected size, aligned up to 256)
        size_t alignedExpected = (expectedSize + 255) & ~255;
        if (desc.Width < alignedExpected) {
            LOG_ERROR("Constant buffer too small: {} bytes, expected at least {}",
                      desc.Width, alignedExpected);
            return E_INVALIDARG;
        }
#endif
        return S_OK;
    }
};
```

### Texture Validation

```cpp
class TextureValidator {
public:
    HRESULT ValidateTexture2D(ID3D12Resource* texture,
                              UINT expectedWidth, UINT expectedHeight,
                              DXGI_FORMAT expectedFormat) {
#ifdef _DEBUG
        if (texture == nullptr) {
            LOG_ERROR("Texture is null");
            return E_POINTER;
        }

        D3D12_RESOURCE_DESC desc = texture->GetDesc();

        if (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
            LOG_ERROR("Resource is not a 2D texture");
            return E_INVALIDARG;
        }

        if (desc.Width != expectedWidth || desc.Height != expectedHeight) {
            LOG_ERROR("Texture size mismatch: {}x{} vs expected {}x{}",
                      desc.Width, desc.Height, expectedWidth, expectedHeight);
            return E_INVALIDARG;
        }

        if (desc.Format != expectedFormat) {
            LOG_ERROR("Texture format mismatch: {} vs expected {}",
                      (int)desc.Format, (int)expectedFormat);
            return E_INVALIDARG;
        }
#endif
        return S_OK;
    }

    HRESULT ValidateRenderTarget(ID3D12Resource* rt) {
#ifdef _DEBUG
        if (rt == nullptr) {
            LOG_ERROR("Render target is null");
            return E_POINTER;
        }

        D3D12_RESOURCE_DESC desc = rt->GetDesc();

        // Must have render target flag
        if (!(desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)) {
            LOG_ERROR("Resource missing ALLOW_RENDER_TARGET flag");
            return E_INVALIDARG;
        }

        // Cannot be a buffer
        if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
            LOG_ERROR("Render target cannot be a buffer");
            return E_INVALIDARG;
        }
#endif
        return S_OK;
    }

    HRESULT ValidateDepthStencil(ID3D12Resource* ds) {
#ifdef _DEBUG
        if (ds == nullptr) {
            LOG_ERROR("Depth stencil is null");
            return E_POINTER;
        }

        D3D12_RESOURCE_DESC desc = ds->GetDesc();

        // Must have depth stencil flag
        if (!(desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) {
            LOG_ERROR("Resource missing ALLOW_DEPTH_STENCIL flag");
            return E_INVALIDARG;
        }

        // Format must be depth format
        if (!IsValidDepthFormat(desc.Format)) {
            LOG_ERROR("Invalid depth stencil format: {}", (int)desc.Format);
            return E_INVALIDARG;
        }
#endif
        return S_OK;
    }
};
```

---

## Descriptor Validation

```cpp
class DescriptorValidator {
public:
    void Initialize(ID3D12Device* device) {
        m_device = device;
        m_cbvSrvUavDescriptorSize = device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        m_samplerDescriptorSize = device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    }

    bool ValidateDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle,
                                  D3D12_DESCRIPTOR_HEAP_TYPE type) {
#ifdef _DEBUG
        // Handle should not be null
        if (handle.ptr == 0) {
            LOG_ERROR("Descriptor handle is null");
            return false;
        }

        // Check alignment based on type
        UINT alignment = GetDescriptorSize(type);
        if ((handle.ptr % alignment) != 0) {
            LOG_WARN("Descriptor handle not aligned to {}: 0x{:X}",
                     alignment, handle.ptr);
        }
#endif
        return true;
    }

    bool ValidateDescriptorHeap(ID3D12DescriptorHeap* heap,
                                D3D12_DESCRIPTOR_HEAP_TYPE expectedType,
                                UINT minDescriptors) {
#ifdef _DEBUG
        if (heap == nullptr) {
            LOG_ERROR("Descriptor heap is null");
            return false;
        }

        D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();

        if (desc.Type != expectedType) {
            LOG_ERROR("Descriptor heap type mismatch: {} vs {}",
                      (int)desc.Type, (int)expectedType);
            return false;
        }

        if (desc.NumDescriptors < minDescriptors) {
            LOG_ERROR("Descriptor heap too small: {} < {}",
                      desc.NumDescriptors, minDescriptors);
            return false;
        }

        // Shader visible heap check for CBV/SRV/UAV and samplers
        if ((expectedType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ||
             expectedType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) &&
            !(desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)) {
            LOG_WARN("CBV/SRV/UAV or Sampler heap should typically be shader visible");
        }
#endif
        return true;
    }

private:
    UINT GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE type) {
        switch (type) {
            case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV: return m_cbvSrvUavDescriptorSize;
            case D3D12_DESCRIPTOR_HEAP_TYPE_RTV: return m_rtvDescriptorSize;
            case D3D12_DESCRIPTOR_HEAP_TYPE_DSV: return m_dsvDescriptorSize;
            case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER: return m_samplerDescriptorSize;
            default: return 0;
        }
    }

    ID3D12Device* m_device = nullptr;
    UINT m_cbvSrvUavDescriptorSize = 0;
    UINT m_rtvDescriptorSize = 0;
    UINT m_dsvDescriptorSize = 0;
    UINT m_samplerDescriptorSize = 0;
};
```

---

## Command List Validation

```cpp
class CommandListValidator {
public:
    // Call at start of recording
    void BeginRecording(ID3D12GraphicsCommandList* cmdList) {
#ifdef _DEBUG
        m_isRecording = true;
        m_currentPSO = nullptr;
        m_currentRootSignature = nullptr;
        m_renderTargetsBound = false;
        m_viewportSet = false;
        m_scissorSet = false;
        m_vertexBufferBound = false;
#endif
    }

    // Validate before draw calls
    HRESULT ValidateDrawState() {
#ifdef _DEBUG
        if (!m_isRecording) {
            LOG_ERROR("Command list not recording");
            return E_FAIL;
        }

        if (m_currentPSO == nullptr) {
            LOG_ERROR("No PSO bound");
            return E_FAIL;
        }

        if (m_currentRootSignature == nullptr) {
            LOG_ERROR("No root signature bound");
            return E_FAIL;
        }

        if (!m_renderTargetsBound) {
            LOG_WARN("No render targets bound");
        }

        if (!m_viewportSet) {
            LOG_ERROR("Viewport not set");
            return E_FAIL;
        }

        if (!m_scissorSet) {
            LOG_ERROR("Scissor rect not set");
            return E_FAIL;
        }

        if (!m_vertexBufferBound) {
            LOG_WARN("No vertex buffer bound");
        }
#endif
        return S_OK;
    }

    // Validate before dispatch calls
    HRESULT ValidateDispatchState() {
#ifdef _DEBUG
        if (!m_isRecording) {
            LOG_ERROR("Command list not recording");
            return E_FAIL;
        }

        if (m_currentPSO == nullptr) {
            LOG_ERROR("No compute PSO bound");
            return E_FAIL;
        }

        if (m_currentRootSignature == nullptr) {
            LOG_ERROR("No root signature bound");
            return E_FAIL;
        }
#endif
        return S_OK;
    }

    // Track state changes
    void OnSetPipelineState(ID3D12PipelineState* pso) {
#ifdef _DEBUG
        m_currentPSO = pso;
#endif
    }

    void OnSetGraphicsRootSignature(ID3D12RootSignature* rs) {
#ifdef _DEBUG
        m_currentRootSignature = rs;
#endif
    }

    void OnOMSetRenderTargets() {
#ifdef _DEBUG
        m_renderTargetsBound = true;
#endif
    }

    void OnRSSetViewports() {
#ifdef _DEBUG
        m_viewportSet = true;
#endif
    }

    void OnRSSetScissorRects() {
#ifdef _DEBUG
        m_scissorSet = true;
#endif
    }

    void OnIASetVertexBuffers() {
#ifdef _DEBUG
        m_vertexBufferBound = true;
#endif
    }

    void EndRecording() {
#ifdef _DEBUG
        m_isRecording = false;
#endif
    }

private:
#ifdef _DEBUG
    bool m_isRecording = false;
    ID3D12PipelineState* m_currentPSO = nullptr;
    ID3D12RootSignature* m_currentRootSignature = nullptr;
    bool m_renderTargetsBound = false;
    bool m_viewportSet = false;
    bool m_scissorSet = false;
    bool m_vertexBufferBound = false;
#endif
};
```

---

## Synchronization Validation

```cpp
class SyncValidator {
public:
    void TrackFenceSignal(ID3D12Fence* fence, UINT64 value) {
#ifdef _DEBUG
        m_fenceValues[fence] = value;
        LOG_TRACE("Fence signaled: {} -> {}", (void*)fence, value);
#endif
    }

    bool ValidateFenceWait(ID3D12Fence* fence, UINT64 waitValue) {
#ifdef _DEBUG
        auto it = m_fenceValues.find(fence);
        if (it == m_fenceValues.end()) {
            LOG_WARN("Waiting on untracked fence");
            return true;
        }

        if (waitValue > it->second) {
            LOG_WARN("Waiting for fence value {} but only {} was signaled",
                     waitValue, it->second);
        }
#endif
        return true;
    }

    void ValidateFrameSync(UINT frameIndex, UINT64 fenceValue) {
#ifdef _DEBUG
        // Check for potential sync issues
        static UINT64 lastFrameFenceValue = 0;

        if (fenceValue <= lastFrameFenceValue) {
            LOG_ERROR("Fence value not increasing: {} <= {}",
                      fenceValue, lastFrameFenceValue);
        }

        lastFrameFenceValue = fenceValue;
#endif
    }

private:
#ifdef _DEBUG
    std::unordered_map<ID3D12Fence*, UINT64> m_fenceValues;
#endif
};
```

---

## Memory Validation

```cpp
class MemoryValidator {
public:
    void Initialize(IDXGIAdapter3* adapter) {
        m_adapter = adapter;
        UpdateMemoryInfo();
    }

    void UpdateMemoryInfo() {
        if (m_adapter) {
            m_adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &m_localMemory);
            m_adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &m_systemMemory);
        }
    }

    bool HasSufficientMemory(UINT64 requiredBytes) {
        UpdateMemoryInfo();

        UINT64 available = m_localMemory.Budget - m_localMemory.CurrentUsage;
        if (requiredBytes > available) {
            LOG_WARN("Insufficient GPU memory: need {} bytes, {} available",
                     requiredBytes, available);
            return false;
        }
        return true;
    }

    float GetMemoryUsagePercent() {
        UpdateMemoryInfo();
        return (float)m_localMemory.CurrentUsage / m_localMemory.Budget * 100.0f;
    }

    void LogMemoryStatus() {
        UpdateMemoryInfo();

        LOG_INFO("=== GPU Memory Status ===");
        LOG_INFO("Local (VRAM):");
        LOG_INFO("  Budget: {} MB", m_localMemory.Budget / (1024 * 1024));
        LOG_INFO("  Usage:  {} MB ({:.1f}%)",
                 m_localMemory.CurrentUsage / (1024 * 1024),
                 GetMemoryUsagePercent());
        LOG_INFO("  Available: {} MB",
                 (m_localMemory.Budget - m_localMemory.CurrentUsage) / (1024 * 1024));

        LOG_INFO("System (Shared):");
        LOG_INFO("  Budget: {} MB", m_systemMemory.Budget / (1024 * 1024));
        LOG_INFO("  Usage:  {} MB", m_systemMemory.CurrentUsage / (1024 * 1024));
    }

    bool CheckForLeaks() {
#ifdef _DEBUG
        // Use DXGI debug interface to report live objects
        ComPtr<IDXGIDebug1> dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)))) {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
            return true;
        }
#endif
        return false;
    }

private:
    IDXGIAdapter3* m_adapter = nullptr;
    DXGI_QUERY_VIDEO_MEMORY_INFO m_localMemory = {};
    DXGI_QUERY_VIDEO_MEMORY_INFO m_systemMemory = {};
};
```

---

## Debug Layer Integration

### Enabling the Debug Layer

```cpp
void EnableDebugLayer() {
#ifdef _DEBUG
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        LOG_INFO("D3D12 Debug Layer enabled");

        // Enable GPU-based validation (more thorough but slower)
        ComPtr<ID3D12Debug1> debugController1;
        if (SUCCEEDED(debugController.As(&debugController1))) {
            debugController1->SetEnableGPUBasedValidation(TRUE);
            debugController1->SetEnableSynchronizedCommandQueueValidation(TRUE);
            LOG_INFO("GPU-based validation enabled");
        }

        // Enable additional debug features
        ComPtr<ID3D12Debug3> debugController3;
        if (SUCCEEDED(debugController.As(&debugController3))) {
            debugController3->SetEnableGPUBasedValidation(TRUE);
            LOG_INFO("Enhanced GPU validation enabled");
        }
    } else {
        LOG_WARN("Debug layer not available - install Graphics Tools feature");
    }
#endif
}
```

### Message Callback

```cpp
void SetupDebugMessageCallback(ID3D12Device* device) {
#ifdef _DEBUG
    ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
        // Break on corruption and errors
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

        // Optionally break on warnings
        // infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

        // Filter out known safe messages
        D3D12_MESSAGE_ID hide[] = {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
        };

        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs = _countof(hide);
        filter.DenyList.pIDList = hide;
        infoQueue->AddStorageFilterEntries(&filter);

        LOG_INFO("Debug message callback configured");
    }
#endif
}

void CheckDebugMessages(ID3D12Device* device) {
#ifdef _DEBUG
    ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
        UINT64 messageCount = infoQueue->GetNumStoredMessages();

        for (UINT64 i = 0; i < messageCount; ++i) {
            SIZE_T messageLength = 0;
            infoQueue->GetMessage(i, nullptr, &messageLength);

            std::vector<char> messageData(messageLength);
            D3D12_MESSAGE* message = reinterpret_cast<D3D12_MESSAGE*>(messageData.data());
            infoQueue->GetMessage(i, message, &messageLength);

            switch (message->Severity) {
                case D3D12_MESSAGE_SEVERITY_CORRUPTION:
                    LOG_CRITICAL("[D3D12] {}", message->pDescription);
                    break;
                case D3D12_MESSAGE_SEVERITY_ERROR:
                    LOG_ERROR("[D3D12] {}", message->pDescription);
                    break;
                case D3D12_MESSAGE_SEVERITY_WARNING:
                    LOG_WARN("[D3D12] {}", message->pDescription);
                    break;
                case D3D12_MESSAGE_SEVERITY_INFO:
                    LOG_INFO("[D3D12] {}", message->pDescription);
                    break;
            }
        }

        infoQueue->ClearStoredMessages();
    }
#endif
}
```

---

## Performance Considerations

### Conditional Validation

```cpp
// Full validation in debug, minimal in release
#ifdef _DEBUG
    #define VALIDATE_FULL 1
    #define VALIDATE_BASIC 1
#elif defined(ENABLE_RELEASE_VALIDATION)
    #define VALIDATE_FULL 0
    #define VALIDATE_BASIC 1
#else
    #define VALIDATE_FULL 0
    #define VALIDATE_BASIC 0
#endif

// Use like this:
void SomeFunction(const SomeParams& params) {
#if VALIDATE_BASIC
    // Quick, cheap validation always performed
    if (params.ptr == nullptr) {
        LOG_ERROR("Null pointer");
        return;
    }
#endif

#if VALIDATE_FULL
    // Expensive validation only in debug
    ValidateComplexInvariants(params);
#endif

    // Actual work...
}
```

### Deferred Validation

```cpp
class DeferredValidator {
public:
    void QueueValidation(std::function<bool()> validator, const char* description) {
#ifdef _DEBUG
        m_pendingValidations.push_back({validator, description});
#endif
    }

    void ProcessValidations() {
#ifdef _DEBUG
        for (const auto& validation : m_pendingValidations) {
            if (!validation.validator()) {
                LOG_ERROR("Deferred validation failed: {}", validation.description);
            }
        }
        m_pendingValidations.clear();
#endif
    }

private:
    struct PendingValidation {
        std::function<bool()> validator;
        const char* description;
    };
    std::vector<PendingValidation> m_pendingValidations;
};
```

### Sampling Validation

```cpp
class SampledValidator {
public:
    // Only validate every Nth call for performance
    template<typename Func>
    void ValidateSampled(Func validator, const char* name, int sampleRate = 100) {
#ifdef _DEBUG
        auto& counter = m_callCounters[name];
        if (++counter % sampleRate == 0) {
            if (!validator()) {
                LOG_ERROR("Sampled validation failed: {} (call #{})", name, counter);
            }
        }
#endif
    }

private:
    std::unordered_map<const char*, int> m_callCounters;
};
```

---

## Validation Macros & Utilities

### Complete Macro Set

```cpp
// =============================================================================
// VALIDATION MACROS
// =============================================================================

// Basic assertions
#ifdef _DEBUG
    #define DEBUG_VALIDATE(condition, msg) \
        do { if (!(condition)) { LOG_ERROR("Validation failed: {} - {}", #condition, msg); __debugbreak(); } } while(0)
#else
    #define DEBUG_VALIDATE(condition, msg) ((void)0)
#endif

// Always-on validation with return
#define VALIDATE_RETURN(condition, retval, msg) \
    do { if (!(condition)) { LOG_ERROR(msg); return retval; } } while(0)

#define VALIDATE_RETURN_VOID(condition, msg) \
    do { if (!(condition)) { LOG_ERROR(msg); return; } } while(0)

// Pointer validation
#define VALIDATE_PTR_RETURN(ptr, retval) \
    VALIDATE_RETURN((ptr) != nullptr, retval, "Null pointer: " #ptr)

#define VALIDATE_PTR_RETURN_VOID(ptr) \
    VALIDATE_RETURN_VOID((ptr) != nullptr, "Null pointer: " #ptr)

// Range validation
#define VALIDATE_RANGE_RETURN(value, min, max, retval) \
    VALIDATE_RETURN((value) >= (min) && (value) <= (max), retval, \
                    fmt::format("{} out of range [{}, {}]: {}", #value, min, max, value))

// HRESULT validation
#define VALIDATE_HR_RETURN(hr) \
    do { \
        HRESULT _hr = (hr); \
        if (FAILED(_hr)) { \
            LOG_ERROR("HRESULT failed: 0x{:08X} at {}:{}", _hr, __FILE__, __LINE__); \
            return _hr; \
        } \
    } while(0)

#define VALIDATE_HR_LOG(hr, msg) \
    do { \
        HRESULT _hr = (hr); \
        if (FAILED(_hr)) { \
            LOG_ERROR("{}: HRESULT 0x{:08X}", msg, _hr); \
        } \
    } while(0)

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

inline bool IsPowerOfTwo(UINT value) {
    return value && !(value & (value - 1));
}

inline UINT AlignUp(UINT value, UINT alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

inline UINT64 AlignUp64(UINT64 value, UINT64 alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

inline UINT CalculateMaxMipLevels(UINT width, UINT height) {
    UINT maxDim = (std::max)(width, height);
    return static_cast<UINT>(std::floor(std::log2(maxDim))) + 1;
}

inline UINT GetBytesPerPixel(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_R32G32B32A32_FLOAT: return 16;
        case DXGI_FORMAT_R16G16B16A16_FLOAT: return 8;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return 4;
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
            return 2;
        case DXGI_FORMAT_R8_UNORM:
            return 1;
        default:
            return 0; // Unknown or compressed
    }
}
```

---

## References

### Microsoft Documentation
- [Direct3D 12 Programming Guide](https://learn.microsoft.com/en-us/windows/win32/direct3d12/direct3d-12-graphics)
- [Using the Debug Layer](https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-d3d12-debug-layer-gpu-based-validation)
- [ID3D12InfoQueue Interface](https://learn.microsoft.com/en-us/windows/win32/api/d3d12sdklayers/nn-d3d12sdklayers-id3d12infoqueue)
- [Resource Binding](https://learn.microsoft.com/en-us/windows/win32/direct3d12/resource-binding)

### Debugging Tools
- [PIX for Windows](https://devblogs.microsoft.com/pix/download/)
- [RenderDoc](https://renderdoc.org/)
- [DirectX Debugging Tools](https://github.com/microsoft/DirectX-Debugging-Tools)

### Best Practices
- [D3D12 Do's and Don'ts](https://developer.nvidia.com/dx12-dos-and-donts)
- [AMD D3D12 Performance Tips](https://gpuopen.com/performance/)
