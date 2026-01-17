#include "AnimationStateMachine.h"
#include "Skeleton.h"
#include "Pose.h"
#include "ProceduralLocomotion.h"
#include "SwimAnimator.h"
#include "WingAnimator.h"
#include "SecondaryMotion.h"
#include "ExpressionSystem.h"
#include "../physics/Morphology.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace animation {

// Constants
constexpr float PI = 3.14159265358979323846f;

// =============================================================================
// ANIMATION STATE MACHINE IMPLEMENTATION
// =============================================================================

AnimationStateMachine::AnimationStateMachine() {
    // Add default idle state
    m_states["idle"] = StatePresets::idleState();
    m_currentState = "idle";
}

void AnimationStateMachine::initializeFromMorphology(const MorphologyGenes& genes) {
    // Store morphology data
    m_canFly = genes.canFly;
    m_canSwim = genes.canSwim;
    m_isAquatic = genes.isAquatic;
    m_legCount = genes.legCount;

    // Create appropriate states based on body plan
    createDefaultStates();

    // Use builder for more complex setup
    *this = MorphologyStateMachineBuilder::buildFromMorphology(genes);
}

void AnimationStateMachine::addState(const AnimationState& state) {
    m_states[state.name] = state;
}

void AnimationStateMachine::removeState(const std::string& name) {
    if (name != "idle") {  // Never remove idle state
        m_states.erase(name);
    }
}

bool AnimationStateMachine::hasState(const std::string& name) const {
    return m_states.find(name) != m_states.end();
}

const AnimationState* AnimationStateMachine::getState(const std::string& name) const {
    auto it = m_states.find(name);
    if (it != m_states.end()) {
        return &it->second;
    }
    return nullptr;
}

void AnimationStateMachine::addTransition(const StateTransition& transition) {
    // Insert sorted by priority (highest first)
    auto it = std::lower_bound(m_transitions.begin(), m_transitions.end(), transition,
        [](const StateTransition& a, const StateTransition& b) {
            return a.priority > b.priority;
        });
    m_transitions.insert(it, transition);
}

void AnimationStateMachine::removeTransition(const std::string& from, const std::string& to) {
    m_transitions.erase(
        std::remove_if(m_transitions.begin(), m_transitions.end(),
            [&](const StateTransition& t) {
                return t.fromState == from && t.toState == to;
            }),
        m_transitions.end()
    );
}

void AnimationStateMachine::addLayer(const AnimationLayer& layer) {
    m_layers.push_back(layer);
}

void AnimationStateMachine::setLayerWeight(const std::string& layerName, float weight) {
    for (auto& layer : m_layers) {
        if (layer.name == layerName) {
            layer.weight = std::clamp(weight, 0.0f, 1.0f);
            break;
        }
    }
}

void AnimationStateMachine::setLayerState(const std::string& layerName, const std::string& stateName) {
    for (auto& layer : m_layers) {
        if (layer.name == layerName) {
            if (hasState(stateName)) {
                layer.currentState = stateName;
                layer.stateTime = 0.0f;
            }
            break;
        }
    }
}

void AnimationStateMachine::setState(const std::string& stateName, bool immediate) {
    if (!hasState(stateName) || stateName == m_currentState) {
        return;
    }

    if (immediate) {
        forceState(stateName);
    } else if (!m_isTransitioning) {
        // Start transition
        m_previousState = m_currentState;
        m_nextState = stateName;
        m_isTransitioning = true;
        m_transitionProgress = 0.0f;

        const AnimationState* nextState = getState(stateName);
        if (nextState) {
            m_transitionDuration = nextState->blendInTime;
            m_transitionBlendMode = nextState->blendMode;
        }
    }
}

void AnimationStateMachine::forceState(const std::string& stateName) {
    if (!hasState(stateName)) {
        return;
    }

    m_previousState = m_currentState;
    m_currentState = stateName;
    m_nextState.clear();
    m_isTransitioning = false;
    m_transitionProgress = 0.0f;
    m_stateTime = 0.0f;

    // Update activity/locomotion mode
    const AnimationState* state = getState(stateName);
    if (state) {
        m_currentActivity = state->activity;
        m_currentLocomotion = state->locomotion;
    }
}

void AnimationStateMachine::setVelocity(const glm::vec3& velocity) {
    m_velocity = velocity;
}

void AnimationStateMachine::setTargetPosition(const glm::vec3& target) {
    m_targetPosition = target;
}

void AnimationStateMachine::setGroundDistance(float distance) {
    m_groundDistance = distance;
}

