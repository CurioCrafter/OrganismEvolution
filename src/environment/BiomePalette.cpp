#include "BiomePalette.h"
#include <cmath>
#include <algorithm>

// ============================================================================
// BiomePalette Implementation
// ============================================================================

glm::vec3 BiomePalette::getSeasonalGrassColor(float season) const {
    // season: 0 = early spring, 0.25 = summer, 0.5 = autumn, 0.75 = winter
    if (season < 0.25f) {
        // Spring: transitioning from winter to summer green
        float t = season / 0.25f;
        return glm::mix(grassDryColor, grassBaseColor, t);
    } else if (season < 0.5f) {
        // Summer: full green
        float t = (season - 0.25f) / 0.25f;
        return glm::mix(grassBaseColor, grassTipColor, t * 0.3f);
    } else if (season < 0.75f) {
        // Autumn: transitioning to dry/golden
        float t = (season - 0.5f) / 0.25f;
        return glm::mix(grassBaseColor, grassDryColor, t);
    } else {
        // Winter: dry/dormant
        float t = (season - 0.75f) / 0.25f;
        glm::vec3 winterColor = grassDryColor * 0.8f;
        return glm::mix(grassDryColor, winterColor, t);
    }
}

glm::vec3 BiomePalette::getSeasonalLeafColor(float season) const {
    if (season < 0.2f) {
        // Early spring: budding
        float t = season / 0.2f;
        return glm::mix(leafColorWinter, leafColorSpring, t);
    } else if (season < 0.4f) {
        // Late spring to summer
        float t = (season - 0.2f) / 0.2f;
        return glm::mix(leafColorSpring, leafColorSummer, t);
    } else if (season < 0.6f) {
        // Summer
        return leafColorSummer;
    } else if (season < 0.8f) {
        // Autumn
        float t = (season - 0.6f) / 0.2f;
        return glm::mix(leafColorSummer, leafColorAutumn, t);
    } else {
        // Late autumn to winter
        float t = (season - 0.8f) / 0.2f;
        return glm::mix(leafColorAutumn, leafColorWinter, t);
    }
}

FlowerPatchColor BiomePalette::getRandomFlowerColor(uint32_t seed) const {
    if (numFlowerColors <= 0) {
        return flowerPalette[0];
    }
    return flowerPalette[seed % numFlowerColors];
}

// ============================================================================
// PlantDistributionRules Implementation
// ============================================================================

float PlantDistributionRules::getBiomeCompatibility(BiomeType biome) const {
    // Define compatibility based on plant category and biome
    switch (category) {
        case PlantCategory::GRASS:
            switch (biome) {
                case BiomeType::GRASSLAND: return 1.0f;
                case BiomeType::SAVANNA: return 0.9f;
                case BiomeType::ALPINE_MEADOW: return 0.8f;
                case BiomeType::TEMPERATE_FOREST: return 0.4f;
                case BiomeType::WETLAND: return 0.6f;
                case BiomeType::TUNDRA: return 0.3f;
                case BiomeType::DESERT_HOT: return 0.1f;
                case BiomeType::GLACIER: return 0.0f;
                default: return 0.5f;
            }

        case PlantCategory::FLOWER:
            switch (biome) {
                case BiomeType::GRASSLAND: return 0.9f;
                case BiomeType::ALPINE_MEADOW: return 1.0f;
                case BiomeType::TROPICAL_RAINFOREST: return 0.8f;
                case BiomeType::TEMPERATE_FOREST: return 0.6f;
                case BiomeType::WETLAND: return 0.5f;
                case BiomeType::DESERT_HOT: return 0.2f;
                case BiomeType::TUNDRA: return 0.3f;
                default: return 0.4f;
            }

        case PlantCategory::BUSH:
            switch (biome) {
                case BiomeType::SHRUBLAND: return 1.0f;
                case BiomeType::TEMPERATE_FOREST: return 0.8f;
                case BiomeType::TROPICAL_RAINFOREST: return 0.7f;
                case BiomeType::BOREAL_FOREST: return 0.6f;
                case BiomeType::GRASSLAND: return 0.4f;
                case BiomeType::DESERT_HOT: return 0.2f;
                default: return 0.3f;
            }

        case PlantCategory::FERN:
            switch (biome) {
                case BiomeType::TROPICAL_RAINFOREST: return 1.0f;
                case BiomeType::TEMPERATE_FOREST: return 0.9f;
                case BiomeType::SWAMP: return 0.8f;
                case BiomeType::WETLAND: return 0.7f;
                case BiomeType::BOREAL_FOREST: return 0.4f;
                case BiomeType::GRASSLAND: return 0.1f;
                case BiomeType::DESERT_HOT: return 0.0f;
                default: return 0.2f;
            }

        case PlantCategory::CACTUS:
            switch (biome) {
                case BiomeType::DESERT_HOT: return 1.0f;
                case BiomeType::DESERT_COLD: return 0.6f;
                case BiomeType::SAVANNA: return 0.3f;
                case BiomeType::SHRUBLAND: return 0.2f;
                default: return 0.0f;
            }

        case PlantCategory::MUSHROOM:
            switch (biome) {
                case BiomeType::TEMPERATE_FOREST: return 1.0f;
                case BiomeType::TROPICAL_RAINFOREST: return 0.9f;
                case BiomeType::BOREAL_FOREST: return 0.8f;
                case BiomeType::SWAMP: return 0.7f;
                case BiomeType::WETLAND: return 0.5f;
                case BiomeType::GRASSLAND: return 0.2f;
                case BiomeType::DESERT_HOT: return 0.0f;
                default: return 0.3f;
            }

        case PlantCategory::REED:
            switch (biome) {
                case BiomeType::WETLAND: return 1.0f;
                case BiomeType::SWAMP: return 0.9f;
                case BiomeType::RIVER_BANK: return 0.9f;
                case BiomeType::LAKE_SHORE: return 0.8f;
                case BiomeType::SALT_MARSH: return 0.7f;
                case BiomeType::MANGROVE: return 0.6f;
                default: return 0.0f;
            }

        case PlantCategory::MOSS:
            switch (biome) {
                case BiomeType::BOREAL_FOREST: return 1.0f;
                case BiomeType::TEMPERATE_FOREST: return 0.9f;
                case BiomeType::TROPICAL_RAINFOREST: return 0.8f;
                case BiomeType::SWAMP: return 0.7f;
                case BiomeType::TUNDRA: return 0.6f;
                case BiomeType::ALPINE_MEADOW: return 0.4f;
                default: return 0.2f;
            }

        case PlantCategory::LICHEN:
            switch (biome) {
                case BiomeType::TUNDRA: return 1.0f;
                case BiomeType::ROCKY_HIGHLANDS: return 0.9f;
                case BiomeType::ALPINE_MEADOW: return 0.8f;
                case BiomeType::BOREAL_FOREST: return 0.6f;
                case BiomeType::GLACIER: return 0.3f;
                default: return 0.1f;
            }

        default:
            return 0.3f;
    }
}

// ============================================================================
// BiomePaletteManager Implementation
// ============================================================================

BiomePaletteManager::BiomePaletteManager() {
    initialize();
}

void BiomePaletteManager::initialize() {
    initializePalettes();
    initializeDistributionRules();
    initializeNutrition();
}

const BiomePalette& BiomePaletteManager::getPalette(BiomeType biome) const {
    size_t index = static_cast<size_t>(biome);
    if (index < m_palettes.size()) {
        return m_palettes[index];
    }
    return m_defaultPalette;
}

BiomePalette& BiomePaletteManager::getMutablePalette(BiomeType biome) {
    size_t index = static_cast<size_t>(biome);
    if (index < m_palettes.size()) {
        return m_palettes[index];
    }
    return m_defaultPalette;
}

BiomePalette BiomePaletteManager::getBlendedPalette(BiomeType primary, BiomeType secondary, float blendFactor) const {
    const BiomePalette& a = getPalette(primary);
    const BiomePalette& b = getPalette(secondary);
    return blendPalettes(a, b, blendFactor);
}

const PlantDistributionRules& BiomePaletteManager::getDistributionRules(PlantCategory category) const {
    size_t index = static_cast<size_t>(category);
    if (index < m_distributionRules.size()) {
        return m_distributionRules[index];
    }
    return m_distributionRules[0];
}

const PlantNutrition& BiomePaletteManager::getNutrition(PlantCategory category) const {
    size_t index = static_cast<size_t>(category);
    if (index < m_nutrition.size()) {
        return m_nutrition[index];
    }
    return m_nutrition[0];
}

float BiomePaletteManager::noise2D(float x, float z) const {
    // Simple hash-based noise
    int xi = static_cast<int>(std::floor(x));
    int zi = static_cast<int>(std::floor(z));
    float xf = x - xi;
    float zf = z - zi;

    auto hash = [](int x, int z) -> float {
        int n = x + z * 57;
        n = (n << 13) ^ n;
        return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
    };

    // Bilinear interpolation
    float v00 = hash(xi, zi);
    float v10 = hash(xi + 1, zi);
    float v01 = hash(xi, zi + 1);
    float v11 = hash(xi + 1, zi + 1);

    float i1 = v00 * (1 - xf) + v10 * xf;
    float i2 = v01 * (1 - xf) + v11 * xf;

    return i1 * (1 - zf) + i2 * zf;
}

float BiomePaletteManager::fractalNoise(float x, float z, int octaves) const {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        value += noise2D(x * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }

    return value / maxValue;
}

glm::vec3 BiomePaletteManager::sampleGrassColor(BiomeType biome, float x, float z, float season) const {
    const BiomePalette& palette = getPalette(biome);

    // Get base seasonal color
    glm::vec3 baseColor = palette.getSeasonalGrassColor(season);

    // Add spatial variation
    float variation = fractalNoise(x * 0.1f, z * 0.1f, 3) * palette.grassColorVariation;

    // Blend towards tip or base color based on variation
    if (variation > 0) {
        baseColor = glm::mix(baseColor, palette.grassTipColor, variation);
    } else {
        baseColor = glm::mix(baseColor, palette.grassBaseColor, -variation);
    }

    return baseColor;
}

glm::vec3 BiomePaletteManager::sampleLeafColor(BiomeType biome, float x, float z, float season) const {
    const BiomePalette& palette = getPalette(biome);

    // Get base seasonal color
    glm::vec3 baseColor = palette.getSeasonalLeafColor(season);

    // Add tree-to-tree variation
    float variation = noise2D(x * 0.5f, z * 0.5f) * palette.leafColorVariation;

    // Subtle variation in hue/brightness
    baseColor *= (1.0f + variation * 0.2f);

    return glm::clamp(baseColor, glm::vec3(0.0f), glm::vec3(1.0f));
}

FlowerPatchColor BiomePaletteManager::sampleFlowerColor(BiomeType biome, float x, float z) const {
    const BiomePalette& palette = getPalette(biome);

    // Use position to deterministically select flower color
    // This creates natural-looking patches of the same color
    float patchScale = 0.05f;  // Size of color patches
    float noiseVal = noise2D(x * patchScale, z * patchScale);

    // Map noise to flower color index
    int colorIndex = static_cast<int>((noiseVal + 1.0f) * 0.5f * palette.numFlowerColors);
    colorIndex = std::clamp(colorIndex, 0, palette.numFlowerColors - 1);

    return palette.flowerPalette[colorIndex];
}

float BiomePaletteManager::getPlantSuitability(PlantCategory category, BiomeType biome,
                                               float elevation, float moisture, float temperature) const {
    const PlantDistributionRules& rules = getDistributionRules(category);

    // Start with biome compatibility
    float suitability = rules.getBiomeCompatibility(biome);
    if (suitability <= 0.01f) return 0.0f;

    // Elevation factor
    if (elevation < rules.minElevation || elevation > rules.maxElevation) {
        return 0.0f;
    }
    float elevDist = std::abs(elevation - rules.optimalElevation);
    float elevRange = std::max(rules.optimalElevation - rules.minElevation,
                               rules.maxElevation - rules.optimalElevation);
    float elevFactor = 1.0f - (elevDist / elevRange);
    elevFactor = std::max(0.0f, elevFactor);

    // Moisture factor
    if (moisture < rules.minMoisture || moisture > rules.maxMoisture) {
        return 0.0f;
    }
    float moistDist = std::abs(moisture - rules.optimalMoisture);
    float moistRange = std::max(rules.optimalMoisture - rules.minMoisture,
                                rules.maxMoisture - rules.optimalMoisture);
    float moistFactor = 1.0f - (moistDist / moistRange);
    moistFactor = std::max(0.0f, moistFactor);

    // Temperature factor
    if (temperature < rules.minTemperature || temperature > rules.maxTemperature) {
        return 0.0f;
    }
    float tempDist = std::abs(temperature - rules.optimalTemperature);
    float tempRange = std::max(rules.optimalTemperature - rules.minTemperature,
                               rules.maxTemperature - rules.optimalTemperature);
    float tempFactor = 1.0f - (tempDist / tempRange);
    tempFactor = std::max(0.0f, tempFactor);

    // Combine all factors
    return suitability * elevFactor * moistFactor * tempFactor;
}

float BiomePaletteManager::getCreatureFoodPreference(PlantCategory plant, float creatureSize,
                                                      float specialization, bool isHerbivore) const {
    const PlantNutrition& nutrition = getNutrition(plant);

    // Base preference from nutrition data
    float preference = isHerbivore ? nutrition.herbivorePreference : nutrition.omnivorePreference;

    // Size affects preference
    float sizePreference = creatureSize < 0.5f ? nutrition.preferredBySmall : nutrition.preferredByLarge;
    preference *= (0.5f + sizePreference * 0.5f);

    // Specialization affects ability to eat difficult plants
    if (nutrition.requiresSpecialization > 0.5f) {
        if (specialization < nutrition.requiresSpecialization) {
            preference *= specialization / nutrition.requiresSpecialization;
        }
    }

    // Toxicity reduces preference (unless adapted)
    if (nutrition.toxicity > 0.0f) {
        float toxicityPenalty = nutrition.toxicity * (1.0f - specialization);
        preference *= (1.0f - toxicityPenalty);
    }

    return std::max(0.0f, preference);
}

