#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>
#include <queue>

// Forward declarations
class TreeGenerator;
class GrassSystem;
class AquaticPlantSystem;
class FungiSystem;
class AlienVegetationSystem;
class Terrain;

// ============================================================
// FRUIT AND SEED SYSTEM
// ============================================================

// Types of fruits that can be produced
enum class FruitType {
    // Tree fruits
    APPLE,
    PEAR,
    CHERRY,
    PLUM,
    PEACH,
    ORANGE,
    LEMON,
    MANGO,
    BANANA,
    COCONUT,
    FIG,
    DATE,
    OLIVE,
    AVOCADO,

    // Berries
    BERRY_RED,
    BERRY_BLUE,
    BERRY_BLACK,
    BERRY_PURPLE,
    BERRY_WHITE,
    BERRY_POISONOUS,

    // Nuts
    ACORN,
    WALNUT,
    CHESTNUT,
    PINE_NUT,
    HAZELNUT,

    // Seeds
    SEED_SMALL,
    SEED_MEDIUM,
    SEED_LARGE,
    SEED_WINGED,      // Maple-style
    SEED_FLUFFY,      // Dandelion-style
    SEED_STICKY,      // Attaches to fur
    SEED_FLOATING,    // Water dispersal

    // Alien fruits
    GLOW_FRUIT,
    CRYSTAL_FRUIT,
    ENERGY_POD,
    VOID_BERRY,
    PLASMA_SEED,
    PSYCHIC_NUT,

    // Special
    SPORE_CLUSTER,    // From fungi
    NECTAR_DROP,      // From flowers

    COUNT
};

// Nutritional value of fruits
struct FruitNutrition {
    float calories;
    float hydration;
    float protein;
    float carbohydrates;
    float vitamins;
    float toxicity;      // 0 = safe, 1 = deadly
    float psychoactive;  // Alien fruits may have effects
};

// Get nutrition for fruit type
FruitNutrition getFruitNutrition(FruitType type);

// Seed dispersal method
enum class SeedDispersalMethod {
    GRAVITY,         // Falls straight down
    WIND,            // Carried by wind
    WATER,           // Floats in water
    ANIMAL_EATEN,    // Eaten and pooped out
    ANIMAL_CARRIED,  // Sticks to fur/feathers
    EXPLOSIVE,       // Seed pod explodes
    CREATURE_CACHED, // Buried by creatures
    ALIEN_TELEPORT   // Alien seeds phase around
};

// Individual fruit instance
struct FruitInstance {
    glm::vec3 position;
    FruitType type;
    float size;
    float ripeness;      // 0 = unripe, 1 = ripe, >1 = overripe/rotting
    float age;

    // Visual
    glm::vec3 color;
    float glowIntensity;  // For alien fruits

    // State
    bool isOnTree;        // Still attached to parent
    bool isOnGround;      // Fallen
    bool isInWater;       // Floating
    bool isBeingCarried;  // By creature
    int carrierCreatureId;

    // Physics
    glm::vec3 velocity;
    float bounceCount;

    // Source tracking
    int sourceTreeId;
    glm::vec3 sourcePosition;

    // Seed info
    bool hasSeed;
    SeedDispersalMethod dispersalMethod;
    float germinationChance;
};

// ============================================================
// SHELTER SYSTEM
// ============================================================

// Types of shelter plants can provide
enum class ShelterType {
    NONE,
    MINIMAL,         // Grass, small plants
    PARTIAL,         // Bushes, low trees
    FULL,            // Dense trees, caves
    UNDERGROUND,     // Burrows near roots
    CANOPY,          // Tree canopy cover
    AQUATIC,         // Underwater kelp/coral
    HOLLOW,          // Tree hollows
    NEST_SITE        // Good for building nests
};

// Shelter quality assessment
struct ShelterQuality {
    ShelterType type;
    float coveragePercent;   // 0-1 how much cover
    float concealment;       // 0-1 how hidden
    float weatherProtection; // 0-1 rain/wind block
    float predatorSafety;    // 0-1 safe from predators
    float comfortLevel;      // 0-1 how comfortable
    float capacity;          // How many creatures can fit
    glm::vec3 center;
    float radius;
};

// Shelter zone in the world
struct ShelterZone {
    glm::vec3 position;
    float radius;
    ShelterQuality quality;

    // Occupancy
    std::vector<int> occupantCreatureIds;
    int maxOccupants;

    // Source
    enum class SourceType {
        TREE,
        BUSH,
        GRASS_PATCH,
        KELP_FOREST,
        CORAL_REEF,
        FUNGI_CLUSTER,
        ALIEN_COLONY
    } sourceType;
    int sourceId;
};

// ============================================================
// POLLINATION SYSTEM
// ============================================================

