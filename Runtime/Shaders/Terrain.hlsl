// Terrain shader - Ported from OrganismEvolution GLSL
// Procedural terrain with biomes, water animation, and lighting

// ============================================================================
// Constant Buffers
// ============================================================================

cbuffer Constants : register(b0)
{
    // Per-frame constants (must match C++ Constants struct exactly)
    // HLSL packing: float4x4 MUST be 16-byte aligned
    float4x4 view;             // Offset 0, 64 bytes
    float4x4 projection;       // Offset 64, 64 bytes
    float4x4 viewProjection;   // Offset 128, 64 bytes
    float3 viewPos;            // Offset 192, 12 bytes
    float time;                // Offset 204, 4 bytes (packs after viewPos)
    float3 lightPos;           // Offset 208, 12 bytes
    float padding1;            // Offset 220, 4 bytes (packs after lightPos)
    float3 lightColor;         // Offset 224, 12 bytes
    float padding2;            // Offset 236, 4 bytes (packs after lightColor)
    // Day/Night cycle data (must match C++ Constants struct order)
    float dayTime;             // Offset 240, 4 bytes - 0-1 time of day
    float3 skyTopColor;        // Offset 244, 12 bytes - Sky color at zenith (packs after dayTime)
    float3 skyHorizonColor;    // Offset 256, 12 bytes - Sky color at horizon
    float sunIntensity;        // Offset 268, 4 bytes - Light intensity multiplier (packs after skyHorizonColor)
    float3 ambientColor;       // Offset 272, 12 bytes - Ambient light color
    float starVisibility;      // Offset 284, 4 bytes - 0-1 star visibility (packs after ambientColor)
    float2 dayNightPadding;    // Offset 288, 8 bytes
    float2 modelAlignPadding;  // Offset 296, 8 bytes - CRITICAL: align model to 16-byte boundary
    // Per-object constants (model starts at offset 304, which is 16-byte aligned)
    float4x4 model;            // Offset 304, 64 bytes (304 % 16 == 0, correctly aligned)
    float3 objectColor;        // Offset 368, 12 bytes
    int creatureID;            // Offset 380, 4 bytes (packs after objectColor)
    int creatureType;          // Offset 384, 4 bytes
    float3 endPadding;         // Offset 388, 12 bytes (pad to 400)
};

// ============================================================================
// Input/Output Structures
// ============================================================================

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 color : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 fragPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 color : TEXCOORD2;
    float3 worldPos : TEXCOORD3;
    float height : TEXCOORD4;
};

