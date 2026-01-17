// ProceduralPattern.hlsl - GPU-based procedural texture generation for creatures
// Generates various pattern types (stripes, spots, scales, etc.) directly on GPU

// ============================================================================
// Constant Buffers
// ============================================================================

cbuffer PatternConstants : register(b0)
{
    float4 primaryColor;       // Main color
    float4 secondaryColor;     // Secondary/pattern color
    float4 accentColor;        // Accent highlights

    float patternScale;        // Pattern feature size
    float patternDensity;      // Pattern density/frequency
    float patternContrast;     // Contrast between colors
    float patternRandomness;   // Random variation amount

    uint patternType;          // Pattern type enum
    float bilateralSymmetry;   // 1.0 for bilateral symmetry, 0.0 for none
    float iridescence;         // Iridescent color shift amount
    float metallic;            // Metallic sheen amount

    float time;                // Animation time
    uint seed;                 // Random seed for variation
    float2 texelSize;          // 1.0 / texture dimensions
};

// Pattern type enum (must match C++ PatternType)
#define PATTERN_SOLID       0
#define PATTERN_STRIPES     1
#define PATTERN_SPOTS       2
#define PATTERN_PATCHES     3
#define PATTERN_GRADIENT    4
#define PATTERN_SCALES      5
#define PATTERN_FEATHERS    6
#define PATTERN_CAMOUFLAGE  7
#define PATTERN_RINGS       8
#define PATTERN_BANDS       9
#define PATTERN_SPECKLED    10

// ============================================================================
// Noise Functions
// ============================================================================

// Fast hash function
uint hash(uint x, uint s)
{
    x ^= s;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

float hashToFloat(uint x, uint s)
{
    return float(hash(x, s)) / float(0xFFFFFFFF);
}

float hash2D(int x, int y, uint s)
{
    return hashToFloat(hash(uint(x) + hash(uint(y), s), s), s);
}

// Smoothstep
float smootherstep(float edge0, float edge1, float x)
{
    float t = saturate((x - edge0) / (edge1 - edge0));
    return t * t * (3.0 - 2.0 * t);
}

// 2D Perlin noise
float perlinNoise(float2 p, uint s)
{
    int2 i0 = int2(floor(p));
    int2 i1 = i0 + int2(1, 1);

    float2 f = frac(p);
    float2 u = f * f * (3.0 - 2.0 * f);

    float n00 = hash2D(i0.x, i0.y, s);
    float n10 = hash2D(i1.x, i0.y, s);
    float n01 = hash2D(i0.x, i1.y, s);
    float n11 = hash2D(i1.x, i1.y, s);

    float nx0 = lerp(n00, n10, u.x);
    float nx1 = lerp(n01, n11, u.x);

    return lerp(nx0, nx1, u.y);
}

// Fractal Brownian Motion
float fbm(float2 p, uint s, int octaves)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    [unroll]
    for (int i = 0; i < octaves && i < 6; i++)
    {
        value += amplitude * perlinNoise(p * frequency, s + i);
        frequency *= 2.0;
        amplitude *= 0.5;
    }

    return value;
}

// Worley (cellular) noise
float worleyNoise(float2 p, uint s)
{
    int2 cell = int2(floor(p));
    float minDist = 10.0;

    [unroll]
    for (int dx = -1; dx <= 1; dx++)
    {
        [unroll]
        for (int dy = -1; dy <= 1; dy++)
        {
            int2 neighbor = cell + int2(dx, dy);
            float2 point = float2(neighbor) + float2(
                hash2D(neighbor.x, neighbor.y, s),
                hash2D(neighbor.y, neighbor.x, s + 1)
            );
            float dist = length(p - point);
            minDist = min(minDist, dist);
        }
    }

    return saturate(minDist);
}

// ============================================================================
// Pattern Functions
// ============================================================================

// Stripe pattern
float stripePattern(float2 uv, float scale, float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    float rotatedX = uv.x * c - uv.y * s;
    return sin(rotatedX * scale * 6.28318) * 0.5 + 0.5;
}

// Spot pattern
float spotPattern(float2 uv, float scale, float density, uint s)
{
    float2 cellUV = uv * scale * density;
    int2 cell = int2(floor(cellUV));
    float minDist = 1.0;

    [unroll]
    for (int dx = -1; dx <= 1; dx++)
    {
        [unroll]
        for (int dy = -1; dy <= 1; dy++)
        {
            int2 neighbor = cell + int2(dx, dy);
            float2 spotCenter = float2(neighbor) + float2(
                hash2D(neighbor.x, neighbor.y, s) * 0.8 + 0.1,
                hash2D(neighbor.y, neighbor.x, s + 100) * 0.8 + 0.1
            );
            float spotRadius = hash2D(neighbor.x + neighbor.y, neighbor.y - neighbor.x, s + 200) * 0.3 + 0.2;

            float dist = length(cellUV - spotCenter);
            float spotValue = smootherstep(spotRadius + 0.1, spotRadius, dist);
            minDist = min(minDist, 1.0 - spotValue);
        }
    }

    return 1.0 - minDist;
}

