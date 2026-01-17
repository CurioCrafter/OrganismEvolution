#include "ActivitySystem.h"
#include "Skeleton.h"
#include "Pose.h"
#include "ProceduralLocomotion.h"
#include "IKSolver.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace animation {

// =============================================================================
// ACTIVITY STATE MACHINE IMPLEMENTATION
// =============================================================================

ActivityStateMachine::ActivityStateMachine() {
    initializeDefaultConfigs();
}

void ActivityStateMachine::initialize() {
    initializeDefaultConfigs();
    m_currentActivity = ActivityType::IDLE;
    m_previousActivity = ActivityType::IDLE;
    m_activityTime = 0.0f;
    m_activityProgress = 0.0f;

    // Reset all cooldowns
    for (size_t i = 0; i < static_cast<size_t>(ActivityType::COUNT); ++i) {
        m_cooldowns[i] = 0.0f;
    }
}

void ActivityStateMachine::initializeDefaultConfigs() {
    m_configs[static_cast<size_t>(ActivityType::IDLE)] = ActivityDefaults::idleConfig();
    m_configs[static_cast<size_t>(ActivityType::EATING)] = ActivityDefaults::eatingConfig();
    m_configs[static_cast<size_t>(ActivityType::DRINKING)] = ActivityDefaults::drinkingConfig();
    m_configs[static_cast<size_t>(ActivityType::MATING)] = ActivityDefaults::matingConfig();
    m_configs[static_cast<size_t>(ActivityType::SLEEPING)] = ActivityDefaults::sleepingConfig();
    m_configs[static_cast<size_t>(ActivityType::EXCRETING)] = ActivityDefaults::excretingConfig();
    m_configs[static_cast<size_t>(ActivityType::GROOMING)] = ActivityDefaults::groomingConfig();
    m_configs[static_cast<size_t>(ActivityType::THREAT_DISPLAY)] = ActivityDefaults::threatDisplayConfig();
    m_configs[static_cast<size_t>(ActivityType::SUBMISSIVE_DISPLAY)] = ActivityDefaults::submissiveConfig();
    m_configs[static_cast<size_t>(ActivityType::MATING_DISPLAY)] = ActivityDefaults::matingDisplayConfig();
    m_configs[static_cast<size_t>(ActivityType::NEST_BUILDING)] = ActivityDefaults::nestBuildingConfig();
    m_configs[static_cast<size_t>(ActivityType::PLAYING)] = ActivityDefaults::playingConfig();
    m_configs[static_cast<size_t>(ActivityType::INVESTIGATING)] = ActivityDefaults::investigatingConfig();
    m_configs[static_cast<size_t>(ActivityType::CALLING)] = ActivityDefaults::callingConfig();
    m_configs[static_cast<size_t>(ActivityType::PARENTAL_CARE)] = ActivityDefaults::parentalCareConfig();
}

void ActivityStateMachine::setActivityConfig(ActivityType type, const ActivityConfig& config) {
    m_configs[static_cast<size_t>(type)] = config;
}

const ActivityConfig& ActivityStateMachine::getActivityConfig(ActivityType type) const {
    return m_configs[static_cast<size_t>(type)];
}

void ActivityStateMachine::setTriggers(const ActivityTriggers& triggers) {
    m_triggers = triggers;
}

void ActivityStateMachine::update(float deltaTime) {
    updateCooldowns(deltaTime);

    // Update transition
    if (m_isTransitioning) {
        m_transitionProgress += deltaTime / m_transitionDuration;
        if (m_transitionProgress >= 1.0f) {
            m_transitionProgress = 1.0f;
            m_isTransitioning = false;
            m_currentActivity = m_nextActivity;
            emitEvent(ActivityEventType::STARTED);
        }
        return;
    }

    // Update current activity
    m_activityTime += deltaTime;
    const auto& config = m_configs[static_cast<size_t>(m_currentActivity)];
    m_activityProgress = std::min(1.0f, m_activityTime / m_currentDuration);

    // Check for activity completion
    if (m_activityTime >= m_currentDuration && m_currentActivity != ActivityType::IDLE) {
        completeActivity();
        return;
    }

    // Evaluate potential new activity (only when not busy or can be interrupted)
    if (m_currentActivity == ActivityType::IDLE ||
        (config.canBeInterrupted && m_activityTime > config.blendInTime)) {

        ActivityType bestActivity = evaluateBestActivity();
        if (bestActivity != m_currentActivity) {
            const auto& newConfig = m_configs[static_cast<size_t>(bestActivity)];

            // Check if new activity has higher priority or current is idle
            if (m_currentActivity == ActivityType::IDLE ||
                newConfig.priority > config.interruptPriority) {

                if (m_currentActivity != ActivityType::IDLE) {
                    emitEvent(ActivityEventType::INTERRUPTED);
                }
                transitionTo(bestActivity);
            }
        }
    }
}

bool ActivityStateMachine::requestActivity(ActivityType type, bool force) {
    if (type == m_currentActivity) return true;

    // Check cooldown
    if (!force && isOnCooldown(type)) {
        return false;
    }

    const auto& currentConfig = m_configs[static_cast<size_t>(m_currentActivity)];
    const auto& newConfig = m_configs[static_cast<size_t>(type)];

    // Check if we can interrupt current activity
    if (!force && m_currentActivity != ActivityType::IDLE &&
        !currentConfig.canBeInterrupted) {
        return false;
    }

    if (!force && newConfig.priority < currentConfig.interruptPriority) {
        return false;
    }

    if (m_currentActivity != ActivityType::IDLE) {
        emitEvent(ActivityEventType::INTERRUPTED);
    }

    transitionTo(type);
    return true;
}

