#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>
#include <functional>
#include <cstdint>

namespace animation {

// Forward declarations
class Skeleton;
struct SkeletonPose;
class ProceduralLocomotion;
class IKSystem;

// =============================================================================
// ACTIVITY STATES
// Activities are high-level behavioral states that drive visible animations
// =============================================================================

enum class ActivityType {
    IDLE,               // Default resting state
    EATING,             // Consuming food
    DRINKING,           // Consuming water (if applicable)
    MATING,             // Reproduction behavior
    SLEEPING,           // Rest/sleep state
    EXCRETING,          // Peeing/pooping
    GROOMING,           // Self-cleaning/maintenance
    THREAT_DISPLAY,     // Warning/territorial display
    SUBMISSIVE_DISPLAY, // Submissive posture
    MATING_DISPLAY,     // Courtship display
    NEST_BUILDING,      // Building shelter
    PLAYING,            // Playful behavior (juveniles)
    INVESTIGATING,      // Examining something
    CALLING,            // Vocalizing
    PARENTAL_CARE,      // Caring for young (feeding, protecting)
    COUNT               // Number of activity types
};

// Excretion sub-types
enum class ExcretionType {
    URINATE,
    DEFECATE
};

// Grooming sub-types
enum class GroomingType {
    SCRATCH,
    LICK,
    SHAKE,
    PREEN,          // For flying creatures
    STRETCH
};

// Display sub-types
enum class DisplayType {
    CREST_RAISE,
    WING_SPREAD,
    TAIL_FAN,
    BODY_INFLATE,
    COLOR_FLASH,    // For bioluminescent creatures
    VOCALIZE
};

// =============================================================================
// ACTIVITY TRIGGERS
// Conditions that can initiate activity transitions
// =============================================================================

struct ActivityTriggers {
    // Energy/hunger
    float hungerLevel = 0.0f;           // 0 = full, 1 = starving
    float thirstLevel = 0.0f;           // 0 = hydrated, 1 = dehydrated
    float energyLevel = 1.0f;           // 0 = exhausted, 1 = full energy

    // Reproduction
    float reproductionUrge = 0.0f;      // 0 = none, 1 = maximum
    bool potentialMateNearby = false;
    float mateDistance = 0.0f;

    // Physiological
    float bladderFullness = 0.0f;       // 0 = empty, 1 = full
    float bowelFullness = 0.0f;         // 0 = empty, 1 = full
    float dirtyLevel = 0.0f;            // 0 = clean, 1 = needs grooming
    float fatigueLevel = 0.0f;          // 0 = rested, 1 = exhausted

    // Environmental/social
    float threatLevel = 0.0f;           // 0 = safe, 1 = danger
    float socialNeed = 0.0f;            // 0 = satisfied, 1 = lonely
    bool territoryIntruder = false;
    bool foodNearby = false;
    float foodDistance = 0.0f;

    // Age-related
    bool isJuvenile = false;
    float playUrge = 0.0f;              // Higher for young creatures

    // Parental care
    bool hasOffspringNearby = false;    // Young offspring within range
    float offspringHungerLevel = 0.0f;  // 0 = fed, 1 = hungry
    float parentalUrge = 0.0f;          // 0 = none, 1 = strong
};

// =============================================================================
// ACTIVITY EVENT
// Fired when activity transitions occur (for UI/other systems)
// =============================================================================

enum class ActivityEventType {
    STARTED,
    COMPLETED,
    INTERRUPTED,
    FAILED
};

struct ActivityEvent {
    ActivityType activity;
    ActivityEventType eventType;
    float timestamp;
    uint32_t creatureId;

    // Optional extra data
    glm::vec3 position;
    glm::vec3 targetPosition;       // For eating/drinking location
};

// Event callback
using ActivityEventCallback = std::function<void(const ActivityEvent&)>;

// =============================================================================
// ACTIVITY CONFIG
// Parameters for each activity type
// =============================================================================

struct ActivityConfig {
    ActivityType type;

    // Timing
    float minDuration = 1.0f;           // Minimum time in activity
    float maxDuration = 5.0f;           // Maximum time in activity
    float cooldownTime = 10.0f;         // Minimum time between occurrences

    // Priority (higher = more important)
    int priority = 0;

