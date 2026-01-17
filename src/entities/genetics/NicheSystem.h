#pragma once

/**
 * @file NicheSystem.h
 * @brief Comprehensive ecological niche system for evolution simulation
 *
 * This module provides a sophisticated niche modeling system that implements
 * core ecological concepts for creature evolution:
 *
 * - Ecological niches defining resource usage, behavior patterns, and habitat
 * - Competitive exclusion and niche overlap calculations
 * - Character displacement detection and tracking
 * - Niche partitioning for resource sharing species
 * - Specialist vs generalist strategy evaluation
 *
 * Key ecological principles modeled:
 * - Gause's Law (competitive exclusion principle)
 * - Hutchinson's n-dimensional hypervolume niche concept
 * - MacArthur's resource partitioning theory
 * - Ecological character displacement
 *
 * @author OrganismEvolution Project
 * @version 1.0
 */

#include "DiploidGenome.h"
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <memory>
#include <functional>
#include <deque>
#include <cstdint>

// Forward declarations
class Creature;
class Terrain;

namespace genetics {

// =============================================================================
// NICHE TYPE ENUMERATION
// =============================================================================

/**
 * @enum NicheType
 * @brief Defines ecological roles that creatures can occupy in the ecosystem
 *
 * These niche types represent fundamental feeding strategies and ecological
 * functions. Each type has characteristic behaviors, resource requirements,
 * and competitive relationships with other types.
 */
enum class NicheType : uint8_t {
    // -------------------------------------------------------------------------
    // Herbivore niches - Primary consumers
    // -------------------------------------------------------------------------

    /** @brief Ground-level vegetation consumers (grasses, low plants) */
    GRAZER,

    /** @brief Elevated vegetation consumers (leaves, twigs from trees/shrubs) */
    BROWSER,

    /** @brief Fruit and seed consumers (important for seed dispersal) */
    FRUGIVORE,

    // -------------------------------------------------------------------------
    // Carnivore niches - Secondary/Tertiary consumers
    // -------------------------------------------------------------------------

    /** @brief Stealth hunters using concealment and surprise attacks */
    AMBUSH_PREDATOR,

    /** @brief Endurance hunters using speed and stamina to run down prey */
    PURSUIT_PREDATOR,

    // -------------------------------------------------------------------------
    // Specialized feeding niches
    // -------------------------------------------------------------------------

    /** @brief Dead organism consumers (ecosystem cleanup, nutrient cycling) */
    SCAVENGER,

    /** @brief Aquatic particle feeders (plankton, suspended organic matter) */
    FILTER_FEEDER,

    // -------------------------------------------------------------------------
    // Symbiotic relationship niches
    // -------------------------------------------------------------------------

    /** @brief Organisms living on/in hosts, potentially harmful */
    PARASITE,

    /** @brief Organisms in mutually beneficial relationships */
    SYMBIONT,

    // -------------------------------------------------------------------------
    // Ecological service niches
    // -------------------------------------------------------------------------

    /** @brief Flower visitors facilitating plant reproduction */
    POLLINATOR,

    /** @brief Seed transporters enabling plant dispersal */
    SEED_DISPERSER,

    // -------------------------------------------------------------------------
    // Meta category
    // -------------------------------------------------------------------------

    /** @brief Unknown or transitional niche state */
    UNDEFINED,

    /** @brief Total count of niche types (for iteration) */
    COUNT
};

// =============================================================================
// RESOURCE TYPE ENUMERATION
// =============================================================================

/**
 * @enum ResourceType
 * @brief Types of resources that creatures can consume or utilize
 */
enum class ResourceType : uint8_t {
    /** @brief Grass, leaves, and general plant material */
    PLANT_MATTER,

    /** @brief Fruits, berries, and fleshy plant parts */
    FRUIT,

    /** @brief Seeds, nuts, and grains */
    SEEDS,

    /** @brief Nectar from flowers */
    NECTAR,

    /** @brief Meat from hunting live prey */
    LIVE_PREY,

    /** @brief Meat from already dead organisms */
    CARRION,

    /** @brief Dead organic matter, decomposing material */
    DETRITUS,

    /** @brief Microscopic organisms and particles */
    PLANKTON,

    /** @brief Blood and host tissues (for parasites) */
    HOST_TISSUE,

    /** @brief Insects and other invertebrates */
    INSECTS,

    /** @brief Total count for iteration */
    COUNT
};

// =============================================================================
// HUNTING STRATEGY ENUMERATION
// =============================================================================

/**
 * @enum HuntingStrategy
 * @brief Methods used by predators to capture prey
 */
enum class HuntingStrategy : uint8_t {
    /** @brief No active hunting (herbivores, detritivores) */
    NONE,

    /** @brief Hide and wait for prey to approach */
    AMBUSH,

    /** @brief Chase prey over distance using endurance */
    PURSUIT,

    /** @brief Hunt cooperatively in groups */
    PACK_HUNTING,

    /** @brief Strain food particles from water */
    FILTER,

    /** @brief Dig or probe for buried food */
    FORAGING,

    /** @brief Locate dead organisms by smell/sight */
    SCAVENGING,

    /** @brief Attach to and feed on living hosts */
    PARASITIC,

    /** @brief Total count for iteration */
    COUNT
};

// =============================================================================
// ACTIVITY PATTERN ENUMERATION
// =============================================================================

/**
 * @enum ActivityPattern
 * @brief Time of day when creature is most active
 *
 * Activity patterns are important for temporal niche partitioning,
 * allowing species to share resources by being active at different times.
 */
enum class ActivityPattern : uint8_t {
    /** @brief Active during daylight hours */
    DIURNAL,

