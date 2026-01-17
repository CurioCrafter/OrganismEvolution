#include "FoodChainManager.h"
#include "CreatureManager.h"
#include "../environment/EcosystemManager.h"
#include "../environment/Terrain.h"
#include "../environment/BiomePalette.h"
#include "../entities/Creature.h"
#include <algorithm>
#include <cmath>

// Global BiomePaletteManager for plant nutrition data
static BiomePaletteManager g_nutritionManager;

namespace Forge {

// ============================================================================
// Constructor
// ============================================================================

FoodChainManager::FoodChainManager() {
    m_recentEvents.reserve(MAX_EVENTS);
    initializeBaseCapacities();
}

void FoodChainManager::init(CreatureManager* creatures, EcosystemManager* ecosystem,
                            Terrain* terrain) {
    m_creatures = creatures;
    m_ecosystem = ecosystem;
    m_terrain = terrain;
    reset();
}

void FoodChainManager::reset() {
    m_energyStats.reset();
    m_recentEvents.clear();
    m_simulationTime = 0.0f;
    m_timeSinceCapacityUpdate = 0.0f;

    // Reset balance
    m_balance.currentPopulation.clear();
    m_balance.birthRateModifier.clear();
    m_balance.deathRateModifier.clear();
    m_balance.avgHunger.clear();

    // Initialize carrying capacities
    for (const auto& [type, cap] : m_baseCapacity) {
        m_balance.carryingCapacity[type] = cap;
    }
}

// ============================================================================
// Main Update
// ============================================================================

void FoodChainManager::update(float deltaTime) {
    m_simulationTime += deltaTime;

    // Periodic carrying capacity update
    m_timeSinceCapacityUpdate += deltaTime;
    if (m_timeSinceCapacityUpdate >= CAPACITY_UPDATE_INTERVAL) {
        updateCarryingCapacity();
        m_timeSinceCapacityUpdate = 0.0f;
    }

    // Update population balance
    updatePopulationBalance();

    // Update energy statistics
    updateEnergyStats();

    // Clean up old feeding events
    auto now = m_simulationTime;
    m_recentEvents.erase(
        std::remove_if(m_recentEvents.begin(), m_recentEvents.end(),
            [now](const FeedingEvent& e) { return now - e.timestamp > 10.0f; }),
        m_recentEvents.end()
    );
}

// ============================================================================
// Feeding Operations
// ============================================================================

float FoodChainManager::tryFeed(Creature& creature) {
    CreatureType type = creature.getType();

    // Route to appropriate feeding method
    if (isHerbivore(type)) {
        return feedOnPlant(creature);
    } else if (type == CreatureType::SCAVENGER) {
        return feedOnCorpse(creature);
    } else if (isPredator(type)) {
        // Find prey and attempt to feed
        Creature* prey = findNearestPrey(creature, creature.getVisionRange());
        if (prey) {
            return feedOnPrey(creature, *prey);
        }
    } else if (type == CreatureType::OMNIVORE) {
        // Omnivores try both
        float energy = feedOnPlant(creature);
        if (energy < 1.0f) {
            Creature* prey = findNearestPrey(creature, creature.getVisionRange() * 0.5f);
            if (prey) {
                energy += feedOnPrey(creature, *prey);
            }
        }
        return energy;
    }

    return 0.0f;
}

float FoodChainManager::feedOnPlant(Creature& herbivore) {
    if (!m_ecosystem) return 0.0f;

    ProducerSystem* producers = m_ecosystem->getProducers();
    if (!producers) return 0.0f;

    glm::vec3 pos = herbivore.getPosition();

    // Determine preferred food type and corresponding PlantCategory
    FoodSourceType preferredType = FoodSourceType::GRASS;
    PlantCategory plantCategory = PlantCategory::GRASS;

    CreatureType creatureType = herbivore.getType();
    if (creatureType == CreatureType::BROWSER) {
        preferredType = FoodSourceType::TREE_LEAF;
        plantCategory = PlantCategory::BUSH;  // Browsers eat bushes and shrubs
    } else if (creatureType == CreatureType::FRUGIVORE) {
        preferredType = FoodSourceType::BUSH_BERRY;
        plantCategory = PlantCategory::FLOWER;  // Frugivores prefer flowering/berry plants
    } else if (creatureType == CreatureType::GRAZER) {
        preferredType = FoodSourceType::GRASS;
        plantCategory = PlantCategory::GRASS;
    }

    float energy = producers->consumeAt(pos, preferredType, m_grazingRate * 0.016f);  // Per frame

    if (energy > 0.0f) {
        // Get nutrition data for this plant type
        const PlantNutrition& nutrition = g_nutritionManager.getNutrition(plantCategory);

        // Calculate creature's preference multiplier based on genome traits
        float creatureSize = herbivore.getSize();
        bool isHerbivoreType = (creatureType == CreatureType::HERBIVORE ||
                               creatureType == CreatureType::GRAZER ||
                               creatureType == CreatureType::BROWSER);

        // Get food preference from BiomePalette system
        float preferenceMultiplier = g_nutritionManager.getCreatureFoodPreference(
            plantCategory,
            creatureSize,
            0.5f,  // Default specialization (could be genome trait)
            isHerbivoreType
        );

        // Apply nutrition modifiers to energy gained
        // energyValue is 0-1 scale, multiply to make it meaningful (0.5-1.5 range)
        float nutritionMultiplier = 0.5f + nutrition.energyValue;
        float digestibilityFactor = nutrition.digestibility;
        float satiationFactor = nutrition.satiation;

        // Final energy calculation with all modifiers
        float finalEnergy = energy * nutritionMultiplier * digestibilityFactor * preferenceMultiplier;

        // Apply toxicity penalty (if any)
        if (nutrition.toxicity > 0.0f) {
            finalEnergy *= (1.0f - nutrition.toxicity * 0.5f);
            // Toxic plants cause damage
            if (nutrition.toxicity > 0.3f) {
                herbivore.takeDamage(nutrition.toxicity * 5.0f);
            }
        }

        // Hydration bonus (plants with high water content provide hydration)
        float hydrationBonus = nutrition.hydrationValue * 0.1f;
        finalEnergy += hydrationBonus;

        herbivore.addEnergy(finalEnergy);

        // Track energy flow
        m_energyStats.plantToHerbivore += finalEnergy;
        m_energyStats.herbivoreEnergy += finalEnergy * m_transferEfficiency;

        // Record event
        recordFeedingEvent(creatureType, CreatureType::GRAZER, finalEnergy, pos);

        return finalEnergy;
    }

    return energy;
}

float FoodChainManager::feedOnPrey(Creature& predator, Creature& prey) {
    if (!m_creatures) return 0.0f;

    // Check if prey is valid
    if (!isValidPrey(predator, prey)) return 0.0f;

    // Calculate hunting success
    float successChance = calculateHuntingSuccess(predator, prey);

    // Random roll for success
    float roll = static_cast<float>(std::rand()) / RAND_MAX;
    if (roll > successChance) {
        return 0.0f;  // Hunt failed
    }

    // Calculate energy gained (based on prey's energy * transfer efficiency)
    float preyEnergy = prey.getEnergy();
    float energyGained = preyEnergy * m_transferEfficiency;

    // Kill prey
    for (size_t i = 0; i < m_creatures->getAllCreatures().size(); ++i) {
        if (m_creatures->getAllCreatures()[i].get() == &prey) {
            CreatureHandle handle;
            handle.index = static_cast<uint32_t>(i);
            // Note: generation check would need to be added in production
            m_creatures->kill(handle, "predation");
            break;
        }
    }

    // Give energy to predator
    predator.addEnergy(energyGained);

    // Track energy flow
    if (isHerbivore(prey.getType())) {
        if (predator.getType() == CreatureType::APEX_PREDATOR ||
            predator.getType() == CreatureType::AERIAL_PREDATOR) {
            m_energyStats.herbivoreToApex += energyGained;
        } else {
            m_energyStats.herbivoreToSmallPred += energyGained;
        }
    } else if (predator.getType() == CreatureType::APEX_PREDATOR) {
        m_energyStats.smallPredToApex += energyGained;
    }

    // Record event
    recordFeedingEvent(predator.getType(), prey.getType(), energyGained, predator.getPosition());

    return energyGained;
}

float FoodChainManager::feedOnCorpse(Creature& scavenger) {
    if (!m_ecosystem) return 0.0f;

    DecomposerSystem* decomposers = m_ecosystem->getDecomposers();
    if (!decomposers) return 0.0f;

    glm::vec3 pos = scavenger.getPosition();

    // Try to consume nearby corpse using scavengeCorpse
    float energy = decomposers->scavengeCorpse(pos, 5.0f);

    if (energy > 0.0f) {
        scavenger.addEnergy(energy * m_transferEfficiency);

        // Track energy flow
        m_energyStats.deathDecay -= energy;  // Corpse energy leaving decay pool

        // Record event
        recordFeedingEvent(CreatureType::SCAVENGER, CreatureType::GRAZER, energy, pos);
    }

    return energy;
}

// ============================================================================
// Food Finding
// ============================================================================

glm::vec3 FoodChainManager::findNearestFood(const Creature& creature, float maxRange) const {
    CreatureType type = creature.getType();

    if (isHerbivore(type)) {
        // Determine preferred food type
        FoodSourceType preferred = FoodSourceType::GRASS;
        if (type == CreatureType::BROWSER) {
            preferred = FoodSourceType::TREE_LEAF;
        } else if (type == CreatureType::FRUGIVORE) {
            preferred = FoodSourceType::BUSH_BERRY;
        }
        return findNearestPlant(creature.getPosition(), preferred, maxRange);
    } else if (type == CreatureType::SCAVENGER) {
        return findNearestCorpse(creature.getPosition(), maxRange);
    } else if (isPredator(type)) {
        Creature* prey = findNearestPrey(creature, maxRange);
        if (prey) {
            return prey->getPosition();
        }
    } else if (type == CreatureType::OMNIVORE) {
        // Check for plants first, then prey
        glm::vec3 plantPos = findNearestPlant(creature.getPosition(), FoodSourceType::GRASS, maxRange);
        if (plantPos != glm::vec3(0.0f)) {
            return plantPos;
        }
        Creature* prey = findNearestPrey(creature, maxRange);
        if (prey) {
            return prey->getPosition();
        }
    }

    return glm::vec3(0.0f);
}

glm::vec3 FoodChainManager::findNearestPlant(const glm::vec3& position,
                                              FoodSourceType preferredType,
                                              float maxRange) const {
    if (!m_ecosystem) return glm::vec3(0.0f);

    ProducerSystem* producers = m_ecosystem->getProducers();
    if (!producers) return glm::vec3(0.0f);

    // Get all food positions based on type and find nearest
    std::vector<glm::vec3> foodPositions;
    switch (preferredType) {
        case FoodSourceType::GRASS:
            foodPositions = producers->getGrassPositions();
            break;
        case FoodSourceType::BUSH_BERRY:
            foodPositions = producers->getBushPositions();
            break;
        case FoodSourceType::TREE_LEAF:
            foodPositions = producers->getTreeLeafPositions();
            break;
        case FoodSourceType::TREE_FRUIT:
            foodPositions = producers->getTreeFruitPositions();
            break;
        default:
            foodPositions = producers->getAllFoodPositions();
            break;
    }

    glm::vec3 nearest(0.0f);
    float nearestDistSq = maxRange * maxRange;

    for (const auto& foodPos : foodPositions) {
        glm::vec3 diff = foodPos - position;
        float distSq = glm::dot(diff, diff);
        if (distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearest = foodPos;
        }
    }

    return nearest;
}

Creature* FoodChainManager::findNearestPrey(const Creature& predator, float maxRange) const {
    if (!m_creatures) return nullptr;
    return m_creatures->findNearestPrey(predator, maxRange);
}

glm::vec3 FoodChainManager::findNearestCorpse(const glm::vec3& position, float maxRange) const {
    if (!m_ecosystem) return glm::vec3(0.0f);

    DecomposerSystem* decomposers = m_ecosystem->getDecomposers();
    if (!decomposers) return glm::vec3(0.0f);

    Corpse* corpse = decomposers->findNearestCorpse(position, maxRange);
    if (corpse) {
        return corpse->position;
    }
    return glm::vec3(0.0f);
}

// ============================================================================
// Population Control
// ============================================================================

std::vector<FoodChainManager::SpawnRecommendation> FoodChainManager::getSpawnRecommendations() const {
    std::vector<SpawnRecommendation> recommendations;

    for (const auto& [type, capacity] : m_balance.carryingCapacity) {
        auto popIt = m_balance.currentPopulation.find(type);
        int current = (popIt != m_balance.currentPopulation.end()) ? popIt->second : 0;

        if (current < capacity * 0.5f) {  // Below 50% capacity
            SpawnRecommendation rec;
            rec.type = type;
            rec.count = static_cast<int>((capacity * 0.7f) - current);
            rec.priority = 1.0f - (static_cast<float>(current) / capacity);

            if (current == 0) {
                rec.reason = "Extinct - critical reintroduction needed";
                rec.priority = 1.0f;
            } else if (current < capacity * 0.2f) {
                rec.reason = "Critically low population";
                rec.priority = 0.9f;
            } else {
                rec.reason = "Below target population";
            }

            recommendations.push_back(rec);
        }
    }

    // Sort by priority (highest first)
    std::sort(recommendations.begin(), recommendations.end(),
              [](const SpawnRecommendation& a, const SpawnRecommendation& b) {
                  return a.priority > b.priority;
              });

    return recommendations;
}

bool FoodChainManager::shouldSpawn(CreatureType type) const {
    auto capIt = m_balance.carryingCapacity.find(type);
    auto popIt = m_balance.currentPopulation.find(type);

    if (capIt == m_balance.carryingCapacity.end()) return true;

    int current = (popIt != m_balance.currentPopulation.end()) ? popIt->second : 0;
    return current < capIt->second;
}

std::vector<FoodChainManager::SpawnRecommendation> FoodChainManager::getCullingRecommendations() const {
    std::vector<SpawnRecommendation> recommendations;

    for (const auto& [type, capacity] : m_balance.carryingCapacity) {
        auto popIt = m_balance.currentPopulation.find(type);
        int current = (popIt != m_balance.currentPopulation.end()) ? popIt->second : 0;

        if (current > capacity * 1.5f) {  // Over 150% capacity
            SpawnRecommendation rec;
            rec.type = type;
            rec.count = current - capacity;
            rec.priority = static_cast<float>(current) / capacity - 1.0f;
            rec.reason = "Overpopulation - ecosystem stress";
            recommendations.push_back(rec);
        }
    }

    return recommendations;
}

// ============================================================================
// Carrying Capacity
// ============================================================================

void FoodChainManager::updateCarryingCapacity() {
    if (!m_ecosystem) return;

    ProducerSystem* producers = m_ecosystem->getProducers();
    if (!producers) return;

    // Get total producer biomass
    float producerBiomass = producers->getTotalBiomass();

    // Carrying capacity scales with producer biomass
    // Rule of 10: each trophic level can support ~10% of the energy of the level below

    // Herbivore capacity based on producer biomass
    float herbivoreCapacity = producerBiomass * 0.10f;

    // Distribute among herbivore types
    m_balance.carryingCapacity[CreatureType::GRAZER] =
        static_cast<int>(herbivoreCapacity * 0.5f / 50.0f);  // ~50 energy per grazer
    m_balance.carryingCapacity[CreatureType::BROWSER] =
        static_cast<int>(herbivoreCapacity * 0.3f / 60.0f);
    m_balance.carryingCapacity[CreatureType::FRUGIVORE] =
        static_cast<int>(herbivoreCapacity * 0.2f / 30.0f);

    // Small predator capacity based on herbivore capacity
    float smallPredCapacity = herbivoreCapacity * 0.10f;
    m_balance.carryingCapacity[CreatureType::SMALL_PREDATOR] =
        static_cast<int>(smallPredCapacity * 0.6f / 80.0f);
    m_balance.carryingCapacity[CreatureType::OMNIVORE] =
        static_cast<int>(smallPredCapacity * 0.4f / 100.0f);

    // Apex predator capacity
    float apexCapacity = smallPredCapacity * 0.10f;
    m_balance.carryingCapacity[CreatureType::APEX_PREDATOR] =
        static_cast<int>(apexCapacity / 150.0f);

    // Scavengers scale with total creature biomass
    m_balance.carryingCapacity[CreatureType::SCAVENGER] =
        static_cast<int>(herbivoreCapacity * 0.05f / 60.0f);

    // Flying creatures
    m_balance.carryingCapacity[CreatureType::FLYING_BIRD] =
        static_cast<int>(herbivoreCapacity * 0.03f / 40.0f);
    m_balance.carryingCapacity[CreatureType::FLYING_INSECT] =
        static_cast<int>(herbivoreCapacity * 0.05f / 10.0f);
    m_balance.carryingCapacity[CreatureType::AERIAL_PREDATOR] =
        static_cast<int>(apexCapacity * 0.2f / 100.0f);

    // Aquatic creatures (scale with water area estimate)
    float waterFactor = 0.3f;  // Assume 30% of world is water
    m_balance.carryingCapacity[CreatureType::AQUATIC_HERBIVORE] =
        static_cast<int>(herbivoreCapacity * waterFactor * 0.3f / 20.0f);
    m_balance.carryingCapacity[CreatureType::AQUATIC_PREDATOR] =
        static_cast<int>(smallPredCapacity * waterFactor * 0.3f / 50.0f);
    m_balance.carryingCapacity[CreatureType::AQUATIC_APEX] =
        static_cast<int>(apexCapacity * waterFactor * 0.3f / 100.0f);

    // Ensure minimums
    for (auto& [type, cap] : m_balance.carryingCapacity) {
        cap = std::max(cap, 2);  // At least 2 for breeding
    }
}

int FoodChainManager::getCarryingCapacity(CreatureType type) const {
    auto it = m_balance.carryingCapacity.find(type);
    if (it != m_balance.carryingCapacity.end()) {
        return it->second;
    }
    return 10;  // Default
}

void FoodChainManager::setBaseCarryingCapacity(CreatureType type, int capacity) {
    m_baseCapacity[type] = capacity;
    m_balance.carryingCapacity[type] = capacity;
}

// ============================================================================
// Statistics
// ============================================================================

float FoodChainManager::getFoodAvailability(const glm::vec3& position, CreatureType forType) const {
    if (!m_ecosystem) return 0.0f;

    if (isHerbivore(forType)) {
        ProducerSystem* producers = m_ecosystem->getProducers();
        if (producers) {
            // Calculate food density by counting nearby food patches
            float totalBiomass = producers->getTotalBiomass();
            int activePatches = producers->getActivePatches();
            if (activePatches > 0) {
                return std::min(1.0f, totalBiomass / (activePatches * 100.0f));
            }
        }
    } else if (forType == CreatureType::SCAVENGER) {
        DecomposerSystem* decomposers = m_ecosystem->getDecomposers();
        if (decomposers) {
            // Calculate corpse density based on count and total biomass
            int corpseCount = decomposers->getCorpseCount();
            float totalBiomass = decomposers->getTotalBiomass();
            if (corpseCount > 0) {
                return std::min(1.0f, totalBiomass / 500.0f);
            }
        }
    } else if (isPredator(forType)) {
        if (m_creatures) {
            // Count potential prey nearby
            const auto& nearby = m_creatures->queryNearby(position, 30.0f);
            int preyCount = 0;
            for (const Creature* c : nearby) {
                if (c && isHerbivore(c->getType())) {
                    preyCount++;
                }
            }
            return std::min(1.0f, preyCount / 10.0f);
        }
    }

    return 0.5f;
}

// ============================================================================
// Private Helpers
// ============================================================================

void FoodChainManager::initializeBaseCapacities() {
    // Base carrying capacities (before producer scaling)
    m_baseCapacity[CreatureType::GRAZER] = 100;
    m_baseCapacity[CreatureType::BROWSER] = 50;
    m_baseCapacity[CreatureType::FRUGIVORE] = 80;
    m_baseCapacity[CreatureType::SMALL_PREDATOR] = 30;
    m_baseCapacity[CreatureType::OMNIVORE] = 20;
    m_baseCapacity[CreatureType::APEX_PREDATOR] = 10;
    m_baseCapacity[CreatureType::SCAVENGER] = 15;
    m_baseCapacity[CreatureType::FLYING_BIRD] = 40;
    m_baseCapacity[CreatureType::FLYING_INSECT] = 100;
    m_baseCapacity[CreatureType::AERIAL_PREDATOR] = 8;
    m_baseCapacity[CreatureType::AQUATIC_HERBIVORE] = 60;
    m_baseCapacity[CreatureType::AQUATIC_PREDATOR] = 20;
    m_baseCapacity[CreatureType::AQUATIC_APEX] = 5;
    m_baseCapacity[CreatureType::AMPHIBIAN] = 25;
}

void FoodChainManager::updateEnergyStats() {
    if (!m_creatures) return;

    // Reset per-frame stats
    m_energyStats.herbivoreEnergy = 0.0f;
    m_energyStats.smallPredatorEnergy = 0.0f;
    m_energyStats.apexPredatorEnergy = 0.0f;

    // Sum energy by trophic level
    m_creatures->forEach([this](Creature& c, size_t) {
        float energy = c.getEnergy();

        if (isHerbivore(c.getType())) {
            m_energyStats.herbivoreEnergy += energy;
        } else if (c.getType() == CreatureType::SMALL_PREDATOR ||
                   c.getType() == CreatureType::OMNIVORE) {
            m_energyStats.smallPredatorEnergy += energy;
        } else if (c.getType() == CreatureType::APEX_PREDATOR) {
            m_energyStats.apexPredatorEnergy += energy;
        }
    });

    // Calculate efficiencies
    if (m_energyStats.producerEnergy > 0.0f) {
        m_energyStats.systemEfficiency =
            (m_energyStats.herbivoreEnergy + m_energyStats.smallPredatorEnergy +
             m_energyStats.apexPredatorEnergy) / m_energyStats.producerEnergy;
    }
}

void FoodChainManager::updatePopulationBalance() {
    if (!m_creatures) return;

    // Clear current counts
    m_balance.currentPopulation.clear();
    m_balance.avgHunger.clear();

    std::unordered_map<CreatureType, float> totalHunger;
    std::unordered_map<CreatureType, int> counts;

    // Count populations and sum hunger
    m_creatures->forEach([&](Creature& c, size_t) {
        CreatureType type = c.getType();
        m_balance.currentPopulation[type]++;
        counts[type]++;

        // Calculate hunger (inverse of energy ratio)
        float hunger = 1.0f - (c.getEnergy() / c.getMaxEnergy());
        totalHunger[type] += hunger;
    });

    // Calculate average hunger
    for (const auto& [type, total] : totalHunger) {
        if (counts[type] > 0) {
            m_balance.avgHunger[type] = total / counts[type];
        }
    }

    // Update birth/death rate modifiers based on population pressure
    for (const auto& [type, pop] : m_balance.currentPopulation) {
        float pressure = m_balance.getPressure(type);

        // High pressure = lower birth rate, higher death rate
        m_balance.birthRateModifier[type] = std::max(0.1f, 1.0f - pressure * 0.5f);
        m_balance.deathRateModifier[type] = std::min(2.0f, 0.5f + pressure * 0.5f);
    }
}

void FoodChainManager::recordFeedingEvent(CreatureType consumer, CreatureType consumed,
                                           float energy, const glm::vec3& location) {
    FeedingEvent event;
    event.consumer = consumer;
    event.consumed = consumed;
    event.energyTransferred = energy;
    event.location = location;
    event.timestamp = m_simulationTime;

    if (m_recentEvents.size() >= MAX_EVENTS) {
        m_recentEvents.erase(m_recentEvents.begin());
    }
    m_recentEvents.push_back(event);
}

float FoodChainManager::calculateHuntingSuccess(const Creature& predator,
                                                 const Creature& prey) const {
    // Base success rate
    float success = 0.3f * m_huntingSuccessModifier;

    // Speed advantage
    float speedRatio = predator.getSpeed() / std::max(0.1f, prey.getSpeed());
    success *= std::min(2.0f, speedRatio);

    // Size advantage
    float sizeRatio = predator.getSize() / std::max(0.1f, prey.getSize());
    if (sizeRatio > 1.5f) {
        success *= 1.2f;
    } else if (sizeRatio < 0.8f) {
        success *= 0.5f;  // Prey too big
    }

    // Energy penalty (tired predator hunts worse)
    float energyFactor = predator.getEnergy() / predator.getMaxEnergy();
    success *= 0.5f + 0.5f * energyFactor;

    // Pack hunting bonus (if applicable)
    CreatureTraits traits = CreatureTraits::getTraitsFor(predator.getType());
    if (traits.isPackHunter) {
        // Would need to check for nearby pack members
        success *= 1.3f;
    }

    return std::min(0.9f, success);  // Cap at 90%
}

bool FoodChainManager::isValidPrey(const Creature& predator, const Creature& prey) const {
    if (!prey.isActive()) return false;
    if (&predator == &prey) return false;

    return canBeHuntedBy(prey.getType(), predator.getType(), prey.getSize());
}

} // namespace Forge
