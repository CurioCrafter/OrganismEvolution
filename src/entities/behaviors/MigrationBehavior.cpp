#include "MigrationBehavior.h"
#include "../Creature.h"
#include "../../core/CreatureManager.h"
#include "../../environment/SeasonManager.h"
#include "../../environment/BiomeSystem.h"
#include "../../environment/Terrain.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <random>

bool MigrationBehavior::canMigrate(CreatureType type) {
    switch (type) {
        // Flying creatures are natural migrators
        case CreatureType::FLYING_BIRD:
        case CreatureType::FLYING_INSECT:
        case CreatureType::AERIAL_PREDATOR:
            return true;

        // Grazing herbivores migrate for food
        case CreatureType::GRAZER:
        case CreatureType::BROWSER:
            return true;

        // Aquatic creatures can migrate through connected waters
        case CreatureType::AQUATIC_HERBIVORE:
        case CreatureType::AQUATIC_PREDATOR:
        case CreatureType::AMPHIBIAN:
            return true;

        // These types generally don't migrate
        case CreatureType::APEX_PREDATOR:
        case CreatureType::SMALL_PREDATOR:
        case CreatureType::FRUGIVORE:
        case CreatureType::OMNIVORE:
        case CreatureType::SCAVENGER:
        case CreatureType::AQUATIC_APEX:
        case CreatureType::PARASITE:
        case CreatureType::CLEANER:
        default:
            return false;
    }
}

void MigrationBehavior::init(SeasonManager* seasons, BiomeSystem* biomes, Terrain* terrain) {
    m_seasonManager = seasons;
    m_biomeSystem = biomes;
    m_terrain = terrain;

    if (m_seasonManager) {
        m_lastSeason = m_seasonManager->getCurrentSeason();
    }

    // Define common migration routes
    m_knownRoutes = {
        // Birds fly south for winter
        {BiomeType::BOREAL_FOREST, BiomeType::TEMPERATE_FOREST, Season::FALL, 0.9f},
        {BiomeType::TEMPERATE_FOREST, BiomeType::GRASSLAND, Season::FALL, 0.8f},
        {BiomeType::TUNDRA, BiomeType::TEMPERATE_FOREST, Season::FALL, 0.95f},

        // Return north for spring
        {BiomeType::TEMPERATE_FOREST, BiomeType::BOREAL_FOREST, Season::SPRING, 0.8f},
        {BiomeType::GRASSLAND, BiomeType::TEMPERATE_FOREST, Season::SPRING, 0.7f},

        // Grazers follow green grass
        {BiomeType::GRASSLAND, BiomeType::SAVANNA, Season::WINTER, 0.6f},
        {BiomeType::SAVANNA, BiomeType::GRASSLAND, Season::SUMMER, 0.6f},

        // Aquatic migrations
        {BiomeType::DEEP_OCEAN, BiomeType::SHALLOW_WATER, Season::SPRING, 0.7f},
        {BiomeType::SHALLOW_WATER, BiomeType::DEEP_OCEAN, Season::FALL, 0.7f},
    };
}

void MigrationBehavior::update(float deltaTime, CreatureManager& creatures) {
    m_currentTime += deltaTime;

    // Update cooldowns
    for (auto it = m_migrationCooldowns.begin(); it != m_migrationCooldowns.end(); ) {
        it->second -= deltaTime;
        if (it->second <= 0.0f) {
            it = m_migrationCooldowns.erase(it);
        } else {
            ++it;
        }
    }

    // Check for season change
    if (m_seasonManager) {
        Season currentSeason = m_seasonManager->getCurrentSeason();
        if (currentSeason != m_lastSeason) {
            m_seasonChanged = true;
            m_lastSeason = currentSeason;
        } else {
            m_seasonChanged = false;
        }
    }

    // Check triggers
    if (m_seasonChanged) {
        checkSeasonalTriggers(creatures);
    }
    checkResourceTriggers(creatures);

    // Update active migrations
    updateActiveMigrations(deltaTime, creatures);

    // Clean up completed migrations
    for (uint32_t id : m_migrationsToRemove) {
        m_activeMigrations.erase(id);
    }
    m_migrationsToRemove.clear();
}

