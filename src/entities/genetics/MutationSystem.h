#pragma once

/**
 * @file MutationSystem.h
 * @brief Comprehensive mutation system for genetic evolution simulation
 *
 * This file defines an advanced mutation framework that extends beyond simple
 * point mutations to include realistic mutation types such as duplications,
 * deletions, inversions, translocations, and regulatory mutations. The system
 * tracks mutation history, calculates fitness effects, and models mutation
 * hotspots and rate modifiers.
 *
 * @author OrganismEvolution Project
 * @version 1.0
 */

#include "Gene.h"
#include "Chromosome.h"
#include "DiploidGenome.h"
#include <vector>
#include <map>
#include <deque>
#include <functional>
#include <string>
#include <cstdint>
#include <memory>
#include <cmath>
#include <algorithm>

namespace genetics {

// Forward declarations
class DiploidGenome;
class Chromosome;

// =============================================================================
// MUTATION TYPE ENUMERATION
// =============================================================================

/**
 * @enum MutationType
 * @brief Comprehensive classification of mutation types
 *
 * Represents the various types of mutations that can occur in the genome,
 * from simple point mutations to complex structural rearrangements.
 */
enum class MutationCategory {
    /**
     * @brief Small value change in a single gene
     *
     * The most common mutation type. Changes the value of a gene slightly.
     * Usually has small phenotypic effects.
     */
    POINT_MUTATION,

    /**
     * @brief Gene copy creating a duplicate
     *
     * Creates a copy of a gene, increasing gene dosage. Can lead to
     * neofunctionalization or subfunctionalization over time.
     */
    DUPLICATION,

    /**
     * @brief Gene loss or inactivation
     *
     * Removes a gene from the chromosome. Usually deleterious but can
     * occasionally be beneficial if the gene was costly to express.
     */
    DELETION,

    /**
     * @brief Gene order reversal within a chromosome
     *
     * Reverses the order of a segment of genes. Can disrupt gene linkage
     * and affect gene expression through position effects.
     */
    INVERSION,

    /**
     * @brief Gene moves between chromosomes
     *
     * A gene or segment moves from one chromosome to another. Can create
     * new gene combinations and disrupt existing linkage groups.
     */
    TRANSLOCATION,

    /**
     * @brief Expression level change without sequence change
     *
     * Changes how much a gene is expressed without altering its function.
     * Often through promoter or enhancer modifications.
     */
    REGULATORY,

    /**
     * @brief Complete gene duplication including regulatory regions
     *
     * Duplicates an entire gene with its regulatory sequences intact.
     * More likely to create functional copies than partial duplications.
     */
    WHOLE_GENE_DUPLICATION,

    /**
     * @brief Reading frame shift causing downstream changes
     *
     * Insertion or deletion that shifts the reading frame. Usually
     * severely disrupts gene function downstream of the mutation.
     */
    FRAMESHIFT,

    /**
     * @brief Silent mutation with no phenotypic effect
     *
     * Changes DNA sequence but does not alter the expressed phenotype.
     * Serves as neutral genetic marker.
     */
    SILENT,

    /**
     * @brief Mutation affecting multiple linked genes
     *
     * A single mutational event that affects several nearby genes,
     * such as a large deletion or chromosomal rearrangement.
     */
    BLOCK_MUTATION,

    /**
     * @brief Total count for iteration
     */
    COUNT
};

// =============================================================================
// MUTATION EFFECT CLASSIFICATION
// =============================================================================

/**
 * @enum MutationEffect
 * @brief Classification of mutation fitness effects
 */
enum class MutationEffect {
    BENEFICIAL,      ///< Increases fitness
    NEUTRAL,         ///< No fitness effect
    DELETERIOUS,     ///< Decreases fitness
    LETHAL,          ///< Causes death (fitness = 0)
    CONDITIONAL      ///< Effect depends on environment
};

// =============================================================================
// HOTSPOT REASON ENUMERATION
// =============================================================================

/**
 * @enum HotspotReason
 * @brief Reasons why certain genomic regions have elevated mutation rates
 */
enum class HotspotReason {
    REPETITIVE_SEQUENCE,    ///< Repeat sequences prone to replication errors
    FRAGILE_SITE,           ///< Chromosomal fragile site
    RECOMBINATION_HOTSPOT,  ///< High recombination area
    METHYLATION_PRONE,      ///< CpG islands prone to deamination
    OXIDATIVE_DAMAGE,       ///< Region sensitive to oxidative stress
    REPLICATION_FORK_PAUSE, ///< Replication stalling site
    TRANSCRIPTION_COUPLED,  ///< Actively transcribed region
    SECONDARY_STRUCTURE,    ///< DNA secondary structure formation
    UNKNOWN                 ///< Unknown reason
};

// =============================================================================
// MUTATION LOCATION STRUCTURE
// =============================================================================

/**
 * @struct MutationLocation
 * @brief Specifies the exact genomic location of a mutation
 */
struct MutationLocation {
    int chromosomeIndex;    ///< Which chromosome pair (0-indexed)
    int geneIndex;          ///< Which gene on the chromosome
    int positionInGene;     ///< Position within the gene (for fine-grained tracking)
    bool isMaternal;        ///< Whether on maternal (true) or paternal (false) chromosome

