// Creature shader - Ported from OrganismEvolution GLSL
// INSTANCED RENDERING with procedural skin textures and animation
//
// INSTANCED RENDERING:
// - Per-vertex data (slot 0): position, normal, texCoord
// - Per-instance data (slot 1): model matrix (4 rows), color+type
// - Uses SV_InstanceID for per-instance animation offset
// - Reduces draw calls from N to 2 (herbivores + carnivores)

// ============================================================================
// Constant Buffers (Per-frame only - per-instance data comes from vertex buffer)
// ============================================================================

cbuffer FrameConstants : register(b0)
{
    // Per-frame constants (shared across all instances)
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
    // Day/Night cycle data (must match Terrain.hlsl and C++ Constants struct)
    float dayTime;             // Offset 240, 4 bytes
    float3 skyTopColor;        // Offset 244, 12 bytes (packs after dayTime)
    float3 skyHorizonColor;    // Offset 256, 12 bytes
    float sunIntensity;        // Offset 268, 4 bytes (packs after skyHorizonColor)
    float3 ambientColor;       // Offset 272, 12 bytes
    float starVisibility;      // Offset 284, 4 bytes (packs after ambientColor)
    float2 dayNightPadding;    // Offset 288, 8 bytes
    float2 modelAlignPadding;  // Offset 296, 8 bytes - CRITICAL: align model to 16-byte boundary
    // Legacy per-object fields (not used with instancing, kept for cbuffer compatibility)
    // Model starts at offset 304, which is 16-byte aligned (304 % 16 == 0)
    float4x4 model;            // Offset 304, 64 bytes (correctly aligned)
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
    // Per-vertex data (slot 0)
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;

    // Per-instance data (slot 1)
    float4 instModelRow0 : INST_MODEL0;   // Model matrix row 0 (16 bytes)
    float4 instModelRow1 : INST_MODEL1;   // Model matrix row 1 (16 bytes)
    float4 instModelRow2 : INST_MODEL2;   // Model matrix row 2 (16 bytes)
    float4 instModelRow3 : INST_MODEL3;   // Model matrix row 3 (16 bytes)
    float4 instColorType : INST_COLOR;    // RGB color + creature type in W (16 bytes)

    // System value for per-instance animation offset
    uint instanceID : SV_InstanceID;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 fragPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 color : TEXCOORD2;
    float3 triplanarUV : TEXCOORD3;
    float creatureType : TEXCOORD4;  // 0 = land, 1 = flying, 2 = aquatic
    float swimPhase : TEXCOORD5;     // For fish swimming animation
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
// Visual Style Parameters (No Man's Sky inspired)
// ============================================================================

// Style parameters with safe defaults
static const float STYLE_STRENGTH = 0.7;              // 0-1: blend between realistic and stylized
static const float RIM_INTENSITY = 0.45;              // Stylized rim lighting intensity
static const float RIM_POWER = 2.5;                   // Rim falloff power (lower = broader rim)
static const float GRADIENT_STRENGTH = 0.3;           // Vertical gradient shading strength
static const float COLOR_BOOST = 1.15;                // Subtle color saturation boost
static const float FRESNEL_BRIGHTNESS = 0.25;         // Edge brightness boost

// Biome-aware tint colors (applied subtly based on environment)
static const float3 WARM_BIOME_TINT = float3(1.05, 1.0, 0.92);    // Desert/volcanic
static const float3 COOL_BIOME_TINT = float3(0.92, 0.98, 1.05);   // Tundra/alpine
static const float3 LUSH_BIOME_TINT = float3(0.95, 1.05, 0.95);   // Forest/jungle
static const float3 ALIEN_BIOME_TINT = float3(1.0, 0.95, 1.08);   // Alien purple

// ============================================================================
// Stylized Lighting Functions
// ============================================================================

// Stylized gradient shading - adds subtle top-to-bottom color variation
float3 applyGradientShading(float3 baseColor, float3 normal, float3 viewDir)
{
    // Vertical gradient based on normal Y component
    float topness = normal.y * 0.5 + 0.5;

    // Slight color shift: warmer at top (sunlit), cooler at bottom (shadowed)
    float3 topTint = float3(1.05, 1.02, 0.98);
    float3 bottomTint = float3(0.95, 0.97, 1.02);

    float3 gradientTint = lerp(bottomTint, topTint, topness);
    return baseColor * lerp(float3(1, 1, 1), gradientTint, GRADIENT_STRENGTH);
}

// Enhanced stylized rim lighting with color
float3 calculateStylizedRim(float3 normal, float3 viewDir, float3 lightDir, float3 lightColor, float3 baseColor)
{
    float NdotV = saturate(dot(normal, viewDir));

    // Primary rim effect - broad and colorful
    float rimFactor = pow(1.0 - NdotV, RIM_POWER);

    // Backlight rim - stronger when light is behind creature
    float backlit = saturate(dot(-viewDir, lightDir));
    float backlitRim = pow(1.0 - NdotV, RIM_POWER * 0.7) * backlit;

    // Combine rim factors
    float totalRim = rimFactor + backlitRim * 0.5;

    // Stylized rim color: blend of light color and complementary tint
    float3 rimColor = lerp(lightColor, baseColor * 1.3 + lightColor * 0.3, 0.4);

    // Add subtle hue shift toward cyan/purple for alien feel
    rimColor.b += 0.08 * totalRim;

    return rimColor * totalRim * RIM_INTENSITY;
}

// Fresnel-based edge brightening for stylized look
float3 applyFresnelBrightness(float3 color, float3 normal, float3 viewDir)
{
    float fresnel = pow(1.0 - saturate(dot(normal, viewDir)), 3.0);
    return color * (1.0 + fresnel * FRESNEL_BRIGHTNESS);
}

// Color saturation adjustment
float3 adjustSaturation(float3 color, float saturation)
{
    float luminance = dot(color, float3(0.299, 0.587, 0.114));
    return lerp(float3(luminance, luminance, luminance), color, saturation);
}

// ============================================================================
// PBR Lighting Functions
// ============================================================================

static const float PI = 3.14159265359;

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

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}

