#include "IslandGenerator.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <queue>
#include <unordered_set>

// ============================================================================
// IslandData Helper Methods
// ============================================================================

float IslandData::getHeight(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return 0.0f;
    }
    return heightmap[y * width + x];
}

float IslandData::getHeightBilinear(float u, float v) const {
    // Convert normalized coords to pixel coords
    float px = u * (width - 1);
    float py = v * (height - 1);

    int x0 = static_cast<int>(std::floor(px));
    int y0 = static_cast<int>(std::floor(py));
    int x1 = std::min(x0 + 1, width - 1);
    int y1 = std::min(y0 + 1, height - 1);

    float tx = px - x0;
    float ty = py - y0;

    float h00 = getHeight(x0, y0);
    float h10 = getHeight(x1, y0);
    float h01 = getHeight(x0, y1);
    float h11 = getHeight(x1, y1);

    float h0 = h00 + (h10 - h00) * tx;
    float h1 = h01 + (h11 - h01) * tx;

    return h0 + (h1 - h0) * ty;
}

CoastalFeature IslandData::getCoastalType(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return CoastalFeature::BEACH;
    }
    return static_cast<CoastalFeature>(coastalTypeMap[y * width + x]);
}

bool IslandData::isLand(int x, int y) const {
    return getHeight(x, y) > params.waterLevel;
}

bool IslandData::isWater(int x, int y) const {
    return getHeight(x, y) <= params.waterLevel;
}

bool IslandData::isUnderwater(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return true;
    }
    return underwaterHeightmap[y * width + x] < params.waterLevel;
}

// ============================================================================
// IslandGenerator Implementation
// ============================================================================

IslandGenerator::IslandGenerator()
    : m_currentSeed(12345) {
    m_perm.resize(512);
    initializeNoise(m_currentSeed);
}

IslandGenerator::~IslandGenerator() = default;

void IslandGenerator::initializeNoise(uint32_t seed) {
    m_currentSeed = seed;
    m_rng.seed(seed);

    // Initialize permutation table with values 0-255
    std::vector<int> base(256);
    std::iota(base.begin(), base.end(), 0);

    // Shuffle using Fisher-Yates
    for (int i = 255; i > 0; --i) {
        std::uniform_int_distribution<int> dist(0, i);
        int j = dist(m_rng);
        std::swap(base[i], base[j]);
    }

    // Duplicate for wraparound
    for (int i = 0; i < 256; ++i) {
        m_perm[i] = base[i];
        m_perm[i + 256] = base[i];
    }
}

// ============================================================================
// Noise Functions
// ============================================================================

namespace {
    inline float fade(float t) {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    inline float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    inline float grad(int hash, float x, float y) {
        int h = hash & 7;
        float u = h < 4 ? x : y;
        float v = h < 4 ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
    }

    inline float smoothstep(float edge0, float edge1, float x) {
        float t = std::max(0.0f, std::min(1.0f, (x - edge0) / (edge1 - edge0)));
        return t * t * (3.0f - 2.0f * t);
    }
}

float IslandGenerator::perlin2D(float x, float y) const {
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;

    x -= std::floor(x);
    y -= std::floor(y);

    float u = fade(x);
    float v = fade(y);

    int A = m_perm[X] + Y;
    int B = m_perm[X + 1] + Y;

    return lerp(
        lerp(grad(m_perm[A], x, y), grad(m_perm[B], x - 1, y), u),
        lerp(grad(m_perm[A + 1], x, y - 1), grad(m_perm[B + 1], x - 1, y - 1), u),
        v
    );
}

float IslandGenerator::fbm(float x, float y, int octaves, float persistence, float lacunarity) const {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        total += perlin2D(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return total / maxValue;
}

float IslandGenerator::ridgedNoise(float x, float y, int octaves) const {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        float n = perlin2D(x * frequency, y * frequency);
        n = 1.0f - std::abs(n);  // Create ridges
        n = n * n;  // Sharpen ridges
        total += n * amplitude;
        maxValue += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }

    return total / maxValue;
}

float IslandGenerator::voronoi(float x, float y, float& cellId) const {
    int xi = static_cast<int>(std::floor(x));
    int yi = static_cast<int>(std::floor(y));

    float minDist = 999999.0f;
    cellId = 0.0f;

    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            int cx = xi + dx;
            int cy = yi + dy;

            // Hash cell coordinates to get feature point offset
            int hash = m_perm[(m_perm[cx & 255] + cy) & 255];
            float fx = static_cast<float>(cx) + (hash & 127) / 127.0f;
            float fy = static_cast<float>(cy) + ((hash >> 7) & 127) / 127.0f;

            float dist = (fx - x) * (fx - x) + (fy - y) * (fy - y);
            if (dist < minDist) {
                minDist = dist;
                cellId = static_cast<float>(hash);
            }
        }
    }

    return std::sqrt(minDist);
}

float IslandGenerator::domainWarp(float x, float y, float strength) const {
    float warpX = fbm(x + 5.3f, y + 1.3f, 4, 0.5f, 2.0f) * strength;
    float warpY = fbm(x + 1.7f, y + 9.2f, 4, 0.5f, 2.0f) * strength;
    return fbm(x + warpX, y + warpY, 6, 0.5f, 2.0f);
}

// ============================================================================
// Main Generation Methods
// ============================================================================

IslandData IslandGenerator::generate(const IslandGenParams& params, int width, int height) {
    initializeNoise(params.seed);

    switch (params.shape) {
        case IslandShape::CIRCULAR:
            return generateCircular(params, width);
        case IslandShape::ARCHIPELAGO:
            return generateArchipelago(params, width);
        case IslandShape::CRESCENT:
            return generateCrescent(params, width);
        case IslandShape::IRREGULAR:
            return generateIrregular(params, width);
        case IslandShape::VOLCANIC:
            return generateVolcanic(params, width);
        case IslandShape::ATOLL:
            return generateAtoll(params, width);
        case IslandShape::CONTINENTAL:
            return generateContinental(params, width);
        default:
            return generateIrregular(params, width);
    }
}

IslandData IslandGenerator::generate(uint32_t seed, int width, int height) {
    IslandGenParams params;
    params.seed = seed;
    return generate(params, width, height);
}

IslandData IslandGenerator::generateRandom(int width, int height) {
    std::random_device rd;
    uint32_t seed = rd();
    IslandGenParams params = randomParams(seed);
    return generate(params, width, height);
}

