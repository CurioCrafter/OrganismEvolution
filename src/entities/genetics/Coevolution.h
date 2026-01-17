#pragma once

/**
 * @file Coevolution.h
 * @brief Comprehensive coevolutionary dynamics tracking system
 *
 * This header defines structures and classes for tracking coevolutionary
 * relationships between species, including:
 * - Predator-prey arms races
 * - Mutualistic relationships (pollinator-plant, etc.)
 * - Parasitic interactions
 * - Mimicry complexes (Batesian and Mullerian)
 * - Competitive character displacement
 * - Red Queen dynamics and evolutionary rate tracking
 *
 * The system monitors trait correlations between species pairs, tracks
 * escalation levels in antagonistic relationships, and measures the
 * oscillating fitness dynamics characteristic of coevolutionary systems.
 *
 * @author OrganismEvolution Development Team
 * @date 2025
 */

#include "DiploidGenome.h"
#include "Gene.h"
#include <vector>
#include <map>
#include <deque>
#include <string>
#include <memory>
#include <functional>

// Forward declarations
class Creature;

namespace genetics {

// Forward declarations within namespace
class Species;
class SpeciationTracker;

// =============================================================================
// COEVOLUTION TYPE ENUMERATION
// =============================================================================

/**
 * @enum CoevolutionType
 * @brief Categorizes the type of coevolutionary interaction between species
 *
 * Coevolutionary relationships fall into several major categories, each with
 * distinct dynamics and selective pressures.
 */
enum class CoevolutionType {
    /**
     * @brief Predator-prey arms race
     *
     * Antagonistic relationship where predators evolve better hunting abilities
     * while prey evolve better defenses or escape mechanisms. Classic example
     * of reciprocal escalation (e.g., cheetah-gazelle speed evolution).
     */
    PREDATOR_PREY,

    /**
     * @brief Pollinator-plant mutualism
     *
     * Mutualistic relationship where plants evolve traits to attract pollinators
     * while pollinators evolve traits to more efficiently extract nectar/pollen.
     * Often results in coevolved morphological matching.
     */
    POLLINATOR_PLANT,

    /**
     * @brief Parasite-host antagonism
     *
     * Antagonistic relationship where parasites evolve to exploit hosts while
     * hosts evolve resistance. Often characterized by rapid evolutionary cycles
     * and negative frequency-dependent selection.
     */
    PARASITE_HOST,

    /**
     * @brief Mimicry complexes (Batesian/Mullerian)
     *
     * Relationship where one or more species evolve to resemble another.
     * Batesian: harmless mimic resembles dangerous model
     * Mullerian: multiple dangerous species converge on similar warning signals
     */
    MIMICRY,

    /**
     * @brief General cooperative mutualism
     *
     * Mutualistic relationship not involving pollination, such as:
     * - Cleaner fish and their clients
     * - Mycorrhizal fungi and plants
     * - Nitrogen-fixing bacteria and legumes
     */
    MUTUALISM,

    /**
     * @brief Competitive character displacement
     *
     * When two species compete for the same resources, natural selection may
     * favor divergent traits that reduce competition (niche partitioning).
     * Classic example: Darwin's finches beak size differentiation.
     */
    COMPETITION
};

// =============================================================================
// MIMICRY TYPE ENUMERATION
// =============================================================================

/**
 * @enum MimicryType
 * @brief Distinguishes between Batesian and Mullerian mimicry
 */
enum class MimicryType {
    /**
     * @brief Batesian mimicry
     *
     * A harmless species (mimic) resembles a harmful/dangerous species (model).
     * The mimic gains protection without paying the cost of being dangerous.
     * Example: Viceroy butterfly mimicking Monarch butterfly.
     */
    BATESIAN,

    /**
     * @brief Mullerian mimicry
     *
     * Two or more harmful species resemble each other, sharing the cost of
     * educating predators. Both species benefit from the shared warning signal.
     * Example: Various species of poison dart frogs with similar coloration.
     */
    MULLERIAN
};

// =============================================================================
// ADVANTAGE SIDE ENUMERATION
// =============================================================================

/**
 * @enum AdvantageSide
 * @brief Indicates which side is currently "winning" in an antagonistic relationship
 */
enum class AdvantageSide {
    NEUTRAL,        ///< Neither side has a significant advantage
    SPECIES_1,      ///< First species (e.g., predator) has the advantage
    SPECIES_2,      ///< Second species (e.g., prey) has the advantage
    OSCILLATING     ///< Advantage is rapidly shifting between species
};

// =============================================================================
// TRAIT CORRELATION STRUCTURE
// =============================================================================

/**
 * @struct TraitCorrelation
 * @brief Represents a correlation between traits in two coevolving species
 *
 * Tracks how changes in one species' trait correlate with changes in
 * another species' trait over evolutionary time.
 */
struct TraitCorrelation {
    GeneType species1Trait;         ///< The trait being tracked in species 1
    GeneType species2Trait;         ///< The trait being tracked in species 2
    float correlationCoefficient;   ///< Pearson correlation coefficient (-1 to 1)
    float pValue;                   ///< Statistical significance of correlation
    int sampleSize;                 ///< Number of generations used to calculate
    bool isSignificant;             ///< True if p-value < significance threshold (0.05)

    /**
     * @brief Default constructor
     */
    TraitCorrelation()
        : species1Trait(GeneType::SIZE)
        , species2Trait(GeneType::SIZE)
        , correlationCoefficient(0.0f)
        , pValue(1.0f)
        , sampleSize(0)
        , isSignificant(false) {}

