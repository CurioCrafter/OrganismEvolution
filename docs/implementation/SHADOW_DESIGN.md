# Shadow System Design Document

## Overview

This document describes the implementation of the Cascaded Shadow Map (CSM) system for the OrganismEvolution DirectX 12 simulation. The system provides high-quality dynamic shadows for terrain, creatures, and vegetation with support for the day/night cycle.

---

## Architecture

### System Components

```
┌─────────────────────────────────────────────────────────────────┐
│                       Shadow Pipeline                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────────┐     ┌──────────────────────────────────┐  │
│  │  ShadowMap_DX12  │     │  CascadedShadowMapDX12           │  │
│  │  (Legacy single) │     │  (4 cascade array)               │  │
│  └──────────────────┘     └──────────────────────────────────┘  │
│           │                           │                          │
│           └───────────┬───────────────┘                          │
│                       ▼                                          │
│              ┌─────────────────┐                                 │
│              │   Shadow.hlsl   │                                 │
│              │  (Depth pass)   │                                 │
│              └─────────────────┘                                 │
│                       │                                          │
│                       ▼                                          │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  │
│  │Terrain.hlsl │  │Creature.hlsl│  │Vegetation.hlsl          │  │
│  │ (CSM sample)│  │ (CSM sample)│  │ (CSM sample)            │  │
│  └─────────────┘  └─────────────┘  └─────────────────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### File Structure

```
src/graphics/
├── ShadowMap_DX12.h          # Shadow map class declarations
├── ShadowMap_DX12.cpp        # Shadow map implementation
└── ...

shaders/hlsl/
├── Shadow.hlsl               # Shadow depth pass shaders
├── Terrain.hlsl              # Terrain shader with CSM integration
├── Creature.hlsl             # Creature shader with CSM integration
└── Vegetation.hlsl           # Vegetation shader with CSM integration
```

---

## C++ Implementation

### CascadedShadowMapDX12 Class

The main shadow map class supports 4 cascades with configurable resolution.

#### Key Members

```cpp
class CascadedShadowMapDX12 {
    // Shadow map array (Texture2DArray with 4 slices)
    ComPtr<ID3D12Resource> m_shadowMapArray;

    // Per-cascade DSV handles for rendering
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 4> m_dsvHandles;

    // Single SRV for shader sampling
    D3D12_GPU_DESCRIPTOR_HANDLE m_srvGpuHandle;

    // Per-cascade light space matrices
    std::array<XMMATRIX, 4> m_cascadeViewProj;

    // Cascade split distances (view-space)
    std::array<float, 4> m_cascadeSplits = {15.0f, 50.0f, 150.0f, 500.0f};

    // Resolution per cascade
    uint32_t m_cascadeSize = 2048;
};
```

#### Key Methods

| Method | Description |
|--------|-------------|
| `init()` | Creates Texture2DArray, DSVs, and SRV |
| `updateCascades()` | Calculates light matrices for each cascade |
| `beginCascade()` | Sets up rendering for a specific cascade |
| `endShadowPass()` | Transitions resource for shader sampling |
| `getCSMConstants()` | Returns constant buffer data for shaders |

### CSMConstants Structure

```cpp
struct CSMConstants {
    XMFLOAT4X4 cascadeViewProj[4];  // Light space matrices
    XMFLOAT4 cascadeSplits;          // View-space split depths
    XMFLOAT4 cascadeOffsets[4];      // Reserved for atlas mode
    XMFLOAT4 cascadeScales[4];       // Reserved for atlas mode
};
```

---

## HLSL Implementation

### Shadow Pass Shaders (Shadow.hlsl)

#### Vertex Shader - Terrain/Vegetation
```hlsl
cbuffer ShadowConstants : register(b0) {
    float4x4 lightSpaceMatrix;
    float4x4 model;
};

