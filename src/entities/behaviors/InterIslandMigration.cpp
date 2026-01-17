#include "InterIslandMigration.h"
#include "../../core/MultiIslandManager.h"
#include <algorithm>
#include <cmath>

namespace Forge {

// ============================================================================
// Constructor
// ============================================================================

InterIslandMigration::InterIslandMigration() {
    // Enable all migration types by default
    m_enabledTypes.fill(true);

    // Seed RNG
    std::random_device rd;
    m_rng.seed(rd());
}

// ============================================================================
// Main Update
// ============================================================================

void InterIslandMigration::update(float deltaTime, MultiIslandManager& islands) {
    // Check for new migration triggers
    checkMigrationTriggers(islands);

    // Process ongoing migrations
    processMigrations(deltaTime, islands);

    // Update statistics
    m_stats.inProgressMigrations = static_cast<int>(m_activeMigrations.size());
}

void InterIslandMigration::checkMigrationTriggers(MultiIslandManager& islands) {
    std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);

    for (uint32_t islandIdx = 0; islandIdx < islands.getIslandCount(); ++islandIdx) {
        auto* island = islands.getIsland(islandIdx);
        if (!island || !island->creatures) continue;

        // Check population pressure
        bool populationPressure = checkPopulationPressure(islandIdx, islands);

        // Iterate through creatures on this island
        island->creatures->forEach([&](Creature& creature, size_t idx) {
            if (!creature.isAlive()) return;

            // Calculate base migration chance
            float migrationChance = m_config.baseMigrationChance;

            // Apply modifiers
            if (populationPressure) {
                migrationChance *= 5.0f;  // Much more likely when crowded
            }

            if (creature.getEnergy() < m_config.starvationThreshold * 100.0f) {
                migrationChance *= 3.0f;  // More likely when hungry
            }

            // Check coastal proximity (simplified - creatures near edge more likely)
            glm::vec3 pos = creature.getPosition();
            float worldSize = static_cast<float>(island->terrain->getWidth() * island->terrain->getScale());
            float distFromCenter = glm::length(glm::vec2(pos.x, pos.z));
            float edgeProximity = distFromCenter / (worldSize * 0.5f);

            if (edgeProximity > 0.7f) {
                migrationChance *= m_config.coastalProximityBonus;
            }

            // Roll for migration attempt
            if (chanceDist(m_rng) < migrationChance) {
                // Determine best migration type for this creature
                MigrationType type = getBestMigrationType(&creature);

                if (!isMigrationTypeEnabled(type)) return;

                // Select target island
                uint32_t targetIsland = selectTargetIsland(islandIdx, &creature, type, islands);

                if (targetIsland != islandIdx && targetIsland < islands.getIslandCount()) {
                    CreatureHandle handle;
                    handle.index = static_cast<uint32_t>(idx);
                    handle.generation = 1;  // Simplified

                    attemptMigration(islandIdx, handle, targetIsland, type, islands);
                }
            }
        });
    }
}

void InterIslandMigration::processMigrations(float deltaTime, MultiIslandManager& islands) {
    // Process active migrations
    for (auto it = m_activeMigrations.begin(); it != m_activeMigrations.end(); ) {
        MigrationEvent& event = *it;

        // Update migration progress
        event.timeElapsed += deltaTime;
        event.progress = std::min(1.0f, event.timeElapsed / event.estimatedDuration);

        // Update position
        event.currentPosition = interpolateMigrationPosition(event);

        // Consume energy
        float energyCost = getTypeEnergyCost(event.type) * deltaTime;
        event.currentEnergy = std::max(0.0f, event.currentEnergy - energyCost);

        // Check for death during transit
        std::uniform_real_distribution<float> survivalRoll(0.0f, 1.0f);
        float deathChance = (1.0f - event.survivalChance) * deltaTime * 0.1f;

        if (survivalRoll(m_rng) < deathChance || event.currentEnergy <= 0.0f) {
            event.state = MigrationState::FAILED;
            failMigration(event, islands);
            it = m_activeMigrations.erase(it);
            continue;
        }

        // Check for completion
        if (event.progress >= 1.0f) {
            event.state = MigrationState::COMPLETED;
            completeMigration(event, islands);
            it = m_activeMigrations.erase(it);
            continue;
        }

        // Update state
        if (event.progress > 0.9f) {
            event.state = MigrationState::ARRIVING;
        } else {
            event.state = MigrationState::IN_TRANSIT;
        }

        notifyCallbacks(event);
        ++it;
    }
}