void AnimationStateMachine::setWaterDepth(float depth) {
    m_waterDepth = depth;
}

void AnimationStateMachine::setIsSwimming(bool swimming) {
    m_isSwimming = swimming;
}

void AnimationStateMachine::setIsFlying(bool flying) {
    m_isFlying = flying;
}

void AnimationStateMachine::setThreatLevel(float threat) {
    m_threatLevel = std::clamp(threat, 0.0f, 1.0f);
}

void AnimationStateMachine::setStamina(float stamina) {
    m_stamina = std::clamp(stamina, 0.0f, 1.0f);
}

void AnimationStateMachine::setHealth(float health) {
    m_health = std::clamp(health, 0.0f, 1.0f);
}

void AnimationStateMachine::update(float deltaTime) {
    // Check for state transitions
    if (!m_isTransitioning) {
        checkTransitions();
    }

    // Update transition
    if (m_isTransitioning) {
        updateTransition(deltaTime);
    }

    // Update current state
    updateState(deltaTime);

    // Update layers
    for (auto& layer : m_layers) {
        layer.stateTime += deltaTime;
    }
}

void AnimationStateMachine::checkTransitions() {
    for (const auto& transition : m_transitions) {
        if (transition.fromState != m_currentState) {
            continue;
        }

        if (transition.condition && transition.condition(*this)) {
            setState(transition.toState);
            m_transitionDuration = transition.transitionTime;
            m_transitionBlendMode = transition.blendMode;
            break;  // Only one transition at a time
        }
    }
}

void AnimationStateMachine::updateTransition(float deltaTime) {
    if (m_transitionDuration > 0.0f) {
        m_transitionProgress += deltaTime / m_transitionDuration;
    } else {
        m_transitionProgress = 1.0f;
    }

    if (m_transitionProgress >= 1.0f) {
        // Transition complete
        m_currentState = m_nextState;
        m_nextState.clear();
        m_isTransitioning = false;
        m_transitionProgress = 0.0f;
        m_stateTime = 0.0f;

        // Update activity/locomotion
        const AnimationState* state = getState(m_currentState);
        if (state) {
            m_currentActivity = state->activity;
            m_currentLocomotion = state->locomotion;
        }
    }
}

void AnimationStateMachine::updateState(float deltaTime) {
    const AnimationState* state = getState(m_currentState);
    if (!state) return;

    // Update state time
    m_stateTime += deltaTime * state->speed;

    // Handle looping
    if (state->isLooping && m_stateTime >= state->duration) {
        m_stateTime = std::fmod(m_stateTime, state->duration);
    }
}

float AnimationStateMachine::calculateBlendWeight(float t, BlendMode mode) const {
    t = std::clamp(t, 0.0f, 1.0f);

    switch (mode) {
        case BlendMode::LINEAR:
            return t;

        case BlendMode::SMOOTH_STEP:
            return t * t * (3.0f - 2.0f * t);

        case BlendMode::SPRING: {
            // Damped spring overshoot
            float omega = 10.0f;
            float zeta = 0.5f;
            float dampedT = 1.0f - std::exp(-omega * t) * std::cos(omega * std::sqrt(1.0f - zeta * zeta) * t);
            return std::clamp(dampedT, 0.0f, 1.0f);
        }

        case BlendMode::FROZEN:
            return t < 1.0f ? 0.0f : 1.0f;

        case BlendMode::ADDITIVE:
            return t;

        default:
            return t;
    }
}

void AnimationStateMachine::blendStates(const AnimationState& from, const AnimationState& to, float t) {
    // Calculate blend weight
    float weight = calculateBlendWeight(t, m_transitionBlendMode);

    // This is called during applyToSkeleton to blend parameters
    // The actual blending happens there
}

