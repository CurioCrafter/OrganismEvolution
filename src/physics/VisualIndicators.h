#pragma once

#include <glm/glm.hpp>
#include <algorithm>
#include "Morphology.h"

// =============================================================================
// VISUAL INDICATORS SYSTEM
// Visual representation of creature health, fitness, and state
// =============================================================================

// Visual state flags
enum class VisualStateFlag : unsigned int {
    NONE            = 0,
    INJURED         = 1 << 0,   // Showing damage
    STARVING        = 1 << 1,   // Very low energy
    EXHAUSTED       = 1 << 2,   // Low stamina
    AFRAID          = 1 << 3,   // Fear response
    AGGRESSIVE      = 1 << 4,   // Attack posture
    ALERT           = 1 << 5,   // Heightened awareness
    RELAXED         = 1 << 6,   // Safe and resting
    MATING_DISPLAY  = 1 << 7,   // Attracting mates
    CARRYING_FOOD   = 1 << 8,   // Has food
    METAMORPHOSING  = 1 << 9,   // During transformation
    DOMINANT        = 1 << 10,  // Alpha/leader
    SUBMISSIVE      = 1 << 11   // Subordinate
};

inline VisualStateFlag operator|(VisualStateFlag a, VisualStateFlag b) {
    return static_cast<VisualStateFlag>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}

inline VisualStateFlag operator&(VisualStateFlag a, VisualStateFlag b) {
    return static_cast<VisualStateFlag>(static_cast<unsigned int>(a) & static_cast<unsigned int>(b));
}

inline bool hasFlag(VisualStateFlag flags, VisualStateFlag flag) {
    return (static_cast<unsigned int>(flags) & static_cast<unsigned int>(flag)) != 0;
}

// =============================================================================
// VISUAL STATE
// Complete visual representation of creature state
// =============================================================================

struct VisualState {
    // === Color Modulation ===
    glm::vec3 baseColor = glm::vec3(0.6f, 0.5f, 0.4f);
    float saturationMultiplier = 1.0f;    // 0=gray, 1=full color
    float brightnessMultiplier = 1.0f;    // <1 = darker, >1 = lighter
    glm::vec3 tintColor = glm::vec3(1.0f); // Overlay color
    float tintStrength = 0.0f;            // 0=no tint, 1=full tint

    // === Posture ===
    float postureSlump = 0.0f;            // 0=upright, 1=fully slumped
    float headDroop = 0.0f;               // Head hang (tired/sad)
    float tailDroop = 0.0f;               // Tail position
    float legSpread = 1.0f;               // Stance width (fear spreads legs)
    float crouchFactor = 0.0f;            // 0=standing, 1=crouched
    float archBack = 0.0f;                // Defensive arch

    // === Animation ===
    float animationSpeed = 1.0f;          // Movement speed multiplier
    float breathingRate = 1.0f;           // Breathing animation speed
    float breathingDepth = 1.0f;          // Breathing amplitude
    float trembleIntensity = 0.0f;        // Shaking/trembling
    float swayAmount = 0.0f;              // Idle swaying

    // === Specific Indicators ===
    float woundVisibility = 0.0f;         // 0=no wounds, 1=severe
    float scarVisibility = 0.0f;          // Old injuries
    float muscleDefinition = 0.5f;        // 0=thin, 1=bulky
    float fatStorage = 0.5f;              // 0=emaciated, 1=well-fed
    float coatCondition = 1.0f;           // 0=ragged, 1=healthy

    // === Display Features ===
    float featureDisplay = 0.0f;          // 0=retracted, 1=fully displayed
    float featureColorIntensity = 1.0f;   // Color of display features
    float pupilDilation = 0.5f;           // 0=contracted, 1=dilated

    // === Effects ===
    bool showInjuryEffect = false;
    bool showHungerEffect = false;
    bool showFearEffect = false;
    bool showAggressionEffect = false;
    bool showExhaustionEffect = false;
    float glowIntensity = 0.0f;           // For bioluminescence

    // Active state flags
    VisualStateFlag stateFlags = VisualStateFlag::NONE;
};

// =============================================================================
// VISUAL STATE CALCULATOR
// Calculate visual state from creature condition
// =============================================================================

class VisualStateCalculator {
public:
    // Update visual state based on creature status
    static void update(
        VisualState& state,
        float energy,           // 0-200 typical
        float maxEnergy,        // Maximum energy
        float health,           // 0-100
        float fear,             // 0-1
        float age,              // Creature age
        float fitness,          // Overall fitness score
        const MorphologyGenes& genes,
        LifeStage lifeStage
    );

    // Apply specific state effects
    static void applyInjuryEffects(VisualState& state, float health);
    static void applyEnergyEffects(VisualState& state, float energyRatio);
    static void applyFearEffects(VisualState& state, float fear);
    static void applyAggressionEffects(VisualState& state);
    static void applyAgeEffects(VisualState& state, float age, float maxAge);
    static void applyMetamorphosisEffects(VisualState& state, float progress);

    // Get final color after all modulations
    static glm::vec3 getFinalColor(const VisualState& state);

    // Get posture transformation matrix
    static glm::mat4 getPostureTransform(const VisualState& state);

private:
    static float smoothStep(float edge0, float edge1, float x);
};

