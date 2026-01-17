#pragma once

/**
 * @file GameplayUI.h
 * @brief UI panels for gameplay features
 *
 * Provides ImGui-based UI for:
 * - Time controls with visual feedback
 * - Statistics display with graphs
 * - Achievement notifications and tracker
 * - Creature highlighting controls
 * - Event log with scrolling display
 * - Spotlight creature bio card
 * - Environmental event status
 */

#include "imgui.h"
// ImPlot is optional - check if context exists before using
#ifdef IMPLOT_VERSION
#include "implot.h"
#endif
#include "../core/GameplayManager.h"
#include "../entities/Creature.h"
#include <string>
#include <deque>

namespace ui {

// ============================================================================
// Gameplay UI Panel
// ============================================================================

class GameplayUI {
public:
    GameplayUI();
    ~GameplayUI() = default;

    // Main render function
    void render(Forge::GameplayManager& gameplay, float screenWidth, float screenHeight);

    // Individual panel renders (can be called separately)
    void renderTimeControls(Forge::GameplayManager& gameplay);
    void renderStatisticsPanel(Forge::GameplayManager& gameplay);
    void renderAchievementsPanel(Forge::GameplayManager& gameplay);
    void renderHighlightControls(Forge::GameplayManager& gameplay);
    void renderEventLog(Forge::GameplayManager& gameplay, float screenWidth, float screenHeight);
    void renderSpotlightCard(Forge::GameplayManager& gameplay, float screenWidth, float screenHeight);
    void renderEnvironmentEventBanner(Forge::GameplayManager& gameplay, float screenWidth);
    void renderAchievementPopup(Forge::GameplayManager& gameplay, float screenWidth, float screenHeight);

    // Visibility toggles
    void toggleTimeControls() { m_showTimeControls = !m_showTimeControls; }
    void toggleStatistics() { m_showStatistics = !m_showStatistics; }
    void toggleAchievements() { m_showAchievements = !m_showAchievements; }
    void toggleHighlighting() { m_showHighlighting = !m_showHighlighting; }
    void toggleEventLog() { m_showEventLog = !m_showEventLog; }
    void toggleSpotlight() { m_showSpotlight = !m_showSpotlight; }

    bool isTimeControlsVisible() const { return m_showTimeControls; }
    bool isStatisticsVisible() const { return m_showStatistics; }
    bool isAchievementsVisible() const { return m_showAchievements; }
    bool isHighlightingVisible() const { return m_showHighlighting; }
    bool isEventLogVisible() const { return m_showEventLog; }
    bool isSpotlightVisible() const { return m_showSpotlight; }

    // Handle keyboard input
    void handleInput(Forge::GameplayManager& gameplay);

private:
    // Visibility flags
    bool m_showTimeControls = true;
    bool m_showStatistics = false;
    bool m_showAchievements = false;
    bool m_showHighlighting = false;
    bool m_showEventLog = true;
    bool m_showSpotlight = true;

    // Achievement popup state
    bool m_showAchievementPopup = false;
    const Forge::Achievement* m_currentAchievement = nullptr;
    float m_achievementPopupTimer = 0.0f;
    static constexpr float ACHIEVEMENT_POPUP_DURATION = 5.0f;

    // Graph data buffers (for ImPlot)
    std::vector<float> m_birthsGraphData;
    std::vector<float> m_deathsGraphData;
    std::vector<float> m_timeAxis;

