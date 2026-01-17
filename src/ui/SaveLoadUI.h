#pragma once

// Save/Load and Replay UI for Evolution Simulator
// Provides ImGui panels for save/load dialogs and replay controls

#include "../core/SaveManager.h"
#include "../core/ReplaySystem.h"
#include "imgui.h"
#include <string>
#include <functional>

namespace Forge {
namespace UI {

// ============================================================================
// Save/Load Dialog State
// ============================================================================

enum class DialogMode {
    None,
    Save,
    Load,
    ConfirmOverwrite,
    ConfirmLoad
};

struct SaveLoadDialogState {
    DialogMode mode = DialogMode::None;
    std::string selectedFile;
    char saveNameBuffer[256] = "save_001";
    std::string pendingAction;
    std::vector<SaveSlotInfo> saveSlots;
    bool needsRefresh = true;
    std::string statusMessage;
    ImVec4 statusColor = ImVec4(1, 1, 1, 1);
};

// ============================================================================
// Save/Load UI Class
// ============================================================================

class SaveLoadUI {
public:
    SaveLoadUI() = default;

    // Set the save manager reference
    void setSaveManager(SaveManager* manager) { m_saveManager = manager; }

    // Callbacks for save/load actions
    using SaveCallback = std::function<bool(const std::string& filename)>;
    using LoadCallback = std::function<bool(const std::string& filename)>;
    using NewSimCallback = std::function<void()>;

    void setSaveCallback(SaveCallback cb) { m_onSave = cb; }
    void setLoadCallback(LoadCallback cb) { m_onLoad = cb; }
    void setNewSimCallback(NewSimCallback cb) { m_onNewSim = cb; }

    // Show the main menu bar file menu items
    void renderFileMenu();

    // Render dialogs (call every frame)
    void renderDialogs();

    // Open dialogs programmatically
    void showSaveDialog();
    void showLoadDialog();

    // Quick save/load (F5/F9)
    void quickSave();
    void quickLoad();

    // Handle keyboard shortcuts
    void handleInput();

    // Check if any dialog is open
    bool isDialogOpen() const { return m_state.mode != DialogMode::None; }

private:
    SaveManager* m_saveManager = nullptr;
    SaveLoadDialogState m_state;

    SaveCallback m_onSave;
    LoadCallback m_onLoad;
    NewSimCallback m_onNewSim;

    void renderSaveDialog();
    void renderLoadDialog();
    void renderConfirmOverwriteDialog();
    void renderConfirmLoadDialog();
    void refreshSaveSlots();
    void setStatus(const std::string& msg, bool isError = false);
};

// ============================================================================
// Replay Control UI Class
// ============================================================================

class ReplayUI {
public:
    ReplayUI() = default;

    // Set references
    void setRecorder(ReplayRecorder* recorder) { m_recorder = recorder; }
    void setPlayer(ReplayPlayer* player) { m_player = player; }

    // Callbacks
    using RecordFrameCallback = std::function<ReplayFrame()>;
    using ApplyFrameCallback = std::function<void(const ReplayFrame&)>;

    void setRecordFrameCallback(RecordFrameCallback cb) { m_getFrame = cb; }
    void setApplyFrameCallback(ApplyFrameCallback cb) { m_applyFrame = cb; }

    // Render the replay control panel
    void renderPanel();

    // Render the replay timeline (can be placed at bottom of screen)
    void renderTimeline(float width = -1.0f);

    // Handle keyboard shortcuts
    void handleInput();

    // Check if in playback mode
    bool isPlayingBack() const { return m_player && m_player->isPlaying(); }

private:
    ReplayRecorder* m_recorder = nullptr;
    ReplayPlayer* m_player = nullptr;

    RecordFrameCallback m_getFrame;
    ApplyFrameCallback m_applyFrame;

    // UI state
    bool m_showPanel = false;
    float m_seekPosition = 0.0f;
};

// ============================================================================
// SaveLoadUI Implementation
// ============================================================================

inline void SaveLoadUI::renderFileMenu() {
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Simulation", "Ctrl+N")) {
            if (m_onNewSim) m_onNewSim();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Save", "Ctrl+S")) {
            showSaveDialog();
        }

        if (ImGui::MenuItem("Load", "Ctrl+L")) {
            showLoadDialog();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Quick Save", "F5")) {
            quickSave();
        }

        if (ImGui::MenuItem("Quick Load", "F9")) {
            quickLoad();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Exit", "Alt+F4")) {
            // Request exit - handled by main loop
        }

        ImGui::EndMenu();
    }
}

