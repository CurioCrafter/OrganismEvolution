#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>

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

// Forward declarations
class DX12Device;
class Terrain;
class ClimateSystem;
class SeasonManager;
class WeatherSystem;

// Types of aquatic plants
enum class AquaticPlantType {
    // Kelp and seaweed (underwater, rooted to bottom)
    KELP_GIANT,          // Tall kelp forests
    KELP_BULL,           // Bull kelp with bulbs
    KELP_RIBBON,         // Flat ribbon kelp
    SEAWEED_GREEN,       // Common green seaweed
    SEAWEED_BROWN,       // Brown algae
    SEAWEED_RED,         // Red algae
    SEAWEED_SARGASSUM,   // Floating seaweed masses

    // Surface plants
    LILY_PAD,            // Floating lily pads
    WATER_LILY,          // Lily with flowers
    LOTUS,               // Sacred lotus
    DUCKWEED,            // Tiny floating plants
    WATER_HYACINTH,      // Purple flowering floater
    WATER_LETTUCE,       // Rosette floater

    // Rooted underwater plants
    EELGRASS,            // Seagrass meadows
    SEAGRASS,            // General seagrass
    WATER_MILFOIL,       // Feathery underwater
    HORNWORT,            // Submerged plant
    PONDWEED,            // Freshwater rooted

    // Emergent plants (roots underwater, stems above)
    CATTAIL,             // Reed-like cattails
    REED,                // Common reeds
    BULRUSH,             // Bulrush/tule
    PAPYRUS,             // Papyrus reeds
    MANGROVE_PROP_ROOT,  // Mangrove root systems

    // Coral (animal/plant hybrid behavior)
    CORAL_BRAIN,         // Brain coral
    CORAL_STAGHORN,      // Branching staghorn
    CORAL_TABLE,         // Flat table coral
    CORAL_FAN,           // Sea fan coral
    CORAL_PILLAR,        // Pillar coral
    CORAL_MUSHROOM,      // Mushroom coral
    ANEMONE,             // Sea anemone

    // Alien aquatic
    BIOLUMINESCENT_KELP, // Glowing kelp
    CRYSTAL_CORAL,       // Crystalline coral
    PLASMA_POLYP,        // Energy-based polyp
    VOID_ANEMONE,        // Dark energy anemone
    TENDRIL_SEAWEED,     // Writhing seaweed

    COUNT
};

// Water depth zone
enum class WaterZone {
    SURFACE,             // Floating on surface
    SHALLOW,             // 0-5m depth
    MEDIUM,              // 5-20m depth
    DEEP,                // 20-50m depth
    ABYSS,               // >50m depth
    TIDAL,               // Intertidal zone
    FRESHWATER,          // Lakes/rivers
    BRACKISH             // Mixed salinity
};

// Coral health state
enum class CoralHealthState {
    THRIVING,            // Healthy, vibrant colors
    STRESSED,            // Slightly bleached
    BLEACHING,           // Actively bleaching
    BLEACHED,            // Fully bleached (white)
    RECOVERING,          // Regaining color
    DEAD                 // No longer living
};

// Single aquatic plant instance
struct AquaticPlantInstance {
    glm::vec3 position;
    float rotation;
    float scale;
    AquaticPlantType type;

    // Physical state
    float swayPhase;         // Current sway animation phase
    float swayAmplitude;     // How much it sways
    float currentHeight;     // Can change with growth
    float health;            // 0-1 plant health

    // For kelp/seaweed - multiple segments
    int segmentCount;
    std::vector<glm::vec3> segmentPositions;

    // For coral
    CoralHealthState coralHealth;
    float bleachProgress;    // 0-1 bleaching amount
    glm::vec3 originalColor;
    glm::vec3 currentColor;

    // For lily pads - surface tracking
    float waterSurfaceHeight;
    float padSize;
    bool hasFlower;
    glm::vec3 flowerColor;

    // Ecosystem
    float oxygenProduction;  // O2 per day
    float shelterValue;      // Cover for fish
    float foodValue;         // Edible by creatures
    bool isPollinated;       // For flowering plants
};

// Kelp forest structure (multiple plants form a forest)
struct KelpForest {
    glm::vec3 center;
    float radius;
    std::vector<AquaticPlantInstance> plants;
    float totalBiomass;
    float oxygenOutput;
    float temperature;       // Local water temp
    float currentStrength;   // Water current
};

// Coral reef structure
struct CoralReef {
    glm::vec3 center;
    float radius;
    std::vector<AquaticPlantInstance> corals;
    float overallHealth;     // 0-1 reef health
    float biodiversityScore; // Species diversity
    float calcificationRate; // Growth rate
    float temperature;       // Critical for bleaching
    bool isProtected;        // Marine sanctuary
};

// Lily pad cluster
struct LilyPadCluster {
    glm::vec3 center;
    float radius;
    std::vector<AquaticPlantInstance> pads;
    int floweringCount;
    float coveragePercent;   // Surface area covered
};

// Aquatic plant species configuration
struct AquaticPlantConfig {
    AquaticPlantType type;
    std::string name;

    // Physical properties
    float minHeight;
    float maxHeight;
    float minScale;
    float maxScale;
    int minSegments;
    int maxSegments;

    // Environmental requirements
    WaterZone preferredZone;
    float minDepth;
    float maxDepth;
    float minTemperature;    // Celsius
    float maxTemperature;
    float minSalinity;       // 0-1 (0=fresh, 1=ocean)
    float maxSalinity;
    float minLight;          // Light required (0-1)

    // Growth and production
    float growthRate;
    float oxygenProduction;
    float carbonSequestration;

