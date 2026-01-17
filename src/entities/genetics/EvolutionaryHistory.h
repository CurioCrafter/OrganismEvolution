#pragma once

/**
 * @file EvolutionaryHistory.h
 * @brief Comprehensive evolutionary history tracking system for the organism evolution simulation.
 *
 * This header provides structures and classes for tracking complete evolutionary histories,
 * including lineage records, trait changes, speciation events, and phylogenetic relationships.
 * The system supports detailed analysis of evolutionary patterns, genetic diversity trends,
 * and extinction dynamics over the course of a simulation.
 *
 * Key Components:
 * - TraitChange: Records individual trait modifications with selection context
 * - LineageRecord: Tracks complete lineage histories from origin to extinction
 * - PhylogeneticRecord: Maintains species-level evolutionary relationships
 * - EvolutionaryHistoryTracker: Central manager for all evolutionary data
 *
 * @author OrganismEvolution Development Team
 * @version 1.0.0
 */

#include "DiploidGenome.h"
#include "Species.h"
#include <vector>
#include <map>
#include <unordered_map>
#include <deque>
#include <string>
#include <memory>
#include <functional>
#include <optional>
#include <cstdint>

// Forward declarations
class Creature;

namespace genetics {

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

class EvolutionaryHistoryTracker;
struct TraitChange;
struct LineageRecord;
struct PhylogeneticRecord;

// =============================================================================
// TYPE ALIASES
// =============================================================================

/**
 * @brief Unique identifier for individual lineages.
 *
 * A lineage represents a continuous line of descent from a single ancestor.
 * Each creature inherits its lineage ID from its primary parent, with new
 * lineages created during speciation events.
 */
using LineageId = uint64_t;

/**
 * @brief Generation counter type for consistent temporal tracking.
 */
using Generation = int;

// =============================================================================
// ENUMERATIONS
// =============================================================================

/**
 * @brief Classification of evolutionary change mechanisms.
 *
 * Describes the mode by which a trait change occurred, which helps in
 * understanding the evolutionary dynamics at play.
 */
enum class ChangeType {
    GRADUAL,        ///< Slow, continuous change over many generations (typical selection)
    PUNCTUATED,     ///< Rapid change followed by stasis (punctuated equilibrium)
    DRIFT,          ///< Random change due to genetic drift (neutral evolution)
    FOUNDER_EFFECT, ///< Change due to founder population sampling
    BOTTLENECK,     ///< Change due to population bottleneck
    HYBRIDIZATION   ///< Change introduced through hybrid crossing
};

/**
 * @brief Types of selection pressure that can drive evolutionary change.
 *
 * Identifies the primary environmental or biological factor that
 * caused a particular trait to change.
 */
enum class SelectionPressure {
    NONE,                   ///< No specific selection pressure identified
    NATURAL,                ///< General survival selection
    SEXUAL,                 ///< Mate choice and reproductive success
    PREDATION,              ///< Predator-driven selection
    RESOURCE_COMPETITION,   ///< Competition for food or territory
    CLIMATE,                ///< Temperature or weather adaptation
    HABITAT,                ///< Physical environment adaptation
    DISEASE,                ///< Pathogen resistance
    SOCIAL,                 ///< Group dynamics and cooperation
    MIGRATION,              ///< Dispersal and colonization
    MULTIPLE                ///< Multiple simultaneous pressures
};

/**
 * @brief Types of genomic changes tracked in lineage histories.
 *
 * Categorizes the molecular-level changes that accumulate in lineages
 * over evolutionary time.
 */
enum class GenomicChangeType {
    POINT_MUTATION,     ///< Single nucleotide/allele change
    INSERTION,          ///< Addition of genetic material
    DELETION,           ///< Removal of genetic material
    DUPLICATION,        ///< Copy of existing genetic segment
    INVERSION,          ///< Reversal of genetic segment
    TRANSLOCATION,      ///< Movement of segment to different location
    WHOLE_GENOME_DUP,   ///< Complete genome duplication (polyploidy)
    HORIZONTAL_TRANSFER ///< Gene transfer from another lineage (rare)
};

/**
 * @brief Trends in evolutionary trait changes over time.
 *
 * Summarizes the overall direction of trait evolution across
 * multiple generations or species.
 */
enum class EvolutionaryTrend {
    INCREASING,     ///< Trait values generally increasing
    DECREASING,     ///< Trait values generally decreasing
    STABLE,         ///< Trait values remaining relatively constant
    OSCILLATING,    ///< Trait values cycling up and down
    DIVERGING,      ///< Trait values spreading to extreme ranges
    CONVERGING      ///< Trait values consolidating toward a mean
};

// =============================================================================
// TRAIT CHANGE RECORD
// =============================================================================

/**
 * @brief Records a single evolutionary change in a trait.
 *
 * Captures detailed information about when, how, and why a particular
 * trait changed in a species or lineage. This forms the atomic unit
 * of evolutionary history tracking.
 *
 * @note TraitChange records are immutable once created. Modifications
 *       require creating a new record.
 */
struct TraitChange {
    // =========================================================================
    // Temporal Context
    // =========================================================================

    Generation generation;      ///< Generation when change was detected

    // =========================================================================
    // Taxonomic Context
    // =========================================================================

    SpeciesId speciesId;        ///< Species in which change occurred
    LineageId lineageId;        ///< Specific lineage (optional, 0 if species-wide)

    // =========================================================================
    // Trait Information
    // =========================================================================

    GeneType traitType;         ///< Type of trait that changed
    float oldValue;             ///< Previous trait value (population mean)
    float newValue;             ///< New trait value (population mean)
    float standardDeviation;    ///< Variability of trait in population

    // =========================================================================
    // Change Characteristics
    // =========================================================================

    ChangeType changeType;              ///< Mode of evolutionary change
    SelectionPressure selectionPressure; ///< Primary driver of change
    float selectionCoefficient;         ///< Strength of selection (-1 to 1)
    int generationsToFixation;          ///< Generations for change to stabilize

    // =========================================================================
    // Statistical Measures
    // =========================================================================

    float heritability;         ///< Proportion of variance due to genetics (h^2)
    float effectSize;           ///< Magnitude of change relative to variance
    float confidenceLevel;      ///< Statistical confidence in the change