void BiomePaletteManager::applyHueShift(float hueOffset) {
    auto shiftHue = [hueOffset](glm::vec3& color) {
        // Convert RGB to HSV
        float maxC = std::max({color.r, color.g, color.b});
        float minC = std::min({color.r, color.g, color.b});
        float delta = maxC - minC;

        float h = 0.0f, s = 0.0f, v = maxC;

        if (delta > 0.0001f) {
            s = delta / maxC;
            if (color.r >= maxC) h = (color.g - color.b) / delta;
            else if (color.g >= maxC) h = 2.0f + (color.b - color.r) / delta;
            else h = 4.0f + (color.r - color.g) / delta;
            h /= 6.0f;
            if (h < 0.0f) h += 1.0f;
        }

        // Shift hue
        h = std::fmod(h + hueOffset, 1.0f);
        if (h < 0.0f) h += 1.0f;

        // Convert back to RGB
        if (s <= 0.0f) {
            color = glm::vec3(v);
            return;
        }

        float hh = h * 6.0f;
        int i = static_cast<int>(hh);
        float f = hh - i;
        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));

        switch (i % 6) {
            case 0: color = glm::vec3(v, t, p); break;
            case 1: color = glm::vec3(q, v, p); break;
            case 2: color = glm::vec3(p, v, t); break;
            case 3: color = glm::vec3(p, q, v); break;
            case 4: color = glm::vec3(t, p, v); break;
            default: color = glm::vec3(v, p, q); break;
        }
    };

    for (auto& palette : m_palettes) {
        shiftHue(palette.groundColor);
        shiftHue(palette.groundAccentColor);
        shiftHue(palette.grassBaseColor);
        shiftHue(palette.grassTipColor);
        shiftHue(palette.grassDryColor);
        shiftHue(palette.treeBarkColor);
        shiftHue(palette.leafColorSpring);
        shiftHue(palette.leafColorSummer);
        shiftHue(palette.leafColorAutumn);
        shiftHue(palette.bushLeafColor);
        shiftHue(palette.fernColor);
        shiftHue(palette.mushroomCapColor);

        for (int i = 0; i < palette.numFlowerColors; ++i) {
            shiftHue(palette.flowerPalette[i].petalColor);
            shiftHue(palette.flowerPalette[i].centerColor);
        }
    }
}

void BiomePaletteManager::applySaturationMultiplier(float mult) {
    auto adjustSaturation = [mult](glm::vec3& color) {
        float gray = 0.299f * color.r + 0.587f * color.g + 0.114f * color.b;
        color = glm::mix(glm::vec3(gray), color, mult);
        color = glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f));
    };

    for (auto& palette : m_palettes) {
        palette.saturationMultiplier *= mult;
        adjustSaturation(palette.groundColor);
        adjustSaturation(palette.grassBaseColor);
        adjustSaturation(palette.grassTipColor);
        adjustSaturation(palette.leafColorSpring);
        adjustSaturation(palette.leafColorSummer);
        adjustSaturation(palette.leafColorAutumn);
    }
}

void BiomePaletteManager::applyBrightnessMultiplier(float mult) {
    for (auto& palette : m_palettes) {
        palette.brightnessMultiplier *= mult;
        palette.groundColor *= mult;
        palette.grassBaseColor *= mult;
        palette.grassTipColor *= mult;

        // Clamp all colors
        palette.groundColor = glm::clamp(palette.groundColor, glm::vec3(0.0f), glm::vec3(1.0f));
        palette.grassBaseColor = glm::clamp(palette.grassBaseColor, glm::vec3(0.0f), glm::vec3(1.0f));
        palette.grassTipColor = glm::clamp(palette.grassTipColor, glm::vec3(0.0f), glm::vec3(1.0f));
    }
}

void BiomePaletteManager::initializePalettes() {
    // Initialize default palette first
    m_defaultPalette.biomeType = BiomeType::GRASSLAND;
    m_defaultPalette.name = "Default";
    m_defaultPalette.groundColor = glm::vec3(0.4f, 0.3f, 0.2f);
    m_defaultPalette.groundAccentColor = glm::vec3(0.35f, 0.28f, 0.18f);
    m_defaultPalette.rockColor = glm::vec3(0.5f, 0.48f, 0.45f);
    m_defaultPalette.sandColor = glm::vec3(0.85f, 0.78f, 0.55f);
    m_defaultPalette.mudColor = glm::vec3(0.35f, 0.28f, 0.2f);
    m_defaultPalette.grassBaseColor = glm::vec3(0.25f, 0.45f, 0.15f);
    m_defaultPalette.grassTipColor = glm::vec3(0.35f, 0.55f, 0.2f);
    m_defaultPalette.grassDryColor = glm::vec3(0.6f, 0.55f, 0.3f);
    m_defaultPalette.grassColorVariation = 0.15f;
    m_defaultPalette.treeBarkColor = glm::vec3(0.35f, 0.25f, 0.15f);
    m_defaultPalette.treeBarkAccent = glm::vec3(0.28f, 0.2f, 0.12f);
    m_defaultPalette.leafColorSpring = glm::vec3(0.4f, 0.6f, 0.25f);
    m_defaultPalette.leafColorSummer = glm::vec3(0.2f, 0.5f, 0.15f);
    m_defaultPalette.leafColorAutumn = glm::vec3(0.7f, 0.4f, 0.15f);
    m_defaultPalette.leafColorWinter = glm::vec3(0.3f, 0.25f, 0.15f);
    m_defaultPalette.leafColorVariation = 0.1f;
    m_defaultPalette.saturationMultiplier = 1.0f;
    m_defaultPalette.brightnessMultiplier = 1.0f;
    m_defaultPalette.numFlowerColors = 3;
    m_defaultPalette.flowerPalette[0] = {{0.9f, 0.85f, 0.2f}, {0.8f, 0.6f, 0.1f}, {0.2f, 0.4f, 0.1f}, 0.0f};
    m_defaultPalette.flowerPalette[1] = {{0.95f, 0.95f, 0.95f}, {0.9f, 0.8f, 0.2f}, {0.2f, 0.4f, 0.1f}, 0.0f};
    m_defaultPalette.flowerPalette[2] = {{0.6f, 0.4f, 0.7f}, {0.9f, 0.9f, 0.5f}, {0.2f, 0.4f, 0.1f}, 0.0f};

    // Initialize all biome palettes
    createGrasslandPalette();
    createForestPalette();
    createDesertPalette();
    createTundraPalette();
    createTropicalPalette();
    createWetlandPalette();
    createSavannaPalette();
    createBorealPalette();
    createAlpinePalette();
    createVolcanicPalette();
    createCoastalPalette();

    // Copy default to uninitialized biomes
    for (size_t i = 0; i < m_palettes.size(); ++i) {
        if (m_palettes[i].name.empty()) {
            m_palettes[i] = m_defaultPalette;
            m_palettes[i].biomeType = static_cast<BiomeType>(i);
        }
    }
}

void BiomePaletteManager::createGrasslandPalette() {
    BiomePalette& p = m_palettes[static_cast<size_t>(BiomeType::GRASSLAND)];
    p.biomeType = BiomeType::GRASSLAND;
    p.name = "Grassland";

    // Terrain
    p.groundColor = glm::vec3(0.45f, 0.35f, 0.22f);
    p.groundAccentColor = glm::vec3(0.4f, 0.32f, 0.2f);
    p.rockColor = glm::vec3(0.55f, 0.52f, 0.48f);
    p.sandColor = glm::vec3(0.82f, 0.75f, 0.52f);
    p.mudColor = glm::vec3(0.38f, 0.3f, 0.22f);

    // Grass - vibrant greens with golden undertones
    p.grassBaseColor = glm::vec3(0.3f, 0.5f, 0.18f);
    p.grassTipColor = glm::vec3(0.45f, 0.6f, 0.25f);
    p.grassDryColor = glm::vec3(0.7f, 0.62f, 0.35f);
    p.grassColorVariation = 0.12f;

    // Trees (sparse in grassland)
    p.treeBarkColor = glm::vec3(0.38f, 0.28f, 0.18f);
    p.treeBarkAccent = glm::vec3(0.3f, 0.22f, 0.14f);
    p.leafColorSpring = glm::vec3(0.45f, 0.62f, 0.28f);
    p.leafColorSummer = glm::vec3(0.28f, 0.52f, 0.18f);
    p.leafColorAutumn = glm::vec3(0.75f, 0.5f, 0.18f);
    p.leafColorWinter = glm::vec3(0.35f, 0.28f, 0.18f);
    p.leafColorVariation = 0.08f;

    // Bushes
    p.bushLeafColor = glm::vec3(0.32f, 0.48f, 0.2f);
    p.bushBerryColor = glm::vec3(0.6f, 0.15f, 0.15f);

    // Flowers - grassland has yellows, whites, light blues
    p.numFlowerColors = 5;
    p.flowerPalette[0] = {{0.95f, 0.9f, 0.25f}, {0.85f, 0.7f, 0.15f}, {0.25f, 0.42f, 0.12f}, 0.0f};  // Yellow
    p.flowerPalette[1] = {{0.98f, 0.98f, 0.95f}, {0.9f, 0.85f, 0.3f}, {0.22f, 0.4f, 0.1f}, 0.0f};    // White
    p.flowerPalette[2] = {{0.6f, 0.75f, 0.95f}, {0.9f, 0.9f, 0.6f}, {0.2f, 0.38f, 0.12f}, 0.0f};     // Light blue
    p.flowerPalette[3] = {{0.95f, 0.6f, 0.7f}, {0.95f, 0.9f, 0.4f}, {0.22f, 0.4f, 0.1f}, 0.0f};      // Light pink
    p.flowerPalette[4] = {{0.85f, 0.5f, 0.2f}, {0.7f, 0.45f, 0.15f}, {0.25f, 0.42f, 0.12f}, 0.0f};   // Orange

    // Other plants
    p.fernColor = glm::vec3(0.25f, 0.45f, 0.18f);
    p.fernUndersideColor = glm::vec3(0.3f, 0.48f, 0.22f);
    p.mushroomCapColor = glm::vec3(0.85f, 0.8f, 0.7f);
    p.mushroomStemColor = glm::vec3(0.9f, 0.88f, 0.82f);
    p.mushroomGillColor = glm::vec3(0.7f, 0.65f, 0.58f);
    p.mushroomGlows = false;
    p.cactusColor = glm::vec3(0.3f, 0.5f, 0.25f);
    p.reedColor = glm::vec3(0.55f, 0.5f, 0.35f);
    p.mossColor = glm::vec3(0.35f, 0.5f, 0.25f);
    p.lichenColor = glm::vec3(0.6f, 0.62f, 0.5f);
    p.vineColor = glm::vec3(0.28f, 0.45f, 0.2f);

    // Environment
    p.ambientTint = glm::vec3(1.0f, 1.0f, 0.95f);
    p.saturationMultiplier = 1.0f;
    p.brightnessMultiplier = 1.0f;
}

void BiomePaletteManager::createForestPalette() {
    BiomePalette& p = m_palettes[static_cast<size_t>(BiomeType::TEMPERATE_FOREST)];
    p.biomeType = BiomeType::TEMPERATE_FOREST;
    p.name = "Temperate Forest";

    // Terrain - rich brown forest floor
    p.groundColor = glm::vec3(0.35f, 0.25f, 0.15f);
    p.groundAccentColor = glm::vec3(0.3f, 0.22f, 0.12f);
    p.rockColor = glm::vec3(0.45f, 0.42f, 0.38f);
    p.sandColor = glm::vec3(0.7f, 0.62f, 0.45f);
    p.mudColor = glm::vec3(0.32f, 0.24f, 0.16f);

    // Grass - darker, shade-tolerant
    p.grassBaseColor = glm::vec3(0.2f, 0.38f, 0.12f);
    p.grassTipColor = glm::vec3(0.28f, 0.45f, 0.18f);
    p.grassDryColor = glm::vec3(0.55f, 0.48f, 0.28f);
    p.grassColorVariation = 0.1f;

    // Trees - rich varied greens
    p.treeBarkColor = glm::vec3(0.32f, 0.22f, 0.12f);
    p.treeBarkAccent = glm::vec3(0.25f, 0.18f, 0.1f);
    p.leafColorSpring = glm::vec3(0.4f, 0.58f, 0.25f);
    p.leafColorSummer = glm::vec3(0.18f, 0.45f, 0.12f);
    p.leafColorAutumn = glm::vec3(0.85f, 0.45f, 0.12f);
    p.leafColorWinter = glm::vec3(0.28f, 0.22f, 0.12f);
    p.leafColorVariation = 0.12f;

    // Bushes
    p.bushLeafColor = glm::vec3(0.22f, 0.4f, 0.15f);
    p.bushBerryColor = glm::vec3(0.55f, 0.1f, 0.15f);

    // Flowers - forest has whites, pale pinks, purples
    p.numFlowerColors = 5;
    p.flowerPalette[0] = {{0.98f, 0.98f, 0.98f}, {0.95f, 0.9f, 0.5f}, {0.18f, 0.35f, 0.1f}, 0.0f};   // White
    p.flowerPalette[1] = {{0.95f, 0.82f, 0.88f}, {0.9f, 0.75f, 0.4f}, {0.2f, 0.38f, 0.12f}, 0.0f};   // Pale pink
    p.flowerPalette[2] = {{0.7f, 0.55f, 0.8f}, {0.92f, 0.88f, 0.5f}, {0.18f, 0.35f, 0.1f}, 0.0f};    // Purple
    p.flowerPalette[3] = {{0.5f, 0.5f, 0.85f}, {0.9f, 0.85f, 0.45f}, {0.18f, 0.35f, 0.1f}, 0.0f};    // Blue-violet
    p.flowerPalette[4] = {{0.95f, 0.95f, 0.8f}, {0.85f, 0.7f, 0.3f}, {0.2f, 0.38f, 0.12f}, 0.0f};    // Cream

    // Other plants - ferns are common
    p.fernColor = glm::vec3(0.18f, 0.42f, 0.15f);
    p.fernUndersideColor = glm::vec3(0.22f, 0.45f, 0.18f);
    p.mushroomCapColor = glm::vec3(0.65f, 0.35f, 0.2f);
    p.mushroomStemColor = glm::vec3(0.92f, 0.9f, 0.85f);
    p.mushroomGillColor = glm::vec3(0.8f, 0.75f, 0.65f);
    p.mushroomGlows = false;
    p.mossColor = glm::vec3(0.25f, 0.48f, 0.2f);
    p.lichenColor = glm::vec3(0.55f, 0.58f, 0.45f);
    p.vineColor = glm::vec3(0.22f, 0.42f, 0.18f);

    // Environment - slightly green tinted
    p.ambientTint = glm::vec3(0.95f, 1.0f, 0.92f);
    p.saturationMultiplier = 0.95f;
    p.brightnessMultiplier = 0.9f;  // Shaded
}

