#include "EcosystemManager.h"
#include "Terrain.h"
#include "../entities/Creature.h"
#include <algorithm>
#include <sstream>
#include <set>
#include <random>
#include <iostream>
#include <iomanip>
#include <cmath>

EcosystemManager::EcosystemManager(Terrain* terrain)
    : terrain(terrain)
    , timeSinceLastBalance(0.0f)
{
    producers = std::make_unique<ProducerSystem>(terrain);
    decomposers = std::make_unique<DecomposerSystem>(producers.get());
    seasons = std::make_unique<SeasonManager>();
    metrics = std::make_unique<EcosystemMetrics>();

    initializeCarryingCapacities();
}

EcosystemManager::~EcosystemManager() = default;

void EcosystemManager::init(unsigned int seed) {
    producers->init(seed);
    generateAquaticSpawnZones();
}

void EcosystemManager::initializeCarryingCapacities() {
    // Base carrying capacities follow trophic pyramid
    // Producers support ~10x herbivores, herbivores support ~10x carnivores

    // Primary consumers (herbivores) - largest population
    baseCarryingCapacity[CreatureType::GRAZER] = 40;
    baseCarryingCapacity[CreatureType::BROWSER] = 30;
    baseCarryingCapacity[CreatureType::FRUGIVORE] = 25;

    // Secondary consumers
    baseCarryingCapacity[CreatureType::SMALL_PREDATOR] = 12;
    baseCarryingCapacity[CreatureType::OMNIVORE] = 10;
    baseCarryingCapacity[CreatureType::SCAVENGER] = 8;

    // Tertiary consumers (apex predators) - smallest population
    baseCarryingCapacity[CreatureType::APEX_PREDATOR] = 6;

    // Special types
    baseCarryingCapacity[CreatureType::PARASITE] = 15;  // Limited by hosts
    baseCarryingCapacity[CreatureType::CLEANER] = 10;
}

int EcosystemManager::getSeasonalCarryingCapacity(CreatureType type) const {
    auto it = baseCarryingCapacity.find(type);
    if (it == baseCarryingCapacity.end()) return 10;

    int base = it->second;

    // Adjust based on season
    float seasonMult = 1.0f;
    if (seasons) {
        Season season = seasons->getCurrentSeason();
        switch (season) {
            case Season::SPRING:
                // Growth season - higher capacity
                seasonMult = 1.2f;
                break;
            case Season::SUMMER:
                // Peak abundance
                seasonMult = 1.0f;
                break;
            case Season::FALL:
                // Preparing for winter - slightly reduced
                seasonMult = 0.9f;
                break;
            case Season::WINTER:
                // Scarcity - lower capacity
                seasonMult = 0.7f;
                break;
        }
    }

    // Herbivores affected more by seasons
    if (isHerbivore(type)) {
        return static_cast<int>(base * seasonMult);
    }

    // Predators less affected
    return static_cast<int>(base * (0.5f + 0.5f * seasonMult));
}

void EcosystemManager::update(float deltaTime,
                              const std::vector<std::unique_ptr<Creature>>& creatures) {
    // Update subsystems
    seasons->update(deltaTime);
    producers->update(deltaTime, seasons.get());
    decomposers->update(deltaTime, seasons.get());

    // Update population counts
    updatePopulationCounts(creatures);

    // Update aquatic population stats (Phase 8 - Ocean Ecosystem)
    updateAquaticPopulation(creatures);

    // Update ecosystem metrics
    metrics->update(deltaTime, currentPopulations, producers.get(),
                   decomposers.get(), seasons.get());

    // Update per-creature ecosystem states
    updateCreatureStates(creatures);

    // Update ecosystem signals (periodically for efficiency)
    signalUpdateTimer += deltaTime;
    if (signalUpdateTimer >= signalUpdateInterval) {
        updateEcosystemSignals();
        updateAquaticSpawnZones();
        signalUpdateTimer = 0.0f;
    }

    // Periodic balance check
    timeSinceLastBalance += deltaTime;
    if (timeSinceLastBalance >= balanceCheckInterval) {
        checkPopulationBalance();
        cleanupDeadCreatureStates(creatures);
        timeSinceLastBalance = 0.0f;
    }
}