glm::vec3 MigrationBehavior::calculateForce(Creature* creature) {
    if (!creature || !creature->isAlive()) {
        return glm::vec3(0.0f);
    }

    auto it = m_activeMigrations.find(creature->getID());
    if (it == m_activeMigrations.end()) {
        return glm::vec3(0.0f);
    }

    const Migration& migration = it->second;

    // Force depends on phase
    switch (migration.phase) {
        case MigrationPhase::PREPARING:
            // No movement force while preparing
            return glm::vec3(0.0f);

        case MigrationPhase::DEPARTING:
        case MigrationPhase::TRAVELING:
        {
            // Strong force toward current waypoint
            glm::vec3 toWaypoint = migration.currentWaypoint - creature->getPosition();
            float dist = glm::length(toWaypoint);

            if (dist < 0.1f) return glm::vec3(0.0f);

            float speed = m_config.migrationSpeed * migration.urgency;
            return glm::normalize(toWaypoint) * speed;
        }

        case MigrationPhase::ARRIVING:
        {
            // Slow down approach to destination
            glm::vec3 toDest = migration.destination - creature->getPosition();
            float dist = glm::length(toDest);

            if (dist < 1.0f) return glm::vec3(0.0f);

            // Arrive behavior - slow down as we approach
            float slowingRadius = 20.0f;
            float speed = (dist < slowingRadius) ?
                          m_config.migrationSpeed * (dist / slowingRadius) :
                          m_config.migrationSpeed;

            return glm::normalize(toDest) * speed * 0.5f;
        }

        case MigrationPhase::RESTING:
        case MigrationPhase::SETTLED:
            // Stay near destination
            {
                glm::vec3 toDest = migration.destination - creature->getPosition();
                float dist = glm::length(toDest);

                if (dist > 10.0f && dist > 0.1f) {
                    return glm::normalize(toDest) * 0.3f;
                }
            }
            return glm::vec3(0.0f);

        default:
            return glm::vec3(0.0f);
    }
}

bool MigrationBehavior::isMigrating(uint32_t creatureID) const {
    return m_activeMigrations.count(creatureID) > 0;
}

const MigrationBehavior::Migration* MigrationBehavior::getMigration(uint32_t creatureID) const {
    auto it = m_activeMigrations.find(creatureID);
    if (it == m_activeMigrations.end()) return nullptr;
    return &it->second;
}

bool MigrationBehavior::startMigration(uint32_t creatureID, const glm::vec3& destination,
                                       MigrationTrigger trigger) {
    // Check cooldown
    if (m_migrationCooldowns.count(creatureID) > 0) {
        return false;
    }

    // Already migrating
    if (m_activeMigrations.count(creatureID) > 0) {
        return false;
    }

    Migration migration;
    migration.creatureID = creatureID;
    migration.destination = destination;
    migration.trigger = trigger;
    migration.phase = MigrationPhase::PREPARING;
    migration.startTime = m_currentTime;
    migration.urgency = (trigger == MigrationTrigger::TEMPERATURE_STRESS ||
                         trigger == MigrationTrigger::PREDATOR_PRESSURE) ? 0.9f : 0.6f;

    // Origin will be set when creature starts moving
    migration.origin = glm::vec3(0.0f);  // Will be updated

    m_activeMigrations[creatureID] = migration;
    return true;
}

void MigrationBehavior::cancelMigration(uint32_t creatureID) {
    m_activeMigrations.erase(creatureID);
}

