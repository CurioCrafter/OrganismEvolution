#pragma once

#include "DiploidGenome.h"
#include "MateSelector.h"
#include "Species.h"
#include "HybridZone.h"
#include <vector>
#include <memory>

class Creature;
class Terrain;

namespace genetics {

// Configuration for the genetics system
struct GeneticsConfig {
    // Mutation rates
    float baseMutationRate = 0.05f;
    float mutationStrength = 0.15f;

    // Species detection
    float speciesDistanceThreshold = 0.15f;
    int minPopulationForSpecies = 10;
    int generationsForSpeciation = 50;

    // Mate selection
    float mateSearchRadius = 30.0f;
    bool useSexualSelection = true;

    // Hybrid zones
    bool trackHybridZones = true;
    float hybridZoneRadius = 30.0f;
    int minSpeciesOverlap = 5;

    // Epigenetics
    bool enableEpigenetics = true;
    float epigeneticDecayRate = 0.8f;

    // Population genetics
    bool trackAlleleFrequencies = true;
    bool detectBottlenecks = true;
};

// Statistics about the genetic state of the simulation
struct GeneticsStats {
    int activeSpeciesCount;
    int extinctSpeciesCount;
    int speciationEvents;
    int extinctionEvents;
    int activeHybridZones;
    int totalHybrids;
    float averageHeterozygosity;
    float averageGeneticLoad;
    float averageInbreeding;
    int totalDeleteriousAlleles;
};

// Main manager class for the genetics system
class GeneticsManager {
public:
    GeneticsManager();
    GeneticsManager(const GeneticsConfig& config);

    // Initialize with default species
    void initialize(std::vector<Creature*>& creatures, int generation);

    // Update the genetics system each frame
    void update(std::vector<Creature*>& creatures, int generation, float deltaTime);

    // Handle reproduction with mate selection
    std::unique_ptr<Creature> handleReproduction(
        Creature& parent1,
        const std::vector<Creature*>& potentialMates,
        const Terrain& terrain,
        int generation);

    // Get statistics
    GeneticsStats getStats() const;

    // Access subsystems
    SpeciationTracker& getSpeciationTracker() { return speciationTracker; }
    const SpeciationTracker& getSpeciationTracker() const { return speciationTracker; }
    MateSelector& getMateSelector() { return mateSelector; }
    const MateSelector& getMateSelector() const { return mateSelector; }
    HybridZoneManager& getHybridZoneManager() { return hybridZoneManager; }
    const HybridZoneManager& getHybridZoneManager() const { return hybridZoneManager; }

    // Configuration
    void setConfig(const GeneticsConfig& config);
    const GeneticsConfig& getConfig() const { return config; }

    // Export phylogenetic tree
    void exportPhylogeneticTree(const std::string& filename) const;

    // Apply environmental effects
    void applyEnvironmentalStress(std::vector<Creature*>& creatures, float stressLevel);
    void applyNutritionEffects(std::vector<Creature*>& creatures, float avgNutrition);

    // Population analysis
    float calculateFST(const std::vector<Creature*>& creatures) const;
    float calculateHardyWeinbergDeviation(const std::vector<Creature*>& creatures) const;

private:
    GeneticsConfig config;
    MateSelector mateSelector;
    SpeciationTracker speciationTracker;
    HybridZoneManager hybridZoneManager;

    int lastUpdateGeneration;
    float timeSinceEpigeneticUpdate;

    // Internal helpers
    void updateEpigenetics(std::vector<Creature*>& creatures, float deltaTime);
    void updateSpeciesAssignments(std::vector<Creature*>& creatures, int generation);
    void detectGeneticDrift(std::vector<Creature*>& creatures);
    void purgingSelection(std::vector<Creature*>& creatures);

    // Calculate population-level statistics
    float calculateAverageHeterozygosity(const std::vector<Creature*>& creatures) const;
    float calculateAverageGeneticLoad(const std::vector<Creature*>& creatures) const;
    float calculateAverageInbreeding(const std::vector<Creature*>& creatures) const;
    int countDeleteriousAlleles(const std::vector<Creature*>& creatures) const;
};

} // namespace genetics
