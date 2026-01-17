#pragma once

/**
 * @file SelectionPressures.h
 * @brief Comprehensive evolutionary selection pressure calculation system
 *
 * This file defines the selection pressure framework for the evolution simulator,
 * providing mechanisms to calculate, track, and apply various evolutionary pressures
 * that drive natural selection. The system models multiple pressure types including
 * predation, competition, climate adaptation, resource scarcity, disease, sexual
 * selection, and migration pressures.
 *
 * Selection pressures are the environmental and biological factors that influence
 * which individuals are more likely to survive and reproduce, thereby shaping the
 * genetic composition of populations over time.
 *
 * @author OrganismEvolution Project
 * @version 1.0
 */

#include "DiploidGenome.h"
#include <vector>
#include <deque>
#include <map>
#include <string>
#include <functional>
#include <cstdint>

// Forward declarations
class Creature;

namespace genetics {

// =============================================================================
// PRESSURE TYPE ENUMERATION
// =============================================================================

/**
 * @enum PressureType
 * @brief Categorizes the different types of evolutionary selection pressures
 *
 * Each pressure type represents a distinct selective force that can influence
 * organism fitness and drive evolutionary change.
 */
enum class PressureType {
    /**
     * @brief Pressure from predator-prey interactions
     *
     * Predation pressure favors traits that help prey avoid predators (speed,
     * camouflage, vigilance) and traits that help predators catch prey (speed,
     * sensory acuity, stealth). This is often one of the strongest selective
     * forces in natural populations.
     */
    PREDATION,

    /**
     * @brief Pressure from resource competition
     *
     * Competition pressure arises from both intraspecific (same species) and
     * interspecific (different species) competition for limited resources.
     * Favors traits that improve competitive ability, niche differentiation,
     * or resource use efficiency.
     */
    COMPETITION,

    /**
     * @brief Pressure from environmental climate conditions
     *
     * Climate pressure results from temperature extremes, seasonal changes,
     * humidity, and other abiotic factors. Favors traits like thermoregulation,
     * cold/heat tolerance, and seasonal adaptation mechanisms.
     */
    CLIMATE,

    /**
     * @brief Pressure from limited food availability
     *
     * Food scarcity pressure drives selection for metabolic efficiency,
     * energy storage capabilities, dietary flexibility, and foraging
     * efficiency. Particularly strong during resource bottlenecks.
     */
    FOOD_SCARCITY,

    /**
     * @brief Pressure from pathogen and disease exposure
     *
     * Disease pressure favors immune system robustness, pathogen resistance,
     * and behaviors that reduce disease transmission. Can drive rapid
     * evolutionary change during epidemics.
     */
    DISEASE,

    /**
     * @brief Pressure from mate choice and mating competition
     *
     * Sexual selection pressure arises from differential mating success.
     * Includes both intersexual selection (mate choice) and intrasexual
     * selection (competition for mates). Drives evolution of ornaments,
     * displays, and mate preference traits.
     */
    SEXUAL_SELECTION,

    /**
     * @brief Pressure from dispersal and movement requirements
     *
     * Migration pressure favors traits that enable successful dispersal
     * and colonization of new habitats. Includes locomotion efficiency,
     * navigation abilities, and stress tolerance during transit.
     */
    MIGRATION,

    /**
     * @brief Total number of pressure types (for iteration)
     */
    COUNT
};

// =============================================================================
// TRAIT DIRECTION STRUCTURE
// =============================================================================

/**
 * @struct TraitDirection
 * @brief Specifies which trait is being selected and in what direction
 *
 * Used to describe how selection pressure affects specific phenotypic traits,
 * indicating whether larger or smaller values are favored.
 */
struct TraitDirection {
    /**
     * @brief The genetic trait being affected by selection
     */
    GeneType trait;

    /**
     * @brief Direction and magnitude of selection
     *
     * Positive values favor increases in the trait (directional selection up),
     * negative values favor decreases (directional selection down),
     * values near zero indicate stabilizing selection toward the optimum.
     * Range: -1.0 (strong decrease) to +1.0 (strong increase)
     */
    float direction;

    /**
     * @brief The optimal value for this trait under current conditions
     *
     * Used for stabilizing selection calculations where deviation from
     * the optimum reduces fitness.
     */
    float optimalValue;

    /**
     * @brief Weight of this trait in overall fitness calculation
     *
     * Higher weights indicate this trait has stronger fitness consequences.
     * Range: 0.0 to 1.0
     */
    float weight;

    /**
     * @brief Default constructor
     */
    TraitDirection()
        : trait(GeneType::SIZE), direction(0.0f), optimalValue(1.0f), weight(0.5f) {}

