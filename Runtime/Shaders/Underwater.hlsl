// Underwater Post-Process Shader - Fog, Absorption, Caustics, Light Shafts
// Provides clear, readable underwater visibility with beautiful atmosphere

#define THREAD_GROUP_SIZE_X 8
#define THREAD_GROUP_SIZE_Y 8

// ============================================================================
// Constant Buffer
// ============================================================================

cbuffer UnderwaterConstants : register(b0)
{
    // Fog and visibility
    float3 fogColor;              // Underwater fog tint (blue-green)
    float fogDensity;             // How quickly fog accumulates
    float3 absorptionRGB;         // Per-channel light absorption (red absorbs fastest)
    float fogStart;               // Distance before fog begins
    float fogEnd;                 // Distance at full fog opacity
    float clarityScalar;          // Multiplier for visibility range
    float underwaterDepth;        // How deep camera is below surface
    float depthTintStrength;      // How much depth affects tint

    // Light shafts
    float2 sunScreenPos;          // Sun position in screen space for shaft origin
    float lightShaftIntensity;    // Strength of god rays
    float lightShaftDecay;        // Radial falloff

    // Caustics
    float causticIntensity;       // Animated caustic brightness
    float causticScale;           // Size of caustic pattern
    float time;                   // Animation time
    float surfaceDistortion;      // Distortion when looking at surface from below

    // Screen info
    float2 screenSize;
    int qualityLevel;             // 0=off, 1=low, 2=medium, 3=high
    float padding;
};

// ============================================================================
// Resources
// ============================================================================

Texture2D<float4> sceneTexture : register(t0);
Texture2D<float> depthTexture : register(t1);
RWTexture2D<float4> outputTexture : register(u0);

SamplerState linearSampler : register(s0);
SamplerState pointSampler : register(s1);

// ============================================================================
// Noise Functions (for caustics and variation)
// ============================================================================

float hash(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

float noise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);  // Smooth interpolation

    float a = hash(i);
    float b = hash(i + float2(1, 0));
    float c = hash(i + float2(0, 1));
    float d = hash(i + float2(1, 1));

    return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

float fbm(float2 p, int octaves)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int i = 0; i < octaves; i++)
    {
        value += amplitude * noise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }

    return value;
}

// ============================================================================
// Caustic Pattern Generation
// ============================================================================

float causticPattern(float2 worldXZ, float time)
{
    // Multi-layer animated caustics using sine waves
    float2 uv1 = worldXZ * causticScale + float2(time * 0.3, time * 0.2);
    float2 uv2 = worldXZ * causticScale * 1.5 + float2(-time * 0.25, time * 0.15);
    float2 uv3 = worldXZ * causticScale * 0.7 + float2(time * 0.1, -time * 0.35);

    // Create cell-like pattern using sine waves
    float c1 = sin(uv1.x * 6.28318 + sin(uv1.y * 4.0)) * 0.5 + 0.5;
    float c2 = sin(uv2.y * 6.28318 + sin(uv2.x * 5.0)) * 0.5 + 0.5;
    float c3 = sin((uv3.x + uv3.y) * 6.28318) * 0.5 + 0.5;

    // Combine with noise for organic variation
    float n = fbm(worldXZ * causticScale * 3.0 + time * 0.1, 2);

    // Create caustic peaks
    float caustic = c1 * c2 * 2.0;
    caustic = pow(caustic, 2.0);  // Sharpen peaks
    caustic += c3 * 0.3;
    caustic = caustic * (0.7 + n * 0.3);

    return saturate(caustic);
}

// ============================================================================
// Light Shaft Calculation (cheap screen-space radial blur approximation)
// ============================================================================

float calculateLightShafts(float2 uv, float2 sunPos)
{
    if (lightShaftIntensity < 0.01) return 0.0;

    // Direction from pixel to sun
    float2 deltaUV = (sunPos - uv);
    float distToSun = length(deltaUV);

    // Skip if sun is off-screen or too far
    if (distToSun > 1.5) return 0.0;

    // Radial falloff from sun position
    float radialFalloff = 1.0 - saturate(distToSun);
    radialFalloff = pow(radialFalloff, 2.0);

    // Simple radial gradient with noise for variation
    float shaftNoise = noise(uv * 20.0 + time * 0.5) * 0.3 + 0.7;

    // Angular variation for shaft appearance
    float angle = atan2(deltaUV.y, deltaUV.x);
    float angularPattern = sin(angle * 8.0) * 0.5 + 0.5;
    angularPattern = pow(angularPattern, 3.0) * 0.5 + 0.5;

    float shafts = radialFalloff * shaftNoise * angularPattern;

    // Fade with depth (shafts stronger near surface)
    float depthFade = exp(-underwaterDepth * 0.1);

    return shafts * lightShaftIntensity * depthFade;
}

// ============================================================================
// Depth-Based Color Absorption
// ============================================================================

float3 applyAbsorption(float3 color, float distance)
{
    // Water absorbs light wavelength-dependently
    // Red is absorbed fastest, blue slowest
    float3 absorption = absorptionRGB * distance * 0.01;

    // Exponential absorption
    float3 transmitted = color * exp(-absorption);

    return transmitted;
}