    /** @brief Active during nighttime */
    NOCTURNAL,

    /** @brief Active during dawn and dusk */
    CREPUSCULAR,

    /** @brief Active throughout day and night (no clear pattern) */
    CATHEMERAL,

    /** @brief Total count for iteration */
    COUNT
};

// =============================================================================
// HABITAT TYPE ENUMERATION
// =============================================================================

/**
 * @enum HabitatType
 * @brief Primary habitat preferences for creatures
 */
enum class HabitatType : uint8_t {
    /** @brief Dense tree coverage environments */
    FOREST,

    /** @brief Open grassland environments */
    PLAINS,

    /** @brief Arid, low-rainfall environments */
    DESERT,

    /** @brief Wetland and swamp environments */
    WETLAND,

    /** @brief Lakes, rivers, and ponds (freshwater) */
    FRESHWATER,

    /** @brief Ocean and coastal environments */
    MARINE,

    /** @brief High altitude environments */
    MOUNTAIN,

    /** @brief Rocky terrain with caves */
    CAVE,

    /** @brief Transition zones between habitats */
    ECOTONE,

    /** @brief Total count for iteration */
    COUNT
};

// =============================================================================
// NICHE CHARACTERISTICS STRUCTURE
// =============================================================================

/**
 * @struct NicheCharacteristics
 * @brief Complete description of an ecological niche's properties
 *
 * This structure encapsulates all the defining characteristics of an
 * ecological niche, forming a multi-dimensional niche hypervolume as
 * conceptualized by G. Evelyn Hutchinson.
 */
struct NicheCharacteristics {
    // -------------------------------------------------------------------------
    // Resource preferences (weighted 0-1 for each resource type)
    // -------------------------------------------------------------------------

    /** @brief Preference weights for each resource type */
    std::map<ResourceType, float> resourcePreferences;

    /** @brief Primary resource type this niche exploits */
    ResourceType primaryResource = ResourceType::PLANT_MATTER;

    /** @brief Secondary resource type (for omnivores/generalists) */
    ResourceType secondaryResource = ResourceType::PLANT_MATTER;

    /** @brief Efficiency of resource extraction (0-1) */
    float resourceEfficiency = 0.5f;

    // -------------------------------------------------------------------------
    // Hunting and foraging strategy
    // -------------------------------------------------------------------------

    /** @brief Primary method of obtaining food */
    HuntingStrategy huntingStrategy = HuntingStrategy::FORAGING;

    /** @brief Attack/foraging success rate (0-1) */
    float huntingEfficiency = 0.5f;

    /** @brief Preferred prey size relative to self (-1 smaller, +1 larger) */
    float preySize Preference = 0.0f;

    /** @brief Distance at which hunting/foraging occurs */
    float foragingRange = 20.0f;

    // -------------------------------------------------------------------------
    // Temporal activity pattern
    // -------------------------------------------------------------------------

    /** @brief When the creature is primarily active */
    ActivityPattern activityPattern = ActivityPattern::DIURNAL;

    /** @brief How strictly the activity pattern is followed (0-1) */
    float activityStrictness = 0.5f;

    /** @brief Peak activity time (0-24 hour scale) */
    float peakActivityTime = 12.0f;

    // -------------------------------------------------------------------------
    // Habitat requirements
    // -------------------------------------------------------------------------

    /** @brief Primary habitat preference */
    HabitatType primaryHabitat = HabitatType::PLAINS;

    /** @brief Secondary acceptable habitat */
    HabitatType secondaryHabitat = HabitatType::PLAINS;

    /** @brief Suitability weights for each habitat type */
    std::map<HabitatType, float> habitatSuitability;

    /** @brief Minimum vegetation density required (0-1) */
    float minVegetationDensity = 0.0f;

    /** @brief Maximum vegetation density tolerated (0-1) */
    float maxVegetationDensity = 1.0f;

    /** @brief Preferred elevation range (min, max) */
    glm::vec2 elevationRange = glm::vec2(0.0f, 1000.0f);

    /** @brief Preferred temperature range (min, max in normalized 0-1) */
    glm::vec2 temperatureRange = glm::vec2(0.2f, 0.8f);

    /** @brief Preferred moisture/humidity range */
    glm::vec2 moistureRange = glm::vec2(0.2f, 0.8f);

    // -------------------------------------------------------------------------
    // Niche dimensions (specialist vs generalist)
    // -------------------------------------------------------------------------

    /**
     * @brief Overall niche width (0 = extreme specialist, 1 = extreme generalist)
     *
     * Specialists have narrow tolerance ranges but high efficiency within them.
     * Generalists have broad tolerance but lower peak efficiency.
     */
    float nicheWidth = 0.5f;

    /** @brief Diet breadth (number of resource types effectively used) */
    float dietBreadth = 0.5f;

    /** @brief Habitat breadth (number of habitats effectively used) */
    float habitatBreadth = 0.5f;

    /** @brief Temporal breadth (activity period flexibility) */
    float temporalBreadth = 0.5f;

    // -------------------------------------------------------------------------
    // Social and competitive traits
    // -------------------------------------------------------------------------

    /** @brief Tendency to defend resources/territory (0-1) */
    float territoriality = 0.5f;

    /** @brief Competitive ability against other species (0-1) */
    float competitiveAbility = 0.5f;

    /** @brief Tolerance for intraspecific competition (0-1) */
    float intraspecificTolerance = 0.5f;