void BiomePaletteManager::createDesertPalette() {
    BiomePalette& p = m_palettes[static_cast<size_t>(BiomeType::DESERT_HOT)];
    p.biomeType = BiomeType::DESERT_HOT;
    p.name = "Hot Desert";

    // Terrain - sandy tones
    p.groundColor = glm::vec3(0.85f, 0.72f, 0.5f);
    p.groundAccentColor = glm::vec3(0.8f, 0.65f, 0.42f);
    p.rockColor = glm::vec3(0.7f, 0.6f, 0.5f);
    p.sandColor = glm::vec3(0.92f, 0.82f, 0.58f);
    p.mudColor = glm::vec3(0.6f, 0.48f, 0.35f);

    // Grass - sparse, dry
    p.grassBaseColor = glm::vec3(0.55f, 0.5f, 0.32f);
    p.grassTipColor = glm::vec3(0.65f, 0.58f, 0.38f);
    p.grassDryColor = glm::vec3(0.75f, 0.68f, 0.45f);
    p.grassColorVariation = 0.08f;

    // Trees - rare, desert adapted
    p.treeBarkColor = glm::vec3(0.5f, 0.4f, 0.3f);
    p.treeBarkAccent = glm::vec3(0.45f, 0.35f, 0.25f);
    p.leafColorSpring = glm::vec3(0.4f, 0.5f, 0.3f);
    p.leafColorSummer = glm::vec3(0.35f, 0.45f, 0.28f);
    p.leafColorAutumn = glm::vec3(0.5f, 0.45f, 0.3f);
    p.leafColorWinter = glm::vec3(0.45f, 0.4f, 0.28f);
    p.leafColorVariation = 0.05f;

    // Bushes
    p.bushLeafColor = glm::vec3(0.4f, 0.48f, 0.32f);
    p.bushBerryColor = glm::vec3(0.7f, 0.25f, 0.2f);

    // Flowers - sparse, drought-resistant
    p.numFlowerColors = 4;
    p.flowerPalette[0] = {{0.95f, 0.65f, 0.2f}, {0.85f, 0.55f, 0.15f}, {0.35f, 0.45f, 0.25f}, 0.0f};  // Orange
    p.flowerPalette[1] = {{0.9f, 0.3f, 0.35f}, {0.95f, 0.85f, 0.4f}, {0.35f, 0.45f, 0.25f}, 0.0f};    // Red
    p.flowerPalette[2] = {{0.95f, 0.92f, 0.5f}, {0.9f, 0.75f, 0.3f}, {0.35f, 0.45f, 0.25f}, 0.0f};    // Yellow
    p.flowerPalette[3] = {{0.85f, 0.55f, 0.7f}, {0.9f, 0.8f, 0.4f}, {0.35f, 0.45f, 0.25f}, 0.0f};     // Magenta

    // Cacti
    p.cactusColor = glm::vec3(0.35f, 0.55f, 0.32f);

    // Other plants - minimal
    p.fernColor = glm::vec3(0.4f, 0.48f, 0.3f);
    p.mushroomCapColor = glm::vec3(0.75f, 0.65f, 0.5f);
    p.mushroomStemColor = glm::vec3(0.85f, 0.8f, 0.7f);
    p.mushroomGlows = false;
    p.mossColor = glm::vec3(0.45f, 0.48f, 0.35f);
    p.lichenColor = glm::vec3(0.65f, 0.6f, 0.5f);

    // Environment - warm, bright
    p.ambientTint = glm::vec3(1.05f, 1.0f, 0.9f);
    p.saturationMultiplier = 0.85f;
    p.brightnessMultiplier = 1.1f;
}

void BiomePaletteManager::createTundraPalette() {
    BiomePalette& p = m_palettes[static_cast<size_t>(BiomeType::TUNDRA)];
    p.biomeType = BiomeType::TUNDRA;
    p.name = "Tundra";

    // Terrain - cold, permafrost
    p.groundColor = glm::vec3(0.55f, 0.52f, 0.48f);
    p.groundAccentColor = glm::vec3(0.5f, 0.48f, 0.45f);
    p.rockColor = glm::vec3(0.58f, 0.55f, 0.52f);
    p.sandColor = glm::vec3(0.72f, 0.68f, 0.62f);
    p.mudColor = glm::vec3(0.45f, 0.42f, 0.38f);

    // Grass - hardy, short
    p.grassBaseColor = glm::vec3(0.4f, 0.45f, 0.32f);
    p.grassTipColor = glm::vec3(0.48f, 0.52f, 0.38f);
    p.grassDryColor = glm::vec3(0.58f, 0.55f, 0.45f);
    p.grassColorVariation = 0.06f;

    // No real trees, dwarf shrubs
    p.treeBarkColor = glm::vec3(0.42f, 0.35f, 0.28f);
    p.treeBarkAccent = glm::vec3(0.38f, 0.32f, 0.25f);
    p.leafColorSpring = glm::vec3(0.42f, 0.5f, 0.35f);
    p.leafColorSummer = glm::vec3(0.38f, 0.48f, 0.32f);
    p.leafColorAutumn = glm::vec3(0.6f, 0.42f, 0.25f);
    p.leafColorWinter = glm::vec3(0.35f, 0.32f, 0.28f);
    p.leafColorVariation = 0.05f;

    // Bushes - low growing
    p.bushLeafColor = glm::vec3(0.35f, 0.42f, 0.28f);
    p.bushBerryColor = glm::vec3(0.5f, 0.15f, 0.2f);

    // Flowers - short blooming season
    p.numFlowerColors = 4;
    p.flowerPalette[0] = {{0.95f, 0.7f, 0.8f}, {0.95f, 0.9f, 0.5f}, {0.3f, 0.4f, 0.25f}, 0.0f};      // Pink
    p.flowerPalette[1] = {{0.8f, 0.6f, 0.9f}, {0.95f, 0.92f, 0.55f}, {0.3f, 0.4f, 0.25f}, 0.0f};     // Lavender
    p.flowerPalette[2] = {{0.98f, 0.98f, 0.95f}, {0.95f, 0.9f, 0.45f}, {0.3f, 0.4f, 0.25f}, 0.0f};   // White
    p.flowerPalette[3] = {{0.95f, 0.95f, 0.5f}, {0.9f, 0.8f, 0.35f}, {0.3f, 0.4f, 0.25f}, 0.0f};     // Pale yellow

    // Other plants - lichens and mosses dominate
    p.fernColor = glm::vec3(0.35f, 0.42f, 0.28f);
    p.mushroomCapColor = glm::vec3(0.7f, 0.65f, 0.58f);
    p.mushroomStemColor = glm::vec3(0.85f, 0.82f, 0.78f);
    p.mushroomGlows = false;
    p.mossColor = glm::vec3(0.42f, 0.52f, 0.35f);
    p.lichenColor = glm::vec3(0.72f, 0.75f, 0.65f);  // Prominent lichen

    // Environment - cold, desaturated
    p.ambientTint = glm::vec3(0.95f, 0.98f, 1.02f);
    p.saturationMultiplier = 0.75f;
    p.brightnessMultiplier = 1.0f;
}

void BiomePaletteManager::createTropicalPalette() {
    BiomePalette& p = m_palettes[static_cast<size_t>(BiomeType::TROPICAL_RAINFOREST)];
    p.biomeType = BiomeType::TROPICAL_RAINFOREST;
    p.name = "Tropical Rainforest";

    // Terrain - dark, rich soil
    p.groundColor = glm::vec3(0.28f, 0.2f, 0.12f);
    p.groundAccentColor = glm::vec3(0.25f, 0.18f, 0.1f);
    p.rockColor = glm::vec3(0.38f, 0.35f, 0.32f);
    p.sandColor = glm::vec3(0.65f, 0.55f, 0.4f);
    p.mudColor = glm::vec3(0.3f, 0.22f, 0.14f);

    // Grass - lush
    p.grassBaseColor = glm::vec3(0.15f, 0.4f, 0.1f);
    p.grassTipColor = glm::vec3(0.2f, 0.48f, 0.15f);
    p.grassDryColor = glm::vec3(0.45f, 0.42f, 0.25f);
    p.grassColorVariation = 0.12f;

    // Trees - tall canopy
    p.treeBarkColor = glm::vec3(0.28f, 0.2f, 0.12f);
    p.treeBarkAccent = glm::vec3(0.22f, 0.16f, 0.1f);
    p.leafColorSpring = glm::vec3(0.25f, 0.55f, 0.18f);
    p.leafColorSummer = glm::vec3(0.12f, 0.42f, 0.08f);
    p.leafColorAutumn = glm::vec3(0.15f, 0.4f, 0.1f);  // Evergreen
    p.leafColorWinter = glm::vec3(0.12f, 0.38f, 0.08f);
    p.leafColorVariation = 0.15f;

    // Bushes
    p.bushLeafColor = glm::vec3(0.18f, 0.42f, 0.12f);
    p.bushBerryColor = glm::vec3(0.8f, 0.2f, 0.25f);

    // Flowers - bright, vibrant tropical colors
    p.numFlowerColors = 6;
    p.flowerPalette[0] = {{0.95f, 0.2f, 0.25f}, {0.95f, 0.9f, 0.4f}, {0.15f, 0.38f, 0.1f}, 0.0f};    // Bright red
    p.flowerPalette[1] = {{0.98f, 0.55f, 0.15f}, {0.95f, 0.85f, 0.3f}, {0.15f, 0.38f, 0.1f}, 0.0f};  // Orange
    p.flowerPalette[2] = {{0.85f, 0.25f, 0.65f}, {0.95f, 0.9f, 0.45f}, {0.15f, 0.38f, 0.1f}, 0.0f};  // Magenta
    p.flowerPalette[3] = {{0.65f, 0.2f, 0.8f}, {0.95f, 0.9f, 0.5f}, {0.15f, 0.38f, 0.1f}, 0.0f};     // Purple
    p.flowerPalette[4] = {{0.98f, 0.95f, 0.35f}, {0.9f, 0.75f, 0.25f}, {0.15f, 0.38f, 0.1f}, 0.0f};  // Bright yellow
    p.flowerPalette[5] = {{0.95f, 0.45f, 0.55f}, {0.98f, 0.92f, 0.5f}, {0.15f, 0.38f, 0.1f}, 0.0f};  // Coral pink

    // Other plants
    p.fernColor = glm::vec3(0.12f, 0.45f, 0.1f);
    p.fernUndersideColor = glm::vec3(0.15f, 0.48f, 0.12f);
    p.mushroomCapColor = glm::vec3(0.9f, 0.55f, 0.25f);
    p.mushroomStemColor = glm::vec3(0.95f, 0.92f, 0.88f);
    p.mushroomGillColor = glm::vec3(0.85f, 0.78f, 0.65f);
    p.mushroomGlows = true;
    p.mushroomGlowColor = glm::vec3(0.4f, 0.9f, 0.5f);
    p.mossColor = glm::vec3(0.18f, 0.5f, 0.15f);
    p.lichenColor = glm::vec3(0.5f, 0.55f, 0.42f);
    p.vineColor = glm::vec3(0.15f, 0.45f, 0.12f);

    // Environment - humid, green-tinted
    p.ambientTint = glm::vec3(0.92f, 1.0f, 0.9f);
    p.saturationMultiplier = 1.1f;
    p.brightnessMultiplier = 0.85f;  // Canopy shade
}

void BiomePaletteManager::createWetlandPalette() {
    BiomePalette& p = m_palettes[static_cast<size_t>(BiomeType::WETLAND)];
    p.biomeType = BiomeType::WETLAND;
    p.name = "Wetland";

    // Terrain - muddy
    p.groundColor = glm::vec3(0.35f, 0.28f, 0.18f);
    p.groundAccentColor = glm::vec3(0.32f, 0.25f, 0.16f);
    p.rockColor = glm::vec3(0.45f, 0.42f, 0.38f);
    p.sandColor = glm::vec3(0.65f, 0.55f, 0.42f);
    p.mudColor = glm::vec3(0.3f, 0.24f, 0.16f);

    // Grass - marsh grass
    p.grassBaseColor = glm::vec3(0.28f, 0.45f, 0.22f);
    p.grassTipColor = glm::vec3(0.38f, 0.52f, 0.28f);
    p.grassDryColor = glm::vec3(0.55f, 0.5f, 0.35f);
    p.grassColorVariation = 0.1f;

    // Trees
    p.treeBarkColor = glm::vec3(0.3f, 0.22f, 0.14f);
    p.treeBarkAccent = glm::vec3(0.25f, 0.18f, 0.12f);
    p.leafColorSpring = glm::vec3(0.35f, 0.55f, 0.25f);
    p.leafColorSummer = glm::vec3(0.22f, 0.48f, 0.18f);
    p.leafColorAutumn = glm::vec3(0.65f, 0.45f, 0.2f);
    p.leafColorWinter = glm::vec3(0.3f, 0.25f, 0.15f);
    p.leafColorVariation = 0.1f;

    // Flowers
    p.numFlowerColors = 5;
    p.flowerPalette[0] = {{0.95f, 0.95f, 0.9f}, {0.95f, 0.9f, 0.5f}, {0.25f, 0.42f, 0.18f}, 0.0f};   // White
    p.flowerPalette[1] = {{0.7f, 0.6f, 0.85f}, {0.92f, 0.88f, 0.5f}, {0.25f, 0.42f, 0.18f}, 0.0f};   // Lavender
    p.flowerPalette[2] = {{0.95f, 0.85f, 0.4f}, {0.9f, 0.75f, 0.3f}, {0.25f, 0.42f, 0.18f}, 0.0f};   // Yellow
    p.flowerPalette[3] = {{0.55f, 0.65f, 0.9f}, {0.9f, 0.88f, 0.55f}, {0.25f, 0.42f, 0.18f}, 0.0f};  // Blue
    p.flowerPalette[4] = {{0.9f, 0.7f, 0.8f}, {0.95f, 0.9f, 0.5f}, {0.25f, 0.42f, 0.18f}, 0.0f};     // Pink

    // Reeds are prominent
    p.reedColor = glm::vec3(0.55f, 0.52f, 0.38f);

    // Other plants
    p.fernColor = glm::vec3(0.22f, 0.45f, 0.18f);
    p.mushroomCapColor = glm::vec3(0.6f, 0.5f, 0.35f);
    p.mushroomStemColor = glm::vec3(0.88f, 0.85f, 0.78f);
    p.mushroomGlows = false;
    p.mossColor = glm::vec3(0.28f, 0.5f, 0.22f);
    p.lichenColor = glm::vec3(0.55f, 0.58f, 0.48f);

    // Environment
    p.ambientTint = glm::vec3(0.95f, 1.0f, 0.95f);
    p.saturationMultiplier = 0.95f;
    p.brightnessMultiplier = 0.95f;

    // Also set swamp
    m_palettes[static_cast<size_t>(BiomeType::SWAMP)] = p;
    m_palettes[static_cast<size_t>(BiomeType::SWAMP)].biomeType = BiomeType::SWAMP;
    m_palettes[static_cast<size_t>(BiomeType::SWAMP)].name = "Swamp";
}

