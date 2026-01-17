#pragma once

// TimeControls - Simulation time manipulation for God Mode
// Control speed, pause, step frames, and skip generations

#include "imgui.h"
#include "../core/SimulationOrchestrator.h"
#include <functional>

namespace ui {

// Preset time scales
enum class TimePreset {
    PAUSED,         // 0x
    SLOW_MO,        // 0.25x
    HALF_SPEED,     // 0.5x
    NORMAL,         // 1x
    FAST,           // 2x
    VERY_FAST,      // 4x
    ULTRA_FAST,     // 10x
    CUSTOM          // User-defined
};

class TimeControls {
public:
    TimeControls();
    ~TimeControls() = default;

    // Main UI render
    void renderUI();

    // Render compact controls (for toolbar)
    void renderCompact();

    // Render as section
    void renderSection();

    // Set simulation orchestrator
    void setSimulation(Forge::SimulationOrchestrator* sim) { m_simulation = sim; }

    // Basic controls
    void pause();
    void resume();
    void togglePause();
    bool isPaused() const;

    // Speed control
    void setSpeed(float multiplier);
    float getSpeed() const { return m_currentSpeed; }
    void setPreset(TimePreset preset);
    TimePreset getCurrentPreset() const { return m_currentPreset; }

    // Frame stepping
    void stepFrame();
    void stepFrames(int count);

    // Generation skipping
    void skipGenerations(int count);
    void skipToGeneration(int targetGen);

    // Get time info
    float getSimulationTime() const;
    int getCurrentDay() const;
    int getMaxGeneration() const;

    // Keyboard shortcuts
    void handleKeyboardShortcuts();

    // Panel visibility
    bool isVisible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }
    void toggleVisible() { m_visible = !m_visible; }

    // Callbacks
    using TimeChangedCallback = std::function<void(float newSpeed, bool paused)>;
    void setOnTimeChanged(TimeChangedCallback cb) { m_onTimeChanged = cb; }

private:
    // Dependencies
    Forge::SimulationOrchestrator* m_simulation = nullptr;

    // State
    float m_currentSpeed = 1.0f;
    float m_customSpeed = 1.0f;
    TimePreset m_currentPreset = TimePreset::NORMAL;
    bool m_wasPaused = false;

    // Frame stepping
    int m_pendingSteps = 0;
    bool m_stepping = false;

    // Generation skipping
    bool m_skippingGenerations = false;
    int m_targetGeneration = 0;

    // UI state
    bool m_visible = true;
    bool m_showAdvanced = false;

    // Callbacks
    TimeChangedCallback m_onTimeChanged;

    // Helper methods
    void renderSpeedButtons();
    void renderTimeInfo();
    void renderFrameStepping();
    void renderGenerationControls();

    static const char* getPresetName(TimePreset preset);
    static float getPresetSpeed(TimePreset preset);

    void notifyTimeChanged();
};

// ============================================================================
// Implementation
// ============================================================================