void EcosystemManager::updatePopulationCounts(
    const std::vector<std::unique_ptr<Creature>>& creatures) {

    currentPopulations = PopulationCounts();

    for (const auto& creature : creatures) {
        if (!creature || !creature->isAlive()) continue;

        switch (creature->getType()) {
            case CreatureType::GRAZER:
                currentPopulations.grazers++;
                break;
            case CreatureType::BROWSER:
                currentPopulations.browsers++;
                break;
            case CreatureType::FRUGIVORE:
                currentPopulations.frugivores++;
                break;
            case CreatureType::SMALL_PREDATOR:
                currentPopulations.smallPredators++;
                break;
            case CreatureType::OMNIVORE:
                currentPopulations.omnivores++;
                break;
            case CreatureType::APEX_PREDATOR:
                currentPopulations.apexPredators++;
                break;
            case CreatureType::SCAVENGER:
                currentPopulations.scavengers++;
                break;
            case CreatureType::PARASITE:
                currentPopulations.parasites++;
                break;
            case CreatureType::CLEANER:
                currentPopulations.cleaners++;
                break;
            default:
                // Legacy HERBIVORE/CARNIVORE mapped to defaults
                if (isHerbivore(creature->getType())) {
                    currentPopulations.grazers++;
                } else if (isPredator(creature->getType())) {
                    currentPopulations.apexPredators++;
                }
                break;
        }
    }
}

void EcosystemManager::onCreatureDeath(const Creature& creature) {
    // Create corpse for decomposition/scavenging
    decomposers->addCorpse(
        creature.getPosition(),
        creature.getType(),
        creature.getGenome().size,
        creature.getEnergy()
    );

    // Clean up creature state
    creatureStates.erase(creature.getID());
}

std::vector<glm::vec3> EcosystemManager::getFoodPositionsFor(CreatureType type) const {
    if (!producers) return {};

    switch (type) {
        case CreatureType::GRAZER:
            return producers->getGrassPositions();

        case CreatureType::BROWSER:
            {
                auto leaves = producers->getTreeLeafPositions();
                auto bushes = producers->getBushPositions();
                leaves.insert(leaves.end(), bushes.begin(), bushes.end());
                return leaves;
            }

        case CreatureType::FRUGIVORE:
            {
                auto fruits = producers->getTreeFruitPositions();
                auto berries = producers->getBushPositions();
                fruits.insert(fruits.end(), berries.begin(), berries.end());
                return fruits;
            }

        case CreatureType::OMNIVORE:
            // Can eat anything plant-based
            return producers->getAllFoodPositions();

        case CreatureType::SCAVENGER:
            // Gets corpse positions instead
            if (decomposers) {
                return decomposers->getCorpsePositions();
            }
            return {};

        // ===== Aquatic Creatures (Phase 8 - Ocean Ecosystem) =====
        case CreatureType::AQUATIC:
        case CreatureType::AQUATIC_HERBIVORE:
            // Herbivore fish eat algae, plankton, seaweed
            return producers->getAllAquaticFoodPositions();

        case CreatureType::AMPHIBIAN:
            {
                // Amphibians can eat both aquatic and some terrestrial food
                auto aquatic = producers->getAllAquaticFoodPositions();
                auto bugs = producers->getBushPositions();  // Proxy for insects near water
                aquatic.insert(aquatic.end(), bugs.begin(), bugs.end());
                return aquatic;
            }

        case CreatureType::AQUATIC_PREDATOR:
        case CreatureType::AQUATIC_APEX:
            // Aquatic predators don't eat plants - they hunt other fish
            return {};

        default:
            // Predators don't eat plants
            return {};
    }
}

bool EcosystemManager::isEcosystemHealthy() const {
    if (!metrics) return true;
    return !metrics->hasCriticalWarnings() && metrics->getEcosystemHealthScore() > 50.0f;
}

float EcosystemManager::getEcosystemHealth() const {
    return metrics ? metrics->getEcosystemHealthScore() : 100.0f;
}

std::string EcosystemManager::getEcosystemStatus() const {
    std::ostringstream ss;

    if (seasons) {
        ss << seasons->getDateString() << "\n";
    }

    ss << "Population: " << currentPopulations.getTotal() << "\n";
    ss << "  Herbivores: " << currentPopulations.getTotalHerbivores() << "\n";
    ss << "  Carnivores: " << currentPopulations.getTotalCarnivores() << "\n";

    if (producers) {
        ss << "Producer Biomass: " << static_cast<int>(producers->getTotalBiomass()) << "\n";
    }

    if (decomposers) {
        ss << "Corpses: " << decomposers->getCorpseCount() << "\n";
    }

    if (metrics) {
        ss << "Ecosystem Health: " << static_cast<int>(metrics->getEcosystemHealthScore()) << "%\n";

        if (metrics->hasCriticalWarnings()) {
            ss << "WARNING: Critical ecosystem issues!\n";
        }
    }

    return ss.str();
}

int EcosystemManager::getTargetPopulation(CreatureType type) const {
    return getSeasonalCarryingCapacity(type);
}

