# Replay System Integration Plan

## Overview

This document outlines the plan to integrate the existing `ReplaySystem.h` into `main.cpp` so that recording and playback functionality works correctly.

**Issues to Fix:**
- S-05: Replay system not wired into main loop
- S-06: Playback mode not implemented
- S-07: Neural weights not recorded in snapshots
- S-08: Circular buffer uses inefficient O(n) erase

## Current State Analysis

### ReplaySystem.h API Summary

**ReplayRecorder:**
- `startRecording(uint32_t terrainSeed)` - Begin recording
- `stopRecording()` - Stop recording
- `update(float dt, float simulationTime)` - Update timing
- `recordFrame(const ReplayFrame& frame)` - Record frame if interval elapsed
- `forceRecordFrame(const ReplayFrame& frame)` - Record immediately
- `saveReplay(const std::string& filename)` - Save to file
- `getFrameCount()` / `getDuration()` - Query stats

**ReplayPlayer:**
- `loadReplay(const std::string& filename)` - Load from file
- `play()` / `pause()` / `stop()` / `togglePause()` - Playback control
- `setSpeed(float speed)` - 0.1x to 10.0x
- `seek(float time)` / `seekPercent(float percent)` / `seekToFrame(size_t)` - Seeking
- `stepForward()` / `stepBackward()` - Frame stepping
- `update(float dt)` - Advance playback
- `getInterpolatedFrame()` - Get smoothly interpolated frame
- `getCurrentFrame()` - Get raw current frame

**Data Structures:**
- `CreatureSnapshot`: id, type, position, rotation, health, energy, animPhase, color, size
- `FoodSnapshot`: position, energy, active flag
- `CameraSnapshot`: position, target, FOV
- `StatisticsSnapshot`: population counts, generation, fitness averages
- `ReplayFrame`: timestamp, creatures[], food[], camera, stats

### main.cpp Integration Points

**Key Variables:**
- `SimulationWorld::creatures` - `std::vector<std::unique_ptr<Creature>>`
- `SimulationWorld::food` - `std::vector<std::unique_ptr<Food>>`
- `SimulationWorld::simulationTime` - float tracking elapsed time
- `Camera` - position, pitch, yaw, zoom

**Key Methods:**
- `SimulationWorld::Update(float dt)` - Main simulation loop
- `HandleInput(float dt)` - Keyboard/mouse input
- `RenderDebugPanel(...)` - ImGui UI rendering

## Implementation Plan

### Phase 1: Recording Integration

**1.1 Add Member Variables to EvolutionSimulation class:**
```cpp
#include "core/ReplaySystem.h"

ReplayRecorder m_replayRecorder;
bool m_isRecording = true;  // Auto-start recording
```

**1.2 Initialize in Constructor:**
```cpp
m_replayRecorder.startRecording(m_world.terrainSeed);
m_replayRecorder.setRecordInterval(1.0f);  // 1 frame per second
```

**1.3 Record Frames in Update():**
After `m_world.Update(scaledDt)`:
```cpp
if (m_isRecording) {
    m_replayRecorder.update(scaledDt, m_world.simulationTime);

    ReplayFrame frame = buildReplayFrame();
    m_replayRecorder.recordFrame(frame);
}
```

