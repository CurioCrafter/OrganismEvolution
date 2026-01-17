#include "VisualIndicators.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

// =============================================================================
// VISUAL STATE CALCULATOR IMPLEMENTATION
// =============================================================================

void VisualStateCalculator::update(
    VisualState& state,
    float energy,
    float maxEnergy,
    float health,
    float fear,
    float age,
    float fitness,
    const MorphologyGenes& genes,
    LifeStage lifeStage)
{
    // Reset flags
    state.stateFlags = VisualStateFlag::NONE;

    // Calculate energy ratio
    float energyRatio = energy / maxEnergy;

    // Set state flags based on conditions
    if (health < 50.0f) {
        state.stateFlags = state.stateFlags | VisualStateFlag::INJURED;
    }
    if (energyRatio < 0.2f) {
        state.stateFlags = state.stateFlags | VisualStateFlag::STARVING;
    }
    if (energyRatio < 0.3f) {
        state.stateFlags = state.stateFlags | VisualStateFlag::EXHAUSTED;
    }
    if (fear > 0.5f) {
        state.stateFlags = state.stateFlags | VisualStateFlag::AFRAID;
    }
    if (energyRatio > 0.8f && health > 80.0f) {
        state.stateFlags = state.stateFlags | VisualStateFlag::RELAXED;
    }
    if (lifeStage == LifeStage::LARVAL || lifeStage == LifeStage::JUVENILE) {
        // Young creatures look different
    }

    // Apply all effects
    applyEnergyEffects(state, energyRatio);
    applyInjuryEffects(state, health);
    applyFearEffects(state, fear);

    // Calculate max age (rough estimate based on mass)
    float maxAge = 100.0f + genes.baseMass * 50.0f;
    applyAgeEffects(state, age, maxAge);

    // Fitness affects muscle definition
    state.muscleDefinition = 0.3f + fitness * 0.5f;
    state.coatCondition = 0.5f + energyRatio * 0.3f + (health / 100.0f) * 0.2f;

    // Fat storage based on energy
    state.fatStorage = smoothStep(0.3f, 0.9f, energyRatio);

    // Feature display based on genes
    if (genes.primaryFeature != FeatureType::NONE) {
        // Display features when healthy and not afraid
        if (energyRatio > 0.5f && fear < 0.3f) {
            state.featureDisplay = smoothStep(0.5f, 0.8f, energyRatio) * (1.0f - fear);
        } else {
            state.featureDisplay = 0.2f;
        }
    }

    // Bioluminescence
    if (genes.primaryFeature == FeatureType::BIOLUMINESCENCE) {
        state.glowIntensity = 0.5f + 0.5f * std::sin(age * 2.0f);
    }

    // Pupil dilation - dilates in low light, fear, or excitement
    state.pupilDilation = 0.5f + fear * 0.3f;

    // Get breathing and posture parameters
    auto breathParams = AnimationParams::getBreathingParams(state.stateFlags, 1.0f - energyRatio);
    state.breathingRate = breathParams.rate;
    state.breathingDepth = breathParams.depth;

    auto postureParams = AnimationParams::getPostureParams(state.stateFlags, energyRatio, fear);
    state.postureSlump = postureParams.slump;
    state.crouchFactor = postureParams.crouch;
    state.archBack = postureParams.arch;
    state.headDroop = -postureParams.headPosition; // Invert for droop

    // Trembling
    state.trembleIntensity = AnimationParams::getTrembleIntensity(state.stateFlags, fear, 0.0f);

    // Animation speed based on energy
    state.animationSpeed = 0.5f + energyRatio * 0.8f;

    // Effect flags
    state.showInjuryEffect = hasFlag(state.stateFlags, VisualStateFlag::INJURED);
    state.showHungerEffect = hasFlag(state.stateFlags, VisualStateFlag::STARVING);
    state.showFearEffect = hasFlag(state.stateFlags, VisualStateFlag::AFRAID);
    state.showExhaustionEffect = hasFlag(state.stateFlags, VisualStateFlag::EXHAUSTED);
}

