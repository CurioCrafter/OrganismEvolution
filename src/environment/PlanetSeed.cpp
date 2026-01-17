#include "PlanetSeed.h"
#include "PlanetTheme.h"
#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>

// ============================================================================
// THEME PROFILE REGISTRY IMPLEMENTATION
// ============================================================================

const ThemeProfileRegistry& ThemeProfileRegistry::getInstance() {
    static ThemeProfileRegistry instance;
    return instance;
}

ThemeProfileRegistry::ThemeProfileRegistry() {
    initializeProfiles();
}

void ThemeProfileRegistry::initializeProfiles() {
    m_profiles.clear();
    m_profiles.reserve(12);

    // ===== COMMON THEMES (40% total) =====

    // Earth-like (most common)
    {
        ThemeProfile profile;
        profile.name = "Terra Prime";
        profile.basePreset = PlanetPreset::EARTH_LIKE;
        profile.rarity = ThemeRarity::COMMON;
        profile.weight = 2.0f;

        profile.ranges.skyHueMin = 200.0f; profile.ranges.skyHueMax = 220.0f;
        profile.ranges.skySatMin = 0.4f; profile.ranges.skySatMax = 0.7f;
        profile.ranges.skyBrightMin = 0.6f; profile.ranges.skyBrightMax = 0.9f;
        profile.ranges.fogDensityMin = 0.01f; profile.ranges.fogDensityMax = 0.03f;
        profile.ranges.fogDistanceMin = 40.0f; profile.ranges.fogDistanceMax = 80.0f;
        profile.ranges.waterHueMin = 190.0f; profile.ranges.waterHueMax = 220.0f;
        profile.ranges.waterClarityMin = 0.5f; profile.ranges.waterClarityMax = 0.9f;
        profile.ranges.sunHueMin = 35.0f; profile.ranges.sunHueMax = 50.0f;
        profile.ranges.sunIntensityMin = 0.9f; profile.ranges.sunIntensityMax = 1.1f;
        profile.ranges.biomeSaturationMin = 0.9f; profile.ranges.biomeSaturationMax = 1.1f;
        profile.ranges.warmthMin = -0.1f; profile.ranges.warmthMax = 0.2f;

        m_profiles.push_back(profile);
    }

    // Ocean World (common)
    {
        ThemeProfile profile;
        profile.name = "Aqua Depths";
        profile.basePreset = PlanetPreset::OCEAN_WORLD;
        profile.rarity = ThemeRarity::COMMON;
        profile.weight = 1.5f;

        profile.ranges.skyHueMin = 190.0f; profile.ranges.skyHueMax = 230.0f;
        profile.ranges.skySatMin = 0.3f; profile.ranges.skySatMax = 0.6f;
        profile.ranges.skyBrightMin = 0.7f; profile.ranges.skyBrightMax = 0.95f;
        profile.ranges.fogDensityMin = 0.02f; profile.ranges.fogDensityMax = 0.05f;
        profile.ranges.fogDistanceMin = 30.0f; profile.ranges.fogDistanceMax = 60.0f;
        profile.ranges.waterHueMin = 180.0f; profile.ranges.waterHueMax = 210.0f;
        profile.ranges.waterClarityMin = 0.6f; profile.ranges.waterClarityMax = 1.0f;
        profile.ranges.sunHueMin = 40.0f; profile.ranges.sunHueMax = 55.0f;
        profile.ranges.sunIntensityMin = 0.85f; profile.ranges.sunIntensityMax = 1.05f;
        profile.ranges.biomeSaturationMin = 0.8f; profile.ranges.biomeSaturationMax = 1.0f;
        profile.ranges.warmthMin = 0.0f; profile.ranges.warmthMax = 0.3f;

        profile.biomeWeights.tropicalWeight = 1.5f;
        profile.biomeWeights.wetlandWeight = 1.3f;
        profile.biomeWeights.desertWeight = 0.3f;

        m_profiles.push_back(profile);
    }

    // Temperate Forest (common)
    {
        ThemeProfile profile;
        profile.name = "Verdant Haven";
        profile.basePreset = PlanetPreset::EARTH_LIKE;
        profile.rarity = ThemeRarity::COMMON;
        profile.weight = 1.0f;

        profile.ranges.skyHueMin = 195.0f; profile.ranges.skyHueMax = 215.0f;
        profile.ranges.skySatMin = 0.35f; profile.ranges.skySatMax = 0.55f;
        profile.ranges.skyBrightMin = 0.65f; profile.ranges.skyBrightMax = 0.85f;
        profile.ranges.fogDensityMin = 0.015f; profile.ranges.fogDensityMax = 0.04f;
        profile.ranges.fogDistanceMin = 35.0f; profile.ranges.fogDistanceMax = 70.0f;
        profile.ranges.waterHueMin = 185.0f; profile.ranges.waterHueMax = 205.0f;
        profile.ranges.waterClarityMin = 0.4f; profile.ranges.waterClarityMax = 0.8f;
        profile.ranges.sunHueMin = 38.0f; profile.ranges.sunHueMax = 52.0f;
        profile.ranges.sunIntensityMin = 0.8f; profile.ranges.sunIntensityMax = 1.0f;
        profile.ranges.biomeSaturationMin = 1.0f; profile.ranges.biomeSaturationMax = 1.2f;
        profile.ranges.warmthMin = -0.1f; profile.ranges.warmthMax = 0.1f;

        profile.biomeWeights.forestWeight = 1.8f;
        profile.biomeWeights.wetlandWeight = 1.2f;
        profile.biomeWeights.desertWeight = 0.4f;
        profile.biomeWeights.tundraWeight = 0.5f;

        m_profiles.push_back(profile);
    }

    // ===== UNCOMMON THEMES (35% total) =====

    // Desert World
    {
        ThemeProfile profile;
        profile.name = "Dune Expanse";
        profile.basePreset = PlanetPreset::DESERT_WORLD;
        profile.rarity = ThemeRarity::UNCOMMON;
        profile.weight = 1.2f;

        profile.ranges.skyHueMin = 25.0f; profile.ranges.skyHueMax = 50.0f;
        profile.ranges.skySatMin = 0.2f; profile.ranges.skySatMax = 0.5f;
        profile.ranges.skyBrightMin = 0.75f; profile.ranges.skyBrightMax = 0.95f;
        profile.ranges.fogDensityMin = 0.005f; profile.ranges.fogDensityMax = 0.02f;
        profile.ranges.fogDistanceMin = 60.0f; profile.ranges.fogDistanceMax = 120.0f;
        profile.ranges.waterHueMin = 170.0f; profile.ranges.waterHueMax = 200.0f;
        profile.ranges.waterClarityMin = 0.3f; profile.ranges.waterClarityMax = 0.7f;
        profile.ranges.sunHueMin = 30.0f; profile.ranges.sunHueMax = 45.0f;
        profile.ranges.sunIntensityMin = 1.0f; profile.ranges.sunIntensityMax = 1.3f;
        profile.ranges.biomeSaturationMin = 0.7f; profile.ranges.biomeSaturationMax = 0.95f;
        profile.ranges.warmthMin = 0.3f; profile.ranges.warmthMax = 0.6f;

        profile.biomeWeights.desertWeight = 2.5f;
        profile.biomeWeights.forestWeight = 0.2f;
        profile.biomeWeights.wetlandWeight = 0.1f;
        profile.biomeWeights.tropicalWeight = 0.3f;

        m_profiles.push_back(profile);
    }

    // Frozen World
    {
        ThemeProfile profile;
        profile.name = "Glacial Reach";
        profile.basePreset = PlanetPreset::FROZEN_WORLD;
        profile.rarity = ThemeRarity::UNCOMMON;
        profile.weight = 1.0f;

        profile.ranges.skyHueMin = 200.0f; profile.ranges.skyHueMax = 230.0f;
        profile.ranges.skySatMin = 0.15f; profile.ranges.skySatMax = 0.35f;
        profile.ranges.skyBrightMin = 0.7f; profile.ranges.skyBrightMax = 0.9f;
        profile.ranges.fogDensityMin = 0.03f; profile.ranges.fogDensityMax = 0.08f;
        profile.ranges.fogDistanceMin = 25.0f; profile.ranges.fogDistanceMax = 50.0f;
        profile.ranges.waterHueMin = 195.0f; profile.ranges.waterHueMax = 220.0f;
        profile.ranges.waterClarityMin = 0.7f; profile.ranges.waterClarityMax = 1.0f;
        profile.ranges.sunHueMin = 45.0f; profile.ranges.sunHueMax = 60.0f;
        profile.ranges.sunIntensityMin = 0.6f; profile.ranges.sunIntensityMax = 0.85f;
        profile.ranges.biomeSaturationMin = 0.5f; profile.ranges.biomeSaturationMax = 0.8f;
        profile.ranges.warmthMin = -0.5f; profile.ranges.warmthMax = -0.2f;

        profile.biomeWeights.tundraWeight = 3.0f;
        profile.biomeWeights.mountainWeight = 1.5f;
        profile.biomeWeights.forestWeight = 0.4f;
        profile.biomeWeights.desertWeight = 0.5f;
        profile.biomeWeights.tropicalWeight = 0.0f;

        m_profiles.push_back(profile);
    }

    // Volcanic World
    {
        ThemeProfile profile;
        profile.name = "Infernal Forge";
        profile.basePreset = PlanetPreset::VOLCANIC_WORLD;
        profile.rarity = ThemeRarity::UNCOMMON;
        profile.weight = 0.8f;

        profile.ranges.skyHueMin = 10.0f; profile.ranges.skyHueMax = 35.0f;
        profile.ranges.skySatMin = 0.3f; profile.ranges.skySatMax = 0.6f;
        profile.ranges.skyBrightMin = 0.4f; profile.ranges.skyBrightMax = 0.65f;
        profile.ranges.fogDensityMin = 0.04f; profile.ranges.fogDensityMax = 0.1f;
        profile.ranges.fogDistanceMin = 20.0f; profile.ranges.fogDistanceMax = 40.0f;
        profile.ranges.waterHueMin = 10.0f; profile.ranges.waterHueMax = 30.0f;
        profile.ranges.waterClarityMin = 0.2f; profile.ranges.waterClarityMax = 0.5f;
        profile.ranges.sunHueMin = 15.0f; profile.ranges.sunHueMax = 35.0f;
        profile.ranges.sunIntensityMin = 0.7f; profile.ranges.sunIntensityMax = 1.0f;
        profile.ranges.biomeSaturationMin = 0.6f; profile.ranges.biomeSaturationMax = 0.9f;
        profile.ranges.warmthMin = 0.4f; profile.ranges.warmthMax = 0.8f;

        profile.biomeWeights.volcanicWeight = 3.0f;
        profile.biomeWeights.mountainWeight = 1.5f;
        profile.biomeWeights.desertWeight = 1.2f;
        profile.biomeWeights.forestWeight = 0.3f;
        profile.biomeWeights.tundraWeight = 0.0f;

        m_profiles.push_back(profile);
    }

    // Ancient World
    {
        ThemeProfile profile;
        profile.name = "Elder Realm";
        profile.basePreset = PlanetPreset::ANCIENT_WORLD;
        profile.rarity = ThemeRarity::UNCOMMON;
        profile.weight = 0.8f;

        profile.ranges.skyHueMin = 180.0f; profile.ranges.skyHueMax = 210.0f;
        profile.ranges.skySatMin = 0.2f; profile.ranges.skySatMax = 0.4f;
        profile.ranges.skyBrightMin = 0.5f; profile.ranges.skyBrightMax = 0.75f;
        profile.ranges.fogDensityMin = 0.025f; profile.ranges.fogDensityMax = 0.06f;
        profile.ranges.fogDistanceMin = 30.0f; profile.ranges.fogDistanceMax = 55.0f;
        profile.ranges.waterHueMin = 170.0f; profile.ranges.waterHueMax = 195.0f;
        profile.ranges.waterClarityMin = 0.3f; profile.ranges.waterClarityMax = 0.6f;
        profile.ranges.sunHueMin = 35.0f; profile.ranges.sunHueMax = 50.0f;
        profile.ranges.sunIntensityMin = 0.65f; profile.ranges.sunIntensityMax = 0.85f;
        profile.ranges.biomeSaturationMin = 0.6f; profile.ranges.biomeSaturationMax = 0.85f;
        profile.ranges.warmthMin = -0.1f; profile.ranges.warmthMax = 0.15f;

        profile.biomeWeights.forestWeight = 1.4f;
        profile.biomeWeights.wetlandWeight = 1.3f;
        profile.biomeWeights.mountainWeight = 1.2f;

        m_profiles.push_back(profile);
    }

    // ===== RARE THEMES (20% total) =====

    // Alien Purple
    {
        ThemeProfile profile;
        profile.name = "Violet Nexus";
        profile.basePreset = PlanetPreset::ALIEN_PURPLE;
        profile.rarity = ThemeRarity::RARE;
        profile.weight = 1.2f;

        profile.ranges.skyHueMin = 260.0f; profile.ranges.skyHueMax = 300.0f;
        profile.ranges.skySatMin = 0.4f; profile.ranges.skySatMax = 0.7f;
        profile.ranges.skyBrightMin = 0.5f; profile.ranges.skyBrightMax = 0.8f;
        profile.ranges.fogDensityMin = 0.02f; profile.ranges.fogDensityMax = 0.05f;
        profile.ranges.fogDistanceMin = 35.0f; profile.ranges.fogDistanceMax = 65.0f;
        profile.ranges.waterHueMin = 240.0f; profile.ranges.waterHueMax = 280.0f;
        profile.ranges.waterClarityMin = 0.4f; profile.ranges.waterClarityMax = 0.8f;
        profile.ranges.sunHueMin = 30.0f; profile.ranges.sunHueMax = 50.0f;
        profile.ranges.sunIntensityMin = 0.8f; profile.ranges.sunIntensityMax = 1.0f;
        profile.ranges.biomeSaturationMin = 1.0f; profile.ranges.biomeSaturationMax = 1.4f;
        profile.ranges.warmthMin = -0.2f; profile.ranges.warmthMax = 0.1f;

        m_profiles.push_back(profile);
    }

    // Alien Red
    {
        ThemeProfile profile;
        profile.name = "Crimson Horizon";
        profile.basePreset = PlanetPreset::ALIEN_RED;
        profile.rarity = ThemeRarity::RARE;
        profile.weight = 1.0f;

        profile.ranges.skyHueMin = 15.0f; profile.ranges.skyHueMax = 45.0f;
        profile.ranges.skySatMin = 0.35f; profile.ranges.skySatMax = 0.65f;
        profile.ranges.skyBrightMin = 0.55f; profile.ranges.skyBrightMax = 0.8f;
        profile.ranges.fogDensityMin = 0.025f; profile.ranges.fogDensityMax = 0.055f;
        profile.ranges.fogDistanceMin = 30.0f; profile.ranges.fogDistanceMax = 60.0f;
        profile.ranges.waterHueMin = 350.0f; profile.ranges.waterHueMax = 20.0f;
        profile.ranges.waterClarityMin = 0.3f; profile.ranges.waterClarityMax = 0.7f;
        profile.ranges.sunHueMin = 40.0f; profile.ranges.sunHueMax = 60.0f;
        profile.ranges.sunIntensityMin = 0.75f; profile.ranges.sunIntensityMax = 1.0f;
        profile.ranges.biomeSaturationMin = 0.9f; profile.ranges.biomeSaturationMax = 1.3f;
        profile.ranges.warmthMin = 0.2f; profile.ranges.warmthMax = 0.5f;

        m_profiles.push_back(profile);
    }

    // Toxic World
    {
        ThemeProfile profile;
        profile.name = "Venomous Mire";
        profile.basePreset = PlanetPreset::TOXIC_WORLD;
        profile.rarity = ThemeRarity::RARE;
        profile.weight = 0.8f;

        profile.ranges.skyHueMin = 70.0f; profile.ranges.skyHueMax = 110.0f;
        profile.ranges.skySatMin = 0.4f; profile.ranges.skySatMax = 0.7f;
        profile.ranges.skyBrightMin = 0.45f; profile.ranges.skyBrightMax = 0.7f;
        profile.ranges.fogDensityMin = 0.05f; profile.ranges.fogDensityMax = 0.12f;
        profile.ranges.fogDistanceMin = 15.0f; profile.ranges.fogDistanceMax = 35.0f;
        profile.ranges.waterHueMin = 60.0f; profile.ranges.waterHueMax = 100.0f;
        profile.ranges.waterClarityMin = 0.2f; profile.ranges.waterClarityMax = 0.5f;
        profile.ranges.sunHueMin = 50.0f; profile.ranges.sunHueMax = 70.0f;
        profile.ranges.sunIntensityMin = 0.6f; profile.ranges.sunIntensityMax = 0.85f;
        profile.ranges.biomeSaturationMin = 0.8f; profile.ranges.biomeSaturationMax = 1.2f;
        profile.ranges.warmthMin = 0.0f; profile.ranges.warmthMax = 0.3f;

        profile.biomeWeights.wetlandWeight = 2.0f;
        profile.biomeWeights.forestWeight = 0.5f;
        profile.biomeWeights.desertWeight = 0.3f;

        m_profiles.push_back(profile);
    }

    // ===== LEGENDARY THEMES (5% total) =====

    // Bioluminescent
    {
        ThemeProfile profile;
        profile.name = "Luminous Abyss";
        profile.basePreset = PlanetPreset::BIOLUMINESCENT;
        profile.rarity = ThemeRarity::LEGENDARY;
        profile.weight = 1.5f;

        profile.ranges.skyHueMin = 220.0f; profile.ranges.skyHueMax = 280.0f;
        profile.ranges.skySatMin = 0.3f; profile.ranges.skySatMax = 0.6f;
        profile.ranges.skyBrightMin = 0.15f; profile.ranges.skyBrightMax = 0.35f;
        profile.ranges.fogDensityMin = 0.01f; profile.ranges.fogDensityMax = 0.03f;
        profile.ranges.fogDistanceMin = 50.0f; profile.ranges.fogDistanceMax = 100.0f;
        profile.ranges.waterHueMin = 180.0f; profile.ranges.waterHueMax = 220.0f;
        profile.ranges.waterClarityMin = 0.5f; profile.ranges.waterClarityMax = 0.9f;
        profile.ranges.sunHueMin = 200.0f; profile.ranges.sunHueMax = 240.0f;
        profile.ranges.sunIntensityMin = 0.2f; profile.ranges.sunIntensityMax = 0.4f;
        profile.ranges.biomeSaturationMin = 1.2f; profile.ranges.biomeSaturationMax = 1.6f;
        profile.ranges.warmthMin = -0.3f; profile.ranges.warmthMax = 0.0f;

        m_profiles.push_back(profile);
    }

    // Crystal World
    {
        ThemeProfile profile;
        profile.name = "Prismatic Spires";
        profile.basePreset = PlanetPreset::CRYSTAL_WORLD;
        profile.rarity = ThemeRarity::LEGENDARY;
        profile.weight = 1.0f;

        profile.ranges.skyHueMin = 280.0f; profile.ranges.skyHueMax = 340.0f;
        profile.ranges.skySatMin = 0.25f; profile.ranges.skySatMax = 0.5f;
        profile.ranges.skyBrightMin = 0.6f; profile.ranges.skyBrightMax = 0.9f;
        profile.ranges.fogDensityMin = 0.01f; profile.ranges.fogDensityMax = 0.025f;
        profile.ranges.fogDistanceMin = 60.0f; profile.ranges.fogDistanceMax = 120.0f;
        profile.ranges.waterHueMin = 260.0f; profile.ranges.waterHueMax = 320.0f;
        profile.ranges.waterClarityMin = 0.8f; profile.ranges.waterClarityMax = 1.0f;
        profile.ranges.sunHueMin = 300.0f; profile.ranges.sunHueMax = 340.0f;
        profile.ranges.sunIntensityMin = 0.85f; profile.ranges.sunIntensityMax = 1.15f;
        profile.ranges.biomeSaturationMin = 0.9f; profile.ranges.biomeSaturationMax = 1.3f;
        profile.ranges.warmthMin = -0.2f; profile.ranges.warmthMax = 0.1f;

        profile.biomeWeights.mountainWeight = 1.8f;
        profile.biomeWeights.tundraWeight = 1.3f;

        m_profiles.push_back(profile);
    }
}

