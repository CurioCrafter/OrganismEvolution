#pragma once

#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <glm/glm.hpp>

// ============================================================================
// PLANET SEED SYSTEM
// ============================================================================
// Provides deterministic sub-seed derivation for all procedural generation
// systems, ensuring run-to-run uniqueness while maintaining reproducibility.
//
// Design: Uses splitmix64 for high-quality sub-seed derivation.

namespace PlanetSeedConstants {
    // Sub-seed indices for deterministic derivation
    constexpr uint32_t PALETTE_SEED_OFFSET     = 0x1A2B3C4D;
    constexpr uint32_t CLIMATE_SEED_OFFSET     = 0x5E6F7A8B;
    constexpr uint32_t TERRAIN_SEED_OFFSET     = 0x9C0D1E2F;
    constexpr uint32_t LIFE_SEED_OFFSET        = 0x3A4B5C6D;
    constexpr uint32_t OCEAN_SEED_OFFSET       = 0x7E8F9A0B;
    constexpr uint32_t BIOME_SEED_OFFSET       = 0xC1D2E3F4;
    constexpr uint32_t VEGETATION_SEED_OFFSET  = 0x5A6B7C8D;
    constexpr uint32_t CREATURE_SEED_OFFSET    = 0x9E0F1A2B;
    constexpr uint32_t WEATHER_SEED_OFFSET     = 0x3C4D5E6F;
    constexpr uint32_t ARCHIPELAGO_SEED_OFFSET = 0x7A8B9C0D;
    constexpr uint32_t CHEMISTRY_SEED_OFFSET   = 0xB1C2D3E4;  // Planet chemistry and biochemistry
}

// Sub-seed container with stable derivation
struct PlanetSeed {
    // Master seed (base for all derivations)
    uint32_t masterSeed = 0;

    // Derived sub-seeds (computed from masterSeed)
    uint32_t paletteSeed;      // Theme colors, atmosphere, color grading
    uint32_t climateSeed;      // Temperature/humidity gradients, seasons
    uint32_t terrainSeed;      // Noise layers, elevation, mountains
    uint32_t lifeSeed;         // Creature spawning, behavior patterns
    uint32_t oceanSeed;        // Ocean coverage, currents, shoreline
    uint32_t biomeSeed;        // Biome distribution and mixing
    uint32_t vegetationSeed;   // Plant placement, tree variation
    uint32_t creatureSeed;     // Creature genetics, evolution parameters
    uint32_t weatherSeed;      // Weather patterns, events
    uint32_t archipelagoSeed;  // Island count, placement, connections
    uint32_t chemistrySeed;    // Planet chemistry and biochemistry constraints

    // Fingerprint for logging/display
    std::string fingerprint;   // e.g., "A7X-Q3M" (short memorable code)

    // Default constructor
    PlanetSeed() : masterSeed(0) {
        deriveAllSeeds();
    }

    // Construct from master seed
    explicit PlanetSeed(uint32_t seed) : masterSeed(seed) {
        deriveAllSeeds();
    }

    // Derive all sub-seeds from master
    void deriveAllSeeds() {
        paletteSeed     = deriveSubSeed(PlanetSeedConstants::PALETTE_SEED_OFFSET);
        climateSeed     = deriveSubSeed(PlanetSeedConstants::CLIMATE_SEED_OFFSET);
        terrainSeed     = deriveSubSeed(PlanetSeedConstants::TERRAIN_SEED_OFFSET);
        lifeSeed        = deriveSubSeed(PlanetSeedConstants::LIFE_SEED_OFFSET);
        oceanSeed       = deriveSubSeed(PlanetSeedConstants::OCEAN_SEED_OFFSET);
        biomeSeed       = deriveSubSeed(PlanetSeedConstants::BIOME_SEED_OFFSET);
        vegetationSeed  = deriveSubSeed(PlanetSeedConstants::VEGETATION_SEED_OFFSET);
        creatureSeed    = deriveSubSeed(PlanetSeedConstants::CREATURE_SEED_OFFSET);
        weatherSeed     = deriveSubSeed(PlanetSeedConstants::WEATHER_SEED_OFFSET);
        archipelagoSeed = deriveSubSeed(PlanetSeedConstants::ARCHIPELAGO_SEED_OFFSET);
        chemistrySeed   = deriveSubSeed(PlanetSeedConstants::CHEMISTRY_SEED_OFFSET);
        fingerprint     = generateFingerprint();
    }