    MutationLocation()
        : chromosomeIndex(0), geneIndex(0), positionInGene(0), isMaternal(true) {}

    MutationLocation(int chromIdx, int geneIdx, int pos = 0, bool maternal = true)
        : chromosomeIndex(chromIdx), geneIndex(geneIdx),
          positionInGene(pos), isMaternal(maternal) {}

    bool operator==(const MutationLocation& other) const {
        return chromosomeIndex == other.chromosomeIndex &&
               geneIndex == other.geneIndex &&
               positionInGene == other.positionInGene &&
               isMaternal == other.isMaternal;
    }

    /**
     * @brief Get a unique hash for this location
     */
    uint64_t getHash() const {
        return (static_cast<uint64_t>(chromosomeIndex) << 48) |
               (static_cast<uint64_t>(geneIndex) << 32) |
               (static_cast<uint64_t>(positionInGene) << 16) |
               (isMaternal ? 1 : 0);
    }
};

// =============================================================================
// MUTATION STRUCTURE
// =============================================================================

/**
 * @struct Mutation
 * @brief Complete description of a single mutation event
 *
 * Captures all information about a mutation including its type, location,
 * magnitude, fitness effects, and metadata for tracking.
 */
struct Mutation {
    /**
     * @brief Unique identifier for this mutation
     */
    uint64_t id;

    /**
     * @brief Type of mutation
     */
    MutationCategory type;

    /**
     * @brief Genomic location of the mutation
     */
    MutationLocation location;

    /**
     * @brief Magnitude of the change (interpretation depends on type)
     *
     * For POINT_MUTATION: the change in gene value
     * For DUPLICATION: number of copies created
     * For DELETION: number of genes deleted
     * For INVERSION: length of inverted segment
     * For REGULATORY: change in expression level
     */
    float magnitude;

    /**
     * @brief Classification of the mutation's effect on fitness
     */
    MutationEffect effect;

    /**
     * @brief Quantitative fitness effect (-1 to +1)
     *
     * Negative values decrease fitness, positive increase it.
     * Magnitude indicates strength of the effect.
     */
    float fitnessEffect;

    /**
     * @brief The gene type affected by this mutation
     */
    GeneType affectedGeneType;

    /**
     * @brief Original value before mutation (for tracking)
     */
    float originalValue;

    /**
     * @brief New value after mutation
     */
    float newValue;

    /**
     * @brief Generation when mutation occurred
     */
    int generationOccurred;

    /**
     * @brief Whether the mutation is dominant
     */
    bool isDominant;

    /**
     * @brief Whether this mutation has been fixed in the population
     */
    bool isFixed;

    /**
     * @brief Description of the mutation
     */
    std::string description;

    /**
     * @brief Source lineage ID where mutation first appeared
     */
    uint64_t sourceLineageId;

    /**
     * @brief Default constructor
     */
    Mutation();

    /**
     * @brief Parameterized constructor
     */
    Mutation(MutationCategory t, const MutationLocation& loc, float mag,
             MutationEffect eff = MutationEffect::NEUTRAL, float fitEffect = 0.0f);

    /**
     * @brief Check if mutation is beneficial in current context
     */
    bool isBeneficial() const { return effect == MutationEffect::BENEFICIAL; }

    /**
     * @brief Check if mutation is deleterious
     */
    bool isDeleterious() const {
        return effect == MutationEffect::DELETERIOUS || effect == MutationEffect::LETHAL;
    }

    /**
     * @brief Check if mutation is neutral
     */
    bool isNeutral() const { return effect == MutationEffect::NEUTRAL; }

private:
    static uint64_t nextId;
};

// =============================================================================
// MUTATION HOTSPOT STRUCTURE
// =============================================================================

/**
 * @struct MutationHotspot
 * @brief Defines a genomic region with elevated mutation rates
 *
 * Certain regions of the genome are more prone to mutations due to various
 * molecular and structural reasons. This structure defines such hotspots.
 */
struct MutationHotspot {
    /**
     * @brief Location of the hotspot
     */
    MutationLocation location;

    /**
     * @brief Multiplier applied to base mutation rate
     *
     * Values > 1.0 increase mutation rate, values < 1.0 decrease it.
     * Range: 0.1 to 10.0
     */
    float mutationRateMultiplier;

    /**
     * @brief Preferred type of mutation at this hotspot
     *
     * Some hotspots favor certain mutation types, e.g., repetitive
     * sequences favor duplications and deletions.
     */
    MutationCategory preferredMutationType;

    /**
     * @brief Reason for the elevated mutation rate
     */
    HotspotReason reason;

    /**
     * @brief Size of the hotspot region (in genes)
     */
    int regionSize;

    /**
     * @brief Whether this hotspot is currently active
     *
     * Some hotspots may be context-dependent (e.g., activated by stress)
     */
    bool isActive;

    /**
     * @brief Text description of the hotspot
     */
    std::string description;

    /**
     * @brief Default constructor
     */
    MutationHotspot()
        : mutationRateMultiplier(1.0f),
          preferredMutationType(MutationCategory::POINT_MUTATION),
          reason(HotspotReason::UNKNOWN),
          regionSize(1),
          isActive(true) {}

