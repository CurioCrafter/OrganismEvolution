#pragma once

#include "TreeGenerator.h"
#include "Terrain.h"
#include "ClimateSystem.h"
#include "BiomeSystem.h"
#include "AquaticPlants.h"
#include "../graphics/mesh/MeshData.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <vector>
#include <memory>
#include <unordered_map>

struct TreeInstance {
    glm::vec3 position;
    glm::vec3 scale;
    float rotation;
    TreeType type;
};

struct BushInstance {
    glm::vec3 position;
    glm::vec3 scale;
    float rotation;
};

struct GrassCluster {
    glm::vec3 position;
    float density;
    glm::vec3 color;  // Biome-specific grass color
};

// Vegetation profile for each biome type
struct VegetationProfile {
    std::vector<TreeType> trees;
    std::vector<float> treeWeights;  // Probability weights (must sum to 1.0)
    float treeDensity;               // 0-1, probability of placing a tree
    float grassDensity;              // 0-1
    float bushDensity;               // 0-1
    glm::vec3 grassColor;
    glm::vec3 flowerColor;
    bool hasFlowers;

    // Blend two profiles together
    static VegetationProfile blend(const VegetationProfile& a, const VegetationProfile& b, float t);
};

// Get vegetation profile for a specific climate biome
VegetationProfile getVegetationProfileForBiome(ClimateBiome biome);

class VegetationManager {
public:
    VegetationManager(Terrain* terrain);

    // Set climate system for biome-aware vegetation
    void setClimateSystem(ClimateSystem* climate) { m_climate = climate; }

    // Generate vegetation based on terrain and biomes
    void generate(unsigned int seed);

    // Clear all vegetation (for regeneration)
    void clear();

    // Render all vegetation
    void render();

    // Get tree instances for rendering
    const std::vector<TreeInstance>& getTreeInstances() const { return trees; }

    // Get bush instances for rendering
    const std::vector<BushInstance>& getBushInstances() const { return bushes; }

    // Get grass positions
    const std::vector<GrassCluster>& getGrassClusters() const { return grass; }

    // Get mesh for specific tree type
    const MeshData* getMeshForType(TreeType type) const;

    // Get bush mesh
    const MeshData* getBushMesh() const { return bushMesh.get(); }

    // ===== Aquatic Plant System (Phase 8 - Ocean Ecosystem) =====

    // Get the aquatic plant system
    AquaticPlantSystem* getAquaticPlants() { return aquaticPlants.get(); }
    const AquaticPlantSystem* getAquaticPlants() const { return aquaticPlants.get(); }

    // Initialize aquatic plants (requires DX12 device for GPU buffers)
    void initializeAquaticPlants(DX12Device* dx12Device, unsigned int seed);

    // Update aquatic plants (for animation, growth, etc.)
    void updateAquaticPlants(float deltaTime, const glm::vec3& cameraPos);

    // Get aquatic ecosystem stats
    struct AquaticStats {
        int totalKelpForests = 0;
        int totalCoralReefs = 0;
        int totalAquaticPlants = 0;
        float totalOxygenProduction = 0.0f;
        float totalFoodValue = 0.0f;
        float averageCoralHealth = 0.0f;
    };
    AquaticStats getAquaticStats() const;

private:
    Terrain* terrain;
    ClimateSystem* m_climate = nullptr;
    std::vector<TreeInstance> trees;
    std::vector<BushInstance> bushes;
    std::vector<GrassCluster> grass;

    // Aquatic plant system (Phase 8 - Ocean Ecosystem)
    std::unique_ptr<AquaticPlantSystem> aquaticPlants;

    // Tree meshes (cached for rendering) - map for all tree types
    std::unordered_map<TreeType, std::unique_ptr<MeshData>> treeMeshes;
    std::unique_ptr<MeshData> bushMesh;

    // Legacy mesh pointers for backward compatibility
    std::unique_ptr<MeshData> oakMesh;
    std::unique_ptr<MeshData> pineMesh;
    std::unique_ptr<MeshData> willowMesh;

    // Generate all meshes for new tree types
    void generateTreeMeshes();

    // Generate tree placement based on terrain biome
    void generateTrees(unsigned int seed);

    // Generate bush placement
    void generateBushes(unsigned int seed);

    // Generate grass placement
    void generateGrass(unsigned int seed);

    // Biome-aware vegetation placement
    void placeVegetationAt(float x, float z, float height, const VegetationProfile& profile);

    // Select weighted random tree type from profile
    TreeType selectWeightedRandom(const std::vector<TreeType>& types,
                                   const std::vector<float>& weights) const;

    // Check if position is suitable for vegetation (legacy methods)
    bool isSuitableForTrees(float x, float z) const;
    bool isSuitableForBushes(float x, float z) const;
    bool isSuitableForGrass(float x, float z) const;

    // ===== Seed-based Variation System =====
    // Global density modifier from world seed
    float m_globalDensityModifier = 1.0f;
    uint32_t m_variationSeed = 0;

public:
    // Set vegetation density modifier from seed (affects all biomes proportionally)
    void setGlobalDensityModifier(float modifier) { m_globalDensityModifier = std::clamp(modifier, 0.1f, 2.0f); }
    float getGlobalDensityModifier() const { return m_globalDensityModifier; }

    // Set vegetation variation seed for deterministic per-run variety
    void setVariationSeed(uint32_t seed) { m_variationSeed = seed; }
    uint32_t getVariationSeed() const { return m_variationSeed; }
};
