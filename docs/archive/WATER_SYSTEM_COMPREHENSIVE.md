# Water System Comprehensive Overhaul

## Overview

This document describes the comprehensive improvements made to the water rendering system in the OrganismEvolution DirectX 12 simulation. The overhaul addresses water coverage, wave animation, realistic rendering, and shoreline transitions.

## File Modified

- `Runtime/Shaders/Terrain.hlsl` - Main terrain shader containing all water rendering code

## Changes Summary

### 1. Water Threshold Tuning

**Problem**: Water covered too much terrain, making the landscape look flooded.

**Before:**
```hlsl
static const float WATER_THRESH = 0.02;    // ~0.6 world units
static const float BEACH_THRESH = 0.06;    // ~1.8 world units
```

**After:**
```hlsl
static const float WATER_THRESH = 0.012;   // ~0.36 world units - minimal water coverage
static const float WET_SAND_THRESH = 0.025; // ~0.75 world units - NEW wet sand zone
static const float BEACH_THRESH = 0.05;    // ~1.5 world units - dry beach
```

**New Biome Threshold Hierarchy:**
| Biome | Normalized Threshold | World Height (units) |
|-------|---------------------|---------------------|
| Water | < 0.012 | < 0.36 |
| Wet Sand | 0.012 - 0.025 | 0.36 - 0.75 |
| Dry Beach | 0.025 - 0.05 | 0.75 - 1.5 |
| Grass | 0.05 - 0.35 | 1.5 - 10.5 |
| Forest | 0.35 - 0.67 | 10.5 - 20 |
| Mountain | 0.67 - 0.85 | 20 - 25.5 |
| Snow | > 0.85 | > 25.5 |

---

### 2. Multi-Frequency Wave Animation System

**Problem**: Wave animation was too aggressive and unrealistic (single frequency, axis-aligned).

**Before:**
```hlsl
float waveSpeed = 0.3;
float waveHeight = 0.008;
float waveFreq = 2.0;
// Simple sin/cos on X and Z axes
```

**After - Four Wave Layers:**

#### Layer 1: Large Slow Swells (Ocean Swell)
- **Frequency**: 0.5
- **Amplitude**: 0.003 (vertex), 0.12 (normal)
- **Speed**: 0.08
- **Purpose**: Simulates distant storm-generated ocean swells
- **Direction**: Wind-influenced with variation

#### Layer 2: Medium Wind Waves
- **Frequency**: 2.0
- **Amplitude**: 0.0015 (vertex), 0.06 (normal)
- **Speed**: 0.2
- **Purpose**: Local wind-driven wave motion
- **Direction**: Primarily follows wind direction

#### Layer 3: Small Ripples (Capillary Waves)
- **Frequency**: 8.0-12.0
- **Amplitude**: 0.0005 (vertex), subtle normal contribution
- **Speed**: 0.3-0.5
- **Purpose**: Surface detail and realistic texture

#### Layer 4: Micro-Ripples (Sparkle Detail)
- **Frequency**: 25.0
- **Amplitude**: Very small
- **Purpose**: Creates light sparkle effects on water surface

**Wind Direction System:**
```hlsl
float windAngle = 0.7;  // ~40 degrees
float2 windDir = float2(cos(windAngle), sin(windAngle));
```

Wave directions are derived from wind direction with variation to create natural-looking wave patterns that don't align to axes.

---

### 3. Realistic Water Rendering

#### 3.1 Improved Fresnel Effect

**Before:**
```hlsl
float fresnel = calculateFresnel(viewDir, waterNormal, 3.0);
```

**After:**
```hlsl
// Physical Fresnel with Schlick's approximation
float baseFresnel = calculateFresnel(viewDir, waterNormal, 4.0);
float F0 = 0.02;  // Water's reflectance at normal incidence (IOR ~1.33)
float schlickFresnel = F0 + (1.0 - F0) * pow(1.0 - saturate(dot(viewDir, waterNormal)), 5.0);
fresnel = lerp(fresnel, schlickFresnel, 0.5);
```

This creates more physically accurate reflections:
- **Looking straight down**: See through water (refraction dominates)
- **Grazing angles**: Strong reflections (reflection dominates)

#### 3.2 Three-Tier Depth Coloring

**Before:**
```hlsl
float3 refractionColor = lerp(SHALLOW_WATER_COLOR, DEEP_WATER_COLOR, depth);
```

