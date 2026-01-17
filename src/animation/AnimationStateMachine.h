#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>

// Forward declarations
struct MorphologyGenes;

namespace animation {

// Forward declarations
class Skeleton;
struct SkeletonPose;
class IKSystem;
class ProceduralLocomotion;
class SwimAnimator;
class WingAnimator;
class SecondaryMotionSystem;
class ExpressionSystem;

// =============================================================================
// ANIMATION STATE DEFINITIONS
// =============================================================================

// High-level creature activity states
enum class ActivityState {
    IDLE,               // Standing/resting
    LOCOMOTION,         // Walking, running, swimming, flying
    ATTACKING,          // Combat actions
    FEEDING,            // Eating behavior
    FLEEING,            // Running away
    SOCIALIZING,        // Interacting with others
    SLEEPING,           // Dormant state
    DYING,              // Death animation
    DEAD                // Post-death
};

// Locomotion sub-states
enum class LocomotionMode {
    NONE,
    WALKING,
    RUNNING,
    SPRINTING,
    SNEAKING,
    SWIMMING,
    FLYING,
    GLIDING,
    HOVERING,
    CLIMBING,
    JUMPING,
    FALLING
};

// Attack sub-states
enum class AttackType {
    NONE,
    BITE,
    CLAW,
    TAIL_STRIKE,
    CHARGE,
    POUNCE,
    STING,
    CONSTRICT
};

// Transition blend mode
enum class BlendMode {
    LINEAR,             // Simple linear interpolation
    SMOOTH_STEP,        // Smooth ease in/out
    SPRING,             // Spring-based overshoot
    FROZEN,             // Keep current pose until blend complete
    ADDITIVE            // Add new animation on top
};

// =============================================================================
// ANIMATION STATE
// =============================================================================

struct AnimationState {
    std::string name;
    ActivityState activity = ActivityState::IDLE;
    LocomotionMode locomotion = LocomotionMode::NONE;
    AttackType attack = AttackType::NONE;

    // Animation timing
    float duration = 1.0f;              // Total animation duration
    float speed = 1.0f;                 // Playback speed multiplier
    bool isLooping = true;              // Does animation loop?

    // Blend parameters
    float blendInTime = 0.2f;           // Time to blend into this state
    float blendOutTime = 0.2f;          // Time to blend out of this state
    BlendMode blendMode = BlendMode::SMOOTH_STEP;

    // Animation weights for different systems
    float locomotionWeight = 1.0f;
    float ikWeight = 1.0f;
    float secondaryMotionWeight = 1.0f;
    float expressionWeight = 1.0f;

    // Pose adjustments
    glm::vec3 rootOffset{0.0f};
    glm::quat rootRotation{1.0f, 0.0f, 0.0f, 0.0f};
    float bodyLeanAmount = 0.0f;
};

// =============================================================================
// STATE TRANSITION
// =============================================================================

// Condition for state transition
using TransitionCondition = std::function<bool(const class AnimationStateMachine&)>;

struct StateTransition {
    std::string fromState;
    std::string toState;
    TransitionCondition condition;
    float transitionTime = 0.2f;
    BlendMode blendMode = BlendMode::SMOOTH_STEP;
    int priority = 0;                   // Higher priority transitions checked first

    // Interrupt settings
    bool canInterrupt = true;           // Can be interrupted by higher priority
    bool interruptsOthers = false;      // Forces interrupt of current transition
};

// =============================================================================
// ANIMATION LAYER
// =============================================================================

// Animation layers for blending multiple animations
struct AnimationLayer {
    std::string name;
    float weight = 1.0f;
    BlendMode blendMode = BlendMode::LINEAR;

    // Bone mask (which bones this layer affects)
    std::vector<int> affectedBones;     // Empty = all bones
    bool isAdditive = false;

    // Current state in this layer
    std::string currentState;
    float stateTime = 0.0f;
};

// =============================================================================
// ANIMATION STATE MACHINE
// =============================================================================

class AnimationStateMachine {
public:
    AnimationStateMachine();
    ~AnimationStateMachine() = default;

    // Initialize from morphology
    void initializeFromMorphology(const MorphologyGenes& genes);

    // State management
    void addState(const AnimationState& state);
    void removeState(const std::string& name);
    bool hasState(const std::string& name) const;
    const AnimationState* getState(const std::string& name) const;

    // Transition management
    void addTransition(const StateTransition& transition);
    void removeTransition(const std::string& from, const std::string& to);

    // Layer management
    void addLayer(const AnimationLayer& layer);
    void setLayerWeight(const std::string& layerName, float weight);
    void setLayerState(const std::string& layerName, const std::string& stateName);

    // State control
    void setState(const std::string& stateName, bool immediate = false);
    void forceState(const std::string& stateName);  // Immediate, no blend
    std::string getCurrentState() const { return m_currentState; }
    std::string getPreviousState() const { return m_previousState; }
    bool isInTransition() const { return m_isTransitioning; }
    float getTransitionProgress() const { return m_transitionProgress; }

    // Input parameters (affect state transitions)
    void setVelocity(const glm::vec3& velocity);
    void setTargetPosition(const glm::vec3& target);
    void setGroundDistance(float distance);
    void setWaterDepth(float depth);
    void setIsSwimming(bool swimming);
    void setIsFlying(bool flying);
    void setThreatLevel(float threat);
    void setStamina(float stamina);
    void setHealth(float health);