IslandShape IslandGenerator::randomShape(std::mt19937& rng) {
    std::uniform_int_distribution<int> dist(0, 6);
    return static_cast<IslandShape>(dist(rng));
}

IslandGenParams IslandGenerator::randomParams(uint32_t seed) {
    std::mt19937 rng(seed);
    IslandGenParams params;

    params.seed = seed;
    params.shape = randomShape(rng);

    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

    params.islandRadius = 0.3f + dist01(rng) * 0.3f;
    params.coastalIrregularity = 0.1f + dist01(rng) * 0.4f;
    params.coastalErosion = 0.3f + dist01(rng) * 0.4f;
    params.mountainousness = 0.3f + dist01(rng) * 0.5f;
    params.riverDensity = 0.1f + dist01(rng) * 0.4f;
    params.lakeDensity = 0.1f + dist01(rng) * 0.3f;

    std::uniform_int_distribution<int> islandCountDist(3, 8);
    params.archipelagoIslandCount = islandCountDist(rng);
    params.archipelagoSpread = 0.4f + dist01(rng) * 0.4f;

    params.volcanoHeight = 1.0f + dist01(rng) * 1.0f;
    params.craterSize = 0.1f + dist01(rng) * 0.15f;
    params.hasLavaFlows = dist01(rng) > 0.5f;

    params.lagoonDepth = 0.2f + dist01(rng) * 0.2f;
    params.reefWidth = 0.05f + dist01(rng) * 0.1f;

    return params;
}

// ============================================================================
// Shape Mask Generators
// ============================================================================

void IslandGenerator::generateCircularMask(std::vector<float>& mask, int size, float radius, float irregularity) {
    mask.resize(size * size);
    float center = size / 2.0f;
    float maxDist = size * radius;

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = x - center;
            float dy = y - center;
            float dist = std::sqrt(dx * dx + dy * dy);

            // Add angular variation for irregular coastline
            float angle = std::atan2(dy, dx);
            float noiseVal = fbm(std::cos(angle) * 3.0f, std::sin(angle) * 3.0f, 4, 0.5f, 2.0f);
            float adjustedRadius = maxDist * (1.0f + noiseVal * irregularity);

            float value = 1.0f - smoothstep(adjustedRadius * 0.7f, adjustedRadius, dist);
            mask[y * size + x] = value;
        }
    }
}

void IslandGenerator::generateArchipelagoMask(std::vector<float>& mask, int size, int islandCount, float spread, float irregularity) {
    mask.resize(size * size, 0.0f);
    float center = size / 2.0f;

    // Generate island centers
    std::vector<glm::vec2> centers;
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);
    std::uniform_real_distribution<float> radiusDist(0.1f, spread);
    std::uniform_real_distribution<float> sizeDist(0.1f, 0.25f);

    // Main island at center
    centers.push_back(glm::vec2(center, center));

    // Satellite islands
    for (int i = 1; i < islandCount; ++i) {
        float angle = angleDist(m_rng);
        float r = radiusDist(m_rng) * size * 0.4f;
        centers.push_back(glm::vec2(
            center + std::cos(angle) * r,
            center + std::sin(angle) * r
        ));
    }

    // Render each island
    for (size_t i = 0; i < centers.size(); ++i) {
        float islandSize = (i == 0 ? 0.2f : sizeDist(m_rng)) * size;

        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                float dx = x - centers[i].x;
                float dy = y - centers[i].y;
                float dist = std::sqrt(dx * dx + dy * dy);

                float angle = std::atan2(dy, dx);
                float noiseVal = fbm(std::cos(angle) * 4.0f + i * 10.0f, std::sin(angle) * 4.0f, 3, 0.5f, 2.0f);
                float adjustedRadius = islandSize * (1.0f + noiseVal * irregularity);

                float value = 1.0f - smoothstep(adjustedRadius * 0.6f, adjustedRadius, dist);
                mask[y * size + x] = std::max(mask[y * size + x], value);
            }
        }
    }
}

void IslandGenerator::generateCrescentMask(std::vector<float>& mask, int size, float radius, float irregularity) {
    mask.resize(size * size);
    float center = size / 2.0f;
    float mainRadius = size * radius;
    float cutoutRadius = mainRadius * 0.7f;
    float cutoutOffset = mainRadius * 0.4f;

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = x - center;
            float dy = y - center;
            float distMain = std::sqrt(dx * dx + dy * dy);

            // Cutout circle offset to create crescent
            float cutoutX = center + cutoutOffset;
            float dxCut = x - cutoutX;
            float distCutout = std::sqrt(dxCut * dxCut + dy * dy);

            // Add noise for irregularity
            float nx = static_cast<float>(x) / size * 5.0f;
            float ny = static_cast<float>(y) / size * 5.0f;
            float noiseVal = fbm(nx, ny, 4, 0.5f, 2.0f) * irregularity * mainRadius * 0.3f;

            float mainValue = 1.0f - smoothstep(mainRadius * 0.7f + noiseVal, mainRadius + noiseVal, distMain);
            float cutoutValue = smoothstep(cutoutRadius * 0.8f, cutoutRadius, distCutout);

            mask[y * size + x] = mainValue * cutoutValue;
        }
    }
}

void IslandGenerator::generateIrregularMask(std::vector<float>& mask, int size, float coverage, float irregularity) {
    mask.resize(size * size);
    float center = size / 2.0f;

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float nx = static_cast<float>(x) / size;
            float ny = static_cast<float>(y) / size;

            // Distance from center for island falloff
            float dx = (x - center) / center;
            float dy = (y - center) / center;
            float dist = std::sqrt(dx * dx + dy * dy);

            // Domain-warped noise for organic shapes
            float warpedNoise = domainWarp(nx * 4.0f, ny * 4.0f, irregularity * 2.0f);

            // Combine noise with radial falloff
            float baseShape = 1.0f - smoothstep(coverage * 0.5f, coverage, dist);
            float noiseShape = (warpedNoise + 1.0f) * 0.5f;

            float value = baseShape * (0.3f + noiseShape * 0.7f);
            value = smoothstep(0.3f, 0.5f, value);

            mask[y * size + x] = value;
        }
    }
}