    // =========================================================================
    // Constructors
    // =========================================================================

    /**
     * @brief Default constructor with neutral values.
     */
    TraitChange()
        : generation(0)
        , speciesId(0)
        , lineageId(0)
        , traitType(GeneType::SIZE)
        , oldValue(0.0f)
        , newValue(0.0f)
        , standardDeviation(0.0f)
        , changeType(ChangeType::GRADUAL)
        , selectionPressure(SelectionPressure::NONE)
        , selectionCoefficient(0.0f)
        , generationsToFixation(0)
        , heritability(0.5f)
        , effectSize(0.0f)
        , confidenceLevel(0.0f)
    {}

    /**
     * @brief Constructs a complete trait change record.
     *
     * @param gen Generation of change
     * @param species Species ID
     * @param trait Type of trait
     * @param oldVal Previous value
     * @param newVal New value
     * @param change Mode of change
     * @param pressure Selection pressure
     */
    TraitChange(Generation gen, SpeciesId species, GeneType trait,
                float oldVal, float newVal, ChangeType change,
                SelectionPressure pressure)
        : generation(gen)
        , speciesId(species)
        , lineageId(0)
        , traitType(trait)
        , oldValue(oldVal)
        , newValue(newVal)
        , standardDeviation(0.0f)
        , changeType(change)
        , selectionPressure(pressure)
        , selectionCoefficient(0.0f)
        , generationsToFixation(0)
        , heritability(0.5f)
        , effectSize(std::abs(newVal - oldVal))
        , confidenceLevel(0.95f)
    {}

    // =========================================================================
    // Utility Methods
    // =========================================================================

    /**
     * @brief Calculates the absolute magnitude of the change.
     * @return Absolute difference between old and new values.
     */
    float getMagnitude() const { return std::abs(newValue - oldValue); }

    /**
     * @brief Determines if the change represents an increase.
     * @return True if new value is greater than old value.
     */
    bool isIncrease() const { return newValue > oldValue; }

    /**
     * @brief Calculates relative change as a percentage.
     * @return Percentage change from old to new value.
     */
    float getPercentChange() const {
        if (oldValue == 0.0f) return newValue > 0.0f ? 100.0f : 0.0f;
        return ((newValue - oldValue) / std::abs(oldValue)) * 100.0f;
    }
};

// =============================================================================
// GENOMIC CHANGE RECORD
// =============================================================================

/**
 * @brief Records a single genomic-level change event.
 *
 * Tracks molecular changes to the genome that may or may not result
 * in phenotypic changes. This provides a more detailed view of genetic
 * evolution than trait changes alone.
 */
struct GenomicChange {
    Generation generation;          ///< When the change occurred
    GenomicChangeType type;         ///< Type of genomic modification
    int chromosomeIndex;            ///< Affected chromosome (-1 if multiple)
    int geneIndex;                  ///< Affected gene index (-1 if multiple)
    GeneType affectedTrait;         ///< Trait potentially affected
    float fitnessEffect;            ///< Estimated fitness consequence
    bool isNeutral;                 ///< Whether change is selectively neutral
    std::string description;        ///< Human-readable description

    /**
     * @brief Default constructor with neutral values.
     */
    GenomicChange()
        : generation(0)
        , type(GenomicChangeType::POINT_MUTATION)
        , chromosomeIndex(-1)
        , geneIndex(-1)
        , affectedTrait(GeneType::SIZE)
        , fitnessEffect(0.0f)
        , isNeutral(true)
        , description("")
    {}

    /**
     * @brief Constructs a genomic change record.
     *
     * @param gen Generation of occurrence
     * @param changeType Type of genomic change
     * @param chrIdx Chromosome index
     * @param fitness Fitness effect
     */
    GenomicChange(Generation gen, GenomicChangeType changeType,
                  int chrIdx = -1, float fitness = 0.0f)
        : generation(gen)
        , type(changeType)
        , chromosomeIndex(chrIdx)
        , geneIndex(-1)
        , affectedTrait(GeneType::SIZE)
        , fitnessEffect(fitness)
        , isNeutral(std::abs(fitness) < 0.001f)
        , description("")
    {}
};

// =============================================================================
// LINEAGE RECORD
// =============================================================================

/**
 * @brief Complete historical record for a single evolutionary lineage.
 *
 * A lineage represents a continuous line of genetic descent from an
 * ancestral individual through all descendants. This record tracks
 * the complete history of a lineage from its founding to its extinction
 * (or continuation to present).
 *
 * Lineages are the fundamental unit of evolutionary tracking, as they
 * represent actual genetic continuity rather than the more abstract
 * concept of species.
 */
struct LineageRecord {
    // =========================================================================
    // Identity
    // =========================================================================

    LineageId lineageId;            ///< Unique identifier for this lineage
    LineageId ancestorLineageId;    ///< Parent lineage (0 if original founder)
    std::string lineageName;        ///< Optional descriptive name

    // =========================================================================
    // Temporal Bounds
    // =========================================================================

    Generation foundingGeneration;  ///< Generation when lineage originated
    Generation extinctionGeneration; ///< Generation of extinction (-1 if extant)

    // =========================================================================
    // Species Affiliation History
    // =========================================================================

    /**
     * @brief History of species memberships over time.
     *
     * Each pair contains (generation, speciesId) indicating when the
     * lineage was assigned to a particular species. The lineage may
     * have belonged to multiple species if speciation or reclassification
     * occurred.
     */
    std::vector<std::pair<Generation, SpeciesId>> speciesHistory;

    // =========================================================================
    // Evolutionary Changes
    // =========================================================================

    /**
     * @brief Significant trait changes during lineage history.
     *
     * Only includes major changes that substantially affected the
     * lineage's phenotype or fitness. Minor variations are not recorded.
     */
    std::vector<TraitChange> majorTraitChanges;

    /**
     * @brief Genomic-level changes accumulated by the lineage.
     *
     * Records mutations, duplications, and other molecular changes
     * that occurred in this lineage over evolutionary time.
     */
    std::vector<GenomicChange> genomicChanges;

    // =========================================================================
    // Population Metrics
    // =========================================================================