void AnimationStateMachine::applyToSkeleton(
    const Skeleton& skeleton,
    SkeletonPose& pose,
    ProceduralLocomotion* locomotion,
    SwimAnimator* swimAnimator,
    WingAnimator* wingAnimator,
    SecondaryMotionSystem* secondaryMotion,
    ExpressionSystem* expression
) {
    const AnimationState* currentState = getState(m_currentState);
    if (!currentState) return;

    float blendWeight = 1.0f;
    const AnimationState* previousState = nullptr;

    if (m_isTransitioning) {
        blendWeight = calculateBlendWeight(m_transitionProgress, m_transitionBlendMode);
        previousState = getState(m_previousState);
    }

    // Calculate blended weights
    float locomotionWeight = currentState->locomotionWeight;
    float ikWeight = currentState->ikWeight;
    float secondaryWeight = currentState->secondaryMotionWeight;
    float expressionWeight = currentState->expressionWeight;

    if (previousState && m_isTransitioning) {
        locomotionWeight = glm::mix(previousState->locomotionWeight, currentState->locomotionWeight, blendWeight);
        ikWeight = glm::mix(previousState->ikWeight, currentState->ikWeight, blendWeight);
        secondaryWeight = glm::mix(previousState->secondaryMotionWeight, currentState->secondaryMotionWeight, blendWeight);
        expressionWeight = glm::mix(previousState->expressionWeight, currentState->expressionWeight, blendWeight);
    }

    // Apply locomotion based on mode
    if (locomotion && locomotionWeight > 0.01f) {
        // Locomotion system handles walk/run/etc
        locomotion->setVelocity(m_velocity * locomotionWeight);
    }

    // Apply swim animation
    if (swimAnimator && m_isSwimming && locomotionWeight > 0.01f) {
        float swimSpeed = 5.0f;  // Max swim speed
        swimAnimator->update(0.016f, m_velocity, swimSpeed);
    }

    // Apply wing animation
    if (wingAnimator && m_isFlying) {
        wingAnimator->setVelocity(glm::length(m_velocity));
        wingAnimator->setVerticalVelocity(m_velocity.y);
    }

    // Apply secondary motion
    if (secondaryMotion && secondaryWeight > 0.01f) {
        secondaryMotion->setOverallWeight(secondaryWeight);
    }

    // Apply expressions based on state
    if (expression && expressionWeight > 0.01f) {
        // Set expression based on activity
        switch (m_currentActivity) {
            case ActivityState::IDLE:
                expression->setExpression(ExpressionType::NEUTRAL, expressionWeight);
                break;
            case ActivityState::ATTACKING:
                expression->setExpression(ExpressionType::ANGRY, expressionWeight);
                break;
            case ActivityState::FLEEING:
                expression->setExpression(ExpressionType::FEARFUL, expressionWeight);
                break;
            case ActivityState::FEEDING:
                expression->setExpression(ExpressionType::HAPPY, expressionWeight);
                break;
            case ActivityState::SLEEPING:
                expression->setExpression(ExpressionType::SLEEPY, expressionWeight);
                break;
            default:
                break;
        }
    }

    // Apply layer animations
    for (const auto& layer : m_layers) {
        if (layer.weight < 0.01f) continue;

        const AnimationState* layerState = getState(layer.currentState);
        if (!layerState) continue;

        // Apply layer-specific animation
        // This would modify specific bones based on layer.affectedBones
    }
}

float AnimationStateMachine::getLocomotionWeight() const {
    const AnimationState* state = getState(m_currentState);
    if (!state) return 0.0f;

    if (m_isTransitioning) {
        const AnimationState* prev = getState(m_previousState);
        if (prev) {
            float t = calculateBlendWeight(m_transitionProgress, m_transitionBlendMode);
            return glm::mix(prev->locomotionWeight, state->locomotionWeight, t);
        }
    }
    return state->locomotionWeight;
}

float AnimationStateMachine::getIKWeight() const {
    const AnimationState* state = getState(m_currentState);
    return state ? state->ikWeight : 0.0f;
}

float AnimationStateMachine::getSecondaryMotionWeight() const {
    const AnimationState* state = getState(m_currentState);
    return state ? state->secondaryMotionWeight : 0.0f;
}

float AnimationStateMachine::getExpressionWeight() const {
    const AnimationState* state = getState(m_currentState);
    return state ? state->expressionWeight : 0.0f;
}

glm::vec3 AnimationStateMachine::getRootOffset() const {
    const AnimationState* state = getState(m_currentState);
    if (!state) return glm::vec3(0.0f);

    if (m_isTransitioning) {
        const AnimationState* prev = getState(m_previousState);
        if (prev) {
            float t = calculateBlendWeight(m_transitionProgress, m_transitionBlendMode);
            return glm::mix(prev->rootOffset, state->rootOffset, t);
        }
    }
    return state->rootOffset;
}

glm::quat AnimationStateMachine::getRootRotation() const {
    const AnimationState* state = getState(m_currentState);
    if (!state) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    if (m_isTransitioning) {
        const AnimationState* prev = getState(m_previousState);
        if (prev) {
            float t = calculateBlendWeight(m_transitionProgress, m_transitionBlendMode);
            return glm::slerp(prev->rootRotation, state->rootRotation, t);
        }
    }
    return state->rootRotation;
}