float3 calculatePBR(float3 N, float3 V, float3 L, float3 albedo, float metallic, float roughness, float3 lightCol)
{
    float3 H = normalize(V + L);
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - metallic);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * lightCol * NdotL;
}

float3 calculateAmbientIBL(float3 N, float3 V, float3 albedo, float metallic, float roughness)
{
    float3 skyCol = float3(0.4, 0.6, 0.9);
    float3 groundCol = float3(0.3, 0.25, 0.2);

    float skyFactor = N.y * 0.5 + 0.5;
    float3 ambientLight = lerp(groundCol, skyCol, skyFactor);

    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
    float3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - metallic);

    float3 diffuseAmbient = kD * albedo * ambientLight;
    float3 specularAmbient = F * ambientLight * (1.0 - roughness) * 0.3;

    return diffuseAmbient + specularAmbient;
}

float3 calculateSubsurfaceScattering(float3 N, float3 V, float3 L, float3 albedo, float3 lightCol)
{
    float NdotL = dot(N, L);
    float wrapDiff = max(0.0, (NdotL + 0.5) / 1.5);

    float3 H = normalize(L + N * 0.5);
    float VdotH = saturate(dot(V, -H));
    float backScatter = pow(VdotH, 3.0) * 0.15;

    return albedo * lightCol * (wrapDiff * 0.3 + backScatter);
}

// ============================================================================
// Skin Pattern Generation
// ============================================================================

float3 generateSkinPattern(float3 pos, float3 normal, float3 baseColor)
{
    float3 scaledPos = pos * 8.0;

    float3 blendWeights = abs(normal);
    blendWeights = blendWeights / (blendWeights.x + blendWeights.y + blendWeights.z);

    float scaleX = voronoi(scaledPos.yzx * 2.0);
    float scaleY = voronoi(scaledPos.xzy * 2.0);
    float scaleZ = voronoi(scaledPos.xyz * 2.0);

    float scalePattern = scaleX * blendWeights.x + scaleY * blendWeights.y + scaleZ * blendWeights.z;

    float detail = fbm(pos * 20.0, 2) * 0.3;
    float pattern = scalePattern * 0.7 + detail;

    float3 darkColor = baseColor * 0.6;
    float3 lightColor = baseColor * 1.2;

    return lerp(darkColor, lightColor, pattern);
}

