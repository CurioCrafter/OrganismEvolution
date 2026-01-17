#include "SmallCreatureEcosystem.h"
#include "../../environment/VegetationManager.h"
#include "../../environment/ProducerSystem.h"
#include "../../environment/DecomposerSystem.h"
#include <algorithm>
#include <random>

namespace small {

// =============================================================================
// SmallCreatureEcosystem Implementation
// =============================================================================

SmallCreatureEcosystem::SmallCreatureEcosystem(float worldSize)
    : worldSize_(worldSize)
    , terrain_(nullptr)
    , largeCreatureGrid_(nullptr)
    , vegManager_(nullptr)
    , producers_(nullptr)
    , decomposers_(nullptr) {
    manager_ = std::make_unique<SmallCreatureManager>(worldSize);
    treeSystem_ = std::make_unique<TreeDwellerSystem>();
}

SmallCreatureEcosystem::~SmallCreatureEcosystem() = default;

void SmallCreatureEcosystem::initialize(Terrain* terrain,
                                         SpatialGrid* largeCreatureGrid,
                                         VegetationManager* vegManager,
                                         ProducerSystem* producers,
                                         DecomposerSystem* decomposers) {
    terrain_ = terrain;
    largeCreatureGrid_ = largeCreatureGrid;
    vegManager_ = vegManager;
    producers_ = producers;
    decomposers_ = decomposers;

    manager_->setLargeCreatureSpatialGrid(largeCreatureGrid);

    if (vegManager) {
        treeSystem_->initialize(vegManager);
    }
}

void SmallCreatureEcosystem::update(float deltaTime,
                                     std::vector<Creature>& largeCreatures,
                                     Terrain* terrain) {
    terrain_ = terrain;

    // Sync food from ecosystem
    syncFoodFromEcosystem(producers_, decomposers_);

    // Update main small creature system
    manager_->update(deltaTime, terrain);

    // Update tree dwellers
    treeSystem_->update(deltaTime, *manager_);

    // Update interactions with large creatures
    updatePredatorInteractions(largeCreatures);

    // Update decomposer activity
    updateDecomposers(deltaTime);

    // Update pollination
    updatePollinators(deltaTime, producers_);
}

void SmallCreatureEcosystem::spawnInitialPopulations(int totalCount) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> posDist(-worldSize_ * 0.4f, worldSize_ * 0.4f);
    std::uniform_int_distribution<int> biomeDist(0, 14);  // Assuming 15 biome types

    // Initialize manager with a base population
    manager_->initialize(terrain_, totalCount);

    // Additional biome-specific spawning
    int additionalCount = totalCount / 5;
    for (int i = 0; i < additionalCount; ++i) {
        XMFLOAT3 pos = { posDist(rng), 0.0f, posDist(rng) };
        if (terrain_) {
            pos.y = terrain_->getHeight(pos.x, pos.z);
        }

        // Get biome at this position (simplified - would use terrain biome data)
        int biome = biomeDist(rng);
        spawnForBiome(pos, biome, 1);
    }
}

void SmallCreatureEcosystem::syncFoodFromEcosystem(ProducerSystem* producers,
                                                     DecomposerSystem* decomposers) {
    // Add food sources from producer system
    if (producers) {
        // Get grass clusters as food
        // This would integrate with the actual ProducerSystem API
        // For now, we add some generic food sources

        // Placeholder: Add plant matter around the world
        static float syncTimer = 0.0f;
        syncTimer += 0.016f;
        if (syncTimer > 5.0f) {
            syncTimer = 0.0f;

            std::mt19937 rng(std::random_device{}());
            std::uniform_real_distribution<float> posDist(-worldSize_ * 0.4f, worldSize_ * 0.4f);

            // Add scattered plant food
            for (int i = 0; i < 50; ++i) {
                XMFLOAT3 pos = { posDist(rng), 0.0f, posDist(rng) };
                if (terrain_) {
                    pos.y = terrain_->getHeight(pos.x, pos.z);
                }
                manager_->addFood(pos, 10.0f, MicroFood::Type::PLANT_MATTER);
            }

            // Add nectar near flowers (simplified)
            for (int i = 0; i < 20; ++i) {
                XMFLOAT3 pos = { posDist(rng), 0.0f, posDist(rng) };
                if (terrain_) {
                    pos.y = terrain_->getHeight(pos.x, pos.z) + 0.5f;
                }
                manager_->addFood(pos, 5.0f, MicroFood::Type::NECTAR);
            }

            // Add seeds
            for (int i = 0; i < 30; ++i) {
                XMFLOAT3 pos = { posDist(rng), 0.0f, posDist(rng) };
                if (terrain_) {
                    pos.y = terrain_->getHeight(pos.x, pos.z);
                }
                manager_->addFood(pos, 8.0f, MicroFood::Type::SEEDS);
            }
        }
    }

    // Add carrion from decomposer system
    if (decomposers) {
        // Would integrate with actual DecomposerSystem carrion
        // For now handled via addCarrion() calls from main ecosystem
    }
}