    // Can this activity be interrupted?
    bool canBeInterrupted = true;
    int interruptPriority = 0;          // Min priority to interrupt

    // Blend parameters
    float blendInTime = 0.3f;
    float blendOutTime = 0.3f;

    // Locomotion requirements
    bool requiresStationary = true;     // Must stop moving?
    bool canWalkDuring = false;         // Can walk slowly during?

    // Trigger thresholds
    float triggerThreshold = 0.7f;      // Level at which activity triggers
};

// =============================================================================
// ACTIVITY STATE MACHINE
// =============================================================================

class ActivityStateMachine {
public:
    ActivityStateMachine();
    ~ActivityStateMachine() = default;

    // Initialize default activities
    void initialize();

    // Configure activities
    void setActivityConfig(ActivityType type, const ActivityConfig& config);
    const ActivityConfig& getActivityConfig(ActivityType type) const;

    // Update triggers from creature state
    void setTriggers(const ActivityTriggers& triggers);
    const ActivityTriggers& getTriggers() const { return m_triggers; }

    // Update state machine
    void update(float deltaTime);

    // Force activity transition (for external control)
    bool requestActivity(ActivityType type, bool force = false);
    void cancelActivity();

    // Query current state
    ActivityType getCurrentActivity() const { return m_currentActivity; }
    ActivityType getPreviousActivity() const { return m_previousActivity; }
    float getActivityProgress() const { return m_activityProgress; }
    float getActivityDuration() const { return m_currentDuration; }
    bool isInActivity() const { return m_currentActivity != ActivityType::IDLE; }
    bool isTransitioning() const { return m_isTransitioning; }
    float getTransitionProgress() const { return m_transitionProgress; }

    // Sub-type queries
    ExcretionType getExcretionType() const { return m_excretionType; }
    GroomingType getGroomingType() const { return m_groomingType; }
    DisplayType getDisplayType() const { return m_displayType; }

    // Event handling
    void registerEventCallback(ActivityEventCallback callback);
    void clearEventCallbacks();

    // Cooldown queries
    float getCooldownRemaining(ActivityType type) const;
    bool isOnCooldown(ActivityType type) const;

    // Debug
    std::string getCurrentActivityName() const;
    std::string getDebugInfo() const;

private:
    // Current state
    ActivityType m_currentActivity = ActivityType::IDLE;
    ActivityType m_previousActivity = ActivityType::IDLE;
    ActivityType m_nextActivity = ActivityType::IDLE;

    // Timing
    float m_activityTime = 0.0f;        // Time in current activity
    float m_activityProgress = 0.0f;    // 0-1 progress through activity
    float m_currentDuration = 0.0f;     // Duration of current activity

    // Transition
    bool m_isTransitioning = false;
    float m_transitionProgress = 0.0f;
    float m_transitionDuration = 0.3f;

    // Sub-types (for activities with variants)
    ExcretionType m_excretionType = ExcretionType::URINATE;
    GroomingType m_groomingType = GroomingType::STRETCH;
    DisplayType m_displayType = DisplayType::CREST_RAISE;

    // Triggers
    ActivityTriggers m_triggers;

    // Configuration
    ActivityConfig m_configs[static_cast<size_t>(ActivityType::COUNT)];

    // Cooldowns
    float m_cooldowns[static_cast<size_t>(ActivityType::COUNT)] = {0};

    // Event callbacks
    std::vector<ActivityEventCallback> m_callbacks;
    uint32_t m_creatureId = 0;

    // Internal methods
    void initializeDefaultConfigs();
    ActivityType evaluateBestActivity() const;
    float calculateActivityScore(ActivityType type) const;
    void transitionTo(ActivityType newActivity);
    void completeActivity();
    void emitEvent(ActivityEventType eventType);
    void updateCooldowns(float deltaTime);
    void selectSubType(ActivityType type);
};

// =============================================================================
// ACTIVITY ANIMATION DRIVER
// Applies activity state to animation systems
// =============================================================================

class ActivityAnimationDriver {
public:
    ActivityAnimationDriver() = default;

    // Set the state machine to drive from
    void setStateMachine(ActivityStateMachine* stateMachine) { m_stateMachine = stateMachine; }

    // Update animation based on activity state
    void update(float deltaTime);

