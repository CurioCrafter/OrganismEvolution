# DX12 Pipeline State Object (PSO) Complete Guide

## Overview

Pipeline State Objects (PSOs) are the foundation of Direct3D 12's rendering architecture. Unlike D3D11's granular state management, DX12 bundles GPU state into immutable objects that can be pre-validated and optimized by the driver.

**Key Benefits:**
- Eliminates runtime state validation overhead
- Enables driver pre-compilation and optimization
- Supports multi-threaded PSO creation
- Reduces CPU overhead by orders of magnitude

## Table of Contents

1. [Pipeline State Architecture](#pipeline-state-architecture)
2. [PSO Components](#pso-components)
3. [Creating Pipeline States](#creating-pipeline-states)
4. [Input Layout Design](#input-layout-design)
5. [Render State Configuration](#render-state-configuration)
6. [Multi-Pipeline Architectures](#multi-pipeline-architectures)
7. [Performance Best Practices](#performance-best-practices)
8. [Debugging and Profiling](#debugging-and-profiling)

---

## Pipeline State Architecture

### What's in a PSO?

A Graphics PSO contains all fixed-function and programmable state:

```cpp
D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
psoDesc.pRootSignature          // Root signature for resource binding
psoDesc.VS                      // Vertex shader bytecode
psoDesc.PS                      // Pixel shader bytecode
psoDesc.DS                      // Domain shader (optional)
psoDesc.HS                      // Hull shader (optional)
psoDesc.GS                      // Geometry shader (optional)
psoDesc.StreamOutput            // Stream output configuration
psoDesc.BlendState              // Alpha blending settings
psoDesc.SampleMask              // Multi-sample coverage mask
psoDesc.RasterizerState         // Culling, fill mode, etc.
psoDesc.DepthStencilState       // Depth/stencil testing
psoDesc.InputLayout             // Vertex format description
psoDesc.IBStripCutValue         // Strip cut index value
psoDesc.PrimitiveTopologyType   // Point/Line/Triangle/Patch
psoDesc.NumRenderTargets        // Number of RTVs
psoDesc.RTVFormats[]            // Render target formats
psoDesc.DSVFormat               // Depth-stencil format
psoDesc.SampleDesc              // Multi-sampling parameters
psoDesc.NodeMask                // GPU node for multi-GPU
psoDesc.CachedPSO               // Cached blob (optional)
psoDesc.Flags                   // Compilation flags
```

### What's NOT in a PSO (Dynamic State)

These states change too frequently to bake into PSOs:

| Dynamic State | Set Method |
|--------------|------------|
| Viewports | `RSSetViewports()` |
| Scissor Rectangles | `RSSetScissorRects()` |
| Blend Factor | `OMSetBlendFactor()` |
| Stencil Reference | `OMSetStencilRef()` |
| Primitive Topology | `IASetPrimitiveTopology()` |
| Vertex Buffers | `IASetVertexBuffers()` |
| Index Buffer | `IASetIndexBuffer()` |
| Render Targets | `OMSetRenderTargets()` |

---

## PSO Components

### Shader Stages

```
Input Assembler → Vertex Shader → Hull Shader → Tessellator →
Domain Shader → Geometry Shader → Stream Output → Rasterizer →
Pixel Shader → Output Merger
```

**Shader Bytecode Format:**
```cpp
// DXIL (recommended) - modern format
D3D12_SHADER_BYTECODE vs = { compiledVS->GetBufferPointer(),
                              compiledVS->GetBufferSize() };

// Avoid FXC/DXBC - causes DXBC→DXIL translation overhead
```

### Blend State

```cpp
D3D12_BLEND_DESC blendDesc = {};
blendDesc.AlphaToCoverageEnable = FALSE;
blendDesc.IndependentBlendEnable = FALSE;

D3D12_RENDER_TARGET_BLEND_DESC& rtBlend = blendDesc.RenderTarget[0];
rtBlend.BlendEnable = TRUE;
rtBlend.LogicOpEnable = FALSE;
rtBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
rtBlend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
rtBlend.BlendOp = D3D12_BLEND_OP_ADD;
rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
rtBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
rtBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
```

**Common Blend Configurations:**

| Mode | Src | Dest | Operation |
|------|-----|------|-----------|
| Opaque | ONE | ZERO | ADD |
| Alpha Blend | SRC_ALPHA | INV_SRC_ALPHA | ADD |
| Additive | ONE | ONE | ADD |
| Premultiplied | ONE | INV_SRC_ALPHA | ADD |
| Multiply | DEST_COLOR | ZERO | ADD |

### Rasterizer State

```cpp
D3D12_RASTERIZER_DESC rasterizerDesc = {};
rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
rasterizerDesc.FrontCounterClockwise = FALSE;
rasterizerDesc.DepthBias = 0;
rasterizerDesc.DepthBiasClamp = 0.0f;
rasterizerDesc.SlopeScaledDepthBias = 0.0f;
rasterizerDesc.DepthClipEnable = TRUE;
rasterizerDesc.MultisampleEnable = FALSE;
rasterizerDesc.AntialiasedLineEnable = FALSE;
rasterizerDesc.ForcedSampleCount = 0;
rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
```

**Performance Implications:**

| Setting | Performance Impact |
|---------|-------------------|
| `CullMode::NONE` | ~2x pixel shader invocations |
| `FillMode::WIREFRAME` | Debug only, significant overhead |
| `ConservativeRaster::ON` | More fragments, ~15-30% slower |
| `DepthClipEnable::FALSE` | Avoids clip-space culling |

### Depth-Stencil State

```cpp
D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
depthStencilDesc.DepthEnable = TRUE;
depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
depthStencilDesc.StencilEnable = FALSE;
depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

// Stencil operations (if enabled)
depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
depthStencilDesc.BackFace = depthStencilDesc.FrontFace;
```

**Depth Function Selection:**

| Function | Use Case |
|----------|----------|
| `LESS` | Standard forward rendering |
| `LESS_EQUAL` | When redrawing same geometry |
| `GREATER` | Reversed-Z buffer |
| `GREATER_EQUAL` | Reversed-Z with overdraw |
| `ALWAYS` | Skybox (always behind) |

---

## Creating Pipeline States

### Basic PSO Creation

```cpp
// 1. Fill out the description
D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
psoDesc.pRootSignature = m_rootSignature.Get();
psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
psoDesc.SampleMask = UINT_MAX;
psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
psoDesc.NumRenderTargets = 1;
psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
psoDesc.SampleDesc.Count = 1;

// 2. Create the PSO
ComPtr<ID3D12PipelineState> pipelineState;
HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc,
    IID_PPV_ARGS(&pipelineState));
```

### Pipeline State Stream (Modern Approach)

```cpp
// More flexible approach using D3D12_PIPELINE_STATE_STREAM_DESC
struct PipelineStateStream {
    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE RootSignature;
    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopology;
    CD3DX12_PIPELINE_STATE_STREAM_VS VS;
    CD3DX12_PIPELINE_STATE_STREAM_PS PS;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
    CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
};

PipelineStateStream pss;
pss.RootSignature = m_rootSignature.Get();
pss.InputLayout = { inputLayout, numElements };
// ... set other fields

D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
streamDesc.SizeInBytes = sizeof(pss);
streamDesc.pPipelineStateSubobjectStream = &pss;

ComPtr<ID3D12PipelineState> pso;
device2->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pso));
```

---

## Input Layout Design

### Basic Input Layout

```cpp
D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};
```

### Input Element Fields Explained

| Field | Description |
|-------|-------------|
| `SemanticName` | HLSL semantic (POSITION, NORMAL, etc.) |
| `SemanticIndex` | Index for same semantic (TEXCOORD0, TEXCOORD1) |
| `Format` | Data format (DXGI_FORMAT_*) |
| `InputSlot` | Vertex buffer slot (0-15) |
| `AlignedByteOffset` | Offset in bytes, or APPEND_ALIGNED_ELEMENT |
| `InputSlotClass` | Per-vertex or per-instance |
| `InstanceDataStepRate` | Instance step rate (0 for vertex data) |

### Multi-Stream Layout (Optimized)

Separate position for shadow/depth passes:

```cpp
// Stream 0: Position only (shadow pass)
// Stream 1: Other attributes (lit pass)
D3D12_INPUT_ELEMENT_DESC multiStreamLayout[] = {
    // Stream 0 - Position
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

    // Stream 1 - Normal, UV
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 12,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};
```

### Instance Data Layout

```cpp
D3D12_INPUT_ELEMENT_DESC instancedLayout[] = {
    // Per-vertex data
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
      D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

    // Per-instance data (slot 1)
    { "INSTANCE_TRANSFORM", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,
      D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
    { "INSTANCE_TRANSFORM", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16,
      D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
    { "INSTANCE_TRANSFORM", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32,
      D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
    { "INSTANCE_TRANSFORM", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48,
      D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
    { "INSTANCE_COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 64,
      D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
};
```

### Vertex Format Optimization

**Compressed Formats for Bandwidth Savings:**

| Data | Full Format | Compressed | Savings |
|------|-------------|------------|---------|
| Position | R32G32B32_FLOAT (12B) | R16G16B16A16_SNORM (8B) | 33% |
| Normal | R32G32B32_FLOAT (12B) | R10G10B10A2_UNORM (4B) | 67% |
| UV | R32G32_FLOAT (8B) | R16G16_UNORM (4B) | 50% |
| Color | R32G32B32A32_FLOAT (16B) | R8G8B8A8_UNORM (4B) | 75% |

**When to use quantized positions** (from [Wicked Engine](https://wickedengine.net/2023/11/dynamic-vertex-formats/)):
- `R10G10B10A2_UNORM`: Small meshes, ~1mm precision
- `R16G16B16A16_UNORM`: Most meshes, acceptable precision
- `R32G32B32_FLOAT`: Large AABBs, ray tracing requirements

---

## Render State Configuration

### Pipeline Configuration for Different Passes

#### Terrain Pipeline
```cpp
// Solid terrain with depth testing
terrainPSO.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
terrainPSO.DepthStencilState.DepthEnable = TRUE;
terrainPSO.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
terrainPSO.BlendState.RenderTarget[0].BlendEnable = FALSE;
```

#### Creature Pipeline
```cpp
// Opaque creatures with instance data
creaturePSO.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
creaturePSO.DepthStencilState.DepthEnable = TRUE;
creaturePSO.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
```

#### Nametag Pipeline
```cpp
// Transparent nametags, no depth write
nametagPSO.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
nametagPSO.DepthStencilState.DepthEnable = TRUE;
nametagPSO.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
nametagPSO.BlendState.RenderTarget[0].BlendEnable = TRUE;
nametagPSO.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
nametagPSO.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
```

#### Shadow Map Pipeline
```cpp
// Depth-only rendering
shadowPSO.PS = {}; // No pixel shader needed
shadowPSO.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
shadowPSO.NumRenderTargets = 0;
shadowPSO.DSVFormat = DXGI_FORMAT_D32_FLOAT;
shadowPSO.RasterizerState.DepthBias = 10000;
shadowPSO.RasterizerState.SlopeScaledDepthBias = 1.0f;
```

---

## Multi-Pipeline Architectures

### Sharing Root Signatures

**Best Practice:** Share root signatures across similar pipelines to minimize binding changes.

```cpp
// One root signature for all terrain/creature rendering
ComPtr<ID3D12RootSignature> m_sceneRootSignature;

// Create PSOs with shared signature
terrainPSO.pRootSignature = m_sceneRootSignature.Get();
creaturePSO.pRootSignature = m_sceneRootSignature.Get();
treePSO.pRootSignature = m_sceneRootSignature.Get();
```

### PSO Inheritance Strategy

```cpp
// Base configuration
D3D12_GRAPHICS_PIPELINE_STATE_DESC basePSO = GetDefaultPSODesc();
basePSO.pRootSignature = m_rootSignature.Get();
basePSO.InputLayout = m_standardInputLayout;

// Derive variants
D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePSO = basePSO;
opaquePSO.VS = m_opaqueVS;
opaquePSO.PS = m_opaquePS;

D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaPSO = basePSO;
alphaPSO.VS = m_alphaVS;
alphaPSO.PS = m_alphaPS;
alphaPSO.BlendState = m_alphaBlend;
```

### Uber-Shaders vs Specialized Pipelines

| Approach | Pros | Cons |
|----------|------|------|
| **Uber-Shaders** | Fewer PSOs, easier management | Shader complexity, dynamic branching |
| **Specialized** | Optimal performance per case | More PSOs to manage, longer load times |
| **Hybrid** | Balance of both | Requires careful design |

**Recommendation:** Use specialized PSOs for major visual differences, uber-shaders for minor variations (controlled via constants).

---

## Performance Best Practices

### PSO Creation Timing

1. **Startup:** Create ~5,000 core PSOs covering common cases
2. **Background:** Compile specialized PSOs asynchronously on worker threads
3. **Runtime:** Avoid creating PSOs during gameplay - causes stalls

### Draw Call Organization

```
// BAD: PSO thrashing
SetPSO(terrainPSO);   Draw(terrain1);
SetPSO(creaturePSO);  Draw(creature1);
SetPSO(terrainPSO);   Draw(terrain2);  // Switch back
SetPSO(creaturePSO);  Draw(creature2);

// GOOD: Batch by PSO
SetPSO(terrainPSO);
Draw(terrain1);
Draw(terrain2);

SetPSO(creaturePSO);
Draw(creature1);
Draw(creature2);
```

### State Sorting Priority

1. **Root Signature** (highest cost to change)
2. **Pipeline State Object**
3. **Descriptor Tables**
4. **Root Descriptors**
5. **Root Constants** (lowest cost)

### Heavyweight Operations to Avoid

- Toggling between compute and graphics on same queue
- Tessellation on/off switches
- Root signature changes
- Large descriptor table rebinds

---

## Debugging and Profiling

### PIX on Windows

Microsoft's [PIX](https://devblogs.microsoft.com/pix/introduction/) is essential for DX12 debugging:

- GPU captures show PSO state per draw
- Timeline view reveals PSO switches
- Shader debugging with live values
- Memory and bandwidth analysis

### NVIDIA Nsight Graphics

[Nsight Graphics](https://developer.nvidia.com/nsight-graphics) provides:

- Frame debugging and profiling
- GPU trace for pipeline analysis
- Shader profiler for bottleneck detection
- Memory access pattern visualization

### AMD Radeon GPU Profiler

[RGP](https://gpuopen.com/rgp/) offers:

- Detailed wavefront occupancy
- Pipeline stage timing
- Barrier visualization
- Cache hit/miss analysis

### Debug Layer Validation

```cpp
#ifdef _DEBUG
ComPtr<ID3D12Debug> debugController;
if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
    debugController->EnableDebugLayer();

    // GPU-based validation (slower but catches more issues)
    ComPtr<ID3D12Debug1> debugController1;
    if (SUCCEEDED(debugController.As(&debugController1))) {
        debugController1->SetEnableGPUBasedValidation(TRUE);
    }
}
#endif
```

---

## References

### Official Documentation
- [Direct3D 12 Programming Guide](https://learn.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide)
- [D3D12_GRAPHICS_PIPELINE_STATE_DESC](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_graphics_pipeline_state_desc)
- [CPU Efficiency Specification](https://microsoft.github.io/DirectX-Specs/d3d/CPUEfficiency.html)

### NVIDIA Resources
- [Advanced API Performance: Pipeline State Objects](https://developer.nvidia.com/blog/advanced-api-performance-pipeline-state-objects/)
- [Advanced API Performance: Descriptors](https://developer.nvidia.com/blog/advanced-api-performance-descriptors/)

### Community Resources
- [Wicked Engine: Dynamic Vertex Formats](https://wickedengine.net/2023/11/dynamic-vertex-formats/)
- [DX12 Root Signature Object](https://logins.github.io/graphics/2020/06/26/DX12RootSignatureObject.html)
- [Pipeline State Cache Studies](https://alegruz.github.io/Notes/2024/11/English/PipelineStateCacheStudies.html)

### Open Source Engines
- [DirectX-Graphics-Samples](https://github.com/microsoft/DirectX-Graphics-Samples)
- [MiniEngine](https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/MiniEngine)
- [Cauldron](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron)