void IslandGenerator::generateVolcanicMask(std::vector<float>& mask, int size, float radius, float craterSize) {
    mask.resize(size * size);
    float center = size / 2.0f;
    float mainRadius = size * radius;
    float craterRadius = mainRadius * craterSize;

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = x - center;
            float dy = y - center;
            float dist = std::sqrt(dx * dx + dy * dy);

            // Main island shape
            float islandValue = 1.0f - smoothstep(mainRadius * 0.6f, mainRadius, dist);

            // Volcano peak (rises toward center, then drops for crater)
            float peakDist = dist / mainRadius;
            float peakValue = 0.0f;

            if (peakDist < 0.3f) {
                // Crater floor
                float craterFloor = smoothstep(0.0f, craterSize, peakDist);
                peakValue = 0.5f + craterFloor * 0.5f;
            } else if (peakDist < 0.5f) {
                // Crater rim (highest point)
                float rimDist = (peakDist - 0.3f) / 0.2f;
                peakValue = 1.0f - rimDist * 0.3f;
            } else {
                // Slopes down to coast
                float slopeDist = (peakDist - 0.5f) / 0.5f;
                peakValue = 0.7f - slopeDist * 0.5f;
            }

            mask[y * size + x] = islandValue * std::max(0.0f, peakValue);
        }
    }
}

void IslandGenerator::generateAtollMask(std::vector<float>& mask, int size, float radius, float lagoonSize, float reefWidth) {
    mask.resize(size * size);
    float center = size / 2.0f;
    float outerRadius = size * radius;
    float innerRadius = outerRadius * (1.0f - reefWidth * 2.0f);
    float lagoonRadius = innerRadius * lagoonSize;

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = x - center;
            float dy = y - center;
            float dist = std::sqrt(dx * dx + dy * dy);

            // Angular noise for broken ring effect
            float angle = std::atan2(dy, dx);
            float noiseVal = fbm(std::cos(angle) * 5.0f, std::sin(angle) * 5.0f, 4, 0.5f, 2.0f);
            float gapNoise = std::abs(noiseVal);

            // Outer edge
            float outerValue = 1.0f - smoothstep(outerRadius * 0.9f, outerRadius, dist);

            // Inner edge (lagoon)
            float innerValue = smoothstep(lagoonRadius, innerRadius * 0.9f, dist);

            // Ring with gaps (some atolls have passages)
            float ringValue = outerValue * innerValue;

            // Add breaks in the ring at certain angles
            if (gapNoise > 0.7f && dist > lagoonRadius && dist < innerRadius) {
                ringValue *= (1.0f - (gapNoise - 0.7f) / 0.3f);
            }

            mask[y * size + x] = ringValue;
        }
    }
}

void IslandGenerator::generateContinentalMask(std::vector<float>& mask, int size, float coverage) {
    mask.resize(size * size);
    float center = size / 2.0f;

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float nx = static_cast<float>(x) / size;
            float ny = static_cast<float>(y) / size;

            // Distance from center
            float dx = (x - center) / center;
            float dy = (y - center) / center;
            float dist = std::sqrt(dx * dx + dy * dy);

            // Multiple noise layers for complex coastline
            float continental = fbm(nx * 2.0f, ny * 2.0f, 4, 0.6f, 2.0f);
            float detail = fbm(nx * 8.0f + 100.0f, ny * 8.0f + 100.0f, 3, 0.5f, 2.0f);

            // Voronoi for coastal features (bays, peninsulas)
            float cellId;
            float voronoiVal = voronoi(nx * 6.0f, ny * 6.0f, cellId);

            // Combine for organic continental shape
            float baseShape = 1.0f - smoothstep(coverage * 0.4f, coverage, dist);
            float noiseShape = (continental + 1.0f) * 0.5f;
            noiseShape = noiseShape * 0.7f + detail * 0.2f + (1.0f - voronoiVal) * 0.1f;

            float value = baseShape * (0.2f + noiseShape * 0.8f);
            value = smoothstep(0.25f, 0.45f, value);

            mask[y * size + x] = value;
        }
    }
}

// ============================================================================
// Shape-Specific Island Generators
// ============================================================================

IslandData IslandGenerator::generateCircular(const IslandGenParams& params, int size) {
    IslandData data;
    data.width = size;
    data.height = size;
    data.params = params;
    data.heightmap.resize(size * size);
    data.underwaterHeightmap.resize(size * size);
    data.coastalTypeMap.resize(size * size, static_cast<uint8_t>(CoastalFeature::BEACH));

    // Generate base mask
    std::vector<float> mask;
    generateCircularMask(mask, size, params.islandRadius, params.coastalIrregularity);

    // Add terrain features
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float nx = static_cast<float>(x) / size;
            float ny = static_cast<float>(y) / size;

            // Multi-octave terrain
            float mountains = ridgedNoise(nx * 4.0f, ny * 4.0f, 6) * params.mountainousness;
            float hills = fbm(nx * 8.0f + 50.0f, ny * 8.0f + 50.0f, 4, 0.5f, 2.0f) * 0.3f;

            float terrain = mountains * 0.6f + hills * 0.4f;
            float height = mask[y * size + x] * (0.4f + terrain * 0.6f);

            // Apply water level cutoff
            if (height < params.waterLevel) {
                height = height * 0.5f;  // Gradual underwater slope
            }

            data.heightmap[y * size + x] = height;
        }
    }

    // Post-processing
    applyCoastalErosion(data, static_cast<int>(params.coastalErosion * 10.0f));
    carveRivers(data);
    createLakes(data);
    generateUnderwaterTerrain(data);
    markCaveEntrances(data);
    smoothCoastlines(data, 3);

    // Generate coastal features
    generateBeaches(data.heightmap, data.coastalTypeMap, size, params.waterLevel);

    return data;
}