    int peakPopulation;             ///< Maximum population size achieved
    Generation peakPopulationGen;   ///< Generation of peak population
    int totalDescendants;           ///< Total individuals descended from founder
    int survivingDescendants;       ///< Currently living descendants
    float averageFitness;           ///< Mean fitness across lineage history
    float fitnessVariance;          ///< Variance in fitness over time

    // =========================================================================
    // Founder Information
    // =========================================================================

    std::optional<DiploidGenome> founderGenome; ///< Genome of founding individual
    float founderFitness;           ///< Fitness of founding individual

    // =========================================================================
    // Derived Lineages
    // =========================================================================

    std::vector<LineageId> childLineages; ///< Lineages derived from this one

    // =========================================================================
    // Constructors
    // =========================================================================

    /**
     * @brief Default constructor with neutral values.
     */
    LineageRecord()
        : lineageId(0)
        , ancestorLineageId(0)
        , lineageName("")
        , foundingGeneration(0)
        , extinctionGeneration(-1)
        , peakPopulation(0)
        , peakPopulationGen(0)
        , totalDescendants(0)
        , survivingDescendants(0)
        , averageFitness(0.0f)
        , fitnessVariance(0.0f)
        , founderFitness(0.0f)
    {}

    /**
     * @brief Constructs a lineage record with core identification.
     *
     * @param id Unique lineage identifier
     * @param ancestorId Parent lineage ID (0 if founder)
     * @param foundGen Generation of founding
     */
    LineageRecord(LineageId id, LineageId ancestorId, Generation foundGen)
        : lineageId(id)
        , ancestorLineageId(ancestorId)
        , lineageName("")
        , foundingGeneration(foundGen)
        , extinctionGeneration(-1)
        , peakPopulation(1)
        , peakPopulationGen(foundGen)
        , totalDescendants(1)
        , survivingDescendants(1)
        , averageFitness(0.0f)
        , fitnessVariance(0.0f)
        , founderFitness(0.0f)
    {}

    // =========================================================================
    // Query Methods
    // =========================================================================

    /**
     * @brief Checks if the lineage is currently extant.
     * @return True if lineage has not gone extinct.
     */
    bool isExtant() const { return extinctionGeneration < 0; }

    /**
     * @brief Gets the duration of the lineage in generations.
     * @param currentGeneration Current simulation generation.
     * @return Number of generations the lineage existed/exists.
     */
    int getLifespan(Generation currentGeneration) const {
        if (extinctionGeneration >= 0) {
            return extinctionGeneration - foundingGeneration;
        }
        return currentGeneration - foundingGeneration;
    }

    /**
     * @brief Gets the current species affiliation.
     * @return Current species ID, or 0 if no species assigned.
     */
    SpeciesId getCurrentSpecies() const {
        if (speciesHistory.empty()) return 0;
        return speciesHistory.back().second;
    }

    /**
     * @brief Gets the count of species transitions.
     * @return Number of times the lineage changed species.
     */
    size_t getSpeciesTransitionCount() const {
        return speciesHistory.size() > 0 ? speciesHistory.size() - 1 : 0;
    }

    /**
     * @brief Calculates the mutation rate for this lineage.
     * @param currentGeneration Current generation for extant lineages.
     * @return Average genomic changes per generation.
     */
    float getMutationRate(Generation currentGeneration) const {
        int lifespan = getLifespan(currentGeneration);
        if (lifespan <= 0) return 0.0f;
        return static_cast<float>(genomicChanges.size()) / static_cast<float>(lifespan);
    }
};

// =============================================================================
// PHYLOGENETIC RECORD
// =============================================================================

/**
 * @brief Species-level evolutionary record for phylogenetic analysis.
 *
 * Tracks the evolutionary history of a species from its origin to
 * its extinction (or continuation). This record forms the basis for
 * constructing phylogenetic trees and analyzing macroevolutionary patterns.
 *
 * Unlike LineageRecord which tracks genetic continuity, PhylogeneticRecord
 * tracks taxonomic relationships and species-level evolutionary patterns.
 */
struct PhylogeneticRecord {
    // =========================================================================
    // Species Identity
    // =========================================================================

    SpeciesId speciesId;            ///< Unique identifier for this species
    SpeciesId parentSpeciesId;      ///< Ancestral species (0 if original)
    std::string speciesName;        ///< Taxonomic or common name

    // =========================================================================
    // Temporal Bounds
    // =========================================================================

    Generation foundingGeneration;  ///< Generation of speciation event
    Generation extinctionGeneration; ///< Generation of extinction (-1 if extant)

    // =========================================================================
    // Founding Population
    // =========================================================================

    /**
     * @brief Representative genomes from the founding population.
     *
     * Samples of genomes that characterize the genetic composition
     * of the species at its founding. Useful for comparing ancestral
     * and derived states.
     */
    std::vector<DiploidGenome> founderGenomes;

    int founderPopulationSize;      ///< Number of individuals in founding group
    float founderGeneticDiversity;  ///< Initial genetic diversity (heterozygosity)

    // =========================================================================
    // Population History
    // =========================================================================

    int peakPopulation;             ///< Maximum population ever achieved
    Generation peakPopulationGen;   ///< Generation of peak population
    float averagePopulationSize;    ///< Mean population across history

    /**
     * @brief Population size history at regular intervals.
     *
     * Pairs of (generation, population) for population trajectory analysis.
     */
    std::deque<std::pair<Generation, int>> populationHistory;

    // =========================================================================
    // Ecological Information
    // =========================================================================

    /**
     * @brief Ecological niches occupied by this species.
     *
     * List of niche descriptors (as strings or EcologicalNiche objects)
     * representing the ecological roles filled by this species.
     */
    std::vector<EcologicalNiche> nichesOccupied;

    std::string primaryHabitat;     ///< Main habitat type
    float nicheWidth;               ///< Breadth of ecological tolerance

    // =========================================================================
    // Descendant Information
    // =========================================================================

    int descendantCount;            ///< Number of daughter species
    std::vector<SpeciesId> daughterSpecies; ///< IDs of species derived from this
    int totalDescendantSpecies;     ///< Including all downstream descendants

    // =========================================================================
    // Trait Averages
    // =========================================================================

    /**
     * @brief Mean trait values for the species.
     *
     * Maps gene types to their population-average phenotypic values.
     */
    std::map<GeneType, float> meanTraitValues;

