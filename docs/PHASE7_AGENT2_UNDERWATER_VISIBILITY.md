# Phase 7 - Agent 2: Underwater Visibility and Water Atmosphere

## Overview

This document describes the underwater visibility system that provides clear, readable, and beautiful underwater environments. The system includes:

- Depth-based fog and color absorption
- Animated caustic patterns
- Light shaft effects (god rays)
- Proper water surface rendering from below (Snell's window)
- Quality presets for performance scaling

## UnderwaterParams Structure

The main configuration struct is `UnderwaterParams` in [WaterRenderer.h](../src/graphics/WaterRenderer.h):

```cpp
struct UnderwaterParams {
    // Fog and visibility
    glm::vec3 fogColor;           // Deep blue fog color (default: 0.0, 0.15, 0.3)
    float fogDensity;             // Fog accumulation rate (default: 0.02, lower = clearer)

    glm::vec3 absorptionRGB;      // Per-channel light absorption (default: 0.4, 0.15, 0.05)
                                  // Red absorbs fastest, blue slowest (realistic)
    float clarityScalar;          // Visibility range multiplier (default: 1.0)

    // Caustics
    float causticIntensity;       // Animated caustic brightness (default: 0.3)
    float causticScale;           // Caustic pattern size (default: 0.02)

    // Light shafts
    float lightShaftIntensity;    // Sun shaft strength (default: 0.4)
    float lightShaftDecay;        // Radial falloff (default: 0.95)

    // Fog range
    float fogStart;               // Distance before fog begins (default: 5.0)
    float fogEnd;                 // Distance at full fog (default: 150.0)
    float depthTintStrength;      // How much depth affects tint (default: 0.3)
    float surfaceDistortion;      // Distortion from below (default: 0.02)

    // Quality
    int qualityLevel;             // 0=off, 1=low, 2=medium, 3=high
};
```

### Suggested Parameter Ranges

| Parameter | Min | Default | Max | Description |
|-----------|-----|---------|-----|-------------|
| fogDensity | 0.0 | 0.02 | 0.1 | Higher = murkier water |
| clarityScalar | 0.5 | 1.0 | 2.0 | Multiplies fog distances |
| causticIntensity | 0.0 | 0.3 | 1.0 | 0 = disabled |
| lightShaftIntensity | 0.0 | 0.4 | 1.0 | 0 = disabled |
| fogStart | 0.0 | 5.0 | 50.0 | Meters before fog starts |
| fogEnd | 50.0 | 150.0 | 500.0 | Meters at full fog |
| depthTintStrength | 0.0 | 0.3 | 1.0 | Blue tint with depth |

## Underwater Depth Computation

The underwater depth is computed in `WaterRenderer`:

```cpp
// In WaterRenderer.h
bool IsCameraUnderwater(const glm::vec3& cameraPos) const {
    return cameraPos.y < m_waterHeight;
}

float GetUnderwaterDepth(const glm::vec3& cameraPos) const {
    return m_waterHeight - cameraPos.y;  // Positive = underwater
}
```

The depth is passed to shaders via the constant buffer and used to:
1. Determine if underwater effects should be applied
2. Scale fog intensity with depth
3. Fade caustics and light shafts near the surface

## Quality Levels and Performance

### Quality Presets

| Level | Name | Features | Performance Impact |
|-------|------|----------|-------------------|
| 0 | Off | No underwater effects | None |
| 1 | Low | Fog + absorption only | Minimal |
| 2 | Medium | + Caustics | Low |
| 3 | High | + Light shafts | Moderate |

### Using Quality Presets

```cpp
// Apply a quality preset
waterRenderer.GetUnderwaterParamsMutable().ApplyQualityPreset(2); // Medium

// Or manually set quality level
waterRenderer.SetUnderwaterQuality(1); // Low
```

### Performance Notes

1. **Fog and Absorption (Level 1)**: Nearly free - simple math in post-process
2. **Caustics (Level 2)**: Adds sine wave calculations per pixel, ~0.5ms on mid-range GPU
3. **Light Shafts (Level 3)**: Screen-space radial sampling, ~1ms on mid-range GPU

The underwater pass uses the same buffer as the HDR scene, avoiding additional render target allocations.

## Integration Notes for Agent 10 (UI)

### Available Setters on WaterRenderer

```cpp
// Direct parameter setters
void SetUnderwaterFogColor(const glm::vec3& color);
void SetUnderwaterFogDensity(float density);
void SetUnderwaterClarity(float clarity);
void SetUnderwaterAbsorption(const glm::vec3& absorption);
void SetCausticIntensity(float intensity);
void SetLightShaftIntensity(float intensity);
void SetUnderwaterQuality(int level);  // 0-3

// Full parameter struct access
void SetUnderwaterParams(const UnderwaterParams& params);
const UnderwaterParams& GetUnderwaterParams() const;
UnderwaterParams& GetUnderwaterParamsMutable();
```

### Available Setters on PostProcessManagerDX12

```cpp
// Toggle underwater effects
bool enableUnderwater;                    // Master toggle

// Individual parameters
DirectX::XMFLOAT3 underwaterFogColor;
float underwaterFogDensity;
DirectX::XMFLOAT3 underwaterAbsorption;
float underwaterFogStart;
float underwaterFogEnd;
float underwaterClarity;
float underwaterDepthTint;
float underwaterCausticIntensity;
float underwaterCausticScale;
float underwaterLightShaftIntensity;
float underwaterLightShaftDecay;
float underwaterSurfaceDistortion;
int underwaterQuality;                    // 0=off, 1=low, 2=med, 3=high
```

### Suggested UI Layout

```
[Underwater Settings]
  [ ] Enable Underwater Effects     <- enableUnderwater
  [Quality: Low|Medium|High]        <- underwaterQuality (1/2/3)

  [Visibility]
    Clarity:     [====|====]        <- underwaterClarity (0.5-2.0)
    Fog Density: [====|====]        <- underwaterFogDensity (0.0-0.1)
    Fog Color:   [Color Picker]     <- underwaterFogColor

  [Effects] (visible when quality >= 2)
    Caustic Intensity: [====|====]  <- underwaterCausticIntensity (0.0-1.0)

  [Advanced] (visible when quality == 3)
    Light Shaft Intensity: [====|====] <- underwaterLightShaftIntensity
```

## Shader Files

- **Underwater.hlsl**: [Runtime/Shaders/Underwater.hlsl](../Runtime/Shaders/Underwater.hlsl)
  - `CSMain`: Full quality compute shader
  - `CSMainLowQuality`: Optimized low quality path

- **Water surface shader**: Embedded in [WaterRenderer.cpp](../src/graphics/WaterRenderer.cpp)
  - Handles both above-water and underwater surface rendering
  - Implements Snell's window effect for underwater view

## Files Modified

- `src/graphics/WaterRenderer.h` - Added UnderwaterParams struct and setters
- `src/graphics/WaterRenderer.cpp` - Added underwater depth calculation, updated shader
- `src/graphics/PostProcess_DX12.h` - Added UnderwaterConstants, underwater parameters
- `src/graphics/PostProcess_DX12.cpp` - Added underwater buffer and render pass
- `Runtime/Shaders/Underwater.hlsl` - New underwater post-process shader

## Files NOT Modified (per parallel safety)

- `Runtime/Shaders/Terrain.hlsl`
- `Runtime/Shaders/Weather.hlsl`
- `src/graphics/GlobalLighting.*`
- `src/ui/*`
