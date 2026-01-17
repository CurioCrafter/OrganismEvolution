#pragma once

// CameraController - Enhanced camera with follow mode, underwater, flying, and cinematic transitions
// Wraps the basic Camera class with advanced behaviors
// Phase 7 Agent 9: Added cinematic presentation modes, target selection, collision avoidance, and photo mode

#include "Camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <functional>
#include <random>

// Forward declarations (global namespace)
class Creature;
class Terrain;

namespace Forge {

// Import global namespace classes into Forge namespace
using ::Camera;
using ::Creature;
using ::Terrain;

// ============================================================================
// Camera Modes
// ============================================================================

enum class CameraMode {
    FREE,           // Standard free-look camera (WASD + mouse)
    FOLLOW,         // Follow a creature from behind
    ORBIT,          // Orbit around a point/creature
    UNDERWATER,     // Underwater camera with depth effects
    FLYING,         // Bird's eye view, smooth movement
    CINEMATIC,      // Preset cinematic path
    OVERVIEW,       // Top-down overview of entire world
    FIRST_PERSON,   // First-person from creature's POV
    // Phase 7: New cinematic presentation modes
    CINEMATIC_SLOW_ORBIT,   // Slow dramatic orbit around target
    CINEMATIC_GLIDE,        // Smooth gliding motion through scene
    CINEMATIC_FOLLOW_TARGET,// Cinematic creature tracking with smart framing
    PHOTO_MODE,             // Frozen camera with manual adjustments
    // Phase 8: Creature inspection mode
    INSPECT                 // Fixed-distance focus on selected creature for inspection
};

// ============================================================================
// Cinematic Camera Configuration (Phase 7)
// ============================================================================

struct CinematicCameraConfig {
    // Movement parameters
    float orbitSpeed = 0.15f;           // Radians per second for orbit modes
    float glideSpeed = 8.0f;            // Units per second for glide mode
    float heightVariation = 0.3f;       // Vertical movement amplitude (0-1)

    // Distance and framing
    float minDistance = 15.0f;          // Minimum distance from target
    float maxDistance = 80.0f;          // Maximum distance from target
    float preferredDistance = 35.0f;    // Default distance for cinematic shots
    float heightOffset = 8.0f;          // Default height above target

    // Smoothing (critically damped spring parameters)
    float positionSmoothTime = 0.8f;    // Position smoothing (higher = smoother)
    float rotationSmoothTime = 0.5f;    // Rotation smoothing
    float fovSmoothTime = 1.5f;         // FOV transition smoothing

    // Presentation effects
    float baseFOV = 45.0f;              // Default field of view
    float cinematicFOV = 35.0f;         // Narrower FOV for dramatic effect
    float maxFOVChange = 15.0f;         // Maximum FOV variation for cinematic effect
    float rollIntensity = 0.02f;        // Camera roll for cinematic feel (radians)
    float rollSpeed = 0.3f;             // Roll oscillation speed

    // Collision avoidance
    float collisionPadding = 2.0f;      // Distance to maintain from terrain
    float underwaterAvoidanceMargin = 1.0f; // Distance above water to maintain
    bool avoidUnderwater = true;        // Auto-correct when below water
    bool avoidTerrain = true;           // Auto-correct when inside terrain
};

// ============================================================================
// Target Selection Heuristics (Phase 7)
// ============================================================================

enum class TargetSelectionMode {
    MANUAL,             // User-selected target only
    LARGEST_CREATURE,   // Auto-select largest creature
    NEAREST_ACTION,     // Select creature with most activity (high velocity)
    RANDOM_FOCUS,       // Periodically switch to random creatures
    PREDATOR_PRIORITY,  // Prioritize predator creatures
    MOST_OFFSPRING      // Select creature with most descendants
};

struct TargetSelectionConfig {
    TargetSelectionMode mode = TargetSelectionMode::MANUAL;
    float switchInterval = 15.0f;       // Seconds between auto-target switches
    float actionThreshold = 5.0f;       // Velocity threshold for "action" detection
    float smoothTransitionTime = 2.0f;  // Time to transition between targets
    bool lockTarget = false;            // Prevent auto-switching when true
};

// ============================================================================
// Camera Transition
// ============================================================================

struct CameraTransition {
    glm::vec3 startPosition;
    glm::vec3 endPosition;
    glm::vec3 startTarget;
    glm::vec3 endTarget;
    float duration;
    float elapsed;
    bool active;