    // Visual
    glm::vec3 primaryColor;
    glm::vec3 secondaryColor;
    float swaySpeed;
    float swayAmount;
    bool isBioluminescent;
    float glowIntensity;

    // Ecosystem role
    float shelterFactor;     // How much shelter it provides
    float foodValue;         // Nutritional value
    bool attractsFish;
    float fishAttractionRadius;
};

// Get configuration for an aquatic plant type
AquaticPlantConfig getAquaticPlantConfig(AquaticPlantType type);

// Check if type is coral
bool isCoralType(AquaticPlantType type);

// Check if type is surface plant
bool isSurfacePlant(AquaticPlantType type);

// Check if type is alien
bool isAlienAquaticPlant(AquaticPlantType type);

class AquaticPlantSystem {
public:
    AquaticPlantSystem();
    ~AquaticPlantSystem();

    // Initialize with device and terrain reference
    void initialize(DX12Device* device, const Terrain* terrain);

    // Set system references
    void setClimateSystem(ClimateSystem* climate) { climateSystem = climate; }
    void setSeasonManager(SeasonManager* season) { seasonManager = season; }
    void setWeatherSystem(WeatherSystem* weather) { weatherSystem = weather; }

    // Generate aquatic plants for terrain
    void generate(unsigned int seed);

    // Update animation and simulation
    void update(float deltaTime, const glm::vec3& cameraPos);

    // Render aquatic plants
    void render(ID3D12GraphicsCommandList* commandList);

    // ===== Kelp Forest Management =====

    // Get all kelp forests
    const std::vector<KelpForest>& getKelpForests() const { return kelpForests; }

    // Find kelp forest at position
    KelpForest* findKelpForestAt(const glm::vec3& position, float radius);

    // Get kelp density at position
    float getKelpDensity(const glm::vec3& position, float radius) const;

    // ===== Coral Reef Management =====

    // Get all coral reefs
    const std::vector<CoralReef>& getCoralReefs() const { return coralReefs; }

    // Find coral reef at position
    CoralReef* findCoralReefAt(const glm::vec3& position, float radius);

    // Get coral health at position
    float getCoralHealth(const glm::vec3& position, float radius) const;

    // Apply temperature stress to corals
    void applyTemperatureStress(float temperature, const glm::vec3& position, float radius);

    // ===== Surface Plants =====

    // Get lily pad clusters
    const std::vector<LilyPadCluster>& getLilyPadClusters() const { return lilyPadClusters; }

    // Check if surface is covered by plants
    bool hasSurfaceCoverage(const glm::vec3& position, float radius) const;

    // ===== Ecosystem Functions =====

    // Get oxygen production at location
    float getOxygenProduction(const glm::vec3& position, float radius) const;

    // Get shelter value for fish at location
    float getShelterValue(const glm::vec3& position, float radius) const;

    // Get food value for herbivores
    float getFoodValue(const glm::vec3& position, float radius) const;

    // Creature consumes plant
    float consumePlant(const glm::vec3& position, float amount);

    // ===== Statistics =====

    struct AquaticEcosystemStats {
        float totalKelpBiomass;
        float totalCoralHealth;
        int healthyCoralCount;
        int bleachedCoralCount;
        float totalOxygenProduction;
        int lilyPadCount;
        int totalPlantCount;
    };
    AquaticEcosystemStats getStats() const;

    // Get all bioluminescent plant positions
    std::vector<std::pair<glm::vec3, glm::vec3>> getBioluminescentPositions() const;

    // Get all instances for rendering
    const std::vector<AquaticPlantInstance>& getAllInstances() const { return allInstances; }

private:
    DX12Device* dx12Device = nullptr;
    const Terrain* terrain = nullptr;
    ClimateSystem* climateSystem = nullptr;
    SeasonManager* seasonManager = nullptr;
    WeatherSystem* weatherSystem = nullptr;

    // Plant collections
    std::vector<KelpForest> kelpForests;
    std::vector<CoralReef> coralReefs;
    std::vector<LilyPadCluster> lilyPadClusters;
    std::vector<AquaticPlantInstance> allInstances;

    // Simulation time
    float simulationTime = 0.0f;
    float currentWaterTemp = 20.0f;

    // Rendering
    size_t visibleInstanceCount = 0;
    float maxRenderDistance = 500.0f;

    // DX12 resources
    ComPtr<ID3D12Resource> instanceBuffer;
    ComPtr<ID3D12Resource> segmentBuffer;

    // Generation
    void generateKelpForests(unsigned int seed);
    void generateCoralReefs(unsigned int seed);
    void generateSurfacePlants(unsigned int seed);
    void generateEmergentPlants(unsigned int seed);

    // Update functions
    void updateKelpAnimation(float deltaTime);
    void updateCoralHealth(float deltaTime);
    void updateSurfacePlants(float deltaTime);
    void updateWaterCurrent(float deltaTime);

    // Helper functions
    bool isValidPlantLocation(float x, float z, WaterZone zone) const;
    float getWaterDepth(float x, float z) const;
    float getWaterTemperature(float x, float z) const;
    float getWaterSalinity(float x, float z) const;
    float getLightLevel(float x, float z, float depth) const;
    glm::vec3 getWaterCurrent(float x, float z) const;

    // Kelp segment generation
    void generateKelpSegments(AquaticPlantInstance& kelp, float waterDepth, unsigned int seed);
    void updateKelpSegments(AquaticPlantInstance& kelp, float deltaTime, const glm::vec3& current);

    // Coral bleaching
    void updateCoralBleaching(AquaticPlantInstance& coral, float temperature, float deltaTime);
    glm::vec3 getCoralBleachedColor(const glm::vec3& originalColor, float bleachAmount);

    // Buffer management
    void createBuffers();
    void updateInstanceBuffer();
};
