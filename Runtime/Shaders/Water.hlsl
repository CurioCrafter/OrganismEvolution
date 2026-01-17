// Water shader - Stylized ocean rendering with No Man's Sky aesthetics
// Features: multi-frequency waves, fresnel reflection, theme-matched colors, foam, caustics
// Integrates with PlanetTheme for palette cohesion

// ============================================================================
// Constant Buffer (must match WaterConstants in WaterRenderer.h)
// ============================================================================

cbuffer WaterConstants : register(b0)
{
    // View/Projection matrices
    float4x4 view;               // 0-63
    float4x4 projection;         // 64-127
    float4x4 viewProjection;     // 128-191

    // Camera and lighting
    float3 cameraPos;            // 192-203
    float time;                  // 204-207
    float3 lightDir;             // 208-219
    float _pad1;                 // 220-223
    float3 lightColor;           // 224-235
    float sunIntensity;          // 236-239

    // Water colors (from PlanetTheme)
    float4 waterColor;           // 240-255 (deep water)
    float4 shallowColor;         // 256-271 (shallow water)

    // Wave parameters
    float waveScale;             // 272-275
    float waveSpeed;             // 276-279
    float waveHeight;            // 280-283
    float transparency;          // 284-287

    // Sky colors for reflection (from PlanetTheme)
    float3 skyTopColor;          // 288-299
    float _pad2;                 // 300-303
    float3 skyHorizonColor;      // 304-315
    float fresnelPower;          // 316-319

    // Foam parameters
    float foamIntensity;         // 320-323
    float foamScale;             // 324-327
    float specularPower;         // 328-331
    float specularIntensity;     // 332-335

    // Underwater rendering
    float waterHeight;           // 336-339
    float underwaterDepth;       // 340-343
    float surfaceClarity;        // 344-347
    float _pad3;                 // 348-351
};

// ============================================================================
// Visual Style Parameters (No Man's Sky inspired)
// ============================================================================

static const float STYLE_STRENGTH = 0.7;           // 0-1: stylized vs realistic
static const float COLOR_VIBRANCY = 1.15;          // Water color saturation
static const float FOAM_BRIGHTNESS = 0.92;         // Foam whiteness
static const float SUBSURFACE_STRENGTH = 0.25;     // Light penetration glow
static const float CAUSTIC_INTENSITY = 0.35;       // Animated caustic patterns
static const float RIM_GLOW = 0.12;                // Edge glow on waves

// Quality levels (0=off, 1=low, 2=medium, 3=high)
static const int QUALITY_LEVEL = 2;

// ============================================================================
// Noise Functions
// ============================================================================

