#pragma once

// IslandCamera - Specialized camera controller for multi-island navigation
// Handles island switching, overview mode, and smooth transitions

#include "Camera.h"
#include "../core/MultiIslandManager.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <functional>

namespace Forge {

// ============================================================================
// Island Camera Mode
// ============================================================================

enum class IslandCameraMode {
    ISLAND_VIEW,        // Focused on a single island
    OVERVIEW,           // Bird's eye view of entire archipelago
    TRANSITION,         // Transitioning between islands
    FOLLOWING,          // Following a migrating creature
    CINEMATIC           // Automated tour of archipelago
};

// ============================================================================
// Camera Bookmark
// ============================================================================

struct CameraBookmark {
    std::string name;
    uint32_t islandIndex;
    glm::vec3 position;
    glm::vec3 target;
    float zoom;
};

// ============================================================================
// Transition Parameters
// ============================================================================

struct IslandTransitionParams {
    float duration = 2.0f;          // Transition duration in seconds
    float arcHeight = 100.0f;       // How high to arc during transition
    float minHeight = 50.0f;        // Minimum height during transition
    bool useArc = true;             // Whether to arc up during transition
    bool pauseSimulation = false;   // Whether to pause simulation during transition
};

// ============================================================================
// Island Camera
// ============================================================================

class IslandCamera {
public:
    using TransitionCallback = std::function<void(uint32_t fromIsland, uint32_t toIsland)>;

    IslandCamera();
    ~IslandCamera() = default;

    // ========================================================================
    // Initialization
    // ========================================================================

    void init(Camera* camera);

    // ========================================================================
    // Island Control
    // ========================================================================

    // Set active island (with smooth transition)
    void setActiveIsland(uint32_t index, const MultiIslandManager& islands);

    // Set active island (instant)
    void setActiveIslandImmediate(uint32_t index, const MultiIslandManager& islands);

    // Get current island (during transition, returns destination)
    uint32_t getActiveIslandIndex() const { return m_targetIslandIndex; }

    // Navigate between islands
    void nextIsland(const MultiIslandManager& islands);
    void previousIsland(const MultiIslandManager& islands);

    // ========================================================================
    // View Modes
    // ========================================================================

    // Switch to overview mode (shows entire archipelago)
    void zoomToOverview(const MultiIslandManager& islands);

    // Return from overview to island view
    void returnFromOverview(const MultiIslandManager& islands);

    // Get current mode
    IslandCameraMode getMode() const { return m_mode; }

    // ========================================================================
    // Update
    // ========================================================================

    // Main update (call every frame)
    void update(float deltaTime);

    // ========================================================================
    // Camera Access
    // ========================================================================

    glm::mat4 getViewMatrix() const;
    glm::vec3 getPosition() const { return m_currentPosition; }
    glm::vec3 getTarget() const { return m_currentTarget; }

    Camera* getCamera() { return m_camera; }
    const Camera* getCamera() const { return m_camera; }

    // ========================================================================
    // Transition State
    // ========================================================================

    bool isTransitioning() const { return m_mode == IslandCameraMode::TRANSITION; }
    float getTransitionProgress() const { return m_transitionProgress; }

    // ========================================================================
    // Configuration
    // ========================================================================

    void setTransitionParams(const IslandTransitionParams& params) { m_transitionParams = params; }
    const IslandTransitionParams& getTransitionParams() const { return m_transitionParams; }

    // Default view heights
    void setDefaultViewHeight(float height) { m_defaultViewHeight = height; }
    void setOverviewHeight(float height) { m_overviewHeight = height; }

    // Zoom limits
    void setZoomLimits(float minZoom, float maxZoom);
    void zoom(float delta);

    // ========================================================================
    // Manual Control (when in ISLAND_VIEW mode)
    // ========================================================================

    // Orbit around island center
    void orbit(float deltaYaw, float deltaPitch);

    // Pan within island bounds
    void pan(float deltaX, float deltaZ);

    // Reset to default view for current island
    void resetView(const MultiIslandManager& islands);

    // ========================================================================
    // Following Mode
    // ========================================================================

    // Follow a migrating creature
    void followMigration(const MigrationEvent& event, const MultiIslandManager& islands);
    void stopFollowing();