    /**
     * @brief Parameterized constructor
     */
    TraitCorrelation(GeneType t1, GeneType t2, float corr = 0.0f)
        : species1Trait(t1)
        , species2Trait(t2)
        , correlationCoefficient(corr)
        , pValue(1.0f)
        , sampleSize(0)
        , isSignificant(false) {}
};

// =============================================================================
// COEVOLUTIONARY PAIR STRUCTURE
// =============================================================================

/**
 * @struct CoevolutionaryPair
 * @brief Represents a coevolutionary relationship between two species
 *
 * This structure tracks all aspects of a coevolutionary interaction,
 * including the strength of the interaction, how long it has persisted,
 * which traits are correlated, and the current escalation level.
 */
struct CoevolutionaryPair {
    SpeciesId species1Id;           ///< First species in the pair
    SpeciesId species2Id;           ///< Second species in the pair
    CoevolutionType type;           ///< Type of coevolutionary interaction
    float interactionStrength;      ///< Strength of ecological interaction (0-1)
    int generationsLinked;          ///< Number of generations relationship has persisted
    int discoveryGeneration;        ///< Generation when relationship was first detected

    /**
     * @brief Map of correlated traits between the two species
     *
     * Key: Descriptive name for the trait pair (e.g., "speed_speed")
     * Value: TraitCorrelation structure with statistical details
     */
    std::map<std::string, TraitCorrelation> traitCorrelations;

    /**
     * @brief Escalation level in antagonistic relationships
     *
     * For arms races (predator-prey, parasite-host), this measures how
     * far the interaction has escalated from initial conditions.
     * Range: 0.0 (no escalation) to unbounded (extreme escalation)
     * Mutualistic relationships may have negative values (de-escalation).
     */
    float escalationLevel;

    /**
     * @brief Current advantage in antagonistic relationships
     */
    AdvantageSide currentAdvantage;

    /**
     * @brief Historical record of interaction strength
     */
    std::deque<float> strengthHistory;

    /**
     * @brief Historical record of escalation levels
     */
    std::deque<float> escalationHistory;

    /**
     * @brief Default constructor
     */
    CoevolutionaryPair()
        : species1Id(0)
        , species2Id(0)
        , type(CoevolutionType::COMPETITION)
        , interactionStrength(0.0f)
        , generationsLinked(0)
        , discoveryGeneration(0)
        , escalationLevel(0.0f)
        , currentAdvantage(AdvantageSide::NEUTRAL) {}

    /**
     * @brief Parameterized constructor
     * @param sp1 First species ID
     * @param sp2 Second species ID
     * @param t Coevolution type
     * @param generation Current generation when pair is created
     */
    CoevolutionaryPair(SpeciesId sp1, SpeciesId sp2, CoevolutionType t, int generation)
        : species1Id(sp1)
        , species2Id(sp2)
        , type(t)
        , interactionStrength(0.5f)
        , generationsLinked(0)
        , discoveryGeneration(generation)
        , escalationLevel(0.0f)
        , currentAdvantage(AdvantageSide::NEUTRAL) {}

    /**
     * @brief Add a trait correlation to the pair
     * @param name Descriptive name for the correlation
     * @param correlation The TraitCorrelation to add
     */
    void addTraitCorrelation(const std::string& name, const TraitCorrelation& correlation) {
        traitCorrelations[name] = correlation;
    }

    /**
     * @brief Record current escalation level in history
     * @param maxHistory Maximum number of historical entries to keep
     */
    void recordEscalation(size_t maxHistory = 100) {
        escalationHistory.push_back(escalationLevel);
        if (escalationHistory.size() > maxHistory) {
            escalationHistory.pop_front();
        }
    }

    /**
     * @brief Record current interaction strength in history
     * @param maxHistory Maximum number of historical entries to keep
     */
    void recordStrength(size_t maxHistory = 100) {
        strengthHistory.push_back(interactionStrength);
        if (strengthHistory.size() > maxHistory) {
            strengthHistory.pop_front();
        }
    }

    /**
     * @brief Check if this is an antagonistic relationship
     * @return True if predator-prey, parasite-host, or competition
     */
    bool isAntagonistic() const {
        return type == CoevolutionType::PREDATOR_PREY ||
               type == CoevolutionType::PARASITE_HOST ||
               type == CoevolutionType::COMPETITION;
    }

    /**
     * @brief Check if this is a mutualistic relationship
     * @return True if pollinator-plant, mutualism, or Mullerian mimicry
     */
    bool isMutualistic() const {
        return type == CoevolutionType::POLLINATOR_PLANT ||
               type == CoevolutionType::MUTUALISM;
    }
};

// =============================================================================
// ARMS RACE STRUCTURE
// =============================================================================

/**
 * @struct PredatorTraits
 * @brief Collection of traits relevant to predator hunting ability
 */
struct PredatorTraits {
    float speed;            ///< Movement speed (pursuit/ambush capability)
    float venom;            ///< Venomous capability (0 = none, 1 = lethal)
    float stealth;          ///< Ability to approach undetected
    float attackPower;      ///< Damage dealt per attack
    float senseAcuity;      ///< Ability to detect prey
    float stamina;          ///< Pursuit duration capability