// ============================================================================
// Migration Attempts
// ============================================================================

bool InterIslandMigration::attemptMigration(uint32_t sourceIsland, CreatureHandle handle,
                                             uint32_t targetIsland, MigrationType type,
                                             MultiIslandManager& islands) {
    auto* srcIsland = islands.getIsland(sourceIsland);
    auto* dstIsland = islands.getIsland(targetIsland);

    if (!srcIsland || !dstIsland) return false;
    if (!srcIsland->creatures) return false;

    Creature* creature = srcIsland->creatures->get(handle);
    if (!creature || !creature->isAlive()) return false;

    // Calculate survival chance
    float survivalChance = calculateSurvivalChance(creature, sourceIsland, targetIsland, type, islands);

    // Calculate travel time
    float travelTime = calculateTravelTime(sourceIsland, targetIsland, type, islands);

    // Create migration event
    MigrationEvent event;
    event.creatureId = handle.index;
    event.sourceIsland = sourceIsland;
    event.targetIsland = targetIsland;
    event.type = type;
    event.state = MigrationState::INITIATING;

    event.progress = 0.0f;
    event.totalDistance = islands.getIslandDistance(sourceIsland, targetIsland);
    event.timeElapsed = 0.0f;
    event.estimatedDuration = travelTime;

    event.startEnergy = creature->getEnergy();
    event.currentEnergy = creature->getEnergy();
    event.survivalChance = survivalChance;

    event.startPosition = srcIsland->localToWorld(creature->getPosition());
    event.currentPosition = event.startPosition;
    event.targetPosition = calculateArrivalPosition(targetIsland, type, islands);

    event.genome = creature->getGenome();
    event.creatureType = creature->getType();

    // Remove creature from source island
    srcIsland->creatures->kill(handle, "Migration departure");

    // Add to active migrations
    m_activeMigrations.push_back(event);

    // Update statistics
    m_stats.totalAttempts++;
    m_stats.attemptsByType[static_cast<int>(type)]++;

    notifyCallbacks(event);
    return true;
}

bool InterIslandMigration::forceMigration(uint32_t sourceIsland, CreatureHandle handle,
                                           uint32_t targetIsland, MultiIslandManager& islands) {
    auto* srcIsland = islands.getIsland(sourceIsland);
    if (!srcIsland || !srcIsland->creatures) return false;

    Creature* creature = srcIsland->creatures->get(handle);
    if (!creature) return false;

    MigrationType type = getBestMigrationType(creature);
    return attemptMigration(sourceIsland, handle, targetIsland, type, islands);
}

// ============================================================================
// Migration Completion
// ============================================================================

void InterIslandMigration::completeMigration(MigrationEvent& event, MultiIslandManager& islands) {
    auto* dstIsland = islands.getIsland(event.targetIsland);
    if (!dstIsland || !dstIsland->creatures) {
        failMigration(event, islands);
        return;
    }

    // Calculate arrival position in local coordinates
    glm::vec3 localArrival = dstIsland->worldToLocal(event.targetPosition);

    // Ensure position is on land (not in water)
    if (dstIsland->terrain) {
        float terrainHeight = dstIsland->terrain->getHeight(localArrival.x, localArrival.z);
        localArrival.y = terrainHeight;

        // If water, find nearest land
        if (dstIsland->terrain->isWater(localArrival.x, localArrival.z)) {
            // Move toward center of island
            glm::vec2 center(0.0f);
            glm::vec2 dir = center - glm::vec2(localArrival.x, localArrival.z);
            if (glm::length(dir) > 0.1f) {
                dir = glm::normalize(dir);
                for (int i = 0; i < 10; ++i) {
                    localArrival.x += dir.x * 10.0f;
                    localArrival.z += dir.y * 10.0f;
                    if (!dstIsland->terrain->isWater(localArrival.x, localArrival.z)) {
                        localArrival.y = dstIsland->terrain->getHeight(localArrival.x, localArrival.z);
                        break;
                    }
                }
            }
        }
    }

    // Spawn creature on destination island
    CreatureHandle newHandle = dstIsland->creatures->spawnWithGenome(localArrival, event.genome);

    if (newHandle.isValid()) {
        // Set remaining energy
        Creature* newCreature = dstIsland->creatures->get(newHandle);
        if (newCreature) {
            // Energy was already reduced during transit - creature arrives with remaining energy
        }

        // Update island statistics for migration tracking
        auto* srcIsland = islands.getIsland(event.sourceIsland);
        if (srcIsland) {
            srcIsland->stats.emigrations++;
        }
        dstIsland->stats.immigrations++;

        // Update migration statistics
        m_stats.successfulMigrations++;
        m_stats.successesByType[static_cast<int>(event.type)]++;

        auto key = std::make_pair(event.sourceIsland, event.targetIsland);
        m_stats.migrationsBetweenIslands[key]++;

        // Update average survival rate and travel time
        float totalSuccess = static_cast<float>(m_stats.successfulMigrations);
        float totalAttempts = static_cast<float>(m_stats.totalAttempts);
        m_stats.avgSurvivalRate = totalSuccess / std::max(1.0f, totalAttempts);

        // Update average travel time (weighted average)
        float weight = 1.0f / std::max(1.0f, totalSuccess);
        m_stats.avgTravelTime = m_stats.avgTravelTime * (1.0f - weight) + event.timeElapsed * weight;
    } else {
        failMigration(event, islands);
    }

    notifyCallbacks(event);
}

