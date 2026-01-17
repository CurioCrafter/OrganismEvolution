#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

// Prevent Windows min/max macro conflicts
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

// Include DX12Device header to avoid forward declaration conflicts
#include "../graphics/DX12Device.h"

// Forward declarations
class Terrain;
class ClimateSystem;
class SeasonManager;
class WeatherSystem;

// Types of grass and ground cover
enum class GrassType {
    // Standard grasses
    LAWN_GRASS,          // Short, dense lawn-type grass
    MEADOW_GRASS,        // Medium height meadow grass
    TALL_GRASS,          // Tall prairie/savanna grass
    WILD_GRASS,          // Irregular, natural wild grass

    // Regional grasses
    TROPICAL_GRASS,      // Broad-leaved tropical grass
    TUNDRA_GRASS,        // Short, hardy tundra grass
    MARSH_GRASS,         // Wetland grass/reeds
    DUNE_GRASS,          // Coastal dune grass
    ALPINE_GRASS,        // Mountain meadow grass
    BAMBOO_GRASS,        // Small bamboo-like grass

    // Ornamental/special
    PAMPAS_GRASS,        // Tall feathery pampas grass
    FOUNTAIN_GRASS,      // Ornamental fountain grass
    BLUE_GRASS,          // Bluish-tinted grass
    RED_GRASS,           // Reddish ornamental grass

    // Alien grasses
    BIOLUMINESCENT_GRASS, // Glowing alien grass
    CRYSTAL_GRASS,        // Crystalline grass blades
    TENDRIL_GRASS,        // Writhing alien grass
    SPORE_GRASS,          // Spore-releasing grass

    COUNT
};

// Types of flowers
enum class FlowerType {
    // Wildflowers
    DAISY,
    POPPY,
    DANDELION,
    BUTTERCUP,
    CLOVER,
    BLUEBELL,
    LAVENDER,
    SUNFLOWER,
    VIOLET,
    MARIGOLD,

    // Seasonal
    TULIP,               // Spring
    DAFFODIL,            // Spring
    CHRYSANTHEMUM,       // Fall
    SNOWDROP,            // Late winter/early spring

    // Tropical
    HIBISCUS,
    ORCHID,
    BIRD_OF_PARADISE,
    PLUMERIA,

    // Alpine
    EDELWEISS,
    ALPINE_ASTER,
    MOUNTAIN_AVENS,

    // Alien
    GLOW_BLOOM,          // Bioluminescent flower
    CRYSTAL_FLOWER,      // Crystalline flower
    VOID_BLOSSOM,        // Dark energy flower
    PLASMA_FLOWER,       // Energy flower

    COUNT
};

// Pollination state
enum class PollinationState {
    CLOSED,              // Flower not yet open
    BLOOMING,            // Open and producing pollen
    POLLINATED,          // Successfully pollinated
    PRODUCING_SEEDS,     // Creating seeds
    DISPERSING,          // Releasing seeds
    WILTED               // Flower lifecycle complete
};

// Single grass blade instance data (for GPU instancing)
struct GrassBladeInstance {
    glm::vec3 position;      // Base position
    float rotation;          // Y-axis rotation
    float height;            // Blade height
    float width;             // Blade width at base
    float bendFactor;        // How much it bends (0-1)
    float colorVariation;    // Color randomization (0-1)
    GrassType type;          // Type of grass
    float windPhase;         // Individual wind animation phase
    float grazedAmount;      // How much has been eaten (0-1)
    float regrowthProgress;  // Regrowth after grazing (0-1)
};

// Flower instance data
struct FlowerInstance {
    glm::vec3 position;
    float rotation;
    float scale;
    FlowerType type;
    PollinationState state;
    glm::vec3 petalColor;
    glm::vec3 centerColor;
    float bloomProgress;     // 0-1 how open the flower is
    float pollenAmount;      // 0-1 pollen available
    float nectarAmount;      // 0-1 nectar available
    float age;               // Days since sprouting
    float health;            // 0-1 overall health
    bool hasBeenVisited;     // By a pollinator
    int seedsProduced;       // Number of seeds created
};

// Ground cover types (moss, lichen, etc.)
enum class GroundCoverType {
    MOSS_GREEN,
    MOSS_BROWN,
    MOSS_CUSHION,
    LICHEN_CRUSTOSE,
    LICHEN_FOLIOSE,
    LICHEN_FRUTICOSE,
    FALLEN_LEAVES,
    PINE_NEEDLES,
    DEAD_GRASS,
    GRAVEL,
    MUD,
    SNOW_PATCH,
    ICE_PATCH,

    // Alien ground cover
    BIOLUMINESCENT_MOSS,
    CRYSTAL_GROWTH,
    ALIEN_SLIME,

    COUNT
};

// Ground cover instance
struct GroundCoverInstance {
    glm::vec3 position;
    float rotation;
    float scale;
    GroundCoverType type;
    glm::vec3 color;
    float density;           // How thick the coverage
    float moisture;          // Affects appearance
};

