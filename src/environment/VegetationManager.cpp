#include "VegetationManager.h"
#include "TerrainSampler.h"
#include "../utils/Random.h"
#include <cmath>
#include <algorithm>
#include <set>

namespace {
    float sampleHeightForVegetation(const Terrain* terrain, float x, float z) {
        if (terrain && terrain->isInBounds(x, z)) {
            return terrain->getHeight(x, z);
        }
        return TerrainSampler::SampleHeight(x, z);
    }

    bool isWaterForVegetation(const Terrain* terrain, float x, float z) {
        if (terrain && terrain->isInBounds(x, z)) {
            return terrain->isWater(x, z);
        }
        return TerrainSampler::IsWater(x, z);
    }

    bool isInWorldBounds(const Terrain* terrain, float x, float z) {
        if (terrain) {
            return terrain->isInBounds(x, z);
        }
        float halfWorld = TerrainSampler::WORLD_SIZE * 0.5f;
        return std::abs(x) <= halfWorld && std::abs(z) <= halfWorld;
    }
}

constexpr float TREE_SCALE_MULTIPLIER = 6.0f;
constexpr float BUSH_SCALE_MULTIPLIER = 3.0f;

// Vegetation profile blending
VegetationProfile VegetationProfile::blend(const VegetationProfile& a, const VegetationProfile& b, float t) {
    VegetationProfile result;

    // Blend densities
    result.treeDensity = a.treeDensity * (1.0f - t) + b.treeDensity * t;
    result.grassDensity = a.grassDensity * (1.0f - t) + b.grassDensity * t;
    result.bushDensity = a.bushDensity * (1.0f - t) + b.bushDensity * t;

    // Blend colors
    result.grassColor = a.grassColor * (1.0f - t) + b.grassColor * t;
    result.flowerColor = a.flowerColor * (1.0f - t) + b.flowerColor * t;
    result.hasFlowers = (t < 0.5f) ? a.hasFlowers : b.hasFlowers;

    // Combine tree types from both profiles
    result.trees = a.trees;
    for (const auto& tree : b.trees) {
        if (std::find(result.trees.begin(), result.trees.end(), tree) == result.trees.end()) {
            result.trees.push_back(tree);
        }
    }

    // Recalculate weights (simple averaging)
    result.treeWeights.resize(result.trees.size(), 0.0f);
    for (size_t i = 0; i < a.trees.size(); i++) {
        auto it = std::find(result.trees.begin(), result.trees.end(), a.trees[i]);
        if (it != result.trees.end()) {
            size_t idx = std::distance(result.trees.begin(), it);
            result.treeWeights[idx] += a.treeWeights[i] * (1.0f - t);
        }
    }
    for (size_t i = 0; i < b.trees.size(); i++) {
        auto it = std::find(result.trees.begin(), result.trees.end(), b.trees[i]);
        if (it != result.trees.end()) {
            size_t idx = std::distance(result.trees.begin(), it);
            result.treeWeights[idx] += b.treeWeights[i] * t;
        }
    }

    // Normalize weights
    float sum = 0.0f;
    for (float w : result.treeWeights) sum += w;
    if (sum > 0.0f) {
        for (float& w : result.treeWeights) w /= sum;
    }

    return result;
}

