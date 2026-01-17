// Vegetation shader - Trees, bushes, and grass
// Enhanced with wind animation and improved lighting
// Ported from OrganismEvolution GLSL

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
    // Day/Night cycle data
    float dayTime;             // Offset 240, 4 bytes
    float3 skyTopColor;        // Offset 244, 12 bytes (packs after dayTime)
    float3 skyHorizonColor;    // Offset 256, 12 bytes
    float sunIntensity;        // Offset 268, 4 bytes (packs after skyHorizonColor)
    float3 ambientColor;       // Offset 272, 12 bytes
    float starVisibility;      // Offset 284, 4 bytes (packs after ambientColor)
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
// Wind Configuration
// ============================================================================

static const float WIND_STRENGTH = 0.15;        // Overall wind intensity
static const float WIND_FREQUENCY = 1.5;        // Wind wave frequency
static const float WIND_TURBULENCE = 0.3;       // Small-scale wind variation
static const float3 WIND_DIRECTION = float3(1.0, 0.0, 0.3);  // Primary wind direction

// ============================================================================
// Visual Style Parameters (No Man's Sky inspired)
// ============================================================================

static const float VEGETATION_STYLE_STRENGTH = 0.7;  // 0-1: stylized vs realistic
static const float FOLIAGE_SHEEN = 0.15;             // Waxy leaf shine
static const float COLOR_VIBRANCY = 1.18;            // Saturation boost for plants
static const float RIM_INTENSITY = 0.3;              // Edge lighting on leaves

// Biome vegetation tints
static const float3 LUSH_LEAF_TINT = float3(0.35, 0.75, 0.38);
static const float3 AUTUMN_LEAF_TINT = float3(0.85, 0.52, 0.25);
static const float3 ALIEN_LEAF_TINT = float3(0.68, 0.45, 0.82);
static const float3 TROPICAL_LEAF_TINT = float3(0.28, 0.72, 0.45);

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

// ============================================================================
// Input/Output Structures
// ============================================================================

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 fragPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 color : TEXCOORD2;
    float heightFactor : TEXCOORD3;  // For foliage detection
};

// ============================================================================
// Wind Animation
// ============================================================================

float3 calculateWindOffset(float3 worldPos, float localHeight, float3 modelPos)
{
    // Height-based wind influence (more at top, none at base)
    float heightInfluence = saturate(localHeight / 4.0);
    heightInfluence = heightInfluence * heightInfluence;  // Quadratic falloff

    // Main wind wave
    float windPhase = time * WIND_FREQUENCY;
    float windWave = sin(windPhase + worldPos.x * 0.3 + worldPos.z * 0.2);

    // Secondary wave for variation
    float windWave2 = sin(windPhase * 0.7 + worldPos.x * 0.5 - worldPos.z * 0.4) * 0.5;

    // Turbulence (small-scale noise)
    float turbulence = noise3D(worldPos * 2.0 + float3(time * 0.5, 0, time * 0.3)) - 0.5;

    // Combine wind components
    float totalWind = (windWave + windWave2) * WIND_STRENGTH + turbulence * WIND_TURBULENCE;

    // Apply wind in primary direction
    float3 windDir = normalize(WIND_DIRECTION);
    float3 offset = windDir * totalWind * heightInfluence;

    // Add slight vertical bob
    offset.y += sin(windPhase * 1.5 + worldPos.x * 0.4) * 0.02 * heightInfluence;

    return offset;
}

// ============================================================================
// Vertex Shader
// ============================================================================

PSInput VSMain(VSInput input)
{
    PSInput output;

    // Get world position without wind first
    float4 baseWorldPos = mul(model, float4(input.position, 1.0));

    // Calculate wind displacement based on local height
    // Foliage (green parts) sway more than trunk (brown parts)
    float localHeight = input.position.y;

    // Detect if this is foliage based on color (green = foliage, brown = trunk)
    // texCoord.r stores red, texCoord.g stores green
    float isFoliage = step(input.texCoord.r, input.texCoord.g);  // Green > Red = foliage

    // Calculate wind offset
    float3 modelPos = float3(model[3][0], model[3][1], model[3][2]);
    float3 windOffset = calculateWindOffset(baseWorldPos.xyz, localHeight, modelPos);

    // Apply more wind to foliage, less to trunk
    float windMultiplier = lerp(0.2, 1.0, isFoliage);
    windOffset *= windMultiplier;

    // Apply wind displacement
    float4 worldPos = baseWorldPos;
    worldPos.xyz += windOffset;

    output.fragPos = worldPos.xyz;
    output.heightFactor = localHeight;

    // Normal transformation with wind perturbation
    float3 baseNormal = mul((float3x3)model, input.normal);

    // Slightly perturb normal based on wind for dynamic lighting
    float windNormalEffect = length(windOffset) * 2.0;
    baseNormal.x += windOffset.x * windNormalEffect;
    baseNormal.z += windOffset.z * windNormalEffect;
    output.normal = normalize(baseNormal);

    // Extract color from texCoord (tree generator stores color in RG channels)
    // Reconstruct full color: RG from texcoord, B estimated from context
    float r = input.texCoord.r;
    float g = input.texCoord.g;
    // For foliage, B is typically low (green), for trunk B matches brownish tones
    float b = lerp(r * 0.4, 0.1, isFoliage);
    output.color = float3(r, g, b);

    output.position = mul(viewProjection, worldPos);

    return output;
}