void SmallCreatureEcosystem::addCarrion(const XMFLOAT3& position, float amount, CreatureType type) {
    // Large creature died - add as food source for decomposers
    manager_->addFood(position, amount * 10.0f, MicroFood::Type::CARRION);

    // Attract carrion beetles and flies
    // This happens naturally as they detect the food
}

bool SmallCreatureEcosystem::isPreyFor(SmallCreature* small, Creature* large) {
    if (!small || !large) return false;
    if (!small->isAlive()) return false;

    // Check if large creature type can eat this small creature
    CreatureType largeType = large->getType();
    SmallCreatureType smallType = small->type;

    // Size check - large creature must be significantly bigger
    float largeSize = large->getSize();
    auto props = getProperties(smallType);
    float smallSize = props.minSize * small->genome->size;

    if (largeSize < smallSize * 5.0f) return false;

    // Type compatibility
    switch (largeType) {
        case CreatureType::SMALL_PREDATOR:
            // Small predators eat insects and small mammals
            return isInsect(smallType) || isSmallMammal(smallType);

        case CreatureType::OMNIVORE:
            // Omnivores eat insects
            return isInsect(smallType);

        case CreatureType::AERIAL_PREDATOR:
            // Birds eat insects, mice, lizards
            return isInsect(smallType) || isSmallMammal(smallType) ||
                   isReptile(smallType);

        case CreatureType::APEX_PREDATOR:
            // Apex predators can eat larger small creatures
            return isSmallMammal(smallType) || isReptile(smallType) ||
                   isAmphibian(smallType);

        case CreatureType::FLYING_BIRD:
            // Birds eat insects
            return isInsect(smallType);

        case CreatureType::AQUATIC_PREDATOR:
            // Aquatic predators eat aquatic insects and amphibians
            return isAmphibian(smallType) ||
                   smallType == SmallCreatureType::CRAYFISH ||
                   smallType == SmallCreatureType::CRAB_SMALL;

        default:
            return false;
    }
}

void SmallCreatureEcosystem::consumeSmallCreature(Creature* predator, SmallCreature* prey) {
    if (!predator || !prey) return;

    // Calculate energy from prey
    auto props = getProperties(prey->type);
    float energyGain = EcosystemIntegration::getEnergyValue(prey->type,
                                                            props.minSize * prey->genome->size);

    // Transfer energy to predator
    predator->addEnergy(energyGain);

    // Kill prey
    prey->setAlive(false);

    // Stats
    smallEatenByLarge_++;

    // Chance to spawn replacement in same area (ecological balance)
    // This would be handled by the main reproduction system
}

SmallCreature* SmallCreatureEcosystem::findSmallPreyNear(const XMFLOAT3& position, float radius,
                                                          CreatureType predatorType) {
    auto* grid = manager_->getSpatialGrid();
    if (!grid) return nullptr;

    auto validTypes = getValidPreyTypes(predatorType);
    if (validTypes.empty()) return nullptr;

    return grid->findNearest(position, radius,
        [this, &validTypes, predatorType](SmallCreature* c) {
            if (!c->isAlive()) return false;
            for (auto type : validTypes) {
                if (c->type == type) return true;
            }
            return false;
        });
}

std::vector<SmallCreature*> SmallCreatureEcosystem::findSmallCreaturesNear(const XMFLOAT3& position,
                                                                            float radius) {
    auto* grid = manager_->getSpatialGrid();
    if (!grid) return {};

    return grid->queryCreatures(position, radius);
}