// ============================================================================
// Underwater Constants
// ============================================================================

static const float WATER_LEVEL = -5.0;  // Water surface height

// ============================================================================
// Fish Swimming Animation
// ============================================================================

float3 applyFishSwimming(float3 pos, float bodyPos, float swimPhase, float instanceOffset)
{
    // bodyPos: 0 = head, 1 = tail (estimated from z position)
    float normalizedZ = saturate((pos.z + 1.0) / 2.0);

    // S-wave motion - amplitude increases toward tail
    float amplitude = 0.15 * pow(normalizedZ, 1.5);
    float phaseOffset = normalizedZ * 2.0 * PI;

    // Side-to-side wave
    float wave = sin(swimPhase + phaseOffset + instanceOffset) * amplitude;
    pos.x += wave;

    // Subtle vertical undulation
    float vertWave = sin(swimPhase * 0.5 + phaseOffset * 0.7 + instanceOffset) * amplitude * 0.3;
    pos.y += vertWave;

    return pos;
}

// ============================================================================
// Vertex Shader (Instanced)
// ============================================================================

PSInput VSMain(VSInput input)
{
    PSInput output;

    // Reconstruct per-instance model matrix from vertex input
    float4x4 instanceModel = float4x4(
        input.instModelRow0,
        input.instModelRow1,
        input.instModelRow2,
        input.instModelRow3
    );

    // Per-instance animation offset based on instanceID for variation
    float instanceOffset = float(input.instanceID) * 0.7;

    // Extract creature type from W component (0=land, 1=flying, 2=aquatic)
    float creatureType = input.instColorType.w;
    output.creatureType = creatureType;

    float3 animatedPos = input.position;

    // Different animations based on creature type
    if (creatureType >= 1.5) // Aquatic (type >= 2)
    {
        // Fish swimming animation
        float swimSpeed = 3.0; // Hz
        float swimPhase = time * swimSpeed * 2.0 * PI;
        output.swimPhase = swimPhase + instanceOffset;

        // Apply S-wave body motion
        animatedPos = applyFishSwimming(animatedPos, 0.0, swimPhase, instanceOffset);

        // Subtle breathing (gill movement)
        float breathScale = 1.0 + sin(time * 1.5 + instanceOffset) * 0.02;
        animatedPos.x *= breathScale;
    }
    else if (creatureType >= 0.5) // Flying (type >= 1)
    {
        output.swimPhase = 0.0;

        // Wing flapping
        float flapPhase = time * 4.0 + instanceOffset;
        float flapAmount = sin(flapPhase) * 0.1;

        // Bob up and down with wing beats
        animatedPos.y += sin(flapPhase * 2.0) * 0.03;

        // Slight banking
        animatedPos.x += sin(flapPhase * 0.5) * 0.02;
    }
    else // Land creature
    {
        output.swimPhase = 0.0;

        // Walk cycle: vertical bob
        float bobAmount = sin(time * 1.5 + instanceOffset) * 0.05;
        animatedPos.y += bobAmount * max(input.position.y, 0.0) * 0.3;

        // Side-to-side sway
        float swayAmount = sin(time * 2.0 + instanceOffset * 1.3) * 0.03;
        animatedPos.x += swayAmount * max(input.position.y, 0.0) * 0.2;

        // Breathing
        float breathScale = 1.0 + sin(time * 0.8 + instanceOffset * 0.5) * 0.01;
        animatedPos.x *= breathScale;
        animatedPos.z *= breathScale;
    }

    // Transform to world space
    float4 worldPos = mul(instanceModel, float4(animatedPos, 1.0));
    output.fragPos = worldPos.xyz;

    // Normal transformation
    output.normal = normalize(mul((float3x3)instanceModel, input.normal));

    // Use color from per-instance data
    output.color = input.instColorType.xyz;
    output.triplanarUV = worldPos.xyz;

    output.position = mul(viewProjection, worldPos);

    return output;
}

// ============================================================================
// Underwater Effects
// ============================================================================