bool EcosystemManager::isPopulationCritical(CreatureType type) const {
    int target = getTargetPopulation(type);
    int critical = std::max(2, target / 4);

    int current = 0;
    switch (type) {
        case CreatureType::GRAZER:
            current = currentPopulations.grazers;
            break;
        case CreatureType::BROWSER:
            current = currentPopulations.browsers;
            break;
        case CreatureType::FRUGIVORE:
            current = currentPopulations.frugivores;
            break;
        case CreatureType::SMALL_PREDATOR:
            current = currentPopulations.smallPredators;
            break;
        case CreatureType::OMNIVORE:
            current = currentPopulations.omnivores;
            break;
        case CreatureType::APEX_PREDATOR:
            current = currentPopulations.apexPredators;
            break;
        case CreatureType::SCAVENGER:
            current = currentPopulations.scavengers;
            break;
        default:
            return false;
    }

    return current < critical;
}

std::vector<EcosystemManager::SpawnRecommendation>
EcosystemManager::getSpawnRecommendations() const {
    std::vector<SpawnRecommendation> recommendations;

    // Check each creature type
    std::vector<std::pair<CreatureType, int*>> typeChecks = {
        {CreatureType::GRAZER, const_cast<int*>(&currentPopulations.grazers)},
        {CreatureType::BROWSER, const_cast<int*>(&currentPopulations.browsers)},
        {CreatureType::FRUGIVORE, const_cast<int*>(&currentPopulations.frugivores)},
        {CreatureType::SMALL_PREDATOR, const_cast<int*>(&currentPopulations.smallPredators)},
        {CreatureType::OMNIVORE, const_cast<int*>(&currentPopulations.omnivores)},
        {CreatureType::APEX_PREDATOR, const_cast<int*>(&currentPopulations.apexPredators)},
        {CreatureType::SCAVENGER, const_cast<int*>(&currentPopulations.scavengers)},
    };

    for (const auto& [type, countPtr] : typeChecks) {
        int target = getTargetPopulation(type);
        int current = *countPtr;
        int minPopulation = std::max(3, target / 3);

        if (current < minPopulation) {
            SpawnRecommendation rec;
            rec.type = type;
            rec.count = minPopulation - current;
            rec.reason = std::string(getCreatureTypeName(type)) + " population below minimum";
            recommendations.push_back(rec);
        }
    }

    return recommendations;
}

EcosystemState& EcosystemManager::getCreatureState(int creatureId) {
    return creatureStates[creatureId];
}

void EcosystemManager::updateCreatureStates(
    const std::vector<std::unique_ptr<Creature>>& creatures) {

    for (const auto& creature : creatures) {
        if (!creature || !creature->isAlive()) continue;

        int id = creature->getID();
        if (creatureStates.find(id) == creatureStates.end()) {
            // Initialize state for new creature
            EcosystemState state;
            state.traits = CreatureTraits::getTraitsFor(creature->getType());
            state.territoryCenter = creature->getPosition();
            creatureStates[id] = state;
        }

        // Update territory center (slow drift toward current position)
        EcosystemState& state = creatureStates[id];
        if (state.traits.isTerritorial) {
            glm::vec3 pos = creature->getPosition();
            state.territoryCenter = state.territoryCenter * 0.99f + pos * 0.01f;
        }
    }
}

void EcosystemManager::cleanupDeadCreatureStates(
    const std::vector<std::unique_ptr<Creature>>& creatures) {

    // Build set of alive creature IDs
    std::set<int> aliveIds;
    for (const auto& creature : creatures) {
        if (creature && creature->isAlive()) {
            aliveIds.insert(creature->getID());
        }
    }

    // Remove states for dead creatures
    for (auto it = creatureStates.begin(); it != creatureStates.end(); ) {
        if (aliveIds.find(it->first) == aliveIds.end()) {
            it = creatureStates.erase(it);
        } else {
            ++it;
        }
    }
}

void EcosystemManager::checkPopulationBalance() {
    // Log warnings about ecosystem state
    if (metrics && metrics->hasCriticalWarnings()) {
        // Ecosystem needs intervention
        // The Simulation class should check getSpawnRecommendations()
    }
}

// ============================================================================
// EcosystemSignals Implementation
// ============================================================================

