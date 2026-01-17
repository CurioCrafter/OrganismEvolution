# HLSL Shader Reference Documentation

> Complete API reference for all shaders in the ModularGameEngine/Forge Engine Runtime/Shaders directory.

## Table of Contents

1. [Shader Overview](#shader-overview)
2. [Creature.hlsl](#creaturehlsl)
3. [Terrain.hlsl](#terrainhlsl)
4. [Nametag.hlsl](#nametaghlsl)
5. [Vegetation.hlsl](#vegetationhlsl)
6. [SteeringCompute.hlsl](#steeringcomputehlsl)
7. [BasicTriangle.hlsl](#basictrianglehlsl)
8. [Shared Constants Structure](#shared-constants-structure)
9. [Common Functions Reference](#common-functions-reference)

---

## Shader Overview

| Shader | Type | Purpose | Key Features |
|--------|------|---------|--------------|
| Creature.hlsl | VS/PS | Render animated creatures | Instancing, PBR, procedural skin |
| Terrain.hlsl | VS/PS | Render procedural terrain | Biomes, water, day/night cycle |
| Nametag.hlsl | VS/PS | Render creature ID labels | Procedural 7-segment digits |
| Vegetation.hlsl | VS/PS | Render trees and plants | Basic Blinn-Phong lighting |
| SteeringCompute.hlsl | CS | AI steering behaviors | Flocking, predator-prey |
| BasicTriangle.hlsl | VS/PS | Debug/test rendering | Passthrough shader |

---

## Creature.hlsl

### Purpose
Instanced rendering of animated creatures with procedural skin textures and PBR lighting.

### Shader Model
Shader Model 5.0+ (uses structured per-instance data)

### Input Structures

#### VSInput
```hlsl
struct VSInput
{
    // Per-vertex data (slot 0, rate: per-vertex)
    float3 position : POSITION;        // Local vertex position
    float3 normal : NORMAL;            // Local vertex normal
    float2 texCoord : TEXCOORD0;       // UV coordinates

    // Per-instance data (slot 1, rate: per-instance)
    float4 instModelRow0 : INST_MODEL0;  // Model matrix row 0
    float4 instModelRow1 : INST_MODEL1;  // Model matrix row 1
    float4 instModelRow2 : INST_MODEL2;  // Model matrix row 2
    float4 instModelRow3 : INST_MODEL3;  // Model matrix row 3
    float4 instColorType : INST_COLOR;   // RGB color (xyz) + creature type (w)

    uint instanceID : SV_InstanceID;     // System-provided instance index
};
```

#### PSInput
```hlsl
struct PSInput
{
    float4 position : SV_POSITION;   // Clip-space position
    float3 fragPos : TEXCOORD0;      // World-space position
    float3 normal : TEXCOORD1;       // World-space normal
    float3 color : TEXCOORD2;        // Instance color
    float3 triplanarUV : TEXCOORD3;  // World position for procedural textures
};
```

### Constant Buffer Layout
Uses `FrameConstants` at register b0. See [Shared Constants Structure](#shared-constants-structure).

### Key Functions

#### `float3 generateSkinPattern(float3 pos, float3 normal, float3 baseColor)`
Generates procedural scale-like skin texture using triplanar projection.

| Parameter | Type | Description |
|-----------|------|-------------|
| pos | float3 | World-space position |
| normal | float3 | Surface normal for triplanar blending |
| baseColor | float3 | Base creature color |
| **Returns** | float3 | Final skin color with pattern |

#### `float3 calculatePBR(float3 N, float3 V, float3 L, float3 albedo, float metallic, float roughness, float3 lightCol)`
Full Cook-Torrance BRDF lighting calculation.

| Parameter | Type | Description |
|-----------|------|-------------|
| N | float3 | Surface normal (normalized) |
| V | float3 | View direction (normalized) |
| L | float3 | Light direction (normalized) |
| albedo | float3 | Surface base color |
| metallic | float | Metallic factor [0,1] |
| roughness | float | Roughness factor [0,1] |
| lightCol | float3 | Light color and intensity |
| **Returns** | float3 | PBR lighting contribution |

#### `float3 calculateSubsurfaceScattering(float3 N, float3 V, float3 L, float3 albedo, float3 lightCol)`
Approximates subsurface scattering for organic materials.

### Vertex Shader Features
- Reconstructs per-instance model matrix from vertex input
- Applies per-instance animation offset using `SV_InstanceID`
- Animations: vertical bob, side sway, breathing

### Pixel Shader Features
- Procedural skin with voronoi-based scales
- Full PBR lighting with Cook-Torrance BRDF
- Subsurface scattering approximation
- Rim lighting
- Distance fog

---

## Terrain.hlsl

### Purpose
Procedural terrain rendering with multiple biomes, realistic water, and day/night cycle support.

### Shader Model
Shader Model 5.0+

### Input Structures

#### VSInput
```hlsl
struct VSInput
{
    float3 position : POSITION;  // Local vertex position
    float3 normal : NORMAL;      // Local vertex normal
    float3 color : COLOR;        // Vertex color (from terrain mesh)
};
```

#### PSInput
```hlsl
struct PSInput
{
    float4 position : SV_POSITION;  // Clip-space position
    float3 fragPos : TEXCOORD0;     // World-space position
    float3 normal : TEXCOORD1;      // World-space normal
    float3 color : TEXCOORD2;       // Vertex color
    float3 worldPos : TEXCOORD3;    // World position for procedural effects
    float height : TEXCOORD4;       // Original Y height for biome selection
};
```

### Constants

```hlsl
static const float WATER_LEVEL = 0.35;           // Normalized water threshold
static const float3 DEEP_WATER_COLOR = float3(0.02, 0.08, 0.15);
static const float3 SHALLOW_WATER_COLOR = float3(0.1, 0.4, 0.5);
static const float3 SKY_COLOR = float3(0.5, 0.7, 0.9);
static const float3 SUN_COLOR = float3(1.0, 0.95, 0.8);
static const float3 FOAM_COLOR = float3(0.9, 0.95, 1.0);
```

### Biome Height Thresholds

| Biome | Height Range (Normalized) | Roughness | Metallic |
|-------|---------------------------|-----------|----------|
| Water | 0.0 - 0.35 | 0.1 | 0.0 |
| Sand | 0.35 - 0.42 | 0.9 | 0.0 |
| Grass | 0.42 - 0.65 | 0.7 | 0.0 |
| Rock | 0.65 - 0.85 | 0.85 | 0.1 |
| Snow | 0.85 - 1.0 | 0.3 | 0.0 |

### Key Functions

#### Water System

##### `float3 calculateWaterNormal(float3 worldPos)`
Calculates animated water surface normal from multiple wave layers.

##### `float calculateFresnel(float3 viewDir, float3 normal, float power)`
Calculates Fresnel effect for water reflectivity.

##### `float calculateWaterDepth(float heightNormalized)`
Maps height to water depth for color interpolation.

##### `float calculateFoam(float3 worldPos, float heightNormalized, float depth)`
Generates shoreline and wave crest foam patterns.

##### `float3 calculateWaterSpecular(float3 viewDir, float3 lightDir, float3 waterNormal, float3 sunColor)`
Calculates sun reflection highlights on water surface.

##### `float3 generateRealisticWaterColor(float3 worldPos, float3 baseNormal, float3 viewDir, float3 lightDir, float heightNormalized)`
Main water rendering function combining all water effects.

#### Terrain Materials

##### `float3 generateSandColor(float3 pos)`
Procedural sand with noise variation.

##### `float3 generateGrassColor(float3 pos, float3 normal)`
Procedural grass with detail noise.

##### `float3 generateRockColor(float3 pos, float3 normal)`
Procedural rock with voronoi pattern.

##### `float3 generateSnowColor(float3 pos)`
Procedural snow with subtle noise.

#### Atmospheric

##### `float3 AddStars(float3 viewDir, float visibility)`
Generates procedural star field for night sky.

### Vertex Shader Features
- Water vertex animation (sine waves)
- Standard world-space transformation

### Pixel Shader Features
- Height-based biome selection with smooth blending
- Realistic water with:
  - Multi-layer wave animation
  - Fresnel-based reflection/refraction
  - Caustics pattern
  - Shoreline foam
  - Sun specular highlights
- Full PBR lighting per biome
- Dynamic day/night cycle integration
- Distance fog with sky color

---

## Nametag.hlsl

### Purpose
Renders creature ID labels using procedural seven-segment digit display.

### Shader Model
Shader Model 5.0+

### Input Structures

#### VSInput
```hlsl
struct VSInput
{
    float3 position : POSITION;   // Quad vertex position
    float2 texCoord : TEXCOORD0;  // UV coordinates [0,1]
};
```

#### PSInput
```hlsl
struct PSInput
{
    float4 position : SV_POSITION;  // Clip-space position
    float2 texCoord : TEXCOORD0;    // UV for digit rendering
};
```

### Seven-Segment Digit Patterns

```
Segment Layout:
   AAA
  F   B
   GGG
  E   C
   DDD

Digit patterns stored as bit flags:
  Bit 0 = A (top horizontal)
  Bit 1 = B (top-right vertical)
  Bit 2 = C (bottom-right vertical)
  Bit 3 = D (bottom horizontal)
  Bit 4 = E (bottom-left vertical)
  Bit 5 = F (top-left vertical)
  Bit 6 = G (middle horizontal)
```

| Digit | Hex Pattern | Active Segments |
|-------|-------------|-----------------|
| 0 | 0x3F | A B C D E F |
| 1 | 0x06 | B C |
| 2 | 0x5B | A B D E G |
| 3 | 0x4F | A B C D G |
| 4 | 0x66 | B C F G |
| 5 | 0x6D | A C D F G |
| 6 | 0x7D | A C D E F G |
| 7 | 0x07 | A B C |
| 8 | 0x7F | A B C D E F G |
| 9 | 0x6F | A B C D F G |

### Key Functions

#### `bool drawSegment(float2 uv, int segment, float digitX, float digitWidth)`
Tests if UV coordinate is within a specific segment of a digit.

#### `bool drawDigit(float2 uv, int digit, float digitX, float digitWidth)`
Renders a single digit at the specified position.

### Uses from Constant Buffer
- `creatureID` - The ID number to display
- `creatureType` - 0 for Herbivore (H), 1 for Carnivore (C)
- `textColor` - Color for rendered text (maps from objectColor)

### Output
- Text pixels: `float4(textColor, 1.0)`
- Background pixels: `float4(0.1, 0.1, 0.1, 0.6)` (semi-transparent dark)

---

## Vegetation.hlsl

### Purpose
Simple shader for rendering trees, bushes, and grass with basic lighting.

### Shader Model
Shader Model 5.0+

### Input Structures

#### VSInput
```hlsl
struct VSInput
{
    float3 position : POSITION;   // Local vertex position
    float3 normal : NORMAL;       // Local vertex normal
    float2 texCoord : TEXCOORD0;  // Encodes color (RG channels)
};
```

#### PSInput
```hlsl
struct PSInput
{
    float4 position : SV_POSITION;  // Clip-space position
    float3 fragPos : TEXCOORD0;     // World-space position
    float3 normal : TEXCOORD1;      // World-space normal
    float3 color : TEXCOORD2;       // Extracted vegetation color
};
```

### Color Encoding
Tree generator stores color in texture coordinates:
```hlsl
output.color = float3(input.texCoord.r, input.texCoord.g, 0.2);
```

### Lighting Model
Uses simplified Blinn-Phong:
- Ambient strength: 0.4
- Specular strength: 0.1 (minimal for natural materials)
- Specular power: 8.0

---

## SteeringCompute.hlsl

### Purpose
GPU-accelerated steering behaviors for massive creature simulation (up to 65,536 creatures).

### Shader Model
Shader Model 5.0+ (Compute Shader)

### Thread Configuration
```hlsl
#define THREAD_GROUP_SIZE 64
#define MAX_NEIGHBORS 32
```

### Data Structures

#### CreatureInput (Read-only)
```hlsl
struct CreatureInput
{
    float3 position;    // World position
    float energy;       // Current energy level

    float3 velocity;    // Current velocity
    float fear;         // Fear level

    uint type;          // CreatureType enum
    uint isAlive;       // Boolean as uint
    float waterLevel;   // Local water level
    float padding;      // Alignment padding
};
```

#### SteeringOutput (Read-write)
```hlsl
struct SteeringOutput
{
    float3 steeringForce;   // Computed steering force
    float priority;         // Behavior priority

    float3 targetPosition;  // Predicted target position
    uint behaviorFlags;     // Active behavior flags
};
```

#### FoodPosition (Read-only)
```hlsl
struct FoodPosition
{
    float3 position;  // Food location
    float amount;     // Food amount available
};
```

### Creature Types

| Type | Value | Can Walk | Can Swim | Can Fly | Is Predator |
|------|-------|----------|----------|---------|-------------|
| TYPE_HERBIVORE | 0 | Yes | No | No | No |
| TYPE_CARNIVORE | 1 | Yes | No | No | Yes |
| TYPE_AQUATIC | 2 | No | Yes | No | No |
| TYPE_AQUATIC_PREDATOR | 3 | No | Yes | No | Yes |
| TYPE_AMPHIBIAN | 4 | Yes | Yes | No | No |
| TYPE_AMPHIBIAN_PREDATOR | 5 | Yes | Yes | No | Yes |
| TYPE_FLYING | 6 | No | No | Yes | No |
| TYPE_AERIAL_PREDATOR | 7 | No | No | Yes | Yes |

### Behavior Flags

| Flag | Bit | Description |
|------|-----|-------------|
| FLAG_SEEK | 0 | Moving toward target |
| FLAG_FLEE | 1 | Moving away from threat |
| FLAG_ARRIVE | 2 | Slowing to reach target |
| FLAG_WANDER | 3 | Random exploration |
| FLAG_PURSUIT | 4 | Chasing predicted position |
| FLAG_EVASION | 5 | Evading predicted pursuer |
| FLAG_SEPARATE | 6 | Maintaining distance from flock |
| FLAG_ALIGN | 7 | Matching flock velocity |
| FLAG_COHESION | 8 | Moving toward flock center |
| FLAG_AVOID_WATER | 9 | Land creature avoiding water |
| FLAG_AVOID_LAND | 10 | Aquatic creature avoiding land |
| FLAG_ALTITUDE | 11 | Flying creature maintaining altitude |
| FLAG_HUNTING | 12 | Predator hunting prey |
| FLAG_FLEEING | 13 | Prey fleeing predator |

### Steering Behavior Functions

#### Basic Steering

| Function | Description |
|----------|-------------|
| `Seek(pos, vel, target)` | Move toward target at max speed |
| `Flee(pos, vel, threat)` | Move away from threat at max speed |
| `Arrive(pos, vel, target)` | Seek with deceleration near target |
| `Wander(pos, vel, seed)` | Random wandering with jitter |
| `Pursuit(pos, vel, targetPos, targetVel)` | Intercept moving target |
| `Evasion(pos, vel, pursuerPos, pursuerVel)` | Evade predicted pursuer |

#### Flocking Behaviors

| Function | Description |
|----------|-------------|
| `FindFlockmates(...)` | Identify nearby same-type creatures |
| `Flock(myIndex, me)` | Combined separation/alignment/cohesion |

#### Predator-Prey

| Function | Description |
|----------|-------------|
| `SeekFood(pos, vel, out foundFood)` | Find and move toward nearest food |
| `FleeFromPredators(index, me, out fleeing)` | Flee from nearby predators |
| `HuntPrey(index, me, out hunting)` | Hunt nearest valid prey |

#### Environment

| Function | Description |
|----------|-------------|
| `AvoidWater(pos, vel, waterLevel)` | Push land creatures away from water |
| `AvoidLand(pos, vel, waterLevel)` | Push aquatic creatures into water |
| `MaintainAltitude(pos, targetAlt)` | Keep flying creatures at altitude |

### Resource Bindings

| Register | Type | Name | Description |
|----------|------|------|-------------|
| b0 | cbuffer | SteeringConstants | Simulation parameters |
| t0 | StructuredBuffer | g_creatureInputs | Input creature data |
| t1 | StructuredBuffer | g_foodPositions | Food locations |
| u0 | RWStructuredBuffer | g_steeringOutputs | Output steering forces |

### Dispatch

```cpp
// C++ dispatch example
uint numGroups = (creatureCount + 63) / 64;
context->Dispatch(numGroups, 1, 1);
```

---

## BasicTriangle.hlsl

### Purpose
Minimal passthrough shader for testing and debugging.

### Shader Model
Shader Model 5.0+

### Input Structures

```hlsl
struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};
```

### Behavior
- Vertex shader: Passes position directly (no transformation)
- Pixel shader: Returns vertex color unchanged

---

## Shared Constants Structure

All rendering shaders share a common constant buffer layout that must match the C++ `Constants` struct:

```hlsl
cbuffer Constants : register(b0)
{
    // Transform matrices (per-frame)
    float4x4 view;             // Offset 0, 64 bytes
    float4x4 projection;       // Offset 64, 64 bytes
    float4x4 viewProjection;   // Offset 128, 64 bytes

    // Camera/Time (per-frame)
    float3 viewPos;            // Offset 192, 12 bytes
    float time;                // Offset 204, 4 bytes

    // Lighting (per-frame)
    float3 lightPos;           // Offset 208, 12 bytes
    float padding1;            // Offset 220, 4 bytes
    float3 lightColor;         // Offset 224, 12 bytes
    float padding2;            // Offset 236, 4 bytes

    // Day/Night cycle (per-frame)
    float dayTime;             // Offset 240, 4 bytes - Time of day [0,1]
    float3 skyTopColor;        // Offset 244, 12 bytes - Sky zenith color
    float3 skyHorizonColor;    // Offset 256, 12 bytes - Sky horizon color
    float sunIntensity;        // Offset 268, 4 bytes - Light multiplier
    float3 ambientColor;       // Offset 272, 12 bytes - Ambient light
    float starVisibility;      // Offset 284, 4 bytes - Star visibility [0,1]
    float2 dayNightPadding;    // Offset 288, 8 bytes - Alignment padding

    // Per-object (updated per draw call)
    float4x4 model;            // Offset 296, 64 bytes
    float3 objectColor;        // Offset 360, 12 bytes
    int creatureID;            // Offset 372, 4 bytes
    int creatureType;          // Offset 376, 4 bytes
    float3 objectPadding;      // Offset 380, 12 bytes

    // Total: 392 bytes
};
```

---

## Common Functions Reference

### Noise Functions (Duplicated in Creature.hlsl, Terrain.hlsl)

| Function | Parameters | Returns | Description |
|----------|------------|---------|-------------|
| `hash3` | `float3 p` | `float3` | 3D hash function for randomness |
| `noise3D` | `float3 p` | `float` | 3D value noise |
| `fbm` | `float3 p, int octaves` | `float` | Fractal Brownian motion |
| `voronoi` | `float3 p` | `float` | Cellular/Voronoi noise |

### PBR Functions (Duplicated in Creature.hlsl, Terrain.hlsl)

| Function | Description |
|----------|-------------|
| `DistributionGGX` | GGX/Trowbridge-Reitz normal distribution |
| `GeometrySchlickGGX` | Schlick-GGX geometry function |
| `GeometrySmith` | Smith geometry function (view + light) |
| `fresnelSchlick` | Fresnel-Schlick approximation |
| `fresnelSchlickRoughness` | Fresnel with roughness for IBL |
| `calculatePBR` | Full Cook-Torrance BRDF calculation |
| `calculateAmbientIBL` | Hemisphere ambient approximation |

### Helper Functions

| Function | Shader | Description |
|----------|--------|-------------|
| `LimitMagnitude` | SteeringCompute | Clamp vector length |
| `PCGHash` | SteeringCompute | PCG random number generator |
| `RandomFloat` | SteeringCompute | Float random [0,1] |
| `RandomInUnitSphere` | SteeringCompute | Random direction |

---

*Document Version: 1.0*
*Last Updated: January 2026*
*Applies to: ModularGameEngine/Forge Engine Runtime/Shaders/*