bool AnimationStateMachine::isMoving() const {
    return glm::length(m_velocity) > 0.1f;
}

bool AnimationStateMachine::isGrounded() const {
    return m_groundDistance < 0.1f && !m_isSwimming && !m_isFlying;
}

bool AnimationStateMachine::isAirborne() const {
    return m_groundDistance > 0.1f || m_isFlying;
}

bool AnimationStateMachine::isInWater() const {
    return m_waterDepth > 0.1f || m_isSwimming;
}

std::vector<std::string> AnimationStateMachine::getAvailableTransitions() const {
    std::vector<std::string> available;
    for (const auto& transition : m_transitions) {
        if (transition.fromState == m_currentState && transition.condition && transition.condition(*this)) {
            available.push_back(transition.toState);
        }
    }
    return available;
}

std::string AnimationStateMachine::getDebugInfo() const {
    std::stringstream ss;
    ss << "State: " << m_currentState;
    if (m_isTransitioning) {
        ss << " -> " << m_nextState << " (" << (int)(m_transitionProgress * 100) << "%)";
    }
    ss << "\nActivity: " << static_cast<int>(m_currentActivity);
    ss << "\nLocomotion: " << static_cast<int>(m_currentLocomotion);
    ss << "\nVelocity: " << glm::length(m_velocity);
    ss << "\nGrounded: " << (isGrounded() ? "yes" : "no");
    ss << "\nSwimming: " << (m_isSwimming ? "yes" : "no");
    ss << "\nFlying: " << (m_isFlying ? "yes" : "no");
    return ss.str();
}

void AnimationStateMachine::createDefaultStates() {
    // Add common states
    m_states["idle"] = StatePresets::idleState();
    m_states["walk"] = StatePresets::walkState();
    m_states["run"] = StatePresets::runState();
    m_states["death"] = StatePresets::deathState();
}

void AnimationStateMachine::createLocomotionStates() {
    m_states["sprint"] = StatePresets::sprintState();

    if (m_canSwim) {
        m_states["swim_idle"] = StatePresets::swimIdleState();
        m_states["swim"] = StatePresets::swimState();
        m_states["swim_fast"] = StatePresets::swimFastState();
    }

    if (m_canFly) {
        m_states["fly"] = StatePresets::flyState();
        m_states["glide"] = StatePresets::glideState();
        m_states["hover"] = StatePresets::hoverState();
    }
}

void AnimationStateMachine::createCombatStates() {
    m_states["attack"] = StatePresets::attackState();
    m_states["defend"] = StatePresets::defendState();
}

void AnimationStateMachine::createSocialStates() {
    m_states["greet"] = StatePresets::greetState();
    m_states["threat"] = StatePresets::threatDisplayState();
    m_states["sleep"] = StatePresets::sleepState();
}

// =============================================================================
// STATE PRESETS IMPLEMENTATION
// =============================================================================

