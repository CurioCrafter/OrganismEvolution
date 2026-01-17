#pragma once

#include "LSystem.h"
#include "../graphics/mesh/MeshData.h"
#include <glm/glm.hpp>
#include <stack>
#include <functional>

enum class TreeType {
    // Temperate
    OAK,
    PINE,
    WILLOW,
    BIRCH,
    MAPLE,
    CHERRY_BLOSSOM,
    APPLE,

    // Tropical
    PALM,
    MANGROVE,
    KAPOK,
    BANYAN,
    COCONUT_PALM,
    BAMBOO,

    // Desert
    CACTUS_SAGUARO,
    CACTUS_BARREL,
    JOSHUA_TREE,
    PRICKLY_PEAR,

    // Boreal
    SPRUCE,
    FIR,
    LARCH,

    // Savanna
    ACACIA,
    BAOBAB,
    UMBRELLA_THORN,

    // Swamp
    CYPRESS,
    BALD_CYPRESS,
    WATER_OAK,

    // Mountain
    JUNIPER,
    ALPINE_FIR,
    MOUNTAIN_ASH,
    BRISTLECONE_PINE,

    // Alien/Fantasy
    CRYSTAL_TREE,
    BIOLUMINESCENT_TREE,
    FLOATING_TREE,
    TENDRIL_TREE,
    SPORE_TREE,
    PLASMA_TREE,

    // Generic
    BUSH,
    FLOWERING_BUSH,
    BERRY_BUSH,

    COUNT
};

// Seasonal state for trees
enum class TreeSeasonalState {
    DORMANT,       // Winter - bare branches
    BUDDING,       // Early spring - small buds
    FLOWERING,     // Spring - flowers/blossoms
    FULL_FOLIAGE,  // Summer - full leaves
    FRUITING,      // Late summer/early fall - fruit production
    AUTUMN_COLORS, // Fall - colored leaves
    LEAF_DROP      // Late fall - losing leaves
};

// Growth stage for trees
enum class TreeGrowthStage {
    SEEDLING,    // Just planted, very small
    SAPLING,     // Young tree, small
    MATURE,      // Full grown
    OLD_GROWTH,  // Ancient, large
    DYING,       // Decaying
    DEAD         // Standing dead tree (snag)
};

// Fruit/seed data for plant-creature interactions
struct FruitData {
    glm::vec3 position;
    float ripeness;      // 0-1, affects nutritional value
    float size;
    bool isEdible;
    float energyValue;
    float toxicity;      // 0-1, some fruits are poisonous
};

// Complete tree instance data with lifecycle
struct TreeInstanceData {
    glm::vec3 position;
    float rotation;
    float scale;
    TreeType type;

    // Lifecycle
    TreeGrowthStage growthStage = TreeGrowthStage::MATURE;
    TreeSeasonalState seasonalState = TreeSeasonalState::FULL_FOLIAGE;
    float age = 0.0f;          // Days since planting
    float health = 1.0f;       // 0-1, affects appearance and production
    float growthProgress = 1.0f; // 0-1 within current growth stage

    // Production
    std::vector<FruitData> fruits;
    float fruitProductionTimer = 0.0f;
    int seedsProduced = 0;

    // Appearance modifiers
    float leafDensity = 1.0f;
    glm::vec3 leafColorTint = glm::vec3(1.0f);
    float barkDamage = 0.0f;   // 0-1, from creatures/weather

    // Death/decay
    float decayProgress = 0.0f;
    bool hasBeenConsumed = false;
};

// 3D Turtle State with full coordinate frame
struct TurtleState {
    glm::vec3 position;
    glm::vec3 heading;    // Forward direction (H)
    glm::vec3 left;       // Left direction (L)
    glm::vec3 up;         // Up direction (U)
    float radius;
    int depth;

    TurtleState()
        : position(0.0f)
        , heading(0.0f, 1.0f, 0.0f)   // Start pointing up
        , left(-1.0f, 0.0f, 0.0f)     // Left is -X
        , up(0.0f, 0.0f, 1.0f)        // Up is +Z (perpendicular to heading)
        , radius(0.15f)
        , depth(0) {}

    // Rotate around Up vector (yaw - turn left/right)
    void rotateU(float angle);

    // Rotate around Left vector (pitch - tilt up/down)
    void rotateL(float angle);

    // Rotate around Heading vector (roll)
    void rotateH(float angle);
};

// Tree species configuration
struct TreeSpeciesConfig {
    TreeType type;
    std::string name;

    // Growth parameters
    float maxAge;              // Days to reach old growth
    float growthRate;          // Base growth speed multiplier
    float maxHeight;           // Maximum tree height
    float canopySpread;        // How wide the canopy spreads

    // Environmental preferences
    float minTemperature;      // Celsius
    float maxTemperature;
    float minMoisture;         // 0-1
    float maxMoisture;
    float minElevation;        // 0-1 normalized
    float maxElevation;

    // Production
    bool producesFruit;
    float fruitEnergyValue;
    float fruitSeasonStart;    // Day of year (0-365)
    float fruitSeasonLength;   // Days

    // Seasonal behavior
    bool isDeciduous;          // Loses leaves in fall
    bool isEvergreen;
    bool hasFlowers;
    glm::vec3 flowerColor;
    glm::vec3 autumnColor;

    // Special properties
    bool isBioluminescent;
    bool isAlien;
    float alienIntensity;      // Glow/special effect intensity
};

// Get configuration for a tree species
TreeSpeciesConfig getTreeSpeciesConfig(TreeType type);

// Check if a tree type is alien/fantasy
bool isAlienTreeType(TreeType type);

