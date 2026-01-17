#pragma once

#include "Morphology.h"
#include <glm/glm.hpp>
#include <functional>
#include <string>

// =============================================================================
// METAMORPHOSIS SYSTEM
// Handles creature life stage transitions and body transformations
// =============================================================================

// Metamorphosis types
enum class MetamorphosisType {
    NONE,               // No metamorphosis (direct development)
    GRADUAL,            // Gradual change through instars (insects)
    COMPLETE,           // Dramatic transformation (butterfly)
    AQUATIC_TO_LAND,    // Tadpole to frog type
    LARVAL_DISPERSAL    // Larvae different for dispersal (marine)
};

// =============================================================================
// AQUATIC-TO-LAND EVOLUTION SYSTEM (PHASE 7 - Agent 3)
// Progressive transition from fully aquatic to amphibious to land creatures
// =============================================================================

// Stages in the aquatic-to-land transition (not instant flips)
enum class AmphibiousStage {
    FULLY_AQUATIC,      // 100% aquatic - cannot survive on land
    TRANSITIONING,      // Developing land capabilities (50-80% aquatic)
    AMPHIBIOUS,         // Can survive both environments equally
    LAND_ADAPTED        // Primarily land-dwelling (can still enter water briefly)
};

// Configuration for amphibious trait thresholds
struct AmphibiousThresholds {
    // Minimum trait values to progress from Aquatic to Transitioning
    float lungCapacityToTransition = 0.2f;      // Need some lung development
    float limbStrengthToTransition = 0.15f;     // Need basic limb strength

    // Minimum trait values to progress from Transitioning to Amphibious
    float lungCapacityToAmphibious = 0.5f;      // Half-developed lungs
    float limbStrengthToAmphibious = 0.4f;      // Can support body weight briefly
    float skinMoistureMinAmphibious = 0.3f;     // Still needs some moisture

    // Minimum trait values to progress to Land Adapted
    float lungCapacityToLand = 0.8f;            // Near-full lung capacity
    float limbStrengthToLand = 0.7f;            // Strong limbs for locomotion
    float skinMoistureMinLand = 0.1f;           // Can tolerate dry conditions
    float aquaticAffinityMaxLand = 0.3f;        // Low water dependence

    // Environmental exposure thresholds (accumulated time in seconds)
    float shoreExposureToTransition = 300.0f;   // 5 minutes near shore
    float landExposureToAmphibious = 600.0f;    // 10 minutes on land
    float landExposureToLandAdapted = 1800.0f;  // 30 minutes sustained land

    // Penalty thresholds
    float maxLandTimeAquatic = 10.0f;           // Aquatic dies after 10s on land
    float maxWaterTimeLand = 60.0f;             // Land creatures struggle after 60s underwater
};

// Per-creature amphibious transition state (saved/restored with creature)
struct AmphibiousTransitionState {
    AmphibiousStage currentStage = AmphibiousStage::FULLY_AQUATIC;

    // Progress within current stage (0.0 to 1.0)
    float transitionProgress = 0.0f;

    // Environmental exposure tracking (accumulated time in seconds)
    float timeNearShore = 0.0f;         // Time spent in shallow water near land
    float timeOnLand = 0.0f;            // Time spent on land
    float timeInDeepWater = 0.0f;       // Time spent in deep water
    float timeSubmerged = 0.0f;         // Current continuous submersion time

    // Trait development (evolves over time based on environment)
    float lungCapacity = 0.0f;          // 0 = gills only, 1 = full lungs
    float limbStrength = 0.0f;          // 0 = fins only, 1 = strong legs
    float skinMoisture = 1.0f;          // 1 = needs water, 0 = dry-tolerant
    float aquaticAffinity = 1.0f;       // 1 = fully aquatic, 0 = fully terrestrial

    // Cooldown timers to prevent rapid oscillation
    float stageChangeCooldown = 0.0f;   // Time until next stage change allowed
    float lastStageChangeTime = 0.0f;   // When last stage change occurred

    // Penalties from wrong environment
    float environmentalStress = 0.0f;   // 0-1, causes energy drain/damage at high levels
    float oxygenLevel = 1.0f;           // For land creatures underwater

    // Debug flags
    bool debugLogEnabled = false;

