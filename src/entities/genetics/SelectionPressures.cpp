/**
 * @file SelectionPressures.cpp
 * @brief Implementation of evolutionary selection pressure calculations
 *
 * This file implements the SelectionPressureCalculator class which models
 * various evolutionary pressures including predation, competition, climate,
 * food scarcity, disease, sexual selection, and migration pressures.
 *
 * @author OrganismEvolution Project
 * @version 1.0
 */

#include "SelectionPressures.h"
#include "../Creature.h"
#include "../../utils/Random.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <fstream>

namespace genetics {

// =============================================================================
// CONSTRUCTION
// =============================================================================

SelectionPressureCalculator::SelectionPressureCalculator()
    : maxHistorySize(1000),
      eventLoggingThreshold(0.1f),
      currentGeneration(0) {
    initializeDefaultWeights();
}

void SelectionPressureCalculator::initializeDefaultWeights() {
    // Initialize default weights for each pressure type
    // Weights reflect typical importance in natural populations
    pressureWeights[PressureType::PREDATION] = 0.25f;
    pressureWeights[PressureType::COMPETITION] = 0.20f;
    pressureWeights[PressureType::CLIMATE] = 0.15f;
    pressureWeights[PressureType::FOOD_SCARCITY] = 0.15f;
    pressureWeights[PressureType::DISEASE] = 0.10f;
    pressureWeights[PressureType::SEXUAL_SELECTION] = 0.10f;
    pressureWeights[PressureType::MIGRATION] = 0.05f;
}

// =============================================================================
// PRIVATE HELPER METHODS
// =============================================================================

float SelectionPressureCalculator::calculatePredationVulnerability(const Creature* creature) const {
    if (!creature) return 1.0f;

    const DiploidGenome& genome = creature->getDiploidGenome();
    Phenotype phenotype = genome.express();

    // Base vulnerability starts at 1.0 (fully vulnerable)
    float vulnerability = 1.0f;

    // Speed reduces vulnerability (faster = harder to catch)
    // Speed typically ranges 8-20, normalize to 0-1 contribution
    float speedFactor = std::clamp(phenotype.speed / 20.0f, 0.0f, 1.0f);
    vulnerability -= speedFactor * 0.25f;

    // Size affects vulnerability - very small or very large are less vulnerable
    // Medium-sized creatures are most vulnerable (predator sweet spot)
    float sizeFactor = phenotype.size;
    if (sizeFactor < 0.5f) {
        vulnerability -= (0.5f - sizeFactor) * 0.2f;  // Small = harder to see
    } else if (sizeFactor > 1.5f) {
        vulnerability -= (sizeFactor - 1.5f) * 0.15f;  // Large = harder to take down
    }

    // Camouflage significantly reduces visibility to predators
    vulnerability -= phenotype.camouflageLevel * 0.3f;

    // Vision range helps detect predators early
    float visionFactor = std::clamp(phenotype.visionRange / 60.0f, 0.0f, 1.0f);
    vulnerability -= visionFactor * 0.1f;

    // Fear response enables quicker escape
    vulnerability -= phenotype.fearResponse * 0.1f;

    // Sociality provides safety in numbers
    vulnerability -= phenotype.sociality * 0.15f;

    // Motion detection helps spot approaching predators
    vulnerability -= phenotype.motionDetection * 0.1f;

    return std::clamp(vulnerability, 0.1f, 1.0f);
}

float SelectionPressureCalculator::calculateCompetitiveAbility(const Creature* creature) const {
    if (!creature) return 0.5f;

    const DiploidGenome& genome = creature->getDiploidGenome();
    Phenotype phenotype = genome.express();

    float competitiveAbility = 0.0f;

    // Size is a major factor in competition
    competitiveAbility += phenotype.size * 0.25f;

    // Aggression helps in direct competition
    competitiveAbility += phenotype.aggression * 0.2f;

    // Efficiency allows getting more from less resources
    competitiveAbility += phenotype.efficiency * 0.2f;

    // Speed helps reach resources first
    float speedFactor = std::clamp(phenotype.speed / 20.0f, 0.0f, 1.0f);
    competitiveAbility += speedFactor * 0.15f;

    // Vision helps find resources
    float visionFactor = std::clamp(phenotype.visionRange / 60.0f, 0.0f, 1.0f);
    competitiveAbility += visionFactor * 0.1f;

    // Smell range for food detection
    float smellFactor = std::clamp(phenotype.smellRange / 150.0f, 0.0f, 1.0f);
    competitiveAbility += smellFactor * 0.1f;

    return std::clamp(competitiveAbility, 0.0f, 1.0f);
}

float SelectionPressureCalculator::calculateClimateTolerance(const Creature* creature,
                                                              float temperature,
                                                              float humidity) const {
    if (!creature) return 0.5f;

    const DiploidGenome& genome = creature->getDiploidGenome();
    Phenotype phenotype = genome.express();

    // Temperature 0.5 is optimal, deviations cause stress
    float tempDeviation = std::abs(temperature - 0.5f);

    // Calculate tolerance based on direction of deviation
    float tolerance = 1.0f;

    if (temperature > 0.5f) {
        // Hot conditions - need heat tolerance
        float heatStress = (temperature - 0.5f) * 2.0f;  // Scale to 0-1
        tolerance -= heatStress * (1.0f - phenotype.heatTolerance);
    } else {
        // Cold conditions - need cold tolerance
        float coldStress = (0.5f - temperature) * 2.0f;  // Scale to 0-1
        tolerance -= coldStress * (1.0f - phenotype.coldTolerance);
    }

    // Humidity affects tolerance (extreme humidity is stressful)
    float humidityDeviation = std::abs(humidity - 0.5f);
    tolerance -= humidityDeviation * 0.2f;

    // Metabolic rate affects temperature regulation
    // High metabolic rate helps in cold, hurts in heat
    if (temperature > 0.5f) {
        tolerance -= (phenotype.metabolicRate - 1.0f) * 0.1f;
    } else {
        tolerance += (phenotype.metabolicRate - 1.0f) * 0.1f;
    }

    // Size affects thermoregulation (larger = better heat retention)
    if (temperature < 0.5f) {
        tolerance += (phenotype.size - 1.0f) * 0.1f;
    } else {
        tolerance -= (phenotype.size - 1.0f) * 0.05f;
    }

    return std::clamp(tolerance, 0.0f, 1.0f);
}

float SelectionPressureCalculator::calculateForagingEfficiency(const Creature* creature) const {
    if (!creature) return 0.5f;

    const DiploidGenome& genome = creature->getDiploidGenome();
    Phenotype phenotype = genome.express();

    float efficiency = 0.0f;

    // Metabolic efficiency is primary factor
    efficiency += phenotype.efficiency * 0.3f;

    // Vision helps find food
    float visionFactor = std::clamp(phenotype.visionRange / 60.0f, 0.0f, 1.0f);
    efficiency += visionFactor * 0.15f;

    // Smell range for food detection
    float smellFactor = std::clamp(phenotype.smellRange / 150.0f, 0.0f, 1.0f);
    efficiency += smellFactor * 0.2f;

    // Speed to reach food quickly
    float speedFactor = std::clamp(phenotype.speed / 20.0f, 0.0f, 1.0f);
    efficiency += speedFactor * 0.15f;

    // Memory helps remember food locations
    efficiency += phenotype.memoryCapacity * 0.1f;
    efficiency += phenotype.memoryRetention * 0.05f;

    // Diet specialization can improve or reduce efficiency
    // Specialists are more efficient in their niche but struggle elsewhere
    float specialization = phenotype.dietSpecialization;
    efficiency += specialization * 0.05f;  // Slight bonus for specialization

    return std::clamp(efficiency, 0.0f, 1.0f);
}

float SelectionPressureCalculator::calculateDiseaseResistance(const Creature* creature) const {
    if (!creature) return 0.5f;

    const DiploidGenome& genome = creature->getDiploidGenome();
    Phenotype phenotype = genome.express();

    float resistance = 0.0f;

    // Size provides some baseline resistance (larger = more robust)
    resistance += std::min(phenotype.size, 1.5f) * 0.15f;

    // Heterozygosity provides disease resistance (MHC diversity)
    float heterozygosity = genome.getHeterozygosity();
    resistance += heterozygosity * 0.3f;

    // Genetic load reduces resistance
    float geneticLoad = genome.getGeneticLoad();
    resistance -= geneticLoad * 0.2f;

    // Low sociality reduces disease transmission
    resistance += (1.0f - phenotype.sociality) * 0.15f;

    // Metabolic rate affects immune system strength
    resistance += phenotype.metabolicRate * 0.1f;

    // Fear response can lead to social distancing behavior
    resistance += phenotype.fearResponse * 0.05f;

    // Base immunity level
    resistance += 0.3f;

    return std::clamp(resistance, 0.0f, 1.0f);
}

float SelectionPressureCalculator::calculateMateAttractiveness(const Creature* creature) const {
    if (!creature) return 0.5f;

    const DiploidGenome& genome = creature->getDiploidGenome();
    Phenotype phenotype = genome.express();

    float attractiveness = 0.0f;

    // Ornament intensity is primary sexual selection trait
    attractiveness += phenotype.ornamentIntensity * 0.35f;

    // Display frequency shows vigor
    attractiveness += phenotype.displayFrequency * 0.2f;

    // Size often indicates fitness
    attractiveness += std::min(phenotype.size, 1.5f) * 0.15f;

    // Color vibrancy (higher saturation = more attractive)
    glm::vec3 color = phenotype.color;
    float saturation = std::max({color.r, color.g, color.b}) -
                       std::min({color.r, color.g, color.b});
    attractiveness += saturation * 0.1f;

    // Energy level (proxy for condition)
    float energyFactor = creature->getEnergy() / 200.0f;
    attractiveness += energyFactor * 0.1f;

    // Low genetic load indicates good genes
    float geneticLoad = genome.getGeneticLoad();
    attractiveness -= geneticLoad * 0.1f;

    // High heterozygosity is attractive (diverse immune genes)
    float heterozygosity = genome.getHeterozygosity();
    attractiveness += heterozygosity * 0.1f;

    return std::clamp(attractiveness, 0.0f, 1.0f);
}

float SelectionPressureCalculator::calculateDispersalAbility(const Creature* creature) const {
    if (!creature) return 0.5f;

    const DiploidGenome& genome = creature->getDiploidGenome();
    Phenotype phenotype = genome.express();

    float ability = 0.0f;

    // Speed is primary dispersal trait
    float speedFactor = std::clamp(phenotype.speed / 20.0f, 0.0f, 1.0f);
    ability += speedFactor * 0.3f;

    // Efficiency helps maintain energy during travel
    ability += phenotype.efficiency * 0.2f;

    // Navigation aids
    ability += phenotype.memoryCapacity * 0.15f;
    ability += phenotype.memoryRetention * 0.1f;

    // Sensory abilities help find suitable habitat
    float visionFactor = std::clamp(phenotype.visionRange / 60.0f, 0.0f, 1.0f);
    ability += visionFactor * 0.1f;

    float smellFactor = std::clamp(phenotype.smellRange / 150.0f, 0.0f, 1.0f);
    ability += smellFactor * 0.1f;

    // Creature type affects dispersal
    CreatureType type = creature->getType();
    if (isFlying(type)) {
        ability += 0.15f;  // Flying creatures disperse easily
    } else if (isAquatic(type)) {
        ability += 0.05f;  // Aquatic can follow water bodies
    }

    // Curiosity drives exploration
    ability += phenotype.curiosity * 0.05f;

    return std::clamp(ability, 0.0f, 1.0f);
}

void SelectionPressureCalculator::pruneHistory() {
    for (auto& [type, history] : pressureHistory) {
        while (history.size() > maxHistorySize) {
            history.pop_front();
        }
    }
}

// =============================================================================
// INDIVIDUAL PRESSURE CALCULATIONS
// =============================================================================

SelectionPressure SelectionPressureCalculator::calculatePredationPressure(
    const Creature* creature,
    float predatorDensity,
    float predatorEfficiency) const {

    SelectionPressure pressure(PressureType::PREDATION, 0.0f);

    if (!creature) return pressure;

    // Calculate base intensity from predator presence
    // Predator density typically 0-1 where 1 is very high
    float densityFactor = std::clamp(predatorDensity, 0.0f, 2.0f);

    // Creature's vulnerability to predation
    float vulnerability = calculatePredationVulnerability(creature);

    // Calculate intensity: density * efficiency * vulnerability
    // Formula: I = D^0.5 * E * V (square root dampens density effect)
    pressure.intensity = std::sqrt(densityFactor) * predatorEfficiency * vulnerability;
    pressure.intensity = std::clamp(pressure.intensity, 0.0f, 1.0f);

    pressure.sourceDescription = "Predation from local predator population";

    // Define affected traits with selection directions
    // High predation favors:
    // - Higher speed (positive direction)
    pressure.addAffectedTrait(GeneType::SPEED, 0.8f, 1.2f, 0.3f);

    // - Better camouflage
    pressure.addAffectedTrait(GeneType::CAMOUFLAGE_LEVEL, 0.7f, 0.8f, 0.25f);

    // - Higher fear response (more vigilant)
    pressure.addAffectedTrait(GeneType::FEAR_RESPONSE, 0.6f, 0.7f, 0.15f);

    // - Better motion detection
    pressure.addAffectedTrait(GeneType::MOTION_DETECTION, 0.5f, 0.8f, 0.1f);

    // - Higher sociality (safety in numbers)
    pressure.addAffectedTrait(GeneType::SOCIALITY, 0.4f, 0.7f, 0.1f);

    // - Larger vision range
    pressure.addAffectedTrait(GeneType::VISION_RANGE, 0.3f, 45.0f, 0.1f);

    return pressure;
}

SelectionPressure SelectionPressureCalculator::calculateCompetitionPressure(
    const Creature* creature,
    const std::vector<Creature*>& competitors,
    float resourceDensity) const {

    SelectionPressure pressure(PressureType::COMPETITION, 0.0f);

    if (!creature) return pressure;

    const DiploidGenome& genome = creature->getDiploidGenome();
    EcologicalNiche creatureNiche = genome.getEcologicalNiche();

    // Calculate population density effect
    float populationDensity = static_cast<float>(competitors.size()) / 100.0f;
    populationDensity = std::clamp(populationDensity, 0.0f, 2.0f);

    // Calculate niche overlap with competitors
    float totalNicheOverlap = 0.0f;
    int sameSpeciesCount = 0;
    int differentSpeciesCount = 0;

    for (const Creature* competitor : competitors) {
        if (!competitor || competitor == creature) continue;

        const DiploidGenome& compGenome = competitor->getDiploidGenome();
        EcologicalNiche compNiche = compGenome.getEcologicalNiche();

        // Niche distance (0 = identical, 1 = completely different)
        float nicheDistance = creatureNiche.distanceTo(compNiche);
        float overlap = 1.0f - nicheDistance;

        totalNicheOverlap += overlap;

        // Track intra vs interspecific competition
        if (genome.getSpeciesId() == compGenome.getSpeciesId()) {
            sameSpeciesCount++;
        } else {
            differentSpeciesCount++;
        }
    }

    // Average niche overlap
    float avgOverlap = competitors.empty() ? 0.0f :
                       totalNicheOverlap / static_cast<float>(competitors.size());

    // Intraspecific competition is typically stronger
    float intraspecificWeight = static_cast<float>(sameSpeciesCount) /
                                std::max(1.0f, static_cast<float>(competitors.size()));

    // Competition intensity formula:
    // I = (D / R) * overlap * (1 + intra_weight)
    // where D = density, R = resources
    float resourceFactor = std::max(0.1f, resourceDensity);
    pressure.intensity = (populationDensity / resourceFactor) * avgOverlap *
                         (1.0f + intraspecificWeight * 0.5f);
    pressure.intensity = std::clamp(pressure.intensity, 0.0f, 1.0f);

    pressure.sourceDescription = "Competition for resources with " +
                                  std::to_string(competitors.size()) + " neighbors";

    // Competition favors:
    // - Higher efficiency (get more from resources)
    pressure.addAffectedTrait(GeneType::EFFICIENCY, 0.7f, 1.3f, 0.25f);

    // - Niche differentiation (different diet)
    pressure.addAffectedTrait(GeneType::DIET_SPECIALIZATION, 0.5f, 0.5f, 0.2f);

    // - Larger size (competitive dominance)
    pressure.addAffectedTrait(GeneType::SIZE, 0.4f, 1.2f, 0.15f);

    // - Higher aggression
    pressure.addAffectedTrait(GeneType::AGGRESSION, 0.3f, 0.6f, 0.1f);

    // - Better foraging senses
    pressure.addAffectedTrait(GeneType::SMELL_RANGE, 0.3f, 100.0f, 0.1f);
    pressure.addAffectedTrait(GeneType::VISION_RANGE, 0.2f, 40.0f, 0.1f);

    // - Different activity time (temporal niche partitioning)
    pressure.addAffectedTrait(GeneType::ACTIVITY_TIME, 0.2f, 0.5f, 0.1f);

    return pressure;
}

SelectionPressure SelectionPressureCalculator::calculateClimatePressure(
    const Creature* creature,
    float temperature,
    float season,
    float humidity) const {

    SelectionPressure pressure(PressureType::CLIMATE, 0.0f);

    if (!creature) return pressure;

    // Calculate creature's tolerance to current conditions
    float tolerance = calculateClimateTolerance(creature, temperature, humidity);

    // Seasonal variation adds extra pressure
    // Season: 0=winter, 0.25=spring, 0.5=summer, 0.75=autumn
    float seasonalSeverity = 0.0f;
    if (season < 0.125f || season > 0.875f) {
        // Winter - harshest
        seasonalSeverity = 0.3f;
    } else if (season > 0.375f && season < 0.625f) {
        // Summer - can be harsh if hot
        seasonalSeverity = 0.15f;
    }

    // Temperature deviation from optimal (0.5)
    float tempDeviation = std::abs(temperature - 0.5f) * 2.0f;

    // Climate pressure intensity formula:
    // I = (1 - tolerance) * tempDeviation * (1 + seasonalSeverity)
    pressure.intensity = (1.0f - tolerance) * tempDeviation * (1.0f + seasonalSeverity);
    pressure.intensity = std::clamp(pressure.intensity, 0.0f, 1.0f);

    // Determine if it's heat or cold stress
    bool isHeatStress = temperature > 0.5f;

    if (isHeatStress) {
        pressure.sourceDescription = "Heat stress from high temperatures";

        // Heat favors:
        // - Higher heat tolerance
        pressure.addAffectedTrait(GeneType::HEAT_TOLERANCE, 0.9f, 0.8f, 0.35f);

        // - Lower metabolic rate (less heat generation)
        pressure.addAffectedTrait(GeneType::METABOLIC_RATE, -0.3f, 0.8f, 0.2f);

        // - Smaller size (better heat dissipation)
        pressure.addAffectedTrait(GeneType::SIZE, -0.2f, 0.8f, 0.15f);

        // - Crepuscular activity (avoid midday heat)
        pressure.addAffectedTrait(GeneType::ACTIVITY_TIME, -0.2f, 0.3f, 0.1f);
    } else {
        pressure.sourceDescription = "Cold stress from low temperatures";

        // Cold favors:
        // - Higher cold tolerance
        pressure.addAffectedTrait(GeneType::COLD_TOLERANCE, 0.9f, 0.8f, 0.35f);

        // - Higher metabolic rate (more heat generation)
        pressure.addAffectedTrait(GeneType::METABOLIC_RATE, 0.4f, 1.2f, 0.2f);

        // - Larger size (better heat retention - Bergmann's rule)
        pressure.addAffectedTrait(GeneType::SIZE, 0.3f, 1.3f, 0.15f);

        // - Diurnal activity (warm part of day)
        pressure.addAffectedTrait(GeneType::ACTIVITY_TIME, 0.3f, 0.7f, 0.1f);

        // - Higher sociality (huddling for warmth)
        pressure.addAffectedTrait(GeneType::SOCIALITY, 0.2f, 0.7f, 0.1f);
    }

    return pressure;
}

SelectionPressure SelectionPressureCalculator::calculateFoodPressure(
    const Creature* creature,
    float foodAvailability,
    float foodQuality) const {

    SelectionPressure pressure(PressureType::FOOD_SCARCITY, 0.0f);

    if (!creature) return pressure;

    // Calculate foraging efficiency
    float foragingEfficiency = calculateForagingEfficiency(creature);

    // Food scarcity is inverse of availability
    float scarcity = 1.0f - std::clamp(foodAvailability, 0.0f, 1.0f);

    // Poor quality food requires eating more
    float qualityPenalty = (1.0f - foodQuality) * 0.3f;

    // Food pressure intensity formula:
    // I = scarcity * (1 - efficiency) * (1 + qualityPenalty)
    pressure.intensity = scarcity * (1.0f - foragingEfficiency * 0.7f) *
                         (1.0f + qualityPenalty);
    pressure.intensity = std::clamp(pressure.intensity, 0.0f, 1.0f);

    pressure.sourceDescription = "Food scarcity (availability: " +
                                  std::to_string(static_cast<int>(foodAvailability * 100)) + "%)";

    // Food scarcity favors:
    // - Higher metabolic efficiency
    pressure.addAffectedTrait(GeneType::EFFICIENCY, 0.8f, 1.4f, 0.3f);

    // - Lower metabolic rate (reduced energy needs)
    pressure.addAffectedTrait(GeneType::METABOLIC_RATE, -0.5f, 0.7f, 0.2f);

    // - Better foraging senses
    pressure.addAffectedTrait(GeneType::SMELL_RANGE, 0.6f, 120.0f, 0.15f);
    pressure.addAffectedTrait(GeneType::SMELL_SENSITIVITY, 0.5f, 0.8f, 0.1f);
    pressure.addAffectedTrait(GeneType::VISION_RANGE, 0.4f, 45.0f, 0.1f);

    // - Diet generalization (can eat more things)
    pressure.addAffectedTrait(GeneType::DIET_SPECIALIZATION, -0.4f, 0.3f, 0.1f);

    // - Smaller size (reduced energy requirements)
    pressure.addAffectedTrait(GeneType::SIZE, -0.3f, 0.7f, 0.1f);

    // - Better memory (remember food locations)
    pressure.addAffectedTrait(GeneType::MEMORY_CAPACITY, 0.3f, 0.8f, 0.05f);

    return pressure;
}

SelectionPressure SelectionPressureCalculator::calculateDiseasePressure(
    const Creature* creature,
    float diseasePrevalence,
    float pathogenVirulence,
    float transmissionRate) const {

    SelectionPressure pressure(PressureType::DISEASE, 0.0f);

    if (!creature) return pressure;

    // Calculate creature's disease resistance
    float resistance = calculateDiseaseResistance(creature);

    // Disease pressure depends on prevalence, virulence, and transmission
    // R0-like calculation: transmission * duration (implied by virulence inverse)
    float spreadPotential = transmissionRate * (1.0f - pathogenVirulence * 0.5f);

    // Epidemic threshold effect - pressure increases rapidly above threshold
    float epidemicEffect = diseasePrevalence > 0.1f ?
                           std::pow(diseasePrevalence, 0.7f) : diseasePrevalence;

    // Disease pressure intensity formula:
    // I = prevalence_effect * (1 - resistance) * virulence * spreadPotential
    pressure.intensity = epidemicEffect * (1.0f - resistance * 0.8f) *
                         pathogenVirulence * spreadPotential;
    pressure.intensity = std::clamp(pressure.intensity, 0.0f, 1.0f);

    pressure.sourceDescription = "Disease outbreak (prevalence: " +
                                  std::to_string(static_cast<int>(diseasePrevalence * 100)) +
                                  "%, virulence: " +
                                  std::to_string(static_cast<int>(pathogenVirulence * 100)) + "%)";

    // Disease favors:
    // - Reduced sociality (social distancing)
    pressure.addAffectedTrait(GeneType::SOCIALITY, -0.6f, 0.3f, 0.25f);

    // - Higher fear response (avoidance behavior)
    pressure.addAffectedTrait(GeneType::FEAR_RESPONSE, 0.4f, 0.7f, 0.15f);

    // - Genetic diversity (heterozygosity advantage)
    // This is handled indirectly through genome-level effects

    // - Higher metabolic rate (stronger immune response)
    pressure.addAffectedTrait(GeneType::METABOLIC_RATE, 0.3f, 1.1f, 0.15f);

    // - Lower population density preferred (spacing)
    pressure.addAffectedTrait(GeneType::AGGRESSION, 0.2f, 0.5f, 0.1f);

    // - Better smell to detect sick individuals
    pressure.addAffectedTrait(GeneType::SMELL_SENSITIVITY, 0.3f, 0.7f, 0.1f);

    return pressure;
}

SelectionPressure SelectionPressureCalculator::calculateSexualPressure(
    const Creature* creature,
    float mateAvailability,
    float populationDensity,
    float averageChoosinessLevel) const {

    SelectionPressure pressure(PressureType::SEXUAL_SELECTION, 0.0f);

    if (!creature) return pressure;

    // Calculate creature's attractiveness
    float attractiveness = calculateMateAttractiveness(creature);

    // Sexual selection intensity depends on:
    // 1. Mate scarcity (more competition when mates are limited)
    float mateScarcity = 1.0f - std::clamp(mateAvailability, 0.0f, 1.0f);

    // 2. Population density (more encounters = more selection)
    float densityFactor = std::clamp(populationDensity, 0.1f, 1.0f);

    // 3. Choosiness (higher choosiness = stronger selection)
    float choosinessFactor = std::clamp(averageChoosinessLevel, 0.0f, 1.0f);

    // Sexual selection intensity formula:
    // I = (1 - attractiveness) * scarcity * density * choosiness
    pressure.intensity = (1.0f - attractiveness * 0.7f) * mateScarcity *
                         densityFactor * (0.5f + choosinessFactor * 0.5f);
    pressure.intensity = std::clamp(pressure.intensity, 0.0f, 1.0f);

    pressure.sourceDescription = "Sexual selection (mate availability: " +
                                  std::to_string(static_cast<int>(mateAvailability * 100)) +
                                  "%, choosiness: " +
                                  std::to_string(static_cast<int>(averageChoosinessLevel * 100)) + "%)";

    // Sexual selection favors:
    // - Higher ornament intensity (primary sexual trait)
    pressure.addAffectedTrait(GeneType::ORNAMENT_INTENSITY, 0.9f, 0.8f, 0.3f);

    // - Higher display frequency
    pressure.addAffectedTrait(GeneType::DISPLAY_FREQUENCY, 0.7f, 0.7f, 0.2f);

    // - Larger size (often preferred)
    pressure.addAffectedTrait(GeneType::SIZE, 0.4f, 1.2f, 0.15f);

    // - Brighter coloration
    pressure.addAffectedTrait(GeneType::COLOR_RED, 0.3f, 0.7f, 0.1f);
    pressure.addAffectedTrait(GeneType::COLOR_GREEN, 0.2f, 0.5f, 0.05f);
    pressure.addAffectedTrait(GeneType::COLOR_BLUE, 0.2f, 0.5f, 0.05f);

    // - Higher aggression (intrasexual competition)
    pressure.addAffectedTrait(GeneType::AGGRESSION, 0.3f, 0.5f, 0.1f);

    // - Pheromone production
    pressure.addAffectedTrait(GeneType::PHEROMONE_PRODUCTION, 0.4f, 0.6f, 0.1f);

    return pressure;
}

SelectionPressure SelectionPressureCalculator::calculateMigrationPressure(
    const Creature* creature,
    float dispersalDistance,
    float habitatFragmentation,
    float barrierPresence) const {

    SelectionPressure pressure(PressureType::MIGRATION, 0.0f);

    if (!creature) return pressure;

    // Calculate creature's dispersal ability
    float dispersalAbility = calculateDispersalAbility(creature);

    // Migration pressure increases with:
    // 1. Required dispersal distance
    float distanceFactor = std::clamp(dispersalDistance / 100.0f, 0.0f, 1.0f);

    // 2. Habitat fragmentation (forces longer dispersal)
    float fragmentationFactor = std::clamp(habitatFragmentation, 0.0f, 1.0f);

    // 3. Barriers that must be crossed
    float barrierFactor = std::clamp(barrierPresence, 0.0f, 1.0f);

    // Migration pressure intensity formula:
    // I = (1 - ability) * distance * (fragmentation + barriers)
    pressure.intensity = (1.0f - dispersalAbility * 0.6f) * distanceFactor *
                         (fragmentationFactor + barrierFactor * 0.5f);
    pressure.intensity = std::clamp(pressure.intensity, 0.0f, 1.0f);

    pressure.sourceDescription = "Migration/dispersal pressure (fragmentation: " +
                                  std::to_string(static_cast<int>(habitatFragmentation * 100)) + "%)";

    // Migration favors:
    // - Higher speed (faster travel)
    pressure.addAffectedTrait(GeneType::SPEED, 0.7f, 18.0f, 0.25f);

    // - Better efficiency (sustained travel)
    pressure.addAffectedTrait(GeneType::EFFICIENCY, 0.6f, 1.3f, 0.2f);

    // - Navigation abilities
    pressure.addAffectedTrait(GeneType::MEMORY_CAPACITY, 0.5f, 0.8f, 0.15f);
    pressure.addAffectedTrait(GeneType::MEMORY_RETENTION, 0.4f, 0.8f, 0.1f);

    // - Better sensory abilities for navigation
    pressure.addAffectedTrait(GeneType::VISION_RANGE, 0.4f, 50.0f, 0.1f);
    pressure.addAffectedTrait(GeneType::SMELL_RANGE, 0.3f, 100.0f, 0.1f);

    // - Higher curiosity (exploration drive)
    pressure.addAffectedTrait(GeneType::CURIOSITY, 0.3f, 0.7f, 0.1f);

    // - Aerial aptitude (if habitat is fragmented, flying helps)
    if (habitatFragmentation > 0.5f) {
        pressure.addAffectedTrait(GeneType::AERIAL_APTITUDE, 0.4f, 0.6f, 0.15f);
    }

    return pressure;
}

// =============================================================================
// COMBINED PRESSURE CALCULATIONS
// =============================================================================

std::vector<SelectionPressure> SelectionPressureCalculator::getCombinedPressure(
    const Creature* creature,
    const std::map<std::string, float>& environmentalData,
    const std::vector<Creature*>& nearbyCreatures) const {

    std::vector<SelectionPressure> pressures;

    if (!creature) return pressures;

    // Extract environmental parameters with defaults
    auto getParam = [&environmentalData](const std::string& key, float defaultVal) {
        auto it = environmentalData.find(key);
        return it != environmentalData.end() ? it->second : defaultVal;
    };

    // Calculate predation pressure
    float predatorDensity = getParam("predator_density", 0.2f);
    float predatorEfficiency = getParam("predator_efficiency", 0.5f);
    pressures.push_back(calculatePredationPressure(creature, predatorDensity, predatorEfficiency));

    // Calculate competition pressure
    float resourceDensity = getParam("resource_density", 0.5f);
    pressures.push_back(calculateCompetitionPressure(creature, nearbyCreatures, resourceDensity));

    // Calculate climate pressure
    float temperature = getParam("temperature", 0.5f);
    float season = getParam("season", 0.5f);
    float humidity = getParam("humidity", 0.5f);
    pressures.push_back(calculateClimatePressure(creature, temperature, season, humidity));

    // Calculate food pressure
    float foodAvailability = getParam("food_availability", 0.5f);
    float foodQuality = getParam("food_quality", 0.8f);
    pressures.push_back(calculateFoodPressure(creature, foodAvailability, foodQuality));

    // Calculate disease pressure
    float diseasePrevalence = getParam("disease_prevalence", 0.05f);
    float pathogenVirulence = getParam("pathogen_virulence", 0.3f);
    float transmissionRate = getParam("transmission_rate", 0.4f);
    pressures.push_back(calculateDiseasePressure(creature, diseasePrevalence,
                                                  pathogenVirulence, transmissionRate));

    // Calculate sexual selection pressure
    float mateAvailability = getParam("mate_availability", 0.5f);
    float populationDensity = getParam("population_density", 0.3f);
    float averageChoosiness = getParam("average_choosiness", 0.5f);
    pressures.push_back(calculateSexualPressure(creature, mateAvailability,
                                                 populationDensity, averageChoosiness));

    // Calculate migration pressure
    float dispersalDistance = getParam("dispersal_distance", 20.0f);
    float habitatFragmentation = getParam("habitat_fragmentation", 0.3f);
    float barrierPresence = getParam("barrier_presence", 0.2f);
    pressures.push_back(calculateMigrationPressure(creature, dispersalDistance,
                                                    habitatFragmentation, barrierPresence));

    // Apply pressure interactions (synergistic effects)
    // Food scarcity increases disease susceptibility
    float foodPressure = pressures[3].intensity;  // FOOD_SCARCITY
    if (foodPressure > 0.5f) {
        pressures[4].intensity *= (1.0f + (foodPressure - 0.5f) * 0.4f);  // DISEASE
        pressures[4].intensity = std::min(pressures[4].intensity, 1.0f);
    }

    // High predation pressure reduces opportunity for sexual selection
    float predationPressure = pressures[0].intensity;  // PREDATION
    if (predationPressure > 0.6f) {
        pressures[5].intensity *= (1.0f - (predationPressure - 0.6f) * 0.3f);  // SEXUAL_SELECTION
    }

    return pressures;
}

float SelectionPressureCalculator::getTotalSelectionIntensity(
    const std::vector<SelectionPressure>& pressures) const {

    if (pressures.empty()) return 0.0f;

    float totalIntensity = 0.0f;
    float totalWeight = 0.0f;

    for (const auto& pressure : pressures) {
        float weight = getPressureWeight(pressure.type);
        totalIntensity += pressure.intensity * weight;
        totalWeight += weight;
    }

    if (totalWeight <= 0.0f) return 0.0f;

    // Normalize by total weight
    float normalizedIntensity = totalIntensity / totalWeight;

    // Apply non-linear scaling - multiple moderate pressures compound
    // Formula: final = normalized * (1 + log(1 + num_pressures) * 0.1)
    float compoundingFactor = 1.0f + std::log(1.0f + pressures.size()) * 0.1f;

    return std::clamp(normalizedIntensity * compoundingFactor, 0.0f, 1.5f);
}

PressureType SelectionPressureCalculator::getDominantPressure(
    const std::vector<SelectionPressure>& pressures) const {

    if (pressures.empty()) return PressureType::PREDATION;

    PressureType dominant = PressureType::PREDATION;
    float maxWeightedIntensity = 0.0f;

    for (const auto& pressure : pressures) {
        float weight = getPressureWeight(pressure.type);
        float weightedIntensity = pressure.intensity * weight;

        if (weightedIntensity > maxWeightedIntensity) {
            maxWeightedIntensity = weightedIntensity;
            dominant = pressure.type;
        }
    }

    return dominant;
}

// =============================================================================
// FITNESS MODIFICATION
// =============================================================================

float SelectionPressureCalculator::applyPressureToFitness(
    const Creature* creature,
    const std::vector<SelectionPressure>& pressures,
    const PressureResponseTraits& responseTraits) const {

    if (!creature || pressures.empty()) return 1.0f;

    const DiploidGenome& genome = creature->getDiploidGenome();
    Phenotype phenotype = genome.express();

    float fitnessModifier = 1.0f;

    for (const auto& pressure : pressures) {
        if (pressure.intensity <= 0.0f) continue;

        float weight = getPressureWeight(pressure.type);

        // Calculate how well creature matches optimal traits for this pressure
        float traitMatch = 0.0f;
        float totalTraitWeight = 0.0f;

        for (const auto& traitDir : pressure.affectedTraits) {
            float traitValue = genome.getTrait(traitDir.trait);
            float optimalValue = traitDir.optimalValue;

            // Calculate match score based on direction
            float match = 0.0f;
            if (traitDir.direction > 0) {
                // Positive selection - higher values better
                match = std::min(1.0f, traitValue / optimalValue);
            } else if (traitDir.direction < 0) {
                // Negative selection - lower values better
                match = std::min(1.0f, optimalValue / std::max(0.01f, traitValue));
            } else {
                // Stabilizing selection - closer to optimal is better
                float deviation = std::abs(traitValue - optimalValue) / optimalValue;
                match = std::max(0.0f, 1.0f - deviation);
            }

            traitMatch += match * traitDir.weight;
            totalTraitWeight += traitDir.weight;
        }

        if (totalTraitWeight > 0.0f) {
            traitMatch /= totalTraitWeight;
        }

        // Calculate fitness impact from this pressure
        // High pressure * low match = fitness penalty
        // Resilience from stress response genes mitigates impact
        float resilience = responseTraits.calculateResilience(pressure.intensity);
        float pressureImpact = pressure.intensity * (1.0f - traitMatch * 0.7f) *
                               (1.0f - resilience * 0.3f);

        // Apply fitness modification (multiplicative)
        fitnessModifier *= (1.0f - pressureImpact * weight);
    }

    // Account for maintenance cost of stress response
    float maintenanceCost = responseTraits.getMaintenanceCost();
    fitnessModifier *= (2.0f - maintenanceCost);  // Penalty for high maintenance

    return std::clamp(fitnessModifier, 0.1f, 1.5f);
}

float SelectionPressureCalculator::calculateSurvivalProbability(
    const Creature* creature,
    const std::vector<SelectionPressure>& pressures,
    const PressureResponseTraits& responseTraits) const {

    if (!creature) return 0.0f;

    // Base survival probability
    float survival = 0.9f;

    // Energy and health factors
    float energyFactor = creature->getEnergy() / 200.0f;
    float healthBonus = 0.1f * energyFactor;
    survival += healthBonus;

    // Apply each pressure's mortality risk
    for (const auto& pressure : pressures) {
        if (pressure.intensity <= 0.0f) continue;

        float weight = getPressureWeight(pressure.type);

        // Calculate mortality risk from this pressure
        // Higher intensity = higher risk, resilience reduces risk
        float resilience = responseTraits.calculateResilience(pressure.intensity);
        float mortalityRisk = pressure.intensity * weight * (1.0f - resilience * 0.5f);

        // Some pressure types are more immediately lethal
        switch (pressure.type) {
            case PressureType::PREDATION:
                mortalityRisk *= 1.5f;  // Predation is directly lethal
                break;
            case PressureType::DISEASE:
                mortalityRisk *= 1.3f;  // Disease can be lethal
                break;
            case PressureType::CLIMATE:
                mortalityRisk *= 1.2f;  // Extreme climate can kill
                break;
            case PressureType::FOOD_SCARCITY:
                // Starvation is slow but certain
                mortalityRisk *= 1.0f + (1.0f - energyFactor) * 0.5f;
                break;
            default:
                mortalityRisk *= 0.8f;  // Other pressures less immediately lethal
                break;
        }

        survival -= mortalityRisk * 0.3f;
    }

    // Age factor (older creatures slightly less likely to survive)
    float ageFactor = creature->getAge() / 1000.0f;  // Normalize to reasonable lifespan
    survival -= ageFactor * 0.1f;

    return std::clamp(survival, 0.01f, 0.99f);
}

float SelectionPressureCalculator::calculateReproductiveModifier(
    const Creature* creature,
    const std::vector<SelectionPressure>& pressures) const {

    if (!creature) return 0.0f;

    const DiploidGenome& genome = creature->getDiploidGenome();
    Phenotype phenotype = genome.express();

    // Base reproductive potential from genome
    float reproduction = phenotype.fertility;

    // Energy requirements for reproduction
    float energyFactor = creature->getEnergy() / 200.0f;
    if (energyFactor < 0.5f) {
        reproduction *= energyFactor * 2.0f;  // Severely limited below half energy
    }

    // Apply pressure effects on reproduction
    for (const auto& pressure : pressures) {
        if (pressure.intensity <= 0.0f) continue;

        switch (pressure.type) {
            case PressureType::FOOD_SCARCITY:
                // Food scarcity directly reduces reproduction
                reproduction *= (1.0f - pressure.intensity * 0.6f);
                break;

            case PressureType::PREDATION:
                // High predation reduces mating opportunities
                reproduction *= (1.0f - pressure.intensity * 0.3f);
                break;

            case PressureType::SEXUAL_SELECTION:
                // Sexual selection can enhance or reduce based on attractiveness
                {
                    float attractiveness = calculateMateAttractiveness(creature);
                    float effect = (attractiveness - 0.5f) * 2.0f;  // -1 to +1
                    reproduction *= (1.0f + effect * pressure.intensity * 0.4f);
                }
                break;

            case PressureType::CLIMATE:
                // Extreme climate reduces reproduction
                reproduction *= (1.0f - pressure.intensity * 0.4f);
                break;

            case PressureType::DISEASE:
                // Disease impacts reproduction
                reproduction *= (1.0f - pressure.intensity * 0.5f);
                break;

            default:
                reproduction *= (1.0f - pressure.intensity * 0.1f);
                break;
        }
    }

    return std::clamp(reproduction, 0.0f, 2.0f);
}

// =============================================================================
// HISTORY TRACKING
// =============================================================================

void SelectionPressureCalculator::trackPressureHistory(
    int generation,
    const std::vector<SelectionPressure>& pressures,
    float populationFitness,
    int populationSize) {

    currentGeneration = generation;

    // Record each pressure type
    for (const auto& pressure : pressures) {
        PressureHistoryRecord record(generation, pressure.type, pressure.intensity);

        // Add to type-specific history
        pressureHistory[pressure.type].push_back(record);
    }

    // Check for significant events
    for (const auto& pressure : pressures) {
        if (pressure.intensity > eventLoggingThreshold) {
            // Check if this is a significant change from previous
            auto& history = pressureHistory[pressure.type];
            if (history.size() >= 2) {
                float prevIntensity = history[history.size() - 2].intensity;
                float change = std::abs(pressure.intensity - prevIntensity);

                if (change > eventLoggingThreshold) {
                    SelectionEvent event;
                    event.generation = generation;
                    event.pressureType = pressure.type;
                    event.pressureIntensity = pressure.intensity;
                    event.fitnessImpact = populationFitness;
                    event.populationBefore = populationSize;
                    event.populationAfter = populationSize;
                    event.description = "Significant change in " +
                                        std::string(pressureTypeToString(pressure.type)) +
                                        " pressure";
                    event.isAdaptiveResponse = change > 0.2f;

                    logSelectionEvent(event);
                }
            }
        }
    }

    // Prune old history
    pruneHistory();
}

std::vector<PressureHistoryRecord> SelectionPressureCalculator::getPressureHistory(
    PressureType type, int lastNGenerations) const {

    auto it = pressureHistory.find(type);
    if (it == pressureHistory.end()) {
        return {};
    }

    const auto& history = it->second;

    if (lastNGenerations <= 0 || lastNGenerations >= static_cast<int>(history.size())) {
        return std::vector<PressureHistoryRecord>(history.begin(), history.end());
    }

    // Return last N generations
    auto startIt = history.end() - lastNGenerations;
    return std::vector<PressureHistoryRecord>(startIt, history.end());
}

std::vector<PressureHistoryRecord> SelectionPressureCalculator::getAllPressureHistory(
    int lastNGenerations) const {

    std::vector<PressureHistoryRecord> allHistory;

    for (const auto& [type, history] : pressureHistory) {
        if (lastNGenerations <= 0) {
            allHistory.insert(allHistory.end(), history.begin(), history.end());
        } else {
            int startIdx = std::max(0, static_cast<int>(history.size()) - lastNGenerations);
            allHistory.insert(allHistory.end(),
                              history.begin() + startIdx, history.end());
        }
    }

    // Sort by generation
    std::sort(allHistory.begin(), allHistory.end(),
              [](const PressureHistoryRecord& a, const PressureHistoryRecord& b) {
                  return a.generation < b.generation;
              });

    return allHistory;
}

float SelectionPressureCalculator::calculatePressureTrend(PressureType type,
                                                          int windowSize) const {
    auto history = getPressureHistory(type, windowSize);

    if (history.size() < 2) return 0.0f;

    // Linear regression to calculate trend
    float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumX2 = 0.0f;
    int n = static_cast<int>(history.size());

    for (int i = 0; i < n; i++) {
        float x = static_cast<float>(i);
        float y = history[i].intensity;

        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumX2 += x * x;
    }

    float denominator = n * sumX2 - sumX * sumX;
    if (std::abs(denominator) < 0.0001f) return 0.0f;

    float slope = (n * sumXY - sumX * sumY) / denominator;

    return slope;
}

void SelectionPressureCalculator::clearHistory() {
    pressureHistory.clear();
    selectionEvents.clear();
}

// =============================================================================
// EVENT LOGGING
// =============================================================================

void SelectionPressureCalculator::logSelectionEvent(const SelectionEvent& event) {
    selectionEvents.push_back(event);

    // Keep event history manageable
    if (selectionEvents.size() > maxHistorySize * 2) {
        selectionEvents.erase(selectionEvents.begin(),
                              selectionEvents.begin() + maxHistorySize);
    }
}

std::vector<SelectionEvent> SelectionPressureCalculator::getSelectionEvents(
    int sinceGeneration) const {

    std::vector<SelectionEvent> filteredEvents;

    for (const auto& event : selectionEvents) {
        if (event.generation >= sinceGeneration) {
            filteredEvents.push_back(event);
        }
    }

    return filteredEvents;
}

std::vector<SelectionEvent> SelectionPressureCalculator::getEventsByType(
    PressureType type, int sinceGeneration) const {

    std::vector<SelectionEvent> filteredEvents;

    for (const auto& event : selectionEvents) {
        if (event.pressureType == type && event.generation >= sinceGeneration) {
            filteredEvents.push_back(event);
        }
    }

    return filteredEvents;
}

void SelectionPressureCalculator::exportEventsToCSV(const std::string& filename) const {
    std::ofstream file(filename);

    if (!file.is_open()) {
        return;
    }

    // Write header
    file << "Generation,PressureType,Intensity,FitnessImpact,MortalityRate,"
         << "PopulationBefore,PopulationAfter,IsAdaptive,Description\n";

    // Write events
    for (const auto& event : selectionEvents) {
        file << event.generation << ","
             << pressureTypeToString(event.pressureType) << ","
             << event.pressureIntensity << ","
             << event.fitnessImpact << ","
             << event.mortalityRate << ","
             << event.populationBefore << ","
             << event.populationAfter << ","
             << (event.isAdaptiveResponse ? "true" : "false") << ","
             << "\"" << event.description << "\"\n";
    }

    file.close();
}

// =============================================================================
// CONFIGURATION
// =============================================================================

void SelectionPressureCalculator::setPressureWeight(PressureType type, float weight) {
    pressureWeights[type] = std::clamp(weight, 0.0f, 1.0f);
}

float SelectionPressureCalculator::getPressureWeight(PressureType type) const {
    auto it = pressureWeights.find(type);
    if (it != pressureWeights.end()) {
        return it->second;
    }

    // Default weight if not found
    return 1.0f / static_cast<float>(static_cast<int>(PressureType::COUNT));
}

void SelectionPressureCalculator::setHistoryBufferSize(int maxGenerations) {
    maxHistorySize = std::max(10, static_cast<size_t>(maxGenerations));
    pruneHistory();
}

void SelectionPressureCalculator::setEventLoggingThreshold(float threshold) {
    eventLoggingThreshold = std::clamp(threshold, 0.01f, 0.5f);
}

// =============================================================================
// UTILITY METHODS
// =============================================================================

float SelectionPressureCalculator::calculateTraitFitnessCorrelation(
    GeneType trait, const SelectionPressure& pressure) const {

    // Find if this trait is affected by the pressure
    for (const auto& traitDir : pressure.affectedTraits) {
        if (traitDir.trait == trait) {
            // Return correlation based on direction and weight
            // Positive direction = positive correlation with fitness
            return traitDir.direction * traitDir.weight * pressure.intensity;
        }
    }

    // Trait not directly affected by this pressure
    return 0.0f;
}

} // namespace genetics