    /**
     * @brief Trait variance for the species.
     *
     * Maps gene types to their population variance.
     */
    std::map<GeneType, float> traitVariances;

    // =========================================================================
    // Extinction Information
    // =========================================================================

    ExtinctionCause extinctionCause; ///< Primary cause of extinction
    std::string extinctionNotes;    ///< Detailed extinction description
    float finalGeneticDiversity;    ///< Diversity at time of extinction
    int finalPopulation;            ///< Population when extinct declared

    // =========================================================================
    // Constructors
    // =========================================================================

    /**
     * @brief Default constructor with neutral values.
     */
    PhylogeneticRecord()
        : speciesId(0)
        , parentSpeciesId(0)
        , speciesName("")
        , foundingGeneration(0)
        , extinctionGeneration(-1)
        , founderPopulationSize(0)
        , founderGeneticDiversity(0.0f)
        , peakPopulation(0)
        , peakPopulationGen(0)
        , averagePopulationSize(0.0f)
        , primaryHabitat("")
        , nicheWidth(0.0f)
        , descendantCount(0)
        , totalDescendantSpecies(0)
        , extinctionCause(ExtinctionCause::UNKNOWN)
        , extinctionNotes("")
        , finalGeneticDiversity(0.0f)
        , finalPopulation(0)
    {}

    /**
     * @brief Constructs a phylogenetic record with core data.
     *
     * @param id Species identifier
     * @param parentId Parent species identifier
     * @param foundGen Generation of origin
     * @param name Species name
     */
    PhylogeneticRecord(SpeciesId id, SpeciesId parentId,
                       Generation foundGen, const std::string& name = "")
        : speciesId(id)
        , parentSpeciesId(parentId)
        , speciesName(name)
        , foundingGeneration(foundGen)
        , extinctionGeneration(-1)
        , founderPopulationSize(0)
        , founderGeneticDiversity(0.0f)
        , peakPopulation(0)
        , peakPopulationGen(foundGen)
        , averagePopulationSize(0.0f)
        , primaryHabitat("")
        , nicheWidth(0.0f)
        , descendantCount(0)
        , totalDescendantSpecies(0)
        , extinctionCause(ExtinctionCause::UNKNOWN)
        , extinctionNotes("")
        , finalGeneticDiversity(0.0f)
        , finalPopulation(0)
    {}

    // =========================================================================
    // Query Methods
    // =========================================================================

    /**
     * @brief Checks if the species is currently extant.
     * @return True if species has not gone extinct.
     */
    bool isExtant() const { return extinctionGeneration < 0; }

    /**
     * @brief Gets the duration of the species in generations.
     * @param currentGeneration Current simulation generation.
     * @return Number of generations the species existed/exists.
     */
    int getDuration(Generation currentGeneration) const {
        if (extinctionGeneration >= 0) {
            return extinctionGeneration - foundingGeneration;
        }
        return currentGeneration - foundingGeneration;
    }

    /**
     * @brief Calculates the speciation rate from this species.
     * @param currentGeneration Current generation for rate calculation.
     * @return Daughter species per generation.
     */
    float getSpeciationRate(Generation currentGeneration) const {
        int duration = getDuration(currentGeneration);
        if (duration <= 0) return 0.0f;
        return static_cast<float>(descendantCount) / static_cast<float>(duration);
    }

    /**
     * @brief Checks if this is a terminal species (no descendants).
     * @return True if species has no daughter species.
     */
    bool isTerminal() const { return descendantCount == 0; }

    /**
     * @brief Checks if this is an original/root species.
     * @return True if species has no parent.
     */
    bool isRoot() const { return parentSpeciesId == 0; }
};

// =============================================================================
// LINEAGE TREE NODE
// =============================================================================

/**
 * @brief Node in a lineage tree structure for hierarchical queries.
 *
 * Used internally by EvolutionaryHistoryTracker to maintain efficient
 * tree structures for ancestry queries.
 */
struct LineageTreeNode {
    LineageId lineageId;                ///< This node's lineage ID
    LineageId parentId;                 ///< Parent lineage (0 if root)
    std::vector<LineageId> childrenIds; ///< Direct descendant lineages
    Generation birthGeneration;         ///< When this lineage started
    Generation deathGeneration;         ///< When extinct (-1 if extant)
    int depth;                          ///< Distance from root ancestor
    float branchLength;                 ///< Evolutionary distance to parent

    /**
     * @brief Default constructor.
     */
    LineageTreeNode()
        : lineageId(0)
        , parentId(0)
        , birthGeneration(0)
        , deathGeneration(-1)
        , depth(0)
        , branchLength(0.0f)
    {}
};

// =============================================================================
// GENETIC DIVERSITY SNAPSHOT
// =============================================================================

/**
 * @brief Snapshot of genetic diversity metrics at a point in time.
 *
 * Used for tracking how genetic diversity changes over evolutionary time.
 */
struct GeneticDiversitySnapshot {
    Generation generation;              ///< When snapshot was taken
    float overallHeterozygosity;        ///< Population-wide heterozygosity
    float nucleotideDiversity;          ///< Average pairwise differences
    int numberOfAlleles;                ///< Total unique alleles in population
    float effectivePopulationSize;      ///< Ne estimate
    std::map<SpeciesId, float> speciesDiversity; ///< Per-species diversity

    /**
     * @brief Default constructor.
     */
    GeneticDiversitySnapshot()
        : generation(0)
        , overallHeterozygosity(0.0f)
        , nucleotideDiversity(0.0f)
        , numberOfAlleles(0)
        , effectivePopulationSize(0.0f)
    {}
};

// =============================================================================
// EVOLUTIONARY TREND ANALYSIS
// =============================================================================

/**
 * @brief Analysis of evolutionary trends for a specific trait.
 *
 * Summarizes how a trait has evolved over time with statistical measures.
 */
struct EvolutionaryTrendAnalysis {
    GeneType trait;                     ///< Trait being analyzed
    EvolutionaryTrend overallTrend;     ///< General direction of change
    float meanChangePerGeneration;      ///< Average rate of change
    float trendStrength;                ///< Statistical strength (R^2)
    float currentValue;                 ///< Most recent value
    float ancestralValue;               ///< Original value
    float totalChange;                  ///< Net change over history
    float volatility;                   ///< Variability in rate of change
    std::vector<std::pair<Generation, float>> trajectory; ///< Value history