void EcosystemManager::updateEcosystemSignals() {
    // Calculate food pressure for herbivores (plant scarcity)
    if (producers) {
        float totalProducerBiomass = producers->getTotalBiomass();
        float maxBiomass = 10000.0f;  // Rough estimate of max possible
        cachedSignals.producerBiomass = std::min(1.0f, totalProducerBiomass / maxBiomass);

        // Plant food pressure: high population + low biomass = scarcity
        int totalHerbivores = currentPopulations.getTotalHerbivores();
        float herbivoreNeed = totalHerbivores * 50.0f;  // Each herbivore needs ~50 biomass
        cachedSignals.plantFoodPressure = std::clamp(
            herbivoreNeed / std::max(1.0f, totalProducerBiomass), 0.0f, 1.0f);

        // Detritus level
        float avgDetritus = producers->getDetritusAt(glm::vec3(0, 0, 0), 100.0f);
        cachedSignals.detritusLevel = std::clamp(avgDetritus / 50.0f, 0.0f, 1.0f);

        // Nutrient saturation (sampled from center soil tile)
        const SoilTile& sampleSoil = producers->getSoilAt(glm::vec3(0, 0, 0));
        cachedSignals.nutrientSaturation = (sampleSoil.nitrogen + sampleSoil.phosphorus +
                                            sampleSoil.organicMatter) / 300.0f;

        // Seasonal bloom signals
        cachedSignals.seasonalBloomStrength = producers->getSeasonalBloomMultiplier();
        cachedSignals.activeBloomType = producers->getBloomType();
    }

    // Calculate prey pressure for carnivores
    int totalPrey = currentPopulations.getTotalHerbivores();
    int targetPrey = getTargetPopulation(CreatureType::GRAZER) +
                     getTargetPopulation(CreatureType::BROWSER) +
                     getTargetPopulation(CreatureType::FRUGIVORE);
    cachedSignals.preyPressure = 1.0f - std::clamp(
        static_cast<float>(totalPrey) / std::max(1, targetPrey), 0.0f, 1.0f);

    // Carrion density for scavengers
    if (decomposers) {
        float carrionBiomass = decomposers->getTotalCarrionBiomass();
        cachedSignals.carrionDensity = std::clamp(carrionBiomass / 500.0f, 0.0f, 1.0f);
    }

    // Population pressures (over/underpopulation)
    int herbTarget = getTargetPopulation(CreatureType::GRAZER) +
                     getTargetPopulation(CreatureType::BROWSER);
    int carnTarget = getTargetPopulation(CreatureType::SMALL_PREDATOR) +
                     getTargetPopulation(CreatureType::APEX_PREDATOR);

    cachedSignals.herbivorePopulationPressure = std::clamp(
        static_cast<float>(currentPopulations.getTotalHerbivores()) / std::max(1, herbTarget),
        0.0f, 2.0f) - 0.5f;  // -0.5 to 1.5, 0 = at target

    cachedSignals.carnivorePopulationPressure = std::clamp(
        static_cast<float>(currentPopulations.getTotalCarnivores()) / std::max(1, carnTarget),
        0.0f, 2.0f) - 0.5f;

    // Seasonal signals
    if (seasons) {
        cachedSignals.isWinter = (seasons->getCurrentSeason() == Season::WINTER);
        cachedSignals.dayLengthFactor = seasons->getDayLength() / 12.0f;  // Relative to 12 hours
    }

    // Update timestamp
    cachedSignals.lastUpdateTime += signalUpdateInterval;
}

float EcosystemManager::getFoodPressure(CreatureType forType) const {
    if (isHerbivore(forType)) {
        return cachedSignals.plantFoodPressure;
    } else if (forType == CreatureType::SCAVENGER) {
        return 1.0f - cachedSignals.carrionDensity;  // Inverse: low carrion = high pressure
    } else if (isPredator(forType)) {
        return cachedSignals.preyPressure;
    }

    // Aquatic creatures (Phase 8 - Ocean Ecosystem)
    if (forType == CreatureType::AQUATIC || forType == CreatureType::AQUATIC_HERBIVORE) {
        // Use plant food pressure as proxy for algae/plankton availability
        return cachedSignals.plantFoodPressure * 0.8f;  // Slightly less pressure - ocean is abundant
    } else if (forType == CreatureType::AQUATIC_PREDATOR || forType == CreatureType::AQUATIC_APEX) {
        // Aquatic predators' pressure based on small fish availability
        return cachedSignals.preyPressure;
    } else if (forType == CreatureType::AMPHIBIAN) {
        // Amphibians can use both aquatic and land resources
        return (cachedSignals.plantFoodPressure + 0.3f) * 0.5f;
    }

    return 0.5f;  // Default neutral
}

float EcosystemManager::getPredationRisk(const glm::vec3& position) const {
    // Estimate based on carnivore population density
    // In a real implementation, this would use spatial queries
    int totalCarnivores = currentPopulations.getTotalCarnivores();
    int targetCarn = getTargetPopulation(CreatureType::SMALL_PREDATOR) +
                     getTargetPopulation(CreatureType::APEX_PREDATOR);

    return std::clamp(static_cast<float>(totalCarnivores) / std::max(1, targetCarn), 0.0f, 1.0f);
}