    /**
     * @brief Parameterized constructor
     *
     * @param t The trait being selected
     * @param dir Direction of selection (-1 to +1)
     * @param opt Optimal trait value
     * @param w Weight in fitness calculation
     */
    TraitDirection(GeneType t, float dir, float opt = 1.0f, float w = 0.5f)
        : trait(t), direction(dir), optimalValue(opt), weight(w) {}
};

// =============================================================================
// SELECTION PRESSURE STRUCTURE
// =============================================================================

/**
 * @struct SelectionPressure
 * @brief Represents a single selection pressure acting on a population
 *
 * Encapsulates all information about a specific selective force, including
 * its type, intensity, which traits it affects, and its source.
 */
struct SelectionPressure {
    /**
     * @brief The category of this selection pressure
     */
    PressureType type;

    /**
     * @brief Strength of the selection pressure
     *
     * Higher intensity means stronger selection effect on fitness.
     * Range: 0.0 (no pressure) to 1.0 (maximum pressure)
     */
    float intensity;

    /**
     * @brief List of traits affected and how they are being selected
     *
     * Multiple traits can be affected by a single pressure, each with
     * its own direction and weight.
     */
    std::vector<TraitDirection> affectedTraits;

    /**
     * @brief Identifier of the pressure source
     *
     * For predation: predator species ID
     * For competition: competitor species ID
     * For climate: climate zone identifier
     * For disease: pathogen strain ID
     * For sexual selection: selecting sex identifier
     */
    uint32_t sourceId;

    /**
     * @brief Human-readable description of the pressure source
     */
    std::string sourceDescription;

    /**
     * @brief Duration this pressure has been active (in generations)
     */
    int durationGenerations;

    /**
     * @brief Whether this pressure is currently active
     */
    bool isActive;

    /**
     * @brief Spatial locality of the pressure
     *
     * 0.0 = global (affects all individuals equally)
     * 1.0 = highly localized (only affects individuals in specific area)
     */
    float spatialLocality;

    /**
     * @brief Default constructor
     */
    SelectionPressure()
        : type(PressureType::PREDATION), intensity(0.0f), sourceId(0),
          durationGenerations(0), isActive(true), spatialLocality(0.0f) {}

    /**
     * @brief Parameterized constructor
     *
     * @param t Pressure type
     * @param inten Intensity (0-1)
     * @param srcId Source identifier
     * @param srcDesc Source description
     */
    SelectionPressure(PressureType t, float inten, uint32_t srcId = 0,
                      const std::string& srcDesc = "")
        : type(t), intensity(inten), sourceId(srcId), sourceDescription(srcDesc),
          durationGenerations(0), isActive(true), spatialLocality(0.0f) {}

    /**
     * @brief Add a trait that is affected by this pressure
     *
     * @param trait The trait being selected
     * @param direction Selection direction (-1 to +1)
     * @param optimal Optimal value for stabilizing selection
     * @param weight Importance weight
     */
    void addAffectedTrait(GeneType trait, float direction,
                          float optimal = 1.0f, float weight = 0.5f) {
        affectedTraits.emplace_back(trait, direction, optimal, weight);
    }
};

// =============================================================================
// SELECTION EVENT STRUCTURE
// =============================================================================

/**
 * @struct TraitChange
 * @brief Records a change in a specific trait due to selection
 */
struct TraitChange {
    GeneType trait;           ///< The trait that changed
    float previousMean;       ///< Population mean before selection
    float newMean;            ///< Population mean after selection
    float previousVariance;   ///< Trait variance before selection
    float newVariance;        ///< Trait variance after selection
    float heritability;       ///< Estimated narrow-sense heritability

    TraitChange()
        : trait(GeneType::SIZE), previousMean(0.0f), newMean(0.0f),
          previousVariance(0.0f), newVariance(0.0f), heritability(0.5f) {}

    /**
     * @brief Calculate the selection differential
     * @return The difference between selected and original population means
     */
    float getSelectionDifferential() const {
        return newMean - previousMean;
    }

    /**
     * @brief Calculate predicted response to selection (breeder's equation)
     * @return R = h^2 * S (response = heritability * selection differential)
     */
    float getPredictedResponse() const {
        return heritability * getSelectionDifferential();
    }
};

/**
 * @struct SelectionEvent
 * @brief Records a significant selection pressure event for historical tracking
 *
 * Selection events capture moments of notable evolutionary change, including
 * the generation, affected species, fitness impacts, and resulting trait changes.
 * This data is essential for understanding evolutionary dynamics over time.
 */
struct SelectionEvent {
    /**
     * @brief Generation number when the event occurred
     */
    int generation;

    /**
     * @brief Type of selection pressure that caused the event
     */
    PressureType pressureType;

    /**
     * @brief Species ID(s) affected by this selection event
     */
    std::vector<SpeciesId> affectedSpecies;

    /**
     * @brief Overall fitness impact on the population
     *
     * Positive values indicate fitness increase (favorable selection),
     * negative values indicate fitness decrease (unfavorable conditions).
     * Magnitude indicates strength of the effect.
     */
    float fitnessImpact;