    // -------------------------------------------------------------------------
    // Constructors
    // -------------------------------------------------------------------------

    NicheCharacteristics();

    /**
     * @brief Construct characteristics for a specific niche type
     * @param type The niche type to create default characteristics for
     */
    explicit NicheCharacteristics(NicheType type);

    // -------------------------------------------------------------------------
    // Utility methods
    // -------------------------------------------------------------------------

    /**
     * @brief Calculate the Euclidean distance to another niche in hyperspace
     * @param other The niche to compare against
     * @return Distance value (0 = identical, higher = more different)
     */
    float distanceTo(const NicheCharacteristics& other) const;

    /**
     * @brief Calculate overlap percentage with another niche
     * @param other The niche to compare against
     * @return Overlap value (0 = no overlap, 1 = complete overlap)
     */
    float calculateOverlap(const NicheCharacteristics& other) const;

    /**
     * @brief Check if this niche can exist in given environmental conditions
     * @param temperature Current temperature (normalized 0-1)
     * @param moisture Current moisture level (normalized 0-1)
     * @param elevation Current elevation
     * @return Suitability score (0 = unsuitable, 1 = optimal)
     */
    float evaluateEnvironmentSuitability(float temperature, float moisture,
                                         float elevation) const;
};

// =============================================================================
// NICHE COMPETITION STRUCTURE
// =============================================================================

/**
 * @struct NicheCompetition
 * @brief Tracks competitive interactions between two niches
 *
 * This structure monitors the ongoing competitive dynamics between
 * two ecological niches, including overlap intensity, population effects,
 * and potential character displacement.
 */
struct NicheCompetition {
    /** @brief First niche in the competitive relationship */
    NicheType niche1 = NicheType::UNDEFINED;

    /** @brief Second niche in the competitive relationship */
    NicheType niche2 = NicheType::UNDEFINED;

    /** @brief Species ID occupying niche1 (0 if multiple/none) */
    SpeciesId species1 = 0;

    /** @brief Species ID occupying niche2 (0 if multiple/none) */
    SpeciesId species2 = 0;

    // -------------------------------------------------------------------------
    // Competition metrics
    // -------------------------------------------------------------------------

    /**
     * @brief Lotka-Volterra competition coefficient (alpha)
     *
     * Measures how much niche1 affects niche2's carrying capacity.
     * alpha = 0: no competition
     * alpha = 1: equivalent to intraspecific competition
     * alpha > 1: stronger than intraspecific competition
     */
    float competitionCoefficient12 = 0.0f;

    /** @brief Competition coefficient from niche2 on niche1 */
    float competitionCoefficient21 = 0.0f;

    /** @brief Proportion of resources shared between niches (0-1) */
    float resourceOverlap = 0.0f;

    /** @brief Proportion of habitat shared between niches (0-1) */
    float habitatOverlap = 0.0f;

    /** @brief Proportion of active time shared between niches (0-1) */
    float temporalOverlap = 0.0f;

    /** @brief Combined niche overlap (Pianka's index style) */
    float totalOverlap = 0.0f;

    // -------------------------------------------------------------------------
    // Population effects
    // -------------------------------------------------------------------------

    /** @brief Fitness reduction for niche1 due to competition */
    float fitnessImpact1 = 0.0f;

    /** @brief Fitness reduction for niche2 due to competition */
    float fitnessImpact2 = 0.0f;

    /** @brief Population density of niche1 in shared areas */
    float density1 = 0.0f;

    /** @brief Population density of niche2 in shared areas */
    float density2 = 0.0f;

    // -------------------------------------------------------------------------
    // Temporal tracking
    // -------------------------------------------------------------------------

    /** @brief Generation when competition was first detected */
    int firstDetectedGeneration = 0;

    /** @brief Number of generations this competition has persisted */
    int generationsPersisted = 0;

    /** @brief Is competition currently active */
    bool isActive = false;

    /** @brief Historical overlap values for trend analysis */
    std::deque<float> overlapHistory;

    /** @brief Maximum history entries to retain */
    static constexpr size_t MAX_HISTORY = 100;

    // -------------------------------------------------------------------------
    // Character displacement tracking
    // -------------------------------------------------------------------------

    /** @brief Has character displacement been detected */
    bool displacementDetected = false;

    /** @brief Magnitude of character displacement observed */
    float displacementMagnitude = 0.0f;

    /** @brief Primary trait showing displacement */
    std::string displacementTrait;

    // -------------------------------------------------------------------------
    // Methods
    // -------------------------------------------------------------------------

    NicheCompetition() = default;
    NicheCompetition(NicheType n1, NicheType n2);

    /**
     * @brief Update competition metrics with new observation
     * @param newOverlap Current measured overlap
     * @param generation Current generation number
     */
    void update(float newOverlap, int generation);

    /**
     * @brief Calculate trend in competition intensity
     * @return Positive = increasing, negative = decreasing, 0 = stable
     */
    float calculateTrend() const;

    /**
     * @brief Predict competitive exclusion outcome
     * @return 1 if niche1 likely wins, -1 if niche2, 0 if coexistence
     */
    int predictOutcome() const;
};

// =============================================================================
// NICHE PARTITION RECORD
// =============================================================================

/**
 * @struct NichePartition
 * @brief Records a niche partitioning event between species
 *
 * Niche partitioning occurs when species sharing resources evolve
 * to use different portions of the available resource spectrum.
 */
struct NichePartition {
    /** @brief Original shared niche type */
    NicheType originalNiche = NicheType::UNDEFINED;

