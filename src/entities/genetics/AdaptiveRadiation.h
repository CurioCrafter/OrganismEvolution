#pragma once

/**
 * @file AdaptiveRadiation.h
 * @brief Adaptive radiation tracking system for evolutionary simulations
 *
 * This module provides comprehensive tracking and analysis of adaptive radiation
 * events - rapid evolutionary diversification from a single ancestral species into
 * multiple descendant species occupying different ecological niches.
 *
 * Key concepts implemented:
 * - Detection of radiation trigger events (colonization, extinction, innovation)
 * - Tracking of lineage diversification rates and patterns
 * - Measurement of morphological disparity across species
 * - Analysis of niche exploitation and saturation dynamics
 * - Historical record keeping for completed radiation events
 *
 * @author Evolution Simulator Team
 * @date 2024
 */

#include "DiploidGenome.h"
#include "Species.h"
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <functional>
#include <cstdint>

// Forward declarations
class Creature;
class Terrain;

namespace genetics {

// Forward declarations within namespace
class Species;
class SpeciationTracker;

// =============================================================================
// RADIATION TRIGGER TYPES
// =============================================================================

/**
 * @enum RadiationTrigger
 * @brief Enumeration of ecological and evolutionary triggers for adaptive radiation
 *
 * These represent the major mechanisms that can initiate rapid diversification:
 * - COLONIZATION: Entry into a new, unoccupied habitat (e.g., island colonization)
 * - MASS_EXTINCTION: Vacancy of niches after catastrophic species loss
 * - KEY_INNOVATION: Evolution of a novel trait enabling new ecological opportunities
 * - NICHE_EXPANSION: Discovery or creation of new resource types
 * - GEOGRAPHIC_ISOLATION: Allopatric fragmentation leading to parallel diversification
 */
enum class RadiationTrigger {
    COLONIZATION,           ///< New habitat colonization (e.g., island, new biome)
    MASS_EXTINCTION,        ///< Vacant niches after extinction event
    KEY_INNOVATION,         ///< Novel adaptation enabling new ecological strategies
    NICHE_EXPANSION,        ///< New resources or microhabitats available
    GEOGRAPHIC_ISOLATION,   ///< Allopatric speciation from population fragmentation
    UNKNOWN                 ///< Trigger not yet determined or multiple factors
};

/**
 * @brief Convert RadiationTrigger enum to human-readable string
 * @param trigger The trigger type to convert
 * @return String representation of the trigger
 */
inline const char* radiationTriggerToString(RadiationTrigger trigger) {
    switch (trigger) {
        case RadiationTrigger::COLONIZATION:        return "Colonization";
        case RadiationTrigger::MASS_EXTINCTION:     return "Mass Extinction";
        case RadiationTrigger::KEY_INNOVATION:      return "Key Innovation";
        case RadiationTrigger::NICHE_EXPANSION:     return "Niche Expansion";
        case RadiationTrigger::GEOGRAPHIC_ISOLATION: return "Geographic Isolation";
        case RadiationTrigger::UNKNOWN:             return "Unknown";
        default:                                     return "Invalid";
    }
}

// =============================================================================
// NICHE TYPES FOR RADIATION ANALYSIS
// =============================================================================

/**
 * @enum NicheType
 * @brief Categories of ecological niches that can be exploited during radiation
 *
 * These represent distinct ecological roles that species can evolve to fill
 * during adaptive radiation events.
 */
enum class NicheType {
    // Trophic niches
    PRIMARY_PRODUCER,       ///< Photosynthetic or chemosynthetic organisms
    HERBIVORE_GRAZER,       ///< Feeding on vegetation (grass, leaves)
    HERBIVORE_BROWSER,      ///< Feeding on trees, shrubs
    HERBIVORE_FRUGIVORE,    ///< Fruit-eating specialists
    OMNIVORE,               ///< Mixed diet
    PREDATOR_SMALL,         ///< Hunting small prey
    PREDATOR_LARGE,         ///< Apex predator, large prey
    SCAVENGER,              ///< Feeding on carrion
    FILTER_FEEDER,          ///< Aquatic filter feeding
    DETRITIVORE,            ///< Decomposing organic matter

    // Habitat niches
    ARBOREAL,               ///< Tree-dwelling
    FOSSORIAL,              ///< Burrowing underground
    AQUATIC_SURFACE,        ///< Surface water dwelling
    AQUATIC_BENTHIC,        ///< Bottom-dwelling aquatic
    AQUATIC_PELAGIC,        ///< Open water swimming
    AERIAL,                 ///< Flying/gliding
    TERRESTRIAL_OPEN,       ///< Open ground dwelling
    TERRESTRIAL_FOREST,     ///< Forest floor dwelling

    // Temporal niches
    DIURNAL,                ///< Day-active
    NOCTURNAL,              ///< Night-active
    CREPUSCULAR,            ///< Dawn/dusk active

    // Specialized niches
    PARASITIC,              ///< Living off host organisms
    SYMBIOTIC,              ///< Mutualistic relationships
    CAVE_DWELLING,          ///< Cave-adapted (troglophilic)