IslandData IslandGenerator::generateArchipelago(const IslandGenParams& params, int size) {
    IslandData data;
    data.width = size;
    data.height = size;
    data.params = params;
    data.heightmap.resize(size * size);
    data.underwaterHeightmap.resize(size * size);
    data.coastalTypeMap.resize(size * size, static_cast<uint8_t>(CoastalFeature::BEACH));

    std::vector<float> mask;
    generateArchipelagoMask(mask, size, params.archipelagoIslandCount, params.archipelagoSpread, params.coastalIrregularity);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float nx = static_cast<float>(x) / size;
            float ny = static_cast<float>(y) / size;

            float mountains = ridgedNoise(nx * 5.0f, ny * 5.0f, 5) * params.mountainousness;
            float hills = fbm(nx * 10.0f + 30.0f, ny * 10.0f + 30.0f, 3, 0.5f, 2.0f) * 0.25f;

            float terrain = mountains * 0.5f + hills * 0.5f;
            float height = mask[y * size + x] * (0.35f + terrain * 0.65f);

            if (height < params.waterLevel) {
                height = height * 0.4f;
            }

            data.heightmap[y * size + x] = height;
        }
    }

    applyCoastalErosion(data, static_cast<int>(params.coastalErosion * 8.0f));
    carveRivers(data);
    createLakes(data);
    generateUnderwaterTerrain(data);
    markCaveEntrances(data);
    smoothCoastlines(data, 2);
    generateBeaches(data.heightmap, data.coastalTypeMap, size, params.waterLevel);

    return data;
}

IslandData IslandGenerator::generateCrescent(const IslandGenParams& params, int size) {
    IslandData data;
    data.width = size;
    data.height = size;
    data.params = params;
    data.heightmap.resize(size * size);
    data.underwaterHeightmap.resize(size * size);
    data.coastalTypeMap.resize(size * size, static_cast<uint8_t>(CoastalFeature::BEACH));

    std::vector<float> mask;
    generateCrescentMask(mask, size, params.islandRadius, params.coastalIrregularity);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float nx = static_cast<float>(x) / size;
            float ny = static_cast<float>(y) / size;

            float mountains = ridgedNoise(nx * 4.0f + 20.0f, ny * 4.0f + 20.0f, 5) * params.mountainousness;
            float hills = fbm(nx * 7.0f, ny * 7.0f, 4, 0.5f, 2.0f) * 0.3f;

            float terrain = mountains * 0.55f + hills * 0.45f;
            float height = mask[y * size + x] * (0.38f + terrain * 0.62f);

            if (height < params.waterLevel) {
                height = height * 0.45f;
            }

            data.heightmap[y * size + x] = height;
        }
    }

    applyCoastalErosion(data, static_cast<int>(params.coastalErosion * 10.0f));
    carveRivers(data);
    createLakes(data);
    generateUnderwaterTerrain(data);
    markCaveEntrances(data);
    smoothCoastlines(data, 3);
    generateBeaches(data.heightmap, data.coastalTypeMap, size, params.waterLevel);

    return data;
}

IslandData IslandGenerator::generateIrregular(const IslandGenParams& params, int size) {
    IslandData data;
    data.width = size;
    data.height = size;
    data.params = params;
    data.heightmap.resize(size * size);
    data.underwaterHeightmap.resize(size * size);
    data.coastalTypeMap.resize(size * size, static_cast<uint8_t>(CoastalFeature::BEACH));

    std::vector<float> mask;
    generateIrregularMask(mask, size, params.islandRadius * 1.5f, params.coastalIrregularity);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float nx = static_cast<float>(x) / size;
            float ny = static_cast<float>(y) / size;

            float mountains = ridgedNoise(nx * 3.5f, ny * 3.5f, 6) * params.mountainousness;
            float hills = fbm(nx * 6.0f + 80.0f, ny * 6.0f + 80.0f, 4, 0.5f, 2.0f) * 0.35f;
            float valleys = 1.0f - std::abs(fbm(nx * 4.0f + 40.0f, ny * 4.0f + 40.0f, 3, 0.5f, 2.0f));

            float terrain = mountains * 0.5f + hills * 0.3f + valleys * 0.2f;
            float height = mask[y * size + x] * (0.35f + terrain * 0.65f);

            if (height < params.waterLevel) {
                height = height * 0.5f;
            }

            data.heightmap[y * size + x] = height;
        }
    }

    applyCoastalErosion(data, static_cast<int>(params.coastalErosion * 12.0f));
    carveRivers(data);
    createLakes(data);
    generateUnderwaterTerrain(data);
    markCaveEntrances(data);
    smoothCoastlines(data, 4);
    generateBeaches(data.heightmap, data.coastalTypeMap, size, params.waterLevel);

    return data;
}

IslandData IslandGenerator::generateVolcanic(const IslandGenParams& params, int size) {
    IslandData data;
    data.width = size;
    data.height = size;
    data.params = params;
    data.heightmap.resize(size * size);
    data.underwaterHeightmap.resize(size * size);
    data.coastalTypeMap.resize(size * size, static_cast<uint8_t>(CoastalFeature::CLIFF));

    std::vector<float> mask;
    generateVolcanicMask(mask, size, params.islandRadius, params.craterSize);

    float center = size / 2.0f;

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float nx = static_cast<float>(x) / size;
            float ny = static_cast<float>(y) / size;

            float dx = (x - center) / center;
            float dy = (y - center) / center;
            float dist = std::sqrt(dx * dx + dy * dy);

            // Volcanic terrain is mostly smooth with radial flows
            float radialNoise = fbm(nx * 8.0f + dist * 10.0f, ny * 8.0f + dist * 10.0f, 3, 0.5f, 2.0f) * 0.15f;
            float lavaChannels = 0.0f;

            if (params.hasLavaFlows) {
                float angle = std::atan2(dy, dx);
                float channelNoise = std::sin(angle * 5.0f + dist * 20.0f);
                lavaChannels = std::max(0.0f, channelNoise) * 0.1f * (1.0f - dist);
            }

            float height = mask[y * size + x] * params.volcanoHeight;
            height += radialNoise - lavaChannels;
            height = std::max(0.0f, height);

            if (height < params.waterLevel && height > 0.0f) {
                height = height * 0.3f;
            }

            data.heightmap[y * size + x] = std::min(1.0f, height);
        }
    }

    applyCoastalErosion(data, static_cast<int>(params.coastalErosion * 5.0f));
    generateUnderwaterTerrain(data);
    markCaveEntrances(data);
    smoothCoastlines(data, 2);
    generateCliffs(data.heightmap, data.coastalTypeMap, size, params.waterLevel);

    return data;
}