void BiomePaletteManager::createSavannaPalette() {
    BiomePalette& p = m_palettes[static_cast<size_t>(BiomeType::SAVANNA)];
    p.biomeType = BiomeType::SAVANNA;
    p.name = "Savanna";

    // Terrain - golden-brown
    p.groundColor = glm::vec3(0.65f, 0.52f, 0.35f);
    p.groundAccentColor = glm::vec3(0.6f, 0.48f, 0.32f);
    p.rockColor = glm::vec3(0.58f, 0.52f, 0.45f);
    p.sandColor = glm::vec3(0.8f, 0.7f, 0.5f);
    p.mudColor = glm::vec3(0.5f, 0.4f, 0.28f);

    // Grass - tall, golden
    p.grassBaseColor = glm::vec3(0.55f, 0.5f, 0.28f);
    p.grassTipColor = glm::vec3(0.7f, 0.62f, 0.35f);
    p.grassDryColor = glm::vec3(0.78f, 0.7f, 0.42f);
    p.grassColorVariation = 0.1f;

    // Trees - sparse, acacia-like
    p.treeBarkColor = glm::vec3(0.45f, 0.35f, 0.25f);
    p.treeBarkAccent = glm::vec3(0.4f, 0.3f, 0.2f);
    p.leafColorSpring = glm::vec3(0.38f, 0.52f, 0.28f);
    p.leafColorSummer = glm::vec3(0.32f, 0.48f, 0.25f);
    p.leafColorAutumn = glm::vec3(0.55f, 0.48f, 0.28f);
    p.leafColorWinter = glm::vec3(0.4f, 0.35f, 0.22f);
    p.leafColorVariation = 0.08f;

    // Flowers
    p.numFlowerColors = 4;
    p.flowerPalette[0] = {{0.95f, 0.75f, 0.25f}, {0.85f, 0.65f, 0.2f}, {0.4f, 0.48f, 0.25f}, 0.0f};  // Golden
    p.flowerPalette[1] = {{0.9f, 0.4f, 0.25f}, {0.95f, 0.85f, 0.45f}, {0.4f, 0.48f, 0.25f}, 0.0f};   // Orange-red
    p.flowerPalette[2] = {{0.95f, 0.95f, 0.75f}, {0.9f, 0.8f, 0.35f}, {0.4f, 0.48f, 0.25f}, 0.0f};   // Pale yellow
    p.flowerPalette[3] = {{0.85f, 0.6f, 0.65f}, {0.9f, 0.8f, 0.45f}, {0.4f, 0.48f, 0.25f}, 0.0f};    // Dusty pink

    // Other plants
    p.fernColor = glm::vec3(0.35f, 0.45f, 0.25f);
    p.mushroomCapColor = glm::vec3(0.75f, 0.65f, 0.48f);
    p.mushroomStemColor = glm::vec3(0.9f, 0.85f, 0.75f);
    p.mushroomGlows = false;
    p.mossColor = glm::vec3(0.4f, 0.48f, 0.3f);
    p.lichenColor = glm::vec3(0.62f, 0.6f, 0.5f);

    // Environment - warm, golden light
    p.ambientTint = glm::vec3(1.02f, 1.0f, 0.92f);
    p.saturationMultiplier = 0.9f;
    p.brightnessMultiplier = 1.05f;
}

void BiomePaletteManager::createBorealPalette() {
    BiomePalette& p = m_palettes[static_cast<size_t>(BiomeType::BOREAL_FOREST)];
    p.biomeType = BiomeType::BOREAL_FOREST;
    p.name = "Boreal Forest";

    // Terrain - dark, needle-covered
    p.groundColor = glm::vec3(0.32f, 0.25f, 0.18f);
    p.groundAccentColor = glm::vec3(0.28f, 0.22f, 0.15f);
    p.rockColor = glm::vec3(0.48f, 0.45f, 0.42f);
    p.sandColor = glm::vec3(0.68f, 0.6f, 0.48f);
    p.mudColor = glm::vec3(0.3f, 0.24f, 0.18f);

    // Grass - sparse, hardy
    p.grassBaseColor = glm::vec3(0.28f, 0.4f, 0.22f);
    p.grassTipColor = glm::vec3(0.35f, 0.45f, 0.28f);
    p.grassDryColor = glm::vec3(0.5f, 0.48f, 0.35f);
    p.grassColorVariation = 0.08f;

    // Trees - conifers
    p.treeBarkColor = glm::vec3(0.35f, 0.25f, 0.18f);
    p.treeBarkAccent = glm::vec3(0.3f, 0.2f, 0.14f);
    p.leafColorSpring = glm::vec3(0.18f, 0.38f, 0.15f);  // Evergreen needles
    p.leafColorSummer = glm::vec3(0.15f, 0.35f, 0.12f);
    p.leafColorAutumn = glm::vec3(0.15f, 0.32f, 0.12f);
    p.leafColorWinter = glm::vec3(0.12f, 0.28f, 0.1f);
    p.leafColorVariation = 0.06f;

    // Flowers - brief spring bloom
    p.numFlowerColors = 4;
    p.flowerPalette[0] = {{0.98f, 0.98f, 0.95f}, {0.95f, 0.9f, 0.5f}, {0.22f, 0.38f, 0.15f}, 0.0f};  // White
    p.flowerPalette[1] = {{0.85f, 0.7f, 0.85f}, {0.95f, 0.9f, 0.5f}, {0.22f, 0.38f, 0.15f}, 0.0f};   // Pale purple
    p.flowerPalette[2] = {{0.9f, 0.8f, 0.5f}, {0.85f, 0.7f, 0.35f}, {0.22f, 0.38f, 0.15f}, 0.0f};    // Pale yellow
    p.flowerPalette[3] = {{0.95f, 0.75f, 0.8f}, {0.95f, 0.88f, 0.5f}, {0.22f, 0.38f, 0.15f}, 0.0f};  // Pale pink

    // Other plants - moss prominent
    p.fernColor = glm::vec3(0.2f, 0.4f, 0.18f);
    p.mushroomCapColor = glm::vec3(0.7f, 0.45f, 0.28f);
    p.mushroomStemColor = glm::vec3(0.92f, 0.9f, 0.85f);
    p.mushroomGlows = false;
    p.mossColor = glm::vec3(0.32f, 0.52f, 0.28f);  // Prominent
    p.lichenColor = glm::vec3(0.65f, 0.68f, 0.58f);

    // Environment - cool, blue-tinted
    p.ambientTint = glm::vec3(0.95f, 0.98f, 1.0f);
    p.saturationMultiplier = 0.85f;
    p.brightnessMultiplier = 0.92f;
}

void BiomePaletteManager::createAlpinePalette() {
    BiomePalette& p = m_palettes[static_cast<size_t>(BiomeType::ALPINE_MEADOW)];
    p.biomeType = BiomeType::ALPINE_MEADOW;
    p.name = "Alpine Meadow";

    // Terrain - rocky with soil
    p.groundColor = glm::vec3(0.48f, 0.42f, 0.35f);
    p.groundAccentColor = glm::vec3(0.45f, 0.4f, 0.32f);
    p.rockColor = glm::vec3(0.55f, 0.52f, 0.48f);
    p.sandColor = glm::vec3(0.7f, 0.65f, 0.55f);
    p.mudColor = glm::vec3(0.42f, 0.38f, 0.32f);

    // Grass - short, alpine
    p.grassBaseColor = glm::vec3(0.32f, 0.48f, 0.25f);
    p.grassTipColor = glm::vec3(0.42f, 0.55f, 0.32f);
    p.grassDryColor = glm::vec3(0.58f, 0.55f, 0.4f);
    p.grassColorVariation = 0.1f;

    // Trees - none/dwarf
    p.treeBarkColor = glm::vec3(0.4f, 0.32f, 0.22f);
    p.treeBarkAccent = glm::vec3(0.35f, 0.28f, 0.18f);
    p.leafColorSpring = glm::vec3(0.35f, 0.5f, 0.28f);
    p.leafColorSummer = glm::vec3(0.3f, 0.48f, 0.25f);
    p.leafColorAutumn = glm::vec3(0.55f, 0.4f, 0.22f);
    p.leafColorWinter = glm::vec3(0.35f, 0.3f, 0.2f);
    p.leafColorVariation = 0.08f;

    // Flowers - alpine wildflowers, vibrant short season
    p.numFlowerColors = 6;
    p.flowerPalette[0] = {{0.55f, 0.55f, 0.95f}, {0.95f, 0.9f, 0.5f}, {0.28f, 0.45f, 0.22f}, 0.0f};  // Gentian blue
    p.flowerPalette[1] = {{0.98f, 0.98f, 0.95f}, {0.95f, 0.9f, 0.5f}, {0.28f, 0.45f, 0.22f}, 0.0f};  // Edelweiss white
    p.flowerPalette[2] = {{0.95f, 0.75f, 0.2f}, {0.9f, 0.65f, 0.15f}, {0.28f, 0.45f, 0.22f}, 0.0f};  // Alpine gold
    p.flowerPalette[3] = {{0.85f, 0.4f, 0.6f}, {0.95f, 0.88f, 0.5f}, {0.28f, 0.45f, 0.22f}, 0.0f};   // Mountain pink
    p.flowerPalette[4] = {{0.7f, 0.5f, 0.85f}, {0.95f, 0.9f, 0.52f}, {0.28f, 0.45f, 0.22f}, 0.0f};   // Alpine purple
    p.flowerPalette[5] = {{0.95f, 0.6f, 0.35f}, {0.9f, 0.75f, 0.35f}, {0.28f, 0.45f, 0.22f}, 0.0f};  // Orange

    // Other plants
    p.fernColor = glm::vec3(0.28f, 0.45f, 0.22f);
    p.mushroomCapColor = glm::vec3(0.72f, 0.65f, 0.55f);
    p.mushroomStemColor = glm::vec3(0.9f, 0.88f, 0.82f);
    p.mushroomGlows = false;
    p.mossColor = glm::vec3(0.38f, 0.52f, 0.32f);
    p.lichenColor = glm::vec3(0.68f, 0.7f, 0.6f);

    // Environment - clear, bright
    p.ambientTint = glm::vec3(0.98f, 1.0f, 1.02f);
    p.saturationMultiplier = 1.0f;
    p.brightnessMultiplier = 1.05f;
}

void BiomePaletteManager::createVolcanicPalette() {
    BiomePalette& p = m_palettes[static_cast<size_t>(BiomeType::VOLCANIC)];
    p.biomeType = BiomeType::VOLCANIC;
    p.name = "Volcanic";

    // Terrain - dark volcanic rock
    p.groundColor = glm::vec3(0.22f, 0.2f, 0.18f);
    p.groundAccentColor = glm::vec3(0.18f, 0.16f, 0.14f);
    p.rockColor = glm::vec3(0.28f, 0.25f, 0.22f);
    p.sandColor = glm::vec3(0.35f, 0.32f, 0.28f);
    p.mudColor = glm::vec3(0.25f, 0.22f, 0.18f);

    // Grass - sparse, hardy
    p.grassBaseColor = glm::vec3(0.35f, 0.4f, 0.28f);
    p.grassTipColor = glm::vec3(0.42f, 0.45f, 0.32f);
    p.grassDryColor = glm::vec3(0.5f, 0.48f, 0.38f);
    p.grassColorVariation = 0.06f;

    // Trees - rare
    p.treeBarkColor = glm::vec3(0.3f, 0.25f, 0.2f);
    p.treeBarkAccent = glm::vec3(0.25f, 0.2f, 0.15f);
    p.leafColorSpring = glm::vec3(0.35f, 0.45f, 0.28f);
    p.leafColorSummer = glm::vec3(0.3f, 0.42f, 0.25f);
    p.leafColorAutumn = glm::vec3(0.45f, 0.38f, 0.25f);
    p.leafColorWinter = glm::vec3(0.32f, 0.28f, 0.2f);
    p.leafColorVariation = 0.05f;

    // Flowers - minimal
    p.numFlowerColors = 2;
    p.flowerPalette[0] = {{0.95f, 0.5f, 0.2f}, {0.9f, 0.7f, 0.3f}, {0.32f, 0.4f, 0.25f}, 0.0f};   // Orange
    p.flowerPalette[1] = {{0.9f, 0.35f, 0.35f}, {0.95f, 0.85f, 0.4f}, {0.32f, 0.4f, 0.25f}, 0.0f}; // Red

    // Other plants
    p.fernColor = glm::vec3(0.3f, 0.4f, 0.25f);
    p.mushroomCapColor = glm::vec3(0.55f, 0.45f, 0.35f);
    p.mushroomStemColor = glm::vec3(0.75f, 0.72f, 0.68f);
    p.mushroomGlows = true;
    p.mushroomGlowColor = glm::vec3(0.9f, 0.5f, 0.2f);  // Warm glow
    p.mossColor = glm::vec3(0.35f, 0.45f, 0.3f);
    p.lichenColor = glm::vec3(0.5f, 0.48f, 0.42f);

    // Environment - dark, warm
    p.ambientTint = glm::vec3(1.0f, 0.95f, 0.9f);
    p.saturationMultiplier = 0.8f;
    p.brightnessMultiplier = 0.85f;

    // Also set lava field (even more sparse)
    m_palettes[static_cast<size_t>(BiomeType::LAVA_FIELD)] = p;
    m_palettes[static_cast<size_t>(BiomeType::LAVA_FIELD)].biomeType = BiomeType::LAVA_FIELD;
    m_palettes[static_cast<size_t>(BiomeType::LAVA_FIELD)].name = "Lava Field";
    m_palettes[static_cast<size_t>(BiomeType::LAVA_FIELD)].grassBaseColor *= 0.5f;  // Even less vegetation
}

void BiomePaletteManager::createCoastalPalette() {
    BiomePalette& p = m_palettes[static_cast<size_t>(BiomeType::BEACH_SANDY)];
    p.biomeType = BiomeType::BEACH_SANDY;
    p.name = "Sandy Beach";

    // Terrain - sandy
    p.groundColor = glm::vec3(0.88f, 0.82f, 0.65f);
    p.groundAccentColor = glm::vec3(0.85f, 0.78f, 0.6f);
    p.rockColor = glm::vec3(0.6f, 0.55f, 0.48f);
    p.sandColor = glm::vec3(0.92f, 0.88f, 0.72f);
    p.mudColor = glm::vec3(0.55f, 0.48f, 0.38f);

    // Grass - dune grass
    p.grassBaseColor = glm::vec3(0.45f, 0.52f, 0.32f);
    p.grassTipColor = glm::vec3(0.55f, 0.6f, 0.38f);
    p.grassDryColor = glm::vec3(0.68f, 0.65f, 0.48f);
    p.grassColorVariation = 0.1f;

    // Trees - palms, coastal
    p.treeBarkColor = glm::vec3(0.5f, 0.4f, 0.3f);
    p.treeBarkAccent = glm::vec3(0.45f, 0.35f, 0.25f);
    p.leafColorSpring = glm::vec3(0.3f, 0.55f, 0.25f);
    p.leafColorSummer = glm::vec3(0.25f, 0.5f, 0.2f);
    p.leafColorAutumn = glm::vec3(0.35f, 0.5f, 0.25f);
    p.leafColorWinter = glm::vec3(0.28f, 0.45f, 0.2f);
    p.leafColorVariation = 0.08f;

    // Flowers
    p.numFlowerColors = 4;
    p.flowerPalette[0] = {{0.95f, 0.95f, 0.9f}, {0.9f, 0.85f, 0.5f}, {0.35f, 0.48f, 0.28f}, 0.0f};   // White
    p.flowerPalette[1] = {{0.95f, 0.75f, 0.85f}, {0.95f, 0.88f, 0.5f}, {0.35f, 0.48f, 0.28f}, 0.0f}; // Pink
    p.flowerPalette[2] = {{0.95f, 0.9f, 0.4f}, {0.88f, 0.75f, 0.3f}, {0.35f, 0.48f, 0.28f}, 0.0f};   // Yellow
    p.flowerPalette[3] = {{0.7f, 0.65f, 0.9f}, {0.92f, 0.88f, 0.5f}, {0.35f, 0.48f, 0.28f}, 0.0f};   // Lavender

    // Other plants
    p.fernColor = glm::vec3(0.32f, 0.5f, 0.28f);
    p.reedColor = glm::vec3(0.6f, 0.58f, 0.45f);
    p.mossColor = glm::vec3(0.38f, 0.5f, 0.32f);
    p.lichenColor = glm::vec3(0.62f, 0.6f, 0.52f);

    // Environment
    p.ambientTint = glm::vec3(1.0f, 1.0f, 1.02f);
    p.saturationMultiplier = 0.95f;
    p.brightnessMultiplier = 1.05f;

    // Also set rocky beach
    m_palettes[static_cast<size_t>(BiomeType::BEACH_ROCKY)] = p;
    m_palettes[static_cast<size_t>(BiomeType::BEACH_ROCKY)].biomeType = BiomeType::BEACH_ROCKY;
    m_palettes[static_cast<size_t>(BiomeType::BEACH_ROCKY)].name = "Rocky Beach";
    m_palettes[static_cast<size_t>(BiomeType::BEACH_ROCKY)].groundColor = glm::vec3(0.5f, 0.45f, 0.4f);
    m_palettes[static_cast<size_t>(BiomeType::BEACH_ROCKY)].sandColor = glm::vec3(0.72f, 0.68f, 0.58f);
}