inline void SaveLoadUI::renderDialogs() {
    switch (m_state.mode) {
        case DialogMode::Save:
            renderSaveDialog();
            break;
        case DialogMode::Load:
            renderLoadDialog();
            break;
        case DialogMode::ConfirmOverwrite:
            renderConfirmOverwriteDialog();
            break;
        case DialogMode::ConfirmLoad:
            renderConfirmLoadDialog();
            break;
        default:
            break;
    }
}

inline void SaveLoadUI::showSaveDialog() {
    m_state.mode = DialogMode::Save;
    m_state.needsRefresh = true;
    m_state.statusMessage.clear();
}

inline void SaveLoadUI::showLoadDialog() {
    m_state.mode = DialogMode::Load;
    m_state.needsRefresh = true;
    m_state.statusMessage.clear();
}

inline void SaveLoadUI::quickSave() {
    if (!m_saveManager || !m_onSave) return;

    if (m_onSave("quicksave.evos")) {
        setStatus("Quick saved!", false);
    } else {
        setStatus("Quick save failed!", true);
    }
}

inline void SaveLoadUI::quickLoad() {
    if (!m_saveManager || !m_onLoad) return;

    if (m_onLoad("quicksave.evos")) {
        setStatus("Quick loaded!", false);
    } else {
        setStatus("Quick load failed!", true);
    }
}

inline void SaveLoadUI::handleInput() {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) return;

    // F5 - Quick Save
    if (ImGui::IsKeyPressed(ImGuiKey_F5)) {
        quickSave();
    }

    // F9 - Quick Load
    if (ImGui::IsKeyPressed(ImGuiKey_F9)) {
        quickLoad();
    }

    // Ctrl+S - Save Dialog
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
        showSaveDialog();
    }

    // Ctrl+L - Load Dialog
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_L)) {
        showLoadDialog();
    }

    // Escape - Close dialogs
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        m_state.mode = DialogMode::None;
    }
}

inline void SaveLoadUI::refreshSaveSlots() {
    if (m_saveManager) {
        m_state.saveSlots = m_saveManager->listSaveSlots();
    }
    m_state.needsRefresh = false;
}

inline void SaveLoadUI::setStatus(const std::string& msg, bool isError) {
    m_state.statusMessage = msg;
    m_state.statusColor = isError ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f)
                                   : ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
}

inline void SaveLoadUI::renderSaveDialog() {
    if (m_state.needsRefresh) {
        refreshSaveSlots();
    }

    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Save Game", nullptr, ImGuiWindowFlags_NoCollapse)) {
        // Save name input
        ImGui::Text("Save Name:");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##savename", m_state.saveNameBuffer, sizeof(m_state.saveNameBuffer));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Existing saves list
        ImGui::Text("Existing Saves:");
        ImGui::BeginChild("SaveList", ImVec2(0, 200), true);

        for (const auto& slot : m_state.saveSlots) {
            bool selected = (m_state.selectedFile == slot.filename);

            char label[512];
            snprintf(label, sizeof(label), "%s\nGen: %u | Creatures: %u | Time: %.1fs\n%s",
                     slot.displayName.c_str(),
                     slot.generation,
                     slot.creatureCount,
                     slot.simulationTime,
                     slot.getTimestampString().c_str());

            if (ImGui::Selectable(label, selected, 0, ImVec2(0, 50))) {
                m_state.selectedFile = slot.filename;
                strncpy(m_state.saveNameBuffer, slot.displayName.c_str(),
                        sizeof(m_state.saveNameBuffer) - 1);
            }
        }

        ImGui::EndChild();

        // Status message
        if (!m_state.statusMessage.empty()) {
            ImGui::TextColored(m_state.statusColor, "%s", m_state.statusMessage.c_str());
        }

        ImGui::Spacing();

        // Buttons
        if (ImGui::Button("Save", ImVec2(100, 30))) {
            std::string filename = std::string(m_state.saveNameBuffer) + ".evos";

            // Check if file exists
            bool exists = false;
            for (const auto& slot : m_state.saveSlots) {
                if (slot.displayName == m_state.saveNameBuffer) {
                    exists = true;
                    break;
                }
            }

            if (exists) {
                m_state.pendingAction = filename;
                m_state.mode = DialogMode::ConfirmOverwrite;
            } else {
                if (m_onSave && m_onSave(filename)) {
                    setStatus("Saved successfully!", false);
                    m_state.mode = DialogMode::None;
                } else {
                    setStatus("Failed to save!", true);
                }
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(100, 30))) {
            m_state.mode = DialogMode::None;
        }
    }
    ImGui::End();
}

