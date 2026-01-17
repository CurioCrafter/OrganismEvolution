#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <array>
#include <string>
#include "BiomeSystem.h"

// ============================================================================
// UNIFIED BIOME COLOR PALETTE SYSTEM
// ============================================================================
// This system provides cohesive color definitions for all vegetation and
// terrain within each biome, ensuring visual consistency.

// Plant types that can exist in the world
enum class PlantCategory : uint8_t {
    GRASS,
    FLOWER,
    BUSH,
    FERN,
    CACTUS,
    MUSHROOM,
    REED,
    MOSS,
    LICHEN,
    VINE,
    SUCCULENT,
    AQUATIC_PLANT,

    COUNT
};

// Detailed flower color within a patch
struct FlowerPatchColor {
    glm::vec3 petalColor;
    glm::vec3 centerColor;
    glm::vec3 stemColor;
    float glowIntensity = 0.0f;  // For bioluminescent flowers
};

// Unified color palette for a single biome
struct BiomePalette {
    BiomeType biomeType;
    std::string name;

    // ===== TERRAIN COLORS =====
    glm::vec3 groundColor;           // Base dirt/soil color
    glm::vec3 groundAccentColor;     // Secondary ground variation
    glm::vec3 rockColor;             // Exposed rock
    glm::vec3 sandColor;             // Sandy areas
    glm::vec3 mudColor;              // Wet areas

    // ===== GRASS COLORS =====
    glm::vec3 grassBaseColor;        // Base of grass blades
    glm::vec3 grassTipColor;         // Tips of grass blades
    glm::vec3 grassDryColor;         // Dried/autumn grass
    float grassColorVariation;       // How much grass color varies (0-1)

    // ===== TREE COLORS =====
    glm::vec3 treeBarkColor;         // Tree trunk color
    glm::vec3 treeBarkAccent;        // Bark detail color
    glm::vec3 leafColorSpring;       // Spring leaves
    glm::vec3 leafColorSummer;       // Summer leaves
    glm::vec3 leafColorAutumn;       // Autumn leaves
    glm::vec3 leafColorWinter;       // Winter (dead/evergreen)
    float leafColorVariation;        // Tree-to-tree variation

    // ===== BUSH COLORS =====
    glm::vec3 bushLeafColor;
    glm::vec3 bushBerryColor;        // If has berries

    // ===== FLOWER COLORS (multiple options per biome) =====
    std::array<FlowerPatchColor, 6> flowerPalette;
    int numFlowerColors;             // How many flower colors are valid

    // ===== FERN COLORS =====
    glm::vec3 fernColor;
    glm::vec3 fernUndersideColor;

    // ===== MUSHROOM COLORS =====
    glm::vec3 mushroomCapColor;
    glm::vec3 mushroomStemColor;
    glm::vec3 mushroomGillColor;
    bool mushroomGlows;
    glm::vec3 mushroomGlowColor;

    // ===== SPECIAL PLANT COLORS =====
    glm::vec3 cactusColor;           // Desert biomes
    glm::vec3 reedColor;             // Wetland biomes
    glm::vec3 mossColor;             // Forest floor
    glm::vec3 lichenColor;           // Rock surfaces
    glm::vec3 vineColor;             // Climbing vines

    // ===== ENVIRONMENTAL TINTS =====
    glm::vec3 ambientTint;           // Overall color cast
    float saturationMultiplier;      // 1.0 = normal, <1 = muted, >1 = vibrant
    float brightnessMultiplier;      // 1.0 = normal

    // ===== SEASONAL MODIFIERS =====
    // These multiply with base colors based on season (0-1 = winter-summer)
    glm::vec3 getSeasonalGrassColor(float season) const;
    glm::vec3 getSeasonalLeafColor(float season) const;
    FlowerPatchColor getRandomFlowerColor(uint32_t seed) const;
};

// ============================================================================
// PLANT DISTRIBUTION RULES
// ============================================================================

// Rules for how plants cluster and distribute
struct PlantDistributionRules {
    PlantCategory category;