void BiomePaletteManager::initializeDistributionRules() {
    // Grass
    {
        PlantDistributionRules& r = m_distributionRules[static_cast<size_t>(PlantCategory::GRASS)];
        r.category = PlantCategory::GRASS;
        r.baseDensity = 1.0f;
        r.densityNearWater = 1.3f;
        r.densityInShade = 0.5f;
        r.clusterRadius = 3.0f;
        r.clusterProbability = 0.2f;
        r.minClusterSize = 10;
        r.maxClusterSize = 50;
        r.minElevation = 0.0f;
        r.maxElevation = 0.85f;
        r.optimalElevation = 0.45f;
        r.minMoisture = 0.2f;
        r.maxMoisture = 0.95f;
        r.optimalMoisture = 0.6f;
        r.minTemperature = -0.5f;
        r.maxTemperature = 0.9f;
        r.optimalTemperature = 0.4f;
        r.minSpacing = 0.05f;
        r.preferredSpacing = 0.15f;
    }

    // Flowers
    {
        PlantDistributionRules& r = m_distributionRules[static_cast<size_t>(PlantCategory::FLOWER)];
        r.category = PlantCategory::FLOWER;
        r.baseDensity = 0.3f;
        r.densityNearWater = 1.5f;
        r.densityInShade = 0.3f;
        r.clusterRadius = 2.0f;
        r.clusterProbability = 0.7f;  // Flowers cluster strongly
        r.minClusterSize = 5;
        r.maxClusterSize = 20;
        r.minElevation = 0.1f;
        r.maxElevation = 0.8f;
        r.optimalElevation = 0.5f;
        r.minMoisture = 0.3f;
        r.maxMoisture = 0.9f;
        r.optimalMoisture = 0.65f;
        r.minTemperature = -0.3f;
        r.maxTemperature = 0.85f;
        r.optimalTemperature = 0.5f;
        r.minSpacing = 0.1f;
        r.preferredSpacing = 0.25f;
    }

    // Bushes
    {
        PlantDistributionRules& r = m_distributionRules[static_cast<size_t>(PlantCategory::BUSH)];
        r.category = PlantCategory::BUSH;
        r.baseDensity = 0.15f;
        r.densityNearWater = 1.2f;
        r.densityInShade = 0.8f;
        r.clusterRadius = 4.0f;
        r.clusterProbability = 0.4f;
        r.minClusterSize = 2;
        r.maxClusterSize = 8;
        r.minElevation = 0.1f;
        r.maxElevation = 0.75f;
        r.optimalElevation = 0.5f;
        r.minMoisture = 0.25f;
        r.maxMoisture = 0.85f;
        r.optimalMoisture = 0.55f;
        r.minTemperature = -0.4f;
        r.maxTemperature = 0.8f;
        r.optimalTemperature = 0.3f;
        r.minSpacing = 1.0f;
        r.preferredSpacing = 3.0f;
    }

    // Ferns
    {
        PlantDistributionRules& r = m_distributionRules[static_cast<size_t>(PlantCategory::FERN)];
        r.category = PlantCategory::FERN;
        r.baseDensity = 0.25f;
        r.densityNearWater = 1.8f;
        r.densityInShade = 1.5f;  // Ferns love shade
        r.clusterRadius = 2.5f;
        r.clusterProbability = 0.6f;
        r.minClusterSize = 3;
        r.maxClusterSize = 15;
        r.minElevation = 0.1f;
        r.maxElevation = 0.7f;
        r.optimalElevation = 0.45f;
        r.minMoisture = 0.5f;  // Need moisture
        r.maxMoisture = 1.0f;
        r.optimalMoisture = 0.8f;
        r.minTemperature = -0.2f;
        r.maxTemperature = 0.8f;
        r.optimalTemperature = 0.5f;
        r.minSpacing = 0.3f;
        r.preferredSpacing = 0.8f;
    }

    // Cacti
    {
        PlantDistributionRules& r = m_distributionRules[static_cast<size_t>(PlantCategory::CACTUS)];
        r.category = PlantCategory::CACTUS;
        r.baseDensity = 0.08f;
        r.densityNearWater = 0.5f;  // Avoid water
        r.densityInShade = 0.3f;
        r.clusterRadius = 5.0f;
        r.clusterProbability = 0.2f;  // More scattered
        r.minClusterSize = 1;
        r.maxClusterSize = 5;
        r.minElevation = 0.2f;
        r.maxElevation = 0.7f;
        r.optimalElevation = 0.45f;
        r.minMoisture = 0.0f;
        r.maxMoisture = 0.3f;  // Dry only
        r.optimalMoisture = 0.1f;
        r.minTemperature = 0.3f;  // Warm only
        r.maxTemperature = 1.0f;
        r.optimalTemperature = 0.7f;
        r.minSpacing = 2.0f;
        r.preferredSpacing = 5.0f;
    }

    // Mushrooms
    {
        PlantDistributionRules& r = m_distributionRules[static_cast<size_t>(PlantCategory::MUSHROOM)];
        r.category = PlantCategory::MUSHROOM;
        r.baseDensity = 0.2f;
        r.densityNearWater = 1.4f;
        r.densityInShade = 2.0f;  // Love shade
        r.clusterRadius = 1.5f;
        r.clusterProbability = 0.8f;  // Cluster heavily
        r.minClusterSize = 3;
        r.maxClusterSize = 12;
        r.minElevation = 0.1f;
        r.maxElevation = 0.7f;
        r.optimalElevation = 0.4f;
        r.minMoisture = 0.5f;
        r.maxMoisture = 1.0f;
        r.optimalMoisture = 0.8f;
        r.minTemperature = -0.3f;
        r.maxTemperature = 0.6f;
        r.optimalTemperature = 0.3f;
        r.minSpacing = 0.1f;
        r.preferredSpacing = 0.3f;
    }

    // Reeds
    {
        PlantDistributionRules& r = m_distributionRules[static_cast<size_t>(PlantCategory::REED)];
        r.category = PlantCategory::REED;
        r.baseDensity = 0.5f;
        r.densityNearWater = 3.0f;  // Only near water
        r.densityInShade = 0.7f;
        r.clusterRadius = 3.0f;
        r.clusterProbability = 0.9f;  // Dense patches
        r.minClusterSize = 10;
        r.maxClusterSize = 40;
        r.minElevation = 0.0f;
        r.maxElevation = 0.45f;  // Low areas only
        r.optimalElevation = 0.38f;
        r.minMoisture = 0.8f;  // Very wet only
        r.maxMoisture = 1.0f;
        r.optimalMoisture = 0.95f;
        r.minTemperature = -0.4f;
        r.maxTemperature = 0.8f;
        r.optimalTemperature = 0.4f;
        r.minSpacing = 0.1f;
        r.preferredSpacing = 0.2f;
    }

    // Moss
    {
        PlantDistributionRules& r = m_distributionRules[static_cast<size_t>(PlantCategory::MOSS)];
        r.category = PlantCategory::MOSS;
        r.baseDensity = 0.4f;
        r.densityNearWater = 1.5f;
        r.densityInShade = 2.0f;
        r.clusterRadius = 2.0f;
        r.clusterProbability = 0.7f;
        r.minClusterSize = 5;
        r.maxClusterSize = 25;
        r.minElevation = 0.0f;
        r.maxElevation = 0.8f;
        r.optimalElevation = 0.4f;
        r.minMoisture = 0.5f;
        r.maxMoisture = 1.0f;
        r.optimalMoisture = 0.75f;
        r.minTemperature = -0.6f;
        r.maxTemperature = 0.6f;
        r.optimalTemperature = 0.2f;
        r.minSpacing = 0.05f;
        r.preferredSpacing = 0.1f;
    }

    // Lichen
    {
        PlantDistributionRules& r = m_distributionRules[static_cast<size_t>(PlantCategory::LICHEN)];
        r.category = PlantCategory::LICHEN;
        r.baseDensity = 0.3f;
        r.densityNearWater = 0.8f;
        r.densityInShade = 1.2f;
        r.clusterRadius = 1.5f;
        r.clusterProbability = 0.6f;
        r.minClusterSize = 3;
        r.maxClusterSize = 15;
        r.minElevation = 0.3f;  // Higher elevations
        r.maxElevation = 1.0f;
        r.optimalElevation = 0.7f;
        r.minMoisture = 0.2f;
        r.maxMoisture = 0.8f;
        r.optimalMoisture = 0.5f;
        r.minTemperature = -0.8f;  // Can survive cold
        r.maxTemperature = 0.5f;
        r.optimalTemperature = 0.0f;
        r.minSpacing = 0.05f;
        r.preferredSpacing = 0.15f;
    }

    // Initialize remaining categories with defaults
    for (size_t i = static_cast<size_t>(PlantCategory::VINE); i < static_cast<size_t>(PlantCategory::COUNT); ++i) {
        PlantDistributionRules& r = m_distributionRules[i];
        r.category = static_cast<PlantCategory>(i);
        r.baseDensity = 0.1f;
        r.densityNearWater = 1.0f;
        r.densityInShade = 1.0f;
        r.clusterRadius = 2.0f;
        r.clusterProbability = 0.5f;
        r.minClusterSize = 2;
        r.maxClusterSize = 10;
        r.minElevation = 0.1f;
        r.maxElevation = 0.8f;
        r.optimalElevation = 0.5f;
        r.minMoisture = 0.3f;
        r.maxMoisture = 0.8f;
        r.optimalMoisture = 0.5f;
        r.minTemperature = -0.3f;
        r.maxTemperature = 0.7f;
        r.optimalTemperature = 0.4f;
        r.minSpacing = 0.5f;
        r.preferredSpacing = 1.0f;
    }
}

void BiomePaletteManager::initializeNutrition() {
    // Grass
    {
        PlantNutrition& n = m_nutrition[static_cast<size_t>(PlantCategory::GRASS)];
        n.category = PlantCategory::GRASS;
        n.energyValue = 0.4f;
        n.hydrationValue = 0.3f;
        n.proteinValue = 0.15f;
        n.fiberValue = 0.8f;
        n.toxicity = 0.0f;
        n.digestibility = 0.6f;
        n.satiation = 0.5f;
        n.isHallucinogenic = false;
        n.isMedicinal = false;
        n.isStimulant = false;
        n.isSedative = false;
        n.herbivorePreference = 1.0f;
        n.omnivorePreference = 0.3f;
        n.preferredBySmall = 0.6f;
        n.preferredByLarge = 1.0f;
        n.requiresSpecialization = 0.0f;
    }

    // Flowers
    {
        PlantNutrition& n = m_nutrition[static_cast<size_t>(PlantCategory::FLOWER)];
        n.category = PlantCategory::FLOWER;
        n.energyValue = 0.2f;
        n.hydrationValue = 0.4f;
        n.proteinValue = 0.1f;
        n.fiberValue = 0.3f;
        n.toxicity = 0.1f;  // Some mild toxicity possible
        n.digestibility = 0.8f;
        n.satiation = 0.2f;
        n.isHallucinogenic = false;
        n.isMedicinal = true;  // Some flowers are medicinal
        n.isStimulant = false;
        n.isSedative = false;
        n.herbivorePreference = 0.5f;
        n.omnivorePreference = 0.4f;
        n.preferredBySmall = 0.8f;
        n.preferredByLarge = 0.3f;
        n.requiresSpecialization = 0.0f;
    }

    // Bushes (berries)
    {
        PlantNutrition& n = m_nutrition[static_cast<size_t>(PlantCategory::BUSH)];
        n.category = PlantCategory::BUSH;
        n.energyValue = 0.6f;  // Berries are nutritious
        n.hydrationValue = 0.5f;
        n.proteinValue = 0.1f;
        n.fiberValue = 0.4f;
        n.toxicity = 0.15f;  // Some berries are toxic
        n.digestibility = 0.9f;
        n.satiation = 0.6f;
        n.isHallucinogenic = false;
        n.isMedicinal = false;
        n.isStimulant = false;
        n.isSedative = false;
        n.herbivorePreference = 0.9f;
        n.omnivorePreference = 0.8f;
        n.preferredBySmall = 0.9f;
        n.preferredByLarge = 0.5f;
        n.requiresSpecialization = 0.0f;
    }

    // Ferns
    {
        PlantNutrition& n = m_nutrition[static_cast<size_t>(PlantCategory::FERN)];
        n.category = PlantCategory::FERN;
        n.energyValue = 0.3f;
        n.hydrationValue = 0.4f;
        n.proteinValue = 0.2f;
        n.fiberValue = 0.7f;
        n.toxicity = 0.2f;  // Many ferns are mildly toxic
        n.digestibility = 0.5f;
        n.satiation = 0.4f;
        n.isHallucinogenic = false;
        n.isMedicinal = false;
        n.isStimulant = false;
        n.isSedative = false;
        n.herbivorePreference = 0.6f;
        n.omnivorePreference = 0.2f;
        n.preferredBySmall = 0.5f;
        n.preferredByLarge = 0.7f;
        n.requiresSpecialization = 0.3f;  // Some adaptation needed
    }

    // Cacti
    {
        PlantNutrition& n = m_nutrition[static_cast<size_t>(PlantCategory::CACTUS)];
        n.category = PlantCategory::CACTUS;
        n.energyValue = 0.5f;
        n.hydrationValue = 0.9f;  // High water content
        n.proteinValue = 0.1f;
        n.fiberValue = 0.5f;
        n.toxicity = 0.3f;  // Spines and some toxins
        n.digestibility = 0.4f;
        n.satiation = 0.7f;
        n.isHallucinogenic = true;  // Some cacti are psychoactive
        n.isMedicinal = true;
        n.isStimulant = false;
        n.isSedative = false;
        n.herbivorePreference = 0.4f;
        n.omnivorePreference = 0.3f;
        n.preferredBySmall = 0.2f;
        n.preferredByLarge = 0.6f;
        n.requiresSpecialization = 0.7f;  // Need adaptation to eat
    }

    // Mushrooms
    {
        PlantNutrition& n = m_nutrition[static_cast<size_t>(PlantCategory::MUSHROOM)];
        n.category = PlantCategory::MUSHROOM;
        n.energyValue = 0.3f;
        n.hydrationValue = 0.6f;
        n.proteinValue = 0.4f;  // Relatively high protein
        n.fiberValue = 0.3f;
        n.toxicity = 0.4f;  // Many are toxic
        n.digestibility = 0.7f;
        n.satiation = 0.3f;
        n.isHallucinogenic = true;
        n.isMedicinal = true;
        n.isStimulant = false;
        n.isSedative = true;
        n.herbivorePreference = 0.5f;
        n.omnivorePreference = 0.6f;
        n.preferredBySmall = 0.7f;
        n.preferredByLarge = 0.4f;
        n.requiresSpecialization = 0.5f;  // Need to know which are safe
    }

    // Reeds
    {
        PlantNutrition& n = m_nutrition[static_cast<size_t>(PlantCategory::REED)];
        n.category = PlantCategory::REED;
        n.energyValue = 0.25f;
        n.hydrationValue = 0.5f;
        n.proteinValue = 0.1f;
        n.fiberValue = 0.9f;  // Very fibrous
        n.toxicity = 0.0f;
        n.digestibility = 0.3f;  // Hard to digest
        n.satiation = 0.4f;
        n.isHallucinogenic = false;
        n.isMedicinal = false;
        n.isStimulant = false;
        n.isSedative = false;
        n.herbivorePreference = 0.5f;
        n.omnivorePreference = 0.1f;
        n.preferredBySmall = 0.3f;
        n.preferredByLarge = 0.6f;
        n.requiresSpecialization = 0.4f;
    }

    // Moss
    {
        PlantNutrition& n = m_nutrition[static_cast<size_t>(PlantCategory::MOSS)];
        n.category = PlantCategory::MOSS;
        n.energyValue = 0.15f;
        n.hydrationValue = 0.7f;
        n.proteinValue = 0.05f;
        n.fiberValue = 0.4f;
        n.toxicity = 0.05f;
        n.digestibility = 0.5f;
        n.satiation = 0.2f;
        n.isHallucinogenic = false;
        n.isMedicinal = true;  // Some mosses are medicinal
        n.isStimulant = false;
        n.isSedative = false;
        n.herbivorePreference = 0.3f;
        n.omnivorePreference = 0.1f;
        n.preferredBySmall = 0.8f;
        n.preferredByLarge = 0.2f;
        n.requiresSpecialization = 0.0f;
    }

    // Lichen
    {
        PlantNutrition& n = m_nutrition[static_cast<size_t>(PlantCategory::LICHEN)];
        n.category = PlantCategory::LICHEN;
        n.energyValue = 0.2f;
        n.hydrationValue = 0.3f;
        n.proteinValue = 0.1f;
        n.fiberValue = 0.5f;
        n.toxicity = 0.1f;
        n.digestibility = 0.4f;
        n.satiation = 0.25f;
        n.isHallucinogenic = false;
        n.isMedicinal = true;
        n.isStimulant = false;
        n.isSedative = false;
        n.herbivorePreference = 0.4f;  // Important for tundra animals
        n.omnivorePreference = 0.1f;
        n.preferredBySmall = 0.5f;
        n.preferredByLarge = 0.6f;  // Caribou/reindeer love it
        n.requiresSpecialization = 0.2f;
    }

    // Initialize remaining with defaults
    for (size_t i = static_cast<size_t>(PlantCategory::VINE); i < static_cast<size_t>(PlantCategory::COUNT); ++i) {
        PlantNutrition& n = m_nutrition[i];
        n.category = static_cast<PlantCategory>(i);
        n.energyValue = 0.3f;
        n.hydrationValue = 0.4f;
        n.proteinValue = 0.1f;
        n.fiberValue = 0.5f;
        n.toxicity = 0.1f;
        n.digestibility = 0.6f;
        n.satiation = 0.3f;
        n.isHallucinogenic = false;
        n.isMedicinal = false;
        n.isStimulant = false;
        n.isSedative = false;
        n.herbivorePreference = 0.5f;
        n.omnivorePreference = 0.3f;
        n.preferredBySmall = 0.5f;
        n.preferredByLarge = 0.5f;
        n.requiresSpecialization = 0.2f;
    }
}

