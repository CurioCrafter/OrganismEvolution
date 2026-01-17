# HLSL Shader Optimization Guide

> GPU architecture considerations and optimization techniques for high-performance shaders in the ModularGameEngine/Forge Engine.

## Table of Contents

1. [GPU Architecture Overview](#gpu-architecture-overview)
2. [Current Shader Analysis](#current-shader-analysis)
3. [ALU vs Texture Balance](#alu-vs-texture-balance)
4. [Divergent Branching](#divergent-branching)
5. [Memory Access Optimization](#memory-access-optimization)
6. [Noise Function Optimization](#noise-function-optimization)
7. [PBR Optimization](#pbr-optimization)
8. [Instancing Optimization](#instancing-optimization)
9. [Compute Shader Optimization](#compute-shader-optimization)
10. [Profiling and Tools](#profiling-and-tools)
11. [References](#references)

---

## GPU Architecture Overview

### SIMD Execution Model

Modern GPUs execute shaders in groups called **waves** (AMD) or **warps** (NVIDIA):
- NVIDIA: 32 threads per warp
- AMD GCN/RDNA: 64/32 threads per wave
- Intel: 8-32 threads per EU

**Key implication**: All threads in a wave execute the same instruction. Divergent branches cause both paths to execute.

### Memory Hierarchy

```
Registers (fastest)
    ↓
L1 Cache / Shared Memory (groupshared)
    ↓
L2 Cache
    ↓
VRAM (slowest)
```

### Latency Hiding

GPUs hide memory latency by switching between waves. Keep enough waves in flight:
- High register usage = fewer concurrent waves
- Complex shaders may become **latency-bound**

---

## Current Shader Analysis

### Creature.hlsl Assessment

| Aspect | Status | Notes |
|--------|--------|-------|
| PBR Implementation | Correct | Standard Cook-Torrance BRDF |
| Noise Functions | Functional | Could benefit from optimization |
| Instancing | Well-implemented | Per-instance matrix reconstruction efficient |
| Cbuffer Layout | Good | Proper padding documented |
| Loop Unrolling | Correct | Uses [unroll] appropriately |

**Identified Optimization Opportunities:**

1. **Noise function redundancy**: `hash3`, `noise3D`, `fbm`, `voronoi` duplicated across Creature.hlsl and Terrain.hlsl
2. **FBM octave loop**: Current implementation uses if-break inside unrolled loop
3. **Voronoi 3x3x3 loop**: 27 iterations could potentially be reduced

### Terrain.hlsl Assessment

| Aspect | Status | Notes |
|--------|--------|-------|
| Water rendering | Comprehensive | Multi-layer waves, fresnel, caustics |
| Biome blending | Correct | Smooth transitions |
| PBR per-biome | Good | Appropriate roughness/metallic values |
| Complexity | High | Consider LOD for distant terrain |

**Identified Optimization Opportunities:**

1. **Water normal calculation**: Computes fbm 6+ times for derivatives
2. **Caustic calculation**: Two voronoi calls in water rendering
3. **Multiple biome checks**: Could use lookup table or texture

### SteeringCompute.hlsl Assessment

| Aspect | Status | Notes |
|--------|--------|-------|
| Thread group size | Good | 64 threads matches common wave sizes |
| Buffer access | Sequential | Good coalescing |
| Branching | Moderate | Type-based branching acceptable |
| Neighbor search | O(n) | Could benefit from spatial hashing |

**Identified Optimization Opportunities:**

1. **FindFlockmates**: Brute-force O(n) search, limited to MAX_NEIGHBORS*10
2. **No spatial hashing**: Comment mentions future implementation
3. **Multiple creature loops**: Could be combined

---

## ALU vs Texture Balance

### Understanding the Balance

Modern GPUs have separate units for:
- **ALU (Arithmetic Logic Unit)**: Math operations
- **Texture Units**: Texture sampling and filtering

Optimal shaders keep both units busy.

### Current Shader Balance

```
Creature.hlsl:
├── High ALU: PBR calculations, noise generation, animation
├── Low TEX: No texture sampling (fully procedural)
└── Assessment: ALU-heavy, could benefit from texture caching

Terrain.hlsl:
├── Very High ALU: Water simulation, multiple noise layers
├── Low TEX: No texture sampling
└── Assessment: Extremely ALU-heavy

Nametag.hlsl:
├── Medium ALU: Digit pattern calculation
├── No TEX: Procedural rendering
└── Assessment: Balanced for its purpose
```

### Optimization Strategies

**For ALU-heavy shaders:**

```hlsl
// OPTION 1: Pre-compute noise to texture (offline or compute shader)
// Instead of:
float noise = fbm(pos * 8.0, 3);

// Use:
float noise = noiseTexture.Sample(noiseSampler, pos.xz * 0.1).r;
```

**For TEX-heavy shaders:**

```hlsl
// Reduce texture samples by:
// 1. Using mipmaps appropriately
// 2. Combining textures (channel packing)
// 3. Using texture arrays instead of individual textures
```

---

## Divergent Branching

### Cost of Divergence

When threads in a wave take different branches:
```hlsl
// BAD: Each thread may take different path
if (heightNormalized < 0.35)
    color = calculateWater();  // 40% of pixels
else if (heightNormalized < 0.42)
    color = calculateSand();   // 10% of pixels
else
    color = calculateGrass();  // 50% of pixels
// Result: All three functions execute for entire wave if any divergence
```

### Current Shader Divergence Analysis

**Terrain.hlsl biome selection:**
```hlsl
// Current implementation has height-based branching
if (heightNormalized < 0.35) { /* water */ }
else if (heightNormalized < 0.42) { /* sand */ }
else if (heightNormalized < 0.65) { /* grass */ }
// etc.
```

**Impact**: Acceptable because:
1. Adjacent pixels likely have similar heights (spatial coherence)
2. Alternative (always computing all biomes) would be more expensive

### Mitigation Strategies

**1. Use lerp/step for simple cases:**
```hlsl
// Instead of:
if (value > threshold)
    result = a;
else
    result = b;

// Use:
result = lerp(b, a, step(threshold, value));
```

**2. Ensure early-outs benefit entire waves:**
```hlsl
// Good: Entire wave likely to take same path
if (index >= g_creatureCount)
    return;  // All threads in wave exit together

// Bad: Individual threads exit
if (me.energy < 0.1)
    return;  // Other threads in wave still execute
```

**3. Reorder conditions by probability:**
```hlsl
// Put most common case first
if (mostLikelyCondition)  // 70% of cases
    // Fast path
else if (lessLikely)      // 25% of cases
    // Medium path
else                      // 5% of cases
    // Rare path
```

---

## Memory Access Optimization

### Constant Buffer Access

**NVIDIA Maxwell+ Optimization:**
Uniform cbuffer loads are up to 32x faster than divergent loads.

```hlsl
// GOOD: All threads read same value (uniform)
float time = g_time;  // Single broadcast

// BAD: Each thread reads different address (scattered)
float values[32];
float val = values[threadIndex];  // May serialize
```

### Structured Buffer Coalescing

```hlsl
// GOOD: Sequential access pattern
uint index = dispatchThreadId.x;
CreatureInput me = g_creatureInputs[index];

// BAD: Random access pattern
uint randomIndex = hash(index) % count;
CreatureInput random = g_creatureInputs[randomIndex];
```

### Groupshared Memory Usage

For compute shaders with shared data:

```hlsl
#define THREAD_GROUP_SIZE 64

groupshared float3 sharedPositions[THREAD_GROUP_SIZE];
groupshared float3 sharedVelocities[THREAD_GROUP_SIZE];

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID, uint GI : SV_GroupIndex)
{
    // Load to shared memory (coalesced)
    sharedPositions[GI] = g_positions[DTid.x];
    sharedVelocities[GI] = g_velocities[DTid.x];

    GroupMemoryBarrierWithGroupSync();

    // Access shared memory (fast, on-chip)
    for (uint i = 0; i < THREAD_GROUP_SIZE; i++)
    {
        float3 neighborPos = sharedPositions[i];
        // Process...
    }
}
```

---

## Noise Function Optimization

### Current Implementation Analysis

**hash3 function:**
```hlsl
float3 hash3(float3 p)
{
    p = frac(p * float3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return frac((p.xxy + p.yxx) * p.zyx);
}
```
- **Status**: Good quality-to-performance ratio
- **ALU**: ~15 operations
- **Quality**: Suitable for visual noise

**noise3D (value noise):**
```hlsl
float noise3D(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);  // Smoothstep

    // 8 hash lookups + trilinear interpolation
    // ...
}
```
- **Status**: Standard implementation
- **ALU**: ~60 operations (8 hash3 calls + interpolation)
- **Consideration**: Perlin noise would be smoother for same cost

**Voronoi:**
```hlsl
float voronoi(float3 p)
{
    // 27 neighbor cell checks (3x3x3)
    [unroll]
    for (int z = -1; z <= 1; z++)
    for (int y = -1; y <= 1; y++)
    for (int x = -1; x <= 1; x++)
    {
        // ...
    }
}
```
- **Status**: Correct but expensive
- **ALU**: ~400+ operations (27 hash3 + 27 distance calculations)

### Optimization Recommendations

**1. Consider 2x2x2 Voronoi (Gustavson optimization):**
```hlsl
// Reduces from 27 to 8 cell checks
// May have minor artifacts at cell boundaries
float voronoiFast(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    float minDist = 1.0;

    [unroll]
    for (int z = 0; z <= 1; z++)
    for (int y = 0; y <= 1; y++)
    for (int x = 0; x <= 1; x++)
    {
        float3 neighbor = float3(x, y, z);
        float3 cellPoint = hash3(i + neighbor);
        float dist = length(neighbor + cellPoint - f);
        minDist = min(minDist, dist);
    }
    return minDist;
}
```

**2. Pre-compute noise to textures:**
```hlsl
// For static patterns, bake to 3D texture
Texture3D<float> noiseVolume : register(t0);

float noise = noiseVolume.Sample(linearSampler, pos * noiseScale).r;
// 1 texture lookup vs. 60+ ALU operations
```

**3. Use gradient noise (Perlin) for smoother results:**
```hlsl
// Perlin noise has continuous derivatives
// Better for normal map generation and smooth terrain
float perlinNoise(float3 p)
{
    // Implementation using gradient interpolation
    // Similar cost to value noise, smoother results
}
```

**4. LOD-based noise reduction:**
```hlsl
// Reduce octaves based on distance
int octaves = (int)lerp(4.0, 1.0, saturate(distance / maxDistance));
float n = fbm(pos, octaves);
```

---

## PBR Optimization

### Current Implementation Review

The PBR implementation follows standard Cook-Torrance BRDF:

```hlsl
float3 calculatePBR(float3 N, float3 V, float3 L, ...)
{
    float3 H = normalize(V + L);
    float3 F0 = lerp(float3(0.04), albedo, metallic);

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    // ...
}
```

**Status**: Physically correct implementation

### Optimization Opportunities

**1. Pre-compute F0 for known materials:**
```hlsl
// Instead of computing F0 each pixel for known material types
static const float3 F0_DIELECTRIC = float3(0.04, 0.04, 0.04);
static const float3 F0_GOLD = float3(1.0, 0.71, 0.29);
static const float3 F0_SILVER = float3(0.95, 0.93, 0.88);
```

**2. Approximate Fresnel for rough surfaces:**
```hlsl
// Schlick-Roughness approximation (already implemented)
// Good balance of quality and performance
float3 F = fresnelSchlickRoughness(NdotV, F0, roughness);
```

**3. Use lookup textures for complex BRDF:**
```hlsl
// Pre-integrated BRDF lookup (for IBL)
Texture2D<float2> brdfLUT : register(t0);
float2 envBRDF = brdfLUT.Sample(linearSampler, float2(NdotV, roughness));
```

**4. Simplify for distant objects:**
```hlsl
// LOD-based PBR simplification
float lod = log2(max(distance / 10.0, 1.0));
if (lod > 2.0)
{
    // Use simplified Lambert + Blinn-Phong
    return simplifiedLighting(N, V, L, albedo);
}
// Full PBR for close objects
```

---

## Instancing Optimization

### Current Implementation

The Creature.hlsl uses efficient instanced rendering:

```hlsl
struct VSInput
{
    // Per-vertex (slot 0)
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;

    // Per-instance (slot 1)
    float4 instModelRow0 : INST_MODEL0;
    float4 instModelRow1 : INST_MODEL1;
    float4 instModelRow2 : INST_MODEL2;
    float4 instModelRow3 : INST_MODEL3;
    float4 instColorType : INST_COLOR;

    uint instanceID : SV_InstanceID;
};
```

**Assessment**: Well-designed for the use case

### Best Practices Applied

1. **Separate vertex streams**: Per-vertex data stays static, per-instance data updated
2. **Matrix as 4 float4s**: Proper for instanced input
3. **SV_InstanceID for variation**: Used for animation offset

### Potential Improvements

**1. Consider quaternion + scale instead of full matrix:**
```hlsl
// Saves 32 bytes per instance (48 vs 80 bytes)
struct InstanceDataCompact
{
    float4 rotationQuat;     // Quaternion rotation
    float3 position;         // World position
    float scale;             // Uniform scale
    float4 colorType;        // Color + type
};

// Reconstruct in vertex shader
float4x4 model = quatToMatrix(rotationQuat) * scale;
model[3] = float4(position, 1.0);
```

**2. Use structured buffer for more instance data:**
```hlsl
// When instance data exceeds vertex input limits
StructuredBuffer<InstanceData> g_instanceData : register(t0);

PSInput VSMain(VSInput input)
{
    InstanceData inst = g_instanceData[input.instanceID];
    // ...
}
```

**3. Sort instances for better cache coherence:**
- Sort by depth (front-to-back for opaque, back-to-front for transparent)
- Sort by material/texture to reduce state changes

---

## Compute Shader Optimization

### Thread Group Sizing

**Current**: `THREAD_GROUP_SIZE = 64`

**Considerations**:
- AMD GCN waves: 64 threads (perfect match)
- AMD RDNA waves: 32 threads (2 waves per group)
- NVIDIA warps: 32 threads (2 warps per group)

**Recommendation**: 64 is a good choice for compatibility

### Occupancy Optimization

**Register usage affects occupancy:**
```hlsl
// High register usage = fewer concurrent waves
// SteeringCompute.hlsl has moderate register usage

// Tips to reduce registers:
// 1. Reuse variables
float temp = compute1();
result1 = process(temp);
temp = compute2();  // Reuse instead of new variable
result2 = process(temp);

// 2. Avoid storing intermediate results
// BAD:
float a = calc1();
float b = calc2();
float c = a + b;
// GOOD:
float c = calc1() + calc2();
```

### Spatial Hashing Improvement

Current brute-force neighbor search could be optimized:

```hlsl
// Proposed spatial hash grid implementation
cbuffer GridConstants : register(b1)
{
    float3 gridMin;
    float cellSize;
    uint3 gridDimensions;
};

StructuredBuffer<uint> g_gridCellStart : register(t2);
StructuredBuffer<uint> g_gridCellEnd : register(t3);
StructuredBuffer<uint> g_sortedIndices : register(t4);

uint3 GetCellCoord(float3 position)
{
    return uint3((position - gridMin) / cellSize);
}

uint GetCellIndex(uint3 coord)
{
    return coord.x + coord.y * gridDimensions.x
         + coord.z * gridDimensions.x * gridDimensions.y;
}

// Query neighbors in O(1) cell lookups instead of O(n)
void FindNeighborsInCell(uint3 cellCoord, ...)
{
    uint cellIdx = GetCellIndex(cellCoord);
    uint start = g_gridCellStart[cellIdx];
    uint end = g_gridCellEnd[cellIdx];

    for (uint i = start; i < end; i++)
    {
        uint neighborIdx = g_sortedIndices[i];
        // Process neighbor...
    }
}
```

---

## Profiling and Tools

### GPU Profilers

| Tool | Platform | Features |
|------|----------|----------|
| **RenderDoc** | Cross-platform | Frame capture, shader debugging |
| **NVIDIA Nsight** | NVIDIA GPUs | Detailed shader profiling |
| **AMD Radeon GPU Profiler** | AMD GPUs | Wave-level analysis |
| **Intel GPA** | Intel GPUs | Frame analysis |
| **PIX** | Windows/Xbox | DirectX debugging |

### Key Metrics to Monitor

1. **GPU Time**: Total frame time on GPU
2. **Shader Time**: Time per shader stage (VS, PS, CS)
3. **Occupancy**: Waves in flight vs. maximum
4. **Register Usage**: Registers per thread
5. **Memory Bandwidth**: GB/s utilization
6. **Cache Hit Rate**: L1/L2 hit percentage

### Debugging Checklist

```
□ Check for GPU stalls (low occupancy)
□ Identify hotspot shaders (highest GPU time)
□ Verify wave divergence is minimal
□ Check memory access patterns (coalescing)
□ Validate cbuffer packing matches C++
□ Test with varying workloads (stress test)
```

### Performance Assertions

```hlsl
// Debug shader to visualize costs
float4 PSDebug(PSInput input) : SV_TARGET
{
    #ifdef DEBUG_OVERDRAW
        return float4(0.1, 0.0, 0.0, 0.1);  // Accumulates to show overdraw
    #endif

    #ifdef DEBUG_COMPLEXITY
        // Color by ALU complexity estimate
        float complexity = estimateComplexity();
        return float4(complexity, 1.0 - complexity, 0.0, 1.0);
    #endif

    // Normal rendering
    return normalShading(input);
}
```

---

## References

### GPU Architecture

- [AMD GCN Architecture Whitepaper](https://developer.amd.com/wordpress/media/2013/07/AMD_GCN_Architecture_White_Paper.pdf)
- [NVIDIA GPU Architecture Overview](https://developer.nvidia.com/content/life-triangle-nvidias-logical-pipeline)
- [Intel Xe Architecture Guide](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-graphics-developer-guides.html)

### Optimization Guides

- [Intel Shader Optimization](https://www.intel.com/content/www/us/en/docs/gpa/user-guide/2024-1/optimize-shaders.html)
- [AMD GPU Open - Optimization](https://gpuopen.com/learn/optimizing-gpu-occupancy-resource-usage-large-thread-groups/)
- [NVIDIA Best Practices Guide](https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/)

### Noise Functions

- [GPU Gems 2 - Perlin Noise](https://developer.nvidia.com/gpugems/gpugems2/part-iii-high-quality-rendering/chapter-26-implementing-improved-perlin-noise)
- [Procedural Tileable Shaders](https://github.com/tuxalin/procedural-tileable-shaders)
- [GPU Voronoi Optimization](https://nickmcd.me/2020/08/01/gpu-accelerated-voronoi/)
- [The Book of Shaders - Noise](https://thebookofshaders.com/12/)

### PBR Implementation

- [LearnOpenGL - PBR](https://learnopengl.com/PBR/Theory)
- [Crash Course in BRDF](https://boksajak.github.io/files/CrashCourseBRDF.pdf)
- [Real Shading in Unreal Engine 4](https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf)

### Constant Buffer Optimization

- [HLSL Packing Rules](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules)
- [Constant Buffer Visualizer](https://maraneshi.github.io/HLSL-ConstantBufferLayoutVisualizer/)
- [Buffer Packing Wiki](https://github.com/microsoft/DirectXShaderCompiler/wiki/Buffer-Packing)

### Tools

- [RenderDoc](https://renderdoc.org/)
- [NVIDIA Nsight Graphics](https://developer.nvidia.com/nsight-graphics)
- [AMD Radeon GPU Profiler](https://gpuopen.com/rgp/)
- [Intel GPA](https://www.intel.com/content/www/us/en/developer/tools/graphics-performance-analyzers/overview.html)
- [PIX for Windows](https://devblogs.microsoft.com/pix/)

---

*Document Version: 1.0*
*Last Updated: January 2026*
*Applies to: ModularGameEngine/Forge Engine HLSL Shaders*