    // Density
    float baseDensity;               // Plants per unit area
    float densityNearWater;          // Multiplier near water
    float densityInShade;            // Multiplier under trees

    // Clustering
    float clusterRadius;             // How large clusters are
    float clusterProbability;        // Chance to spawn a cluster vs single
    int minClusterSize;
    int maxClusterSize;

    // Elevation preferences
    float minElevation;              // 0-1 normalized
    float maxElevation;
    float optimalElevation;          // Where density is highest

    // Moisture preferences
    float minMoisture;
    float maxMoisture;
    float optimalMoisture;

    // Temperature preferences
    float minTemperature;            // -1 to 1
    float maxTemperature;
    float optimalTemperature;

    // Spacing
    float minSpacing;                // Minimum distance between plants
    float preferredSpacing;          // Ideal distance

    // Biome compatibility (0 = incompatible, 1 = ideal)
    float getBiomeCompatibility(BiomeType biome) const;
};

// ============================================================================
// PLANT NUTRITION FOR CREATURES
// ============================================================================

// Nutritional value and properties of each plant type
struct PlantNutrition {
    PlantCategory category;

    // Basic nutrition
    float energyValue;               // Calories/energy provided
    float hydrationValue;            // Water content
    float proteinValue;
    float fiberValue;

    // Special properties
    float toxicity;                  // 0 = safe, 1 = deadly
    float digestibility;             // 0-1, how easy to digest
    float satiation;                 // How filling it is

    // Effects
    bool isHallucinogenic;
    bool isMedicinal;
    bool isStimulant;
    bool isSedative;

    // Preference weights for different creature types
    float herbivorePreference;       // Base preference for grazers
    float omnivorePreference;        // Base preference for omnivores

    // Which genome traits affect preference
    // Higher value = this plant is more preferred by creatures with that trait
    float preferredBySmall;          // Small creatures prefer this
    float preferredByLarge;          // Large creatures prefer this
    float requiresSpecialization;    // Requires specific adaptation to eat
};

// ============================================================================
// BIOME PALETTE MANAGER
// ============================================================================

class BiomePaletteManager {
public:
    BiomePaletteManager();
    ~BiomePaletteManager() = default;

    // Initialize all biome palettes
    void initialize();

    // Get palette for a specific biome
    const BiomePalette& getPalette(BiomeType biome) const;
    BiomePalette& getMutablePalette(BiomeType biome);

    // Get blended palette for biome transitions
    BiomePalette getBlendedPalette(BiomeType primary, BiomeType secondary, float blendFactor) const;

    // Get plant distribution rules
    const PlantDistributionRules& getDistributionRules(PlantCategory category) const;

    // Get plant nutrition info
    const PlantNutrition& getNutrition(PlantCategory category) const;

    // Sample color with variation
    glm::vec3 sampleGrassColor(BiomeType biome, float x, float z, float season) const;
    glm::vec3 sampleLeafColor(BiomeType biome, float x, float z, float season) const;
    FlowerPatchColor sampleFlowerColor(BiomeType biome, float x, float z) const;

    // Check if plant can exist at location
    float getPlantSuitability(PlantCategory category, BiomeType biome,
                              float elevation, float moisture, float temperature) const;

    // Get creature food preference
    float getCreatureFoodPreference(PlantCategory plant, float creatureSize,
                                    float specialization, bool isHerbivore) const;

    // Apply theme modifications (for alien planets)
    void applyHueShift(float hueOffset);
    void applySaturationMultiplier(float mult);
    void applyBrightnessMultiplier(float mult);

private:
    void initializePalettes();
    void initializeDistributionRules();
    void initializeNutrition();

    // Create specific biome palettes
    void createGrasslandPalette();
    void createForestPalette();
    void createDesertPalette();
    void createTundraPalette();
    void createTropicalPalette();
    void createWetlandPalette();
    void createSavannaPalette();
    void createBorealPalette();
    void createAlpinePalette();
    void createVolcanicPalette();
    void createCoastalPalette();

    // Noise functions for variation
    float noise2D(float x, float z) const;
    float fractalNoise(float x, float z, int octaves) const;