    /**
     * @brief Mortality rate caused by this selection event
     *
     * Proportion of population that died due to selection (0-1)
     */
    float mortalityRate;

    /**
     * @brief Recorded changes in specific traits
     */
    std::vector<TraitChange> traitChanges;

    /**
     * @brief Intensity of the pressure that caused this event
     */
    float pressureIntensity;

    /**
     * @brief Description of the event
     */
    std::string description;

    /**
     * @brief Whether this event triggered rapid evolutionary change
     */
    bool isAdaptiveResponse;

    /**
     * @brief Population size before the event
     */
    int populationBefore;

    /**
     * @brief Population size after the event
     */
    int populationAfter;

    /**
     * @brief Default constructor
     */
    SelectionEvent()
        : generation(0), pressureType(PressureType::PREDATION),
          fitnessImpact(0.0f), mortalityRate(0.0f), pressureIntensity(0.0f),
          isAdaptiveResponse(false), populationBefore(0), populationAfter(0) {}

    /**
     * @brief Add a trait change record to this event
     *
     * @param trait The affected trait
     * @param prevMean Previous population mean
     * @param newMean New population mean
     * @param prevVar Previous variance
     * @param newVar New variance
     * @param h2 Heritability estimate
     */
    void addTraitChange(GeneType trait, float prevMean, float newMean,
                        float prevVar = 0.0f, float newVar = 0.0f, float h2 = 0.5f) {
        TraitChange change;
        change.trait = trait;
        change.previousMean = prevMean;
        change.newMean = newMean;
        change.previousVariance = prevVar;
        change.newVariance = newVar;
        change.heritability = h2;
        traitChanges.push_back(change);
    }

    /**
     * @brief Calculate the overall selection intensity
     * @return Selection intensity based on mortality and fitness impact
     */
    float getSelectionIntensity() const {
        return pressureIntensity * (1.0f + mortalityRate) * std::abs(fitnessImpact);
    }
};

// =============================================================================
// PRESSURE RESPONSE TRAITS
// =============================================================================

/**
 * @struct StressResponseGenes
 * @brief Genetic factors that influence response to environmental stress
 *
 * These genes modulate how an organism responds to selection pressures,
 * affecting survival probability and adaptation potential.
 */
struct StressResponseGenes {
    /**
     * @brief Heat shock protein expression level
     *
     * Higher values provide better protection against thermal and
     * oxidative stress. Range: 0.0 to 1.0
     */
    float heatShockResponse;

    /**
     * @brief Cortisol/stress hormone regulation efficiency
     *
     * Affects behavioral response to threats and recovery from stress.
     * Range: 0.0 (poor regulation) to 1.0 (excellent regulation)
     */
    float stressHormoneRegulation;

    /**
     * @brief DNA repair mechanism efficiency
     *
     * Higher values provide better protection against mutation and
     * cellular damage. Range: 0.0 to 1.0
     */
    float dnaRepairEfficiency;

    /**
     * @brief Immune system baseline activity
     *
     * Higher values indicate stronger innate immunity but also higher
     * metabolic cost. Range: 0.0 to 1.0
     */
    float immuneBaselineActivity;

    /**
     * @brief Antioxidant production capacity
     *
     * Protects against oxidative stress from metabolic activity and
     * environmental factors. Range: 0.0 to 1.0
     */
    float antioxidantCapacity;

    /**
     * @brief Cellular autophagy efficiency
     *
     * Ability to recycle damaged cellular components during stress.
     * Range: 0.0 to 1.0
     */
    float autophagyEfficiency;

    /**
     * @brief Default constructor with neutral values
     */
    StressResponseGenes()
        : heatShockResponse(0.5f), stressHormoneRegulation(0.5f),
          dnaRepairEfficiency(0.5f), immuneBaselineActivity(0.5f),
          antioxidantCapacity(0.5f), autophagyEfficiency(0.5f) {}

    /**
     * @brief Calculate overall stress resistance
     * @return Weighted average of stress response factors (0-1)
     */
    float getOverallStressResistance() const {
        return (heatShockResponse * 0.2f +
                stressHormoneRegulation * 0.15f +
                dnaRepairEfficiency * 0.2f +
                immuneBaselineActivity * 0.2f +
                antioxidantCapacity * 0.15f +
                autophagyEfficiency * 0.1f);
    }