    PredatorTraits()
        : speed(1.0f), venom(0.0f), stealth(0.5f)
        , attackPower(1.0f), senseAcuity(0.5f), stamina(0.5f) {}

    /**
     * @brief Calculate overall predator effectiveness score
     * @return Weighted combination of all predator traits
     */
    float getEffectiveness() const {
        return (speed * 0.25f) + (venom * 0.15f) + (stealth * 0.2f) +
               (attackPower * 0.2f) + (senseAcuity * 0.1f) + (stamina * 0.1f);
    }
};

/**
 * @struct PreyTraits
 * @brief Collection of traits relevant to prey survival
 */
struct PreyTraits {
    float speed;            ///< Escape speed
    float armor;            ///< Physical defense (0 = none, 1 = impenetrable)
    float detection;        ///< Ability to detect approaching predators
    float camouflage;       ///< Ability to avoid detection
    float toxicity;         ///< Chemical defense (0 = none, 1 = lethal)
    float agility;          ///< Evasion and maneuverability

    PreyTraits()
        : speed(1.0f), armor(0.0f), detection(0.5f)
        , camouflage(0.3f), toxicity(0.0f), agility(0.5f) {}

    /**
     * @brief Calculate overall prey defense score
     * @return Weighted combination of all prey traits
     */
    float getDefenseScore() const {
        return (speed * 0.25f) + (armor * 0.15f) + (detection * 0.2f) +
               (camouflage * 0.15f) + (toxicity * 0.15f) + (agility * 0.1f);
    }
};

/**
 * @struct ArmsRace
 * @brief Tracks the evolutionary arms race between predator and prey species
 *
 * An arms race occurs when improvements in predator offense drive selection
 * for improved prey defense, which in turn drives selection for even better
 * predator offense, creating an escalating cycle.
 */
struct ArmsRace {
    SpeciesId predatorSpeciesId;    ///< The predator species
    SpeciesId preySpeciesId;        ///< The prey species
    PredatorTraits predatorTraits;  ///< Current predator trait values
    PreyTraits preyTraits;          ///< Current prey trait values

    /**
     * @brief Historical record of escalation levels by generation
     *
     * Each entry represents the escalation level at a specific generation.
     * Increasing values indicate ongoing escalation; plateaus may indicate
     * evolutionary constraints or equilibrium.
     */
    std::vector<float> escalationHistory;

    /**
     * @brief Which side currently has the advantage
     */
    AdvantageSide currentAdvantage;

    /**
     * @brief Generation when the arms race was first detected
     */
    int startGeneration;

    /**
     * @brief Current rate of escalation (change per generation)
     */
    float escalationRate;

    /**
     * @brief Number of advantage reversals (oscillations)
     */
    int oscillationCount;

    /**
     * @brief Default constructor
     */
    ArmsRace()
        : predatorSpeciesId(0)
        , preySpeciesId(0)
        , currentAdvantage(AdvantageSide::NEUTRAL)
        , startGeneration(0)
        , escalationRate(0.0f)
        , oscillationCount(0) {}

    /**
     * @brief Parameterized constructor
     */
    ArmsRace(SpeciesId predator, SpeciesId prey, int generation)
        : predatorSpeciesId(predator)
        , preySpeciesId(prey)
        , currentAdvantage(AdvantageSide::NEUTRAL)
        , startGeneration(generation)
        , escalationRate(0.0f)
        , oscillationCount(0) {}

    /**
     * @brief Calculate the current escalation level
     * @return Sum of predator effectiveness and prey defense scores
     */
    float getCurrentEscalation() const {
        return predatorTraits.getEffectiveness() + preyTraits.getDefenseScore();
    }

    /**
     * @brief Update the advantage based on current traits
     */
    void updateAdvantage() {
        float predScore = predatorTraits.getEffectiveness();
        float preyScore = preyTraits.getDefenseScore();
        float diff = predScore - preyScore;

        AdvantageSide newAdvantage;
        if (std::abs(diff) < 0.1f) {
            newAdvantage = AdvantageSide::NEUTRAL;
        } else if (diff > 0) {
            newAdvantage = AdvantageSide::SPECIES_1;  // Predator advantage
        } else {
            newAdvantage = AdvantageSide::SPECIES_2;  // Prey advantage
        }

        // Track oscillations
        if (currentAdvantage != AdvantageSide::NEUTRAL &&
            newAdvantage != AdvantageSide::NEUTRAL &&
            currentAdvantage != newAdvantage) {
            oscillationCount++;
        }

        currentAdvantage = newAdvantage;
    }

    /**
     * @brief Record current state in history
     */
    void recordState() {
        float escalation = getCurrentEscalation();
        if (!escalationHistory.empty()) {
            escalationRate = escalation - escalationHistory.back();
        }
        escalationHistory.push_back(escalation);
    }

    /**
     * @brief Get the duration of the arms race in generations
     * @param currentGeneration The current simulation generation
     * @return Number of generations since arms race started
     */
    int getDuration(int currentGeneration) const {
        return currentGeneration - startGeneration;
    }
};

// =============================================================================
// MIMICRY COMPLEX STRUCTURE
// =============================================================================

/**
 * @struct MimicryComplex
 * @brief Represents a mimicry ring with one model and multiple mimics
 *
 * Mimicry complexes can be:
 * - Batesian: harmless mimics copying a dangerous model
 * - Mullerian: multiple dangerous species converging on similar signals
 */
struct MimicryComplex {
    /**
     * @brief The model species (toxic/dangerous species being mimicked)
     *
     * In Batesian mimicry, this is the dangerous model.
     * In Mullerian mimicry, this is typically the most abundant co-model.
     */
    SpeciesId modelSpeciesId;