    /**
     * @brief Default constructor.
     */
    EvolutionaryTrendAnalysis()
        : trait(GeneType::SIZE)
        , overallTrend(EvolutionaryTrend::STABLE)
        , meanChangePerGeneration(0.0f)
        , trendStrength(0.0f)
        , currentValue(0.0f)
        , ancestralValue(0.0f)
        , totalChange(0.0f)
        , volatility(0.0f)
    {}
};

// =============================================================================
// LINEAGE SUCCESS METRICS
// =============================================================================

/**
 * @brief Metrics for evaluating lineage evolutionary success.
 *
 * Used for ranking and comparing lineages by their evolutionary outcomes.
 */
struct LineageSuccessMetrics {
    LineageId lineageId;                ///< Lineage being evaluated
    int totalDescendants;               ///< All-time descendants
    int currentDescendants;             ///< Living descendants
    int speciesGenerated;               ///< Species arising from lineage
    float averageFitness;               ///< Mean fitness of members
    int generationsSurvived;            ///< Longevity
    float geneticContribution;          ///< Proportion of gene pool
    float compositeScore;               ///< Weighted success score

    /**
     * @brief Default constructor.
     */
    LineageSuccessMetrics()
        : lineageId(0)
        , totalDescendants(0)
        , currentDescendants(0)
        , speciesGenerated(0)
        , averageFitness(0.0f)
        , generationsSurvived(0)
        , geneticContribution(0.0f)
        , compositeScore(0.0f)
    {}

    /**
     * @brief Comparison operator for sorting by success.
     */
    bool operator<(const LineageSuccessMetrics& other) const {
        return compositeScore > other.compositeScore; // Descending order
    }
};

// =============================================================================
// EVOLUTIONARY HISTORY TRACKER CLASS
// =============================================================================

/**
 * @brief Central manager for tracking complete evolutionary history.
 *
 * This class provides comprehensive tracking and analysis of evolutionary
 * dynamics throughout a simulation. It maintains records of:
 * - Individual births and deaths
 * - Speciation and extinction events
 * - Trait changes over time
 * - Lineage relationships and ancestry
 * - Phylogenetic tree structure
 *
 * The tracker supports both real-time updates during simulation and
 * retrospective analysis of evolutionary patterns. It includes memory
 * management features to prevent unbounded growth of historical data.
 *
 * Thread Safety: This class is NOT thread-safe. External synchronization
 * is required if accessed from multiple threads.
 *
 * @example Basic Usage:
 * @code
 * EvolutionaryHistoryTracker tracker;
 *
 * // During simulation
 * tracker.recordBirth(newCreature, parent1, parent2);
 * tracker.recordDeath(deadCreature);
 * tracker.recordSpeciation(parentSpecies, childSpecies, cause);
 *
 * // Analysis
 * auto ancestor = tracker.getMostRecentCommonAncestor(lineage1, lineage2);
 * auto trends = tracker.getEvolutionaryTrends(GeneType::SIZE);
 * tracker.exportToNewick("phylogeny.nwk");
 * @endcode
 */
class EvolutionaryHistoryTracker {
public:
    // =========================================================================
    // CONSTRUCTION & DESTRUCTION
    // =========================================================================

    /**
     * @brief Default constructor.
     *
     * Initializes an empty tracker ready to record evolutionary events.
     */
    EvolutionaryHistoryTracker();

    /**
     * @brief Destructor.
     */
    ~EvolutionaryHistoryTracker();

    /**
     * @brief Copy constructor (deleted - tracker maintains unique state).
     */
    EvolutionaryHistoryTracker(const EvolutionaryHistoryTracker&) = delete;

    /**
     * @brief Copy assignment (deleted).
     */
    EvolutionaryHistoryTracker& operator=(const EvolutionaryHistoryTracker&) = delete;

    /**
     * @brief Move constructor.
     */
    EvolutionaryHistoryTracker(EvolutionaryHistoryTracker&&) noexcept = default;

    /**
     * @brief Move assignment.
     */
    EvolutionaryHistoryTracker& operator=(EvolutionaryHistoryTracker&&) noexcept = default;

    // =========================================================================
    // EVENT RECORDING - LIFE CYCLE
    // =========================================================================

    /**
     * @brief Records the birth of a new creature.
     *
     * Creates or updates lineage records based on parental information.
     * For sexual reproduction, lineage is inherited from the first parent.
     *
     * @param creature Pointer to the newly born creature.
     * @param parent1 First parent (primary for lineage inheritance).
     * @param parent2 Second parent (optional, nullptr for asexual).
     */
    void recordBirth(const Creature* creature,
                     const Creature* parent1,
                     const Creature* parent2 = nullptr);

    /**
     * @brief Records the death of a creature.
     *
     * Updates lineage statistics and checks for lineage extinction.
     *
     * @param creature Pointer to the deceased creature.
     */
    void recordDeath(const Creature* creature);

    // =========================================================================
    // EVENT RECORDING - SPECIATION & EXTINCTION
    // =========================================================================

    /**
     * @brief Records a speciation event.
     *
     * Documents the origin of a new species from an existing one,
     * including the cause and initial characteristics.
     *
     * @param parentSpecies ID of the ancestral species.
     * @param childSpecies ID of the newly formed species.
     * @param cause Mechanism of speciation.
     * @param founderGenomes Representative genomes of founders (optional).
     */
    void recordSpeciation(SpeciesId parentSpecies,
                          SpeciesId childSpecies,
                          SpeciationCause cause,
                          const std::vector<DiploidGenome>& founderGenomes = {});

    /**
     * @brief Records an extinction event.
     *
     * Documents the extinction of a species, including the cause
     * and final population characteristics.
     *
     * @param species ID of the extinct species.
     * @param cause Primary cause of extinction.
     * @param notes Additional details about the extinction.
     */
    void recordExtinction(SpeciesId species,
                          ExtinctionCause cause,
                          const std::string& notes = "");

    // =========================================================================
    // EVENT RECORDING - TRAIT CHANGES
    // =========================================================================