const ThemeProfile& ThemeProfileRegistry::selectTheme(uint32_t seed) const {
    if (m_profiles.empty()) {
        static ThemeProfile defaultProfile;
        defaultProfile.name = "Default";
        defaultProfile.basePreset = PlanetPreset::EARTH_LIKE;
        return defaultProfile;
    }

    // Use seed to determine rarity tier
    float rarityRoll = PlanetSeed::seedToFloat(seed);
    ThemeRarity targetRarity;

    if (rarityRoll < COMMON_THRESHOLD) {
        targetRarity = ThemeRarity::COMMON;
    } else if (rarityRoll < UNCOMMON_THRESHOLD) {
        targetRarity = ThemeRarity::UNCOMMON;
    } else if (rarityRoll < RARE_THRESHOLD) {
        targetRarity = ThemeRarity::RARE;
    } else {
        targetRarity = ThemeRarity::LEGENDARY;
    }

    // Get profiles matching rarity
    std::vector<const ThemeProfile*> candidates = getProfilesByRarity(targetRarity);

    // Fallback to common if no candidates
    if (candidates.empty()) {
        candidates = getProfilesByRarity(ThemeRarity::COMMON);
    }

    if (candidates.empty()) {
        return m_profiles[0];
    }

    // Weight-based selection within tier
    float totalWeight = 0.0f;
    for (const auto* p : candidates) {
        totalWeight += p->weight;
    }

    uint32_t selectionSeed = PlanetSeed::getSubSeed(seed, 1);
    float selection = PlanetSeed::seedToFloat(selectionSeed) * totalWeight;

    float accumulated = 0.0f;
    for (const auto* p : candidates) {
        accumulated += p->weight;
        if (selection <= accumulated) {
            return *p;
        }
    }

    return *candidates.back();
}