// Get vegetation profile for each biome
VegetationProfile getVegetationProfileForBiome(ClimateBiome biome) {
    VegetationProfile p;
    p.hasFlowers = false;
    p.flowerColor = glm::vec3(0.8f, 0.3f, 0.5f);  // Default pink

    switch (biome) {
        case ClimateBiome::TROPICAL_RAINFOREST:
            p.trees = {TreeType::PALM, TreeType::KAPOK, TreeType::MANGROVE};
            p.treeWeights = {0.4f, 0.4f, 0.2f};
            p.treeDensity = 0.85f;
            p.grassDensity = 0.3f;
            p.bushDensity = 0.7f;
            p.grassColor = glm::vec3(0.15f, 0.55f, 0.15f);  // Deep tropical green
            p.hasFlowers = true;
            p.flowerColor = glm::vec3(0.9f, 0.2f, 0.4f);  // Bright tropical flowers
            break;

        case ClimateBiome::TROPICAL_SEASONAL:
            p.trees = {TreeType::PALM, TreeType::OAK, TreeType::ACACIA};
            p.treeWeights = {0.4f, 0.35f, 0.25f};
            p.treeDensity = 0.5f;
            p.grassDensity = 0.6f;
            p.bushDensity = 0.4f;
            p.grassColor = glm::vec3(0.35f, 0.55f, 0.2f);  // Yellow-green
            p.hasFlowers = true;
            break;

        case ClimateBiome::TEMPERATE_FOREST:
            p.trees = {TreeType::OAK, TreeType::BIRCH, TreeType::PINE, TreeType::WILLOW};
            p.treeWeights = {0.4f, 0.25f, 0.2f, 0.15f};
            p.treeDensity = 0.7f;
            p.grassDensity = 0.4f;
            p.bushDensity = 0.5f;
            p.grassColor = glm::vec3(0.25f, 0.5f, 0.2f);  // Forest green
            p.hasFlowers = true;
            p.flowerColor = glm::vec3(0.9f, 0.85f, 0.3f);  // Wildflowers
            break;

        case ClimateBiome::TEMPERATE_GRASSLAND:
            p.trees = {TreeType::OAK, TreeType::WILLOW};
            p.treeWeights = {0.7f, 0.3f};
            p.treeDensity = 0.08f;  // Very sparse trees
            p.grassDensity = 0.9f;  // Lots of grass
            p.bushDensity = 0.15f;
            p.grassColor = glm::vec3(0.4f, 0.6f, 0.25f);  // Bright meadow green
            p.hasFlowers = true;
            p.flowerColor = glm::vec3(0.95f, 0.9f, 0.4f);  // Yellow wildflowers
            break;

        case ClimateBiome::BOREAL_FOREST:
            p.trees = {TreeType::SPRUCE, TreeType::FIR, TreeType::BIRCH, TreeType::PINE};
            p.treeWeights = {0.35f, 0.3f, 0.2f, 0.15f};
            p.treeDensity = 0.65f;
            p.grassDensity = 0.35f;
            p.bushDensity = 0.25f;
            p.grassColor = glm::vec3(0.25f, 0.45f, 0.25f);  // Dark boreal green
            p.hasFlowers = false;
            break;

        case ClimateBiome::TUNDRA:
            p.trees = {};  // No trees in tundra
            p.treeWeights = {};
            p.treeDensity = 0.0f;
            p.grassDensity = 0.4f;
            p.bushDensity = 0.15f;
            p.grassColor = glm::vec3(0.35f, 0.4f, 0.3f);  // Grayish tundra grass
            p.hasFlowers = true;
            p.flowerColor = glm::vec3(0.6f, 0.4f, 0.7f);  // Arctic flowers
            break;

        case ClimateBiome::ICE:
            p.trees = {};
            p.treeWeights = {};
            p.treeDensity = 0.0f;
            p.grassDensity = 0.0f;
            p.bushDensity = 0.0f;
            p.grassColor = glm::vec3(0.9f, 0.95f, 1.0f);  // Snow white
            p.hasFlowers = false;
            break;

        case ClimateBiome::DESERT_HOT:
            p.trees = {TreeType::CACTUS_SAGUARO, TreeType::CACTUS_BARREL, TreeType::JOSHUA_TREE};
            p.treeWeights = {0.5f, 0.3f, 0.2f};
            p.treeDensity = 0.04f;  // Very sparse
            p.grassDensity = 0.03f;
            p.bushDensity = 0.08f;
            p.grassColor = glm::vec3(0.6f, 0.5f, 0.35f);  // Dead/dry grass
            p.hasFlowers = false;
            break;

        case ClimateBiome::DESERT_COLD:
            p.trees = {TreeType::JUNIPER, TreeType::CACTUS_BARREL};
            p.treeWeights = {0.7f, 0.3f};
            p.treeDensity = 0.06f;
            p.grassDensity = 0.1f;
            p.bushDensity = 0.12f;
            p.grassColor = glm::vec3(0.5f, 0.45f, 0.35f);  // Dry grayish
            p.hasFlowers = false;
            break;

        case ClimateBiome::SAVANNA:
            p.trees = {TreeType::ACACIA, TreeType::BAOBAB};
            p.treeWeights = {0.75f, 0.25f};
            p.treeDensity = 0.12f;  // Scattered trees
            p.grassDensity = 0.85f;  // Lots of tall grass
            p.bushDensity = 0.2f;
            p.grassColor = glm::vec3(0.7f, 0.6f, 0.3f);  // Golden savanna grass
            p.hasFlowers = true;
            p.flowerColor = glm::vec3(0.9f, 0.6f, 0.2f);  // Orange flowers
            break;

        case ClimateBiome::SWAMP:
            p.trees = {TreeType::CYPRESS, TreeType::WILLOW, TreeType::MANGROVE};
            p.treeWeights = {0.5f, 0.3f, 0.2f};
            p.treeDensity = 0.55f;
            p.grassDensity = 0.2f;
            p.bushDensity = 0.4f;
            p.grassColor = glm::vec3(0.2f, 0.4f, 0.2f);  // Dark swamp green
            p.hasFlowers = true;
            p.flowerColor = glm::vec3(0.4f, 0.6f, 0.3f);  // Swamp lily color
            break;

        case ClimateBiome::MOUNTAIN_MEADOW:
            p.trees = {TreeType::ALPINE_FIR, TreeType::JUNIPER};
            p.treeWeights = {0.6f, 0.4f};
            p.treeDensity = 0.25f;
            p.grassDensity = 0.7f;
            p.bushDensity = 0.3f;
            p.grassColor = glm::vec3(0.3f, 0.55f, 0.25f);  // Alpine meadow green
            p.hasFlowers = true;
            p.flowerColor = glm::vec3(0.7f, 0.5f, 0.8f);  // Alpine flowers
            break;

        case ClimateBiome::MOUNTAIN_ROCK:
            p.trees = {TreeType::JUNIPER};
            p.treeWeights = {1.0f};
            p.treeDensity = 0.03f;  // Very sparse, stunted trees
            p.grassDensity = 0.1f;
            p.bushDensity = 0.05f;
            p.grassColor = glm::vec3(0.4f, 0.4f, 0.35f);  // Rocky gray-green
            p.hasFlowers = false;
            break;

        case ClimateBiome::MOUNTAIN_SNOW:
            p.trees = {};
            p.treeWeights = {};
            p.treeDensity = 0.0f;
            p.grassDensity = 0.0f;
            p.bushDensity = 0.0f;
            p.grassColor = glm::vec3(0.95f, 0.97f, 1.0f);  // Snow
            p.hasFlowers = false;
            break;

        case ClimateBiome::BEACH:
            p.trees = {TreeType::PALM};
            p.treeWeights = {1.0f};
            p.treeDensity = 0.08f;  // Sparse palms
            p.grassDensity = 0.15f;
            p.bushDensity = 0.1f;
            p.grassColor = glm::vec3(0.55f, 0.5f, 0.35f);  // Beach grass
            p.hasFlowers = false;
            break;

        case ClimateBiome::SHALLOW_WATER:
        case ClimateBiome::DEEP_OCEAN:
            p.trees = {};
            p.treeWeights = {};
            p.treeDensity = 0.0f;
            p.grassDensity = 0.0f;
            p.bushDensity = 0.0f;
            p.grassColor = glm::vec3(0.1f, 0.3f, 0.5f);  // Water color
            p.hasFlowers = false;
            break;

        default:
            // Default temperate profile
            p.trees = {TreeType::OAK, TreeType::PINE};
            p.treeWeights = {0.6f, 0.4f};
            p.treeDensity = 0.4f;
            p.grassDensity = 0.5f;
            p.bushDensity = 0.3f;
            p.grassColor = glm::vec3(0.3f, 0.5f, 0.2f);
            p.hasFlowers = true;
            break;
    }

    return p;
}