    /**
     * @brief Vector of species that mimic the model
     */
    std::vector<SpeciesId> mimicSpeciesIds;

    /**
     * @brief Type of mimicry (Batesian or Mullerian)
     */
    MimicryType mimicryType;

    /**
     * @brief Accuracy of mimicry (0-1)
     *
     * Measures how closely the mimic's appearance matches the model.
     * Higher values indicate more convincing mimicry.
     */
    float mimicryAccuracy;

    /**
     * @brief Predator recognition rate (0-1)
     *
     * Measures how well predators have learned to recognize and avoid
     * the model's warning signals. Higher values = better protection.
     */
    float predatorRecognition;

    /**
     * @brief Toxicity/danger level of the model (0-1)
     */
    float modelToxicity;

    /**
     * @brief Average toxicity of Mullerian co-mimics (for Mullerian only)
     */
    float averageMimicToxicity;

    /**
     * @brief Generation when the complex was first detected
     */
    int discoveryGeneration;

    /**
     * @brief Stability of the mimicry complex
     *
     * For Batesian mimicry, if mimics become too common relative to models,
     * predators may stop avoiding the signal. This tracks that ratio.
     */
    float modelToMimicRatio;

    /**
     * @brief Default constructor
     */
    MimicryComplex()
        : modelSpeciesId(0)
        , mimicryType(MimicryType::BATESIAN)
        , mimicryAccuracy(0.0f)
        , predatorRecognition(0.5f)
        , modelToxicity(0.5f)
        , averageMimicToxicity(0.0f)
        , discoveryGeneration(0)
        , modelToMimicRatio(1.0f) {}

    /**
     * @brief Add a mimic species to the complex
     * @param mimicId Species ID of the mimic to add
     */
    void addMimic(SpeciesId mimicId) {
        // Avoid duplicates
        for (SpeciesId id : mimicSpeciesIds) {
            if (id == mimicId) return;
        }
        mimicSpeciesIds.push_back(mimicId);
    }

    /**
     * @brief Remove a mimic species (e.g., if it goes extinct)
     * @param mimicId Species ID of the mimic to remove
     */
    void removeMimic(SpeciesId mimicId) {
        mimicSpeciesIds.erase(
            std::remove(mimicSpeciesIds.begin(), mimicSpeciesIds.end(), mimicId),
            mimicSpeciesIds.end()
        );
    }

    /**
     * @brief Check if the mimicry complex is stable
     * @return True if mimic-to-model ratio doesn't destabilize the system
     */
    bool isStable() const {
        if (mimicryType == MimicryType::BATESIAN) {
            // Batesian mimicry breaks down if mimics outnumber models too much
            return modelToMimicRatio > 0.2f;
        }
        // Mullerian mimicry is generally stable
        return true;
    }

    /**
     * @brief Calculate the protection level provided by mimicry
     * @return Estimated predator avoidance probability
     */
    float getProtectionLevel() const {
        float baseProtection = predatorRecognition * mimicryAccuracy;
        if (mimicryType == MimicryType::BATESIAN) {
            // Reduce protection if mimics are too common
            return baseProtection * std::min(1.0f, modelToMimicRatio * 2.0f);
        }
        // Mullerian mimicry: shared toxicity increases protection
        return baseProtection * (1.0f + averageMimicToxicity * 0.5f);
    }
};

// =============================================================================
// RED QUEEN DYNAMICS STRUCTURE
// =============================================================================

/**
 * @struct RedQueenMetrics
 * @brief Tracks Red Queen dynamics in coevolutionary relationships
 *
 * The Red Queen hypothesis states that organisms must constantly evolve
 * just to maintain relative fitness, as competing species are also evolving.
 * This structure tracks the metrics associated with this phenomenon.
 */
struct RedQueenMetrics {
    SpeciesId speciesId;            ///< The species being tracked

    /**
     * @brief Evolutionary rate (change in mean trait values per generation)
     *
     * Higher values indicate faster evolution, which may be necessary
     * when under strong coevolutionary pressure.
     */
    float evolutionaryRate;

    /**
     * @brief Historical record of evolutionary rates
     */
    std::deque<float> rateHistory;

    /**
     * @brief Fitness oscillation amplitude
     *
     * Measures how much mean fitness varies over time due to
     * coevolutionary dynamics. High values indicate strong Red Queen effects.
     */
    float fitnessOscillationAmplitude;

    /**
     * @brief Fitness oscillation frequency
     *
     * How often fitness peaks and troughs occur (cycles per N generations).
     */
    float fitnessOscillationFrequency;

    /**
     * @brief Historical record of mean fitness values
     */
    std::deque<float> fitnessHistory;

    /**
     * @brief Number of adaptation-counter-adaptation cycles detected
     */
    int adaptationCycles;

    /**
     * @brief Mean time lag between adaptation and counter-adaptation
     */
    float meanResponseLag;

    /**
     * @brief Current phase in the adaptation cycle
     *
     * 0.0 = just adapted, 1.0 = about to adapt again
     */
    float cyclePhase;

    /**
     * @brief Whether the species is currently "running in place"
     *
     * True if the species is evolving rapidly but not gaining fitness.
     */
    bool isRunningInPlace;

