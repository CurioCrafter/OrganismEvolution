#pragma once

#include "../entities/CreatureType.h"  // For shared FoodSourceType enum
#include <glm/glm.hpp>
#include <vector>
#include <memory>

class Terrain;
class SeasonManager;

// A consumable food patch with growth dynamics
struct FoodPatch {
    glm::vec3 position;
    FoodSourceType type;
    
    float currentBiomass;    // Current available food (0-maxBiomass)
    float maxBiomass;        // Maximum food this patch can hold
    float regrowthRate;      // Biomass units per second at full nutrients
    float energyPerUnit;     // Energy gained per unit consumed
    
    float lastConsumedTime;  // For tracking consumption rate
    float consumptionPressure; // How heavily this patch is being grazed
    
    // Nutrient requirements
    float soilNitrogen;      // Local nitrogen level affecting growth
    float soilMoisture;      // Local moisture level
    
    bool isAvailable() const { return currentBiomass > 0.1f; }
    float getEffectiveEnergy() const { return currentBiomass * energyPerUnit; }
};

// Soil tile for nutrient tracking
struct SoilTile {
    float nitrogen;      // 0-100, affects plant growth rate
    float phosphorus;    // 0-100, affects plant reproduction/density
    float organicMatter; // 0-100, from decomposition
    float moisture;      // 0-100, seasonal + terrain based
    float detritus;      // 0-100, dead organic matter (leaves, dead roots) - feeds decomposers

    SoilTile() : nitrogen(50.0f), phosphorus(50.0f), organicMatter(30.0f), moisture(50.0f), detritus(20.0f) {}

    float getGrowthMultiplier() const {
        // Growth depends on nitrogen and moisture
        float nitrogenFactor = nitrogen / 100.0f;
        float moistureFactor = moisture / 100.0f;
        return 0.5f + 0.5f * nitrogenFactor * moistureFactor;
    }

    // Detritus boosts growth when decomposed
    float getDetritusBonusMultiplier() const {
        return 1.0f + (detritus / 100.0f) * 0.3f;  // Up to 30% bonus from detritus
    }
};

class ProducerSystem {
public:
    ProducerSystem(Terrain* terrain, int gridResolution = 50);
    
    void init(unsigned int seed);
    void update(float deltaTime, const SeasonManager* seasonMgr);
    
    // Food consumption by creatures
    float consumeAt(const glm::vec3& position, FoodSourceType preferredType, float amount);
    
    // Get available food positions for creatures
    std::vector<glm::vec3> getGrassPositions() const;
    std::vector<glm::vec3> getBushPositions() const;
    std::vector<glm::vec3> getTreeFruitPositions() const;
    std::vector<glm::vec3> getTreeLeafPositions() const;
    std::vector<glm::vec3> getAllFoodPositions() const;

    // Aquatic food positions
    std::vector<glm::vec3> getPlanktonPositions() const;
    std::vector<glm::vec3> getAlgaePositions() const;
    std::vector<glm::vec3> getSeaweedPositions() const;
    std::vector<glm::vec3> getAllAquaticFoodPositions() const;
    
    // Nutrient cycling interface
    void addNutrients(const glm::vec3& position, float nitrogen, float phosphorus, float organicMatter);
    void addDetritus(const glm::vec3& position, float amount);  // Add dead organic matter
    SoilTile& getSoilAt(const glm::vec3& position);
    const SoilTile& getSoilAt(const glm::vec3& position) const;

    // Detritus system - dead organic matter for decomposers and scavengers
    float getDetritusAt(const glm::vec3& position, float radius) const;
    float consumeDetritus(const glm::vec3& position, float amount);  // Decomposers consume detritus
    std::vector<glm::vec3> getDetritusHotspots() const;  // Positions with high detritus for scavengers

    // Seasonal bloom system
    float getSeasonalBloomMultiplier() const { return currentBloomMultiplier; }
    bool isInBloomPeriod() const { return currentBloomMultiplier > 1.2f; }
    int getBloomType() const { return activeBloomType; }  // 0=none, 1=spring, 2=fungal, 3=plankton
    
    // Statistics
    float getTotalBiomass() const;
    float getGrassBiomass() const;
    float getBushBiomass() const;
    float getTreeBiomass() const;
    int getActivePatches() const;
    
    // For rendering
    const std::vector<FoodPatch>& getGrassPatches() const { return grassPatches; }
    const std::vector<FoodPatch>& getBushPatches() const { return bushPatches; }
    const std::vector<FoodPatch>& getTreePatches() const { return treePatches; }
    const std::vector<FoodPatch>& getPlanktonPatches() const { return planktonPatches; }
    const std::vector<FoodPatch>& getAlgaePatches() const { return algaePatches; }
    const std::vector<FoodPatch>& getSeaweedPatches() const { return seaweedPatches; }

private:
    Terrain* terrain;
    int gridResolution;
    
    std::vector<FoodPatch> grassPatches;
    std::vector<FoodPatch> bushPatches;
    std::vector<FoodPatch> treePatches;

    // Aquatic food patches
    std::vector<FoodPatch> planktonPatches;  // Floating in water column
    std::vector<FoodPatch> algaePatches;     // On sea floor/rocks
    std::vector<FoodPatch> seaweedPatches;   // Larger underwater plants
    
    // Soil nutrient grid
    std::vector<std::vector<SoilTile>> soilGrid;
    float soilTileSize;
    
    void generateGrassPatches(unsigned int seed);
    void generateBushPatches(unsigned int seed);
    void linkTreePatches();
    void generateAquaticPatches(unsigned int seed);  // Generate plankton, algae, seaweed
    
    void updateGrowth(float deltaTime, float seasonMultiplier);
    void updateSoilNutrients(float deltaTime);
    void updateDetritus(float deltaTime);  // Detritus decay and conversion
    void updateSeasonalBlooms(float deltaTime, const SeasonManager* seasonMgr);

    // Seasonal bloom state
    float currentBloomMultiplier = 1.0f;
    int activeBloomType = 0;  // 0=none, 1=spring growth, 2=fungal burst, 3=plankton bloom
    float bloomTimer = 0.0f;

    // Convert world position to soil grid index
    std::pair<int, int> worldToSoilIndex(float x, float z) const;

    // Find nearest food patch of type within range
    FoodPatch* findNearestPatch(const glm::vec3& pos, FoodSourceType type, float range);
};