IslandData IslandGenerator::generateAtoll(const IslandGenParams& params, int size) {
    IslandData data;
    data.width = size;
    data.height = size;
    data.params = params;
    data.heightmap.resize(size * size);
    data.underwaterHeightmap.resize(size * size);
    data.coastalTypeMap.resize(size * size, static_cast<uint8_t>(CoastalFeature::REEF));

    std::vector<float> mask;
    generateAtollMask(mask, size, params.islandRadius, 1.0f - params.lagoonDepth, params.reefWidth);

    float center = size / 2.0f;
    float lagoonRadius = size * params.islandRadius * (1.0f - params.reefWidth * 2.0f) * (1.0f - params.lagoonDepth);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float nx = static_cast<float>(x) / size;
            float ny = static_cast<float>(y) / size;

            float dx = x - center;
            float dy = y - center;
            float dist = std::sqrt(dx * dx + dy * dy);

            // Coral reef texture
            float reefNoise = fbm(nx * 20.0f, ny * 20.0f, 4, 0.6f, 2.0f) * 0.1f;

            float height = mask[y * size + x] * (0.45f + reefNoise);

            // Lagoon floor
            if (dist < lagoonRadius) {
                float lagoonFloor = params.waterLevel * 0.6f;
                float lagoonNoise = fbm(nx * 15.0f, ny * 15.0f, 3, 0.5f, 2.0f) * 0.1f;
                height = std::max(height, lagoonFloor + lagoonNoise);
            }

            data.heightmap[y * size + x] = height;
        }
    }

    generateUnderwaterTerrain(data);
    smoothCoastlines(data, 2);

    // Mark reef areas
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float h = data.heightmap[y * size + x];
            if (h > params.waterLevel * 0.7f && h < params.waterLevel * 1.2f) {
                data.coastalTypeMap[y * size + x] = static_cast<uint8_t>(CoastalFeature::REEF);
            }
        }
    }

    return data;
}

IslandData IslandGenerator::generateContinental(const IslandGenParams& params, int size) {
    IslandData data;
    data.width = size;
    data.height = size;
    data.params = params;
    data.heightmap.resize(size * size);
    data.underwaterHeightmap.resize(size * size);
    data.coastalTypeMap.resize(size * size, static_cast<uint8_t>(CoastalFeature::BEACH));

    std::vector<float> mask;
    generateContinentalMask(mask, size, params.islandRadius * 1.8f);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float nx = static_cast<float>(x) / size;
            float ny = static_cast<float>(y) / size;

            // Continental terrain with mountain ranges
            float mountains = ridgedNoise(nx * 3.0f, ny * 3.0f, 7) * params.mountainousness;
            float hills = fbm(nx * 6.0f + 100.0f, ny * 6.0f + 100.0f, 5, 0.55f, 2.0f) * 0.35f;
            float plains = fbm(nx * 2.0f + 50.0f, ny * 2.0f + 50.0f, 3, 0.5f, 2.0f) * 0.15f;

            // Mountain range bias (linear feature across the continent)
            float rangeBias = std::sin(nx * 3.14159f + ny * 2.0f) * 0.3f + 0.7f;
            mountains *= rangeBias;

            float terrain = mountains * 0.45f + hills * 0.35f + plains * 0.2f;
            float height = mask[y * size + x] * (0.3f + terrain * 0.7f);

            if (height < params.waterLevel) {
                height = height * 0.5f;
            }

            data.heightmap[y * size + x] = height;
        }
    }

    applyCoastalErosion(data, static_cast<int>(params.coastalErosion * 15.0f));
    carveRivers(data);
    createLakes(data);
    generateUnderwaterTerrain(data);
    markCaveEntrances(data);
    smoothCoastlines(data, 5);
    generateBeaches(data.heightmap, data.coastalTypeMap, size, params.waterLevel);
    generateCliffs(data.heightmap, data.coastalTypeMap, size, params.waterLevel);

    return data;
}

// ============================================================================
// Terrain Feature Generation
// ============================================================================

void IslandGenerator::generateMountains(std::vector<float>& heightmap, int size, float intensity) {
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float nx = static_cast<float>(x) / size;
            float ny = static_cast<float>(y) / size;

            float mountain = ridgedNoise(nx * 4.0f, ny * 4.0f, 6);
            heightmap[y * size + x] += mountain * intensity * 0.3f;
        }
    }
}

void IslandGenerator::generateValleys(std::vector<float>& heightmap, int size, float intensity) {
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float nx = static_cast<float>(x) / size;
            float ny = static_cast<float>(y) / size;

            float valley = 1.0f - std::abs(fbm(nx * 3.0f, ny * 3.0f, 4, 0.5f, 2.0f));
            valley = valley * valley;
            heightmap[y * size + x] -= valley * intensity * 0.15f;
            heightmap[y * size + x] = std::max(0.0f, heightmap[y * size + x]);
        }
    }
}

void IslandGenerator::generatePlateaus(std::vector<float>& heightmap, int size, float intensity) {
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float h = heightmap[y * size + x];

            // Create stepped terrain effect
            float step = 0.15f;
            float stepped = std::floor(h / step) * step;
            float blend = smoothstep(0.0f, step * 0.3f, h - stepped);

            heightmap[y * size + x] = lerp(stepped, h, 1.0f - intensity + blend * intensity);
        }
    }
}

void IslandGenerator::generateBeaches(std::vector<float>& heightmap, std::vector<uint8_t>& coastalMap, int size, float waterLevel) {
    float beachLow = waterLevel * 0.95f;
    float beachHigh = waterLevel * 1.15f;

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float h = heightmap[y * size + x];

            if (h > beachLow && h < beachHigh) {
                // Check if near water
                bool nearWater = false;
                for (int dy = -3; dy <= 3 && !nearWater; ++dy) {
                    for (int dx = -3; dx <= 3 && !nearWater; ++dx) {
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 0 && nx < size && ny >= 0 && ny < size) {
                            if (heightmap[ny * size + nx] < waterLevel) {
                                nearWater = true;
                            }
                        }
                    }
                }

                if (nearWater) {
                    coastalMap[y * size + x] = static_cast<uint8_t>(CoastalFeature::BEACH);
                    // Smooth beach transition
                    float beachBlend = (h - beachLow) / (beachHigh - beachLow);
                    heightmap[y * size + x] = beachLow + beachBlend * (beachHigh - beachLow) * 0.5f;
                }
            }
        }
    }
}