    /**
     * @brief Calculate metabolic cost of stress response maintenance
     * @return Energy cost multiplier (higher = more costly)
     */
    float getMaintenanceCost() const {
        return 1.0f + (immuneBaselineActivity * 0.1f +
                       antioxidantCapacity * 0.05f +
                       heatShockResponse * 0.05f);
    }
};

/**
 * @struct PlasticityCoefficients
 * @brief Coefficients governing phenotypic plasticity in response to environment
 *
 * Phenotypic plasticity allows organisms to modify their phenotype in response
 * to environmental conditions without genetic change. These coefficients
 * determine the extent and speed of plastic responses.
 */
struct PlasticityCoefficients {
    /**
     * @brief Morphological plasticity coefficient
     *
     * Ability to modify physical traits (size, shape) in response to
     * environment. Range: 0.0 (fixed) to 1.0 (highly plastic)
     */
    float morphological;

    /**
     * @brief Behavioral plasticity coefficient
     *
     * Ability to modify behavior in response to conditions.
     * Range: 0.0 (fixed behavior) to 1.0 (highly flexible)
     */
    float behavioral;

    /**
     * @brief Physiological plasticity coefficient
     *
     * Ability to modify metabolic and physiological processes.
     * Range: 0.0 (fixed) to 1.0 (highly adjustable)
     */
    float physiological;

    /**
     * @brief Developmental plasticity coefficient
     *
     * Extent to which development can be modified by early environment.
     * Range: 0.0 (canalized) to 1.0 (environmentally sensitive)
     */
    float developmental;

    /**
     * @brief Reaction norm slope
     *
     * Rate of phenotypic change per unit environmental change.
     * Higher values mean more responsive plastic changes.
     */
    float reactionNormSlope;

    /**
     * @brief Plasticity cost coefficient
     *
     * Metabolic cost of maintaining plastic capacity.
     * Range: 0.0 (no cost) to 1.0 (high cost)
     */
    float plasticityCost;

    /**
     * @brief Reversibility of plastic changes
     *
     * How quickly plastic changes can be reversed when conditions change.
     * Range: 0.0 (permanent) to 1.0 (instantly reversible)
     */
    float reversibility;

    /**
     * @brief Default constructor
     */
    PlasticityCoefficients()
        : morphological(0.3f), behavioral(0.6f), physiological(0.4f),
          developmental(0.5f), reactionNormSlope(0.5f), plasticityCost(0.2f),
          reversibility(0.5f) {}

    /**
     * @brief Calculate overall plasticity capacity
     * @return Weighted average plasticity (0-1)
     */
    float getOverallPlasticity() const {
        return (morphological * 0.2f + behavioral * 0.3f +
                physiological * 0.25f + developmental * 0.25f);
    }

    /**
     * @brief Calculate the fitness benefit of plasticity under variable conditions
     * @param environmentalVariability How variable the environment is (0-1)
     * @return Net fitness effect of plasticity
     */
    float getPlasticityBenefit(float environmentalVariability) const {
        float benefit = getOverallPlasticity() * environmentalVariability * reactionNormSlope;
        float cost = plasticityCost * getOverallPlasticity();
        return benefit - cost;
    }
};

/**
 * @struct AdaptationSpeed
 * @brief Parameters controlling the rate of evolutionary adaptation
 *
 * These parameters determine how quickly a population can respond to
 * selection pressures through genetic change.
 */
struct AdaptationSpeed {
    /**
     * @brief Mutation rate modifier
     *
     * Multiplier on base mutation rate. Higher values allow faster
     * adaptation but also more deleterious mutations.
     * Range: 0.5 (reduced) to 2.0 (elevated)
     */
    float mutationRateModifier;

    /**
     * @brief Recombination rate modifier
     *
     * Affects the rate of genetic shuffling during meiosis.
     * Higher values increase genetic variation.
     * Range: 0.5 to 2.0
     */
    float recombinationModifier;

    /**
     * @brief Generation time modifier
     *
     * Shorter generation times allow faster evolutionary response.
     * Range: 0.5 (fast generations) to 2.0 (slow generations)
     */
    float generationTimeModifier;

    /**
     * @brief Standing genetic variation
     *
     * Amount of pre-existing genetic variation available for selection.
     * Higher values allow faster initial response to new pressures.
     * Range: 0.0 to 1.0
     */
    float standingVariation;

    /**
     * @brief Epistatic constraint
     *
     * Degree to which gene interactions constrain adaptation.
     * Higher values slow adaptation due to genetic correlations.
     * Range: 0.0 (no constraint) to 1.0 (heavily constrained)
     */
    float epistaticConstraint;

    /**
     * @brief Effective population size factor
     *
     * Larger populations have more variation and respond more
     * predictably to selection. Range: 0.1 to 10.0
     */
    float effectivePopulationFactor;

    /**
     * @brief Default constructor
     */
    AdaptationSpeed()
        : mutationRateModifier(1.0f), recombinationModifier(1.0f),
          generationTimeModifier(1.0f), standingVariation(0.5f),
          epistaticConstraint(0.3f), effectivePopulationFactor(1.0f) {}