    // Easing function (0-1 input, 0-1 output)
    std::function<float(float)> easing;
};

// ============================================================================
// Cinematic Keyframe
// ============================================================================

struct CameraKeyframe {
    glm::vec3 position;
    glm::vec3 target;
    float fov;
    float time;  // Time at which to reach this keyframe
};

// ============================================================================
// Underwater Camera Effects
// ============================================================================

struct UnderwaterEffects {
    float fogDensity = 0.05f;
    glm::vec3 fogColor = glm::vec3(0.0f, 0.3f, 0.5f);
    float causticIntensity = 0.5f;
    float bubbleFrequency = 2.0f;
    float distortionStrength = 0.02f;
    float depthDarkening = 0.1f;  // How much darker per unit depth
};

// ============================================================================
// Photo Mode State (Phase 7)
// ============================================================================

struct PhotoModeState {
    bool active = false;
    bool freezeCamera = true;           // Camera movement frozen
    bool freezeSimulation = false;      // Simulation paused (requested to Agent 10)
    float manualFOV = 45.0f;            // User-controlled FOV in photo mode
    float manualRoll = 0.0f;            // User-controlled roll angle
    glm::vec3 frozenPosition;           // Position when photo mode activated
    glm::vec3 frozenTarget;             // Target when photo mode activated
    float depthOfFieldFocus = 50.0f;    // Focus distance (for shader use)
    float depthOfFieldStrength = 0.0f;  // DoF blur intensity
};

// ============================================================================
// Inspect Mode Configuration (Phase 8)
// ============================================================================

struct InspectModeConfig {
    float distance = 15.0f;             // Distance from creature
    float height = 5.0f;                // Height above creature
    float orbitSpeed = 0.0f;            // Optional slow orbit (0 = static)
    float smoothTime = 0.6f;            // Position smoothing time
    float minDistance = 8.0f;           // Minimum zoom distance
    float maxDistance = 40.0f;          // Maximum zoom distance
    bool allowZoom = true;              // Allow mouse wheel zoom
    bool allowOrbit = true;             // Allow mouse drag to orbit
};

// ============================================================================
// Camera Controller
// ============================================================================

class CameraController {
public:
    CameraController();
    ~CameraController() = default;

    // Initialization
    void init(Camera* camera, float worldWidth, float worldHeight);

    // ========================================================================
    // Mode Control
    // ========================================================================

    // Set camera mode with optional transition
    void setMode(CameraMode mode, bool smooth = true);
    CameraMode getMode() const { return m_mode; }

    // Mode-specific configuration
    void setFollowTarget(const Creature* creature);
    void setOrbitTarget(const glm::vec3& target);
    void setCinematicPath(const std::vector<CameraKeyframe>& keyframes);

    // ========================================================================
    // Update
    // ========================================================================

    // Main update (call every frame)
    void update(float deltaTime);

    // Process input (returns true if input was consumed)
    bool processKeyboard(CameraMovement direction, float deltaTime);
    bool processMouseMovement(float xOffset, float yOffset);
    bool processMouseScroll(float yOffset);

    // ========================================================================
    // Follow Mode
    // ========================================================================

    // Follow distance and height
    void setFollowDistance(float distance) { m_followDistance = distance; }
    void setFollowHeight(float height) { m_followHeight = height; }
    void setFollowSmoothing(float smoothing) { m_followSmoothing = smoothing; }

    // Auto-rotate to face creature's direction
    void setAutoRotate(bool enabled) { m_autoRotate = enabled; }

    // ========================================================================
    // Orbit Mode
    // ========================================================================

    void setOrbitRadius(float radius) { m_orbitRadius = radius; }
    void setOrbitSpeed(float speed) { m_orbitSpeed = speed; }
    void setOrbitHeight(float height) { m_orbitHeight = height; }

    // ========================================================================
    // Underwater Mode
    // ========================================================================

    // Set water surface level
    void setWaterLevel(float level) { m_waterLevel = level; }

    // Check if camera is underwater
    bool isUnderwater() const { return m_camera && m_camera->Position.y < m_waterLevel; }

    // Get underwater effects (for shader parameters)
    const UnderwaterEffects& getUnderwaterEffects() const { return m_underwaterEffects; }
    void setUnderwaterEffects(const UnderwaterEffects& effects) { m_underwaterEffects = effects; }