    /** @brief Species that shifted to sub-niche A */
    SpeciesId speciesA = 0;

    /** @brief Species that shifted to sub-niche B */
    SpeciesId speciesB = 0;

    /** @brief Dimension along which partitioning occurred */
    std::string partitionDimension;

    /** @brief Generation when partitioning was detected */
    int generation = 0;

    /** @brief New niche position for species A */
    float positionA = 0.0f;

    /** @brief New niche position for species B */
    float positionB = 0.0f;

    /** @brief Separation distance after partitioning */
    float separation = 0.0f;

    /** @brief Was partitioning driven by competition */
    bool competitionDriven = false;
};

// =============================================================================
// CHARACTER DISPLACEMENT RECORD
// =============================================================================

/**
 * @struct CharacterDisplacement
 * @brief Records evolutionary character displacement between species
 *
 * Character displacement is the phenomenon where competing species
 * evolve to become more different where they coexist (sympatry)
 * than where they occur alone (allopatry).
 */
struct CharacterDisplacement {
    /** @brief First species in displacement pair */
    SpeciesId species1 = 0;

    /** @brief Second species in displacement pair */
    SpeciesId species2 = 0;

    /** @brief Trait undergoing displacement */
    std::string traitName;

    /** @brief Generation when displacement started */
    int startGeneration = 0;

    /** @brief Generations over which displacement occurred */
    int duration = 0;

    /** @brief Initial trait difference between species */
    float initialDifference = 0.0f;

    /** @brief Final trait difference between species */
    float finalDifference = 0.0f;

    /** @brief Magnitude of displacement (final - initial) */
    float displacementMagnitude = 0.0f;

    /** @brief Direction: +1 = divergence, -1 = convergence */
    int direction = 0;

    /** @brief Competition intensity that drove displacement */
    float competitionIntensity = 0.0f;

    /** @brief Geographic region where displacement occurred */
    glm::vec3 regionCentroid = glm::vec3(0.0f);

    /** @brief Is displacement still ongoing */
    bool ongoing = false;
};

// =============================================================================
// NICHE SHIFT RECORD
// =============================================================================

/**
 * @struct NicheShift
 * @brief Records when a species shifts to a new ecological niche
 */
struct NicheShift {
    /** @brief Species undergoing the shift */
    SpeciesId speciesId = 0;

    /** @brief Original niche type */
    NicheType fromNiche = NicheType::UNDEFINED;

    /** @brief New niche type */
    NicheType toNiche = NicheType::UNDEFINED;

    /** @brief Generation when shift occurred */
    int generation = 0;

    /** @brief Cause of the shift */
    std::string cause;

    /** @brief Was the new niche previously empty */
    bool colonizedEmptyNiche = false;

    /** @brief Fitness before shift */
    float fitnessBefore = 0.0f;

    /** @brief Fitness after shift */
    float fitnessAfter = 0.0f;

    /** @brief Population size at time of shift */
    int populationSize = 0;
};

// =============================================================================
// NICHE OCCUPANCY DATA
// =============================================================================

/**
 * @struct NicheOccupancy
 * @brief Tracks population statistics for a single niche
 */
struct NicheOccupancy {
    /** @brief The niche type being tracked */
    NicheType nicheType = NicheType::UNDEFINED;

    /** @brief Current population count in this niche */
    int currentPopulation = 0;

    /** @brief Number of distinct species in this niche */
    int speciesCount = 0;

    /** @brief List of species IDs occupying this niche */
    std::vector<SpeciesId> occupyingSpecies;

    /** @brief Average fitness of creatures in this niche */
    float averageFitness = 0.0f;

    /** @brief Average niche width of occupants (specialist vs generalist) */
    float averageNicheWidth = 0.5f;

    /** @brief Historical population for trend analysis */
    std::deque<int> populationHistory;

    /** @brief Carrying capacity estimate for this niche */
    int estimatedCarryingCapacity = 100;

    /** @brief Is this niche currently empty */
    bool isEmpty() const { return currentPopulation == 0; }

    /** @brief Is this niche overcrowded (above carrying capacity) */
    bool isOvercrowded() const { return currentPopulation > estimatedCarryingCapacity; }

    /** @brief Update with new population count */
    void update(int newPopulation, int generation);

    /** @brief Calculate population trend */
    float calculateTrend() const;
};

// =============================================================================
// EMPTY NICHE DETECTION RESULT
// =============================================================================

/**
 * @struct EmptyNicheInfo
 * @brief Information about a detected empty or underutilized niche
 */
struct EmptyNicheInfo {
    /** @brief The empty niche type */
    NicheType nicheType = NicheType::UNDEFINED;

    /** @brief Environmental region where niche is empty */
    glm::vec3 regionCenter = glm::vec3(0.0f);

    /** @brief Size of the region */
    float regionRadius = 50.0f;

    /** @brief Estimated resource availability in empty niche */
    float resourceAvailability = 0.0f;

    /** @brief Estimated fitness potential for colonizers */
    float fitnessPotential = 0.0f;

    /** @brief Nearest occupied niche that could evolve into this one */
    NicheType nearestSourceNiche = NicheType::UNDEFINED;

    /** @brief Species most likely to colonize this niche */
    SpeciesId likelyColonizer = 0;

    /** @brief Generations this niche has been empty */
    int generationsEmpty = 0;

    /** @brief Why is this niche empty */
    std::string reason;
};

// =============================================================================
// NICHE MANAGER CONFIGURATION
// =============================================================================

/**
 * @struct NicheConfig
 * @brief Configuration parameters for the niche management system
 */
struct NicheConfig {
    // -------------------------------------------------------------------------
    // Overlap and competition thresholds
    // -------------------------------------------------------------------------