float3 calculateCaustics(float3 worldPos, float depth)
{
    // Animated caustic pattern using multiple voronoi layers
    float2 causticUV = worldPos.xz * 0.3;

    float caustic1 = voronoi(float3(causticUV + time * 0.04, 0.0));
    float caustic2 = voronoi(float3(causticUV * 1.7 - time * 0.055, 1.0));
    float caustic3 = voronoi(float3(causticUV * 0.8 + float2(time * 0.03, -time * 0.02), 2.0));

    // Combine caustic layers
    float causticPattern = caustic1 * caustic2 * caustic3;
    causticPattern = pow(causticPattern, 0.5);

    // Caustics are stronger in shallow water
    float depthFade = saturate(1.0 - depth * 0.1);
    float causticIntensity = causticPattern * depthFade * 0.4;

    // Blue-green caustic color
    return float3(0.5, 0.8, 1.0) * causticIntensity;
}

float3 applyUnderwaterFog(float3 color, float3 fragPos, float3 viewPos, float depth)
{
    // Underwater visibility is much lower than air
    float underwaterFogDensity = 0.08;
    float dist = length(viewPos - fragPos);

    // Exponential fog
    float fogFactor = exp(-underwaterFogDensity * dist);
    fogFactor = saturate(fogFactor);

    // Underwater fog color varies with depth
    float3 shallowFogColor = float3(0.1, 0.4, 0.5);  // Turquoise
    float3 deepFogColor = float3(0.02, 0.08, 0.15);  // Dark blue

    float depthFactor = saturate(depth * 0.15);
    float3 fogColor = lerp(shallowFogColor, deepFogColor, depthFactor);

    return lerp(fogColor, color, fogFactor);
}

float3 generateFishScales(float3 pos, float3 normal, float3 baseColor, float swimPhase)
{
    // Fish have a more reflective, scale-like pattern
    float3 scaledPos = pos * 15.0;

    // Triplanar blending
    float3 blendWeights = abs(normal);
    blendWeights = blendWeights / (blendWeights.x + blendWeights.y + blendWeights.z);

    // Elongated scale pattern (fish scales are oval)
    float3 scaleUV = scaledPos;
    scaleUV.z *= 0.5; // Elongate along body

    float scaleX = voronoi(scaleUV.yzx * 1.5);
    float scaleY = voronoi(scaleUV.xzy * 1.5);
    float scaleZ = voronoi(scaleUV.xyz * 1.5);

    float scalePattern = scaleX * blendWeights.x + scaleY * blendWeights.y + scaleZ * blendWeights.z;

    // Add iridescence effect (color shift based on view angle - computed in caller)
    float3 scaleColor = lerp(baseColor * 0.7, baseColor * 1.3, scalePattern);

    // Add subtle shimmer based on swim phase
    float shimmer = sin(swimPhase + pos.x * 10.0) * 0.1 + 0.9;
    scaleColor *= shimmer;

    return scaleColor;
}

// ============================================================================
// Pixel Shader
// ============================================================================