EcosystemSignals EcosystemManager::getLocalSignals(const glm::vec3& position, float radius) const {
    EcosystemSignals localSignals = cachedSignals;  // Start with cached values

    // Override with local calculations where possible
    if (producers) {
        // Local detritus
        localSignals.detritusLevel = std::clamp(
            producers->getDetritusAt(position, radius) / 50.0f, 0.0f, 1.0f);

        // Local nutrient saturation
        const SoilTile& localSoil = producers->getSoilAt(position);
        localSignals.nutrientSaturation = (localSoil.nitrogen + localSoil.phosphorus +
                                           localSoil.organicMatter) / 300.0f;
    }

    if (decomposers) {
        // Local carrion density
        localSignals.carrionDensity = std::clamp(
            decomposers->getCarrionDensity(position, radius) / 100.0f, 0.0f, 1.0f);
    }

    return localSignals;
}

// ============================================================================
// Aquatic Ecosystem Implementation (Phase 8 - Ocean Ecosystem)
// ============================================================================

float EcosystemManager::getWaterDepthAt(float x, float z) const {
    if (!terrain) return 0.0f;

    const float WATER_LEVEL = SwimBehavior::getWaterLevelConstant();  // 10.5f
    float terrainHeight = terrain->getHeight(x, z);

    if (terrainHeight >= WATER_LEVEL) {
        return 0.0f;  // Not water
    }

    return WATER_LEVEL - terrainHeight;
}

void EcosystemManager::generateAquaticSpawnZones() {
    aquaticSpawnZones.clear();

    if (!terrain) return;

    const float WATER_LEVEL = SwimBehavior::getWaterLevelConstant();
    float terrainWidth = terrain->getWidth() * terrain->getScale();
    float zoneSpacing = 40.0f;  // Space between zone centers

    std::mt19937 rng(12345);  // Deterministic seed for consistent zones

    int zonesGenerated = 0;

    // Scan terrain for water areas and create spawn zones
    for (float x = -terrainWidth * 0.5f; x < terrainWidth * 0.5f; x += zoneSpacing) {
        for (float z = -terrainWidth * 0.5f; z < terrainWidth * 0.5f; z += zoneSpacing) {

            float depth = getWaterDepthAt(x, z);
            if (depth < 3.0f) continue;  // Too shallow for a zone

            // Sample surrounding area to find zone extent
            float minDepthFound = depth;
            float maxDepthFound = depth;
            float validRadius = 0.0f;

            for (float r = 5.0f; r < zoneSpacing * 0.4f; r += 5.0f) {
                bool allWater = true;
                for (float angle = 0; angle < 6.28f; angle += 0.5f) {
                    float sx = x + r * std::cos(angle);
                    float sz = z + r * std::sin(angle);
                    float sampleDepth = getWaterDepthAt(sx, sz);

                    if (sampleDepth < 2.0f) {
                        allWater = false;
                        break;
                    }
                    minDepthFound = std::min(minDepthFound, sampleDepth);
                    maxDepthFound = std::max(maxDepthFound, sampleDepth);
                }

                if (!allWater) break;
                validRadius = r;
            }

            if (validRadius < 10.0f) continue;  // Zone too small

            AquaticSpawnZone zone;
            zone.center = glm::vec3(x, WATER_LEVEL - depth * 0.5f, z);
            zone.radius = validRadius;
            zone.minDepth = minDepthFound;
            zone.maxDepth = maxDepthFound;
            zone.primaryBand = getDepthBand(depth);

            // Set environmental factors based on depth
            zone.temperature = 20.0f - depth * 0.1f;  // Cooler at depth
            zone.oxygenLevel = 1.0f - (depth / 100.0f) * 0.3f;  // Less O2 at depth

            // Food density based on plankton/algae patches nearby
            zone.foodDensity = 0.5f;
            if (producers) {
                auto planktonPositions = producers->getPlanktonPositions();
                int nearbyFood = 0;
                for (const auto& pos : planktonPositions) {
                    float dist = glm::length(glm::vec2(pos.x - x, pos.z - z));
                    if (dist < validRadius * 1.5f) nearbyFood++;
                }
                zone.foodDensity = std::clamp(nearbyFood * 0.1f, 0.2f, 1.0f);
            }

            // Shelter based on kelp/seaweed nearby
            zone.shelterDensity = 0.3f;
            if (producers) {
                auto seaweedPositions = producers->getSeaweedPositions();
                int nearbyShelter = 0;
                for (const auto& pos : seaweedPositions) {
                    float dist = glm::length(glm::vec2(pos.x - x, pos.z - z));
                    if (dist < validRadius * 1.5f) nearbyShelter++;
                }
                zone.shelterDensity = std::clamp(nearbyShelter * 0.15f, 0.1f, 0.9f);
            }

            // Spawn weights by depth band
            switch (zone.primaryBand) {
                case DepthBand::SURFACE:
                case DepthBand::SHALLOW:
                    zone.herbivoreWeight = 0.7f;
                    zone.predatorWeight = 0.25f;
                    zone.apexWeight = 0.05f;
                    zone.maxCapacity = 40;
                    break;

                case DepthBand::MID_WATER:
                    zone.herbivoreWeight = 0.55f;
                    zone.predatorWeight = 0.35f;
                    zone.apexWeight = 0.10f;
                    zone.maxCapacity = 60;
                    break;

                case DepthBand::DEEP:
                    zone.herbivoreWeight = 0.40f;
                    zone.predatorWeight = 0.40f;
                    zone.apexWeight = 0.20f;
                    zone.maxCapacity = 30;
                    break;

                case DepthBand::ABYSS:
                    zone.herbivoreWeight = 0.30f;
                    zone.predatorWeight = 0.35f;
                    zone.apexWeight = 0.35f;
                    zone.maxCapacity = 15;
                    break;

                default:
                    break;
            }

            aquaticSpawnZones.push_back(zone);
            zonesGenerated++;
        }
    }

    std::cout << "[AQUATIC ECOSYSTEM] Generated " << zonesGenerated << " aquatic spawn zones" << std::endl;
}

