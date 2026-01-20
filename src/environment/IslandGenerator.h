#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include <random>
#include <functional>

// Forward declarations
class BiomeSystem;
class PlanetTheme;

// Island shape types for procedural generation
enum class IslandShape {
    CIRCULAR,           // Classic round island
    ARCHIPELAGO,        // Multiple connected islands
    CRESCENT,           // Moon-shaped island
    IRREGULAR,          // Organic, noise-driven shape
    VOLCANIC,           // Central peak with crater
    ATOLL,              // Ring-shaped with central lagoon
    CONTINENTAL         // Large landmass with varied coastline
};

// Coastal feature types
enum class CoastalFeature {
    BEACH,              // Sandy gradual slope
    CLIFF,              // Steep rocky drop
    MANGROVE,           // Shallow water with vegetation
    REEF,               // Underwater coral structures
    FJORD               // Deep water inlet
};

// Island generation parameters
struct IslandGenParams {
    // Shape parameters
    IslandShape shape = IslandShape::IRREGULAR;
    float islandRadius = 0.4f;          // 0-1 normalized, how much of map is land
    float coastalIrregularity = 0.3f;   // How jagged the coastline is
    float coastalErosion = 0.5f;        // Erosion simulation strength

    // Archipelago specific
    int archipelagoIslandCount = 5;     // Number of islands in archipelago
    float archipelagoSpread = 0.6f;     // How spread out islands are

    // Volcanic specific
    float volcanoHeight = 1.5f;         // Height multiplier for volcanic peak
    float craterSize = 0.15f;           // Size of volcanic crater
    bool hasLavaFlows = true;           // Whether to generate lava channels

    // Atoll specific
    float lagoonDepth = 0.3f;           // How deep the central lagoon is
    float reefWidth = 0.1f;             // Width of the coral reef ring

    // Terrain features
    float mountainousness = 0.5f;       // How mountainous the terrain is
    float riverDensity = 0.3f;          // How many rivers to generate
    float lakeDensity = 0.2f;           // Probability of lake basins
    bool generateCaves = true;          // Whether to mark cave entrances

    // Water parameters
    float waterLevel = 0.35f;           // Sea level (0-1)
    float underwaterDepth = 0.2f;       // How deep underwater terrain goes
    float beachWidth = 0.6f;            // Beach width as fraction of waterLevel (default: 60%)
    float maxBeachSlope = 0.06f;        // Maximum beach slope before becoming cliff

    // Generation seed
    uint32_t seed = 12345;
};

// River data for carving and rendering
struct RiverSegment {
    glm::vec2 start;
    glm::vec2 end;
    float width;
    float depth;
    int order;  // Strahler order for tributaries
};

// Lake data
struct LakeBasin {
    glm::vec2 center;
    float radius;
    float depth;
    float elevation;
    bool isVolcanic;  // Crater lake
};

// Cave entrance marker
struct CaveEntrance {
    glm::vec3 position;
    glm::vec3 direction;  // Into the hillside
    float size;
};

// Result of island generation
struct IslandData {
    std::vector<float> heightmap;           // Main terrain heightmap
    std::vector<float> underwaterHeightmap; // Seafloor terrain
    std::vector<uint8_t> coastalTypeMap;    // CoastalFeature per cell
    std::vector<RiverSegment> rivers;
    std::vector<LakeBasin> lakes;
    std::vector<CaveEntrance> caveEntrances;

    int width;
    int height;
    IslandGenParams params;

    // Helper methods
    float getHeight(int x, int y) const;
    float getHeightBilinear(float u, float v) const;
    CoastalFeature getCoastalType(int x, int y) const;
    bool isLand(int x, int y) const;
    bool isWater(int x, int y) const;
    bool isUnderwater(int x, int y) const;
};

// Main island generator class
class IslandGenerator {
public:
    static constexpr int DEFAULT_SIZE = 2048;

    IslandGenerator();
    ~IslandGenerator();

    // Main generation method
    IslandData generate(const IslandGenParams& params, int width = DEFAULT_SIZE, int height = DEFAULT_SIZE);

    // Generate with specific seed (convenience)
    IslandData generate(uint32_t seed, int width = DEFAULT_SIZE, int height = DEFAULT_SIZE);

    // Generate random island
    IslandData generateRandom(int width = DEFAULT_SIZE, int height = DEFAULT_SIZE);