    /**
     * @brief Parameterized constructor
     */
    MutationHotspot(const MutationLocation& loc, float multiplier,
                    MutationCategory prefType, HotspotReason r,
                    int size = 1, const std::string& desc = "")
        : location(loc), mutationRateMultiplier(multiplier),
          preferredMutationType(prefType), reason(r),
          regionSize(size), isActive(true), description(desc) {}

    /**
     * @brief Check if a location is within this hotspot
     */
    bool containsLocation(const MutationLocation& loc) const {
        return loc.chromosomeIndex == location.chromosomeIndex &&
               loc.geneIndex >= location.geneIndex &&
               loc.geneIndex < location.geneIndex + regionSize;
    }
};

// =============================================================================
// MUTATION FATE TRACKING
// =============================================================================

/**
 * @struct MutationFate
 * @brief Tracks the evolutionary fate of a mutation over generations
 */
struct MutationFate {
    uint64_t mutationId;              ///< ID of the tracked mutation
    int generationAppeared;           ///< When the mutation first appeared
    int generationLastSeen;           ///< Last generation where mutation was present
    std::vector<float> frequencyHistory; ///< Allele frequency over time
    bool isFixed;                     ///< Whether mutation reached fixation
    bool isLost;                      ///< Whether mutation was lost
    int peakFrequencyGeneration;      ///< Generation of highest frequency
    float peakFrequency;              ///< Highest frequency achieved

    MutationFate()
        : mutationId(0), generationAppeared(0), generationLastSeen(0),
          isFixed(false), isLost(false), peakFrequencyGeneration(0),
          peakFrequency(0.0f) {}

    /**
     * @brief Update fate with new frequency data
     */
    void update(int generation, float frequency) {
        generationLastSeen = generation;
        frequencyHistory.push_back(frequency);

        if (frequency > peakFrequency) {
            peakFrequency = frequency;
            peakFrequencyGeneration = generation;
        }

        if (frequency >= 0.99f) isFixed = true;
        if (frequency <= 0.0f) isLost = true;
    }

    /**
     * @brief Get the current frequency trend
     * @return Positive if increasing, negative if decreasing
     */
    float getTrend(int windowSize = 10) const {
        if (frequencyHistory.size() < 2) return 0.0f;

        size_t start = frequencyHistory.size() > static_cast<size_t>(windowSize)
                       ? frequencyHistory.size() - windowSize : 0;
        size_t end = frequencyHistory.size();

        float sumDiff = 0.0f;
        for (size_t i = start + 1; i < end; i++) {
            sumDiff += frequencyHistory[i] - frequencyHistory[i - 1];
        }
        return sumDiff / static_cast<float>(end - start - 1);
    }
};

// =============================================================================
// MUTATION SPECTRUM
// =============================================================================

/**
 * @struct MutationSpectrum
 * @brief Distribution of mutation types in a population or lineage
 */
struct MutationSpectrum {
    std::map<MutationCategory, int> typeCounts;
    std::map<MutationEffect, int> effectCounts;
    int totalMutations;

    MutationSpectrum() : totalMutations(0) {
        // Initialize all categories to 0
        for (int i = 0; i < static_cast<int>(MutationCategory::COUNT); i++) {
            typeCounts[static_cast<MutationCategory>(i)] = 0;
        }
        effectCounts[MutationEffect::BENEFICIAL] = 0;
        effectCounts[MutationEffect::NEUTRAL] = 0;
        effectCounts[MutationEffect::DELETERIOUS] = 0;
        effectCounts[MutationEffect::LETHAL] = 0;
        effectCounts[MutationEffect::CONDITIONAL] = 0;
    }

    /**
     * @brief Add a mutation to the spectrum
     */
    void addMutation(const Mutation& mut) {
        typeCounts[mut.type]++;
        effectCounts[mut.effect]++;
        totalMutations++;
    }

    /**
     * @brief Get proportion of a mutation type
     */
    float getTypeProportion(MutationCategory type) const {
        if (totalMutations == 0) return 0.0f;
        auto it = typeCounts.find(type);
        return it != typeCounts.end()
            ? static_cast<float>(it->second) / totalMutations : 0.0f;
    }

    /**
     * @brief Get proportion of an effect type
     */
    float getEffectProportion(MutationEffect effect) const {
        if (totalMutations == 0) return 0.0f;
        auto it = effectCounts.find(effect);
        return it != effectCounts.end()
            ? static_cast<float>(it->second) / totalMutations : 0.0f;
    }

    /**
     * @brief Get beneficial to deleterious ratio
     */
    float getBeneficialDeleteriousRatio() const {
        int del = effectCounts.at(MutationEffect::DELETERIOUS) +
                  effectCounts.at(MutationEffect::LETHAL);
        if (del == 0) return 1.0f;
        return static_cast<float>(effectCounts.at(MutationEffect::BENEFICIAL)) / del;
    }
};

// =============================================================================
// MUTATION RATE MODIFIERS
// =============================================================================

/**
 * @struct MutationRateModifiers
 * @brief Factors that modify the base mutation rate
 *
 * Various biological and environmental factors can increase or decrease
 * mutation rates. This structure tracks these modifiers.
 */
struct MutationRateModifiers {
    /**
     * @brief Stress-induced mutagenesis modifier
     *
     * Under stress, some organisms increase their mutation rate to generate
     * more variation for adaptation. Range: 1.0 (no stress) to 5.0 (high stress)
     */
    float stressModifier;