void ActivityStateMachine::cancelActivity() {
    if (m_currentActivity != ActivityType::IDLE) {
        emitEvent(ActivityEventType::INTERRUPTED);
        transitionTo(ActivityType::IDLE);
    }
}

void ActivityStateMachine::transitionTo(ActivityType newActivity) {
    m_previousActivity = m_currentActivity;
    m_nextActivity = newActivity;

    // Select sub-type for activities with variants
    selectSubType(newActivity);

    const auto& config = m_configs[static_cast<size_t>(newActivity)];
    m_transitionDuration = config.blendInTime;

    // Calculate duration for this instance
    float durationRange = config.maxDuration - config.minDuration;
    m_currentDuration = config.minDuration + (static_cast<float>(rand()) / RAND_MAX) * durationRange;

    m_isTransitioning = true;
    m_transitionProgress = 0.0f;
    m_activityTime = 0.0f;
    m_activityProgress = 0.0f;
}

void ActivityStateMachine::completeActivity() {
    emitEvent(ActivityEventType::COMPLETED);

    // Set cooldown
    const auto& config = m_configs[static_cast<size_t>(m_currentActivity)];
    m_cooldowns[static_cast<size_t>(m_currentActivity)] = config.cooldownTime;

    transitionTo(ActivityType::IDLE);
}

void ActivityStateMachine::emitEvent(ActivityEventType eventType) {
    ActivityEvent event;
    event.activity = m_currentActivity;
    event.eventType = eventType;
    event.timestamp = m_activityTime;
    event.creatureId = m_creatureId;
    event.position = glm::vec3(0.0f); // Filled by external code
    event.targetPosition = glm::vec3(0.0f);

    for (const auto& callback : m_callbacks) {
        callback(event);
    }
}

void ActivityStateMachine::updateCooldowns(float deltaTime) {
    for (size_t i = 0; i < static_cast<size_t>(ActivityType::COUNT); ++i) {
        if (m_cooldowns[i] > 0.0f) {
            m_cooldowns[i] = std::max(0.0f, m_cooldowns[i] - deltaTime);
        }
    }
}

void ActivityStateMachine::selectSubType(ActivityType type) {
    switch (type) {
        case ActivityType::EXCRETING:
            // Alternate between urinate and defecate based on trigger levels
            m_excretionType = (m_triggers.bladderFullness > m_triggers.bowelFullness)
                ? ExcretionType::URINATE : ExcretionType::DEFECATE;
            break;

        case ActivityType::GROOMING:
            // Select grooming type based on context
            if (m_triggers.dirtyLevel > 0.8f) {
                m_groomingType = GroomingType::SHAKE;
            } else if (m_triggers.fatigueLevel > 0.5f) {
                m_groomingType = GroomingType::STRETCH;
            } else {
                // Random selection
                int r = rand() % 5;
                m_groomingType = static_cast<GroomingType>(r);
            }
            break;

        case ActivityType::THREAT_DISPLAY:
        case ActivityType::MATING_DISPLAY:
            // Select display type (would be based on morphology in full implementation)
            m_displayType = static_cast<DisplayType>(rand() % 6);
            break;

        default:
            break;
    }
}

ActivityType ActivityStateMachine::evaluateBestActivity() const {
    ActivityType best = ActivityType::IDLE;
    float bestScore = 0.0f;

    for (size_t i = 0; i < static_cast<size_t>(ActivityType::COUNT); ++i) {
        ActivityType type = static_cast<ActivityType>(i);
        if (type == ActivityType::IDLE) continue;
        if (isOnCooldown(type)) continue;

        float score = calculateActivityScore(type);
        if (score > bestScore && score >= m_configs[i].triggerThreshold) {
            bestScore = score;
            best = type;
        }
    }

    return best;
}

float ActivityStateMachine::calculateActivityScore(ActivityType type) const {
    const auto& config = m_configs[static_cast<size_t>(type)];

    switch (type) {
        case ActivityType::EATING:
            return m_triggers.foodNearby ?
                (m_triggers.hungerLevel * 1.2f) : 0.0f;

        case ActivityType::DRINKING:
            return m_triggers.thirstLevel;

        case ActivityType::MATING:
            return m_triggers.potentialMateNearby ?
                (m_triggers.reproductionUrge * 1.1f) : 0.0f;

        case ActivityType::SLEEPING:
            return m_triggers.fatigueLevel * (1.0f - m_triggers.threatLevel);

        case ActivityType::EXCRETING:
            return std::max(m_triggers.bladderFullness, m_triggers.bowelFullness) *
                   (1.0f - m_triggers.threatLevel * 0.5f);

        case ActivityType::GROOMING:
            return m_triggers.dirtyLevel * (1.0f - m_triggers.threatLevel) *
                   (1.0f - m_triggers.hungerLevel * 0.5f);

        case ActivityType::THREAT_DISPLAY:
            return m_triggers.territoryIntruder ?
                (m_triggers.threatLevel * 0.8f + 0.3f) : 0.0f;

        case ActivityType::SUBMISSIVE_DISPLAY:
            return (m_triggers.threatLevel > 0.7f && !m_triggers.territoryIntruder) ?
                m_triggers.threatLevel : 0.0f;

        case ActivityType::MATING_DISPLAY:
            return m_triggers.potentialMateNearby ?
                (m_triggers.reproductionUrge * 0.9f) : 0.0f;

        case ActivityType::PLAYING:
            return m_triggers.isJuvenile ?
                (m_triggers.playUrge * (1.0f - m_triggers.hungerLevel)) : 0.0f;

        case ActivityType::INVESTIGATING:
            return (m_triggers.foodNearby && m_triggers.hungerLevel < 0.5f) ? 0.4f : 0.0f;

        case ActivityType::CALLING:
            return m_triggers.socialNeed * (1.0f - m_triggers.threatLevel);

        case ActivityType::NEST_BUILDING:
            // High reproduction urge + safe environment triggers nesting
            return (m_triggers.reproductionUrge > 0.5f && m_triggers.threatLevel < 0.3f) ?
                (m_triggers.reproductionUrge * 0.6f) : 0.0f;

        case ActivityType::PARENTAL_CARE:
            // Offspring nearby and hungry triggers parental care
            return m_triggers.hasOffspringNearby ?
                (m_triggers.parentalUrge + m_triggers.offspringHungerLevel * 0.5f) : 0.0f;

        default:
            return 0.0f;
    }
}

