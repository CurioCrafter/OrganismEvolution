#pragma once

// CinematicCamera - Advanced cinematic presentation system
// Provides SlowOrbit, Glide, and FollowTarget modes with smooth transitions
// Integrates with CameraController for enhanced visual experience

#include "Camera.h"
#include "CameraController.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <functional>
#include <vector>
#include <random>

// Forward declarations
class Creature;
class Terrain;

namespace Forge {

// ============================================================================
// Cinematic Camera Modes
// ============================================================================

enum class CinematicMode {
    DISABLED,           // Standard camera behavior
    SLOW_ORBIT,         // Slowly orbit around a target
    GLIDE,              // Smooth glide through the world
    FOLLOW_TARGET,      // Follow a specific creature with cinematic framing
    AUTO_DIRECTOR       // Automatically switch between interesting subjects
};

// ============================================================================
// Target Selection Heuristic
// ============================================================================

enum class TargetSelectionHeuristic {
    LARGEST_CREATURE,   // Select the largest creature by size
    NEAREST_ACTION,     // Select creature with most activity nearby
    RANDOM_FOCUS,       // Random selection from active creatures
    MOST_OFFSPRING,     // Creature with most offspring (successful genes)
    OLDEST_LIVING,      // Longest-lived creature
    USER_SELECTED       // User/UI has locked a specific target
};

// ============================================================================
// Cinematic Camera Configuration
// ============================================================================

struct CinematicCameraConfig {
    // Movement parameters
    float orbitSpeed = 0.15f;           // Radians per second for orbit
    float glideSpeed = 8.0f;            // Units per second for glide
    float followDistance = 25.0f;       // Distance from followed target
    float followHeight = 12.0f;         // Height above target

    // Smoothing (critically damped spring parameters)
    float positionSmoothTime = 0.8f;    // Position interpolation time
    float rotationSmoothTime = 0.5f;    // Rotation interpolation time
    float fovSmoothTime = 1.5f;         // FOV transition time

    // Presentation
    float minFOV = 35.0f;               // Narrow for dramatic shots
    float maxFOV = 65.0f;               // Wide for establishing shots
    float defaultFOV = 50.0f;           // Standard cinematic FOV
    float maxRollAngle = 3.0f;          // Subtle roll in degrees
    float rollSpeed = 0.3f;             // How fast roll changes

    // Collision avoidance
    float minTerrainClearance = 5.0f;   // Minimum height above terrain
    float minWaterClearance = 2.0f;     // Minimum height above water
    float collisionCheckRadius = 3.0f;  // Radius for collision detection

    // Auto-director timing
    float minShotDuration = 5.0f;       // Minimum time on a subject
    float maxShotDuration = 20.0f;      // Maximum time before switching
    float transitionDuration = 2.0f;    // Time to transition between targets
};

// ============================================================================
// Camera State Snapshot (for smooth interpolation)
// ============================================================================

struct CameraStateSnapshot {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 target = glm::vec3(0.0f);
    float fov = 45.0f;
    float roll = 0.0f;

    // Interpolate between two snapshots
    static CameraStateSnapshot lerp(const CameraStateSnapshot& a,
                                     const CameraStateSnapshot& b,
                                     float t);
};

// ============================================================================
// Target Info (for selection system)
// ============================================================================

struct CinematicTarget {
    const Creature* creature = nullptr;
    glm::vec3 position = glm::vec3(0.0f);
    float interestScore = 0.0f;         // Higher = more interesting
    float size = 1.0f;
    bool isActive = true;
};

// ============================================================================
// Glide Waypoint
// ============================================================================

struct GlideWaypoint {
    glm::vec3 position;
    glm::vec3 lookTarget;
    float fov = 50.0f;
    float duration = 5.0f;              // Time to reach this waypoint
};

// ============================================================================
// Cinematic Camera Controller
// ============================================================================

class CinematicCamera {
public:
    // Callbacks for UI integration
    using TargetChangedCallback = std::function<void(const Creature* newTarget)>;
    using ModeChangedCallback = std::function<void(CinematicMode newMode)>;

    CinematicCamera();
    ~CinematicCamera() = default;

    // ========================================================================
    // Initialization
    // ========================================================================

    void init(Camera* camera, CameraController* controller);
    void setTerrain(const Terrain* terrain) { m_terrain = terrain; }
    void setWaterLevel(float level) { m_waterLevel = level; }

    // ========================================================================
    // Mode Control (API for Agent 10 UI integration)
    // ========================================================================

    void setMode(CinematicMode mode, bool smooth = true);
    CinematicMode getMode() const { return m_mode; }
    bool isActive() const { return m_mode != CinematicMode::DISABLED; }

    // Mode-specific configuration
    void setConfig(const CinematicCameraConfig& config) { m_config = config; }
    const CinematicCameraConfig& getConfig() const { return m_config; }

    // ========================================================================
    // Target Selection (API for Agent 10 UI integration)
    // ========================================================================

    // Set the target selection heuristic
    void setTargetHeuristic(TargetSelectionHeuristic heuristic);
    TargetSelectionHeuristic getTargetHeuristic() const { return m_heuristic; }

    // Override/lock target (UI can call this)
    void lockTarget(const Creature* creature);
    void unlockTarget();
    bool isTargetLocked() const { return m_targetLocked; }

    // Get current target
    const Creature* getCurrentTarget() const { return m_currentTarget; }

    // Provide creature list for target selection
    void updateCreatureList(const std::vector<const Creature*>& creatures);

    // ========================================================================
    // Glide Mode Configuration
    // ========================================================================