namespace StatePresets {

AnimationState idleState() {
    AnimationState state;
    state.name = "idle";
    state.activity = ActivityState::IDLE;
    state.locomotion = LocomotionMode::NONE;
    state.duration = 2.0f;
    state.isLooping = true;
    state.locomotionWeight = 0.0f;
    state.ikWeight = 1.0f;
    state.secondaryMotionWeight = 0.5f;
    state.expressionWeight = 1.0f;
    return state;
}

AnimationState walkState() {
    AnimationState state;
    state.name = "walk";
    state.activity = ActivityState::LOCOMOTION;
    state.locomotion = LocomotionMode::WALKING;
    state.duration = 1.0f;
    state.speed = 1.0f;
    state.isLooping = true;
    state.blendInTime = 0.25f;
    state.locomotionWeight = 1.0f;
    state.ikWeight = 1.0f;
    state.secondaryMotionWeight = 0.7f;
    state.expressionWeight = 0.8f;
    return state;
}

AnimationState runState() {
    AnimationState state;
    state.name = "run";
    state.activity = ActivityState::LOCOMOTION;
    state.locomotion = LocomotionMode::RUNNING;
    state.duration = 0.6f;
    state.speed = 1.0f;
    state.isLooping = true;
    state.blendInTime = 0.2f;
    state.locomotionWeight = 1.0f;
    state.ikWeight = 1.0f;
    state.secondaryMotionWeight = 1.0f;
    state.expressionWeight = 0.6f;
    state.bodyLeanAmount = 0.15f;
    return state;
}

AnimationState sprintState() {
    AnimationState state;
    state.name = "sprint";
    state.activity = ActivityState::LOCOMOTION;
    state.locomotion = LocomotionMode::SPRINTING;
    state.duration = 0.4f;
    state.speed = 1.2f;
    state.isLooping = true;
    state.blendInTime = 0.15f;
    state.locomotionWeight = 1.0f;
    state.ikWeight = 0.8f;
    state.secondaryMotionWeight = 1.0f;
    state.expressionWeight = 0.4f;
    state.bodyLeanAmount = 0.25f;
    return state;
}

AnimationState swimIdleState() {
    AnimationState state;
    state.name = "swim_idle";
    state.activity = ActivityState::IDLE;
    state.locomotion = LocomotionMode::SWIMMING;
    state.duration = 3.0f;
    state.isLooping = true;
    state.locomotionWeight = 0.3f;
    state.ikWeight = 0.0f;
    state.secondaryMotionWeight = 0.8f;
    state.expressionWeight = 1.0f;
    return state;
}

AnimationState swimState() {
    AnimationState state;
    state.name = "swim";
    state.activity = ActivityState::LOCOMOTION;
    state.locomotion = LocomotionMode::SWIMMING;
    state.duration = 1.0f;
    state.isLooping = true;
    state.blendInTime = 0.3f;
    state.locomotionWeight = 1.0f;
    state.ikWeight = 0.0f;
    state.secondaryMotionWeight = 1.0f;
    state.expressionWeight = 0.7f;
    return state;
}

AnimationState swimFastState() {
    AnimationState state;
    state.name = "swim_fast";
    state.activity = ActivityState::LOCOMOTION;
    state.locomotion = LocomotionMode::SWIMMING;
    state.duration = 0.5f;
    state.speed = 1.5f;
    state.isLooping = true;
    state.blendInTime = 0.2f;
    state.locomotionWeight = 1.0f;
    state.ikWeight = 0.0f;
    state.secondaryMotionWeight = 1.0f;
    state.expressionWeight = 0.5f;
    return state;
}

AnimationState flyState() {
    AnimationState state;
    state.name = "fly";
    state.activity = ActivityState::LOCOMOTION;
    state.locomotion = LocomotionMode::FLYING;
    state.duration = 0.5f;
    state.isLooping = true;
    state.blendInTime = 0.3f;
    state.locomotionWeight = 1.0f;
    state.ikWeight = 0.0f;
    state.secondaryMotionWeight = 0.8f;
    state.expressionWeight = 0.7f;
    return state;
}

AnimationState glideState() {
    AnimationState state;
    state.name = "glide";
    state.activity = ActivityState::LOCOMOTION;
    state.locomotion = LocomotionMode::GLIDING;
    state.duration = 2.0f;
    state.isLooping = true;
    state.blendInTime = 0.5f;
    state.locomotionWeight = 0.2f;
    state.ikWeight = 0.0f;
    state.secondaryMotionWeight = 0.3f;
    state.expressionWeight = 0.8f;
    return state;
}

AnimationState hoverState() {
    AnimationState state;
    state.name = "hover";
    state.activity = ActivityState::LOCOMOTION;
    state.locomotion = LocomotionMode::HOVERING;
    state.duration = 0.3f;
    state.isLooping = true;
    state.blendInTime = 0.2f;
    state.locomotionWeight = 1.0f;
    state.ikWeight = 0.0f;
    state.secondaryMotionWeight = 0.6f;
    state.expressionWeight = 0.9f;
    return state;
}

AnimationState attackState() {
    AnimationState state;
    state.name = "attack";
    state.activity = ActivityState::ATTACKING;
    state.attack = AttackType::BITE;
    state.duration = 0.8f;
    state.isLooping = false;
    state.blendInTime = 0.1f;
    state.blendOutTime = 0.2f;
    state.blendMode = BlendMode::SMOOTH_STEP;
    state.locomotionWeight = 0.0f;
    state.ikWeight = 0.5f;
    state.secondaryMotionWeight = 1.0f;
    state.expressionWeight = 1.0f;
    return state;
}

AnimationState defendState() {
    AnimationState state;
    state.name = "defend";
    state.activity = ActivityState::ATTACKING;
    state.duration = 1.0f;
    state.isLooping = true;
    state.blendInTime = 0.15f;
    state.locomotionWeight = 0.0f;
    state.ikWeight = 0.8f;
    state.secondaryMotionWeight = 0.5f;
    state.expressionWeight = 1.0f;
    return state;
}

AnimationState greetState() {
    AnimationState state;
    state.name = "greet";
    state.activity = ActivityState::SOCIALIZING;
    state.duration = 2.0f;
    state.isLooping = false;
    state.blendInTime = 0.3f;
    state.blendOutTime = 0.3f;
    state.locomotionWeight = 0.0f;
    state.ikWeight = 1.0f;
    state.secondaryMotionWeight = 0.8f;
    state.expressionWeight = 1.0f;
    return state;
}

AnimationState threatDisplayState() {
    AnimationState state;
    state.name = "threat";
    state.activity = ActivityState::SOCIALIZING;
    state.duration = 3.0f;
    state.isLooping = true;
    state.blendInTime = 0.2f;
    state.locomotionWeight = 0.0f;
    state.ikWeight = 1.0f;
    state.secondaryMotionWeight = 1.0f;
    state.expressionWeight = 1.0f;
    return state;
}

AnimationState sleepState() {
    AnimationState state;
    state.name = "sleep";
    state.activity = ActivityState::SLEEPING;
    state.duration = 5.0f;
    state.isLooping = true;
    state.blendInTime = 1.0f;
    state.blendOutTime = 0.5f;
    state.blendMode = BlendMode::SMOOTH_STEP;
    state.locomotionWeight = 0.0f;
    state.ikWeight = 0.0f;
    state.secondaryMotionWeight = 0.2f;
    state.expressionWeight = 1.0f;
    state.rootOffset = glm::vec3(0.0f, -0.3f, 0.0f);  // Lower body
    return state;
}

AnimationState deathState() {
    AnimationState state;
    state.name = "death";
    state.activity = ActivityState::DYING;
    state.duration = 2.0f;
    state.isLooping = false;
    state.blendInTime = 0.1f;
    state.blendMode = BlendMode::LINEAR;
    state.locomotionWeight = 0.0f;
    state.ikWeight = 0.0f;
    state.secondaryMotionWeight = 1.0f;
    state.expressionWeight = 1.0f;
    return state;
}

}  // namespace StatePresets

