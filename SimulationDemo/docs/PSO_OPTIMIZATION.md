# DX12 PSO Optimization & Caching Guide

## Overview

Pipeline State Object (PSO) compilation is one of the most expensive operations in Direct3D 12. A single PSO creation can take 100+ milliseconds as the driver compiles shaders and validates state. This guide covers strategies to eliminate runtime hitches and optimize PSO management.

## Table of Contents

1. [The PSO Compilation Problem](#the-pso-compilation-problem)
2. [Caching Strategies](#caching-strategies)
3. [Pipeline Libraries](#pipeline-libraries)
4. [Async Compilation](#async-compilation)
5. [State Deduplication](#state-deduplication)
6. [Hot-Reload for Development](#hot-reload-for-development)
7. [Production Best Practices](#production-best-practices)
8. [Debugging PSO Issues](#debugging-pso-issues)

---

## The PSO Compilation Problem

### Why PSO Creation is Expensive

```
CreateGraphicsPipelineState() involves:
├── Shader compilation (DXIL → native GPU ISA)
├── State validation and error checking
├── Resource binding verification
├── Hardware-specific optimization passes
└── Driver internal caching
```

**Typical Costs:**
| Operation | Time |
|-----------|------|
| Simple PSO (basic VS/PS) | 10-50ms |
| Complex PSO (all stages) | 50-200ms |
| First-time shader compile | 100-500ms |
| Cached shader recompile | 5-20ms |

### The Stutter Problem

```cpp
// BAD: Creating PSO during gameplay
void RenderObject(Object* obj) {
    if (!obj->pso) {
        // STUTTER! 100ms+ freeze
        obj->pso = CreatePSOForMaterial(obj->material);
    }
    cmdList->SetPipelineState(obj->pso);
    cmdList->DrawIndexed(...);
}
```

---

## Caching Strategies

### Strategy 1: Cached PSO Blob

D3D12 allows extracting compiled PSO data for reuse:

```cpp
// After creating PSO, extract cache blob
ComPtr<ID3DBlob> cachedBlob;
HRESULT hr = pso->GetCachedBlob(&cachedBlob);

// Save to disk
SaveToFile("cache/terrain_pso.bin",
    cachedBlob->GetBufferPointer(),
    cachedBlob->GetBufferSize());
```

```cpp
// On subsequent runs, load and use cached blob
std::vector<uint8_t> cachedData = LoadFromFile("cache/terrain_pso.bin");

D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = { ... };
psoDesc.CachedPSO.pCachedBlob = cachedData.data();
psoDesc.CachedPSO.CachedBlobSizeInBytes = cachedData.size();

// Much faster creation!
hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
```

### Cache Invalidation

```cpp
HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));

switch (hr) {
case S_OK:
    // Cache hit, fast creation
    break;

case D3D12_ERROR_DRIVER_VERSION_MISMATCH:
    // Driver updated, recompile needed
    psoDesc.CachedPSO = {};  // Clear cache
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
    // Save new cache
    break;

case D3D12_ERROR_ADAPTER_NOT_FOUND:
    // Different GPU, recompile needed
    psoDesc.CachedPSO = {};
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
    break;

case E_INVALIDARG:
    // Cache corrupted or PSO desc changed
    psoDesc.CachedPSO = {};
    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
    break;
}
```

### Cache Key Design

```cpp
struct PSOCacheKey {
    uint64_t vsHash;           // Vertex shader content hash
    uint64_t psHash;           // Pixel shader content hash
    uint64_t stateHash;        // Blend, raster, depth-stencil hash
    uint32_t inputLayoutHash;  // Vertex format hash
    DXGI_FORMAT rtvFormats[8]; // Render target formats
    DXGI_FORMAT dsvFormat;     // Depth format
    uint32_t sampleCount;      // MSAA level

    bool operator==(const PSOCacheKey& other) const;
    size_t GetHash() const;
};

std::unordered_map<PSOCacheKey, ComPtr<ID3D12PipelineState>> m_psoCache;
```

---

## Pipeline Libraries

### Overview

Pipeline Libraries (`ID3D12PipelineLibrary`) provide driver-level PSO caching with automatic deduplication:

```cpp
// Check feature support
D3D12_FEATURE_DATA_SHADER_CACHE shaderCacheSupport = {};
device->CheckFeatureSupport(D3D12_FEATURE_SHADER_CACHE,
    &shaderCacheSupport, sizeof(shaderCacheSupport));

bool librarySupported =
    (shaderCacheSupport.SupportFlags & D3D12_SHADER_CACHE_SUPPORT_LIBRARY);
```

### Creating a Pipeline Library

```cpp
// Create empty library
ComPtr<ID3D12PipelineLibrary> pipelineLibrary;

// Option 1: Create empty (first run)
hr = device->CreatePipelineLibrary(nullptr, 0,
    IID_PPV_ARGS(&pipelineLibrary));

// Option 2: Load from existing data
std::vector<uint8_t> libraryData = LoadFromFile("cache/pipeline_library.bin");
hr = device->CreatePipelineLibrary(libraryData.data(), libraryData.size(),
    IID_PPV_ARGS(&pipelineLibrary));
```

### Storing PSOs in Library

```cpp
// Create PSO normally
ComPtr<ID3D12PipelineState> terrainPSO;
device->CreateGraphicsPipelineState(&terrainDesc, IID_PPV_ARGS(&terrainPSO));

// Store in library with unique name
hr = pipelineLibrary->StorePipeline(L"TerrainOpaque", terrainPSO.Get());

// Store more PSOs
pipelineLibrary->StorePipeline(L"CreatureBase", creaturePSO.Get());
pipelineLibrary->StorePipeline(L"NametagAlpha", nametagPSO.Get());
```

### Loading PSOs from Library

```cpp
ComPtr<ID3D12PipelineState> terrainPSO;

// Try to load from library (fast path)
hr = pipelineLibrary->LoadGraphicsPipeline(L"TerrainOpaque",
    &terrainDesc, IID_PPV_ARGS(&terrainPSO));

if (hr == E_INVALIDARG) {
    // PSO not in library, create and store
    device->CreateGraphicsPipelineState(&terrainDesc, IID_PPV_ARGS(&terrainPSO));
    pipelineLibrary->StorePipeline(L"TerrainOpaque", terrainPSO.Get());
}
```

### Serializing Library to Disk

```cpp
// Get serialized size
SIZE_T librarySize = pipelineLibrary->GetSerializedSize();

// Allocate and serialize
std::vector<uint8_t> serializedData(librarySize);
hr = pipelineLibrary->Serialize(serializedData.data(), librarySize);

// Save to disk
SaveToFile("cache/pipeline_library.bin", serializedData);
```

### ID3D12PipelineLibrary1 (Enhanced)

```cpp
// ID3D12PipelineLibrary1 supports pipeline state streams
ComPtr<ID3D12PipelineLibrary1> pipelineLibrary1;
pipelineLibrary.As(&pipelineLibrary1);

// Load using stream description (more flexible)
D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = { ... };
hr = pipelineLibrary1->LoadPipeline(L"TerrainOpaque",
    &streamDesc, IID_PPV_ARGS(&terrainPSO));
```

### Thread Safety Notes

```cpp
// Pipeline library methods are thread-safe EXCEPT:
// - Multiple threads loading the SAME PSO simultaneously
// - Must synchronize external access to same PSO name

std::mutex psoLoadMutex;

ID3D12PipelineState* LoadPSOThreadSafe(const wchar_t* name,
    const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc) {

    std::lock_guard<std::mutex> lock(psoLoadMutex);

    ComPtr<ID3D12PipelineState> pso;
    HRESULT hr = pipelineLibrary->LoadGraphicsPipeline(name, desc,
        IID_PPV_ARGS(&pso));

    if (FAILED(hr)) {
        device->CreateGraphicsPipelineState(desc, IID_PPV_ARGS(&pso));
        pipelineLibrary->StorePipeline(name, pso.Get());
    }

    return pso.Detach();
}
```

---

## Async Compilation

### Worker Thread PSO Creation

```cpp
class PSOCompiler {
    std::queue<PSOCompileRequest> m_pendingRequests;
    std::unordered_map<uint64_t, ComPtr<ID3D12PipelineState>> m_completedPSOs;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::vector<std::thread> m_workerThreads;
    bool m_shutdown = false;

public:
    void Initialize(int numWorkers = 4) {
        for (int i = 0; i < numWorkers; i++) {
            m_workerThreads.emplace_back(&PSOCompiler::WorkerThread, this);

            // Set to idle priority to avoid impacting game threads
            SetThreadPriority(m_workerThreads.back().native_handle(),
                THREAD_PRIORITY_IDLE);
        }
    }

    void RequestPSO(uint64_t id, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingRequests.push({ id, desc });
        m_cv.notify_one();
    }

    ID3D12PipelineState* TryGetPSO(uint64_t id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_completedPSOs.find(id);
        return (it != m_completedPSOs.end()) ? it->second.Get() : nullptr;
    }

private:
    void WorkerThread() {
        while (!m_shutdown) {
            PSOCompileRequest request;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [this] {
                    return m_shutdown || !m_pendingRequests.empty();
                });

                if (m_shutdown) break;

                request = m_pendingRequests.front();
                m_pendingRequests.pop();
            }

            // Compile PSO (expensive, but off main thread)
            ComPtr<ID3D12PipelineState> pso;
            device->CreateGraphicsPipelineState(&request.desc,
                IID_PPV_ARGS(&pso));

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_completedPSOs[request.id] = pso;
            }
        }
    }
};
```

### Generic Fallback Pattern

```cpp
class PSOManager {
    ComPtr<ID3D12PipelineState> m_genericPSO;      // Simple, fast-compiling fallback
    PSOCompiler m_compiler;

public:
    void Initialize() {
        // Create generic PSO synchronously at startup
        // Uses simplified shaders that compile quickly
        m_genericPSO = CreateGenericPSO();

        // Start async compilation of specialized PSOs
        m_compiler.Initialize();
        QueueAllSpecializedPSOs();
    }

    ID3D12PipelineState* GetPSO(uint64_t specializedId) {
        // Try to get specialized PSO
        auto* specialized = m_compiler.TryGetPSO(specializedId);

        // Fall back to generic if not ready
        return specialized ? specialized : m_genericPSO.Get();
    }
};
```

### Priority-Based Compilation

```cpp
enum class PSOPriority {
    Critical,   // Needed immediately (player, UI)
    High,       // Needed soon (nearby objects)
    Normal,     // Background compilation
    Low         // Speculative pre-compilation
};

struct PSOCompileRequest {
    uint64_t id;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
    PSOPriority priority;
};

// Use priority queue instead of regular queue
std::priority_queue<PSOCompileRequest, std::vector<PSOCompileRequest>,
    PSOPriorityComparator> m_pendingRequests;
```

---

## State Deduplication

### Identical Defaults Strategy

```cpp
// NVIDIA recommendation: Use identical sensible defaults
// for "don't care" fields to maximize PSO reuse

D3D12_GRAPHICS_PIPELINE_STATE_DESC GetDefaultPSODesc() {
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};

    // Always use these exact defaults
    desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    desc.SampleMask = UINT_MAX;
    desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.DepthStencilState.DepthEnable = TRUE;
    desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    desc.DepthStencilState.StencilEnable = FALSE;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    return desc;
}
```

### State Hashing for Deduplication

```cpp
struct BlendStateKey {
    D3D12_BLEND_DESC desc;

    size_t Hash() const {
        // Hash all relevant fields
        size_t h = 0;
        HashCombine(h, desc.AlphaToCoverageEnable);
        HashCombine(h, desc.IndependentBlendEnable);
        for (int i = 0; i < 8; i++) {
            const auto& rt = desc.RenderTarget[i];
            HashCombine(h, rt.BlendEnable);
            HashCombine(h, rt.SrcBlend);
            HashCombine(h, rt.DestBlend);
            // ... etc
        }
        return h;
    }
};

// Cache unique blend states
std::unordered_map<size_t, D3D12_BLEND_DESC> m_blendStateCache;
```

### PSO Variant Management

```cpp
class PSOVariantManager {
    // Base PSO configurations
    std::unordered_map<std::string, D3D12_GRAPHICS_PIPELINE_STATE_DESC> m_basePSOs;

    // Variant flags
    enum VariantFlags {
        VARIANT_NONE = 0,
        VARIANT_WIREFRAME = 1 << 0,
        VARIANT_NO_CULL = 1 << 1,
        VARIANT_ALPHA_BLEND = 1 << 2,
        VARIANT_MSAA_4X = 1 << 3,
        // ... etc
    };

    // Cached variants
    std::unordered_map<uint64_t, ComPtr<ID3D12PipelineState>> m_variants;

public:
    ID3D12PipelineState* GetVariant(const std::string& baseName, uint32_t flags) {
        uint64_t key = HashString(baseName) ^ (uint64_t(flags) << 32);

        auto it = m_variants.find(key);
        if (it != m_variants.end()) {
            return it->second.Get();
        }

        // Create variant
        auto desc = m_basePSOs[baseName];
        ApplyVariantFlags(desc, flags);

        ComPtr<ID3D12PipelineState> pso;
        device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));
        m_variants[key] = pso;

        return pso.Get();
    }

private:
    void ApplyVariantFlags(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, uint32_t flags) {
        if (flags & VARIANT_WIREFRAME) {
            desc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
        }
        if (flags & VARIANT_NO_CULL) {
            desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        }
        if (flags & VARIANT_ALPHA_BLEND) {
            desc.BlendState.RenderTarget[0].BlendEnable = TRUE;
            // ... setup alpha blend
        }
        if (flags & VARIANT_MSAA_4X) {
            desc.SampleDesc.Count = 4;
        }
    }
};
```

---

## Hot-Reload for Development

### Shader Hot-Reload System

```cpp
class ShaderHotReload {
    FileWatcher m_watcher;
    std::unordered_map<std::wstring, ShaderInfo> m_shaders;
    std::unordered_set<ID3D12PipelineState*> m_invalidatedPSOs;

public:
    void Initialize() {
        m_watcher.Watch(L"shaders/", [this](const std::wstring& path) {
            OnShaderFileChanged(path);
        });
    }

    void OnShaderFileChanged(const std::wstring& path) {
        // Recompile shader
        ComPtr<ID3DBlob> newShader = CompileShader(path);
        if (!newShader) {
            OutputDebugString(L"Shader compilation failed!\n");
            return;
        }

        // Update shader in cache
        m_shaders[path].bytecode = newShader;

        // Mark affected PSOs for recreation
        for (auto* pso : m_shaders[path].dependentPSOs) {
            m_invalidatedPSOs.insert(pso);
        }
    }

    void FlushInvalidatedPSOs() {
        for (auto* oldPSO : m_invalidatedPSOs) {
            // Get PSO description
            auto& info = m_psoInfo[oldPSO];

            // Update shader bytecode
            UpdateShaderBytecode(info.desc);

            // Recreate PSO
            ComPtr<ID3D12PipelineState> newPSO;
            device->CreateGraphicsPipelineState(&info.desc,
                IID_PPV_ARGS(&newPSO));

            // Swap in new PSO
            info.currentPSO = newPSO;
        }
        m_invalidatedPSOs.clear();
    }
};
```

### Development vs Release Mode

```cpp
#ifdef _DEBUG
    // Development: Enable hot-reload, skip caching
    class PSOManager_Debug {
        ShaderHotReload m_hotReload;

        ID3D12PipelineState* GetPSO(const PSOKey& key) {
            m_hotReload.FlushInvalidatedPSOs();
            return CreatePSODirect(key);  // Always fresh
        }
    };
    using PSOManager = PSOManager_Debug;
#else
    // Release: Full caching, no hot-reload
    class PSOManager_Release {
        ID3D12PipelineLibrary* m_library;

        ID3D12PipelineState* GetPSO(const PSOKey& key) {
            return LoadOrCreateCached(key);
        }
    };
    using PSOManager = PSOManager_Release;
#endif
```

---

## Production Best Practices

### Startup PSO Warming

```cpp
void WarmupPSOs() {
    // Create PSOs for all known material/state combinations at startup

    std::vector<std::future<void>> futures;

    for (const auto& material : g_materialDatabase) {
        for (const auto& renderPass : g_renderPasses) {
            futures.push_back(std::async(std::launch::async, [&] {
                auto desc = BuildPSODesc(material, renderPass);
                ComPtr<ID3D12PipelineState> pso;
                device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));
                StorePSO(material.id, renderPass.id, pso);
            }));
        }
    }

    // Wait for all PSOs to compile
    for (auto& f : futures) {
        f.wait();
    }
}
```

### Miss-and-Update Strategy

```cpp
// Don't serialize huge libraries at once
// Instead, incrementally update

void UpdatePipelineLibrary() {
    // Only save newly created PSOs since last save
    static size_t lastSavedCount = 0;
    size_t currentCount = GetPSOCount();

    if (currentCount > lastSavedCount + 10) {  // Save every 10 new PSOs
        SIZE_T size = m_pipelineLibrary->GetSerializedSize();
        std::vector<uint8_t> data(size);
        m_pipelineLibrary->Serialize(data.data(), size);
        SaveToFileAsync("cache/pipeline_library.bin", data);
        lastSavedCount = currentCount;
    }
}
```

### Graceful Degradation

```cpp
enum class PSOReadiness {
    Ready,              // Optimal PSO available
    UsingFallback,      // Generic PSO in use
    Compiling,          // PSO being compiled
    Failed              // Compilation failed
};

struct RenderItem {
    uint64_t psoId;
    PSOReadiness readiness;

    void Render(ID3D12GraphicsCommandList* cmdList) {
        switch (readiness) {
        case PSOReadiness::Ready:
            // Use optimal PSO
            cmdList->SetPipelineState(GetOptimalPSO(psoId));
            DrawFull(cmdList);
            break;

        case PSOReadiness::UsingFallback:
            // Use simpler rendering
            cmdList->SetPipelineState(GetFallbackPSO());
            DrawSimplified(cmdList);
            break;

        case PSOReadiness::Compiling:
            // Skip or use placeholder
            DrawPlaceholder(cmdList);
            break;

        case PSOReadiness::Failed:
            // Log error, skip rendering
            break;
        }
    }
};
```

---

## Debugging PSO Issues

### Common Errors and Solutions

| Error | Cause | Solution |
|-------|-------|----------|
| `E_INVALIDARG` | Mismatched state/shader | Validate input layout matches VS |
| `D3D12_ERROR_DRIVER_VERSION_MISMATCH` | Driver updated | Clear cache, recompile |
| `D3D12_ERROR_ADAPTER_NOT_FOUND` | Different GPU | Clear cache, recompile |
| Slow creation | No caching | Implement pipeline library |
| Runtime stutter | On-demand creation | Pre-compile or use async |

### PIX PSO Analysis

```cpp
// Use PIX programmatic capture around PSO creation
#include <pix3.h>

void CreatePSOWithPIXMarker(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc) {
    PIXBeginEvent(PIX_COLOR(255, 0, 0), "PSO Creation: %s", psoName);

    ComPtr<ID3D12PipelineState> pso;
    HRESULT hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso));

    PIXEndEvent();

    // PIX will show PSO creation time in timeline
}
```

### Validation Layer Checks

```cpp
// Enable D3D12 debug layer for PSO validation
ComPtr<ID3D12Debug> debugController;
D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
debugController->EnableDebugLayer();

// Common validation errors:
// - "Root signature doesn't match shader"
// - "Input layout element semantic not found in shader"
// - "Render target format mismatch"
```

---

## References

### Official Documentation
- [ID3D12PipelineLibrary](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nn-d3d12-id3d12pipelinelibrary)
- [D3D12_CACHED_PIPELINE_STATE](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_cached_pipeline_state)
- [Direct3D 12 Pipeline State Cache Sample](https://learn.microsoft.com/en-us/samples/microsoft/directx-graphics-samples/d3d12-pipeline-state-cache-sample-uwp/)

### Technical Articles
- [Advanced API Performance: PSOs - NVIDIA](https://developer.nvidia.com/blog/advanced-api-performance-pipeline-state-objects/)
- [Pipeline State Cache Studies](https://alegruz.github.io/Notes/2024/11/English/PipelineStateCacheStudies.html)
- [D3D12 PSO Caching - GameDev.net](https://www.gamedev.net/blogs/entry/2277163-d3d12-pipeline-state-object-caching/)

### Engine Implementations
- [Unreal Engine PSO Caching](https://dev.epicgames.com/documentation/en-us/unreal-engine/optimizing-rendering-with-pso-caches-in-unreal-engine)
- [MiniEngine (Microsoft)](https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/MiniEngine)

### Profiling Tools
- [PIX on Windows](https://devblogs.microsoft.com/pix/introduction/)
- [NVIDIA Nsight Graphics](https://developer.nvidia.com/nsight-graphics)
- [AMD Radeon GPU Profiler](https://gpuopen.com/rgp/)