VegetationManager::VegetationManager(Terrain* terrain)
    : terrain(terrain), m_climate(nullptr) {
}

void VegetationManager::clear() {
    trees.clear();
    bushes.clear();
    grass.clear();
}

void VegetationManager::generate(unsigned int seed) {
    if (seed != 0) {
        Random::init();  // Reinitialize with time-based seed
    }

    clear();
    generateTrees(seed);
    generateBushes(seed);
    generateGrass(seed);

    // Generate all tree meshes
    generateTreeMeshes();
}

void VegetationManager::generateTreeMeshes() {
    // Generate meshes for all tree types that are being used
    std::set<TreeType> usedTypes;
    for (const auto& tree : trees) {
        usedTypes.insert(tree.type);
    }

    // Always generate the basic types for backward compatibility
    usedTypes.insert(TreeType::OAK);
    usedTypes.insert(TreeType::PINE);
    usedTypes.insert(TreeType::WILLOW);

    for (TreeType type : usedTypes) {
        if (treeMeshes.find(type) == treeMeshes.end()) {
            treeMeshes[type] = std::make_unique<MeshData>(TreeGenerator::generateTree(type));
            treeMeshes[type]->upload();
        }
    }

    // Generate bush mesh
    if (!bushMesh) {
        bushMesh = std::make_unique<MeshData>(TreeGenerator::generateBush());
        bushMesh->upload();
    }

    // Set legacy pointers for backward compatibility
    if (treeMeshes.find(TreeType::OAK) != treeMeshes.end()) {
        oakMesh = std::make_unique<MeshData>(*treeMeshes[TreeType::OAK]);
    }
    if (treeMeshes.find(TreeType::PINE) != treeMeshes.end()) {
        pineMesh = std::make_unique<MeshData>(*treeMeshes[TreeType::PINE]);
    }
    if (treeMeshes.find(TreeType::WILLOW) != treeMeshes.end()) {
        willowMesh = std::make_unique<MeshData>(*treeMeshes[TreeType::WILLOW]);
    }
}