// Patch pattern (Voronoi-based)
float patchPattern(float2 uv, float scale, uint s)
{
    float value = worleyNoise(uv * scale, s);
    float edgeNoise = fbm(uv * scale * 2.0, s + 1000, 3);
    return smootherstep(0.3 - edgeNoise * 0.1, 0.5 + edgeNoise * 0.1, value);
}

// Scale pattern (reptile/fish)
float scalePattern(float2 uv, float scale)
{
    float2 scaledUV = uv * scale;

    // Offset every other row
    if (int(floor(scaledUV.y * 2.0)) % 2 == 1)
    {
        scaledUV.x += 0.5;
    }

    float2 f = frac(scaledUV);
    float2 center = float2(0.5, 0.5);
    float dist = length((f - center) * float2(1.0, 1.5));

    return smootherstep(0.6, 0.3, dist);
}

// Feather pattern
float featherPattern(float2 uv, float scale, float direction)
{
    float c = cos(direction);
    float s = sin(direction);

    float2 rotatedUV = float2(
        uv.x * c - uv.y * s,
        uv.x * s + uv.y * c
    );

    // Main shaft
    float shaftDist = abs(rotatedUV.x) * scale;
    float shaft = smootherstep(0.1, 0.0, shaftDist);

    // Barbs
    float barbAngle = rotatedUV.y * scale * 20.0;
    float barbPhase = sin(barbAngle);
    float barbDist = abs(rotatedUV.x - barbPhase * 0.1) * scale;
    float barbs = smootherstep(0.3, 0.1, barbDist);

    return max(shaft, barbs * (1.0 - smootherstep(0.0, 0.5, abs(rotatedUV.x) * scale)));
}

// Ring pattern (concentric circles)
float ringPattern(float2 uv, float scale, uint s)
{
    float2 center = float2(0.5, 0.5);
    float dist = length(uv - center);

    float noise = fbm(uv * 5.0, s, 2) * patternRandomness * 0.1;
    return sin((dist + noise) * scale * 30.0) * 0.5 + 0.5;
}

// Band pattern (horizontal stripes)
float bandPattern(float2 uv, float scale, uint s)
{
    float noise = fbm(float2(uv.x * 8.0, uv.y * 2.0), s, 2) * patternRandomness;
    float value = sin((uv.y + noise * 0.1) * scale * 20.0) * 0.5 + 0.5;
    return smootherstep(0.3, 0.7, value);
}

// ============================================================================
// Main Pattern Generation
// ============================================================================