// Grazing event data
struct GrazingEvent {
    glm::vec3 position;
    float radius;
    float intensity;         // How much to graze (0-1)
    float time;              // When grazing occurred
};

// Grass patch - a collection of blades in an area
struct GrassPatch {
    glm::vec3 center;
    float radius;
    std::vector<GrassBladeInstance> blades;
    std::vector<FlowerInstance> flowers;
    std::vector<GroundCoverInstance> groundCover;
    bool isVisible = true;
    float lodLevel = 0.0f;

    // Grazing state
    float grazedAmount = 0.0f;  // Overall grazing level
    float lastGrazedTime = 0.0f;
    float regrowthRate = 0.1f;  // Rate of regrowth per day

    // Biomass for ecosystem
    float biomass = 100.0f;     // Total plant material
    float maxBiomass = 100.0f;  // Maximum capacity
};

// Grass configuration per biome
struct GrassConfig {
    float density = 1.0f;           // Blades per square unit
    float minHeight = 0.2f;
    float maxHeight = 0.5f;
    float minWidth = 0.02f;
    float maxWidth = 0.05f;
    glm::vec3 baseColor = {0.3f, 0.5f, 0.2f};
    glm::vec3 tipColor = {0.4f, 0.6f, 0.25f};
    bool hasFlowers = false;
    float flowerDensity = 0.0f;
    GrassType primaryType = GrassType::MEADOW_GRASS;
    std::vector<GrassType> allowedTypes;
    std::vector<FlowerType> allowedFlowers;
    bool hasGroundCover = true;
    float groundCoverDensity = 0.3f;
};

// Flower species configuration
struct FlowerSpeciesConfig {
    FlowerType type;
    std::string name;

    // Appearance
    glm::vec3 defaultPetalColor;
    glm::vec3 defaultCenterColor;
    float minSize;
    float maxSize;
    int petalCount;

    // Lifecycle
    float bloomDuration;        // Days flower stays open
    float pollenProduction;     // Pollen per day
    float nectarProduction;     // Nectar per day
    float seedProduction;       // Seeds when pollinated

    // Environmental preferences
    float minTemperature;
    float maxTemperature;
    float minMoisture;
    float maxMoisture;

    // Bloom timing
    float bloomSeasonStart;     // Day of year (0-365)
    float bloomSeasonEnd;

    // Special properties
    bool isBioluminescent;
    float glowIntensity;
    bool attractsPollinators;
    float attractionRadius;
};

// Get configuration for a flower species
FlowerSpeciesConfig getFlowerSpeciesConfig(FlowerType type);

// Get grass type characteristics
struct GrassTypeConfig {
    GrassType type;
    float baseHeight;
    float heightVariation;
    float baseWidth;
    float widthVariation;
    float stiffness;           // How much it resists wind
    glm::vec3 baseColor;
    glm::vec3 tipColor;
    bool isAlien;
    float glowIntensity;
};

GrassTypeConfig getGrassTypeConfig(GrassType type);

class GrassSystem {
public:
    GrassSystem();
    ~GrassSystem();

    // Initialize with device and terrain reference
    void initialize(DX12Device* device, const Terrain* terrain);

    // Set additional system references
    void setClimateSystem(ClimateSystem* climate) { climateSystem = climate; }
    void setSeasonManager(SeasonManager* season) { seasonManager = season; }
    void setWeatherSystem(WeatherSystem* weather) { weatherSystem = weather; }

    // Generate grass for terrain
    void generate(unsigned int seed);

    // Update grass animation (wind, etc.)
    void update(float deltaTime, const glm::vec3& cameraPos);

    // Render grass instances
    void render(ID3D12GraphicsCommandList* commandList);

    // Get instance data for rendering
    const std::vector<GrassBladeInstance>& getInstances() const { return allInstances; }
    const std::vector<FlowerInstance>& getFlowerInstances() const { return allFlowers; }
    const std::vector<GroundCoverInstance>& getGroundCoverInstances() const { return allGroundCover; }
    size_t getVisibleInstanceCount() const { return visibleInstanceCount; }
    size_t getVisibleFlowerCount() const { return visibleFlowerCount; }

    // Configuration
    void setMaxRenderDistance(float dist) { maxRenderDistance = dist; }
    void setDensityMultiplier(float mult) { densityMultiplier = mult; }
    void setWindStrength(float strength) { windStrength = strength; }
    void setWindDirection(const glm::vec2& dir) { windDirection = glm::normalize(dir); }

    // Get grass configuration for biome
    static GrassConfig getConfigForBiome(int biomeType);

    // ===== Grazing System =====

    // Apply grazing effect at a location (called by creatures eating grass)
    void applyGrazing(const glm::vec3& position, float radius, float intensity);

