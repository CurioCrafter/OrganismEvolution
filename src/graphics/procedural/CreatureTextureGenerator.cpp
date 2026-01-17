#include "CreatureTextureGenerator.h"
#include <cmath>
#include <algorithm>
#include <random>

namespace graphics {

// Helper functions
namespace {
    // Fast hash function
    uint32_t hash(uint32_t x, uint32_t seed = 0) {
        x ^= seed;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = (x >> 16) ^ x;
        return x;
    }

    // Hash to float [0, 1]
    float hashToFloat(uint32_t x, uint32_t seed = 0) {
        return static_cast<float>(hash(x, seed)) / static_cast<float>(UINT32_MAX);
    }

    // 2D hash
    float hash2D(int x, int y, uint32_t seed) {
        return hashToFloat(hash(x + hash(y, seed), seed));
    }

    // Smoothstep
    float smoothstep(float edge0, float edge1, float x) {
        float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    // Lerp
    float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    // Convert float color to uint8
    uint8_t colorToU8(float c) {
        return static_cast<uint8_t>(std::clamp(c * 255.0f, 0.0f, 255.0f));
    }
}

CreatureTextureGenerator::CreatureTextureGenerator() {
    std::random_device rd;
    m_seed = rd();
}

float CreatureTextureGenerator::perlinNoise(float x, float y, uint32_t seed) const {
    // Grid coordinates
    int x0 = static_cast<int>(std::floor(x));
    int y0 = static_cast<int>(std::floor(y));
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    // Fractional position
    float fx = x - x0;
    float fy = y - y0;

    // Smoothstep
    float sx = smoothstep(0, 1, fx);
    float sy = smoothstep(0, 1, fy);

    // Random gradients at corners
    auto gradient = [seed](int ix, int iy) -> glm::vec2 {
        float angle = hash2D(ix, iy, seed) * 6.28318f;
        return glm::vec2(std::cos(angle), std::sin(angle));
    };

    // Dot products
    glm::vec2 g00 = gradient(x0, y0);
    glm::vec2 g10 = gradient(x1, y0);
    glm::vec2 g01 = gradient(x0, y1);
    glm::vec2 g11 = gradient(x1, y1);

    float d00 = glm::dot(g00, glm::vec2(fx, fy));
    float d10 = glm::dot(g10, glm::vec2(fx - 1, fy));
    float d01 = glm::dot(g01, glm::vec2(fx, fy - 1));
    float d11 = glm::dot(g11, glm::vec2(fx - 1, fy - 1));

    // Interpolate
    float v0 = lerp(d00, d10, sx);
    float v1 = lerp(d01, d11, sx);
    return lerp(v0, v1, sy) * 0.5f + 0.5f;  // Map to [0, 1]
}

float CreatureTextureGenerator::simplexNoise(float x, float y, uint32_t seed) const {
    // Simplified 2D simplex noise
    const float F2 = 0.366025404f;  // (sqrt(3)-1)/2
    const float G2 = 0.211324865f;  // (3-sqrt(3))/6

    float s = (x + y) * F2;
    int i = static_cast<int>(std::floor(x + s));
    int j = static_cast<int>(std::floor(y + s));

    float t = (i + j) * G2;
    float X0 = i - t;
    float Y0 = j - t;
    float x0 = x - X0;
    float y0 = y - Y0;

    int i1, j1;
    if (x0 > y0) { i1 = 1; j1 = 0; }
    else { i1 = 0; j1 = 1; }

    float x1 = x0 - i1 + G2;
    float y1 = y0 - j1 + G2;
    float x2 = x0 - 1.0f + 2.0f * G2;
    float y2 = y0 - 1.0f + 2.0f * G2;

    auto grad = [seed](int ix, int iy, float px, float py) -> float {
        float angle = hash2D(ix, iy, seed) * 6.28318f;
        return std::cos(angle) * px + std::sin(angle) * py;
    };

    float n0 = 0, n1 = 0, n2 = 0;

    float t0 = 0.5f - x0*x0 - y0*y0;
    if (t0 >= 0) {
        t0 *= t0;
        n0 = t0 * t0 * grad(i, j, x0, y0);
    }

    float t1 = 0.5f - x1*x1 - y1*y1;
    if (t1 >= 0) {
        t1 *= t1;
        n1 = t1 * t1 * grad(i + i1, j + j1, x1, y1);
    }

    float t2 = 0.5f - x2*x2 - y2*y2;
    if (t2 >= 0) {
        t2 *= t2;
        n2 = t2 * t2 * grad(i + 1, j + 1, x2, y2);
    }

    return 40.0f * (n0 + n1 + n2) * 0.5f + 0.5f;
}

float CreatureTextureGenerator::worleyNoise(float x, float y, uint32_t seed) const {
    int ix = static_cast<int>(std::floor(x));
    int iy = static_cast<int>(std::floor(y));

    float minDist = 10.0f;

    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            int cx = ix + dx;
            int cy = iy + dy;

            // Random point in this cell
            float px = cx + hash2D(cx, cy, seed);
            float py = cy + hash2D(cy, cx, seed + 1);

            float dist = std::sqrt((x - px) * (x - px) + (y - py) * (y - py));
            minDist = std::min(minDist, dist);
        }
    }

    return std::min(minDist, 1.0f);
}

float CreatureTextureGenerator::fbmNoise(float x, float y, uint32_t seed, int octaves) const {
    float value = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;

    for (int i = 0; i < octaves; i++) {
        value += amplitude * perlinNoise(x * frequency, y * frequency, seed + i);
        frequency *= 2.0f;
        amplitude *= 0.5f;
    }

    return value;
}

float CreatureTextureGenerator::stripePattern(float x, float y, float scale, float angle) const {
    float c = std::cos(angle);
    float s = std::sin(angle);
    float rotatedX = x * c - y * s;
    return std::sin(rotatedX * scale * 6.28318f) * 0.5f + 0.5f;
}

float CreatureTextureGenerator::spotPattern(float x, float y, float scale, float density, uint32_t seed) const {
    float cellX = x * scale * density;
    float cellY = y * scale * density;

    int ix = static_cast<int>(std::floor(cellX));
    int iy = static_cast<int>(std::floor(cellY));

    float minDist = 1.0f;

    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            int cx = ix + dx;
            int cy = iy + dy;

            // Random spot center in this cell
            float spotX = cx + hash2D(cx, cy, seed) * 0.8f + 0.1f;
            float spotY = cy + hash2D(cy, cx, seed + 100) * 0.8f + 0.1f;
            float spotRadius = hash2D(cx + cy, cy - cx, seed + 200) * 0.3f + 0.2f;

            float dist = std::sqrt((cellX - spotX) * (cellX - spotX) +
                                   (cellY - spotY) * (cellY - spotY));
            float spotValue = smoothstep(spotRadius + 0.1f, spotRadius, dist);
            minDist = std::min(minDist, 1.0f - spotValue);
        }
    }

    return 1.0f - minDist;
}