void IslandGenerator::generateCliffs(std::vector<float>& heightmap, std::vector<uint8_t>& coastalMap, int size, float waterLevel) {
    for (int y = 1; y < size - 1; ++y) {
        for (int x = 1; x < size - 1; ++x) {
            float h = heightmap[y * size + x];

            if (h > waterLevel) {
                // Calculate slope
                float hL = heightmap[y * size + (x - 1)];
                float hR = heightmap[y * size + (x + 1)];
                float hU = heightmap[(y - 1) * size + x];
                float hD = heightmap[(y + 1) * size + x];

                float slope = std::max({
                    std::abs(h - hL),
                    std::abs(h - hR),
                    std::abs(h - hU),
                    std::abs(h - hD)
                });

                // Steep slope near water = cliff
                if (slope > 0.15f) {
                    bool nearWater = (hL < waterLevel || hR < waterLevel || hU < waterLevel || hD < waterLevel);
                    if (nearWater) {
                        coastalMap[y * size + x] = static_cast<uint8_t>(CoastalFeature::CLIFF);
                    }
                }
            }
        }
    }
}

// ============================================================================
// River Generation
// ============================================================================

std::vector<RiverSegment> IslandGenerator::traceRivers(const std::vector<float>& heightmap, int size, int riverCount) {
    std::vector<RiverSegment> rivers;

    // Find high points to start rivers
    std::vector<std::pair<float, glm::ivec2>> highPoints;

    for (int y = size / 4; y < size * 3 / 4; ++y) {
        for (int x = size / 4; x < size * 3 / 4; ++x) {
            float h = heightmap[y * size + x];
            if (h > 0.6f) {
                highPoints.push_back({h, glm::ivec2(x, y)});
            }
        }
    }

    // Sort by height
    std::sort(highPoints.begin(), highPoints.end(), [](const auto& a, const auto& b) {
        return a.first > b.first;
    });

    // Trace rivers from high points
    int tracedCount = 0;
    std::vector<bool> visited(size * size, false);

    for (const auto& startPoint : highPoints) {
        if (tracedCount >= riverCount) break;

        glm::ivec2 current = startPoint.second;
        if (visited[current.y * size + current.x]) continue;

        std::vector<glm::ivec2> path;
        path.push_back(current);

        // Gradient descent to water
        while (true) {
            visited[current.y * size + current.x] = true;

            float currentHeight = heightmap[current.y * size + current.x];

            // Check if we've reached water
            if (currentHeight < 0.35f) break;

            // Find lowest neighbor
            glm::ivec2 lowestNeighbor = current;
            float lowestHeight = currentHeight;

            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;

                    int nx = current.x + dx;
                    int ny = current.y + dy;

                    if (nx >= 0 && nx < size && ny >= 0 && ny < size) {
                        float nh = heightmap[ny * size + nx];
                        if (nh < lowestHeight) {
                            lowestHeight = nh;
                            lowestNeighbor = glm::ivec2(nx, ny);
                        }
                    }
                }
            }

            // Check if stuck (no lower neighbor)
            if (lowestNeighbor == current) break;

            current = lowestNeighbor;
            path.push_back(current);

            // Limit path length
            if (path.size() > static_cast<size_t>(size)) break;
        }

        // Create river segments from path
        if (path.size() >= 10) {
            for (size_t i = 0; i < path.size() - 1; ++i) {
                RiverSegment seg;
                seg.start = glm::vec2(path[i]) / static_cast<float>(size);
                seg.end = glm::vec2(path[i + 1]) / static_cast<float>(size);

                // Width increases downstream
                float progress = static_cast<float>(i) / path.size();
                seg.width = 0.001f + progress * 0.005f;
                seg.depth = 0.02f + progress * 0.03f;
                seg.order = 1;

                rivers.push_back(seg);
            }
            tracedCount++;
        }
    }

    return rivers;
}

void IslandGenerator::carveRiverBed(std::vector<float>& heightmap, int size, const RiverSegment& river) {
    glm::vec2 start = river.start * static_cast<float>(size);
    glm::vec2 end = river.end * static_cast<float>(size);
    glm::vec2 dir = end - start;
    float length = glm::length(dir);

    if (length < 0.001f) return;
    dir /= length;

    float widthPixels = river.width * size;

    for (float t = 0; t <= length; t += 0.5f) {
        glm::vec2 pos = start + dir * t;
        int cx = static_cast<int>(pos.x);
        int cy = static_cast<int>(pos.y);

        int radius = static_cast<int>(std::ceil(widthPixels));

        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                int nx = cx + dx;
                int ny = cy + dy;

                if (nx >= 0 && nx < size && ny >= 0 && ny < size) {
                    float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                    if (dist <= widthPixels) {
                        float factor = 1.0f - (dist / widthPixels);
                        factor = factor * factor;
                        heightmap[ny * size + nx] -= river.depth * factor;
                        heightmap[ny * size + nx] = std::max(0.0f, heightmap[ny * size + nx]);
                    }
                }
            }
        }
    }
}

// ============================================================================
// Lake Generation
// ============================================================================

std::vector<LakeBasin> IslandGenerator::findLakeBasins(const std::vector<float>& heightmap, int size, float probability) {
    std::vector<LakeBasin> lakes;
    std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> sizeDist(0.01f, 0.05f);

    // Look for local minima above water level
    for (int y = size / 5; y < size * 4 / 5; y += size / 20) {
        for (int x = size / 5; x < size * 4 / 5; x += size / 20) {
            float h = heightmap[y * size + x];

            if (h > 0.4f && h < 0.7f && probDist(m_rng) < probability) {
                // Check if this is a local depression
                bool isDepression = true;
                float minNeighbor = h;

                for (int dy = -3; dy <= 3 && isDepression; ++dy) {
                    for (int dx = -3; dx <= 3 && isDepression; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 0 && nx < size && ny >= 0 && ny < size) {
                            float nh = heightmap[ny * size + nx];
                            if (nh < h - 0.05f) {
                                isDepression = false;
                            }
                            minNeighbor = std::min(minNeighbor, nh);
                        }
                    }
                }

                if (isDepression || probDist(m_rng) < 0.3f) {
                    LakeBasin lake;
                    lake.center = glm::vec2(x, y) / static_cast<float>(size);
                    lake.radius = sizeDist(m_rng);
                    lake.depth = 0.05f + probDist(m_rng) * 0.1f;
                    lake.elevation = h;
                    lake.isVolcanic = false;
                    lakes.push_back(lake);
                }
            }
        }
    }

    return lakes;
}

