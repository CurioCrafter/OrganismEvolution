// Common.hlsl - Shared shader utilities and quality settings
// Include this file in all shaders for consistent visual style and quality controls

#ifndef COMMON_HLSL
#define COMMON_HLSL

// ============================================================================
// Quality Levels - Global Toggle
// Set via constant buffer or compile-time define
// 0 = Off (minimal/disabled effects)
// 1 = Low (basic effects, good performance)
// 2 = Medium (balanced quality and performance) [DEFAULT]
// 3 = High (full quality, maximum visual fidelity)
// ============================================================================

#ifndef QUALITY_LEVEL
#define QUALITY_LEVEL 2
#endif

// Quality-based feature flags
#define QUALITY_STARS           (QUALITY_LEVEL >= 1)
#define QUALITY_ATMOSPHERE      (QUALITY_LEVEL >= 2)
#define QUALITY_CLOUD_HINTS     (QUALITY_LEVEL >= 2)
#define QUALITY_CAUSTICS        (QUALITY_LEVEL >= 2)
#define QUALITY_SUBSURFACE      (QUALITY_LEVEL >= 1)
#define QUALITY_RIM_LIGHTING    (QUALITY_LEVEL >= 1)
#define QUALITY_GRADIENT_SHADE  (QUALITY_LEVEL >= 1)
#define QUALITY_FOAM_DETAIL     (QUALITY_LEVEL >= 2)
#define QUALITY_WAVE_DETAIL     (QUALITY_LEVEL >= 2)
#define QUALITY_BIOME_BLEND     (QUALITY_LEVEL >= 1)
#define QUALITY_SLOPE_VARIATION (QUALITY_LEVEL >= 2)
#define QUALITY_FOLIAGE_SHEEN   (QUALITY_LEVEL >= 2)

// ============================================================================
// Visual Style Master Controls (No Man's Sky inspired)
// These can be overridden per-shader or via constant buffer
// ============================================================================

#ifndef STYLE_STRENGTH
#define STYLE_STRENGTH 0.7          // 0-1: blend between realistic and stylized
#endif

#ifndef COLOR_VIBRANCY
#define COLOR_VIBRANCY 1.15         // Saturation boost (1.0 = neutral)
#endif

// ============================================================================
// Common Style Parameters
// ============================================================================

// Creature styling
static const float CREATURE_RIM_INTENSITY = 0.45;
static const float CREATURE_RIM_POWER = 2.5;
static const float CREATURE_GRADIENT_STRENGTH = 0.3;
static const float CREATURE_COLOR_BOOST = 1.15;
static const float CREATURE_FRESNEL_BRIGHTNESS = 0.25;

// Terrain styling
static const float TERRAIN_STYLE_STRENGTH = 0.65;
static const float TERRAIN_BIOME_TINT_STRENGTH = 0.4;
static const float TERRAIN_SLOPE_VARIATION = 0.35;
static const float TERRAIN_HEIGHT_GRADIENT = 0.25;
static const float TERRAIN_COLOR_VIBRANCY = 1.12;

// Vegetation styling
static const float VEGETATION_STYLE_STRENGTH = 0.7;
static const float VEGETATION_FOLIAGE_SHEEN = 0.15;
static const float VEGETATION_VIBRANCY = 1.18;
static const float VEGETATION_RIM_INTENSITY = 0.3;

// Sky styling
static const float SKY_SUN_GLOW_INTENSITY = 0.35;
static const float SKY_HORIZON_GLOW = 0.25;
static const float SKY_GRADIENT_CONTRAST = 1.1;
static const float SKY_VIBRANCY = 1.12;

// Water styling
static const float WATER_FOAM_BRIGHTNESS = 0.92;
static const float WATER_SUBSURFACE_STRENGTH = 0.25;
static const float WATER_CAUSTIC_INTENSITY = 0.35;
static const float WATER_RIM_GLOW = 0.12;

// ============================================================================
// Biome Tint Colors
// ============================================================================

