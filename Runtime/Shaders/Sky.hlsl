// Sky shader - Procedural sky with stylized No Man's Sky aesthetics
// Renders as fullscreen triangle using SV_VertexID
// Features: gradient sky, sun/moon, stars, atmospheric haze, color grading

// ============================================================================
// Constant Buffer (must match SkyConstants in SkyRenderer.h)
// ============================================================================

cbuffer SkyConstants : register(b0)
{
    // View/Projection
    float4x4 invViewProj;         // 0-63

    // Sun parameters
    float3 sunDirection;          // 64-75
    float sunIntensity;           // 76-79
    float3 sunColor;              // 80-91
    float sunSize;                // 92-95

    // Moon parameters
    float3 moonDirection;         // 96-107
    float moonPhase;              // 108-111
    float3 moonColor;             // 112-123
    float moonSize;               // 124-127

    // Sky gradient
    float3 zenithColor;           // 128-139
    float starVisibility;         // 140-143
    float3 horizonColor;          // 144-155
    float time;                   // 156-159

    // Atmosphere
    float3 fogColor;              // 160-171
    float fogDensity;             // 172-175

    // Camera
    float3 cameraPosition;        // 176-187
    float padding1;               // 188-191

    // Additional parameters
    float rayleighStrength;       // 192-195
    float mieStrength;            // 196-199
    float mieG;                   // 200-203
    float atmosphereHeight;       // 204-207
};

// ============================================================================
// Visual Style Parameters (No Man's Sky inspired)
// ============================================================================

static const float STYLE_STRENGTH = 0.7;          // 0-1: stylized vs realistic
static const float SUN_GLOW_SIZE = 0.15;          // Size of stylized sun glow
static const float SUN_GLOW_INTENSITY = 0.35;     // Intensity of sun glow
static const float HORIZON_GLOW_STRENGTH = 0.25;  // Atmospheric glow at horizon
static const float SKY_VIBRANCY = 1.12;           // Color saturation boost
static const float STAR_TWINKLE_SPEED = 2.5;      // Twinkle animation speed
static const float CLOUD_HINT_STRENGTH = 0.08;    // Subtle procedural cloud hints

// Quality levels (0=off, 1=low, 2=medium, 3=high)
static const int QUALITY_LEVEL = 2;

// ============================================================================
// Noise Functions
// ============================================================================

float3 hash3(float3 p)
{
    p = frac(p * float3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return frac((p.xxy + p.yxx) * p.zyx);
}

float noise3D(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);

    float n000 = hash3(i + float3(0,0,0)).x;
    float n100 = hash3(i + float3(1,0,0)).x;
    float n010 = hash3(i + float3(0,1,0)).x;
    float n110 = hash3(i + float3(1,1,0)).x;
    float n001 = hash3(i + float3(0,0,1)).x;
    float n101 = hash3(i + float3(1,0,1)).x;
    float n011 = hash3(i + float3(0,1,1)).x;
    float n111 = hash3(i + float3(1,1,1)).x;

    return lerp(
        lerp(lerp(n000, n100, f.x), lerp(n010, n110, f.x), f.y),
        lerp(lerp(n001, n101, f.x), lerp(n011, n111, f.x), f.y),
        f.z
    );
}

float fbm(float3 p, int octaves)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    [unroll]
    for (int i = 0; i < 4; i++)
    {
        if (i >= octaves) break;
        value += amplitude * noise3D(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }

    return value;
}

// ============================================================================
// Input/Output Structures
// ============================================================================

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 viewDir : TEXCOORD1;
};

// ============================================================================
// Vertex Shader - Fullscreen Triangle
// ============================================================================

VSOutput VSMain(uint vertexID : SV_VertexID)
{
    VSOutput output;

    // Generate fullscreen triangle vertices
    // Triangle covers [-1,-1] to [3,3] for seamless fullscreen coverage
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(uv * 2.0 - 1.0, 0.999999, 1.0);  // Far plane
    output.uv = uv;

    // Calculate view direction from UV
    float4 nearPos = float4(uv * 2.0 - 1.0, 0.0, 1.0);
    float4 farPos = float4(uv * 2.0 - 1.0, 1.0, 1.0);

    float4 worldNear = mul(invViewProj, nearPos);
    float4 worldFar = mul(invViewProj, farPos);

    worldNear /= worldNear.w;
    worldFar /= worldFar.w;

    output.viewDir = normalize(worldFar.xyz - worldNear.xyz);

    return output;
}