// =============================================================================
// TRANSITION CONDITIONS IMPLEMENTATION
// =============================================================================

namespace TransitionConditions {

TransitionCondition velocityAbove(float threshold) {
    return [threshold](const AnimationStateMachine& sm) {
        return sm.isMoving() && glm::length(sm.m_velocity) > threshold;
    };
}

TransitionCondition velocityBelow(float threshold) {
    return [threshold](const AnimationStateMachine& sm) {
        return glm::length(sm.m_velocity) < threshold;
    };
}

TransitionCondition isMoving() {
    return [](const AnimationStateMachine& sm) {
        return sm.isMoving();
    };
}

TransitionCondition isStopped() {
    return [](const AnimationStateMachine& sm) {
        return !sm.isMoving();
    };
}

TransitionCondition isInWater() {
    return [](const AnimationStateMachine& sm) {
        return sm.isInWater();
    };
}

TransitionCondition isNotInWater() {
    return [](const AnimationStateMachine& sm) {
        return !sm.isInWater();
    };
}

TransitionCondition isAirborne() {
    return [](const AnimationStateMachine& sm) {
        return sm.isAirborne();
    };
}

TransitionCondition isGrounded() {
    return [](const AnimationStateMachine& sm) {
        return sm.isGrounded();
    };
}

TransitionCondition healthBelow(float threshold) {
    return [threshold](const AnimationStateMachine& sm) {
        return sm.m_health < threshold;
    };
}

TransitionCondition staminaBelow(float threshold) {
    return [threshold](const AnimationStateMachine& sm) {
        return sm.m_stamina < threshold;
    };
}

TransitionCondition threatAbove(float threshold) {
    return [threshold](const AnimationStateMachine& sm) {
        return sm.m_threatLevel > threshold;
    };
}

TransitionCondition stateTimeExceeds(float duration) {
    return [duration](const AnimationStateMachine& sm) {
        return sm.m_stateTime > duration;
    };
}

TransitionCondition allOf(std::vector<TransitionCondition> conditions) {
    return [conditions](const AnimationStateMachine& sm) {
        for (const auto& condition : conditions) {
            if (!condition(sm)) return false;
        }
        return true;
    };
}

TransitionCondition anyOf(std::vector<TransitionCondition> conditions) {
    return [conditions](const AnimationStateMachine& sm) {
        for (const auto& condition : conditions) {
            if (condition(sm)) return true;
        }
        return false;
    };
}

TransitionCondition noneOf(std::vector<TransitionCondition> conditions) {
    return [conditions](const AnimationStateMachine& sm) {
        for (const auto& condition : conditions) {
            if (condition(sm)) return false;
        }
        return true;
    };
}

}  // namespace TransitionConditions