// Pollinator types
enum class PollinatorType {
    BEE,
    BUTTERFLY,
    MOTH,
    HUMMINGBIRD,
    BAT,
    FLY,
    BEETLE,
    WASP,
    ANT,
    WIND,            // Not creature
    ALIEN_CREATURE,
    PSYCHIC_LINK     // Alien pollination
};

// Pollen packet carried by creature
struct PollenPacket {
    int sourceFlowerId;
    glm::vec3 sourcePosition;
    int flowerSpeciesId;     // What flower species
    float viability;         // 0-1, decreases over time
    float amount;
    float collectionTime;
};

// Pollination event record
struct PollinationEvent {
    int sourceFlowerId;
    int targetFlowerId;
    int pollinatorCreatureId;
    PollinatorType pollinatorType;
    float timestamp;
    bool successful;
    glm::vec3 position;
};

// Nectar source for pollinators
struct NectarSource {
    glm::vec3 position;
    int flowerId;
    float nectarAmount;      // Current nectar
    float maxNectar;
    float nectarRefillRate;
    float sugarContent;      // Quality
    glm::vec3 flowerColor;   // Visual attraction
    float scentStrength;     // Olfactory attraction
    bool isAlien;
};

// ============================================================
// SEED DISPERSAL SYSTEM
// ============================================================

// Seed being dispersed
struct DispersingSeed {
    glm::vec3 position;
    glm::vec3 velocity;
    FruitType fruitType;
    SeedDispersalMethod method;

    // Source
    glm::vec3 originPosition;
    int originPlantId;

    // State
    float age;
    float viability;
    bool isAttachedToCreature;
    int carrierCreatureId;
    glm::vec3 attachmentPoint;

    // Wind dispersal
    float windResistance;
    float liftCoefficient;

    // Water dispersal
    float buoyancy;

    // Germination
    float germinationChance;
    float dormancyTimer;     // Some seeds need time
    bool requiresColdPeriod; // Stratification
    bool hasExperiencedCold;
};

// Seed cache (buried by creatures)
struct SeedCache {
    glm::vec3 position;
    std::vector<FruitType> seeds;
    int creatorCreatureId;
    float burialTime;
    float depth;
    bool isRetrieved;
    bool hasSprouted;
};

// ============================================================
// PLANT EFFECTS ON CREATURES
// ============================================================

// Effect a plant has on nearby creatures
enum class PlantEffectType {
    NONE,
    HEALING,         // Medicinal plants
    POISON,          // Toxic plants
    ENERGY_BOOST,    // Stimulant
    SEDATIVE,        // Calming
    HALLUCINOGEN,    // Alien plants
    PHEROMONE,       // Attracts/repels
    CAMOUFLAGE,      // Helps hide
    NUTRITION,       // Food source
    PARASITIC,       // Drains energy
    SYMBIOTIC,       // Mutual benefit
    MIND_CONTROL,    // Alien psychic plants
    MUTATION         // Causes genetic changes
};

// Active plant effect on a creature
struct PlantEffect {
    PlantEffectType type;
    float strength;
    float duration;
    float elapsed;
    int sourcePlantId;
    glm::vec3 sourcePosition;
};

// ============================================================
// CREATURE CALLBACKS
// ============================================================

// Callback types for creature behavior
using FruitFoundCallback = std::function<void(int creatureId, const FruitInstance& fruit)>;
using ShelterFoundCallback = std::function<void(int creatureId, const ShelterZone& shelter)>;
using NectarFoundCallback = std::function<void(int creatureId, const NectarSource& nectar)>;
using PlantEffectCallback = std::function<void(int creatureId, const PlantEffect& effect)>;
using SeedAttachCallback = std::function<void(int creatureId, const DispersingSeed& seed)>;

// ============================================================
// MAIN INTERACTION SYSTEM
// ============================================================

class PlantCreatureInteraction {
public:
    PlantCreatureInteraction();
    ~PlantCreatureInteraction();

    // Initialize with all plant systems
    void initialize(
        TreeGenerator* trees,
        GrassSystem* grass,
        AquaticPlantSystem* aquatic,
        FungiSystem* fungi,
        AlienVegetationSystem* alien,
        const Terrain* terrain
    );

    // Update simulation
    void update(float deltaTime);

    // ===== FRUIT SYSTEM =====

    // Spawn fruit from a tree
    void spawnFruit(const glm::vec3& position, FruitType type, int sourceTreeId);

    // Drop fruit from tree (ripened or knocked)
    void dropFruit(int fruitId);

    // Creature eats fruit
    FruitNutrition eatFruit(int fruitId, int creatureId);

    // Get all fruits in radius
    std::vector<FruitInstance*> getFruitsInRadius(const glm::vec3& position, float radius);

    // Get ripe fruits only
    std::vector<FruitInstance*> getRipeFruitsInRadius(const glm::vec3& position, float radius);