void InterIslandMigration::failMigration(MigrationEvent& event, MultiIslandManager& islands) {
    m_stats.failedMigrations++;

    // Creature is lost at sea - already removed from source island
    notifyCallbacks(event);
}

// ============================================================================
// Migration Queries
// ============================================================================

std::vector<const MigrationEvent*> InterIslandMigration::getMigrationsFrom(uint32_t islandIndex) const {
    std::vector<const MigrationEvent*> result;
    for (const auto& event : m_activeMigrations) {
        if (event.sourceIsland == islandIndex) {
            result.push_back(&event);
        }
    }
    return result;
}

std::vector<const MigrationEvent*> InterIslandMigration::getMigrationsTo(uint32_t islandIndex) const {
    std::vector<const MigrationEvent*> result;
    for (const auto& event : m_activeMigrations) {
        if (event.targetIsland == islandIndex) {
            result.push_back(&event);
        }
    }
    return result;
}

// ============================================================================
// Configuration
// ============================================================================

void InterIslandMigration::setMigrationTypeEnabled(MigrationType type, bool enabled) {
    int index = static_cast<int>(type);
    if (index >= 0 && index < static_cast<int>(m_enabledTypes.size())) {
        m_enabledTypes[index] = enabled;
    }
}

bool InterIslandMigration::isMigrationTypeEnabled(MigrationType type) const {
    int index = static_cast<int>(type);
    if (index >= 0 && index < static_cast<int>(m_enabledTypes.size())) {
        return m_enabledTypes[index];
    }
    return false;
}

void InterIslandMigration::registerCallback(MigrationCallback callback) {
    if (callback) {
        m_callbacks.push_back(callback);
    }
}

// ============================================================================
// Calculations
// ============================================================================

float InterIslandMigration::calculateSurvivalChance(const Creature* creature, uint32_t sourceIsland,
                                                     uint32_t targetIsland, MigrationType type,
                                                     const MultiIslandManager& islands) const {
    if (!creature) return 0.0f;

    // Base survival by type
    float survival = getTypeBaseSurvival(type);

    // Distance penalty
    float distance = islands.getIslandDistance(sourceIsland, targetIsland);
    float distancePenalty = std::min(0.5f, distance / 1000.0f);
    survival -= distancePenalty;

    // Fitness bonus
    float fitness = creature->getFitness();
    if (type == MigrationType::FLYING) {
        survival += fitness * m_config.fitnessFlyBonus;
    } else {
        survival += fitness * m_config.fitnessSwimBonus;
    }

    // Energy bonus
    float energyRatio = creature->getEnergy() / 200.0f;  // Assume max energy 200
    survival += energyRatio * 0.1f;

    // Ocean current bonus (for water-based migration)
    if (type == MigrationType::COASTAL_DRIFT || type == MigrationType::FLOATING_DEBRIS) {
        const OceanCurrent* current = islands.getArchipelagoData().getCurrentBetween(sourceIsland, targetIsland);
        if (current) {
            survival += current->strength * 0.15f;
        }
    }

    // Creature type bonuses
    CreatureType creatureType = creature->getType();
    if (type == MigrationType::FLYING && isFlying(creatureType)) {
        survival += 0.2f;  // Flying creatures are better at flying migration
    }
    if ((type == MigrationType::COASTAL_DRIFT) && isAquatic(creatureType)) {
        survival += 0.3f;  // Aquatic creatures are better at swimming
    }

    return std::max(0.05f, std::min(0.95f, survival));
}