void EcosystemManager::updateAquaticSpawnZones() {
    // Update zone populations and environmental factors
    // This is called periodically during the signal update

    for (auto& zone : aquaticSpawnZones) {
        // Refresh food density
        if (producers) {
            auto planktonPositions = producers->getPlanktonPositions();
            int nearbyFood = 0;
            for (const auto& pos : planktonPositions) {
                float dist = glm::length(glm::vec2(pos.x - zone.center.x, pos.z - zone.center.z));
                if (dist < zone.radius * 1.5f) nearbyFood++;
            }
            zone.foodDensity = std::clamp(nearbyFood * 0.1f, 0.2f, 1.0f);
        }

        // Seasonal temperature variation
        if (seasons) {
            float seasonMod = 0.0f;
            switch (seasons->getCurrentSeason()) {
                case Season::SUMMER: seasonMod = 2.0f; break;
                case Season::WINTER: seasonMod = -4.0f; break;
                default: break;
            }
            zone.temperature = 20.0f - zone.maxDepth * 0.1f + seasonMod;
        }
    }
}

void EcosystemManager::updateAquaticPopulation(
    const std::vector<std::unique_ptr<Creature>>& creatures) {

    aquaticStats.reset();

    const float WATER_LEVEL = SwimBehavior::getWaterLevelConstant();
    float totalDepth = 0.0f;

    // Reset zone populations
    for (auto& zone : aquaticSpawnZones) {
        zone.currentPopulation = 0;
    }

    // Count aquatic creatures and their depth distribution
    for (const auto& creature : creatures) {
        if (!creature || !creature->isAlive()) continue;

        CreatureType type = creature->getType();
        if (!isAquatic(type)) continue;

        const glm::vec3& pos = creature->getPosition();
        float depth = WATER_LEVEL - pos.y;

        if (depth < 0) depth = 0;  // Clamp to surface

        // Count by type
        switch (type) {
            case CreatureType::AQUATIC:
            case CreatureType::AQUATIC_HERBIVORE:
                aquaticStats.herbivoreCount++;
                break;
            case CreatureType::AQUATIC_PREDATOR:
                aquaticStats.predatorCount++;
                break;
            case CreatureType::AQUATIC_APEX:
                aquaticStats.apexCount++;
                break;
            case CreatureType::AMPHIBIAN:
                aquaticStats.amphibianCount++;
                break;
            default:
                break;
        }

        // Count by depth band
        DepthBand band = getDepthBand(depth);
        int bandIndex = static_cast<int>(band);
        if (bandIndex >= 0 && bandIndex < static_cast<int>(DepthBand::COUNT)) {
            aquaticStats.countByDepth[bandIndex]++;
        }

        totalDepth += depth;
        aquaticStats.totalAquatic++;

        // Update zone population
        for (auto& zone : aquaticSpawnZones) {
            float dist = glm::length(glm::vec2(pos.x - zone.center.x, pos.z - zone.center.z));
            if (dist < zone.radius) {
                zone.currentPopulation++;
                break;  // Only count in one zone
            }
        }
    }

    // Calculate averages
    if (aquaticStats.totalAquatic > 0) {
        aquaticStats.avgDepth = totalDepth / aquaticStats.totalAquatic;

        // Calculate variance
        float varianceSum = 0.0f;
        for (const auto& creature : creatures) {
            if (!creature || !creature->isAlive()) continue;
            if (!isAquatic(creature->getType())) continue;

            float depth = WATER_LEVEL - creature->getPosition().y;
            float diff = depth - aquaticStats.avgDepth;
            varianceSum += diff * diff;
        }
        aquaticStats.depthVariance = varianceSum / aquaticStats.totalAquatic;
    }

    // Calculate food web ratios
    if (producers) {
        int aquaticFoodCount = static_cast<int>(producers->getPlanktonPatches().size() +
                                                 producers->getAlgaePatches().size() +
                                                 producers->getSeaweedPatches().size());
        if (aquaticFoodCount > 0) {
            aquaticStats.herbivorePreyRatio =
                static_cast<float>(aquaticStats.herbivoreCount) / aquaticFoodCount;
        }
    }

    if (aquaticStats.herbivoreCount > 0) {
        aquaticStats.predatorPreyRatio =
            static_cast<float>(aquaticStats.predatorCount) / aquaticStats.herbivoreCount;
    }

    if (aquaticStats.predatorCount > 0) {
        aquaticStats.apexPredatorRatio =
            static_cast<float>(aquaticStats.apexCount) / aquaticStats.predatorCount;
    }
}