    COUNT                   ///< Number of niche types
};

/**
 * @brief Convert NicheType enum to human-readable string
 * @param niche The niche type to convert
 * @return String representation of the niche
 */
inline const char* nicheTypeToString(NicheType niche) {
    switch (niche) {
        case NicheType::PRIMARY_PRODUCER:    return "Primary Producer";
        case NicheType::HERBIVORE_GRAZER:    return "Grazer";
        case NicheType::HERBIVORE_BROWSER:   return "Browser";
        case NicheType::HERBIVORE_FRUGIVORE: return "Frugivore";
        case NicheType::OMNIVORE:            return "Omnivore";
        case NicheType::PREDATOR_SMALL:      return "Small Predator";
        case NicheType::PREDATOR_LARGE:      return "Apex Predator";
        case NicheType::SCAVENGER:           return "Scavenger";
        case NicheType::FILTER_FEEDER:       return "Filter Feeder";
        case NicheType::DETRITIVORE:         return "Detritivore";
        case NicheType::ARBOREAL:            return "Arboreal";
        case NicheType::FOSSORIAL:           return "Fossorial";
        case NicheType::AQUATIC_SURFACE:     return "Surface Aquatic";
        case NicheType::AQUATIC_BENTHIC:     return "Benthic";
        case NicheType::AQUATIC_PELAGIC:     return "Pelagic";
        case NicheType::AERIAL:              return "Aerial";
        case NicheType::TERRESTRIAL_OPEN:    return "Open Terrestrial";
        case NicheType::TERRESTRIAL_FOREST:  return "Forest Terrestrial";
        case NicheType::DIURNAL:             return "Diurnal";
        case NicheType::NOCTURNAL:           return "Nocturnal";
        case NicheType::CREPUSCULAR:         return "Crepuscular";
        case NicheType::PARASITIC:           return "Parasitic";
        case NicheType::SYMBIOTIC:           return "Symbiotic";
        case NicheType::CAVE_DWELLING:       return "Cave Dwelling";
        default:                              return "Unknown";
    }
}

// =============================================================================
// ENVIRONMENT CONTEXT TYPES
// =============================================================================

/**
 * @enum EnvironmentContext
 * @brief Environmental settings where adaptive radiation events occur
 */
enum class EnvironmentContext {
    ISLAND_ARCHIPELAGO,     ///< Island chain colonization (e.g., Darwin's finches)
    ISOLATED_LAKE,          ///< Isolated water body (e.g., cichlid radiations)
    MOUNTAIN_RANGE,         ///< Montane isolation
    NEW_BIOME,              ///< Expansion into novel biome type
    POST_EXTINCTION,        ///< Recovering ecosystem after mass extinction
    HABITAT_FRAGMENTATION,  ///< Anthropogenic or natural fragmentation
    CONTINENTAL,            ///< Large-scale continental radiation
    OCEANIC,                ///< Open ocean radiation
    CAVE_SYSTEM,            ///< Subterranean radiation
    UNKNOWN                 ///< Context not determined
};

/**
 * @brief Convert EnvironmentContext enum to human-readable string
 * @param context The environment context to convert
 * @return String representation of the context
 */
inline const char* environmentContextToString(EnvironmentContext context) {
    switch (context) {
        case EnvironmentContext::ISLAND_ARCHIPELAGO:     return "Island Archipelago";
        case EnvironmentContext::ISOLATED_LAKE:          return "Isolated Lake";
        case EnvironmentContext::MOUNTAIN_RANGE:         return "Mountain Range";
        case EnvironmentContext::NEW_BIOME:              return "New Biome";
        case EnvironmentContext::POST_EXTINCTION:        return "Post-Extinction";
        case EnvironmentContext::HABITAT_FRAGMENTATION:  return "Habitat Fragmentation";
        case EnvironmentContext::CONTINENTAL:            return "Continental";
        case EnvironmentContext::OCEANIC:                return "Oceanic";
        case EnvironmentContext::CAVE_SYSTEM:            return "Cave System";
        case EnvironmentContext::UNKNOWN:                return "Unknown";
        default:                                          return "Invalid";
    }
}

// =============================================================================
// RADIATION EVENT STRUCTURE
// =============================================================================

/**
 * @struct RadiationEvent
 * @brief Complete record of an adaptive radiation event
 *
 * Tracks all relevant information about a single adaptive radiation event,
 * from its initiation through completion or ongoing progress. This includes
 * the ancestral species, all descendant lineages, ecological context, and
 * quantitative metrics of diversification.
 */
struct RadiationEvent {
    // -------------------------------------------------------------------------
    // Core identification
    // -------------------------------------------------------------------------

    uint64_t radiationId;               ///< Unique identifier for this radiation event
    int startGeneration;                ///< Generation when radiation was first detected
    SpeciesId ancestorSpeciesId;        ///< The founding/ancestral species ID
    std::string ancestorSpeciesName;    ///< Name of the ancestral species at start

    // -------------------------------------------------------------------------
    // Descendant tracking
    // -------------------------------------------------------------------------

    std::vector<SpeciesId> descendantSpeciesIds;    ///< All species descended from ancestor
    std::vector<SpeciesId> extantDescendants;       ///< Currently living descendant species
    std::vector<SpeciesId> extinctDescendants;      ///< Descendant species that went extinct

    // -------------------------------------------------------------------------
    // Trigger and context
    // -------------------------------------------------------------------------

    RadiationTrigger triggerType;               ///< What initiated this radiation
    EnvironmentContext environmentContext;       ///< Environmental setting of radiation
    std::string triggerDescription;              ///< Detailed description of trigger event

    // -------------------------------------------------------------------------
    // Diversification metrics
    // -------------------------------------------------------------------------

    float diversificationRate;          ///< Net speciation - extinction rate (per generation)
    float peakDiversificationRate;      ///< Maximum diversification rate achieved
    int peakDiversificationGeneration;  ///< Generation of peak diversification
    float speciationRate;               ///< Raw speciation rate (births per generation)
    float extinctionRate;               ///< Extinction rate (deaths per generation)

    // -------------------------------------------------------------------------
    // Ecological metrics
    // -------------------------------------------------------------------------

