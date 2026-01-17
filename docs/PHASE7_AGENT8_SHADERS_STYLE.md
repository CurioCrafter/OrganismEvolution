# Phase 7 Agent 8: Shaders and Visual Identity (No Man's Sky Style)

## Overview

This document describes the stylized visual rendering system inspired by No Man's Sky, providing distinct biome visuals while maintaining performance and compatibility with existing systems.

## Files Modified

### Owned Files (Modified)
- `Runtime/Shaders/Creature.hlsl` - Stylized rim lighting and gradient shading for creatures
- `Runtime/Shaders/Terrain.hlsl` - Biome-specific tints and slope/height blending
- `Runtime/Shaders/Vegetation.hlsl` - Foliage sheen and vibrant vegetation colors
- `src/graphics/SkyRenderer.h/.cpp` - Sky color grading and atmosphere tints
- `src/graphics/GlobalLighting.h` - Centralized VisualStyleParams configuration

### Protected Files (Not Modified)
- `src/graphics/WaterRenderer.*` - Water rendering unchanged (Agent 2 owned)
- `src/graphics/PostProcess_DX12.*` - Post-processing unchanged (Agent 1 owned)
- `src/ui/*` - UI unchanged
- `src/environment/PlanetTheme.*` - Theme data consumed via getters only

---

## Shader Parameters and Defaults

### Global Style Controls (GlobalLighting.h - VisualStyleParams)

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| `styleStrength` | 0.7 | 0-1 | Master blend between realistic and stylized |
| `colorVibrancy` | 1.15 | 0.5-2 | Overall saturation boost (1.0 = neutral) |

### Creature Shading (Creature.hlsl)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `STYLE_STRENGTH` | 0.7 | Blend between realistic and stylized |
| `RIM_INTENSITY` | 0.45 | Stylized rim lighting intensity |
| `RIM_POWER` | 2.5 | Rim falloff (lower = broader rim) |
| `GRADIENT_STRENGTH` | 0.3 | Vertical gradient shading strength |
| `COLOR_BOOST` | 1.15 | Creature color saturation |
| `FRESNEL_BRIGHTNESS` | 0.25 | Edge brightness boost |

**Features:**
- Stylized rim lighting with backlight support
- Gradient shading (warmer at top, cooler at bottom)
- Iridescent shimmer for aquatic creatures
- Preserved PBR lighting as base

### Terrain Materials (Terrain.hlsl)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `TERRAIN_STYLE_STRENGTH` | 0.65 | Terrain style blend |
| `BIOME_TINT_STRENGTH` | 0.4 | How strongly biomes color terrain |
| `SLOPE_VARIATION_STRENGTH` | 0.35 | Rock exposure on steep slopes |
| `HEIGHT_GRADIENT_STRENGTH` | 0.25 | Vertical color gradient |
| `TERRAIN_COLOR_VIBRANCY` | 1.12 | Terrain saturation boost |

**Biome Palette Constants:**
```hlsl
// Desert biome - warm oranges
DESERT_PRIMARY = float3(0.88, 0.72, 0.52)
DESERT_SECONDARY = float3(0.95, 0.58, 0.38)

// Lush biome - vibrant greens
LUSH_PRIMARY = float3(0.32, 0.68, 0.35)
LUSH_SECONDARY = float3(0.22, 0.55, 0.28)

// Ice biome - cool blues
ICE_PRIMARY = float3(0.85, 0.92, 0.98)
ICE_SECONDARY = float3(0.72, 0.85, 0.95)

// Alien biome - purples and teals
ALIEN_PRIMARY = float3(0.65, 0.48, 0.78)
ALIEN_SECONDARY = float3(0.35, 0.72, 0.68)
```

**Features:**
- Slope-based rock/cliff exposure
- Height-based color gradient (warm lowlands, cool highlands)
- Biome-specific tinting based on position/height
- Alien biome patches via noise

### Vegetation Shading (Vegetation.hlsl)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `VEGETATION_STYLE_STRENGTH` | 0.7 | Vegetation style blend |
| `FOLIAGE_SHEEN` | 0.15 | Waxy leaf shine |
| `COLOR_VIBRANCY` | 1.18 | Plant saturation boost |
| `RIM_INTENSITY` | 0.3 | Edge lighting on leaves |

**Features:**
- Waxy foliage sheen (broader than specular)
- Enhanced subsurface scattering for leaves
- Stylized rim lighting tinted by leaf color
- Higher vibrancy for natural pop