const AquaticSpawnZone* EcosystemManager::findBestSpawnZone(CreatureType type) const {
    if (aquaticSpawnZones.empty()) return nullptr;

    const AquaticSpawnZone* bestZone = nullptr;
    float bestScore = -1.0f;

    // Get preferred depth range for this creature type
    float minDepth, maxDepth;
    getAquaticSpawnDepthRange(type, minDepth, maxDepth);

    for (const auto& zone : aquaticSpawnZones) {
        if (!zone.isValid()) continue;

        // Skip zones at capacity
        if (zone.currentPopulation >= zone.maxCapacity) continue;

        // Check depth compatibility
        if (zone.maxDepth < minDepth || zone.minDepth > maxDepth) continue;

        float score = 0.0f;

        // Capacity availability (prefer zones with room)
        float capacityRatio = 1.0f - (static_cast<float>(zone.currentPopulation) / zone.maxCapacity);
        score += capacityRatio * 30.0f;

        // Depth preference matching
        float avgZoneDepth = (zone.minDepth + zone.maxDepth) * 0.5f;
        float preferredDepth = (minDepth + maxDepth) * 0.5f;
        float depthMatch = 1.0f - std::abs(avgZoneDepth - preferredDepth) / 50.0f;
        score += std::max(0.0f, depthMatch) * 25.0f;

        // Environmental factors
        score += zone.oxygenLevel * 15.0f;

        // Type-specific preferences
        switch (type) {
            case CreatureType::AQUATIC:
            case CreatureType::AQUATIC_HERBIVORE:
                score += zone.foodDensity * 20.0f;
                score += zone.shelterDensity * 10.0f;
                break;

            case CreatureType::AQUATIC_PREDATOR:
                // Predators prefer zones with prey (herbivores)
                score += zone.herbivoreWeight * 15.0f;
                score += (1.0f - zone.shelterDensity) * 10.0f;  // Less shelter = easier hunting
                break;

            case CreatureType::AQUATIC_APEX:
                // Apex predators prefer deep, larger zones
                score += (zone.maxDepth / 50.0f) * 15.0f;
                score += (zone.radius / 30.0f) * 10.0f;
                break;

            case CreatureType::AMPHIBIAN:
                // Amphibians prefer shallow zones near surface
                score += (1.0f - zone.minDepth / 10.0f) * 20.0f;
                score += zone.shelterDensity * 10.0f;
                break;

            default:
                break;
        }

        if (score > bestScore) {
            bestScore = score;
            bestZone = &zone;
        }
    }

    return bestZone;
}

glm::vec3 EcosystemManager::getAquaticSpawnPosition(
    const AquaticSpawnZone& zone, CreatureType type) const {

    const float WATER_LEVEL = SwimBehavior::getWaterLevelConstant();

    // Random position within zone
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);
    std::uniform_real_distribution<float> radiusDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> depthDist(0.0f, 1.0f);

    float angle = angleDist(rng);
    float r = zone.radius * std::sqrt(radiusDist(rng));  // Square root for uniform distribution

    float x = zone.center.x + r * std::cos(angle);
    float z = zone.center.z + r * std::sin(angle);

    // Get spawn depth for this creature type
    float availableDepth = getWaterDepthAt(x, z);
    float spawnDepth = calculateAquaticSpawnDepth(type, availableDepth, depthDist(rng));

    float y = WATER_LEVEL - spawnDepth;

    return glm::vec3(x, y, z);
}

int EcosystemManager::getPopulationInDepthBand(DepthBand band) const {
    int bandIndex = static_cast<int>(band);
    if (bandIndex >= 0 && bandIndex < static_cast<int>(DepthBand::COUNT)) {
        return aquaticStats.countByDepth[bandIndex];
    }
    return 0;
}