    /**
     * @brief DNA repair gene effect
     *
     * Better DNA repair reduces mutation rate. Range: 0.1 (excellent repair)
     * to 2.0 (poor repair)
     */
    float dnaRepairModifier;

    /**
     * @brief Mutator allele effect
     *
     * Some alleles increase mutation rate genome-wide (mutator phenotype).
     * Range: 1.0 (normal) to 10.0 (strong mutator)
     */
    float mutatorAlleleModifier;

    /**
     * @brief Age-related mutation rate increase
     *
     * Older individuals may accumulate more mutations. Range: 1.0 (young) to
     * 3.0 (very old)
     */
    float ageModifier;

    /**
     * @brief Environmental mutagen exposure
     *
     * External mutagens (radiation, chemicals) increase mutation rate.
     * Range: 1.0 (no exposure) to 10.0 (heavy exposure)
     */
    float environmentalModifier;

    /**
     * @brief Replication error rate modifier
     *
     * Affects mutations that occur during DNA replication.
     * Range: 0.5 (high fidelity) to 3.0 (low fidelity)
     */
    float replicationErrorModifier;

    /**
     * @brief Default constructor with neutral values
     */
    MutationRateModifiers()
        : stressModifier(1.0f), dnaRepairModifier(1.0f),
          mutatorAlleleModifier(1.0f), ageModifier(1.0f),
          environmentalModifier(1.0f), replicationErrorModifier(1.0f) {}

    /**
     * @brief Calculate combined modifier
     * @return Total multiplier on base mutation rate
     */
    float getCombinedModifier() const {
        return stressModifier * dnaRepairModifier * mutatorAlleleModifier *
               ageModifier * environmentalModifier * replicationErrorModifier;
    }

    /**
     * @brief Get clamped combined modifier (prevents extreme values)
     */
    float getClampedModifier(float minMod = 0.1f, float maxMod = 20.0f) const {
        float combined = getCombinedModifier();
        return std::max(minMod, std::min(maxMod, combined));
    }
};

// =============================================================================
// MUTATION CONFIGURATION
// =============================================================================

/**
 * @struct MutationConfig
 * @brief Configuration parameters for the mutation system
 */
struct MutationConfig {
    // Base mutation rates
    float baseMutationRate;           ///< Base rate per gene per generation
    float pointMutationStrength;      ///< Maximum magnitude of point mutations

    // Type-specific probabilities (should sum to 1.0)
    float pointMutationProb;          ///< Probability of point mutation
    float duplicationProb;            ///< Probability of gene duplication
    float deletionProb;               ///< Probability of gene deletion
    float inversionProb;              ///< Probability of inversion
    float translocationProb;          ///< Probability of translocation
    float regulatoryProb;             ///< Probability of regulatory mutation
    float wholeGeneDupProb;           ///< Probability of whole gene duplication
    float frameshiftProb;             ///< Probability of frameshift

    // Effect probabilities
    float beneficialProb;             ///< Probability mutation is beneficial
    float neutralProb;                ///< Probability mutation is neutral
    float deleteriousProb;            ///< Probability mutation is deleterious
    float lethalProb;                 ///< Probability mutation is lethal

    // Fitness effect parameters
    float avgBeneficialEffect;        ///< Average beneficial fitness effect
    float avgDeleteriousEffect;       ///< Average deleterious fitness effect
    float fitnessEffectVariance;      ///< Variance in fitness effects

    // Environmental context
    bool enableStressInducedMutagenesis;
    bool enableHotspots;
    bool trackMutationFates;

    /**
     * @brief Default constructor with realistic values
     */
    MutationConfig()
        : baseMutationRate(0.05f),
          pointMutationStrength(0.15f),
          pointMutationProb(0.70f),
          duplicationProb(0.05f),
          deletionProb(0.05f),
          inversionProb(0.05f),
          translocationProb(0.02f),
          regulatoryProb(0.08f),
          wholeGeneDupProb(0.03f),
          frameshiftProb(0.02f),
          beneficialProb(0.01f),
          neutralProb(0.30f),
          deleteriousProb(0.68f),
          lethalProb(0.01f),
          avgBeneficialEffect(0.05f),
          avgDeleteriousEffect(-0.10f),
          fitnessEffectVariance(0.05f),
          enableStressInducedMutagenesis(true),
          enableHotspots(true),
          trackMutationFates(true) {}
};

// =============================================================================
// ENVIRONMENTAL CONTEXT FOR FITNESS CALCULATION
// =============================================================================

/**
 * @struct EnvironmentContext
 * @brief Environmental factors affecting mutation fitness effects
 */
struct EnvironmentContext {
    float temperature;          ///< Normalized temperature (0-1, 0.5 = optimal)
    float resourceAvailability; ///< Food/resource abundance (0-1)
    float predationPressure;    ///< Predation intensity (0-1)
    float competitionLevel;     ///< Intra/interspecific competition (0-1)
    float habitatStability;     ///< Environmental stability (0-1)
    float seasonalPhase;        ///< Current season (0-1, 0=winter, 0.5=summer)