    /**
     * @brief Calculate overall adaptation rate
     * @return Relative speed of evolutionary adaptation
     */
    float getAdaptationRate() const {
        float geneticSupply = mutationRateModifier * recombinationModifier * standingVariation;
        float constraint = 1.0f - epistaticConstraint;
        float populationEffect = std::sqrt(effectivePopulationFactor);
        return (geneticSupply * constraint * populationEffect) / generationTimeModifier;
    }

    /**
     * @brief Estimate generations needed to respond to selection
     * @param selectionIntensity Strength of selection (0-1)
     * @return Estimated generations for 50% response
     */
    float estimateResponseTime(float selectionIntensity) const {
        if (selectionIntensity <= 0.0f || getAdaptationRate() <= 0.0f) {
            return 1000.0f; // Very slow/no response
        }
        return 10.0f / (selectionIntensity * getAdaptationRate());
    }
};

/**
 * @struct PressureResponseTraits
 * @brief Combined structure for all pressure response genetic factors
 *
 * Bundles together the stress response genes, plasticity coefficients,
 * and adaptation speed parameters that determine how an organism
 * responds to selection pressures.
 */
struct PressureResponseTraits {
    StressResponseGenes stressGenes;
    PlasticityCoefficients plasticity;
    AdaptationSpeed adaptationSpeed;

    /**
     * @brief Calculate overall resilience to selection pressure
     * @param pressureIntensity How strong the pressure is (0-1)
     * @return Survival probability modifier (0-1)
     */
    float calculateResilience(float pressureIntensity) const {
        float stressResistance = stressGenes.getOverallStressResistance();
        float plasticResponse = plasticity.getOverallPlasticity();

        // Immediate survival depends on stress resistance
        float immediateSurvival = stressResistance * (1.0f - pressureIntensity * 0.5f);

        // Plasticity helps buffer against moderate pressures
        float plasticBuffer = plasticResponse * 0.3f * (1.0f - pressureIntensity);

        return std::min(1.0f, immediateSurvival + plasticBuffer);
    }

    /**
     * @brief Calculate the metabolic cost of maintaining response capacity
     * @return Energy cost multiplier
     */
    float getMaintenanceCost() const {
        return stressGenes.getMaintenanceCost() *
               (1.0f + plasticity.plasticityCost * plasticity.getOverallPlasticity());
    }
};

// =============================================================================
// PRESSURE HISTORY RECORD
// =============================================================================

/**
 * @struct PressureHistoryRecord
 * @brief Historical record of pressure values for tracking trends
 */
struct PressureHistoryRecord {
    int generation;
    PressureType type;
    float intensity;
    float populationFitnessChange;
    int populationSizeChange;

    PressureHistoryRecord()
        : generation(0), type(PressureType::PREDATION), intensity(0.0f),
          populationFitnessChange(0.0f), populationSizeChange(0) {}

    PressureHistoryRecord(int gen, PressureType t, float inten,
                          float fitChange = 0.0f, int popChange = 0)
        : generation(gen), type(t), intensity(inten),
          populationFitnessChange(fitChange), populationSizeChange(popChange) {}
};

// =============================================================================
// SELECTION PRESSURE CALCULATOR CLASS
// =============================================================================

/**
 * @class SelectionPressureCalculator
 * @brief Calculates and manages evolutionary selection pressures
 *
 * This class provides methods to calculate various types of selection pressures
 * acting on creatures, combine multiple pressures into overall fitness effects,
 * apply pressures to modify creature fitness, and track pressure history over
 * evolutionary time.
 *
 * The calculator uses ecological and population data to determine pressure
 * intensities and can log significant selection events for analysis.
 */
class SelectionPressureCalculator {
public:
    // =========================================================================
    // CONSTRUCTION
    // =========================================================================

    /**
     * @brief Default constructor
     */
    SelectionPressureCalculator();

    /**
     * @brief Destructor
     */
    ~SelectionPressureCalculator() = default;

    // =========================================================================
    // INDIVIDUAL PRESSURE CALCULATIONS
    // =========================================================================

    /**
     * @brief Calculate predation pressure on a creature
     *
     * Evaluates the selection pressure from predators based on predator density,
     * prey vulnerability traits, and predator hunting efficiency.
     *
     * @param creature The creature being evaluated
     * @param predatorDensity Number of predators per unit area (0+)
     * @param predatorEfficiency Average hunting success rate of predators (0-1)
     * @return SelectionPressure representing predation effects
     *
     * @note Higher predator density and efficiency increase pressure intensity.
     * Favors traits: speed, camouflage, vigilance, group living
     */
    SelectionPressure calculatePredationPressure(const Creature* creature,
                                                  float predatorDensity,
                                                  float predatorEfficiency = 0.5f) const;

