#include "DisasterPanel.h"
#include "../environment/DisasterSystem.h"
#include <imgui.h>
#include <algorithm>

DisasterPanel::DisasterPanel() {
}

void DisasterPanel::render(DisasterSystem& disasters) {
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(20, 200), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Disaster Control", &m_visible)) {
        // Tab bar for different sections
        if (ImGui::BeginTabBar("DisasterTabs")) {
            if (ImGui::BeginTabItem("Active")) {
                renderActiveDisasters(disasters.getActiveDisasters());
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Trigger")) {
                renderTriggerButtons(disasters);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Settings")) {
                renderSettings(disasters);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("History")) {
                renderHistory(disasters.getDisasterHistory());
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void DisasterPanel::renderActiveDisasters(const std::vector<ActiveDisaster>& disasters) {
    if (disasters.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No active disasters");
        ImGui::Separator();
        ImGui::TextWrapped("Use the 'Trigger' tab to start a disaster, or enable "
                          "random disasters in Settings.");
        return;
    }

    ImGui::Text("Active Disasters: %d", static_cast<int>(disasters.size()));
    ImGui::Separator();

    for (size_t i = 0; i < disasters.size(); ++i) {
        const auto& disaster = disasters[i];
        if (!disaster.isActive) continue;

        ImGui::PushID(static_cast<int>(i));

        // Header with type and severity
        glm::vec4 color = getDisasterColor(disaster.type);
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(color.r, color.g, color.b, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(color.r, color.g, color.b, 0.5f));

        bool expanded = ImGui::CollapsingHeader(
            DisasterSystem::getDisasterTypeName(disaster.type),
            ImGuiTreeNodeFlags_DefaultOpen);

        ImGui::PopStyleColor(2);

        if (expanded) {
            renderDisasterDetails(disaster);
        }

        ImGui::PopID();
        ImGui::Spacing();
    }
}

void DisasterPanel::renderDisasterDetails(const ActiveDisaster& disaster) {
    ImGui::Indent();

    // Severity badge
    glm::vec4 sevColor = getSeverityColor(disaster.severity);
    ImGui::TextColored(ImVec4(sevColor.r, sevColor.g, sevColor.b, 1.0f),
                       "Severity: %s", DisasterSystem::getSeverityName(disaster.severity));

    // Progress bar
    glm::vec4 color = getDisasterColor(disaster.type);
    drawProgressBar(disaster.progress, color, "Progress");

    // Time info
    ImGui::Text("Time Remaining: %.1fs", disaster.getTimeRemaining());
    ImGui::Text("Duration: %.1fs", disaster.duration);

    // Statistics
    ImGui::Separator();
    ImGui::Text("Creatures Affected: %d", disaster.creaturesAffected);
    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                       "Creatures Killed: %d", disaster.creaturesKilled);

    if (disaster.vegetationDestroyed > 0) {
        ImGui::Text("Vegetation Destroyed: %d", disaster.vegetationDestroyed);
    }

    // Location
    ImGui::Separator();
    ImGui::Text("Epicenter: (%.1f, %.1f, %.1f)",
                disaster.epicenter.x, disaster.epicenter.y, disaster.epicenter.z);
    ImGui::Text("Radius: %.1f", disaster.radius);

    // Description
    if (!disaster.description.empty()) {
        ImGui::Separator();
        ImGui::TextWrapped("%s", disaster.description.c_str());
    }

    ImGui::Unindent();
}

void DisasterPanel::renderTriggerButtons(DisasterSystem& disasters) {
    ImGui::TextWrapped("Manually trigger disasters to create evolutionary "
                      "pressure and dramatic events.");
    ImGui::Separator();

    // Disaster type selection
    ImGui::Text("Select Disaster Type:");
    const char* disasterTypes[] = {
        "Volcanic Eruption",
        "Meteor Impact",
        "Disease Outbreak",
        "Ice Age",
        "Drought",
        "Flood"
    };
    ImGui::Combo("Type", &m_selectedDisasterType, disasterTypes, IM_ARRAYSIZE(disasterTypes));

    // Show description
    DisasterType type = static_cast<DisasterType>(m_selectedDisasterType);
    ImGui::TextWrapped("%s", DisasterSystem::getDisasterTypeDescription(type));
    ImGui::Separator();

    // Severity selection
    ImGui::Text("Select Severity:");
    const char* severityLevels[] = { "Minor", "Moderate", "Major", "Catastrophic" };
    ImGui::Combo("Severity", &m_selectedSeverity, severityLevels, IM_ARRAYSIZE(severityLevels));

    // Severity warning for catastrophic
    if (m_selectedSeverity == 3) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                          "WARNING: Catastrophic disasters cause mass extinctions!");
    }

    ImGui::Separator();

    // Position options
    ImGui::Checkbox("Custom Position", &m_useCustomPosition);
    if (m_useCustomPosition) {
        ImGui::DragFloat3("Position", &m_triggerPosition.x, 1.0f, -300.0f, 300.0f);
    } else {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                          "(Random position will be used)");
    }

    ImGui::Separator();

    // Trigger button
    glm::vec4 btnColor = getDisasterColor(type);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(btnColor.r, btnColor.g, btnColor.b, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(btnColor.r, btnColor.g, btnColor.b, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(btnColor.r, btnColor.g, btnColor.b, 1.0f));

    if (ImGui::Button("TRIGGER DISASTER", ImVec2(-1, 40))) {
        DisasterSeverity severity = static_cast<DisasterSeverity>(m_selectedSeverity);
        glm::vec3 position = m_useCustomPosition ? m_triggerPosition : glm::vec3(0.0f);
        disasters.triggerDisaster(type, position, severity);
    }

    ImGui::PopStyleColor(3);

    // Random disaster button
    ImGui::Spacing();
    if (ImGui::Button("Trigger Random Disaster", ImVec2(-1, 0))) {
        disasters.triggerRandomDisaster();
    }

    // End all disasters (debug)
    ImGui::Spacing();
    ImGui::Separator();
    if (ImGui::Button("End All Disasters", ImVec2(-1, 0))) {
        disasters.endAllDisasters();
    }
}