    EnvironmentContext()
        : temperature(0.5f), resourceAvailability(0.5f),
          predationPressure(0.5f), competitionLevel(0.5f),
          habitatStability(0.5f), seasonalPhase(0.5f) {}

    /**
     * @brief Calculate overall environmental stress
     */
    float getStressLevel() const {
        float tempStress = std::abs(temperature - 0.5f) * 2.0f;
        float resourceStress = 1.0f - resourceAvailability;
        return (tempStress + resourceStress + predationPressure + competitionLevel) / 4.0f;
    }
};

// =============================================================================
// MUTATION TRACKER CLASS
// =============================================================================

/**
 * @class MutationTracker
 * @brief Tracks mutation history and analyzes mutation patterns
 *
 * Records all mutations that occur in a population, tracks their fates over
 * generations, and provides analysis tools for understanding mutation dynamics.
 */
class MutationTracker {
public:
    MutationTracker();
    ~MutationTracker() = default;

    // =========================================================================
    // MUTATION RECORDING
    // =========================================================================

    /**
     * @brief Record a new mutation
     * @param mutation The mutation to record
     */
    void recordMutation(const Mutation& mutation);

    /**
     * @brief Record multiple mutations at once
     * @param mutations Vector of mutations to record
     */
    void recordMutations(const std::vector<Mutation>& mutations);

    // =========================================================================
    // MUTATION QUERIES
    // =========================================================================

    /**
     * @brief Get all beneficial mutations recorded
     * @param sinceGeneration Only return mutations after this generation (0 = all)
     * @return Vector of beneficial mutations
     */
    std::vector<Mutation> getBeneficialMutations(int sinceGeneration = 0) const;

    /**
     * @brief Get all deleterious mutations recorded
     * @param sinceGeneration Only return mutations after this generation (0 = all)
     * @return Vector of deleterious mutations
     */
    std::vector<Mutation> getDeleteriousMutations(int sinceGeneration = 0) const;

    /**
     * @brief Get all neutral mutations recorded
     * @param sinceGeneration Only return mutations after this generation (0 = all)
     * @return Vector of neutral mutations
     */
    std::vector<Mutation> getNeutralMutations(int sinceGeneration = 0) const;

    /**
     * @brief Get mutations of a specific type
     * @param type The mutation type to filter by
     * @param sinceGeneration Generation cutoff
     * @return Vector of matching mutations
     */
    std::vector<Mutation> getMutationsByType(MutationCategory type,
                                              int sinceGeneration = 0) const;

    /**
     * @brief Get mutations affecting a specific gene type
     * @param geneType The gene type to filter by
     * @return Vector of mutations affecting that gene type
     */
    std::vector<Mutation> getMutationsForGeneType(GeneType geneType) const;

    // =========================================================================
    // MUTATION RATE ANALYSIS
    // =========================================================================

    /**
     * @brief Get mutation rate for a specific gene type
     * @param geneType The gene type to query
     * @param windowGenerations Number of generations to average over
     * @return Mutations per gene per generation
     */
    float getMutationRate(GeneType geneType, int windowGenerations = 100) const;

    /**
     * @brief Get overall mutation rate
     * @param windowGenerations Number of generations to average over
     * @return Total mutations per genome per generation
     */
    float getOverallMutationRate(int windowGenerations = 100) const;

    // =========================================================================
    // MUTATION SPECTRUM ANALYSIS
    // =========================================================================

    /**
     * @brief Get the distribution of mutation types
     * @param sinceGeneration Generation cutoff (0 = all time)
     * @return MutationSpectrum with type and effect distributions
     */
    MutationSpectrum getMutationSpectrum(int sinceGeneration = 0) const;

    /**
     * @brief Get mutation spectrum for a specific lineage
     * @param lineageId The lineage to analyze
     * @return MutationSpectrum for that lineage
     */
    MutationSpectrum getLineageSpectrum(uint64_t lineageId) const;

    // =========================================================================
    // MUTATION FATE TRACKING
    // =========================================================================

    /**
     * @brief Track the fate of a mutation over generations
     * @param mutationId ID of the mutation to track
     * @param generations Number of generations to track forward
     * @return MutationFate structure with tracking data
     */
    MutationFate trackMutationFate(uint64_t mutationId, int generations) const;

    /**
     * @brief Update mutation frequencies for fate tracking
     * @param generation Current generation
     * @param mutationFrequencies Map of mutation ID to current frequency
     */
    void updateMutationFrequencies(int generation,
                                    const std::map<uint64_t, float>& mutationFrequencies);

    /**
     * @brief Get mutations that reached fixation
     * @return Vector of fixed mutations
     */
    std::vector<Mutation> getFixedMutations() const;

    /**
     * @brief Get mutations that were lost
     * @return Vector of lost mutations
     */
    std::vector<Mutation> getLostMutations() const;

    // =========================================================================
    // STATISTICS
    // =========================================================================

    /**
     * @brief Get total number of recorded mutations
     */
    size_t getTotalMutationCount() const { return allMutations.size(); }

    /**
     * @brief Get number of mutations in a generation
     */
    int getMutationsInGeneration(int generation) const;