const ThemeProfile* ThemeProfileRegistry::getProfile(const std::string& name) const {
    for (const auto& p : m_profiles) {
        if (p.name == name) {
            return &p;
        }
    }
    return nullptr;
}

std::vector<const ThemeProfile*> ThemeProfileRegistry::getProfilesByRarity(ThemeRarity rarity) const {
    std::vector<const ThemeProfile*> result;
    for (const auto& p : m_profiles) {
        if (p.rarity == rarity) {
            result.push_back(&p);
        }
    }
    return result;
}

// ============================================================================
// CONTRAST BUDGET IMPLEMENTATION
// ============================================================================

float ContrastBudget::colorDistance(const glm::vec3& a, const glm::vec3& b) {
    // Use weighted RGB distance (perceptual approximation)
    glm::vec3 diff = a - b;
    float rmean = (a.r + b.r) * 0.5f;

    float dr2 = diff.r * diff.r;
    float dg2 = diff.g * diff.g;
    float db2 = diff.b * diff.b;

    // Weighted RGB distance formula
    float weight_r = 2.0f + rmean;
    float weight_g = 4.0f;
    float weight_b = 3.0f - rmean;

    return std::sqrt(weight_r * dr2 + weight_g * dg2 + weight_b * db2) / 3.0f;
}