    // Set master seed and re-derive all
    void setMasterSeed(uint32_t seed) {
        masterSeed = seed;
        deriveAllSeeds();
    }

    // Get a further sub-seed for a specific subsystem
    // e.g., getSubSeed(terrainSeed, 0) for first noise layer
    static uint32_t getSubSeed(uint32_t baseSeed, uint32_t index) {
        return splitmix64(static_cast<uint64_t>(baseSeed) + index);
    }

    // Get normalized float from seed (0.0 to 1.0)
    static float seedToFloat(uint32_t seed) {
        return static_cast<float>(seed) / static_cast<float>(UINT32_MAX);
    }

    // Get float in range [min, max] from seed
    static float seedToRange(uint32_t seed, float min, float max) {
        return min + seedToFloat(seed) * (max - min);
    }

    // Get int in range [min, max] from seed
    static int seedToInt(uint32_t seed, int min, int max) {
        return min + static_cast<int>(seed % static_cast<uint32_t>(max - min + 1));
    }

    // Comparison
    bool operator==(const PlanetSeed& other) const {
        return masterSeed == other.masterSeed;
    }

private:
    // Splitmix64 hash for high-quality sub-seed derivation
    static uint32_t splitmix64(uint64_t seed) {
        uint64_t z = seed + 0x9E3779B97F4A7C15ULL;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return static_cast<uint32_t>((z ^ (z >> 31)) & 0xFFFFFFFF);
    }

    // Derive a sub-seed using offset
    uint32_t deriveSubSeed(uint32_t offset) const {
        return splitmix64(static_cast<uint64_t>(masterSeed) + offset);
    }

    // Generate human-readable fingerprint (6 chars)
    std::string generateFingerprint() const {
        static const char* chars = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
        static const int charCount = 32;

        std::string fp;
        fp.reserve(7);

        uint32_t hash = splitmix64(masterSeed);
        for (int i = 0; i < 3; ++i) {
            fp += chars[hash % charCount];
            hash /= charCount;
        }
        fp += '-';
        hash = splitmix64(masterSeed + 12345);
        for (int i = 0; i < 3; ++i) {
            fp += chars[hash % charCount];
            hash /= charCount;
        }

        return fp;
    }
};

// ============================================================================
// THEME PROFILE - Weighted selection for planet variety
// ============================================================================

enum class ThemeRarity {
    COMMON,      // 40% chance total
    UNCOMMON,    // 35% chance total
    RARE,        // 20% chance total
    LEGENDARY    // 5% chance total
};

// Forward declaration
enum class PlanetPreset;

struct ThemeProfile {
    std::string name;
    PlanetPreset basePreset;
    ThemeRarity rarity;
    float weight;  // Selection weight within rarity tier

    // Parameter ranges for variation
    struct ParameterRanges {
        // Sky
        float skyHueMin, skyHueMax;
        float skySatMin, skySatMax;
        float skyBrightMin, skyBrightMax;

        // Fog
        float fogDensityMin, fogDensityMax;
        float fogDistanceMin, fogDistanceMax;

        // Water
        float waterHueMin, waterHueMax;
        float waterClarityMin, waterClarityMax;

        // Sun
        float sunHueMin, sunHueMax;
        float sunIntensityMin, sunIntensityMax;

        // Biome saturation bias
        float biomeSaturationMin, biomeSaturationMax;

        // Overall palette warmth (-1 cool, +1 warm)
        float warmthMin, warmthMax;
    } ranges;

    // Biome weight modifiers (multiply default weights)
    struct BiomeWeights {
        float desertWeight = 1.0f;
        float forestWeight = 1.0f;
        float tundraWeight = 1.0f;
        float tropicalWeight = 1.0f;
        float wetlandWeight = 1.0f;
        float mountainWeight = 1.0f;
        float volcanicWeight = 1.0f;
    } biomeWeights;
};

