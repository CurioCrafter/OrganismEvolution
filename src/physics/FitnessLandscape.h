#pragma once

#include "Morphology.h"
#include "../entities/CreatureType.h"
#include <glm/glm.hpp>

// =============================================================================
// FITNESS LANDSCAPE SYSTEM
// Calculates how body shape affects survival, speed, energy, and combat
// =============================================================================

// Individual fitness factors
struct FitnessFactors {
    // Movement capability
    float movementSpeed = 0.0f;       // Base speed capability
    float acceleration = 0.0f;        // How quickly can change speed
    float maneuverability = 0.0f;     // Turning ability
    float stability = 0.0f;           // Balance/fall resistance
    float terrainTraversal = 0.0f;    // Ability to cross obstacles

    // Energy efficiency
    float energyEfficiency = 0.0f;    // Distance traveled per energy unit
    float metabolicEfficiency = 0.0f; // Base energy consumption rate
    float staminaFactor = 0.0f;       // How long can sustain activity

    // Combat capability
    float attackReach = 0.0f;         // How far can attack
    float attackPower = 0.0f;         // Damage dealt
    float attackSpeed = 0.0f;         // Attack frequency
    float defensiveAbility = 0.0f;    // Damage resistance
    float intimidation = 0.0f;        // Scares off predators/rivals

    // Survival capability
    float predatorEvasion = 0.0f;     // Escaping predators
    float camouflage = 0.0f;          // Hiding ability
    float sensoryRange = 0.0f;        // Detection distance
    float coldResistance = 0.0f;      // Temperature tolerance
    float heatResistance = 0.0f;

    // Resource gathering
    float foodGathering = 0.0f;       // Finding and collecting food
    float reachCapability = 0.0f;     // Accessing distant resources
    float carryingCapacity = 0.0f;    // Amount can transport

    // Reproduction
    float mateFinding = 0.0f;         // Locating potential mates
    float displayQuality = 0.0f;      // Attractiveness to mates
    float offspringCare = 0.0f;       // Nurturing ability
};

// Environment types that affect fitness
enum class EnvironmentType {
    PLAINS,         // Open flat terrain
    FOREST,         // Trees, undergrowth
    MOUNTAIN,       // Steep, rocky
    SWAMP,          // Wet, muddy
    DESERT,         // Hot, sandy
    TUNDRA,         // Cold, sparse
    AQUATIC,        // Water-based
    AERIAL          // Primarily flying
};

// Niche specialization types
enum class NicheType {
    GENERALIST,     // Jack of all trades
    PURSUIT_PREDATOR,   // Fast, endurance hunter
    AMBUSH_PREDATOR,    // Stealth hunter
    GRAZER,         // Bulk plant eater
    BROWSER,        // Selective plant eater
    SCAVENGER,      // Opportunistic feeder
    CLIMBER,        // Tree/cliff specialist
    SWIMMER,        // Aquatic specialist
    FLYER,          // Aerial specialist
    BURROWER        // Underground specialist
};

// =============================================================================
// FITNESS CALCULATOR
// =============================================================================

class FitnessCalculator {
public:
    // Calculate all fitness factors from morphology
    static FitnessFactors calculateFactors(
        const MorphologyGenes& genes,
        CreatureType type
    );

    // Calculate overall fitness score
    static float calculateOverallFitness(
        const FitnessFactors& factors,
        CreatureType type,
        EnvironmentType environment = EnvironmentType::PLAINS
    );

    // Calculate fitness for specific niche
    static float calculateNicheFitness(
        const FitnessFactors& factors,
        NicheType niche
    );

    // Calculate combat outcome probability
    static float calculateCombatAdvantage(
        const FitnessFactors& attacker,
        const FitnessFactors& defender
    );