    // ========================================================================
    // Bookmarks
    // ========================================================================

    void addBookmark(const std::string& name);
    void gotoBookmark(const std::string& name);
    const std::vector<CameraBookmark>& getBookmarks() const { return m_bookmarks; }
    void clearBookmarks() { m_bookmarks.clear(); }

    // ========================================================================
    // Cinematic Mode
    // ========================================================================

    // Start cinematic camera (smooth orbit around archipelago)
    void startCinematicMode(const MultiIslandManager& islands);
    void stopCinematicMode();
    bool isCinematicMode() const { return m_mode == IslandCameraMode::CINEMATIC; }

    // Configure cinematic parameters
    void setCinematicSpeed(float speed) { m_cinematicSpeed = speed; }
    void setCinematicOrbitRadius(float radius) { m_cinematicOrbitRadius = radius; }
    void setCinematicHeight(float height) { m_cinematicHeight = height; }

    // ========================================================================
    // Callbacks
    // ========================================================================

    void setTransitionCallback(TransitionCallback callback) { m_transitionCallback = callback; }

private:
    // Base camera
    Camera* m_camera = nullptr;

    // Current mode
    IslandCameraMode m_mode = IslandCameraMode::ISLAND_VIEW;
    IslandCameraMode m_previousMode = IslandCameraMode::ISLAND_VIEW;

    // Island tracking
    uint32_t m_currentIslandIndex = 0;
    uint32_t m_targetIslandIndex = 0;
    uint32_t m_islandCount = 0;

    // Position state
    glm::vec3 m_currentPosition;
    glm::vec3 m_currentTarget;
    glm::vec3 m_startPosition;
    glm::vec3 m_startTarget;
    glm::vec3 m_targetPosition;
    glm::vec3 m_targetTarget;

    // Orbit state
    float m_orbitYaw = 0.0f;
    float m_orbitPitch = 30.0f;  // Looking down at 30 degrees
    float m_orbitDistance = 150.0f;

    // Zoom state
    float m_currentZoom = 1.0f;
    float m_minZoom = 0.5f;
    float m_maxZoom = 3.0f;

    // Transition state
    float m_transitionProgress = 0.0f;
    float m_transitionDuration = 2.0f;
    IslandTransitionParams m_transitionParams;

    // View heights
    float m_defaultViewHeight = 100.0f;
    float m_overviewHeight = 500.0f;

    // Following state
    const MigrationEvent* m_followingMigration = nullptr;

    // Bookmarks
    std::vector<CameraBookmark> m_bookmarks;

    // Callbacks
    TransitionCallback m_transitionCallback;

    // Stored overview state
    glm::vec3 m_overviewPosition;
    glm::vec3 m_overviewTarget;
    glm::vec3 m_preOverviewPosition;
    glm::vec3 m_preOverviewTarget;
    bool m_inOverview = false;

    // Cinematic mode state
    float m_cinematicTime = 0.0f;
    float m_cinematicSpeed = 0.05f;        // Radians per second
    float m_cinematicOrbitRadius = 400.0f;
    float m_cinematicHeight = 200.0f;
    glm::vec3 m_cinematicCenter;

    // Internal methods
    void updateIslandView(float deltaTime);
    void updateOverview(float deltaTime);
    void updateTransition(float deltaTime);
    void updateFollowing(float deltaTime);
    void updateCinematic(float deltaTime);

    void startTransition(const glm::vec3& targetPos, const glm::vec3& targetTarget, float duration);
    void completeTransition();

    // Calculate ideal camera position for an island
    glm::vec3 calculateIslandCameraPosition(uint32_t islandIndex, const MultiIslandManager& islands) const;
    glm::vec3 calculateIslandCenter(uint32_t islandIndex, const MultiIslandManager& islands) const;

    // Calculate overview camera position
    glm::vec3 calculateOverviewPosition(const MultiIslandManager& islands) const;
    glm::vec3 calculateOverviewTarget(const MultiIslandManager& islands) const;

    // Interpolation
    glm::vec3 interpolateArc(const glm::vec3& start, const glm::vec3& end, float t, float arcHeight) const;
    float easeInOutCubic(float t) const;

    // Apply camera state
    void applyCameraState();
};

} // namespace Forge