static const float3 BIOME_TINT_WARM = float3(1.05, 1.0, 0.92);     // Desert/volcanic
static const float3 BIOME_TINT_COOL = float3(0.92, 0.98, 1.05);    // Tundra/alpine
static const float3 BIOME_TINT_LUSH = float3(0.95, 1.05, 0.95);    // Forest/jungle
static const float3 BIOME_TINT_ALIEN = float3(1.0, 0.95, 1.08);    // Alien purple

// ============================================================================
// Common Noise Functions
// ============================================================================

float3 commonHash3(float3 p)
{
    p = frac(p * float3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return frac((p.xxy + p.yxx) * p.zyx);
}

float commonNoise3D(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);

    float n000 = commonHash3(i + float3(0,0,0)).x;
    float n100 = commonHash3(i + float3(1,0,0)).x;
    float n010 = commonHash3(i + float3(0,1,0)).x;
    float n110 = commonHash3(i + float3(1,1,0)).x;
    float n001 = commonHash3(i + float3(0,0,1)).x;
    float n101 = commonHash3(i + float3(1,0,1)).x;
    float n011 = commonHash3(i + float3(0,1,1)).x;
    float n111 = commonHash3(i + float3(1,1,1)).x;

    return lerp(
        lerp(lerp(n000, n100, f.x), lerp(n010, n110, f.x), f.y),
        lerp(lerp(n001, n101, f.x), lerp(n011, n111, f.x), f.y),
        f.z
    );
}

float commonFBM(float3 p, int octaves)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    [unroll]
    for (int i = 0; i < 5; i++)
    {
        if (i >= octaves) break;
        value += amplitude * commonNoise3D(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }

    return value;
}

// ============================================================================
// Common Color Utilities
// ============================================================================

// Adjust saturation
float3 adjustSaturation(float3 color, float saturation)
{
    float luminance = dot(color, float3(0.299, 0.587, 0.114));
    return lerp(float3(luminance, luminance, luminance), color, saturation);
}

// Apply color vibrancy (saturation boost with luminance preservation)
float3 applyVibrancy(float3 color, float vibrancy)
{
    return adjustSaturation(color, vibrancy);
}

// Stylized gradient shading
float3 applyGradientShading(float3 baseColor, float3 normal, float strength)
{
    float topness = normal.y * 0.5 + 0.5;
    float3 topTint = float3(1.05, 1.02, 0.98);
    float3 bottomTint = float3(0.95, 0.97, 1.02);
    float3 gradientTint = lerp(bottomTint, topTint, topness);
    return baseColor * lerp(float3(1, 1, 1), gradientTint, strength);
}

// Stylized rim lighting
float3 calculateRim(float3 normal, float3 viewDir, float3 lightColor, float intensity, float power)
{
    float NdotV = saturate(dot(normal, viewDir));
    float rimFactor = pow(1.0 - NdotV, power);
    return lightColor * rimFactor * intensity;
}

// Fresnel calculation
float calculateFresnel(float3 viewDir, float3 normal, float power)
{
    float NdotV = saturate(dot(viewDir, normal));
    return pow(1.0 - NdotV, power);
}

