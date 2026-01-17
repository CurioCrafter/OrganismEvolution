#pragma once

// CreatureInspectionPanel - Detailed creature inspection UI for up-close viewing
// Phase 8 Agent 8: UI Integration Owner
//
// Features:
// - Full creature detail panel with multiple sections
// - Camera focus/track/release controls
// - Live data updates as creature moves
// - Compact palette strip from creature colors
// - Safe fallback when creature despawns

#include "imgui.h"
#include "../entities/Creature.h"
#include "../entities/Genome.h"
#include "../graphics/CameraController.h"
#include "../environment/BiomeSystem.h"
#include <functional>
#include <string>

namespace Forge {
class CreatureManager;
}

namespace ui {

// Inspection mode states
enum class InspectionMode {
    NONE,       // No creature selected
    VIEWING,    // Viewing creature details (camera unchanged)
    FOCUSED,    // Camera smoothly transitioned to creature
    TRACKING    // Camera following creature movement
};

// Callback types for camera integration
using FocusCameraCallback = std::function<void(const Creature*)>;
using TrackCameraCallback = std::function<void(const Creature*)>;
using ReleaseCameraCallback = std::function<void()>;

// ============================================================================
// CreatureInspectionPanel
// ============================================================================

class CreatureInspectionPanel {
public:
    CreatureInspectionPanel();
    ~CreatureInspectionPanel() = default;

    // ========================================================================
    // Main Interface
    // ========================================================================

    // Set the creature to inspect (can be null to clear)
    void setInspectedCreature(Creature* creature);

    // Get currently inspected creature
    Creature* getInspectedCreature() const { return m_inspectedCreature; }

    // Check if actively inspecting
    bool hasInspectedCreature() const { return m_inspectedCreature != nullptr && m_inspectedCreature->isAlive(); }

    // Clear inspection (deselect creature)
    void clearInspection();

    // ========================================================================
    // Rendering
    // ========================================================================

    // Main render function - call each frame
    // Returns true if panel wants to remain open
    void render();

    // Render a compact on-screen indicator for inspected creature
    // Call this during main render pass (draws to foreground)
    void renderScreenIndicator(const Camera& camera, float screenWidth, float screenHeight);

    // ========================================================================
    // Camera Integration
    // ========================================================================

    // Set callback for focusing camera on creature
    void setFocusCameraCallback(FocusCameraCallback cb) { m_focusCameraCallback = cb; }

    // Set callback for tracking creature with camera
    void setTrackCameraCallback(TrackCameraCallback cb) { m_trackCameraCallback = cb; }

    // Set callback for releasing camera from creature
    void setReleaseCameraCallback(ReleaseCameraCallback cb) { m_releaseCameraCallback = cb; }

    // Get current inspection mode
    InspectionMode getInspectionMode() const { return m_mode; }

    // ========================================================================
    // Optional Integrations
    // ========================================================================

    // Set biome system for environment info
    void setBiomeSystem(const BiomeSystem* biomes) { m_biomeSystem = biomes; }

    // Set camera controller for direct integration (optional)
    void setCameraController(Forge::CameraController* controller) { m_cameraController = controller; }

    // ========================================================================
    // Settings
    // ========================================================================

    // Panel visibility
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    void toggleVisible() { m_visible = !m_visible; }

    // Show/hide individual sections
    void setShowIdentity(bool show) { m_showIdentity = show; }
    void setShowBiology(bool show) { m_showBiology = show; }
    void setShowMorphology(bool show) { m_showMorphology = show; }
    void setShowStatus(bool show) { m_showStatus = show; }
    void setShowEnvironment(bool show) { m_showEnvironment = show; }
    void setShowGenetics(bool show) { m_showGenetics = show; }
    void setShowBrain(bool show) { m_showBrain = show; }

private:
    // Currently inspected creature
    Creature* m_inspectedCreature = nullptr;
    int m_inspectedCreatureId = -1;  // For tracking if creature pointer becomes invalid

    // Inspection mode
    InspectionMode m_mode = InspectionMode::NONE;

    // Panel state
    bool m_visible = true;

    // Section visibility flags
    bool m_showIdentity = true;
    bool m_showBiology = true;
    bool m_showMorphology = true;
    bool m_showStatus = true;
    bool m_showEnvironment = true;
    bool m_showGenetics = false;  // Collapsed by default
    bool m_showBrain = false;     // Collapsed by default

    // Callbacks
    FocusCameraCallback m_focusCameraCallback;
    TrackCameraCallback m_trackCameraCallback;
    ReleaseCameraCallback m_releaseCameraCallback;

    // Optional integrations
    const BiomeSystem* m_biomeSystem = nullptr;
    Forge::CameraController* m_cameraController = nullptr;

    // ========================================================================
    // Section Rendering
    // ========================================================================

    void renderIdentitySection(Creature* c);
    void renderBiologySection(Creature* c);
    void renderMorphologySection(Creature* c);
    void renderStatusSection(Creature* c);
    void renderEnvironmentSection(Creature* c);
    void renderGeneticsSection(Creature* c);
    void renderBrainSection(Creature* c);
    void renderCameraControls();
    void renderColorPalette(Creature* c);

    // ========================================================================
    // Helpers
    // ========================================================================

    // Get display string for creature type
    const char* getCreatureTypeString(CreatureType type) const;

    // Get display string for activity state
    const char* getActivityStateString(Creature* c) const;

    // Get biome name at creature position
    std::string getBiomeNameAtCreature(Creature* c) const;

    // Calculate similarity color (for species visualization)
    ImVec4 getSpeciesSimilarityColor(Creature* c) const;

    // Validate creature is still alive
    bool validateCreature();
};

} // namespace ui