    // Get depth below water surface
    float getUnderwaterDepth() const;

    // ========================================================================
    // Flying Mode (Bird's Eye)
    // ========================================================================

    void setFlyingHeight(float height) { m_flyingHeight = height; }
    void setFlyingSpeed(float speed) { m_flyingSpeed = speed; }

    // ========================================================================
    // Cinematic Mode (Legacy keyframe-based)
    // ========================================================================

    // Start cinematic playback
    void playCinematic();
    void pauseCinematic();
    void stopCinematic();

    // Is cinematic currently playing?
    bool isCinematicPlaying() const { return m_cinematicPlaying; }

    // Get current cinematic progress (0-1)
    float getCinematicProgress() const;

    // ========================================================================
    // Phase 7: Cinematic Presentation Modes
    // ========================================================================

    // Configure cinematic behavior
    void setCinematicConfig(const CinematicCameraConfig& config) { m_cinematicConfig = config; }
    const CinematicCameraConfig& getCinematicConfig() const { return m_cinematicConfig; }

    // Start specific cinematic modes
    void startSlowOrbit(const glm::vec3& center, float radius = -1.0f);  // -1 uses config default
    void startGlide(const glm::vec3& startPos, const glm::vec3& endPos, float duration = 10.0f);
    void startFollowTarget(const Creature* creature);

    // Get current cinematic roll (for shader effects)
    float getCinematicRoll() const { return m_cinematicRoll; }

    // Get current effective FOV (includes cinematic adjustments)
    float getCinematicFOV() const { return m_cinematicFOV; }

    // ========================================================================
    // Phase 7: Target Selection System
    // ========================================================================

    // Configure target selection
    void setTargetSelectionConfig(const TargetSelectionConfig& config) { m_targetConfig = config; }
    const TargetSelectionConfig& getTargetSelectionConfig() const { return m_targetConfig; }

    // Set the creature pool for auto-target selection
    void setCreaturePool(const std::vector<Creature*>* creatures) { m_creaturePool = creatures; }

    // Lock/unlock target (prevents auto-switching)
    void lockTarget(bool locked) { m_targetConfig.lockTarget = locked; }
    bool isTargetLocked() const { return m_targetConfig.lockTarget; }

    // Override current target (for Agent 10 integration)
    void overrideTarget(const Creature* creature);
    void overrideTargetPosition(const glm::vec3& position);
    void clearTargetOverride();
    bool hasTargetOverride() const { return m_targetOverrideActive; }

    // Get current target info
    const Creature* getCurrentTarget() const { return m_currentCinematicTarget; }
    glm::vec3 getCurrentTargetPosition() const;

    // ========================================================================
    // Phase 7: Collision and Terrain Avoidance
    // ========================================================================

    // Set terrain for collision detection
    void setTerrain(const Terrain* terrain) { m_terrain = terrain; }

    // Manual collision check (returns corrected position)
    glm::vec3 applyCollisionAvoidance(const glm::vec3& desiredPosition) const;

    // Check if position would collide
    bool wouldCollide(const glm::vec3& position) const;

    // ========================================================================
    // Phase 7: Photo Mode
    // ========================================================================

    // Enter/exit photo mode
    void enterPhotoMode();
    void exitPhotoMode();
    bool isInPhotoMode() const { return m_photoMode.active; }

    // Photo mode controls
    void setPhotoModeFOV(float fov);
    void setPhotoModeRoll(float roll);
    void setPhotoModeFocus(float distance, float strength);
    void photoModeNudge(const glm::vec3& offset);  // Small position adjustments

    // Get photo mode state (for UI display)
    const PhotoModeState& getPhotoModeState() const { return m_photoMode; }

    // Request simulation freeze (Agent 10 should listen to this)
    std::function<void(bool)> onPhotoModeFreeze;  // Callback for Agent 10

    // ========================================================================
    // Phase 8: Inspect Mode (Creature Inspection Panel Integration)
    // ========================================================================

    // Enter inspect mode focused on a creature
    void startInspect(const Creature* creature);

    // Exit inspect mode and return to free camera
    void exitInspect();

    // Check if in inspect mode
    bool isInspectMode() const { return m_mode == CameraMode::INSPECT; }