// Fresnel with Schlick approximation
float fresnelSchlick(float cosTheta, float F0)
{
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

float3 fresnelSchlick3(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

// ============================================================================
// Atmospheric Fog
// ============================================================================

float3 applyAtmosphericFog(float3 color, float3 fogColor, float distance, float density, float start)
{
    float fogFactor = exp(-density * max(0.0, distance - start));
    fogFactor = saturate(fogFactor);
    return lerp(fogColor, color, fogFactor);
}

// ============================================================================
// Quality-Aware Feature Helpers
// ============================================================================

// Returns true if a feature should be enabled at current quality
bool featureEnabled(int minQuality)
{
    return QUALITY_LEVEL >= minQuality;
}

// Returns a quality-adjusted iteration count
int qualityIterations(int lowCount, int medCount, int highCount)
{
    if (QUALITY_LEVEL >= 3) return highCount;
    if (QUALITY_LEVEL >= 2) return medCount;
    if (QUALITY_LEVEL >= 1) return lowCount;
    return 1;
}

// ============================================================================
// Runtime Style Constants (can be bound as cbuffer b1)
// Maps to VisualStyleParams in GlobalLighting.h
// ============================================================================
#ifdef USE_RUNTIME_STYLE

cbuffer StyleConstants : register(b1)
{
    // Master controls
    float g_styleStrength;              // 0-1: blend between realistic and stylized
    float g_colorVibrancy;              // Saturation boost (1.0 = neutral)
    float g_pad1;
    float g_pad2;

    // Creature styling
    float g_creatureRimIntensity;
    float g_creatureRimPower;
    float g_creatureGradientStrength;
    float g_creatureColorBoost;

    // Terrain styling
    float g_terrainStyleStrength;
    float g_biomeTintStrength;
    float g_slopeVariation;
    float g_heightGradient;

    // Vegetation styling
    float g_vegetationStyleStrength;
    float g_foliageSheen;
    float g_vegetationVibrancy;
    float g_vegetationRimIntensity;

    // Sky and atmosphere
    float g_sunGlowIntensity;
    float g_horizonGlow;
    float g_skyGradientContrast;
    float g_pad3;

    // Color grading
    float3 g_colorFilter;
    float g_shadowWarmth;
    float3 g_highlightCool;
    float g_pad4;

    // Biome tints (from PlanetTheme)
    float4 g_warmBiomeTint;
    float4 g_coolBiomeTint;
    float4 g_lushBiomeTint;
    float4 g_alienBiomeTint;
};

// Helper functions to get style values (uses runtime or compile-time)
float getStyleStrength() { return g_styleStrength; }
float getColorVibrancy() { return g_colorVibrancy; }
float getCreatureRimIntensity() { return g_creatureRimIntensity; }
float getCreatureRimPower() { return g_creatureRimPower; }
float getCreatureGradientStrength() { return g_creatureGradientStrength; }
float getCreatureColorBoost() { return g_creatureColorBoost; }
float getTerrainStyleStrength() { return g_terrainStyleStrength; }
float getBiomeTintStrength() { return g_biomeTintStrength; }
float getSlopeVariation() { return g_slopeVariation; }
float getHeightGradient() { return g_heightGradient; }
float getVegetationStyleStrength() { return g_vegetationStyleStrength; }
float getFoliageSheen() { return g_foliageSheen; }
float getVegetationVibrancy() { return g_vegetationVibrancy; }
float getVegetationRimIntensity() { return g_vegetationRimIntensity; }
float getSunGlowIntensity() { return g_sunGlowIntensity; }
float getHorizonGlow() { return g_horizonGlow; }
float getSkyGradientContrast() { return g_skyGradientContrast; }
float3 getColorFilter() { return g_colorFilter; }
float getShadowWarmth() { return g_shadowWarmth; }
float4 getWarmBiomeTint() { return g_warmBiomeTint; }
float4 getCoolBiomeTint() { return g_coolBiomeTint; }
float4 getLushBiomeTint() { return g_lushBiomeTint; }
float4 getAlienBiomeTint() { return g_alienBiomeTint; }

#else

// Compile-time defaults when runtime style is not bound
float getStyleStrength() { return STYLE_STRENGTH; }
float getColorVibrancy() { return COLOR_VIBRANCY; }
float getCreatureRimIntensity() { return CREATURE_RIM_INTENSITY; }
float getCreatureRimPower() { return CREATURE_RIM_POWER; }
float getCreatureGradientStrength() { return CREATURE_GRADIENT_STRENGTH; }
float getCreatureColorBoost() { return CREATURE_COLOR_BOOST; }
float getTerrainStyleStrength() { return TERRAIN_STYLE_STRENGTH; }
float getBiomeTintStrength() { return TERRAIN_BIOME_TINT_STRENGTH; }
float getSlopeVariation() { return TERRAIN_SLOPE_VARIATION; }
float getHeightGradient() { return TERRAIN_HEIGHT_GRADIENT; }
float getVegetationStyleStrength() { return VEGETATION_STYLE_STRENGTH; }
float getFoliageSheen() { return VEGETATION_FOLIAGE_SHEEN; }
float getVegetationVibrancy() { return VEGETATION_VIBRANCY; }
float getVegetationRimIntensity() { return VEGETATION_RIM_INTENSITY; }
float getSunGlowIntensity() { return SKY_SUN_GLOW_INTENSITY; }
float getHorizonGlow() { return SKY_HORIZON_GLOW; }
float getSkyGradientContrast() { return SKY_GRADIENT_CONTRAST; }
float3 getColorFilter() { return float3(1.0, 1.0, 1.0); }
float getShadowWarmth() { return 0.1; }
float4 getWarmBiomeTint() { return float4(BIOME_TINT_WARM, 1.0); }
float4 getCoolBiomeTint() { return float4(BIOME_TINT_COOL, 1.0); }
float4 getLushBiomeTint() { return float4(BIOME_TINT_LUSH, 1.0); }
float4 getAlienBiomeTint() { return float4(BIOME_TINT_ALIEN, 1.0); }

#endif // USE_RUNTIME_STYLE

// ============================================================================
// Enhanced Color Grading Functions
// ============================================================================

// Apply color filter from PlanetTheme
float3 applyColorFilter(float3 color, float3 filter)
{
    return color * filter;
}

// Apply shadow warmth / highlight cool split toning
float3 applySplitToning(float3 color, float shadowWarmth, float highlightCool)
{
    float luminance = dot(color, float3(0.299, 0.587, 0.114));

    // Warm shadows (add red/orange in dark areas)
    float shadowMask = 1.0 - luminance;
    float3 warmShift = float3(shadowWarmth * 0.15, shadowWarmth * 0.05, -shadowWarmth * 0.05);
    color += warmShift * shadowMask;

    // Cool highlights (add blue in bright areas)
    float highlightMask = luminance;
    float3 coolShift = float3(-highlightCool * 0.05, 0.0, highlightCool * 0.08);
    color += coolShift * highlightMask;

    return color;
}

// Complete stylized color grading pass
float3 applyStylizedGrading(float3 color)
{
    // Apply color filter
    color = applyColorFilter(color, getColorFilter());

    // Apply vibrancy
    color = applyVibrancy(color, getColorVibrancy());

    // Apply split toning
    color = applySplitToning(color, getShadowWarmth(), 0.05);

    return color;
}

// ============================================================================
// Biome-Aware Tinting
// ============================================================================

// Get biome tint based on world position and height
float3 getBiomeTint(float3 worldPos, float heightNormalized)
{
    // Determine biome influence based on height
    float desertInfluence = smoothstep(0.05, 0.25, heightNormalized) * (1.0 - smoothstep(0.25, 0.45, heightNormalized));
    float lushInfluence = smoothstep(0.15, 0.35, heightNormalized) * (1.0 - smoothstep(0.55, 0.75, heightNormalized));
    float iceInfluence = smoothstep(0.75, 0.95, heightNormalized);

    // Position-based variation for alien patches
    float biomeNoise = commonFBM(worldPos * 0.02, 3);
    float alienPatch = smoothstep(0.65, 0.85, biomeNoise) * 0.3;

    // Combine tints
    float3 tint = float3(1, 1, 1);
    tint = lerp(tint, getWarmBiomeTint().rgb, desertInfluence * getBiomeTintStrength());
    tint = lerp(tint, getLushBiomeTint().rgb, lushInfluence * getBiomeTintStrength());
    tint = lerp(tint, getCoolBiomeTint().rgb, iceInfluence * getBiomeTintStrength());
    tint = lerp(tint, getAlienBiomeTint().rgb, alienPatch * getBiomeTintStrength());

    return tint;
}

#endif // COMMON_HLSL
