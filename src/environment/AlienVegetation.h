#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include <memory>

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

// Types of alien vegetation
enum class AlienPlantType {
    // Bioluminescent plants
    GLOW_TENDRIL,        // Softly glowing tendrils
    LIGHT_BULB_PLANT,    // Bulb-shaped glowing plants
    AURORA_FERN,         // Shifting color ferns
    NEON_MOSS,           // Brightly glowing ground moss
    PHOTON_FLOWER,       // Intense light-emitting flowers
    STARLIGHT_TREE,      // Trees with glowing leaves

    // Crystal-based plants
    CRYSTAL_SPIRE,       // Tall crystalline spires
    GEM_FLOWER,          // Crystalline flower formations
    PRISM_BUSH,          // Refractive crystal bushes
    QUARTZ_VINE,         // Crystal-encrusted vines
    DIAMOND_GRASS,       // Grass-like crystal blades

    // Energy-based plants
    PLASMA_TREE,         // Trees with plasma energy flows
    VOID_BLOSSOM,        // Dark energy flowers
    LIGHTNING_FERN,      // Electric discharge plants
    SOLAR_COLLECTOR,     // Energy-harvesting plants
    QUANTUM_MOSS,        // Phasing moss patches

    // Organic alien plants
    TENDRIL_TREE,        // Trees with reaching tendrils
    SPORE_TOWER,         // Tall spore-releasing towers
    FLESH_FLOWER,        // Organic pulsing flowers
    MEMBRANE_BUSH,       // Translucent membrane plants
    EYE_STALK,           // Plants with eye-like structures
    TENTACLE_GRASS,      // Writhing grass-like tendrils

    // Floating/antigravity plants
    FLOAT_POD,           // Floating seed pods
    HOVER_BLOOM,         // Flowers that hover
    DRIFT_SPORE,         // Floating spore clouds
    LEVITATION_TREE,     // Trees with floating elements

    // Sound/vibration plants
    SONIC_CHIME,         // Plants that emit sounds
    RESONANCE_CRYSTAL,   // Vibrating crystal plants
    HARMONIC_FLOWER,     // Musical flowering plants

    // Reactive/interactive plants
    TOUCH_SENSITIVE,     // React to proximity
    PREDATOR_PLANT,      // Carnivorous alien plant
    SYMBIOTE_VINE,       // Attaches to other organisms
    MIMIC_PLANT,         // Changes appearance

    // Extreme environment plants
    THERMAL_VENT_PLANT,  // Thrives on heat
    ICE_CRYSTAL_PLANT,   // Thrives in extreme cold
    RADIATION_FEEDER,    // Absorbs radiation
    ACID_BLOOM,          // Grows in toxic conditions

    COUNT
};

// Alien plant behavior state
enum class AlienBehaviorState {
    DORMANT,             // Inactive/sleeping
    IDLE,                // Normal passive state
    ACTIVE,              // Actively doing something
    HUNTING,             // Predatory behavior
    REPRODUCING,         // Spawning offspring
    DEFENDING,           // Defensive mode
    COMMUNICATING,       // Signaling other plants
    FEEDING,             // Consuming energy/matter
    MIGRATING            // Moving (for mobile plants)
};

// Energy source for alien plants
enum class AlienEnergySource {
    PHOTOSYNTHESIS,      // Normal light
    BIOLUMINESCENCE,     // Internal light
    THERMAL,             // Heat energy
    KINETIC,             // Movement/vibration
    RADIATION,           // Background radiation
    CHEMICAL,            // Chemical reactions
    ELECTROMAGNETIC,     // EM fields
    DARK_ENERGY,         // Unknown energy
    PSYCHIC              // Mental energy from creatures
};

// Single alien plant instance
struct AlienPlantInstance {
    glm::vec3 position;
    float rotation;
    float scale;
    AlienPlantType type;

    // Visual properties
    glm::vec3 primaryColor;
    glm::vec3 secondaryColor;
    glm::vec3 glowColor;
    float glowIntensity;
    float glowPhase;         // Animation phase for glow

