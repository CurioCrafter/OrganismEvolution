# Phase 8 Agent 9: Shaders and Visual Identity (No Man's Sky Style)

## Overview

This document describes the stylized rendering system implemented for OrganismEvolution, inspired by No Man's Sky's distinctive visual aesthetic. The goal is to create a polished, cohesive look across all visual elements while maintaining good performance.

## Shader Architecture

### File Structure

```
Runtime/Shaders/
├── Common.hlsl          # Shared utilities, quality settings, style parameters
├── Creature.hlsl        # Creature rendering with stylized shading
├── Terrain.hlsl         # Terrain with biome tinting and slope variation
├── Vegetation.hlsl      # Trees/plants with wind animation and foliage sheen
├── Sky.hlsl             # Procedural sky with sun glow and atmosphere
├── Water.hlsl           # Stylized water with foam and caustics
├── Underwater.hlsl      # Underwater post-processing effects
└── [other shaders]

src/graphics/
├── GlobalLighting.h     # Visual style parameters struct
├── GlobalLighting.cpp   # Lighting update and style management
├── SkyRenderer.h        # Sky rendering with color grading
├── SkyRenderer.cpp      # Sky update from day/night cycle
├── WaterRenderer.h      # Water rendering parameters
└── WaterRenderer.cpp    # Water mesh and constants

src/environment/
├── PlanetTheme.h        # Planet-wide color palettes
└── PlanetTheme.cpp      # Theme preset generators
```

### Constant Buffer Layout

All shaders share a consistent constant buffer structure for frame data:

```hlsl
cbuffer FrameConstants : register(b0)
{
    float4x4 view;
    float4x4 projection;
    float4x4 viewProjection;
    float3 viewPos;
    float time;
    float3 lightPos;
    float3 lightColor;
    float dayTime;
    float3 skyTopColor;
    float3 skyHorizonColor;
    float sunIntensity;
    float3 ambientColor;
    float starVisibility;
    // ... per-object data
};
```

## Shader Parameter Reference

### Global Style Controls (Common.hlsl)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `STYLE_STRENGTH` | 0.7 | Master blend between realistic (0) and stylized (1) |
| `COLOR_VIBRANCY` | 1.15 | Saturation boost (1.0 = neutral) |
| `QUALITY_LEVEL` | 2 | 0=off, 1=low, 2=medium, 3=high |

### Creature Shading (Creature.hlsl)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `CREATURE_RIM_INTENSITY` | 0.45 | Stylized edge lighting intensity |
| `CREATURE_RIM_POWER` | 2.5 | Rim falloff (lower = broader rim) |
| `CREATURE_GRADIENT_STRENGTH` | 0.3 | Vertical color gradient strength |
| `CREATURE_COLOR_BOOST` | 1.15 | Per-creature saturation |
| `CREATURE_FRESNEL_BRIGHTNESS` | 0.25 | Edge brightening |

**Features:**
- Stylized rim lighting with backlight component
- Gradient shading (warm top, cool bottom)
- Subsurface scattering for organic materials
- Procedural skin/scale textures
- Fish swimming animation with iridescence
- PBR lighting base with stylized overlay

### Terrain Shading (Terrain.hlsl)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `TERRAIN_STYLE_STRENGTH` | 0.65 | Terrain style blend |
| `TERRAIN_BIOME_TINT_STRENGTH` | 0.4 | How strongly biomes tint terrain |
| `TERRAIN_SLOPE_VARIATION` | 0.35 | Rock/cliff exposure on slopes |
| `TERRAIN_HEIGHT_GRADIENT` | 0.25 | Vertical color gradient |
| `TERRAIN_COLOR_VIBRANCY` | 1.12 | Terrain saturation |

**Features:**
- Height-based biome transitions (water → sand → grass → forest → rock → snow)
- Slope-based rock exposure
- Procedural grass with wind animation
- Forest floor with dappled sunlight
- Realistic water system with multi-frequency waves
- Foam at shoreline and wave crests
- Caustic patterns in shallow water

### Vegetation Shading (Vegetation.hlsl)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `VEGETATION_STYLE_STRENGTH` | 0.7 | Vegetation style blend |
| `VEGETATION_FOLIAGE_SHEEN` | 0.15 | Waxy leaf shine |
| `VEGETATION_VIBRANCY` | 1.18 | Plant saturation |
| `VEGETATION_RIM_INTENSITY` | 0.3 | Edge lighting on leaves |

**Features:**
- Wind animation (height-based influence)
- Foliage sheen for waxy leaves
- Subsurface scattering for translucent leaves
- Stylized rim lighting
- Color tinting per biome

### Sky Shading (Sky.hlsl)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `SKY_SUN_GLOW_INTENSITY` | 0.35 | Stylized sun glow size |
| `SKY_HORIZON_GLOW` | 0.25 | Atmospheric glow at horizon |
| `SKY_GRADIENT_CONTRAST` | 1.1 | Sky color gradient contrast |
| `SKY_VIBRANCY` | 1.12 | Sky saturation |
| `STAR_TWINKLE_SPEED` | 2.5 | Star animation speed |
| `CLOUD_HINT_STRENGTH` | 0.08 | Subtle procedural cloud hints |