std::vector<SmallCreatureType> SmallCreatureEcosystem::getValidPreyTypes(CreatureType predatorType) {
    std::vector<SmallCreatureType> types;

    switch (predatorType) {
        case CreatureType::SMALL_PREDATOR:
            types.push_back(SmallCreatureType::MOUSE);
            types.push_back(SmallCreatureType::CRICKET);
            types.push_back(SmallCreatureType::GRASSHOPPER);
            types.push_back(SmallCreatureType::BEETLE_GROUND);
            break;

        case CreatureType::AERIAL_PREDATOR:
            types.push_back(SmallCreatureType::MOUSE);
            types.push_back(SmallCreatureType::RABBIT);
            types.push_back(SmallCreatureType::LIZARD_SMALL);
            types.push_back(SmallCreatureType::SNAKE_SMALL);
            break;

        case CreatureType::FLYING_BIRD:
            types.push_back(SmallCreatureType::EARTHWORM);
            types.push_back(SmallCreatureType::BEETLE_GROUND);
            types.push_back(SmallCreatureType::CRICKET);
            types.push_back(SmallCreatureType::GRASSHOPPER);
            types.push_back(SmallCreatureType::CATERPILLAR);
            break;

        case CreatureType::APEX_PREDATOR:
            types.push_back(SmallCreatureType::RABBIT);
            types.push_back(SmallCreatureType::SQUIRREL_TREE);
            types.push_back(SmallCreatureType::SNAKE_SMALL);
            break;

        case CreatureType::OMNIVORE:
            types.push_back(SmallCreatureType::BEETLE_GROUND);
            types.push_back(SmallCreatureType::CRICKET);
            types.push_back(SmallCreatureType::ANT_WORKER);
            break;

        default:
            break;
    }

    return types;
}

SmallCreatureEcosystem::EcosystemStats SmallCreatureEcosystem::getStats() const {
    EcosystemStats stats = {};

    auto managerStats = manager_->getStats();
    stats.totalSmallCreatures = managerStats.totalCreatures;
    stats.aliveSmallCreatures = managerStats.aliveCreatures;
    stats.insectCount = managerStats.insectCount;
    stats.arachnidCount = managerStats.arachnidCount;
    stats.mammalCount = managerStats.mammalCount;
    stats.reptileCount = managerStats.reptileCount;
    stats.amphibianCount = managerStats.amphibianCount;

    stats.colonyCount = managerStats.colonyCount;

    // Count colony members
    for (const auto& colony : manager_->getColonies()) {
        stats.totalColonyMembers += colony->getMemberCount();
    }
    if (stats.colonyCount > 0) {
        stats.averageColonySize = static_cast<float>(stats.totalColonyMembers) / stats.colonyCount;
    }

    stats.smallEatenByLarge = smallEatenByLarge_;
    stats.smallKilledBySmall = smallKilledBySmall_;
    stats.foodConsumed = foodConsumed_;

    stats.nestCount = treeSystem_->getNestCount();

    // Count creatures in trees (simplified - would track properly)
    stats.creaturesInTrees = 0;
    for (const auto& c : manager_->getCreatures()) {
        if (c.isAlive() && c.position.y > 2.0f) {
            stats.creaturesInTrees++;
        }
    }

    return stats;
}