    /**
     * @brief Default constructor
     */
    RedQueenMetrics()
        : speciesId(0)
        , evolutionaryRate(0.0f)
        , fitnessOscillationAmplitude(0.0f)
        , fitnessOscillationFrequency(0.0f)
        , adaptationCycles(0)
        , meanResponseLag(0.0f)
        , cyclePhase(0.0f)
        , isRunningInPlace(false) {}

    /**
     * @brief Record current evolutionary rate
     * @param rate The current rate to record
     * @param maxHistory Maximum history size
     */
    void recordRate(float rate, size_t maxHistory = 100) {
        evolutionaryRate = rate;
        rateHistory.push_back(rate);
        if (rateHistory.size() > maxHistory) {
            rateHistory.pop_front();
        }
    }

    /**
     * @brief Record current mean fitness
     * @param fitness The fitness value to record
     * @param maxHistory Maximum history size
     */
    void recordFitness(float fitness, size_t maxHistory = 100) {
        fitnessHistory.push_back(fitness);
        if (fitnessHistory.size() > maxHistory) {
            fitnessHistory.pop_front();
        }
        updateOscillationMetrics();
    }

    /**
     * @brief Calculate oscillation metrics from fitness history
     */
    void updateOscillationMetrics() {
        if (fitnessHistory.size() < 10) return;

        // Calculate amplitude as difference between max and min in recent history
        float maxFit = *std::max_element(fitnessHistory.begin(), fitnessHistory.end());
        float minFit = *std::min_element(fitnessHistory.begin(), fitnessHistory.end());
        fitnessOscillationAmplitude = maxFit - minFit;

        // Simple frequency estimation: count zero-crossings of detrended data
        float mean = 0.0f;
        for (float f : fitnessHistory) mean += f;
        mean /= fitnessHistory.size();

        int crossings = 0;
        bool wasAbove = fitnessHistory.front() > mean;
        for (float f : fitnessHistory) {
            bool isAbove = f > mean;
            if (isAbove != wasAbove) {
                crossings++;
                wasAbove = isAbove;
            }
        }
        fitnessOscillationFrequency = static_cast<float>(crossings) /
                                       (2.0f * fitnessHistory.size());

        // Check if running in place: high evolutionary rate but stable/declining fitness
        if (rateHistory.size() >= 10) {
            float avgRate = 0.0f;
            for (float r : rateHistory) avgRate += r;
            avgRate /= rateHistory.size();

            float fitnessChange = fitnessHistory.back() - fitnessHistory.front();
            isRunningInPlace = (avgRate > 0.01f && std::abs(fitnessChange) < 0.05f);
        }
    }
};

// =============================================================================
// COEVOLUTION CONFIGURATION
// =============================================================================

/**
 * @struct CoevolutionConfig
 * @brief Configuration parameters for coevolution tracking
 */
struct CoevolutionConfig {
    /**
     * @brief Minimum interaction strength to consider a coevolutionary pair
     */
    float minInteractionStrength = 0.2f;

    /**
     * @brief Minimum generations of association to confirm coevolution
     */
    int minGenerationsForCoevolution = 20;

    /**
     * @brief Correlation coefficient threshold for significance
     */
    float correlationThreshold = 0.3f;

    /**
     * @brief Maximum number of coevolutionary pairs to track
     */
    int maxTrackedPairs = 100;

    /**
     * @brief Number of generations to keep in history
     */
    int historyLength = 100;

    /**
     * @brief How often to update coevolution metrics (generations)
     */
    int updateFrequency = 5;

    /**
     * @brief Minimum mimicry accuracy to detect a mimicry relationship
     */
    float minMimicryAccuracy = 0.6f;

    /**
     * @brief Enable Red Queen dynamics tracking (computationally intensive)
     */
    bool trackRedQueenDynamics = true;

    /**
     * @brief Enable automatic mimicry detection
     */
    bool detectMimicry = true;

    /**
     * @brief Enable arms race tracking
     */
    bool trackArmsRaces = true;
};

// =============================================================================
// COEVOLUTION STATISTICS
// =============================================================================

/**
 * @struct CoevolutionStats
 * @brief Summary statistics for the coevolution system
 */
struct CoevolutionStats {
    int totalPairs;                     ///< Total coevolutionary pairs tracked
    int predatorPreyPairs;              ///< Number of predator-prey relationships
    int mutualisticPairs;               ///< Number of mutualistic relationships
    int parasitePairs;                  ///< Number of parasite-host relationships
    int mimicryComplexes;               ///< Number of mimicry complexes
    int competitivePairs;               ///< Number of competitive relationships
    int activeArmsRaces;                ///< Number of ongoing arms races
    float averageEscalation;            ///< Mean escalation level across pairs
    float averageInteractionStrength;   ///< Mean interaction strength
    int speciesWithRedQueenDynamics;    ///< Species showing Red Queen effects
    float averageEvolutionaryRate;      ///< Mean evolutionary rate

