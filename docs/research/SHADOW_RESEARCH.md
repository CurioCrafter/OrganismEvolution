# Shadow Mapping Research for DirectX 12

## Executive Summary

This document provides comprehensive research on modern shadow mapping techniques for real-time 3D applications, specifically targeting DirectX 12 implementation. Shadow mapping remains the dominant approach in games due to its flexibility and reasonable performance characteristics.

---

## Table of Contents

1. [Cascaded Shadow Maps (CSM)](#cascaded-shadow-maps-csm)
2. [Cascade Splitting Schemes](#cascade-splitting-schemes)
3. [Shadow Filtering Techniques](#shadow-filtering-techniques)
4. [Shadow Bias Techniques](#shadow-bias-techniques)
5. [Modern Shadow Techniques](#modern-shadow-techniques)
6. [DirectX 12 Implementation Best Practices](#directx-12-implementation-best-practices)
7. [Performance Optimization](#performance-optimization)
8. [Recommended Implementation](#recommended-implementation)

---

## Cascaded Shadow Maps (CSM)

### Overview

Cascaded Shadow Maps (CSM), also known as Parallel-Split Shadow Maps (PSSM), are the industry-standard solution for handling perspective aliasing in shadow mapping. The core idea is that objects near the camera require higher shadow map resolution than distant objects.

### How CSM Works

1. **Frustum Partitioning**: The view frustum is divided into multiple subfrusta (cascades)
2. **Per-Cascade Shadow Maps**: Each cascade gets its own shadow map rendered from the light's perspective
3. **Cascade Selection**: During shading, the appropriate cascade is selected based on pixel depth
4. **Shadow Sampling**: The shadow test is performed using the selected cascade's shadow map

### Key Configuration Options

#### Fit to Scene vs. Fit to Cascade

| Approach | Pros | Cons |
|----------|------|------|
| **Fit to Scene** | Stable shadows when camera moves; eliminates "swimming" artifacts | Wastes shadow map resolution |
| **Fit to Cascade** | More efficient resolution usage; tighter frustum fit | Shadows can "swim" as view changes; more complex implementation |

**Recommendation**: Use Fit to Scene for stable shadows, or implement stabilization techniques with Fit to Cascade for efficiency.

#### Cascade Selection Methods

**Interval-Based Selection**:
```hlsl
// Vertex Shader
Output.vDepth = mul(Input.vPosition, m_mWorldView).z;

// Pixel Shader
float4 vCurrentPixelDepth = Input.vDepth;
float4 fComparison = (vCurrentPixelDepth > m_fCascadeFrustumsEyeSpaceDepths);
float fIndex = dot(float4(1, 1, 1, 1), fComparison);
int iCurrentCascadeIndex = (int)min(fIndex, CASCADE_COUNT - 1);
```

**Map-Based Selection**:
- Tests texture coordinates against cascade bounds
- More efficient when cascades don't align perfectly
- Iterates through cascades until valid bounds found

### Cascade Blending

To avoid visible seams between cascades:
```hlsl
// Blend between two cascades in transition zone
float blendFactor = saturate((depth - cascadeEnd + blendWidth) / blendWidth);
float shadow = lerp(shadowCurrent, shadowNext, blendFactor);
```

---

## Cascade Splitting Schemes

### Three Main Approaches

#### 1. Uniform Split Scheme
```
Split[i] = near + (far - near) * i / numCascades
```
- **Pros**: Simple to implement
- **Cons**: Worst perspective aliasing distribution; wastes resolution near camera

#### 2. Logarithmic Split Scheme
```
Split[i] = near * pow(far / near, i / numCascades)
```
- **Pros**: Theoretically optimal for perspective aliasing
- **Cons**: Too aggressive for near plane; can create very thin first cascade

#### 3. Practical Split Scheme (Recommended)
```cpp
float lambda = 0.5f; // Blend factor (0.5 is common default)
for (int i = 1; i < numCascades; i++) {
    float uniform = near + (far - near) * (float)i / numCascades;
    float log = near * pow(far / near, (float)i / numCascades);
    cascadeSplits[i] = lambda * log + (1.0f - lambda) * uniform;
}
```
- **Pros**: Balances near-field quality with far-field coverage
- **Cons**: Requires tuning lambda parameter (0.5-0.9 typical)

### Recommended Split Values

For a typical game camera (near=0.5, far=500, 4 cascades):
| Lambda | Cascade 1 | Cascade 2 | Cascade 3 | Cascade 4 |
|--------|-----------|-----------|-----------|-----------|
| 0.5 | 0.5-5.0 | 5.0-25.0 | 25.0-100.0 | 100.0-500.0 |
| 0.75 | 0.5-3.0 | 3.0-15.0 | 15.0-75.0 | 75.0-500.0 |
| 0.9 | 0.5-2.0 | 2.0-10.0 | 10.0-50.0 | 50.0-500.0 |

---

## Shadow Filtering Techniques

### Percentage Closer Filtering (PCF)

PCF is the most common shadow filtering technique, providing soft shadow edges by averaging multiple depth comparisons.

#### Basic PCF Implementation
```hlsl
float PCF_Shadow(Texture2D shadowMap, SamplerComparisonState samplerState,
                 float3 shadowCoord, float2 texelSize) {
    float shadow = 0.0f;

    // 3x3 kernel
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float2 offset = float2(x, y) * texelSize;
            shadow += shadowMap.SampleCmpLevelZero(samplerState,
                shadowCoord.xy + offset, shadowCoord.z);
        }
    }
    return shadow / 9.0f;
}
```

#### Hardware-Accelerated PCF
DirectX 12 supports hardware PCF through `SamplerComparisonState`:
```hlsl
// Single sample with hardware 2x2 bilinear PCF
float shadow = shadowMap.SampleCmpLevelZero(shadowSampler,
    shadowCoord.xy, shadowCoord.z);
```

#### Poisson Disk Sampling (Recommended for Quality)
```hlsl
static const float2 poissonDisk[16] = {
    float2(-0.94201624, -0.39906216), float2(0.94558609, -0.76890725),
    float2(-0.094184101, -0.92938870), float2(0.34495938, 0.29387760),
    // ... more samples
};

float PoissonPCF(Texture2D shadowMap, float3 shadowCoord, float radius) {
    float shadow = 0.0f;
    for (int i = 0; i < 16; i++) {
        float2 offset = poissonDisk[i] * radius;
        shadow += shadowMap.SampleCmpLevelZero(shadowSampler,
            shadowCoord.xy + offset, shadowCoord.z);
    }
    return shadow / 16.0f;
}
```

### Variance Shadow Maps (VSM)

VSM stores depth moments (mean and variance) allowing hardware filtering and blur passes.

#### Shadow Map Generation
```hlsl
// Store depth and depth^2
float depth = input.LightSpacePos.z;
return float2(depth, depth * depth);
```

#### Shadow Lookup with Chebyshev's Inequality
```hlsl
float VSM_Shadow(float2 moments, float fragmentDepth) {
    float mean = moments.x;
    float variance = max(moments.y - mean * mean, 0.0001f);

    float d = fragmentDepth - mean;
    float pMax = variance / (variance + d * d);

    // Light bleeding reduction
    pMax = smoothstep(0.2f, 1.0f, pMax);

    return (fragmentDepth <= mean) ? 1.0f : pMax;
}
```

#### VSM Pros and Cons
| Pros | Cons |
|------|------|
| Pre-filterable (mipmaps, blur) | Light bleeding artifacts |
| Single texture sample | Requires FP16/FP32 texture |
| Compatible with hardware filtering | More memory usage |
| Fast separable blur | Numerical instability possible |

### Exponential Shadow Maps (ESM)

ESM uses exponential functions to approximate the shadow test.

```hlsl
// Shadow map stores: exp(c * depth)
float ESM_Shadow(float shadowMapValue, float fragmentDepth, float c) {
    return saturate(shadowMapValue * exp(-c * fragmentDepth));
}
```

**Typical c value**: 40-80 (higher = sharper shadows, more artifacts)

### Exponential Variance Shadow Maps (EVSM)

Combines ESM and VSM for best quality with minimal light bleeding.

```hlsl
// Store positive and negative exponential warps
float4 EVSM_Store(float depth, float c) {
    float pos = exp(c * depth);
    float neg = -exp(-c * depth);
    return float4(pos, pos * pos, neg, neg * neg);
}
```

### Percentage-Closer Soft Shadows (PCSS)

PCSS provides physically-based contact-hardening shadows.

#### Algorithm Steps
1. **Blocker Search**: Find average blocker depth in search region
2. **Penumbra Estimation**: Calculate penumbra size based on blocker distance
3. **Variable PCF**: Apply PCF with kernel size based on penumbra

```hlsl
float PCSS_Shadow(float3 shadowCoord, float lightSize) {
    // Step 1: Blocker search
    float avgBlockerDepth = FindBlockerDepth(shadowCoord, lightSize);
    if (avgBlockerDepth < 0) return 1.0f; // No blockers

    // Step 2: Penumbra estimation
    float penumbraSize = (shadowCoord.z - avgBlockerDepth) * lightSize / avgBlockerDepth;

    // Step 3: PCF with variable kernel
    return PCF_Shadow(shadowCoord, penumbraSize);
}
```

### Filtering Comparison

| Technique | Quality | Performance | Memory | Best Use Case |
|-----------|---------|-------------|--------|---------------|
| PCF (3x3) | Low | Fast | Low | Mobile/Low-end |
| PCF (5x5+) | Medium | Medium | Low | General purpose |
| PCF Poisson | High | Medium | Low | Quality shadows |
| VSM | Medium | Fast | Medium | Large blur radius |
| ESM | Medium | Fast | Low | Bandwidth-limited |
| EVSM | High | Medium | High | Quality + blur |
| PCSS | Highest | Slow | Low | Hero lights only |

---

## Shadow Bias Techniques

### The Problem: Shadow Acne and Peter Panning

- **Shadow Acne**: Self-shadowing artifacts from depth precision limitations
- **Peter Panning**: Shadows detaching from objects due to excessive bias

### Bias Techniques

#### 1. Constant Depth Bias
```hlsl
float bias = 0.005f;
float shadow = (fragmentDepth - bias > shadowMapDepth) ? 0.0f : 1.0f;
```

#### 2. Slope-Scaled Depth Bias (Recommended)
```hlsl
float CalculateBias(float3 normal, float3 lightDir) {
    float cosAngle = saturate(dot(normal, lightDir));
    float slopeScale = sqrt(1.0f - cosAngle * cosAngle) / cosAngle;
    return 0.001f + 0.005f * slopeScale;
}
```

#### 3. Normal Offset Bias
```hlsl
float3 ApplyNormalOffset(float3 worldPos, float3 normal, float3 lightDir, float texelSize) {
    float cosAngle = saturate(dot(normal, lightDir));
    float normalOffset = texelSize * sqrt(1.0f - cosAngle * cosAngle);
    return worldPos + normal * normalOffset;
}
```

#### 4. Receiver Plane Depth Bias
```hlsl
float ReceiverPlaneBias(float3 shadowCoord, float2 texelSize) {
    float2 dz_duv = float2(ddx(shadowCoord.z), ddy(shadowCoord.z));
    return dot(texelSize, abs(dz_duv));
}
```

### Combined Bias Strategy (Recommended)
```hlsl
float3 BiasedPosition(float3 worldPos, float3 normal, float3 lightDir,
                      float depthBias, float normalBias) {
    float NdotL = dot(normal, lightDir);
    float slopeBias = depthBias * tan(acos(saturate(NdotL)));
    slopeBias = clamp(slopeBias, 0, depthBias * 2);

    float3 normalOffset = normal * normalBias * (1.0f - NdotL);
    return worldPos + normalOffset + lightDir * slopeBias;
}
```

### Front-Face Culling Trick
Render shadow map with front-face culling to push depth values further:
```cpp
// During shadow map render pass
rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT;
```
**Note**: Only works for closed meshes without holes.

---

## Modern Shadow Techniques

### Virtual Shadow Maps (VSM/Nanite Shadows)

Used in Unreal Engine 5, virtual shadow maps provide:
- Consistent texel density regardless of distance
- Efficient memory usage through sparse textures
- Per-page caching and streaming

### Ray-Traced Shadows

Modern GPUs with DXR support offer ray-traced shadows:
- Pixel-perfect accuracy
- Natural soft shadows from area lights
- No aliasing or bias issues
- Higher performance cost

### Hybrid Approaches

Many modern games combine techniques:
1. **CSM + Ray Tracing**: CSM for distant shadows, RT for near-field
2. **CSM + PCSS**: PCSS for hero lights, PCF for secondary lights
3. **Denoised RT**: Low sample RT with temporal denoising

---

## DirectX 12 Implementation Best Practices

### Resource Management

```cpp
// Create shadow map texture array for CSM
D3D12_RESOURCE_DESC shadowMapDesc = {};
shadowMapDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
shadowMapDesc.Width = SHADOW_MAP_SIZE;
shadowMapDesc.Height = SHADOW_MAP_SIZE;
shadowMapDesc.DepthOrArraySize = CASCADE_COUNT;
shadowMapDesc.MipLevels = 1;
shadowMapDesc.Format = DXGI_FORMAT_R32_TYPELESS;
shadowMapDesc.SampleDesc.Count = 1;
shadowMapDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
```

### Descriptor Setup

```cpp
// Depth Stencil View for rendering
D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
dsvDesc.Texture2DArray.ArraySize = 1;
dsvDesc.Texture2DArray.FirstArraySlice = cascadeIndex;

// Shader Resource View for sampling
D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
srvDesc.Texture2DArray.ArraySize = CASCADE_COUNT;
srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
```

### Pipeline State for Shadow Pass

```cpp
D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPsoDesc = {};
shadowPsoDesc.pRootSignature = m_shadowRootSignature.Get();
shadowPsoDesc.VS = shadowVertexShader;
shadowPsoDesc.PS = {}; // NULL pixel shader for depth-only
shadowPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT; // Front-face culling
shadowPsoDesc.RasterizerState.DepthBias = 100000;
shadowPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
shadowPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
shadowPsoDesc.DepthStencilState.DepthEnable = TRUE;
shadowPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
shadowPsoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
```

### Async Shadow Rendering

DirectX 12 enables parallel shadow map generation:
```cpp
// Execute shadow pass on separate command list
m_shadowCommandList->Reset(m_shadowAllocator.Get(), m_shadowPSO.Get());
// Record shadow rendering commands...
m_shadowCommandList->Close();

// Submit to graphics queue
ID3D12CommandList* ppShadowLists[] = { m_shadowCommandList.Get() };
m_commandQueue->ExecuteCommandLists(1, ppShadowLists);
```

---

## Performance Optimization

### Shadow Map Resolution Guidelines

| Scene Type | Resolution per Cascade | Total Cascades |
|------------|----------------------|----------------|
| Small indoor | 1024x1024 | 2-3 |
| Medium outdoor | 2048x2048 | 3-4 |
| Large open world | 2048x2048 - 4096x4096 | 4-5 |

### Optimization Techniques

1. **Cull shadow casters per cascade**: Don't render objects outside cascade bounds
2. **LOD for shadow rendering**: Use lower LOD meshes for shadow passes
3. **Texture atlasing**: Store all cascades in one texture for reduced state changes
4. **Instanced rendering**: Batch similar objects in shadow pass
5. **Depth-only pass**: Use null pixel shader for shadow map generation

### Shadow Map Stabilization

Prevent shadow "swimming" when camera moves:
```cpp
// Snap shadow map to texel boundaries
float texelsPerUnit = shadowMapSize / (2.0f * orthoRadius);
lightViewMatrix[3][0] = floor(lightViewMatrix[3][0] * texelsPerUnit) / texelsPerUnit;
lightViewMatrix[3][1] = floor(lightViewMatrix[3][1] * texelsPerUnit) / texelsPerUnit;
```

### Temporal Stability

For jitter-free shadows:
1. Use world-space stable jitter patterns
2. Apply temporal filtering to shadow results
3. Consider VSM/ESM for natural blur that hides temporal artifacts

---

## Recommended Implementation

### For This Evolution Simulation Project

Given the nature of an outdoor terrain simulation with a day/night cycle:

#### Recommended Setup
- **Technique**: Cascaded Shadow Maps with 4 cascades
- **Filtering**: PCF with 5x5 Poisson disk sampling
- **Split Scheme**: Practical split with lambda=0.75
- **Resolution**: 2048x2048 per cascade
- **Bias**: Slope-scaled + normal offset

#### Implementation Priority
1. Basic CSM with PCF filtering
2. Proper bias configuration
3. Cascade blending
4. Shadow map stabilization
5. PCSS for sun (optional, quality setting)

#### Shader Structure
```hlsl
cbuffer ShadowConstants : register(b1) {
    matrix g_LightViewProj[4];      // Per-cascade matrices
    float4 g_CascadeSplits;          // Split depths
    float4 g_ShadowMapSize;          // Resolution info
    float g_ShadowBias;
    float g_NormalBias;
    float g_PCFRadius;
    float g_BlendWidth;
};

float CalculateShadow(float3 worldPos, float3 normal, float viewDepth) {
    // Select cascade
    int cascade = SelectCascade(viewDepth);

    // Apply bias
    float3 biasedPos = ApplyBias(worldPos, normal, g_LightDirection);

    // Transform to shadow space
    float4 shadowCoord = mul(float4(biasedPos, 1.0f), g_LightViewProj[cascade]);
    shadowCoord.xyz /= shadowCoord.w;

    // PCF sampling
    float shadow = PCF_Poisson(shadowCoord.xyz, cascade);

    // Cascade blending
    shadow = BlendCascades(shadow, viewDepth, cascade);

    return shadow;
}
```

---

## References and Further Reading

### Microsoft Documentation
- [Cascaded Shadow Maps - Microsoft Learn](https://learn.microsoft.com/en-us/windows/win32/dxtecharts/cascaded-shadow-maps)
- [Common Techniques to Improve Shadow Depth Maps](https://learn.microsoft.com/en-us/windows/win32/dxtecharts/common-techniques-to-improve-shadow-depth-maps)

### NVIDIA Resources
- [GPU Gems 3: Parallel-Split Shadow Maps](https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-10-parallel-split-shadow-maps-programmable-gpus)
- [GPU Gems 3: Summed-Area Variance Shadow Maps](https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-8-summed-area-variance-shadow-maps)
- [GPU Gems: Shadow Map Antialiasing](https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing)
- [Percentage-Closer Soft Shadows (PCSS)](https://developer.download.nvidia.com/shaderlibrary/docs/shadow_PCSS.pdf)

### AMD Resources
- [ShadowFX Library for DirectX 12](https://gpuopen.com/shadowfx-effect-library-for-directx-12/)
- [GitHub: ShadowFX](https://github.com/GPUOpen-Effects/ShadowFX)

### Academic Papers
- [Variance Shadow Maps - Donnelly & Lauritzen](https://pierremezieres.github.io/site-co-master/references/vsm_paper.pdf)
- [Exponential Shadow Maps - Annen et al.](https://jankautz.com/publications/esm_gi08.pdf)
- [Variance Soft Shadow Mapping](https://jankautz.com/publications/VSSM_PG2010.pdf)

### Tutorials and Guides
- [LearnOpenGL: Shadow Mapping](https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping)
- [OGLDev: PCF Tutorial](https://www.ogldev.org/www/tutorial42/tutorial42.html)
- [The Danger Zone: A Sampling of Shadow Techniques](https://therealmjp.github.io/posts/shadow-maps/)
- [Contact-Hardening Soft Shadows Made Fast](https://www.gamedev.net/tutorials/programming/graphics/contact-hardening-soft-shadows-made-fast-r4906/)
- [Fabien Sanglard: Soft Shadows with PCF](https://www.fabiensanglard.net/shadowmappingPCF/index.php)
- [Fabien Sanglard: VSM](https://fabiensanglard.net/shadowmappingVSM/)

### Sample Code
- [DirectX SDK: CascadedShadowMaps11](https://github.com/walbourn/directx-sdk-samples-reworked/blob/main/CascadedShadowMaps11/CascadedShadowMaps11.cpp)
- [GitHub: Cascaded Shadow Map DX12](https://github.com/hlembeck/CascadedShadowMap)
- [Matt's Webcorner: Variance Shadow Maps](https://graphics.stanford.edu/~mdfisher/Shadows.html)

---

*Document created: January 2025*
*For use in OrganismEvolution DirectX 12 project*
