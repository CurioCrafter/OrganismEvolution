#pragma once

// ScanningSystem - No Man's Sky style creature scanning
// Handles scan targeting, progress tracking, and discovery integration

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <functional>
#include "SpeciesCatalog.h"
#include "../entities/Creature.h"
#include "../environment/BiomeSystem.h"

// Forward declarations
class Camera;
class Terrain;

namespace discovery {

// ============================================================================
// Scan Target Info - Information about potential scan targets
// ============================================================================

struct ScanTargetInfo {
    const Creature* creature = nullptr;
    int creatureId = -1;
    uint32_t speciesId = 0;

    glm::vec3 position{0.0f};
    float distance = 0.0f;
    float screenX = 0.0f;      // Screen-space position (0-1)
    float screenY = 0.0f;

    DiscoveryState discoveryState = DiscoveryState::UNDISCOVERED;
    RarityTier rarity = RarityTier::COMMON;
    std::string displayName;

    bool isInView = false;      // Within camera frustum
    bool isTargetable = false;  // Close enough and in view

    float scanProgress = 0.0f;  // 0-1 scan completion
    bool isBeingScanned = false;
};

// ============================================================================
// Scan UI Style - Visual configuration for scan HUD
// ============================================================================

struct ScanUIStyle {
    // Colors
    glm::vec3 targetingColor{0.3f, 0.8f, 1.0f};       // Cyan targeting reticle
    glm::vec3 scanningColor{0.4f, 1.0f, 0.4f};        // Green when scanning
    glm::vec3 completeColor{1.0f, 0.8f, 0.2f};        // Gold when complete
    glm::vec3 undiscoveredColor{0.6f, 0.6f, 0.6f};    // Gray for unknown

    // Sizes
    float reticleSize = 32.0f;
    float progressBarWidth = 80.0f;
    float progressBarHeight = 8.0f;

    // Animation
    float pulseSpeed = 2.0f;
    float rotationSpeed = 30.0f;

    // Distance display
    bool showDistance = true;
    bool showRarity = true;
    bool showName = true;  // Only when discovered
};

// ============================================================================
// Scanning System - Main scanning mechanic controller
// ============================================================================

class ScanningSystem {
public:
    ScanningSystem();
    ~ScanningSystem() = default;

    // ========================================
    // Initialization
    // ========================================

    void initialize(SpeciesCatalog* catalog);
    void shutdown();

    // ========================================
    // Update Loop
    // ========================================

    // Call every frame to update scanning
    void update(float deltaTime,
                const Camera& camera,
                const std::vector<std::unique_ptr<Creature>>& creatures,
                const BiomeSystem* biomeSystem,
                float simulationTime,
                float screenWidth,
                float screenHeight);

    // ========================================
    // Scan Control
    // ========================================

    // Toggle scanning mode on/off
    void setScanning(bool enabled);
    bool isScanning() const { return m_scanningEnabled; }

    // Lock onto a specific creature
    bool lockTarget(int creatureId);
    void unlockTarget();
    bool hasLockedTarget() const { return m_lockedTargetId != -1; }
    int getLockedTargetId() const { return m_lockedTargetId; }

    // Auto-targeting (lock nearest scannable creature)
    void enableAutoTarget(bool enabled) { m_autoTargetEnabled = enabled; }
    bool isAutoTargetEnabled() const { return m_autoTargetEnabled; }

    // ========================================
    // Target Queries
    // ========================================

    // Get all visible targets
    const std::vector<ScanTargetInfo>& getVisibleTargets() const { return m_visibleTargets; }

    // Get current scan target
    const ScanTargetInfo* getCurrentTarget() const;

    // Get target at screen position (for mouse targeting)
    const ScanTargetInfo* getTargetAtScreenPos(float screenX, float screenY, float tolerance = 30.0f) const;

    // Get nearest targetable creature
    const ScanTargetInfo* getNearestTargetable() const;

    // ========================================
    // Configuration
    // ========================================

    // Scanning parameters
    void setMaxScanDistance(float distance) { m_maxScanDistance = distance; }
    float getMaxScanDistance() const { return m_maxScanDistance; }

    void setScanAngle(float angleDegrees) { m_scanAngle = angleDegrees; }
    float getScanAngle() const { return m_scanAngle; }

    // UI style
    ScanUIStyle& getUIStyle() { return m_uiStyle; }
    const ScanUIStyle& getUIStyle() const { return m_uiStyle; }

    // ========================================
    // Statistics
    // ========================================

    uint32_t getScansStarted() const { return m_scansStarted; }
    uint32_t getScansCompleted() const { return m_scansCompleted; }
    float getTotalScanTime() const { return m_totalScanTime; }

    // ========================================
    // Event Callbacks
    // ========================================

    using ScanCompleteCallback = std::function<void(const ScanTargetInfo&)>;
    using TargetChangedCallback = std::function<void(const ScanTargetInfo*)>;

    void setScanCompleteCallback(ScanCompleteCallback callback) { m_scanCompleteCallback = callback; }
    void setTargetChangedCallback(TargetChangedCallback callback) { m_targetChangedCallback = callback; }

private:
    // Find visible creatures within scan range
    void findVisibleTargets(const Camera& camera,
                            const std::vector<std::unique_ptr<Creature>>& creatures,
                            float screenWidth,
                            float screenHeight);

    // Update scan progress for locked target
    void updateScanProgress(float deltaTime,
                           const BiomeSystem* biomeSystem,
                           float simulationTime);

    // Calculate screen position from world position
    bool worldToScreen(const glm::vec3& worldPos,
                       const Camera& camera,
                       const glm::mat4& viewProjection,
                       float screenWidth,
                       float screenHeight,
                       float& outScreenX,
                       float& outScreenY) const;

    // Check if point is within scan cone
    bool isInScanCone(const glm::vec3& cameraPos,
                      const glm::vec3& cameraForward,
                      const glm::vec3& targetPos,
                      float maxDistance,
                      float coneAngleDegrees) const;

    // Select best auto-target
    void updateAutoTarget();

    // Create target info from creature
    ScanTargetInfo createTargetInfo(const Creature* creature,
                                    const Camera& camera,
                                    const glm::mat4& viewProjection,
                                    float screenWidth,
                                    float screenHeight);

    // State
    bool m_initialized = false;
    bool m_scanningEnabled = false;
    bool m_autoTargetEnabled = true;

    // References
    SpeciesCatalog* m_catalog = nullptr;

    // Target tracking
    int m_lockedTargetId = -1;
    uint32_t m_lockedSpeciesId = 0;
    float m_lockTime = 0.0f;
    std::vector<ScanTargetInfo> m_visibleTargets;

    // Scan parameters
    float m_maxScanDistance = 100.0f;
    float m_scanAngle = 45.0f;  // Degrees (half-angle of cone)

    // Statistics
    uint32_t m_scansStarted = 0;
    uint32_t m_scansCompleted = 0;
    float m_totalScanTime = 0.0f;

    // UI configuration
    ScanUIStyle m_uiStyle;

    // Callbacks
    ScanCompleteCallback m_scanCompleteCallback;
    TargetChangedCallback m_targetChangedCallback;

    // Cache
    mutable glm::mat4 m_cachedViewProjection;
    float m_cacheScreenWidth = 0.0f;
    float m_cacheScreenHeight = 0.0f;
};

// ============================================================================
// Global Scanner Accessor
// ============================================================================

ScanningSystem& getScanner();

} // namespace discovery