    /** @brief Overlap threshold triggering competition effects */
    float competitionOverlapThreshold = 0.3f;

    /** @brief Overlap threshold for severe competition */
    float severeCompetitionThreshold = 0.6f;

    /** @brief Minimum overlap to consider niches as competing */
    float minSignificantOverlap = 0.1f;

    // -------------------------------------------------------------------------
    // Niche assignment parameters
    // -------------------------------------------------------------------------

    /** @brief Weight of diet traits in niche assignment */
    float dietWeight = 0.35f;

    /** @brief Weight of behavioral traits in niche assignment */
    float behaviorWeight = 0.30f;

    /** @brief Weight of habitat traits in niche assignment */
    float habitatWeight = 0.20f;

    /** @brief Weight of activity pattern in niche assignment */
    float activityWeight = 0.15f;

    // -------------------------------------------------------------------------
    // Evolution parameters
    // -------------------------------------------------------------------------

    /** @brief Minimum generations before niche shift is recognized */
    int minGenerationsForShift = 10;

    /** @brief Trait change threshold to detect character displacement */
    float displacementThreshold = 0.15f;

    /** @brief Enable automatic niche partitioning */
    bool enablePartitioning = true;

    /** @brief Enable character displacement effects */
    bool enableDisplacement = true;

    // -------------------------------------------------------------------------
    // Fitness modifiers
    // -------------------------------------------------------------------------

    /** @brief Fitness bonus for specialists in optimal conditions */
    float specialistBonus = 0.2f;

    /** @brief Fitness bonus for generalists in variable conditions */
    float generalistBonus = 0.15f;

    /** @brief Fitness penalty per unit of competition overlap */
    float competitionPenalty = 0.3f;

    /** @brief Fitness bonus for colonizing empty niche */
    float emptyNicheBonus = 0.25f;
};

// =============================================================================
// NICHE MANAGER CLASS
// =============================================================================

/**
 * @class NicheManager
 * @brief Manages ecological niche assignment, competition, and evolution
 *
 * The NicheManager is the central class for handling ecological niche
 * dynamics in the simulation. It:
 *
 * - Assigns creatures to appropriate niches based on their genome
 * - Calculates competitive interactions between niches
 * - Detects and tracks evolutionary responses to competition
 * - Identifies ecological opportunities (empty niches)
 * - Evaluates fitness within niche context
 *
 * Usage:
 * @code
 * genetics::NicheManager nicheManager;
 *
 * // Each update cycle:
 * for (Creature* creature : creatures) {
 *     nicheManager.assignNiche(creature);
 * }
 * nicheManager.updateCompetition(creatures, generation);
 *
 * // Query competition effects:
 * float pressure = nicheManager.calculateCompetitionPressure(creature);
 * float fitness = nicheManager.evaluateNicheFitness(creature);
 * @endcode
 */
class NicheManager {
public:
    // =========================================================================
    // CONSTRUCTORS AND INITIALIZATION
    // =========================================================================

    /**
     * @brief Default constructor with default configuration
     */
    NicheManager();

    /**
     * @brief Construct with custom configuration
     * @param config Configuration parameters for niche management
     */
    explicit NicheManager(const NicheConfig& config);

    /**
     * @brief Initialize niche characteristics for all niche types
     */
    void initialize();

    /**
     * @brief Reset all niche tracking data
     */
    void reset();

    // =========================================================================
    // NICHE ASSIGNMENT
    // =========================================================================

    /**
     * @brief Assign a creature to its most appropriate ecological niche
     *
     * Analyzes the creature's genome traits to determine which niche
     * best matches its phenotype. Considers diet preferences, hunting
     * behavior, activity patterns, and habitat requirements.
     *
     * @param creature Pointer to the creature to assign
     * @return The assigned NicheType
     */
    NicheType assignNiche(Creature* creature);

    /**
     * @brief Assign niche based on genome without creature context
     * @param genome The diploid genome to analyze
     * @return The most appropriate NicheType
     */
    NicheType assignNicheFromGenome(const DiploidGenome& genome) const;

    /**
     * @brief Get the current niche assignment for a creature
     * @param creature Pointer to the creature
     * @return Currently assigned NicheType (UNDEFINED if not assigned)
     */
    NicheType getNiche(const Creature* creature) const;

    /**
     * @brief Calculate how well a creature fits each niche type
     * @param creature Pointer to the creature to evaluate
     * @return Map of niche types to fit scores (0-1)
     */
    std::map<NicheType, float> calculateNicheFit(const Creature* creature) const;

    // =========================================================================
    // NICHE OVERLAP AND COMPETITION
    // =========================================================================

    /**
     * @brief Calculate the ecological overlap between two niches
     *
     * Implements Pianka's niche overlap index considering resource use,
     * habitat overlap, and temporal overlap.
     *
     * @param niche1 First niche type
     * @param niche2 Second niche type
     * @return Overlap index (0 = no overlap, 1 = complete overlap)
     */
    float calculateNicheOverlap(NicheType niche1, NicheType niche2) const;

    /**
     * @brief Calculate overlap between two specific niche characteristics
     * @param chars1 First niche characteristics
     * @param chars2 Second niche characteristics
     * @return Overlap index (0-1)
     */
    float calculateCharacteristicsOverlap(const NicheCharacteristics& chars1,
                                          const NicheCharacteristics& chars2) const;