    // Find nearest fruit of type
    FruitInstance* findNearestFruit(const glm::vec3& position, FruitType type, float maxDistance);

    // Get all fruits
    const std::vector<FruitInstance>& getAllFruits() const { return fruits; }

    // ===== SHELTER SYSTEM =====

    // Find shelter near position
    ShelterZone* findShelter(const glm::vec3& position, float searchRadius, float minQuality = 0.3f);

    // Find best shelter in area
    ShelterZone* findBestShelter(const glm::vec3& position, float searchRadius);

    // Creature enters shelter
    bool enterShelter(int shelterIndex, int creatureId);

    // Creature leaves shelter
    void leaveShelter(int shelterIndex, int creatureId);

    // Check if position is sheltered
    ShelterQuality getShelterQuality(const glm::vec3& position, float radius) const;

    // Get all shelter zones
    const std::vector<ShelterZone>& getShelterZones() const { return shelterZones; }

    // ===== POLLINATION SYSTEM =====

    // Creature visits flower (collects pollen/nectar)
    void creatureVisitsFlower(int creatureId, int flowerId, PollinatorType type);

    // Check if creature can pollinate
    bool canCreaturePollinate(int creatureId, int targetFlowerId) const;

    // Get pollen carried by creature
    std::vector<PollenPacket>* getCreaturePollen(int creatureId);

    // Find nectar sources in radius
    std::vector<NectarSource> findNectarSources(const glm::vec3& position, float radius) const;

    // Get best nectar source nearby
    NectarSource* findBestNectarSource(const glm::vec3& position, float maxDistance);

    // Consume nectar
    float consumeNectar(int nectarSourceIndex, float amount, int creatureId);

    // Get pollination history
    const std::vector<PollinationEvent>& getPollinationHistory() const { return pollinationHistory; }

    // ===== SEED DISPERSAL =====

    // Release seed from fruit
    void releaseSeed(const FruitInstance& fruit, const glm::vec3& position);

    // Attach seed to creature (sticky seeds)
    void attachSeedToCreature(int seedIndex, int creatureId, const glm::vec3& attachPoint);

    // Detach seed from creature
    void detachSeedFromCreature(int seedIndex);

    // Creature caches seed (buries it)
    void cacheSeed(int creatureId, FruitType seedType, const glm::vec3& position);

    // Creature retrieves cached seeds
    std::vector<FruitType> retrieveCache(int cacheIndex, int creatureId);

    // Get seeds attached to creature
    std::vector<DispersingSeed*> getSeedsOnCreature(int creatureId);

    // Check for seed germination
    void checkGermination(float deltaTime);

    // Get dispersing seeds
    const std::vector<DispersingSeed>& getDispersingSeed() const { return dispersingSeeds; }

    // Get seed caches
    const std::vector<SeedCache>& getSeedCaches() const { return seedCaches; }

    // ===== PLANT EFFECTS =====

    // Get plant effects at position
    std::vector<PlantEffect> getPlantEffectsAt(const glm::vec3& position, float radius) const;

    // Apply plant effects to creature
    void applyPlantEffects(int creatureId, const glm::vec3& position, float deltaTime);

    // Check for dangerous plants
    bool isDangerousPlantNearby(const glm::vec3& position, float radius) const;

    // Get danger level at position
    float getPlantDangerLevel(const glm::vec3& position, float radius) const;

    // ===== CALLBACKS =====

    void setFruitFoundCallback(FruitFoundCallback callback) { onFruitFound = callback; }
    void setShelterFoundCallback(ShelterFoundCallback callback) { onShelterFound = callback; }
    void setNectarFoundCallback(NectarFoundCallback callback) { onNectarFound = callback; }
    void setPlantEffectCallback(PlantEffectCallback callback) { onPlantEffect = callback; }
    void setSeedAttachCallback(SeedAttachCallback callback) { onSeedAttach = callback; }

    // ===== CREATURE QUERIES =====

    // Creature scans for food (fruits, nectar)
    struct FoodScanResult {
        std::vector<FruitInstance*> fruits;
        std::vector<NectarSource> nectarSources;
        float closestFoodDistance;
        glm::vec3 closestFoodPosition;
    };
    FoodScanResult scanForFood(const glm::vec3& position, float radius) const;

    // Creature scans for shelter
    struct ShelterScanResult {
        std::vector<ShelterZone*> shelters;
        ShelterZone* bestShelter;
        float closestShelterDistance;
    };
    ShelterScanResult scanForShelter(const glm::vec3& position, float radius);

    // Creature scans for danger
    struct DangerScanResult {
        float overallDangerLevel;
        std::vector<glm::vec3> dangerousPlantPositions;
        std::vector<PlantEffectType> activeThreats;
    };
    DangerScanResult scanForDanger(const glm::vec3& position, float radius) const;

    // ===== STATISTICS =====