float4 generatePattern(float2 uv)
{
    float4 color = primaryColor;
    float patternValue = 0.0;

    // Apply bilateral symmetry
    if (bilateralSymmetry > 0.5)
    {
        uv.x = uv.x < 0.5 ? uv.x : 1.0 - uv.x;
    }

    // Generate pattern based on type
    [branch]
    switch (patternType)
    {
        case PATTERN_SOLID:
            {
                // Solid with subtle noise
                float noise = fbm(uv * 10.0, seed, 2) * 0.1;
                color = primaryColor * (1.0 + noise - 0.05);
            }
            break;

        case PATTERN_STRIPES:
            {
                float angle = hash2D(seed, 0, seed) * 1.57 - 0.78;
                float noise = fbm(uv * 5.0, seed + 100, 2) * patternRandomness;
                patternValue = stripePattern(uv + float2(noise, noise), patternScale * 5.0, angle);
                patternValue = smootherstep(0.5 - patternContrast * 0.3, 0.5 + patternContrast * 0.3, patternValue);
                color = lerp(primaryColor, secondaryColor, patternValue);
            }
            break;

        case PATTERN_SPOTS:
            {
                patternValue = spotPattern(uv, patternScale * 3.0, patternDensity * 5.0 + 2.0, seed);
                color = lerp(primaryColor, secondaryColor, patternValue);
            }
            break;

        case PATTERN_PATCHES:
            {
                patternValue = patchPattern(uv, patternScale * 4.0, seed);
                color = lerp(primaryColor, secondaryColor, patternValue);
            }
            break;

        case PATTERN_GRADIENT:
            {
                float noise = fbm(uv * 3.0, seed, 2) * patternRandomness;
                float gradientValue = uv.y + noise * 0.2;
                gradientValue = smootherstep(0.0, 1.0, gradientValue);

                if (gradientValue < 0.5)
                    color = lerp(primaryColor, accentColor, gradientValue * 2.0);
                else
                    color = lerp(accentColor, secondaryColor, (gradientValue - 0.5) * 2.0);
            }
            break;

        case PATTERN_SCALES:
            {
                patternValue = scalePattern(uv, patternScale * 10.0);

                // Iridescence effect
                float iriShift = sin(uv.x * 20.0 + uv.y * 20.0) * iridescence;
                color = lerp(secondaryColor, primaryColor, patternValue);
                color.r += iriShift * 0.1;
                color.g -= iriShift * 0.05;
                color.b += iriShift * 0.15;
            }
            break;

        case PATTERN_FEATHERS:
            {
                patternValue = 0.0;
                [unroll]
                for (int i = 0; i < 3; i++)
                {
                    float offsetU = hash2D(i, 0, seed) * 0.3;
                    float offsetV = hash2D(0, i, seed) * 0.3;
                    float angle = hash2D(i, i, seed) * 0.5 - 0.25;
                    patternValue += featherPattern(uv - float2(0.5 - offsetU, 0.5 - offsetV), patternScale * 3.0, angle) * 0.4;
                }
                patternValue = saturate(patternValue);
                color = lerp(primaryColor, secondaryColor, patternValue);

                // Accent at edges
                if (patternValue > 0.3 && patternValue < 0.6)
                    color = lerp(color, accentColor, 0.3);
            }
            break;

        case PATTERN_CAMOUFLAGE:
            {
                float noise1 = fbm(uv * patternScale * 4.0, seed, 4);
                float noise2 = fbm(uv * patternScale * 8.0 + float2(100, 100), seed + 1, 3);
                float noise3 = worleyNoise(uv * patternScale * 3.0, seed + 2);

                float combined = noise1 * 0.5 + noise2 * 0.3 + noise3 * 0.2;

                if (combined < 0.33)
                    color = primaryColor;
                else if (combined < 0.66)
                    color = secondaryColor;
                else
                    color = accentColor;

                // Soft edge blending
                float edgeBlend = fbm(uv * 20.0, seed + 3, 2) * 0.1;
                if (combined > 0.3 && combined < 0.36)
                    color = lerp(primaryColor, secondaryColor, (combined - 0.3) / 0.06 + edgeBlend);
                else if (combined > 0.63 && combined < 0.69)
                    color = lerp(secondaryColor, accentColor, (combined - 0.63) / 0.06 + edgeBlend);
            }
            break;

        case PATTERN_RINGS:
            {
                patternValue = ringPattern(uv, patternScale, seed);
                color = lerp(primaryColor, secondaryColor, patternValue);
            }
            break;

        case PATTERN_BANDS:
            {
                patternValue = bandPattern(uv, patternScale, seed);
                color = lerp(primaryColor, secondaryColor, patternValue);
            }
            break;

        case PATTERN_SPECKLED:
            {
                color = primaryColor;
                float speckleNoise = hash2D(int(uv.x * 256), int(uv.y * 256), seed);
                float threshold = 1.0 - patternDensity * 0.3;

                if (speckleNoise > threshold)
                {
                    float speckleType = hash2D(int(uv.x * 256) + 1000, int(uv.y * 256) + 1000, seed);
                    color = speckleType < 0.5 ? secondaryColor : accentColor;
                }

                // Subtle noise overlay
                float noise = fbm(uv * 20.0, seed + 500, 2) * 0.1;
                color.rgb *= (1.0 + noise - 0.05);
            }
            break;
    }

    // Apply metallic sheen
    if (metallic > 0.0)
    {
        float metalSheen = pow(abs(uv.x - 0.5) * 2.0, 2.0) * metallic;
        color.rgb = lerp(color.rgb, color.rgb * 1.5, metalSheen);
    }

    color = saturate(color);
    color.a = 1.0;

    return color;
}

// ============================================================================
// Compute Shader for Texture Generation
// ============================================================================

RWTexture2D<float4> OutputTexture : register(u0);

[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint width, height;
    OutputTexture.GetDimensions(width, height);

    if (DTid.x >= width || DTid.y >= height)
        return;

    float2 uv = float2(DTid.xy) / float2(width, height);
    float4 color = generatePattern(uv);

    OutputTexture[DTid.xy] = color;
}

// ============================================================================
// Pixel Shader for Real-time Pattern Preview
// ============================================================================

struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

cbuffer PreviewConstants : register(b1)
{
    float4x4 worldViewProj;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = mul(worldViewProj, float4(input.position, 1.0));
    output.texCoord = input.texCoord;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return generatePattern(input.texCoord);
}

// ============================================================================
// Normal Map Generation (from pattern)
// ============================================================================

float4 PSMainNormal(PSInput input) : SV_TARGET
{
    // Sample pattern at neighboring pixels for gradient
    float2 uv = input.texCoord;
    float center = generatePattern(uv).r * 0.3 + generatePattern(uv).g * 0.59 + generatePattern(uv).b * 0.11;
    float right = generatePattern(uv + float2(texelSize.x, 0)).r * 0.3;
    float up = generatePattern(uv + float2(0, texelSize.y)).r * 0.3;

    // Calculate normal from height differences
    float3 normal;
    normal.x = (center - right) * 2.0;
    normal.y = (center - up) * 2.0;
    normal.z = 1.0;
    normal = normalize(normal);

    // Encode to [0,1] range
    return float4(normal * 0.5 + 0.5, 1.0);
}