    // Configure inspect mode behavior
    void setInspectConfig(const InspectModeConfig& config) { m_inspectConfig = config; }
    const InspectModeConfig& getInspectConfig() const { return m_inspectConfig; }

    // Get the creature being inspected
    const Creature* getInspectedCreature() const { return m_inspectTarget; }

    // Adjust inspect view (for mouse interaction)
    void inspectZoom(float delta);    // Zoom in/out
    void inspectOrbit(float yaw, float pitch);  // Orbit around creature

    // ========================================================================
    // Transitions
    // ========================================================================

    // Smoothly transition to a new position/target
    void transitionTo(const glm::vec3& position, const glm::vec3& target,
                      float duration = 1.0f);

    // Is currently transitioning?
    bool isTransitioning() const { return m_transition.active; }

    // Skip current transition
    void skipTransition();

    // ========================================================================
    // Shake Effects
    // ========================================================================

    // Add camera shake (for impacts, etc.)
    void addShake(float intensity, float duration);

    // ========================================================================
    // Bounds & Constraints
    // ========================================================================

    // Set world bounds (camera won't go outside)
    void setWorldBounds(const glm::vec3& min, const glm::vec3& max);

    // Set minimum height above terrain
    void setMinHeight(float height) { m_minHeight = height; }

    // Enable/disable bounds clamping
    void setBoundsEnabled(bool enabled) { m_boundsEnabled = enabled; }

    // ========================================================================
    // Accessors
    // ========================================================================

    Camera* getCamera() { return m_camera; }
    const Camera* getCamera() const { return m_camera; }

    glm::vec3 getPosition() const { return m_camera ? m_camera->Position : glm::vec3(0); }
    glm::vec3 getTarget() const { return m_targetPoint; }

    // Get effective FOV (may be modified by underwater, etc.)
    float getEffectiveFOV() const;

private:
    // The underlying camera
    Camera* m_camera = nullptr;

    // Current mode
    CameraMode m_mode = CameraMode::FREE;

    // World dimensions
    float m_worldWidth = 500.0f;
    float m_worldHeight = 500.0f;

    // Follow mode state
    const Creature* m_followTarget = nullptr;
    float m_followDistance = 20.0f;
    float m_followHeight = 10.0f;
    float m_followSmoothing = 5.0f;
    bool m_autoRotate = true;
    float m_followYaw = 0.0f;  // User-controlled yaw offset

    // Orbit mode state
    glm::vec3 m_orbitCenter = glm::vec3(0.0f);
    float m_orbitRadius = 30.0f;
    float m_orbitHeight = 15.0f;
    float m_orbitAngle = 0.0f;
    float m_orbitSpeed = 0.5f;

    // Target point (what camera is looking at)
    glm::vec3 m_targetPoint = glm::vec3(0.0f);

    // Underwater
    float m_waterLevel = 0.0f;
    UnderwaterEffects m_underwaterEffects;

    // Flying mode
    float m_flyingHeight = 100.0f;
    float m_flyingSpeed = 50.0f;

    // Cinematic mode
    std::vector<CameraKeyframe> m_cinematicKeyframes;
    float m_cinematicTime = 0.0f;
    bool m_cinematicPlaying = false;
    bool m_cinematicPaused = false;

    // Transition
    CameraTransition m_transition;

    // Shake
    float m_shakeIntensity = 0.0f;
    float m_shakeDuration = 0.0f;
    float m_shakeTimer = 0.0f;

    // Bounds
    glm::vec3 m_boundsMin = glm::vec3(-250.0f, 0.0f, -250.0f);
    glm::vec3 m_boundsMax = glm::vec3(250.0f, 200.0f, 250.0f);
    float m_minHeight = 2.0f;
    bool m_boundsEnabled = true;

    // ========================================================================
    // Phase 7: Cinematic Presentation State
    // ========================================================================

    CinematicCameraConfig m_cinematicConfig;

    // Slow orbit mode state
    glm::vec3 m_slowOrbitCenter = glm::vec3(0.0f);
    float m_slowOrbitRadius = 35.0f;
    float m_slowOrbitAngle = 0.0f;
    float m_slowOrbitVerticalPhase = 0.0f;

    // Glide mode state
    glm::vec3 m_glideStart = glm::vec3(0.0f);
    glm::vec3 m_glideEnd = glm::vec3(0.0f);
    float m_glideDuration = 10.0f;
    float m_glideProgress = 0.0f;