void VisualStateCalculator::applyInjuryEffects(VisualState& state, float health) {
    // Health 0-100, lower = more injured
    float injurySeverity = 1.0f - (health / 100.0f);

    if (injurySeverity > 0.1f) {
        // Tint towards red
        glm::vec3 tint = ColorSchemes::injuryTint(injurySeverity);
        state.tintColor = glm::mix(state.tintColor, tint, 0.5f);
        state.tintStrength = std::max(state.tintStrength, injurySeverity * 0.3f);

        // Wound visibility
        state.woundVisibility = injurySeverity;

        // Movement affected
        state.animationSpeed *= (1.0f - injurySeverity * 0.3f);

        // Posture affected
        state.postureSlump = std::max(state.postureSlump, injurySeverity * 0.4f);
    }
}

void VisualStateCalculator::applyEnergyEffects(VisualState& state, float energyRatio) {
    // Low energy effects
    if (energyRatio < 0.5f) {
        float severity = 1.0f - (energyRatio / 0.5f);

        // Desaturate and darken
        state.saturationMultiplier *= (1.0f - severity * 0.4f);
        state.brightnessMultiplier *= (1.0f - severity * 0.2f);

        // Posture slumps
        state.postureSlump = std::max(state.postureSlump, severity * 0.5f);
        state.tailDroop = std::max(state.tailDroop, severity * 0.6f);

        // Animation slows
        state.animationSpeed *= (1.0f - severity * 0.4f);

        // Breathing changes
        if (energyRatio < 0.3f) {
            state.breathingRate *= 1.3f;
            state.breathingDepth *= 0.8f;
        }
    }

    // High energy - vibrant
    if (energyRatio > 0.8f) {
        float vitality = (energyRatio - 0.8f) / 0.2f;
        state.saturationMultiplier *= (1.0f + vitality * 0.1f);
        state.brightnessMultiplier *= (1.0f + vitality * 0.05f);
    }
}

void VisualStateCalculator::applyFearEffects(VisualState& state, float fear) {
    if (fear > 0.1f) {
        // Pale tint
        glm::vec3 fearTint = ColorSchemes::fearTint(fear);
        state.tintColor = glm::mix(state.tintColor, fearTint, fear * 0.5f);
        state.tintStrength = std::max(state.tintStrength, fear * 0.2f);

        // Trembling
        state.trembleIntensity = std::max(state.trembleIntensity, fear * 0.5f);

        // Crouched posture
        state.crouchFactor = std::max(state.crouchFactor, fear * 0.4f);

        // Wide-eyed
        state.pupilDilation = std::max(state.pupilDilation, 0.5f + fear * 0.4f);

        // Rapid breathing
        state.breathingRate *= (1.0f + fear * 0.5f);

        // Retracted features (make yourself small)
        state.featureDisplay *= (1.0f - fear * 0.5f);

        // Wider stance for stability
        state.legSpread = 1.0f + fear * 0.3f;
    }
}

void VisualStateCalculator::applyAggressionEffects(VisualState& state) {
    // Darker, reddish tint
    glm::vec3 aggressionTint = ColorSchemes::aggressionTint(1.0f);
    state.tintColor = glm::mix(state.tintColor, aggressionTint, 0.3f);
    state.tintStrength = std::max(state.tintStrength, 0.2f);

    // Display features prominently
    state.featureDisplay = 1.0f;

    // Arched back, raised head
    state.archBack = std::max(state.archBack, 0.4f);
    state.headDroop = -0.3f; // Head raised

    // Heavy breathing
    state.breathingRate *= 1.3f;
    state.breathingDepth *= 1.2f;

    // Slightly contracted pupils
    state.pupilDilation = 0.3f;
}