glm::vec3 MigrationBehavior::findMigrationDestination(Creature* creature, MigrationTrigger trigger) {
    if (!creature || !m_biomeSystem || !m_terrain) {
        return creature ? creature->getPosition() : glm::vec3(0.0f);
    }

    glm::vec3 currentPos = creature->getPosition();

    // Get current biome
    BiomeQuery currentBiome = m_biomeSystem->queryBiome(currentPos.x, currentPos.z);

    // For seasonal migration, check known routes
    if (trigger == MigrationTrigger::SEASONAL && m_seasonManager) {
        Season currentSeason = m_seasonManager->getCurrentSeason();

        for (const auto& route : m_knownRoutes) {
            if (route.sourceBiome == currentBiome.biome && route.triggerSeason == currentSeason) {
                // Find this destination biome
                return findSuitableBiome(creature, currentSeason);
            }
        }
    }

    // For resource scarcity, find high-fertility area
    if (trigger == MigrationTrigger::RESOURCE_SCARCITY) {
        // Search in expanding circles for better resources
        static std::random_device rd;
        static std::mt19937 gen(rd());

        float searchRadius = 100.0f;
        glm::vec3 bestDest = currentPos;
        float bestScore = 0.0f;

        for (int i = 0; i < 16; i++) {
            std::uniform_real_distribution<float> angleDist(0.0f, glm::two_pi<float>());
            std::uniform_real_distribution<float> radiusDist(50.0f, searchRadius);

            float angle = angleDist(gen);
            float radius = radiusDist(gen);

            glm::vec3 testPos = currentPos + glm::vec3(
                std::cos(angle) * radius,
                0.0f,
                std::sin(angle) * radius
            );

            // Check biome quality
            BiomeQuery testBiome = m_biomeSystem->queryBiome(testPos.x, testPos.z);
            float score = testBiome.properties.fertility * testBiome.properties.habitability;

            // Bonus for herbivore capacity
            if (isHerbivore(creature->getType())) {
                score += m_biomeSystem->getHerbivoreCapacity(testPos.x, testPos.z) * 0.3f;
            }

            if (score > bestScore) {
                bestScore = score;
                bestDest = testPos;
            }
        }

        return bestDest;
    }

    // For temperature stress, find moderate temperature biome
    if (trigger == MigrationTrigger::TEMPERATURE_STRESS) {
        return findSuitableBiome(creature, m_seasonManager ? m_seasonManager->getCurrentSeason()
                                                          : Season::SUMMER);
    }

    // Default: move toward center of world (usually more resources)
    return glm::vec3(0.0f, 0.0f, 0.0f);
}

void MigrationBehavior::checkSeasonalTriggers(CreatureManager& creatures) {
    if (!m_seasonManager) return;

    Season currentSeason = m_seasonManager->getCurrentSeason();
    float seasonProgress = m_seasonManager->getSeasonProgress();

    // Only trigger near end of season
    if (seasonProgress < m_config.seasonalTriggerThreshold) return;

    creatures.forEach([&](Creature& c, size_t) {
        if (!c.isAlive()) return;
        if (!canMigrate(c.getType())) return;
        if (m_activeMigrations.count(c.getID()) > 0) return;
        if (m_migrationCooldowns.count(c.getID()) > 0) return;

        // Check energy
        if (c.getEnergy() < m_config.minEnergyToMigrate) return;

        // Calculate migration priority
        float priority = calculateMigrationPriority(&c, MigrationTrigger::SEASONAL);

        // Random chance based on priority
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        if (dist(gen) < priority * 0.2f) {  // 20% base chance scaled by priority
            glm::vec3 destination = findMigrationDestination(&c, MigrationTrigger::SEASONAL);
            startMigration(c.getID(), destination, MigrationTrigger::SEASONAL);
        }
    });
}

void MigrationBehavior::checkResourceTriggers(CreatureManager& creatures) {
    // Only check periodically to save performance
    static float lastCheck = 0.0f;
    if (m_currentTime - lastCheck < 5.0f) return;
    lastCheck = m_currentTime;

    creatures.forEach([&](Creature& c, size_t) {
        if (!c.isAlive()) return;
        if (!canMigrate(c.getType())) return;
        if (m_activeMigrations.count(c.getID()) > 0) return;
        if (m_migrationCooldowns.count(c.getID()) > 0) return;

        // Check for low energy (resource scarcity indicator)
        float energyRatio = c.getEnergy() / 200.0f;  // Assume max energy 200
        if (energyRatio < m_config.resourceTriggerThreshold) {
            // Check if creature has been low for a while (not just a temporary dip)
            // Use fitness as a proxy for long-term success
            if (c.getFitness() < 0.3f && c.getEnergy() >= m_config.minEnergyToMigrate * 0.5f) {
                glm::vec3 destination = findMigrationDestination(&c, MigrationTrigger::RESOURCE_SCARCITY);
                startMigration(c.getID(), destination, MigrationTrigger::RESOURCE_SCARCITY);
            }
        }
    });
}