    // Update state machine
    void update(float deltaTime);

    // Apply to animation systems
    void applyToSkeleton(
        const Skeleton& skeleton,
        SkeletonPose& pose,
        ProceduralLocomotion* locomotion,
        SwimAnimator* swimAnimator,
        WingAnimator* wingAnimator,
        SecondaryMotionSystem* secondaryMotion,
        ExpressionSystem* expression
    );

    // Get current animation weights for different systems
    float getLocomotionWeight() const;
    float getIKWeight() const;
    float getSecondaryMotionWeight() const;
    float getExpressionWeight() const;

    // Get root motion
    glm::vec3 getRootOffset() const;
    glm::quat getRootRotation() const;

    // State queries
    ActivityState getActivity() const { return m_currentActivity; }
    LocomotionMode getLocomotionMode() const { return m_currentLocomotion; }
    bool isMoving() const;
    bool isGrounded() const;
    bool isAirborne() const;
    bool isInWater() const;

    // Debug
    std::vector<std::string> getAvailableTransitions() const;
    std::string getDebugInfo() const;

private:
    // States and transitions
    std::unordered_map<std::string, AnimationState> m_states;
    std::vector<StateTransition> m_transitions;
    std::vector<AnimationLayer> m_layers;

    // Current state
    std::string m_currentState = "idle";
    std::string m_previousState;
    std::string m_nextState;
    ActivityState m_currentActivity = ActivityState::IDLE;
    LocomotionMode m_currentLocomotion = LocomotionMode::NONE;

    // Transition state
    bool m_isTransitioning = false;
    float m_transitionProgress = 0.0f;
    float m_transitionDuration = 0.2f;
    BlendMode m_transitionBlendMode = BlendMode::SMOOTH_STEP;

    // Animation timing
    float m_stateTime = 0.0f;

    // Input parameters
    glm::vec3 m_velocity{0.0f};
    glm::vec3 m_targetPosition{0.0f};
    float m_groundDistance = 0.0f;
    float m_waterDepth = 0.0f;
    bool m_isSwimming = false;
    bool m_isFlying = false;
    float m_threatLevel = 0.0f;
    float m_stamina = 1.0f;
    float m_health = 1.0f;

    // Morphology data
    bool m_canFly = false;
    bool m_canSwim = true;
    bool m_isAquatic = false;
    int m_legCount = 4;

    // Internal methods
    void checkTransitions();
    void updateTransition(float deltaTime);
    void updateState(float deltaTime);
    float calculateBlendWeight(float t, BlendMode mode) const;
    void blendStates(const AnimationState& from, const AnimationState& to, float t);

    // Create default states based on morphology
    void createDefaultStates();
    void createLocomotionStates();
    void createCombatStates();
    void createSocialStates();
};

// =============================================================================
// STATE PRESETS
// =============================================================================

namespace StatePresets {

    // Default idle state
    AnimationState idleState();

    // Walking states
    AnimationState walkState();
    AnimationState runState();
    AnimationState sprintState();

    // Swimming states
    AnimationState swimIdleState();
    AnimationState swimState();
    AnimationState swimFastState();

    // Flying states
    AnimationState flyState();
    AnimationState glideState();
    AnimationState hoverState();

    // Combat states
    AnimationState attackState();
    AnimationState defendState();

    // Social states
    AnimationState greetState();
    AnimationState threatDisplayState();

    // Misc states
    AnimationState sleepState();
    AnimationState deathState();
}

// =============================================================================
// TRANSITION CONDITIONS
// =============================================================================

namespace TransitionConditions {

    // Movement-based
    TransitionCondition velocityAbove(float threshold);
    TransitionCondition velocityBelow(float threshold);
    TransitionCondition isMoving();
    TransitionCondition isStopped();

    // Environment-based
    TransitionCondition isInWater();
    TransitionCondition isNotInWater();
    TransitionCondition isAirborne();
    TransitionCondition isGrounded();

    // Status-based
    TransitionCondition healthBelow(float threshold);
    TransitionCondition staminaBelow(float threshold);
    TransitionCondition threatAbove(float threshold);

    // Time-based
    TransitionCondition stateTimeExceeds(float duration);

    // Composite conditions
    TransitionCondition allOf(std::vector<TransitionCondition> conditions);
    TransitionCondition anyOf(std::vector<TransitionCondition> conditions);
    TransitionCondition noneOf(std::vector<TransitionCondition> conditions);
}

// =============================================================================
// MORPHOLOGY-DRIVEN STATE MACHINE BUILDER
// =============================================================================

class MorphologyStateMachineBuilder {
public:
    static AnimationStateMachine buildFromMorphology(const MorphologyGenes& genes);

private:
    static void addBipedStates(AnimationStateMachine& sm, const MorphologyGenes& genes);
    static void addQuadrupedStates(AnimationStateMachine& sm, const MorphologyGenes& genes);
    static void addHexapodStates(AnimationStateMachine& sm, const MorphologyGenes& genes);
    static void addAquaticStates(AnimationStateMachine& sm, const MorphologyGenes& genes);
    static void addFlyingStates(AnimationStateMachine& sm, const MorphologyGenes& genes);
    static void addSerpentineStates(AnimationStateMachine& sm, const MorphologyGenes& genes);

    static void addCommonTransitions(AnimationStateMachine& sm);
};

} // namespace animation