    std::vector<NicheType> nichesExploited;     ///< Niches occupied by descendants
    float nichePackingDensity;                   ///< How tightly niches are packed (0-1)
    float ecologicalDisparity;                   ///< Ecological distance between species
    bool nicheSaturationReached;                 ///< Whether niche space is fully occupied
    int saturationGeneration;                    ///< Generation when saturation occurred

    // -------------------------------------------------------------------------
    // Temporal metrics
    // -------------------------------------------------------------------------

    int duration;                       ///< Total generations elapsed (ongoing or final)
    int timeToFirstSpeciation;          ///< Generations until first new species
    bool isOngoing;                     ///< Whether radiation is still active
    int endGeneration;                  ///< Generation when radiation ended (if completed)

    // -------------------------------------------------------------------------
    // Morphological metrics
    // -------------------------------------------------------------------------

    float morphologicalDisparity;       ///< Total phenotypic variance across descendants
    float initialMorphology;            ///< Morphological disparity at start
    float maxMorphologicalDisparity;    ///< Peak disparity achieved
    int maxDisparityGeneration;         ///< Generation of maximum disparity

    // -------------------------------------------------------------------------
    // Constructor
    // -------------------------------------------------------------------------

    /**
     * @brief Default constructor initializing all fields to safe defaults
     */
    RadiationEvent()
        : radiationId(0)
        , startGeneration(0)
        , ancestorSpeciesId(0)
        , triggerType(RadiationTrigger::UNKNOWN)
        , environmentContext(EnvironmentContext::UNKNOWN)
        , diversificationRate(0.0f)
        , peakDiversificationRate(0.0f)
        , peakDiversificationGeneration(0)
        , speciationRate(0.0f)
        , extinctionRate(0.0f)
        , nichePackingDensity(0.0f)
        , ecologicalDisparity(0.0f)
        , nicheSaturationReached(false)
        , saturationGeneration(0)
        , duration(0)
        , timeToFirstSpeciation(0)
        , isOngoing(true)
        , endGeneration(0)
        , morphologicalDisparity(0.0f)
        , initialMorphology(0.0f)
        , maxMorphologicalDisparity(0.0f)
        , maxDisparityGeneration(0) {}

    // -------------------------------------------------------------------------
    // Utility methods
    // -------------------------------------------------------------------------

    /**
     * @brief Get the total number of descendant species (extant + extinct)
     * @return Total descendant count
     */
    int getTotalDescendantCount() const {
        return static_cast<int>(descendantSpeciesIds.size());
    }

    /**
     * @brief Get the number of currently living descendant species
     * @return Extant descendant count
     */
    int getExtantDescendantCount() const {
        return static_cast<int>(extantDescendants.size());
    }

    /**
     * @brief Calculate the survival rate of descendant species
     * @return Proportion of descendants still extant (0-1)
     */
    float getDescendantSurvivalRate() const {
        if (descendantSpeciesIds.empty()) return 0.0f;
        return static_cast<float>(extantDescendants.size()) /
               static_cast<float>(descendantSpeciesIds.size());
    }

    /**
     * @brief Check if this radiation is considered successful
     * @param minDescendants Minimum descendants to count as successful
     * @return True if radiation produced sufficient diversity
     */
    bool isSuccessful(int minDescendants = 3) const {
        return getTotalDescendantCount() >= minDescendants;
    }

    /**
     * @brief Get number of unique niches exploited
     * @return Count of distinct niche types
     */
    int getNicheCount() const {
        return static_cast<int>(nichesExploited.size());
    }
};

// =============================================================================
// LINEAGE DIVERSIFICATION STRUCTURE
// =============================================================================

/**
 * @struct LineageDiversification
 * @brief Tracks diversification statistics for a single evolutionary lineage
 *
 * A lineage represents a monophyletic group descending from a single ancestor.
 * This structure tracks the branching history, extinction patterns, and
 * morphological evolution of the lineage over time.
 */
struct LineageDiversification {
    // -------------------------------------------------------------------------
    // Identification
    // -------------------------------------------------------------------------

    uint64_t lineageId;                 ///< Unique identifier for this lineage
    SpeciesId rootSpeciesId;            ///< Species at the root of this lineage
    int originGeneration;               ///< When lineage was established

    // -------------------------------------------------------------------------
    // Branching statistics
    // -------------------------------------------------------------------------

    int branchingEvents;                ///< Total speciation events in this lineage
    int extinctionEvents;               ///< Total extinction events in this lineage
    int currentSpeciesCount;            ///< Number of extant species in lineage
    int peakSpeciesCount;               ///< Maximum species count reached
    int peakGeneration;                 ///< Generation of peak diversity

    // -------------------------------------------------------------------------
    // Rate calculations
    // -------------------------------------------------------------------------

    float birthRate;                    ///< Speciation rate (lambda)
    float deathRate;                    ///< Extinction rate (mu)
    float netDiversification;           ///< Net rate: birthRate - deathRate (r)
    float turnoverRate;                 ///< Relative extinction: deathRate / birthRate

    // -------------------------------------------------------------------------
    // Morphological evolution
    // -------------------------------------------------------------------------

    float morphologicalDisparity;       ///< Phenotypic variance within lineage
    float disparityChangeRate;          ///< Rate of disparity change per generation
    float averageTraitDistance;         ///< Mean pairwise trait distance between species

    // -------------------------------------------------------------------------
    // Time-series data
    // -------------------------------------------------------------------------

    std::vector<int> speciesCountHistory;       ///< Species count over time
    std::vector<float> diversificationHistory;  ///< Diversification rate over time
    std::vector<float> disparityHistory;        ///< Morphological disparity over time

    // -------------------------------------------------------------------------
    // Constructor
    // -------------------------------------------------------------------------