**1.4 Helper Method to Build Frame:**
```cpp
ReplayFrame buildReplayFrame() {
    ReplayFrame frame;
    frame.timestamp = m_world.simulationTime;

    // Capture creatures
    for (const auto& creature : m_world.creatures) {
        CreatureSnapshot snap;
        snap.id = creature->id;
        snap.type = static_cast<uint8_t>(creature->type);
        snap.posX = creature->position.x;
        snap.posY = creature->position.y;
        snap.posZ = creature->position.z;
        snap.rotation = creature->rotation;
        snap.health = creature->health;
        snap.energy = creature->energy;
        snap.animPhase = creature->animPhase;
        // Color from genome
        snap.colorR = creature->genome.colorR;
        snap.colorG = creature->genome.colorG;
        snap.colorB = creature->genome.colorB;
        snap.size = creature->genome.size;
        frame.creatures.push_back(snap);
    }

    // Capture food
    for (const auto& food : m_world.food) {
        FoodSnapshot fsnap;
        fsnap.posX = food->position.x;
        fsnap.posY = food->position.y;
        fsnap.posZ = food->position.z;
        fsnap.energy = food->amount;
        fsnap.active = food->active;
        frame.food.push_back(fsnap);
    }

    // Capture camera
    frame.camera.posX = m_camera.position.x;
    frame.camera.posY = m_camera.position.y;
    frame.camera.posZ = m_camera.position.z;
    // Calculate target from yaw/pitch
    Vec3 forward = getForwardVector(m_camera.yaw, m_camera.pitch);
    frame.camera.targetX = m_camera.position.x + forward.x;
    frame.camera.targetY = m_camera.position.y + forward.y;
    frame.camera.targetZ = m_camera.position.z + forward.z;
    frame.camera.fov = m_camera.zoom;

    // Capture statistics
    frame.stats.herbivoreCount = countHerbivores();
    frame.stats.carnivoreCount = countCarnivores();
    frame.stats.foodCount = m_world.food.size();
    frame.stats.generation = m_world.generation;

    return frame;
}
```

### Phase 2: Fix S-07 - Add Neural Weights to Snapshots

**2.1 Extend CreatureSnapshot in ReplaySystem.h:**
```cpp
struct CreatureSnapshot {
    // ... existing fields ...

    // Neural network weights (S-07 fix)
    std::vector<float> neuralWeights;

    // Genome data for complete reproduction
    float genomeSpeed;
    float genomeSize;
    float genomeVision;
    float genomeAggression;
};
```

**2.2 Capture Neural Weights:**
```cpp
// In buildReplayFrame():
snap.neuralWeights = creature->brain.getWeights();
snap.genomeSpeed = creature->genome.speed;
snap.genomeSize = creature->genome.size;
snap.genomeVision = creature->genome.visionRange;
snap.genomeAggression = creature->genome.aggression;
```

### Phase 3: Fix S-08 - Efficient Circular Buffer

**3.1 Replace Vector with Ring Buffer in ReplayRecorder:**
```cpp
class ReplayRecorder {
private:
    std::vector<ReplayFrame> m_frames;
    size_t m_writeIndex = 0;
    size_t m_frameCount = 0;
    size_t m_maxFrames = 36000;

    void addFrame(const ReplayFrame& frame) {
        if (m_frames.size() < m_maxFrames) {
            m_frames.push_back(frame);
        } else {
            m_frames[m_writeIndex] = frame;
        }
        m_writeIndex = (m_writeIndex + 1) % m_maxFrames;
        m_frameCount = std::min(m_frameCount + 1, m_maxFrames);
    }

    const ReplayFrame& getFrame(size_t index) const {
        size_t startIndex = (m_frameCount >= m_maxFrames)
            ? m_writeIndex
            : 0;
        return m_frames[(startIndex + index) % m_maxFrames];
    }
};
```

### Phase 4: Playback Integration

**4.1 Add Playback Variables:**
```cpp
ReplayPlayer m_replayPlayer;
bool m_isPlayingReplay = false;
```

**4.2 Mode Toggle Methods:**
```cpp
void enterReplayMode() {
    if (m_replayRecorder.getFrameCount() == 0) {
        SetStatusMessage("No replay data to play");
        return;
    }
    m_isPlayingReplay = true;
    m_isRecording = false;
    m_renderer.m_simulationPaused = true;

    // Transfer frames to player
    m_replayPlayer.loadFromRecorder(m_replayRecorder);
    m_replayPlayer.play();
    SetStatusMessage("Entered replay mode");
}

void exitReplayMode() {
    m_isPlayingReplay = false;
    m_isRecording = true;
    m_replayPlayer.stop();
    SetStatusMessage("Exited replay mode");
}
```

