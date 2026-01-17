# HLSL Best Practices Guide

> Comprehensive coding standards and best practices for HLSL shader development in the ModularGameEngine/Forge Engine project.

## Table of Contents

1. [Naming Conventions](#naming-conventions)
2. [Code Organization](#code-organization)
3. [Constant Buffer Design](#constant-buffer-design)
4. [Data Types and Precision](#data-types-and-precision)
5. [Control Flow Best Practices](#control-flow-best-practices)
6. [Memory Access Patterns](#memory-access-patterns)
7. [Shader Model 6.x Features](#shader-model-6x-features)
8. [Common Pitfalls](#common-pitfalls)
9. [Documentation Standards](#documentation-standards)
10. [References](#references)

---

## Naming Conventions

### Variables

| Type | Convention | Example |
|------|------------|---------|
| Local variables | camelCase | `worldPos`, `lightDir`, `normalizedHeight` |
| Constants | UPPER_SNAKE_CASE | `WATER_LEVEL`, `MAX_NEIGHBORS`, `PI` |
| Function parameters | camelCase | `position`, `roughness`, `metallic` |
| Struct members | camelCase | `fragPos`, `texCoord`, `steeringForce` |
| Global variables | g_prefixCamelCase | `g_maxForce`, `g_creatureCount`, `g_time` |

### Functions

```hlsl
// Good: Clear, descriptive verb-noun pattern
float3 CalculatePBR(float3 N, float3 V, float3 L, ...);
float3 GenerateSkinPattern(float3 pos, float3 normal, float3 baseColor);
float3 LimitMagnitude(float3 v, float maxMag);

// Good: Boolean functions use Is/Has/Can prefix
bool IsPredator(uint type);
bool CanSwim(uint type);

// Avoid: Vague or abbreviated names
float3 calc(float3 n, float3 v);  // Too vague
float3 gsp(float3 p, float3 n);   // Unclear abbreviation
```

### Semantic Names

```hlsl
// Input semantics - use descriptive custom semantics for instance data
float4 instModelRow0 : INST_MODEL0;   // Clear purpose
float4 instColorType : INST_COLOR;     // Self-documenting

// Standard semantics
float3 position : POSITION;
float3 normal : NORMAL;
float2 texCoord : TEXCOORD0;
```

### Shader Entry Points

```hlsl
// Standard naming convention
PSInput VSMain(VSInput input);     // Vertex shader
float4 PSMain(PSInput input);      // Pixel shader
void CSMain(uint3 id : SV_DispatchThreadID);  // Compute shader
```

---

## Code Organization

### File Structure

```hlsl
// 1. Header comment with file purpose
// Creature shader - Ported from OrganismEvolution GLSL
// INSTANCED RENDERING with procedural skin textures and animation

// 2. Constant Buffers
// ============================================================================
// Constant Buffers
// ============================================================================
cbuffer FrameConstants : register(b0) { ... }

// 3. Input/Output Structures
// ============================================================================
// Input/Output Structures
// ============================================================================
struct VSInput { ... };
struct PSInput { ... };

// 4. Static Constants
static const float PI = 3.14159265359;
static const float WATER_LEVEL = 0.35;

// 5. Helper Functions (grouped by purpose)
// ============================================================================
// Procedural Noise Functions
// ============================================================================

// 6. Main Shader Functions
// ============================================================================
// Vertex Shader
// ============================================================================

// ============================================================================
// Pixel Shader
// ============================================================================
```

### Section Separators

Use consistent visual separators for readability:

```hlsl
// ============================================================================
// Section Name
// ============================================================================
```

### Function Grouping

Group related functions together:

```hlsl
// ============================================================================
// PBR Lighting Functions
// ============================================================================

float DistributionGGX(float3 N, float3 H, float roughness) { ... }
float GeometrySchlickGGX(float NdotV, float roughness) { ... }
float GeometrySmith(float3 N, float3 V, float3 L, float roughness) { ... }
float3 fresnelSchlick(float cosTheta, float3 F0) { ... }
float3 calculatePBR(...) { ... }
```

---

## Constant Buffer Design

### Packing Rules

HLSL packs data into 16-byte (4-component vector) boundaries. Variables cannot straddle boundaries.

```hlsl
// BAD: Inefficient packing with implicit padding
cbuffer BadLayout : register(b0)
{
    float3 position;    // 12 bytes + 4 bytes padding
    float4x4 matrix;    // 64 bytes (starts at offset 16)
    float value;        // 4 bytes + 12 bytes padding
};
// Total: 96 bytes with 16 bytes wasted

// GOOD: Optimal packing
cbuffer GoodLayout : register(b0)
{
    float4x4 matrix;    // 64 bytes (offset 0)
    float3 position;    // 12 bytes (offset 64)
    float value;        // 4 bytes (offset 76) - fits in same 16-byte slot
};
// Total: 80 bytes, no waste
```

### Update Frequency Separation

Separate buffers by update frequency for optimal performance:

```hlsl
// Per-frame data (updated once per frame)
cbuffer FrameConstants : register(b0)
{
    float4x4 view;
    float4x4 projection;
    float4x4 viewProjection;
    float3 viewPos;
    float time;
    float3 lightPos;
    float padding1;
    float3 lightColor;
    float padding2;
};

// Per-object data (updated per draw call) - use instancing when possible
cbuffer ObjectConstants : register(b1)
{
    float4x4 model;
    float3 objectColor;
    int objectID;
};
```

### Alignment Documentation

Always document offsets in cbuffers that must match C++ structs:

```hlsl
cbuffer Constants : register(b0)
{
    float4x4 view;             // Offset 0, 64 bytes
    float4x4 projection;       // Offset 64, 64 bytes
    float4x4 viewProjection;   // Offset 128, 64 bytes
    float3 viewPos;            // Offset 192, 12 bytes
    float time;                // Offset 204, 4 bytes
    float3 lightPos;           // Offset 208, 12 bytes
    float padding1;            // Offset 220, 4 bytes (explicit padding)
};
```

### Array Considerations

Arrays are NOT packed by default - each element starts on a 16-byte boundary:

```hlsl
// Each light takes 16 bytes, even though only 12 are used
float3 lights[4];  // 64 bytes total, not 48!

// For tightly packed arrays, use float4 or explicit packing
float4 lightsPacked[3];  // Store 4 lights in 3 float4s with manual unpacking
```

---

## Data Types and Precision

### Type Selection

| Use Case | Type | Notes |
|----------|------|-------|
| Positions, directions | `float3` | Full precision needed |
| Colors | `float3` or `float4` | Full precision for HDR |
| UV coordinates | `float2` | Sufficient precision |
| Counters, indices | `uint` or `int` | Avoid float for indices |
| Boolean flags | `uint` with bit ops | Pack multiple bools |

### Precision Considerations

```hlsl
// GOOD: Use saturate() for 0-1 clamping (single instruction)
float result = saturate(value);

// AVOID: Explicit clamp (may be slower)
float result = clamp(value, 0.0, 1.0);

// GOOD: Use built-in intrinsics
float len = length(vector);
float3 norm = normalize(vector);

// CONSIDER: For repeated normalization, cache the length
float len = length(vector);
float3 norm = vector / len;  // Reuse len if needed elsewhere
```

### Half Precision (SM 6.2+)

```hlsl
// Use half precision where appropriate for bandwidth savings
// Particularly useful for mobile/lower-end GPUs
half3 color = half3(1.0h, 0.5h, 0.25h);
min16float factor = min16float(0.5);
```

---

## Control Flow Best Practices

### Avoid Divergent Branching

```hlsl
// BAD: Divergent branch in pixel shader
if (some_per_pixel_condition)
{
    // Heavy computation A
}
else
{
    // Heavy computation B
}
// Both branches may execute on GPU!

// BETTER: Use lerp/step for simple cases
float3 result = lerp(colorA, colorB, step(threshold, value));

// ACCEPTABLE: Early-out for entire wavefronts
if (all_pixels_skip_condition)
{
    return earlyResult;
}
```

### Loop Unrolling

```hlsl
// Fixed iteration count - use [unroll]
[unroll]
for (int i = 0; i < 5; i++)
{
    value += amplitude * noise3D(p * frequency);
    frequency *= 2.0;
    amplitude *= 0.5;
}

// Variable iteration with fixed max - hybrid approach
[unroll]
for (int i = 0; i < 5; i++)
{
    if (i >= octaves) break;  // Early exit
    value += amplitude * noise3D(p * frequency);
    frequency *= 2.0;
    amplitude *= 0.5;
}

// Large/variable loops - use [loop]
[loop]
for (uint i = 0; i < g_creatureCount && i < MAX_NEIGHBORS * 10; i++)
{
    // Process creature
}
```

### Conditional Assignment

```hlsl
// BAD: Branch for simple assignment
if (value > 0.0)
    result = value;
else
    result = 0.0;

// GOOD: Use intrinsics
result = max(value, 0.0);

// BAD: Branch for interpolation
if (t < 0.5)
    color = colorA;
else
    color = colorB;

// GOOD: Use step and lerp
color = lerp(colorA, colorB, step(0.5, t));

// GOOD: Use smoothstep for smooth transitions
color = lerp(colorA, colorB, smoothstep(0.4, 0.6, t));
```

---

## Memory Access Patterns

### Texture Sampling

```hlsl
// GOOD: Sample with calculated LOD when needed
float4 color = texture.SampleLevel(sampler, uv, mipLevel);

// GOOD: Use SampleGrad for anisotropic filtering control
float4 color = texture.SampleGrad(sampler, uv, ddx(uv), ddy(uv));

// AVOID: Sampling in divergent branches (undefined derivatives)
if (divergent_condition)
{
    color = texture.Sample(sampler, uv);  // BAD - derivatives undefined
}
```

### Buffer Access

```hlsl
// GOOD: Coalesced access in compute shaders
uint index = dispatchThreadId.x;
CreatureInput me = g_creatureInputs[index];

// AVOID: Scattered random access
uint randomIndex = hash(index) % g_creatureCount;
CreatureInput random = g_creatureInputs[randomIndex];  // Cache thrashing

// GOOD: Access structured buffers sequentially
[loop]
for (uint i = 0; i < count; i++)
{
    data = g_buffer[i];  // Sequential access
}
```

### Shared Memory (Compute Shaders)

```hlsl
// Define shared memory
groupshared float3 sharedPositions[THREAD_GROUP_SIZE];

// GOOD: Load coalesced, compute, then share
sharedPositions[localIndex] = g_positions[globalIndex];
GroupMemoryBarrierWithGroupSync();  // Ensure all threads have loaded

// Now access shared memory without global memory latency
float3 neighborPos = sharedPositions[neighborIndex];
```

---

## Shader Model 6.x Features

### Wave Intrinsics (SM 6.0+)

```hlsl
// Sum across wave (replaces manual reduction)
float waveSum = WaveActiveSum(localValue);

// Check if any/all lanes have condition
bool anyTrue = WaveActiveAnyTrue(condition);
bool allTrue = WaveActiveAllTrue(condition);

// Broadcast from first lane
float firstLaneValue = WaveReadLaneFirst(value);

// Prefix sum (scan)
float prefixSum = WavePrefixSum(value);
```

### Quad Operations (SM 6.0+)

```hlsl
// Read from adjacent pixels in 2x2 quad
float right = QuadReadAcrossX(value);  // Read from horizontal neighbor
float down = QuadReadAcrossY(value);   // Read from vertical neighbor
float diag = QuadReadAcrossDiagonal(value);

// Check quad-wide uniformity (SM 6.7+)
bool quadUniform = QuadAll(condition);
bool quadAny = QuadAny(condition);
```

### Resource Binding (SM 6.6+)

```hlsl
// Direct heap indexing without root signature tables
Texture2D tex = ResourceDescriptorHeap[textureIndex];
SamplerState samp = SamplerDescriptorHeap[samplerIndex];
float4 color = tex.Sample(samp, uv);
```

---

## Common Pitfalls

### 1. Division by Zero

```hlsl
// BAD: Potential division by zero
float3 normalized = vector / length(vector);

// GOOD: Add epsilon
float len = length(vector);
float3 normalized = vector / max(len, 0.0001);

// OR use normalize() which handles this
float3 normalized = normalize(vector);

// For BRDF denominators
float denominator = 4.0 * NdotV * NdotL + 0.0001;  // Add small epsilon
```

### 2. NaN Propagation

```hlsl
// BAD: sqrt of negative number
float result = sqrt(value);  // NaN if value < 0

// GOOD: Ensure non-negative
float result = sqrt(max(value, 0.0));

// BAD: pow with negative base
float result = pow(value, exponent);  // NaN if value < 0

// GOOD: Use abs or saturate
float result = pow(abs(value), exponent);
float result = pow(saturate(value), exponent);
```

### 3. Matrix Multiplication Order

```hlsl
// DirectX uses row-major by default, HLSL column-major
// Always document your convention!

// Column-major (HLSL default): matrix-on-left
float4 worldPos = mul(model, float4(position, 1.0));
float4 clipPos = mul(viewProjection, worldPos);

// Row-major: matrix-on-right
float4 worldPos = mul(float4(position, 1.0), model);
```

### 4. Integer Division Truncation

```hlsl
// BAD: Integer division loses precision
int halfValue = value / 2;  // Truncates toward zero

// GOOD: Use appropriate type or rounding
float halfValue = value / 2.0;
int halfValueRounded = (value + 1) / 2;  // Round up
```

### 5. Texture Coordinate Wrapping

```hlsl
// Be aware of UV wrapping at boundaries
// frac() can cause seams at 0/1 boundaries

// GOOD: Use proper wrapping
float2 wrappedUV = frac(uv + 1000.0);  // Avoid negative values
```

### 6. Forgetting Padding in Structs

```hlsl
// BAD: C++ struct won't match
struct CreatureInput
{
    float3 position;  // 12 bytes
    float energy;     // 4 bytes - OK, fills the 16-byte slot
    float3 velocity;  // 12 bytes
    float fear;       // 4 bytes - OK
    uint type;        // 4 bytes
    uint isAlive;     // 4 bytes
    float waterLevel; // 4 bytes
    // Missing padding! Next struct misaligned
};

// GOOD: Explicit padding
struct CreatureInput
{
    float3 position;
    float energy;
    float3 velocity;
    float fear;
    uint type;
    uint isAlive;
    float waterLevel;
    float padding;    // Explicit padding to 16-byte boundary
};
```

---

## Documentation Standards

### File Header

```hlsl
// =============================================================================
// Shader Name: Creature.hlsl
// Purpose: Instanced rendering with procedural skin textures and animation
// Author: [Your Name]
// Date: [Creation Date]
//
// INSTANCED RENDERING:
// - Per-vertex data (slot 0): position, normal, texCoord
// - Per-instance data (slot 1): model matrix (4 rows), color+type
// - Uses SV_InstanceID for per-instance animation offset
// =============================================================================
```

### Function Documentation

```hlsl
/// Calculates Cook-Torrance PBR lighting
/// @param N      Surface normal (normalized)
/// @param V      View direction (normalized)
/// @param L      Light direction (normalized)
/// @param albedo Surface base color
/// @param metallic Metallic factor [0,1]
/// @param roughness Roughness factor [0,1]
/// @param lightCol Light color and intensity
/// @return Final lit color contribution
float3 calculatePBR(float3 N, float3 V, float3 L,
                    float3 albedo, float metallic, float roughness,
                    float3 lightCol)
```

### Inline Comments

```hlsl
// Calculate Fresnel at normal incidence
// Dielectrics use 0.04 (4% reflectance), metals use albedo
float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

// Cook-Torrance BRDF: f = DFG / (4 * NdotV * NdotL)
// Add small epsilon to prevent division by zero
float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
```

---

## References

### Official Documentation

- [HLSL Shader Model 6.0](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/hlsl-shader-model-6-0-features-for-direct3d-12) - Microsoft Learn
- [HLSL Shader Model 6.5](https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_5.html) - DirectX Specs
- [HLSL Shader Model 6.6](https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_6.html) - DirectX Specs
- [HLSL Shader Model 6.7](https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_7.html) - DirectX Specs
- [HLSL Shader Model 6.8](https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_8.html) - DirectX Specs
- [Packing Rules for Constant Variables](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules) - Microsoft Learn

### Optimization Guides

- [Intel Shader Optimization Guide](https://www.intel.com/content/www/us/en/docs/gpa/user-guide/2024-1/optimize-shaders.html)
- [HLSL Constant Buffer Packing Visualizer](https://maraneshi.github.io/HLSL-ConstantBufferLayoutVisualizer/)
- [DirectX Shader Compiler Wiki](https://github.com/microsoft/DirectXShaderCompiler/wiki/Shader-Model)

### PBR Resources

- [LearnOpenGL - PBR Theory](https://learnopengl.com/PBR/Theory)
- [Coding Labs - Cook-Torrance](https://www.codinglabs.net/article_physically_based_rendering_cook_torrance.aspx)
- [Crash Course in BRDF Implementation](https://boksajak.github.io/files/CrashCourseBRDF.pdf)

### Instancing

- [DirectXTK - Multistream Rendering and Instancing](https://github.com/microsoft/DirectXTK/wiki/Multistream-rendering-and-instancing)
- [LearnOpenGL - Instancing](https://learnopengl.com/Advanced-OpenGL/Instancing)
- [Braynzar Soft - Instancing Tutorial](https://www.braynzarsoft.net/viewtutorial/q16390-33-instancing-with-indexed-primitives)

### Procedural Generation

- [The Book of Shaders - Noise](https://thebookofshaders.com/12/)
- [Procedural Tileable Shaders](https://github.com/tuxalin/procedural-tileable-shaders)
- [GPU Voronoi Optimization](https://nickmcd.me/2020/08/01/gpu-accelerated-voronoi/)

---

*Document Version: 1.0*
*Last Updated: January 2026*
*Applies to: ModularGameEngine/Forge Engine HLSL Shaders*