float4 PSMain(PSInput input) : SV_TARGET
{
    // Determine if this is an aquatic creature
    bool isAquatic = input.creatureType >= 1.5;
    bool isUnderwater = input.fragPos.y < WATER_LEVEL;

    // Generate procedural skin/scale texture
    float3 baseColor = max(input.color, float3(0.15, 0.15, 0.15));
    float3 skinColor;

    if (isAquatic)
    {
        // Fish scales pattern
        skinColor = generateFishScales(input.triplanarUV, input.normal, baseColor, input.swimPhase);

        // Fish are more metallic (shiny scales)
    }
    else
    {
        // Regular skin pattern for land/flying creatures
        skinColor = generateSkinPattern(input.triplanarUV, input.normal, baseColor);
    }

    float colorVariation = fbm(input.triplanarUV * 5.0, 2) * 0.15;
    skinColor = skinColor * (1.0 + colorVariation);
    skinColor = max(skinColor, float3(0.08, 0.08, 0.08));

    // Material properties
    float metallic = isAquatic ? 0.1 : 0.0;  // Fish scales are slightly metallic
    float roughness = isAquatic ? 0.4 : 0.6;  // Fish are shinier

    float3 norm = normalize(input.normal);
    float3 lightDir = normalize(lightPos - input.fragPos);
    float3 viewDir = normalize(viewPos - input.fragPos);

    // Calculate water depth for underwater effects
    float depth = max(0.0, WATER_LEVEL - input.fragPos.y);

    // Lighting calculation
    float3 directLighting = calculatePBR(norm, viewDir, lightDir, skinColor, metallic, roughness, lightColor);

    // Underwater: light is absorbed and scattered differently
    if (isUnderwater)
    {
        // Light absorption increases with depth (red absorbed first)
        float3 absorption = float3(
            exp(-depth * 0.15),  // Red absorbed quickly
            exp(-depth * 0.07),  // Green absorbed slower
            exp(-depth * 0.04)   // Blue absorbed slowest
        );
        directLighting *= absorption;

        // Add caustic lighting
        float3 caustics = calculateCaustics(input.fragPos, depth);
        directLighting += caustics * skinColor;
    }

    float3 ambient = calculateAmbientIBL(norm, viewDir, skinColor, metallic, roughness);

    // Underwater ambient is bluish
    if (isUnderwater)
    {
        float3 underwaterAmbient = float3(0.1, 0.2, 0.35);
        float ambientDepthFade = exp(-depth * 0.1);
        ambient = lerp(underwaterAmbient * skinColor, ambient, ambientDepthFade);
    }

    float3 subsurface = calculateSubsurfaceScattering(norm, viewDir, lightDir, skinColor, lightColor);

    // Enhanced subsurface for fish (light passes through fins)
    if (isAquatic)
    {
        subsurface *= 1.5;
    }

    // ========================================================================
    // STYLIZED LIGHTING (No Man's Sky inspired)
    // ========================================================================

    // Apply gradient shading for stylized look
    float3 gradientSkin = applyGradientShading(skinColor, norm, viewDir);

    // Enhanced stylized rim lighting
    float3 stylizedRim = calculateStylizedRim(norm, viewDir, lightDir, lightColor, skinColor);

    // Aquatic creatures get extra shimmer
    if (isAquatic)
    {
        stylizedRim *= 1.4;
        // Iridescent shimmer based on view angle
        float iridescence = sin(dot(viewDir, norm) * 8.0 + time) * 0.1 + 0.9;
        stylizedRim *= iridescence;
    }

    // Fresnel edge brightening
    float3 fresnelEnhanced = applyFresnelBrightness(gradientSkin, norm, viewDir);

    // Blend between realistic and stylized based on STYLE_STRENGTH
    float3 realisticResult = directLighting + subsurface + ambient * 0.4;
    float3 stylizedResult = directLighting * 0.8 + subsurface + ambient * 0.5 + stylizedRim;

    // Apply gradient to stylized result
    stylizedResult = lerp(realisticResult, stylizedResult, STYLE_STRENGTH);

    // Apply color boost for more vibrant creatures
    stylizedResult = adjustSaturation(stylizedResult, COLOR_BOOST);

    // Legacy rim lighting (blended with new system)
    float rimStrength = isAquatic ? 0.2 : 0.12;  // Reduced since we have new stylized rim
    float rimPower = 3.0;
    float rim = 1.0 - max(dot(viewDir, norm), 0.0);
    rim = pow(rim, rimPower);

    float3 F0 = isAquatic ? float3(0.08, 0.08, 0.08) : float3(0.04, 0.04, 0.04);
    float3 rimFresnel = fresnelSchlick(max(dot(norm, viewDir), 0.0), F0);
    float3 rimLight = rim * rimStrength * lightColor * (1.0 - rimFresnel);

    // Combine all lighting with stylized blend
    float3 result = stylizedResult + rimLight * (1.0 - STYLE_STRENGTH);

    // Apply underwater or atmospheric fog
    if (isUnderwater)
    {
        result = applyUnderwaterFog(result, input.fragPos, viewPos, depth);
    }
    else
    {
        // Regular atmospheric fog for creatures above water
        float fogDensity = 0.008;
        float fogStart = 50.0;
        float dist = length(viewPos - input.fragPos);
        float fogFactor = exp(-fogDensity * max(0.0, dist - fogStart));
        fogFactor = saturate(fogFactor);

        float3 fogColor = float3(0.7, 0.8, 0.9);
        result = lerp(fogColor, result, fogFactor);
    }

    return float4(result, 1.0);
}