    CoevolutionStats()
        : totalPairs(0), predatorPreyPairs(0), mutualisticPairs(0)
        , parasitePairs(0), mimicryComplexes(0), competitivePairs(0)
        , activeArmsRaces(0), averageEscalation(0.0f)
        , averageInteractionStrength(0.0f), speciesWithRedQueenDynamics(0)
        , averageEvolutionaryRate(0.0f) {}
};

// =============================================================================
// COEVOLUTION TRACKER CLASS
// =============================================================================

/**
 * @class CoevolutionTracker
 * @brief Main class for tracking and analyzing coevolutionary dynamics
 *
 * This class provides comprehensive functionality for detecting, monitoring,
 * and analyzing coevolutionary relationships between species. It integrates
 * with the existing genetics and speciation systems to provide insights
 * into how species evolve in response to each other.
 *
 * Key features:
 * - Automatic detection of coevolutionary pairs based on interaction data
 * - Tracking of trait correlations between coevolving species
 * - Arms race monitoring for predator-prey and parasite-host relationships
 * - Mimicry complex detection and stability analysis
 * - Red Queen dynamics measurement
 * - Data export for external analysis
 */
class CoevolutionTracker {
public:
    // =========================================================================
    // CONSTRUCTORS & DESTRUCTOR
    // =========================================================================

    /**
     * @brief Default constructor
     */
    CoevolutionTracker();

    /**
     * @brief Constructor with configuration
     * @param config Configuration parameters for the tracker
     */
    explicit CoevolutionTracker(const CoevolutionConfig& config);

    /**
     * @brief Destructor
     */
    ~CoevolutionTracker() = default;

    // =========================================================================
    // PAIR DETECTION METHODS
    // =========================================================================

    /**
     * @brief Detect if two species are coevolving
     *
     * Analyzes interaction patterns, trait correlations, and evolutionary
     * histories to determine if the species have a coevolutionary relationship.
     *
     * @param species1 Pointer to first species
     * @param species2 Pointer to second species
     * @return True if a coevolutionary relationship is detected
     */
    bool detectCoevolutionaryPair(const Species* species1, const Species* species2);

    /**
     * @brief Classify the type of coevolutionary interaction
     *
     * Based on the nature of the interaction (beneficial, harmful, neutral),
     * determines the specific type of coevolution.
     *
     * @param species1 First species in the pair
     * @param species2 Second species in the pair
     * @param interactionMatrix Matrix of species interactions (positive = mutualism, negative = antagonism)
     * @return The classified CoevolutionType
     */
    CoevolutionType classifyInteraction(const Species* species1,
                                        const Species* species2,
                                        const std::map<std::pair<SpeciesId, SpeciesId>, float>& interactionMatrix);

    // =========================================================================
    // ARMS RACE TRACKING
    // =========================================================================

    /**
     * @brief Start tracking an arms race between predator and prey
     *
     * @param predator Pointer to the predator species
     * @param prey Pointer to the prey species
     * @param currentGeneration Current simulation generation
     * @return Reference to the created ArmsRace structure
     */
    ArmsRace& trackArmsRace(const Species* predator, const Species* prey, int currentGeneration);

    /**
     * @brief Update an existing arms race with current trait values
     *
     * @param predatorId Species ID of the predator
     * @param preyId Species ID of the prey
     * @param predatorCreatures Current predator population for trait extraction
     * @param preyCreatures Current prey population for trait extraction
     */
    void updateArmsRace(SpeciesId predatorId, SpeciesId preyId,
                        const std::vector<Creature*>& predatorCreatures,
                        const std::vector<Creature*>& preyCreatures);

    /**
     * @brief Get an existing arms race by species IDs
     * @param predatorId Predator species ID
     * @param preyId Prey species ID
     * @return Pointer to ArmsRace or nullptr if not found
     */
    ArmsRace* getArmsRace(SpeciesId predatorId, SpeciesId preyId);

    /**
     * @brief Get an existing arms race (const version)
     */
    const ArmsRace* getArmsRace(SpeciesId predatorId, SpeciesId preyId) const;

    /**
     * @brief Get all active arms races
     * @return Vector of pointers to all tracked arms races
     */
    std::vector<const ArmsRace*> getAllArmsRaces() const;

    // =========================================================================
    // MUTUALISM TRACKING
    // =========================================================================

    /**
     * @brief Start tracking a mutualistic relationship
     *
     * @param species1 First mutualist species
     * @param species2 Second mutualist species
     * @param type Specific type of mutualism (MUTUALISM or POLLINATOR_PLANT)
     * @param currentGeneration Current simulation generation
     * @return Reference to the created CoevolutionaryPair
     */
    CoevolutionaryPair& trackMutualism(const Species* species1,
                                        const Species* species2,
                                        CoevolutionType type,
                                        int currentGeneration);

    /**
     * @brief Calculate the benefit each species receives from mutualism
     *
     * @param pairId The unique identifier for the coevolutionary pair
     * @param[out] benefit1 Fitness benefit to species 1
     * @param[out] benefit2 Fitness benefit to species 2
     */
    void calculateMutualismBenefits(const std::pair<SpeciesId, SpeciesId>& pairId,
                                    float& benefit1, float& benefit2) const;

    // =========================================================================
    // TRAIT CORRELATION METHODS
    // =========================================================================

    /**
     * @brief Calculate the correlation between traits in two species
     *
     * Uses historical trait data to compute Pearson correlation coefficient
     * between specified traits in two potentially coevolving species.
     *
     * @param species1 First species
     * @param species2 Second species
     * @param trait1 Trait to track in species 1
     * @param trait2 Trait to track in species 2
     * @param windowSize Number of generations to analyze
     * @return TraitCorrelation structure with correlation details
     */
    TraitCorrelation calculateTraitCorrelation(const Species* species1,
                                                const Species* species2,
                                                GeneType trait1,
                                                GeneType trait2,
                                                int windowSize = 50);