float CreatureTextureGenerator::patchPattern(float x, float y, float scale, uint32_t seed) const {
    // Voronoi-based patches
    float value = worleyNoise(x * scale, y * scale, seed);

    // Add some noise to edges
    float edgeNoise = fbmNoise(x * scale * 2.0f, y * scale * 2.0f, seed + 1000, 3);

    return smoothstep(0.3f - edgeNoise * 0.1f, 0.5f + edgeNoise * 0.1f, value);
}

float CreatureTextureGenerator::scalePattern(float x, float y, float scale) const {
    float sx = x * scale;
    float sy = y * scale;

    // Offset every other row
    if (static_cast<int>(std::floor(sy * 2)) % 2 == 1) {
        sx += 0.5f;
    }

    // Hexagonal-ish pattern
    float fx = std::fmod(sx, 1.0f);
    float fy = std::fmod(sy, 1.0f);
    if (fx < 0) fx += 1.0f;
    if (fy < 0) fy += 1.0f;

    // Distance from center of scale
    float cx = 0.5f;
    float cy = 0.5f;
    float dist = std::sqrt((fx - cx) * (fx - cx) + (fy - cy) * (fy - cy) * 1.5f);

    return smoothstep(0.6f, 0.3f, dist);
}

float CreatureTextureGenerator::featherPattern(float x, float y, float scale, float direction) const {
    // Feather barb pattern
    float angle = direction;
    float c = std::cos(angle);
    float s = std::sin(angle);

    float rx = x * c - y * s;
    float ry = x * s + y * c;

    // Main shaft
    float shaftDist = std::abs(rx) * scale;
    float shaft = smoothstep(0.1f, 0.0f, shaftDist);

    // Barbs
    float barbAngle = ry * scale * 20.0f;
    float barbPhase = std::sin(barbAngle);
    float barbDist = std::abs(rx - barbPhase * 0.1f) * scale;
    float barbs = smoothstep(0.3f, 0.1f, barbDist);

    return std::max(shaft, barbs * (1.0f - smoothstep(0.0f, 0.5f, std::abs(rx) * scale)));
}

glm::vec3 CreatureTextureGenerator::blendColors(const glm::vec3& a, const glm::vec3& b, float t) const {
    return glm::mix(a, b, std::clamp(t, 0.0f, 1.0f));
}

glm::vec3 CreatureTextureGenerator::applySymmetry(const glm::vec3& color, float u, float v,
                                                   bool bilateral, float strength) const {
    if (!bilateral) return color;

    // Mirror around u = 0.5
    float mirrorU = u < 0.5f ? u : 1.0f - u;
    // Slight variation to break perfect symmetry
    float variation = 1.0f - (1.0f - strength) * std::abs(u - 0.5f) * 2.0f;

    return color * variation;
}

GeneratedTexture CreatureTextureGenerator::generate(const ColorGenes& genes,
                                                     const TextureGenParams& params) {
    m_seed = params.seed;

    switch (genes.patternType) {
        case PatternType::Solid:
            return generateSolid(genes, params);
        case PatternType::Stripes:
            return generateStripes(genes, params);
        case PatternType::Spots:
            return generateSpots(genes, params);
        case PatternType::Patches:
            return generatePatches(genes, params);
        case PatternType::Gradient:
            return generateGradient(genes, params);
        case PatternType::Scales:
            return generateScales(genes, params);
        case PatternType::Feathers:
            return generateFeathers(genes, params);
        case PatternType::Camouflage:
            return generateCamouflage(genes, params);
        case PatternType::Rings:
            return generateRings(genes, params);
        case PatternType::Bands:
            return generateBands(genes, params);
        case PatternType::Speckled:
            return generateSpeckled(genes, params);

        // Extended patterns (Phase 7)
        case PatternType::Marbled:
            return generateMarbled(genes, params);
        case PatternType::Mottled:
            return generateMottled(genes, params);
        case PatternType::Rosettes:
            return generateRosettes(genes, params);
        case PatternType::Lightning:
            return generateLightning(genes, params);
        case PatternType::Countershading:
            return generateCountershading(genes, params);
        case PatternType::Eyespots:
            return generateEyespots(genes, params);
        case PatternType::Tribal:
            return generateTribal(genes, params);
        case PatternType::Brindle:
            return generateBrindle(genes, params);

        // Phase 8 archetype-specific patterns
        case PatternType::Segmented:
            return generateSegmented(genes, params);
        case PatternType::PlatedArmor:
            return generatePlatedArmor(genes, params);
        case PatternType::RadialBurst:
            return generateRadialBurst(genes, params);
        case PatternType::Reticulated:
            return generateReticulated(genes, params);
        case PatternType::Bioluminescent:
            return generateBioluminescent(genes, params);

        default:
            return generateSolid(genes, params);
    }
}