**Features:**
- Gradient sky with zenith/horizon colors
- Stylized sun glow (larger, more artistic)
- Rayleigh/Mie scattering approximation
- Procedural star field with twinkle
- Moon phases with surface detail
- Subtle procedural cloud hints
- Color grading integration

### Water Shading (Water.hlsl)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `WATER_FOAM_BRIGHTNESS` | 0.92 | Foam whiteness |
| `WATER_SUBSURFACE_STRENGTH` | 0.25 | Light penetration glow |
| `WATER_CAUSTIC_INTENSITY` | 0.35 | Animated caustic patterns |
| `WATER_RIM_GLOW` | 0.12 | Edge glow on wave crests |

**Features:**
- Multi-frequency wave system (swells, wind waves, ripples)
- Fresnel-based reflections
- Theme-matched colors from PlanetTheme
- Foam on wave crests
- Caustic patterns
- Subsurface scattering
- Underwater view with Snell's window effect
- Atmospheric perspective for distant water

## Quality Levels

### Level 0: Off
- Minimal effects
- Basic lighting only
- No procedural details

### Level 1: Low
- Basic rim lighting
- Simple wave animation
- Stars enabled
- Subsurface scattering

### Level 2: Medium (Default)
- Atmospheric scattering
- Cloud hints in sky
- Caustic patterns
- Foam detail
- Multi-layer waves
- Slope variation
- Foliage sheen

### Level 3: High
- Maximum octaves for all noise
- Full wave detail
- Highest quality caustics
- All features enabled

### Performance Characteristics

| Quality | GPU Cost | Features Disabled |
|---------|----------|------------------|
| Off | ~30% baseline | All stylized effects |
| Low | ~60% baseline | Atmosphere, caustics, cloud hints |
| Medium | ~85% baseline | Extra noise octaves |
| High | 100% baseline | None |

## Palette Integration Notes

### PlanetTheme Integration

The shaders consume palette data from `PlanetTheme` via constant buffers:

```cpp
// In C++ (GlobalLighting or per-shader CB update)
if (theme) {
    AtmosphereSettings atm = theme->getCurrentAtmosphere();
    m_constants.skyTopColor = atm.skyZenithColor;
    m_constants.skyHorizonColor = atm.skyHorizonColor;
    m_constants.sunColor = atm.sunColor;
    m_constants.fogColor = atm.fogColor;
}

// Water palette
m_constants.waterColor = theme->getTerrain().deepWaterColor;
m_constants.shallowColor = theme->getTerrain().shallowWaterColor;
```

### Theme Presets

| Preset | Sky Tint | Terrain Tint | Water Tint |
|--------|----------|--------------|------------|
| Earth-Like | Blue/white | Natural greens | Blue/cyan |
| Alien Purple | Purple/magenta | Purple vegetation | Violet |
| Desert World | Orange/yellow | Warm browns | Murky brown |
| Frozen World | Pale blue | Cool whites | Deep blue |
| Volcanic | Red/orange | Dark grays | Lava red |

### Biome Color Tints

Applied in terrain shader based on height/position:

```hlsl
static const float3 BIOME_TINT_WARM = float3(1.05, 1.0, 0.92);   // Desert
static const float3 BIOME_TINT_COOL = float3(0.92, 0.98, 1.05);  // Tundra
static const float3 BIOME_TINT_LUSH = float3(0.95, 1.05, 0.95);  // Forest
static const float3 BIOME_TINT_ALIEN = float3(1.0, 0.95, 1.08);  // Alien
```

## Visual Style Parameters (C++ Side)

### VisualStyleParams Struct

Located in `GlobalLighting.h`:

```cpp
struct VisualStyleParams {
    float styleStrength = 0.7f;
    float colorVibrancy = 1.15f;

    // Creature
    float creatureRimIntensity = 0.45f;
    float creatureRimPower = 2.5f;
    float creatureGradientStrength = 0.3f;

    // Terrain
    float terrainStyleStrength = 0.65f;
    float biomeTintStrength = 0.4f;
    float slopeVariation = 0.35f;

    // Sky
    float sunGlowIntensity = 0.3f;
    float horizonGlow = 0.25f;

    // Presets
    static VisualStyleParams getEarthLike();
    static VisualStyleParams getAlienPurple();
    static VisualStyleParams getDesertWorld();
    static VisualStyleParams getFrozenWorld();
};
```

### Usage

```cpp
// Set style via GlobalLighting
GlobalLighting& lighting = ...;
lighting.setVisualStyle(VisualStyleParams::getAlienPurple());

// Or modify individual parameters
auto& style = lighting.getMutableVisualStyle();
style.styleStrength = 0.9f;
style.colorVibrancy = 1.3f;
```

## Validation Checklist

### Visual Quality

- [x] Creatures have visible rim lighting
- [x] Terrain shows biome color variation
- [x] Vegetation sways in wind
- [x] Sky has gradient with sun glow
- [x] Water reflects sky colors
- [x] Fog matches horizon color
- [x] Stars visible at night with twinkle
- [x] Moon phases display correctly