VS_SHADOW_OUTPUT VS_Shadow(VS_SHADOW_INPUT input) {
    VS_SHADOW_OUTPUT output;
    float4 worldPos = mul(model, float4(input.position, 1.0));
    output.position = mul(lightSpaceMatrix, worldPos);
    return output;
}
```

#### Vertex Shader - Instanced Creatures
```hlsl
VS_SHADOW_OUTPUT VS_CreatureShadow(VS_CREATURE_SHADOW_INPUT input) {
    VS_SHADOW_OUTPUT output;

    // Reconstruct instance model matrix
    float4x4 instanceModel = float4x4(
        input.instModelRow0,
        input.instModelRow1,
        input.instModelRow2,
        input.instModelRow3
    );

    float4 worldPos = mul(instanceModel, float4(input.position, 1.0));
    output.position = mul(lightSpaceMatrixInst, worldPos);
    return output;
}
```

#### Pixel Shader
```hlsl
void PS_Shadow(VS_SHADOW_OUTPUT input) {
    // Empty - depth written automatically by rasterizer
}
```

### Shadow Sampling Functions

Integrated into each main pass shader:

#### Cascade Selection
```hlsl
int selectCascade(float viewDepth) {
    int cascade = CSM_CASCADE_COUNT - 1;

    [unroll]
    for (int i = 0; i < CSM_CASCADE_COUNT; i++) {
        float splitDist = (i == 0) ? cascadeSplits.x :
                          (i == 1) ? cascadeSplits.y :
                          (i == 2) ? cascadeSplits.z : cascadeSplits.w;

        if (viewDepth < splitDist) {
            cascade = i;
            break;
        }
    }

    return cascade;
}
```

#### PCF Filtering (3x3 kernel)
```hlsl
float PCFFilterCSM(int cascade, float3 projCoords) {
    float shadow = 0.0;
    float texelSize = 1.0 / SHADOW_MAP_SIZE;

    [unroll]
    for (int x = -1; x <= 1; x++) {
        [unroll]
        for (int y = -1; y <= 1; y++) {
            float2 offset = float2(float(x), float(y)) * texelSize;
            float3 sampleCoord = float3(projCoords.xy + offset, cascade);
            shadow += shadowMapCSM.SampleCmpLevelZero(
                shadowSampler, sampleCoord, projCoords.z - SHADOW_BIAS);
        }
    }

    return shadow / 9.0;
}
```

#### Main Shadow Calculation
```hlsl
float calculateShadowCSM(float3 worldPos, float viewDepth) {
    int cascade = selectCascade(viewDepth);

    float4 fragPosLightSpace = mul(cascadeViewProj[cascade], float4(worldPos, 1.0));
    float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // NDC to texture coordinates
    projCoords.x = projCoords.x * 0.5 + 0.5;
    projCoords.y = -projCoords.y * 0.5 + 0.5;  // Flip Y for DX12

    // Bounds check
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z < 0.0 || projCoords.z > 1.0) {
        return 1.0;
    }

    return PCFFilterCSM(cascade, projCoords);
}
```

---

## Shader Integration

### Terrain.hlsl Integration

```hlsl
// In PSMain:
float viewDepth = length(viewPos - input.fragPos);
float shadow = calculateShadowCSMBlended(input.worldPos, viewDepth);
float shadowFactor = 0.3 + 0.7 * shadow;

// Apply to direct lighting only
float3 directLighting = calculatePBR(...);
float3 ambient = calculateAmbientIBL(...);
result = directLighting * shadowFactor + ambient * 0.4;
```

### Creature.hlsl Integration

```hlsl
// In PSMain:
float viewDepth = length(viewPos - input.fragPos);
float shadow = calculateShadowCSM(input.fragPos, viewDepth);
float shadowFactor = 0.3 + 0.7 * shadow;

// Apply to direct lighting and subsurface
float3 directLighting = calculatePBR(...);
float3 subsurface = calculateSubsurfaceScattering(...);
float3 ambient = calculateAmbientIBL(...);
result = directLighting * shadowFactor + subsurface * shadowFactor + ambient * 0.4;
```

### Vegetation.hlsl Integration

```hlsl
// In PSMain:
float viewDepth = length(viewPos - input.fragPos);
float shadow = calculateShadowCSM(input.fragPos, viewDepth);
float shadowFactor = 0.4 + 0.6 * shadow;  // Higher ambient for foliage