    struct InteractionStats {
        int totalFruits;
        int ripeFruits;
        int fallenFruits;
        int fruitsEaten;
        int shelterZoneCount;
        int occupiedShelters;
        int pollinationEvents;
        int successfulPollinations;
        int dispersingSeeds;
        int seedCaches;
        int germinatedSeeds;
    };
    InteractionStats getStats() const;

private:
    // Plant system references
    TreeGenerator* treeGenerator = nullptr;
    GrassSystem* grassSystem = nullptr;
    AquaticPlantSystem* aquaticSystem = nullptr;
    FungiSystem* fungiSystem = nullptr;
    AlienVegetationSystem* alienSystem = nullptr;
    const Terrain* terrain = nullptr;

    // Fruit system
    std::vector<FruitInstance> fruits;
    int nextFruitId = 0;
    float fruitSpawnTimer = 0.0f;
    float fruitSpawnInterval = 5.0f;

    // Shelter system
    std::vector<ShelterZone> shelterZones;
    float shelterUpdateTimer = 0.0f;
    float shelterUpdateInterval = 10.0f;

    // Pollination system
    std::unordered_map<int, std::vector<PollenPacket>> creaturePollenCarried;
    std::vector<NectarSource> nectarSources;
    std::vector<PollinationEvent> pollinationHistory;
    float pollinationCheckTimer = 0.0f;

    // Seed dispersal
    std::vector<DispersingSeed> dispersingSeeds;
    std::vector<SeedCache> seedCaches;
    float seedUpdateTimer = 0.0f;

    // Statistics
    int totalFruitsEaten = 0;
    int totalSuccessfulPollinations = 0;
    int totalGerminatedSeeds = 0;

    // Callbacks
    FruitFoundCallback onFruitFound;
    ShelterFoundCallback onShelterFound;
    NectarFoundCallback onNectarFound;
    PlantEffectCallback onPlantEffect;
    SeedAttachCallback onSeedAttach;

    // Update functions
    void updateFruits(float deltaTime);
    void updateShelterZones(float deltaTime);
    void updatePollination(float deltaTime);
    void updateSeedDispersal(float deltaTime);
    void updateNectarSources(float deltaTime);

    // Generation functions
    void generateShelterZones();
    void generateNectarSources();
    void spawnTreeFruits();

    // Helper functions
    FruitType getTreeFruitType(int treeType) const;
    float calculateShelterQuality(const glm::vec3& position, float radius) const;
    glm::vec3 calculateWindDispersal(const DispersingSeed& seed, float deltaTime) const;
    bool checkGerminationConditions(const DispersingSeed& seed, const glm::vec3& position) const;
    void spawnNewPlant(FruitType seedType, const glm::vec3& position);
};

// ============================================================
// VEGETATION MANAGER - COORDINATES ALL PLANT SYSTEMS
// ============================================================

class VegetationManager {
public:
    VegetationManager();
    ~VegetationManager();

    // Initialize all vegetation systems
    void initialize(class DX12Device* device, const Terrain* terrain);

    // Update all systems
    void update(float deltaTime, const glm::vec3& cameraPos);

    // Render all vegetation
    void render(ID3D12GraphicsCommandList* commandList);

    // Access individual systems
    TreeGenerator* getTrees() { return treeGenerator.get(); }
    GrassSystem* getGrass() { return grassSystem.get(); }
    AquaticPlantSystem* getAquatic() { return aquaticSystem.get(); }
    FungiSystem* getFungi() { return fungiSystem.get(); }
    AlienVegetationSystem* getAlien() { return alienSystem.get(); }
    PlantCreatureInteraction* getInteraction() { return interaction.get(); }

    // Global queries
    float getVegetationDensity(const glm::vec3& position, float radius) const;
    float getFoodAvailability(const glm::vec3& position, float radius) const;
    float getShelterAvailability(const glm::vec3& position, float radius) const;

    // Biome info
    glm::vec3 getBiomeAmbientColor(const glm::vec3& position) const;
    float getBiomeAlienness(const glm::vec3& position) const;

    // Statistics
    struct VegetationStats {
        int totalTrees;
        int totalGrassBlades;
        int totalFlowers;
        int totalAquaticPlants;
        int totalFungi;
        int totalAlienPlants;
        float totalBiomass;
        float averageHealth;
    };
    VegetationStats getStats() const;

private:
    std::unique_ptr<TreeGenerator> treeGenerator;
    std::unique_ptr<GrassSystem> grassSystem;
    std::unique_ptr<AquaticPlantSystem> aquaticSystem;
    std::unique_ptr<FungiSystem> fungiSystem;
    std::unique_ptr<AlienVegetationSystem> alienSystem;
    std::unique_ptr<PlantCreatureInteraction> interaction;

    const Terrain* terrain = nullptr;
};