    // Helper functions
    void renderSpeedButton(Forge::GameplayManager& gameplay, const char* label, float speed);
    const char* getHighlightModeName(Forge::HighlightMode mode);
    const char* getSpotlightCategoryName(Forge::SpotlightCategory category);
    const char* getEnvironmentEventName(Forge::EnvironmentEventType type);
    ImVec4 glmToImVec4(const glm::vec3& color, float alpha = 1.0f);
};

// ============================================================================
// Implementation
// ============================================================================

inline GameplayUI::GameplayUI() {
    m_birthsGraphData.reserve(Forge::LiveStatistics::MAX_MINUTES);
    m_deathsGraphData.reserve(Forge::LiveStatistics::MAX_MINUTES);
    m_timeAxis.reserve(Forge::LiveStatistics::MAX_MINUTES);
}

inline void GameplayUI::render(Forge::GameplayManager& gameplay, float screenWidth, float screenHeight) {
    // Always render environment event banner if active
    if (gameplay.hasEnvironmentEvent()) {
        renderEnvironmentEventBanner(gameplay, screenWidth);
    }

    // Render visible panels
    if (m_showTimeControls) {
        renderTimeControls(gameplay);
    }

    if (m_showStatistics) {
        renderStatisticsPanel(gameplay);
    }

    if (m_showAchievements) {
        renderAchievementsPanel(gameplay);
    }

    if (m_showHighlighting) {
        renderHighlightControls(gameplay);
    }

    if (m_showEventLog) {
        renderEventLog(gameplay, screenWidth, screenHeight);
    }

    if (m_showSpotlight && gameplay.hasSpotlight()) {
        renderSpotlightCard(gameplay, screenWidth, screenHeight);
    }

    // Achievement popup (always check)
    renderAchievementPopup(gameplay, screenWidth, screenHeight);
}

inline void GameplayUI::renderTimeControls(Forge::GameplayManager& gameplay) {
    ImGui::SetNextWindowPos(ImVec2(10, 60), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280, 120), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("Time Controls", &m_showTimeControls, flags)) {
        auto& tc = gameplay.getTimeControl();

        // Pause button with visual feedback
        ImVec4 pauseColor = tc.paused ?
            ImVec4(0.2f, 0.7f, 0.2f, 1.0f) :
            ImVec4(0.7f, 0.2f, 0.2f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, pauseColor);
        if (ImGui::Button(tc.paused ? "RESUME (Space)" : "PAUSE (Space)", ImVec2(130, 35))) {
            gameplay.togglePause();
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();

        // Current speed display
        ImGui::BeginGroup();
        ImGui::Text("Speed:");
        if (tc.paused) {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "PAUSED");
        } else {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%.2fx", tc.timeScale);
        }
        ImGui::EndGroup();

        ImGui::Separator();

        // Speed preset buttons
        ImGui::Text("Presets:");
        ImGui::SameLine();

        renderSpeedButton(gameplay, "0.25x", Forge::TimeControl::SPEED_QUARTER);
        ImGui::SameLine();
        renderSpeedButton(gameplay, "0.5x", Forge::TimeControl::SPEED_HALF);
        ImGui::SameLine();
        renderSpeedButton(gameplay, "1x", Forge::TimeControl::SPEED_NORMAL);
        ImGui::SameLine();
        renderSpeedButton(gameplay, "2x", Forge::TimeControl::SPEED_DOUBLE);
        ImGui::SameLine();
        renderSpeedButton(gameplay, "4x", Forge::TimeControl::SPEED_QUAD);
        ImGui::SameLine();
        renderSpeedButton(gameplay, "8x", Forge::TimeControl::SPEED_OCTO);

        // Speed slider
        float speed = tc.timeScale;
        if (ImGui::SliderFloat("##SpeedSlider", &speed, 0.1f, 10.0f, "%.2f")) {
            gameplay.setTimeScale(speed);
        }
    }
    ImGui::End();
}

inline void GameplayUI::renderSpeedButton(Forge::GameplayManager& gameplay, const char* label, float speed) {
    auto& tc = gameplay.getTimeControl();
    bool isSelected = std::abs(tc.timeScale - speed) < 0.01f;

    if (isSelected) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    }

    if (ImGui::SmallButton(label)) {
        gameplay.setTimeScale(speed);
    }

    if (isSelected) {
        ImGui::PopStyleColor();
    }
}