**After:**
```hlsl
static const float3 SHALLOW_WATER_COLOR = float3(0.08, 0.35, 0.45);  // Turquoise
static const float3 MID_WATER_COLOR = float3(0.04, 0.15, 0.25);      // Mid-depth
static const float3 DEEP_WATER_COLOR = float3(0.015, 0.06, 0.12);    // Dark blue-green

// Three-color gradient with exponential depth falloff
if (depth < 0.3) {
    refractionColor = lerp(SHALLOW_WATER_COLOR, MID_WATER_COLOR, depth / 0.3);
} else {
    refractionColor = lerp(MID_WATER_COLOR, DEEP_WATER_COLOR, (depth - 0.3) / 0.7);
}
```

Depth calculation uses exponential falloff for realistic light absorption:
```hlsl
float linearDepth = saturate(depthBelow / maxDepth);
return 1.0 - exp(-linearDepth * 3.0);
```

#### 3.3 Enhanced Caustics

Triple-layer caustic pattern for more realistic underwater light effects:
```hlsl
float caustic1 = voronoi(float3(causticUV + time * 0.04, 0.0));
float caustic2 = voronoi(float3(causticUV * 1.7 - time * 0.055, 1.0));
float caustic3 = voronoi(float3(causticUV * 0.8 + float2(time * 0.03, -time * 0.02), 2.0));
```

#### 3.4 Dynamic Sky Reflection

Reflections now use the dynamic sky colors from the day/night cycle:
```hlsl
float3 horizonReflect = lerp(float3(0.55, 0.65, 0.75), skyHorizonColor, 0.7);
float3 zenithReflect = lerp(float3(0.4, 0.55, 0.75), skyTopColor, 0.7);
```

Includes subtle procedural cloud reflections:
```hlsl
float cloudNoise = fbm(float3(reflectedView.xz * 2.0 + time * 0.01, 0.0), 3);
float clouds = smoothstep(0.4, 0.7, cloudNoise) * 0.15;
```

#### 3.5 Subsurface Scattering

Simulates light passing through shallow water:
```hlsl
float wrapLighting = saturate((NdotL + 0.3) / 1.3);
float3 subsurface = subsurfaceColor * wrapLighting * (1.0 - depth) * 0.25;
```

#### 3.6 Depth Absorption

Deep water absorbs more light:
```hlsl
float absorption = exp(-depth * 2.5);
waterColor *= lerp(0.4, 1.0, absorption);
```

#### 3.7 Atmospheric Perspective

Distant water appears hazier:
```hlsl
float viewDist = length(worldPos - viewPos);
float haze = 1.0 - exp(-viewDist * 0.002);
waterColor = lerp(waterColor, reflectionColor * 0.8, haze * 0.3);
```

---

### 4. Improved Foam System

**Before:**
- Simple shoreline foam
- Basic wave crest foam

**After:**

#### Multiple Foam Sources:
1. **Shoreline Foam**: Concentrated at water's edge
2. **Wave Crest Foam**: White caps on wave peaks
3. **Breaking Wave Foam**: Periodic wash onto shore
4. **Receding Foam Trails**: Foam patterns as waves retreat

```hlsl
// Breaking wave foam - periodic wash
float wavePhase = sin(worldPos.x * 0.1 + time * 0.3) * 0.5 + 0.5;
float breakingFoam = smoothstep(0.6, 1.0, shorelineFoam) * wavePhase;

// Receding foam trails
float recedingWave = sin(worldPos.z * 0.2 - time * 0.4 + worldPos.x * 0.1);
float recedingFoam = smoothstep(0.3, 0.8, shorelineFoam) * smoothstep(-0.2, 0.5, recedingWave) * 0.4;
```

---

### 5. Water-Beach Transition System

#### 5.1 New Wet Sand Zone

A dedicated zone between water and dry beach:

```hlsl
else if (heightNormalized < WET_SAND_THRESH)
{
    // Darker wet sand with water sheen
    float wetness = 1.0 - smoothstep(WATER_THRESH, WET_SAND_THRESH, heightNormalized);

    // Animated wave wash effect
    float waveWash = sin(worldPos.x * 0.3 + time * 0.5) * sin(worldPos.z * 0.2 - time * 0.4);
    wetness = saturate(wetness + waveWash * 0.2);

    // Specular reflection on wet sand
    float wetSpec = pow(saturate(dot(reflect(-lightDir, wetNormal), viewDir)), 32.0) * wetness * 0.3;
}
```