    // Get grazing food value at a location
    float getGrazingFoodValue(const glm::vec3& position, float radius) const;

    // Check if location has grazeable grass
    bool hasGrazeableGrass(const glm::vec3& position, float radius) const;

    // Get biomass at location (for ecosystem calculations)
    float getBiomassAt(const glm::vec3& position, float radius) const;

    // ===== Pollination System =====

    // Find flowers that need pollination near a position
    std::vector<FlowerInstance*> findPollinatingFlowers(const glm::vec3& position, float radius);

    // Pollinator visits a flower (returns pollen/nectar collected)
    struct PollinatorReward {
        float pollenCollected;
        float nectarCollected;
        bool causedPollination;
    };
    PollinatorReward pollinatorVisit(FlowerInstance* flower, float pollenCarried);

    // Get flowers producing seeds (for seed dispersal)
    std::vector<glm::vec3> getSeedPositions() const;

    // Plant a seed at location
    bool plantSeed(const glm::vec3& position, FlowerType type);

    // ===== Ecosystem Integration =====

    // Get total ecosystem statistics
    struct GrassEcosystemStats {
        float totalBiomass;
        float totalGrazedArea;
        int totalFlowers;
        int bloomingFlowers;
        int pollinatedFlowers;
        float averageGrassHealth;
    };
    GrassEcosystemStats getEcosystemStats() const;

    // Get bioluminescent grass positions for night lighting
    std::vector<std::pair<glm::vec3, glm::vec3>> getBioluminescentGrassPositions() const;

    // Get alien grass positions
    std::vector<glm::vec3> getAlienGrassPositions() const;

private:
    DX12Device* dx12Device = nullptr;
    const Terrain* terrain = nullptr;
    ClimateSystem* climateSystem = nullptr;
    SeasonManager* seasonManager = nullptr;
    WeatherSystem* weatherSystem = nullptr;

    // All grass patches
    std::vector<GrassPatch> patches;

    // Flattened instance buffers for rendering
    std::vector<GrassBladeInstance> allInstances;
    std::vector<FlowerInstance> allFlowers;
    std::vector<GroundCoverInstance> allGroundCover;

    size_t visibleInstanceCount = 0;
    size_t visibleFlowerCount = 0;

    // DX12 resources
    ComPtr<ID3D12Resource> instanceBuffer;
    ComPtr<ID3D12Resource> instanceUploadBuffer;
    ComPtr<ID3D12Resource> flowerInstanceBuffer;
    ComPtr<ID3D12Resource> groundCoverInstanceBuffer;
    D3D12_VERTEX_BUFFER_VIEW instanceBufferView = {};
    D3D12_VERTEX_BUFFER_VIEW flowerBufferView = {};

    // Rendering settings
    float maxRenderDistance = 1000.0f;
    float lodDistance1 = 250.0f;
    float lodDistance2 = 600.0f;
    float densityMultiplier = 1.0f;

    // Wind animation
    float windStrength = 0.3f;
    glm::vec2 windDirection = {1.0f, 0.0f};
    float windTime = 0.0f;

    // Seasonal state
    float seasonalDensity = 1.0f;
    glm::vec3 seasonalColorTint = {1.0f, 1.0f, 1.0f};

    // Simulation time
    float simulationTime = 0.0f;
    float currentDayOfYear = 180.0f; // Start in summer

    // Grazing tracking
    std::vector<GrazingEvent> recentGrazingEvents;
    float grazingDecayRate = 0.05f;   // How fast grazing recovers

    // Generation helpers
    void generatePatch(const glm::vec3& center, float radius, const GrassConfig& config, unsigned int seed);
    void generateFlowersInPatch(GrassPatch& patch, const GrassConfig& config, unsigned int seed);
    void generateGroundCoverInPatch(GrassPatch& patch, const GrassConfig& config, unsigned int seed);
    bool isSuitableForGrass(float x, float z) const;
    GrassConfig getLocalConfig(float x, float z) const;
    GrassType selectGrassType(const GrassConfig& config, float x, float z, unsigned int seed) const;
    FlowerType selectFlowerType(const GrassConfig& config, float x, float z, unsigned int seed) const;

    // Update helpers
    void updateSeasonalEffects();
    void updateVisibility(const glm::vec3& cameraPos);
    void updateInstanceBuffer();
    void updateGrazingRecovery(float deltaTime);
    void updateFlowerLifecycles(float deltaTime);
    void updateWindAnimation(float deltaTime);

    // Buffer management
    void createBuffers();
    void uploadInstanceData();

    // Flower helpers
    void updateFlowerBloom(FlowerInstance& flower, float deltaTime);
    void updateFlowerPollination(FlowerInstance& flower, float deltaTime);
    glm::vec3 getFlowerSeasonalColor(FlowerType type, float dayOfYear) const;
    bool isFlowerInBloomSeason(FlowerType type, float dayOfYear) const;
};