inline void GameplayUI::renderStatisticsPanel(Forge::GameplayManager& gameplay) {
    ImGui::SetNextWindowPos(ImVec2(300, 60), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 350), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Live Statistics", &m_showStatistics)) {
        const auto& stats = gameplay.getStatistics();

        // Current rates
        if (ImGui::CollapsingHeader("Birth/Death Rates", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("This Minute:");
            ImGui::BulletText("Births: %d", stats.birthsThisMinute);
            ImGui::BulletText("Deaths: %d", stats.deathsThisMinute);

            ImGui::Separator();
            ImGui::Text("Averages (last %d min):", (int)stats.birthsHistory.size());
            ImGui::BulletText("Births/min: %.1f", stats.getAverageBirthsPerMinute());
            ImGui::BulletText("Deaths/min: %.1f", stats.getAverageDeathsPerMinute());

            // Simple text-based history display (works without ImPlot)
            if (!stats.birthsHistory.empty()) {
                ImGui::Text("Recent History (last %d min):", (int)stats.birthsHistory.size());
                ImGui::Indent();
                std::string birthHist = "Births: ";
                std::string deathHist = "Deaths: ";
                int count = 0;
                for (int b : stats.birthsHistory) {
                    if (count > 0) birthHist += ", ";
                    birthHist += std::to_string(b);
                    count++;
                    if (count >= 5) break;  // Show last 5 minutes
                }
                count = 0;
                for (int d : stats.deathsHistory) {
                    if (count > 0) deathHist += ", ";
                    deathHist += std::to_string(d);
                    count++;
                    if (count >= 5) break;
                }
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%s", birthHist.c_str());
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", deathHist.c_str());
                ImGui::Unindent();
            }
        }

        // Records
        if (ImGui::CollapsingHeader("Records", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("All-Time Records:");
            ImGui::BulletText("Peak Population: %d", stats.peakPopulation);
            ImGui::BulletText("Longest Lineage: %d generations", stats.longestLineage);

            if (stats.oldestEver.value > 0) {
                ImGui::BulletText("Oldest Ever: %.1f age", stats.oldestEver.value);
            }
            if (stats.fastestEver.value > 0) {
                ImGui::BulletText("Fastest Ever: %.2f speed", stats.fastestEver.value);
            }
            if (stats.largestEver.value > 0) {
                ImGui::BulletText("Largest Ever: %.2f size", stats.largestEver.value);
            }

            ImGui::Separator();
            ImGui::Text("Current Records:");
            if (stats.currentOldest.creatureId >= 0) {
                ImGui::BulletText("Oldest: #%d (%.1f age)", stats.currentOldest.creatureId, stats.currentOldest.value);
            }
            if (stats.currentFastest.creatureId >= 0) {
                ImGui::BulletText("Fastest: #%d (%.2f)", stats.currentFastest.creatureId, stats.currentFastest.value);
            }
            if (stats.currentLargest.creatureId >= 0) {
                ImGui::BulletText("Largest: #%d (%.2f)", stats.currentLargest.creatureId, stats.currentLargest.value);
            }
        }
    }
    ImGui::End();
}

inline void GameplayUI::renderAchievementsPanel(Forge::GameplayManager& gameplay) {
    ImGui::SetNextWindowPos(ImVec2(720, 60), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Achievements", &m_showAchievements)) {
        const auto& achievements = gameplay.getAchievements();
        int unlocked = gameplay.getUnlockedAchievementCount();
        int total = static_cast<int>(achievements.size());

        // Progress bar
        float progress = total > 0 ? static_cast<float>(unlocked) / total : 0.0f;
        ImGui::ProgressBar(progress, ImVec2(-1, 20),
            (std::to_string(unlocked) + "/" + std::to_string(total) + " Unlocked").c_str());

        ImGui::Separator();

        // List achievements
        for (const auto& ach : achievements) {
            ImVec4 color = ach.unlocked ?
                ImVec4(0.2f, 0.8f, 0.2f, 1.0f) :
                ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

            ImGui::PushStyleColor(ImGuiCol_Text, color);

            bool open = ImGui::TreeNode(ach.name.c_str());
            ImGui::PopStyleColor();

            if (open) {
                ImGui::TextWrapped("%s", ach.description.c_str());
                if (ach.unlocked) {
                    ImGui::TextColored(ImVec4(0.6f, 0.8f, 0.6f, 1.0f),
                        "Unlocked at %.1f seconds", ach.unlockTime);
                }
                ImGui::TreePop();
            }
        }
    }
    ImGui::End();
}

inline void GameplayUI::renderHighlightControls(Forge::GameplayManager& gameplay) {
    ImGui::SetNextWindowPos(ImVec2(10, 190), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(250, 200), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Creature Highlighting", &m_showHighlighting)) {
        auto& settings = gameplay.getHighlightSettings();

        // Mode selection
        ImGui::Text("Highlight Mode:");

        for (int i = 0; i < static_cast<int>(Forge::HighlightMode::COUNT); ++i) {
            Forge::HighlightMode mode = static_cast<Forge::HighlightMode>(i);
            bool selected = settings.mode == mode;

            if (ImGui::RadioButton(getHighlightModeName(mode), selected)) {
                gameplay.setHighlightMode(mode);
            }
        }

        ImGui::Separator();

        // Settings
        ImGui::Text("Settings:");
        ImGui::SliderFloat("Hunger Threshold", &settings.hungerThreshold, 0.1f, 0.5f, "%.2f");
        ImGui::SliderFloat("Pulse Speed", &settings.pulseSpeed, 0.5f, 5.0f, "%.1f");
        ImGui::SliderFloat("Pulse Intensity", &settings.pulseIntensity, 0.1f, 0.5f, "%.2f");

        ImGui::Separator();

        // Keyboard shortcuts reminder
        ImGui::TextDisabled("Shortcuts:");
        ImGui::TextDisabled("H - Toggle highlighting");
        ImGui::TextDisabled("1-7 - Quick select mode");
    }
    ImGui::End();
}

inline void GameplayUI::renderEventLog(Forge::GameplayManager& gameplay, float screenWidth, float screenHeight) {
    // Position in bottom-right corner
    float logWidth = 350.0f;
    float logHeight = 200.0f;
    float margin = 10.0f;

    ImGui::SetNextWindowPos(ImVec2(screenWidth - logWidth - margin, screenHeight - logHeight - 35), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(logWidth, logHeight), ImGuiCond_Always);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.6f));

    if (ImGui::Begin("##EventLog", &m_showEventLog, flags)) {
        const auto& events = gameplay.getEventLog();

        ImGui::BeginChild("EventScroll", ImVec2(0, 0), false);

        for (const auto& event : events) {
            // Fade out old events
            float alpha = 1.0f;
            if (event.elapsed > event.duration * 0.7f) {
                alpha = 1.0f - (event.elapsed - event.duration * 0.7f) / (event.duration * 0.3f);
            }

            if (alpha <= 0.0f) continue;

            ImVec4 color = glmToImVec4(event.color, alpha);
            ImGui::TextColored(color, "%s", event.text.c_str());
        }

        // Auto-scroll to bottom for new events
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10) {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndChild();
    }
    ImGui::End();

    ImGui::PopStyleColor();
}