    // Apply to skeleton pose
    void applyToPose(
        const Skeleton& skeleton,
        SkeletonPose& pose,
        ProceduralLocomotion* locomotion,
        IKSystem* ikSystem
    );

    // Get animation weight for activity overlay
    float getActivityWeight() const { return m_activityWeight; }

    // Get body position offset from activity
    glm::vec3 getBodyOffset() const { return m_bodyOffset; }
    glm::quat getBodyRotation() const { return m_bodyRotation; }

    // Head target for looking
    bool hasHeadTarget() const { return m_hasHeadTarget; }
    glm::vec3 getHeadTarget() const { return m_headTarget; }

    // Set target positions for activities
    void setFoodPosition(const glm::vec3& pos) { m_foodPosition = pos; }
    void setMatePosition(const glm::vec3& pos) { m_matePosition = pos; }
    void setGroundPosition(const glm::vec3& pos) { m_groundPosition = pos; }

    // Configure morphology-based parameters
    void setHasWings(bool hasWings) { m_hasWings = hasWings; }
    void setHasTail(bool hasTail) { m_hasTail = hasTail; }
    void setHasCrest(bool hasCrest) { m_hasCrest = hasCrest; }
    void setBodySize(float size) { m_bodySize = size; }

private:
    ActivityStateMachine* m_stateMachine = nullptr;

    // Animation state
    float m_activityWeight = 0.0f;
    float m_animationTime = 0.0f;

    // Body modifications
    glm::vec3 m_bodyOffset{0.0f};
    glm::quat m_bodyRotation{1.0f, 0.0f, 0.0f, 0.0f};

    // Head IK target
    bool m_hasHeadTarget = false;
    glm::vec3 m_headTarget{0.0f};

    // Target positions
    glm::vec3 m_foodPosition{0.0f};
    glm::vec3 m_matePosition{0.0f};
    glm::vec3 m_groundPosition{0.0f};

    // Morphology info
    bool m_hasWings = false;
    bool m_hasTail = true;
    bool m_hasCrest = false;
    float m_bodySize = 1.0f;

    // Animation generators
    void generateIdleAnimation(float progress);
    void generateEatingAnimation(float progress);
    void generateDrinkingAnimation(float progress);
    void generateMatingAnimation(float progress);
    void generateSleepingAnimation(float progress);
    void generateExcretingAnimation(float progress, ExcretionType type);
    void generateGroomingAnimation(float progress, GroomingType type);
    void generateThreatDisplayAnimation(float progress);
    void generateSubmissiveAnimation(float progress);
    void generateMatingDisplayAnimation(float progress);
    void generatePlayingAnimation(float progress);
    void generateInvestigatingAnimation(float progress);
    void generateCallingAnimation(float progress);
    void generateParentalCareAnimation(float progress);
    void generateNestBuildingAnimation(float progress);

    // Shared motion primitives
    glm::quat calculateHeadBob(float time, float amplitude) const;
    glm::vec3 calculateBodySway(float time, float amplitude) const;
    glm::quat calculateTailWag(float time, float amplitude) const;
};

// =============================================================================
// ACTIVITY NAME UTILITIES
// =============================================================================

namespace ActivityNames {
    const char* getName(ActivityType type);
    const char* getDescription(ActivityType type);
    const char* getExcretionName(ExcretionType type);
    const char* getGroomingName(GroomingType type);
    const char* getDisplayName(DisplayType type);
}

// =============================================================================
// DEFAULT ACTIVITY CONFIGURATIONS
// =============================================================================

namespace ActivityDefaults {
    ActivityConfig idleConfig();
    ActivityConfig eatingConfig();
    ActivityConfig drinkingConfig();
    ActivityConfig matingConfig();
    ActivityConfig sleepingConfig();
    ActivityConfig excretingConfig();
    ActivityConfig groomingConfig();
    ActivityConfig threatDisplayConfig();
    ActivityConfig submissiveConfig();
    ActivityConfig matingDisplayConfig();
    ActivityConfig nestBuildingConfig();
    ActivityConfig playingConfig();
    ActivityConfig investigatingConfig();
    ActivityConfig callingConfig();
    ActivityConfig parentalCareConfig();
}

} // namespace animation