    /**
     * @brief Records a significant trait change in a species.
     *
     * Documents evolutionary changes in trait values, including
     * the selection context that drove the change.
     *
     * @param species ID of the species undergoing change.
     * @param trait Type of trait that changed.
     * @param oldValue Previous population mean value.
     * @param newValue New population mean value.
     * @param changeType Mode of evolutionary change.
     * @param pressure Selection pressure driving the change.
     */
    void recordTraitChange(SpeciesId species,
                           GeneType trait,
                           float oldValue,
                           float newValue,
                           ChangeType changeType = ChangeType::GRADUAL,
                           SelectionPressure pressure = SelectionPressure::NATURAL);

    /**
     * @brief Records a genomic-level change.
     *
     * @param lineage Lineage in which change occurred.
     * @param change The genomic change details.
     */
    void recordGenomicChange(LineageId lineage, const GenomicChange& change);

    // =========================================================================
    // ANCESTRY QUERIES
    // =========================================================================

    /**
     * @brief Finds the most recent common ancestor of two lineages.
     *
     * Traverses ancestry trees to find the nearest shared ancestor.
     * Returns 0 if lineages are unrelated.
     *
     * @param lineage1 First lineage ID.
     * @param lineage2 Second lineage ID.
     * @return LineageId of common ancestor, or 0 if none found.
     */
    LineageId getMostRecentCommonAncestor(LineageId lineage1,
                                          LineageId lineage2) const;

    /**
     * @brief Calculates evolutionary distance between two lineages.
     *
     * Returns the number of generations separating two lineages through
     * their most recent common ancestor. This represents genetic distance.
     *
     * @param lineage1 First lineage ID.
     * @param lineage2 Second lineage ID.
     * @return Generation count representing evolutionary distance, or -1 if unrelated.
     */
    int getEvolutionaryDistance(LineageId lineage1,
                                LineageId lineage2) const;

    /**
     * @brief Gets the complete ancestry chain for a lineage.
     *
     * Returns all ancestor lineage IDs from the given lineage back to
     * the original founder, in order from most recent to most ancient.
     *
     * @param lineage Starting lineage ID.
     * @return Vector of ancestor lineage IDs.
     */
    std::vector<LineageId> getAncestry(LineageId lineage) const;

    /**
     * @brief Gets all descendants of a lineage.
     *
     * @param lineage Ancestor lineage ID.
     * @param extantOnly If true, only return currently living lineages.
     * @return Vector of descendant lineage IDs.
     */
    std::vector<LineageId> getDescendants(LineageId lineage,
                                          bool extantOnly = false) const;

    // =========================================================================
    // TRAIT HISTORY QUERIES
    // =========================================================================

    /**
     * @brief Gets the history of changes for a specific trait in a species.
     *
     * Returns all recorded trait changes for the specified trait and species,
     * ordered chronologically.
     *
     * @param species Species ID.
     * @param trait Type of trait.
     * @return Vector of trait changes in chronological order.
     */
    std::vector<TraitChange> getTraitHistory(SpeciesId species,
                                             GeneType trait) const;

    /**
     * @brief Gets trait value at a specific historical generation.
     *
     * Reconstructs the approximate trait value for a species at a
     * given point in its history.
     *
     * @param species Species ID.
     * @param trait Type of trait.
     * @param generation Target generation.
     * @return Estimated trait value at that generation.
     */
    float getHistoricalTraitValue(SpeciesId species,
                                  GeneType trait,
                                  Generation generation) const;

    // =========================================================================
    // TREE ACCESS
    // =========================================================================

    /**
     * @brief Gets the complete lineage tree structure.
     *
     * Returns a map of all lineage tree nodes for custom traversal
     * and analysis.
     *
     * @return Const reference to the lineage tree node map.
     */
    const std::unordered_map<LineageId, LineageTreeNode>& getLineageTree() const;

    /**
     * @brief Gets all root lineages (original founders).
     *
     * @return Vector of lineage IDs with no parents.
     */
    std::vector<LineageId> getRootLineages() const;

    /**
     * @brief Gets the maximum tree depth.
     *
     * @return Maximum number of generations from any root to leaf.
     */
    int getMaxTreeDepth() const;

    // =========================================================================
    // RECORD ACCESS
    // =========================================================================

    /**
     * @brief Gets the lineage record for a specific lineage.
     *
     * @param lineage Lineage ID to retrieve.
     * @return Pointer to record, or nullptr if not found.
     */
    const LineageRecord* getLineageRecord(LineageId lineage) const;

    /**
     * @brief Gets the phylogenetic record for a specific species.
     *
     * @param species Species ID to retrieve.
     * @return Pointer to record, or nullptr if not found.
     */
    const PhylogeneticRecord* getPhylogeneticRecord(SpeciesId species) const;

    /**
     * @brief Gets all lineage records.
     *
     * @return Const reference to the map of all lineage records.
     */
    const std::unordered_map<LineageId, LineageRecord>& getAllLineageRecords() const;

    /**
     * @brief Gets all phylogenetic records.
     *
     * @return Const reference to the map of all phylogenetic records.
     */
    const std::unordered_map<SpeciesId, PhylogeneticRecord>& getAllPhylogeneticRecords() const;

    // =========================================================================
    // RATE CALCULATIONS
    // =========================================================================

    /**
     * @brief Calculates the extinction rate over a generation window.
     *
     * Returns the number of extinctions per generation over the
     * specified window of generations.
     *
     * @param generationWindow Number of generations to consider.
     * @return Extinctions per generation.
     */
    float getExtinctionRate(int generationWindow = 100) const;

    /**
     * @brief Calculates the speciation rate over a generation window.
     *
     * Returns the number of speciation events per generation over the
     * specified window of generations.
     *
     * @param generationWindow Number of generations to consider.
     * @return Speciations per generation.
     */
    float getSpeciationRate(int generationWindow = 100) const;

    /**
     * @brief Calculates the net diversification rate.
     *
     * Speciation rate minus extinction rate, representing net gain
     * or loss of species diversity.
     *
     * @param generationWindow Number of generations to consider.
     * @return Net diversification rate.
     */
    float getDiversificationRate(int generationWindow = 100) const;