float ActivityStateMachine::getCooldownRemaining(ActivityType type) const {
    return m_cooldowns[static_cast<size_t>(type)];
}

bool ActivityStateMachine::isOnCooldown(ActivityType type) const {
    return m_cooldowns[static_cast<size_t>(type)] > 0.0f;
}

void ActivityStateMachine::registerEventCallback(ActivityEventCallback callback) {
    m_callbacks.push_back(std::move(callback));
}

void ActivityStateMachine::clearEventCallbacks() {
    m_callbacks.clear();
}

std::string ActivityStateMachine::getCurrentActivityName() const {
    return ActivityNames::getName(m_currentActivity);
}

std::string ActivityStateMachine::getDebugInfo() const {
    std::stringstream ss;
    ss << "Activity: " << getCurrentActivityName();
    ss << " | Progress: " << static_cast<int>(m_activityProgress * 100) << "%";
    ss << " | Time: " << m_activityTime << "/" << m_currentDuration;
    if (m_isTransitioning) {
        ss << " | TRANSITIONING (" << static_cast<int>(m_transitionProgress * 100) << "%)";
    }
    return ss.str();
}

// =============================================================================
// ACTIVITY ANIMATION DRIVER IMPLEMENTATION
// =============================================================================

void ActivityAnimationDriver::update(float deltaTime) {
    if (!m_stateMachine) return;

    m_animationTime += deltaTime;

    // Calculate activity weight (for blending with locomotion)
    float targetWeight = m_stateMachine->isInActivity() ? 1.0f : 0.0f;
    if (m_stateMachine->isTransitioning()) {
        targetWeight = m_stateMachine->getTransitionProgress();
    }

    // Smooth weight transitions
    float weightSpeed = 5.0f;
    m_activityWeight = m_activityWeight + (targetWeight - m_activityWeight) * std::min(1.0f, deltaTime * weightSpeed);

    // Generate activity-specific animation
    float progress = m_stateMachine->getActivityProgress();
    ActivityType activity = m_stateMachine->getCurrentActivity();

    // Reset state
    m_bodyOffset = glm::vec3(0.0f);
    m_bodyRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    m_hasHeadTarget = false;

    switch (activity) {
        case ActivityType::IDLE:
            generateIdleAnimation(progress);
            break;
        case ActivityType::EATING:
            generateEatingAnimation(progress);
            break;
        case ActivityType::DRINKING:
            generateDrinkingAnimation(progress);
            break;
        case ActivityType::MATING:
            generateMatingAnimation(progress);
            break;
        case ActivityType::SLEEPING:
            generateSleepingAnimation(progress);
            break;
        case ActivityType::EXCRETING:
            generateExcretingAnimation(progress, m_stateMachine->getExcretionType());
            break;
        case ActivityType::GROOMING:
            generateGroomingAnimation(progress, m_stateMachine->getGroomingType());
            break;
        case ActivityType::THREAT_DISPLAY:
            generateThreatDisplayAnimation(progress);
            break;
        case ActivityType::SUBMISSIVE_DISPLAY:
            generateSubmissiveAnimation(progress);
            break;
        case ActivityType::MATING_DISPLAY:
            generateMatingDisplayAnimation(progress);
            break;
        case ActivityType::PLAYING:
            generatePlayingAnimation(progress);
            break;
        case ActivityType::INVESTIGATING:
            generateInvestigatingAnimation(progress);
            break;
        case ActivityType::CALLING:
            generateCallingAnimation(progress);
            break;
        case ActivityType::PARENTAL_CARE:
            generateParentalCareAnimation(progress);
            break;
        case ActivityType::NEST_BUILDING:
            generateNestBuildingAnimation(progress);
            break;
        default:
            break;
    }
}

void ActivityAnimationDriver::applyToPose(
    const Skeleton& skeleton,
    SkeletonPose& pose,
    ProceduralLocomotion* locomotion,
    IKSystem* ikSystem)
{
    if (!m_stateMachine || m_activityWeight < 0.01f) return;

    // The body offset/rotation would be blended with locomotion
    // This is a simplified implementation - full version would modify bones directly
}