inline void SaveLoadUI::renderLoadDialog() {
    if (m_state.needsRefresh) {
        refreshSaveSlots();
    }

    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Load Game", nullptr, ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("Select a save to load:");
        ImGui::Spacing();

        // Saves list
        ImGui::BeginChild("SaveList", ImVec2(0, 280), true);

        for (const auto& slot : m_state.saveSlots) {
            bool selected = (m_state.selectedFile == slot.filename);

            char label[512];
            snprintf(label, sizeof(label), "%s\nGeneration: %u | Creatures: %u | Sim Time: %.1fs\nSaved: %s",
                     slot.displayName.c_str(),
                     slot.generation,
                     slot.creatureCount,
                     slot.simulationTime,
                     slot.getTimestampString().c_str());

            if (ImGui::Selectable(label, selected, 0, ImVec2(0, 60))) {
                m_state.selectedFile = slot.filename;
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                // Double-click to load
                m_state.pendingAction = slot.filename;
                m_state.mode = DialogMode::ConfirmLoad;
            }
        }

        if (m_state.saveSlots.empty()) {
            ImGui::TextDisabled("No saved games found.");
        }

        ImGui::EndChild();

        // Status message
        if (!m_state.statusMessage.empty()) {
            ImGui::TextColored(m_state.statusColor, "%s", m_state.statusMessage.c_str());
        }

        ImGui::Spacing();

        // Buttons
        bool hasSelection = !m_state.selectedFile.empty();

        if (!hasSelection) ImGui::BeginDisabled();
        if (ImGui::Button("Load", ImVec2(100, 30))) {
            m_state.pendingAction = m_state.selectedFile;
            m_state.mode = DialogMode::ConfirmLoad;
        }
        if (!hasSelection) ImGui::EndDisabled();

        ImGui::SameLine();

        if (!hasSelection) ImGui::BeginDisabled();
        if (ImGui::Button("Delete", ImVec2(100, 30))) {
            if (m_saveManager && m_saveManager->deleteSave(m_state.selectedFile)) {
                setStatus("Deleted successfully!", false);
                m_state.selectedFile.clear();
                m_state.needsRefresh = true;
            } else {
                setStatus("Failed to delete!", true);
            }
        }
        if (!hasSelection) ImGui::EndDisabled();

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(100, 30))) {
            m_state.mode = DialogMode::None;
        }
    }
    ImGui::End();
}

