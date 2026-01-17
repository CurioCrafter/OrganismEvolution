#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <functional>

// Forward declarations
struct IslandData;
class PlanetTheme;

// Biome types - terrestrial
enum class BiomeType : uint8_t {
    // Water biomes
    DEEP_OCEAN = 0,
    OCEAN,
    SHALLOW_WATER,
    CORAL_REEF,
    KELP_FOREST,

    // Coastal biomes
    BEACH_SANDY,
    BEACH_ROCKY,
    TIDAL_POOL,
    MANGROVE,
    SALT_MARSH,

    // Lowland biomes
    GRASSLAND,
    SAVANNA,
    TROPICAL_RAINFOREST,
    TEMPERATE_FOREST,
    SWAMP,
    WETLAND,

    // Highland biomes
    SHRUBLAND,
    BOREAL_FOREST,
    ALPINE_MEADOW,
    ROCKY_HIGHLANDS,
    MOUNTAIN_FOREST,

    // Extreme biomes
    DESERT_HOT,
    DESERT_COLD,
    TUNDRA,
    GLACIER,
    VOLCANIC,
    LAVA_FIELD,
    CRATER_LAKE,

    // Special biomes
    CAVE_ENTRANCE,
    RIVER_BANK,
    LAKE_SHORE,

    BIOME_COUNT
};

// Biome properties for simulation and rendering
struct BiomeProperties {
    BiomeType type;
    std::string name;

    // Visual properties (can be modified by PlanetTheme)
    glm::vec3 baseColor;
    glm::vec3 accentColor;
    float roughness;
    float metallic;

    // Environmental properties
    float temperature;      // -1 to 1 (cold to hot)
    float moisture;         // 0 to 1
    float fertility;        // 0 to 1, affects plant growth
    float habitability;     // 0 to 1, for creature spawning

    // Terrain properties
    float minHeight;        // Normalized height range
    float maxHeight;
    float minSlope;         // Slope range (0-1)
    float maxSlope;

    // Vegetation
    float treeDensity;
    float grassDensity;
    float shrubDensity;

    // Wildlife
    float herbivoreCapacity;
    float carnivoreCapacity;
    float aquaticCapacity;
    float flyingCapacity;
};

// Biome transition data
struct BiomeTransition {
    BiomeType from;
    BiomeType to;
    float blendWidth;       // How gradual the transition is
    BiomeType transitionBiome; // Optional intermediate biome
};

// Biome cell in the map
struct BiomeCell {
    BiomeType primaryBiome;
    BiomeType secondaryBiome;   // For transitions
    float blendFactor;          // 0 = fully primary, 1 = fully secondary

    // Environmental factors
    float temperature;
    float moisture;
    float elevation;
    float slope;

    // Cached properties
    glm::vec3 color;
    float fertility;
};

// Climate zone definition
struct ClimateZone {
    float latitudeStart;    // -1 to 1 (south to north)
    float latitudeEnd;
    float baseTemperature;
    float baseMoisture;
    float temperatureVariation;
    float moistureVariation;
};

// Biome query result
struct BiomeQuery {
    BiomeType biome;
    BiomeProperties properties;
    glm::vec3 color;
    float blendFactor;

    // Neighboring biomes for smooth transitions
    BiomeType neighbors[4];
    float neighborWeights[4];
};

// Main biome system class
class BiomeSystem {
public:
    BiomeSystem();
    ~BiomeSystem();

    // Initialize with default Earth-like biomes
    void initializeDefaultBiomes();

    // Initialize with alien planet theme
    void initializeWithTheme(const PlanetTheme& theme);

    // Generate biome map from heightmap
    void generateBiomeMap(const std::vector<float>& heightmap, int width, int height, uint32_t seed);

    // Generate biome map from IslandData
    void generateFromIslandData(const struct IslandData& islandData);

    // Query biome at position
    BiomeQuery queryBiome(float worldX, float worldZ) const;
    BiomeQuery queryBiomeNormalized(float u, float v) const;  // 0-1 coords

    // Get biome cell directly
    const BiomeCell& getCell(int x, int y) const;
    BiomeType getBiomeAt(int x, int y) const;

    // Get biome properties
    const BiomeProperties& getProperties(BiomeType type) const;
    BiomeProperties& getMutableProperties(BiomeType type);