// Get seasonal leaf color for deciduous trees
glm::vec3 getSeasonalLeafColor(TreeType type, TreeSeasonalState state, float progress);

class TreeGenerator {
public:
    // Generate a tree/bush mesh from L-system
    static MeshData generateTree(TreeType type, unsigned int seed = 0);

    // Generate tree with specific growth stage and seasonal state
    static MeshData generateTreeWithState(
        TreeType type,
        TreeGrowthStage growth,
        TreeSeasonalState season,
        float leafDensity = 1.0f,
        const glm::vec3& leafColorTint = glm::vec3(1.0f),
        unsigned int seed = 0
    );

    // Generate dead/snag tree
    static MeshData generateDeadTree(TreeType type, float decayProgress, unsigned int seed = 0);

    // Generate bush mesh
    static MeshData generateBush(unsigned int seed = 0);

    // Generate flowering bush
    static MeshData generateFloweringBush(const glm::vec3& flowerColor, unsigned int seed = 0);

    // Generate berry bush with fruit
    static MeshData generateBerryBush(float fruitDensity, const glm::vec3& berryColor, unsigned int seed = 0);

    // Generate alien tree types
    static MeshData generateCrystalTree(const glm::vec3& crystalColor, float complexity, unsigned int seed = 0);
    static MeshData generateBioluminescentTree(const glm::vec3& glowColor, float glowIntensity, unsigned int seed = 0);
    static MeshData generateFloatingTree(float floatHeight, unsigned int seed = 0);
    static MeshData generateTendrilTree(int tendrilCount, unsigned int seed = 0);
    static MeshData generateSporeTree(float sporeCloudDensity, unsigned int seed = 0);

    // Get fruit positions from a tree mesh
    static std::vector<glm::vec3> getFruitPositions(TreeType type, const MeshData& treeMesh, unsigned int seed = 0);

private:
    // Create L-system for specific tree type
    static LSystem createLSystem(TreeType type);

    // Create L-system with growth stage modifier
    static LSystem createLSystemWithGrowth(TreeType type, TreeGrowthStage growth);

    // Interpret L-system string to generate mesh using 3D turtle graphics
    static MeshData interpretLSystem(
        const std::string& lString,
        float angle,
        float baseRadius,
        float segmentLength,
        TreeType type
    );

    // Enhanced interpretation with seasonal/growth modifiers
    static MeshData interpretLSystemEnhanced(
        const std::string& lString,
        float angle,
        float baseRadius,
        float segmentLength,
        TreeType type,
        float leafDensity,
        const glm::vec3& leafColorTint,
        bool addFruit = false
    );

    // Add a cylinder segment to the mesh
    static void addBranch(
        MeshData& mesh,
        const glm::vec3& start,
        const glm::vec3& end,
        float startRadius,
        float endRadius,
        const glm::vec3& color,
        int segments = 6
    );

    // Add damaged/decaying branch
    static void addDecayedBranch(
        MeshData& mesh,
        const glm::vec3& start,
        const glm::vec3& end,
        float startRadius,
        float endRadius,
        float decayLevel
    );

    // Add leaves cluster (sphere-like for oaks/willows)
    static void addLeafCluster(
        MeshData& mesh,
        const glm::vec3& position,
        float size,
        const glm::vec3& color
    );

    // Add sparse leaves for autumn/spring
    static void addSparseLeaves(
        MeshData& mesh,
        const glm::vec3& position,
        float size,
        const glm::vec3& color,
        float density
    );

    // Add flower cluster
    static void addFlowerCluster(
        MeshData& mesh,
        const glm::vec3& position,
        float size,
        const glm::vec3& petalColor,
        const glm::vec3& centerColor
    );

    // Add fruit
    static void addFruit(
        MeshData& mesh,
        const glm::vec3& position,
        float size,
        const glm::vec3& color
    );

    // Add pine needle cluster (cone-like for pines)
    static void addPineNeedles(
        MeshData& mesh,
        const glm::vec3& position,
        const glm::vec3& direction,
        float size,
        const glm::vec3& color
    );

    // Add bush foliage (dense, rounded)
    static void addBushFoliage(
        MeshData& mesh,
        const glm::vec3& position,
        float size,
        const glm::vec3& color
    );

    // Add berry clusters to bush
    static void addBerryClusters(
        MeshData& mesh,
        const glm::vec3& position,
        float size,
        const glm::vec3& berryColor,
        float density
    );

    // Alien tree components
    static void addCrystalFormation(
        MeshData& mesh,
        const glm::vec3& position,
        float size,
        const glm::vec3& color,
        int facets
    );

    static void addGlowingOrb(
        MeshData& mesh,
        const glm::vec3& position,
        float size,
        const glm::vec3& color
    );

    static void addTendril(
        MeshData& mesh,
        const glm::vec3& start,
        const glm::vec3& end,
        float thickness,
        const glm::vec3& color,
        int segments
    );

    static void addSporeCloud(
        MeshData& mesh,
        const glm::vec3& position,
        float radius,
        const glm::vec3& color,
        int particleCount
    );

    // Helper: rotate vector around axis by angle (radians)
    static glm::vec3 rotateAroundAxis(
        const glm::vec3& vec,
        const glm::vec3& axis,
        float angle
    );

    // Get bark color for tree type (with optional damage/decay)
    static glm::vec3 getBarkColor(TreeType type, float damage = 0.0f);

    // Get leaf color for tree type with seasonal modifier
    static glm::vec3 getLeafColor(TreeType type, TreeSeasonalState season = TreeSeasonalState::FULL_FOLIAGE);
};