    /**
     * @brief Get competition data between two niches
     * @param niche1 First niche type
     * @param niche2 Second niche type
     * @return Pointer to competition data, nullptr if no competition tracked
     */
    const NicheCompetition* getCompetition(NicheType niche1, NicheType niche2) const;

    /**
     * @brief Update competition tracking for all niche pairs
     * @param creatures All creatures in the simulation
     * @param generation Current generation number
     */
    void updateCompetition(const std::vector<Creature*>& creatures, int generation);

    /**
     * @brief Get all active competition relationships
     * @return Vector of active NicheCompetition records
     */
    std::vector<NicheCompetition> getActiveCompetitions() const;

    // =========================================================================
    // EMPTY NICHE DETECTION
    // =========================================================================

    /**
     * @brief Detect ecological niches that are empty or underutilized
     *
     * Scans the environment for niches that have available resources
     * but no species currently exploiting them. These represent
     * evolutionary opportunities.
     *
     * @param terrain The terrain to analyze for resources
     * @return Vector of empty niche information
     */
    std::vector<EmptyNicheInfo> detectEmptyNiches(const Terrain& terrain) const;

    /**
     * @brief Detect empty niches based on creature distribution only
     * @param creatures All creatures to analyze
     * @return Vector of empty niche information
     */
    std::vector<EmptyNicheInfo> detectEmptyNiches(
        const std::vector<Creature*>& creatures) const;

    /**
     * @brief Check if a specific niche is empty in a region
     * @param nicheType The niche to check
     * @param center Center of the region to check
     * @param radius Radius of the region
     * @return True if niche is empty in that region
     */
    bool isNicheEmpty(NicheType nicheType, const glm::vec3& center,
                      float radius) const;

    // =========================================================================
    // NICHE WIDTH CALCULATION
    // =========================================================================

    /**
     * @brief Calculate the niche width for a creature (specialist vs generalist)
     *
     * Niche width is determined by the range of resources, habitats,
     * and conditions a creature can exploit effectively.
     *
     * @param creature Pointer to the creature to analyze
     * @return Niche width (0 = extreme specialist, 1 = extreme generalist)
     */
    float calculateNicheWidth(const Creature* creature) const;

    /**
     * @brief Calculate niche width from genome
     * @param genome The genome to analyze
     * @return Niche width value (0-1)
     */
    float calculateNicheWidthFromGenome(const DiploidGenome& genome) const;

    /**
     * @brief Determine if creature is specialist or generalist
     * @param creature Pointer to the creature
     * @return True if specialist (niche width < 0.4)
     */
    bool isSpecialist(const Creature* creature) const;

    /**
     * @brief Determine if creature is generalist
     * @param creature Pointer to the creature
     * @return True if generalist (niche width > 0.6)
     */
    bool isGeneralist(const Creature* creature) const;

    // =========================================================================
    // NICHE OCCUPANCY TRACKING
    // =========================================================================

    /**
     * @brief Update occupancy data for all niches
     * @param creatures All creatures in the simulation
     * @param generation Current generation number
     */
    void trackNicheOccupancy(const std::vector<Creature*>& creatures,
                             int generation);

    /**
     * @brief Get occupancy data for a specific niche
     * @param nicheType The niche to query
     * @return Occupancy data for the niche
     */
    const NicheOccupancy& getOccupancy(NicheType nicheType) const;

    /**
     * @brief Get occupancy data for all niches
     * @return Map of niche types to occupancy data
     */
    const std::map<NicheType, NicheOccupancy>& getAllOccupancy() const;

    /**
     * @brief Get list of all creatures in a specific niche
     * @param nicheType The niche to query
     * @param creatures All creatures (to filter)
     * @return Vector of creatures in the specified niche
     */
    std::vector<Creature*> getCreaturesInNiche(
        NicheType nicheType,
        const std::vector<Creature*>& creatures) const;

    // =========================================================================
    // COMPETITION PRESSURE
    // =========================================================================

    /**
     * @brief Calculate competitive pressure on a creature from others in its niche
     *
     * Competition pressure increases with:
     * - Number of competitors in the same niche
     * - Niche overlap with competitors
     * - Resource scarcity in the environment
     *
     * @param creature Pointer to the creature
     * @return Competition pressure (0 = no competition, 1 = extreme competition)
     */
    float calculateCompetitionPressure(const Creature* creature) const;

    /**
     * @brief Calculate intraspecific competition (within same species)
     * @param creature Pointer to the creature
     * @param conspecifics Other creatures of the same species
     * @return Intraspecific competition intensity (0-1)
     */
    float calculateIntraspecificCompetition(
        const Creature* creature,
        const std::vector<Creature*>& conspecifics) const;

    /**
     * @brief Calculate interspecific competition (between different species)
     * @param creature Pointer to the creature
     * @param heterospecifics Creatures of different species in same niche
     * @return Interspecific competition intensity (0-1)
     */
    float calculateInterspecificCompetition(
        const Creature* creature,
        const std::vector<Creature*>& heterospecifics) const;

    // =========================================================================
    // NICHE FITNESS EVALUATION
    // =========================================================================

    /**
     * @brief Evaluate a creature's fitness within its current niche
     *
     * Considers how well the creature's traits match its niche requirements,
     * competition pressure, resource availability, and environmental conditions.
     *
     * @param creature Pointer to the creature
     * @return Niche-based fitness modifier (multiplier, typically 0.5-1.5)
     */
    float evaluateNicheFitness(const Creature* creature) const;