    /**
     * @brief Get average fitness effect of all mutations
     */
    float getAverageFitnessEffect() const;

    /**
     * @brief Get fitness effect variance
     */
    float getFitnessEffectVariance() const;

    // =========================================================================
    // MANAGEMENT
    // =========================================================================

    /**
     * @brief Clear all recorded mutations
     */
    void clear();

    /**
     * @brief Set maximum history size
     * @param maxMutations Maximum mutations to keep in memory
     */
    void setMaxHistorySize(size_t maxMutations);

    /**
     * @brief Export mutation data to CSV
     * @param filename Output file path
     */
    void exportToCSV(const std::string& filename) const;

    /**
     * @brief Set current generation for tracking
     */
    void setCurrentGeneration(int gen) { currentGeneration = gen; }

private:
    std::vector<Mutation> allMutations;
    std::map<uint64_t, MutationFate> mutationFates;
    std::map<GeneType, std::vector<uint64_t>> mutationsByGeneType;
    std::map<int, std::vector<uint64_t>> mutationsByGeneration;
    size_t maxHistorySize;
    int currentGeneration;

    void pruneHistory();
};

// =============================================================================
// MUTATION EFFECT CALCULATOR CLASS
// =============================================================================

/**
 * @class MutationEffectCalculator
 * @brief Calculates fitness effects of mutations in various contexts
 *
 * Determines whether a mutation is beneficial, neutral, or deleterious
 * based on environmental context, epistatic interactions, and the
 * organism's ecological niche.
 */
class MutationEffectCalculator {
public:
    MutationEffectCalculator();
    ~MutationEffectCalculator() = default;

    // =========================================================================
    // FITNESS EFFECT CALCULATION
    // =========================================================================

    /**
     * @brief Calculate the fitness effect of a mutation
     * @param mutation The mutation to evaluate
     * @param environment Current environmental context
     * @return Fitness effect value (-1 to +1)
     */
    float calculateFitnessEffect(const Mutation& mutation,
                                  const EnvironmentContext& environment) const;

    /**
     * @brief Check if mutation is positive in a specific niche
     * @param mutation The mutation to evaluate
     * @param niche The ecological niche
     * @return True if mutation is beneficial in this niche
     */
    bool isPositiveInContext(const Mutation& mutation,
                              const EcologicalNiche& niche) const;

    /**
     * @brief Classify the effect of a mutation
     * @param mutation The mutation to classify
     * @param environment Environmental context
     * @return MutationEffect classification
     */
    MutationEffect classifyEffect(const Mutation& mutation,
                                   const EnvironmentContext& environment) const;

    // =========================================================================
    // EPISTATIC INTERACTIONS
    // =========================================================================

    /**
     * @brief Calculate epistatic modifier for a mutation
     *
     * Epistasis occurs when the effect of one mutation depends on the
     * presence of other mutations. This calculates the modifier.
     *
     * @param newMutation The new mutation being evaluated
     * @param existingMutations Mutations already present in the genome
     * @return Epistatic modifier (multiplier on fitness effect)
     */
    float calculateEpistaticModifier(const Mutation& newMutation,
                                      const std::vector<Mutation>& existingMutations) const;

    /**
     * @brief Check for synthetic lethality
     *
     * Some mutation combinations are lethal even if individual mutations
     * are not.
     *
     * @param mutations Set of mutations to check
     * @return True if combination is synthetically lethal
     */
    bool checkSyntheticLethality(const std::vector<Mutation>& mutations) const;

    /**
     * @brief Calculate compensatory effect
     *
     * Some mutations can compensate for the deleterious effects of others.
     *
     * @param deleterious The deleterious mutation
     * @param compensatory The potentially compensatory mutation
     * @return Compensation coefficient (0 = none, 1 = full compensation)
     */
    float calculateCompensation(const Mutation& deleterious,
                                 const Mutation& compensatory) const;

    // =========================================================================
    // TRAIT-SPECIFIC EFFECTS
    // =========================================================================

    /**
     * @brief Get fitness effect for a specific trait change
     * @param trait The affected trait
     * @param oldValue Previous trait value
     * @param newValue New trait value
     * @param environment Environmental context
     * @return Fitness effect of this trait change
     */
    float getTraitFitnessEffect(GeneType trait, float oldValue, float newValue,
                                 const EnvironmentContext& environment) const;

    // =========================================================================
    // CONFIGURATION
    // =========================================================================

    /**
     * @brief Set the base distribution of effects
     * @param beneficialProb Probability of beneficial effect
     * @param neutralProb Probability of neutral effect
     * @param deleteriousProb Probability of deleterious effect
     */
    void setEffectDistribution(float beneficialProb, float neutralProb,
                                float deleteriousProb);

private:
    float beneficialProb;
    float neutralProb;
    float deleteriousProb;

    // Trait-specific optimal values for fitness calculation
    std::map<GeneType, std::function<float(float, const EnvironmentContext&)>> traitOptimalFunctions;