void IslandGenerator::fillLakeBasin(std::vector<float>& heightmap, int size, const LakeBasin& lake) {
    glm::vec2 center = lake.center * static_cast<float>(size);
    float radiusPixels = lake.radius * size;

    int cx = static_cast<int>(center.x);
    int cy = static_cast<int>(center.y);
    int radius = static_cast<int>(std::ceil(radiusPixels));

    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int nx = cx + dx;
            int ny = cy + dy;

            if (nx >= 0 && nx < size && ny >= 0 && ny < size) {
                float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                if (dist <= radiusPixels) {
                    float factor = 1.0f - (dist / radiusPixels);
                    factor = factor * factor;

                    float targetHeight = lake.elevation - lake.depth * factor;
                    heightmap[ny * size + nx] = std::min(heightmap[ny * size + nx], targetHeight);
                    heightmap[ny * size + nx] = std::max(0.0f, heightmap[ny * size + nx]);
                }
            }
        }
    }
}

// ============================================================================
// Cave Entrance Detection
// ============================================================================

std::vector<CaveEntrance> IslandGenerator::findCaveLocations(const std::vector<float>& heightmap, int size) {
    std::vector<CaveEntrance> caves;

    for (int y = 2; y < size - 2; y += 5) {
        for (int x = 2; x < size - 2; x += 5) {
            float h = heightmap[y * size + x];

            // Caves form near water on steep slopes
            if (h > 0.35f && h < 0.55f) {
                // Calculate slope
                float hL = heightmap[y * size + (x - 1)];
                float hR = heightmap[y * size + (x + 1)];
                float hU = heightmap[(y - 1) * size + x];
                float hD = heightmap[(y + 1) * size + x];

                glm::vec2 gradient(hR - hL, hD - hU);
                float slope = glm::length(gradient);

                // Steep slope near water level
                if (slope > 0.1f) {
                    // Check for nearby water
                    bool nearWater = false;
                    for (int dy = -5; dy <= 5 && !nearWater; ++dy) {
                        for (int dx = -5; dx <= 5 && !nearWater; ++dx) {
                            int nx = x + dx;
                            int ny = y + dy;
                            if (nx >= 0 && nx < size && ny >= 0 && ny < size) {
                                if (heightmap[ny * size + nx] < 0.35f) {
                                    nearWater = true;
                                }
                            }
                        }
                    }

                    if (nearWater) {
                        CaveEntrance cave;
                        cave.position = glm::vec3(
                            static_cast<float>(x) / size,
                            h,
                            static_cast<float>(y) / size
                        );

                        // Direction into the hillside (opposite of gradient)
                        if (slope > 0.001f) {
                            cave.direction = glm::normalize(glm::vec3(-gradient.x, 0.0f, -gradient.y));
                        } else {
                            cave.direction = glm::vec3(1.0f, 0.0f, 0.0f);
                        }

                        cave.size = 0.01f + (slope - 0.1f) * 0.05f;
                        caves.push_back(cave);
                    }
                }
            }
        }
    }

    return caves;
}

// ============================================================================
// Erosion Simulation
// ============================================================================

void IslandGenerator::thermalErosion(std::vector<float>& heightmap, int size, int iterations) {
    const float talusAngle = 0.05f;  // Maximum stable slope

    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<float> deltas(size * size, 0.0f);

        for (int y = 1; y < size - 1; ++y) {
            for (int x = 1; x < size - 1; ++x) {
                float h = heightmap[y * size + x];

                float maxDiff = 0.0f;
                int maxDir = -1;

                // Check 4 neighbors
                int dx[] = {-1, 1, 0, 0};
                int dy[] = {0, 0, -1, 1};

                for (int d = 0; d < 4; ++d) {
                    float nh = heightmap[(y + dy[d]) * size + (x + dx[d])];
                    float diff = h - nh;
                    if (diff > talusAngle && diff > maxDiff) {
                        maxDiff = diff;
                        maxDir = d;
                    }
                }

                if (maxDir >= 0) {
                    float transfer = (maxDiff - talusAngle) * 0.5f;
                    deltas[y * size + x] -= transfer;
                    deltas[(y + dy[maxDir]) * size + (x + dx[maxDir])] += transfer;
                }
            }
        }

        // Apply deltas
        for (int i = 0; i < size * size; ++i) {
            heightmap[i] += deltas[i];
            heightmap[i] = std::max(0.0f, heightmap[i]);
        }
    }
}

void IslandGenerator::hydraulicErosion(std::vector<float>& heightmap, int size, int iterations) {
    std::vector<float> water(size * size, 0.0f);
    std::vector<float> sediment(size * size, 0.0f);

    std::uniform_real_distribution<float> rainDist(0.0f, 1.0f);

    for (int iter = 0; iter < iterations; ++iter) {
        // Rain
        for (int i = 0; i < size * size; ++i) {
            if (rainDist(m_rng) < 0.01f) {
                water[i] += 0.01f;
            }
        }

        // Erosion and flow
        for (int y = 1; y < size - 1; ++y) {
            for (int x = 1; x < size - 1; ++x) {
                int idx = y * size + x;

                if (water[idx] < 0.001f) continue;

                float h = heightmap[idx] + water[idx];

                // Find lowest neighbor
                float lowestH = h;
                int lowestIdx = idx;

                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        int nidx = (y + dy) * size + (x + dx);
                        float nh = heightmap[nidx] + water[nidx];
                        if (nh < lowestH) {
                            lowestH = nh;
                            lowestIdx = nidx;
                        }
                    }
                }

                if (lowestIdx != idx) {
                    float flow = std::min(water[idx], (h - lowestH) * 0.5f);

                    // Erode
                    float erosion = flow * 0.1f;
                    heightmap[idx] -= erosion;
                    sediment[idx] += erosion;

                    // Move water and sediment
                    water[idx] -= flow;
                    water[lowestIdx] += flow * 0.9f;  // Some evaporation

                    float sedFlow = sediment[idx] * 0.3f;
                    sediment[idx] -= sedFlow;
                    sediment[lowestIdx] += sedFlow;
                }
            }
        }

        // Deposit sediment where water evaporates
        for (int i = 0; i < size * size; ++i) {
            water[i] *= 0.95f;  // Evaporation

            if (water[i] < 0.001f && sediment[i] > 0.0f) {
                heightmap[i] += sediment[i] * 0.5f;
                sediment[i] *= 0.5f;
            }
        }
    }
}