    /**
     * @brief Calculate competition pressure on a creature
     *
     * Evaluates selection pressure from competition with both conspecifics
     * (same species) and heterospecifics (different species).
     *
     * @param creature The creature being evaluated
     * @param competitors List of competing creatures in the local area
     * @param resourceDensity Available resources per capita (0+)
     * @return SelectionPressure representing competition effects
     *
     * @note Pressure increases with competitor density and decreases with resources.
     * Favors traits: competitive ability, niche differentiation, efficiency
     */
    SelectionPressure calculateCompetitionPressure(const Creature* creature,
                                                    const std::vector<Creature*>& competitors,
                                                    float resourceDensity = 1.0f) const;

    /**
     * @brief Calculate climate-related selection pressure
     *
     * Evaluates selection pressure from temperature, seasonal changes,
     * and other abiotic environmental factors.
     *
     * @param creature The creature being evaluated
     * @param temperature Current temperature (normalized 0-1, 0.5 = optimal)
     * @param season Current season (0=winter, 0.25=spring, 0.5=summer, 0.75=autumn)
     * @param humidity Environmental humidity level (0-1)
     * @return SelectionPressure representing climate effects
     *
     * @note Extreme temperatures increase pressure intensity.
     * Favors traits: thermoregulation, seasonal adaptation, appropriate metabolism
     */
    SelectionPressure calculateClimatePressure(const Creature* creature,
                                                float temperature,
                                                float season,
                                                float humidity = 0.5f) const;

    /**
     * @brief Calculate food scarcity selection pressure
     *
     * Evaluates selection pressure from limited food availability.
     *
     * @param creature The creature being evaluated
     * @param foodAvailability Food availability relative to population needs (0-1+)
     * @param foodQuality Nutritional quality of available food (0-1)
     * @return SelectionPressure representing food scarcity effects
     *
     * @note Low food availability strongly increases pressure intensity.
     * Favors traits: efficiency, foraging ability, energy storage, diet breadth
     */
    SelectionPressure calculateFoodPressure(const Creature* creature,
                                             float foodAvailability,
                                             float foodQuality = 0.8f) const;

    /**
     * @brief Calculate disease-related selection pressure
     *
     * Evaluates selection pressure from pathogen exposure and disease.
     *
     * @param creature The creature being evaluated
     * @param diseasePrevalence Proportion of population infected (0-1)
     * @param pathogenVirulence Severity of the disease (0-1)
     * @param transmissionRate Rate of disease spread (0-1)
     * @return SelectionPressure representing disease effects
     *
     * @note Epidemics create strong, rapid selection pressure.
     * Favors traits: immune function, disease resistance, social distancing behaviors
     */
    SelectionPressure calculateDiseasePressure(const Creature* creature,
                                                float diseasePrevalence,
                                                float pathogenVirulence = 0.5f,
                                                float transmissionRate = 0.5f) const;

    /**
     * @brief Calculate sexual selection pressure
     *
     * Evaluates selection pressure from mate choice and mating competition.
     *
     * @param creature The creature being evaluated
     * @param mateAvailability Operational sex ratio and mate encounter rate (0-1)
     * @param populationDensity Local population density for mate finding
     * @param averageChoosinessLevel How selective potential mates are (0-1)
     * @return SelectionPressure representing sexual selection effects
     *
     * @note Low mate availability increases competition pressure.
     * Favors traits: ornaments, displays, competitive ability, mate attraction
     */
    SelectionPressure calculateSexualPressure(const Creature* creature,
                                               float mateAvailability,
                                               float populationDensity = 0.5f,
                                               float averageChoosinessLevel = 0.5f) const;

    /**
     * @brief Calculate migration/dispersal selection pressure
     *
     * Evaluates selection pressure related to movement and colonization.
     *
     * @param creature The creature being evaluated
     * @param dispersalDistance Required/typical dispersal distance
     * @param habitatFragmentation Degree of habitat patchiness (0-1)
     * @param barrierPresence Presence of dispersal barriers (0-1)
     * @return SelectionPressure representing migration effects
     *
     * @note Fragmented habitats increase dispersal pressure.
     * Favors traits: locomotion, navigation, stress tolerance, colonization ability
     */
    SelectionPressure calculateMigrationPressure(const Creature* creature,
                                                  float dispersalDistance,
                                                  float habitatFragmentation = 0.3f,
                                                  float barrierPresence = 0.2f) const;

    // =========================================================================
    // COMBINED PRESSURE CALCULATIONS
    // =========================================================================

    /**
     * @brief Get all active pressures combined for a creature
     *
     * Calculates the weighted sum of all selection pressures acting on a creature,
     * accounting for interactions between different pressure types.
     *
     * @param creature The creature to evaluate
     * @param environmentalData Map of environmental parameters for calculations
     * @param nearbyCreatures Creatures in the local area for competition/predation
     * @return Vector of all active SelectionPressure objects
     *
     * @note Pressures can interact - e.g., food scarcity may increase disease susceptibility
     */
    std::vector<SelectionPressure> getCombinedPressure(
        const Creature* creature,
        const std::map<std::string, float>& environmentalData,
        const std::vector<Creature*>& nearbyCreatures) const;