bool EcosystemManager::isAquaticEcosystemHealthy() const {
    // Check if aquatic ecosystem is balanced
    if (aquaticStats.totalAquatic == 0) return true;  // No aquatic = healthy by default

    // Check predator/prey ratio (should be around 0.2-0.4 for predators per herbivore)
    if (aquaticStats.predatorPreyRatio > 0.8f) return false;  // Too many predators

    // Check apex ratio (should be low, around 0.1-0.3)
    if (aquaticStats.apexPredatorRatio > 0.6f) return false;

    // Check if there's distribution across depth bands
    int occupiedBands = 0;
    for (int i = 0; i < static_cast<int>(DepthBand::COUNT); i++) {
        if (aquaticStats.countByDepth[i] > 0) occupiedBands++;
    }
    if (occupiedBands < 2 && aquaticStats.totalAquatic > 20) return false;  // Poor distribution

    return true;
}

float EcosystemManager::getAquaticEcosystemHealth() const {
    if (aquaticStats.totalAquatic == 0) return 100.0f;

    float health = 100.0f;

    // Deduct for imbalanced predator/prey ratio
    float idealPredRatio = 0.3f;
    float predRatioDiff = std::abs(aquaticStats.predatorPreyRatio - idealPredRatio);
    health -= predRatioDiff * 50.0f;

    // Deduct for poor depth distribution
    int occupiedBands = 0;
    for (int i = 0; i < static_cast<int>(DepthBand::COUNT); i++) {
        if (aquaticStats.countByDepth[i] > 0) occupiedBands++;
    }
    if (occupiedBands < 3) health -= (3 - occupiedBands) * 10.0f;

    // Deduct if zones are overcrowded
    int overcrowdedZones = 0;
    for (const auto& zone : aquaticSpawnZones) {
        if (zone.currentPopulation > zone.maxCapacity * 0.9f) {
            overcrowdedZones++;
        }
    }
    health -= overcrowdedZones * 5.0f;

    return std::clamp(health, 0.0f, 100.0f);
}

float EcosystemManager::getAquaticFoodWebBalance() const {
    // Returns 0-1 where 0.5 is balanced
    if (aquaticStats.totalAquatic == 0) return 0.5f;

    // Ideal ratios for a healthy marine ecosystem:
    // Herbivores: 60%, Predators: 30%, Apex: 10%
    float herbRatio = static_cast<float>(aquaticStats.herbivoreCount) / aquaticStats.totalAquatic;
    float predRatio = static_cast<float>(aquaticStats.predatorCount) / aquaticStats.totalAquatic;
    float apexRatio = static_cast<float>(aquaticStats.apexCount) / aquaticStats.totalAquatic;

    float herbDiff = std::abs(herbRatio - 0.6f);
    float predDiff = std::abs(predRatio - 0.3f);
    float apexDiff = std::abs(apexRatio - 0.1f);

    float avgDiff = (herbDiff + predDiff + apexDiff) / 3.0f;
    return std::clamp(0.5f - avgDiff, 0.0f, 1.0f);
}

std::string EcosystemManager::getDepthBandHistogram() const {
    std::ostringstream ss;

    ss << "=== AQUATIC DEPTH DISTRIBUTION ===" << std::endl;
    ss << "Total Aquatic: " << aquaticStats.totalAquatic << std::endl;
    ss << "Avg Depth: " << std::fixed << std::setprecision(1) << aquaticStats.avgDepth << "m" << std::endl;
    ss << std::endl;

    // Find max count for scaling
    int maxCount = 1;
    for (int i = 0; i < static_cast<int>(DepthBand::COUNT); i++) {
        maxCount = std::max(maxCount, aquaticStats.countByDepth[i]);
    }

    // Print histogram
    for (int i = 0; i < static_cast<int>(DepthBand::COUNT); i++) {
        DepthBand band = static_cast<DepthBand>(i);
        int count = aquaticStats.countByDepth[i];
        int barLength = (count * 30) / maxCount;

        ss << std::setw(18) << getDepthBandName(band) << " |";
        for (int j = 0; j < barLength; j++) ss << "#";
        ss << " " << count << std::endl;
    }

    ss << std::endl;
    ss << "--- Population by Type ---" << std::endl;
    ss << "Herbivores: " << aquaticStats.herbivoreCount << std::endl;
    ss << "Predators:  " << aquaticStats.predatorCount << std::endl;
    ss << "Apex:       " << aquaticStats.apexCount << std::endl;
    ss << "Amphibians: " << aquaticStats.amphibianCount << std::endl;
    ss << std::endl;
    ss << "--- Food Web Ratios ---" << std::endl;
    ss << "Herb/Food:    " << std::fixed << std::setprecision(2) << aquaticStats.herbivorePreyRatio << std::endl;
    ss << "Pred/Herb:    " << aquaticStats.predatorPreyRatio << std::endl;
    ss << "Apex/Pred:    " << aquaticStats.apexPredatorRatio << std::endl;
    ss << "Ecosystem HP: " << std::setprecision(0) << getAquaticEcosystemHealth() << "%" << std::endl;

    return ss.str();
}
