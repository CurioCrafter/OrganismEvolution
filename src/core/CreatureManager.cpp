#include "CreatureManager.h"
#include "../environment/Terrain.h"
#include "../environment/EcosystemManager.h"
#include "../entities/Creature.h"
#include "../entities/SwimBehavior.h"
#include "../ai/NEATGenome.h"
#include "../ai/CreatureBrainInterface.h"
#include <algorithm>
#include <random>
#include <iostream>
#include <limits>
#include <cmath>

namespace Forge {

// ============================================================================
// Constructor / Destructor
// ============================================================================

CreatureManager::CreatureManager(float worldWidth, float worldDepth)
    : m_worldWidth(worldWidth)
    , m_worldDepth(worldDepth)
    , m_seed(0)
{
    // Pre-allocate creature storage
    m_creatures.reserve(INITIAL_POOL_SIZE);
    m_generations.reserve(INITIAL_POOL_SIZE);
    m_freeIndices.reserve(INITIAL_POOL_SIZE / 4);
    m_queryBuffer.reserve(256);

    // Initialize spatial grids
    m_landGrid = std::make_unique<SpatialGrid>(worldWidth, worldDepth, GRID_RESOLUTION);
    m_waterGrid = std::make_unique<SpatialGrid>(worldWidth, worldDepth, GRID_RESOLUTION);
    m_airGrid = std::make_unique<SpatialGrid>(worldWidth, worldDepth, GRID_RESOLUTION);
    m_globalGrid = std::make_unique<SpatialGrid>(worldWidth, worldDepth, GRID_RESOLUTION);

    // Initialize domain lists
    for (auto& list : m_domainLists) {
        list.reserve(INITIAL_POOL_SIZE / 4);
    }
}

CreatureManager::~CreatureManager() {
    clear();
}

// ============================================================================
// Initialization
// ============================================================================

void CreatureManager::init(Terrain* terrain, EcosystemManager* ecosystem, unsigned int seed) {
    m_terrain = terrain;
    m_ecosystem = ecosystem;
    m_seed = seed;
    clear();
}

void CreatureManager::clear() {
    m_creatures.clear();
    m_generations.clear();
    m_freeIndices.clear();
    m_pendingDeaths.clear();

    for (auto& list : m_domainLists) {
        list.clear();
    }

    m_landGrid->clear();
    m_waterGrid->clear();
    m_airGrid->clear();
    m_globalGrid->clear();

    m_stats.reset();
    m_selectedCreature = CreatureHandle::invalid();
}

// ============================================================================
// Creature Lifecycle
// ============================================================================

CreatureHandle CreatureManager::spawn(CreatureType type, const glm::vec3& position,
                                       const Genome* parentGenome) {
    m_stats.spawnAttempts++;

    // Check 1: Population limit
    if (m_creatures.size() >= MAX_CREATURES && m_freeIndices.empty()) {
        m_stats.spawnFailures++;
        m_stats.failureReasons[static_cast<int>(SpawnFailureReason::POPULATION_LIMIT)]++;
        std::cout << "[SPAWN FAILED] Population limit reached (" << MAX_CREATURES << ") for "
                  << getCreatureTypeName(type) << std::endl;
        return CreatureHandle::invalid();
    }

    // Check 2: Terrain available
    if (!m_terrain) {
        m_stats.spawnFailures++;
        m_stats.failureReasons[static_cast<int>(SpawnFailureReason::NO_TERRAIN)]++;
        std::cerr << "[SPAWN FAILED] Terrain not initialized for " << getCreatureTypeName(type) << std::endl;
        return CreatureHandle::invalid();
    }

    size_t index = allocateSlot();

    // Set position (clamped and validated)
    glm::vec3 validPos = clampToWorld(position);
    validPos.y = getTerrainHeight(validPos);

    // Adjust height based on creature type
    CreatureDomain domain = getDomain(type);
    if (domain == CreatureDomain::AIR) {
        validPos.y += 10.0f + (std::rand() % 30);  // 10-40 units above terrain
    } else if (domain == CreatureDomain::WATER) {
        // Water level is at Y=10.5 (terrain waterLevel=0.35 * heightScale=30)
        float waterLevel = SwimBehavior::getWaterLevelConstant();  // 10.5f

        // Check if this position is actually over water (terrain below water level)
        float terrainHeight = getTerrainHeight(validPos);
        if (terrainHeight < waterLevel) {
            // This is a valid water spawn location
            // Spawn fish between sea floor and water surface
            float waterDepth = waterLevel - terrainHeight;
            float minDepth = 1.0f;  // Don't spawn too close to surface
            float maxDepth = std::max(waterDepth - 1.0f, minDepth + 1.0f);

            // Random depth within water column
            float spawnDepth = minDepth + (static_cast<float>(std::rand()) / RAND_MAX) * (maxDepth - minDepth);
            validPos.y = waterLevel - spawnDepth;

            std::cout << "[AQUATIC SPAWN] Type=" << static_cast<int>(type)
                      << " at (" << validPos.x << ", " << validPos.y << ", " << validPos.z
                      << ") waterLevel=" << waterLevel << " terrainHeight=" << terrainHeight << std::endl;
        } else {
            // Terrain is above water here - find a water position with progressive search
            bool foundWater = false;
            const int maxAttempts = 20;

            for (int attempt = 0; attempt < maxAttempts; ++attempt) {
                // Progressive radius: start small, expand outward
                float searchRadius = 10.0f + (attempt * 10.0f);  // 10, 20, 30... up to 200
                float angle = (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f * 3.14159f;

                float searchX = position.x + cos(angle) * searchRadius;
                float searchZ = position.z + sin(angle) * searchRadius;
                glm::vec3 searchPos = clampToWorld(glm::vec3(searchX, 0.0f, searchZ));
                float searchHeight = getTerrainHeight(searchPos);

                if (searchHeight < waterLevel) {
                    float waterDepth = waterLevel - searchHeight;
                    if (waterDepth >= 1.0f) {  // Minimum depth check
                        validPos.x = searchPos.x;
                        validPos.z = searchPos.z;
                        validPos.y = waterLevel - std::min(5.0f, waterDepth * 0.5f);
                        foundWater = true;
                        std::cout << "[AQUATIC SPAWN] Found water after " << (attempt + 1)
                                  << " attempts (radius=" << searchRadius << ") at ("
                                  << validPos.x << ", " << validPos.y << ", " << validPos.z << ")" << std::endl;
                        break;
                    }
                }
            }

            if (!foundWater) {
                // Failed to find water - abort spawn
                m_stats.spawnFailures++;
                m_stats.failureReasons[static_cast<int>(SpawnFailureReason::NO_WATER_FOUND)]++;
                std::cout << "[SPAWN FAILED] No water found within 200 unit radius for "
                          << getCreatureTypeName(type) << " at ("
                          << position.x << ", " << position.z << "). Try spawning near coastline." << std::endl;
                releaseSlot(index);  // Release allocated slot
                return CreatureHandle::invalid();
            }
        }
    }

    // Create genome
    Genome genome;
    if (parentGenome) {
        genome = *parentGenome;
        genome.mutate(0.1f, 0.2f);  // mutationRate, mutationStrength
    } else {
        // Initialize genome based on type
        if (isAquatic(type)) {
            if (type == CreatureType::AQUATIC_APEX) {
                genome.randomizeShark();
            } else if (isAquaticPredator(type)) {
                genome.randomizeAquaticPredator();
            } else {
                genome.randomizeAquatic();
            }
        } else if (isFlying(type)) {
            if (type == CreatureType::AERIAL_PREDATOR) {
                genome.randomizeAerialPredator();
            } else if (isBirdType(type)) {
                genome.randomizeBird();
            } else if (isInsectType(type)) {
                genome.randomizeInsect();
            } else {
                genome.randomizeFlying();
            }
        } else {
            genome.randomize();
        }
    }

    // Create creature using proper constructor
    auto creature = std::make_unique<Creature>(validPos, genome, type);

    // Store creature
    m_creatures[index] = std::move(creature);

    CreatureHandle handle;
    handle.index = static_cast<uint32_t>(index);
    handle.generation = m_generations[index];

    // PHASE 7 - Agent 3: Initialize transition controller for aquatic/amphibian creatures
    initializeTransitionController(static_cast<uint32_t>(index), type);

    // Update stats
    m_stats.total++;
    m_stats.alive++;
    m_stats.births++;
    m_stats.byType[static_cast<size_t>(type)]++;
    m_stats.byDomain[static_cast<size_t>(domain)]++;

    // PHASE 11 - Agent 8: Track successful spawn
    m_stats.spawnSuccesses++;

    return handle;
}

CreatureHandle CreatureManager::spawnWithGenome(const glm::vec3& position, const Genome& genome) {
    if (m_creatures.size() >= MAX_CREATURES && m_freeIndices.empty()) {
        return CreatureHandle::invalid();
    }

    size_t index = allocateSlot();

    glm::vec3 validPos = clampToWorld(position);
    validPos.y = getTerrainHeight(validPos);

    // Default type - could be inferred from genome
    CreatureType type = CreatureType::GRAZER;

    auto creature = std::make_unique<Creature>(validPos, genome, type);

    m_creatures[index] = std::move(creature);

    CreatureHandle handle;
    handle.index = static_cast<uint32_t>(index);
    handle.generation = m_generations[index];

    m_stats.total++;
    m_stats.alive++;
    m_stats.births++;

    return handle;
}

void CreatureManager::kill(CreatureHandle handle, const std::string& cause) {
    if (!isAlive(handle)) return;

    m_pendingDeaths.push_back({handle.index, cause});
}

bool CreatureManager::isAlive(CreatureHandle handle) const {
    if (handle.index >= m_creatures.size()) return false;
    if (handle.generation != m_generations[handle.index]) return false;
    return m_creatures[handle.index] && m_creatures[handle.index]->isAlive();
}

Creature* CreatureManager::get(CreatureHandle handle) {
    if (!isAlive(handle)) return nullptr;
    return m_creatures[handle.index].get();
}

const Creature* CreatureManager::get(CreatureHandle handle) const {
    if (!isAlive(handle)) return nullptr;
    return m_creatures[handle.index].get();
}

Creature* CreatureManager::getCreatureByID(uint32_t id) {
    // Linear search - O(n), use sparingly
    for (auto& creature : m_creatures) {
        if (creature && creature->isAlive() && static_cast<uint32_t>(creature->getID()) == id) {
            return creature.get();
        }
    }
    return nullptr;
}

const Creature* CreatureManager::getCreatureByID(uint32_t id) const {
    // Linear search - O(n), use sparingly
    for (const auto& creature : m_creatures) {
        if (creature && creature->isAlive() && static_cast<uint32_t>(creature->getID()) == id) {
            return creature.get();
        }
    }
    return nullptr;
}

// ============================================================================
// Spatial Queries
// ============================================================================

const std::vector<Creature*>& CreatureManager::queryNearby(const glm::vec3& position,
                                                            float radius) {
    return m_globalGrid->query(position, radius);
}

const std::vector<Creature*>& CreatureManager::queryNearbyInDomain(const glm::vec3& position,
                                                                    float radius,
                                                                    CreatureDomain domain) {
    switch (domain) {
        case CreatureDomain::LAND:
            return m_landGrid->query(position, radius);
        case CreatureDomain::WATER:
            return m_waterGrid->query(position, radius);
        case CreatureDomain::AIR:
            return m_airGrid->query(position, radius);
        default:
            return m_globalGrid->query(position, radius);
    }
}

const std::vector<Creature*>& CreatureManager::queryNearbyByType(const glm::vec3& position,
                                                                  float radius,
                                                                  CreatureType type) {
    return m_globalGrid->queryByType(position, radius, static_cast<int>(type));
}

Creature* CreatureManager::findNearest(const glm::vec3& position, float maxRadius,
                                        CreatureType typeFilter) {
    return m_globalGrid->findNearest(position, maxRadius, static_cast<int>(typeFilter));
}

Creature* CreatureManager::findNearestPrey(const Creature& predator, float maxRadius) {
    m_queryBuffer.clear();

    const auto& nearby = queryNearby(predator.getPosition(), maxRadius);

    Creature* nearest = nullptr;
    float nearestDistSq = maxRadius * maxRadius;

    for (Creature* candidate : nearby) {
        if (!candidate || !candidate->isAlive()) continue;
        if (candidate == &predator) continue;

        // Check if this creature can be hunted
        float candidateSize = candidate->getGenome().size;
        if (canBeHuntedBy(candidate->getType(), predator.getType(), candidateSize)) {
            glm::vec3 diff = candidate->getPosition() - predator.getPosition();
            float distSq = glm::dot(diff, diff);

            if (distSq < nearestDistSq) {
                nearestDistSq = distSq;
                nearest = candidate;
            }
        }
    }

    return nearest;
}

Creature* CreatureManager::findNearestThreat(const Creature& prey, float maxRadius) {
    const auto& nearby = queryNearby(prey.getPosition(), maxRadius);

    Creature* nearest = nullptr;
    float nearestDistSq = maxRadius * maxRadius;
    float preySize = prey.getGenome().size;

    for (Creature* candidate : nearby) {
        if (!candidate || !candidate->isAlive()) continue;
        if (candidate == &prey) continue;

        // Check if this creature can hunt us
        if (canBeHuntedBy(prey.getType(), candidate->getType(), preySize)) {
            glm::vec3 diff = candidate->getPosition() - prey.getPosition();
            float distSq = glm::dot(diff, diff);

            if (distSq < nearestDistSq) {
                nearestDistSq = distSq;
                nearest = candidate;
            }
        }
    }

    return nearest;
}

// ============================================================================
// Selection and Camera Following
// ============================================================================

void CreatureManager::select(CreatureHandle handle) {
    if (isAlive(handle)) {
        m_selectedCreature = handle;
    }
}

void CreatureManager::selectNearest(const glm::vec3& position) {
    Creature* nearest = m_globalGrid->findNearest(position, 50.0f);
    if (nearest) {
        // Find handle for this creature
        for (size_t i = 0; i < m_creatures.size(); ++i) {
            if (m_creatures[i].get() == nearest) {
                m_selectedCreature.index = static_cast<uint32_t>(i);
                m_selectedCreature.generation = m_generations[i];
                return;
            }
        }
    }
}

void CreatureManager::selectRandom() {
    if (m_stats.alive == 0) return;

    // Collect alive indices
    std::vector<size_t> aliveIndices;
    aliveIndices.reserve(m_stats.alive);

    for (size_t i = 0; i < m_creatures.size(); ++i) {
        if (m_creatures[i] && m_creatures[i]->isAlive()) {
            aliveIndices.push_back(i);
        }
    }

    if (!aliveIndices.empty()) {
        size_t idx = aliveIndices[std::rand() % aliveIndices.size()];
        m_selectedCreature.index = static_cast<uint32_t>(idx);
        m_selectedCreature.generation = m_generations[idx];
    }
}

void CreatureManager::selectNextOfType(CreatureType type) {
    size_t startIdx = m_selectedCreature.isValid() ? m_selectedCreature.index + 1 : 0;

    // Search from current position to end
    for (size_t i = startIdx; i < m_creatures.size(); ++i) {
        if (m_creatures[i] && m_creatures[i]->isAlive() &&
            m_creatures[i]->getType() == type) {
            m_selectedCreature.index = static_cast<uint32_t>(i);
            m_selectedCreature.generation = m_generations[i];
            return;
        }
    }

    // Wrap around to beginning
    for (size_t i = 0; i < startIdx && i < m_creatures.size(); ++i) {
        if (m_creatures[i] && m_creatures[i]->isAlive() &&
            m_creatures[i]->getType() == type) {
            m_selectedCreature.index = static_cast<uint32_t>(i);
            m_selectedCreature.generation = m_generations[i];
            return;
        }
    }
}

void CreatureManager::clearSelection() {
    m_selectedCreature = CreatureHandle::invalid();
}

Creature* CreatureManager::getSelectedCreature() {
    return get(m_selectedCreature);
}

const Creature* CreatureManager::getSelectedCreature() const {
    return get(m_selectedCreature);
}

void CreatureManager::followFittest(CreatureType type) {
    Creature* fittest = nullptr;
    float bestFitness = -1.0f;

    for (auto& creature : m_creatures) {
        if (creature && creature->isAlive() && creature->getType() == type) {
            float fitness = creature->getFitness();
            if (fitness > bestFitness) {
                bestFitness = fitness;
                fittest = creature.get();
            }
        }
    }

    if (fittest) {
        for (size_t i = 0; i < m_creatures.size(); ++i) {
            if (m_creatures[i].get() == fittest) {
                m_selectedCreature.index = static_cast<uint32_t>(i);
                m_selectedCreature.generation = m_generations[i];
                return;
            }
        }
    }
}

void CreatureManager::followOldest(CreatureType type) {
    Creature* oldest = nullptr;
    float maxAge = -1.0f;

    for (auto& creature : m_creatures) {
        if (creature && creature->isAlive() && creature->getType() == type) {
            float age = creature->getAge();
            if (age > maxAge) {
                maxAge = age;
                oldest = creature.get();
            }
        }
    }

    if (oldest) {
        for (size_t i = 0; i < m_creatures.size(); ++i) {
            if (m_creatures[i].get() == oldest) {
                m_selectedCreature.index = static_cast<uint32_t>(i);
                m_selectedCreature.generation = m_generations[i];
                return;
            }
        }
    }
}

// ============================================================================
// Update & Maintenance
// ============================================================================

void CreatureManager::update(float deltaTime) {
    // Process pending deaths
    for (const auto& [index, cause] : m_pendingDeaths) {
        if (index < m_creatures.size() && m_creatures[index]) {
            Creature* creature = m_creatures[index].get();

            // Notify ecosystem of death (creates corpse)
            if (m_ecosystem) {
                m_ecosystem->onCreatureDeath(*creature);
            }

            // Update stats
            m_stats.alive--;
            m_stats.deaths++;
            m_stats.byType[static_cast<size_t>(creature->getType())]--;
            m_stats.byDomain[static_cast<size_t>(getDomain(creature->getType()))]--;

            // Release the creature (it will be marked as dead)
            releaseSlot(index);
        }
    }
    m_pendingDeaths.clear();

    // Check for natural death (creatures that died during update)
    for (size_t i = 0; i < m_creatures.size(); ++i) {
        if (m_creatures[i] && !m_creatures[i]->isAlive()) {
            // Already dead, queue for removal
            m_pendingDeaths.push_back({i, "natural"});
        }
    }

    // Update statistics
    updateStats();
}

void CreatureManager::rebuildSpatialGrids() {
    // Clear all grids
    m_landGrid->clear();
    m_waterGrid->clear();
    m_airGrid->clear();
    m_globalGrid->clear();

    // Rebuild domain lists
    rebuildDomainLists();

    // Insert into appropriate grids
    for (auto& creature : m_creatures) {
        if (creature && creature->isAlive()) {
            m_globalGrid->insert(creature.get());

            CreatureDomain domain = getDomain(creature->getType());
            switch (domain) {
                case CreatureDomain::LAND:
                    m_landGrid->insert(creature.get());
                    break;
                case CreatureDomain::WATER:
                    m_waterGrid->insert(creature.get());
                    break;
                case CreatureDomain::AIR:
                    m_airGrid->insert(creature.get());
                    break;
                case CreatureDomain::AMPHIBIOUS:
                    // Add to both land and water grids
                    m_landGrid->insert(creature.get());
                    m_waterGrid->insert(creature.get());
                    break;
                default:
                    break;
            }
        }
    }
}

void CreatureManager::cleanup() {
    // Compact the creature list by removing dead creatures
    size_t writeIdx = 0;

    for (size_t readIdx = 0; readIdx < m_creatures.size(); ++readIdx) {
        if (m_creatures[readIdx] && m_creatures[readIdx]->isAlive()) {
            if (writeIdx != readIdx) {
                m_creatures[writeIdx] = std::move(m_creatures[readIdx]);
                m_generations[writeIdx] = m_generations[readIdx];
            }
            ++writeIdx;
        }
    }

    m_creatures.resize(writeIdx);
    m_generations.resize(writeIdx);
    m_freeIndices.clear();

    // Rebuild grids after compaction
    rebuildSpatialGrids();
}

void CreatureManager::cullToLimit(size_t maxCount) {
    if (m_stats.alive <= static_cast<int>(maxCount)) return;

    // Sort by fitness and remove weakest
    std::vector<std::pair<float, size_t>> fitnessIndices;
    fitnessIndices.reserve(m_stats.alive);

    for (size_t i = 0; i < m_creatures.size(); ++i) {
        if (m_creatures[i] && m_creatures[i]->isAlive()) {
            fitnessIndices.push_back({m_creatures[i]->getFitness(), i});
        }
    }

    // Sort by fitness (ascending - weakest first)
    std::sort(fitnessIndices.begin(), fitnessIndices.end());

    // Kill the weakest to get down to limit
    size_t toKill = m_stats.alive - maxCount;
    for (size_t i = 0; i < toKill && i < fitnessIndices.size(); ++i) {
        size_t idx = fitnessIndices[i].second;
        m_pendingDeaths.push_back({idx, "culled"});
    }
}

void CreatureManager::cullWeakest(CreatureType type, size_t targetCount) {
    std::vector<std::pair<float, size_t>> fitnessIndices;

    for (size_t i = 0; i < m_creatures.size(); ++i) {
        if (m_creatures[i] && m_creatures[i]->isAlive() &&
            m_creatures[i]->getType() == type) {
            fitnessIndices.push_back({m_creatures[i]->getFitness(), i});
        }
    }

    if (fitnessIndices.size() <= targetCount) return;

    std::sort(fitnessIndices.begin(), fitnessIndices.end());

    size_t toKill = fitnessIndices.size() - targetCount;
    for (size_t i = 0; i < toKill; ++i) {
        m_pendingDeaths.push_back({fitnessIndices[i].second, "culled_by_type"});
    }
}

// ============================================================================
// Statistics
// ============================================================================

int CreatureManager::getPopulation(CreatureType type) const {
    return m_stats.byType[static_cast<size_t>(type)];
}

int CreatureManager::getPopulation(CreatureDomain domain) const {
    return m_stats.byDomain[static_cast<size_t>(domain)];
}

// ============================================================================
// Reproduction
// ============================================================================

CreatureHandle CreatureManager::reproduce(CreatureHandle parent1, CreatureHandle parent2) {
    Creature* p1 = get(parent1);
    Creature* p2 = get(parent2);

    if (!p1 || !p2) return CreatureHandle::invalid();

    // Spawn near first parent with crossover genome
    glm::vec3 offset(
        (std::rand() % 100 - 50) * 0.1f,
        0.0f,
        (std::rand() % 100 - 50) * 0.1f
    );

    glm::vec3 spawnPos = p1->getPosition() + offset;

    // Create child with parent genomes (uses Creature's crossover constructor)
    if (m_creatures.size() >= MAX_CREATURES && m_freeIndices.empty()) {
        return CreatureHandle::invalid();
    }

    size_t index = allocateSlot();

    glm::vec3 validPos = clampToWorld(spawnPos);
    validPos.y = getTerrainHeight(validPos);

    auto creature = std::make_unique<Creature>(validPos, p1->getGenome(), p2->getGenome(), p1->getType());
    creature->setGeneration(std::max(p1->getGeneration(), p2->getGeneration()) + 1);

    // === NEAT BRAIN CROSSOVER ===
    // If both parents have NEAT brains, perform NEAT crossover on their genomes
    if (p1->hasNEATBrain() && p2->hasNEATBrain()) {
        static std::mt19937 rng(std::random_device{}());

        // Get parent genomes
        const ai::NEATGenome& genome1 = p1->getNEATGenome();
        const ai::NEATGenome& genome2 = p2->getNEATGenome();

        // Fitter parent is primary for crossover
        ai::NEATGenome childGenome;
        if (p1->getFitness() >= p2->getFitness()) {
            childGenome = ai::NEATGenome::crossover(genome1, genome2, rng);
        } else {
            childGenome = ai::NEATGenome::crossover(genome2, genome1, rng);
        }

        // Apply mutation to child
        ai::MutationParams mutParams;
        mutParams.addConnectionProb = 0.08f;  // Slightly higher for exploration
        mutParams.addNodeProb = 0.05f;
        mutParams.mutateWeightProb = 0.8f;
        mutParams.weightPerturbStrength = 0.3f;
        childGenome.mutate(rng, mutParams);

        // Set the child's brain to use the crossed-over genome
        creature->initializeNEATBrain(childGenome);
    }

    m_creatures[index] = std::move(creature);

    CreatureHandle handle;
    handle.index = static_cast<uint32_t>(index);
    handle.generation = m_generations[index];

    m_stats.total++;
    m_stats.alive++;
    m_stats.births++;
    m_stats.byType[static_cast<size_t>(p1->getType())]++;
    m_stats.byDomain[static_cast<size_t>(getDomain(p1->getType()))]++;

    return handle;
}

CreatureHandle CreatureManager::reproduceAsexual(CreatureHandle parent) {
    Creature* p = get(parent);
    if (!p) return CreatureHandle::invalid();

    glm::vec3 offset(
        (std::rand() % 100 - 50) * 0.1f,
        0.0f,
        (std::rand() % 100 - 50) * 0.1f
    );

    return spawn(p->getType(), p->getPosition() + offset, &p->getGenome());
}

bool CreatureManager::canReproduce(CreatureHandle handle) const {
    const Creature* creature = get(handle);
    if (!creature) return false;

    return creature->canReproduce();
}

// ============================================================================
// Private Helpers
// ============================================================================

size_t CreatureManager::allocateSlot() {
    if (!m_freeIndices.empty()) {
        size_t index = m_freeIndices.back();
        m_freeIndices.pop_back();
        m_generations[index]++;
        return index;
    }

    size_t index = m_creatures.size();
    m_creatures.push_back(nullptr);
    m_generations.push_back(1);
    return index;
}

void CreatureManager::releaseSlot(size_t index) {
    if (index < m_creatures.size()) {
        m_creatures[index].reset();
        m_freeIndices.push_back(static_cast<uint32_t>(index));
    }
}

void CreatureManager::updateStats() {
    float totalEnergy = 0.0f;
    float totalAge = 0.0f;
    float totalFitness = 0.0f;
    float totalBrainComplexity = 0.0f;
    float bestFit = 0.0f;
    float minFit = std::numeric_limits<float>::max();
    std::vector<float> fitnessValues;
    int count = 0;
    int maxGeneration = 0;

    for (const auto& creature : m_creatures) {
        if (creature && creature->isAlive()) {
            float fit = creature->getFitness();
            totalEnergy += creature->getEnergy();
            totalAge += creature->getAge();
            totalFitness += fit;
            fitnessValues.push_back(fit);

            if (fit > bestFit) bestFit = fit;
            if (fit < minFit) minFit = fit;

            // Track generation
            if (creature->getGeneration() > maxGeneration) {
                maxGeneration = creature->getGeneration();
            }

            // Track brain complexity if using NEAT
            if (creature->hasNEATBrain()) {
                totalBrainComplexity += creature->getNEATGenome().getComplexity();
            }

            ++count;
        }
    }

    if (count > 0) {
        m_stats.avgEnergy = totalEnergy / count;
        m_stats.avgAge = totalAge / count;
        m_stats.avgFitness = totalFitness / count;
        m_stats.bestFitness = bestFit;
        m_stats.minFitness = minFit;
        m_stats.avgBrainComplexity = totalBrainComplexity / count;

        // Calculate standard deviation
        float variance = 0.0f;
        for (float fit : fitnessValues) {
            float diff = fit - m_stats.avgFitness;
            variance += diff * diff;
        }
        m_stats.fitnessStdDev = std::sqrt(variance / count);

        // Track generation changes
        int previousGen = m_stats.currentGeneration;
        m_stats.currentGeneration = maxGeneration;

        // Log evolution progress when generation changes
        if (maxGeneration > previousGen && maxGeneration % 10 == 0) {
            std::cout << "\n=== EVOLUTION PROGRESS (Generation " << maxGeneration << ") ===" << std::endl;
            std::cout << "  Population: " << count << std::endl;
            std::cout << "  Avg Fitness: " << m_stats.avgFitness << std::endl;
            std::cout << "  Best Fitness: " << m_stats.bestFitness << std::endl;
            std::cout << "  Fitness StdDev: " << m_stats.fitnessStdDev << std::endl;
            std::cout << "  Avg Brain Complexity: " << m_stats.avgBrainComplexity << std::endl;
            std::cout << "=============================================\n" << std::endl;
        }
    }
}

void CreatureManager::rebuildDomainLists() {
    for (auto& list : m_domainLists) {
        list.clear();
    }

    for (auto& creature : m_creatures) {
        if (creature && creature->isAlive()) {
            CreatureDomain domain = getDomain(creature->getType());
            m_domainLists[static_cast<size_t>(domain)].push_back(creature.get());

            // Amphibious creatures appear in both land and water lists
            if (domain == CreatureDomain::AMPHIBIOUS) {
                m_domainLists[static_cast<size_t>(CreatureDomain::LAND)].push_back(creature.get());
                m_domainLists[static_cast<size_t>(CreatureDomain::WATER)].push_back(creature.get());
            }
        }
    }
}

float CreatureManager::getTerrainHeight(const glm::vec3& position) const {
    if (!m_terrain) {
        // PHASE 11 - Agent 8: Log terrain access without initialization
        static bool warningShown = false;
        if (!warningShown) {
            std::cerr << "[TERRAIN WARNING] Terrain not initialized, returning default height 0.0f" << std::endl;
            warningShown = true;
        }
        return 0.0f;
    }
    return m_terrain->getHeight(position.x, position.z);
}

glm::vec3 CreatureManager::clampToWorld(const glm::vec3& position) const {
    float halfWidth = m_worldWidth * 0.5f;
    float halfDepth = m_worldDepth * 0.5f;

    return glm::vec3(
        glm::clamp(position.x, -halfWidth, halfWidth),
        position.y,
        glm::clamp(position.z, -halfDepth, halfDepth)
    );
}

bool CreatureManager::isValidPosition(const glm::vec3& position, CreatureType type) const {
    // Check world bounds
    float halfWidth = m_worldWidth * 0.5f;
    float halfDepth = m_worldDepth * 0.5f;

    if (position.x < -halfWidth || position.x > halfWidth ||
        position.z < -halfDepth || position.z > halfDepth) {
        return false;
    }

    // Check domain-specific constraints
    float terrainHeight = getTerrainHeight(position);
    float waterLevel = SwimBehavior::getWaterLevelConstant();  // 10.5f

    CreatureDomain domain = getDomain(type);

    if (domain == CreatureDomain::WATER) {
        // Aquatic creatures must be in water (terrain below water level AND creature below water surface)
        bool terrainIsWater = terrainHeight < waterLevel;
        bool creatureInWater = position.y < waterLevel && position.y > terrainHeight;
        return terrainIsWater && creatureInWater;
    }

    if (domain == CreatureDomain::LAND) {
        // Land creatures must be on terrain that's above water
        return terrainHeight >= waterLevel;
    }

    // Flying and amphibious can be anywhere
    return true;
}

// ============================================================================
// Amphibious Transition System (PHASE 7 - Agent 3)
// ============================================================================

EnvironmentZone CreatureManager::determineEnvironmentZone(const glm::vec3& position, float waterLevel) const {
    float terrainHeight = getTerrainHeight(position);
    float creatureHeight = position.y;
    float waterDepth = waterLevel - creatureHeight;
    float distanceFromShore = waterLevel - terrainHeight;

    // Above water surface
    if (creatureHeight > waterLevel) {
        if (distanceFromShore < 5.0f && terrainHeight < waterLevel) {
            return EnvironmentZone::SHORE;  // Near water's edge
        }
        return EnvironmentZone::LAND;
    }

    // Below water surface
    if (waterDepth > 5.0f) {
        return EnvironmentZone::DEEP_WATER;
    }
    if (waterDepth > 1.0f) {
        return EnvironmentZone::SHALLOW_WATER;
    }

    // Very shallow or at surface
    return EnvironmentZone::SHORE;
}

void CreatureManager::initializeTransitionController(uint32_t creatureIndex, CreatureType type) {
    if (!isAquatic(type) && type != CreatureType::AMPHIBIAN) {
        return;  // Only aquatic and amphibian creatures get transition controllers
    }

    AmphibiousTransitionController controller;

    if (type == CreatureType::AMPHIBIAN) {
        controller.initializeAsAmphibian();
    } else if (isAquatic(type)) {
        // Pure aquatic creatures start fully aquatic
        controller.initialize(0.0f, 0.0f, AmphibiousStage::FULLY_AQUATIC);
    }

    if (m_amphibiousDebugEnabled) {
        controller.setDebugLogging(true);
    }

    m_transitionControllers[creatureIndex] = std::move(controller);
}

void CreatureManager::updateAmphibiousTransitions(float deltaTime, float waterLevel) {
    // Update global cooldown
    if (m_transitionCooldownTimer > 0.0f) {
        m_transitionCooldownTimer -= deltaTime;
    }

    m_stats.transitionsThisFrame = 0;
    float totalProgress = 0.0f;
    int transitioningCount = 0;

    // Reset stage counts
    m_stats.byAmphibiousStage.fill(0);

    // Iterate through creatures with transition controllers
    for (auto& [index, controller] : m_transitionControllers) {
        if (index >= m_creatures.size() || !m_creatures[index] || !m_creatures[index]->isAlive()) {
            continue;
        }

        Creature* creature = m_creatures[index].get();
        const glm::vec3& pos = creature->getPosition();

        // Determine current environment
        EnvironmentZone zone = determineEnvironmentZone(pos, waterLevel);
        float waterDepth = waterLevel - pos.y;
        float terrainHeight = getTerrainHeight(pos);
        float distanceToShore = std::abs(pos.y - waterLevel) + std::abs(terrainHeight - waterLevel);
        float oxygenAvailability = (zone == EnvironmentZone::LAND || zone == EnvironmentZone::SHORE) ? 1.0f : 0.8f;

        // Update transition controller
        AmphibiousUpdateResult result = controller.update(
            deltaTime,
            zone,
            waterDepth,
            distanceToShore,
            oxygenAvailability,
            creature->getAge()
        );

        // Track stage statistics
        int stageIndex = static_cast<int>(controller.getCurrentStage());
        if (stageIndex >= 0 && stageIndex < 4) {
            m_stats.byAmphibiousStage[stageIndex]++;
        }

        // Track transition progress
        if (controller.getCurrentStage() == AmphibiousStage::TRANSITIONING) {
            totalProgress += controller.getTransitionProgress();
            transitioningCount++;
        }

        // Handle stage change
        if (result.stageChanged) {
            // Check if we've hit the max transitions limit
            if (m_stats.transitionsThisFrame >= m_maxTransitionsPerFrame) {
                // Revert the change - too many transitions this frame
                controller.forceStage(result.previousStage);
                continue;
            }

            m_stats.transitionsThisFrame++;
            m_stats.totalTransitions++;

            if (m_amphibiousDebugEnabled && !result.debugMessage.empty()) {
                std::cout << result.debugMessage << " (Creature ID: " << creature->getID() << ")" << std::endl;
            }

            // Update creature's animation blend
            creature->getAnimator().setAmphibiousBlend(controller.getLocomotionBlend());
        }

        // Apply environmental penalties
        if (result.energyDrain > 0.0f) {
            creature->addEnergy(-result.energyDrain);
        }

        if (result.healthDamage > 0.0f) {
            creature->takeDamage(result.healthDamage);
        }
    }

    // Update average progress
    if (transitioningCount > 0) {
        m_stats.avgTransitionProgress = totalProgress / transitioningCount;
    } else {
        m_stats.avgTransitionProgress = 0.0f;
    }
}

AmphibiousTransitionController* CreatureManager::getTransitionController(CreatureHandle handle) {
    if (!isAlive(handle)) return nullptr;

    auto it = m_transitionControllers.find(handle.index);
    if (it != m_transitionControllers.end()) {
        return &it->second;
    }
    return nullptr;
}

const AmphibiousTransitionController* CreatureManager::getTransitionController(CreatureHandle handle) const {
    if (!isAlive(handle)) return nullptr;

    auto it = m_transitionControllers.find(handle.index);
    if (it != m_transitionControllers.end()) {
        return &it->second;
    }
    return nullptr;
}

void CreatureManager::forceTransitionStage(CreatureHandle handle, AmphibiousStage stage) {
    AmphibiousTransitionController* controller = getTransitionController(handle);
    if (controller) {
        controller->forceStage(stage);

        // Update animation blend
        Creature* creature = get(handle);
        if (creature) {
            creature->getAnimator().setAmphibiousBlend(controller->getLocomotionBlend());
        }

        if (m_amphibiousDebugEnabled) {
            std::cout << "[AMPHIBIOUS] Forced creature " << handle.index
                      << " to stage: " << getAmphibiousStageName(stage) << std::endl;
        }
    }
}

int CreatureManager::getAmphibiousStageCount(AmphibiousStage stage) const {
    int stageIndex = static_cast<int>(stage);
    if (stageIndex >= 0 && stageIndex < 4) {
        return m_stats.byAmphibiousStage[stageIndex];
    }
    return 0;
}

} // namespace Forge
