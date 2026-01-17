#pragma once

#include "DiploidGenome.h"
#include "Species.h"  // For IsolationType enum
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

// Forward declaration
class Creature;

namespace genetics {

// ============================================
// Enhanced Mate Evaluation Structs
// ============================================

// Ornament display characteristics
struct OrnamentDisplay {
    float intensity;          // 0-1 how vibrant/prominent
    float complexity;         // 0-1 number of ornament components
    float symmetry;           // 0-1 bilateral symmetry of ornaments
    float conditionDependence; // 0-1 how much ornament reflects health

    OrnamentDisplay()
        : intensity(0.5f), complexity(0.3f),
          symmetry(0.8f), conditionDependence(0.6f) {}

    float calculateQuality() const {
        return intensity * 0.3f + complexity * 0.2f +
               symmetry * 0.25f + conditionDependence * 0.25f;
    }
};

// Courtship display behavior
struct DisplayBehavior {
    float duration;           // Seconds of display
    float frequency;          // Displays per time unit
    float vigor;              // Energy/intensity of display
    float creativity;         // Novelty/unpredictability

    DisplayBehavior()
        : duration(5.0f), frequency(0.5f), vigor(0.6f), creativity(0.3f) {}

    float calculateAttractiveness() const {
        return duration * 0.1f + frequency * 0.25f +
               vigor * 0.35f + creativity * 0.3f;
    }
};

// Honest signaling and handicap traits
struct MateQualitySignal {
    float signalStrength;       // 0-1 signal intensity
    float signalCost;           // Metabolic cost of maintaining signal
    float honestyLevel;         // 0-1 correlation with actual quality
    float handicapMagnitude;    // Size of Zahavian handicap
    float conditionDependence;  // 0-1 how much signal depends on condition
    bool isHonestSignal;        // True if signal reliably indicates quality

    MateQualitySignal()
        : signalStrength(0.5f), signalCost(0.2f), honestyLevel(0.7f),
          handicapMagnitude(0.3f), conditionDependence(0.6f), isHonestSignal(true) {}
};

// Runaway selection tracking data
struct RunawaySelectionData {
    float ornamentTraitMean;      // Population mean ornament value
    float preferenceTraitMean;    // Population mean preference value
    float covariance;             // Ornament-preference covariance
    float selectionGradient;      // Rate of change
    float runawayStrength;        // Fisher runaway intensity
    int generationsTracked;       // Number of generations monitored

    RunawaySelectionData()
        : ornamentTraitMean(0.5f), preferenceTraitMean(0.5f),
          covariance(0.0f), selectionGradient(0.0f),
          runawayStrength(0.0f), generationsTracked(0) {}
};

// Sexual conflict data
struct SexualConflictData {
    float maleFitnessOptimum;     // Trait value optimal for males
    float femaleFitnessOptimum;   // Trait value optimal for females
    float conflictIntensity;      // Magnitude of fitness trade-off
    float chaseAwayStrength;      // Antagonistic coevolution rate
    bool isAntagonistic;          // True if in chase-away mode

    SexualConflictData()
        : maleFitnessOptimum(0.7f), femaleFitnessOptimum(0.3f),
          conflictIntensity(0.0f), chaseAwayStrength(0.0f),
          isAntagonistic(false) {}
};

// Male competition result
struct CombatResult {
    bool winner1;                 // True if male1 wins
    float winnerDamage;           // Damage taken by winner
    float loserDamage;            // Damage taken by loser
    float dominanceChange;        // Rank change from combat

    CombatResult()
        : winner1(true), winnerDamage(0.0f),
          loserDamage(0.0f), dominanceChange(0.0f) {}
};

// Territory quality for male competition
struct TerritoryQuality {
    float resourceDensity;        // Food/resources in territory
    float safetyLevel;            // Protection from predators
    float displayVisibility;      // How visible for displays
    float size;                   // Territory area
    float overallQuality;         // Combined score

    TerritoryQuality()
        : resourceDensity(0.5f), safetyLevel(0.5f),
          displayVisibility(0.5f), size(0.5f), overallQuality(0.5f) {}
};

// Result of mate evaluation
struct MateEvaluation {
    Creature* candidate;
    float attractiveness;
    float geneticCompatibility;
    float totalScore;

    // Enhanced evaluation components
    float ornamentScore;
    float displayScore;
    float handicapScore;
    float preferenceMatch;
    float dominanceScore;
    float territoryScore;

    MateEvaluation(Creature* c = nullptr)
        : candidate(c), attractiveness(0.0f),
          geneticCompatibility(0.0f), totalScore(0.0f),
          ornamentScore(0.0f), displayScore(0.0f),
          handicapScore(0.0f), preferenceMatch(0.0f),
          dominanceScore(0.0f), territoryScore(0.0f) {}
};

// Reproductive compatibility between species
struct ReproductiveCompatibility {
    float preMatingBarrier;    // 0 = no barrier, 1 = complete isolation
    float postMatingBarrier;   // Hybrid fitness reduction
    float hybridSterility;     // Probability hybrid is sterile
    float hybridVigor;         // Potential heterosis bonus