    /**
     * @brief Default constructor initializing all fields to safe defaults
     */
    LineageDiversification()
        : lineageId(0)
        , rootSpeciesId(0)
        , originGeneration(0)
        , branchingEvents(0)
        , extinctionEvents(0)
        , currentSpeciesCount(1)
        , peakSpeciesCount(1)
        , peakGeneration(0)
        , birthRate(0.0f)
        , deathRate(0.0f)
        , netDiversification(0.0f)
        , turnoverRate(0.0f)
        , morphologicalDisparity(0.0f)
        , disparityChangeRate(0.0f)
        , averageTraitDistance(0.0f) {}

    // -------------------------------------------------------------------------
    // Utility methods
    // -------------------------------------------------------------------------

    /**
     * @brief Calculate the net diversification (birth - death)
     * @return Net diversification rate
     */
    float calculateNetDiversification() const {
        return birthRate - deathRate;
    }

    /**
     * @brief Check if lineage is in decline
     * @return True if extinction rate exceeds speciation rate
     */
    bool isInDecline() const {
        return deathRate > birthRate;
    }

    /**
     * @brief Check if lineage is extinct (no living species)
     * @return True if no extant species remain
     */
    bool isExtinct() const {
        return currentSpeciesCount == 0;
    }

    /**
     * @brief Get the diversification imbalance
     * @return Ratio of current to peak species count
     */
    float getDiversityRetention() const {
        if (peakSpeciesCount == 0) return 0.0f;
        return static_cast<float>(currentSpeciesCount) /
               static_cast<float>(peakSpeciesCount);
    }
};

// =============================================================================
// KEY INNOVATION STRUCTURE
// =============================================================================

/**
 * @struct KeyInnovation
 * @brief Records a key evolutionary innovation that may trigger radiation
 *
 * Key innovations are major evolutionary changes that open up new ecological
 * opportunities, such as the evolution of flight, jaws, or endothermy.
 */
struct KeyInnovation {
    uint64_t innovationId;              ///< Unique identifier
    int detectionGeneration;            ///< When innovation was detected
    SpeciesId originSpeciesId;          ///< Species where innovation first appeared

    GeneType primaryGene;               ///< Main gene involved in innovation
    float traitChange;                  ///< Magnitude of trait change
    float ancestralValue;               ///< Trait value before innovation
    float derivedValue;                 ///< Trait value after innovation

    std::string description;            ///< Description of the innovation
    std::vector<NicheType> nicheUnlocked;   ///< New niches enabled by innovation

    bool triggeredRadiation;            ///< Whether this led to adaptive radiation
    uint64_t associatedRadiationId;     ///< ID of triggered radiation (if any)

    /**
     * @brief Default constructor
     */
    KeyInnovation()
        : innovationId(0)
        , detectionGeneration(0)
        , originSpeciesId(0)
        , primaryGene(GeneType::SIZE)
        , traitChange(0.0f)
        , ancestralValue(0.0f)
        , derivedValue(0.0f)
        , triggeredRadiation(false)
        , associatedRadiationId(0) {}

    /**
     * @brief Get the relative magnitude of the innovation
     * @return Proportional change from ancestral value
     */
    float getRelativeMagnitude() const {
        if (ancestralValue == 0.0f) return traitChange;
        return std::abs(traitChange / ancestralValue);
    }
};

// =============================================================================
// RADIATION STATISTICS STRUCTURE
// =============================================================================

/**
 * @struct RadiationStatistics
 * @brief Aggregate statistics across all radiation events in simulation
 */
struct RadiationStatistics {
    // -------------------------------------------------------------------------
    // Counts
    // -------------------------------------------------------------------------

    int totalRadiationEvents;           ///< Total radiations detected
    int activeRadiations;               ///< Currently ongoing radiations
    int completedRadiations;            ///< Finished radiation events
    int successfulRadiations;           ///< Radiations with >= 3 descendant species
    int failedRadiations;               ///< Radiations that produced < 3 species

    // -------------------------------------------------------------------------
    // Timing metrics
    // -------------------------------------------------------------------------

    float averageTimeToFirstSpeciation; ///< Mean generations to first branching
    float averageRadiationDuration;     ///< Mean length of radiation events
    float averageTimeToSaturation;      ///< Mean time to niche saturation

    // -------------------------------------------------------------------------
    // Rate metrics
    // -------------------------------------------------------------------------

    float averageDiversificationRate;   ///< Mean diversification rate
    float maxDiversificationRate;       ///< Highest rate observed
    float averagePeakRate;              ///< Mean peak diversification rate

    // -------------------------------------------------------------------------
    // Diversity metrics
    // -------------------------------------------------------------------------

    float averageDescendantCount;       ///< Mean species per radiation
    int maxDescendantCount;             ///< Most species from single radiation
    float averageNicheCount;            ///< Mean niches exploited per radiation
    int maxNicheCount;                  ///< Most niches from single radiation

    // -------------------------------------------------------------------------
    // Morphological metrics
    // -------------------------------------------------------------------------

    float averageMorphologicalDisparity;    ///< Mean disparity achieved
    float maxMorphologicalDisparity;        ///< Highest disparity observed

    // -------------------------------------------------------------------------
    // Trigger breakdown
    // -------------------------------------------------------------------------

    std::map<RadiationTrigger, int> triggerCounts;  ///< Count by trigger type
    std::map<EnvironmentContext, int> contextCounts;///< Count by environment

    // -------------------------------------------------------------------------
    // Clade-level metrics
    // -------------------------------------------------------------------------

    float overallCladeExtinctionRisk;   ///< Risk of entire clade extinction
    float backgroundExtinctionRate;     ///< Baseline extinction rate
    float radiationAssociatedExtinction;///< Extinction during radiations