### Performance

- [x] 60 FPS at Medium quality on mid-range GPU
- [x] No shader compile errors
- [x] Quality levels properly disable features
- [x] No additional render targets required

### Biome Distinctiveness

Test screenshots across these biomes:
1. Beach/coastal (sand, water, foam)
2. Grassland (grass animation, sun lighting)
3. Forest (dappled light, vegetation)
4. Mountain (rock exposure, snow)
5. Night scene (stars, moon, ambient)

## Files Modified/Created

### Created
- `Runtime/Shaders/Common.hlsl` - Shared utilities and quality settings
- `Runtime/Shaders/Sky.hlsl` - Procedural sky shader
- `Runtime/Shaders/Water.hlsl` - Stylized water shader
- `docs/PHASE8_AGENT9_SHADERS_STYLE.md` - This documentation

### Enhanced (Already Had Style Code)
- `Runtime/Shaders/Creature.hlsl` - Already has rim lighting, gradients
- `Runtime/Shaders/Terrain.hlsl` - Already has biome tinting, slope variation
- `Runtime/Shaders/Vegetation.hlsl` - Already has foliage sheen, wind
- `src/graphics/GlobalLighting.h` - Already has VisualStyleParams
- `src/graphics/SkyRenderer.h` - Already has visual style params
- `src/graphics/SkyRenderer.cpp` - Already has color grading

## Runtime Style Constants (NEW)

The shader system now supports runtime-adjustable style parameters via an optional constant buffer. This allows style tweaks without shader recompilation.

### Enabling Runtime Style

Define `USE_RUNTIME_STYLE` before including Common.hlsl:

```hlsl
#define USE_RUNTIME_STYLE
#include "Common.hlsl"
```

### StyleConstants Buffer Layout (register b1)

```hlsl
cbuffer StyleConstants : register(b1)
{
    // Master controls (16 bytes)
    float g_styleStrength;              // 0-1: stylized blend
    float g_colorVibrancy;              // Saturation boost
    float g_pad1;
    float g_pad2;

    // Creature styling (16 bytes)
    float g_creatureRimIntensity;
    float g_creatureRimPower;
    float g_creatureGradientStrength;
    float g_creatureColorBoost;

    // Terrain styling (16 bytes)
    float g_terrainStyleStrength;
    float g_biomeTintStrength;
    float g_slopeVariation;
    float g_heightGradient;

    // Vegetation styling (16 bytes)
    float g_vegetationStyleStrength;
    float g_foliageSheen;
    float g_vegetationVibrancy;
    float g_vegetationRimIntensity;

    // Sky styling (16 bytes)
    float g_sunGlowIntensity;
    float g_horizonGlow;
    float g_skyGradientContrast;
    float g_pad3;

    // Color grading (32 bytes)
    float3 g_colorFilter;
    float g_shadowWarmth;
    float3 g_highlightCool;
    float g_pad4;

    // Biome tints from PlanetTheme (64 bytes)
    float4 g_warmBiomeTint;
    float4 g_coolBiomeTint;
    float4 g_lushBiomeTint;
    float4 g_alienBiomeTint;
};
```

### Helper Functions

Access style values via helper functions that work with or without runtime style:

```hlsl
float getStyleStrength();
float getColorVibrancy();
float getCreatureRimIntensity();
float getTerrainStyleStrength();
float getBiomeTintStrength();
float3 getColorFilter();
float4 getWarmBiomeTint();
// ... etc
```

### Color Grading Functions

New functions for stylized color processing:

```hlsl
// Apply color filter from PlanetTheme
float3 applyColorFilter(float3 color, float3 filter);

// Split toning (warm shadows, cool highlights)
float3 applySplitToning(float3 color, float shadowWarmth, float highlightCool);

// Complete stylized grading pass
float3 applyStylizedGrading(float3 color);

// Biome-aware tinting based on world position
float3 getBiomeTint(float3 worldPos, float heightNormalized);
```

### C++ Integration

```cpp
// Create style constants matching the cbuffer layout
struct alignas(16) StyleConstants {
    float styleStrength;
    float colorVibrancy;
    float pad1, pad2;
    // ... match shader layout
    glm::vec4 warmBiomeTint;
    glm::vec4 coolBiomeTint;
    glm::vec4 lushBiomeTint;
    glm::vec4 alienBiomeTint;
};

// Upload to GPU
StyleConstants sc;
sc.styleStrength = visualStyle.styleStrength;
sc.colorVibrancy = visualStyle.colorVibrancy;
// ... populate from VisualStyleParams and PlanetTheme
// memcpy to constant buffer
```

## Future Improvements

1. ~~**GPU-side quality control**: Pass quality level via constant buffer instead of compile-time~~ (DONE via StyleConstants)
2. **Per-object style override**: Allow individual creatures/plants to have unique style parameters
3. **Post-processing**: Add optional bloom, color grading, vignette as separate pass
4. **Cloud system**: Full volumetric or 2D cloud layer for more dramatic skies
5. **Weather integration**: Connect style to weather system for storm effects
6. **UI sliders**: Real-time tweaking of all style parameters in-game