glm::vec3 ContrastBudget::pushAway(const glm::vec3& color, const glm::vec3& reference, float minDistance) {
    float dist = colorDistance(color, reference);
    if (dist >= minDistance) {
        return color;
    }

    glm::vec3 direction = color - reference;
    float len = glm::length(direction);
    if (len < 0.001f) {
        // Colors are nearly identical, push in a default direction
        direction = glm::vec3(0.1f, -0.1f, 0.05f);
        len = glm::length(direction);
    }

    direction = direction / len;
    float pushAmount = minDistance - dist;

    glm::vec3 result = color + direction * pushAmount;
    return glm::clamp(result, glm::vec3(0.0f), glm::vec3(1.0f));
}

bool ContrastBudget::validatePalette(const TerrainPalette& palette, const AtmosphereSettings& atm) {
    ContrastBudget budget;

    // Check sky vs ground contrast
    glm::vec3 avgGround = (palette.grassColor + palette.dirtColor + palette.rockColor) / 3.0f;
    glm::vec3 avgSky = (atm.skyZenithColor + atm.skyHorizonColor) / 2.0f;
    if (colorDistance(avgSky, avgGround) < budget.minSkyGroundContrast) {
        return false;
    }

    // Check water vs land contrast
    if (colorDistance(palette.shallowWaterColor, palette.sandColor) < budget.minWaterLandContrast) {
        return false;
    }

    // Check biome color distances
    if (colorDistance(palette.grassColor, palette.forestColor) < budget.minBiomeColorDistance) {
        return false;
    }
    if (colorDistance(palette.grassColor, palette.rockColor) < budget.minBiomeColorDistance) {
        return false;
    }

    return true;
}