// Theme profile registry
class ThemeProfileRegistry {
public:
    static const ThemeProfileRegistry& getInstance();

    // Select theme based on seed
    const ThemeProfile& selectTheme(uint32_t seed) const;

    // Get specific profile
    const ThemeProfile* getProfile(const std::string& name) const;

    // Get all profiles
    const std::vector<ThemeProfile>& getAllProfiles() const { return m_profiles; }

    // Get profiles by rarity
    std::vector<const ThemeProfile*> getProfilesByRarity(ThemeRarity rarity) const;

private:
    ThemeProfileRegistry();
    void initializeProfiles();

    std::vector<ThemeProfile> m_profiles;

    // Rarity thresholds (cumulative)
    static constexpr float COMMON_THRESHOLD = 0.40f;
    static constexpr float UNCOMMON_THRESHOLD = 0.75f;
    static constexpr float RARE_THRESHOLD = 0.95f;
    // Above 0.95 = LEGENDARY
};

// ============================================================================
// CONTRAST BUDGET SYSTEM
// ============================================================================

struct ContrastBudget {
    // Minimum color distance requirements
    float minBiomeColorDistance = 0.15f;   // Minimum RGB distance between biomes
    float minSkyGroundContrast = 0.20f;    // Sky vs average ground color
    float minWaterLandContrast = 0.25f;    // Water vs beach/coast
    float minVegetationContrast = 0.10f;   // Different vegetation types

    // Validate a palette meets contrast requirements
    static bool validatePalette(const struct TerrainPalette& palette,
                                 const struct AtmosphereSettings& atm);

    // Auto-adjust palette to meet contrast requirements
    static void enforceContrast(struct TerrainPalette& palette,
                                 struct AtmosphereSettings& atm,
                                 float strength = 1.0f);

private:
    // Color distance (perceptual, not just RGB)
    static float colorDistance(const struct glm::vec3& a, const struct glm::vec3& b);

    // Adjust color to increase distance from reference
    static glm::vec3 pushAway(const glm::vec3& color, const glm::vec3& reference, float minDistance);
};

// ============================================================================
// SEED-DRIVEN PARAMETER VARIATION
// ============================================================================

namespace SeedVariation {
    // Get terrain variation parameters from seed
    struct TerrainVariation {
        float noiseFrequency;      // Base noise frequency
        int noiseOctaves;          // Number of noise octaves
        float ridgeBias;           // Ridge vs smooth terrain
        float valleyBias;          // Valley depth tendency
        float plateauChance;       // Probability of flat areas
        float erosionStrength;     // Hydraulic erosion intensity
        float thermalStrength;     // Thermal erosion intensity

        static TerrainVariation fromSeed(uint32_t seed);
    };

    // Get ocean variation parameters from seed
    struct OceanVariation {
        float coverage;            // 0.0-1.0, amount of ocean
        float shorelineComplexity; // Coastal irregularity
        float coralReefDensity;    // Amount of coral features
        float depthVariation;      // How varied ocean floor is
        float currentStrength;     // Ocean current intensity

        static OceanVariation fromSeed(uint32_t seed);
    };

    // Get archipelago variation parameters from seed
    struct ArchipelagoVariation {
        int islandCount;           // Number of islands
        float sizeDispersion;      // Size variation between islands
        float coastIrregularity;   // Coastline jaggedness
        float lagoonProbability;   // Chance of internal lagoons
        float volcanoChance;       // Chance of volcanic islands
        float connectionDensity;   // How connected islands are

        static ArchipelagoVariation fromSeed(uint32_t seed);
    };

    // Get climate variation parameters from seed
    struct ClimateVariation {
        float temperatureBase;     // Global temperature offset
        float temperatureRange;    // Hot-cold variation
        float moistureBase;        // Global moisture offset
        float moistureRange;       // Wet-dry variation
        float latitudinalStrength; // How much latitude affects climate
        float altitudeStrength;    // How much altitude affects climate

        static ClimateVariation fromSeed(uint32_t seed);
    };
}