    // Modify biome colors (for alien themes)
    void setBaseColor(BiomeType type, const glm::vec3& color);
    void setAccentColor(BiomeType type, const glm::vec3& color);
    void applyColorShift(const glm::vec3& hueShift);  // HSV shift to all biomes

    // Climate simulation
    void setClimateZones(const std::vector<ClimateZone>& zones);
    float getTemperature(float x, float y) const;
    float getMoisture(float x, float y) const;

    // Vegetation queries (for VegetationManager integration)
    float getTreeDensity(float worldX, float worldZ) const;
    float getGrassDensity(float worldX, float worldZ) const;
    float getShrubDensity(float worldX, float worldZ) const;
    bool canPlaceTree(float worldX, float worldZ, float& outScale) const;

    // Wildlife queries (for creature spawning)
    float getHerbivoreCapacity(float worldX, float worldZ) const;
    float getCarnivoreCapacity(float worldX, float worldZ) const;
    float getAquaticCapacity(float worldX, float worldZ) const;
    float getFlyingCapacity(float worldX, float worldZ) const;

    // Biome transition helpers
    void addTransition(BiomeType from, BiomeType to, float blendWidth, BiomeType intermediate = BiomeType::BIOME_COUNT);
    float getTransitionFactor(BiomeType from, BiomeType to, float distance) const;

    // Serialization
    void serialize(std::vector<uint8_t>& data) const;
    void deserialize(const std::vector<uint8_t>& data);

    // Debug/visualization
    std::vector<glm::vec3> generateBiomeColorMap() const;
    std::string getBiomeName(BiomeType type) const;

    // Biome diversity metrics (Phase 11 Agent 3)
    struct BiomeDiversityMetrics {
        int totalBiomeCount = 0;              // Number of distinct biomes
        int largestPatchSize = 0;              // Size of largest contiguous patch
        std::vector<int> biomeCounts;          // Count per biome type
        std::vector<int> patchSizes;           // All patch sizes
        float diversityIndex = 0.0f;           // Shannon diversity index
        float dominance = 0.0f;                // Largest patch / total cells
    };
    BiomeDiversityMetrics calculateDiversityMetrics() const;
    void logDiversityMetrics() const;

    // Accessors
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    float getWorldScale() const { return m_worldScale; }
    void setWorldScale(float scale) { m_worldScale = scale; }

private:
    // Biome determination from environmental factors
    BiomeType determineBiome(float height, float slope, float temperature, float moisture, float distanceToWater) const;

    // Environmental factor calculation
    float calculateTemperature(float height, float latitude, float localVariation) const;
    float calculateMoisture(float height, float distanceToWater, float windExposure) const;
    float calculateSlope(const std::vector<float>& heightmap, int x, int y, int width) const;
    float calculateDistanceToWater(const std::vector<float>& heightmap, int x, int y, int width, int height, float waterLevel) const;

    // Transition smoothing
    void smoothTransitions(int iterations);
    void calculateBlendFactors();

    // Color blending
    glm::vec3 blendBiomeColors(BiomeType primary, BiomeType secondary, float factor) const;

    // Initialize biome properties
    void initializeBiomeProperties();

    // Patch noise generation (Phase 11 Agent 3)
    float generatePatchNoise(float x, float y, uint32_t seed) const;
    float perlinNoise(float x, float y, uint32_t seed) const;

    // Data
    std::vector<BiomeCell> m_biomeMap;
    std::unordered_map<BiomeType, BiomeProperties> m_properties;
    std::vector<BiomeTransition> m_transitions;
    std::vector<ClimateZone> m_climateZones;

    int m_width = 0;
    int m_height = 0;
    float m_worldScale = 1.0f;
    float m_waterLevel = 0.35f;

    // Default cell for out-of-bounds queries
    BiomeCell m_defaultCell;

    // Cached distance-to-water map
    std::vector<float> m_distanceToWaterMap;
};

// Utility functions
const char* biomeToString(BiomeType type);
BiomeType stringToBiome(const std::string& name);
glm::vec3 getDefaultBiomeColor(BiomeType type);
bool isAquaticBiome(BiomeType type);
bool isTransitionBiome(BiomeType type);
