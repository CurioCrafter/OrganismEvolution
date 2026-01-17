#include "IslandCamera.h"
#include <algorithm>
#include <cmath>

namespace Forge {

// ============================================================================
// Constructor
// ============================================================================

IslandCamera::IslandCamera()
    : m_currentPosition(0.0f, 100.0f, 100.0f)
    , m_currentTarget(0.0f)
    , m_startPosition(m_currentPosition)
    , m_startTarget(m_currentTarget)
    , m_targetPosition(m_currentPosition)
    , m_targetTarget(m_currentTarget) {
}

// ============================================================================
// Initialization
// ============================================================================

void IslandCamera::init(Camera* camera) {
    m_camera = camera;

    if (m_camera) {
        m_currentPosition = m_camera->Position;
        m_currentTarget = m_camera->Position + m_camera->Front * 100.0f;
    }
}

// ============================================================================
// Island Control
// ============================================================================

void IslandCamera::setActiveIsland(uint32_t index, const MultiIslandManager& islands) {
    if (index >= islands.getIslandCount()) return;
    if (index == m_targetIslandIndex && !m_inOverview) return;  // Already there

    m_islandCount = islands.getIslandCount();

    // Store previous island for transition
    uint32_t previousIsland = m_currentIslandIndex;

    // Calculate target camera position for new island
    glm::vec3 newPosition = calculateIslandCameraPosition(index, islands);
    glm::vec3 newTarget = calculateIslandCenter(index, islands);

    // Start transition
    m_targetIslandIndex = index;
    startTransition(newPosition, newTarget, m_transitionParams.duration);

    // If coming from overview, adjust transition
    if (m_inOverview) {
        m_inOverview = false;
    }

    // Notify callback
    if (m_transitionCallback) {
        m_transitionCallback(previousIsland, index);
    }
}

void IslandCamera::setActiveIslandImmediate(uint32_t index, const MultiIslandManager& islands) {
    if (index >= islands.getIslandCount()) return;

    m_islandCount = islands.getIslandCount();
    m_currentIslandIndex = index;
    m_targetIslandIndex = index;

    m_currentPosition = calculateIslandCameraPosition(index, islands);
    m_currentTarget = calculateIslandCenter(index, islands);
    m_targetPosition = m_currentPosition;
    m_targetTarget = m_currentTarget;

    m_mode = IslandCameraMode::ISLAND_VIEW;
    m_transitionProgress = 1.0f;
    m_inOverview = false;

    applyCameraState();
}

void IslandCamera::nextIsland(const MultiIslandManager& islands) {
    if (islands.getIslandCount() == 0) return;

    uint32_t next = (m_targetIslandIndex + 1) % islands.getIslandCount();
    setActiveIsland(next, islands);
}

void IslandCamera::previousIsland(const MultiIslandManager& islands) {
    if (islands.getIslandCount() == 0) return;

    uint32_t prev = (m_targetIslandIndex + islands.getIslandCount() - 1) % islands.getIslandCount();
    setActiveIsland(prev, islands);
}

// ============================================================================
// View Modes
// ============================================================================

void IslandCamera::zoomToOverview(const MultiIslandManager& islands) {
    if (m_inOverview) return;

    // Store current position for return
    m_preOverviewPosition = m_currentPosition;
    m_preOverviewTarget = m_currentTarget;

    // Calculate overview camera position
    m_overviewPosition = calculateOverviewPosition(islands);
    m_overviewTarget = calculateOverviewTarget(islands);

    // Start transition to overview
    startTransition(m_overviewPosition, m_overviewTarget, m_transitionParams.duration * 1.5f);

    m_inOverview = true;
    m_previousMode = m_mode;
}

void IslandCamera::returnFromOverview(const MultiIslandManager& islands) {
    if (!m_inOverview) return;

    // Return to current island view
    glm::vec3 islandPos = calculateIslandCameraPosition(m_targetIslandIndex, islands);
    glm::vec3 islandTarget = calculateIslandCenter(m_targetIslandIndex, islands);

    startTransition(islandPos, islandTarget, m_transitionParams.duration);

    m_inOverview = false;
}

// ============================================================================
// Update
// ============================================================================

void IslandCamera::update(float deltaTime) {
    switch (m_mode) {
        case IslandCameraMode::ISLAND_VIEW:
            updateIslandView(deltaTime);
            break;
        case IslandCameraMode::OVERVIEW:
            updateOverview(deltaTime);
            break;
        case IslandCameraMode::TRANSITION:
            updateTransition(deltaTime);
            break;
        case IslandCameraMode::FOLLOWING:
            updateFollowing(deltaTime);
            break;
        case IslandCameraMode::CINEMATIC:
            // Cinematic mode not implemented - fall back to island view
            // To implement: add keyframe-based camera paths, smooth easing, etc.
            updateIslandView(deltaTime);
            break;
    }

    applyCameraState();
}

void IslandCamera::updateIslandView(float deltaTime) {
    // Smooth damp toward target position
    float smoothing = 5.0f * deltaTime;
    smoothing = std::min(1.0f, smoothing);

    m_currentPosition = glm::mix(m_currentPosition, m_targetPosition, smoothing);
    m_currentTarget = glm::mix(m_currentTarget, m_targetTarget, smoothing);
}

void IslandCamera::updateOverview(float deltaTime) {
    // Gently drift/rotate in overview mode
    float smoothing = 3.0f * deltaTime;
    smoothing = std::min(1.0f, smoothing);

    m_currentPosition = glm::mix(m_currentPosition, m_overviewPosition, smoothing);
    m_currentTarget = glm::mix(m_currentTarget, m_overviewTarget, smoothing);
}

void IslandCamera::updateTransition(float deltaTime) {
    m_transitionProgress += deltaTime / m_transitionDuration;

    if (m_transitionProgress >= 1.0f) {
        completeTransition();
        return;
    }

    // Eased interpolation
    float t = easeInOutCubic(m_transitionProgress);

    // Calculate interpolated position
    if (m_transitionParams.useArc) {
        m_currentPosition = interpolateArc(m_startPosition, m_targetPosition, t, m_transitionParams.arcHeight);
    } else {
        m_currentPosition = glm::mix(m_startPosition, m_targetPosition, t);
    }

    // Target interpolation is always linear
    m_currentTarget = glm::mix(m_startTarget, m_targetTarget, t);
}

void IslandCamera::updateFollowing(float deltaTime) {
    if (!m_followingMigration) {
        m_mode = IslandCameraMode::ISLAND_VIEW;
        return;
    }

    // Follow the migrating creature
    glm::vec3 creaturePos = m_followingMigration->currentPosition;

    // Camera position above and behind creature direction
    glm::vec2 velocity = m_followingMigration->currentVelocity;
    glm::vec3 behindOffset(0.0f, 30.0f, -50.0f);

    if (glm::length(velocity) > 0.1f) {
        glm::vec2 dir = glm::normalize(velocity);
        behindOffset = glm::vec3(-dir.x * 50.0f, 30.0f, -dir.y * 50.0f);
    }

    m_targetPosition = creaturePos + behindOffset;
    m_targetTarget = creaturePos;

    // Smooth following
    float smoothing = 3.0f * deltaTime;
    m_currentPosition = glm::mix(m_currentPosition, m_targetPosition, smoothing);
    m_currentTarget = glm::mix(m_currentTarget, m_targetTarget, smoothing);
}

// ============================================================================
// Camera Access
// ============================================================================

glm::mat4 IslandCamera::getViewMatrix() const {
    if (m_camera) {
        return m_camera->getViewMatrix();
    }
    return glm::lookAt(m_currentPosition, m_currentTarget, glm::vec3(0, 1, 0));
}

// ============================================================================
// Configuration
// ============================================================================

void IslandCamera::setZoomLimits(float minZoom, float maxZoom) {
    m_minZoom = minZoom;
    m_maxZoom = maxZoom;
    m_currentZoom = std::max(m_minZoom, std::min(m_maxZoom, m_currentZoom));
}

void IslandCamera::zoom(float delta) {
    m_currentZoom = std::max(m_minZoom, std::min(m_maxZoom, m_currentZoom + delta));

    // Apply zoom by adjusting orbit distance
    m_orbitDistance = 150.0f / m_currentZoom;

    // Recalculate target position based on new zoom
    float yaw = glm::radians(m_orbitYaw);
    float pitch = glm::radians(m_orbitPitch);

    float horizontal = m_orbitDistance * std::cos(pitch);
    m_targetPosition = m_currentTarget + glm::vec3(
        horizontal * std::sin(yaw),
        m_orbitDistance * std::sin(pitch),
        horizontal * std::cos(yaw)
    );
}

// ============================================================================
// Manual Control
// ============================================================================

void IslandCamera::orbit(float deltaYaw, float deltaPitch) {
    if (m_mode != IslandCameraMode::ISLAND_VIEW) return;

    m_orbitYaw += deltaYaw;
    m_orbitPitch = std::max(10.0f, std::min(80.0f, m_orbitPitch + deltaPitch));

    // Recalculate camera position
    float yaw = glm::radians(m_orbitYaw);
    float pitch = glm::radians(m_orbitPitch);

    float horizontal = m_orbitDistance * std::cos(pitch);
    m_targetPosition = m_currentTarget + glm::vec3(
        horizontal * std::sin(yaw),
        m_orbitDistance * std::sin(pitch),
        horizontal * std::cos(yaw)
    );
}

void IslandCamera::pan(float deltaX, float deltaZ) {
    if (m_mode != IslandCameraMode::ISLAND_VIEW) return;

    glm::vec3 right = glm::normalize(glm::cross(
        m_currentTarget - m_currentPosition,
        glm::vec3(0, 1, 0)
    ));
    glm::vec3 forward = glm::normalize(glm::vec3(
        m_currentTarget.x - m_currentPosition.x,
        0,
        m_currentTarget.z - m_currentPosition.z
    ));

    glm::vec3 offset = right * deltaX + forward * deltaZ;

    m_targetPosition += offset;
    m_targetTarget += offset;
}

void IslandCamera::resetView(const MultiIslandManager& islands) {
    if (m_targetIslandIndex >= islands.getIslandCount()) return;

    m_orbitYaw = 0.0f;
    m_orbitPitch = 30.0f;
    m_currentZoom = 1.0f;
    m_orbitDistance = 150.0f;

    m_targetPosition = calculateIslandCameraPosition(m_targetIslandIndex, islands);
    m_targetTarget = calculateIslandCenter(m_targetIslandIndex, islands);
}

// ============================================================================
// Following Mode
// ============================================================================

void IslandCamera::followMigration(const MigrationEvent& event, const MultiIslandManager& islands) {
    m_followingMigration = &event;
    m_mode = IslandCameraMode::FOLLOWING;
}

void IslandCamera::stopFollowing() {
    m_followingMigration = nullptr;
    m_mode = IslandCameraMode::ISLAND_VIEW;
}

// ============================================================================
// Bookmarks
// ============================================================================

void IslandCamera::addBookmark(const std::string& name) {
    CameraBookmark bookmark;
    bookmark.name = name;
    bookmark.islandIndex = m_targetIslandIndex;
    bookmark.position = m_currentPosition;
    bookmark.target = m_currentTarget;
    bookmark.zoom = m_currentZoom;

    m_bookmarks.push_back(bookmark);
}

void IslandCamera::gotoBookmark(const std::string& name) {
    for (const auto& bookmark : m_bookmarks) {
        if (bookmark.name == name) {
            m_targetIslandIndex = bookmark.islandIndex;
            m_currentZoom = bookmark.zoom;

            startTransition(bookmark.position, bookmark.target, m_transitionParams.duration);
            return;
        }
    }
}

// ============================================================================
// Internal Methods
// ============================================================================

void IslandCamera::startTransition(const glm::vec3& targetPos, const glm::vec3& targetTarget, float duration) {
    m_startPosition = m_currentPosition;
    m_startTarget = m_currentTarget;
    m_targetPosition = targetPos;
    m_targetTarget = targetTarget;
    m_transitionDuration = duration;
    m_transitionProgress = 0.0f;

    m_mode = IslandCameraMode::TRANSITION;
}

void IslandCamera::completeTransition() {
    m_currentPosition = m_targetPosition;
    m_currentTarget = m_targetTarget;
    m_currentIslandIndex = m_targetIslandIndex;
    m_transitionProgress = 1.0f;

    if (m_inOverview) {
        m_mode = IslandCameraMode::OVERVIEW;
    } else {
        m_mode = IslandCameraMode::ISLAND_VIEW;
    }
}

glm::vec3 IslandCamera::calculateIslandCameraPosition(uint32_t islandIndex,
                                                        const MultiIslandManager& islands) const {
    const auto* island = islands.getIsland(islandIndex);
    if (!island) return glm::vec3(0, m_defaultViewHeight, 0);

    glm::vec3 center = calculateIslandCenter(islandIndex, islands);

    // Position camera at orbit distance and height
    float yaw = glm::radians(m_orbitYaw);
    float pitch = glm::radians(m_orbitPitch);

    float horizontal = m_orbitDistance * std::cos(pitch);

    return center + glm::vec3(
        horizontal * std::sin(yaw),
        m_orbitDistance * std::sin(pitch),
        horizontal * std::cos(yaw)
    );
}

glm::vec3 IslandCamera::calculateIslandCenter(uint32_t islandIndex,
                                                const MultiIslandManager& islands) const {
    const auto* island = islands.getIsland(islandIndex);
    if (!island) return glm::vec3(0);

    // Island center in world coordinates
    return glm::vec3(island->worldPosition.x, 0.0f, island->worldPosition.y);
}

glm::vec3 IslandCamera::calculateOverviewPosition(const MultiIslandManager& islands) const {
    const auto& data = islands.getArchipelagoData();

    // Center of archipelago
    glm::vec2 center = data.center;

    // Height based on archipelago size
    float size = glm::length(data.maxBounds - data.minBounds);
    float height = std::max(m_overviewHeight, size * 0.7f);

    // Offset slightly to give angled view
    return glm::vec3(center.x, height, center.y + height * 0.3f);
}

glm::vec3 IslandCamera::calculateOverviewTarget(const MultiIslandManager& islands) const {
    const auto& data = islands.getArchipelagoData();
    return glm::vec3(data.center.x, 0.0f, data.center.y);
}

glm::vec3 IslandCamera::interpolateArc(const glm::vec3& start, const glm::vec3& end,
                                         float t, float arcHeight) const {
    // Linear interpolation for XZ
    glm::vec3 linear = glm::mix(start, end, t);

    // Add arc to Y
    float arc = std::sin(t * 3.14159f) * arcHeight;

    // Ensure minimum height
    float baseHeight = glm::mix(start.y, end.y, t);
    linear.y = std::max(m_transitionParams.minHeight, baseHeight + arc);

    return linear;
}

float IslandCamera::easeInOutCubic(float t) const {
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    } else {
        return 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
    }
}

void IslandCamera::applyCameraState() {
    if (!m_camera) return;

    m_camera->Position = m_currentPosition;

    // Calculate front vector from position to target
    glm::vec3 front = glm::normalize(m_currentTarget - m_currentPosition);
    m_camera->Front = front;

    // Update camera vectors
    m_camera->Right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
    m_camera->Up = glm::normalize(glm::cross(m_camera->Right, front));

    // Update yaw and pitch from front vector
    m_camera->Yaw = glm::degrees(std::atan2(front.x, front.z));
    m_camera->Pitch = glm::degrees(std::asin(front.y));
}

} // namespace Forge