    void initializeTraitFunctions();
};

// =============================================================================
// MUTATION SYSTEM CLASS
// =============================================================================

/**
 * @class MutationSystem
 * @brief Main class for applying mutations to genomes
 *
 * Provides comprehensive mutation functionality including various mutation
 * types, hotspot effects, rate modifiers, and tracking. This is the primary
 * interface for the mutation system.
 */
class MutationSystem {
public:
    MutationSystem();
    explicit MutationSystem(const MutationConfig& config);
    ~MutationSystem() = default;

    // =========================================================================
    // PRIMARY MUTATION INTERFACE
    // =========================================================================

    /**
     * @brief Apply mutations to a genome with various types
     * @param genome The genome to mutate
     * @param modifiers Rate modifiers to apply
     * @return Vector of mutations that occurred
     */
    std::vector<Mutation> mutateWithTypes(DiploidGenome& genome,
                                           const MutationRateModifiers& modifiers =
                                               MutationRateModifiers());

    /**
     * @brief Apply mutations using current configuration
     * @param genome The genome to mutate
     * @return Vector of mutations that occurred
     */
    std::vector<Mutation> mutate(DiploidGenome& genome);

    // =========================================================================
    // SPECIFIC MUTATION OPERATIONS
    // =========================================================================

    /**
     * @brief Apply a point mutation to a gene
     * @param gene The gene to mutate
     * @param strength Mutation strength (magnitude)
     * @return The mutation that occurred (or empty if none)
     */
    Mutation applyPointMutation(Gene& gene, float strength = -1.0f);

    /**
     * @brief Apply a gene duplication to a chromosome
     * @param chromosome The chromosome to modify
     * @param geneIndex Index of gene to duplicate
     * @return The mutation that occurred
     */
    Mutation applyDuplication(Chromosome& chromosome, size_t geneIndex);

    /**
     * @brief Apply a gene deletion to a chromosome
     * @param chromosome The chromosome to modify
     * @param geneIndex Index of gene to delete
     * @return The mutation that occurred
     */
    Mutation applyDeletion(Chromosome& chromosome, size_t geneIndex);

    /**
     * @brief Apply an inversion to a chromosome segment
     * @param chromosome The chromosome to modify
     * @param startIndex Start of segment to invert
     * @param endIndex End of segment to invert
     * @return The mutation that occurred
     */
    Mutation applyInversion(Chromosome& chromosome, size_t startIndex, size_t endIndex);

    /**
     * @brief Apply a regulatory mutation to a gene
     * @param gene The gene to modify
     * @param expressionChange Change in expression level
     * @return The mutation that occurred
     */
    Mutation applyRegulatory(Gene& gene, float expressionChange = -1.0f);

    /**
     * @brief Apply a translocation between chromosomes
     * @param genome The genome to modify
     * @param sourceChrom Source chromosome index
     * @param targetChrom Target chromosome index
     * @param geneIndex Gene to move
     * @return The mutation that occurred
     */
    Mutation applyTranslocation(DiploidGenome& genome, size_t sourceChrom,
                                 size_t targetChrom, size_t geneIndex);

    /**
     * @brief Apply whole gene duplication
     * @param chromosome The chromosome to modify
     * @param geneIndex Index of gene to duplicate
     * @return The mutation that occurred
     */
    Mutation applyWholeGeneDuplication(Chromosome& chromosome, size_t geneIndex);

    /**
     * @brief Apply frameshift mutation
     * @param chromosome The chromosome to modify
     * @param geneIndex Starting gene index
     * @return The mutation that occurred
     */
    Mutation applyFrameshift(Chromosome& chromosome, size_t geneIndex);

    // =========================================================================
    // HOTSPOT MANAGEMENT
    // =========================================================================

    /**
     * @brief Add a mutation hotspot
     * @param hotspot The hotspot to add
     */
    void addHotspot(const MutationHotspot& hotspot);

    /**
     * @brief Remove a hotspot at a location
     * @param location Location to clear
     */
    void removeHotspot(const MutationLocation& location);

    /**
     * @brief Get all active hotspots
     * @return Vector of hotspots
     */
    const std::vector<MutationHotspot>& getHotspots() const { return hotspots; }

    /**
     * @brief Check if a location is in a hotspot
     * @param location The location to check
     * @return Pointer to hotspot if found, nullptr otherwise
     */
    const MutationHotspot* getHotspotAt(const MutationLocation& location) const;

    /**
     * @brief Get effective mutation rate at a location
     * @param location The genomic location
     * @param baseRate The base mutation rate
     * @return Adjusted mutation rate considering hotspots
     */
    float getEffectiveMutationRate(const MutationLocation& location,
                                    float baseRate) const;

    // =========================================================================
    // RATE MODIFIER FUNCTIONS
    // =========================================================================

    /**
     * @brief Calculate stress-induced mutagenesis modifier
     * @param stressLevel Current stress level (0-1)
     * @return Modifier to apply to mutation rate
     */
    float calculateStressModifier(float stressLevel) const;

    /**
     * @brief Calculate DNA repair gene effect
     * @param repairEfficiency DNA repair efficiency (0-1)
     * @return Modifier to apply to mutation rate
     */
    float calculateDNARepairModifier(float repairEfficiency) const;

    /**
     * @brief Calculate mutator allele effect
     * @param mutatorStrength Strength of mutator phenotype (0-1)
     * @return Modifier to apply to mutation rate
     */
    float calculateMutatorModifier(float mutatorStrength) const;

