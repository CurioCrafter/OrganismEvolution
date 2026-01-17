#pragma once

#include "Chromosome.h"
#include "../CreatureType.h"
#include <vector>
#include <utility>
#include <cstdint>
#include <glm/glm.hpp>

namespace genetics {

// Forward declarations for enhanced mutation system
class MutationSystem;
struct MutationRateModifiers;
struct Mutation;

// Species identifier
using SpeciesId = uint32_t;

// ============================================
// Phenotype: Expressed traits from genotype
// ============================================
struct Phenotype {
    // Physical traits
    float size = 1.0f;
    float speed = 12.0f;
    float visionRange = 30.0f;
    float efficiency = 1.0f;
    float metabolicRate = 1.0f;
    float fertility = 1.0f;
    float maturationRate = 1.0f;

    // Color
    glm::vec3 color = glm::vec3(0.5f);
    float patternType = 0.0f;
    float ornamentIntensity = 0.3f;
    float displayFrequency = 0.3f;

    // Behavioral
    float aggression = 0.5f;
    float sociality = 0.5f;
    float curiosity = 0.5f;
    float fearResponse = 0.5f;

    // Aptitudes (for creature type determination)
    float terrestrialAptitude = 0.8f;
    float aquaticAptitude = 0.2f;
    float aerialAptitude = 0.1f;

    // Flying traits
    float wingSpan = 1.0f;
    float flapFrequency = 5.0f;
    float glideRatio = 0.5f;
    float preferredAltitude = 25.0f;

    // Aquatic traits
    float finSize = 0.6f;
    float tailSize = 0.8f;
    float swimFrequency = 2.0f;
    float swimAmplitude = 0.2f;
    float preferredDepth = 0.3f;
    float schoolingStrength = 0.7f;

    // Sensory - Vision
    float visionFOV = 3.0f;
    float visionAcuity = 0.5f;
    float colorPerception = 0.5f;
    float motionDetection = 0.6f;

    // Sensory - Hearing
    float hearingRange = 40.0f;
    float hearingDirectionality = 0.5f;
    float echolocationAbility = 0.0f;

    // Sensory - Smell
    float smellRange = 50.0f;
    float smellSensitivity = 0.5f;
    float pheromoneProduction = 0.3f;

    // Sensory - Touch
    float touchRange = 2.0f;
    float vibrationSensitivity = 0.4f;

    // Defense & Communication
    float camouflageLevel = 0.3f;
    float alarmCallVolume = 0.5f;

    // Memory
    float memoryCapacity = 0.5f;
    float memoryRetention = 0.5f;

    // Tolerance
    float heatTolerance = 0.5f;
    float coldTolerance = 0.5f;

    // Niche
    float dietSpecialization = 0.3f;
    float habitatPreference = 0.5f;
    float activityTime = 0.5f;

    // Calculate sensory energy cost (metabolic trade-off)
    float calculateSensoryEnergyCost() const;
};

// ============================================
// Creature Statistics for fitness calculation
// ============================================
struct CreatureStats {
    CreatureType type = CreatureType::GRAZER;
    float survivalTime = 0.0f;
    float currentEnergy = 100.0f;
    float currentHealth = 100.0f;
    int offspringCount = 0;
    int offspringSurvived = 0;
    int foodEaten = 0;
    float energyEfficiency = 1.0f;
    int killCount = 0;
    float huntingSuccessRate = 0.0f;
    int matingAttempts = 0;
    float matingSuccessRate = 0.0f;
};

// Configuration for genome structure
struct GenomeConfig {
    int numChromosomes = 9;        // Extended: physical, color, behavioral, niche, aptitudes, flying, aquatic, sensory, memory
    int genesPerChromosome = 12;   // Base genes per chromosome
    float baseMutationRate = 0.05f;
    float mutationStrength = 0.15f;
    int neuralWeightCount = 24;

    // Mutation type probabilities
    float pointMutationProb = 0.8f;
    float insertionProb = 0.05f;
    float deletionProb = 0.05f;
    float duplicationProb = 0.05f;
    float inversionProb = 0.05f;
};

// Mate preferences for sexual selection
struct MatePreferences {
    float sizePreference;          // -1 to 1 (smaller to larger)
    float ornamentPreference;      // How much ornaments matter
    float similarityPreference;    // Prefer similar (positive) or different (negative)
    float choosiness;              // How selective (0 = accept all, 1 = very picky)
    float minimumAcceptable;       // Won't mate below this threshold

    MatePreferences()
        : sizePreference(0.0f), ornamentPreference(0.5f),
          similarityPreference(0.0f), choosiness(0.5f),
          minimumAcceptable(0.3f) {}
};

// Ecological niche for sympatric speciation
struct EcologicalNiche {
    float dietSpecialization;   // 0 = generalist, 1 = specialist
    float habitatPreference;    // What terrain type preferred
    float activityTime;         // 0 = nocturnal, 1 = diurnal

    EcologicalNiche()
        : dietSpecialization(0.3f), habitatPreference(0.5f), activityTime(0.5f) {}

    float distanceTo(const EcologicalNiche& other) const;
};

// Diploid genome with paired chromosomes
class DiploidGenome {
public:
    DiploidGenome();
    DiploidGenome(const GenomeConfig& config);

    // Sexual reproduction constructor
    DiploidGenome(const DiploidGenome& parent1, const DiploidGenome& parent2,
                  bool isHybrid = false);