// ============================================================================
// Utility Functions Implementation
// ============================================================================

BiomePalette getDefaultBiomePalette(BiomeType biome) {
    BiomePaletteManager manager;
    return manager.getPalette(biome);
}

BiomePalette blendPalettes(const BiomePalette& a, const BiomePalette& b, float t) {
    BiomePalette result;

    result.biomeType = t < 0.5f ? a.biomeType : b.biomeType;
    result.name = t < 0.5f ? a.name : b.name;

    // Blend all colors
    result.groundColor = glm::mix(a.groundColor, b.groundColor, t);
    result.groundAccentColor = glm::mix(a.groundAccentColor, b.groundAccentColor, t);
    result.rockColor = glm::mix(a.rockColor, b.rockColor, t);
    result.sandColor = glm::mix(a.sandColor, b.sandColor, t);
    result.mudColor = glm::mix(a.mudColor, b.mudColor, t);

    result.grassBaseColor = glm::mix(a.grassBaseColor, b.grassBaseColor, t);
    result.grassTipColor = glm::mix(a.grassTipColor, b.grassTipColor, t);
    result.grassDryColor = glm::mix(a.grassDryColor, b.grassDryColor, t);
    result.grassColorVariation = glm::mix(a.grassColorVariation, b.grassColorVariation, t);

    result.treeBarkColor = glm::mix(a.treeBarkColor, b.treeBarkColor, t);
    result.treeBarkAccent = glm::mix(a.treeBarkAccent, b.treeBarkAccent, t);
    result.leafColorSpring = glm::mix(a.leafColorSpring, b.leafColorSpring, t);
    result.leafColorSummer = glm::mix(a.leafColorSummer, b.leafColorSummer, t);
    result.leafColorAutumn = glm::mix(a.leafColorAutumn, b.leafColorAutumn, t);
    result.leafColorWinter = glm::mix(a.leafColorWinter, b.leafColorWinter, t);
    result.leafColorVariation = glm::mix(a.leafColorVariation, b.leafColorVariation, t);

    result.bushLeafColor = glm::mix(a.bushLeafColor, b.bushLeafColor, t);
    result.bushBerryColor = glm::mix(a.bushBerryColor, b.bushBerryColor, t);

    result.fernColor = glm::mix(a.fernColor, b.fernColor, t);
    result.fernUndersideColor = glm::mix(a.fernUndersideColor, b.fernUndersideColor, t);

    result.mushroomCapColor = glm::mix(a.mushroomCapColor, b.mushroomCapColor, t);
    result.mushroomStemColor = glm::mix(a.mushroomStemColor, b.mushroomStemColor, t);
    result.mushroomGillColor = glm::mix(a.mushroomGillColor, b.mushroomGillColor, t);
    result.mushroomGlows = t < 0.5f ? a.mushroomGlows : b.mushroomGlows;
    result.mushroomGlowColor = glm::mix(a.mushroomGlowColor, b.mushroomGlowColor, t);

    result.cactusColor = glm::mix(a.cactusColor, b.cactusColor, t);
    result.reedColor = glm::mix(a.reedColor, b.reedColor, t);
    result.mossColor = glm::mix(a.mossColor, b.mossColor, t);
    result.lichenColor = glm::mix(a.lichenColor, b.lichenColor, t);
    result.vineColor = glm::mix(a.vineColor, b.vineColor, t);

    result.ambientTint = glm::mix(a.ambientTint, b.ambientTint, t);
    result.saturationMultiplier = glm::mix(a.saturationMultiplier, b.saturationMultiplier, t);
    result.brightnessMultiplier = glm::mix(a.brightnessMultiplier, b.brightnessMultiplier, t);

    // Blend flower colors (use from palette with more colors, or blend)
    result.numFlowerColors = std::max(a.numFlowerColors, b.numFlowerColors);
    for (int i = 0; i < result.numFlowerColors; ++i) {
        if (i < a.numFlowerColors && i < b.numFlowerColors) {
            result.flowerPalette[i].petalColor = glm::mix(a.flowerPalette[i].petalColor, b.flowerPalette[i].petalColor, t);
            result.flowerPalette[i].centerColor = glm::mix(a.flowerPalette[i].centerColor, b.flowerPalette[i].centerColor, t);
            result.flowerPalette[i].stemColor = glm::mix(a.flowerPalette[i].stemColor, b.flowerPalette[i].stemColor, t);
            result.flowerPalette[i].glowIntensity = glm::mix(a.flowerPalette[i].glowIntensity, b.flowerPalette[i].glowIntensity, t);
        } else if (i < a.numFlowerColors) {
            result.flowerPalette[i] = a.flowerPalette[i];
        } else {
            result.flowerPalette[i] = b.flowerPalette[i];
        }
    }

    return result;
}

float getSeasonalModifier(float dayOfYear) {
    // dayOfYear: 0-365
    // Returns 0 = deep winter, 0.5 = peak summer, 1.0 = deep winter again
    float normalized = dayOfYear / 365.0f;

    // Use cosine to get smooth seasonal curve
    // Peak summer at day 182 (July 1), winter at day 0 and 365
    return (1.0f - std::cos(normalized * 2.0f * 3.14159f)) * 0.5f;
}

std::vector<FlowerPatchColor> getBiomeFlowerColors(BiomeType biome) {
    BiomePaletteManager manager;
    const BiomePalette& palette = manager.getPalette(biome);

    std::vector<FlowerPatchColor> colors;
    for (int i = 0; i < palette.numFlowerColors; ++i) {
        colors.push_back(palette.flowerPalette[i]);
    }
    return colors;
}

const char* plantCategoryToString(PlantCategory category) {
    switch (category) {
        case PlantCategory::GRASS: return "Grass";
        case PlantCategory::FLOWER: return "Flower";
        case PlantCategory::BUSH: return "Bush";
        case PlantCategory::FERN: return "Fern";
        case PlantCategory::CACTUS: return "Cactus";
        case PlantCategory::MUSHROOM: return "Mushroom";
        case PlantCategory::REED: return "Reed";
        case PlantCategory::MOSS: return "Moss";
        case PlantCategory::LICHEN: return "Lichen";
        case PlantCategory::VINE: return "Vine";
        case PlantCategory::SUCCULENT: return "Succulent";
        case PlantCategory::AQUATIC_PLANT: return "Aquatic Plant";
        default: return "Unknown";
    }
}

// ============================================================================
// ColorRamp Implementation
// ============================================================================

glm::vec3 ColorRamp::sample(float warmth, float saturation) const {
    // Clamp inputs
    warmth = std::clamp(warmth, -1.0f, 1.0f);
    saturation = std::clamp(saturation, 0.0f, 1.0f);

    // First interpolate between cool-neutral-warm based on warmth
    glm::vec3 tempColor;
    if (warmth < 0.0f) {
        // Interpolate cool to neutral
        tempColor = glm::mix(cool, neutral, warmth + 1.0f);
    } else {
        // Interpolate neutral to warm
        tempColor = glm::mix(neutral, warm, warmth);
    }

    // Then blend with muted/vibrant based on saturation
    glm::vec3 satColor;
    if (saturation < 0.5f) {
        // Lean towards muted
        satColor = glm::mix(muted, tempColor, saturation * 2.0f);
    } else {
        // Lean towards vibrant
        satColor = glm::mix(tempColor, vibrant, (saturation - 0.5f) * 2.0f);
    }

    return glm::clamp(satColor, glm::vec3(0.0f), glm::vec3(1.0f));
}

// ============================================================================
// BiomePaletteRamps Implementation
// ============================================================================

BiomePalette BiomePaletteRamps::generatePalette(float warmth, float saturation, uint32_t seed) const {
    BiomePalette palette;
    palette.biomeType = biomeType;
    palette.name = "Generated";

    // Clamp to constraints
    warmth = std::clamp(warmth, -constraints.maxWarmthShift, constraints.maxWarmthShift);

    // Sample all color ramps
    palette.grassBaseColor = grassBase.sample(warmth, saturation);
    palette.grassTipColor = grassTip.sample(warmth, saturation);
    palette.grassDryColor = grassDry.sample(warmth, saturation);
    palette.grassColorVariation = 0.1f + (saturation * 0.1f);

    palette.leafColorSpring = leafSpring.sample(warmth, saturation);
    palette.leafColorSummer = leafSummer.sample(warmth, saturation);
    palette.leafColorAutumn = leafAutumn.sample(warmth, saturation);
    palette.leafColorWinter = palette.leafColorAutumn * 0.6f;
    palette.leafColorVariation = 0.08f + (saturation * 0.08f);

    palette.groundColor = groundBase.sample(warmth, saturation);
    palette.groundAccentColor = palette.groundColor * 0.9f;
    palette.rockColor = rockBase.sample(warmth, saturation);
    palette.sandColor = sandBase.sample(warmth, saturation);
    palette.mudColor = palette.groundColor * 0.85f;

    // Tree bark - derived from ground with variation
    palette.treeBarkColor = palette.groundColor * 0.8f;
    palette.treeBarkAccent = palette.treeBarkColor * 0.85f;

    // Bush colors
    palette.bushLeafColor = palette.leafColorSummer * 1.05f;
    palette.bushBerryColor = glm::vec3(0.6f, 0.15f, 0.15f);

    // Fern and moss - derived from grass
    palette.fernColor = palette.grassBaseColor * 0.9f;
    palette.fernUndersideColor = palette.fernColor * 1.1f;
    palette.mossColor = glm::mix(palette.grassBaseColor, glm::vec3(0.2f, 0.4f, 0.15f), 0.5f);
    palette.lichenColor = glm::vec3(0.6f, 0.58f, 0.5f);
    palette.vineColor = palette.leafColorSummer;

    // Mushroom colors
    palette.mushroomCapColor = glm::vec3(0.7f, 0.55f, 0.4f);
    palette.mushroomStemColor = glm::vec3(0.92f, 0.9f, 0.85f);
    palette.mushroomGillColor = glm::vec3(0.75f, 0.7f, 0.6f);
    palette.mushroomGlows = false;

    // Environment tint based on warmth
    if (warmth < 0.0f) {
        palette.ambientTint = glm::vec3(0.95f, 0.98f, 1.0f);  // Cool blue tint
    } else {
        palette.ambientTint = glm::vec3(1.0f, 0.98f, 0.95f);  // Warm yellow tint
    }

    palette.saturationMultiplier = 0.85f + saturation * 0.3f;
    palette.brightnessMultiplier = 0.95f + warmth * 0.1f;

    // Generate flower colors using seed for variety
    palette.numFlowerColors = 5;
    auto seedFloat = [seed](int index) -> float {
        uint32_t h = seed + index * 7919;
        h = (h ^ (h >> 15)) * 0x85ebca6b;
        h = (h ^ (h >> 13)) * 0xc2b2ae35;
        return static_cast<float>(h & 0xFFFFFF) / static_cast<float>(0xFFFFFF);
    };

    for (int i = 0; i < 5; ++i) {
        float hue = seedFloat(i) * 360.0f;
        float sat = 0.5f + seedFloat(i + 10) * 0.4f;
        float val = 0.7f + seedFloat(i + 20) * 0.25f;

        // Convert HSV to RGB
        float c = val * sat;
        float x = c * (1.0f - std::abs(std::fmod(hue / 60.0f, 2.0f) - 1.0f));
        float m = val - c;

        glm::vec3 rgb;
        if (hue < 60) rgb = {c, x, 0};
        else if (hue < 120) rgb = {x, c, 0};
        else if (hue < 180) rgb = {0, c, x};
        else if (hue < 240) rgb = {0, x, c};
        else if (hue < 300) rgb = {x, 0, c};
        else rgb = {c, 0, x};

        palette.flowerPalette[i].petalColor = rgb + glm::vec3(m);
        palette.flowerPalette[i].centerColor = glm::vec3(0.9f, 0.85f, 0.4f);
        palette.flowerPalette[i].stemColor = palette.grassBaseColor;
        palette.flowerPalette[i].glowIntensity = 0.0f;
    }

    return palette;
}