GeneratedTexture CreatureTextureGenerator::generateSolid(const ColorGenes& genes,
                                                          const TextureGenParams& params) {
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    glm::vec3 color = genes.primaryColor * genes.brightnessMod;

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            uint32_t idx = (y * params.width + x) * 4;

            // Add subtle noise for more natural look
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;
            float noise = fbmNoise(u * 10.0f, v * 10.0f, m_seed, 2) * 0.1f;

            glm::vec3 finalColor = color * (1.0f + noise - 0.05f);
            finalColor = glm::clamp(finalColor * genes.saturationMod, 0.0f, 1.0f);

            result.albedoData[idx + 0] = colorToU8(finalColor.r);
            result.albedoData[idx + 1] = colorToU8(finalColor.g);
            result.albedoData[idx + 2] = colorToU8(finalColor.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

GeneratedTexture CreatureTextureGenerator::generateStripes(const ColorGenes& genes,
                                                            const TextureGenParams& params) {
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    float angle = hash2D(m_seed, 0, m_seed) * 1.57f - 0.78f;  // -45 to 45 degrees

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            // Add noise to stripe edges
            float noise = fbmNoise(u * 5.0f, v * 5.0f, m_seed + 100, 2) * genes.patternRandomness;
            float stripeValue = stripePattern(u + noise, v + noise,
                                              genes.patternScale * 5.0f, angle);

            // Sharpen stripes
            stripeValue = smoothstep(0.5f - genes.patternContrast * 0.3f,
                                     0.5f + genes.patternContrast * 0.3f,
                                     stripeValue);

            glm::vec3 color = blendColors(genes.primaryColor, genes.secondaryColor, stripeValue);
            color = applySymmetry(color, u, v, genes.bilateralSymmetry, genes.symmetryStrength);
            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

GeneratedTexture CreatureTextureGenerator::generateSpots(const ColorGenes& genes,
                                                          const TextureGenParams& params) {
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            float spotValue = spotPattern(u, v, genes.patternScale * 3.0f,
                                          genes.patternDensity * 5.0f + 2.0f, m_seed);

            glm::vec3 color = blendColors(genes.primaryColor, genes.secondaryColor, spotValue);
            color = applySymmetry(color, u, v, genes.bilateralSymmetry, genes.symmetryStrength);
            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

GeneratedTexture CreatureTextureGenerator::generatePatches(const ColorGenes& genes,
                                                            const TextureGenParams& params) {
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            float patchValue = patchPattern(u, v, genes.patternScale * 4.0f, m_seed);

            glm::vec3 color = blendColors(genes.primaryColor, genes.secondaryColor, patchValue);
            color = applySymmetry(color, u, v, genes.bilateralSymmetry, genes.symmetryStrength);
            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

GeneratedTexture CreatureTextureGenerator::generateGradient(const ColorGenes& genes,
                                                             const TextureGenParams& params) {
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            // Gradient from top to bottom with noise
            float noise = fbmNoise(u * 3.0f, v * 3.0f, m_seed, 2) * genes.patternRandomness;
            float gradientValue = v + noise * 0.2f;
            gradientValue = smoothstep(0.0f, 1.0f, gradientValue);

            // Three-color gradient: primary -> accent -> secondary
            glm::vec3 color;
            if (gradientValue < 0.5f) {
                color = blendColors(genes.primaryColor, genes.accentColor, gradientValue * 2.0f);
            } else {
                color = blendColors(genes.accentColor, genes.secondaryColor, (gradientValue - 0.5f) * 2.0f);
            }

            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

GeneratedTexture CreatureTextureGenerator::generateScales(const ColorGenes& genes,
                                                           const TextureGenParams& params) {
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            float scaleValue = scalePattern(u, v, genes.patternScale * 10.0f);

            // Add iridescence effect for scales
            float iridescence = 0.0f;
            if (genes.iridescence > 0.0f) {
                iridescence = std::sin(u * 20.0f + v * 20.0f) * genes.iridescence;
            }

            glm::vec3 color = blendColors(genes.secondaryColor, genes.primaryColor, scaleValue);

            // Apply iridescent shift
            color.r += iridescence * 0.1f;
            color.g -= iridescence * 0.05f;
            color.b += iridescence * 0.15f;

            color = applySymmetry(color, u, v, genes.bilateralSymmetry, genes.symmetryStrength);
            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

GeneratedTexture CreatureTextureGenerator::generateFeathers(const ColorGenes& genes,
                                                             const TextureGenParams& params) {
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            // Multiple overlapping feathers
            float featherValue = 0.0f;
            for (int i = 0; i < 3; i++) {
                float offsetU = hash2D(i, 0, m_seed) * 0.3f;
                float offsetV = hash2D(0, i, m_seed) * 0.3f;
                float angle = hash2D(i, i, m_seed) * 0.5f - 0.25f;

                featherValue += featherPattern(u - 0.5f + offsetU, v - 0.5f + offsetV,
                                               genes.patternScale * 3.0f, angle) * 0.4f;
            }
            featherValue = std::min(featherValue, 1.0f);

            glm::vec3 color = blendColors(genes.primaryColor, genes.secondaryColor, featherValue);

            // Add accent color at feather edges
            if (featherValue > 0.3f && featherValue < 0.6f) {
                color = blendColors(color, genes.accentColor, 0.3f);
            }

            color = applySymmetry(color, u, v, genes.bilateralSymmetry, genes.symmetryStrength);
            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

GeneratedTexture CreatureTextureGenerator::generateCamouflage(const ColorGenes& genes,
                                                               const TextureGenParams& params) {
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            // Multiple noise layers for organic camo look
            float noise1 = fbmNoise(u * genes.patternScale * 4.0f,
                                    v * genes.patternScale * 4.0f, m_seed, 4);
            float noise2 = fbmNoise(u * genes.patternScale * 8.0f + 100,
                                    v * genes.patternScale * 8.0f + 100, m_seed + 1, 3);
            float noise3 = worleyNoise(u * genes.patternScale * 3.0f,
                                       v * genes.patternScale * 3.0f, m_seed + 2);

            // Combine noises
            float combined = noise1 * 0.5f + noise2 * 0.3f + noise3 * 0.2f;

            // Quantize to create distinct patches
            glm::vec3 color;
            if (combined < 0.33f) {
                color = genes.primaryColor;
            } else if (combined < 0.66f) {
                color = genes.secondaryColor;
            } else {
                color = genes.accentColor;
            }

            // Blend at edges
            float edgeBlend = fbmNoise(u * 20.0f, v * 20.0f, m_seed + 3, 2) * 0.1f;
            if (combined > 0.3f && combined < 0.36f) {
                color = blendColors(genes.primaryColor, genes.secondaryColor, (combined - 0.3f) / 0.06f + edgeBlend);
            } else if (combined > 0.63f && combined < 0.69f) {
                color = blendColors(genes.secondaryColor, genes.accentColor, (combined - 0.63f) / 0.06f + edgeBlend);
            }

            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

GeneratedTexture CreatureTextureGenerator::generateRings(const ColorGenes& genes,
                                                          const TextureGenParams& params) {
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            // Distance from center
            float cu = u - 0.5f;
            float cv = v - 0.5f;
            float dist = std::sqrt(cu * cu + cv * cv);

            // Concentric rings with noise
            float noise = fbmNoise(u * 5.0f, v * 5.0f, m_seed, 2) * genes.patternRandomness * 0.1f;
            float ringValue = std::sin((dist + noise) * genes.patternScale * 30.0f) * 0.5f + 0.5f;

            glm::vec3 color = blendColors(genes.primaryColor, genes.secondaryColor, ringValue);
            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

GeneratedTexture CreatureTextureGenerator::generateBands(const ColorGenes& genes,
                                                          const TextureGenParams& params) {
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            // Horizontal bands with noise
            float noise = fbmNoise(u * 8.0f, v * 2.0f, m_seed, 2) * genes.patternRandomness;
            float bandValue = std::sin((v + noise * 0.1f) * genes.patternScale * 20.0f) * 0.5f + 0.5f;
            bandValue = smoothstep(0.3f, 0.7f, bandValue);

            glm::vec3 color = blendColors(genes.primaryColor, genes.secondaryColor, bandValue);
            color = applySymmetry(color, u, v, genes.bilateralSymmetry, genes.symmetryStrength);
            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

GeneratedTexture CreatureTextureGenerator::generateSpeckled(const ColorGenes& genes,
                                                             const TextureGenParams& params) {
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            // Base color
            glm::vec3 color = genes.primaryColor;

            // Add small random speckles
            float speckleNoise = hash2D(x, y, m_seed);
            float threshold = 1.0f - genes.patternDensity * 0.3f;

            if (speckleNoise > threshold) {
                // Speckle color varies
                float speckleType = hash2D(x + 1000, y + 1000, m_seed);
                if (speckleType < 0.5f) {
                    color = genes.secondaryColor;
                } else {
                    color = genes.accentColor;
                }
            }

            // Add subtle noise overlay
            float noise = fbmNoise(u * 20.0f, v * 20.0f, m_seed + 500, 2) * 0.1f;
            color *= (1.0f + noise - 0.05f);

            color = applySymmetry(color, u, v, genes.bilateralSymmetry, genes.symmetryStrength);
            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

// =============================================================================
// EXTENDED PATTERN GENERATORS (Phase 7)
// =============================================================================

GeneratedTexture CreatureTextureGenerator::generateMarbled(const ColorGenes& genes,
                                                            const TextureGenParams& params) {
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            // Create marble veins using warped noise
            float warpX = fbmNoise(u * 3.0f, v * 3.0f, m_seed, 3) * 2.0f;
            float warpY = fbmNoise(u * 3.0f + 100.0f, v * 3.0f + 100.0f, m_seed + 50, 3) * 2.0f;

            float veinNoise = fbmNoise(
                (u + warpX) * genes.patternScale * 6.0f,
                (v + warpY) * genes.patternScale * 6.0f,
                m_seed + 200, 5
            );

            // Create sharp veins by using sine of noise
            float veinValue = std::sin(veinNoise * 8.0f + u * 4.0f) * 0.5f + 0.5f;
            veinValue = std::pow(veinValue, 0.3f);  // Sharpen veins

            // Secondary vein layer
            float vein2 = std::sin(veinNoise * 12.0f + v * 3.0f) * 0.5f + 0.5f;
            vein2 = std::pow(vein2, 0.5f);

            float combined = veinValue * 0.6f + vein2 * 0.4f;

            glm::vec3 color = blendColors(genes.primaryColor, genes.secondaryColor, combined);

            // Add subtle accent at vein intersections
            if (veinValue > 0.7f && vein2 > 0.6f) {
                color = blendColors(color, genes.accentColor, 0.3f);
            }

            color = applySymmetry(color, u, v, genes.bilateralSymmetry, genes.symmetryStrength);
            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

GeneratedTexture CreatureTextureGenerator::generateMottled(const ColorGenes& genes,
                                                            const TextureGenParams& params) {
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            // Multiple scales of irregular blotches
            float blotch1 = worleyNoise(u * genes.patternScale * 4.0f, v * genes.patternScale * 4.0f, m_seed);
            float blotch2 = worleyNoise(u * genes.patternScale * 8.0f, v * genes.patternScale * 8.0f, m_seed + 100);
            float blotch3 = worleyNoise(u * genes.patternScale * 2.0f, v * genes.patternScale * 2.0f, m_seed + 200);

            // Combine with different weights for irregular appearance
            float combined = blotch1 * 0.5f + blotch2 * 0.3f + blotch3 * 0.2f;

            // Add noise to break up edges
            float edgeNoise = fbmNoise(u * 15.0f, v * 15.0f, m_seed + 300, 2) * genes.patternRandomness;
            combined += edgeNoise * 0.2f;

            // Three-color blotches
            glm::vec3 color;
            if (combined < 0.35f) {
                color = genes.primaryColor;
            } else if (combined < 0.6f) {
                color = genes.secondaryColor;
            } else {
                color = blendColors(genes.secondaryColor, genes.accentColor, (combined - 0.6f) * 2.0f);
            }

            color = applySymmetry(color, u, v, genes.bilateralSymmetry, genes.symmetryStrength);
            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

GeneratedTexture CreatureTextureGenerator::generateRosettes(const ColorGenes& genes,
                                                             const TextureGenParams& params) {
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    float cellSize = genes.patternScale * 3.0f + 2.0f;

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            float cellX = u * cellSize;
            float cellY = v * cellSize;

            int ix = static_cast<int>(std::floor(cellX));
            int iy = static_cast<int>(std::floor(cellY));

            float minOuterDist = 10.0f;
            float minInnerDist = 10.0f;

            // Check surrounding cells for rosette centers
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    int cx = ix + dx;
                    int cy = iy + dy;

                    // Random rosette center in cell
                    float centerX = cx + hash2D(cx, cy, m_seed) * 0.7f + 0.15f;
                    float centerY = cy + hash2D(cy, cx, m_seed + 50) * 0.7f + 0.15f;

                    // Rosette size variation
                    float rosetteSize = 0.25f + hash2D(cx + cy, cy - cx, m_seed + 100) * 0.2f;
                    float innerSize = rosetteSize * 0.5f;

                    float dist = std::sqrt((cellX - centerX) * (cellX - centerX) +
                                           (cellY - centerY) * (cellY - centerY));

                    // Outer ring distance
                    float outerDist = std::abs(dist - rosetteSize);
                    minOuterDist = std::min(minOuterDist, outerDist);

                    // Inner spot distance
                    if (dist < innerSize) {
                        minInnerDist = std::min(minInnerDist, dist);
                    }
                }
            }

            // Create rosette pattern
            glm::vec3 color = genes.primaryColor;

            // Outer ring (dark outline)
            float ringValue = smoothstep(0.08f, 0.02f, minOuterDist);
            if (ringValue > 0.1f) {
                color = blendColors(color, genes.secondaryColor, ringValue * genes.patternContrast);
            }

            // Inner spot (lighter center)
            if (minInnerDist < 10.0f) {
                float spotValue = smoothstep(0.15f, 0.0f, minInnerDist);
                color = blendColors(color, genes.accentColor, spotValue * 0.5f);
            }

            color = applySymmetry(color, u, v, genes.bilateralSymmetry, genes.symmetryStrength);
            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

GeneratedTexture CreatureTextureGenerator::generateLightning(const ColorGenes& genes,
                                                              const TextureGenParams& params) {
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            // Base color
            glm::vec3 color = genes.primaryColor;

            // Create branching patterns using warped coordinates
            float warp = fbmNoise(u * 4.0f, v * 4.0f, m_seed, 3) * 0.5f;

            // Multiple branch paths
            float branch1 = std::abs(u - 0.5f + warp);
            float branch2 = std::abs(v * 0.7f - warp * 2.0f);
            float branch3 = std::abs((u + v) * 0.5f - 0.5f + warp * 1.5f);

            // Diagonal branches
            float diag1 = std::abs(u - v + warp);
            float diag2 = std::abs(u + v - 1.0f + warp);

            // Find minimum distance to any branch
            float minDist = std::min({branch1, branch2, branch3, diag1, diag2});
            minDist *= genes.patternScale * 3.0f;

            // Create lightning/branch pattern
            float lightningValue = smoothstep(0.15f, 0.02f, minDist);

            // Add noise for electric/jagged effect
            float jitter = fbmNoise(u * 30.0f, v * 30.0f, m_seed + 100, 2) * 0.3f;
            lightningValue *= (1.0f + jitter);

            if (lightningValue > 0.1f) {
                // Bright core with color gradient
                glm::vec3 lightningColor = genes.secondaryColor;
                if (lightningValue > 0.7f) {
                    lightningColor = genes.accentColor;  // Bright center
                }
                color = blendColors(color, lightningColor, lightningValue * genes.patternContrast);
            }

            color = applySymmetry(color, u, v, genes.bilateralSymmetry, genes.symmetryStrength);
            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

GeneratedTexture CreatureTextureGenerator::generateCountershading(const ColorGenes& genes,
                                                                   const TextureGenParams& params) {
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            // Natural countershading: dark on top (dorsal), light on bottom (ventral)
            // v = 0 is typically the back/top, v = 1 is the belly/bottom

            // Add organic variation to the transition
            float noise = fbmNoise(u * 8.0f, v * 4.0f, m_seed, 3) * genes.patternRandomness;

            // S-curve transition for natural look
            float transition = v + noise * 0.15f;
            transition = smoothstep(0.3f, 0.7f, transition);

            // Three-zone coloring: dark back, mid-tone sides, light belly
            glm::vec3 color;
            if (transition < 0.3f) {
                // Dark dorsal (top)
                color = genes.secondaryColor * 0.7f;
            } else if (transition > 0.7f) {
                // Light ventral (bottom)
                color = genes.primaryColor * 1.2f;
            } else {
                // Transition zone (sides)
                float midT = (transition - 0.3f) / 0.4f;
                color = blendColors(genes.secondaryColor * 0.7f, genes.primaryColor * 1.2f, midT);
            }

            // Add subtle lateral line accent
            float lateralLine = std::abs(v - 0.5f);
            if (lateralLine < 0.05f + noise * 0.02f) {
                color = blendColors(color, genes.accentColor, 0.3f * (1.0f - lateralLine / 0.05f));
            }

            color = applySymmetry(color, u, v, false, 0.0f);  // No bilateral symmetry needed
            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

GeneratedTexture CreatureTextureGenerator::generateEyespots(const ColorGenes& genes,
                                                             const TextureGenParams& params) {
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    // Determine number and positions of eyespots
    int numSpots = static_cast<int>(genes.patternDensity * 4.0f) + 2;

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            glm::vec3 color = genes.primaryColor;

            // Add subtle background texture
            float bgNoise = fbmNoise(u * 10.0f, v * 10.0f, m_seed, 2) * 0.1f;
            color *= (1.0f + bgNoise - 0.05f);

            // Check distance to each eyespot
            for (int i = 0; i < numSpots; i++) {
                // Position eyespots semi-randomly
                float spotU = 0.15f + hash2D(i, 0, m_seed) * 0.35f;
                float spotV = 0.2f + hash2D(0, i, m_seed + 100) * 0.6f;

                // Mirror on other side
                float distLeft = std::sqrt((u - spotU) * (u - spotU) + (v - spotV) * (v - spotV));
                float distRight = std::sqrt((u - (1.0f - spotU)) * (u - (1.0f - spotU)) + (v - spotV) * (v - spotV));
                float dist = std::min(distLeft, distRight);

                // Eyespot size variation
                float spotSize = 0.08f + hash2D(i, i, m_seed + 200) * 0.04f;
                spotSize *= genes.patternScale;

                if (dist < spotSize * 1.5f) {
                    // Outer ring (dark)
                    if (dist > spotSize * 0.8f) {
                        float ringT = (dist - spotSize * 0.8f) / (spotSize * 0.7f);
                        ringT = 1.0f - ringT;
                        color = blendColors(color, genes.secondaryColor * 0.4f, ringT * genes.patternContrast);
                    }
                    // Middle ring (light)
                    else if (dist > spotSize * 0.4f) {
                        float midT = (dist - spotSize * 0.4f) / (spotSize * 0.4f);
                        midT = 1.0f - midT;
                        color = blendColors(color, genes.accentColor, midT * 0.7f);
                    }
                    // Pupil (dark center)
                    else {
                        float pupilT = dist / (spotSize * 0.4f);
                        pupilT = 1.0f - pupilT;
                        color = blendColors(color, genes.secondaryColor * 0.2f, pupilT);
                    }
                }
            }

            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

GeneratedTexture CreatureTextureGenerator::generateTribal(const ColorGenes& genes,
                                                           const TextureGenParams& params) {
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            glm::vec3 color = genes.primaryColor;

            // Create bold geometric patterns
            float scale = genes.patternScale * 6.0f;

            // Chevrons/zigzags
            float chevron = std::abs(std::fmod((u * scale + std::abs(v - 0.5f) * 2.0f), 1.0f) - 0.5f);
            chevron = smoothstep(0.15f, 0.1f, chevron);

            // Horizontal bands
            float bands = std::abs(std::sin(v * scale * 3.0f));
            bands = smoothstep(0.3f, 0.5f, bands);

            // Diamond shapes
            float diamond = std::abs(u - 0.5f) + std::abs(v - 0.5f);
            diamond = std::fmod(diamond * scale * 0.5f, 1.0f);
            diamond = smoothstep(0.4f, 0.5f, diamond);

            // Combine patterns
            float tribal = std::max({chevron, bands * 0.7f, diamond * 0.8f});

            // Sharp edges for bold look
            tribal = tribal > 0.4f ? 1.0f : 0.0f;

            if (tribal > 0.5f) {
                color = blendColors(color, genes.secondaryColor, genes.patternContrast);
            }

            // Add accent lines
            float accentLine = std::abs(std::sin(v * scale * 6.0f)) *
                               std::abs(std::cos(u * scale * 6.0f));
            if (accentLine > 0.9f && tribal < 0.5f) {
                color = blendColors(color, genes.accentColor, 0.5f);
            }

            color = applySymmetry(color, u, v, genes.bilateralSymmetry, genes.symmetryStrength);
            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

GeneratedTexture CreatureTextureGenerator::generateBrindle(const ColorGenes& genes,
                                                            const TextureGenParams& params) {
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            // Brindle: subtle irregular streaks, often diagonal
            float angle = 0.3f + genes.patternRandomness * 0.2f;  // Slight diagonal

            // Warped coordinate for organic streaks
            float warpX = fbmNoise(u * 5.0f, v * 5.0f, m_seed, 3) * 0.3f;
            float warpY = fbmNoise(u * 5.0f + 50.0f, v * 5.0f + 50.0f, m_seed + 100, 3) * 0.3f;

            // Rotated and warped coordinate
            float rotU = (u + warpX) * std::cos(angle) - (v + warpY) * std::sin(angle);

            // Multiple frequency streaks for organic look
            float streak1 = std::sin(rotU * genes.patternScale * 15.0f);
            float streak2 = std::sin(rotU * genes.patternScale * 25.0f + 1.5f);
            float streak3 = std::sin(rotU * genes.patternScale * 40.0f + 3.0f);

            // Combine with varying intensities
            float combined = streak1 * 0.5f + streak2 * 0.3f + streak3 * 0.2f;
            combined = combined * 0.5f + 0.5f;

            // Add noise to break up regularity
            float noise = fbmNoise(u * 20.0f, v * 20.0f, m_seed + 200, 2) * 0.2f;
            combined += noise;

            // Soft threshold for subtle streaks
            combined = smoothstep(0.35f, 0.65f, combined);

            // Subtle contrast for brindle (not as bold as stripes)
            float streakIntensity = genes.patternContrast * 0.6f;

            glm::vec3 color = blendColors(genes.primaryColor, genes.secondaryColor, combined * streakIntensity);

            // Subtle variation in base color
            float baseNoise = fbmNoise(u * 8.0f, v * 8.0f, m_seed + 300, 2) * 0.08f;
            color *= (1.0f + baseNoise);

            color = applySymmetry(color, u, v, genes.bilateralSymmetry, genes.symmetryStrength);
            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

ColorGenes CreatureTextureGenerator::genesFromGenome(const glm::vec3& color, float size, uint32_t speciesId) {
    ColorGenes genes;

    genes.primaryColor = color;

    // Enhanced secondary color generation with more variety
    // Use golden ratio for better hue distribution
    float goldenRatio = 0.618033988749895f;
    float hueShift = fmod(speciesId * goldenRatio, 1.0f) * 0.4f - 0.2f;

    // Size influences color saturation (larger creatures may have bolder patterns)
    float saturationBoost = (size - 0.5f) / 1.5f * 0.2f;  // -0.1 to +0.2

    genes.secondaryColor = glm::vec3(
        color.r * (0.6f + saturationBoost) + hueShift,
        color.g * (0.6f + saturationBoost) - hueShift * 0.5f,
        color.b * (0.6f + saturationBoost) + hueShift * 0.3f
    );
    genes.secondaryColor = glm::clamp(genes.secondaryColor, 0.0f, 1.0f);

    // Accent color - more vibrant variation
    float accentHueShift = fmod((speciesId * 2 + 137) * goldenRatio, 1.0f);
    genes.accentColor = glm::vec3(
        glm::clamp(1.0f - color.r * 0.3f + accentHueShift * 0.4f, 0.0f, 1.0f),
        glm::clamp(1.0f - color.g * 0.3f - accentHueShift * 0.2f, 0.0f, 1.0f),
        glm::clamp(1.0f - color.b * 0.3f + accentHueShift * 0.3f, 0.0f, 1.0f)
    );

    // Pattern based on species ID - now all 11 patterns available
    genes.patternType = patternFromSpeciesId(speciesId);

    // Enhanced pattern parameters with more variation
    // Pattern scale varies from 0.3 to 1.2 (wider range for more visual difference)
    genes.patternScale = 0.3f + fmod(speciesId * 0.0789f, 0.9f);

    // Pattern density varies from 0.2 to 0.9
    genes.patternDensity = 0.2f + fmod((speciesId * 1.23f + 47.0f), 0.7f);

    // Pattern contrast - more variation for dramatic differences
    genes.patternContrast = 0.3f + fmod(speciesId * 0.0456f + 0.3f, 0.6f);

    // Pattern randomness - varies per species for unique appearance
    genes.patternRandomness = 0.1f + fmod(speciesId * 0.0234f, 0.4f);

    // Bilateral symmetry - most creatures have it, some don't
    genes.bilateralSymmetry = (speciesId % 10) < 8;  // 80% have symmetry
    genes.symmetryStrength = 0.7f + fmod(speciesId * 0.0567f, 0.25f);

    // Special effects based on species characteristics
    // Iridescence - some species get shiny/iridescent coloring
    if ((speciesId % 15) == 0) {
        genes.iridescence = 0.2f + fmod(speciesId * 0.0123f, 0.5f);
    }

    // Metallic sheen - rare but distinctive
    if ((speciesId % 20) == 5) {
        genes.metallic = 0.3f + fmod(speciesId * 0.0345f, 0.4f);
    }

    // Roughness varies by pattern type
    switch (genes.patternType) {
        case PatternType::Scales:
        case PatternType::Feathers:
            genes.roughness = 0.3f + fmod(speciesId * 0.0234f, 0.3f);
            break;
        case PatternType::Speckled:
        case PatternType::Camouflage:
            genes.roughness = 0.6f + fmod(speciesId * 0.0456f, 0.3f);
            break;
        default:
            genes.roughness = 0.4f + fmod(speciesId * 0.0345f, 0.4f);
    }

    return genes;
}

PatternType CreatureTextureGenerator::patternFromSpeciesId(uint32_t speciesId) {
    // Deterministic pattern selection based on species ID
    uint32_t patternIndex = speciesId % static_cast<uint32_t>(PatternType::Count);
    return static_cast<PatternType>(patternIndex);
}

// =============================================================================
// PHASE 8 ARCHETYPE-SPECIFIC PATTERN GENERATORS
// =============================================================================

GeneratedTexture CreatureTextureGenerator::generateSegmented(const ColorGenes& genes,
                                                               const TextureGenParams& params) {
    // Arthropod-like segmentation lines - horizontal bands with chitin-like sheen
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            // Segment bands - sharp horizontal divisions
            float segmentCount = 8.0f + genes.patternDensity * 6.0f;  // 8-14 segments
            float segmentV = v * segmentCount;
            float segmentIndex = std::floor(segmentV);
            float segmentFrac = segmentV - segmentIndex;

            // Segment edge darkening (joints between segments)
            float jointDarkness = 0.0f;
            if (segmentFrac < 0.1f || segmentFrac > 0.9f) {
                jointDarkness = smoothstep(0.1f, 0.0f, std::min(segmentFrac, 1.0f - segmentFrac));
            }

            // Subtle longitudinal ridge down center
            float centerRidge = smoothstep(0.1f, 0.0f, std::abs(u - 0.5f));

            // Add chitin-like noise variation per segment
            float segmentNoise = fbmNoise(u * 8.0f + segmentIndex * 10.0f, v * 3.0f, m_seed + static_cast<uint32_t>(segmentIndex), 2);

            // Base color with segment variation
            glm::vec3 color = genes.primaryColor;

            // Darken at joints
            color *= (1.0f - jointDarkness * 0.4f);

            // Lighter center ridge
            color = blendColors(color, genes.accentColor, centerRidge * 0.2f);

            // Chitin sheen variation
            float sheen = 0.05f + segmentNoise * 0.1f;
            color *= (1.0f + sheen);

            // Lateral stripe on some segments
            if (static_cast<int>(segmentIndex) % 2 == 0) {
                float stripeIntensity = smoothstep(0.35f, 0.4f, std::abs(u - 0.5f)) *
                                       smoothstep(0.45f, 0.4f, std::abs(u - 0.5f));
                color = blendColors(color, genes.secondaryColor, stripeIntensity * 0.5f);
            }

            color = applySymmetry(color, u, v, genes.bilateralSymmetry, genes.symmetryStrength);
            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

GeneratedTexture CreatureTextureGenerator::generatePlatedArmor(const ColorGenes& genes,
                                                                 const TextureGenParams& params) {
    // Overlapping armor plates like pangolin or armadillo
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    float plateScale = genes.patternScale * 8.0f + 4.0f;  // 4-12 plates per texture

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            // Offset alternating rows for overlapping effect
            float rowV = v * plateScale;
            int row = static_cast<int>(std::floor(rowV));
            float rowOffset = (row % 2 == 0) ? 0.0f : 0.5f / plateScale;

            float cellU = (u + rowOffset) * plateScale;
            int col = static_cast<int>(std::floor(cellU));

            float localU = std::fmod(cellU, 1.0f);
            float localV = std::fmod(rowV, 1.0f);

            // Plate shape - rounded rectangle with raised center
            float edgeDist = std::min(std::min(localU, 1.0f - localU), std::min(localV, 1.0f - localV));
            float plateEdge = smoothstep(0.05f, 0.15f, edgeDist);

            // Raised center of plate
            float centerDist = std::sqrt((localU - 0.5f) * (localU - 0.5f) +
                                         (localV - 0.5f) * (localV - 0.5f) * 0.5f);
            float raisedCenter = smoothstep(0.5f, 0.1f, centerDist);

            // Per-plate color variation
            float plateVar = hash2D(col, row, m_seed);

            // Base plate color
            glm::vec3 color = blendColors(genes.primaryColor, genes.secondaryColor, plateVar * 0.3f);

            // Darken edges (plate overlap shadow)
            color *= (0.7f + plateEdge * 0.3f);

            // Lighter raised center highlight
            color = blendColors(color, genes.accentColor, raisedCenter * 0.2f);

            // Subtle scratches/wear on plates
            float scratches = fbmNoise(u * 30.0f, v * 30.0f, m_seed + 500, 2);
            if (scratches > 0.8f) {
                color *= 0.9f;
            }

            color = applySymmetry(color, u, v, genes.bilateralSymmetry, genes.symmetryStrength);
            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

GeneratedTexture CreatureTextureGenerator::generateRadialBurst(const ColorGenes& genes,
                                                                 const TextureGenParams& params) {
    // Radial symmetry pattern - star/sunburst from center
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    int rayCount = 5 + static_cast<int>(genes.patternDensity * 8.0f);  // 5-13 rays

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            // Center at 0.5, 0.5
            float cu = u - 0.5f;
            float cv = v - 0.5f;

            // Polar coordinates
            float dist = std::sqrt(cu * cu + cv * cv);
            float angle = std::atan2(cv, cu);

            // Ray pattern
            float rayAngle = angle * rayCount / 6.28318f;
            float rayFrac = std::fmod(rayAngle + 100.0f, 1.0f);  // Ensure positive
            float rayIntensity = smoothstep(0.3f, 0.5f, rayFrac) - smoothstep(0.5f, 0.7f, rayFrac);
            rayIntensity = std::abs(rayIntensity);

            // Concentric rings
            float ringFreq = genes.patternScale * 10.0f;
            float rings = std::sin(dist * ringFreq * 6.28318f) * 0.5f + 0.5f;

            // Central disc
            float centralDisc = smoothstep(0.15f, 0.05f, dist);

            // Combine patterns
            float pattern = rayIntensity * 0.6f + rings * 0.2f;

            // Warp with noise for organic feel
            float warpNoise = fbmNoise(u * 5.0f, v * 5.0f, m_seed, 2) * genes.patternRandomness;
            pattern += warpNoise * 0.2f;

            // Colors
            glm::vec3 color = blendColors(genes.primaryColor, genes.secondaryColor, pattern);

            // Central disc gets accent color
            if (dist < 0.1f) {
                color = blendColors(color, genes.accentColor, centralDisc * 0.8f);
            }

            // Outer edge fade
            float outerFade = smoothstep(0.5f, 0.4f, dist);
            color *= (0.7f + outerFade * 0.3f);

            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

GeneratedTexture CreatureTextureGenerator::generateReticulated(const ColorGenes& genes,
                                                                 const TextureGenParams& params) {
    // Network/mesh pattern - like giraffe or net-casting spider
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    float cellScale = genes.patternScale * 6.0f + 3.0f;  // 3-9 cells

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            // Warped Voronoi for organic cells
            float warpX = fbmNoise(u * 3.0f, v * 3.0f, m_seed, 2) * 0.2f;
            float warpY = fbmNoise(u * 3.0f + 50.0f, v * 3.0f + 50.0f, m_seed + 100, 2) * 0.2f;

            float cellU = (u + warpX) * cellScale;
            float cellV = (v + warpY) * cellScale;

            // Find distance to nearest and second-nearest cell centers
            float minDist1 = 10.0f;
            float minDist2 = 10.0f;
            glm::vec2 nearestCenter(0.0f);

            int cx = static_cast<int>(std::floor(cellU));
            int cy = static_cast<int>(std::floor(cellV));

            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    int ix = cx + dx;
                    int iy = cy + dy;

                    float centerX = ix + hash2D(ix, iy, m_seed) * 0.8f + 0.1f;
                    float centerY = iy + hash2D(iy, ix, m_seed + 50) * 0.8f + 0.1f;

                    float dist = std::sqrt((cellU - centerX) * (cellU - centerX) +
                                          (cellV - centerY) * (cellV - centerY));

                    if (dist < minDist1) {
                        minDist2 = minDist1;
                        minDist1 = dist;
                        nearestCenter = glm::vec2(centerX, centerY);
                    } else if (dist < minDist2) {
                        minDist2 = dist;
                    }
                }
            }

            // Edge detection - network lines
            float edgeDist = minDist2 - minDist1;
            float networkLine = smoothstep(0.1f, 0.05f, edgeDist);

            // Cell interior pattern
            float cellNoise = hash2D(static_cast<int>(nearestCenter.x * 10),
                                    static_cast<int>(nearestCenter.y * 10), m_seed + 200);

            // Colors
            glm::vec3 cellColor = blendColors(genes.primaryColor, genes.secondaryColor, cellNoise * 0.4f);
            glm::vec3 lineColor = genes.secondaryColor * 0.5f;  // Dark network lines

            glm::vec3 color = blendColors(cellColor, lineColor, networkLine);

            // Add subtle texture within cells
            float cellTexture = fbmNoise(cellU * 5.0f, cellV * 5.0f, m_seed + 300, 2) * 0.1f;
            color *= (1.0f + cellTexture);

            color = applySymmetry(color, u, v, genes.bilateralSymmetry, genes.symmetryStrength);
            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

GeneratedTexture CreatureTextureGenerator::generateBioluminescent(const ColorGenes& genes,
                                                                    const TextureGenParams& params) {
    // Glowing patterns - bright spots/lines on dark base
    GeneratedTexture result;
    result.width = params.width;
    result.height = params.height;
    result.albedoData.resize(params.width * params.height * 4);

    for (uint32_t y = 0; y < params.height; y++) {
        for (uint32_t x = 0; x < params.width; x++) {
            float u = static_cast<float>(x) / params.width;
            float v = static_cast<float>(y) / params.height;

            // Dark base color
            glm::vec3 baseColor = genes.primaryColor * 0.2f;

            // Glowing spots - multiple sizes
            float glow = 0.0f;

            // Large photophores (3-6)
            int largeSpots = 3 + static_cast<int>(genes.patternDensity * 3);
            for (int i = 0; i < largeSpots; i++) {
                float spotU = hash2D(i, 0, m_seed) * 0.6f + 0.2f;
                float spotV = hash2D(0, i, m_seed + 100) * 0.6f + 0.2f;

                float dist = std::sqrt((u - spotU) * (u - spotU) + (v - spotV) * (v - spotV));
                float spotSize = 0.08f + hash2D(i, i, m_seed + 200) * 0.04f;
                glow += smoothstep(spotSize, spotSize * 0.3f, dist) * 0.8f;

                // Mirror for bilateral
                if (genes.bilateralSymmetry) {
                    float mirrorU = 1.0f - spotU;
                    float distMirror = std::sqrt((u - mirrorU) * (u - mirrorU) + (v - spotV) * (v - spotV));
                    glow += smoothstep(spotSize, spotSize * 0.3f, distMirror) * 0.8f;
                }
            }

            // Small scattered photophores
            float smallSpots = worleyNoise(u * 15.0f * genes.patternScale,
                                          v * 15.0f * genes.patternScale, m_seed + 300);
            glow += smoothstep(0.2f, 0.1f, smallSpots) * 0.4f;

            // Glowing lines/veins
            float lineNoise = fbmNoise(u * 8.0f, v * 8.0f, m_seed + 400, 3);
            float lines = std::sin(lineNoise * 20.0f + v * 10.0f);
            lines = smoothstep(0.7f, 0.9f, lines);
            glow += lines * 0.3f;

            // Clamp total glow
            glow = std::min(glow, 1.5f);

            // Glow color (typically cyan, green, or blue bioluminescence)
            glm::vec3 glowColor = genes.accentColor;

            // Add slight color shift based on intensity
            if (glow > 0.5f) {
                glowColor = blendColors(genes.accentColor, glm::vec3(1.0f), (glow - 0.5f) * 0.3f);
            }

            // Combine base and glow
            glm::vec3 color = blendColors(baseColor, glowColor, glow);

            // Subtle dark pattern in non-glowing areas
            float darkPattern = fbmNoise(u * 10.0f, v * 10.0f, m_seed + 500, 2) * 0.1f;
            color *= (1.0f - darkPattern * (1.0f - glow));

            color = glm::clamp(color * genes.brightnessMod * genes.saturationMod, 0.0f, 1.0f);

            uint32_t idx = (y * params.width + x) * 4;
            result.albedoData[idx + 0] = colorToU8(color.r);
            result.albedoData[idx + 1] = colorToU8(color.g);
            result.albedoData[idx + 2] = colorToU8(color.b);
            result.albedoData[idx + 3] = 255;
        }
    }

    return result;
}

// TextureUploader implementation
TextureUploader::TextureUploader(ID3D12Device* device) : m_device(device) {}

Microsoft::WRL::ComPtr<ID3D12Resource> TextureUploader::uploadTexture(
    const GeneratedTexture& texture,
    ID3D12GraphicsCommandList* commandList)
{
    if (!m_device || texture.albedoData.empty()) return nullptr;

    // Create texture resource
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = texture.width;
    texDesc.Height = texture.height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    Microsoft::WRL::ComPtr<ID3D12Resource> texResource;
    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&texResource));

    if (FAILED(hr)) return nullptr;

    // Create upload buffer
    UINT64 uploadBufferSize = 0;
    m_device->GetCopyableFootprints(&texDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

    D3D12_HEAP_PROPERTIES uploadHeapProps = {};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC uploadDesc = {};
    uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    uploadDesc.Width = uploadBufferSize;
    uploadDesc.Height = 1;
    uploadDesc.DepthOrArraySize = 1;
    uploadDesc.MipLevels = 1;
    uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
    uploadDesc.SampleDesc.Count = 1;
    uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
    hr = m_device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer));

    if (FAILED(hr)) return nullptr;

    // Copy data to upload buffer
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
    UINT numRows;
    UINT64 rowSizeInBytes;
    m_device->GetCopyableFootprints(&texDesc, 0, 1, 0, &footprint, &numRows, &rowSizeInBytes, nullptr);

    void* mappedData = nullptr;
    D3D12_RANGE readRange = {0, 0};
    uploadBuffer->Map(0, &readRange, &mappedData);

    uint8_t* destPtr = static_cast<uint8_t*>(mappedData) + footprint.Offset;
    const uint8_t* srcPtr = texture.albedoData.data();
    UINT srcRowPitch = texture.width * 4;

    for (UINT row = 0; row < numRows; row++) {
        memcpy(destPtr + row * footprint.Footprint.RowPitch,
               srcPtr + row * srcRowPitch,
               srcRowPitch);
    }

    uploadBuffer->Unmap(0, nullptr);

    // Copy from upload buffer to texture
    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = texResource.Get();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = uploadBuffer.Get();
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint = footprint;

    commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    // Transition to shader resource
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = texResource.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);

    // Keep upload buffer alive until command list executes
    m_uploadBuffers.push_back(uploadBuffer);

    return texResource;
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureUploader::createSRV(
    ID3D12Resource* texture,
    ID3D12DescriptorHeap* srvHeap,
    uint32_t heapIndex)
{
    if (!texture || !srvHeap) return {};

    UINT descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE handle = srvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += heapIndex * descriptorSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    m_device->CreateShaderResourceView(texture, &srvDesc, handle);

    return handle;
}

} // namespace graphics