    /**
     * @brief Default constructor
     */
    RadiationStatistics()
        : totalRadiationEvents(0)
        , activeRadiations(0)
        , completedRadiations(0)
        , successfulRadiations(0)
        , failedRadiations(0)
        , averageTimeToFirstSpeciation(0.0f)
        , averageRadiationDuration(0.0f)
        , averageTimeToSaturation(0.0f)
        , averageDiversificationRate(0.0f)
        , maxDiversificationRate(0.0f)
        , averagePeakRate(0.0f)
        , averageDescendantCount(0.0f)
        , maxDescendantCount(0)
        , averageNicheCount(0.0f)
        , maxNicheCount(0)
        , averageMorphologicalDisparity(0.0f)
        , maxMorphologicalDisparity(0.0f)
        , overallCladeExtinctionRisk(0.0f)
        , backgroundExtinctionRate(0.0f)
        , radiationAssociatedExtinction(0.0f) {}

    /**
     * @brief Get the success rate of radiation events
     * @return Proportion of successful radiations (0-1)
     */
    float getSuccessRate() const {
        if (completedRadiations == 0) return 0.0f;
        return static_cast<float>(successfulRadiations) /
               static_cast<float>(completedRadiations);
    }

    /**
     * @brief Get the most common radiation trigger
     * @return Most frequent trigger type
     */
    RadiationTrigger getMostCommonTrigger() const {
        RadiationTrigger result = RadiationTrigger::UNKNOWN;
        int maxCount = 0;
        for (const auto& [trigger, count] : triggerCounts) {
            if (count > maxCount) {
                maxCount = count;
                result = trigger;
            }
        }
        return result;
    }
};

// =============================================================================
// ISLAND COLONIZATION DATA
// =============================================================================

/**
 * @struct IslandColonizationData
 * @brief Tracks island colonization events and founder effects
 */
struct IslandColonizationData {
    uint64_t eventId;                   ///< Unique colonization event ID
    int colonizationGeneration;         ///< When colonization occurred

    SpeciesId sourceSpeciesId;          ///< Mainland/source species
    int founderPopulation;              ///< Number of initial colonizers
    float founderGeneticDiversity;      ///< Heterozygosity of founders

    std::string islandIdentifier;       ///< Name/ID of colonized island
    float islandSize;                   ///< Relative size of island
    float distanceFromSource;           ///< Distance from source population
    float resourceAvailability;         ///< Resource abundance on island
    int availableNiches;                ///< Empty niches on island

    bool triggeredRadiation;            ///< Whether radiation followed
    uint64_t radiationEventId;          ///< Associated radiation (if any)

    std::vector<SpeciesId> endemicSpecies;  ///< Species unique to this island
    float endemismRate;                     ///< Proportion of species endemic

    /**
     * @brief Default constructor
     */
    IslandColonizationData()
        : eventId(0)
        , colonizationGeneration(0)
        , sourceSpeciesId(0)
        , founderPopulation(0)
        , founderGeneticDiversity(0.0f)
        , islandSize(1.0f)
        , distanceFromSource(0.0f)
        , resourceAvailability(1.0f)
        , availableNiches(0)
        , triggeredRadiation(false)
        , radiationEventId(0)
        , endemismRate(0.0f) {}

    /**
     * @brief Check if founder effect is significant
     * @return True if founder population small enough for strong drift
     */
    bool hasSignificantFounderEffect() const {
        return founderPopulation < 20 && founderGeneticDiversity < 0.5f;
    }
};

// =============================================================================
// ADAPTIVE RADIATION TRACKER CLASS
// =============================================================================

/**
 * @class AdaptiveRadiationTracker
 * @brief Main class for detecting, tracking, and analyzing adaptive radiation events
 *
 * This class provides comprehensive functionality for monitoring evolutionary
 * dynamics related to adaptive radiation. It detects the onset of radiation
 * events based on speciation patterns and ecological triggers, tracks their
 * progress through time, and maintains historical records for analysis.
 *
 * Key capabilities:
 * - Automatic detection of radiation initiation events
 * - Real-time tracking of diversification rates and patterns
 * - Key innovation detection and impact assessment
 * - Island biogeography modeling
 * - Morphological disparity measurement
 * - Comprehensive statistical analysis and data export
 *
 * Usage example:
 * @code
 * AdaptiveRadiationTracker tracker;
 * tracker.initialize(config);
 *
 * // Each generation:
 * tracker.detectRadiationStart(speciationEvents, environmentData);
 * tracker.trackRadiationProgress(currentGeneration);
 *
 * // Query results:
 * auto activeRadiations = tracker.getActiveRadiations();
 * auto stats = tracker.getRadiationStatistics();
 * tracker.exportRadiationData("radiation_data.csv");
 * @endcode
 */
class AdaptiveRadiationTracker {
public:
    // =========================================================================
    // CONSTRUCTION AND INITIALIZATION
    // =========================================================================

    /**
     * @brief Default constructor
     */
    AdaptiveRadiationTracker();

    /**
     * @brief Destructor
     */
    ~AdaptiveRadiationTracker();

    /**
     * @brief Initialize the tracker with configuration
     * @param speciationTracker Pointer to the speciation tracking system
     */
    void initialize(SpeciationTracker* speciationTracker);

    /**
     * @brief Reset all tracking data
     */
    void reset();

    // =========================================================================
    // RADIATION DETECTION
    // =========================================================================

    /**
     * @brief Detect the start of a new adaptive radiation event
     *
     * Analyzes recent speciation events and environmental conditions to
     * identify potential adaptive radiation initiations. Triggers include
     * rapid speciation bursts, colonization events, and major extinctions.
     *
     * @param speciationEvents Recent speciation events to analyze
     * @param environmentData Current environmental state
     * @return True if new radiation event was detected
     */
    bool detectRadiationStart(
        const std::vector<SpeciationEvent>& speciationEvents,
        const std::map<std::string, float>& environmentData);