void SmallCreatureEcosystem::updatePredatorInteractions(std::vector<Creature>& largeCreatures) {
    auto* grid = manager_->getSpatialGrid();
    if (!grid) return;

    for (auto& large : largeCreatures) {
        if (!large.isAlive()) continue;

        // Only predators hunt small creatures
        if (!EcosystemBehaviors::isPredator(large.getType()) &&
            large.getType() != CreatureType::OMNIVORE &&
            large.getType() != CreatureType::FLYING_BIRD) {
            continue;
        }

        // Check for small prey near large creature
        XMFLOAT3 largePos = { large.getPosition().x, large.getPosition().y, large.getPosition().z };
        float huntRadius = large.getSensorySystem().getVisionRange() * 0.5f;

        auto validTypes = getValidPreyTypes(large.getType());

        for (auto type : validTypes) {
            auto nearbySmall = grid->queryByType(largePos, huntRadius, type);

            for (auto* small : nearbySmall) {
                if (!small->isAlive()) continue;

                float dx = small->position.x - largePos.x;
                float dy = small->position.y - largePos.y;
                float dz = small->position.z - largePos.z;
                float dist = sqrtf(dx * dx + dy * dy + dz * dz);

                // Small creature should flee
                if (EcosystemIntegration::shouldFlee(small, &large, dist)) {
                    small->setFleeing(true);
                    small->fear = 1.0f;

                    XMFLOAT3 fleeDir = EcosystemIntegration::calculateFleeDirection(small, &large);
                    small->targetPosition.x = small->position.x + fleeDir.x * 5.0f;
                    small->targetPosition.y = small->position.y + fleeDir.y;
                    small->targetPosition.z = small->position.z + fleeDir.z * 5.0f;
                }

                // Check if large creature catches small
                if (dist < large.getSize() * 0.5f && large.isHunting()) {
                    consumeSmallCreature(&large, small);
                }
            }
        }
    }
}

void SmallCreatureEcosystem::updateDecomposers(float deltaTime) {
    // Dung beetles, carrion beetles, and earthworms process dead matter
    auto& creatures = manager_->getCreatures();
    auto& food = const_cast<std::vector<MicroFood>&>(manager_->getFoodSources());

    for (auto& creature : creatures) {
        if (!creature.isAlive()) continue;

        auto props = getProperties(creature.type);
        if (!props.isDecomposer) continue;

        // Find carrion or organic matter nearby
        auto* grid = manager_->getSpatialGrid();
        auto nearbyFood = grid->queryFood(creature.position, creature.genome->smellRange);

        for (auto* foodItem : nearbyFood) {
            if (foodItem->type != MicroFood::Type::CARRION &&
                foodItem->type != MicroFood::Type::SOIL_ORGANIC) {
                continue;
            }

            float dx = foodItem->position.x - creature.position.x;
            float dz = foodItem->position.z - creature.position.z;
            float dist = sqrtf(dx * dx + dz * dz);

            if (dist < 0.1f) {
                // Consume and process
                float consume = std::min(foodItem->amount, deltaTime * creature.genome->metabolism);
                foodItem->amount -= consume;
                creature.energy += consume * 2.0f;
                foodConsumed_++;

                // Decomposers convert carrion to soil nutrients
                // This would update the terrain/producer system
            }
        }
    }

    // Clean up empty food sources
    food.erase(
        std::remove_if(food.begin(), food.end(),
            [](const MicroFood& f) { return f.amount <= 0.0f; }),
        food.end()
    );
}

void SmallCreatureEcosystem::updatePollinators(float deltaTime, ProducerSystem* producers) {
    if (!producers) return;

    // Bees and butterflies pollinate flowers
    auto& creatures = manager_->getCreatures();

    for (auto& creature : creatures) {
        if (!creature.isAlive()) continue;

        auto props = getProperties(creature.type);
        if (!props.isPollinator) continue;

        // Check if carrying pollen and near a flower
        if (creature.isCarryingFood()) {
            // Would trigger pollination in ProducerSystem
            // producers->pollinate(creature.position);
        }
    }
}

