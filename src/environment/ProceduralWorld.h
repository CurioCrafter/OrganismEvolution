#pragma once

#include "IslandGenerator.h"
#include "BiomeSystem.h"
#include "PlanetTheme.h"
#include "PlanetSeed.h"
#include <memory>
#include <random>

// Forward declarations
class Terrain;
class VegetationManager;

// ============================================================================
// STAR TYPE SYSTEM - Run-to-Run Variety
// ============================================================================
// Defines the parent star's characteristics which affect planet colors,
// temperature, and day/night cycle.

enum class StarSpectralClass {
    O_BLUE_GIANT,       // Very hot, blue-white (rare)
    B_BLUE,             // Hot, blue (rare)
    A_WHITE,            // White (uncommon)
    F_YELLOW_WHITE,     // Yellow-white (common)
    G_YELLOW,           // Sun-like yellow (common) - default
    K_ORANGE,           // Orange (common)
    M_RED_DWARF,        // Cool, red (very common)

    // Special types
    BINARY_SYSTEM,      // Two suns
    RED_GIANT,          // Old, expanded star
    WHITE_DWARF         // Dead star remnant
};

struct StarType {
    StarSpectralClass spectralClass = StarSpectralClass::G_YELLOW;

    // Visual properties
    glm::vec3 color = glm::vec3(1.0f, 0.95f, 0.85f);      // Star color
    float intensity = 1.0f;                                 // Brightness multiplier
    float temperature = 5778.0f;                            // Kelvin (for reference)
    float angularSize = 1.0f;                               // Visual size multiplier

    // Effects on planet
    float dayLengthModifier = 1.0f;                         // Day cycle speed
    float temperatureOffset = 0.0f;                         // Global temperature change
    float uvIntensity = 1.0f;                               // Affects creature evolution

    // Atmosphere effects
    glm::vec3 skyTintModifier = glm::vec3(1.0f);           // How star color affects sky
    float twilightDuration = 1.0f;                          // How long dawn/dusk lasts

    // Generate from seed
    static StarType fromSeed(uint32_t seed);

    // Get preset star types
    static StarType sunLike();
    static StarType redDwarf();
    static StarType blueGiant();
    static StarType binarySystem();
};

// ============================================================================
// REGION CONFIG - Multi-Island Competition System
// ============================================================================
// Allows distinct biome mixes and evolution biases per region/island,
// enabling competing species and unique ecosystems.

struct EvolutionBiasHook {
    // Creature trait biases for this region (multipliers, 1.0 = neutral)
    float sizeBias = 1.0f;              // Favors larger (>1) or smaller (<1) creatures
    float speedBias = 1.0f;             // Favors faster creatures
    float intelligenceBias = 1.0f;      // Favors smarter creatures
    float aggressionBias = 1.0f;        // Favors more aggressive creatures
    float socialBias = 1.0f;            // Favors social/pack behavior
    float aquaticBias = 1.0f;           // Favors water adaptation
    float flyingBias = 1.0f;            // Favors flight capability

    // Special adaptations
    float venomChance = 0.0f;           // Increased chance of venom evolution
    float camouflageChance = 0.0f;      // Increased chance of camouflage
    float bioluminescenceChance = 0.0f; // Increased chance of glow

    // Environmental pressure
    float predationPressure = 1.0f;     // How much predators affect selection
    float resourceScarcity = 1.0f;      // Competition for food
};

struct RegionConfig {
    int regionId = 0;
    std::string name;

    // Islands belonging to this region
    std::vector<int> islandIds;

    // Biome weight overrides (multiply default weights)
    float desertWeight = 1.0f;
    float forestWeight = 1.0f;
    float tundraWeight = 1.0f;
    float tropicalWeight = 1.0f;
    float wetlandWeight = 1.0f;
    float mountainWeight = 1.0f;
    float volcanicWeight = 1.0f;
    float coastalWeight = 1.0f;

    // Climate overrides
    float temperatureOffset = 0.0f;     // Degrees C offset from global
    float moistureMultiplier = 1.0f;    // Multiply global moisture

    // Vegetation density override
    float vegetationDensity = 1.0f;