inline void GameplayUI::renderSpotlightCard(Forge::GameplayManager& gameplay, float screenWidth, float screenHeight) {
    const auto& spotlight = gameplay.getSpotlight();
    if (!spotlight.creature) return;

    // Position in top-right area
    float cardWidth = 280.0f;
    float cardHeight = 180.0f;
    float margin = 10.0f;

    ImGui::SetNextWindowPos(ImVec2(screenWidth - cardWidth - margin, 60), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(cardWidth, cardHeight), ImGuiCond_Always);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.15f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));

    std::string title = "Spotlight: " + std::string(getSpotlightCategoryName(spotlight.category));

    if (ImGui::Begin(title.c_str(), nullptr, flags)) {
        const Creature* c = spotlight.creature;

        // Creature info
        ImGui::Text("Creature #%d", c->getID());
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), "Type: %s", getCreatureTypeName(c->getType()));
        ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.5f, 1.0f), "Species: %llu", c->getSpeciesId());

        ImGui::Separator();

        // Stats
        ImGui::Columns(2, nullptr, false);

        ImGui::Text("Age: %.1f", c->getAge());
        ImGui::Text("Energy: %.0f%%", (c->getEnergy() / c->getMaxEnergy()) * 100.0f);
        ImGui::Text("Fitness: %.2f", c->getFitness());

        ImGui::NextColumn();

        ImGui::Text("Size: %.2f", c->getSize());
        ImGui::Text("Speed: %.2f", c->getSpeed());
        ImGui::Text("Gen: %d", c->getGeneration());

        ImGui::Columns(1);

        ImGui::Separator();

        // Progress bar for spotlight time
        float progress = spotlight.showTime / spotlight.maxShowTime;
        ImGui::ProgressBar(progress, ImVec2(-1, 5), "");

        // Buttons
        if (ImGui::Button("Follow Camera")) {
            gameplay.toggleSpotlightFollow();
        }
        ImGui::SameLine();
        if (ImGui::Button("Next")) {
            gameplay.nextSpotlight();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            gameplay.clearSpotlight();
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(3);
}