void ContrastBudget::enforceContrast(TerrainPalette& palette, AtmosphereSettings& atm, float strength) {
    ContrastBudget budget;

    // Enforce sky vs ground contrast
    glm::vec3 avgGround = (palette.grassColor + palette.dirtColor + palette.rockColor) / 3.0f;
    glm::vec3 avgSky = (atm.skyZenithColor + atm.skyHorizonColor) / 2.0f;
    float skyGroundDist = colorDistance(avgSky, avgGround);

    if (skyGroundDist < budget.minSkyGroundContrast) {
        float pushFactor = (budget.minSkyGroundContrast - skyGroundDist) * strength;
        // Push sky brighter/different
        atm.skyZenithColor = pushAway(atm.skyZenithColor, avgGround, budget.minSkyGroundContrast);
        atm.skyHorizonColor = pushAway(atm.skyHorizonColor, avgGround, budget.minSkyGroundContrast * 0.8f);
    }

    // Enforce water vs land contrast
    palette.shallowWaterColor = pushAway(palette.shallowWaterColor, palette.sandColor, budget.minWaterLandContrast);
    palette.deepWaterColor = pushAway(palette.deepWaterColor, palette.sandColor, budget.minWaterLandContrast * 1.2f);

    // Enforce biome color distances
    palette.forestColor = pushAway(palette.forestColor, palette.grassColor, budget.minBiomeColorDistance);
    palette.rockColor = pushAway(palette.rockColor, palette.grassColor, budget.minBiomeColorDistance);
    palette.sandColor = pushAway(palette.sandColor, palette.grassColor, budget.minBiomeColorDistance);
}