    /**
     * @brief Detect radiation triggered by specific trigger type
     *
     * @param trigger The type of trigger to check for
     * @param relevantSpecies Species involved in potential radiation
     * @param generation Current generation
     * @return Pointer to new RadiationEvent if detected, nullptr otherwise
     */
    RadiationEvent* detectRadiationByTrigger(
        RadiationTrigger trigger,
        const std::vector<SpeciesId>& relevantSpecies,
        int generation);

    // =========================================================================
    // RADIATION PROGRESS TRACKING
    // =========================================================================

    /**
     * @brief Update tracking for all active radiation events
     *
     * Should be called each generation to update diversification rates,
     * descendant lists, morphological disparity, and other metrics for
     * all ongoing radiation events.
     *
     * @param generation Current generation number
     */
    void trackRadiationProgress(int generation);

    /**
     * @brief Update a specific radiation event
     * @param radiationId ID of the radiation to update
     * @param generation Current generation
     */
    void updateRadiationEvent(uint64_t radiationId, int generation);

    /**
     * @brief Mark a radiation event as completed
     * @param radiationId ID of the radiation to complete
     * @param generation Generation when radiation ended
     * @param reason Description of why radiation ended
     */
    void completeRadiation(uint64_t radiationId, int generation,
                           const std::string& reason = "");

    // =========================================================================
    // DIVERSIFICATION RATE CALCULATION
    // =========================================================================

    /**
     * @brief Calculate the current diversification rate for a species
     *
     * Computes the net diversification rate (speciation - extinction) for
     * the lineage containing the specified species.
     *
     * @param speciesId The species to calculate rate for
     * @return Net diversification rate (can be negative)
     */
    float calculateDiversificationRate(SpeciesId speciesId) const;

    /**
     * @brief Calculate diversification rate for a radiation event
     * @param radiationId The radiation event ID
     * @return Diversification rate for that radiation
     */
    float calculateRadiationDiversificationRate(uint64_t radiationId) const;

    /**
     * @brief Calculate birth-death model parameters for a lineage
     * @param lineageId The lineage to analyze
     * @param outBirthRate Output: speciation rate (lambda)
     * @param outDeathRate Output: extinction rate (mu)
     * @return True if calculation successful
     */
    bool calculateBirthDeathRates(uint64_t lineageId,
                                   float& outBirthRate,
                                   float& outDeathRate) const;

    // =========================================================================
    // KEY INNOVATION DETECTION
    // =========================================================================

    /**
     * @brief Detect major trait changes that could trigger radiation
     *
     * Analyzes a genome for significant deviations from ancestral values
     * that might represent key evolutionary innovations (e.g., evolution
     * of flight, specialized feeding apparatus).
     *
     * @param genome The genome to analyze
     * @param ancestralGenome Reference ancestral genome for comparison
     * @return Pointer to KeyInnovation if detected, nullptr otherwise
     */
    KeyInnovation* detectKeyInnovation(
        const DiploidGenome& genome,
        const DiploidGenome& ancestralGenome);

    /**
     * @brief Detect key innovation from phenotype comparison
     * @param currentPhenotype Current species phenotype
     * @param ancestralPhenotype Ancestral phenotype
     * @param speciesId Species where change occurred
     * @param generation Current generation
     * @return Pointer to KeyInnovation if detected, nullptr otherwise
     */
    KeyInnovation* detectKeyInnovationFromPhenotype(
        const Phenotype& currentPhenotype,
        const Phenotype& ancestralPhenotype,
        SpeciesId speciesId,
        int generation);

    /**
     * @brief Get all detected key innovations
     * @return Vector of all key innovations
     */
    const std::vector<KeyInnovation>& getKeyInnovations() const;

    /**
     * @brief Get key innovations that triggered radiations
     * @return Vector of innovations that led to radiation
     */
    std::vector<KeyInnovation> getRadiationTriggeringInnovations() const;

    // =========================================================================
    // ISLAND BIOGEOGRAPHY
    // =========================================================================

    /**
     * @brief Model the effect of island colonization on diversification
     *
     * Evaluates whether a colonization event has the characteristics
     * to trigger adaptive radiation based on island biogeography theory.
     *
     * @param colonizers Species colonizing the island
     * @param islandId Identifier for the island
     * @param islandProperties Properties of the island environment
     * @return Colonization data with radiation prediction
     */
    IslandColonizationData islandColonizationEffect(
        const std::vector<const Species*>& colonizers,
        const std::string& islandId,
        const std::map<std::string, float>& islandProperties);

    /**
     * @brief Record a colonization event
     * @param data The colonization data to record
     */
    void recordColonization(const IslandColonizationData& data);

    /**
     * @brief Get all island colonization events
     * @return Vector of colonization events
     */
    const std::vector<IslandColonizationData>& getColonizationEvents() const;

    /**
     * @brief Check if island has endemic radiation
     * @param islandId The island to check
     * @return True if island has active or completed radiation
     */
    bool hasIslandRadiation(const std::string& islandId) const;

    // =========================================================================
    // MORPHOLOGICAL DISPARITY
    // =========================================================================

    /**
     * @brief Measure morphological disparity across a set of species
     *
     * Calculates the total phenotypic variance (disparity) across all
     * trait dimensions for a group of species. Higher values indicate
     * greater morphological diversity.
     *
     * @param species Vector of species to analyze
     * @return Morphological disparity value (variance)
     */
    float measureMorphologicalDisparity(
        const std::vector<const Species*>& species) const;