    // Serialization support
    void reset() {
        currentStage = AmphibiousStage::FULLY_AQUATIC;
        transitionProgress = 0.0f;
        timeNearShore = timeOnLand = timeInDeepWater = timeSubmerged = 0.0f;
        lungCapacity = limbStrength = 0.0f;
        skinMoisture = aquaticAffinity = 1.0f;
        stageChangeCooldown = lastStageChangeTime = 0.0f;
        environmentalStress = 0.0f;
        oxygenLevel = 1.0f;
    }
};

// Result of amphibious transition update
struct AmphibiousUpdateResult {
    bool stageChanged = false;
    AmphibiousStage previousStage = AmphibiousStage::FULLY_AQUATIC;
    AmphibiousStage newStage = AmphibiousStage::FULLY_AQUATIC;
    float energyDrain = 0.0f;           // Energy lost due to environmental stress
    float speedPenalty = 0.0f;          // Speed reduction (0-1, 0 = no penalty)
    float healthDamage = 0.0f;          // Damage from prolonged wrong environment
    std::string debugMessage;           // For logging stage changes
};

// Helper to get stage name for debugging
inline const char* getAmphibiousStageName(AmphibiousStage stage) {
    switch (stage) {
        case AmphibiousStage::FULLY_AQUATIC: return "Fully Aquatic";
        case AmphibiousStage::TRANSITIONING: return "Transitioning";
        case AmphibiousStage::AMPHIBIOUS: return "Amphibious";
        case AmphibiousStage::LAND_ADAPTED: return "Land Adapted";
        default: return "Unknown";
    }
}

// Calculate locomotion blend factor (0 = swim, 1 = walk)
inline float calculateLocomotionBlend(const AmphibiousTransitionState& state) {
    switch (state.currentStage) {
        case AmphibiousStage::FULLY_AQUATIC:
            return 0.0f;  // Pure swim
        case AmphibiousStage::TRANSITIONING:
            return state.transitionProgress * 0.4f;  // Up to 40% walk blend
        case AmphibiousStage::AMPHIBIOUS:
            return 0.4f + state.transitionProgress * 0.3f;  // 40-70% walk blend
        case AmphibiousStage::LAND_ADAPTED:
            return 0.7f + state.transitionProgress * 0.3f;  // 70-100% walk blend
        default:
            return 0.0f;
    }
}

// Transformation stage during metamorphosis
enum class TransformationStage {
    NOT_STARTED,
    INITIATING,         // Beginning transformation
    REORGANIZING,       // Internal changes happening
    EMERGING,           // New form appearing
    HARDENING,          // New form solidifying
    COMPLETE
};

// =============================================================================
// LIFE STAGE CONTROLLER
// Manages transitions between life stages
// =============================================================================

class LifeStageController {
public:
    LifeStageController() = default;

    // Initialize with genes
    void initialize(const MorphologyGenes& genes);

    // Update life stage (call each frame)
    void update(float deltaTime, float energy, float health);

    // Get current life stage
    LifeStage getCurrentStage() const { return currentStage; }
    LifeStageInfo getStageInfo() const { return stageInfo; }

    // Check if metamorphosis is happening
    bool isMetamorphosing() const { return transformationStage != TransformationStage::NOT_STARTED &&
                                           transformationStage != TransformationStage::COMPLETE; }

    // Get transformation progress (0-1)
    float getTransformationProgress() const { return transformationProgress; }
    TransformationStage getTransformationStage() const { return transformationStage; }

    // Get current morphology (interpolated during metamorphosis)
    MorphologyGenes getCurrentMorphology() const;

    // Event callbacks
    using StageChangeCallback = std::function<void(LifeStage, LifeStage)>;
    void setStageChangeCallback(StageChangeCallback callback) { onStageChange = callback; }

    // Force stage change (for testing)
    void setStage(LifeStage stage);

private:
    MorphologyGenes baseGenes;
    MorphologyGenes larvalMorphology;
    MorphologyGenes adultMorphology;

    LifeStage currentStage = LifeStage::ADULT;
    LifeStageInfo stageInfo;

    MetamorphosisType metamorphosisType = MetamorphosisType::NONE;
    TransformationStage transformationStage = TransformationStage::NOT_STARTED;
    float transformationProgress = 0.0f;
    float transformationDuration = 10.0f; // Seconds

    float age = 0.0f;
    float energyAccumulated = 0.0f;

    StageChangeCallback onStageChange;