inline void GameplayUI::renderEnvironmentEventBanner(Forge::GameplayManager& gameplay, float screenWidth) {
    const auto& event = gameplay.getCurrentEnvironmentEvent();
    if (!event.isActive()) return;

    float bannerHeight = 40.0f;

    ImGui::SetNextWindowPos(ImVec2(screenWidth * 0.25f, 30), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(screenWidth * 0.5f, bannerHeight), ImGuiCond_Always);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    // Color based on event type
    ImVec4 bgColor;
    switch (event.type) {
        case Forge::EnvironmentEventType::GOLDEN_AGE:
            bgColor = ImVec4(0.3f, 0.5f, 0.2f, 0.9f);
            break;
        case Forge::EnvironmentEventType::DROUGHT:
            bgColor = ImVec4(0.5f, 0.3f, 0.1f, 0.9f);
            break;
        case Forge::EnvironmentEventType::FOOD_BLOOM:
            bgColor = ImVec4(0.2f, 0.5f, 0.3f, 0.9f);
            break;
        case Forge::EnvironmentEventType::HARSH_WINTER:
            bgColor = ImVec4(0.3f, 0.4f, 0.5f, 0.9f);
            break;
        case Forge::EnvironmentEventType::BREEDING_SEASON:
            bgColor = ImVec4(0.5f, 0.3f, 0.4f, 0.9f);
            break;
        case Forge::EnvironmentEventType::PLAGUE:
            bgColor = ImVec4(0.4f, 0.2f, 0.3f, 0.9f);
            break;
        default:
            bgColor = ImVec4(0.3f, 0.3f, 0.3f, 0.9f);
    }

    ImGui::PushStyleColor(ImGuiCol_WindowBg, bgColor);

    if (ImGui::Begin("##EnvironmentBanner", nullptr, flags)) {
        // Center the text
        std::string text = event.name + " - " + event.description;
        float textWidth = ImGui::CalcTextSize(text.c_str()).x;
        ImGui::SetCursorPosX((screenWidth * 0.5f - textWidth) * 0.5f);

        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", text.c_str());

        // Progress bar
        float remaining = event.getRemainingTime();
        int mins = static_cast<int>(remaining) / 60;
        int secs = static_cast<int>(remaining) % 60;

        char timeStr[32];
        snprintf(timeStr, sizeof(timeStr), "%d:%02d remaining", mins, secs);

        ImGui::SameLine();
        ImGui::SetCursorPosX(screenWidth * 0.5f - 80);
        ImGui::ProgressBar(1.0f - event.getProgress(), ImVec2(150, 15), timeStr);
    }
    ImGui::End();

    ImGui::PopStyleColor();
}