// ============================================================================
// Star Field Generation
// ============================================================================

float3 generateStars(float3 viewDir, float visibility)
{
    if (visibility <= 0.0 || QUALITY_LEVEL < 1) return float3(0, 0, 0);

    // High-frequency star noise
    float3 starPos = viewDir * 500.0;
    float starNoise = noise3D(starPos * 100.0);

    // Only brightest points become stars
    float starThreshold = 0.97;
    float star = smoothstep(starThreshold, 1.0, starNoise);

    // Twinkle effect
    float twinkle = sin(time * STAR_TWINKLE_SPEED + starNoise * 100.0) * 0.3 + 0.7;

    // Star color variation (white, blue-white, yellow)
    float colorVariation = noise3D(starPos * 10.0);
    float3 starColor = lerp(float3(1.0, 0.95, 0.9), float3(0.9, 0.95, 1.0), colorVariation);

    // Larger bright stars (less frequent)
    float brightStarNoise = noise3D(starPos * 30.0);
    float brightStar = smoothstep(0.995, 1.0, brightStarNoise) * 2.0;

    return starColor * (star * twinkle + brightStar) * visibility * 0.8;
}

// ============================================================================
// Sun Rendering
// ============================================================================

float3 renderSun(float3 viewDir, float3 lightDir, float3 color, float size, float intensity)
{
    float sunDot = dot(viewDir, lightDir);

    // Core sun disk
    float sunDisk = smoothstep(1.0 - size * 0.5, 1.0 - size * 0.3, sunDot);

    // Inner corona (bright)
    float corona1 = pow(saturate((sunDot - (1.0 - size)) / size), 4.0);

    // Outer glow (stylized, larger)
    float outerGlow = pow(saturate((sunDot - (1.0 - SUN_GLOW_SIZE)) / SUN_GLOW_SIZE), 2.0);

    // Combine layers
    float3 sunCore = color * sunDisk * 2.0;
    float3 sunCorona = color * corona1 * 1.5;
    float3 sunGlow = color * outerGlow * SUN_GLOW_INTENSITY * STYLE_STRENGTH;

    return (sunCore + sunCorona + sunGlow) * intensity;
}

// ============================================================================
// Moon Rendering
// ============================================================================

float3 renderMoon(float3 viewDir, float3 moonDir, float3 color, float size, float phase)
{
    float moonDot = dot(viewDir, moonDir);

    // Moon disk
    float moonDisk = smoothstep(1.0 - size * 0.6, 1.0 - size * 0.4, moonDot);

    if (moonDisk <= 0.0) return float3(0, 0, 0);

    // Calculate moon phase shading
    // phase: 0 = new moon, 0.5 = full moon, 1.0 = new moon again
    float illumination = abs(phase - 0.5) * 2.0;  // 0 at new, 1 at full

    // Simple phase effect - darken part of the moon
    float phaseAngle = phase * 6.28318;
    float3 moonRight = normalize(cross(moonDir, float3(0, 1, 0)));
    float phaseFactor = dot(viewDir, moonRight) * 0.5 + 0.5;
    float phaseShading = lerp(1.0 - illumination, 1.0, phaseFactor);

    // Surface detail
    float3 moonCoord = viewDir * 50.0;
    float craters = fbm(moonCoord, 3);
    float surfaceDetail = 0.85 + craters * 0.15;

    // Subtle glow
    float moonGlow = pow(saturate((moonDot - (1.0 - size * 2.0)) / (size * 2.0)), 3.0) * 0.3;

    float3 moonColor = color * moonDisk * surfaceDetail * phaseShading;
    moonColor += color * moonGlow * 0.5;

    return moonColor;
}

// ============================================================================
// Atmospheric Scattering (simplified Rayleigh/Mie)
// ============================================================================