    void checkForTransition(float energy);
    void updateTransformation(float deltaTime);
    void calculateStageInfo();
    MorphologyGenes generateLarvalMorphology(const MorphologyGenes& adult);
    void transitionToStage(LifeStage newStage);
};

// =============================================================================
// LARVAL MORPHOLOGY GENERATOR
// Creates larval forms from adult morphology
// =============================================================================

namespace LarvalMorphology {
    // Generate larval morphology for complete metamorphosis
    inline MorphologyGenes generateCompleteLarval(const MorphologyGenes& adult) {
        MorphologyGenes larval = adult;

        // Larval forms are typically:
        // - Smaller
        // - Simpler body plan
        // - Often legless or different leg arrangement
        // - Different feeding apparatus
        // - More segmented

        larval.baseMass = adult.baseMass * 0.3f;
        larval.bodyLength *= 0.6f;
        larval.bodyWidth *= 1.2f;  // Often more rounded
        larval.bodyHeight *= 0.8f;

        // Simplified limbs or none
        larval.legPairs = 0;
        larval.armPairs = 0;
        larval.wingPairs = 0;

        // Longer, segmented body
        larval.segmentCount = std::min(8, adult.segmentCount + 3);

        // No tail (or very different)
        larval.hasTail = false;

        // Simplified head
        larval.headSize = adult.headSize * 0.8f;
        larval.eyeCount = std::max(2, adult.eyeCount / 2);
        larval.eyeSize *= 0.7f;

        // No display features
        larval.primaryFeature = FeatureType::NONE;
        larval.secondaryFeature = FeatureType::NONE;

        // Different metabolism (often higher)
        larval.metabolicMultiplier = adult.metabolicMultiplier * 1.3f;

        return larval;
    }

    // Generate larval morphology for aquatic-to-land transition
    inline MorphologyGenes generateAquaticLarval(const MorphologyGenes& adult) {
        MorphologyGenes larval = adult;

        // Tadpole-like form
        larval.baseMass = adult.baseMass * 0.2f;
        larval.bodyLength *= 0.4f;
        larval.bodyWidth *= 0.5f;
        larval.bodyHeight *= 0.5f;

        // No legs yet
        larval.legPairs = 0;
        larval.armPairs = 0;
        larval.wingPairs = 0;

        // Swimming tail
        larval.hasTail = true;
        larval.tailLength = adult.bodyLength * 0.8f;
        larval.tailSegments = 6;
        larval.hasCaudalFin = true;

        // Gills represented as fins
        larval.finCount = 2;
        larval.hasPectoralFins = false;

        // Large head relative to body
        larval.headSize = adult.headSize * 1.5f;

        // Lateral line/aquatic senses
        larval.eyeSize *= 0.8f;
        larval.eyesSideFacing = true;

        return larval;
    }

    // Generate larval morphology for gradual metamorphosis (nymph)
    inline MorphologyGenes generateNymphMorphology(const MorphologyGenes& adult) {
        MorphologyGenes nymph = adult;

        // Nymphs look like small adults but with:
        // - Smaller size
        // - Undeveloped wings
        // - Smaller reproductive features

        nymph.baseMass = adult.baseMass * 0.4f;
        nymph.bodyLength *= 0.7f;
        nymph.bodyWidth *= 0.7f;
        nymph.bodyHeight *= 0.7f;

        // Wing buds instead of wings
        if (adult.wingPairs > 0) {
            nymph.wingPairs = adult.wingPairs;
            nymph.wingSpan = adult.wingSpan * 0.1f; // Tiny wing buds
            nymph.canFly = false;
        }

        // Smaller features
        nymph.featureSize = adult.featureSize * 0.5f;

        return nymph;
    }
}

// =============================================================================
// MORPHOLOGY INTERPOLATOR
// Smoothly interpolates between morphologies during transformation
// =============================================================================

class MorphologyInterpolator {
public:
    // Interpolate between two morphologies
    static MorphologyGenes interpolate(
        const MorphologyGenes& from,
        const MorphologyGenes& to,
        float t
    );

    // Smooth interpolation curves
    static float easeInOut(float t);
    static float easeOutElastic(float t);

private:
    static float lerp(float a, float b, float t);
    static int lerpInt(int a, int b, float t);
};

// =============================================================================
// INSTAR SYSTEM
// For gradual metamorphosis through multiple molts
// =============================================================================

