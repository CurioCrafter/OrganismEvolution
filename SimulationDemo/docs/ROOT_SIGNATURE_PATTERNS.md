# DX12 Root Signature Design Patterns

## Overview

The root signature is Direct3D 12's mechanism for connecting shader resources to the graphics pipeline. It defines the "API contract" between your application and shaders, specifying what resources are available and how they're accessed.

**Key Insight:** The root signature is like a function signature for your shaders - it declares the parameters (resources) that shaders expect to receive.

## Table of Contents

1. [Root Signature Fundamentals](#root-signature-fundamentals)
2. [Root Parameter Types](#root-parameter-types)
3. [Performance Hierarchy](#performance-hierarchy)
4. [Design Patterns](#design-patterns)
5. [Visibility and Optimization](#visibility-and-optimization)
6. [Version 1.1 Enhancements](#version-11-enhancements)
7. [Implementation Examples](#implementation-examples)
8. [Best Practices](#best-practices)

---

## Root Signature Fundamentals

### Size Constraints

| Constraint | Value |
|------------|-------|
| Maximum size | **64 DWORDs** (256 bytes) |
| Root constant | 1 DWORD per 32-bit value |
| Root descriptor | 2 DWORDs (64-bit GPU address) |
| Descriptor table | 1 DWORD |

### Root Signature Components

```cpp
D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
rootSigDesc.NumParameters = numParams;           // Root parameters
rootSigDesc.pParameters = params;                // Parameter array
rootSigDesc.NumStaticSamplers = numSamplers;     // Static samplers
rootSigDesc.pStaticSamplers = samplers;          // Sampler array
rootSigDesc.Flags = flags;                       // Configuration flags
```

### Common Flags

```cpp
D3D12_ROOT_SIGNATURE_FLAGS flags =
    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |  // Use vertex input
    D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |        // Optimize: deny HS
    D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |      // Optimize: deny DS
    D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;     // Optimize: deny GS
```

---

## Root Parameter Types

### 1. Root Constants (Fastest)

**Cost:** 1 DWORD per 32-bit value
**Indirection:** None
**Best for:** Per-draw data, small frequently-changing values

```cpp
// Definition
D3D12_ROOT_PARAMETER1 param = {};
param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
param.Constants.ShaderRegister = 0;      // b0 in HLSL
param.Constants.RegisterSpace = 0;
param.Constants.Num32BitValues = 16;     // 16 floats = 64 bytes
param.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

// Usage at runtime
struct PerDrawConstants {
    XMMATRIX worldMatrix;  // 16 floats
};
PerDrawConstants constants = { ... };
cmdList->SetGraphicsRoot32BitConstants(0, 16, &constants, 0);
```

**HLSL:**
```hlsl
cbuffer PerDrawCB : register(b0) {
    float4x4 WorldMatrix;
};
```

### 2. Root Descriptors (Single Indirection)

**Cost:** 2 DWORDs (64-bit GPU virtual address)
**Indirection:** 1 level
**Best for:** Frequently-accessed CBVs, raw buffers

```cpp
// Definition
D3D12_ROOT_PARAMETER1 param = {};
param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;  // or SRV, UAV
param.Descriptor.ShaderRegister = 1;    // b1 in HLSL
param.Descriptor.RegisterSpace = 0;
param.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

// Usage at runtime
cmdList->SetGraphicsRootConstantBufferView(1, cbvGpuAddress);
// or
cmdList->SetGraphicsRootShaderResourceView(1, srvGpuAddress);
cmdList->SetGraphicsRootUnorderedAccessView(1, uavGpuAddress);
```

**Supported Types:**
- `D3D12_ROOT_PARAMETER_TYPE_CBV` - Constant buffer view
- `D3D12_ROOT_PARAMETER_TYPE_SRV` - Shader resource view (raw/structured buffers only)
- `D3D12_ROOT_PARAMETER_TYPE_UAV` - Unordered access view (raw/structured buffers only)

**Limitations:**
- SRV/UAV only work with buffers, not textures
- No bounds checking (shader must be careful)
- Cannot index into arrays

### 3. Descriptor Tables (Most Flexible)

**Cost:** 1 DWORD
**Indirection:** 2 levels (table → heap → resource)
**Best for:** Textures, large resource collections, bindless

```cpp
// Define ranges
D3D12_DESCRIPTOR_RANGE1 ranges[2] = {};

// Range 0: Textures
ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
ranges[0].NumDescriptors = 8;           // t0-t7
ranges[0].BaseShaderRegister = 0;
ranges[0].RegisterSpace = 0;
ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
ranges[0].OffsetInDescriptorsFromTableStart = 0;

// Range 1: Constant buffers
ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
ranges[1].NumDescriptors = 4;           // b0-b3
ranges[1].BaseShaderRegister = 0;
ranges[1].RegisterSpace = 0;
ranges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
ranges[1].OffsetInDescriptorsFromTableStart = 8;

// Define table parameter
D3D12_ROOT_PARAMETER1 param = {};
param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
param.DescriptorTable.NumDescriptorRanges = 2;
param.DescriptorTable.pDescriptorRanges = ranges;
param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

// Usage at runtime
cmdList->SetGraphicsRootDescriptorTable(2, gpuHandleToTableStart);
```

### 4. Static Samplers

**Cost:** 0 DWORDs (baked into signature)
**Best for:** Common samplers that never change

```cpp
D3D12_STATIC_SAMPLER_DESC staticSampler = {};
staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
staticSampler.MipLODBias = 0;
staticSampler.MaxAnisotropy = 16;
staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
staticSampler.MinLOD = 0.0f;
staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
staticSampler.ShaderRegister = 0;       // s0 in HLSL
staticSampler.RegisterSpace = 0;
staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
```

---

## Performance Hierarchy

### Access Speed (Fastest to Slowest)

```
┌─────────────────────────────────────────────────────────────┐
│  Root Constants                                              │
│  ├── 0 indirections                                         │
│  ├── Directly indexable                                     │
│  └── Fastest possible access                                │
├─────────────────────────────────────────────────────────────┤
│  Root Descriptors (CBV/SRV/UAV)                             │
│  ├── 1 indirection                                          │
│  ├── No bounds checking                                     │
│  └── Very fast for single resources                         │
├─────────────────────────────────────────────────────────────┤
│  Descriptor Tables                                           │
│  ├── 2 indirections                                         │
│  ├── Bounds checking enabled                                │
│  └── Most flexible, supports textures                       │
└─────────────────────────────────────────────────────────────┘
```

### Cost to Change

| Operation | Relative Cost |
|-----------|---------------|
| Set Root Constants | Lowest |
| Set Root Descriptor | Low |
| Set Descriptor Table | Medium |
| Change Root Signature | **Highest** (clears all bindings) |

---

## Design Patterns

### Pattern 1: Per-Frame / Per-Object Split

```cpp
// Slot 0: Per-frame constants (root constants, 4 DWORDs)
//   - View-projection matrix offset into CBV
// Slot 1: Per-frame CBV (root descriptor, 2 DWORDs)
//   - Large per-frame data
// Slot 2: Per-object constants (root constants, 16 DWORDs)
//   - World matrix
// Slot 3: Material textures (descriptor table, 1 DWORD)
//   - Albedo, normal, roughness, etc.

// Total: 23 DWORDs
```

### Pattern 2: Bindless Rendering

```cpp
// Slot 0: Per-draw constants (root constants)
//   - Object ID, material ID for indexing
// Slot 1: Global resource table (descriptor table)
//   - All textures in one big table
// Slot 2: Global buffer table (descriptor table)
//   - All structured buffers

// HLSL uses indices to access:
// Texture2D textures[] : register(t0, space0);
// StructuredBuffer<Material> materials : register(t0, space1);
```

### Pattern 3: Uber Root Signature

**Used by AAA games like Cyberpunk 2077:**

```cpp
// One root signature for ALL graphics shaders
// One root signature for ALL compute shaders

// Benefits:
// - Never change root signature during frame
// - Simplified binding code
// - Works well with bindless

// Drawbacks:
// - May waste some root signature space
// - Less optimal for simple shaders
```

### Pattern 4: Render Pass Grouping

```cpp
// Shadow Pass Root Signature (minimal)
// - Just MVP matrix as root constants
// - Total: 16 DWORDs

// GBuffer Pass Root Signature
// - Per-object transform (root constants)
// - Material textures (descriptor table)
// - Total: 20 DWORDs

// Lighting Pass Root Signature
// - GBuffer textures (descriptor table)
// - Light data (root descriptor)
// - Total: 8 DWORDs
```

---

## Visibility and Optimization

### Shader Visibility Options

```cpp
D3D12_SHADER_VISIBILITY_ALL             // All shader stages
D3D12_SHADER_VISIBILITY_VERTEX          // Vertex shader only
D3D12_SHADER_VISIBILITY_HULL            // Hull shader only
D3D12_SHADER_VISIBILITY_DOMAIN          // Domain shader only
D3D12_SHADER_VISIBILITY_GEOMETRY        // Geometry shader only
D3D12_SHADER_VISIBILITY_PIXEL           // Pixel shader only
D3D12_SHADER_VISIBILITY_AMPLIFICATION   // Amplification shader (mesh shaders)
D3D12_SHADER_VISIBILITY_MESH            // Mesh shader
```

### Visibility Best Practices

```cpp
// GOOD: Restrict visibility
param.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;  // Only VS needs it

// BAD: Unnecessary visibility
param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;     // Wasteful if only VS uses it
```

**Why it matters:** Some hardware can optimize descriptor access when visibility is restricted.

### Deny Flags for Optimization

```cpp
// If not using tessellation or geometry shaders:
D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
```

---

## Version 1.1 Enhancements

### Descriptor Range Flags

```cpp
// Default: Static (best optimization)
D3D12_DESCRIPTOR_RANGE_FLAG_NONE

// Descriptors may change during recording
D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE

// Data may change during recording
D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE

// Data won't change after set at execute
D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE

// Data is completely static (best optimization)
D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC
```

### Root Descriptor Flags

```cpp
D3D12_ROOT_DESCRIPTOR_FLAG_NONE                           // Static (default)
D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE                  // May change
D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE
D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC                    // Best optimization
```

### Version 1.1 Creation

```cpp
D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedDesc = {};
versionedDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
versionedDesc.Desc_1_1.NumParameters = numParams;
versionedDesc.Desc_1_1.pParameters = params1_1;
versionedDesc.Desc_1_1.NumStaticSamplers = numSamplers;
versionedDesc.Desc_1_1.pStaticSamplers = staticSamplers;
versionedDesc.Desc_1_1.Flags = flags;

ComPtr<ID3DBlob> signature;
ComPtr<ID3DBlob> error;
HRESULT hr = D3D12SerializeVersionedRootSignature(&versionedDesc,
    &signature, &error);
```

---

## Implementation Examples

### Example 1: Simple Scene Rendering

```cpp
// Root signature for basic scene rendering
// Total: 21 DWORDs

D3D12_ROOT_PARAMETER1 params[4] = {};

// Slot 0: Per-frame constants (view-proj matrix) - 16 DWORDs
params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
params[0].Constants.ShaderRegister = 0;  // b0
params[0].Constants.Num32BitValues = 16;
params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

// Slot 1: Per-object world matrix CBV - 2 DWORDs
params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
params[1].Descriptor.ShaderRegister = 1;  // b1
params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

// Slot 2: Material data CBV - 2 DWORDs
params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
params[2].Descriptor.ShaderRegister = 2;  // b2
params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

// Slot 3: Texture table - 1 DWORD
D3D12_DESCRIPTOR_RANGE1 texRange = {};
texRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
texRange.NumDescriptors = 4;  // t0-t3
texRange.BaseShaderRegister = 0;
texRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;

params[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
params[3].DescriptorTable.NumDescriptorRanges = 1;
params[3].DescriptorTable.pDescriptorRanges = &texRange;
params[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

// Static samplers (free!)
D3D12_STATIC_SAMPLER_DESC samplers[2] = {};
// Linear sampler at s0
samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
samplers[0].AddressU = samplers[0].AddressV = samplers[0].AddressW =
    D3D12_TEXTURE_ADDRESS_MODE_WRAP;
samplers[0].ShaderRegister = 0;
samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

// Point sampler at s1
samplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
samplers[1].AddressU = samplers[1].AddressV = samplers[1].AddressW =
    D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
samplers[1].ShaderRegister = 1;
samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
```

### Example 2: Multi-Pipeline Shared Signature

```cpp
// Shared signature for terrain, creatures, and vegetation
// All use same resource layout for efficient batching

D3D12_ROOT_PARAMETER1 sharedParams[5] = {};

// 0: View data (per-frame)
// 1: Transform data (per-object)
// 2: Material index (per-draw, root constant)
// 3: All textures (bindless table)
// 4: All materials (structured buffer)

// Create ONCE, use for multiple PSOs
device->CreateRootSignature(0, signature->GetBufferPointer(),
    signature->GetBufferSize(), IID_PPV_ARGS(&m_sharedRootSignature));

// All PSOs reference same signature
terrainPSO.pRootSignature = m_sharedRootSignature.Get();
creaturePSO.pRootSignature = m_sharedRootSignature.Get();
vegetationPSO.pRootSignature = m_sharedRootSignature.Get();
```

### Example 3: Compute Shader Signature

```cpp
// Compute-only root signature
D3D12_ROOT_PARAMETER1 computeParams[3] = {};

// Slot 0: Constants
computeParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
computeParams[0].Constants.Num32BitValues = 4;  // dispatchSize, time, etc.
computeParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;  // Only option for compute

// Slot 1: Input SRV (structured buffer)
computeParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
computeParams[1].Descriptor.ShaderRegister = 0;
computeParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

// Slot 2: Output UAV
computeParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
computeParams[2].Descriptor.ShaderRegister = 0;
computeParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

// No input assembler flag for compute
rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
```

---

## Best Practices

### Parameter Ordering

```cpp
// Order by frequency of change (most frequent first)
// and by importance for low-latency access

// Slot 0: Per-draw constants (change every draw)
// Slot 1: Per-object data (change per object)
// Slot 2: Per-material data (change per material)
// Slot 3: Per-frame data (change once per frame)
// Slot 4: Global resources (rarely change)
```

### Space Efficiency

```cpp
// GOOD: Use root constants for small, frequent data
param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
param.Constants.Num32BitValues = 4;  // 4 DWORDs for MVP index, material ID, etc.

// BAD: Wasting root space on large infrequent data
param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
param.Constants.Num32BitValues = 64;  // Uses entire root signature!
```

### Caching and Reuse

```cpp
// Cache CPU-side values, only update on change
struct RootSignatureCache {
    D3D12_GPU_VIRTUAL_ADDRESS perFrameCBV;
    D3D12_GPU_DESCRIPTOR_HANDLE textureTable;
    // ... etc
};

void SetRootSignatureIfChanged(const RootSignatureCache& newValues) {
    if (newValues.perFrameCBV != m_cache.perFrameCBV) {
        cmdList->SetGraphicsRootConstantBufferView(0, newValues.perFrameCBV);
        m_cache.perFrameCBV = newValues.perFrameCBV;
    }
    // ... similar for other slots
}
```

### Signature Serialization

```cpp
// Serialize for disk caching
ComPtr<ID3DBlob> serialized;
D3D12SerializeVersionedRootSignature(&desc, &serialized, nullptr);

// Save to disk
WriteFile(cacheFile, serialized->GetBufferPointer(),
    serialized->GetBufferSize());

// Load and create without re-serialization
device->CreateRootSignature(0, loadedData, loadedSize,
    IID_PPV_ARGS(&rootSignature));
```

---

## Common Mistakes to Avoid

### 1. Root Signature Churn
```cpp
// BAD: Changing signature every draw
for (auto& object : objects) {
    cmdList->SetGraphicsRootSignature(object.signature);  // Expensive!
    cmdList->Draw(...);
}

// GOOD: One signature, change only parameters
cmdList->SetGraphicsRootSignature(sharedSignature);
for (auto& object : objects) {
    cmdList->SetGraphicsRoot32BitConstants(0, ...);  // Cheap
    cmdList->Draw(...);
}
```

### 2. Overusing Descriptor Tables
```cpp
// BAD: Table for single CBV
ranges[0].NumDescriptors = 1;  // Just one CBV? Use root descriptor instead!

// GOOD: Root descriptor for single resource
param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
```

### 3. Ignoring Visibility
```cpp
// BAD: All stages for vertex-only data
param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

// GOOD: Restrict to actual usage
param.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
```

---

## References

- [Root Signatures Overview - Microsoft](https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signatures-overview)
- [Root Signature Limits](https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signature-limits)
- [The D3D12 Root Signature Object](https://logins.github.io/graphics/2020/06/26/DX12RootSignatureObject.html)
- [Advanced API Performance: Descriptors - NVIDIA](https://developer.nvidia.com/blog/advanced-api-performance-descriptors/)
- [Shapes and Forms of DX12 Root Signatures](https://asawicki.info/news_1778_shapes_and_forms_of_dx12_root_signatures)