// ============================================================================
// SEED VARIATION IMPLEMENTATIONS
// ============================================================================

namespace SeedVariation {

TerrainVariation TerrainVariation::fromSeed(uint32_t seed) {
    TerrainVariation v;

    v.noiseFrequency = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 0), 0.5f, 2.0f);
    v.noiseOctaves = PlanetSeed::seedToInt(PlanetSeed::getSubSeed(seed, 1), 4, 8);
    v.ridgeBias = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 2), 0.0f, 1.0f);
    v.valleyBias = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 3), 0.2f, 0.8f);
    v.plateauChance = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 4), 0.0f, 0.4f);
    v.erosionStrength = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 5), 0.3f, 1.0f);
    v.thermalStrength = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 6), 0.2f, 0.8f);

    return v;
}

OceanVariation OceanVariation::fromSeed(uint32_t seed) {
    OceanVariation v;

    v.coverage = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 0), 0.3f, 0.7f);
    v.shorelineComplexity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 1), 0.2f, 0.8f);
    v.coralReefDensity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 2), 0.0f, 0.5f);
    v.depthVariation = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 3), 0.3f, 1.0f);
    v.currentStrength = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 4), 0.2f, 0.8f);

    return v;
}

ArchipelagoVariation ArchipelagoVariation::fromSeed(uint32_t seed) {
    ArchipelagoVariation v;

    v.islandCount = PlanetSeed::seedToInt(PlanetSeed::getSubSeed(seed, 0), 3, 12);
    v.sizeDispersion = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 1), 0.3f, 0.9f);
    v.coastIrregularity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 2), 0.2f, 0.7f);
    v.lagoonProbability = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 3), 0.0f, 0.4f);
    v.volcanoChance = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 4), 0.0f, 0.3f);
    v.connectionDensity = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 5), 0.2f, 0.8f);

    return v;
}

ClimateVariation ClimateVariation::fromSeed(uint32_t seed) {
    ClimateVariation v;

    v.temperatureBase = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 0), -10.0f, 10.0f);
    v.temperatureRange = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 1), 20.0f, 50.0f);
    v.moistureBase = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 2), 0.3f, 0.7f);
    v.moistureRange = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 3), 0.3f, 0.7f);
    v.latitudinalStrength = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 4), 0.5f, 1.0f);
    v.altitudeStrength = PlanetSeed::seedToRange(PlanetSeed::getSubSeed(seed, 5), 0.5f, 1.0f);

    return v;
}

} // namespace SeedVariation