TreeType VegetationManager::selectWeightedRandom(const std::vector<TreeType>& types,
                                                  const std::vector<float>& weights) const {
    if (types.empty()) return TreeType::OAK;  // Fallback
    if (types.size() == 1) return types[0];

    float r = Random::value();
    float cumulative = 0.0f;

    for (size_t i = 0; i < types.size(); i++) {
        cumulative += weights[i];
        if (r <= cumulative) {
            return types[i];
        }
    }

    return types.back();  // Fallback to last type
}

void VegetationManager::generateTrees(unsigned int seed) {
    trees.clear();

    float worldWidth = TerrainSampler::WORLD_SIZE;
    float worldDepth = TerrainSampler::WORLD_SIZE;
    if (terrain) {
        worldWidth = terrain->getWidth() * terrain->getScale();
        worldDepth = terrain->getDepth() * terrain->getScale();
    }

    // Sample points across terrain for tree placement
    int sampleDensity = 20;

    for (float x = -worldWidth/2; x < worldWidth/2; x += sampleDensity) {
        for (float z = -worldDepth/2; z < worldDepth/2; z += sampleDensity) {
            float offsetX = (Random::value() - 0.5f) * sampleDensity * 0.8f;
            float offsetZ = (Random::value() - 0.5f) * sampleDensity * 0.8f;

            float worldX = x + offsetX;
            float worldZ = z + offsetZ;
            if (!isInWorldBounds(terrain, worldX, worldZ)) continue;
            float height = sampleHeightForVegetation(terrain, worldX, worldZ);

            // Skip if water
            if (isWaterForVegetation(terrain, worldX, worldZ)) continue;

            // Get vegetation profile based on biome
            VegetationProfile profile;
            ClimateBiome primaryBiome = ClimateBiome::TEMPERATE_FOREST;
            float blendFactor = 0.0f;
            ClimateBiome secondaryBiome = primaryBiome;

            if (m_climate) {
                // Use climate system for biome-aware placement
                ClimateData climate = m_climate->getClimateAt(worldX, worldZ);
                primaryBiome = climate.getBiome();
                BiomeBlend blend = m_climate->calculateBiomeBlend(climate);
                blendFactor = blend.blendFactor;
                secondaryBiome = blend.secondary;

                profile = getVegetationProfileForBiome(primaryBiome);

                // Blend with secondary biome at transitions
                if (blendFactor > 0.1f) {
                    VegetationProfile secondary = getVegetationProfileForBiome(secondaryBiome);
                    profile = VegetationProfile::blend(profile, secondary, blendFactor);
                }
            } else {
                // Legacy height-based selection
                float heightNormalized = height / TerrainSampler::HEIGHT_SCALE;
                if (heightNormalized < TerrainSampler::BEACH_LEVEL + 0.02f || heightNormalized > 0.82f) continue;

                // Use temperate forest as default
                profile = getVegetationProfileForBiome(ClimateBiome::TEMPERATE_FOREST);
            }

            // Skip if no trees in this biome
            if (profile.trees.empty() || profile.treeDensity <= 0.0f) continue;

            // Probability-based placement using biome tree density
            if (Random::value() < profile.treeDensity) {
                TreeInstance tree;
                tree.position = glm::vec3(worldX, height, worldZ);

                // Scale variation
                float scaleVar = 0.8f + Random::value() * 1.0f;
                tree.scale = glm::vec3(scaleVar * TREE_SCALE_MULTIPLIER);

                // Random rotation
                tree.rotation = Random::value() * 6.28318f;

                // Select tree type based on biome weights
                tree.type = selectWeightedRandom(profile.trees, profile.treeWeights);

                trees.push_back(tree);
            }
        }
    }
}