void DisasterPanel::renderSettings(DisasterSystem& disasters) {
    ImGui::Text("Disaster System Settings");
    ImGui::Separator();

    // Random disasters toggle
    bool randomEnabled = disasters.areRandomDisastersEnabled();
    if (ImGui::Checkbox("Enable Random Disasters", &randomEnabled)) {
        disasters.setRandomDisastersEnabled(randomEnabled);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextWrapped("When enabled, disasters will randomly occur based on probability.");
        ImGui::EndTooltip();
    }

    // Probability slider
    float probability = disasters.getDisasterProbability();
    if (ImGui::SliderFloat("Daily Probability", &probability, 0.0f, 0.1f, "%.4f")) {
        disasters.setDisasterProbability(probability);
    }

    // Cooldown slider
    float cooldown = disasters.getMinDisasterCooldown();
    if (ImGui::SliderFloat("Min Cooldown (sec)", &cooldown, 10.0f, 300.0f, "%.0f")) {
        disasters.setMinDisasterCooldown(cooldown);
    }

    // Max concurrent
    int maxConcurrent = disasters.getMaxConcurrentDisasters();
    if (ImGui::SliderInt("Max Concurrent", &maxConcurrent, 1, 5)) {
        disasters.setMaxConcurrentDisasters(maxConcurrent);
    }

    ImGui::Separator();

    // Statistics
    ImGui::Text("Statistics");
    ImGui::Text("Active Disasters: %d", disasters.getActiveDisasterCount());
    ImGui::Text("Total Historical Deaths: %d", disasters.getTotalHistoricalDeaths());
    ImGui::Text("Total Events: %d", static_cast<int>(disasters.getDisasterHistory().size()));
}