inline const char* TimeControls::getPresetName(TimePreset preset) {
    switch (preset) {
        case TimePreset::PAUSED: return "Paused";
        case TimePreset::SLOW_MO: return "0.25x";
        case TimePreset::HALF_SPEED: return "0.5x";
        case TimePreset::NORMAL: return "1x";
        case TimePreset::FAST: return "2x";
        case TimePreset::VERY_FAST: return "4x";
        case TimePreset::ULTRA_FAST: return "10x";
        case TimePreset::CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

inline float TimeControls::getPresetSpeed(TimePreset preset) {
    switch (preset) {
        case TimePreset::PAUSED: return 0.0f;
        case TimePreset::SLOW_MO: return 0.25f;
        case TimePreset::HALF_SPEED: return 0.5f;
        case TimePreset::NORMAL: return 1.0f;
        case TimePreset::FAST: return 2.0f;
        case TimePreset::VERY_FAST: return 4.0f;
        case TimePreset::ULTRA_FAST: return 10.0f;
        case TimePreset::CUSTOM: return 1.0f;  // Will use m_customSpeed
        default: return 1.0f;
    }
}

inline TimeControls::TimeControls() {}

inline bool TimeControls::isPaused() const {
    if (m_simulation) {
        return m_simulation->getState() == Forge::SimulationState::PAUSED;
    }
    return m_currentPreset == TimePreset::PAUSED;
}

inline void TimeControls::pause() {
    if (m_simulation) {
        m_simulation->pause();
    }
    m_currentPreset = TimePreset::PAUSED;
    notifyTimeChanged();
}

inline void TimeControls::resume() {
    if (m_simulation) {
        m_simulation->resume();
        m_simulation->setTimeScale(m_currentSpeed);
    }
    if (m_currentPreset == TimePreset::PAUSED) {
        m_currentPreset = TimePreset::NORMAL;
        m_currentSpeed = 1.0f;
    }
    notifyTimeChanged();
}

inline void TimeControls::togglePause() {
    if (isPaused()) {
        resume();
    } else {
        pause();
    }
}

inline void TimeControls::setSpeed(float multiplier) {
    m_currentSpeed = glm::clamp(multiplier, 0.1f, 10.0f);

    if (m_simulation) {
        m_simulation->setTimeScale(m_currentSpeed);
        if (m_currentSpeed > 0.0f && isPaused()) {
            m_simulation->resume();
        }
    }

    // Determine preset
    if (abs(m_currentSpeed - 0.25f) < 0.01f) {
        m_currentPreset = TimePreset::SLOW_MO;
    } else if (abs(m_currentSpeed - 0.5f) < 0.01f) {
        m_currentPreset = TimePreset::HALF_SPEED;
    } else if (abs(m_currentSpeed - 1.0f) < 0.01f) {
        m_currentPreset = TimePreset::NORMAL;
    } else if (abs(m_currentSpeed - 2.0f) < 0.01f) {
        m_currentPreset = TimePreset::FAST;
    } else if (abs(m_currentSpeed - 4.0f) < 0.01f) {
        m_currentPreset = TimePreset::VERY_FAST;
    } else if (abs(m_currentSpeed - 10.0f) < 0.01f) {
        m_currentPreset = TimePreset::ULTRA_FAST;
    } else {
        m_currentPreset = TimePreset::CUSTOM;
        m_customSpeed = m_currentSpeed;
    }

    notifyTimeChanged();
}

inline void TimeControls::setPreset(TimePreset preset) {
    m_currentPreset = preset;

    if (preset == TimePreset::PAUSED) {
        pause();
        return;
    }

    float speed = (preset == TimePreset::CUSTOM) ? m_customSpeed : getPresetSpeed(preset);
    setSpeed(speed);
}

inline void TimeControls::stepFrame() {
    // Pause, then advance one frame
    if (m_simulation) {
        if (!isPaused()) {
            pause();
        }
        // Would need simulation to support single frame stepping
        // For now, briefly unpause
        m_simulation->resume();
        m_simulation->setTimeScale(0.1f);
        m_pendingSteps = 1;
        m_stepping = true;
    }
}

inline void TimeControls::stepFrames(int count) {
    m_pendingSteps = count;
    m_stepping = true;
}

inline void TimeControls::skipGenerations(int count) {
    if (!m_simulation) return;

    int currentGen = getMaxGeneration();
    m_targetGeneration = currentGen + count;
    m_skippingGenerations = true;

    // Run at maximum speed
    m_simulation->setTimeScale(10.0f);
    m_simulation->resume();
}

inline void TimeControls::skipToGeneration(int targetGen) {
    if (!m_simulation) return;

    m_targetGeneration = targetGen;
    m_skippingGenerations = true;

    m_simulation->setTimeScale(10.0f);
    m_simulation->resume();
}

inline float TimeControls::getSimulationTime() const {
    if (m_simulation) {
        return m_simulation->getStats().simulationTime;
    }
    return 0.0f;
}

inline int TimeControls::getCurrentDay() const {
    if (m_simulation) {
        return m_simulation->getStats().dayCount;
    }
    return 0;
}

inline int TimeControls::getMaxGeneration() const {
    if (m_simulation) {
        return m_simulation->getStats().maxGeneration;
    }
    return 0;
}

inline void TimeControls::handleKeyboardShortcuts() {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) return;

    // Space - toggle pause
    if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
        togglePause();
    }

    // Period/dot - step frame
    if (ImGui::IsKeyPressed(ImGuiKey_Period)) {
        stepFrame();
    }

    // Number keys for speed presets
    if (ImGui::IsKeyPressed(ImGuiKey_1)) setPreset(TimePreset::SLOW_MO);
    if (ImGui::IsKeyPressed(ImGuiKey_2)) setPreset(TimePreset::HALF_SPEED);
    if (ImGui::IsKeyPressed(ImGuiKey_3)) setPreset(TimePreset::NORMAL);
    if (ImGui::IsKeyPressed(ImGuiKey_4)) setPreset(TimePreset::FAST);
    if (ImGui::IsKeyPressed(ImGuiKey_5)) setPreset(TimePreset::VERY_FAST);
    if (ImGui::IsKeyPressed(ImGuiKey_6)) setPreset(TimePreset::ULTRA_FAST);
}

inline void TimeControls::notifyTimeChanged() {
    if (m_onTimeChanged) {
        m_onTimeChanged(m_currentSpeed, isPaused());
    }
}

inline void TimeControls::renderUI() {
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(300, 350), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Time Controls", &m_visible)) {
        renderSection();
    }
    ImGui::End();
}

inline void TimeControls::renderCompact() {
    // Compact horizontal layout for toolbar
    bool paused = isPaused();

    // Pause/Play button
    if (paused) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        if (ImGui::Button("Play", ImVec2(50, 0))) {
            resume();
        }
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.3f, 0.2f, 1.0f));
        if (ImGui::Button("Pause", ImVec2(50, 0))) {
            pause();
        }
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();

    // Speed indicator
    ImGui::Text("%.1fx", m_currentSpeed);

    ImGui::SameLine();

    // Speed buttons
    if (ImGui::Button("-", ImVec2(25, 0))) {
        setSpeed(m_currentSpeed * 0.5f);
    }
    ImGui::SameLine();
    if (ImGui::Button("+", ImVec2(25, 0))) {
        setSpeed(m_currentSpeed * 2.0f);
    }
}