// Apply to direct lighting
float3 ambient = ...;
float3 diffuse = ...;
float3 specular = ...;
result = ambient + (diffuse + specular) * shadowFactor;
```

---

## Resource Bindings

### Texture Registers

| Register | Resource | Description |
|----------|----------|-------------|
| t0 | shadowMap | Legacy single shadow map (unused) |
| t1 | shadowMapCSM | Texture2DArray with 4 cascades |

### Sampler Registers

| Register | Sampler | Description |
|----------|---------|-------------|
| s0 | shadowSampler | SamplerComparisonState for hardware PCF |
| s1 | shadowSamplerPoint | Regular sampler for manual filtering |

### Constant Buffer Registers

| Register | Buffer | Description |
|----------|--------|-------------|
| b0 | Constants | Main frame constants (view, projection, etc.) |
| b2 | CSMConstants | Cascade matrices and split distances |

---

## Configuration

### Default Settings

```cpp
static constexpr uint32_t SHADOW_MAP_SIZE_DX12 = 2048;
static constexpr uint32_t CSM_CASCADE_COUNT = 4;

std::array<float, 4> m_cascadeSplits = {15.0f, 50.0f, 150.0f, 500.0f};
```

### Shadow Bias

```hlsl
static const float SHADOW_BIAS = 0.002;  // Terrain/Creature
static const float SHADOW_BIAS = 0.003;  // Vegetation (higher to avoid self-shadow)
```

### Shadow Factor

Shadow factor controls minimum illumination in shadowed areas:
- **Terrain**: 0.3 + 0.7 * shadow (30% minimum)
- **Creatures**: 0.3 + 0.7 * shadow (30% minimum)
- **Vegetation**: 0.4 + 0.6 * shadow (40% minimum - brighter foliage)
- **Water**: 0.5 + 0.5 * shadow (50% minimum - softer shadows on reflective surfaces)

---

## Cascade Blending

The Terrain shader includes cascade blending for smooth transitions:

```hlsl
float calculateShadowCSMBlended(float3 worldPos, float viewDepth) {
    int cascade = selectCascade(viewDepth);

    float blendFactor = 0.0;
    float blendRange = 5.0;  // Blend over 5 units

    float currentSplit = cascadeSplits[cascade];

    if (cascade < CSM_CASCADE_COUNT - 1) {
        float distToSplit = currentSplit - viewDepth;
        if (distToSplit < blendRange) {
            blendFactor = 1.0 - (distToSplit / blendRange);
        }
    }

    float shadow = PCFFilterCSM(cascade, projCoords);

    if (blendFactor > 0.0 && cascade < CSM_CASCADE_COUNT - 1) {
        float nextShadow = PCFFilterCSM(cascade + 1, nextProjCoords);
        shadow = lerp(shadow, nextShadow, blendFactor);
    }

    return shadow;
}
```

---

## Debug Visualization

Enable cascade debug colors by setting:

```hlsl
static const bool SHOW_CASCADE_DEBUG = true;
```

Cascade colors:
- **Cascade 0**: Red (nearest, 0-15 units)
- **Cascade 1**: Green (15-50 units)
- **Cascade 2**: Blue (50-150 units)
- **Cascade 3**: Yellow (150-500 units)

---

## Performance Considerations

### Memory Usage

| Component | Size |
|-----------|------|
| Shadow map array | 4 x 2048 x 2048 x 4 bytes = 64 MB |
| Constant buffer | 400 bytes |

### GPU Time

Typical frame breakdown:
1. Shadow pass (4 cascades): ~2-4ms
2. Shadow sampling in main pass: ~0.5-1ms additional

### Optimization Tips

1. **Frustum culling**: Only render objects visible in each cascade
2. **LOD selection**: Use lower LOD meshes for shadow passes
3. **Cascade culling**: Skip distant cascades for close-up views
4. **Temporal stability**: Snap shadow map to texel boundaries

---

## Integration Checklist

- [x] C++ ShadowMap_DX12.h/cpp implementation
- [x] Shadow.hlsl depth pass shaders
- [x] CSM constant buffer structure
- [x] Shadow sampling in Terrain.hlsl
- [x] Shadow sampling in Creature.hlsl
- [x] Shadow sampling in Vegetation.hlsl
- [x] Cascade blending (Terrain.hlsl)
- [x] Debug visualization support
- [x] Documentation

---

## Future Enhancements

1. **PCSS (Percentage-Closer Soft Shadows)**: Variable penumbra based on blocker distance
2. **Contact shadows**: Ray-marched shadows for small-scale details
3. **Colored shadows**: Tinted shadows based on light color
4. **Temporal filtering**: Reduce shadow flickering
5. **Virtual shadow maps**: Higher resolution for near-field shadows

---

*Document created: January 2025*
*For OrganismEvolution DirectX 12 Project*