// ============================================================================
// PaletteRampRegistry Implementation
// ============================================================================

PaletteRampRegistry& PaletteRampRegistry::getInstance() {
    static PaletteRampRegistry instance;
    return instance;
}

PaletteRampRegistry::PaletteRampRegistry() {
    initializeRamps();
}

void PaletteRampRegistry::initializeRamps() {
    // Initialize default ramps
    m_defaultRamps.biomeType = BiomeType::GRASSLAND;
    m_defaultRamps.grassBase = {
        {0.2f, 0.45f, 0.25f},   // cool
        {0.25f, 0.45f, 0.18f},  // neutral
        {0.35f, 0.45f, 0.15f},  // warm
        {0.3f, 0.55f, 0.2f},    // vibrant
        {0.3f, 0.4f, 0.25f}     // muted
    };
    m_defaultRamps.grassTip = {
        {0.3f, 0.55f, 0.35f},
        {0.35f, 0.55f, 0.22f},
        {0.45f, 0.55f, 0.18f},
        {0.4f, 0.65f, 0.25f},
        {0.38f, 0.48f, 0.28f}
    };
    m_defaultRamps.grassDry = {
        {0.5f, 0.5f, 0.35f},
        {0.6f, 0.55f, 0.32f},
        {0.7f, 0.55f, 0.28f},
        {0.65f, 0.6f, 0.35f},
        {0.55f, 0.5f, 0.38f}
    };
    m_defaultRamps.leafSpring = {
        {0.35f, 0.55f, 0.35f},
        {0.4f, 0.58f, 0.28f},
        {0.5f, 0.58f, 0.22f},
        {0.45f, 0.65f, 0.3f},
        {0.4f, 0.5f, 0.32f}
    };
    m_defaultRamps.leafSummer = {
        {0.15f, 0.45f, 0.2f},
        {0.2f, 0.48f, 0.15f},
        {0.28f, 0.45f, 0.12f},
        {0.22f, 0.55f, 0.18f},
        {0.22f, 0.4f, 0.2f}
    };
    m_defaultRamps.leafAutumn = {
        {0.6f, 0.4f, 0.2f},
        {0.75f, 0.45f, 0.15f},
        {0.85f, 0.45f, 0.1f},
        {0.9f, 0.5f, 0.15f},
        {0.6f, 0.45f, 0.25f}
    };
    m_defaultRamps.groundBase = {
        {0.35f, 0.3f, 0.28f},
        {0.4f, 0.32f, 0.22f},
        {0.48f, 0.35f, 0.2f},
        {0.45f, 0.35f, 0.25f},
        {0.38f, 0.32f, 0.28f}
    };
    m_defaultRamps.rockBase = {
        {0.45f, 0.45f, 0.5f},
        {0.5f, 0.48f, 0.45f},
        {0.55f, 0.48f, 0.42f},
        {0.52f, 0.5f, 0.48f},
        {0.48f, 0.46f, 0.45f}
    };
    m_defaultRamps.sandBase = {
        {0.75f, 0.72f, 0.6f},
        {0.82f, 0.75f, 0.52f},
        {0.88f, 0.75f, 0.45f},
        {0.85f, 0.78f, 0.55f},
        {0.78f, 0.72f, 0.58f}
    };

    // Create specific biome ramps
    createGrasslandRamps();
    createForestRamps();
    createDesertRamps();
    createTundraRamps();
    createTropicalRamps();
    createWetlandRamps();
    createSavannaRamps();
    createBorealRamps();
    createAlpineRamps();
    createVolcanicRamps();
    createCoastalRamps();

    // Copy defaults to uninitialized biomes
    for (size_t i = 0; i < m_ramps.size(); ++i) {
        if (m_ramps[i].biomeType == BiomeType::DEEP_OCEAN) {
            m_ramps[i] = m_defaultRamps;
            m_ramps[i].biomeType = static_cast<BiomeType>(i);
        }
    }
}

const BiomePaletteRamps& PaletteRampRegistry::getRamps(BiomeType biome) const {
    size_t index = static_cast<size_t>(biome);
    if (index < m_ramps.size()) {
        return m_ramps[index];
    }
    return m_defaultRamps;
}

BiomePalette PaletteRampRegistry::generateVariedPalette(BiomeType biome, uint32_t seed) const {
    const BiomePaletteRamps& ramps = getRamps(biome);

    // Derive warmth and saturation from seed
    auto seedToFloat = [](uint32_t s) -> float {
        return static_cast<float>(s) / static_cast<float>(UINT32_MAX);
    };

    // Use different bits of seed for different parameters
    float warmth = (seedToFloat(seed) * 2.0f - 1.0f) * ramps.constraints.maxWarmthShift;
    float saturation = 0.3f + seedToFloat(seed >> 16) * 0.7f;

    return ramps.generatePalette(warmth, saturation, seed);
}

bool PaletteRampRegistry::validateContrast(const BiomePalette& palette,
                                            const std::vector<BiomeType>& neighbors) const {
    auto colorDistance = [](const glm::vec3& a, const glm::vec3& b) -> float {
        glm::vec3 diff = a - b;
        return std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    };

    for (BiomeType neighbor : neighbors) {
        const BiomePaletteRamps& neighborRamps = getRamps(neighbor);
        BiomePalette neighborPalette = neighborRamps.generatePalette(0.0f, 0.5f, 12345);

        // Check grass contrast
        if (colorDistance(palette.grassBaseColor, neighborPalette.grassBaseColor) < 0.15f) {
            return false;
        }

        // Check ground contrast
        if (colorDistance(palette.groundColor, neighborPalette.groundColor) < 0.1f) {
            return false;
        }
    }

    return true;
}

float PaletteRampRegistry::getVegetationDensityModifier(BiomeType biome, uint32_t seed) const {
    // Base density modifiers per biome
    float baseDensity = 1.0f;
    switch (biome) {
        case BiomeType::TROPICAL_RAINFOREST: baseDensity = 1.5f; break;
        case BiomeType::TEMPERATE_FOREST: baseDensity = 1.3f; break;
        case BiomeType::BOREAL_FOREST: baseDensity = 1.2f; break;
        case BiomeType::GRASSLAND: baseDensity = 1.0f; break;
        case BiomeType::SAVANNA: baseDensity = 0.7f; break;
        case BiomeType::DESERT_HOT: baseDensity = 0.2f; break;
        case BiomeType::TUNDRA: baseDensity = 0.4f; break;
        case BiomeType::ALPINE_MEADOW: baseDensity = 0.8f; break;
        case BiomeType::WETLAND: baseDensity = 1.1f; break;
        default: break;
    }

    // Add seed-based variation (+/- 20%)
    float variation = static_cast<float>(seed & 0xFFFF) / static_cast<float>(0xFFFF);
    variation = 0.8f + variation * 0.4f;

    return baseDensity * variation;
}

// ============================================================================
// Biome-Specific Ramp Creation
// ============================================================================

void PaletteRampRegistry::createGrasslandRamps() {
    BiomePaletteRamps& r = m_ramps[static_cast<size_t>(BiomeType::GRASSLAND)];
    r.biomeType = BiomeType::GRASSLAND;

    // Grassland has wide green variation with golden undertones
    r.grassBase = {
        {0.2f, 0.5f, 0.25f},    // cool - blue-green
        {0.3f, 0.5f, 0.18f},    // neutral
        {0.4f, 0.48f, 0.12f},   // warm - yellow-green
        {0.35f, 0.6f, 0.2f},    // vibrant
        {0.32f, 0.42f, 0.25f}   // muted
    };
    r.grassTip = {
        {0.3f, 0.6f, 0.32f},
        {0.4f, 0.58f, 0.22f},
        {0.5f, 0.55f, 0.18f},
        {0.45f, 0.68f, 0.25f},
        {0.4f, 0.5f, 0.28f}
    };
    r.grassDry = {
        {0.55f, 0.55f, 0.38f},
        {0.68f, 0.6f, 0.35f},
        {0.78f, 0.62f, 0.3f},
        {0.72f, 0.65f, 0.38f},
        {0.6f, 0.55f, 0.4f}
    };
    r.leafSummer = r.grassBase;
    r.leafSpring = r.grassTip;
    r.leafAutumn = {
        {0.6f, 0.45f, 0.2f},
        {0.75f, 0.48f, 0.15f},
        {0.85f, 0.5f, 0.1f},
        {0.9f, 0.55f, 0.15f},
        {0.65f, 0.48f, 0.25f}
    };
    r.groundBase = {
        {0.38f, 0.32f, 0.28f},
        {0.45f, 0.35f, 0.22f},
        {0.52f, 0.38f, 0.18f},
        {0.48f, 0.38f, 0.25f},
        {0.42f, 0.35f, 0.28f}
    };
    r.rockBase = m_defaultRamps.rockBase;
    r.sandBase = m_defaultRamps.sandBase;

    r.constraints.maxWarmthShift = 0.4f;
}

void PaletteRampRegistry::createForestRamps() {
    BiomePaletteRamps& r = m_ramps[static_cast<size_t>(BiomeType::TEMPERATE_FOREST)];
    r.biomeType = BiomeType::TEMPERATE_FOREST;

    // Forest has darker, richer greens
    r.grassBase = {
        {0.15f, 0.4f, 0.2f},
        {0.2f, 0.38f, 0.12f},
        {0.25f, 0.36f, 0.1f},
        {0.22f, 0.45f, 0.15f},
        {0.2f, 0.35f, 0.18f}
    };
    r.grassTip = {
        {0.22f, 0.48f, 0.25f},
        {0.28f, 0.45f, 0.18f},
        {0.32f, 0.42f, 0.15f},
        {0.3f, 0.52f, 0.2f},
        {0.26f, 0.4f, 0.22f}
    };
    r.grassDry = {
        {0.45f, 0.45f, 0.32f},
        {0.55f, 0.48f, 0.28f},
        {0.62f, 0.5f, 0.25f},
        {0.58f, 0.52f, 0.3f},
        {0.5f, 0.46f, 0.32f}
    };
    r.leafSpring = {
        {0.35f, 0.55f, 0.32f},
        {0.4f, 0.58f, 0.25f},
        {0.48f, 0.55f, 0.2f},
        {0.45f, 0.65f, 0.28f},
        {0.38f, 0.5f, 0.3f}
    };
    r.leafSummer = {
        {0.12f, 0.42f, 0.18f},
        {0.18f, 0.45f, 0.12f},
        {0.25f, 0.42f, 0.1f},
        {0.2f, 0.52f, 0.15f},
        {0.18f, 0.38f, 0.18f}
    };
    r.leafAutumn = {
        {0.7f, 0.4f, 0.18f},
        {0.85f, 0.45f, 0.12f},
        {0.92f, 0.48f, 0.08f},
        {0.95f, 0.55f, 0.12f},
        {0.7f, 0.45f, 0.22f}
    };
    r.groundBase = {
        {0.3f, 0.25f, 0.2f},
        {0.35f, 0.25f, 0.15f},
        {0.4f, 0.28f, 0.12f},
        {0.38f, 0.28f, 0.18f},
        {0.32f, 0.26f, 0.2f}
    };
    r.rockBase = {
        {0.42f, 0.42f, 0.45f},
        {0.45f, 0.42f, 0.38f},
        {0.48f, 0.42f, 0.35f},
        {0.46f, 0.44f, 0.4f},
        {0.44f, 0.42f, 0.42f}
    };
    r.sandBase = m_defaultRamps.sandBase;

    r.constraints.maxWarmthShift = 0.3f;
}

void PaletteRampRegistry::createDesertRamps() {
    BiomePaletteRamps& r = m_ramps[static_cast<size_t>(BiomeType::DESERT_HOT)];
    r.biomeType = BiomeType::DESERT_HOT;

    // Desert has warm sandy tones with sparse muted vegetation
    r.grassBase = {
        {0.45f, 0.45f, 0.35f},
        {0.55f, 0.5f, 0.32f},
        {0.62f, 0.52f, 0.28f},
        {0.58f, 0.55f, 0.35f},
        {0.5f, 0.48f, 0.38f}
    };
    r.grassTip = {
        {0.52f, 0.52f, 0.4f},
        {0.62f, 0.55f, 0.35f},
        {0.7f, 0.58f, 0.32f},
        {0.65f, 0.6f, 0.38f},
        {0.56f, 0.52f, 0.4f}
    };
    r.grassDry = {
        {0.65f, 0.6f, 0.45f},
        {0.75f, 0.65f, 0.42f},
        {0.82f, 0.68f, 0.38f},
        {0.78f, 0.68f, 0.45f},
        {0.68f, 0.62f, 0.48f}
    };
    r.leafSpring = r.grassBase;
    r.leafSummer = r.grassBase;
    r.leafAutumn = r.grassDry;
    r.groundBase = {
        {0.75f, 0.65f, 0.48f},
        {0.85f, 0.72f, 0.5f},
        {0.9f, 0.75f, 0.45f},
        {0.88f, 0.75f, 0.52f},
        {0.78f, 0.68f, 0.52f}
    };
    r.rockBase = {
        {0.6f, 0.55f, 0.48f},
        {0.7f, 0.6f, 0.5f},
        {0.75f, 0.62f, 0.48f},
        {0.72f, 0.62f, 0.52f},
        {0.65f, 0.58f, 0.52f}
    };
    r.sandBase = {
        {0.82f, 0.75f, 0.58f},
        {0.92f, 0.82f, 0.58f},
        {0.95f, 0.85f, 0.52f},
        {0.93f, 0.85f, 0.6f},
        {0.85f, 0.78f, 0.62f}
    };

    r.constraints.maxWarmthShift = 0.5f;  // Desert can be more varied
}