float InterIslandMigration::calculateTravelTime(uint32_t sourceIsland, uint32_t targetIsland,
                                                 MigrationType type, const MultiIslandManager& islands) const {
    float distance = islands.getIslandDistance(sourceIsland, targetIsland);
    float speed = getTypeSpeed(type);

    // Apply current/wind bonuses
    const auto& data = islands.getArchipelagoData();

    if (type == MigrationType::COASTAL_DRIFT || type == MigrationType::FLOATING_DEBRIS) {
        const OceanCurrent* current = data.getCurrentBetween(sourceIsland, targetIsland);
        if (current) {
            speed *= (1.0f + current->strength * (m_config.currentSpeedBonus - 1.0f));
        }
    }

    if (type == MigrationType::FLYING) {
        // Check wind direction alignment
        const auto* srcIsland = islands.getIsland(sourceIsland);
        const auto* dstIsland = islands.getIsland(targetIsland);
        if (srcIsland && dstIsland) {
            glm::vec2 travelDir = glm::normalize(dstIsland->worldPosition - srcIsland->worldPosition);
            float windAlignment = glm::dot(travelDir, data.wind.prevailingDirection);
            if (windAlignment > 0.0f) {
                speed *= (1.0f + windAlignment * data.wind.strength * (m_config.windSpeedBonus - 1.0f));
            }
        }
    }

    return distance / std::max(0.1f, speed);
}

MigrationType InterIslandMigration::getBestMigrationType(const Creature* creature) {
    if (!creature) return MigrationType::RANDOM_DISPERSAL;

    CreatureType type = creature->getType();

    // Flying creatures prefer flying migration
    if (isFlying(type)) {
        return MigrationType::FLYING;
    }

    // Aquatic creatures prefer coastal drift
    if (isAquatic(type)) {
        return MigrationType::COASTAL_DRIFT;
    }

    // Land creatures most likely to raft or drift
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float roll = dist(m_rng);

    if (roll < 0.4f) {
        return MigrationType::FLOATING_DEBRIS;
    } else if (roll < 0.7f) {
        return MigrationType::COASTAL_DRIFT;
    } else {
        return MigrationType::RANDOM_DISPERSAL;
    }
}

// ============================================================================
// Trigger Checks
// ============================================================================

bool InterIslandMigration::checkCoastalDrift(const Creature* creature, uint32_t islandIndex,
                                              const MultiIslandManager& islands) {
    // Check if creature is near coast and in water
    const auto* island = islands.getIsland(islandIndex);
    if (!island || !island->terrain) return false;

    glm::vec3 pos = creature->getPosition();
    return island->terrain->isWater(pos.x, pos.z);
}

bool InterIslandMigration::checkPopulationPressure(uint32_t islandIndex, const MultiIslandManager& islands) {
    const auto* island = islands.getIsland(islandIndex);
    if (!island || !island->creatures) return false;

    int population = island->creatures->getTotalPopulation();
    int capacity = static_cast<int>(CreatureManager::MAX_CREATURES * 0.8f);

    return (static_cast<float>(population) / capacity) > m_config.populationPressureThreshold;
}

bool InterIslandMigration::checkFoodScarcity(const Creature* creature, uint32_t islandIndex,
                                              const MultiIslandManager& islands) {
    if (!creature) return false;
    return creature->getEnergy() < m_config.starvationThreshold * 100.0f;
}

// ============================================================================
// Target Selection
// ============================================================================

uint32_t InterIslandMigration::selectTargetIsland(uint32_t sourceIsland, const Creature* creature,
                                                    MigrationType type, const MultiIslandManager& islands) {
    // Get neighboring islands
    float maxRange = 500.0f;  // Maximum migration range

    if (type == MigrationType::FLYING) {
        maxRange = 800.0f;  // Flying can go further
    } else if (type == MigrationType::FLOATING_DEBRIS) {
        maxRange = 400.0f;  // Rafting is shorter range
    }

    auto neighbors = islands.getNeighborIslands(sourceIsland, maxRange);

    if (neighbors.empty()) {
        return sourceIsland;  // No valid targets
    }

    // Weight targets by survival chance
    std::vector<float> weights;
    float totalWeight = 0.0f;

    for (uint32_t targetIdx : neighbors) {
        float survivalChance = calculateSurvivalChance(creature, sourceIsland, targetIdx, type, islands);
        weights.push_back(survivalChance);
        totalWeight += survivalChance;
    }

    if (totalWeight <= 0.0f) {
        return neighbors[0];  // Default to nearest
    }

    // Weighted random selection
    std::uniform_real_distribution<float> dist(0.0f, totalWeight);
    float roll = dist(m_rng);

    float cumulative = 0.0f;
    for (size_t i = 0; i < neighbors.size(); ++i) {
        cumulative += weights[i];
        if (roll <= cumulative) {
            return neighbors[i];
        }
    }

    return neighbors.back();
}