    ReproductiveCompatibility()
        : preMatingBarrier(0.0f), postMatingBarrier(0.0f),
          hybridSterility(0.0f), hybridVigor(0.0f) {}
};

// IsolationType is defined in Species.h

// Female choice modes
enum class ChoiceMode {
    THRESHOLD,     // Accept first mate above threshold
    BEST_OF_N,     // Sample N males, choose best
    SEQUENTIAL     // Evaluate sequentially, accept if improving
};

// ============================================
// Mate selection system
// ============================================
class MateSelector {
public:
    MateSelector();

    // ========================================
    // Core mate selection (existing)
    // ========================================

    // Find potential mates within search radius
    std::vector<Creature*> findPotentialMates(
        const Creature& seeker,
        const std::vector<Creature*>& candidates,
        float searchRadius) const;

    // Evaluate a single mate
    MateEvaluation evaluateMate(const Creature& chooser, const Creature& candidate) const;

    // Select the best mate (or nullptr if none acceptable)
    Creature* selectMate(
        const Creature& chooser,
        const std::vector<Creature*>& potentialMates) const;

    // Check if two creatures can potentially interbreed
    bool canInterbreed(const Creature& c1, const Creature& c2) const;

    // Calculate reproductive compatibility
    ReproductiveCompatibility calculateCompatibility(
        const DiploidGenome& g1, const DiploidGenome& g2) const;

    // Identify isolation type between two individuals
    IsolationType identifyIsolation(const Creature& c1, const Creature& c2) const;

    // Configuration
    void setSearchRadius(float radius) { defaultSearchRadius = radius; }
    void setMinimumAcceptance(float min) { minimumAcceptance = min; }
    void setSpeciesThreshold(float threshold) { speciesDistanceThreshold = threshold; }

    // Get isolation type as string
    static const char* isolationTypeToString(IsolationType type);

    // ========================================
    // Runaway Selection Support
    // ========================================

    // Track ornament evolution for a species
    void trackOrnamentEvolution(SpeciesId speciesId, const std::vector<Creature*>& population);

    // Calculate the strength of Fisher runaway selection
    float calculateRunawayStrength(SpeciesId speciesId) const;

    // Get runaway selection data for a species
    const RunawaySelectionData* getRunawayData(SpeciesId speciesId) const;

    // Calculate preference-ornament coevolution strength
    float calculatePreferenceOrnamentCovariance(const std::vector<Creature*>& population) const;

    // ========================================
    // Handicap Principle Traits
    // ========================================

    // Calculate metabolic cost of maintaining ornament
    float calculateHandicapCost(const OrnamentDisplay& ornament) const;

    // Evaluate if signal is honest indicator of quality
    float evaluateHonesty(const MateQualitySignal& signal, const Creature& signaler) const;

    // Calculate Zahavian handicap score for mate evaluation
    float calculateZahavianScore(const Creature& male) const;

    // Extract ornament display from creature
    OrnamentDisplay extractOrnamentDisplay(const Creature& creature) const;

    // Extract quality signal from creature
    MateQualitySignal extractQualitySignal(const Creature& creature) const;

    // ========================================
    // Display Behavior Evaluation
    // ========================================

    // Evaluate overall courtship display quality
    float evaluateDisplayQuality(const Creature& creature) const;

    // Evaluate novelty/creativity of display
    float evaluateDisplayCreativity(const DisplayBehavior& display) const;

    // Evaluate bilateral symmetry of creature/display
    float evaluateDisplaySymmetry(const Creature& creature) const;

    // Extract display behavior from creature
    DisplayBehavior extractDisplayBehavior(const Creature& creature) const;

    // ========================================
    // Female Choice Mechanics
    // ========================================

    // Set the female choice mode
    void setChoiceMode(ChoiceMode mode) { choiceMode = mode; }
    ChoiceMode getChoiceMode() const { return choiceMode; }

    // Calculate how selective a female is
    float calculateChoiceStrength(const Creature& female) const;

    // Evaluate how well male traits match female preferences
    float evaluatePreferenceMatch(const Creature& male, const Creature& female) const;

    // Select mate using specified choice mode
    Creature* selectMateWithMode(
        const Creature& chooser,
        const std::vector<Creature*>& potentialMates,
        ChoiceMode mode) const;

    // Best-of-N mate selection
    Creature* selectBestOfN(
        const Creature& chooser,
        const std::vector<Creature*>& potentialMates,
        int sampleSize) const;

    // Set sample size for best-of-N selection
    void setBestOfNSampleSize(int n) { bestOfNSampleSize = n; }
    int getBestOfNSampleSize() const { return bestOfNSampleSize; }

