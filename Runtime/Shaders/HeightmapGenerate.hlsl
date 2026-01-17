// HeightmapGenerate.hlsl
// GPU compute shader for procedural heightmap generation
// Generates 2048x2048 heightmap with Perlin noise, erosion, ridges, and voronoi

#define THREAD_GROUP_SIZE 16
#define PI 3.14159265359f
#define HEIGHTMAP_SIZE 2048

// ============================================================================
// Constants
// ============================================================================

cbuffer HeightmapConstants : register(b0)
{
    // Base terrain noise
    float g_baseFrequency;
    float g_baseAmplitude;
    int g_octaves;
    float g_persistence;

    float g_lacunarity;
    float g_erosionStrength;
    float g_ridgeSharpness;
    float g_voronoiScale;

    float g_voronoiWeight;
    float g_islandFalloffStart;
    float g_islandFalloffEnd;
    float g_waterLevel;

    uint g_seed;
    float g_heightScale;
    float2 g_padding;
};

// Output heightmap (R32_FLOAT)
RWTexture2D<float> g_heightmap : register(u0);

// ============================================================================
// Hash Functions (GPU-optimized)
// ============================================================================

// Simple hash for uint to float [0, 1]
float Hash1(uint n)
{
    n = (n << 13u) ^ n;
    n = n * (n * n * 15731u + 789221u) + 1376312589u;
    return float(n & 0x7fffffffu) / float(0x7fffffff);
}

// 2D hash function
float2 Hash2(float2 p)
{
    p = float2(dot(p, float2(127.1, 311.7)),
               dot(p, float2(269.5, 183.3)));
    return frac(sin(p) * 43758.5453);
}

// 3D hash for gradients
float3 Hash3(float3 p)
{
    p = float3(dot(p, float3(127.1, 311.7, 74.7)),
               dot(p, float3(269.5, 183.3, 246.1)),
               dot(p, float3(113.5, 271.9, 124.6)));
    return frac(sin(p) * 43758.5453) * 2.0 - 1.0;
}

// ============================================================================
// Noise Functions
// ============================================================================

// Improved Perlin noise with quintic interpolation
float GradientNoise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);

    // Quintic interpolation curve (smoother than cubic)
    float2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

    // Four corners
    float2 ga = Hash2(i + float2(0.0, 0.0)) * 2.0 - 1.0;
    float2 gb = Hash2(i + float2(1.0, 0.0)) * 2.0 - 1.0;
    float2 gc = Hash2(i + float2(0.0, 1.0)) * 2.0 - 1.0;
    float2 gd = Hash2(i + float2(1.0, 1.0)) * 2.0 - 1.0;

    float va = dot(ga, f - float2(0.0, 0.0));
    float vb = dot(gb, f - float2(1.0, 0.0));
    float vc = dot(gc, f - float2(0.0, 1.0));
    float vd = dot(gd, f - float2(1.0, 1.0));

    return lerp(lerp(va, vb, u.x), lerp(vc, vd, u.x), u.y);
}

// Value noise for variation
float ValueNoise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);

    // Smooth interpolation
    float2 u = f * f * (3.0 - 2.0 * f);

    float a = Hash1(uint(i.x + i.y * 57.0 + g_seed));
    float b = Hash1(uint(i.x + 1.0 + i.y * 57.0 + g_seed));
    float c = Hash1(uint(i.x + (i.y + 1.0) * 57.0 + g_seed));
    float d = Hash1(uint(i.x + 1.0 + (i.y + 1.0) * 57.0 + g_seed));

    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

// Fractal Brownian Motion
float FBM(float2 p, int octaves, float persistence, float lacunarity)
{
    float value = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    float maxValue = 0.0;

    for (int i = 0; i < octaves && i < 12; i++)
    {
        value += amplitude * GradientNoise(p * frequency + float2(g_seed * 0.01, g_seed * 0.013));
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return value / maxValue;
}

// Ridge noise for mountain peaks
float RidgeNoise(float2 p, float sharpness)
{
    float n = GradientNoise(p + float2(g_seed * 0.01, g_seed * 0.013));
    n = 1.0 - abs(n);           // Fold into ridges
    n = pow(n, sharpness);      // Sharpen peaks
    return n;
}

// Multi-octave ridge noise
float RidgeFBM(float2 p, int octaves, float persistence, float lacunarity, float sharpness)
{
    float value = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    float maxValue = 0.0;

    for (int i = 0; i < octaves && i < 8; i++)
    {
        value += amplitude * RidgeNoise(p * frequency, sharpness);
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return value / maxValue;
}

// Voronoi for cracked terrain patterns
float2 Voronoi(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);

    float minDist = 1.0;
    float secondMinDist = 1.0;
    float2 minPoint = float2(0, 0);

    for (int y = -1; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++)
        {
            float2 neighbor = float2(x, y);
            float2 cellPoint = Hash2(i + neighbor + float2(g_seed, g_seed * 1.3));
            float2 diff = neighbor + cellPoint - f;
            float dist = length(diff);

            if (dist < minDist)
            {
                secondMinDist = minDist;
                minDist = dist;
                minPoint = cellPoint;
            }
            else if (dist < secondMinDist)
            {
                secondMinDist = dist;
            }
        }
    }

    return float2(minDist, secondMinDist - minDist);
}

// Domain warping for more organic shapes
float2 DomainWarp(float2 p, float strength)
{
    float2 offset;
    offset.x = FBM(p + float2(5.2, 1.3), 4, 0.5, 2.0);
    offset.y = FBM(p + float2(9.7, 3.1), 4, 0.5, 2.0);
    return p + offset * strength;
}