void SmallCreatureEcosystem::spawnForBiome(const XMFLOAT3& position, int biomeType, int count) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> offsetDist(-2.0f, 2.0f);

    // Different creatures for different biomes
    std::vector<SmallCreatureType> suitableTypes;

    // Forest biomes (0-2)
    if (biomeType <= 2) {
        suitableTypes = {
            SmallCreatureType::ANT_WORKER,
            SmallCreatureType::BEETLE_GROUND,
            SmallCreatureType::SPIDER_ORB_WEAVER,
            SmallCreatureType::SQUIRREL_TREE,
            SmallCreatureType::MOUSE,
            SmallCreatureType::EARTHWORM,
            SmallCreatureType::CENTIPEDE,
            SmallCreatureType::BUTTERFLY,
            SmallCreatureType::MOTH
        };
    }
    // Grassland biomes (3-4)
    else if (biomeType <= 4) {
        suitableTypes = {
            SmallCreatureType::GRASSHOPPER,
            SmallCreatureType::CRICKET,
            SmallCreatureType::RABBIT,
            SmallCreatureType::MOUSE,
            SmallCreatureType::BEE_WORKER,
            SmallCreatureType::BUTTERFLY,
            SmallCreatureType::ANT_WORKER,
            SmallCreatureType::SPIDER_WOLF
        };
    }
    // Desert biomes (5-6)
    else if (biomeType <= 6) {
        suitableTypes = {
            SmallCreatureType::SCORPION,
            SmallCreatureType::BEETLE_GROUND,
            SmallCreatureType::LIZARD_SMALL,
            SmallCreatureType::GECKO,
            SmallCreatureType::SNAKE_SMALL,
            SmallCreatureType::ANT_WORKER
        };
    }
    // Wetland biomes (7-8)
    else if (biomeType <= 8) {
        suitableTypes = {
            SmallCreatureType::FROG,
            SmallCreatureType::TOAD,
            SmallCreatureType::SALAMANDER,
            SmallCreatureType::DRAGONFLY,
            SmallCreatureType::MOSQUITO,
            SmallCreatureType::SNAIL,
            SmallCreatureType::CRAYFISH,
            SmallCreatureType::EARTHWORM
        };
    }
    // Mountain/Tundra (9+)
    else {
        suitableTypes = {
            SmallCreatureType::MOUSE,
            SmallCreatureType::MOLE,
            SmallCreatureType::BEETLE_GROUND,
            SmallCreatureType::SPIDER_WOLF
        };
    }

    if (suitableTypes.empty()) return;

    std::uniform_int_distribution<int> typeDist(0, static_cast<int>(suitableTypes.size()) - 1);

    for (int i = 0; i < count; ++i) {
        SmallCreatureType type = suitableTypes[typeDist(rng)];

        XMFLOAT3 pos = {
            position.x + offsetDist(rng),
            position.y,
            position.z + offsetDist(rng)
        };

        if (terrain_) {
            pos.y = terrain_->getHeight(pos.x, pos.z);
        }

        // Adjust spawn height based on habitat type
        auto props = getProperties(type);
        pos.y = getSpawnHeightForHabitat(pos, props.primaryHabitat, rng);

        manager_->spawn(type, pos);
    }
}

float SmallCreatureEcosystem::getSpawnHeightForHabitat(const XMFLOAT3& basePos,
                                                         HabitatType habitat,
                                                         std::mt19937& rng) {
    float groundHeight = basePos.y;
    if (terrain_) {
        groundHeight = terrain_->getHeight(basePos.x, basePos.z);
    }

    std::uniform_real_distribution<float> heightVar(0.0f, 1.0f);

    switch (habitat) {
        case HabitatType::UNDERGROUND:
            // Spawn at or slightly below ground (burrowers will dig down)
            return groundHeight - 0.1f;

        case HabitatType::GROUND_SURFACE:
            // Spawn at ground level
            return groundHeight;

        case HabitatType::GRASS:
            // Spawn slightly above ground in grass
            return groundHeight + 0.05f + heightVar(rng) * 0.15f;

        case HabitatType::BUSH:
            // Spawn in bush level (0.3-1.5m above ground)
            return groundHeight + 0.3f + heightVar(rng) * 1.2f;

        case HabitatType::TREE_TRUNK:
            // Spawn on tree trunk (1-5m above ground)
            return groundHeight + 1.0f + heightVar(rng) * 4.0f;

        case HabitatType::CANOPY:
            // Spawn in tree canopy (4-10m above ground)
            return groundHeight + 4.0f + heightVar(rng) * 6.0f;

        case HabitatType::WATER_SURFACE:
            // Spawn at water level (approximate)
            return groundHeight + 0.01f;

        case HabitatType::UNDERWATER:
            // Spawn below water surface
            return groundHeight - 0.5f;

        case HabitatType::AERIAL:
            // Flying insects spawn in air (1-5m above ground for variety)
            return groundHeight + 1.0f + heightVar(rng) * 4.0f;

        default:
            return groundHeight;
    }
}

// =============================================================================
// EcosystemIntegration Utility Functions
// =============================================================================