float3 calculateAtmosphere(float3 viewDir, float3 sunDir)
{
    // Sun-view angle for scattering
    float sunViewCos = dot(viewDir, sunDir);

    // Rayleigh scattering (blue sky)
    float rayleighPhase = 0.75 * (1.0 + sunViewCos * sunViewCos);
    float3 rayleigh = float3(0.3, 0.5, 1.0) * rayleighPhase * rayleighStrength;

    // Mie scattering (sun halo)
    float miePhase = (1.0 - mieG * mieG) / pow(1.0 + mieG * mieG - 2.0 * mieG * sunViewCos, 1.5);
    float3 mie = sunColor * miePhase * mieStrength;

    return rayleigh + mie;
}

// ============================================================================
// Stylized Color Grading
// ============================================================================

float3 applySkyStyleGrading(float3 color)
{
    // Saturation boost
    float luminance = dot(color, float3(0.299, 0.587, 0.114));
    color = lerp(float3(luminance, luminance, luminance), color, SKY_VIBRANCY);

    return color;
}

// ============================================================================
// Pixel Shader
// ============================================================================

float4 PSMain(VSOutput input) : SV_TARGET
{
    float3 viewDir = normalize(input.viewDir);

    // Height-based gradient (0 at horizon, 1 at zenith)
    float height = viewDir.y;
    float horizonFactor = 1.0 - abs(height);

    // Sky gradient
    float gradientT = saturate(height * 0.5 + 0.5);  // Remap -1..1 to 0..1
    float3 skyColor = lerp(horizonColor, zenithColor, pow(gradientT, 1.5));

    // Below horizon - darker, fog-colored
    if (height < 0.0)
    {
        float belowHorizon = saturate(-height * 3.0);
        skyColor = lerp(horizonColor, fogColor * 0.5, belowHorizon);
    }

    // ========================================================================
    // Atmospheric scattering
    // ========================================================================
    if (QUALITY_LEVEL >= 2)
    {
        float3 atmosphere = calculateAtmosphere(viewDir, sunDirection);
        skyColor += atmosphere * sunIntensity * 0.1;
    }

    // ========================================================================
    // Stylized horizon glow
    // ========================================================================
    float horizonGlow = pow(horizonFactor, 3.0) * HORIZON_GLOW_STRENGTH * STYLE_STRENGTH;

    // Warm tint toward sun direction at horizon
    float sunHorizonInfluence = saturate(dot(normalize(float3(viewDir.x, 0, viewDir.z)),
                                              normalize(float3(sunDirection.x, 0, sunDirection.z))));
    sunHorizonInfluence = pow(sunHorizonInfluence, 2.0);

    float3 horizonGlowColor = lerp(horizonColor, sunColor * 0.5, sunHorizonInfluence) * horizonGlow;
    skyColor += horizonGlowColor;

    // ========================================================================
    // Subtle cloud hints (stylized, not realistic clouds)
    // ========================================================================
    if (QUALITY_LEVEL >= 2 && height > 0.0)
    {
        float2 cloudUV = viewDir.xz / (height + 0.1) * 0.5;
        float cloudNoise = fbm(float3(cloudUV + time * 0.02, 0.0), 3);
        float cloudHint = smoothstep(0.45, 0.65, cloudNoise) * CLOUD_HINT_STRENGTH;
        cloudHint *= smoothstep(0.0, 0.3, height);  // Fade near horizon
        skyColor += float3(cloudHint, cloudHint, cloudHint) * 0.8;
    }

    // ========================================================================
    // Sun
    // ========================================================================
    float3 sun = renderSun(viewDir, sunDirection, sunColor, sunSize, sunIntensity);
    skyColor += sun;

    // ========================================================================
    // Moon (only when visible and sun is low)
    // ========================================================================
    if (sunIntensity < 0.5)
    {
        float moonVisibility = 1.0 - sunIntensity * 2.0;
        float3 moon = renderMoon(viewDir, moonDirection, moonColor, moonSize, moonPhase);
        skyColor += moon * moonVisibility;
    }

    // ========================================================================
    // Stars (only at night)
    // ========================================================================
    float3 stars = generateStars(viewDir, starVisibility);
    skyColor += stars;

    // ========================================================================
    // Color grading
    // ========================================================================
    skyColor = applySkyStyleGrading(skyColor);

    // Clamp to prevent over-bright areas
    skyColor = min(skyColor, float3(3.0, 3.0, 3.0));

    return float4(skyColor, 1.0);
}