    std::array<BiomePalette, static_cast<size_t>(BiomeType::BIOME_COUNT)> m_palettes;
    std::array<PlantDistributionRules, static_cast<size_t>(PlantCategory::COUNT)> m_distributionRules;
    std::array<PlantNutrition, static_cast<size_t>(PlantCategory::COUNT)> m_nutrition;

    BiomePalette m_defaultPalette;
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Get default palette for a biome type
BiomePalette getDefaultBiomePalette(BiomeType biome);

// Blend two palettes
BiomePalette blendPalettes(const BiomePalette& a, const BiomePalette& b, float t);

// Get seasonal modifier (0 = winter, 0.5 = summer, 1 = winter again)
float getSeasonalModifier(float dayOfYear);

// Convert biome to recommended flower colors
std::vector<FlowerPatchColor> getBiomeFlowerColors(BiomeType biome);

// Get plant category name
const char* plantCategoryToString(PlantCategory category);

// ============================================================================
// EXPANDED PALETTE RAMP SYSTEM - Run-to-Run Variety
// ============================================================================
// Provides 3-5 distinct palette variations per biome for visual diversity
// while maintaining biome identity and coherence.

// Palette ramp for a single color element
struct ColorRamp {
    glm::vec3 cool;      // Cool variant (blue-shifted)
    glm::vec3 neutral;   // Base/neutral color
    glm::vec3 warm;      // Warm variant (red-shifted)
    glm::vec3 vibrant;   // High saturation variant
    glm::vec3 muted;     // Low saturation variant

    // Interpolate along the ramp based on warmth (-1 to 1) and saturation (0 to 1)
    glm::vec3 sample(float warmth, float saturation) const;
};

// Complete palette ramp set for a biome
struct BiomePaletteRamps {
    BiomeType biomeType;

    // Grass color ramps
    ColorRamp grassBase;
    ColorRamp grassTip;
    ColorRamp grassDry;

    // Leaf color ramps (for deciduous trees)
    ColorRamp leafSpring;
    ColorRamp leafSummer;
    ColorRamp leafAutumn;

    // Ground cover ramps
    ColorRamp groundBase;
    ColorRamp rockBase;
    ColorRamp sandBase;

    // Water-adjacent biomes
    ColorRamp wetlandAccent;

    // Constraints for ensuring biome identity
    struct Constraints {
        float minHueDelta = 20.0f;      // Min hue distance from adjacent biomes
        float minSatDelta = 0.15f;      // Min saturation difference
        float maxWarmthShift = 0.3f;    // Max warmth deviation
        float minValueRange = 0.1f;     // Min brightness variation
    } constraints;

    // Generate a complete palette from ramps using seed-derived parameters
    BiomePalette generatePalette(float warmth, float saturation, uint32_t seed) const;
};

// Registry of all biome palette ramps
class PaletteRampRegistry {
public:
    static PaletteRampRegistry& getInstance();

    // Get ramps for a specific biome
    const BiomePaletteRamps& getRamps(BiomeType biome) const;

    // Generate a varied palette for a biome using seed
    BiomePalette generateVariedPalette(BiomeType biome, uint32_t seed) const;

    // Check if palette meets contrast requirements with neighbors
    bool validateContrast(const BiomePalette& palette,
                          const std::vector<BiomeType>& neighbors) const;

    // Get vegetation density modifier from ramps
    float getVegetationDensityModifier(BiomeType biome, uint32_t seed) const;

private:
    PaletteRampRegistry();
    void initializeRamps();

    // Create ramps for each biome type
    void createGrasslandRamps();
    void createForestRamps();
    void createDesertRamps();
    void createTundraRamps();
    void createTropicalRamps();
    void createWetlandRamps();
    void createSavannaRamps();
    void createBorealRamps();
    void createAlpineRamps();
    void createVolcanicRamps();
    void createCoastalRamps();

    std::array<BiomePaletteRamps, static_cast<size_t>(BiomeType::BIOME_COUNT)> m_ramps;
    BiomePaletteRamps m_defaultRamps;
};