    // Initialize with random values
    void randomize(const GenomeConfig& config);

    // Mutation - basic mutation (legacy method)
    void mutate(float mutationRate, float mutationStrength);

    // ========================================
    // ENHANCED MUTATION SYSTEM INTEGRATION
    // ========================================
    // Note: For enhanced mutation using MutationSystem, use:
    //   MutationSystem system;
    //   auto mutations = system.mutateWithTypes(genome, modifiers);
    // See MutationSystem.h for the comprehensive mutation API

    // Get expressed phenotype for a trait
    float getTrait(GeneType type) const;

    // Get color as vec3
    glm::vec3 getColor() const;

    // Get all neural weights
    std::vector<float> getNeuralWeights() const;

    // Get mate preferences from genome
    MatePreferences getMatePreferences() const;

    // Get ecological niche
    EcologicalNiche getEcologicalNiche() const;

    // ========================================
    // NEW: Express genotype to phenotype (C-15 fix)
    // ========================================
    Phenotype express() const;

    // ========================================
    // NEW: Determine creature type from genes (C-15 fix)
    // ========================================
    CreatureType determineCreatureType() const;

    // ========================================
    // NEW: Enhanced fitness calculation (C-16 fix)
    // ========================================
    float calculateFitness(const CreatureStats& stats) const;

    // ========================================
    // NEW: Flying trait accessors
    // ========================================
    float getWingSpan() const { return getTrait(GeneType::WING_SPAN); }
    float getFlapFrequency() const { return getTrait(GeneType::FLAP_FREQUENCY); }
    float getGlideRatio() const { return getTrait(GeneType::GLIDE_RATIO); }
    float getPreferredAltitude() const { return getTrait(GeneType::PREFERRED_ALTITUDE); }

    // ========================================
    // NEW: Aquatic trait accessors
    // ========================================
    float getFinSize() const { return getTrait(GeneType::FIN_SIZE); }
    float getTailSize() const { return getTrait(GeneType::TAIL_SIZE); }
    float getSwimFrequency() const { return getTrait(GeneType::SWIM_FREQUENCY); }
    float getSwimAmplitude() const { return getTrait(GeneType::SWIM_AMPLITUDE); }
    float getPreferredDepth() const { return getTrait(GeneType::PREFERRED_DEPTH); }
    float getSchoolingStrength() const { return getTrait(GeneType::SCHOOLING_STRENGTH); }

    // ========================================
    // NEW: Aptitude accessors
    // ========================================
    float getTerrestrialAptitude() const { return getTrait(GeneType::TERRESTRIAL_APTITUDE); }
    float getAquaticAptitude() const { return getTrait(GeneType::AQUATIC_APTITUDE); }
    float getAerialAptitude() const { return getTrait(GeneType::AERIAL_APTITUDE); }

    // ========================================
    // NEW: Sensory trait accessors
    // ========================================
    float getCamouflageLevel() const { return getTrait(GeneType::CAMOUFLAGE_LEVEL); }
    float getVisionFOV() const { return getTrait(GeneType::VISION_FOV); }
    float getVisionAcuity() const { return getTrait(GeneType::VISION_ACUITY); }
    float getMotionDetection() const { return getTrait(GeneType::MOTION_DETECTION); }

    // Genetic distance to another genome
    float distanceTo(const DiploidGenome& other) const;

    // Inbreeding coefficient (based on homozygosity)
    float calculateInbreedingCoeff() const;

    // Total fitness effect from all genes (genetic load)
    float getGeneticLoad() const;

    // Get number of deleterious alleles
    int countDeleteriousAlleles() const;

    // Heterozygosity (genetic diversity)
    float getHeterozygosity() const;

    // Chromosome access
    size_t getChromosomeCount() const { return chromosomePairs.size(); }
    const std::pair<Chromosome, Chromosome>& getChromosomePair(size_t index) const {
        return chromosomePairs[index];
    }
    std::pair<Chromosome, Chromosome>& getChromosomePair(size_t index) {
        return chromosomePairs[index];
    }

    // Species tracking
    SpeciesId getSpeciesId() const { return speciesId; }
    void setSpeciesId(SpeciesId id) { speciesId = id; }
    uint64_t getLineageId() const { return lineageId; }
    void setLineageId(uint64_t id) { lineageId = id; }

    // Hybrid status
    bool isHybrid() const { return hybrid; }
    void setHybrid(bool h) { hybrid = h; }

    // Epigenetics
    void applyEnvironmentalStress(float stressLevel);
    void applyNutritionEffect(float nutritionLevel);
    void updateEpigeneticMarks();
    void inheritEpigeneticMarks(const DiploidGenome& parent);

    // Gene access by type
    const Gene* getGene(GeneType type) const;
    Gene* getGene(GeneType type);

    // Static configuration
    static GenomeConfig defaultConfig;

private:
    std::vector<std::pair<Chromosome, Chromosome>> chromosomePairs;
    SpeciesId speciesId;
    uint64_t lineageId;
    bool hybrid;

    static uint64_t nextLineageId;

    // Helper to find gene across all chromosomes
    const Gene* findGene(GeneType type) const;
    Gene* findGene(GeneType type);

    // Perform meiosis (gamete creation)
    std::vector<Chromosome> createGamete() const;
};

} // namespace genetics