    /**
     * @brief Update trait correlations for all tracked pairs
     * @param currentGeneration Current simulation generation
     */
    void updateAllTraitCorrelations(int currentGeneration);

    // =========================================================================
    // ESCALATION MEASUREMENT
    // =========================================================================

    /**
     * @brief Measure the current escalation level of a coevolutionary pair
     *
     * For arms races, this measures how much both species have "escalated"
     * from their baseline traits. For mutualisms, it measures specialization.
     *
     * @param pair The coevolutionary pair to measure
     * @return Current escalation level (0 = no escalation, higher = more escalated)
     */
    float measureEscalation(const CoevolutionaryPair& pair) const;

    /**
     * @brief Calculate the rate of escalation over recent generations
     *
     * @param pair The coevolutionary pair
     * @param windowSize Number of generations to analyze
     * @return Rate of change in escalation per generation
     */
    float calculateEscalationRate(const CoevolutionaryPair& pair, int windowSize = 20) const;

    // =========================================================================
    // MIMICRY DETECTION
    // =========================================================================

    /**
     * @brief Detect if a species is involved in mimicry
     *
     * Analyzes visual traits and toxicity levels to determine if a species
     * is a model, Batesian mimic, or Mullerian co-mimic.
     *
     * @param species The species to analyze
     * @param allSpecies All species in the simulation for comparison
     * @return Pointer to MimicryComplex if mimicry is detected, nullptr otherwise
     */
    MimicryComplex* detectMimicry(const Species* species,
                                   const std::vector<Species*>& allSpecies);

    /**
     * @brief Calculate the visual similarity between two species
     *
     * Compares color patterns, markings, and other visual traits.
     *
     * @param species1 First species
     * @param species2 Second species
     * @return Similarity score (0 = completely different, 1 = identical)
     */
    float calculateVisualSimilarity(const Species* species1, const Species* species2) const;

    /**
     * @brief Get all detected mimicry complexes
     * @return Vector of pointers to all mimicry complexes
     */
    std::vector<const MimicryComplex*> getMimicryComplexes() const;

    /**
     * @brief Update mimicry complex stability based on population ratios
     * @param complex The mimicry complex to update
     * @param modelPopulation Current model species population size
     * @param mimicPopulations Map of mimic species IDs to their population sizes
     */
    void updateMimicryStability(MimicryComplex& complex,
                                int modelPopulation,
                                const std::map<SpeciesId, int>& mimicPopulations);

    // =========================================================================
    // MAIN UPDATE METHOD
    // =========================================================================

    /**
     * @brief Update all coevolutionary dynamics
     *
     * This is the main update method that should be called each generation
     * (or every few generations based on config.updateFrequency).
     *
     * @param currentGeneration The current simulation generation
     * @param allSpecies All active species in the simulation
     * @param creatures All creatures for trait extraction
     * @param interactionMatrix Species interaction strengths
     */
    void updateCoevolutionaryDynamics(int currentGeneration,
                                       const std::vector<Species*>& allSpecies,
                                       const std::vector<Creature*>& creatures,
                                       const std::map<std::pair<SpeciesId, SpeciesId>, float>& interactionMatrix);

    // =========================================================================
    // RED QUEEN DYNAMICS
    // =========================================================================

    /**
     * @brief Track Red Queen dynamics for a species
     *
     * Monitors evolutionary rate and fitness oscillations characteristic
     * of Red Queen dynamics (constant evolution to maintain relative fitness).
     *
     * @param speciesId Species to track
     * @param evolutionaryRate Current rate of trait change
     * @param meanFitness Current mean fitness of the species
     */
    void trackRedQueenDynamics(SpeciesId speciesId, float evolutionaryRate, float meanFitness);

    /**
     * @brief Get Red Queen metrics for a species
     * @param speciesId The species ID
     * @return Pointer to RedQueenMetrics or nullptr if not tracked
     */
    const RedQueenMetrics* getRedQueenMetrics(SpeciesId speciesId) const;

    /**
     * @brief Detect adaptation-counter-adaptation cycles
     *
     * Analyzes trait changes in both species to detect when one species
     * adapts and the other responds.
     *
     * @param pair The coevolutionary pair to analyze
     * @return Number of cycles detected in recent history
     */
    int detectAdaptationCycles(const CoevolutionaryPair& pair) const;

    /**
     * @brief Calculate the mean lag between adaptation and response
     *
     * @param pair The coevolutionary pair
     * @return Average number of generations between adaptation events
     */
    float calculateResponseLag(const CoevolutionaryPair& pair) const;

    // =========================================================================
    // QUERY METHODS
    // =========================================================================

    /**
     * @brief Get all tracked coevolutionary pairs
     * @return Vector of pointers to all coevolutionary pairs
     */
    std::vector<const CoevolutionaryPair*> getCoevolutionaryPairs() const;

    /**
     * @brief Get coevolutionary pairs involving a specific species
     * @param speciesId The species to query
     * @return Vector of pairs involving this species
     */
    std::vector<const CoevolutionaryPair*> getPairsForSpecies(SpeciesId speciesId) const;

    /**
     * @brief Get coevolutionary pairs of a specific type
     * @param type The type of coevolution to filter by
     * @return Vector of pairs matching the specified type
     */
    std::vector<const CoevolutionaryPair*> getPairsByType(CoevolutionType type) const;