// =============================================================================
// COLOR SCHEMES
// Predefined color schemes for different states
// =============================================================================

namespace ColorSchemes {
    // Injury colors (red tints)
    inline glm::vec3 injuryTint(float severity) {
        return glm::mix(glm::vec3(1.0f), glm::vec3(0.8f, 0.3f, 0.3f), severity);
    }

    // Hunger colors (desaturated, darker)
    inline glm::vec3 hungerModulation(float hungerLevel) {
        float sat = 1.0f - hungerLevel * 0.5f;
        float bright = 1.0f - hungerLevel * 0.3f;
        return glm::vec3(sat, bright, 1.0f); // sat, bright, unused
    }

    // Fear colors (pale)
    inline glm::vec3 fearTint(float fear) {
        return glm::mix(glm::vec3(1.0f), glm::vec3(0.8f, 0.85f, 0.9f), fear);
    }

    // Aggression colors (darker, reddish)
    inline glm::vec3 aggressionTint(float aggression) {
        return glm::mix(glm::vec3(1.0f), glm::vec3(1.1f, 0.7f, 0.7f), aggression);
    }

    // Health colors (vibrant = healthy, dull = sick)
    inline float healthSaturation(float health) {
        return 0.5f + health * 0.5f; // 0.5-1.0 range
    }

    // Age colors (younger = brighter)
    inline float ageBrightness(float normalizedAge) {
        return 1.1f - normalizedAge * 0.3f; // Gets duller with age
    }

    // Dominant display (brighter, more saturated)
    inline glm::vec3 dominanceModulation(float dominance) {
        return glm::vec3(
            1.0f + dominance * 0.2f,  // Brightness
            1.0f + dominance * 0.3f,  // Saturation
            1.0f
        );
    }
}

// =============================================================================
// ANIMATION PARAMETERS
// Parameters for procedural animations
// =============================================================================

namespace AnimationParams {
    // Breathing parameters by state
    struct BreathingParams {
        float rate;     // Cycles per second
        float depth;    // Amplitude
        float pattern;  // 0=regular, 1=irregular
    };

    inline BreathingParams getBreathingParams(VisualStateFlag flags, float exertion) {
        BreathingParams params = {1.0f, 1.0f, 0.0f};

        if (hasFlag(flags, VisualStateFlag::EXHAUSTED)) {
            params.rate = 2.5f;
            params.depth = 1.5f;
            params.pattern = 0.3f;
        } else if (hasFlag(flags, VisualStateFlag::AFRAID)) {
            params.rate = 2.0f;
            params.depth = 0.8f;
            params.pattern = 0.5f;
        } else if (hasFlag(flags, VisualStateFlag::RELAXED)) {
            params.rate = 0.5f;
            params.depth = 0.7f;
            params.pattern = 0.0f;
        } else if (hasFlag(flags, VisualStateFlag::AGGRESSIVE)) {
            params.rate = 1.5f;
            params.depth = 1.2f;
            params.pattern = 0.1f;
        }

        // Modify by exertion
        params.rate *= (1.0f + exertion * 0.5f);
        params.depth *= (1.0f + exertion * 0.3f);

        return params;
    }

    // Trembling parameters
    inline float getTrembleIntensity(VisualStateFlag flags, float fear, float cold) {
        float tremble = 0.0f;

        if (hasFlag(flags, VisualStateFlag::AFRAID)) {
            tremble += fear * 0.5f;
        }
        if (hasFlag(flags, VisualStateFlag::INJURED)) {
            tremble += 0.2f;
        }
        if (hasFlag(flags, VisualStateFlag::STARVING)) {
            tremble += 0.3f;
        }
        tremble += cold * 0.4f;

        return std::min(1.0f, tremble);
    }

    // Posture parameters
    struct PostureParams {
        float slump;
        float crouch;
        float arch;
        float headPosition; // -1=down, 0=neutral, 1=up
    };

    inline PostureParams getPostureParams(VisualStateFlag flags, float energy, float fear) {
        PostureParams params = {0.0f, 0.0f, 0.0f, 0.0f};

        if (hasFlag(flags, VisualStateFlag::EXHAUSTED)) {
            params.slump = 0.6f;
            params.headPosition = -0.5f;
        }
        if (hasFlag(flags, VisualStateFlag::STARVING)) {
            params.slump = 0.4f;
        }
        if (hasFlag(flags, VisualStateFlag::AFRAID)) {
            params.crouch = 0.5f;
            params.headPosition = -0.3f;
        }
        if (hasFlag(flags, VisualStateFlag::AGGRESSIVE)) {
            params.arch = 0.3f;
            params.headPosition = 0.2f;
        }
        if (hasFlag(flags, VisualStateFlag::ALERT)) {
            params.headPosition = 0.3f;
        }
        if (hasFlag(flags, VisualStateFlag::DOMINANT)) {
            params.headPosition = 0.5f;
            params.arch = 0.1f;
        }
        if (hasFlag(flags, VisualStateFlag::SUBMISSIVE)) {
            params.crouch = 0.4f;
            params.headPosition = -0.6f;
        }

        // Energy affects posture
        float energyRatio = energy;
        params.slump += (1.0f - energyRatio) * 0.4f;

        return params;
    }
}