    /**
     * @brief Evaluate potential fitness in a different niche
     * @param creature Pointer to the creature
     * @param targetNiche The niche to evaluate fitness in
     * @return Potential fitness in the target niche
     */
    float evaluatePotentialFitness(const Creature* creature,
                                   NicheType targetNiche) const;

    /**
     * @brief Calculate fitness bonus/penalty from niche specialization
     * @param creature Pointer to the creature
     * @return Specialization modifier (-0.3 to +0.3)
     */
    float calculateSpecializationModifier(const Creature* creature) const;

    // =========================================================================
    // NICHE EVOLUTION TRACKING
    // =========================================================================

    /**
     * @brief Detect and record niche partitioning events
     * @param creatures All creatures to analyze
     * @param generation Current generation
     * @return Vector of newly detected partition events
     */
    std::vector<NichePartition> detectNichePartitioning(
        const std::vector<Creature*>& creatures,
        int generation);

    /**
     * @brief Detect character displacement between competing species
     * @param creatures All creatures to analyze
     * @param generation Current generation
     * @return Vector of displacement events detected
     */
    std::vector<CharacterDisplacement> detectCharacterDisplacement(
        const std::vector<Creature*>& creatures,
        int generation);

    /**
     * @brief Record a niche shift event
     * @param speciesId Species that shifted
     * @param fromNiche Original niche
     * @param toNiche New niche
     * @param generation When the shift occurred
     * @param cause Reason for the shift
     */
    void recordNicheShift(SpeciesId speciesId, NicheType fromNiche,
                          NicheType toNiche, int generation,
                          const std::string& cause);

    /**
     * @brief Get all recorded niche shift events
     * @param sinceGeneration Only return shifts after this generation (0 = all)
     * @return Vector of niche shift records
     */
    std::vector<NicheShift> getNicheShifts(int sinceGeneration = 0) const;

    /**
     * @brief Get all recorded partitioning events
     * @return Vector of niche partition records
     */
    const std::vector<NichePartition>& getPartitionEvents() const;

    /**
     * @brief Get all recorded displacement events
     * @return Vector of character displacement records
     */
    const std::vector<CharacterDisplacement>& getDisplacementEvents() const;

    // =========================================================================
    // NICHE CHARACTERISTICS ACCESS
    // =========================================================================

    /**
     * @brief Get the default characteristics for a niche type
     * @param nicheType The niche to query
     * @return Reference to niche characteristics
     */
    const NicheCharacteristics& getNicheCharacteristics(NicheType nicheType) const;

    /**
     * @brief Set custom characteristics for a niche type
     * @param nicheType The niche to modify
     * @param characteristics New characteristics
     */
    void setNicheCharacteristics(NicheType nicheType,
                                 const NicheCharacteristics& characteristics);

    /**
     * @brief Calculate derived characteristics for a creature
     * @param creature Pointer to the creature
     * @return Creature-specific niche characteristics
     */
    NicheCharacteristics calculateCreatureCharacteristics(
        const Creature* creature) const;

    // =========================================================================
    // CONFIGURATION
    // =========================================================================

    /**
     * @brief Set configuration parameters
     * @param config New configuration
     */
    void setConfig(const NicheConfig& config);

    /**
     * @brief Get current configuration
     * @return Reference to current configuration
     */
    const NicheConfig& getConfig() const { return m_config; }

    // =========================================================================
    // STATISTICS AND REPORTING
    // =========================================================================

    /**
     * @brief Get summary statistics about niche distribution
     * @return String with formatted statistics
     */
    std::string getNicheStatistics() const;

    /**
     * @brief Get total number of occupied niches
     * @return Count of niches with at least one creature
     */
    int getOccupiedNicheCount() const;

    /**
     * @brief Get total number of empty niches
     * @return Count of niches with no creatures
     */
    int getEmptyNicheCount() const;

    /**
     * @brief Get the most crowded niche
     * @return Niche type with highest population
     */
    NicheType getMostCrowdedNiche() const;

    /**
     * @brief Export niche data to CSV format
     * @param filename Path to output file
     */
    void exportToCSV(const std::string& filename) const;

    // =========================================================================
    // UTILITY FUNCTIONS
    // =========================================================================

    /**
     * @brief Convert NicheType to human-readable string
     * @param type The niche type
     * @return String name of the niche
     */
    static std::string nicheTypeToString(NicheType type);

    /**
     * @brief Convert string to NicheType
     * @param name String name of the niche
     * @return Corresponding NicheType (UNDEFINED if not found)
     */
    static NicheType stringToNicheType(const std::string& name);

    /**
     * @brief Convert ResourceType to string
     * @param type The resource type
     * @return String name
     */
    static std::string resourceTypeToString(ResourceType type);

    /**
     * @brief Convert HuntingStrategy to string
     * @param strategy The hunting strategy
     * @return String name
     */
    static std::string huntingStrategyToString(HuntingStrategy strategy);

    /**
     * @brief Convert ActivityPattern to string
     * @param pattern The activity pattern
     * @return String name
     */
    static std::string activityPatternToString(ActivityPattern pattern);

    /**
     * @brief Convert HabitatType to string
     * @param habitat The habitat type
     * @return String name
     */
    static std::string habitatTypeToString(HabitatType habitat);

private:
    // =========================================================================
    // PRIVATE DATA MEMBERS
    // =========================================================================

    /** @brief Configuration parameters */
    NicheConfig m_config;

    /** @brief Default characteristics for each niche type */
    std::map<NicheType, NicheCharacteristics> m_nicheCharacteristics;