    /**
     * @brief Get a specific coevolutionary pair
     * @param species1Id First species ID
     * @param species2Id Second species ID
     * @return Pointer to the pair or nullptr if not found
     */
    const CoevolutionaryPair* getPair(SpeciesId species1Id, SpeciesId species2Id) const;

    /**
     * @brief Get statistics about the coevolution system
     * @return CoevolutionStats structure with summary statistics
     */
    CoevolutionStats getStats() const;

    // =========================================================================
    // DATA EXPORT
    // =========================================================================

    /**
     * @brief Export all coevolution data to a file
     *
     * Exports coevolutionary pairs, arms races, mimicry complexes, and
     * Red Queen metrics to a CSV file for external analysis.
     *
     * @param filename Path to the output file
     * @return True if export was successful
     */
    bool exportCoevolutionData(const std::string& filename) const;

    /**
     * @brief Export coevolution network in graph format
     *
     * Exports the network of coevolutionary relationships in a format
     * suitable for graph visualization tools.
     *
     * @param filename Path to the output file
     * @param format Output format ("graphml", "dot", or "adjacency")
     * @return True if export was successful
     */
    bool exportCoevolutionNetwork(const std::string& filename,
                                   const std::string& format = "graphml") const;

    /**
     * @brief Export arms race history to CSV
     *
     * @param armsRace The arms race to export
     * @param filename Path to the output file
     * @return True if export was successful
     */
    bool exportArmsRaceHistory(const ArmsRace& armsRace, const std::string& filename) const;

    // =========================================================================
    // CONFIGURATION
    // =========================================================================

    /**
     * @brief Set the configuration for the tracker
     * @param config New configuration parameters
     */
    void setConfig(const CoevolutionConfig& config);

    /**
     * @brief Get the current configuration
     * @return Current configuration parameters
     */
    const CoevolutionConfig& getConfig() const { return config; }

    // =========================================================================
    // CLEANUP
    // =========================================================================

    /**
     * @brief Remove pairs involving an extinct species
     * @param extinctSpeciesId ID of the extinct species
     */
    void handleExtinction(SpeciesId extinctSpeciesId);

    /**
     * @brief Clear all tracked data
     */
    void clear();

private:
    // =========================================================================
    // PRIVATE DATA MEMBERS
    // =========================================================================

    /// Configuration parameters
    CoevolutionConfig config;

    /// All tracked coevolutionary pairs, keyed by ordered species ID pair
    std::map<std::pair<SpeciesId, SpeciesId>, CoevolutionaryPair> coevolutionaryPairs;

    /// Active arms races, keyed by predator-prey ID pair
    std::map<std::pair<SpeciesId, SpeciesId>, ArmsRace> armsRaces;

    /// Detected mimicry complexes, keyed by model species ID
    std::map<SpeciesId, MimicryComplex> mimicryComplexes;

    /// Red Queen metrics per species
    std::map<SpeciesId, RedQueenMetrics> redQueenMetrics;

    /// Historical trait values for correlation analysis
    /// Key: (species ID, gene type), Value: deque of trait values by generation
    std::map<std::pair<SpeciesId, GeneType>, std::deque<float>> traitHistories;

    /// Last generation when full update was performed
    int lastUpdateGeneration;

    // =========================================================================
    // PRIVATE HELPER METHODS
    // =========================================================================

    /**
     * @brief Create an ordered pair of species IDs for map keys
     */
    std::pair<SpeciesId, SpeciesId> makeOrderedPair(SpeciesId s1, SpeciesId s2) const;

    /**
     * @brief Extract predator traits from a population
     */
    PredatorTraits extractPredatorTraits(const std::vector<Creature*>& creatures) const;

    /**
     * @brief Extract prey traits from a population
     */
    PreyTraits extractPreyTraits(const std::vector<Creature*>& creatures) const;

    /**
     * @brief Calculate Pearson correlation coefficient
     */
    float calculatePearsonCorrelation(const std::deque<float>& x,
                                       const std::deque<float>& y) const;

    /**
     * @brief Record trait value in history
     */
    void recordTraitValue(SpeciesId speciesId, GeneType trait, float value);

    /**
     * @brief Prune old history entries
     */
    void pruneHistory();

    /**
     * @brief Detect new coevolutionary pairs
     */
    void detectNewPairs(const std::vector<Species*>& allSpecies,
                        const std::map<std::pair<SpeciesId, SpeciesId>, float>& interactionMatrix,
                        int currentGeneration);

    /**
     * @brief Update existing pairs
     */
    void updateExistingPairs(int currentGeneration,
                             const std::vector<Creature*>& creatures);

    /**
     * @brief Detect mimicry relationships
     */
    void detectMimicryRelationships(const std::vector<Species*>& allSpecies);

    /**
     * @brief Update Red Queen metrics for all species
     */
    void updateRedQueenMetrics(const std::vector<Species*>& allSpecies,
                               const std::vector<Creature*>& creatures);
};

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Convert CoevolutionType to string
 * @param type The coevolution type
 * @return String representation
 */
const char* coevolutionTypeToString(CoevolutionType type);

/**
 * @brief Convert MimicryType to string
 * @param type The mimicry type
 * @return String representation
 */
const char* mimicryTypeToString(MimicryType type);

/**
 * @brief Convert AdvantageSide to string
 * @param side The advantage side
 * @return String representation
 */
const char* advantageSideToString(AdvantageSide side);

} // namespace genetics