// ============================================================================
// Underwater Fog with Clarity
// ============================================================================

float3 applyUnderwaterFog(float3 color, float distance, float viewDepth)
{
    // Adjust fog range by clarity
    float adjustedFogEnd = fogEnd * clarityScalar;
    float adjustedFogStart = fogStart * clarityScalar;

    // Calculate fog factor with smooth falloff
    float fogFactor = saturate((distance - adjustedFogStart) / (adjustedFogEnd - adjustedFogStart));
    fogFactor = fogFactor * fogFactor;  // Quadratic falloff for better silhouette preservation

    // Depth-based tint (deeper = bluer)
    float depthFactor = saturate(underwaterDepth / 50.0);  // Normalize to ~50m depth
    float3 depthTint = lerp(fogColor, fogColor * float3(0.7, 0.8, 1.0), depthFactor * depthTintStrength);

    // Blend scene with fog color
    float3 foggedColor = lerp(color, depthTint, fogFactor * fogDensity);

    return foggedColor;
}

// ============================================================================
// Main Compute Shader
// ============================================================================

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixelCoord = dispatchThreadID.xy;
    if (pixelCoord.x >= (uint)screenSize.x || pixelCoord.y >= (uint)screenSize.y)
        return;

    float2 uv = (float2(pixelCoord) + 0.5) / screenSize;

    // Sample scene color and depth
    float4 sceneColor = sceneTexture.Load(int3(pixelCoord, 0));
    float depth = depthTexture.Load(int3(pixelCoord, 0));

    // Convert depth to linear distance (simplified - assumes standard perspective)
    // In production, use proper inverse projection
    float linearDepth = 1.0 / (1.0 - depth + 0.001);
    float viewDistance = linearDepth * 100.0;  // Scale factor

    float3 outputColor = sceneColor.rgb;

    // Skip processing for very close objects (UI, particles at surface)
    if (depth > 0.999 || qualityLevel == 0)
    {
        outputTexture[pixelCoord] = float4(outputColor, sceneColor.a);
        return;
    }

    // ========================================================================
    // Phase 1: Color Absorption
    // ========================================================================
    outputColor = applyAbsorption(outputColor, viewDistance);

    // ========================================================================
    // Phase 2: Underwater Fog
    // ========================================================================
    outputColor = applyUnderwaterFog(outputColor, viewDistance, underwaterDepth);

    // ========================================================================
    // Phase 3: Caustics (quality level 2+)
    // ========================================================================
    if (qualityLevel >= 2 && causticIntensity > 0.01)
    {
        // Approximate world position from screen UV (simplified)
        // In production, use inverse view-projection matrix
        float2 worldXZ = (uv - 0.5) * viewDistance * 2.0;

        float caustic = causticPattern(worldXZ, time);

        // Caustics fade with depth (stronger near surface)
        float causticDepthFade = exp(-underwaterDepth * 0.05);

        // Also fade with view distance
        float causticDistFade = 1.0 - saturate(viewDistance / (fogEnd * clarityScalar));

        outputColor += caustic * causticIntensity * causticDepthFade * causticDistFade * fogColor;
    }

    // ========================================================================
    // Phase 4: Light Shafts (quality level 3)
    // ========================================================================
    if (qualityLevel >= 3)
    {
        float shaftLight = calculateLightShafts(uv, sunScreenPos);
        outputColor += shaftLight * float3(0.8, 0.9, 1.0);  // Pale blue shaft color
    }

    // ========================================================================
    // Final Output
    // ========================================================================
    // Subtle blue shift to entire underwater scene
    float3 underwaterTint = lerp(float3(1, 1, 1), float3(0.9, 0.95, 1.05), 0.2);
    outputColor *= underwaterTint;

    outputTexture[pixelCoord] = float4(outputColor, sceneColor.a);
}

// ============================================================================
// Alternative Entry Point for Low Quality (no caustics/shafts)
// ============================================================================

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMainLowQuality(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixelCoord = dispatchThreadID.xy;
    if (pixelCoord.x >= (uint)screenSize.x || pixelCoord.y >= (uint)screenSize.y)
        return;

    float2 uv = (float2(pixelCoord) + 0.5) / screenSize;

    float4 sceneColor = sceneTexture.Load(int3(pixelCoord, 0));
    float depth = depthTexture.Load(int3(pixelCoord, 0));

    float linearDepth = 1.0 / (1.0 - depth + 0.001);
    float viewDistance = linearDepth * 100.0;

    float3 outputColor = sceneColor.rgb;

    if (depth > 0.999)
    {
        outputTexture[pixelCoord] = float4(outputColor, sceneColor.a);
        return;
    }

    // Simple absorption and fog only
    outputColor = applyAbsorption(outputColor, viewDistance);
    outputColor = applyUnderwaterFog(outputColor, viewDistance, underwaterDepth);

    // Blue tint
    outputColor *= float3(0.9, 0.95, 1.05);

    outputTexture[pixelCoord] = float4(outputColor, sceneColor.a);
}