    // Cinematic target follow state
    const Creature* m_currentCinematicTarget = nullptr;
    glm::vec3 m_cinematicTargetVelocity = glm::vec3(0.0f);

    // Cinematic presentation effects
    float m_cinematicRoll = 0.0f;       // Current roll angle
    float m_cinematicRollPhase = 0.0f;  // Phase for roll oscillation
    float m_cinematicFOV = 45.0f;       // Current effective FOV
    float m_targetCinematicFOV = 45.0f; // Target FOV for smoothing

    // ========================================================================
    // Phase 7: Target Selection State
    // ========================================================================

    TargetSelectionConfig m_targetConfig;
    const std::vector<Creature*>* m_creaturePool = nullptr;
    float m_targetSwitchTimer = 0.0f;

    // Target override (for Agent 10 integration)
    bool m_targetOverrideActive = false;
    const Creature* m_overrideCreature = nullptr;
    glm::vec3 m_overridePosition = glm::vec3(0.0f);
    bool m_usePositionOverride = false;

    // Random number generation for random focus mode
    mutable std::mt19937 m_rng;

    // ========================================================================
    // Phase 7: Collision Avoidance State
    // ========================================================================

    const Terrain* m_terrain = nullptr;

    // ========================================================================
    // Phase 7: Photo Mode State
    // ========================================================================

    PhotoModeState m_photoMode;

    // ========================================================================
    // Phase 8: Inspect Mode State
    // ========================================================================

    InspectModeConfig m_inspectConfig;
    const Creature* m_inspectTarget = nullptr;
    float m_inspectYaw = 0.0f;          // User-controlled orbit angle
    float m_inspectPitch = 0.3f;        // User-controlled pitch (radians)
    float m_inspectDistance = 15.0f;    // Current zoom distance

    // ========================================================================
    // Smoothing velocities for critically damped springs
    // ========================================================================

    glm::vec3 m_positionVelocity = glm::vec3(0.0f);
    glm::vec3 m_targetVelocity = glm::vec3(0.0f);
    float m_fovVelocity = 0.0f;
    float m_rollVelocity = 0.0f;

    // Helper methods
    void updateFreeMode(float deltaTime);
    void updateFollowMode(float deltaTime);
    void updateOrbitMode(float deltaTime);
    void updateUnderwaterMode(float deltaTime);
    void updateFlyingMode(float deltaTime);
    void updateCinematicMode(float deltaTime);
    void updateOverviewMode(float deltaTime);
    void updateFirstPersonMode(float deltaTime);

    // Phase 7: New cinematic mode updates
    void updateSlowOrbitMode(float deltaTime);
    void updateGlideMode(float deltaTime);
    void updateFollowTargetMode(float deltaTime);
    void updatePhotoMode(float deltaTime);

    // Phase 8: Inspect mode update
    void updateInspectMode(float deltaTime);

    // Phase 7: Target selection logic
    void updateTargetSelection(float deltaTime);
    const Creature* selectBestTarget() const;
    const Creature* findLargestCreature() const;
    const Creature* findMostActiveCreature() const;
    const Creature* findRandomCreature() const;
    const Creature* findPredatorCreature() const;

    // Phase 7: Collision helpers
    float getTerrainHeight(float x, float z) const;
    bool isPositionUnderwater(const glm::vec3& pos) const;
    glm::vec3 correctForTerrain(const glm::vec3& pos) const;
    glm::vec3 correctForWater(const glm::vec3& pos) const;

    // Phase 7: Cinematic effects
    void updateCinematicEffects(float deltaTime);
    void updateCinematicRoll(float deltaTime);
    void updateCinematicFOV(float deltaTime);

    void updateTransition(float deltaTime);
    void updateShake(float deltaTime);
    void applyBounds();

    // Interpolation helpers
    glm::vec3 smoothDamp(const glm::vec3& current, const glm::vec3& target,
                         glm::vec3& velocity, float smoothTime, float deltaTime);
    float smoothDampFloat(float current, float target, float& velocity,
                          float smoothTime, float deltaTime);

    // Easing functions
    static float easeInOutCubic(float t);
    static float easeOutQuad(float t);
    static float easeInOutSine(float t);

    // Get keyframe at time
    CameraKeyframe interpolateKeyframes(float time) const;
};

} // namespace Forge