    /**
     * @brief Calculates turnover rate.
     *
     * Sum of speciation and extinction rates, representing total
     * evolutionary activity.
     *
     * @param generationWindow Number of generations to consider.
     * @return Turnover rate.
     */
    float getTurnoverRate(int generationWindow = 100) const;

    // =========================================================================
    // STATISTICS - LINEAGE ANALYSIS
    // =========================================================================

    /**
     * @brief Calculates the average lifespan of all lineages.
     *
     * For extinct lineages, uses actual lifespan. For extant lineages,
     * uses current age. Optionally include only extinct lineages.
     *
     * @param extinctOnly If true, only consider extinct lineages.
     * @return Average lineage lifespan in generations.
     */
    float getAverageLineageLifespan(bool extinctOnly = false) const;

    /**
     * @brief Gets the most successful lineages by a composite score.
     *
     * Success is measured by a combination of longevity, descendant count,
     * fitness, and species generated.
     *
     * @param count Number of top lineages to return.
     * @return Vector of success metrics, sorted by score.
     */
    std::vector<LineageSuccessMetrics> getMostSuccessfulLineages(int count) const;

    /**
     * @brief Gets lineages that have survived the longest.
     *
     * @param count Number of lineages to return.
     * @return Vector of lineage IDs sorted by longevity.
     */
    std::vector<LineageId> getLongestSurvivingLineages(int count) const;

    /**
     * @brief Gets lineages with the most descendants.
     *
     * @param count Number of lineages to return.
     * @param extantOnly If true, count only living descendants.
     * @return Vector of lineage IDs sorted by descendant count.
     */
    std::vector<LineageId> getMostProlificLineages(int count,
                                                    bool extantOnly = false) const;

    // =========================================================================
    // STATISTICS - EVOLUTIONARY TRENDS
    // =========================================================================

    /**
     * @brief Analyzes evolutionary trends for a specific trait.
     *
     * Examines how a trait has changed across all species and lineages
     * over time, identifying patterns and calculating statistics.
     *
     * @param trait Type of trait to analyze.
     * @return Comprehensive trend analysis.
     */
    EvolutionaryTrendAnalysis getEvolutionaryTrends(GeneType trait) const;

    /**
     * @brief Gets trends for all traits.
     *
     * @return Map of trait types to their trend analyses.
     */
    std::map<GeneType, EvolutionaryTrendAnalysis> getAllTraitTrends() const;

    /**
     * @brief Identifies the most rapidly evolving traits.
     *
     * @param count Number of traits to return.
     * @return Vector of trait types sorted by rate of change.
     */
    std::vector<GeneType> getFastestEvolvingTraits(int count) const;

    // =========================================================================
    // STATISTICS - GENETIC DIVERSITY
    // =========================================================================

    /**
     * @brief Gets the history of genetic diversity over time.
     *
     * Returns snapshots of genetic diversity metrics at regular
     * intervals throughout the simulation history.
     *
     * @param interval Generation interval between snapshots (0 = all available).
     * @return Vector of diversity snapshots.
     */
    std::vector<GeneticDiversitySnapshot> getGeneticDiversityHistory(
        int interval = 0) const;

    /**
     * @brief Gets the current genetic diversity.
     *
     * @return Most recent diversity snapshot.
     */
    GeneticDiversitySnapshot getCurrentDiversity() const;

    /**
     * @brief Records a diversity snapshot.
     *
     * Should be called periodically during simulation to build
     * diversity history.
     *
     * @param snapshot Diversity metrics to record.
     */
    void recordDiversitySnapshot(const GeneticDiversitySnapshot& snapshot);

    // =========================================================================
    // EXPORT FUNCTIONS
    // =========================================================================

    /**
     * @brief Exports the phylogenetic tree in Newick format.
     *
     * Newick format is a standard for representing tree structures
     * and can be read by most phylogenetic analysis software.
     *
     * @return String in Newick format representing the species tree.
     */
    std::string exportToNewick() const;

    /**
     * @brief Exports phylogenetic tree to a Newick file.
     *
     * @param filename Path to output file.
     * @return True if export succeeded.
     */
    bool exportNewickToFile(const std::string& filename) const;

    /**
     * @brief Exports all evolutionary data to CSV files.
     *
     * Creates multiple CSV files with different aspects of the
     * evolutionary history:
     * - lineages.csv: Lineage records
     * - species.csv: Phylogenetic records
     * - trait_changes.csv: Trait change history
     * - diversity.csv: Genetic diversity over time
     *
     * @param baseFilename Base filename (extensions will be added).
     * @return True if all exports succeeded.
     */
    bool exportToCSV(const std::string& baseFilename) const;

    /**
     * @brief Exports a specific data type to CSV.
     *
     * @param filename Output filename.
     * @param dataType Type of data: "lineages", "species", "traits", "diversity".
     * @return True if export succeeded.
     */
    bool exportDataToCSV(const std::string& filename,
                         const std::string& dataType) const;

    // =========================================================================
    // MEMORY MANAGEMENT
    // =========================================================================

    /**
     * @brief Prunes old records to manage memory usage.
     *
     * Removes detailed records for extinct lineages and species older
     * than the specified threshold, while preserving summary statistics
     * and essential ancestry information.
     *
     * @param generationThreshold Records older than this are candidates for pruning.
     * @param preserveAncestry If true, keep minimal ancestry data for tree structure.
     * @return Number of records pruned.
     */
    int pruneOldRecords(Generation generationThreshold,
                        bool preserveAncestry = true);

    /**
     * @brief Gets the current memory usage estimate.
     *
     * @return Approximate memory usage in bytes.
     */
    size_t getMemoryUsage() const;

    /**
     * @brief Sets the maximum records to retain.
     *
     * When exceeded, oldest records are automatically pruned.
     * Set to 0 to disable automatic pruning.
     *
     * @param maxLineages Maximum lineage records.
     * @param maxSpecies Maximum species records.
     */
    void setMaxRecords(size_t maxLineages, size_t maxSpecies);

    /**
     * @brief Clears all recorded history.
     *
     * Resets the tracker to its initial empty state.
     */
    void clear();

    // =========================================================================
    // CONFIGURATION
    // =========================================================================

    /**
     * @brief Sets the current generation for reference.
     *
     * Should be called each simulation tick to keep generation
     * tracking synchronized.
     *
     * @param generation Current simulation generation.
     */
    void setCurrentGeneration(Generation generation);