void MigrationBehavior::updateActiveMigrations(float deltaTime, CreatureManager& creatures) {
    for (auto& [creatureID, migration] : m_activeMigrations) {
        Creature* creature = creatures.getCreatureByID(creatureID);
        if (!creature || !creature->isAlive()) {
            m_migrationsToRemove.insert(creatureID);
            continue;
        }

        glm::vec3 pos = creature->getPosition();

        switch (migration.phase) {
            case MigrationPhase::PREPARING:
                // Wait for creature to have enough energy
                if (creature->getEnergy() >= m_config.minEnergyToMigrate) {
                    migration.phase = MigrationPhase::DEPARTING;
                    migration.origin = pos;
                    migration.waypoints = generateWaypoints(migration.origin, migration.destination);
                    migration.waypointIndex = 0;
                    if (!migration.waypoints.empty()) {
                        migration.currentWaypoint = migration.waypoints[0];
                    } else {
                        migration.currentWaypoint = migration.destination;
                    }
                }
                break;

            case MigrationPhase::DEPARTING:
                // Transition to traveling after moving a bit
                {
                    float movedDist = glm::distance(pos, migration.origin);
                    if (movedDist > 20.0f) {
                        migration.phase = MigrationPhase::TRAVELING;
                    }
                }
                // Fall through to traveling logic

            case MigrationPhase::TRAVELING:
                // Update progress
                {
                    float totalDist = glm::distance(migration.origin, migration.destination);
                    float remainingDist = glm::distance(pos, migration.destination);
                    migration.distanceRemaining = remainingDist;
                    migration.progress = (totalDist > 0.0f) ?
                                        1.0f - (remainingDist / totalDist) : 1.0f;

                    // Check if reached current waypoint
                    float waypointDist = glm::distance(pos, migration.currentWaypoint);
                    if (waypointDist < m_config.waypointReachDistance) {
                        migration.waypointIndex++;
                        if (migration.waypointIndex < migration.waypoints.size()) {
                            migration.currentWaypoint = migration.waypoints[migration.waypointIndex];
                        } else {
                            migration.currentWaypoint = migration.destination;
                            migration.phase = MigrationPhase::ARRIVING;
                        }
                    }

                    // Check if close to destination
                    if (remainingDist < 30.0f) {
                        migration.phase = MigrationPhase::ARRIVING;
                    }
                }
                break;

            case MigrationPhase::ARRIVING:
                // Check if reached destination
                {
                    float distToDest = glm::distance(pos, migration.destination);
                    if (distToDest < m_config.waypointReachDistance) {
                        processArrival(migration, creature);
                    }
                }
                break;

            case MigrationPhase::RESTING:
                // Rest at destination
                {
                    float restTime = m_currentTime - migration.startTime;
                    if (restTime > m_config.restDuration) {
                        migration.phase = MigrationPhase::SETTLED;
                        migration.arrived = true;
                        m_completedMigrations++;
                        m_migrationCooldowns[creatureID] = m_config.migrationCooldown;
                        m_migrationsToRemove.insert(creatureID);
                    }
                }
                break;

            case MigrationPhase::SETTLED:
                // Migration complete - remove
                m_migrationsToRemove.insert(creatureID);
                break;

            default:
                break;
        }
    }
}

std::vector<glm::vec3> MigrationBehavior::generateWaypoints(const glm::vec3& origin,
                                                            const glm::vec3& destination) {
    std::vector<glm::vec3> waypoints;

    glm::vec3 direction = destination - origin;
    float totalDist = glm::length(direction);

    if (totalDist < 50.0f) {
        // Short distance - direct path
        return waypoints;
    }

    direction = glm::normalize(direction);
    glm::vec3 perpendicular(-direction.z, 0, direction.x);

    // Generate waypoints with some variation
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> offsetDist(-20.0f, 20.0f);

    int numWaypoints = static_cast<int>(totalDist / 50.0f);
    numWaypoints = glm::clamp(numWaypoints, 1, 10);

    for (int i = 1; i <= numWaypoints; i++) {
        float t = static_cast<float>(i) / (numWaypoints + 1);
        glm::vec3 basePos = origin + direction * totalDist * t;

        // Add some variation
        float offset = offsetDist(gen);
        glm::vec3 waypoint = basePos + perpendicular * offset;

        // Adjust height to terrain
        if (m_terrain) {
            waypoint.y = m_terrain->getHeight(waypoint.x, waypoint.z) + 1.0f;
        }

        waypoints.push_back(waypoint);
    }

    return waypoints;
}