#### 5.2 Improved Biome Blending

Smoother transitions with edge foam:

```hlsl
// Water to wet sand - with edge foam
float edgeFoam = (1.0 - abs(t - 0.5) * 2.0);
float foamNoise = noise3D(float3(worldPos.xz * 2.0 + time * 0.3, time * 0.2));
terrainColor = lerp(terrainColor, FOAM_COLOR, edgeFoam * foamNoise * 0.4);
```

#### 5.3 Sand Ripple Normals

Wet sand area has subtle ripple patterns in the normal calculation:
```hlsl
float rippleNx = sin(pos.x * 15.0) * cos(pos.z * 12.0) * 0.05;
float rippleNz = cos(pos.x * 12.0) * sin(pos.z * 15.0) * 0.05;
```

---

## Complete Value Changes Reference

### Water Configuration Constants

| Constant | Before | After | Description |
|----------|--------|-------|-------------|
| WATER_LEVEL | 0.02 | 0.012 | Water surface threshold |
| DEEP_WATER_COLOR | (0.02, 0.08, 0.15) | (0.015, 0.06, 0.12) | Darker deep water |
| SHALLOW_WATER_COLOR | (0.1, 0.4, 0.5) | (0.08, 0.35, 0.45) | More turquoise |
| MID_WATER_COLOR | N/A | (0.04, 0.15, 0.25) | NEW: Mid-depth color |
| FOAM_COLOR | (0.9, 0.95, 1.0) | (0.92, 0.96, 1.0) | Slightly brighter |
| WET_SAND_COLOR | N/A | (0.55, 0.48, 0.35) | NEW: Wet sand color |

### Wave Animation Parameters

| Parameter | Before | After | Description |
|-----------|--------|-------|-------------|
| waveSpeed | 0.3 | Multi-freq: 0.08-0.8 | Multiple speeds per layer |
| waveHeight | 0.008 | Multi-freq: 0.0005-0.003 | Smaller, layered heights |
| waveFreq | 2.0 | Multi-freq: 0.5-25.0 | Four frequency layers |
| Wind direction | N/A | ~40 degrees | NEW: Wind-influenced direction |

### Biome Thresholds

| Threshold | Before | After |
|-----------|--------|-------|
| WATER_THRESH | 0.02 | 0.012 |
| WET_SAND_THRESH | N/A | 0.025 (NEW) |
| BEACH_THRESH | 0.06 | 0.05 |
| GRASS_THRESH | 0.35 | 0.35 (unchanged) |

### Fresnel Parameters

| Parameter | Before | After |
|-----------|--------|-------|
| Fresnel power | 3.0 | 4.0 |
| F0 (water IOR) | N/A | 0.02 (Schlick's approximation) |

---

## Technical Notes

### Shader Compatibility

- Uses HLSL for DirectX 12
- Column-major matrix multiplication: `mul(matrix, vector)`
- Height normalization: `heightNormalized = height / 30.0`
- Water vertices identified by `input.position.y < 0.5`

### Performance Considerations

- Multi-frequency waves add computational cost
- Caustic triple-layer increases fragment shader work
- Consider reducing wave layers for lower-end hardware

### Constant Buffer Alignment

The shader constant buffer must match the C++ `Constants` struct exactly:
- `float4x4` matrices must be 16-byte aligned
- Day/night cycle data follows main transform matrices
- Total buffer size: 400 bytes

---

## Visual Comparison

### Before
- Flat water color with basic ripples
- Harsh water-to-beach transition
- Single-frequency wave animation
- Too much water coverage

### After
- Dynamic depth-based coloring with three tiers
- Smooth wet sand transition zone
- Multi-frequency realistic wave motion
- Minimal water coverage exposing more terrain
- Physically-based Fresnel reflections
- Enhanced foam with wave wash effects
- Caustic light patterns in shallow water
- Atmospheric perspective for distant water

---

## Future Improvements

Potential enhancements for future updates:

1. **Screen-space reflections** for dynamic environment reflection
2. **Underwater rendering** pass for submerged camera
3. **Foam particle system** for breaking waves
4. **Tide system** with time-based water level variation
5. **Rain ripple effects** integrated with weather system