// ============================================================================
// Terrain Features
// ============================================================================

// Simplified thermal erosion (slope-based)
float ThermalErosion(float2 uv, float baseHeight)
{
    float2 texelSize = 1.0 / float2(HEIGHTMAP_SIZE, HEIGHTMAP_SIZE);

    // Sample neighbors at larger scale for erosion effect
    float erosionScale = 8.0;
    float2 offsets[4] = {
        float2(-texelSize.x, 0) * erosionScale,
        float2(texelSize.x, 0) * erosionScale,
        float2(0, -texelSize.y) * erosionScale,
        float2(0, texelSize.y) * erosionScale
    };

    float erosion = 0.0;
    for (int i = 0; i < 4; i++)
    {
        float2 neighborUV = uv + offsets[i];
        // Estimate neighbor height using same noise (approximation)
        float neighborHeight = FBM(neighborUV * g_baseFrequency * 500.0, min(g_octaves, 6), g_persistence, g_lacunarity);
        neighborHeight = neighborHeight * 0.5 + 0.5;

        float diff = baseHeight - neighborHeight;
        if (diff > 0.0)
        {
            erosion += diff * g_erosionStrength;
        }
    }

    return erosion * 0.25;  // Average of 4 neighbors
}

// Island falloff mask
float IslandMask(float2 uv)
{
    float2 center = float2(0.5, 0.5);
    float distFromCenter = length(uv - center) * 2.0;  // 0 at center, 1+ at corners

    // Smooth falloff
    float mask = 1.0 - smoothstep(g_islandFalloffStart, g_islandFalloffEnd, distFromCenter);

    // Add some noise to the edge for irregular coastline
    float edgeNoise = FBM(uv * 20.0, 4, 0.5, 2.0) * 0.1;
    mask = saturate(mask + edgeNoise * (1.0 - mask));

    return mask;
}

// Plateau feature using stepped noise
float PlateauFeature(float2 p, float height)
{
    // Create stepped terrain effect in mountainous areas
    if (height > 0.6)
    {
        float plateauNoise = Voronoi(p * 0.5).x;
        float stepped = floor(height * 8.0) / 8.0;
        return lerp(height, stepped, plateauNoise * 0.3);
    }
    return height;
}

// ============================================================================
// Main Compute Shader
// ============================================================================

[numthreads(THREAD_GROUP_SIZE, THREAD_GROUP_SIZE, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint2 texCoord = dispatchThreadId.xy;

    // Bounds check
    if (texCoord.x >= HEIGHTMAP_SIZE || texCoord.y >= HEIGHTMAP_SIZE)
        return;

    // Normalize to 0-1 range
    float2 uv = float2(texCoord) / float(HEIGHTMAP_SIZE);

    // Apply seed offset for variation
    float2 seedOffset = float2(g_seed * 0.001, g_seed * 0.0013);

    // === Domain warping for organic shapes ===
    float2 warpedUV = DomainWarp(uv + seedOffset, 0.15);

    // === Base terrain with FBM ===
    float baseNoise = FBM(warpedUV * g_baseFrequency * 500.0, g_octaves, g_persistence, g_lacunarity);
    float height = baseNoise * 0.5 + 0.5;  // Remap to 0-1

    // === Add ridge mountains in high areas ===
    float ridgeHeight = RidgeFBM(warpedUV * g_baseFrequency * 200.0, 5, 0.6, 2.2, g_ridgeSharpness);
    float ridgeBlend = smoothstep(0.5, 0.75, height);
    height = lerp(height, lerp(height, ridgeHeight * 1.2, 0.5), ridgeBlend);

    // === Add voronoi-based features ===
    float2 voronoiResult = Voronoi(warpedUV * g_voronoiScale * 500.0);
    float voronoiHeight = voronoiResult.x;
    float voronoiEdge = voronoiResult.y;

    // Voronoi creates rocky/cracked appearance
    height = height * (1.0 - g_voronoiWeight) + height * (0.8 + voronoiHeight * 0.4) * g_voronoiWeight;

    // === Valley carving ===
    float valleyNoise = FBM((uv + seedOffset) * 150.0, 4, 0.5, 2.0);
    valleyNoise = abs(valleyNoise);  // Creates valley patterns
    float valleyMask = smoothstep(0.35, 0.55, height);  // Only in mid elevations
    height -= valleyNoise * 0.1 * valleyMask;

    // === Apply erosion ===
    float erosion = ThermalErosion(uv, height);
    height -= erosion;

    // === Plateau features ===
    height = PlateauFeature(warpedUV * 30.0, height);

    // === Detail noise ===
    float detailNoise = FBM((uv + seedOffset) * 2000.0, 3, 0.5, 2.0) * 0.02;
    height += detailNoise;

    // === Island mask (falloff at edges) ===
    float islandMask = IslandMask(uv);
    height *= islandMask;

    // === Beach smoothing ===
    // Create flatter beach areas near water level
    float beachBlend = smoothstep(g_waterLevel - 0.05, g_waterLevel + 0.1, height);
    float flattenedHeight = lerp(g_waterLevel - 0.02, height, beachBlend);
    height = lerp(flattenedHeight, height, smoothstep(0.0, 0.1, abs(height - g_waterLevel)));

    // === Clamp and normalize ===
    height = saturate(height);

    // Write to heightmap
    // Height value in [0,1] will be scaled by g_heightScale when rendering
    g_heightmap[texCoord] = height;
}