inline void SaveLoadUI::renderConfirmOverwriteDialog() {
    ImGui::SetNextWindowSize(ImVec2(350, 120), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::Begin("Confirm Overwrite", nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("A save with this name already exists.");
        ImGui::Text("Do you want to overwrite it?");

        ImGui::Spacing();

        if (ImGui::Button("Overwrite", ImVec2(100, 30))) {
            if (m_onSave && m_onSave(m_state.pendingAction)) {
                setStatus("Saved successfully!", false);
                m_state.mode = DialogMode::None;
            } else {
                setStatus("Failed to save!", true);
                m_state.mode = DialogMode::Save;
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(100, 30))) {
            m_state.mode = DialogMode::Save;
        }
    }
    ImGui::End();
}

inline void SaveLoadUI::renderConfirmLoadDialog() {
    ImGui::SetNextWindowSize(ImVec2(350, 120), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::Begin("Confirm Load", nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("Loading will discard current simulation.");
        ImGui::Text("Do you want to continue?");

        ImGui::Spacing();

        if (ImGui::Button("Load", ImVec2(100, 30))) {
            if (m_onLoad && m_onLoad(m_state.pendingAction)) {
                setStatus("Loaded successfully!", false);
                m_state.mode = DialogMode::None;
            } else {
                setStatus("Failed to load!", true);
                m_state.mode = DialogMode::Load;
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(100, 30))) {
            m_state.mode = DialogMode::Load;
        }
    }
    ImGui::End();
}

// ============================================================================
// ReplayUI Implementation
// ============================================================================

inline void ReplayUI::renderPanel() {
    if (!m_recorder || !m_player) return;

    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Replay Controls", &m_showPanel)) {
        // Recording section
        ImGui::Text("Recording");
        ImGui::Separator();

        if (m_recorder->isRecording()) {
            ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "RECORDING");
            ImGui::SameLine();
            ImGui::Text("Frames: %zu | Duration: %.1fs",
                        m_recorder->getFrameCount(),
                        m_recorder->getDuration());

            if (ImGui::Button("Stop Recording", ImVec2(-1, 30))) {
                m_recorder->stopRecording();
            }
        } else {
            ImGui::Text("Not recording");

            if (ImGui::Button("Start Recording", ImVec2(-1, 30))) {
                // Would need terrain seed passed in
                m_recorder->startRecording(12345);
            }
        }

        ImGui::Spacing();

        // Playback section
        ImGui::Text("Playback");
        ImGui::Separator();

        if (m_player->hasReplay()) {
            // Status
            if (m_player->isPlaying()) {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1), "PLAYING");
            } else if (m_player->isPaused()) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.3f, 1), "PAUSED");
            } else {
                ImGui::Text("STOPPED");
            }

            ImGui::SameLine();
            ImGui::Text("%.1f / %.1fs",
                        m_player->getCurrentTime(),
                        m_player->getDuration());

            // Transport controls
            if (ImGui::Button("|<", ImVec2(30, 30))) {
                m_player->stop();
            }
            ImGui::SameLine();
            if (ImGui::Button("<", ImVec2(30, 30))) {
                m_player->stepBackward();
            }
            ImGui::SameLine();
            if (m_player->isPlaying()) {
                if (ImGui::Button("||", ImVec2(30, 30))) {
                    m_player->pause();
                }
            } else {
                if (ImGui::Button(">", ImVec2(30, 30))) {
                    m_player->play();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(">", ImVec2(30, 30))) {
                m_player->stepForward();
            }
            ImGui::SameLine();
            if (ImGui::Button(">|", ImVec2(30, 30))) {
                m_player->seekPercent(1.0f);
            }

            // Speed control
            ImGui::Text("Speed:");
            ImGui::SameLine();
            float speed = m_player->getSpeed();
            if (ImGui::SliderFloat("##speed", &speed, 0.1f, 4.0f, "%.1fx")) {
                m_player->setSpeed(speed);
            }

            // Quick speed buttons
            if (ImGui::Button("0.5x")) m_player->setSpeed(0.5f);
            ImGui::SameLine();
            if (ImGui::Button("1x")) m_player->setSpeed(1.0f);
            ImGui::SameLine();
            if (ImGui::Button("2x")) m_player->setSpeed(2.0f);
            ImGui::SameLine();
            if (ImGui::Button("4x")) m_player->setSpeed(4.0f);

        } else {
            ImGui::TextDisabled("No replay loaded");
        }
    }
    ImGui::End();
}

inline void ReplayUI::renderTimeline(float width) {
    if (!m_player || !m_player->hasReplay()) return;

    if (width < 0) {
        width = ImGui::GetContentRegionAvail().x;
    }

    float progress = m_player->getProgress();
    float duration = m_player->getDuration();

    // Timeline bar
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size(width, 20);

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                            IM_COL32(40, 40, 40, 255), 4.0f);

    // Progress bar
    float progressWidth = size.x * progress;
    drawList->AddRectFilled(pos, ImVec2(pos.x + progressWidth, pos.y + size.y),
                            IM_COL32(100, 150, 255, 255), 4.0f);

    // Border
    drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                      IM_COL32(80, 80, 80, 255), 4.0f);

    // Playhead
    float playheadX = pos.x + progressWidth;
    drawList->AddLine(ImVec2(playheadX, pos.y),
                      ImVec2(playheadX, pos.y + size.y),
                      IM_COL32(255, 255, 255, 255), 2.0f);

    // Make it interactive
    ImGui::SetCursorScreenPos(pos);
    ImGui::InvisibleButton("timeline", size);

    if (ImGui::IsItemActive()) {
        float mouseX = ImGui::GetMousePos().x - pos.x;
        float newProgress = std::clamp(mouseX / size.x, 0.0f, 1.0f);
        m_player->seekPercent(newProgress);
    }

    // Time label
    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + size.y + 2));
    ImGui::Text("%.1f / %.1f s  |  Frame %zu / %zu",
                m_player->getCurrentTime(), duration,
                m_player->getCurrentFrameIndex() + 1,
                m_player->getTotalFrames());
}

inline void ReplayUI::handleInput() {
    if (!m_player) return;

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) return;

    // Space - Play/Pause
    if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
        m_player->togglePause();
    }

    // Left/Right arrows - Step
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        m_player->stepBackward();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        m_player->stepForward();
    }

    // Home/End - Seek to start/end
    if (ImGui::IsKeyPressed(ImGuiKey_Home)) {
        m_player->stop();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_End)) {
        m_player->seekPercent(1.0f);
    }
}

} // namespace UI
} // namespace Forge