    // Shape-specific generators
    IslandData generateCircular(const IslandGenParams& params, int size);
    IslandData generateArchipelago(const IslandGenParams& params, int size);
    IslandData generateCrescent(const IslandGenParams& params, int size);
    IslandData generateIrregular(const IslandGenParams& params, int size);
    IslandData generateVolcanic(const IslandGenParams& params, int size);
    IslandData generateAtoll(const IslandGenParams& params, int size);
    IslandData generateContinental(const IslandGenParams& params, int size);

    // Post-processing
    void applyCoastalErosion(IslandData& data, int iterations = 5);
    void carveRivers(IslandData& data);
    void createLakes(IslandData& data);
    void markCaveEntrances(IslandData& data);
    void generateUnderwaterTerrain(IslandData& data);
    void smoothCoastlines(IslandData& data, int iterations = 3);

    // Debug and analysis
    struct CoastalStats {
        int totalCoastalCells = 0;
        int beachCells = 0;
        int cliffCells = 0;
        int mangroveCells = 0;
        int reefCells = 0;
        int fjordCells = 0;
        float avgBeachSlope = 0.0f;
        float avgCliffSlope = 0.0f;
    };
    CoastalStats analyzeCoastline(const IslandData& data) const;

    // Utility
    static IslandShape randomShape(std::mt19937& rng);
    static IslandGenParams randomParams(uint32_t seed);

private:
    // Noise functions
    float perlin2D(float x, float y) const;
    float fbm(float x, float y, int octaves, float persistence, float lacunarity) const;
    float ridgedNoise(float x, float y, int octaves) const;
    float voronoi(float x, float y, float& cellId) const;
    float domainWarp(float x, float y, float strength) const;

    // Shape mask generators
    void generateCircularMask(std::vector<float>& mask, int size, float radius, float irregularity);
    void generateArchipelagoMask(std::vector<float>& mask, int size, int islandCount, float spread, float irregularity);
    void generateCrescentMask(std::vector<float>& mask, int size, float radius, float irregularity);
    void generateIrregularMask(std::vector<float>& mask, int size, float coverage, float irregularity);
    void generateVolcanicMask(std::vector<float>& mask, int size, float radius, float craterSize);
    void generateAtollMask(std::vector<float>& mask, int size, float radius, float lagoonSize, float reefWidth);
    void generateContinentalMask(std::vector<float>& mask, int size, float coverage);

    // Terrain feature generation
    void generateMountains(std::vector<float>& heightmap, int size, float intensity);
    void generateValleys(std::vector<float>& heightmap, int size, float intensity);
    void generatePlateaus(std::vector<float>& heightmap, int size, float intensity);
    void generateBeaches(std::vector<float>& heightmap, std::vector<uint8_t>& coastalMap, int size,
                         float waterLevel, float beachWidth, float maxBeachSlope);
    void generateCliffs(std::vector<float>& heightmap, std::vector<uint8_t>& coastalMap, int size, float waterLevel);

    // River generation (hydraulic erosion simplified)
    std::vector<RiverSegment> traceRivers(const std::vector<float>& heightmap, int size, int riverCount);
    void carveRiverBed(std::vector<float>& heightmap, int size, const RiverSegment& river);

    // Lake generation
    std::vector<LakeBasin> findLakeBasins(const std::vector<float>& heightmap, int size, float probability);
    void fillLakeBasin(std::vector<float>& heightmap, int size, const LakeBasin& lake);

    // Cave entrance detection
    std::vector<CaveEntrance> findCaveLocations(const std::vector<float>& heightmap, int size);

    // Erosion simulation
    void thermalErosion(std::vector<float>& heightmap, int size, int iterations);
    void hydraulicErosion(std::vector<float>& heightmap, int size, int iterations);
    void coastalErosion(std::vector<float>& heightmap, int size, float waterLevel, int iterations);

    // Underwater terrain
    void generateSeafloor(std::vector<float>& underwater, int size, float maxDepth);
    void generateCoralReefs(std::vector<float>& underwater, int size, float waterLevel);
    void generateKelpForests(std::vector<float>& underwater, int size, float waterLevel);

    // Permutation table for noise
    std::vector<int> m_perm;
    std::mt19937 m_rng;

    // Current seed for reproducibility
    uint32_t m_currentSeed;

    void initializeNoise(uint32_t seed);
};