    /**
     * @brief Calculate total selection intensity from all pressures
     *
     * @param pressures Vector of individual pressures
     * @return Combined intensity value (0-1+, can exceed 1 under extreme conditions)
     */
    float getTotalSelectionIntensity(const std::vector<SelectionPressure>& pressures) const;

    /**
     * @brief Get the dominant (strongest) pressure type
     *
     * @param pressures Vector of pressures to analyze
     * @return The PressureType with highest intensity
     */
    PressureType getDominantPressure(const std::vector<SelectionPressure>& pressures) const;

    // =========================================================================
    // FITNESS MODIFICATION
    // =========================================================================

    /**
     * @brief Apply selection pressures to modify creature fitness
     *
     * Modifies the creature's fitness value based on the selection pressures,
     * taking into account the creature's traits and pressure response capabilities.
     *
     * @param creature The creature whose fitness to modify
     * @param pressures Active selection pressures
     * @param responseTraits The creature's pressure response traits
     * @return Fitness modifier (multiplier on base fitness)
     *
     * @note This method does not directly modify the creature but returns the modifier.
     * Caller is responsible for applying the modification.
     */
    float applyPressureToFitness(const Creature* creature,
                                  const std::vector<SelectionPressure>& pressures,
                                  const PressureResponseTraits& responseTraits) const;

    /**
     * @brief Calculate survival probability under current pressures
     *
     * @param creature The creature to evaluate
     * @param pressures Active selection pressures
     * @param responseTraits The creature's response capabilities
     * @return Probability of survival (0-1)
     */
    float calculateSurvivalProbability(const Creature* creature,
                                        const std::vector<SelectionPressure>& pressures,
                                        const PressureResponseTraits& responseTraits) const;

    /**
     * @brief Calculate reproductive success modifier under pressures
     *
     * @param creature The creature to evaluate
     * @param pressures Active selection pressures
     * @return Multiplier on reproductive output (0-2)
     */
    float calculateReproductiveModifier(const Creature* creature,
                                         const std::vector<SelectionPressure>& pressures) const;

    // =========================================================================
    // HISTORY TRACKING
    // =========================================================================

    /**
     * @brief Record current pressures to history
     *
     * @param generation Current generation number
     * @param pressures Active pressures to record
     * @param populationFitness Average population fitness
     * @param populationSize Current population size
     */
    void trackPressureHistory(int generation,
                              const std::vector<SelectionPressure>& pressures,
                              float populationFitness,
                              int populationSize);

    /**
     * @brief Get pressure history for a specific type
     *
     * @param type The pressure type to query
     * @param lastNGenerations Number of generations to retrieve (0 = all)
     * @return Vector of historical records
     */
    std::vector<PressureHistoryRecord> getPressureHistory(
        PressureType type,
        int lastNGenerations = 0) const;

    /**
     * @brief Get all pressure history
     *
     * @param lastNGenerations Number of generations to retrieve (0 = all)
     * @return Vector of all historical records
     */
    std::vector<PressureHistoryRecord> getAllPressureHistory(int lastNGenerations = 0) const;

    /**
     * @brief Calculate pressure trend over time
     *
     * @param type Pressure type to analyze
     * @param windowSize Number of generations to analyze
     * @return Trend slope (positive = increasing, negative = decreasing)
     */
    float calculatePressureTrend(PressureType type, int windowSize = 50) const;

    /**
     * @brief Clear pressure history
     */
    void clearHistory();

    // =========================================================================
    // EVENT LOGGING
    // =========================================================================

    /**
     * @brief Log a significant selection event
     *
     * @param event The selection event to record
     */
    void logSelectionEvent(const SelectionEvent& event);

    /**
     * @brief Get logged selection events
     *
     * @param sinceGeneration Only return events after this generation (0 = all)
     * @return Vector of selection events
     */
    std::vector<SelectionEvent> getSelectionEvents(int sinceGeneration = 0) const;

    /**
     * @brief Get events of a specific pressure type
     *
     * @param type Pressure type to filter by
     * @param sinceGeneration Generation cutoff
     * @return Filtered vector of events
     */
    std::vector<SelectionEvent> getEventsByType(PressureType type,
                                                 int sinceGeneration = 0) const;

    /**
     * @brief Export events to CSV file
     *
     * @param filename Output file path
     */
    void exportEventsToCSV(const std::string& filename) const;

    // =========================================================================
    // CONFIGURATION
    // =========================================================================

    /**
     * @brief Set weight for a specific pressure type in combined calculations
     *
     * @param type Pressure type to configure
     * @param weight Weight value (0-1, default is equal weighting)
     */
    void setPressureWeight(PressureType type, float weight);