    // =========================================================================
    // EFFECT CALCULATION
    // =========================================================================

    /**
     * @brief Get the mutation effect calculator
     * @return Reference to the effect calculator
     */
    MutationEffectCalculator& getEffectCalculator() { return effectCalculator; }

    /**
     * @brief Calculate fitness effect of a mutation
     * @param mutation The mutation to evaluate
     * @param environment Environmental context
     * @return Fitness effect value
     */
    float calculateFitnessEffect(const Mutation& mutation,
                                  const EnvironmentContext& environment) const;

    // =========================================================================
    // TRACKING
    // =========================================================================

    /**
     * @brief Get the mutation tracker
     * @return Reference to the tracker
     */
    MutationTracker& getTracker() { return tracker; }

    /**
     * @brief Get the mutation tracker (const)
     * @return Const reference to the tracker
     */
    const MutationTracker& getTracker() const { return tracker; }

    // =========================================================================
    // CONFIGURATION
    // =========================================================================

    /**
     * @brief Set mutation configuration
     * @param newConfig New configuration to use
     */
    void setConfig(const MutationConfig& newConfig) { config = newConfig; }

    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    const MutationConfig& getConfig() const { return config; }

    /**
     * @brief Set current generation for tracking
     * @param generation Current generation number
     */
    void setCurrentGeneration(int generation);

    /**
     * @brief Update environmental context
     * @param env New environment context
     */
    void setEnvironment(const EnvironmentContext& env) { environment = env; }

    // =========================================================================
    // UTILITY
    // =========================================================================

    /**
     * @brief Convert mutation category to string
     */
    static const char* mutationCategoryToString(MutationCategory category);

    /**
     * @brief Convert mutation effect to string
     */
    static const char* mutationEffectToString(MutationEffect effect);

    /**
     * @brief Convert hotspot reason to string
     */
    static const char* hotspotReasonToString(HotspotReason reason);

private:
    MutationConfig config;
    MutationTracker tracker;
    MutationEffectCalculator effectCalculator;
    std::vector<MutationHotspot> hotspots;
    EnvironmentContext environment;
    int currentGeneration;

    // Helper methods
    MutationCategory selectMutationType() const;
    MutationEffect determineMutationEffect(const Mutation& mutation) const;
    float generateFitnessEffect(MutationEffect effect) const;
    void initializeDefaultHotspots();

    /**
     * @brief Apply a mutation of specific type to a chromosome
     * @param type The mutation category to apply
     * @param chromosome The chromosome to mutate
     * @param geneIdx Gene index for the mutation
     * @param loc Location information
     * @return The mutation that was applied
     */
    Mutation applyMutationOfType(MutationCategory type, Chromosome& chromosome,
                                  size_t geneIdx, const MutationLocation& loc);
};

// =============================================================================
// INLINE UTILITY IMPLEMENTATIONS
// =============================================================================

inline const char* MutationSystem::mutationCategoryToString(MutationCategory category) {
    switch (category) {
        case MutationCategory::POINT_MUTATION:        return "Point Mutation";
        case MutationCategory::DUPLICATION:           return "Duplication";
        case MutationCategory::DELETION:              return "Deletion";
        case MutationCategory::INVERSION:             return "Inversion";
        case MutationCategory::TRANSLOCATION:         return "Translocation";
        case MutationCategory::REGULATORY:            return "Regulatory";
        case MutationCategory::WHOLE_GENE_DUPLICATION:return "Whole Gene Duplication";
        case MutationCategory::FRAMESHIFT:            return "Frameshift";
        case MutationCategory::SILENT:                return "Silent";
        case MutationCategory::BLOCK_MUTATION:        return "Block Mutation";
        default:                                      return "Unknown";
    }
}

inline const char* MutationSystem::mutationEffectToString(MutationEffect effect) {
    switch (effect) {
        case MutationEffect::BENEFICIAL:  return "Beneficial";
        case MutationEffect::NEUTRAL:     return "Neutral";
        case MutationEffect::DELETERIOUS: return "Deleterious";
        case MutationEffect::LETHAL:      return "Lethal";
        case MutationEffect::CONDITIONAL: return "Conditional";
        default:                          return "Unknown";
    }
}

inline const char* MutationSystem::hotspotReasonToString(HotspotReason reason) {
    switch (reason) {
        case HotspotReason::REPETITIVE_SEQUENCE:    return "Repetitive Sequence";
        case HotspotReason::FRAGILE_SITE:           return "Fragile Site";
        case HotspotReason::RECOMBINATION_HOTSPOT:  return "Recombination Hotspot";
        case HotspotReason::METHYLATION_PRONE:      return "Methylation Prone";
        case HotspotReason::OXIDATIVE_DAMAGE:       return "Oxidative Damage";
        case HotspotReason::REPLICATION_FORK_PAUSE: return "Replication Fork Pause";
        case HotspotReason::TRANSCRIPTION_COUPLED:  return "Transcription Coupled";
        case HotspotReason::SECONDARY_STRUCTURE:    return "Secondary Structure";
        case HotspotReason::UNKNOWN:                return "Unknown";
        default:                                    return "Unknown";
    }
}

} // namespace genetics