float hash2D(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

float noise2D(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash2D(i);
    float b = hash2D(i + float2(1, 0));
    float c = hash2D(i + float2(0, 1));
    float d = hash2D(i + float2(1, 1));

    return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

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

float fbm(float2 p, int octaves)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    [unroll]
    for (int i = 0; i < 4; i++)
    {
        if (i >= octaves) break;
        value += amplitude * noise2D(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }

    return value;
}

float voronoi(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    float minDist = 1.0;

    [unroll]
    for (int z = -1; z <= 1; z++)
    {
        [unroll]
        for (int y = -1; y <= 1; y++)
        {
            [unroll]
            for (int x = -1; x <= 1; x++)
            {
                float3 neighbor = float3(x, y, z);
                float3 cellPoint = hash3(i + neighbor);
                float3 diff = neighbor + cellPoint - f;
                float dist = length(diff);
                minDist = min(minDist, dist);
            }
        }
    }

    return minDist;
}

// ============================================================================
// Input/Output Structures
// ============================================================================

struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 texCoord : TEXCOORD1;
    float3 viewDir : TEXCOORD2;
};

// ============================================================================
// Wave Functions
// ============================================================================

// Gerstner-style wave calculation
float3 calculateWave(float2 uv, float freq, float amp, float speed, float2 dir)
{
    float phase = dot(uv, dir) * freq + time * speed;
    float height = sin(phase) * amp;
    float dx = cos(phase) * amp * freq * dir.x;
    float dz = cos(phase) * amp * freq * dir.y;
    return float3(dx, height, dz);
}

// Multi-frequency wave system
float3 calculateWaveOffset(float3 worldPos, out float3 normal)
{
    float2 uv = worldPos.xz;

    // Wind direction
    float windAngle = 0.7;
    float2 windDir = float2(cos(windAngle), sin(windAngle));

    float3 totalOffset = float3(0, 0, 0);
    float3 totalNormal = float3(0, 1, 0);

    // Layer 1: Large slow swells
    float2 swell1Dir = normalize(windDir + float2(0.2, 0.1));
    float3 wave1 = calculateWave(uv, 0.5 * waveScale, waveHeight * 0.4, waveSpeed * 0.08, swell1Dir);
    totalOffset.y += wave1.y;
    totalNormal.x -= wave1.x;
    totalNormal.z -= wave1.z;

    // Layer 2: Medium wind waves
    if (QUALITY_LEVEL >= 1)
    {
        float2 med1Dir = windDir;
        float3 wave2 = calculateWave(uv, 2.0 * waveScale, waveHeight * 0.2, waveSpeed * 0.2, med1Dir);
        totalOffset.y += wave2.y;
        totalNormal.x -= wave2.x * 0.5;
        totalNormal.z -= wave2.z * 0.5;
    }

    // Layer 3: Small ripples (detail)
    if (QUALITY_LEVEL >= 2)
    {
        float ripple = noise2D(uv * 8.0 + time * 0.4) - 0.5;
        totalOffset.y += ripple * waveHeight * 0.05;
    }

    normal = normalize(totalNormal);
    return totalOffset;
}

// ============================================================================
// Vertex Shader
// ============================================================================

PSInput VSMain(VSInput input)
{
    PSInput output;

    float3 worldPos = input.position;

    // Apply wave displacement
    float3 waveNormal;
    float3 waveOffset = calculateWaveOffset(worldPos, waveNormal);
    worldPos.y += waveOffset.y;

    // Transform to clip space
    output.position = mul(float4(worldPos, 1.0), viewProjection);
    output.worldPos = worldPos;
    output.texCoord = input.texCoord;
    output.viewDir = normalize(cameraPos - worldPos);

    return output;
}

// ============================================================================
// Stylized Water Functions
// ============================================================================

// Fresnel with Schlick approximation
float calculateFresnel(float3 viewDir, float3 normal, float power)
{
    float NdotV = saturate(dot(viewDir, normal));
    float F0 = 0.02;  // Water IOR
    return F0 + (1.0 - F0) * pow(1.0 - NdotV, power);
}

// Caustic pattern
float3 calculateCaustics(float3 worldPos, float depth)
{
    if (QUALITY_LEVEL < 2) return float3(0, 0, 0);

    float2 uv = worldPos.xz * 0.25;

    // Multiple voronoi layers for caustic effect
    float caustic1 = voronoi(float3(uv + time * 0.04, 0.0));
    float caustic2 = voronoi(float3(uv * 1.7 - time * 0.055, 1.0));
    float caustic3 = voronoi(float3(uv * 0.8 + float2(time * 0.03, -time * 0.02), 2.0));

    float caustic = (caustic1 + caustic2 + caustic3) / 3.0;
    caustic = pow(caustic, 0.6);

    // Stronger in shallow water
    float intensity = (1.0 - saturate(depth * 0.5)) * CAUSTIC_INTENSITY;

    return float3(caustic, caustic * 0.95, caustic * 0.85) * intensity;
}

// Foam calculation
float calculateFoam(float3 worldPos, float waveHeight_local)
{
    float2 uv = worldPos.xz;

    // Wave crest foam
    float wavePhase = sin(uv.x * waveScale * 3.14159 + uv.y * waveScale * 2.5 + time * waveSpeed);
    float crestFoam = smoothstep(0.6, 1.0, wavePhase) * 0.5;

    // Noise-based foam detail
    float foamNoise = fbm(uv * foamScale + time * 0.3, 3);
    float foam = crestFoam * foamNoise;

    // Additional white caps
    float whiteCap = smoothstep(0.7, 0.9, foamNoise) * 0.3;

    return saturate(foam + whiteCap) * foamIntensity;
}

// Subsurface scattering approximation
float3 calculateSubsurface(float3 normal, float3 viewDir, float3 lightDirection, float3 baseColor)
{
    float NdotL = dot(normal, lightDirection);
    float wrapDiff = saturate((NdotL + 0.5) / 1.5);

    // Back-lighting glow
    float backScatter = pow(saturate(dot(viewDir, -lightDirection)), 3.0);

    return baseColor * (wrapDiff * 0.2 + backScatter * 0.15) * SUBSURFACE_STRENGTH;
}

// Color vibrancy adjustment
float3 adjustVibrancy(float3 color, float amount)
{
    float luminance = dot(color, float3(0.299, 0.587, 0.114));
    return lerp(float3(luminance, luminance, luminance), color, amount);
}

// ============================================================================
// Pixel Shader
// ============================================================================

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 worldPos = input.worldPos;
    float3 viewDir = normalize(input.viewDir);

    // Recalculate wave normal for better quality
    float3 waveNormal;
    calculateWaveOffset(worldPos, waveNormal);

    // Check if underwater
    bool cameraUnderwater = underwaterDepth > 0.0;

    // Flip normal for underwater viewing
    float3 effectiveNormal = cameraUnderwater ? -waveNormal : waveNormal;

    // Fresnel effect
    float fresnel = calculateFresnel(viewDir, effectiveNormal, fresnelPower);

    float3 finalColor;
    float alpha;

    if (cameraUnderwater)
    {
        // ====================================================================
        // UNDERWATER VIEW
        // ====================================================================

        // Snell's window effect
        float criticalAngleCos = 0.66;
        float viewDotUp = dot(viewDir, float3(0, 1, 0));
        float tirFactor = saturate((criticalAngleCos - abs(viewDotUp)) / 0.2);
        float snellWindow = 1.0 - tirFactor;

        // Surface color from below
        float3 underwaterSurfaceColor = waterColor.rgb * 0.3;

        // Sky through Snell's window
        float3 refractedSky = lerp(skyHorizonColor, skyTopColor, saturate(viewDotUp * 2.0));
        refractedSky *= 0.7;

        finalColor = lerp(underwaterSurfaceColor, refractedSky, snellWindow * surfaceClarity);

        // Caustic shimmer
        float2 causticUV = worldPos.xz * waveScale * 2.0 + time * 0.3;
        float causticShimmer = sin(causticUV.x * 6.28) * sin(causticUV.y * 6.28);
        causticShimmer = pow(causticShimmer * 0.5 + 0.5, 3.0);
        finalColor += causticShimmer * 0.1 * snellWindow;

        alpha = lerp(0.85, 0.95, snellWindow);
    }
    else
    {
        // ====================================================================
        // ABOVE WATER VIEW
        // ====================================================================

        // Depth-based color (using distance from surface)
        float viewDepth = saturate(length(worldPos.xz - cameraPos.xz) * 0.002);
        float3 baseWater = lerp(shallowColor.rgb, waterColor.rgb, viewDepth);

        // Sky reflection
        float skyMix = saturate(waveNormal.y * 0.5 + 0.5);
        float3 skyReflection = lerp(skyHorizonColor, skyTopColor, skyMix);

        // Blend water with reflection based on fresnel
        float3 waterBase = lerp(baseWater, skyReflection, fresnel * 0.6);

        // Sun specular
        float3 halfVec = normalize(lightDir + viewDir);
        float spec = pow(saturate(dot(waveNormal, halfVec)), specularPower);
        float3 specular = lightColor * spec * specularIntensity * sunIntensity;

        // Caustics
        float3 caustics = calculateCaustics(worldPos, viewDepth);

        // Foam
        float foam = calculateFoam(worldPos, waveNormal.y);
        float3 foamColor = float3(FOAM_BRIGHTNESS, FOAM_BRIGHTNESS, FOAM_BRIGHTNESS);

        // Subsurface scattering
        float3 subsurface = calculateSubsurface(waveNormal, viewDir, lightDir, shallowColor.rgb);

        // ================================================================
        // STYLIZED EFFECTS
        // ================================================================

        // Rim glow on wave crests
        float rimFactor = 1.0 - saturate(dot(viewDir, waveNormal));
        float rimGlow = pow(rimFactor, 3.0) * RIM_GLOW * STYLE_STRENGTH;
        float3 rimColor = lerp(skyHorizonColor, shallowColor.rgb * 1.5, 0.5) * rimGlow;

        // Combine all layers
        finalColor = waterBase;
        finalColor += caustics * (1.0 - fresnel * 0.5);
        finalColor += specular;
        finalColor += subsurface * sunIntensity;
        finalColor += rimColor;
        finalColor = lerp(finalColor, foamColor, foam);

        // Apply vibrancy
        finalColor = adjustVibrancy(finalColor, COLOR_VIBRANCY);

        // Alpha with fresnel influence
        alpha = transparency;
        alpha = lerp(alpha, 1.0, fresnel * 0.3 + foam * 0.5);
    }

    // ========================================================================
    // Atmospheric perspective (distant water fades to fog)
    // ========================================================================
    float viewDist = length(worldPos - cameraPos);
    float haze = 1.0 - exp(-viewDist * 0.001);
    float3 hazeColor = lerp(skyHorizonColor, skyTopColor, 0.3);
    finalColor = lerp(finalColor, hazeColor, haze * 0.4);

    return float4(finalColor, alpha);
}