void DisasterPanel::renderHistory(const std::vector<DisasterRecord>& history) {
    if (history.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No disaster history yet");
        return;
    }

    ImGui::Text("Disaster History (%d events)", static_cast<int>(history.size()));
    ImGui::Separator();

    // Show most recent first
    int count = std::min(m_historyDisplayCount, static_cast<int>(history.size()));
    for (int i = static_cast<int>(history.size()) - 1; i >= static_cast<int>(history.size()) - count; --i) {
        const auto& record = history[i];

        ImGui::PushID(i);

        glm::vec4 color = getDisasterColor(record.type);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(color.r, color.g, color.b, 1.0f));
        ImGui::Text("%s", DisasterSystem::getDisasterTypeName(record.type));
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(%s)",
                          DisasterSystem::getSeverityName(record.severity));

        // Compact info
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "  Deaths: %d", record.totalKilled);
        ImGui::SameLine();
        ImGui::Text("| Duration: %.0fs", record.duration);

        if (m_showHistoryDetails) {
            ImGui::TextWrapped("  %s", record.summary.c_str());
        }

        ImGui::Separator();
        ImGui::PopID();
    }

    // Controls
    ImGui::SliderInt("Show Count", &m_historyDisplayCount, 5, 50);
    ImGui::Checkbox("Show Details", &m_showHistoryDetails);
}

glm::vec4 DisasterPanel::getDisasterColor(DisasterType type) const {
    switch (type) {
        case DisasterType::VOLCANIC_ERUPTION:
            return glm::vec4(1.0f, 0.3f, 0.0f, 1.0f);  // Orange-red
        case DisasterType::METEOR_IMPACT:
            return glm::vec4(0.8f, 0.5f, 0.0f, 1.0f);  // Orange
        case DisasterType::DISEASE_OUTBREAK:
            return glm::vec4(0.3f, 0.8f, 0.2f, 1.0f);  // Sickly green
        case DisasterType::ICE_AGE:
            return glm::vec4(0.4f, 0.7f, 1.0f, 1.0f);  // Ice blue
        case DisasterType::DROUGHT:
            return glm::vec4(0.8f, 0.6f, 0.2f, 1.0f);  // Desert tan
        case DisasterType::FLOOD:
            return glm::vec4(0.2f, 0.4f, 0.9f, 1.0f);  // Deep blue
        default:
            return glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);  // Gray
    }
}

glm::vec4 DisasterPanel::getSeverityColor(DisasterSeverity severity) const {
    switch (severity) {
        case DisasterSeverity::MINOR:
            return glm::vec4(0.5f, 0.8f, 0.5f, 1.0f);  // Light green
        case DisasterSeverity::MODERATE:
            return glm::vec4(1.0f, 0.8f, 0.2f, 1.0f);  // Yellow
        case DisasterSeverity::MAJOR:
            return glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);  // Orange
        case DisasterSeverity::CATASTROPHIC:
            return glm::vec4(1.0f, 0.2f, 0.2f, 1.0f);  // Red
        default:
            return glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);  // Gray
    }
}

const char* DisasterPanel::getPhaseDescription(const ActiveDisaster& disaster) const {
    // Would depend on specific disaster type for accurate phase descriptions
    float progress = disaster.progress;

    if (progress < 0.2f) return "Beginning";
    if (progress < 0.5f) return "Intensifying";
    if (progress < 0.8f) return "Peak";
    return "Subsiding";
}

void DisasterPanel::drawProgressBar(float progress, const glm::vec4& color, const char* label) const {
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
                         ImVec4(color.r, color.g, color.b, color.a));
    ImGui::ProgressBar(progress, ImVec2(-1, 0),
                      (std::string(label) + ": " + std::to_string(static_cast<int>(progress * 100)) + "%").c_str());
    ImGui::PopStyleColor();
}