glm::vec3 MigrationBehavior::findSuitableBiome(Creature* creature, Season targetSeason) {
    if (!m_biomeSystem || !creature) {
        return creature ? creature->getPosition() : glm::vec3(0.0f);
    }

    glm::vec3 currentPos = creature->getPosition();

    // Determine target biome based on creature type and season
    BiomeType preferredBiome;

    bool isWinter = (targetSeason == Season::WINTER || targetSeason == Season::FALL);

    if (isFlying(creature->getType())) {
        // Birds migrate to warmer biomes in winter
        preferredBiome = isWinter ? BiomeType::TEMPERATE_FOREST : BiomeType::BOREAL_FOREST;
    } else if (isHerbivore(creature->getType())) {
        // Herbivores follow grass
        preferredBiome = isWinter ? BiomeType::SAVANNA : BiomeType::GRASSLAND;
    } else if (isAquatic(creature->getType())) {
        // Fish migrate between deep and shallow water
        preferredBiome = isWinter ? BiomeType::DEEP_OCEAN : BiomeType::SHALLOW_WATER;
    } else {
        preferredBiome = BiomeType::TEMPERATE_FOREST;  // Default
    }

    // Search for suitable location
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(0.0f, glm::two_pi<float>());
    std::uniform_real_distribution<float> radiusDist(100.0f, 300.0f);

    glm::vec3 bestLocation = currentPos;
    float bestScore = -1.0f;

    for (int i = 0; i < 32; i++) {
        float angle = angleDist(gen);
        float radius = radiusDist(gen);

        glm::vec3 testPos = currentPos + glm::vec3(
            std::cos(angle) * radius,
            0.0f,
            std::sin(angle) * radius
        );

        BiomeQuery biome = m_biomeSystem->queryBiome(testPos.x, testPos.z);

        float score = 0.0f;

        // Score based on biome match
        if (biome.biome == preferredBiome) {
            score += 1.0f;
        }

        // Score based on habitability
        score += biome.properties.habitability * 0.5f;

        // Score based on creature-specific capacity
        if (isHerbivore(creature->getType())) {
            score += m_biomeSystem->getHerbivoreCapacity(testPos.x, testPos.z) * 0.3f;
        } else if (isFlying(creature->getType())) {
            score += m_biomeSystem->getFlyingCapacity(testPos.x, testPos.z) * 0.3f;
        } else if (isAquatic(creature->getType())) {
            score += m_biomeSystem->getAquaticCapacity(testPos.x, testPos.z) * 0.3f;
        }

        if (score > bestScore) {
            bestScore = score;
            bestLocation = testPos;
        }
    }

    // Adjust height to terrain
    if (m_terrain) {
        bestLocation.y = m_terrain->getHeight(bestLocation.x, bestLocation.z) + 1.0f;
    }

    return bestLocation;
}

float MigrationBehavior::calculateMigrationPriority(Creature* creature, MigrationTrigger trigger) {
    if (!creature) return 0.0f;

    float priority = 0.5f;

    switch (trigger) {
        case MigrationTrigger::SEASONAL:
            // Flying creatures are more likely to migrate seasonally
            if (isFlying(creature->getType())) {
                priority += 0.3f;
            }
            break;

        case MigrationTrigger::RESOURCE_SCARCITY:
            // Low energy = high priority
            priority += (1.0f - creature->getEnergy() / 200.0f) * 0.4f;
            break;

        case MigrationTrigger::TEMPERATURE_STRESS:
            priority += 0.4f;  // High priority
            break;

        case MigrationTrigger::PREDATOR_PRESSURE:
            priority += 0.5f;  // Very high priority
            break;

        default:
            break;
    }

    // Age affects migration likelihood (younger creatures less likely)
    if (creature->getAge() < 30.0f) {
        priority *= 0.5f;
    }

    return glm::clamp(priority, 0.0f, 1.0f);
}

bool MigrationBehavior::shouldJoinMigration(Creature* creature, const Migration& nearbyMigration) {
    if (!creature) return false;
    if (m_activeMigrations.count(creature->getID()) > 0) return false;
    if (m_migrationCooldowns.count(creature->getID()) > 0) return false;

    // Same type can join
    Creature* migrator = nullptr;  // Would need CreatureManager to look up
    // For now, return false - this would need access to creature data
    return false;
}

void MigrationBehavior::processArrival(Migration& migration, Creature* creature) {
    migration.phase = MigrationPhase::RESTING;
    migration.arrived = true;
    migration.progress = 1.0f;

    // Mark creature as not migrating (in Creature class)
    if (creature) {
        creature->setMigrating(false);
    }
}