    // Animation
    float animationPhase;
    float animationSpeed;
    glm::vec3 movementOffset;    // For floating/moving plants

    // Behavior
    AlienBehaviorState behaviorState;
    float behaviorTimer;
    glm::vec3 targetPosition;    // For reactive plants

    // Stats
    float health;
    float energy;
    float age;
    float growthStage;       // 0-1 maturity

    // Interaction
    float dangerRadius;      // How close creatures should avoid
    float attractionRadius;  // How close creatures are drawn
    float psychicRange;      // Mental influence range
    bool isHostile;
    bool isPredatory;

    // Special properties
    bool isFloating;
    float floatHeight;
    bool emitsSound;
    float soundFrequency;
    bool hasSpores;
    float sporeTimer;

    // Bioluminescence pattern
    int glowPattern;         // Pattern type
    float glowCycleSpeed;
    std::vector<float> glowKeyframes;

    // Crystal properties (for crystal types)
    int facetCount;
    float refractiveIndex;
    float crystalClarity;

    // Tendril properties (for tendril types)
    int tendrilCount;
    std::vector<glm::vec3> tendrilPositions;
    std::vector<float> tendrilLengths;
};

// Alien plant colony (group behavior)
struct AlienPlantColony {
    glm::vec3 center;
    float radius;
    AlienPlantType dominantType;
    std::vector<AlienPlantInstance> plants;

    // Colony behavior
    float colonyHealth;
    float colonyEnergy;
    bool isHiveMind;         // Plants share consciousness
    float synchronization;   // How in-sync the plants are

    // Environmental modification
    float areaGlowIntensity;
    float areaDanger;
    float areaWeirdness;     // How alien the area feels
};

// Alien plant species configuration
struct AlienPlantConfig {
    AlienPlantType type;
    std::string name;

    // Physical
    float minHeight;
    float maxHeight;
    float minScale;
    float maxScale;

    // Colors
    glm::vec3 basePrimaryColor;
    glm::vec3 baseSecondaryColor;
    glm::vec3 baseGlowColor;
    float colorVariation;

    // Glow
    bool glows;
    float baseGlowIntensity;
    float glowPulseSpeed;
    int defaultGlowPattern;

    // Behavior
    AlienEnergySource energySource;
    bool isHostile;
    bool isPredatory;
    float dangerLevel;       // 0-1
    float aggressiveness;

    // Movement
    bool canFloat;
    bool canMove;
    float movementSpeed;

    // Special abilities
    bool emitsSpores;
    float sporeInterval;
    bool emitsSound;
    float soundRange;
    bool hasTendrils;
    int baseTendrilCount;

    // Crystal properties
    bool isCrystalline;
    int minFacets;
    int maxFacets;

    // Environment
    float preferredTemperature;
    float temperatureTolerance;
    bool prefersRadiation;
    bool prefersDarkness;
};

// Get configuration for alien plant type
AlienPlantConfig getAlienPlantConfig(AlienPlantType type);

// Check if plant type is dangerous
bool isAlienPlantDangerous(AlienPlantType type);

// Check if plant type glows
bool doesAlienPlantGlow(AlienPlantType type);

// Check if plant type is crystalline
bool isAlienPlantCrystalline(AlienPlantType type);

class AlienVegetationSystem {
public:
    AlienVegetationSystem();
    ~AlienVegetationSystem();

    // Initialize
    void initialize(DX12Device* device, const Terrain* terrain);

    // System references
    void setClimateSystem(ClimateSystem* climate) { climateSystem = climate; }
    void setSeasonManager(SeasonManager* season) { seasonManager = season; }
    void setWeatherSystem(WeatherSystem* weather) { weatherSystem = weather; }

    // Generation
    void generate(unsigned int seed);

    // Update
    void update(float deltaTime, const glm::vec3& cameraPos);

    // Render
    void render(ID3D12GraphicsCommandList* commandList);

    // ===== Colony Management =====