    // Calculate pursuit/escape outcome
    static float calculateChaseOutcome(
        const FitnessFactors& pursuer,
        const FitnessFactors& prey,
        float initialDistance
    );

private:
    // Movement calculations
    static float calculateSpeedFactor(const MorphologyGenes& genes);
    static float calculateAccelerationFactor(const MorphologyGenes& genes);
    static float calculateManeuverabilityFactor(const MorphologyGenes& genes);
    static float calculateStabilityFactor(const MorphologyGenes& genes);
    static float calculateTerrainFactor(const MorphologyGenes& genes);

    // Energy calculations
    static float calculateEfficiencyFactor(const MorphologyGenes& genes);
    static float calculateMetabolicFactor(const MorphologyGenes& genes);
    static float calculateStaminaFactor(const MorphologyGenes& genes);

    // Combat calculations
    static float calculateAttackReach(const MorphologyGenes& genes);
    static float calculateAttackPower(const MorphologyGenes& genes);
    static float calculateDefense(const MorphologyGenes& genes);

    // Survival calculations
    static float calculateEvasion(const MorphologyGenes& genes);
    static float calculateSensory(const MorphologyGenes& genes);

    // Resource calculations
    static float calculateGathering(const MorphologyGenes& genes);
};

// =============================================================================
// ENVIRONMENT MODIFIERS
// How different environments affect fitness factors
// =============================================================================

namespace EnvironmentModifiers {
    // Get modifier for a trait in an environment
    inline float getSpeedModifier(EnvironmentType env) {
        switch (env) {
            case EnvironmentType::PLAINS:   return 1.2f;  // Speed is very important
            case EnvironmentType::FOREST:   return 0.9f;  // Less room to run
            case EnvironmentType::MOUNTAIN: return 0.7f;  // Terrain limits speed
            case EnvironmentType::SWAMP:    return 0.6f;  // Mud slows everything
            case EnvironmentType::DESERT:   return 1.0f;  // Heat limits sustained speed
            case EnvironmentType::TUNDRA:   return 0.8f;  // Snow and cold
            case EnvironmentType::AQUATIC:  return 0.5f;  // Different movement
            case EnvironmentType::AERIAL:   return 1.3f;  // Speed crucial in air
            default: return 1.0f;
        }
    }

    inline float getManeuverabilityModifier(EnvironmentType env) {
        switch (env) {
            case EnvironmentType::PLAINS:   return 0.8f;
            case EnvironmentType::FOREST:   return 1.3f;  // Must navigate obstacles
            case EnvironmentType::MOUNTAIN: return 1.2f;
            case EnvironmentType::SWAMP:    return 0.9f;
            case EnvironmentType::DESERT:   return 0.7f;
            case EnvironmentType::TUNDRA:   return 0.8f;
            case EnvironmentType::AQUATIC:  return 1.1f;  // 3D movement
            case EnvironmentType::AERIAL:   return 1.4f;  // Very important in air
            default: return 1.0f;
        }
    }

    inline float getEfficiencyModifier(EnvironmentType env) {
        switch (env) {
            case EnvironmentType::PLAINS:   return 1.0f;
            case EnvironmentType::FOREST:   return 1.1f;
            case EnvironmentType::MOUNTAIN: return 1.3f;  // Must climb efficiently
            case EnvironmentType::SWAMP:    return 1.2f;  // Hard to move
            case EnvironmentType::DESERT:   return 1.4f;  // Energy precious
            case EnvironmentType::TUNDRA:   return 1.5f;  // Food scarce
            case EnvironmentType::AQUATIC:  return 1.0f;
            case EnvironmentType::AERIAL:   return 1.6f;  // Flight is costly
            default: return 1.0f;
        }
    }

    inline float getCombatModifier(EnvironmentType env) {
        switch (env) {
            case EnvironmentType::PLAINS:   return 1.1f;  // Open combat
            case EnvironmentType::FOREST:   return 0.9f;  // Ambush territory
            case EnvironmentType::MOUNTAIN: return 0.8f;  // Position matters more
            case EnvironmentType::SWAMP:    return 0.7f;
            case EnvironmentType::DESERT:   return 1.0f;
            case EnvironmentType::TUNDRA:   return 1.0f;
            case EnvironmentType::AQUATIC:  return 0.8f;
            case EnvironmentType::AERIAL:   return 0.5f;  // Hard to fight while flying
            default: return 1.0f;
        }
    }