inline void GameplayUI::renderAchievementPopup(Forge::GameplayManager& gameplay, float screenWidth, float screenHeight) {
    // Check for new achievements
    const Forge::Achievement* latest = gameplay.getLatestAchievement();
    if (latest && !m_showAchievementPopup) {
        m_currentAchievement = latest;
        m_showAchievementPopup = true;
        m_achievementPopupTimer = 0.0f;

        // Mark as notified (cast away const for this operation)
        const_cast<Forge::Achievement*>(latest)->notified = true;
    }

    if (!m_showAchievementPopup || !m_currentAchievement) return;

    m_achievementPopupTimer += ImGui::GetIO().DeltaTime;

    // Fade in/out animation
    float alpha = 1.0f;
    if (m_achievementPopupTimer < 0.5f) {
        alpha = m_achievementPopupTimer / 0.5f;
    } else if (m_achievementPopupTimer > ACHIEVEMENT_POPUP_DURATION - 0.5f) {
        alpha = (ACHIEVEMENT_POPUP_DURATION - m_achievementPopupTimer) / 0.5f;
    }

    if (m_achievementPopupTimer >= ACHIEVEMENT_POPUP_DURATION) {
        m_showAchievementPopup = false;
        m_currentAchievement = nullptr;
        return;
    }

    float popupWidth = 350.0f;
    float popupHeight = 100.0f;

    // Slide in from top
    float yOffset = -popupHeight * (1.0f - std::min(1.0f, m_achievementPopupTimer / 0.3f));

    ImGui::SetNextWindowPos(ImVec2((screenWidth - popupWidth) * 0.5f, 50 + yOffset), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(popupWidth, popupHeight), ImGuiCond_Always);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.12f, 0.05f, 0.95f * alpha));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.9f, 0.7f, 0.2f, alpha));

    if (ImGui::Begin("##AchievementPopup", nullptr, flags)) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.3f, alpha));
        ImGui::SetWindowFontScale(1.3f);

        // Center "Achievement Unlocked!"
        float textWidth = ImGui::CalcTextSize("Achievement Unlocked!").x;
        ImGui::SetCursorPosX((popupWidth - textWidth) * 0.5f);
        ImGui::Text("Achievement Unlocked!");

        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        ImGui::Separator();

        // Achievement name
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, alpha));
        textWidth = ImGui::CalcTextSize(m_currentAchievement->name.c_str()).x;
        ImGui::SetCursorPosX((popupWidth - textWidth) * 0.5f);
        ImGui::Text("%s", m_currentAchievement->name.c_str());

        // Description
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, alpha));
        textWidth = ImGui::CalcTextSize(m_currentAchievement->description.c_str()).x;
        ImGui::SetCursorPosX((popupWidth - textWidth) * 0.5f);
        ImGui::Text("%s", m_currentAchievement->description.c_str());
        ImGui::PopStyleColor(2);
    }
    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
}

inline void GameplayUI::handleInput(Forge::GameplayManager& gameplay) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) return;

    // Space to toggle pause
    if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
        gameplay.togglePause();
    }

    // Number keys for time scale (when not in text input)
    if (ImGui::IsKeyPressed(ImGuiKey_1)) gameplay.setTimeScale(Forge::TimeControl::SPEED_QUARTER);
    if (ImGui::IsKeyPressed(ImGuiKey_2)) gameplay.setTimeScale(Forge::TimeControl::SPEED_HALF);
    if (ImGui::IsKeyPressed(ImGuiKey_3)) gameplay.setTimeScale(Forge::TimeControl::SPEED_NORMAL);
    if (ImGui::IsKeyPressed(ImGuiKey_4)) gameplay.setTimeScale(Forge::TimeControl::SPEED_DOUBLE);
    if (ImGui::IsKeyPressed(ImGuiKey_5)) gameplay.setTimeScale(Forge::TimeControl::SPEED_QUAD);
    if (ImGui::IsKeyPressed(ImGuiKey_6)) gameplay.setTimeScale(Forge::TimeControl::SPEED_OCTO);

    // H for highlighting
    if (ImGui::IsKeyPressed(ImGuiKey_H)) {
        m_showHighlighting = !m_showHighlighting;
    }

    // G for statistics
    if (ImGui::IsKeyPressed(ImGuiKey_G)) {
        m_showStatistics = !m_showStatistics;
    }

    // J for achievements
    if (ImGui::IsKeyPressed(ImGuiKey_J)) {
        m_showAchievements = !m_showAchievements;
    }

    // K for spotlight
    if (ImGui::IsKeyPressed(ImGuiKey_K)) {
        if (gameplay.hasSpotlight()) {
            gameplay.nextSpotlight();
        }
    }

    // Highlight mode quick keys (Shift + number)
    if (io.KeyShift) {
        if (ImGui::IsKeyPressed(ImGuiKey_1)) gameplay.setHighlightMode(Forge::HighlightMode::NONE);
        if (ImGui::IsKeyPressed(ImGuiKey_2)) gameplay.setHighlightMode(Forge::HighlightMode::PREDATORS);
        if (ImGui::IsKeyPressed(ImGuiKey_3)) gameplay.setHighlightMode(Forge::HighlightMode::PREY);
        if (ImGui::IsKeyPressed(ImGuiKey_4)) gameplay.setHighlightMode(Forge::HighlightMode::HUNGRY);
        if (ImGui::IsKeyPressed(ImGuiKey_5)) gameplay.setHighlightMode(Forge::HighlightMode::REPRODUCING);
        if (ImGui::IsKeyPressed(ImGuiKey_6)) gameplay.setHighlightMode(Forge::HighlightMode::OLDEST);
        if (ImGui::IsKeyPressed(ImGuiKey_7)) gameplay.setHighlightMode(Forge::HighlightMode::YOUNGEST);
    }
}