// ============================================================================
// Position Calculations
// ============================================================================

glm::vec3 InterIslandMigration::calculateArrivalPosition(uint32_t targetIsland, MigrationType type,
                                                          const MultiIslandManager& islands) {
    const auto* island = islands.getIsland(targetIsland);
    if (!island) return glm::vec3(0.0f);

    // Base arrival at island center (world coords)
    glm::vec3 arrival(island->worldPosition.x, 0.0f, island->worldPosition.y);

    // Offset based on migration type
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);
    std::uniform_real_distribution<float> radiusDist(0.3f, 0.8f);

    float angle = angleDist(m_rng);
    float radius = radiusDist(m_rng);

    // Water-based arrivals come from edge, flying can land anywhere
    if (type == MigrationType::COASTAL_DRIFT || type == MigrationType::FLOATING_DEBRIS) {
        radius = 0.9f;  // Arrive at coast
    }

    float worldSize = 100.0f;  // Approximate
    if (island->terrain) {
        worldSize = island->terrain->getWidth() * island->terrain->getScale() * 0.5f;
    }

    arrival.x += std::cos(angle) * worldSize * radius;
    arrival.z += std::sin(angle) * worldSize * radius;

    return arrival;
}

glm::vec3 InterIslandMigration::interpolateMigrationPosition(const MigrationEvent& event) const {
    // Simple linear interpolation
    float t = event.progress;

    // Add some wave motion for water-based travel
    if (event.type == MigrationType::COASTAL_DRIFT || event.type == MigrationType::FLOATING_DEBRIS) {
        float wave = std::sin(event.timeElapsed * 2.0f) * 5.0f;
        glm::vec3 pos = glm::mix(event.startPosition, event.targetPosition, t);
        pos.y += wave;
        return pos;
    }

    // Arc trajectory for flying
    if (event.type == MigrationType::FLYING) {
        glm::vec3 pos = glm::mix(event.startPosition, event.targetPosition, t);
        float arcHeight = std::sin(t * 3.14159f) * 50.0f;  // Arc up to 50 units high
        pos.y += arcHeight;
        return pos;
    }

    return glm::mix(event.startPosition, event.targetPosition, t);
}

// ============================================================================
// Type Helpers
// ============================================================================

float InterIslandMigration::getTypeBaseSurvival(MigrationType type) const {
    switch (type) {
        case MigrationType::FLYING:
            return m_config.baseFlyingSurvival;
        case MigrationType::COASTAL_DRIFT:
            return m_config.baseSwimSurvival;
        case MigrationType::FLOATING_DEBRIS:
            return m_config.baseRaftingSurvival;
        case MigrationType::SEASONAL:
            return 0.6f;  // Prepared migration
        case MigrationType::POPULATION_PRESSURE:
            return 0.4f;
        case MigrationType::FOOD_SCARCITY:
            return 0.3f;  // Weakened creatures
        case MigrationType::MATE_SEEKING:
            return 0.5f;
        case MigrationType::RANDOM_DISPERSAL:
        default:
            return 0.25f;
    }
}

float InterIslandMigration::getTypeSpeed(MigrationType type) const {
    switch (type) {
        case MigrationType::FLYING:
            return m_config.flyingSpeed;
        case MigrationType::COASTAL_DRIFT:
            return m_config.swimSpeed;
        case MigrationType::FLOATING_DEBRIS:
            return m_config.raftingSpeed;
        default:
            return m_config.swimSpeed * 0.5f;
    }
}

float InterIslandMigration::getTypeEnergyCost(MigrationType type) const {
    switch (type) {
        case MigrationType::FLYING:
            return m_config.flyingEnergyPerUnit;
        case MigrationType::COASTAL_DRIFT:
            return m_config.swimEnergyPerUnit;
        case MigrationType::FLOATING_DEBRIS:
            return m_config.raftingEnergyPerUnit;
        default:
            return m_config.swimEnergyPerUnit;
    }
}

void InterIslandMigration::notifyCallbacks(const MigrationEvent& event) {
    for (const auto& callback : m_callbacks) {
        callback(event);
    }
}

} // namespace Forge
