#pragma once

// DiscoveryPanel - No Man's Sky style discovery UI
// Shows discovered species, rarity, scan progress, and catalog browser

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <functional>
#include "../core/SpeciesCatalog.h"
#include "../core/ScanningSystem.h"

namespace ui {

// ============================================================================
// Discovery Panel Display Mode
// ============================================================================

enum class DiscoveryViewMode {
    CATALOG,        // Full species catalog browser
    RECENT,         // Recently discovered/seen species
    RARITY,         // Filtered by rarity tier
    BIOME,          // Filtered by biome
    SCAN_PROGRESS   // Currently scanning with progress
};

// ============================================================================
// Discovery Panel Configuration
// ============================================================================

struct DiscoveryPanelConfig {
    // Panel positioning (screen-relative, 0-1)
    float panelX = 0.02f;       // Left margin
    float panelY = 0.1f;        // Top margin
    float panelWidth = 0.25f;   // Width as fraction of screen
    float panelHeight = 0.6f;   // Height as fraction of screen

    // Appearance
    float opacity = 0.85f;
    float entryHeight = 60.0f;  // Pixels per species entry
    float iconSize = 48.0f;

    // Behavior
    bool showNotifications = true;
    float notificationDuration = 4.0f;  // Seconds
    bool autoExpandOnScan = true;
    bool showMiniProgress = true;       // Show mini scan progress in corner

    // Scan HUD overlay
    bool showScanHUD = true;
    float scanHUDOpacity = 0.9f;
};

// ============================================================================
// Discovery Notification
// ============================================================================

struct DiscoveryNotification {
    discovery::DiscoveryEvent event;
    float timeRemaining = 4.0f;
    float fadeProgress = 0.0f;   // 0-1, starts fading near end
    bool dismissed = false;
};

// ============================================================================
// DiscoveryPanel - Main UI component
// ============================================================================

class DiscoveryPanel {
public:
    DiscoveryPanel();
    ~DiscoveryPanel() = default;

    // ========================================
    // Lifecycle
    // ========================================

    void initialize(discovery::SpeciesCatalog* catalog, discovery::ScanningSystem* scanner);
    void shutdown();

    // Update (call every frame)
    void update(float deltaTime);

    // Render (call during ImGui frame)
    void render(float screenWidth, float screenHeight);

    // ========================================
    // Panel Visibility
    // ========================================

    void show() { m_visible = true; }
    void hide() { m_visible = false; }
    void toggle() { m_visible = !m_visible; }
    bool isVisible() const { return m_visible; }

    // Collapse/expand panel
    void setCollapsed(bool collapsed) { m_collapsed = collapsed; }
    bool isCollapsed() const { return m_collapsed; }

    // ========================================
    // View Control
    // ========================================

    void setViewMode(DiscoveryViewMode mode);
    DiscoveryViewMode getViewMode() const { return m_viewMode; }

    // Filter control
    void setRarityFilter(discovery::RarityTier rarity) { m_rarityFilter = rarity; }
    void setBiomeFilter(BiomeType biome) { m_biomeFilter = biome; }
    void clearFilters();

    // Search
    void setSearchQuery(const std::string& query) { m_searchQuery = query; }

    // ========================================
    // Selected Species
    // ========================================

    void selectSpecies(uint32_t speciesId);
    void clearSelection() { m_selectedSpeciesId = 0; }
    uint32_t getSelectedSpeciesId() const { return m_selectedSpeciesId; }
    const discovery::SpeciesDiscoveryEntry* getSelectedEntry() const;

    // ========================================
    // Notifications
    // ========================================

    void addNotification(const discovery::DiscoveryEvent& event);
    void clearNotifications();

    // ========================================
    // Callbacks
    // ========================================

    using SpeciesSelectedCallback = std::function<void(uint32_t speciesId)>;
    void setSpeciesSelectedCallback(SpeciesSelectedCallback callback) { m_speciesSelectedCallback = callback; }

    // ========================================
    // Configuration
    // ========================================

    DiscoveryPanelConfig& getConfig() { return m_config; }
    const DiscoveryPanelConfig& getConfig() const { return m_config; }

private:
    // Render components
    void renderScanHUD(float screenWidth, float screenHeight);
    void renderMiniProgress(float screenWidth, float screenHeight);
    void renderNotifications(float screenWidth, float screenHeight);
    void renderCatalogPanel(float screenWidth, float screenHeight);
    void renderSpeciesDetails(const discovery::SpeciesDiscoveryEntry& entry);
    void renderSpeciesListEntry(const discovery::SpeciesDiscoveryEntry& entry, bool isSelected);
    void renderScanProgressBar(const discovery::ScanProgress& progress, float width);
    void renderRarityBadge(discovery::RarityTier rarity);
    void renderTraitUnlocks(const discovery::SpeciesDiscoveryEntry& entry);

    // Get filtered species list
    std::vector<const discovery::SpeciesDiscoveryEntry*> getFilteredEntries() const;

    // Helpers
    std::string formatTimestamp(uint64_t timestamp) const;
    std::string formatDuration(float seconds) const;

    // State
    bool m_initialized = false;
    bool m_visible = true;
    bool m_collapsed = false;

    // References
    discovery::SpeciesCatalog* m_catalog = nullptr;
    discovery::ScanningSystem* m_scanner = nullptr;

    // View state
    DiscoveryViewMode m_viewMode = DiscoveryViewMode::RECENT;
    discovery::RarityTier m_rarityFilter = discovery::RarityTier::COMMON;
    BiomeType m_biomeFilter = BiomeType::GRASSLAND;
    std::string m_searchQuery;

    // Selection
    uint32_t m_selectedSpeciesId = 0;

    // Notifications
    std::vector<DiscoveryNotification> m_notifications;
    static constexpr size_t MAX_NOTIFICATIONS = 5;

    // Configuration
    DiscoveryPanelConfig m_config;

    // Callbacks
    SpeciesSelectedCallback m_speciesSelectedCallback;

    // UI animation state
    float m_scanPulse = 0.0f;
    float m_scanRotation = 0.0f;
};

// ============================================================================
// Discovery HUD Overlay - Lightweight scan reticle
// ============================================================================

class DiscoveryHUD {
public:
    DiscoveryHUD();
    ~DiscoveryHUD() = default;

    void initialize(discovery::ScanningSystem* scanner);
    void render(float screenWidth, float screenHeight);

    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }

private:
    void renderScanReticle(const discovery::ScanTargetInfo& target, float screenWidth, float screenHeight);
    void renderTargetIndicator(const discovery::ScanTargetInfo& target, float screenWidth, float screenHeight);
    void renderProgressRing(float x, float y, float radius, float progress, const glm::vec3& color);

    discovery::ScanningSystem* m_scanner = nullptr;
    bool m_visible = true;
    float m_animTime = 0.0f;
};

} // namespace ui