void ActivityAnimationDriver::generateIdleAnimation(float progress) {
    // Subtle breathing motion
    float breathCycle = std::sin(m_animationTime * 1.5f) * 0.5f + 0.5f;
    m_bodyOffset.y = breathCycle * 0.01f * m_bodySize;

    // Occasional weight shift
    float shiftCycle = std::sin(m_animationTime * 0.3f);
    m_bodyOffset.x = shiftCycle * 0.005f * m_bodySize;
}

void ActivityAnimationDriver::generateEatingAnimation(float progress) {
    // Look at food
    m_hasHeadTarget = true;
    m_headTarget = m_foodPosition;

    // Head bobbing motion (pecking/chewing)
    float bobFrequency = 4.0f;
    float bobPhase = progress * bobFrequency * 2.0f * 3.14159f;
    float bobAmount = std::sin(bobPhase) * 0.3f;

    // Lower body slightly while eating
    m_bodyOffset.y = -0.05f * m_bodySize;

    // Head dips down then up
    m_bodyRotation = calculateHeadBob(m_animationTime * bobFrequency, 0.15f);
}

void ActivityAnimationDriver::generateDrinkingAnimation(float progress) {
    // Look down at water
    m_hasHeadTarget = true;
    m_headTarget = m_groundPosition + glm::vec3(0.3f * m_bodySize, -0.2f * m_bodySize, 0.0f);

    // Slow lapping motion
    float lapCycle = std::sin(progress * 6.0f * 3.14159f);

    // Body lowers significantly
    m_bodyOffset.y = -0.1f * m_bodySize;

    // Head dips in rhythm
    float headDip = lapCycle * 0.1f;
    m_bodyRotation = glm::angleAxis(headDip + 0.2f, glm::vec3(1, 0, 0));
}

void ActivityAnimationDriver::generateMatingAnimation(float progress) {
    // Look at mate
    m_hasHeadTarget = true;
    m_headTarget = m_matePosition;

    // Body movements depend on phase
    if (progress < 0.3f) {
        // Approach phase - body rises
        m_bodyOffset.y = progress * 0.1f * m_bodySize;
    } else if (progress < 0.8f) {
        // Active phase - rhythmic motion
        float phase = (progress - 0.3f) / 0.5f;
        float rhythm = std::sin(phase * 8.0f * 3.14159f);
        m_bodyOffset.y = 0.03f * m_bodySize;
        m_bodyOffset.z = rhythm * 0.05f * m_bodySize;
    } else {
        // Wind-down phase
        float phase = (progress - 0.8f) / 0.2f;
        m_bodyOffset.y = (1.0f - phase) * 0.03f * m_bodySize;
    }
}

void ActivityAnimationDriver::generateSleepingAnimation(float progress) {
    // Body lowers to resting position
    float settlePhase = std::min(1.0f, progress * 3.0f);
    m_bodyOffset.y = -0.15f * m_bodySize * settlePhase;

    // Very slow breathing
    float breathCycle = std::sin(m_animationTime * 0.5f);
    m_bodyOffset.y += breathCycle * 0.01f * m_bodySize;

    // Head tucks slightly
    m_bodyRotation = glm::angleAxis(0.1f * settlePhase, glm::vec3(1, 0, 0));
}

void ActivityAnimationDriver::generateExcretingAnimation(float progress, ExcretionType type) {
    if (type == ExcretionType::URINATE) {
        // Leg lift for males (simplified - all creatures do squat)
        float squatDepth = 0.0f;
        if (progress < 0.2f) {
            squatDepth = progress / 0.2f;
        } else if (progress < 0.8f) {
            squatDepth = 1.0f;
        } else {
            squatDepth = (1.0f - progress) / 0.2f;
        }
        m_bodyOffset.y = -0.1f * m_bodySize * squatDepth;
    } else {
        // Defecation - squat and strain
        float squatDepth = 0.0f;
        if (progress < 0.15f) {
            squatDepth = progress / 0.15f;
        } else if (progress < 0.85f) {
            squatDepth = 1.0f;
            // Strain motion
            float strain = std::sin((progress - 0.15f) * 10.0f * 3.14159f) * 0.02f;
            m_bodyOffset.z = strain * m_bodySize;
        } else {
            squatDepth = (1.0f - progress) / 0.15f;
        }
        m_bodyOffset.y = -0.12f * m_bodySize * squatDepth;
        m_bodyRotation = glm::angleAxis(0.1f * squatDepth, glm::vec3(1, 0, 0));
    }
}