inline const char* GameplayUI::getHighlightModeName(Forge::HighlightMode mode) {
    switch (mode) {
        case Forge::HighlightMode::NONE: return "None";
        case Forge::HighlightMode::PREDATORS: return "Predators (Red)";
        case Forge::HighlightMode::PREY: return "Herbivores (Green)";
        case Forge::HighlightMode::HUNGRY: return "Hungry (Yellow)";
        case Forge::HighlightMode::REPRODUCING: return "Ready to Reproduce (Pink)";
        case Forge::HighlightMode::SELECTED_SPECIES: return "Selected Species";
        case Forge::HighlightMode::OLDEST: return "Oldest (Gold)";
        case Forge::HighlightMode::YOUNGEST: return "Youngest (Cyan)";
        default: return "Unknown";
    }
}

inline const char* GameplayUI::getSpotlightCategoryName(Forge::SpotlightCategory category) {
    switch (category) {
        case Forge::SpotlightCategory::NONE: return "None";
        case Forge::SpotlightCategory::OLDEST: return "Oldest";
        case Forge::SpotlightCategory::FASTEST: return "Fastest";
        case Forge::SpotlightCategory::LARGEST: return "Largest";
        case Forge::SpotlightCategory::MOST_OFFSPRING: return "Most Offspring";
        case Forge::SpotlightCategory::MOST_KILLS: return "Most Kills";
        case Forge::SpotlightCategory::HIGHEST_FITNESS: return "Highest Fitness";
        case Forge::SpotlightCategory::RANDOM_INTERESTING: return "Random";
        default: return "Unknown";
    }
}

inline const char* GameplayUI::getEnvironmentEventName(Forge::EnvironmentEventType type) {
    switch (type) {
        case Forge::EnvironmentEventType::NONE: return "None";
        case Forge::EnvironmentEventType::GOLDEN_AGE: return "Golden Age";
        case Forge::EnvironmentEventType::DROUGHT: return "Drought";
        case Forge::EnvironmentEventType::FOOD_BLOOM: return "Food Bloom";
        case Forge::EnvironmentEventType::HARSH_WINTER: return "Harsh Winter";
        case Forge::EnvironmentEventType::MIGRATION_SEASON: return "Migration Season";
        case Forge::EnvironmentEventType::BREEDING_SEASON: return "Breeding Season";
        case Forge::EnvironmentEventType::PLAGUE: return "Plague";
        default: return "Unknown";
    }
}

inline ImVec4 GameplayUI::glmToImVec4(const glm::vec3& color, float alpha) {
    return ImVec4(color.r, color.g, color.b, alpha);
}

} // namespace ui