    inline float getSensoryModifier(EnvironmentType env) {
        switch (env) {
            case EnvironmentType::PLAINS:   return 1.2f;  // Can see far
            case EnvironmentType::FOREST:   return 0.7f;  // Blocked sightlines
            case EnvironmentType::MOUNTAIN: return 1.1f;  // High vantage points
            case EnvironmentType::SWAMP:    return 0.8f;  // Fog, vegetation
            case EnvironmentType::DESERT:   return 1.3f;  // Clear air
            case EnvironmentType::TUNDRA:   return 1.0f;
            case EnvironmentType::AQUATIC:  return 0.6f;  // Limited visibility
            case EnvironmentType::AERIAL:   return 1.4f;  // Bird's eye view
            default: return 1.0f;
        }
    }
}

// =============================================================================
// NICHE WEIGHTS
// How important each factor is for different ecological roles
// =============================================================================

namespace NicheWeights {
    struct Weights {
        float speed;
        float maneuverability;
        float efficiency;
        float attackPower;
        float defense;
        float sensory;
        float gathering;
        float stealth;
    };

    inline Weights getWeights(NicheType niche) {
        switch (niche) {
            case NicheType::PURSUIT_PREDATOR:
                return {0.25f, 0.15f, 0.15f, 0.20f, 0.05f, 0.15f, 0.0f, 0.05f};

            case NicheType::AMBUSH_PREDATOR:
                return {0.10f, 0.15f, 0.10f, 0.25f, 0.05f, 0.10f, 0.0f, 0.25f};

            case NicheType::GRAZER:
                return {0.10f, 0.10f, 0.25f, 0.0f, 0.20f, 0.15f, 0.20f, 0.0f};

            case NicheType::BROWSER:
                return {0.10f, 0.15f, 0.20f, 0.0f, 0.10f, 0.15f, 0.25f, 0.05f};

            case NicheType::SCAVENGER:
                return {0.15f, 0.15f, 0.20f, 0.05f, 0.10f, 0.20f, 0.15f, 0.0f};

            case NicheType::CLIMBER:
                return {0.05f, 0.25f, 0.20f, 0.05f, 0.10f, 0.10f, 0.20f, 0.05f};

            case NicheType::SWIMMER:
                return {0.20f, 0.20f, 0.15f, 0.10f, 0.10f, 0.15f, 0.10f, 0.0f};

            case NicheType::FLYER:
                return {0.20f, 0.25f, 0.25f, 0.05f, 0.05f, 0.15f, 0.05f, 0.0f};

            case NicheType::BURROWER:
                return {0.05f, 0.10f, 0.20f, 0.05f, 0.20f, 0.10f, 0.15f, 0.15f};

            case NicheType::GENERALIST:
            default:
                return {0.15f, 0.15f, 0.15f, 0.10f, 0.10f, 0.15f, 0.15f, 0.05f};
        }
    }
}

// =============================================================================
// SPECIALIZATION BONUSES
// Bonus fitness for morphology that matches niche
// =============================================================================

class SpecializationCalculator {
public:
    // Calculate how well morphology matches a niche
    static float calculateNicheMatch(
        const MorphologyGenes& genes,
        NicheType niche
    );

    // Determine optimal niche for morphology
    static NicheType determineOptimalNiche(const MorphologyGenes& genes);

    // Get specialization bonus (or penalty for mismatch)
    static float getSpecializationBonus(
        const MorphologyGenes& genes,
        NicheType attemptedNiche
    );

private:
    static float matchPursuitPredator(const MorphologyGenes& genes);
    static float matchAmbushPredator(const MorphologyGenes& genes);
    static float matchGrazer(const MorphologyGenes& genes);
    static float matchClimber(const MorphologyGenes& genes);
    static float matchSwimmer(const MorphologyGenes& genes);
    static float matchFlyer(const MorphologyGenes& genes);
};