void VisualStateCalculator::applyAgeEffects(VisualState& state, float age, float maxAge) {
    float normalizedAge = std::min(1.0f, age / maxAge);

    // Brightness decreases with age
    float ageBright = ColorSchemes::ageBrightness(normalizedAge);
    state.brightnessMultiplier *= ageBright;

    // Old creatures are slightly desaturated
    if (normalizedAge > 0.7f) {
        float oldAge = (normalizedAge - 0.7f) / 0.3f;
        state.saturationMultiplier *= (1.0f - oldAge * 0.2f);

        // Posture affected
        state.postureSlump = std::max(state.postureSlump, oldAge * 0.3f);

        // Movement slower
        state.animationSpeed *= (1.0f - oldAge * 0.2f);

        // Show age through coat
        state.coatCondition *= (1.0f - oldAge * 0.3f);

        // Scars accumulate
        state.scarVisibility = oldAge * 0.4f;
    }

    // Young creatures are slightly brighter
    if (normalizedAge < 0.2f) {
        float youth = 1.0f - (normalizedAge / 0.2f);
        state.brightnessMultiplier *= (1.0f + youth * 0.1f);
        state.saturationMultiplier *= (1.0f + youth * 0.05f);
    }
}

void VisualStateCalculator::applyMetamorphosisEffects(VisualState& state, float progress) {
    // During metamorphosis, creature looks different
    state.stateFlags = state.stateFlags | VisualStateFlag::METAMORPHOSING;

    // Shimmer/glow effect
    state.glowIntensity = std::sin(progress * 3.14159f) * 0.5f;

    // Color shift during transformation
    float colorShift = std::sin(progress * 3.14159f * 2.0f);
    state.tintColor = glm::vec3(
        1.0f + colorShift * 0.1f,
        1.0f - colorShift * 0.05f,
        1.0f + colorShift * 0.15f
    );
    state.tintStrength = 0.3f;

    // Posture changes during transformation
    state.crouchFactor = std::sin(progress * 3.14159f) * 0.3f;

    // Trembling from the transformation
    state.trembleIntensity = 0.2f + std::sin(progress * 10.0f) * 0.1f;
}

glm::vec3 VisualStateCalculator::getFinalColor(const VisualState& state) {
    glm::vec3 color = state.baseColor;

    // Apply saturation
    float gray = (color.r + color.g + color.b) / 3.0f;
    color = glm::mix(glm::vec3(gray), color, state.saturationMultiplier);

    // Apply brightness
    color *= state.brightnessMultiplier;

    // Apply tint
    if (state.tintStrength > 0.0f) {
        color = glm::mix(color, color * state.tintColor, state.tintStrength);
    }

    // Clamp to valid range
    color = glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f));

    return color;
}

glm::mat4 VisualStateCalculator::getPostureTransform(const VisualState& state) {
    glm::mat4 transform = glm::mat4(1.0f);

    // Apply crouch (scale down Y)
    if (state.crouchFactor > 0.0f) {
        float scaleY = 1.0f - state.crouchFactor * 0.3f;
        transform = glm::scale(transform, glm::vec3(1.0f, scaleY, 1.0f));
    }

    // Apply slump (rotate forward)
    if (state.postureSlump > 0.0f) {
        float angle = state.postureSlump * 0.2f; // radians
        transform = glm::rotate(transform, angle, glm::vec3(1.0f, 0.0f, 0.0f));
    }

    // Apply back arch
    if (state.archBack > 0.0f) {
        float angle = -state.archBack * 0.15f;
        transform = glm::rotate(transform, angle, glm::vec3(1.0f, 0.0f, 0.0f));
    }

    return transform;
}

float VisualStateCalculator::smoothStep(float edge0, float edge1, float x) {
    x = std::max(0.0f, std::min(1.0f, (x - edge0) / (edge1 - edge0)));
    return x * x * (3.0f - 2.0f * x);
}