// ============================================================================
// Procedural Noise Functions
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
    for (int i = 0; i < 5; i++)
    {
        if (i >= octaves) break;
        value += amplitude * noise3D(p * frequency);
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
// Star Generation Function
// ============================================================================

float3 AddStars(float3 viewDir, float visibility)
{
    if (visibility <= 0.0) return float3(0, 0, 0);

    // Generate procedural star field
    float3 starPos = viewDir * 1000.0;
    float starNoise = noise3D(starPos * 50.0);
    float star = step(0.98, starNoise);  // Only brightest points become stars

    // Twinkle effect
    float twinkle = sin(time * 3.0 + starNoise * 100.0) * 0.5 + 0.5;

    return float3(star * twinkle * 0.8 * visibility, star * twinkle * 0.8 * visibility, star * twinkle * 0.8 * visibility);
}

// ============================================================================
// PBR Lighting Functions
// ============================================================================

static const float PI = 3.14159265359;

// GGX/Trowbridge-Reitz Normal Distribution Function
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

// Schlick-GGX Geometry Function (single direction)
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

// Smith's Geometry Function (combines view and light directions)
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Fresnel-Schlick Approximation for PBR
float3 fresnelSchlickPBR(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

// Fresnel-Schlick with roughness for IBL
float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

// Cook-Torrance BRDF
float3 calculatePBR(float3 N, float3 V, float3 L, float3 albedo, float metallic, float roughness, float3 lightCol)
{
    float3 H = normalize(V + L);

    // Calculate reflectance at normal incidence
    // Dielectrics use 0.04, metals use albedo color
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    // Cook-Torrance BRDF components
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = fresnelSchlickPBR(max(dot(H, V), 0.0), F0);

    // Specular reflection
    float3 kS = F;
    // Diffuse reflection (energy conservation)
    float3 kD = (1.0 - kS) * (1.0 - metallic);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;

    // Combine diffuse and specular
    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * lightCol * NdotL;
}

// Hemisphere ambient with ground contribution (IBL approximation)
float3 calculateAmbientIBL(float3 N, float3 V, float3 albedo, float metallic, float roughness)
{
    // Sky color (from above) - use dynamic sky color from constant buffer
    // Ensure minimum ambient light to prevent terrain from going completely dark
    float3 skyCol = max(skyTopColor, float3(0.15, 0.15, 0.18));
    // Ground color (from below) - darker version of horizon color with minimum floor
    float3 groundCol = max(skyHorizonColor * 0.4, float3(0.08, 0.08, 0.1));

    // Hemisphere blend based on normal direction
    float skyFactor = N.y * 0.5 + 0.5;
    float3 ambientLight = lerp(groundCol, skyCol, skyFactor);

    // Calculate F0 for Fresnel
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    // Fresnel for ambient
    float3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - metallic);

    // Diffuse ambient
    float3 diffuseAmbient = kD * albedo * ambientLight;

    // Simplified specular ambient (approximation without cubemap)
    float3 specularAmbient = F * ambientLight * (1.0 - roughness) * 0.3;

    return diffuseAmbient + specularAmbient;
}

// ============================================================================
// Realistic Water System
// ============================================================================

// Water configuration constants
// C++ terrain: HEIGHT_SCALE=30, WATER_LEVEL=0.0, heights range 0-30
// Normalization: heightNormalized = height / 30.0 (0 to 1 range)
// Water threshold: 0.0 world height = 0.0 normalized
// COMPREHENSIVE OVERHAUL: Further reduced to 0.012 for minimal water coverage
static const float WATER_LEVEL = 0.012;          // Normalized height threshold (~0.36 world units)
static const float3 DEEP_WATER_COLOR = float3(0.015, 0.06, 0.12);    // Darker deep water
static const float3 SHALLOW_WATER_COLOR = float3(0.08, 0.35, 0.45);  // More turquoise shallow
static const float3 MID_WATER_COLOR = float3(0.04, 0.15, 0.25);      // Mid-depth transition
static const float3 SKY_COLOR = float3(0.5, 0.7, 0.9);
static const float3 SUN_COLOR = float3(1.0, 0.95, 0.8);
static const float3 FOAM_COLOR = float3(0.92, 0.96, 1.0);
static const float3 WET_SAND_COLOR = float3(0.55, 0.48, 0.35);       // Darker wet sand near water

// Multi-frequency wave system for realistic ocean motion
// Uses Gerstner-inspired wave patterns with multiple octaves
struct WaveParams {
    float frequency;
    float amplitude;
    float speed;
    float2 direction;
};

// Calculate wave height contribution from a single wave
float calculateWaveHeight(float2 pos, float freq, float amp, float speed, float2 dir)
{
    float phase = dot(pos, dir) * freq + time * speed;
    return amp * sin(phase);
}

// Calculate wave normal contribution from a single wave
float3 calculateWaveNormalContrib(float2 pos, float freq, float amp, float speed, float2 dir)
{
    float phase = dot(pos, dir) * freq + time * speed;
    float derivative = amp * freq * cos(phase);
    return float3(-dir.x * derivative, 0.0, -dir.y * derivative);
}

// Calculate scrolling wave normals using multiple octaves
// COMPREHENSIVE OVERHAUL: Multi-frequency wave system for realistic ocean
float3 calculateWaterNormal(float3 worldPos)
{
    float2 uv = worldPos.xz;

    // Wind direction affects wave propagation (simulate light breeze)
    float windAngle = 0.7;  // ~40 degrees
    float2 windDir = float2(cos(windAngle), sin(windAngle));

    // Wave Layer 1: Large slow swells (ocean swell from distant storms)
    float2 swell1Dir = normalize(windDir + float2(0.2, 0.1));
    float2 swell2Dir = normalize(windDir + float2(-0.3, 0.2));
    float swellFreq = 0.5;
    float swellSpeed = 0.08;
    float swellAmp = 0.12;

    float3 swellNormal1 = calculateWaveNormalContrib(uv, swellFreq, swellAmp, swellSpeed, swell1Dir);
    float3 swellNormal2 = calculateWaveNormalContrib(uv, swellFreq * 0.7, swellAmp * 0.8, swellSpeed * 0.9, swell2Dir);

    // Wave Layer 2: Medium waves (wind-driven waves)
    float2 medDir1 = normalize(windDir);
    float2 medDir2 = normalize(windDir + float2(0.5, -0.3));
    float medFreq = 2.0;
    float medSpeed = 0.2;
    float medAmp = 0.06;

    float3 medNormal1 = calculateWaveNormalContrib(uv, medFreq, medAmp, medSpeed, medDir1);
    float3 medNormal2 = calculateWaveNormalContrib(uv, medFreq * 1.3, medAmp * 0.7, medSpeed * 1.1, medDir2);

    // Wave Layer 3: Small ripples (capillary waves / surface detail)
    float2 rippleUV1 = uv * 8.0 + float2(time * 0.4, -time * 0.35);
    float2 rippleUV2 = uv * 12.0 - float2(time * 0.5, time * 0.45);
    float ripple1 = noise3D(float3(rippleUV1, 0.0));
    float ripple2 = noise3D(float3(rippleUV2, 1.0));
    float rippleHeight = (ripple1 + ripple2) * 0.5 - 0.5;

    // Calculate ripple normal from derivatives
    float eps = 0.05;
    float rippleX = noise3D(float3(rippleUV1 + float2(eps, 0), 0.0)) + noise3D(float3(rippleUV2 + float2(eps, 0), 1.0));
    float rippleZ = noise3D(float3(rippleUV1 + float2(0, eps), 0.0)) + noise3D(float3(rippleUV2 + float2(0, eps), 1.0));
    float3 rippleNormal = float3(-(rippleX - (ripple1 + ripple2)) * 0.3, 0.0, -(rippleZ - (ripple1 + ripple2)) * 0.3);

    // Wave Layer 4: Very fine detail (micro-ripples for sparkle)
    float2 microUV = uv * 25.0 + time * 0.8;
    float microNoise = noise3D(float3(microUV, time * 0.1));
    float3 microNormal = float3((microNoise - 0.5) * 0.02, 0.0, (noise3D(float3(microUV.yx, time * 0.15)) - 0.5) * 0.02);

    // Combine all wave normal contributions
    float3 combinedNormal = float3(0.0, 1.0, 0.0);
    combinedNormal.xz += swellNormal1.xz * 0.4 + swellNormal2.xz * 0.3;  // Large swells
    combinedNormal.xz += medNormal1.xz * 0.35 + medNormal2.xz * 0.25;    // Medium waves
    combinedNormal.xz += rippleNormal.xz * 0.15;                          // Small ripples
    combinedNormal.xz += microNormal.xz;                                  // Micro detail

    return normalize(combinedNormal);
}

// Calculate wave vertex displacement for vertex shader
float calculateWaveDisplacement(float3 worldPos)
{
    float2 uv = worldPos.xz;

    // Wind direction
    float windAngle = 0.7;
    float2 windDir = float2(cos(windAngle), sin(windAngle));

    float displacement = 0.0;

    // Large slow swells
    float2 swell1Dir = normalize(windDir + float2(0.2, 0.1));
    displacement += calculateWaveHeight(uv, 0.5, 0.003, 0.08, swell1Dir);
    displacement += calculateWaveHeight(uv, 0.35, 0.0025, 0.07, normalize(windDir + float2(-0.3, 0.2)));

    // Medium waves
    displacement += calculateWaveHeight(uv, 2.0, 0.0015, 0.2, windDir);
    displacement += calculateWaveHeight(uv, 2.6, 0.001, 0.22, normalize(windDir + float2(0.5, -0.3)));

    // Small ripples (very subtle)
    float ripple = noise3D(float3(uv * 8.0 + time * 0.3, 0.0));
    displacement += (ripple - 0.5) * 0.0005;

    return displacement;
}

// Calculate Fresnel effect - more reflective at grazing angles
float calculateFresnel(float3 viewDir, float3 normal, float power)
{
    float NdotV = saturate(dot(viewDir, normal));
    return pow(1.0 - NdotV, power);
}

// Calculate water depth based on height - IMPROVED for better depth perception
float calculateWaterDepth(float heightNormalized)
{
    // Map from water surface down to deep water
    float waterSurfaceHeight = WATER_LEVEL;
    float maxDepth = 0.15; // Reduced for more dramatic depth gradient in shallower water

    float depthBelow = waterSurfaceHeight - heightNormalized;

    // Use exponential falloff for more realistic depth perception
    float linearDepth = saturate(depthBelow / maxDepth);
    return 1.0 - exp(-linearDepth * 3.0);  // Exponential curve for natural light absorption
}

// Calculate shoreline foam - IMPROVED with wave wash and receding foam trails
float calculateFoam(float3 worldPos, float heightNormalized, float depth)
{
    // Distance from water edge
    float shoreDistance = (WATER_LEVEL - heightNormalized) / WATER_LEVEL;

    // Foam appears near the shoreline (shallow water)
    float shorelineFoam = smoothstep(0.8, 0.0, shoreDistance);
    shorelineFoam *= shorelineFoam;  // Squared for sharper falloff

    // Animated foam pattern - multiple scales
    float2 foamUV = worldPos.xz;

    // Large foam patches
    float foamNoise1 = noise3D(float3(foamUV * 0.3 + time * 0.08, time * 0.15));

    // Medium foam detail
    float foamNoise2 = noise3D(float3(foamUV * 1.5 - time * 0.12, time * 0.25));

    // Small foam bubbles
    float foamNoise3 = noise3D(float3(foamUV * 5.0 + time * 0.3, time * 0.4));

    float foamPattern = foamNoise1 * 0.4 + foamNoise2 * 0.35 + foamNoise3 * 0.25;

    // Wave crest foam (from height variation) - white caps
    float2 waveUV = worldPos.xz * 0.15;
    float waveCrest = fbm(float3(waveUV + time * 0.025, 0.0), 2);
    float crestFoam = smoothstep(0.58, 0.72, waveCrest) * (1.0 - depth * 0.7);

    // Breaking wave foam - periodic wash onto shore
    float wavePhase = sin(worldPos.x * 0.1 + time * 0.3) * 0.5 + 0.5;
    float breakingFoam = smoothstep(0.6, 1.0, shorelineFoam) * wavePhase;

    // Receding foam trails
    float recedingWave = sin(worldPos.z * 0.2 - time * 0.4 + worldPos.x * 0.1);
    float recedingFoam = smoothstep(0.3, 0.8, shorelineFoam) * smoothstep(-0.2, 0.5, recedingWave) * 0.4;

    // Combine foam sources
    float totalFoam = shorelineFoam * foamPattern * 0.8 + crestFoam * 0.6 + breakingFoam * 0.5 + recedingFoam;
    return saturate(totalFoam);
}

// Calculate sun specular highlights on water
float3 calculateWaterSpecular(float3 viewDir, float3 lightDir, float3 waterNormal, float3 sunColor)
{
    // Primary sun reflection
    float3 halfVec = normalize(lightDir + viewDir);
    float NdotH = saturate(dot(waterNormal, halfVec));
    float primarySpec = pow(NdotH, 256.0); // Sharp sun reflection

    // Secondary sparkle from wave normal perturbation
    float sparkleSpec = pow(NdotH, 64.0) * 0.3;

    // Sun disk reflection (Blinn-Phong with high power)
    float3 reflectDir = reflect(-lightDir, waterNormal);
    float RdotV = saturate(dot(reflectDir, viewDir));
    float sunDisk = pow(RdotV, 512.0) * 2.0;

    return sunColor * (primarySpec + sparkleSpec + sunDisk);
}

// Main realistic water color function - COMPREHENSIVE OVERHAUL
float3 generateRealisticWaterColor(float3 worldPos, float3 baseNormal, float3 viewDir,
                                    float3 lightDir, float heightNormalized)
{
    // Calculate water surface normal with multi-frequency wave animation
    float3 waterNormal = calculateWaterNormal(worldPos);

    // Blend base terrain normal with water normal (less blend for more wave influence)
    waterNormal = normalize(lerp(baseNormal, waterNormal, 0.85));

    // Calculate water depth with improved exponential falloff
    float depth = calculateWaterDepth(heightNormalized);

    // === FRESNEL EFFECT (Critical for realism) ===
    // More reflective at grazing angles, more transparent looking straight down
    float baseFresnel = calculateFresnel(viewDir, waterNormal, 4.0);

    // Add subtle variation from wave perturbation
    float waveVariation = noise3D(float3(worldPos.xz * 0.15 + time * 0.03, 0.0)) * 0.08;
    float fresnel = saturate(baseFresnel + waveVariation);

    // Schlick's approximation with water IOR (~1.33)
    float F0 = 0.02;  // Water's reflectance at normal incidence
    float schlickFresnel = F0 + (1.0 - F0) * pow(1.0 - saturate(dot(viewDir, waterNormal)), 5.0);
    fresnel = lerp(fresnel, schlickFresnel, 0.5);  // Blend both approaches

    // === DEPTH-BASED WATER COLORING ===
    // Three-color gradient: shallow -> mid -> deep
    float3 refractionColor;
    if (depth < 0.3)
    {
        // Shallow water - turquoise/cyan
        refractionColor = lerp(SHALLOW_WATER_COLOR, MID_WATER_COLOR, depth / 0.3);
    }
    else
    {
        // Deep water - dark blue-green
        refractionColor = lerp(MID_WATER_COLOR, DEEP_WATER_COLOR, saturate((depth - 0.3) / 0.7));
    }

    // Add color variation based on position (natural water isn't uniform)
    float colorVariation = noise3D(worldPos * 0.05) * 0.15;
    refractionColor *= (1.0 + colorVariation);

    // === CAUSTICS (light patterns on seafloor) ===
    float2 causticUV = worldPos.xz * 0.25;
    float caustic1 = voronoi(float3(causticUV + time * 0.04, 0.0));
    float caustic2 = voronoi(float3(causticUV * 1.7 - time * 0.055, 1.0));
    float caustic3 = voronoi(float3(causticUV * 0.8 + float2(time * 0.03, -time * 0.02), 2.0));
    float caustics = (caustic1 + caustic2 + caustic3) / 3.0;
    caustics = pow(caustics, 0.6);

    // Caustics stronger in shallow water, animated intensity
    float causticIntensity = (1.0 - depth) * 0.35 * (0.8 + 0.2 * sin(time * 0.5));
    float3 causticColor = float3(caustics, caustics * 0.95, caustics * 0.85) * causticIntensity;
    refractionColor += causticColor;

    // === SKY REFLECTION ===
    float3 reflectedView = reflect(-viewDir, waterNormal);
    float skyGradient = saturate(reflectedView.y * 0.5 + 0.5);

    // Use dynamic sky colors if available, otherwise defaults
    float3 horizonReflect = lerp(float3(0.55, 0.65, 0.75), skyHorizonColor, 0.7);
    float3 zenithReflect = lerp(float3(0.4, 0.55, 0.75), skyTopColor, 0.7);
    float3 reflectionColor = lerp(horizonReflect, zenithReflect, skyGradient);

    // Add subtle clouds to reflection
    float cloudNoise = fbm(float3(reflectedView.xz * 2.0 + time * 0.01, 0.0), 3);
    float clouds = smoothstep(0.4, 0.7, cloudNoise) * 0.15;
    reflectionColor += float3(clouds, clouds, clouds);

    // === BLEND REFRACTION AND REFLECTION ===
    float3 waterColor = lerp(refractionColor, reflectionColor, fresnel);

    // === SPECULAR SUN HIGHLIGHTS ===
    float3 specular = calculateWaterSpecular(viewDir, lightDir, waterNormal, SUN_COLOR);

    // Scale specular by sun intensity
    specular *= sunIntensity;
    waterColor += specular;

    // === FOAM ===
    float foam = calculateFoam(worldPos, heightNormalized, depth);

    // Foam has slight color variation
    float3 foamColor = FOAM_COLOR;
    foamColor.rg -= noise3D(worldPos * 5.0) * 0.05;  // Slight variation
    waterColor = lerp(waterColor, foamColor, foam * 0.85);

    // === SUBSURFACE SCATTERING ===
    // Light passing through shallow water creates glow
    float NdotL = saturate(dot(waterNormal, lightDir));
    float3 subsurfaceColor = SHALLOW_WATER_COLOR * 1.2;

    // Wrap lighting for subsurface effect
    float wrapLighting = saturate((NdotL + 0.3) / 1.3);
    float3 subsurface = subsurfaceColor * wrapLighting * (1.0 - depth) * 0.25;
    waterColor += subsurface;

    // === DEPTH ABSORPTION ===
    // Deep water absorbs more light (darker)
    float absorption = exp(-depth * 2.5);
    waterColor *= lerp(0.4, 1.0, absorption);

    // === ATMOSPHERIC PERSPECTIVE ===
    // Distant water appears hazier
    float viewDist = length(worldPos - viewPos);
    float haze = 1.0 - exp(-viewDist * 0.002);
    waterColor = lerp(waterColor, reflectionColor * 0.8, haze * 0.3);

    return waterColor;
}

// Legacy wrapper for compatibility
float3 generateWaterColor(float3 pos, float3 normal)
{
    // Simplified version when view/light info not available
    float3 defaultViewDir = float3(0.0, 1.0, 0.0);
    float3 defaultLightDir = normalize(float3(0.5, 0.8, 0.3));
    return generateRealisticWaterColor(pos, normal, defaultViewDir, defaultLightDir, 0.2);
}

// ============================================================================
// Visual Style Parameters (No Man's Sky inspired)
// ============================================================================

// Global style controls with safe defaults
static const float TERRAIN_STYLE_STRENGTH = 0.65;     // 0-1: blend between realistic and stylized
static const float BIOME_TINT_STRENGTH = 0.4;         // How strongly biome colors affect terrain
static const float SLOPE_VARIATION_STRENGTH = 0.35;   // Rock/cliff exposure based on slope
static const float HEIGHT_GRADIENT_STRENGTH = 0.25;   // Vertical color gradient
static const float TERRAIN_COLOR_VIBRANCY = 1.12;     // Overall saturation boost

// Biome style palettes (No Man's Sky inspired)
// Desert biome - warm oranges and reds
static const float3 DESERT_PRIMARY = float3(0.88, 0.72, 0.52);
static const float3 DESERT_SECONDARY = float3(0.95, 0.58, 0.38);
static const float3 DESERT_ACCENT = float3(0.75, 0.48, 0.35);

// Lush/Forest biome - vibrant greens
static const float3 LUSH_PRIMARY = float3(0.32, 0.68, 0.35);
static const float3 LUSH_SECONDARY = float3(0.22, 0.55, 0.28);
static const float3 LUSH_ACCENT = float3(0.45, 0.72, 0.38);

// Ice/Tundra biome - cool blues and whites
static const float3 ICE_PRIMARY = float3(0.85, 0.92, 0.98);
static const float3 ICE_SECONDARY = float3(0.72, 0.85, 0.95);
static const float3 ICE_ACCENT = float3(0.55, 0.75, 0.88);

// Alien/Exotic biome - purples and teals
static const float3 ALIEN_PRIMARY = float3(0.65, 0.48, 0.78);
static const float3 ALIEN_SECONDARY = float3(0.35, 0.72, 0.68);
static const float3 ALIEN_ACCENT = float3(0.82, 0.55, 0.88);

// Volcanic biome - dark with red accents
static const float3 VOLCANIC_PRIMARY = float3(0.25, 0.22, 0.25);
static const float3 VOLCANIC_SECONDARY = float3(0.85, 0.32, 0.15);
static const float3 VOLCANIC_ACCENT = float3(0.45, 0.35, 0.32);

// ============================================================================
// Stylized Terrain Functions
// ============================================================================

// Apply slope-based rock exposure
float3 applySlopeVariation(float3 baseColor, float3 rockColor, float3 normal, float3 worldPos)
{
    // Slope factor: 1 = flat, 0 = vertical cliff
    float flatness = abs(normal.y);
    float slopeFactor = 1.0 - pow(flatness, 2.0);

    // Add noise to make the transition more natural
    float noiseVariation = fbm(worldPos * 3.0, 2) * 0.3;
    slopeFactor = saturate(slopeFactor + noiseVariation - 0.15);

    // Rock exposure increases on steep slopes
    return lerp(baseColor, rockColor, slopeFactor * SLOPE_VARIATION_STRENGTH);
}

// Height-based color gradient
float3 applyTerrainHeightGradient(float3 baseColor, float heightNormalized, float3 lowColor, float3 highColor)
{
    // Smooth height blend
    float heightFactor = smoothstep(0.0, 1.0, heightNormalized);

    // Subtle color shift based on elevation
    float3 heightTint = lerp(lowColor, highColor, heightFactor);

    return baseColor * lerp(float3(1, 1, 1), heightTint, HEIGHT_GRADIENT_STRENGTH);
}

// Biome color tinting based on height/position
float3 applyBiomeTint(float3 baseColor, float heightNormalized, float3 worldPos)
{
    // Determine biome influence based on height and position
    float desertInfluence = smoothstep(0.05, 0.25, heightNormalized) * (1.0 - smoothstep(0.25, 0.45, heightNormalized));
    float lushInfluence = smoothstep(0.15, 0.35, heightNormalized) * (1.0 - smoothstep(0.55, 0.75, heightNormalized));
    float iceInfluence = smoothstep(0.75, 0.95, heightNormalized);

    // Position-based variation for biome patches
    float biomeNoise = fbm(worldPos * 0.02, 3);
    float alienPatch = smoothstep(0.65, 0.85, biomeNoise) * 0.3;

    // Combine biome tints
    float3 biomeTint = float3(1, 1, 1);

    // Low elevation: warmer tones
    biomeTint = lerp(biomeTint, float3(1.08, 1.02, 0.95), desertInfluence * 0.2);

    // Mid elevation: lush greens
    biomeTint = lerp(biomeTint, float3(0.95, 1.08, 0.95), lushInfluence * 0.15);

    // High elevation: cool blues
    biomeTint = lerp(biomeTint, float3(0.95, 0.98, 1.08), iceInfluence * 0.25);

    // Alien patches - subtle purple tint
    biomeTint = lerp(biomeTint, float3(1.02, 0.95, 1.08), alienPatch);

    return baseColor * lerp(float3(1, 1, 1), biomeTint, BIOME_TINT_STRENGTH);
}

// Stylized color saturation boost for terrain
float3 applyTerrainVibrancy(float3 color)
{
    float luminance = dot(color, float3(0.299, 0.587, 0.114));
    return lerp(float3(luminance, luminance, luminance), color, TERRAIN_COLOR_VIBRANCY);
}

// ============================================================================
// Terrain Material Generation
// ============================================================================

float3 generateSandColor(float3 pos)
{
    float sandNoise = fbm(pos * 15.0, 3);
    // Enhanced with stylized warm tones
    float3 baseSand = lerp(float3(0.85, 0.75, 0.55), DESERT_PRIMARY, 0.2);
    float3 darkSand = lerp(float3(0.75, 0.65, 0.45), DESERT_ACCENT, 0.15);
    return lerp(darkSand, baseSand, sandNoise);
}

float3 generateGrassColor(float3 pos, float3 normal)
{
    // Base grass color palette
    float3 lightGrass = float3(0.38, 0.68, 0.28);      // Bright healthy grass
    float3 darkGrass = float3(0.18, 0.48, 0.18);       // Shadowed grass
    float3 yellowGrass = float3(0.52, 0.55, 0.22);     // Dried/golden grass patches
    float3 lushGrass = float3(0.22, 0.58, 0.22);       // Dense lawn-like grass
    float3 meadowFlower = float3(0.55, 0.55, 0.35);    // Wildflower hint

    // === WIND ANIMATION ===
    // Create animated wind waves across the grass
    float windSpeed = 1.2;
    float windScale = 0.08;
    float2 windDir = float2(1.0, 0.4);

    // Primary wind wave
    float windPhase = time * windSpeed;
    float windWave = sin(windPhase + dot(pos.xz, windDir) * windScale);

    // Secondary wave for complexity
    float windWave2 = sin(windPhase * 0.7 + dot(pos.xz, float2(-0.3, 1.0)) * windScale * 1.5) * 0.5;

    // Combined wind effect (0 to 1 range)
    float windEffect = (windWave + windWave2 + 1.5) / 3.0;

    // === BASE COLOR VARIATION ===
    float grassNoise = fbm(pos * 8.0, 3);
    float grassDetail = noise3D(pos * 40.0) * 0.15;

    float3 grassColor = lerp(darkGrass, lightGrass, grassNoise);

    // === GRASS BLADE PATTERNS ===
    // High frequency noise creates blade-like appearance
    // Animate blade pattern slightly with wind
    float3 animatedPos = pos + float3(windEffect * 0.05, 0, windEffect * 0.03);
    float bladeNoise = noise3D(animatedPos * 100.0);
    float bladePattern = smoothstep(0.35, 0.65, bladeNoise);

    // Blade direction variation (simulates blades pointing different ways)
    float bladeDirection = noise3D(pos * 50.0 + float3(time * 0.1, 0, 0));
    float bladeTilt = smoothstep(0.4, 0.6, bladeDirection);

    // === GRASS CLUMPS AND TUFTS ===
    float clumpNoise = fbm(pos * 3.5, 2);
    float clumpPattern = smoothstep(0.35, 0.65, clumpNoise);

    // Taller grass in clumps appears darker
    float3 tallGrass = float3(0.14, 0.42, 0.14);
    grassColor = lerp(grassColor, tallGrass, clumpPattern * 0.45);

    // === DRIED GRASS PATCHES ===
    float dryPatch = fbm(pos * 1.8, 2);
    float dryAmount = smoothstep(0.62, 0.88, dryPatch);
    grassColor = lerp(grassColor, yellowGrass, dryAmount * 0.35);

    // === LUSH AREAS (near hypothetical water sources) ===
    float lushArea = fbm(pos * 0.8, 2);
    grassColor = lerp(grassColor, lushGrass, smoothstep(0.7, 0.9, lushArea) * 0.25);

    // === WILDFLOWER HINTS ===
    float flowerNoise = noise3D(pos * 25.0);
    float hasFlowers = step(0.92, flowerNoise);
    float3 flowerTint = meadowFlower * hasFlowers * 0.15;

    // === BLADE TIP HIGHLIGHTS ===
    // Light catching grass blade tips - animated with wind
    float tipHighlight = bladePattern * (1.0 - clumpPattern * 0.4);
    tipHighlight *= (0.8 + windEffect * 0.4);  // Brighter when wind exposes tips

    // Highlight color varies - more yellow in dry areas
    float3 highlightColor = lerp(float3(0.06, 0.12, 0.03), float3(0.1, 0.1, 0.02), dryAmount);
    grassColor += highlightColor * tipHighlight;

    // === AMBIENT OCCLUSION IN GRASS ===
    // Simulate shadows at base of grass clumps
    float ao = saturate(normal.y);
    float clumpAO = 1.0 - clumpPattern * 0.15;
    grassColor *= 0.75 + ao * 0.25 * clumpAO;

    // === WIND SHADING ===
    // Grass facing wind appears slightly different
    float windShading = (windEffect - 0.5) * 0.08;
    grassColor += float3(windShading, windShading * 1.5, windShading * 0.5);

    // === POSITION-BASED VARIATION ===
    // Natural color variation across the landscape
    grassColor.r += sin(pos.x * 4.5 + pos.z * 2.8) * 0.018;
    grassColor.g += cos(pos.x * 2.8 - pos.z * 3.5) * 0.025;
    grassColor.b += sin(pos.x * 3.2 + pos.z * 4.1) * 0.01;

    // Add flower tint
    grassColor += flowerTint;

    // Final detail layer
    grassColor *= (1.0 + grassDetail);

    // Ensure color stays in reasonable range
    return saturate(grassColor);
}

// Forest color with tree canopy patterns and shadows
float3 generateForestColor(float3 pos, float3 normal)
{
    // Base forest floor color (darker grass with leaf litter)
    float3 forestFloor = float3(0.15, 0.35, 0.12);
    float3 leafLitter = float3(0.4, 0.32, 0.18);  // Brown leaf debris
    float3 denseForest = float3(0.08, 0.25, 0.08);  // Very dark under canopy

    // Tree canopy pattern using voronoi for tree placement
    float treePlacement = voronoi(float3(pos.xz * 0.15, 0.0));  // Large scale tree positions
    float canopyNoise = fbm(float3(pos.xz * 0.3, 0.0), 3);

    // Tree shadow circles - darker under tree canopy
    float underCanopy = smoothstep(0.3, 0.5, treePlacement);
    float3 forestColor = lerp(denseForest, forestFloor, underCanopy);

    // Leaf litter patterns
    float litterNoise = fbm(pos * 5.0, 2);
    forestColor = lerp(forestColor, leafLitter, smoothstep(0.5, 0.7, litterNoise) * 0.3);

    // Dappled sunlight effect (light filtering through canopy)
    float dapple = noise3D(pos * 15.0 + float3(time * 0.1, 0.0, time * 0.05));
    float sunSpot = smoothstep(0.6, 0.8, dapple) * (1.0 - underCanopy * 0.5);
    forestColor += float3(0.15, 0.2, 0.08) * sunSpot;

    // Moss on ground (greenish patches)
    float mossNoise = fbm(pos * 8.0, 2);
    float3 mossColor = float3(0.12, 0.4, 0.15);
    forestColor = lerp(forestColor, mossColor, smoothstep(0.55, 0.75, mossNoise) * 0.4);

    // Add some grass blade detail in lighter areas
    float grassBlades = noise3D(pos * 60.0) * underCanopy;
    forestColor.g += grassBlades * 0.05;

    return forestColor;
}

float3 generateRockColor(float3 pos, float3 normal)
{
    float rockPattern = voronoi(pos * 5.0);
    float rockDetail = fbm(pos * 10.0, 4);

    float3 darkRock = float3(0.4, 0.4, 0.4);
    float3 lightRock = float3(0.6, 0.6, 0.6);

    return lerp(darkRock, lightRock, rockPattern * 0.7 + rockDetail * 0.3);
}

float3 generateSnowColor(float3 pos)
{
    float snowNoise = noise3D(pos * 20.0) * 0.15;
    float3 pureSnow = float3(0.95, 0.95, 0.98);
    return pureSnow * (1.0 - snowNoise);
}

float3 calculateProceduralNormal(float3 pos, float3 baseNormal, float heightNormalized)
{
    float3 normal = baseNormal;

    // Thresholds matching pixel shader biome boundaries (UPDATED for comprehensive overhaul)
    // WATER_THRESH = 0.012, WET_SAND_THRESH = 0.025, BEACH_THRESH = 0.05, GRASS_THRESH = 0.35
    if (heightNormalized < 0.05)
    {
        // Water/beach - subtle wave-like normal perturbation
        float nx = noise3D(pos * 3.0 + float3(0.1, 0, 0)) - noise3D(pos * 3.0 - float3(0.1, 0, 0));
        float nz = noise3D(pos * 3.0 + float3(0, 0, 0.1)) - noise3D(pos * 3.0 - float3(0, 0, 0.1));
        normal += float3(nx, 0.0, nz) * 0.1;

        // Add sand ripple pattern for wet sand zone
        if (heightNormalized >= 0.012 && heightNormalized < 0.025)
        {
            float rippleNx = sin(pos.x * 15.0) * cos(pos.z * 12.0) * 0.05;
            float rippleNz = cos(pos.x * 12.0) * sin(pos.z * 15.0) * 0.05;
            normal += float3(rippleNx, 0.0, rippleNz);
        }
    }
    else if (heightNormalized >= 0.05 && heightNormalized < 0.35)
    {
        // Grass - medium detail normal perturbation
        float nx = noise3D(pos * 25.0 + float3(0.1, 0, 0)) - noise3D(pos * 25.0 - float3(0.1, 0, 0));
        float nz = noise3D(pos * 25.0 + float3(0, 0, 0.1)) - noise3D(pos * 25.0 - float3(0, 0, 0.1));
        normal += float3(nx, 0.0, nz) * 0.2;
    }
    else if (heightNormalized >= 0.35)
    {
        // Rock/mountain/snow - strong rocky normal perturbation
        float nx = voronoi(pos * 8.0 + float3(0.1, 0, 0)) - voronoi(pos * 8.0 - float3(0.1, 0, 0));
        float nz = voronoi(pos * 8.0 + float3(0, 0, 0.1)) - voronoi(pos * 8.0 - float3(0, 0, 0.1));
        normal += float3(nx, 0.0, nz) * 0.4;
    }

    return normalize(normal);
}

// ============================================================================
// Vertex Shader
// ============================================================================

PSInput VSMain(VSInput input)
{
    PSInput output;

    float3 pos = input.position;

    // Animate water vertices ONLY - COMPREHENSIVE OVERHAUL
    // C++ terrain: WATER_LEVEL = 0.0, HEIGHT_SCALE = 30.0
    // Water is at height <= 0.0, so only animate vertices at or below water level
    // Using small threshold (0.5) to catch shoreline vertices without affecting land
    if (input.position.y < 0.5)
    {
        // Scale animation based on how far below water surface (0.0)
        // Vertices at 0 get minimal animation, deeper gets more
        float waterFactor = saturate((0.5 - input.position.y) / 5.0);

        // Use the multi-frequency wave displacement function
        float waveDisplacement = calculateWaveDisplacement(input.position);
        pos.y += waveDisplacement * waterFactor;

        // Add very subtle horizontal displacement for more organic motion
        float horizDisp = noise3D(float3(input.position.xz * 0.5 + time * 0.1, 0.0)) * 0.002;
        pos.x += horizDisp * waterFactor;
        pos.z += horizDisp * waterFactor * 0.7;
    }

    float4 worldPos = mul(model, float4(pos, 1.0));  // Column-major: matrix-on-left
    output.fragPos = worldPos.xyz;
    output.worldPos = worldPos.xyz;
    output.normal = mul((float3x3)model, input.normal); // Column-major: matrix-on-left
    output.color = input.color;
    output.height = input.position.y;

    output.position = mul(viewProjection, worldPos);  // Column-major: matrix-on-left

    return output;
}

// ============================================================================
// Pixel Shader
// ============================================================================

float4 PSMain(PSInput input) : SV_TARGET
{
    // Normalize height to 0-1 range
    // C++ terrain generates heights 0-30 (HEIGHT_SCALE=30, no offset)
    float heightNormalized = input.height / 30.0;
    heightNormalized = saturate(heightNormalized);

    // Calculate view and light directions for water rendering
    float3 viewDir = normalize(viewPos - input.fragPos);
    float3 lightDir = normalize(lightPos - input.fragPos);

    // Generate procedural terrain color based on height
    // C++ biome thresholds (world height -> normalized):
    // Water: < 0 (< 0.0), Beach: < 2 (< 0.067), Grass: < 10 (< 0.333)
    // Forest: < 20 (< 0.667), Mountain: < 25 (< 0.833), Snow: >= 25 (>= 0.833)
    float3 terrainColor;
    float roughness = 0.8;
    bool isWater = false;

    // Biome thresholds (normalized 0-1, matching C++ heights 0-30)
    // COMPREHENSIVE OVERHAUL: Reduced water coverage, added wet sand transition
    static const float WATER_THRESH = 0.012;   // ~0.36 world units - minimal water coverage
    static const float WET_SAND_THRESH = 0.025; // ~0.75 world units - wet sand zone above waterline
    static const float BEACH_THRESH = 0.05;    // ~1.5 world units - dry beach transition
    static const float GRASS_THRESH = 0.35;    // ~10.5 world units
    static const float FOREST_THRESH = 0.67;   // ~20 world units
    static const float MOUNTAIN_THRESH = 0.85; // ~25.5 world units

    if (heightNormalized < WATER_THRESH)
    {
        terrainColor = generateRealisticWaterColor(input.worldPos, input.normal, viewDir, lightDir, heightNormalized);
        roughness = 0.05; // Very smooth for water
        isWater = true;
    }
    else if (heightNormalized < WET_SAND_THRESH)
    {
        // Wet sand zone - darker, more reflective sand just above waterline
        float3 wetSand = WET_SAND_COLOR;

        // Add subtle water sheen reflections on wet sand
        float wetness = 1.0 - smoothstep(WATER_THRESH, WET_SAND_THRESH, heightNormalized);

        // Animated wave wash effect on wet sand
        float waveWash = sin(input.worldPos.x * 0.3 + time * 0.5) * 0.5 + 0.5;
        waveWash *= sin(input.worldPos.z * 0.2 - time * 0.4) * 0.5 + 0.5;
        wetness = saturate(wetness + waveWash * 0.2);

        // Darken sand when wet
        float3 drySand = generateSandColor(input.worldPos);
        wetSand = lerp(drySand, wetSand * 0.7, wetness);

        // Add slight specular to wet sand
        float3 wetNormal = normalize(input.normal + float3(0.0, 0.3, 0.0));
        float wetSpec = pow(saturate(dot(reflect(-lightDir, wetNormal), viewDir)), 32.0) * wetness * 0.3;
        wetSand += float3(wetSpec, wetSpec, wetSpec) * SUN_COLOR;

        terrainColor = wetSand;
        roughness = lerp(0.9, 0.4, wetness);  // Wet sand is more specular
    }
    else if (heightNormalized < BEACH_THRESH)
    {
        terrainColor = generateSandColor(input.worldPos);
        roughness = 0.9;
    }
    else if (heightNormalized < GRASS_THRESH)
    {
        terrainColor = generateGrassColor(input.worldPos, input.normal);
        roughness = 0.85;
    }
    else if (heightNormalized < FOREST_THRESH)
    {
        // Forest biome - darker with tree canopy patterns
        terrainColor = generateForestColor(input.worldPos, input.normal);
        roughness = 0.75;
    }
    else if (heightNormalized < MOUNTAIN_THRESH)
    {
        terrainColor = generateRockColor(input.worldPos, input.normal);
        roughness = 0.95;
    }
    else
    {
        terrainColor = generateSnowColor(input.worldPos);
        roughness = 0.5;
    }

    // Blend between biomes - IMPROVED transitions
    float blendRange = 0.015;  // Slightly tighter blend for sharper biome boundaries

    // Water to wet sand transition
    if (heightNormalized >= WATER_THRESH - blendRange && heightNormalized < WATER_THRESH + blendRange)
    {
        float t = (heightNormalized - (WATER_THRESH - blendRange)) / (2.0 * blendRange);
        float3 waterCol = generateRealisticWaterColor(input.worldPos, input.normal, viewDir, lightDir, heightNormalized);
        float3 wetSand = WET_SAND_COLOR * 0.8;  // Darker wet sand at water's edge
        terrainColor = lerp(waterCol, wetSand, t);
        isWater = (t < 0.4); // Treat as water for lighting if mostly water
        roughness = lerp(0.05, 0.4, t);

        // Add foam at the water's edge
        float edgeFoam = (1.0 - abs(t - 0.5) * 2.0);
        float foamNoise = noise3D(float3(input.worldPos.xz * 2.0 + time * 0.3, time * 0.2));
        terrainColor = lerp(terrainColor, FOAM_COLOR, edgeFoam * foamNoise * 0.4);
    }
    // Wet sand to dry sand transition
    else if (heightNormalized >= WET_SAND_THRESH - blendRange && heightNormalized < WET_SAND_THRESH + blendRange)
    {
        float t = (heightNormalized - (WET_SAND_THRESH - blendRange)) / (2.0 * blendRange);
        float3 wetSand = WET_SAND_COLOR * 0.75;
        float3 drySand = generateSandColor(input.worldPos);
        terrainColor = lerp(wetSand, drySand, t);
        roughness = lerp(0.5, 0.9, t);
    }
    // Dry sand to grass transition
    else if (heightNormalized >= BEACH_THRESH - blendRange && heightNormalized < BEACH_THRESH + blendRange)
    {
        float t = (heightNormalized - (BEACH_THRESH - blendRange)) / (2.0 * blendRange);
        terrainColor = lerp(generateSandColor(input.worldPos), generateGrassColor(input.worldPos, input.normal), t);
        roughness = lerp(0.9, 0.85, t);
    }
    else if (heightNormalized >= GRASS_THRESH - blendRange && heightNormalized < GRASS_THRESH + blendRange)
    {
        // Grass to Forest transition
        float t = (heightNormalized - (GRASS_THRESH - blendRange)) / (2.0 * blendRange);
        terrainColor = lerp(generateGrassColor(input.worldPos, input.normal), generateForestColor(input.worldPos, input.normal), t);
    }
    else if (heightNormalized >= FOREST_THRESH - blendRange && heightNormalized < FOREST_THRESH + blendRange)
    {
        // Forest to Rock transition
        float t = (heightNormalized - (FOREST_THRESH - blendRange)) / (2.0 * blendRange);
        terrainColor = lerp(generateForestColor(input.worldPos, input.normal), generateRockColor(input.worldPos, input.normal), t);
    }
    else if (heightNormalized >= MOUNTAIN_THRESH - blendRange && heightNormalized < MOUNTAIN_THRESH + blendRange)
    {
        float t = (heightNormalized - (MOUNTAIN_THRESH - blendRange)) / (2.0 * blendRange);
        terrainColor = lerp(generateRockColor(input.worldPos, input.normal), generateSnowColor(input.worldPos), t);
    }

    // Calculate enhanced normal (use water normal for water surfaces)
    float3 norm;
    if (isWater)
    {
        norm = calculateWaterNormal(input.worldPos);
    }
    else
    {
        norm = calculateProceduralNormal(input.worldPos, normalize(input.normal), heightNormalized);
    }

    // PBR Material properties per biome (using same thresholds as above)
    // Water: metallic=0, roughness=0.1
    // Sand: metallic=0, roughness=0.9
    // Grass: metallic=0, roughness=0.7
    // Forest: metallic=0, roughness=0.75
    // Rock: metallic=0.1, roughness=0.85
    // Snow: metallic=0, roughness=0.3
    float metallic = 0.0;
    if (heightNormalized >= FOREST_THRESH && heightNormalized < MOUNTAIN_THRESH)
    {
        metallic = 0.1; // Rock has slight metallic quality
    }

    // Adjust roughness per biome (using consistent thresholds - UPDATED for comprehensive overhaul)
    if (heightNormalized < WATER_THRESH)
    {
        roughness = 0.08;  // Water - very smooth
    }
    else if (heightNormalized < WET_SAND_THRESH)
    {
        // Wet sand - moderately reflective
        float wetness = 1.0 - smoothstep(WATER_THRESH, WET_SAND_THRESH, heightNormalized);
        roughness = lerp(0.85, 0.35, wetness);
    }
    else if (heightNormalized < BEACH_THRESH)
    {
        roughness = 0.9;  // Dry sand
    }
    else if (heightNormalized < GRASS_THRESH)
    {
        roughness = 0.7;  // Grass
    }
    else if (heightNormalized < FOREST_THRESH)
    {
        roughness = 0.75; // Forest floor
    }
    else if (heightNormalized < MOUNTAIN_THRESH)
    {
        roughness = 0.85; // Rock
    }
    else
    {
        roughness = 0.3;  // Snow
    }

    // PBR Lighting calculation with sunIntensity multiplier
    float3 result;
    // Ensure sunIntensity has a minimum value to prevent completely dark terrain
    float effectiveSunIntensity = max(sunIntensity, 0.2);
    float3 scaledLightColor = lightColor * effectiveSunIntensity;

    // Ensure ambient color has a minimum floor
    float3 minAmbient = max(ambientColor, float3(0.2, 0.2, 0.25));

    if (isWater)
    {
        // Water already has specular, fresnel, and lighting calculated in generateRealisticWaterColor
        // Apply subtle PBR ambient modulation
        float3 ambient = calculateAmbientIBL(norm, viewDir, terrainColor, 0.0, 0.1);

        float diff = max(dot(norm, lightDir), 0.0);
        float3 diffuse = diff * scaledLightColor * 0.2; // Reduced diffuse for water

        result = ambient * 0.5 + diffuse * terrainColor + terrainColor * 0.5;
    }
    else
    {
        // Full PBR lighting for terrain surfaces
        // Direct lighting (Cook-Torrance BRDF) with sunIntensity applied
        float3 directLighting = calculatePBR(norm, viewDir, lightDir, terrainColor, metallic, roughness, scaledLightColor);

        // Ambient/IBL contribution
        float3 ambient = calculateAmbientIBL(norm, viewDir, terrainColor, metallic, roughness);

        // Ensure minimum ambient contribution to prevent dark terrain
        ambient = max(ambient, minAmbient * terrainColor * 0.5);

        // Combine direct and ambient lighting with stronger ambient contribution
        result = directLighting + ambient * 0.6;
    }

    // CRITICAL FIX: Ensure minimum visibility to prevent black terrain
    // This guarantees terrain is always visible regardless of lighting conditions
    result = max(result, terrainColor * 0.2);

    // ========================================================================
    // STYLIZED TERRAIN EFFECTS (No Man's Sky inspired)
    // ========================================================================
    if (!isWater)
    {
        // Apply slope-based rock/cliff exposure
        float3 slopeRockColor = generateRockColor(input.worldPos, norm);
        result = applySlopeVariation(result, slopeRockColor * 0.7, norm, input.worldPos);

        // Apply height-based color gradient (warmer at low, cooler at high)
        float3 lowTint = float3(1.05, 1.0, 0.95);   // Warm lowlands
        float3 highTint = float3(0.95, 0.98, 1.05); // Cool highlands
        result = applyTerrainHeightGradient(result, heightNormalized, lowTint, highTint);

        // Apply biome-specific tinting
        result = applyBiomeTint(result, heightNormalized, input.worldPos);

        // Apply color vibrancy boost
        result = applyTerrainVibrancy(result);

        // Blend stylized result based on style strength
        // This preserves original look where style strength is low
        result = lerp(result / TERRAIN_COLOR_VIBRANCY, result, TERRAIN_STYLE_STRENGTH);
    }

    // Atmospheric fog with dynamic sky colors
    float fogDensity = 0.008;
    float fogStart = 50.0;
    float dist = length(viewPos - input.fragPos);
    float fogFactor = exp(-fogDensity * max(0.0, dist - fogStart));
    fogFactor = saturate(fogFactor);

    // Calculate fog color from dynamic sky colors
    float3 fogViewDir = normalize(input.fragPos - viewPos);
    float horizonFactor = 1.0 - abs(fogViewDir.y);
    float3 fogColor = lerp(skyTopColor, skyHorizonColor, horizonFactor);

    // Add stars to the fog/sky
    fogColor += AddStars(fogViewDir, starVisibility);

    result = lerp(fogColor, result, fogFactor);

    return float4(result, 1.0);
}