// ============================================================================
// Stylized Vegetation Functions
// ============================================================================

// Apply stylized sheen to foliage
float3 applyFoliageSheen(float3 baseColor, float3 normal, float3 viewDir, float3 lightDir, float3 lightCol)
{
    float3 halfDir = normalize(lightDir + viewDir);
    float NdotH = saturate(dot(normal, halfDir));

    // Waxy leaf sheen - broader than typical specular
    float sheen = pow(NdotH, 8.0) * FOLIAGE_SHEEN;

    // Tint sheen slightly toward leaf color
    float3 sheenColor = lerp(lightCol, baseColor * 1.5, 0.3);

    return baseColor + sheenColor * sheen;
}

// Stylized rim lighting for vegetation
float3 applyVegetationRim(float3 baseColor, float3 normal, float3 viewDir, float3 lightCol)
{
    float NdotV = saturate(dot(normal, viewDir));
    float rimFactor = pow(1.0 - NdotV, 2.5);

    // Vegetation rim is tinted by the leaf color
    float3 rimColor = lerp(lightCol, baseColor * 1.2, 0.5);

    return rimColor * rimFactor * RIM_INTENSITY;
}

// Saturation boost for vibrant vegetation
float3 applyVegetationVibrancy(float3 color)
{
    float luminance = dot(color, float3(0.299, 0.587, 0.114));
    return lerp(float3(luminance, luminance, luminance), color, COLOR_VIBRANCY);
}

// ============================================================================
// Pixel Shader
// ============================================================================

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 norm = normalize(input.normal);
    float3 viewDir = normalize(viewPos - input.fragPos);
    float3 lightDir = normalize(lightPos - input.fragPos);

    // Enhanced ambient with sky color influence
    float3 ambient = ambientColor * 0.4;

    // Diffuse lighting
    float diff = max(dot(norm, lightDir), 0.0);

    // Wrap lighting for softer look on vegetation
    float wrapDiff = (diff + 0.3) / 1.3;
    float3 diffuse = wrapDiff * lightColor * sunIntensity;

    // Subsurface scattering approximation for leaves
    // Light passing through thin leaves
    float3 backLightDir = -lightDir;
    float backScatter = max(dot(viewDir, backLightDir), 0.0);
    backScatter = pow(backScatter, 2.0) * 0.35;  // Slightly increased for stylized look
    float3 subsurface = input.color * backScatter * sunIntensity;

    // ========================================================================
    // STYLIZED VEGETATION EFFECTS (No Man's Sky inspired)
    // ========================================================================

    // Apply stylized sheen to foliage
    float3 sheenColor = applyFoliageSheen(input.color, norm, viewDir, lightDir, lightColor);

    // Stylized rim lighting
    float3 stylizedRim = applyVegetationRim(input.color, norm, viewDir, lightColor);

    // Very subtle specular for wet/waxy leaves (reduced, sheen handles this now)
    float specularStrength = 0.04;
    float3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), 16.0);
    float3 specular = specularStrength * spec * lightColor;

    // Legacy rim lighting (reduced since we have stylized rim)
    float rimDot = 1.0 - max(dot(viewDir, norm), 0.0);
    float rimIntensityLegacy = smoothstep(0.5, 1.0, rimDot);
    float3 rimLight = rimIntensityLegacy * 0.05 * lightColor;

    // Combine realistic lighting
    float3 realisticResult = (ambient + diffuse + specular) * input.color;
    realisticResult += subsurface;
    realisticResult += rimLight * input.color;

    // Combine stylized lighting
    float3 stylizedResult = (ambient + diffuse * 0.9) * sheenColor;
    stylizedResult += subsurface * 1.1;  // More translucency
    stylizedResult += stylizedRim;

    // Apply vibrancy to stylized result
    stylizedResult = applyVegetationVibrancy(stylizedResult);

    // Blend between realistic and stylized
    float3 result = lerp(realisticResult, stylizedResult, VEGETATION_STYLE_STRENGTH);

    // Ensure minimum visibility
    result = max(result, input.color * 0.15);

    // Apply atmospheric fog
    float fogDensity = 0.005;
    float fogStart = 80.0;
    float dist = length(viewPos - input.fragPos);
    float fogFactor = exp(-fogDensity * max(0.0, dist - fogStart));
    fogFactor = saturate(fogFactor);

    // Fog color from sky
    float3 fogColor = lerp(skyHorizonColor, skyTopColor, 0.3);
    result = lerp(fogColor, result, fogFactor);

    return float4(result, 1.0);
}