    // Evolution biases for creatures in this region
    EvolutionBiasHook evolutionBias;

    // Inter-region interactions
    float isolationLevel = 0.5f;        // 0 = connected, 1 = isolated
    bool allowsMigration = true;        // Can creatures migrate to/from

    // Generate from seed for variety
    static RegionConfig fromSeed(uint32_t seed, int regionId);

    // Preset configurations
    static RegionConfig tropical();
    static RegionConfig arctic();
    static RegionConfig volcanic();
    static RegionConfig competitive();  // High predation pressure
};

// Multi-region world configuration
struct MultiRegionConfig {
    bool enabled = false;
    std::vector<RegionConfig> regions;

    // Global inter-region settings
    float globalMigrationRate = 0.1f;   // Base migration probability
    bool competitiveMode = false;       // Enables species competition tracking

    // Get region for a given island
    const RegionConfig* getRegionForIsland(int islandId) const;

    // Generate automatic region assignment from archipelago
    static MultiRegionConfig fromArchipelago(uint32_t seed, int islandCount);
};

// ============================================================================
// VEGETATION DENSITY PRESETS - Run-to-Run Variety
// ============================================================================

enum class VegetationPreset {
    DEFAULT,        // Standard density
    SPARSE,         // Minimal vegetation (desert-like)
    LUSH,           // Dense vegetation (jungle-like)
    ALIEN,          // Strange patterns with glowing plants
    DEAD,           // Post-apocalyptic, dying vegetation
    OVERGROWN       // Maximum density, abandoned world feel
};

struct VegetationDensityConfig {
    VegetationPreset preset = VegetationPreset::DEFAULT;

    // Multipliers (1.0 = default)
    float treeDensity = 1.0f;
    float grassDensity = 1.0f;
    float flowerDensity = 1.0f;
    float shrubDensity = 1.0f;
    float alienPlantDensity = 0.0f;     // Alien/bioluminescent plants

    // Variation per biome (can override)
    float biomeDensityVariation = 0.2f;  // Random variation per biome

    // Generate from seed
    static VegetationDensityConfig fromSeed(uint32_t seed);

    // Get preset configurations
    static VegetationDensityConfig sparse();
    static VegetationDensityConfig lush();
    static VegetationDensityConfig alien();
    static VegetationDensityConfig dead();
    static VegetationDensityConfig overgrown();
};

// World generation configuration
struct WorldGenConfig {
    // Island settings
    IslandShape islandShape = IslandShape::IRREGULAR;
    float islandSize = 0.5f;            // 0-1, relative size of land mass
    float coastComplexity = 0.4f;       // How jagged the coastline is
    bool generateRivers = true;
    bool generateLakes = true;
    bool generateCaves = true;

    // Planet theme
    PlanetPreset themePreset = PlanetPreset::EARTH_LIKE;
    bool randomizeTheme = false;        // If true, ignores themePreset
    bool useWeightedThemeSelection = true;  // Use rarity-weighted theme profiles

    // Star system (affects lighting, sky colors, temperature)
    StarType starType;
    bool randomizeStarType = true;      // Generate random star from seed

    // Multi-region configuration (for archipelago worlds)
    MultiRegionConfig multiRegion;

    // Vegetation density preset
    VegetationDensityConfig vegetationConfig;

    // Terrain erosion control
    int erosionPasses = 3;              // 0-10, hydraulic erosion iterations
    float erosionStrength = 0.5f;       // 0-1, erosion intensity
    int noiseOctaves = 6;               // 1-8, terrain detail layers
    float noiseFrequency = 1.0f;        // Base noise frequency

    // Resolution
    int heightmapResolution = 2048;     // 256, 512, 1024, 2048

    // Master seed (0 = random)
    uint32_t seed = 0;

    // Planet seed system (derived from master seed)
    PlanetSeed planetSeed;

    // Update planet seed from master seed
    void updatePlanetSeed() {
        if (seed != 0) {
            planetSeed.setMasterSeed(seed);
        }
    }

    // Apply seed-based variation to config parameters
    void applyVariationFromSeed();
};