    /**
     * @brief Gets the current generation.
     *
     * @return Current simulation generation.
     */
    Generation getCurrentGeneration() const;

    /**
     * @brief Sets the minimum change threshold for recording trait changes.
     *
     * Changes smaller than this threshold are not recorded to reduce
     * noise and memory usage.
     *
     * @param threshold Minimum absolute change to record.
     */
    void setTraitChangeThreshold(float threshold);

    /**
     * @brief Enables or disables detailed genomic change tracking.
     *
     * Detailed tracking increases memory usage but provides richer
     * evolutionary history.
     *
     * @param enabled True to enable detailed tracking.
     */
    void setDetailedGenomicTracking(bool enabled);

    // =========================================================================
    // VALIDATION & DEBUGGING
    // =========================================================================

    /**
     * @brief Validates the internal consistency of all records.
     *
     * Checks for orphaned records, invalid references, and logical
     * inconsistencies in the evolutionary history.
     *
     * @return True if all records are consistent.
     */
    bool validateRecords() const;

    /**
     * @brief Gets summary statistics for debugging.
     *
     * @return String with summary of record counts and statistics.
     */
    std::string getDebugSummary() const;

private:
    // =========================================================================
    // INTERNAL DATA STRUCTURES
    // =========================================================================

    /// All lineage records indexed by lineage ID
    std::unordered_map<LineageId, LineageRecord> m_lineageRecords;

    /// All phylogenetic records indexed by species ID
    std::unordered_map<SpeciesId, PhylogeneticRecord> m_phylogeneticRecords;

    /// Lineage tree structure for ancestry queries
    std::unordered_map<LineageId, LineageTreeNode> m_lineageTree;

    /// All trait changes, indexed by species
    std::unordered_map<SpeciesId, std::vector<TraitChange>> m_traitChangesBySpecies;

    /// Genetic diversity history
    std::deque<GeneticDiversitySnapshot> m_diversityHistory;

    /// Current simulation generation
    Generation m_currentGeneration;

    /// Threshold for recording trait changes
    float m_traitChangeThreshold;

    /// Whether to track detailed genomic changes
    bool m_detailedGenomicTracking;

    /// Maximum records before automatic pruning (0 = no limit)
    size_t m_maxLineageRecords;
    size_t m_maxSpeciesRecords;

    /// Counters for statistics
    int m_totalBirths;
    int m_totalDeaths;
    int m_totalSpeciations;
    int m_totalExtinctions;

    /// Generation-indexed event counts for rate calculations
    std::map<Generation, int> m_speciationsByGeneration;
    std::map<Generation, int> m_extinctionsByGeneration;

    /// Cache for common ancestor queries
    mutable std::unordered_map<uint64_t, LineageId> m_ancestorCache;

    // =========================================================================
    // INTERNAL HELPER METHODS
    // =========================================================================

    /**
     * @brief Creates a new lineage record for a creature.
     *
     * @param creature The creature to create a record for.
     * @param parentLineage Parent lineage ID (0 if new founder).
     * @return The newly created lineage ID.
     */
    LineageId createLineageRecord(const Creature* creature,
                                  LineageId parentLineage);

    /**
     * @brief Updates lineage statistics when a creature dies.
     *
     * @param lineageId The lineage to update.
     */
    void updateLineageOnDeath(LineageId lineageId);

    /**
     * @brief Builds ancestor chain for MRCA calculations.
     *
     * @param lineage Starting lineage.
     * @return Set of all ancestor lineage IDs.
     */
    std::unordered_set<LineageId> buildAncestorSet(LineageId lineage) const;

    /**
     * @brief Generates Newick string recursively.
     *
     * @param speciesId Current species node.
     * @return Newick substring for this subtree.
     */
    std::string newickForSpecies(SpeciesId speciesId) const;

    /**
     * @brief Calculates composite success score for a lineage.
     *
     * @param record Lineage record to evaluate.
     * @return Weighted success score.
     */
    float calculateSuccessScore(const LineageRecord& record) const;

    /**
     * @brief Automatically prunes if record limits are exceeded.
     */
    void autoPruneIfNeeded();

    /**
     * @brief Creates cache key for ancestor pair.
     *
     * @param l1 First lineage.
     * @param l2 Second lineage.
     * @return Unique cache key.
     */
    uint64_t makeAncestorCacheKey(LineageId l1, LineageId l2) const;

    /**
     * @brief Invalidates ancestor cache entries affected by changes.
     *
     * @param lineage Lineage that was modified.
     */
    void invalidateAncestorCache(LineageId lineage);

    /// Next lineage ID to assign
    static LineageId s_nextLineageId;
};

// =============================================================================
// INLINE IMPLEMENTATIONS
// =============================================================================

inline void EvolutionaryHistoryTracker::setCurrentGeneration(Generation generation) {
    m_currentGeneration = generation;
}

inline Generation EvolutionaryHistoryTracker::getCurrentGeneration() const {
    return m_currentGeneration;
}

inline void EvolutionaryHistoryTracker::setTraitChangeThreshold(float threshold) {
    m_traitChangeThreshold = threshold;
}

inline void EvolutionaryHistoryTracker::setDetailedGenomicTracking(bool enabled) {
    m_detailedGenomicTracking = enabled;
}

inline const std::unordered_map<LineageId, LineageRecord>&
EvolutionaryHistoryTracker::getAllLineageRecords() const {
    return m_lineageRecords;
}

inline const std::unordered_map<SpeciesId, PhylogeneticRecord>&
EvolutionaryHistoryTracker::getAllPhylogeneticRecords() const {
    return m_phylogeneticRecords;
}

inline const std::unordered_map<LineageId, LineageTreeNode>&
EvolutionaryHistoryTracker::getLineageTree() const {
    return m_lineageTree;
}

inline float EvolutionaryHistoryTracker::getDiversificationRate(int generationWindow) const {
    return getSpeciationRate(generationWindow) - getExtinctionRate(generationWindow);
}

inline float EvolutionaryHistoryTracker::getTurnoverRate(int generationWindow) const {
    return getSpeciationRate(generationWindow) + getExtinctionRate(generationWindow);
}

} // namespace genetics