// =============================================================================
// MORPHOLOGY STATE MACHINE BUILDER IMPLEMENTATION
// =============================================================================

AnimationStateMachine MorphologyStateMachineBuilder::buildFromMorphology(const MorphologyGenes& genes) {
    AnimationStateMachine sm;

    // Add common states
    sm.addState(StatePresets::idleState());
    sm.addState(StatePresets::deathState());

    // Add morphology-specific states
    if (genes.legCount == 2) {
        addBipedStates(sm, genes);
    } else if (genes.legCount == 4) {
        addQuadrupedStates(sm, genes);
    } else if (genes.legCount >= 6) {
        addHexapodStates(sm, genes);
    } else if (genes.legCount == 0 && !genes.canFly) {
        addSerpentineStates(sm, genes);
    }

    if (genes.canSwim || genes.isAquatic) {
        addAquaticStates(sm, genes);
    }

    if (genes.canFly) {
        addFlyingStates(sm, genes);
    }

    // Add common transitions
    addCommonTransitions(sm);

    return sm;
}

void MorphologyStateMachineBuilder::addBipedStates(AnimationStateMachine& sm, const MorphologyGenes& genes) {
    sm.addState(StatePresets::walkState());
    sm.addState(StatePresets::runState());
    sm.addState(StatePresets::sprintState());
    sm.addState(StatePresets::attackState());

    // Walking transitions
    sm.addTransition({
        "idle", "walk",
        TransitionConditions::allOf({
            TransitionConditions::isMoving(),
            TransitionConditions::velocityBelow(3.0f)
        }),
        0.25f, BlendMode::SMOOTH_STEP, 1
    });

    sm.addTransition({
        "walk", "idle",
        TransitionConditions::isStopped(),
        0.3f, BlendMode::SMOOTH_STEP, 1
    });

    // Running transitions
    sm.addTransition({
        "walk", "run",
        TransitionConditions::velocityAbove(3.0f),
        0.2f, BlendMode::SMOOTH_STEP, 2
    });

    sm.addTransition({
        "run", "walk",
        TransitionConditions::velocityBelow(2.5f),
        0.25f, BlendMode::SMOOTH_STEP, 2
    });

    sm.addTransition({
        "run", "sprint",
        TransitionConditions::velocityAbove(6.0f),
        0.15f, BlendMode::SMOOTH_STEP, 3
    });

    sm.addTransition({
        "sprint", "run",
        TransitionConditions::velocityBelow(5.5f),
        0.2f, BlendMode::SMOOTH_STEP, 3
    });
}

void MorphologyStateMachineBuilder::addQuadrupedStates(AnimationStateMachine& sm, const MorphologyGenes& genes) {
    sm.addState(StatePresets::walkState());
    sm.addState(StatePresets::runState());  // Trot
    sm.addState(StatePresets::sprintState());  // Gallop
    sm.addState(StatePresets::attackState());

    // Walk state is modified for quadruped gait
    AnimationState quadWalk = StatePresets::walkState();
    quadWalk.duration = 1.2f;
    quadWalk.bodyLeanAmount = 0.05f;
    sm.addState(quadWalk);

    // Similar transitions as biped but with different thresholds
    sm.addTransition({
        "idle", "walk",
        TransitionConditions::allOf({
            TransitionConditions::isMoving(),
            TransitionConditions::velocityBelow(2.0f)
        }),
        0.3f, BlendMode::SMOOTH_STEP, 1
    });

    sm.addTransition({
        "walk", "run",
        TransitionConditions::velocityAbove(2.0f),
        0.2f, BlendMode::SMOOTH_STEP, 2
    });

    sm.addTransition({
        "run", "sprint",
        TransitionConditions::velocityAbove(5.0f),
        0.15f, BlendMode::SMOOTH_STEP, 3
    });
}

void MorphologyStateMachineBuilder::addHexapodStates(AnimationStateMachine& sm, const MorphologyGenes& genes) {
    AnimationState hexWalk = StatePresets::walkState();
    hexWalk.duration = 0.8f;
    hexWalk.bodyLeanAmount = 0.02f;
    sm.addState(hexWalk);

    AnimationState hexRun = StatePresets::runState();
    hexRun.duration = 0.4f;
    sm.addState(hexRun);

    sm.addTransition({
        "idle", "walk",
        TransitionConditions::isMoving(),
        0.2f, BlendMode::SMOOTH_STEP, 1
    });
}