struct InstarInfo {
    int currentInstar = 1;      // Which molt/stage (1 = first instar)
    int totalInstars = 5;       // Total number before adult
    float sizeMultiplier = 0.2f; // Size at this instar
    float nextMoltEnergy = 50.0f; // Energy needed to molt
    bool isFinalInstar = false;
};

class InstarController {
public:
    InstarController() = default;

    void initialize(const MorphologyGenes& genes, int numInstars = 5);

    // Check if ready to molt
    bool canMolt(float currentEnergy) const;

    // Perform molt
    void molt();

    // Get current instar info
    InstarInfo getInstarInfo() const { return info; }

    // Get morphology for current instar
    MorphologyGenes getCurrentMorphology() const;

private:
    MorphologyGenes adultGenes;
    InstarInfo info;

    float calculateSizeForInstar(int instar) const;
};

// =============================================================================
// AMPHIBIOUS TRANSITION CONTROLLER (PHASE 7 - Agent 3)
// Manages gradual aquatic-to-land evolution for individual creatures
// =============================================================================

// Environment type for transition logic
enum class EnvironmentZone {
    DEEP_WATER,         // Far from shore, fully submerged
    SHALLOW_WATER,      // Near shore, submerged
    SHORE,              // At water's edge (triggers transition)
    LAND                // Above water level
};

class AmphibiousTransitionController {
public:
    AmphibiousTransitionController() = default;

    // Initialize with starting traits (called when creature spawns)
    void initialize(float initialLungCapacity = 0.0f,
                    float initialLimbStrength = 0.0f,
                    AmphibiousStage startingStage = AmphibiousStage::FULLY_AQUATIC);

    // Initialize for an existing amphibian creature type
    void initializeAsAmphibian();

    // Main update - call each frame with environment info
    // Returns penalties/effects to apply to creature
    AmphibiousUpdateResult update(
        float deltaTime,
        EnvironmentZone currentZone,
        float waterDepth,           // Current water depth (negative = above water)
        float distanceToShore,      // Distance to nearest shoreline
        float oxygenAvailability,   // 0-1, how much oxygen is available
        float totalAge              // Creature's total age for cooldown tracking
    );

    // Get current state (for saving/UI)
    const AmphibiousTransitionState& getState() const { return m_state; }
    AmphibiousTransitionState& getState() { return m_state; }

    // Get current thresholds (for UI display)
    const AmphibiousThresholds& getThresholds() const { return m_thresholds; }

    // Set custom thresholds (for different species)
    void setThresholds(const AmphibiousThresholds& thresholds) { m_thresholds = thresholds; }

    // Direct state access for serialization
    void setState(const AmphibiousTransitionState& state) { m_state = state; }

    // Query methods
    AmphibiousStage getCurrentStage() const { return m_state.currentStage; }
    float getTransitionProgress() const { return m_state.transitionProgress; }
    float getLocomotionBlend() const { return calculateLocomotionBlend(m_state); }

    // Check if creature can survive in given environment
    bool canSurviveInZone(EnvironmentZone zone) const;

    // Get speed multiplier for current environment
    float getSpeedMultiplier(EnvironmentZone zone) const;

    // Get energy cost multiplier for current environment
    float getEnergyCostMultiplier(EnvironmentZone zone) const;

    // Debug
    void setDebugLogging(bool enabled) { m_state.debugLogEnabled = enabled; }
    bool isDebugLoggingEnabled() const { return m_state.debugLogEnabled; }

    // Force stage change (for testing/scenarios)
    void forceStage(AmphibiousStage stage);

private:
    AmphibiousTransitionState m_state;
    AmphibiousThresholds m_thresholds;

    // Stage transition logic
    bool checkTransitionToNextStage();
    bool checkRegressionToPreviousStage();

    // Trait development based on environment
    void updateTraitDevelopment(float deltaTime, EnvironmentZone zone);

    // Calculate penalties for being in wrong environment
    void calculateEnvironmentalPenalties(
        float deltaTime,
        EnvironmentZone zone,
        AmphibiousUpdateResult& result
    );

    // Update exposure timers
    void updateExposureTimers(float deltaTime, EnvironmentZone zone);

    // Helper to determine if traits meet threshold for stage
    bool meetsTraitThresholds(AmphibiousStage targetStage) const;
    bool meetsExposureThresholds(AmphibiousStage targetStage) const;
};