    const std::vector<AlienPlantColony>& getColonies() const { return colonies; }

    // Find colony at position
    AlienPlantColony* findColonyAt(const glm::vec3& position, float radius);

    // Get alienness level at position (0-1)
    float getAliennessLevel(const glm::vec3& position, float radius) const;

    // ===== Creature Interaction =====

    // Check if position is dangerous
    bool isDangerousArea(const glm::vec3& position, float radius) const;

    // Get danger level at position
    float getDangerLevel(const glm::vec3& position, float radius) const;

    // Get attraction level (for curious creatures)
    float getAttractionLevel(const glm::vec3& position, float radius) const;

    // Creature enters plant's detection range
    void onCreatureNearby(const glm::vec3& creaturePos, float creatureSize);

    // Plant attacks creature (returns damage)
    float plantAttack(const glm::vec3& position, float radius);

    // ===== Visual Effects =====

    // Get all glowing plant positions and colors
    std::vector<std::pair<glm::vec3, glm::vec3>> getGlowingPlantPositions() const;

    // Get total glow intensity at position
    float getGlowIntensity(const glm::vec3& position, float radius) const;

    // Get ambient alien light color
    glm::vec3 getAmbientAlienColor(const glm::vec3& position, float radius) const;

    // ===== Sound Effects =====

    // Get sound-emitting plant positions
    std::vector<std::pair<glm::vec3, float>> getSoundEmitters() const;

    // ===== Statistics =====

    struct AlienVegetationStats {
        int totalPlants;
        int glowingPlants;
        int dangerousPlants;
        int predatoryPlants;
        float totalGlowOutput;
        float averageDangerLevel;
        int activeColonies;
    };
    AlienVegetationStats getStats() const;

    // Get all instances
    const std::vector<AlienPlantInstance>& getAllInstances() const { return allInstances; }

private:
    DX12Device* dx12Device = nullptr;
    const Terrain* terrain = nullptr;
    ClimateSystem* climateSystem = nullptr;
    SeasonManager* seasonManager = nullptr;
    WeatherSystem* weatherSystem = nullptr;

    // Plant collections
    std::vector<AlienPlantColony> colonies;
    std::vector<AlienPlantInstance> allInstances;

    // Simulation
    float simulationTime = 0.0f;
    float dayNightCycle = 0.0f;

    // Rendering
    size_t visibleInstanceCount = 0;
    float maxRenderDistance = 500.0f;

    // DX12 resources
    ComPtr<ID3D12Resource> instanceBuffer;

    // Generation
    void generateBioluminescentZone(const glm::vec3& center, float radius, unsigned int seed);
    void generateCrystalFormation(const glm::vec3& center, float radius, unsigned int seed);
    void generateTendrilForest(const glm::vec3& center, float radius, unsigned int seed);
    void generateFloatingGarden(const glm::vec3& center, float radius, unsigned int seed);
    void generatePredatorPatch(const glm::vec3& center, float radius, unsigned int seed);

    // Update functions
    void updateGlowAnimations(float deltaTime);
    void updatePlantBehaviors(float deltaTime);
    void updateFloatingPlants(float deltaTime);
    void updateTendrils(float deltaTime);
    void updatePredatoryPlants(float deltaTime);
    void updateSporeRelease(float deltaTime);
    void updateColonyBehavior(float deltaTime);

    // Helper functions
    bool isValidAlienPlantLocation(float x, float z) const;
    float getRadiationLevel(float x, float z) const;
    glm::vec3 generateGlowColor(AlienPlantType type, unsigned int seed) const;
    void initializeTendrils(AlienPlantInstance& plant, unsigned int seed);
    void updateTendrilPositions(AlienPlantInstance& plant, float deltaTime);

    // Glow pattern functions
    float calculateGlowValue(const AlienPlantInstance& plant, float time) const;
    glm::vec3 calculateGlowColorAnimated(const AlienPlantInstance& plant, float time) const;

    // Buffer management
    void createBuffers();
    void updateInstanceBuffer();
};