    /**
     * @brief Measure disparity for a specific radiation event
     * @param radiationId The radiation event to measure
     * @return Morphological disparity of descendants
     */
    float measureRadiationDisparity(uint64_t radiationId) const;

    /**
     * @brief Calculate pairwise trait distance between two species
     * @param species1 First species
     * @param species2 Second species
     * @return Euclidean distance in trait space
     */
    float calculateTraitDistance(const Species* species1,
                                  const Species* species2) const;

    /**
     * @brief Get disparity through time for a radiation
     * @param radiationId The radiation to analyze
     * @return Vector of (generation, disparity) pairs
     */
    std::vector<std::pair<int, float>> getDisparityThroughTime(
        uint64_t radiationId) const;

    // =========================================================================
    // RADIATION QUERIES
    // =========================================================================

    /**
     * @brief Get all currently active (ongoing) radiation events
     * @return Vector of pointers to active radiations
     */
    std::vector<const RadiationEvent*> getActiveRadiations() const;

    /**
     * @brief Get all active radiations (mutable access)
     * @return Vector of pointers to active radiations
     */
    std::vector<RadiationEvent*> getActiveRadiationsMutable();

    /**
     * @brief Get all completed (historical) radiation events
     * @return Vector of pointers to historical radiations
     */
    std::vector<const RadiationEvent*> getHistoricalRadiations() const;

    /**
     * @brief Get a specific radiation event by ID
     * @param radiationId The radiation event ID
     * @return Pointer to the radiation, or nullptr if not found
     */
    const RadiationEvent* getRadiation(uint64_t radiationId) const;

    /**
     * @brief Get a specific radiation event by ID (mutable)
     * @param radiationId The radiation event ID
     * @return Pointer to the radiation, or nullptr if not found
     */
    RadiationEvent* getRadiationMutable(uint64_t radiationId);

    /**
     * @brief Get all radiation events (active and historical)
     * @return Vector of all radiation events
     */
    std::vector<const RadiationEvent*> getAllRadiations() const;

    /**
     * @brief Find radiations involving a specific species
     * @param speciesId The species to search for
     * @return Vector of radiations where species participated
     */
    std::vector<const RadiationEvent*> getRadiationsForSpecies(
        SpeciesId speciesId) const;

    // =========================================================================
    // LINEAGE TRACKING
    // =========================================================================

    /**
     * @brief Get diversification data for a lineage
     * @param lineageId The lineage ID
     * @return Pointer to lineage data, or nullptr if not found
     */
    const LineageDiversification* getLineageDiversification(
        uint64_t lineageId) const;

    /**
     * @brief Update lineage diversification statistics
     * @param lineageId The lineage to update
     * @param generation Current generation
     */
    void updateLineage(uint64_t lineageId, int generation);

    /**
     * @brief Get all tracked lineages
     * @return Map of lineage ID to diversification data
     */
    const std::map<uint64_t, LineageDiversification>& getAllLineages() const;

    /**
     * @brief Register a new lineage for tracking
     * @param rootSpeciesId Species at the root of the lineage
     * @param generation Generation when lineage started
     * @return The new lineage ID
     */
    uint64_t registerLineage(SpeciesId rootSpeciesId, int generation);

    // =========================================================================
    // NICHE ANALYSIS
    // =========================================================================

    /**
     * @brief Determine the niche type for a species based on traits
     * @param species The species to classify
     * @return Primary niche type
     */
    NicheType classifyNiche(const Species* species) const;

    /**
     * @brief Get all niches occupied within a radiation
     * @param radiationId The radiation event
     * @return Vector of occupied niche types
     */
    std::vector<NicheType> getNichesInRadiation(uint64_t radiationId) const;

    /**
     * @brief Calculate niche packing density
     * @param radiationId The radiation event
     * @return Packing density (0-1, higher = more packed)
     */
    float calculateNichePacking(uint64_t radiationId) const;

    /**
     * @brief Check if niche saturation has been reached
     * @param radiationId The radiation event
     * @param saturationThreshold Threshold for considering saturated
     * @return True if niches are saturated
     */
    bool isNicheSaturated(uint64_t radiationId,
                          float saturationThreshold = 0.8f) const;

    // =========================================================================
    // CLADE EXTINCTION RISK
    // =========================================================================

    /**
     * @brief Calculate extinction risk for an entire clade
     * @param radiationId Radiation/clade to assess
     * @return Risk score (0-1, higher = more at risk)
     */
    float calculateCladeExtinctionRisk(uint64_t radiationId) const;

    /**
     * @brief Get species within radiation most at risk of extinction
     * @param radiationId The radiation event
     * @param topN Number of at-risk species to return
     * @return Vector of (speciesId, riskScore) pairs
     */
    std::vector<std::pair<SpeciesId, float>> getMostEndangeredDescendants(
        uint64_t radiationId, int topN = 5) const;

    // =========================================================================
    // STATISTICS
    // =========================================================================

    /**
     * @brief Get comprehensive radiation statistics
     * @return Aggregate statistics across all radiations
     */
    RadiationStatistics getRadiationStatistics() const;

    /**
     * @brief Get time to first speciation for a radiation
     * @param radiationId The radiation event
     * @return Generations until first speciation
     */
    int getTimeToFirstSpeciation(uint64_t radiationId) const;

    /**
     * @brief Get peak diversification rate for a radiation
     * @param radiationId The radiation event
     * @return Maximum diversification rate achieved
     */
    float getPeakDiversificationRate(uint64_t radiationId) const;

    /**
     * @brief Get the generation when peak occurred
     * @param radiationId The radiation event
     * @return Generation of peak diversification
     */
    int getPeakDiversificationGeneration(uint64_t radiationId) const;