// Complete world generation result
struct GeneratedWorld {
    IslandData islandData;
    std::unique_ptr<BiomeSystem> biomeSystem;
    std::unique_ptr<PlanetTheme> planetTheme;

    // Planet seed used for this world
    PlanetSeed planetSeed;

    // Star type for this world
    StarType starType;

    // Multi-region configuration (if archipelago)
    MultiRegionConfig multiRegion;

    // Vegetation configuration used
    VegetationDensityConfig vegetationConfig;

    // Theme profile used (if weighted selection)
    std::string themeName;
    ThemeRarity themeRarity = ThemeRarity::COMMON;

    // Erosion and terrain parameters actually used
    int erosionPassesUsed;
    int noiseOctavesUsed;

    // World statistics
    float landPercentage;
    float waterPercentage;
    float averageElevation;
    int riverCount;
    int lakeCount;
    int caveCount;

    // Biome distribution (percentage of land)
    float desertCoverage;
    float forestCoverage;
    float tundraCoverage;
    float tropicalCoverage;
    float wetlandCoverage;
    float mountainCoverage;

    // Climate statistics
    float averageTemperature;
    float temperatureRange;
    float averageMoisture;

    // Generation timing
    float generationTimeMs;
};

// Main procedural world generation manager
// Combines IslandGenerator, BiomeSystem, and PlanetTheme into a cohesive system
class ProceduralWorld {
public:
    ProceduralWorld();
    ~ProceduralWorld();

    // Generate a complete world
    GeneratedWorld generate(const WorldGenConfig& config);

    // Generate with just a seed (uses random settings)
    GeneratedWorld generateRandom(uint32_t seed = 0);

    // Generate with a specific preset
    GeneratedWorld generatePreset(IslandShape shape, PlanetPreset theme, uint32_t seed = 0);

    // Quick generation methods
    GeneratedWorld generateEarthLikeIsland(uint32_t seed = 0);
    GeneratedWorld generateAlienWorld(uint32_t seed = 0);
    GeneratedWorld generateArchipelago(uint32_t seed = 0);
    GeneratedWorld generateVolcanicIsland(uint32_t seed = 0);

    // Get the raw heightmap for terrain rendering
    const std::vector<float>& getHeightmap() const;

    // Get biome map for terrain shader
    const std::vector<uint8_t>& getBiomeMapRGBA() const;

    // Apply to existing terrain system
    void applyToTerrain(Terrain* terrain);

    // Apply theme to renderers
    void applyThemeToRenderers();

    // Get current generation
    const GeneratedWorld* getCurrentWorld() const { return m_currentWorld.get(); }
    GeneratedWorld* getMutableCurrentWorld() { return m_currentWorld.get(); }

    // Regenerate with same config but new seed
    GeneratedWorld regenerate();

    // Accessors
    const IslandGenerator& getIslandGenerator() const { return m_islandGenerator; }
    uint32_t getLastSeed() const { return m_lastSeed; }
    const WorldGenConfig& getLastConfig() const { return m_lastConfig; }

    // Get planet seed fingerprint for display
    std::string getSeedFingerprint() const;

    // ===== Session Seed Override System =====
    // Returns the session seed (time-based if no explicit seed was provided)
    uint32_t getSessionSeed() const { return m_sessionSeed; }

    // Check if current world was generated with an auto-generated seed
    bool isAutoGeneratedSeed() const { return m_wasAutoGenerated; }

    // Get the full seed fingerprint including session info
    std::string getFullSeedInfo() const;

    // ===== Theme Variety Getters for Agent 9 (Shaders) =====
    // Get current palette variation values (0-1 normalized)
    struct PaletteVariation {
        float skyHue;           // Sky hue shift from base
        float skySaturation;    // Sky saturation multiplier
        float skyBrightness;    // Sky brightness multiplier
        float fogDensity;       // Fog density (0-1)
        float fogDistance;      // Fog start distance (normalized)
        float waterHue;         // Water hue shift
        float waterClarity;     // Water transparency (0-1)
        float sunHue;           // Sun color hue
        float sunIntensity;     // Sun brightness multiplier
        float biomeSaturation;  // Overall biome saturation
        float warmth;           // Color temperature (-1 cool, +1 warm)
        float vegetationDensity;// Overall vegetation density multiplier
    };
    PaletteVariation getCurrentPaletteVariation() const;