void ActivityAnimationDriver::generateGroomingAnimation(float progress, GroomingType type) {
    switch (type) {
        case GroomingType::SCRATCH:
            // Body tilts to scratching side
            {
                float scratchPhase = std::sin(progress * 12.0f * 3.14159f);
                m_bodyRotation = glm::angleAxis(0.15f, glm::vec3(0, 0, 1));
                m_bodyOffset.y = scratchPhase * 0.02f * m_bodySize;
            }
            break;

        case GroomingType::LICK:
            // Head turns to lick body part
            m_hasHeadTarget = true;
            m_headTarget = m_groundPosition + glm::vec3(0, 0.1f * m_bodySize, 0.3f * m_bodySize);
            m_bodyRotation = glm::angleAxis(0.3f, glm::vec3(0, 1, 0));
            break;

        case GroomingType::SHAKE:
            // Vigorous shake
            {
                float shakePhase = std::sin(progress * 20.0f * 3.14159f);
                float shakeEnvelope = std::sin(progress * 3.14159f); // Rise and fall
                m_bodyOffset.x = shakePhase * shakeEnvelope * 0.05f * m_bodySize;
                m_bodyRotation = glm::angleAxis(
                    shakePhase * shakeEnvelope * 0.2f,
                    glm::vec3(0, 0, 1));
            }
            break;

        case GroomingType::PREEN:
            // Wing preening (if has wings)
            if (m_hasWings) {
                m_hasHeadTarget = true;
                float side = std::sin(progress * 2.0f * 3.14159f);
                m_headTarget = m_groundPosition + glm::vec3(side * 0.5f * m_bodySize, 0.2f * m_bodySize, 0);
                m_bodyRotation = glm::angleAxis(side * 0.2f, glm::vec3(0, 1, 0));
            }
            break;

        case GroomingType::STRETCH:
            // Full body stretch
            {
                float stretchPhase = 0.0f;
                if (progress < 0.3f) {
                    stretchPhase = progress / 0.3f;
                } else if (progress < 0.7f) {
                    stretchPhase = 1.0f;
                } else {
                    stretchPhase = (1.0f - progress) / 0.3f;
                }

                // Body elongates
                m_bodyOffset.y = 0.05f * m_bodySize * stretchPhase;
                m_bodyOffset.z = 0.1f * m_bodySize * stretchPhase;

                // Head up, back arched
                m_bodyRotation = glm::angleAxis(-0.2f * stretchPhase, glm::vec3(1, 0, 0));
            }
            break;
    }
}

void ActivityAnimationDriver::generateThreatDisplayAnimation(float progress) {
    // Rise up to look bigger
    float risePhase = std::min(1.0f, progress * 2.0f);
    m_bodyOffset.y = 0.15f * m_bodySize * risePhase;

    // Puff up (body expands slightly - would affect morph targets)

    // Aggressive head motion
    if (progress > 0.3f && progress < 0.8f) {
        float bobPhase = std::sin((progress - 0.3f) * 15.0f * 3.14159f);
        m_bodyOffset.z = bobPhase * 0.03f * m_bodySize;
    }

    // Body tilts forward aggressively
    m_bodyRotation = glm::angleAxis(-0.15f * risePhase, glm::vec3(1, 0, 0));
}

void ActivityAnimationDriver::generateSubmissiveAnimation(float progress) {
    // Crouch down
    float crouchPhase = std::min(1.0f, progress * 2.0f);
    m_bodyOffset.y = -0.2f * m_bodySize * crouchPhase;

    // Head down, avoiding eye contact
    m_bodyRotation = glm::angleAxis(0.3f * crouchPhase, glm::vec3(1, 0, 0));

    // Slight trembling
    float tremble = std::sin(progress * 30.0f * 3.14159f) * 0.01f * m_bodySize;
    m_bodyOffset.x = tremble * (1.0f - progress);
}

void ActivityAnimationDriver::generateMatingDisplayAnimation(float progress) {
    // Look at potential mate
    m_hasHeadTarget = true;
    m_headTarget = m_matePosition;

    // Display behavior varies
    if (m_hasWings) {
        // Wing display - body tilts and wings spread
        float displayPhase = std::sin(progress * 4.0f * 3.14159f) * 0.5f + 0.5f;
        m_bodyOffset.y = 0.1f * m_bodySize * displayPhase;
        m_bodyRotation = glm::angleAxis(-0.1f * displayPhase, glm::vec3(1, 0, 0));
    } else if (m_hasTail) {
        // Tail display - body sways
        float swayPhase = std::sin(progress * 3.0f * 3.14159f);
        m_bodyRotation = glm::angleAxis(swayPhase * 0.15f, glm::vec3(0, 1, 0));
        m_bodyOffset.y = 0.05f * m_bodySize;
    } else {
        // Generic display - bob and weave
        float bobPhase = std::sin(progress * 5.0f * 3.14159f);
        m_bodyOffset.y = (bobPhase * 0.5f + 0.5f) * 0.08f * m_bodySize;
    }

    // Crest display (if applicable)
    if (m_hasCrest) {
        // Crest would be raised via morph target
    }
}

void ActivityAnimationDriver::generatePlayingAnimation(float progress) {
    // Playful bouncing
    float bouncePhase = std::sin(progress * 8.0f * 3.14159f);
    m_bodyOffset.y = (bouncePhase * 0.5f + 0.5f) * 0.1f * m_bodySize;

    // Random-ish direction changes
    float dirPhase = std::sin(progress * 3.0f * 3.14159f);
    m_bodyOffset.x = dirPhase * 0.05f * m_bodySize;

    // Play bow (front down)
    if (progress > 0.5f && progress < 0.7f) {
        float bowPhase = std::sin((progress - 0.5f) / 0.2f * 3.14159f);
        m_bodyRotation = glm::angleAxis(0.2f * bowPhase, glm::vec3(1, 0, 0));
        m_bodyOffset.y = -0.05f * m_bodySize * bowPhase;
    }
}

void ActivityAnimationDriver::generateInvestigatingAnimation(float progress) {
    // Cautious approach
    m_hasHeadTarget = true;
    m_headTarget = m_foodPosition;

    // Body low and elongated
    m_bodyOffset.y = -0.05f * m_bodySize;

    // Head weaving to examine object
    float weavePhase = std::sin(progress * 4.0f * 3.14159f);
    m_bodyOffset.x = weavePhase * 0.03f * m_bodySize;

    // Occasional sniffing motion
    if (std::fmod(progress, 0.25f) < 0.1f) {
        float sniffPhase = std::sin(std::fmod(progress, 0.1f) / 0.1f * 3.14159f);
        m_bodyRotation = glm::angleAxis(sniffPhase * 0.1f, glm::vec3(1, 0, 0));
    }
}