    // ========================================
    // Male Competition
    // ========================================

    // Evaluate combat success between two males
    CombatResult evaluateCombatSuccess(const Creature& male1, const Creature& male2) const;

    // Evaluate quality of male's territory
    TerritoryQuality evaluateTerritoryQuality(const Creature& male) const;

    // Evaluate male's dominance rank in population
    float evaluateDominanceRank(const Creature& male, const std::vector<Creature*>& population) const;

    // Calculate fighting ability
    float calculateFightingAbility(const Creature& male) const;

    // Update dominance hierarchy
    void updateDominanceHierarchy(const std::vector<Creature*>& population);

    // Get male's current dominance rank (0-1, 1 = highest)
    float getDominanceRank(int creatureId) const;

    // ========================================
    // Assortative Mating
    // ========================================

    // Calculate population-level assortative mating index
    float calculateAssortativeIndex(const std::vector<Creature*>& population) const;

    // Filter candidates by similarity to chooser (positive assortative)
    std::vector<Creature*> enforceAssortativeMating(
        const Creature& chooser,
        const std::vector<Creature*>& candidates,
        float strength) const;

    // Filter candidates by dissimilarity (negative/disassortative)
    std::vector<Creature*> enforceDisassortativeMating(
        const Creature& chooser,
        const std::vector<Creature*>& candidates,
        float strength) const;

    // Check if population is trending toward sympatric speciation
    bool detectSympatricSpeciation(const std::vector<Creature*>& population) const;

    // Set assortative mating strength (0=random, 1=perfect assortment)
    void setAssortativeStrength(float strength) { assortativeStrength = strength; }
    float getAssortativeStrength() const { return assortativeStrength; }

    // ========================================
    // Sexual Conflict
    // ========================================

    // Detect sexual conflict in species
    SexualConflictData detectSexualConflict(SpeciesId speciesId,
        const std::vector<Creature*>& population) const;

    // Track chase-away selection dynamics
    void trackChaseAwaySelection(SpeciesId speciesId, const std::vector<Creature*>& population);

    // Get chase-away selection strength
    float getChaseAwayStrength(SpeciesId speciesId) const;

    // Calculate fitness trade-off between sexes
    float calculateSexualConflictCost(const Creature& creature,
        const SexualConflictData& conflict) const;

    // ========================================
    // Utility Methods
    // ========================================

    // Get choice mode as string
    static const char* choiceModeToString(ChoiceMode mode);

    // Clear all tracking data (for new simulation)
    void reset();

    // Update per-generation tracking
    void updateGeneration(const std::vector<Creature*>& population);

private:
    float defaultSearchRadius;
    float minimumAcceptance;
    float speciesDistanceThreshold;

    // Female choice settings
    ChoiceMode choiceMode;
    int bestOfNSampleSize;
    float assortativeStrength;

    // Runaway selection tracking per species
    std::unordered_map<SpeciesId, RunawaySelectionData> runawayData;

    // Sexual conflict tracking per species
    std::unordered_map<SpeciesId, SexualConflictData> conflictData;

    // Dominance hierarchy (creature ID -> rank)
    std::unordered_map<int, float> dominanceRanks;

    // Historical trait means for tracking (species -> vector of means)
    std::unordered_map<SpeciesId, std::vector<float>> ornamentHistory;
    std::unordered_map<SpeciesId, std::vector<float>> preferenceHistory;

    // ========================================
    // Private helper methods (existing)
    // ========================================

    // Preference-based evaluation
    float evaluateByPreferences(const MatePreferences& prefs,
                                const DiploidGenome& chooser,
                                const DiploidGenome& candidate) const;

    // Genetic compatibility (MHC-like dissimilarity preference)
    float evaluateGeneticCompatibility(const DiploidGenome& g1,
                                       const DiploidGenome& g2) const;

    // Physical compatibility check
    float evaluatePhysicalCompatibility(const DiploidGenome& g1,
                                        const DiploidGenome& g2) const;

    // Temporal compatibility (activity time overlap)
    float evaluateTemporalCompatibility(const DiploidGenome& g1,
                                        const DiploidGenome& g2) const;

    // Ecological compatibility (niche overlap)
    float evaluateEcologicalCompatibility(const DiploidGenome& g1,
                                          const DiploidGenome& g2) const;

    // ========================================
    // Private helper methods (new)
    // ========================================

    // Calculate genetic similarity for assortative mating
    float calculateGeneticSimilarity(const Creature& c1, const Creature& c2) const;

    // Calculate phenotypic similarity
    float calculatePhenotypicSimilarity(const Creature& c1, const Creature& c2) const;

    // Estimate creature condition (health/energy proxy)
    float estimateCondition(const Creature& creature) const;

    // Calculate signal reliability
    float calculateSignalReliability(const Creature& signaler) const;
};

} // namespace genetics