    /**
     * @brief Get saturation point generation
     * @param radiationId The radiation event
     * @return Generation when niche saturation reached, or -1 if not reached
     */
    int getSaturationPoint(uint64_t radiationId) const;

    // =========================================================================
    // DATA EXPORT
    // =========================================================================

    /**
     * @brief Export radiation data to CSV file
     *
     * Exports comprehensive data about all radiation events, including
     * timing, trigger types, diversification rates, and descendant counts.
     *
     * @param filename Path to output file
     * @return True if export successful
     */
    bool exportRadiationData(const std::string& filename) const;

    /**
     * @brief Export lineage diversification data to CSV
     * @param filename Path to output file
     * @return True if export successful
     */
    bool exportLineageData(const std::string& filename) const;

    /**
     * @brief Export key innovations to CSV
     * @param filename Path to output file
     * @return True if export successful
     */
    bool exportInnovationData(const std::string& filename) const;

    /**
     * @brief Export disparity through time data
     * @param radiationId The radiation to export
     * @param filename Path to output file
     * @return True if export successful
     */
    bool exportDisparityThroughTime(uint64_t radiationId,
                                     const std::string& filename) const;

    /**
     * @brief Generate summary report as string
     * @return Formatted summary of radiation events
     */
    std::string generateSummaryReport() const;

    // =========================================================================
    // CONFIGURATION
    // =========================================================================

    /**
     * @brief Set minimum speciation rate to trigger radiation detection
     * @param rate Minimum rate threshold
     */
    void setRadiationDetectionThreshold(float rate);

    /**
     * @brief Set minimum species for successful radiation
     * @param count Minimum descendant species
     */
    void setMinSuccessfulRadiationSize(int count);

    /**
     * @brief Set threshold for key innovation detection
     * @param magnitude Minimum trait change magnitude
     */
    void setInnovationThreshold(float magnitude);

    /**
     * @brief Set window size for rate calculations
     * @param generations Number of generations to average
     */
    void setRateCalculationWindow(int generations);

private:
    // =========================================================================
    // PRIVATE DATA MEMBERS
    // =========================================================================

    /// Pointer to the speciation tracking system
    SpeciationTracker* speciationTracker_;

    /// All radiation events (active and historical)
    std::vector<std::unique_ptr<RadiationEvent>> radiationEvents_;

    /// Map from radiation ID to event for quick lookup
    std::map<uint64_t, RadiationEvent*> radiationById_;

    /// Lineage diversification tracking
    std::map<uint64_t, LineageDiversification> lineages_;

    /// Detected key innovations
    std::vector<KeyInnovation> keyInnovations_;

    /// Island colonization events
    std::vector<IslandColonizationData> colonizationEvents_;

    /// Cached statistics (updated periodically)
    mutable RadiationStatistics cachedStats_;
    mutable bool statsCacheValid_;

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    float radiationDetectionThreshold_;     ///< Min speciation rate for detection
    int minSuccessfulRadiationSize_;        ///< Min species for "successful"
    float innovationThreshold_;             ///< Min trait change for innovation
    int rateCalculationWindow_;             ///< Generations for rate averaging

    // -------------------------------------------------------------------------
    // ID generators
    // -------------------------------------------------------------------------

    uint64_t nextRadiationId_;
    uint64_t nextLineageId_;
    uint64_t nextInnovationId_;
    uint64_t nextColonizationId_;

    // =========================================================================
    // PRIVATE HELPER METHODS
    // =========================================================================

    /**
     * @brief Update descendant lists for a radiation event
     */
    void updateDescendantLists(RadiationEvent* radiation);

    /**
     * @brief Calculate current diversification metrics
     */
    void updateDiversificationMetrics(RadiationEvent* radiation, int generation);

    /**
     * @brief Check if radiation has ended and should be completed
     */
    bool shouldCompleteRadiation(const RadiationEvent* radiation) const;

    /**
     * @brief Determine environment context from data
     */
    EnvironmentContext inferEnvironmentContext(
        const std::map<std::string, float>& environmentData) const;

    /**
     * @brief Invalidate the statistics cache
     */
    void invalidateStatsCache();

    /**
     * @brief Recalculate cached statistics
     */
    void recalculateStats() const;

    /**
     * @brief Extract phenotype traits for disparity calculation
     */
    std::vector<float> extractTraitVector(const Species* species) const;

    /**
     * @brief Calculate variance of trait vectors
     */
    float calculateTraitVariance(
        const std::vector<std::vector<float>>& traitVectors) const;
};

// =============================================================================
// INLINE IMPLEMENTATIONS
// =============================================================================

inline const std::vector<KeyInnovation>&
AdaptiveRadiationTracker::getKeyInnovations() const {
    return keyInnovations_;
}

inline const std::vector<IslandColonizationData>&
AdaptiveRadiationTracker::getColonizationEvents() const {
    return colonizationEvents_;
}

inline const std::map<uint64_t, LineageDiversification>&
AdaptiveRadiationTracker::getAllLineages() const {
    return lineages_;
}

inline void AdaptiveRadiationTracker::setRadiationDetectionThreshold(float rate) {
    radiationDetectionThreshold_ = rate;
    invalidateStatsCache();
}

inline void AdaptiveRadiationTracker::setMinSuccessfulRadiationSize(int count) {
    minSuccessfulRadiationSize_ = count;
    invalidateStatsCache();
}

inline void AdaptiveRadiationTracker::setInnovationThreshold(float magnitude) {
    innovationThreshold_ = magnitude;
}

inline void AdaptiveRadiationTracker::setRateCalculationWindow(int generations) {
    rateCalculationWindow_ = generations;
}

inline void AdaptiveRadiationTracker::invalidateStatsCache() {
    statsCacheValid_ = false;
}

} // namespace genetics