namespace EcosystemIntegration {

float getThreatLevel(CreatureType largeType) {
    switch (largeType) {
        case CreatureType::APEX_PREDATOR:
            return 1.0f;
        case CreatureType::AERIAL_PREDATOR:
            return 0.9f;
        case CreatureType::SMALL_PREDATOR:
            return 0.7f;
        case CreatureType::OMNIVORE:
            return 0.4f;
        case CreatureType::FLYING_BIRD:
            return 0.5f;
        default:
            return 0.0f;
    }
}

float getAttractionLevel(SmallCreatureType smallType, CreatureType largeType) {
    // How attractive is this small creature as prey?
    auto props = getProperties(smallType);

    float baseAttraction = 0.0f;

    // Larger small creatures are more attractive
    baseAttraction += props.maxSize * 2.0f;

    // Slow creatures are easier targets
    baseAttraction += (1.0f / (props.baseSpeed + 0.1f)) * 0.5f;

    // Colonial creatures might be avoided (danger in numbers)
    if (props.isColonial) {
        baseAttraction *= 0.5f;
    }

    // Venomous/poisonous creatures are less attractive
    if (props.isVenomous || props.isPoisonous) {
        baseAttraction *= 0.3f;
    }

    return std::clamp(baseAttraction, 0.0f, 1.0f);
}

bool isSuitableBiome(SmallCreatureType type, int biomeType) {
    auto props = getProperties(type);

    // Desert creatures need dry biomes
    if (type == SmallCreatureType::SCORPION ||
        type == SmallCreatureType::GECKO) {
        return biomeType >= 5 && biomeType <= 6;
    }

    // Aquatic/semi-aquatic need water
    if (isAmphibian(type) || type == SmallCreatureType::CRAYFISH) {
        return biomeType >= 7 && biomeType <= 8;
    }

    // Forest dwellers
    if (type == SmallCreatureType::SQUIRREL_TREE ||
        type == SmallCreatureType::TREE_FROG ||
        type == SmallCreatureType::SPIDER_ORB_WEAVER) {
        return biomeType <= 2;
    }

    // Most creatures are generalists
    return true;
}

float getSpawnProbability(SmallCreatureType type, int biomeType) {
    if (!isSuitableBiome(type, biomeType)) {
        return 0.0f;
    }

    auto props = getProperties(type);

    // Colonial insects spawn in colonies, not individually
    if (props.isColonial) {
        return 0.01f;  // Low individual spawn, high colony spawn
    }

    // Common vs rare creatures
    if (isInsect(type)) {
        return 0.5f;  // Insects are common
    }
    if (isSmallMammal(type)) {
        return 0.2f;
    }
    if (isReptile(type) || isAmphibian(type)) {
        return 0.1f;
    }

    return 0.3f;
}

float getEnergyValue(SmallCreatureType type, float size) {
    auto props = getProperties(type);

    // Base energy from size
    float energy = size * 100.0f;

    // Insects have exoskeletons - less digestible
    if (props.hasExoskeleton) {
        energy *= 0.7f;
    }

    // Mammals are nutritious
    if (isSmallMammal(type)) {
        energy *= 1.5f;
    }

    // Poisonous creatures might cause harm
    if (props.isPoisonous) {
        energy *= 0.5f;  // Less net gain
    }

    return energy;
}

bool shouldFlee(SmallCreature* small, Creature* large, float distance) {
    if (!small || !large) return false;

    float threatLevel = getThreatLevel(large->getType());
    if (threatLevel < 0.1f) return false;

    // Fear threshold based on creature's fear response trait
    float fleeDistance = small->genome->fearResponse * large->getSize() * 10.0f;

    return distance < fleeDistance;
}

XMFLOAT3 calculateFleeDirection(SmallCreature* small, Creature* large) {
    if (!small || !large) return { 0, 0, 0 };

    XMFLOAT3 largePos = { large->getPosition().x, large->getPosition().y, large->getPosition().z };

    float dx = small->position.x - largePos.x;
    float dy = small->position.y - largePos.y;
    float dz = small->position.z - largePos.z;

    float dist = sqrtf(dx * dx + dy * dy + dz * dz);
    if (dist < 0.01f) {
        // Random direction if on top of predator
        return { 1.0f, 0.0f, 0.0f };
    }

    // Normalize and return away direction
    return { dx / dist, dy / dist * 0.5f, dz / dist };
}

}  // namespace EcosystemIntegration

}  // namespace small
