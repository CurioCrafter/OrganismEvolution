#include "GeneticsManager.h"
#include "../Creature.h"
#include "../../environment/Terrain.h"
#include "../../utils/Random.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <map>

namespace genetics {

GeneticsManager::GeneticsManager()
    : lastUpdateGeneration(0), timeSinceEpigeneticUpdate(0.0f) {
    mateSelector.setSpeciesThreshold(config.speciesDistanceThreshold);
    mateSelector.setSearchRadius(config.mateSearchRadius);
    speciationTracker.setSpeciesThreshold(config.speciesDistanceThreshold);
    speciationTracker.setMinPopulationForSpecies(config.minPopulationForSpecies);
    hybridZoneManager.setZoneDetectionRadius(config.hybridZoneRadius);
    hybridZoneManager.setMinSpeciesOverlap(config.minSpeciesOverlap);
}

GeneticsManager::GeneticsManager(const GeneticsConfig& config)
    : config(config), lastUpdateGeneration(0), timeSinceEpigeneticUpdate(0.0f) {
    setConfig(config);
}

void GeneticsManager::initialize(std::vector<Creature*>& creatures, int generation) {
    std::cout << "[GeneticsManager] Initializing genetics system..." << std::endl;

    // Assign all creatures to initial species
    updateSpeciesAssignments(creatures, generation);

    std::cout << "[GeneticsManager] Initial species count: "
              << speciationTracker.getActiveSpeciesCount() << std::endl;
}

void GeneticsManager::update(std::vector<Creature*>& creatures, int generation, float deltaTime) {
    // Update species tracking (every generation)
    if (generation != lastUpdateGeneration) {
        updateSpeciesAssignments(creatures, generation);

        // Check for genetic drift effects in small populations
        detectGeneticDrift(creatures);

        // Natural selection against high genetic load
        purgingSelection(creatures);

        lastUpdateGeneration = generation;
    }

    // Update hybrid zones
    if (config.trackHybridZones) {
        hybridZoneManager.update(creatures, speciationTracker, generation);
    }

    // Update epigenetics periodically
    if (config.enableEpigenetics) {
        timeSinceEpigeneticUpdate += deltaTime;
        if (timeSinceEpigeneticUpdate > 10.0f) {  // Every 10 seconds
            updateEpigenetics(creatures, timeSinceEpigeneticUpdate);
            timeSinceEpigeneticUpdate = 0.0f;
        }
    }
}

std::unique_ptr<Creature> GeneticsManager::handleReproduction(
    Creature& parent1,
    const std::vector<Creature*>& potentialMates,
    const Terrain& terrain,
    int generation) {

    if (!parent1.canReproduce()) return nullptr;

    // Find suitable mate using mate selection
    auto candidates = mateSelector.findPotentialMates(parent1, potentialMates,
                                                       config.mateSearchRadius);

    Creature* selectedMate = nullptr;

    if (config.useSexualSelection && !candidates.empty()) {
        selectedMate = mateSelector.selectMate(parent1, candidates);
    }

    // Calculate spawn position
    glm::vec3 spawnPos = parent1.getPosition();
    spawnPos.x += Random::range(-5.0f, 5.0f);
    spawnPos.z += Random::range(-5.0f, 5.0f);

    // Ensure spawn on land
    if (terrain.isWater(spawnPos.x, spawnPos.z)) {
        spawnPos = parent1.getPosition();
    }
    spawnPos.y = terrain.getHeight(spawnPos.x, spawnPos.z);

    std::unique_ptr<Creature> offspring;

    if (selectedMate && selectedMate->canReproduce()) {
        // Sexual reproduction
        const DiploidGenome& g1 = parent1.getDiploidGenome();
        const DiploidGenome& g2 = selectedMate->getDiploidGenome();

        // Check if this is a hybrid mating
        bool isHybrid = (g1.getSpeciesId() != g2.getSpeciesId());

        if (isHybrid && config.trackHybridZones) {
            // Use hybrid zone manager for cross-species mating
            offspring = hybridZoneManager.attemptHybridMating(
                parent1, *selectedMate, mateSelector, generation);
        } else {
            // Normal sexual reproduction
            offspring = std::make_unique<Creature>(spawnPos, g1, g2, parent1.getType());
        }

        if (offspring) {
            // Apply mutations
            offspring->getDiploidGenome().mutate(config.baseMutationRate, config.mutationStrength);

            // Set generation
            offspring->setGeneration(std::max(parent1.getGeneration(),
                                               selectedMate->getGeneration()) + 1);

            // Deduct reproduction cost from both parents
            float cost;
            parent1.reproduce(cost);
            selectedMate->reproduce(cost);
        }
    } else {
        // Asexual reproduction (fallback or intentional)
        DiploidGenome childGenome = parent1.getDiploidGenome();
        childGenome.mutate(config.baseMutationRate * 1.5f, config.mutationStrength);  // Higher mutation

        offspring = std::make_unique<Creature>(spawnPos, childGenome, parent1.getType());
        offspring->setGeneration(parent1.getGeneration() + 1);

        float cost;
        parent1.reproduce(cost);
    }

    return offspring;
}

GeneticsStats GeneticsManager::getStats() const {
    GeneticsStats stats;
    stats.activeSpeciesCount = speciationTracker.getActiveSpeciesCount();
    stats.extinctSpeciesCount = static_cast<int>(speciationTracker.getExtinctSpecies().size());
    stats.speciationEvents = speciationTracker.getSpeciationEventCount();
    stats.extinctionEvents = speciationTracker.getExtinctionEventCount();
    stats.activeHybridZones = hybridZoneManager.getActiveZoneCount();

    // Other stats would need the creature list
    stats.totalHybrids = 0;
    stats.averageHeterozygosity = 0;
    stats.averageGeneticLoad = 0;
    stats.averageInbreeding = 0;
    stats.totalDeleteriousAlleles = 0;

    return stats;
}

void GeneticsManager::setConfig(const GeneticsConfig& newConfig) {
    config = newConfig;
    mateSelector.setSpeciesThreshold(config.speciesDistanceThreshold);
    mateSelector.setSearchRadius(config.mateSearchRadius);
    speciationTracker.setSpeciesThreshold(config.speciesDistanceThreshold);
    speciationTracker.setMinPopulationForSpecies(config.minPopulationForSpecies);
    hybridZoneManager.setZoneDetectionRadius(config.hybridZoneRadius);
    hybridZoneManager.setMinSpeciesOverlap(config.minSpeciesOverlap);
}

void GeneticsManager::exportPhylogeneticTree(const std::string& filename) const {
    speciationTracker.getPhylogeneticTree().exportNewick(filename);
    std::cout << "[GeneticsManager] Exported phylogenetic tree to " << filename << std::endl;
}

void GeneticsManager::applyEnvironmentalStress(std::vector<Creature*>& creatures, float stressLevel) {
    if (!config.enableEpigenetics) return;

    for (Creature* c : creatures) {
        if (c && c->isAlive()) {
            c->getDiploidGenome().applyEnvironmentalStress(stressLevel);
        }
    }
}

void GeneticsManager::applyNutritionEffects(std::vector<Creature*>& creatures, float avgNutrition) {
    if (!config.enableEpigenetics) return;

    for (Creature* c : creatures) {
        if (c && c->isAlive()) {
            c->getDiploidGenome().applyNutritionEffect(avgNutrition);
        }
    }
}

float GeneticsManager::calculateFST(const std::vector<Creature*>& creatures) const {
    // Calculate Wright's fixation index (population differentiation)
    auto activeSpecies = speciationTracker.getActiveSpecies();
    if (activeSpecies.size() < 2) return 0.0f;

    // Group creatures by species
    std::map<SpeciesId, std::vector<const Creature*>> bySpecies;
    for (const Creature* c : creatures) {
        if (c && c->isAlive()) {
            bySpecies[c->getDiploidGenome().getSpeciesId()].push_back(c);
        }
    }

    if (bySpecies.size() < 2) return 0.0f;

    // Calculate Hs (average within-subpopulation heterozygosity)
    float Hs = 0.0f;
    int popCount = 0;
    for (const auto& [spId, members] : bySpecies) {
        if (members.size() < 5) continue;

        float popHet = 0.0f;
        for (const Creature* c : members) {
            popHet += c->getDiploidGenome().getHeterozygosity();
        }
        Hs += popHet / members.size();
        popCount++;
    }
    if (popCount > 0) Hs /= popCount;

    // Calculate Ht (total heterozygosity if all combined)
    float Ht = calculateAverageHeterozygosity(creatures);

    // FST = (Ht - Hs) / Ht
    if (Ht < 0.001f) return 0.0f;
    return (Ht - Hs) / Ht;
}

float GeneticsManager::calculateHardyWeinbergDeviation(const std::vector<Creature*>& creatures) const {
    // Simplified - would need specific locus analysis for proper calculation
    // This gives a rough measure based on overall heterozygosity

    float observedHet = calculateAverageHeterozygosity(creatures);
    float expectedHet = 0.5f;  // Expected under random mating

    return std::abs(observedHet - expectedHet);
}

void GeneticsManager::updateEpigenetics(std::vector<Creature*>& creatures, float deltaTime) {
    for (Creature* c : creatures) {
        if (c && c->isAlive()) {
            c->getDiploidGenome().updateEpigeneticMarks();
        }
    }
}

void GeneticsManager::updateSpeciesAssignments(std::vector<Creature*>& creatures, int generation) {
    speciationTracker.update(creatures, generation);
}

void GeneticsManager::detectGeneticDrift(std::vector<Creature*>& creatures) {
    // Group by species and check for small populations
    std::map<SpeciesId, std::vector<Creature*>> bySpecies;
    for (Creature* c : creatures) {
        if (c && c->isAlive()) {
            bySpecies[c->getDiploidGenome().getSpeciesId()].push_back(c);
        }
    }

    for (auto& [spId, members] : bySpecies) {
        // Small populations experience stronger drift
        if (members.size() < 20) {
            float driftStrength = 1.0f / std::sqrt(static_cast<float>(members.size()));

            // Some random individuals don't reproduce (simulating drift)
            for (Creature* c : members) {
                if (Random::chance(driftStrength * 0.1f)) {
                    // Mark as having reduced reproduction chance
                    c->setFitnessModifier(c->getFitnessModifier() * 0.8f);
                }
            }

            if (members.size() < 10) {
                std::cout << "[DRIFT] Small population detected for species "
                          << spId << " (n=" << members.size() << ")" << std::endl;
            }
        }
    }
}

void GeneticsManager::purgingSelection(std::vector<Creature*>& creatures) {
    // Natural selection against high genetic load
    for (Creature* c : creatures) {
        if (!c || !c->isAlive()) continue;

        float load = c->getDiploidGenome().getGeneticLoad();

        // High load reduces survival/reproduction
        if (load > 0.3f) {
            float survivalPenalty = (load - 0.3f) * 0.2f;
            c->setFitnessModifier(c->getFitnessModifier() * (1.0f - survivalPenalty));
        }

        // Very high load can be lethal
        if (load > 0.8f && Random::chance(load - 0.7f)) {
            // This creature has too many deleterious mutations
            // Natural selection removes them from the population
            c->setFitnessModifier(c->getFitnessModifier() * 0.5f);
        }
    }
}

float GeneticsManager::calculateAverageHeterozygosity(const std::vector<Creature*>& creatures) const {
    if (creatures.empty()) return 0.0f;

    float totalHet = 0.0f;
    int count = 0;

    for (const Creature* c : creatures) {
        if (c && c->isAlive()) {
            totalHet += c->getDiploidGenome().getHeterozygosity();
            count++;
        }
    }

    return count > 0 ? totalHet / count : 0.0f;
}

float GeneticsManager::calculateAverageGeneticLoad(const std::vector<Creature*>& creatures) const {
    if (creatures.empty()) return 0.0f;

    float totalLoad = 0.0f;
    int count = 0;

    for (const Creature* c : creatures) {
        if (c && c->isAlive()) {
            totalLoad += c->getDiploidGenome().getGeneticLoad();
            count++;
        }
    }

    return count > 0 ? totalLoad / count : 0.0f;
}

float GeneticsManager::calculateAverageInbreeding(const std::vector<Creature*>& creatures) const {
    if (creatures.empty()) return 0.0f;

    float totalInbreeding = 0.0f;
    int count = 0;

    for (const Creature* c : creatures) {
        if (c && c->isAlive()) {
            totalInbreeding += c->getDiploidGenome().calculateInbreedingCoeff();
            count++;
        }
    }

    return count > 0 ? totalInbreeding / count : 0.0f;
}

int GeneticsManager::countDeleteriousAlleles(const std::vector<Creature*>& creatures) const {
    int total = 0;

    for (const Creature* c : creatures) {
        if (c && c->isAlive()) {
            total += c->getDiploidGenome().countDeleteriousAlleles();
        }
    }

    return total;
}

} // namespace genetics