void ActivityAnimationDriver::generateCallingAnimation(float progress) {
    // Head up for calling
    m_bodyOffset.y = 0.05f * m_bodySize;

    // Body pulses with call
    float callPhase = std::sin(progress * 6.0f * 3.14159f);
    float callEnvelope = std::sin(progress * 3.14159f);

    m_bodyOffset.z = callPhase * callEnvelope * 0.02f * m_bodySize;

    // Head tilts back during call
    m_bodyRotation = glm::angleAxis(-0.15f * callEnvelope, glm::vec3(1, 0, 0));
}

void ActivityAnimationDriver::generateParentalCareAnimation(float progress) {
    // Gentle, protective posture
    m_hasHeadTarget = true;
    m_headTarget = m_groundPosition + glm::vec3(0.2f * m_bodySize, -0.1f * m_bodySize, 0.3f * m_bodySize);

    // Body lowers slightly to interact with young
    float interactPhase = 0.0f;
    if (progress < 0.2f) {
        // Approach/lower phase
        interactPhase = progress / 0.2f;
    } else if (progress < 0.8f) {
        // Active care phase - gentle bobbing
        interactPhase = 1.0f;
        float careMotion = std::sin((progress - 0.2f) * 5.0f * 3.14159f);
        m_bodyOffset.z = careMotion * 0.02f * m_bodySize;
    } else {
        // Rise back up
        interactPhase = (1.0f - progress) / 0.2f;
    }

    m_bodyOffset.y = -0.05f * m_bodySize * interactPhase;

    // Gentle head movements for feeding/grooming young
    float headNod = std::sin(progress * 4.0f * 3.14159f) * 0.1f * interactPhase;
    m_bodyRotation = glm::angleAxis(headNod + 0.1f * interactPhase, glm::vec3(1, 0, 0));
}

void ActivityAnimationDriver::generateNestBuildingAnimation(float progress) {
    // Working posture - alternating between gathering and placing
    float cyclePhase = std::fmod(progress * 3.0f, 1.0f);

    if (cyclePhase < 0.5f) {
        // Gathering phase - head down, picking up materials
        float gatherPhase = cyclePhase / 0.5f;
        float pickMotion = std::sin(gatherPhase * 3.14159f);

        m_bodyOffset.y = -0.08f * m_bodySize * pickMotion;
        m_bodyRotation = glm::angleAxis(0.2f * pickMotion, glm::vec3(1, 0, 0));

        m_hasHeadTarget = true;
        m_headTarget = m_groundPosition + glm::vec3(0.3f * m_bodySize, -0.15f * m_bodySize, 0);
    } else {
        // Placing phase - head up, arranging materials
        float placePhase = (cyclePhase - 0.5f) / 0.5f;
        float placeMotion = std::sin(placePhase * 3.14159f);

        m_bodyOffset.y = 0.02f * m_bodySize * placeMotion;
        m_bodyRotation = glm::angleAxis(-0.1f * placeMotion, glm::vec3(1, 0, 0));

        // Head weaves as placing materials
        float weave = std::sin(placePhase * 4.0f * 3.14159f) * 0.1f;
        m_bodyOffset.x = weave * m_bodySize;
    }

    // Occasional pausing to inspect work
    if (progress > 0.7f && progress < 0.8f) {
        float inspectPhase = (progress - 0.7f) / 0.1f;
        m_bodyOffset.y = 0.05f * m_bodySize * std::sin(inspectPhase * 3.14159f);
    }
}

glm::quat ActivityAnimationDriver::calculateHeadBob(float time, float amplitude) const {
    float bobAngle = std::sin(time * 2.0f * 3.14159f) * amplitude;
    return glm::angleAxis(bobAngle, glm::vec3(1, 0, 0));
}

glm::vec3 ActivityAnimationDriver::calculateBodySway(float time, float amplitude) const {
    float swayX = std::sin(time * 0.5f * 3.14159f) * amplitude;
    float swayZ = std::cos(time * 0.3f * 3.14159f) * amplitude * 0.5f;
    return glm::vec3(swayX, 0.0f, swayZ);
}

glm::quat ActivityAnimationDriver::calculateTailWag(float time, float amplitude) const {
    float wagAngle = std::sin(time * 3.0f * 3.14159f) * amplitude;
    return glm::angleAxis(wagAngle, glm::vec3(0, 1, 0));
}

// =============================================================================
// ACTIVITY NAMES
// =============================================================================