**4.3 Modify Update Loop:**
```cpp
void Update(f32 dt) {
    m_time += dt;
    if (m_time > 1000.0f) m_time -= 1000.0f;

    if (m_isPlayingReplay) {
        // Playback mode
        m_replayPlayer.update(dt);
        applyReplayFrame(m_replayPlayer.getInterpolatedFrame());
    } else if (!m_renderer.m_simulationPaused) {
        // Normal simulation
        f32 scaledDt = dt * m_renderer.m_simulationSpeed;
        m_world.Update(scaledDt);

        // Recording
        if (m_isRecording) {
            m_replayRecorder.update(scaledDt, m_world.simulationTime);
            ReplayFrame frame = buildReplayFrame();
            m_replayRecorder.recordFrame(frame);
        }
    }
}
```

**4.4 Apply Replay Frame:**
```cpp
void applyReplayFrame(const ReplayFrame& frame) {
    // Update camera
    m_camera.position.x = frame.camera.posX;
    m_camera.position.y = frame.camera.posY;
    m_camera.position.z = frame.camera.posZ;
    m_camera.zoom = frame.camera.fov;
    // Calculate yaw/pitch from target
    Vec3 target(frame.camera.targetX, frame.camera.targetY, frame.camera.targetZ);
    Vec3 dir = normalize(target - m_camera.position);
    m_camera.pitch = asin(dir.y);
    m_camera.yaw = atan2(dir.x, dir.z);

    // Update creature visuals (create ghost creatures for display)
    updateCreaturesFromFrame(frame.creatures);

    // Update food visuals
    updateFoodFromFrame(frame.food);
}
```

### Phase 5: UI Integration

**5.1 Recording Indicator:**
```cpp
// In RenderDebugPanel():
if (m_isRecording) {
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "● REC");
    ImGui::SameLine();
    ImGui::Text("%.1f min (%zu frames)",
        m_replayRecorder.getDuration() / 60.0f,
        m_replayRecorder.getFrameCount());
}
```

**5.2 Replay Controls Panel:**
```cpp
if (ImGui::CollapsingHeader("Replay Controls")) {
    if (!m_isPlayingReplay) {
        if (ImGui::Button("Enter Replay Mode (F10)")) {
            enterReplayMode();
        }
        ImGui::Text("Recorded: %zu frames (%.1f min)",
            m_replayRecorder.getFrameCount(),
            m_replayRecorder.getDuration() / 60.0f);
    } else {
        // Timeline scrubber
        float progress = m_replayPlayer.getProgress();
        if (ImGui::SliderFloat("Timeline", &progress, 0.0f, 1.0f)) {
            m_replayPlayer.seekPercent(progress);
        }

        ImGui::Text("Frame: %zu / %zu",
            m_replayPlayer.getCurrentFrameIndex() + 1,
            m_replayPlayer.getTotalFrames());
        ImGui::Text("Time: %.1f / %.1f sec",
            m_replayPlayer.getCurrentTime(),
            m_replayPlayer.getDuration());

        // Playback controls
        if (ImGui::Button(m_replayPlayer.isPaused() ? "Play" : "Pause")) {
            m_replayPlayer.togglePause();
        }
        ImGui::SameLine();

        // Frame stepping
        if (ImGui::Button("<<")) m_replayPlayer.stepBackward();
        ImGui::SameLine();
        if (ImGui::Button(">>")) m_replayPlayer.stepForward();

        // Speed control
        float speed = m_replayPlayer.getSpeed();
        if (ImGui::SliderFloat("Speed", &speed, 0.25f, 4.0f, "%.2fx")) {
            m_replayPlayer.setSpeed(speed);
        }

        ImGui::Separator();
        if (ImGui::Button("Exit Replay (F10)")) {
            exitReplayMode();
        }
    }
}
```

