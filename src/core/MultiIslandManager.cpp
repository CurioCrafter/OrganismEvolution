#include "MultiIslandManager.h"
#include "../environment/IslandGenerator.h"
#include "../environment/ClimateSystem.h"
#include "../graphics/Camera.h"
#include <algorithm>
#include <cmath>

namespace Forge {

// ============================================================================
// Constructor/Destructor
// ============================================================================

MultiIslandManager::MultiIslandManager() {
    m_islands.reserve(MAX_ISLANDS);
}

MultiIslandManager::~MultiIslandManager() {
    clear();
}

// ============================================================================
// Initialization
// ============================================================================

void MultiIslandManager::init(const ArchipelagoGenerator& archipelago) {
    clear();

    // Store archipelago data
    m_archipelagoData = archipelago.getArchipelagoData();

    // Create islands from configurations
    const auto& configs = archipelago.getIslandConfigs();

    for (uint32_t i = 0; i < static_cast<uint32_t>(configs.size()); ++i) {
        createIsland(configs[i], i);
    }

    // Set first island as active
    if (!m_islands.empty()) {
        setActiveIsland(0);
    }
}

void MultiIslandManager::initializeDX12(DX12Device* device, ID3D12PipelineState* terrainPSO,
                                         ID3D12RootSignature* rootSig) {
    m_dx12Device = device;
    m_terrainPSO = terrainPSO;
    m_rootSignature = rootSig;

    // Initialize DX12 resources for all island terrains
    for (auto& island : m_islands) {
        if (island.terrain) {
            island.terrain->initializeDX12(device, terrainPSO, rootSig);
        }
    }
}

void MultiIslandManager::generateAll(unsigned int baseSeed) {
    for (size_t i = 0; i < m_islands.size(); ++i) {
        auto& island = m_islands[i];

        // Generate terrain
        generateIslandTerrain(island);

        // Populate with creatures
        populateIsland(island, baseSeed + static_cast<unsigned int>(i * 10000));

        island.isLoaded = true;
    }

    // Update all statistics
    updateStatistics();
}

void MultiIslandManager::clear() {
    for (auto& island : m_islands) {
        island.terrain.reset();
        island.creatures.reset();
        island.vegetation.reset();
    }
    m_islands.clear();
    m_activeIslandIndex = 0;
    m_recentEvents.clear();
    m_globalStatsDirty = true;
}

void MultiIslandManager::createIsland(const IslandConfig& config, uint32_t index) {
    Island island;
    island.id = index;
    island.name = config.name;
    island.worldPosition = config.position;
    island.size = config.size;
    island.config = config;

    // Calculate terrain size based on island size
    int terrainSize = static_cast<int>(m_terrainSize * config.size);
    terrainSize = std::max(64, std::min(512, terrainSize));  // Clamp to reasonable range

    // Create terrain
    island.terrain = std::make_unique<Terrain>(terrainSize, terrainSize, m_terrainScale);

    // Create creature manager with island-specific world size
    float worldSize = terrainSize * m_terrainScale;
    island.creatures = std::make_unique<CreatureManager>(worldSize, worldSize);

    // Create vegetation manager
    island.vegetation = std::make_unique<VegetationManager>(island.terrain.get());

    m_islands.push_back(std::move(island));
}

void MultiIslandManager::generateIslandTerrain(Island& island) {
    if (!island.terrain) return;

    // Generate terrain with island-specific seed
    island.terrain->generate(island.config.seed);

    // Generate vegetation
    if (island.vegetation) {
        island.vegetation->generate(island.config.seed + 1);
    }
}

void MultiIslandManager::populateIsland(Island& island, unsigned int seed) {
    if (!island.terrain || !island.creatures) return;

    // Initialize creature manager
    island.creatures->init(island.terrain.get(), nullptr, seed);

    // Calculate number of creatures based on island size
    int basePopulation = static_cast<int>(50 * island.size);

    // Spawn initial population
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> posDist(0.1f, 0.9f);

    float worldWidth = island.terrain->getWidth() * island.terrain->getScale();
    float worldDepth = island.terrain->getDepth() * island.terrain->getScale();

    // Spawn herbivores (majority)
    int herbivoreCount = static_cast<int>(basePopulation * 0.7f);
    for (int i = 0; i < herbivoreCount; ++i) {
        float x = posDist(rng) * worldWidth - worldWidth * 0.5f;
        float z = posDist(rng) * worldDepth - worldDepth * 0.5f;
        float y = island.terrain->getHeight(x, z);

        if (!island.terrain->isWater(x, z)) {
            island.creatures->spawn(CreatureType::HERBIVORE, glm::vec3(x, y, z), nullptr);
        }
    }

    // Spawn carnivores (minority)
    int carnivoreCount = static_cast<int>(basePopulation * 0.15f);
    for (int i = 0; i < carnivoreCount; ++i) {
        float x = posDist(rng) * worldWidth - worldWidth * 0.5f;
        float z = posDist(rng) * worldDepth - worldDepth * 0.5f;
        float y = island.terrain->getHeight(x, z);

        if (!island.terrain->isWater(x, z)) {
            island.creatures->spawn(CreatureType::CARNIVORE, glm::vec3(x, y, z), nullptr);
        }
    }

    // Spawn aquatic creatures
    int aquaticCount = static_cast<int>(basePopulation * 0.15f);
    for (int i = 0; i < aquaticCount; ++i) {
        float x = posDist(rng) * worldWidth - worldWidth * 0.5f;
        float z = posDist(rng) * worldDepth - worldDepth * 0.5f;
        float y = island.terrain->getHeight(x, z);

        if (island.terrain->isWater(x, z)) {
            island.creatures->spawn(CreatureType::FISH, glm::vec3(x, y - 1.0f, z), nullptr);
        }
    }
}

// ============================================================================
// Island Access
// ============================================================================

Island* MultiIslandManager::getIsland(uint32_t index) {
    if (index >= m_islands.size()) return nullptr;
    return &m_islands[index];
}

const Island* MultiIslandManager::getIsland(uint32_t index) const {
    if (index >= m_islands.size()) return nullptr;
    return &m_islands[index];
}

Island* MultiIslandManager::getActiveIsland() {
    return getIsland(m_activeIslandIndex);
}

const Island* MultiIslandManager::getActiveIsland() const {
    return getIsland(m_activeIslandIndex);
}

void MultiIslandManager::setActiveIsland(uint32_t index) {
    if (index >= m_islands.size()) return;

    uint32_t previousActive = m_activeIslandIndex;
    m_activeIslandIndex = index;

    // Mark islands active/inactive
    for (size_t i = 0; i < m_islands.size(); ++i) {
        m_islands[i].isActive = (i == index);
    }

    // Emit events
    if (previousActive != index) {
        if (previousActive < m_islands.size()) {
            IslandEvent deactivateEvent;
            deactivateEvent.type = IslandEvent::Type::ISLAND_DEACTIVATED;
            deactivateEvent.islandId = previousActive;
            deactivateEvent.description = m_islands[previousActive].name + " deactivated";
            deactivateEvent.timestamp = m_totalTime;
            emitEvent(deactivateEvent);
        }

        IslandEvent activateEvent;
        activateEvent.type = IslandEvent::Type::ISLAND_ACTIVATED;
        activateEvent.islandId = index;
        activateEvent.description = m_islands[index].name + " activated";
        activateEvent.timestamp = m_totalTime;
        emitEvent(activateEvent);
    }
}

int MultiIslandManager::findIslandAt(const glm::vec3& worldPos) const {
    glm::vec2 pos2D(worldPos.x, worldPos.z);

    for (size_t i = 0; i < m_islands.size(); ++i) {
        const auto& island = m_islands[i];

        if (!island.terrain) continue;

        // Calculate island bounds
        float halfWidth = island.terrain->getWidth() * island.terrain->getScale() * 0.5f;
        float halfDepth = island.terrain->getDepth() * island.terrain->getScale() * 0.5f;

        glm::vec2 minBound = island.worldPosition - glm::vec2(halfWidth, halfDepth);
        glm::vec2 maxBound = island.worldPosition + glm::vec2(halfWidth, halfDepth);

        if (pos2D.x >= minBound.x && pos2D.x <= maxBound.x &&
            pos2D.y >= minBound.y && pos2D.y <= maxBound.y) {
            return static_cast<int>(i);
        }
    }

    return -1;  // Not on any island
}

// ============================================================================
// Update
// ============================================================================

void MultiIslandManager::update(float deltaTime) {
    m_totalTime += deltaTime;
    m_globalStatsDirty = true;

    for (size_t i = 0; i < m_islands.size(); ++i) {
        auto& island = m_islands[i];

        if (!island.isLoaded) continue;

        // Always update active island
        if (island.isActive && m_alwaysUpdateActive) {
            updateIsland(static_cast<uint32_t>(i), deltaTime);
        }
        // Update inactive islands at reduced rate
        else if (island.needsUpdate) {
            // Update at 1/4 rate for inactive islands
            static float accumulatedTime = 0.0f;
            accumulatedTime += deltaTime;

            if (accumulatedTime > 0.1f) {  // Update every 100ms
                updateIsland(static_cast<uint32_t>(i), accumulatedTime);
                accumulatedTime = 0.0f;
            }
        }
    }
}

void MultiIslandManager::updateIsland(uint32_t index, float deltaTime) {
    if (index >= m_islands.size()) return;

    auto& island = m_islands[index];

    if (!island.creatures) return;

    // Store previous stats for event detection
    int prevCreatureCount = island.stats.totalCreatures;

    // Update creature manager
    island.creatures->update(deltaTime);

    // Update statistics
    updateIslandStats(island);

    // Detect significant events
    int newCount = island.stats.totalCreatures;

    // Population boom (>50% increase)
    if (newCount > prevCreatureCount * 1.5f && prevCreatureCount > 10) {
        IslandEvent event;
        event.type = IslandEvent::Type::POPULATION_BOOM;
        event.islandId = index;
        event.description = island.name + " population boom!";
        event.timestamp = m_totalTime;
        emitEvent(event);
    }

    // Population crash (>50% decrease)
    if (newCount < prevCreatureCount * 0.5f && prevCreatureCount > 10) {
        IslandEvent event;
        event.type = IslandEvent::Type::POPULATION_CRASH;
        event.islandId = index;
        event.description = island.name + " population crash!";
        event.timestamp = m_totalTime;
        emitEvent(event);
    }
}

void MultiIslandManager::updateStatistics() {
    for (auto& island : m_islands) {
        if (island.isLoaded) {
            updateIslandStats(island);
        }
    }
    m_globalStatsDirty = true;
}

void MultiIslandManager::updateIslandStats(Island& island) {
    island.stats.reset();

    if (!island.creatures) return;

    const auto& popStats = island.creatures->getStats();

    island.stats.totalCreatures = popStats.alive;
    island.stats.avgFitness = popStats.avgFitness;
    island.stats.avgEnergy = popStats.avgEnergy;
    island.stats.births = popStats.births;
    island.stats.deaths = popStats.deaths;

    // Calculate genetic diversity (simplified)
    island.stats.geneticDiversity = calculateGeneticDiversity(island);

    // Count unique species (approximation based on genome clusters)
    // In a full implementation, this would use the SpeciationTracker
    island.stats.speciesCount = std::max(1, island.stats.totalCreatures / 20);

    // Vegetation density
    if (island.vegetation) {
        island.stats.vegetationDensity = static_cast<float>(
            island.vegetation->getTreeInstances().size() +
            island.vegetation->getBushInstances().size()
        ) / 1000.0f;
    }
}

float MultiIslandManager::calculateGeneticDiversity(const Island& island) const {
    if (!island.creatures) return 0.0f;

    const auto& creatures = island.creatures->getAllCreatures();
    if (creatures.empty()) return 0.0f;

    // Calculate variance in key genome traits
    float sumSize = 0.0f, sumSizeSq = 0.0f;
    float sumSpeed = 0.0f, sumSpeedSq = 0.0f;
    int count = 0;

    for (const auto& creature : creatures) {
        if (!creature || !creature->isAlive()) continue;

        const auto& genome = creature->getGenome();

        sumSize += genome.size;
        sumSizeSq += genome.size * genome.size;
        sumSpeed += genome.speed;
        sumSpeedSq += genome.speed * genome.speed;

        count++;
    }

    if (count < 2) return 0.0f;

    float n = static_cast<float>(count);

    // Variance = E[X^2] - E[X]^2
    float varSize = (sumSizeSq / n) - (sumSize / n) * (sumSize / n);
    float varSpeed = (sumSpeedSq / n) - (sumSpeed / n) * (sumSpeed / n);

    // Normalize and combine
    float diversity = std::sqrt(varSize) / 0.5f + std::sqrt(varSpeed) / 5.0f;
    return std::min(1.0f, diversity);
}

// ============================================================================
// Rendering
// ============================================================================

void MultiIslandManager::render(const Camera& camera, ID3D12GraphicsCommandList* commandList) {
    if (!commandList) return;

    // For now, render all loaded islands
    // In a full implementation, this would use frustum culling and LOD
    for (size_t i = 0; i < m_islands.size(); ++i) {
        renderIsland(static_cast<uint32_t>(i), commandList);
    }
}

void MultiIslandManager::renderIsland(uint32_t index, ID3D12GraphicsCommandList* commandList) {
    if (index >= m_islands.size()) return;

    auto& island = m_islands[index];

    if (!island.isLoaded || !island.terrain) return;

    // Render terrain
    island.terrain->render(commandList);

    // Note: Creature and vegetation rendering would be handled by a separate renderer
    // that takes the island's world offset into account
}

void MultiIslandManager::renderForShadow(const Camera& camera, ID3D12GraphicsCommandList* commandList) {
    if (!commandList) return;

    // Only render active island for shadow maps (performance optimization)
    auto* active = getActiveIsland();
    if (active && active->terrain) {
        active->terrain->renderForShadow(commandList);
    }
}

// ============================================================================
// Creature Management
// ============================================================================

int MultiIslandManager::getTotalCreatureCount() const {
    int total = 0;
    for (const auto& island : m_islands) {
        if (island.creatures) {
            total += island.creatures->getTotalPopulation();
        }
    }
    return total;
}

int MultiIslandManager::getCreatureCount(uint32_t islandIndex) const {
    const auto* island = getIsland(islandIndex);
    if (!island || !island->creatures) return 0;
    return island->creatures->getTotalPopulation();
}

CreatureHandle MultiIslandManager::spawnCreature(uint32_t islandIndex, CreatureType type,
                                                   const glm::vec3& localPosition,
                                                   const Genome* parentGenome) {
    auto* island = getIsland(islandIndex);
    if (!island || !island->creatures) return CreatureHandle::invalid();

    return island->creatures->spawn(type, localPosition, parentGenome);
}

bool MultiIslandManager::transferCreature(uint32_t fromIsland, uint32_t toIsland,
                                           CreatureHandle handle, const glm::vec3& arrivalPosition) {
    auto* srcIsland = getIsland(fromIsland);
    auto* dstIsland = getIsland(toIsland);

    if (!srcIsland || !dstIsland) return false;
    if (!srcIsland->creatures || !dstIsland->creatures) return false;

    // Get the creature
    Creature* creature = srcIsland->creatures->get(handle);
    if (!creature || !creature->isAlive()) return false;

    // Store creature data
    Genome genome = creature->getGenome();
    CreatureType type = creature->getType();

    // Kill creature on source island
    srcIsland->creatures->kill(handle, "Migration");

    // Spawn on destination island with same genome
    CreatureHandle newHandle = dstIsland->creatures->spawnWithGenome(arrivalPosition, genome);

    if (!newHandle.isValid()) return false;

    // Update statistics
    srcIsland->stats.emigrations++;
    dstIsland->stats.immigrations++;

    // Emit events
    IslandEvent emigrationEvent;
    emigrationEvent.type = IslandEvent::Type::CREATURE_MIGRATED_OUT;
    emigrationEvent.islandId = fromIsland;
    emigrationEvent.creatureId = handle.index;
    emigrationEvent.description = "Creature emigrated from " + srcIsland->name;
    emigrationEvent.timestamp = m_totalTime;
    emitEvent(emigrationEvent);

    IslandEvent immigrationEvent;
    immigrationEvent.type = IslandEvent::Type::CREATURE_MIGRATED_IN;
    immigrationEvent.islandId = toIsland;
    immigrationEvent.creatureId = newHandle.index;
    immigrationEvent.description = "Creature immigrated to " + dstIsland->name;
    immigrationEvent.timestamp = m_totalTime;
    emitEvent(immigrationEvent);

    m_globalStatsDirty = true;
    return true;
}

// ============================================================================
// Inter-Island Queries
// ============================================================================

float MultiIslandManager::getIslandDistance(uint32_t islandA, uint32_t islandB) const {
    if (islandA >= m_islands.size() || islandB >= m_islands.size()) {
        return std::numeric_limits<float>::max();
    }

    return glm::length(m_islands[islandA].worldPosition - m_islands[islandB].worldPosition);
}

std::vector<uint32_t> MultiIslandManager::getNeighborIslands(uint32_t islandIndex, float maxDistance) const {
    std::vector<uint32_t> neighbors;

    if (islandIndex >= m_islands.size()) return neighbors;

    const glm::vec2& pos = m_islands[islandIndex].worldPosition;

    for (uint32_t i = 0; i < static_cast<uint32_t>(m_islands.size()); ++i) {
        if (i == islandIndex) continue;

        float dist = glm::length(m_islands[i].worldPosition - pos);
        if (dist <= maxDistance) {
            neighbors.push_back(i);
        }
    }

    // Sort by distance
    std::sort(neighbors.begin(), neighbors.end(), [this, &pos](uint32_t a, uint32_t b) {
        return glm::length(m_islands[a].worldPosition - pos) <
               glm::length(m_islands[b].worldPosition - pos);
    });

    return neighbors;
}

bool MultiIslandManager::isOnAnyIsland(const glm::vec3& worldPos) const {
    return findIslandAt(worldPos) >= 0;
}

// ============================================================================
// Statistics
// ============================================================================

const IslandStats& MultiIslandManager::getIslandStats(uint32_t index) const {
    static IslandStats emptyStats;
    if (index >= m_islands.size()) return emptyStats;
    return m_islands[index].stats;
}

IslandStats MultiIslandManager::getGlobalStats() const {
    if (!m_globalStatsDirty) return m_globalStatsCache;

    m_globalStatsCache.reset();

    int totalCount = 0;
    float totalFitness = 0.0f;
    float totalEnergy = 0.0f;
    float totalDiversity = 0.0f;

    for (const auto& island : m_islands) {
        m_globalStatsCache.totalCreatures += island.stats.totalCreatures;
        m_globalStatsCache.speciesCount += island.stats.speciesCount;
        m_globalStatsCache.births += island.stats.births;
        m_globalStatsCache.deaths += island.stats.deaths;
        m_globalStatsCache.immigrations += island.stats.immigrations;
        m_globalStatsCache.emigrations += island.stats.emigrations;

        if (island.stats.totalCreatures > 0) {
            totalFitness += island.stats.avgFitness * island.stats.totalCreatures;
            totalEnergy += island.stats.avgEnergy * island.stats.totalCreatures;
            totalDiversity += island.stats.geneticDiversity;
            totalCount += island.stats.totalCreatures;
        }
    }

    if (totalCount > 0) {
        m_globalStatsCache.avgFitness = totalFitness / totalCount;
        m_globalStatsCache.avgEnergy = totalEnergy / totalCount;
    }

    if (!m_islands.empty()) {
        m_globalStatsCache.geneticDiversity = totalDiversity / m_islands.size();
    }

    m_globalStatsDirty = false;
    return m_globalStatsCache;
}

float MultiIslandManager::getGeneticDistance(uint32_t islandA, uint32_t islandB) const {
    // Simplified genetic distance based on average genome traits
    const auto* a = getIsland(islandA);
    const auto* b = getIsland(islandB);

    if (!a || !b || !a->creatures || !b->creatures) return 1.0f;

    // In a full implementation, this would use proper genetic distance calculations
    // using allele frequencies from the SpeciationTracker

    // Simplified: compare average fitness as proxy for genetic distance
    float fitnessDiff = std::abs(a->stats.avgFitness - b->stats.avgFitness);
    float diversityDiff = std::abs(a->stats.geneticDiversity - b->stats.geneticDiversity);

    return (fitnessDiff + diversityDiff) / 2.0f;
}

// ============================================================================
// Events
// ============================================================================

void MultiIslandManager::registerEventCallback(EventCallback callback) {
    if (callback) {
        m_eventCallbacks.push_back(callback);
    }
}

void MultiIslandManager::emitEvent(const IslandEvent& event) {
    // Store in recent events
    m_recentEvents.push_back(event);
    if (m_recentEvents.size() > MAX_RECENT_EVENTS) {
        m_recentEvents.erase(m_recentEvents.begin());
    }

    // Notify callbacks
    for (const auto& callback : m_eventCallbacks) {
        callback(event);
    }
}

} // namespace Forge
