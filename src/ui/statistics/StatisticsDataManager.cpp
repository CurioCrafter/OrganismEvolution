#include "StatisticsDataManager.h"
#include "../../entities/Creature.h"
#include "../../entities/CreatureType.h"

namespace ui {
namespace statistics {

// ============================================================================
// Constructor
// ============================================================================

StatisticsDataManager::StatisticsDataManager() {
    // Reserve space for history
    m_populationHistory.resize(0);
    m_fitnessHistory.resize(0);
    m_energyFlowHistory.resize(0);
    m_selectionPressureHistory.resize(0);
    m_nicheOccupancyHistory.resize(0);
    m_fpsHistory.resize(0);
}

// ============================================================================
// Update Methods
// ============================================================================

void StatisticsDataManager::update(float deltaTime, SimulationOrchestrator& orchestrator) {
    // Extract system pointers from orchestrator and delegate
    // Note: This assumes SimulationOrchestrator provides these accessors
    // Adjust based on actual SimulationOrchestrator interface

    update(deltaTime,
           orchestrator.getCreatureManager(),
           orchestrator.getEcosystemMetrics(),
           orchestrator.getSpeciationTracker(),
           orchestrator.getEvolutionaryHistory(),
           orchestrator.getNicheManager(),
           orchestrator.getFoodChainManager(),
           orchestrator.getSelectionPressureCalculator(),
           orchestrator.getPerformanceManager(),
           orchestrator.getSimulationTime(),
           orchestrator.getCurrentGeneration());
}

void StatisticsDataManager::update(float deltaTime,
                                   const Forge::CreatureManager* creatures,
                                   const EcosystemMetrics* ecosystemMetrics,
                                   const genetics::SpeciationTracker* speciesTracker,
                                   const genetics::EvolutionaryHistoryTracker* evolutionHistory,
                                   const genetics::NicheManager* nicheManager,
                                   const Forge::FoodChainManager* foodChain,
                                   const genetics::SelectionPressureCalculator* selectionPressures,
                                   const Forge::PerformanceManager* performance,
                                   float simulationTime,
                                   int generation) {
    if (m_paused) return;

    m_simulationTime = simulationTime;
    m_totalGenerations = generation;
    m_timeSinceLastSample += deltaTime;
    m_timeSinceFastSample += deltaTime;
    m_timeSinceTraitUpdate += deltaTime;

    // Fast sampling (for responsive FPS display)
    if (m_timeSinceFastSample >= FAST_SAMPLE_INTERVAL) {
        m_timeSinceFastSample = 0.0f;
        samplePerformance(performance);
    }

    // Regular sampling
    if (m_timeSinceLastSample >= m_sampleInterval) {
        m_timeSinceLastSample = 0.0f;

        // Sample all data sources
        samplePopulation(creatures, speciesTracker);
        sampleFitness(creatures);
        sampleEnergyFlow(foodChain, ecosystemMetrics);
        sampleSelectionPressures(selectionPressures);
        sampleNicheOccupancy(nicheManager);
        sampleAquaticDepths(creatures);  // Aquatic ecosystem depth tracking
        checkForNewEvents(speciesTracker, evolutionHistory);
        calculateSummaryStatistics(ecosystemMetrics);

        // Add to history
        m_currentPopulation.time = simulationTime;
        m_populationHistory.push_back(m_currentPopulation);

        m_currentFitness.time = simulationTime;
        m_fitnessHistory.push_back(m_currentFitness);

        m_currentEnergyFlow.time = simulationTime;
        m_energyFlowHistory.push_back(m_currentEnergyFlow);

        m_currentSelectionPressures.time = simulationTime;
        m_selectionPressureHistory.push_back(m_currentSelectionPressures);

        m_currentNicheOccupancy.time = simulationTime;
        m_nicheOccupancyHistory.push_back(m_currentNicheOccupancy);

        // Trim excess history
        trimHistory();
    }

    // Trait distributions updated less frequently (expensive)
    if (m_timeSinceTraitUpdate >= TRAIT_UPDATE_INTERVAL) {
        m_timeSinceTraitUpdate = 0.0f;
        sampleTraitDistributions(creatures);
    }
}

// ============================================================================
// Sampling Methods
// ============================================================================

void StatisticsDataManager::samplePopulation(const Forge::CreatureManager* creatures,
                                              const genetics::SpeciationTracker* speciesTracker) {
    m_currentPopulation = PopulationSample();

    if (!creatures) return;

    const auto& allCreatures = creatures->getAllCreatures();

    for (const auto& creature : allCreatures) {
        if (!creature || !creature->isAlive()) continue;

        m_currentPopulation.totalCreatures++;

        CreatureType type = creature->getType();

        if (isHerbivore(type)) {
            m_currentPopulation.herbivoreCount++;
        } else if (isPredator(type)) {
            m_currentPopulation.carnivoreCount++;
        } else if (type == CreatureType::OMNIVORE) {
            m_currentPopulation.omnivoreCount++;
        }

        if (isAquatic(type)) {
            m_currentPopulation.aquaticCount++;
        }
        if (isFlying(type)) {
            m_currentPopulation.flyingCount++;
        }
    }

    // Get species populations if tracker available
    if (speciesTracker) {
        m_currentPopulation.speciesCount = speciesTracker->getActiveSpeciesCount();

        for (const auto& species : speciesTracker->getAllSpecies()) {
            if (species.isExtant()) {
                m_currentPopulation.speciesPopulations[species.id] = species.currentPopulation;
            }
        }
    }
}

void StatisticsDataManager::sampleFitness(const Forge::CreatureManager* creatures) {
    m_currentFitness = FitnessSample();

    if (!creatures) return;

    const auto& allCreatures = creatures->getAllCreatures();

    std::vector<float> fitnessValues;
    fitnessValues.reserve(allCreatures.size());

    float totalFitness = 0.0f;
    float maxFitness = -1e9f;
    float minFitness = 1e9f;

    for (const auto& creature : allCreatures) {
        if (!creature || !creature->isAlive()) continue;

        float fitness = creature->getFitness();
        fitnessValues.push_back(fitness);
        totalFitness += fitness;
        maxFitness = std::max(maxFitness, fitness);
        minFitness = std::min(minFitness, fitness);
    }

    if (!fitnessValues.empty()) {
        m_currentFitness.avgFitness = totalFitness / static_cast<float>(fitnessValues.size());
        m_currentFitness.maxFitness = maxFitness;
        m_currentFitness.minFitness = minFitness;

        // Calculate variance
        float variance = 0.0f;
        for (float f : fitnessValues) {
            float diff = f - m_currentFitness.avgFitness;
            variance += diff * diff;
        }
        m_currentFitness.fitnessVariance = variance / static_cast<float>(fitnessValues.size());
    }

    // Calculate genetic diversity (coefficient of variation of key traits)
    if (!m_traitDistributions.size.samples.empty()) {
        float cvSize = m_traitDistributions.size.mean > 0.001f ?
            m_traitDistributions.size.stdDev / m_traitDistributions.size.mean : 0.0f;
        float cvSpeed = m_traitDistributions.speed.mean > 0.001f ?
            m_traitDistributions.speed.stdDev / m_traitDistributions.speed.mean : 0.0f;
        float cvVision = m_traitDistributions.visionRange.mean > 0.001f ?
            m_traitDistributions.visionRange.stdDev / m_traitDistributions.visionRange.mean : 0.0f;

        m_currentFitness.geneticDiversity = std::min(1.0f, (cvSize + cvSpeed + cvVision) / 3.0f);
    }
}

void StatisticsDataManager::sampleTraitDistributions(const Forge::CreatureManager* creatures) {
    // Clear previous samples
    m_traitDistributions.size.samples.clear();
    m_traitDistributions.speed.samples.clear();
    m_traitDistributions.visionRange.samples.clear();
    m_traitDistributions.efficiency.samples.clear();
    m_traitDistributions.aggression.samples.clear();
    m_traitDistributions.reproductionRate.samples.clear();
    m_traitDistributions.lifespan.samples.clear();
    m_traitDistributions.mutationRate.samples.clear();

    if (!creatures) return;

    const auto& allCreatures = creatures->getAllCreatures();

    // Reserve space
    size_t estimatedSize = allCreatures.size();
    m_traitDistributions.size.samples.reserve(estimatedSize);
    m_traitDistributions.speed.samples.reserve(estimatedSize);
    m_traitDistributions.visionRange.samples.reserve(estimatedSize);
    m_traitDistributions.efficiency.samples.reserve(estimatedSize);
    m_traitDistributions.aggression.samples.reserve(estimatedSize);
    m_traitDistributions.reproductionRate.samples.reserve(estimatedSize);
    m_traitDistributions.lifespan.samples.reserve(estimatedSize);
    m_traitDistributions.mutationRate.samples.reserve(estimatedSize);

    for (const auto& creature : allCreatures) {
        if (!creature || !creature->isAlive()) continue;

        const Genome& genome = creature->getGenome();

        m_traitDistributions.size.samples.push_back(genome.size);
        m_traitDistributions.speed.samples.push_back(genome.speed);
        m_traitDistributions.visionRange.samples.push_back(genome.visionRange);
        m_traitDistributions.efficiency.samples.push_back(genome.efficiency);
        m_traitDistributions.aggression.samples.push_back(genome.aggression);
        m_traitDistributions.reproductionRate.samples.push_back(genome.reproductionRate);
        m_traitDistributions.lifespan.samples.push_back(genome.maxAge);
        m_traitDistributions.mutationRate.samples.push_back(genome.mutationRate);
    }

    // Calculate statistics and build histograms
    m_traitDistributions.calculateAll();
    m_traitDistributions.calculateCorrelations();
}

void StatisticsDataManager::sampleEnergyFlow(const Forge::FoodChainManager* foodChain,
                                              const EcosystemMetrics* ecosystem) {
    m_currentEnergyFlow = EnergyFlowSample();

    if (foodChain) {
        const auto& stats = foodChain->getEnergyStats();
        m_currentEnergyFlow.producerEnergy = stats.producerEnergy;
        m_currentEnergyFlow.herbivoreEnergy = stats.herbivoreEnergy;
        m_currentEnergyFlow.carnivoreEnergy = stats.smallPredatorEnergy + stats.apexPredatorEnergy;
        m_currentEnergyFlow.transferEfficiency = stats.transferEfficiency;
    }

    if (ecosystem) {
        m_currentEnergyFlow.producerEnergy = ecosystem->getProducerBiomass();
        m_currentEnergyFlow.decomposerEnergy = ecosystem->getDecomposerBiomass();
    }
}

void StatisticsDataManager::sampleSelectionPressures(
    const genetics::SelectionPressureCalculator* pressures) {
    m_currentSelectionPressures = SelectionPressureSample();

    if (!pressures) return;

    // Get recent pressure history and average
    auto predationHistory = pressures->getPressureHistory(genetics::PressureType::PREDATION, 10);
    auto competitionHistory = pressures->getPressureHistory(genetics::PressureType::COMPETITION, 10);
    auto climateHistory = pressures->getPressureHistory(genetics::PressureType::CLIMATE, 10);
    auto foodHistory = pressures->getPressureHistory(genetics::PressureType::FOOD_SCARCITY, 10);
    auto diseaseHistory = pressures->getPressureHistory(genetics::PressureType::DISEASE, 10);
    auto sexualHistory = pressures->getPressureHistory(genetics::PressureType::SEXUAL_SELECTION, 10);

    // Average recent values
    auto averageIntensity = [](const std::vector<genetics::PressureHistoryRecord>& history) {
        if (history.empty()) return 0.0f;
        float sum = 0.0f;
        for (const auto& rec : history) {
            sum += rec.intensity;
        }
        return sum / static_cast<float>(history.size());
    };

    m_currentSelectionPressures.predationPressure = averageIntensity(predationHistory);
    m_currentSelectionPressures.competitionPressure = averageIntensity(competitionHistory);
    m_currentSelectionPressures.climatePressure = averageIntensity(climateHistory);
    m_currentSelectionPressures.foodPressure = averageIntensity(foodHistory);
    m_currentSelectionPressures.diseasePressure = averageIntensity(diseaseHistory);
    m_currentSelectionPressures.sexualSelectionPressure = averageIntensity(sexualHistory);
}

void StatisticsDataManager::sampleNicheOccupancy(const genetics::NicheManager* nicheManager) {
    m_currentNicheOccupancy = NicheOccupancySample();

    if (!nicheManager) return;

    const auto& allOccupancy = nicheManager->getAllOccupancy();

    int occupied = 0;
    int empty = 0;

    for (const auto& [nicheType, occupancy] : allOccupancy) {
        m_currentNicheOccupancy.occupancy[nicheType] = occupancy.currentPopulation;

        if (occupancy.currentPopulation > 0) {
            occupied++;
        } else {
            empty++;
        }
    }

    m_currentNicheOccupancy.occupiedNiches = occupied;
    m_currentNicheOccupancy.emptyNiches = empty;

    // Calculate niche overlap index
    auto competitions = nicheManager->getActiveCompetitions();
    float totalOverlap = 0.0f;
    for (const auto& comp : competitions) {
        totalOverlap += comp.totalOverlap;
    }
    m_currentNicheOccupancy.nicheOverlapIndex = competitions.empty() ? 0.0f :
        totalOverlap / static_cast<float>(competitions.size());
}

void StatisticsDataManager::samplePerformance(const Forge::PerformanceManager* performance) {
    if (!performance) return;

    const auto& stats = performance->getStats();
    m_currentFPS = stats.currentFPS;
    m_averageFPS = stats.avgFPS;
    m_drawCalls = stats.drawCalls;
    m_visibleCreatures = stats.visibleCreatures;
    m_memoryUsage = stats.creaturePoolMemory + stats.gpuMemoryUsed;

    // Add to FPS history
    m_fpsHistory.push_back(m_currentFPS);
    while (m_fpsHistory.size() > MAX_HISTORY_POINTS) {
        m_fpsHistory.pop_front();
    }
}

void StatisticsDataManager::checkForNewEvents(const genetics::SpeciationTracker* tracker,
                                               const genetics::EvolutionaryHistoryTracker* history) {
    if (!tracker) return;

    // Check for new speciation events
    const auto& events = tracker->getSpeciationEvents();
    int currentCount = static_cast<int>(events.size());

    if (currentCount > m_lastSpeciationCount) {
        // New speciation events detected
        for (int i = m_lastSpeciationCount; i < currentCount; ++i) {
            const auto& event = events[i];

            SpeciationEventDisplay display;
            display.time = m_simulationTime;
            display.parentId = event.parentSpeciesId;
            display.childId = event.childSpeciesId;
            display.cause = event.cause;

            // Get species names if available
            if (const auto* parent = tracker->getSpecies(event.parentSpeciesId)) {
                display.parentName = parent->name;
                display.color = parent->displayColor;
            }
            if (const auto* child = tracker->getSpecies(event.childSpeciesId)) {
                display.childName = child->name;
            }

            m_speciationEvents.push_back(display);
        }
        m_lastSpeciationCount = currentCount;
    }

    // Check for extinction events
    const auto& extinctions = tracker->getExtinctionEvents();
    int extinctCount = static_cast<int>(extinctions.size());

    if (extinctCount > m_lastExtinctionCount) {
        for (int i = m_lastExtinctionCount; i < extinctCount; ++i) {
            const auto& event = extinctions[i];

            ExtinctionEventDisplay display;
            display.time = m_simulationTime;
            display.speciesId = event.speciesId;
            display.cause = event.cause;
            display.finalPopulation = event.finalPopulation;

            // Get species info if available
            if (const auto* species = tracker->getSpecies(event.speciesId)) {
                display.speciesName = species->name;
                display.lifespan = event.generation - species->foundingGeneration;
            }

            m_extinctionEvents.push_back(display);
        }
        m_lastExtinctionCount = extinctCount;
    }

    // Trim event lists
    while (m_speciationEvents.size() > MAX_EVENTS) {
        m_speciationEvents.pop_front();
    }
    while (m_extinctionEvents.size() > MAX_EVENTS) {
        m_extinctionEvents.pop_front();
    }
}

void StatisticsDataManager::calculateSummaryStatistics(const EcosystemMetrics* ecosystem) {
    if (ecosystem) {
        m_speciesDiversity = ecosystem->getSpeciesDiversity();
        m_ecosystemHealth = ecosystem->getEcosystemHealthScore();
        m_trophicBalance = ecosystem->getTrophicBalance();
    } else {
        // Calculate from our own data
        if (m_currentPopulation.speciesCount > 0) {
            // Shannon diversity approximation
            float total = static_cast<float>(m_currentPopulation.totalCreatures);
            if (total > 0) {
                float diversity = 0.0f;
                for (const auto& [id, pop] : m_currentPopulation.speciesPopulations) {
                    if (pop > 0) {
                        float p = static_cast<float>(pop) / total;
                        diversity -= p * std::log(p);
                    }
                }
                // Normalize to 0-1
                float maxDiversity = std::log(static_cast<float>(m_currentPopulation.speciesCount));
                m_speciesDiversity = maxDiversity > 0 ? diversity / maxDiversity : 0.0f;
            }
        }

        // Trophic balance (herbivores / carnivores ratio, ideal ~10:1)
        if (m_currentPopulation.carnivoreCount > 0) {
            float ratio = static_cast<float>(m_currentPopulation.herbivoreCount) /
                         static_cast<float>(m_currentPopulation.carnivoreCount);
            // Score 1.0 at ratio 10, decreasing as we deviate
            m_trophicBalance = std::exp(-std::pow(std::log(ratio / 10.0f), 2));
        } else {
            m_trophicBalance = m_currentPopulation.herbivoreCount > 0 ? 0.5f : 0.0f;
        }
    }
}

void StatisticsDataManager::trimHistory() {
    while (m_populationHistory.size() > MAX_HISTORY_POINTS) {
        m_populationHistory.pop_front();
    }
    while (m_fitnessHistory.size() > MAX_HISTORY_POINTS) {
        m_fitnessHistory.pop_front();
    }
    while (m_energyFlowHistory.size() > MAX_HISTORY_POINTS) {
        m_energyFlowHistory.pop_front();
    }
    while (m_selectionPressureHistory.size() > MAX_HISTORY_POINTS) {
        m_selectionPressureHistory.pop_front();
    }
    while (m_nicheOccupancyHistory.size() > MAX_HISTORY_POINTS) {
        m_nicheOccupancyHistory.pop_front();
    }
}

void StatisticsDataManager::sampleAquaticDepths(const Forge::CreatureManager* creatures) {
    // Reset depth counts
    m_aquaticDepthCounts.fill(0);

    if (!creatures) return;

    const auto& allCreatures = creatures->getAllCreatures();

    // Water level constant (from TerrainSampler)
    constexpr float WATER_LEVEL = 0.35f;
    constexpr float HEIGHT_SCALE = 30.0f;
    const float waterSurfaceY = WATER_LEVEL * HEIGHT_SCALE;  // 10.5f

    for (const auto& creature : allCreatures) {
        if (!creature || !creature->isAlive()) continue;
        if (!isAquatic(creature->getType())) continue;

        // Calculate depth below water surface
        float depth = waterSurfaceY - creature->getPosition().y;

        // Classify into depth bands
        // Surface: 0-2m, Shallow: 2-5m, MidWater: 5-25m, Deep: 25-50m, Abyss: 50m+
        int bandIndex = 0;
        if (depth < 2.0f) {
            bandIndex = 0;  // Surface
        } else if (depth < 5.0f) {
            bandIndex = 1;  // Shallow
        } else if (depth < 25.0f) {
            bandIndex = 2;  // MidWater
        } else if (depth < 50.0f) {
            bandIndex = 3;  // Deep
        } else {
            bandIndex = 4;  // Abyss
        }

        m_aquaticDepthCounts[bandIndex]++;
    }
}

void StatisticsDataManager::clear() {
    m_populationHistory.clear();
    m_fitnessHistory.clear();
    m_energyFlowHistory.clear();
    m_selectionPressureHistory.clear();
    m_nicheOccupancyHistory.clear();
    m_speciationEvents.clear();
    m_extinctionEvents.clear();
    m_fpsHistory.clear();

    m_currentPopulation = PopulationSample();
    m_currentFitness = FitnessSample();
    m_currentEnergyFlow = EnergyFlowSample();
    m_currentSelectionPressures = SelectionPressureSample();
    m_currentNicheOccupancy = NicheOccupancySample();
    m_aquaticDepthCounts.fill(0);

    m_timeSinceLastSample = 0.0f;
    m_timeSinceFastSample = 0.0f;
    m_timeSinceTraitUpdate = 0.0f;
    m_simulationTime = 0.0f;
    m_totalGenerations = 0;
    m_lastSpeciationCount = 0;
    m_lastExtinctionCount = 0;
}

} // namespace statistics
} // namespace ui