**5.3 Save/Load UI:**
```cpp
// In Replay Controls section:
ImGui::Separator();
static char replayFilename[256] = "replay.rpl";
ImGui::InputText("Filename", replayFilename, sizeof(replayFilename));

if (ImGui::Button("Save Replay")) {
    std::string path = "replays/" + std::string(replayFilename);
    if (m_replayRecorder.saveReplay(path)) {
        SetStatusMessage("Saved: " + path);
    } else {
        SetStatusMessage("Failed to save replay");
    }
}
ImGui::SameLine();
if (ImGui::Button("Load Replay")) {
    std::string path = "replays/" + std::string(replayFilename);
    if (m_replayPlayer.loadReplay(path)) {
        enterReplayMode();
    } else {
        SetStatusMessage("Failed to load: " + path);
    }
}
```

### Phase 6: Keyboard Shortcuts

**6.1 Add to HandleInput():**
```cpp
// F10 - Toggle replay mode
static bool s_wasF10Pressed = false;
bool f10Pressed = GetAsyncKeyState(VK_F10) & 0x8000;
if (f10Pressed && !s_wasF10Pressed && !imguiWantsKeyboard) {
    if (m_isPlayingReplay) {
        exitReplayMode();
    } else {
        enterReplayMode();
    }
}
s_wasF10Pressed = f10Pressed;

// During replay mode, override normal inputs
if (m_isPlayingReplay) {
    // Space to toggle play/pause
    static bool s_wasSpacePressed = false;
    bool spacePressed = GetAsyncKeyState(VK_SPACE) & 0x8000;
    if (spacePressed && !s_wasSpacePressed && !imguiWantsKeyboard) {
        m_replayPlayer.togglePause();
    }
    s_wasSpacePressed = spacePressed;

    // Left/Right arrows for frame stepping
    static bool s_wasLeftPressed = false;
    static bool s_wasRightPressed = false;
    bool leftPressed = GetAsyncKeyState(VK_LEFT) & 0x8000;
    bool rightPressed = GetAsyncKeyState(VK_RIGHT) & 0x8000;
    if (leftPressed && !s_wasLeftPressed) m_replayPlayer.stepBackward();
    if (rightPressed && !s_wasRightPressed) m_replayPlayer.stepForward();
    s_wasLeftPressed = leftPressed;
    s_wasRightPressed = rightPressed;

    return; // Skip normal input handling
}
```

## Deliverables Checklist

- [ ] `ReplayRecorder` member variable in `EvolutionSimulation`
- [ ] `recordFrame()` called each update with proper frame data
- [ ] Neural weights included in `CreatureSnapshot` (S-07 fixed)
- [ ] Ring buffer optimization in `ReplayRecorder` (S-08 fixed)
- [ ] `ReplayPlayer` member variable
- [ ] Playback mode with smooth interpolation
- [ ] ImGui replay controls panel
- [ ] Timeline scrubber with seek functionality
- [ ] Playback speed control (0.25x to 4x)
- [ ] Frame stepping (forward/backward)
- [ ] F10 keyboard shortcut for mode toggle
- [ ] Save/load replays to file
- [ ] Recording indicator in UI (● REC with duration)

## File Changes Summary

1. **src/core/ReplaySystem.h** - Add neural weights to CreatureSnapshot, fix ring buffer
2. **src/main.cpp** - Add recorder/player, modify Update(), add UI, add input handling

## Estimated Effort

- Phase 1 (Recording): 1 hour
- Phase 2 (Neural Weights): 30 minutes
- Phase 3 (Ring Buffer): 30 minutes
- Phase 4 (Playback): 1 hour
- Phase 5 (UI): 45 minutes
- Phase 6 (Shortcuts): 15 minutes

**Total: ~4 hours**