void MorphologyStateMachineBuilder::addAquaticStates(AnimationStateMachine& sm, const MorphologyGenes& genes) {
    sm.addState(StatePresets::swimIdleState());
    sm.addState(StatePresets::swimState());
    sm.addState(StatePresets::swimFastState());

    // Entering water
    sm.addTransition({
        "idle", "swim_idle",
        TransitionConditions::isInWater(),
        0.5f, BlendMode::SMOOTH_STEP, 5
    });

    sm.addTransition({
        "walk", "swim",
        TransitionConditions::isInWater(),
        0.3f, BlendMode::SMOOTH_STEP, 5
    });

    // Exiting water
    sm.addTransition({
        "swim_idle", "idle",
        TransitionConditions::isNotInWater(),
        0.5f, BlendMode::SMOOTH_STEP, 5
    });

    sm.addTransition({
        "swim", "walk",
        TransitionConditions::allOf({
            TransitionConditions::isNotInWater(),
            TransitionConditions::isGrounded()
        }),
        0.3f, BlendMode::SMOOTH_STEP, 5
    });

    // Swimming speed transitions
    sm.addTransition({
        "swim_idle", "swim",
        TransitionConditions::allOf({
            TransitionConditions::isInWater(),
            TransitionConditions::isMoving()
        }),
        0.3f, BlendMode::SMOOTH_STEP, 4
    });

    sm.addTransition({
        "swim", "swim_fast",
        TransitionConditions::allOf({
            TransitionConditions::isInWater(),
            TransitionConditions::velocityAbove(4.0f)
        }),
        0.2f, BlendMode::SMOOTH_STEP, 4
    });
}

void MorphologyStateMachineBuilder::addFlyingStates(AnimationStateMachine& sm, const MorphologyGenes& genes) {
    sm.addState(StatePresets::flyState());
    sm.addState(StatePresets::glideState());
    sm.addState(StatePresets::hoverState());

    // Takeoff
    sm.addTransition({
        "idle", "fly",
        TransitionConditions::isAirborne(),
        0.3f, BlendMode::SMOOTH_STEP, 6
    });

    // Landing
    sm.addTransition({
        "fly", "idle",
        TransitionConditions::isGrounded(),
        0.4f, BlendMode::SMOOTH_STEP, 6
    });

    sm.addTransition({
        "glide", "idle",
        TransitionConditions::isGrounded(),
        0.4f, BlendMode::SMOOTH_STEP, 6
    });

    // Flight mode transitions
    sm.addTransition({
        "fly", "glide",
        TransitionConditions::allOf({
            TransitionConditions::isAirborne(),
            TransitionConditions::velocityAbove(5.0f)
        }),
        0.5f, BlendMode::SMOOTH_STEP, 5
    });

    sm.addTransition({
        "glide", "fly",
        TransitionConditions::allOf({
            TransitionConditions::isAirborne(),
            TransitionConditions::velocityBelow(4.0f)
        }),
        0.3f, BlendMode::SMOOTH_STEP, 5
    });

    sm.addTransition({
        "fly", "hover",
        TransitionConditions::allOf({
            TransitionConditions::isAirborne(),
            TransitionConditions::isStopped()
        }),
        0.2f, BlendMode::SMOOTH_STEP, 5
    });
}

void MorphologyStateMachineBuilder::addSerpentineStates(AnimationStateMachine& sm, const MorphologyGenes& genes) {
    AnimationState slitherIdle = StatePresets::idleState();
    slitherIdle.name = "slither_idle";
    slitherIdle.locomotionWeight = 0.3f;  // Subtle movement even idle
    sm.addState(slitherIdle);

    AnimationState slither = StatePresets::walkState();
    slither.name = "slither";
    slither.duration = 2.0f;
    sm.addState(slither);

    AnimationState slitherFast = StatePresets::runState();
    slitherFast.name = "slither_fast";
    slitherFast.duration = 1.0f;
    sm.addState(slitherFast);

    sm.addTransition({
        "idle", "slither",
        TransitionConditions::isMoving(),
        0.3f, BlendMode::SMOOTH_STEP, 1
    });

    sm.addTransition({
        "slither", "idle",
        TransitionConditions::isStopped(),
        0.4f, BlendMode::SMOOTH_STEP, 1
    });
}

void MorphologyStateMachineBuilder::addCommonTransitions(AnimationStateMachine& sm) {
    // Death transition (highest priority)
    sm.addTransition({
        "*", "death",
        TransitionConditions::healthBelow(0.01f),
        0.1f, BlendMode::LINEAR, 100
    });
}

} // namespace animation