    // Get terrain variation parameters for shaders
    struct TerrainShaderParams {
        float erosionStrength;  // 0-1
        float ridginess;        // 0-1, how ridge-like terrain is
        float valleyDepth;      // 0-1
        float noiseFrequency;   // Base noise frequency
        int noiseOctaves;       // Noise complexity
    };
    TerrainShaderParams getTerrainShaderParams() const;

    // Get vegetation density configuration
    const VegetationDensityConfig& getVegetationConfig() const;

    // ===== Star Type Getters for Agent 9 (Shaders) =====
    // Get current star type parameters for sky/lighting shaders
    struct StarShaderParams {
        glm::vec3 starColor;            // RGB color of star
        float starIntensity;            // Brightness multiplier
        glm::vec3 skyTintModifier;      // How star affects sky color
        float dayLengthModifier;        // Speed of day/night cycle
        float twilightDuration;         // Dawn/dusk length
        float temperatureOffset;        // Global temperature change
        bool isBinarySystem;            // Two suns
    };
    StarShaderParams getStarShaderParams() const;

    // Get the current star type (for direct access)
    const StarType& getCurrentStarType() const;

    // ===== Region Config Getters for Agent 9 =====
    // Get region config for a specific island
    const RegionConfig* getRegionConfig(int islandId) const;

    // Get all regions
    const MultiRegionConfig& getMultiRegionConfig() const;

    // Check if position is in a specific region
    int getRegionAtPosition(const glm::vec3& worldPos) const;

    // Debug logging
    void logWorldGeneration(const GeneratedWorld& world) const;

private:
    // Apply seed-driven variation to theme
    void applyThemeVariation(PlanetTheme& theme, const PlanetSeed& seed, const ThemeProfile& profile);

    // Calculate biome distribution statistics
    void calculateBiomeDistribution(GeneratedWorld& world);

    // Apply climate variation from seed
    void applyClimateVariation(BiomeSystem& biomeSystem, const PlanetSeed& seed);

    // Cache palette variation for shader integration
    void cachePaletteVariation(const PlanetSeed& seed);

    // Cache terrain params for shader integration
    void cacheTerrainParams(const PlanetSeed& seed);

    // Determine vegetation preset from seed
    void determineVegetationPreset(const PlanetSeed& seed);

    // Generate a random seed if needed
    uint32_t ensureSeed(uint32_t seed);

    // Convert biome map to RGBA texture format
    void generateBiomeMapTexture();

    // Calculate world statistics
    void calculateStatistics(GeneratedWorld& world);

    IslandGenerator m_islandGenerator;
    std::unique_ptr<GeneratedWorld> m_currentWorld;
    std::vector<uint8_t> m_biomeMapRGBA;

    WorldGenConfig m_lastConfig;
    uint32_t m_lastSeed = 0;

    // Session seed tracking
    uint32_t m_sessionSeed = 0;
    bool m_wasAutoGenerated = false;

    // Cached palette variation for shader integration
    PaletteVariation m_cachedPaletteVariation;
    TerrainShaderParams m_cachedTerrainParams;
    VegetationDensityConfig m_vegetationConfig;
    StarShaderParams m_cachedStarParams;

    std::random_device m_rd;
    std::mt19937 m_rng;
};

// Integration helpers
namespace WorldGeneration {
    // Generate complete world configuration from string description
    // e.g., "alien purple archipelago" -> WorldGenConfig
    WorldGenConfig parseDescription(const std::string& description);

    // Get random island shape
    IslandShape randomIslandShape(uint32_t seed);

    // Get random planet theme
    PlanetPreset randomPlanetTheme(uint32_t seed);

    // Recommended combinations for interesting worlds
    struct WorldPreset {
        std::string name;
        IslandShape shape;
        PlanetPreset theme;
        float coastComplexity;
        bool hasRivers;
        bool hasLakes;
    };

    // Get list of curated world presets
    std::vector<WorldPreset> getCuratedPresets();

    // Get a random curated preset
    WorldPreset getRandomPreset(uint32_t seed);
}
