#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

// Forward declarations
class DisasterSystem;
struct ActiveDisaster;
struct DisasterRecord;
enum class DisasterType;
enum class DisasterSeverity;

/**
 * @brief UI Panel for disaster system control and monitoring
 *
 * Provides interface for:
 * - Viewing active disasters
 * - Triggering manual disasters
 * - Adjusting disaster settings
 * - Viewing disaster history
 */
class DisasterPanel {
public:
    DisasterPanel();
    ~DisasterPanel() = default;

    /**
     * @brief Render the disaster panel using ImGui
     * @param disasters Reference to the disaster system
     */
    void render(DisasterSystem& disasters);

    /**
     * @brief Check if panel is currently visible
     */
    bool isVisible() const { return m_visible; }

    /**
     * @brief Set panel visibility
     */
    void setVisible(bool visible) { m_visible = visible; }

    /**
     * @brief Toggle panel visibility
     */
    void toggleVisible() { m_visible = !m_visible; }

private:
    // === Render Sections ===
    void renderActiveDisasters(const std::vector<ActiveDisaster>& disasters);
    void renderTriggerButtons(DisasterSystem& disasters);
    void renderSettings(DisasterSystem& disasters);
    void renderHistory(const std::vector<DisasterRecord>& history);
    void renderDisasterDetails(const ActiveDisaster& disaster);

    // === Helper Methods ===
    glm::vec4 getDisasterColor(DisasterType type) const;
    glm::vec4 getSeverityColor(DisasterSeverity severity) const;
    const char* getPhaseDescription(const ActiveDisaster& disaster) const;
    void drawProgressBar(float progress, const glm::vec4& color, const char* label) const;

    // === State ===
    bool m_visible = true;
    int m_selectedDisasterType = 0;
    int m_selectedSeverity = 1;  // Default to moderate
    glm::vec3 m_triggerPosition = glm::vec3(0.0f);
    bool m_useCustomPosition = false;

    // === History Display ===
    int m_historyDisplayCount = 10;
    bool m_showHistoryDetails = false;
};