void IslandGenerator::coastalErosion(std::vector<float>& heightmap, int size, float waterLevel, int iterations) {
    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<float> erosion(size * size, 0.0f);

        for (int y = 1; y < size - 1; ++y) {
            for (int x = 1; x < size - 1; ++x) {
                float h = heightmap[y * size + x];

                // Only erode near water level
                if (h > waterLevel && h < waterLevel + 0.15f) {
                    // Count water neighbors
                    int waterNeighbors = 0;
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            if (heightmap[(y + dy) * size + (x + dx)] < waterLevel) {
                                waterNeighbors++;
                            }
                        }
                    }

                    if (waterNeighbors > 0) {
                        erosion[y * size + x] = waterNeighbors * 0.002f;
                    }
                }
            }
        }

        // Apply erosion
        for (int i = 0; i < size * size; ++i) {
            heightmap[i] -= erosion[i];
            heightmap[i] = std::max(0.0f, heightmap[i]);
        }
    }
}

// ============================================================================
// Underwater Terrain
// ============================================================================

void IslandGenerator::generateSeafloor(std::vector<float>& underwater, int size, float maxDepth) {
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float nx = static_cast<float>(x) / size;
            float ny = static_cast<float>(y) / size;

            // Base seafloor with gentle undulations
            float base = fbm(nx * 3.0f + 200.0f, ny * 3.0f + 200.0f, 4, 0.5f, 2.0f);
            base = (base + 1.0f) * 0.5f;

            // Add some ridge features
            float ridges = ridgedNoise(nx * 5.0f + 300.0f, ny * 5.0f + 300.0f, 3) * 0.2f;

            // Depth increases toward edges
            float dx = (x - size / 2.0f) / (size / 2.0f);
            float dy = (y - size / 2.0f) / (size / 2.0f);
            float distFromCenter = std::sqrt(dx * dx + dy * dy);
            float depthFactor = smoothstep(0.3f, 1.0f, distFromCenter);

            float depth = maxDepth * (0.3f + depthFactor * 0.7f);
            underwater[y * size + x] = -depth * (base * 0.7f + ridges + 0.3f);
        }
    }
}

void IslandGenerator::generateCoralReefs(std::vector<float>& underwater, int size, float waterLevel) {
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float depth = -underwater[y * size + x];

            // Coral grows in shallow warm water
            if (depth > 0.0f && depth < 0.15f) {
                float nx = static_cast<float>(x) / size;
                float ny = static_cast<float>(y) / size;

                float coralNoise = fbm(nx * 30.0f, ny * 30.0f, 4, 0.6f, 2.0f);

                if (coralNoise > 0.2f) {
                    float coralHeight = (coralNoise - 0.2f) * 0.1f;
                    underwater[y * size + x] += coralHeight;
                }
            }
        }
    }
}

void IslandGenerator::generateKelpForests(std::vector<float>& underwater, int size, float waterLevel) {
    // Kelp forests are marked by slightly elevated seafloor areas
    // The actual kelp would be rendered separately
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float depth = -underwater[y * size + x];

            // Kelp grows in medium depth
            if (depth > 0.05f && depth < 0.2f) {
                float nx = static_cast<float>(x) / size;
                float ny = static_cast<float>(y) / size;

                float kelpNoise = fbm(nx * 15.0f + 500.0f, ny * 15.0f + 500.0f, 3, 0.5f, 2.0f);

                if (kelpNoise > 0.3f) {
                    // Slightly raise seafloor where kelp grows (holdfast areas)
                    underwater[y * size + x] += 0.01f;
                }
            }
        }
    }
}

// ============================================================================
// Post-Processing Methods
// ============================================================================

void IslandGenerator::applyCoastalErosion(IslandData& data, int iterations) {
    coastalErosion(data.heightmap, data.width, data.params.waterLevel, iterations);
}

void IslandGenerator::carveRivers(IslandData& data) {
    int riverCount = static_cast<int>(data.params.riverDensity * 10.0f);
    if (riverCount <= 0) return;

    data.rivers = traceRivers(data.heightmap, data.width, riverCount);

    for (const auto& river : data.rivers) {
        carveRiverBed(data.heightmap, data.width, river);
    }
}

void IslandGenerator::createLakes(IslandData& data) {
    if (data.params.lakeDensity <= 0.0f) return;

    data.lakes = findLakeBasins(data.heightmap, data.width, data.params.lakeDensity);

    for (const auto& lake : data.lakes) {
        fillLakeBasin(data.heightmap, data.width, lake);
    }
}

void IslandGenerator::markCaveEntrances(IslandData& data) {
    if (!data.params.generateCaves) return;

    data.caveEntrances = findCaveLocations(data.heightmap, data.width);
}

void IslandGenerator::generateUnderwaterTerrain(IslandData& data) {
    generateSeafloor(data.underwaterHeightmap, data.width, data.params.underwaterDepth);

    // Blend underwater terrain with main heightmap at coastlines
    for (int y = 0; y < data.height; ++y) {
        for (int x = 0; x < data.width; ++x) {
            float h = data.heightmap[y * data.width + x];

            if (h < data.params.waterLevel) {
                // Underwater: use seafloor heightmap
                float underwaterH = data.underwaterHeightmap[y * data.width + x];

                // Smooth transition from land slope to seafloor
                float blendFactor = smoothstep(0.0f, data.params.waterLevel, h);
                data.underwaterHeightmap[y * data.width + x] = lerp(underwaterH, h, blendFactor);
            } else {
                // Above water: match terrain height
                data.underwaterHeightmap[y * data.width + x] = h;
            }
        }
    }

    // Add coral and kelp
    generateCoralReefs(data.underwaterHeightmap, data.width, data.params.waterLevel);
    generateKelpForests(data.underwaterHeightmap, data.width, data.params.waterLevel);
}

void IslandGenerator::smoothCoastlines(IslandData& data, int iterations) {
    float waterLevel = data.params.waterLevel;

    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<float> smoothed = data.heightmap;

        for (int y = 1; y < data.height - 1; ++y) {
            for (int x = 1; x < data.width - 1; ++x) {
                float h = data.heightmap[y * data.width + x];

                // Only smooth near coastline
                if (h > waterLevel * 0.8f && h < waterLevel * 1.3f) {
                    float sum = 0.0f;
                    int count = 0;

                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            sum += data.heightmap[(y + dy) * data.width + (x + dx)];
                            count++;
                        }
                    }

                    smoothed[y * data.width + x] = sum / count;
                }
            }
        }

        data.heightmap = smoothed;
    }
}