void VegetationManager::generateBushes(unsigned int seed) {
    bushes.clear();

    float worldWidth = TerrainSampler::WORLD_SIZE;
    float worldDepth = TerrainSampler::WORLD_SIZE;
    if (terrain) {
        worldWidth = terrain->getWidth() * terrain->getScale();
        worldDepth = terrain->getDepth() * terrain->getScale();
    }

    int sampleDensity = 20;
    for (float x = -worldWidth/2; x < worldWidth/2; x += sampleDensity) {
        for (float z = -worldDepth/2; z < worldDepth/2; z += sampleDensity) {
            float offsetX = (Random::value() - 0.5f) * sampleDensity * 0.9f;
            float offsetZ = (Random::value() - 0.5f) * sampleDensity * 0.9f;

            float worldX = x + offsetX;
            float worldZ = z + offsetZ;
            if (!isInWorldBounds(terrain, worldX, worldZ)) continue;
            float height = sampleHeightForVegetation(terrain, worldX, worldZ);

            if (isWaterForVegetation(terrain, worldX, worldZ)) continue;

            // Get vegetation profile
            VegetationProfile profile;
            if (m_climate) {
                ClimateData climate = m_climate->getClimateAt(worldX, worldZ);
                ClimateBiome biome = climate.getBiome();
                BiomeBlend blend = m_climate->calculateBiomeBlend(climate);

                profile = getVegetationProfileForBiome(biome);
                if (blend.blendFactor > 0.1f) {
                    VegetationProfile secondary = getVegetationProfileForBiome(blend.secondary);
                    profile = VegetationProfile::blend(profile, secondary, blend.blendFactor);
                }
            } else {
                // Legacy height-based
                float heightNormalized = height / TerrainSampler::HEIGHT_SCALE;
                if (heightNormalized < TerrainSampler::BEACH_LEVEL + 0.02f || heightNormalized > 0.75f) continue;
                profile = getVegetationProfileForBiome(ClimateBiome::TEMPERATE_FOREST);
            }

            if (Random::value() < profile.bushDensity) {
                BushInstance bush;
                bush.position = glm::vec3(worldX, height, worldZ);

                float scaleVar = 0.8f + Random::value() * 1.2f;
                bush.scale = glm::vec3(scaleVar * BUSH_SCALE_MULTIPLIER);
                bush.rotation = Random::value() * 6.28318f;

                bushes.push_back(bush);
            }
        }
    }
}

void VegetationManager::generateGrass(unsigned int seed) {
    grass.clear();

    float worldWidth = TerrainSampler::WORLD_SIZE;
    float worldDepth = TerrainSampler::WORLD_SIZE;
    if (terrain) {
        worldWidth = terrain->getWidth() * terrain->getScale();
        worldDepth = terrain->getDepth() * terrain->getScale();
    }

    int grassSampleDensity = 15;
    for (float x = -worldWidth/2; x < worldWidth/2; x += grassSampleDensity) {
        for (float z = -worldDepth/2; z < worldDepth/2; z += grassSampleDensity) {
            float offsetX = (Random::value() - 0.5f) * grassSampleDensity;
            float offsetZ = (Random::value() - 0.5f) * grassSampleDensity;

            float worldX = x + offsetX;
            float worldZ = z + offsetZ;
            if (!isInWorldBounds(terrain, worldX, worldZ)) continue;
            float height = sampleHeightForVegetation(terrain, worldX, worldZ);

            if (isWaterForVegetation(terrain, worldX, worldZ)) continue;

            // Get vegetation profile
            VegetationProfile profile;
            if (m_climate) {
                ClimateData climate = m_climate->getClimateAt(worldX, worldZ);
                ClimateBiome biome = climate.getBiome();
                BiomeBlend blend = m_climate->calculateBiomeBlend(climate);

                profile = getVegetationProfileForBiome(biome);
                if (blend.blendFactor > 0.1f) {
                    VegetationProfile secondary = getVegetationProfileForBiome(blend.secondary);
                    profile = VegetationProfile::blend(profile, secondary, blend.blendFactor);
                }
            } else {
                // Legacy height-based
                float heightNormalized = height / TerrainSampler::HEIGHT_SCALE;
                if (heightNormalized < TerrainSampler::BEACH_LEVEL + 0.01f || heightNormalized > 0.70f) continue;
                profile = getVegetationProfileForBiome(ClimateBiome::TEMPERATE_GRASSLAND);
            }

            if (Random::value() < profile.grassDensity) {
                GrassCluster cluster;
                cluster.position = glm::vec3(worldX, height, worldZ);
                cluster.density = 0.5f + Random::value() * 0.5f;
                cluster.color = profile.grassColor;

                grass.push_back(cluster);
            }
        }
    }
}