inline void TimeControls::renderSection() {
    // Current state display
    renderTimeInfo();

    ImGui::Separator();

    // Main controls
    if (ImGui::CollapsingHeader("Speed Control", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderSpeedButtons();
    }

    // Frame stepping
    if (ImGui::CollapsingHeader("Frame Stepping")) {
        renderFrameStepping();
    }

    // Generation controls
    if (ImGui::CollapsingHeader("Generation Control")) {
        renderGenerationControls();
    }

    // Keyboard shortcuts help
    ImGui::Separator();
    ImGui::TextDisabled("Shortcuts: Space=Pause, .=Step, 1-6=Speed");
}

inline void TimeControls::renderTimeInfo() {
    // Simulation state
    bool paused = isPaused();

    if (paused) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "PAUSED");
    } else {
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "RUNNING");
    }

    ImGui::SameLine(100);
    ImGui::Text("Speed: %.2fx", m_currentSpeed);

    // Time statistics
    ImGui::Text("Simulation Time: %.1f s", getSimulationTime());
    ImGui::Text("Day: %d", getCurrentDay());
    ImGui::Text("Max Generation: %d", getMaxGeneration());

    // Skip progress
    if (m_skippingGenerations) {
        int currentGen = getMaxGeneration();
        if (currentGen >= m_targetGeneration) {
            m_skippingGenerations = false;
            setSpeed(1.0f);
        } else {
            ImGui::ProgressBar(
                static_cast<float>(currentGen) / m_targetGeneration,
                ImVec2(-1, 0),
                "Skipping generations..."
            );
        }
    }
}

inline void TimeControls::renderSpeedButtons() {
    bool paused = isPaused();

    // Big pause/play button
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

    if (paused) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
        if (ImGui::Button("PLAY", ImVec2(-1, 40))) {
            resume();
        }
        ImGui::PopStyleColor(2);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.3f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.4f, 0.3f, 1.0f));
        if (ImGui::Button("PAUSE", ImVec2(-1, 40))) {
            pause();
        }
        ImGui::PopStyleColor(2);
    }

    ImGui::PopStyleVar();

    ImGui::Separator();

    // Speed preset buttons
    ImGui::Text("Speed Presets:");

    const TimePreset presets[] = {
        TimePreset::SLOW_MO, TimePreset::HALF_SPEED, TimePreset::NORMAL,
        TimePreset::FAST, TimePreset::VERY_FAST, TimePreset::ULTRA_FAST
    };

    for (int i = 0; i < 6; ++i) {
        bool selected = (m_currentPreset == presets[i]);

        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));
        }

        if (ImGui::Button(getPresetName(presets[i]), ImVec2(70, 25))) {
            setPreset(presets[i]);
        }

        if (selected) {
            ImGui::PopStyleColor();
        }

        if ((i + 1) % 3 != 0) ImGui::SameLine();
    }

    ImGui::Separator();

    // Custom speed slider
    ImGui::Text("Custom Speed:");
    if (ImGui::SliderFloat("##CustomSpeed", &m_customSpeed, 0.1f, 10.0f, "%.2fx")) {
        setSpeed(m_customSpeed);
    }
}

inline void TimeControls::renderFrameStepping() {
    ImGui::TextWrapped("Step through the simulation frame by frame:");

    if (ImGui::Button("Step 1 Frame", ImVec2(-1, 0))) {
        stepFrame();
    }

    if (ImGui::Button("Step 10 Frames", ImVec2(-1, 0))) {
        stepFrames(10);
    }

    if (ImGui::Button("Step 60 Frames", ImVec2(-1, 0))) {
        stepFrames(60);
    }
}

inline void TimeControls::renderGenerationControls() {
    ImGui::TextWrapped("Skip forward to observe evolution over longer periods:");

    int currentGen = getMaxGeneration();
    ImGui::Text("Current Generation: %d", currentGen);

    static int skipCount = 10;
    ImGui::SetNextItemWidth(100);
    ImGui::InputInt("Generations to Skip", &skipCount);
    skipCount = std::max(1, std::min(1000, skipCount));

    if (ImGui::Button("Skip Generations", ImVec2(-1, 0))) {
        skipGenerations(skipCount);
    }

    ImGui::Separator();

    // Jump to specific generation
    static int targetGen = 50;
    ImGui::SetNextItemWidth(100);
    ImGui::InputInt("Target Generation", &targetGen);
    targetGen = std::max(currentGen + 1, targetGen);

    if (ImGui::Button("Skip to Generation", ImVec2(-1, 0))) {
        skipToGeneration(targetGen);
    }

    if (m_skippingGenerations) {
        ImGui::Separator();
        if (ImGui::Button("Cancel Skip", ImVec2(-1, 0))) {
            m_skippingGenerations = false;
            setSpeed(1.0f);
        }
    }
}

} // namespace ui