namespace ActivityNames {

const char* getName(ActivityType type) {
    switch (type) {
        case ActivityType::IDLE: return "Idle";
        case ActivityType::EATING: return "Eating";
        case ActivityType::DRINKING: return "Drinking";
        case ActivityType::MATING: return "Mating";
        case ActivityType::SLEEPING: return "Sleeping";
        case ActivityType::EXCRETING: return "Excreting";
        case ActivityType::GROOMING: return "Grooming";
        case ActivityType::THREAT_DISPLAY: return "Threat Display";
        case ActivityType::SUBMISSIVE_DISPLAY: return "Submissive";
        case ActivityType::MATING_DISPLAY: return "Mating Display";
        case ActivityType::NEST_BUILDING: return "Nest Building";
        case ActivityType::PLAYING: return "Playing";
        case ActivityType::INVESTIGATING: return "Investigating";
        case ActivityType::CALLING: return "Calling";
        case ActivityType::PARENTAL_CARE: return "Parental Care";
        default: return "Unknown";
    }
}

const char* getDescription(ActivityType type) {
    switch (type) {
        case ActivityType::IDLE: return "Resting and observing surroundings";
        case ActivityType::EATING: return "Consuming food";
        case ActivityType::DRINKING: return "Consuming water";
        case ActivityType::MATING: return "Reproductive behavior";
        case ActivityType::SLEEPING: return "Resting/sleeping state";
        case ActivityType::EXCRETING: return "Waste elimination";
        case ActivityType::GROOMING: return "Self-maintenance behavior";
        case ActivityType::THREAT_DISPLAY: return "Warning display to threats";
        case ActivityType::SUBMISSIVE_DISPLAY: return "Submissive posture";
        case ActivityType::MATING_DISPLAY: return "Courtship display";
        case ActivityType::NEST_BUILDING: return "Building shelter";
        case ActivityType::PLAYING: return "Playful behavior";
        case ActivityType::INVESTIGATING: return "Examining something";
        case ActivityType::CALLING: return "Vocalizing";
        case ActivityType::PARENTAL_CARE: return "Caring for young";
        default: return "";
    }
}

const char* getExcretionName(ExcretionType type) {
    switch (type) {
        case ExcretionType::URINATE: return "Urinating";
        case ExcretionType::DEFECATE: return "Defecating";
        default: return "Unknown";
    }
}

const char* getGroomingName(GroomingType type) {
    switch (type) {
        case GroomingType::SCRATCH: return "Scratching";
        case GroomingType::LICK: return "Licking";
        case GroomingType::SHAKE: return "Shaking";
        case GroomingType::PREEN: return "Preening";
        case GroomingType::STRETCH: return "Stretching";
        default: return "Unknown";
    }
}

const char* getDisplayName(DisplayType type) {
    switch (type) {
        case DisplayType::CREST_RAISE: return "Crest Raise";
        case DisplayType::WING_SPREAD: return "Wing Spread";
        case DisplayType::TAIL_FAN: return "Tail Fan";
        case DisplayType::BODY_INFLATE: return "Body Inflate";
        case DisplayType::COLOR_FLASH: return "Color Flash";
        case DisplayType::VOCALIZE: return "Vocalize";
        default: return "Unknown";
    }
}

} // namespace ActivityNames

// =============================================================================
// DEFAULT ACTIVITY CONFIGURATIONS
// =============================================================================