    /** @brief Current occupancy data for each niche */
    std::map<NicheType, NicheOccupancy> m_nicheOccupancy;

    /** @brief Creature to niche assignments */
    std::unordered_map<int, NicheType> m_creatureNiches;

    /** @brief Competition tracking between niche pairs */
    std::map<std::pair<NicheType, NicheType>, NicheCompetition> m_competitions;

    /** @brief History of niche shift events */
    std::vector<NicheShift> m_nicheShifts;

    /** @brief History of niche partitioning events */
    std::vector<NichePartition> m_partitionEvents;

    /** @brief History of character displacement events */
    std::vector<CharacterDisplacement> m_displacementEvents;

    /** @brief Previous generation's trait values for displacement detection */
    std::map<SpeciesId, std::map<std::string, float>> m_previousTraits;

    /** @brief Last update generation */
    int m_lastUpdateGeneration = 0;

    /** @brief Is the system initialized */
    bool m_initialized = false;

    // =========================================================================
    // PRIVATE HELPER METHODS
    // =========================================================================

    /**
     * @brief Initialize default niche characteristics
     */
    void initializeNicheCharacteristics();

    /**
     * @brief Create characteristics for grazer niche
     */
    NicheCharacteristics createGrazerCharacteristics() const;

    /**
     * @brief Create characteristics for browser niche
     */
    NicheCharacteristics createBrowserCharacteristics() const;

    /**
     * @brief Create characteristics for frugivore niche
     */
    NicheCharacteristics createFrugivoreCharacteristics() const;

    /**
     * @brief Create characteristics for ambush predator niche
     */
    NicheCharacteristics createAmbushPredatorCharacteristics() const;

    /**
     * @brief Create characteristics for pursuit predator niche
     */
    NicheCharacteristics createPursuitPredatorCharacteristics() const;

    /**
     * @brief Create characteristics for scavenger niche
     */
    NicheCharacteristics createScavengerCharacteristics() const;

    /**
     * @brief Create characteristics for filter feeder niche
     */
    NicheCharacteristics createFilterFeederCharacteristics() const;

    /**
     * @brief Create characteristics for parasite niche
     */
    NicheCharacteristics createParasiteCharacteristics() const;

    /**
     * @brief Create characteristics for symbiont niche
     */
    NicheCharacteristics createSymbiontCharacteristics() const;

    /**
     * @brief Create characteristics for pollinator niche
     */
    NicheCharacteristics createPollinatorCharacteristics() const;

    /**
     * @brief Create characteristics for seed disperser niche
     */
    NicheCharacteristics createSeedDisperserCharacteristics() const;

    /**
     * @brief Calculate resource overlap component
     * @param chars1 First niche characteristics
     * @param chars2 Second niche characteristics
     * @return Resource overlap (0-1)
     */
    float calculateResourceOverlap(const NicheCharacteristics& chars1,
                                   const NicheCharacteristics& chars2) const;

    /**
     * @brief Calculate habitat overlap component
     * @param chars1 First niche characteristics
     * @param chars2 Second niche characteristics
     * @return Habitat overlap (0-1)
     */
    float calculateHabitatOverlap(const NicheCharacteristics& chars1,
                                  const NicheCharacteristics& chars2) const;

    /**
     * @brief Calculate temporal overlap component
     * @param chars1 First niche characteristics
     * @param chars2 Second niche characteristics
     * @return Temporal overlap (0-1)
     */
    float calculateTemporalOverlap(const NicheCharacteristics& chars1,
                                   const NicheCharacteristics& chars2) const;

    /**
     * @brief Extract diet traits from genome for niche assignment
     * @param genome The genome to analyze
     * @return Diet score vector
     */
    std::vector<float> extractDietTraits(const DiploidGenome& genome) const;

    /**
     * @brief Extract behavioral traits from genome for niche assignment
     * @param genome The genome to analyze
     * @return Behavior score vector
     */
    std::vector<float> extractBehaviorTraits(const DiploidGenome& genome) const;

    /**
     * @brief Get ordered niche pair key for map lookups
     * @param n1 First niche
     * @param n2 Second niche
     * @return Ordered pair with smaller value first
     */
    std::pair<NicheType, NicheType> makeNichePair(NicheType n1, NicheType n2) const;

    /**
     * @brief Update trait tracking for displacement detection
     * @param creatures All creatures
     */
    void updateTraitTracking(const std::vector<Creature*>& creatures);
};

// =============================================================================
// INLINE UTILITY IMPLEMENTATIONS
// =============================================================================

/**
 * @brief Default niche occupancy update implementation
 */
inline void NicheOccupancy::update(int newPopulation, int generation) {
    currentPopulation = newPopulation;
    populationHistory.push_back(newPopulation);

    // Limit history size
    while (populationHistory.size() > 100) {
        populationHistory.pop_front();
    }
}

/**
 * @brief Calculate population trend from history
 */
inline float NicheOccupancy::calculateTrend() const {
    if (populationHistory.size() < 10) {
        return 0.0f;
    }

    // Simple linear regression slope
    float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumXX = 0.0f;
    int n = static_cast<int>(populationHistory.size());

    int i = 0;
    for (int pop : populationHistory) {
        sumX += static_cast<float>(i);
        sumY += static_cast<float>(pop);
        sumXY += static_cast<float>(i * pop);
        sumXX += static_cast<float>(i * i);
        i++;
    }

    float denominator = n * sumXX - sumX * sumX;
    if (denominator == 0.0f) return 0.0f;

    return (n * sumXY - sumX * sumY) / denominator;
}

} // namespace genetics