    /**
     * @brief Get current weight for a pressure type
     *
     * @param type Pressure type to query
     * @return Current weight value
     */
    float getPressureWeight(PressureType type) const;

    /**
     * @brief Set the history buffer size
     *
     * @param maxGenerations Maximum generations to keep in history
     */
    void setHistoryBufferSize(int maxGenerations);

    /**
     * @brief Set threshold for logging selection events
     *
     * @param threshold Minimum intensity change to trigger event logging
     */
    void setEventLoggingThreshold(float threshold);

    // =========================================================================
    // UTILITY METHODS
    // =========================================================================

    /**
     * @brief Convert pressure type to string
     *
     * @param type The pressure type
     * @return String name of the pressure type
     */
    static const char* pressureTypeToString(PressureType type);

    /**
     * @brief Get a description of a pressure type
     *
     * @param type The pressure type
     * @return Detailed description string
     */
    static const char* getPressureDescription(PressureType type);

    /**
     * @brief Calculate trait-fitness correlation under pressure
     *
     * @param trait The trait to analyze
     * @param pressure The active pressure
     * @return Correlation coefficient (-1 to +1)
     */
    float calculateTraitFitnessCorrelation(GeneType trait,
                                            const SelectionPressure& pressure) const;

private:
    // =========================================================================
    // PRIVATE MEMBERS
    // =========================================================================

    /// Weights for combining different pressure types
    std::map<PressureType, float> pressureWeights;

    /// Historical records of pressure values
    std::map<PressureType, std::deque<PressureHistoryRecord>> pressureHistory;

    /// Logged selection events
    std::vector<SelectionEvent> selectionEvents;

    /// Maximum history buffer size (generations)
    size_t maxHistorySize;

    /// Threshold for event logging
    float eventLoggingThreshold;

    /// Current generation (for tracking)
    int currentGeneration;

    // =========================================================================
    // PRIVATE HELPER METHODS
    // =========================================================================

    /**
     * @brief Calculate base vulnerability of creature to predation
     */
    float calculatePredationVulnerability(const Creature* creature) const;

    /**
     * @brief Calculate competitive ability of creature
     */
    float calculateCompetitiveAbility(const Creature* creature) const;

    /**
     * @brief Calculate climate tolerance based on creature traits
     */
    float calculateClimateTolerance(const Creature* creature,
                                     float temperature, float humidity) const;

    /**
     * @brief Calculate foraging efficiency of creature
     */
    float calculateForagingEfficiency(const Creature* creature) const;

    /**
     * @brief Calculate disease resistance based on traits
     */
    float calculateDiseaseResistance(const Creature* creature) const;

    /**
     * @brief Calculate mate attractiveness score
     */
    float calculateMateAttractiveness(const Creature* creature) const;

    /**
     * @brief Calculate dispersal ability of creature
     */
    float calculateDispersalAbility(const Creature* creature) const;

    /**
     * @brief Prune old history entries beyond buffer size
     */
    void pruneHistory();

    /**
     * @brief Initialize default pressure weights
     */
    void initializeDefaultWeights();
};

// =============================================================================
// INLINE UTILITY IMPLEMENTATIONS
// =============================================================================

/**
 * @brief Convert pressure type enum to string representation
 */
inline const char* SelectionPressureCalculator::pressureTypeToString(PressureType type) {
    switch (type) {
        case PressureType::PREDATION:        return "Predation";
        case PressureType::COMPETITION:      return "Competition";
        case PressureType::CLIMATE:          return "Climate";
        case PressureType::FOOD_SCARCITY:    return "Food Scarcity";
        case PressureType::DISEASE:          return "Disease";
        case PressureType::SEXUAL_SELECTION: return "Sexual Selection";
        case PressureType::MIGRATION:        return "Migration";
        default:                             return "Unknown";
    }
}

/**
 * @brief Get detailed description of pressure type
 */
inline const char* SelectionPressureCalculator::getPressureDescription(PressureType type) {
    switch (type) {
        case PressureType::PREDATION:
            return "Selection from predator-prey interactions, favoring escape and defense traits";
        case PressureType::COMPETITION:
            return "Selection from resource competition, favoring efficiency and niche differentiation";
        case PressureType::CLIMATE:
            return "Selection from environmental conditions, favoring thermoregulation and adaptation";
        case PressureType::FOOD_SCARCITY:
            return "Selection from limited resources, favoring metabolic efficiency and foraging";
        case PressureType::DISEASE:
            return "Selection from pathogen exposure, favoring immune function and resistance";
        case PressureType::SEXUAL_SELECTION:
            return "Selection from mate choice, favoring ornaments and competitive ability";
        case PressureType::MIGRATION:
            return "Selection from dispersal requirements, favoring locomotion and navigation";
        default:
            return "Unknown pressure type";
    }
}

} // namespace genetics