void PaletteRampRegistry::createTundraRamps() {
    BiomePaletteRamps& r = m_ramps[static_cast<size_t>(BiomeType::TUNDRA)];
    r.biomeType = BiomeType::TUNDRA;

    // Tundra is cold with sparse hardy vegetation
    r.grassBase = {
        {0.28f, 0.38f, 0.32f},
        {0.35f, 0.42f, 0.3f},
        {0.4f, 0.45f, 0.28f},
        {0.38f, 0.48f, 0.32f},
        {0.32f, 0.38f, 0.32f}
    };
    r.grassTip = {
        {0.35f, 0.45f, 0.38f},
        {0.42f, 0.48f, 0.35f},
        {0.48f, 0.5f, 0.32f},
        {0.45f, 0.55f, 0.38f},
        {0.38f, 0.45f, 0.38f}
    };
    r.grassDry = {
        {0.48f, 0.48f, 0.4f},
        {0.55f, 0.52f, 0.38f},
        {0.6f, 0.55f, 0.35f},
        {0.58f, 0.55f, 0.4f},
        {0.5f, 0.5f, 0.42f}
    };
    r.leafSpring = r.grassBase;
    r.leafSummer = r.grassBase;
    r.leafAutumn = r.grassDry;
    r.groundBase = {
        {0.45f, 0.42f, 0.4f},
        {0.52f, 0.48f, 0.42f},
        {0.58f, 0.52f, 0.4f},
        {0.55f, 0.5f, 0.44f},
        {0.48f, 0.45f, 0.42f}
    };
    r.rockBase = {
        {0.5f, 0.5f, 0.52f},
        {0.55f, 0.52f, 0.5f},
        {0.58f, 0.54f, 0.48f},
        {0.56f, 0.54f, 0.52f},
        {0.52f, 0.5f, 0.5f}
    };
    r.sandBase = m_defaultRamps.sandBase;

    r.constraints.maxWarmthShift = 0.25f;  // Tundra stays cold
}

void PaletteRampRegistry::createTropicalRamps() {
    BiomePaletteRamps& r = m_ramps[static_cast<size_t>(BiomeType::TROPICAL_RAINFOREST)];
    r.biomeType = BiomeType::TROPICAL_RAINFOREST;

    // Tropical is vibrant and saturated
    r.grassBase = {
        {0.15f, 0.5f, 0.25f},
        {0.1f, 0.48f, 0.15f},
        {0.15f, 0.45f, 0.1f},
        {0.12f, 0.55f, 0.18f},
        {0.15f, 0.42f, 0.2f}
    };
    r.grassTip = {
        {0.22f, 0.58f, 0.3f},
        {0.18f, 0.55f, 0.2f},
        {0.22f, 0.52f, 0.15f},
        {0.2f, 0.62f, 0.22f},
        {0.2f, 0.48f, 0.25f}
    };
    r.grassDry = r.grassBase;  // Tropical doesn't really have dry grass
    r.leafSpring = {
        {0.18f, 0.55f, 0.3f},
        {0.12f, 0.52f, 0.2f},
        {0.18f, 0.5f, 0.15f},
        {0.15f, 0.6f, 0.22f},
        {0.15f, 0.48f, 0.25f}
    };
    r.leafSummer = r.leafSpring;
    r.leafAutumn = r.leafSpring;  // Evergreen
    r.groundBase = {
        {0.28f, 0.22f, 0.18f},
        {0.32f, 0.24f, 0.15f},
        {0.38f, 0.28f, 0.12f},
        {0.35f, 0.26f, 0.16f},
        {0.3f, 0.24f, 0.18f}
    };
    r.rockBase = {
        {0.4f, 0.4f, 0.42f},
        {0.45f, 0.42f, 0.38f},
        {0.48f, 0.44f, 0.35f},
        {0.46f, 0.44f, 0.4f},
        {0.42f, 0.4f, 0.4f}
    };
    r.sandBase = m_defaultRamps.sandBase;

    r.constraints.maxWarmthShift = 0.35f;
}

void PaletteRampRegistry::createWetlandRamps() {
    BiomePaletteRamps& r = m_ramps[static_cast<size_t>(BiomeType::WETLAND)];
    r.biomeType = BiomeType::WETLAND;

    // Wetland has murky greens and browns
    r.grassBase = {
        {0.2f, 0.42f, 0.25f},
        {0.25f, 0.45f, 0.2f},
        {0.3f, 0.45f, 0.18f},
        {0.28f, 0.5f, 0.22f},
        {0.25f, 0.4f, 0.24f}
    };
    r.grassTip = {
        {0.28f, 0.48f, 0.3f},
        {0.32f, 0.5f, 0.25f},
        {0.38f, 0.5f, 0.22f},
        {0.35f, 0.55f, 0.28f},
        {0.3f, 0.45f, 0.28f}
    };
    r.grassDry = {
        {0.4f, 0.42f, 0.32f},
        {0.48f, 0.48f, 0.3f},
        {0.52f, 0.5f, 0.28f},
        {0.5f, 0.5f, 0.32f},
        {0.45f, 0.45f, 0.34f}
    };
    r.leafSpring = r.grassTip;
    r.leafSummer = r.grassBase;
    r.leafAutumn = r.grassDry;
    r.groundBase = {
        {0.3f, 0.26f, 0.22f},
        {0.35f, 0.28f, 0.2f},
        {0.4f, 0.32f, 0.18f},
        {0.38f, 0.3f, 0.22f},
        {0.32f, 0.28f, 0.24f}
    };
    r.rockBase = m_defaultRamps.rockBase;
    r.sandBase = m_defaultRamps.sandBase;
    r.wetlandAccent = {
        {0.2f, 0.35f, 0.28f},
        {0.25f, 0.38f, 0.25f},
        {0.3f, 0.4f, 0.22f},
        {0.28f, 0.42f, 0.28f},
        {0.24f, 0.35f, 0.28f}
    };

    r.constraints.maxWarmthShift = 0.3f;
}

void PaletteRampRegistry::createSavannaRamps() {
    BiomePaletteRamps& r = m_ramps[static_cast<size_t>(BiomeType::SAVANNA)];
    r.biomeType = BiomeType::SAVANNA;

    // Savanna is golden with sparse trees
    r.grassBase = {
        {0.5f, 0.48f, 0.32f},
        {0.6f, 0.55f, 0.28f},
        {0.7f, 0.58f, 0.22f},
        {0.65f, 0.6f, 0.3f},
        {0.55f, 0.52f, 0.35f}
    };
    r.grassTip = {
        {0.58f, 0.55f, 0.38f},
        {0.68f, 0.6f, 0.32f},
        {0.78f, 0.62f, 0.25f},
        {0.72f, 0.65f, 0.35f},
        {0.62f, 0.58f, 0.4f}
    };
    r.grassDry = {
        {0.7f, 0.65f, 0.45f},
        {0.8f, 0.7f, 0.4f},
        {0.88f, 0.72f, 0.35f},
        {0.82f, 0.72f, 0.42f},
        {0.72f, 0.68f, 0.48f}
    };
    r.leafSpring = {
        {0.35f, 0.48f, 0.28f},
        {0.4f, 0.5f, 0.22f},
        {0.48f, 0.5f, 0.18f},
        {0.45f, 0.55f, 0.25f},
        {0.38f, 0.46f, 0.28f}
    };
    r.leafSummer = r.leafSpring;
    r.leafAutumn = r.grassDry;
    r.groundBase = {
        {0.55f, 0.48f, 0.38f},
        {0.65f, 0.55f, 0.38f},
        {0.72f, 0.58f, 0.35f},
        {0.68f, 0.58f, 0.4f},
        {0.58f, 0.52f, 0.42f}
    };
    r.rockBase = m_defaultRamps.rockBase;
    r.sandBase = m_defaultRamps.sandBase;

    r.constraints.maxWarmthShift = 0.4f;
}

void PaletteRampRegistry::createBorealRamps() {
    BiomePaletteRamps& r = m_ramps[static_cast<size_t>(BiomeType::BOREAL_FOREST)];
    r.biomeType = BiomeType::BOREAL_FOREST;

    // Boreal is dark evergreen
    r.grassBase = {
        {0.22f, 0.38f, 0.25f},
        {0.28f, 0.4f, 0.22f},
        {0.32f, 0.42f, 0.2f},
        {0.3f, 0.45f, 0.24f},
        {0.26f, 0.36f, 0.24f}
    };
    r.grassTip = {
        {0.28f, 0.42f, 0.3f},
        {0.35f, 0.45f, 0.25f},
        {0.4f, 0.46f, 0.22f},
        {0.38f, 0.5f, 0.28f},
        {0.32f, 0.4f, 0.28f}
    };
    r.grassDry = {
        {0.42f, 0.42f, 0.35f},
        {0.5f, 0.48f, 0.32f},
        {0.55f, 0.5f, 0.3f},
        {0.52f, 0.5f, 0.35f},
        {0.45f, 0.45f, 0.36f}
    };
    r.leafSpring = {
        {0.15f, 0.35f, 0.18f},
        {0.18f, 0.38f, 0.15f},
        {0.22f, 0.38f, 0.12f},
        {0.2f, 0.42f, 0.16f},
        {0.18f, 0.34f, 0.18f}
    };
    r.leafSummer = r.leafSpring;  // Evergreen
    r.leafAutumn = r.leafSpring;
    r.groundBase = {
        {0.28f, 0.24f, 0.2f},
        {0.32f, 0.25f, 0.18f},
        {0.38f, 0.28f, 0.15f},
        {0.35f, 0.27f, 0.18f},
        {0.3f, 0.25f, 0.2f}
    };
    r.rockBase = m_defaultRamps.rockBase;
    r.sandBase = m_defaultRamps.sandBase;

    r.constraints.maxWarmthShift = 0.25f;
}

void PaletteRampRegistry::createAlpineRamps() {
    BiomePaletteRamps& r = m_ramps[static_cast<size_t>(BiomeType::ALPINE_MEADOW)];
    r.biomeType = BiomeType::ALPINE_MEADOW;

    // Alpine is short but vibrant in summer
    r.grassBase = {
        {0.28f, 0.45f, 0.3f},
        {0.32f, 0.48f, 0.25f},
        {0.38f, 0.48f, 0.22f},
        {0.35f, 0.55f, 0.28f},
        {0.3f, 0.42f, 0.28f}
    };
    r.grassTip = {
        {0.35f, 0.52f, 0.35f},
        {0.42f, 0.55f, 0.3f},
        {0.48f, 0.54f, 0.28f},
        {0.45f, 0.6f, 0.32f},
        {0.38f, 0.48f, 0.32f}
    };
    r.grassDry = {
        {0.5f, 0.5f, 0.4f},
        {0.58f, 0.55f, 0.38f},
        {0.65f, 0.58f, 0.35f},
        {0.6f, 0.58f, 0.4f},
        {0.52f, 0.52f, 0.42f}
    };
    r.leafSpring = r.grassTip;
    r.leafSummer = r.grassBase;
    r.leafAutumn = r.grassDry;
    r.groundBase = {
        {0.42f, 0.4f, 0.38f},
        {0.48f, 0.42f, 0.35f},
        {0.52f, 0.45f, 0.32f},
        {0.5f, 0.44f, 0.36f},
        {0.45f, 0.42f, 0.38f}
    };
    r.rockBase = {
        {0.52f, 0.52f, 0.55f},
        {0.55f, 0.52f, 0.48f},
        {0.58f, 0.54f, 0.45f},
        {0.56f, 0.54f, 0.5f},
        {0.53f, 0.52f, 0.52f}
    };
    r.sandBase = m_defaultRamps.sandBase;

    r.constraints.maxWarmthShift = 0.3f;
}

void PaletteRampRegistry::createVolcanicRamps() {
    BiomePaletteRamps& r = m_ramps[static_cast<size_t>(BiomeType::VOLCANIC)];
    r.biomeType = BiomeType::VOLCANIC;

    // Volcanic is dark with ash and hardy pioneer plants
    r.grassBase = {
        {0.25f, 0.35f, 0.28f},
        {0.3f, 0.38f, 0.22f},
        {0.35f, 0.4f, 0.18f},
        {0.32f, 0.42f, 0.25f},
        {0.28f, 0.34f, 0.26f}
    };
    r.grassTip = {
        {0.32f, 0.4f, 0.32f},
        {0.38f, 0.42f, 0.28f},
        {0.42f, 0.44f, 0.22f},
        {0.4f, 0.48f, 0.3f},
        {0.35f, 0.38f, 0.3f}
    };
    r.grassDry = {
        {0.38f, 0.38f, 0.35f},
        {0.45f, 0.42f, 0.32f},
        {0.5f, 0.45f, 0.28f},
        {0.48f, 0.45f, 0.34f},
        {0.4f, 0.4f, 0.36f}
    };
    r.leafSpring = r.grassBase;
    r.leafSummer = r.grassBase;
    r.leafAutumn = r.grassDry;
    r.groundBase = {
        {0.25f, 0.25f, 0.28f},
        {0.3f, 0.3f, 0.32f},
        {0.35f, 0.32f, 0.3f},
        {0.32f, 0.3f, 0.32f},
        {0.28f, 0.28f, 0.3f}
    };
    r.rockBase = {
        {0.32f, 0.32f, 0.35f},
        {0.38f, 0.38f, 0.4f},
        {0.42f, 0.4f, 0.38f},
        {0.4f, 0.38f, 0.4f},
        {0.35f, 0.35f, 0.38f}
    };
    r.sandBase = {
        {0.35f, 0.35f, 0.38f},
        {0.4f, 0.4f, 0.42f},
        {0.45f, 0.42f, 0.4f},
        {0.42f, 0.4f, 0.42f},
        {0.38f, 0.38f, 0.4f}
    };

    r.constraints.maxWarmthShift = 0.2f;
}

void PaletteRampRegistry::createCoastalRamps() {
    BiomePaletteRamps& r = m_ramps[static_cast<size_t>(BiomeType::BEACH_SANDY)];
    r.biomeType = BiomeType::BEACH_SANDY;

    // Coastal has sparse vegetation with sandy tones
    r.grassBase = {
        {0.35f, 0.45f, 0.32f},
        {0.4f, 0.48f, 0.28f},
        {0.48f, 0.5f, 0.25f},
        {0.45f, 0.52f, 0.3f},
        {0.38f, 0.44f, 0.32f}
    };
    r.grassTip = {
        {0.42f, 0.5f, 0.38f},
        {0.48f, 0.52f, 0.32f},
        {0.55f, 0.54f, 0.28f},
        {0.52f, 0.56f, 0.35f},
        {0.45f, 0.48f, 0.36f}
    };
    r.grassDry = {
        {0.55f, 0.55f, 0.42f},
        {0.62f, 0.58f, 0.38f},
        {0.68f, 0.6f, 0.35f},
        {0.65f, 0.6f, 0.4f},
        {0.58f, 0.56f, 0.44f}
    };
    r.leafSpring = r.grassBase;
    r.leafSummer = r.grassBase;
    r.leafAutumn = r.grassDry;
    r.groundBase = {
        {0.7f, 0.68f, 0.58f},
        {0.78f, 0.72f, 0.55f},
        {0.82f, 0.75f, 0.5f},
        {0.8f, 0.74f, 0.56f},
        {0.72f, 0.7f, 0.6f}
    };
    r.rockBase = m_defaultRamps.rockBase;
    r.sandBase = {
        {0.85f, 0.8f, 0.65f},
        {0.92f, 0.85f, 0.62f},
        {0.95f, 0.88f, 0.55f},
        {0.93f, 0.87f, 0.65f},
        {0.88f, 0.82f, 0.68f}
    };

    r.constraints.maxWarmthShift = 0.35f;
}