bool VegetationManager::isSuitableForTrees(float x, float z) const {
    if (!isInWorldBounds(terrain, x, z)) {
        return false;
    }
    float height = sampleHeightForVegetation(terrain, x, z);

    float heightNormalized = height / TerrainSampler::HEIGHT_SCALE;
    if (heightNormalized < TerrainSampler::BEACH_LEVEL + 0.02f || heightNormalized > 0.82f) {
        return false;
    }

    if (isWaterForVegetation(terrain, x, z)) {
        return false;
    }

    return Random::value() < 0.40f;
}

bool VegetationManager::isSuitableForBushes(float x, float z) const {
    if (!isInWorldBounds(terrain, x, z)) {
        return false;
    }
    float height = sampleHeightForVegetation(terrain, x, z);

    float heightNormalized = height / TerrainSampler::HEIGHT_SCALE;
    if (heightNormalized < TerrainSampler::BEACH_LEVEL + 0.02f || heightNormalized > 0.75f) {
        return false;
    }

    if (isWaterForVegetation(terrain, x, z)) {
        return false;
    }

    return Random::value() < 0.35f;
}

bool VegetationManager::isSuitableForGrass(float x, float z) const {
    if (!isInWorldBounds(terrain, x, z)) {
        return false;
    }
    float height = sampleHeightForVegetation(terrain, x, z);

    float heightNormalized = height / TerrainSampler::HEIGHT_SCALE;
    if (heightNormalized < TerrainSampler::BEACH_LEVEL + 0.01f || heightNormalized > 0.70f) {
        return false;
    }

    if (isWaterForVegetation(terrain, x, z)) {
        return false;
    }

    return Random::value() < 0.4f;
}

const MeshData* VegetationManager::getMeshForType(TreeType type) const {
    auto it = treeMeshes.find(type);
    if (it != treeMeshes.end()) {
        return it->second.get();
    }

    // Fallback to legacy meshes
    switch (type) {
        case TreeType::OAK:
            return oakMesh.get();
        case TreeType::PINE:
            return pineMesh.get();
        case TreeType::WILLOW:
            return willowMesh.get();
        case TreeType::BUSH:
            return bushMesh.get();
        default:
            // For new tree types without meshes, return oak as fallback
            return oakMesh.get();
    }
}

void VegetationManager::render() {
    // Rendering handled by main simulation with instancing
}

// ============================================================================
// AQUATIC PLANT SYSTEM (Phase 8 - Ocean Ecosystem)
// ============================================================================

void VegetationManager::initializeAquaticPlants(DX12Device* dx12Device, unsigned int seed) {
    // Create aquatic plant system if not already created
    if (!aquaticPlants) {
        aquaticPlants = std::make_unique<AquaticPlantSystem>();
    }

    // Initialize with terrain and device
    aquaticPlants->initialize(dx12Device, terrain);
    aquaticPlants->generate(seed);
}

void VegetationManager::updateAquaticPlants(float deltaTime, const glm::vec3& cameraPos) {
    if (aquaticPlants) {
        aquaticPlants->update(deltaTime, cameraPos);
    }
}

VegetationManager::AquaticStats VegetationManager::getAquaticStats() const {
    AquaticStats stats;

    if (!aquaticPlants) {
        return stats;
    }

    // Get kelp forest stats
    const auto& kelpForests = aquaticPlants->getKelpForests();
    stats.totalKelpForests = static_cast<int>(kelpForests.size());

    // Get coral reef stats
    const auto& coralReefs = aquaticPlants->getCoralReefs();
    stats.totalCoralReefs = static_cast<int>(coralReefs.size());

    // Get all aquatic plant instances
    const auto& allPlants = aquaticPlants->getAllInstances();
    stats.totalAquaticPlants = static_cast<int>(allPlants.size());

    // Calculate total oxygen production and food value
    float totalHealth = 0.0f;
    int coralCount = 0;

    for (const auto& plant : allPlants) {
        auto config = getAquaticPlantConfig(plant.type);
        stats.totalOxygenProduction += config.oxygenProduction * plant.health;
        stats.totalFoodValue += config.foodValue * plant.health;
    }

    // Calculate average coral health
    for (const auto& reef : coralReefs) {
        totalHealth += reef.overallHealth;
        coralCount++;
    }
    stats.averageCoralHealth = coralCount > 0 ? totalHealth / coralCount : 0.0f;

    return stats;
}
