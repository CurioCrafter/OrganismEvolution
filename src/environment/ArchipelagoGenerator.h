#pragma once

#include "IslandGenerator.h"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include <random>

// Archipelago layout patterns
enum class ArchipelagoPattern {
    RANDOM,         // Random island placement
    LINEAR,         // Islands in a line (like Hawaii)
    CIRCULAR,       // Ring of islands around central lagoon
    CLUSTERED,      // Groups of closely spaced islands
    VOLCANIC_ARC,   // Arc shape (like Aleutian Islands)
    SCATTERED       // Widely spaced, no pattern
};

// Configuration for individual island within archipelago
struct IslandConfig {
    glm::vec2 position;             // World position (center of island)
    float size;                     // Island size multiplier (0.5-2.0)
    float elevation;                // Base elevation modifier
    uint32_t seed;                  // Unique seed for this island
    IslandShape shape;              // Island shape type
    IslandGenParams genParams;      // Full generation parameters

    // Metadata
    std::string name;               // Island name for UI
    int biomeIndex;                 // Primary biome type
    bool hasVolcano;                // Volcanic activity
    float isolationFactor;          // How isolated (affects migration)

    IslandConfig()
        : position(0.0f), size(1.0f), elevation(0.0f), seed(0),
          shape(IslandShape::IRREGULAR), biomeIndex(0),
          hasVolcano(false), isolationFactor(0.5f) {}
};

// Ocean current between islands (affects migration paths)
struct OceanCurrent {
    uint32_t fromIsland;
    uint32_t toIsland;
    float strength;                 // 0-1, affects migration probability
    glm::vec2 direction;            // Normalized direction
    float distance;                 // Distance between islands
    bool bidirectional;             // Can creatures travel both ways easily

    OceanCurrent()
        : fromIsland(0), toIsland(0), strength(0.5f),
          direction(0.0f), distance(0.0f), bidirectional(true) {}
};

// Wind pattern affecting flying creature migration
struct WindPattern {
    glm::vec2 prevailingDirection;
    float seasonalVariation;        // How much direction changes seasonally
    float strength;                 // Average wind strength

    WindPattern()
        : prevailingDirection(1.0f, 0.0f), seasonalVariation(0.3f), strength(0.5f) {}
};

// Result of archipelago generation
struct ArchipelagoData {
    std::vector<IslandConfig> islands;
    std::vector<OceanCurrent> currents;
    WindPattern wind;

    // Archipelago bounds
    glm::vec2 minBounds;
    glm::vec2 maxBounds;
    glm::vec2 center;
    float totalArea;

    // Helper methods
    const IslandConfig* getNearestIsland(const glm::vec2& position) const;
    float getDistanceBetween(uint32_t islandA, uint32_t islandB) const;
    const OceanCurrent* getCurrentBetween(uint32_t fromIsland, uint32_t toIsland) const;
    std::vector<uint32_t> getNeighborIslands(uint32_t islandIndex, float maxDistance) const;
};

class ArchipelagoGenerator {
public:
    // Configuration constants
    static constexpr float MIN_ISLAND_SPACING = 100.0f;     // Minimum distance between islands
    static constexpr float DEFAULT_SPACING = 200.0f;        // Default spacing
    static constexpr int MAX_ISLANDS = 16;                  // Maximum islands supported

    ArchipelagoGenerator();
    ~ArchipelagoGenerator() = default;

    // Main generation methods
    void generate(int islandCount, float spacing = DEFAULT_SPACING);
    void generate(int islandCount, float spacing, ArchipelagoPattern pattern);
    void generateWithSeed(int islandCount, float spacing, uint32_t seed);

    // Pattern-specific generators
    void generateRandom(int count, float spacing);
    void generateLinear(int count, float spacing);
    void generateCircular(int count, float spacing);
    void generateClustered(int count, float spacing);
    void generateVolcanicArc(int count, float spacing);
    void generateScattered(int count, float spacing);

    // Access generated data
    const std::vector<IslandConfig>& getIslandConfigs() const { return m_data.islands; }
    const ArchipelagoData& getArchipelagoData() const { return m_data; }

    // Individual island access
    const IslandConfig* getIsland(uint32_t index) const;
    uint32_t getIslandCount() const { return static_cast<uint32_t>(m_data.islands.size()); }

    // Ocean currents
    const std::vector<OceanCurrent>& getOceanCurrents() const { return m_data.currents; }
    float getMigrationDifficulty(uint32_t fromIsland, uint32_t toIsland) const;

    // Configuration
    void setBaseSeed(uint32_t seed) { m_baseSeed = seed; }
    void setIslandSizeVariation(float variation) { m_sizeVariation = variation; }
    void setShapeDistribution(const std::vector<std::pair<IslandShape, float>>& distribution);

    // Utility
    static std::string generateIslandName(uint32_t index, uint32_t seed);
    glm::vec2 worldToArchipelagoCoord(const glm::vec3& worldPos) const;
    int findNearestIsland(const glm::vec3& worldPos) const;

private:
    ArchipelagoData m_data;
    uint32_t m_baseSeed;
    float m_sizeVariation;          // How much island sizes vary (0-1)
    std::mt19937 m_rng;

    // Shape distribution (sum should be 1.0)
    std::vector<std::pair<IslandShape, float>> m_shapeWeights;

    // Internal methods
    void initializeRng(uint32_t seed);
    void calculateBounds();
    void generateOceanCurrents();
    void generateWindPatterns();
    void assignIslandProperties();

    // Island placement helpers
    bool isValidPlacement(const glm::vec2& position, float minDist) const;
    glm::vec2 findValidPosition(float minDist, const glm::vec2& hint, int maxAttempts = 100);

    // Shape selection
    IslandShape selectRandomShape();

    // Name generation syllables
    static const std::vector<std::string> NAME_PREFIXES;
    static const std::vector<std::string> NAME_ROOTS;
    static const std::vector<std::string> NAME_SUFFIXES;
};