namespace ActivityDefaults {

ActivityConfig idleConfig() {
    ActivityConfig config;
    config.type = ActivityType::IDLE;
    config.minDuration = 1.0f;
    config.maxDuration = 10.0f;
    config.cooldownTime = 0.0f;
    config.priority = 0;
    config.canBeInterrupted = true;
    config.interruptPriority = 0;
    config.blendInTime = 0.3f;
    config.blendOutTime = 0.3f;
    config.requiresStationary = false;
    config.canWalkDuring = true;
    config.triggerThreshold = 0.0f;
    return config;
}

ActivityConfig eatingConfig() {
    ActivityConfig config;
    config.type = ActivityType::EATING;
    config.minDuration = 2.0f;
    config.maxDuration = 8.0f;
    config.cooldownTime = 5.0f;
    config.priority = 7;
    config.canBeInterrupted = true;
    config.interruptPriority = 8;
    config.blendInTime = 0.4f;
    config.blendOutTime = 0.3f;
    config.requiresStationary = true;
    config.canWalkDuring = false;
    config.triggerThreshold = 0.4f;
    return config;
}

ActivityConfig drinkingConfig() {
    ActivityConfig config;
    config.type = ActivityType::DRINKING;
    config.minDuration = 1.5f;
    config.maxDuration = 4.0f;
    config.cooldownTime = 10.0f;
    config.priority = 6;
    config.canBeInterrupted = true;
    config.interruptPriority = 7;
    config.blendInTime = 0.3f;
    config.blendOutTime = 0.3f;
    config.requiresStationary = true;
    config.canWalkDuring = false;
    config.triggerThreshold = 0.5f;
    return config;
}

ActivityConfig matingConfig() {
    ActivityConfig config;
    config.type = ActivityType::MATING;
    config.minDuration = 3.0f;
    config.maxDuration = 10.0f;
    config.cooldownTime = 30.0f;
    config.priority = 8;
    config.canBeInterrupted = true;
    config.interruptPriority = 9;
    config.blendInTime = 0.5f;
    config.blendOutTime = 0.5f;
    config.requiresStationary = true;
    config.canWalkDuring = false;
    config.triggerThreshold = 0.6f;
    return config;
}

ActivityConfig sleepingConfig() {
    ActivityConfig config;
    config.type = ActivityType::SLEEPING;
    config.minDuration = 10.0f;
    config.maxDuration = 60.0f;
    config.cooldownTime = 120.0f;
    config.priority = 3;
    config.canBeInterrupted = true;
    config.interruptPriority = 4;
    config.blendInTime = 1.0f;
    config.blendOutTime = 0.5f;
    config.requiresStationary = true;
    config.canWalkDuring = false;
    config.triggerThreshold = 0.7f;
    return config;
}

ActivityConfig excretingConfig() {
    ActivityConfig config;
    config.type = ActivityType::EXCRETING;
    config.minDuration = 1.0f;
    config.maxDuration = 3.0f;
    config.cooldownTime = 20.0f;
    config.priority = 5;
    config.canBeInterrupted = false;
    config.interruptPriority = 10;
    config.blendInTime = 0.3f;
    config.blendOutTime = 0.3f;
    config.requiresStationary = true;
    config.canWalkDuring = false;
    config.triggerThreshold = 0.8f;
    return config;
}

ActivityConfig groomingConfig() {
    ActivityConfig config;
    config.type = ActivityType::GROOMING;
    config.minDuration = 2.0f;
    config.maxDuration = 8.0f;
    config.cooldownTime = 15.0f;
    config.priority = 2;
    config.canBeInterrupted = true;
    config.interruptPriority = 3;
    config.blendInTime = 0.3f;
    config.blendOutTime = 0.3f;
    config.requiresStationary = true;
    config.canWalkDuring = false;
    config.triggerThreshold = 0.5f;
    return config;
}

ActivityConfig threatDisplayConfig() {
    ActivityConfig config;
    config.type = ActivityType::THREAT_DISPLAY;
    config.minDuration = 1.5f;
    config.maxDuration = 5.0f;
    config.cooldownTime = 10.0f;
    config.priority = 9;
    config.canBeInterrupted = true;
    config.interruptPriority = 9;
    config.blendInTime = 0.2f;
    config.blendOutTime = 0.3f;
    config.requiresStationary = false;
    config.canWalkDuring = true;
    config.triggerThreshold = 0.5f;
    return config;
}

ActivityConfig submissiveConfig() {
    ActivityConfig config;
    config.type = ActivityType::SUBMISSIVE_DISPLAY;
    config.minDuration = 2.0f;
    config.maxDuration = 6.0f;
    config.cooldownTime = 5.0f;
    config.priority = 8;
    config.canBeInterrupted = true;
    config.interruptPriority = 9;
    config.blendInTime = 0.2f;
    config.blendOutTime = 0.4f;
    config.requiresStationary = true;
    config.canWalkDuring = false;
    config.triggerThreshold = 0.6f;
    return config;
}

ActivityConfig matingDisplayConfig() {
    ActivityConfig config;
    config.type = ActivityType::MATING_DISPLAY;
    config.minDuration = 3.0f;
    config.maxDuration = 12.0f;
    config.cooldownTime = 20.0f;
    config.priority = 6;
    config.canBeInterrupted = true;
    config.interruptPriority = 7;
    config.blendInTime = 0.4f;
    config.blendOutTime = 0.4f;
    config.requiresStationary = false;
    config.canWalkDuring = true;
    config.triggerThreshold = 0.5f;
    return config;
}

ActivityConfig playingConfig() {
    ActivityConfig config;
    config.type = ActivityType::PLAYING;
    config.minDuration = 5.0f;
    config.maxDuration = 20.0f;
    config.cooldownTime = 30.0f;
    config.priority = 2;
    config.canBeInterrupted = true;
    config.interruptPriority = 3;
    config.blendInTime = 0.3f;
    config.blendOutTime = 0.3f;
    config.requiresStationary = false;
    config.canWalkDuring = true;
    config.triggerThreshold = 0.4f;
    return config;
}

ActivityConfig investigatingConfig() {
    ActivityConfig config;
    config.type = ActivityType::INVESTIGATING;
    config.minDuration = 2.0f;
    config.maxDuration = 8.0f;
    config.cooldownTime = 5.0f;
    config.priority = 3;
    config.canBeInterrupted = true;
    config.interruptPriority = 4;
    config.blendInTime = 0.3f;
    config.blendOutTime = 0.2f;
    config.requiresStationary = false;
    config.canWalkDuring = true;
    config.triggerThreshold = 0.3f;
    return config;
}

ActivityConfig callingConfig() {
    ActivityConfig config;
    config.type = ActivityType::CALLING;
    config.minDuration = 1.0f;
    config.maxDuration = 4.0f;
    config.cooldownTime = 15.0f;
    config.priority = 4;
    config.canBeInterrupted = true;
    config.interruptPriority = 5;
    config.blendInTime = 0.2f;
    config.blendOutTime = 0.2f;
    config.requiresStationary = false;
    config.canWalkDuring = false;
    config.triggerThreshold = 0.5f;
    return config;
}

ActivityConfig nestBuildingConfig() {
    ActivityConfig config;
    config.type = ActivityType::NEST_BUILDING;
    config.minDuration = 5.0f;
    config.maxDuration = 20.0f;
    config.cooldownTime = 60.0f;
    config.priority = 4;
    config.canBeInterrupted = true;
    config.interruptPriority = 5;
    config.blendInTime = 0.4f;
    config.blendOutTime = 0.4f;
    config.requiresStationary = true;
    config.canWalkDuring = false;
    config.triggerThreshold = 0.5f;
    return config;
}

ActivityConfig parentalCareConfig() {
    ActivityConfig config;
    config.type = ActivityType::PARENTAL_CARE;
    config.minDuration = 3.0f;
    config.maxDuration = 15.0f;
    config.cooldownTime = 10.0f;
    config.priority = 7;  // High priority - protecting young
    config.canBeInterrupted = true;
    config.interruptPriority = 8;
    config.blendInTime = 0.3f;
    config.blendOutTime = 0.3f;
    config.requiresStationary = false;
    config.canWalkDuring = true;
    config.triggerThreshold = 0.4f;
    return config;
}

} // namespace ActivityDefaults

} // namespace animation