    void setGlidePath(const std::vector<GlideWaypoint>& waypoints);
    void addGlideWaypoint(const GlideWaypoint& waypoint);
    void clearGlidePath();

    // Generate automatic glide path around world
    void generateAutoGlidePath(float worldWidth, float worldHeight, int numWaypoints = 8);

    // ========================================================================
    // Orbit Configuration
    // ========================================================================

    void setOrbitCenter(const glm::vec3& center);
    void setOrbitRadius(float radius) { m_orbitRadius = radius; }
    void setOrbitHeight(float height) { m_orbitHeight = height; }

    // ========================================================================
    // Photo Mode
    // ========================================================================

    void enablePhotoMode(bool enable);
    bool isPhotoModeEnabled() const { return m_photoModeEnabled; }

    // Manual control in photo mode
    void photoModeRotate(float yaw, float pitch);
    void photoModeZoom(float delta);
    void photoModePan(float x, float y);

    // ========================================================================
    // Update
    // ========================================================================

    void update(float deltaTime);

    // ========================================================================
    // Callbacks (for UI notification)
    // ========================================================================

    void setTargetChangedCallback(TargetChangedCallback callback) { m_targetChangedCallback = callback; }
    void setModeChangedCallback(ModeChangedCallback callback) { m_modeChangedCallback = callback; }

    // ========================================================================
    // Presentation State
    // ========================================================================

    float getCurrentFOV() const { return m_currentState.fov; }
    float getCurrentRoll() const { return m_currentState.roll; }
    glm::vec3 getCurrentPosition() const { return m_currentState.position; }
    glm::vec3 getCurrentTarget() const { return m_currentState.target; }

private:
    // Core references
    Camera* m_camera = nullptr;
    CameraController* m_controller = nullptr;
    const Terrain* m_terrain = nullptr;
    float m_waterLevel = 0.0f;

    // Mode state
    CinematicMode m_mode = CinematicMode::DISABLED;
    CinematicMode m_previousMode = CinematicMode::DISABLED;
    CinematicCameraConfig m_config;

    // Camera state
    CameraStateSnapshot m_currentState;
    CameraStateSnapshot m_targetState;

    // Smoothing velocities (for critically damped spring)
    glm::vec3 m_positionVelocity = glm::vec3(0.0f);
    glm::vec3 m_targetVelocity = glm::vec3(0.0f);
    float m_fovVelocity = 0.0f;
    float m_rollVelocity = 0.0f;

    // Target selection
    TargetSelectionHeuristic m_heuristic = TargetSelectionHeuristic::LARGEST_CREATURE;
    const Creature* m_currentTarget = nullptr;
    bool m_targetLocked = false;
    std::vector<CinematicTarget> m_potentialTargets;

    // Auto-director
    float m_shotTimer = 0.0f;
    float m_currentShotDuration = 10.0f;

    // Orbit state
    glm::vec3 m_orbitCenter = glm::vec3(0.0f);
    float m_orbitRadius = 50.0f;
    float m_orbitHeight = 30.0f;
    float m_orbitAngle = 0.0f;

    // Glide state
    std::vector<GlideWaypoint> m_glideWaypoints;
    size_t m_currentWaypointIndex = 0;
    float m_waypointProgress = 0.0f;

    // Photo mode
    bool m_photoModeEnabled = false;
    float m_photoYaw = 0.0f;
    float m_photoPitch = 0.0f;
    float m_photoZoom = 1.0f;

    // Transition state
    bool m_inTransition = false;
    float m_transitionProgress = 0.0f;
    CameraStateSnapshot m_transitionStart;
    CameraStateSnapshot m_transitionEnd;

    // RNG for random selection
    std::mt19937 m_rng;

    // Callbacks
    TargetChangedCallback m_targetChangedCallback;
    ModeChangedCallback m_modeChangedCallback;

    // ========================================================================
    // Update Methods
    // ========================================================================

    void updateSlowOrbit(float deltaTime);
    void updateGlide(float deltaTime);
    void updateFollowTarget(float deltaTime);
    void updateAutoDirector(float deltaTime);
    void updateTransition(float deltaTime);
    void updatePhotoMode(float deltaTime);

    // ========================================================================
    // Target Selection Methods
    // ========================================================================

    const Creature* selectTarget();
    const Creature* selectLargestCreature();
    const Creature* selectNearestAction();
    const Creature* selectRandomCreature();
    const Creature* selectMostOffspring();
    const Creature* selectOldestLiving();
    float calculateInterestScore(const Creature* creature);

    // ========================================================================
    // Collision Avoidance
    // ========================================================================

    glm::vec3 applyCollisionAvoidance(const glm::vec3& desiredPosition);
    float getTerrainHeight(float x, float z) const;
    bool isPositionSafe(const glm::vec3& position) const;

    // ========================================================================
    // Smoothing Helpers
    // ========================================================================

    float smoothDampFloat(float current, float target, float& velocity,
                          float smoothTime, float deltaTime);
    glm::vec3 smoothDampVec3(const glm::vec3& current, const glm::vec3& target,
                             glm::vec3& velocity, float smoothTime, float deltaTime);

    // ========================================================================
    // Easing Functions
    // ========================================================================

    static float easeInOutCubic(float t);
    static float easeOutQuad(float t);
    static float easeInOutSine(float t);

    // ========================================================================
    // State Application
    // ========================================================================

    void applyStateToCamera();
    void startTransition(const CameraStateSnapshot& target, float duration);
};

} // namespace Forge