### Sky and Atmosphere (SkyRenderer)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `sunGlowIntensity` | 0.3 | Stylized sun glow |
| `horizonGlow` | 0.25 | Horizon atmospheric warmth |
| `skyGradientContrast` | 1.1 | Sky color contrast |
| `shadowWarmth` | 0.1 | Warm shadows color grading |
| `highlightCool` | 0.05 | Cool highlights color grading |

**Features:**
- Color grading applied to sky colors
- Enhanced sun warmth
- Horizon glow effect
- Increased sky gradient contrast

---

## Biome Style Mapping Rules

### Height-Based Biome Detection
```
0.00 - 0.15: Water/Beach (excluded from styling)
0.15 - 0.25: Desert influence (warm tones)
0.25 - 0.55: Lush influence (green tones)
0.55 - 0.75: Transitional
0.75 - 1.00: Ice influence (cool tones)
```

### Position-Based Variation
- Alien biome patches appear via noise (threshold > 0.65)
- Creates "pockets" of exotic colors within standard biomes
- Blends smoothly with surrounding terrain

### Theme Presets (VisualStyleParams)

| Preset | Style | Characteristics |
|--------|-------|-----------------|
| `getEarthLike()` | Default | Balanced, natural colors |
| `getAlienPurple()` | Exotic | Purple tints, high vibrancy (1.25) |
| `getDesertWorld()` | Warm | Orange filter, strong sun glow |
| `getFrozenWorld()` | Cool | Blue filter, high highlights |

---

## Performance Notes

### GPU Cost
- **Creature shader**: +3-5 ALU ops per pixel (rim + gradient calculations)
- **Terrain shader**: +4-6 ALU ops per pixel (slope + height + biome tinting)
- **Vegetation shader**: +3-4 ALU ops per pixel (sheen + rim)
- **Sky renderer**: CPU-side color grading (negligible)

### Memory
- No additional textures required
- No additional constant buffers
- Uses existing cbuffer slots

### Optimization Opportunities
1. Set `styleStrength = 0` to completely disable stylization
2. Reduce `TERRAIN_STYLE_STRENGTH` on low-end hardware
3. Biome tinting can be removed by setting `BIOME_TINT_STRENGTH = 0`

---

## Toggle Strategy

### Runtime Toggles (C++ Side)
```cpp
// Full stylization
globalLighting.setVisualStyle(VisualStyleParams::getEarthLike());

// Disable stylization
VisualStyleParams disabled;
disabled.styleStrength = 0.0f;
globalLighting.setVisualStyle(disabled);

// Quality levels
VisualStyleParams lowQuality;
lowQuality.styleStrength = 0.3f;
lowQuality.biomeTintStrength = 0.0f;
globalLighting.setVisualStyle(lowQuality);
```

### Shader-Level Toggles
All stylized effects are gated by style strength constants:
```hlsl
// In pixel shader
float3 result = lerp(realisticResult, stylizedResult, STYLE_STRENGTH);
```

Setting `STYLE_STRENGTH = 0` returns pure realistic rendering.

---

## Validation Checklist

- [x] Multiple biomes appear visually distinct in single run
- [x] Creature stylization blends with PBR lighting
- [x] Terrain slope variation creates natural rock exposure
- [x] Vegetation has subtle sheen without oversaturation
- [x] Sky colors respond to time of day
- [x] Water rendering unchanged (isWater check bypasses styling)
- [x] Underwater rendering unchanged (separate fog path)
- [x] No impact on WaterRenderer (Agent 2 files untouched)
- [x] No impact on PostProcess_DX12 (Agent 1 files untouched)
- [x] All parameters have safe defaults
- [x] Style can be disabled via styleStrength = 0

---

## Usage Example

```cpp
// In game initialization or biome transition
void applyBiomeVisualStyle(BiomeType biome, GlobalLighting& lighting) {
    VisualStyleParams style;

    switch (biome) {
        case BiomeType::DESERT:
            style = VisualStyleParams::getDesertWorld();
            break;
        case BiomeType::TUNDRA:
            style = VisualStyleParams::getFrozenWorld();
            break;
        case BiomeType::ALIEN:
            style = VisualStyleParams::getAlienPurple();
            break;
        default:
            style = VisualStyleParams::getEarthLike();
    }

    lighting.setVisualStyle(style);
}
```

---

## Future Enhancements

1. **Per-biome shader variants**: Compile different parameter sets per biome
2. **Dynamic style interpolation**: Smooth transitions when crossing biome boundaries
3. **Weather integration**: Adjust style params based on weather (e.g., muted during rain)
4. **Time-of-day presets**: Different style params for dawn/noon/dusk/night
5. **Creature biome adaptation**: Tint creatures based on home biome
