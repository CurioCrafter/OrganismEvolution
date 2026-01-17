#pragma once

// Replay System for Evolution Simulator
// Records and plays back simulation state over time

#include "Serializer.h"
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <chrono>

namespace Forge {

// ============================================================================
// Replay Data Structures
// ============================================================================

// Snapshot of a single creature at a point in time
struct CreatureSnapshot {
    uint32_t id = 0;
    uint8_t type = 0;

    // Transform
    float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
    float rotation = 0.0f;

    // State
    float health = 100.0f;
    float energy = 100.0f;
    float animPhase = 0.0f;

    // Visual (for rendering)
    float colorR = 0.5f, colorG = 0.5f, colorB = 0.5f;
    float size = 1.0f;

    // Genome data (S-07 fix: include genetic information)
    float genomeSpeed = 1.0f;
    float genomeSize = 1.0f;
    float genomeVision = 50.0f;

    // Behavior tracking (Phase 10 Agent 7: for replay visualization)
    float age = 0.0f;           // Creature age for replay UI display
    int32_t generation = 1;     // Generation number for replay stats

    // Neural network weights (S-07 fix: include brain state)
    // Stored as compact vectors for serialization
    std::vector<float> neuralWeightsIH;  // Input->Hidden weights
    std::vector<float> neuralWeightsHO;  // Hidden->Output weights
    std::vector<float> neuralBiasH;      // Hidden biases
    std::vector<float> neuralBiasO;      // Output biases

    void write(BinaryWriter& writer) const {
        writer.write(id);
        writer.write(type);
        writer.write(posX);
        writer.write(posY);
        writer.write(posZ);
        writer.write(rotation);
        writer.write(health);
        writer.write(energy);
        writer.write(animPhase);
        writer.write(colorR);
        writer.write(colorG);
        writer.write(colorB);
        writer.write(size);

        // Write genome data
        writer.write(genomeSpeed);
        writer.write(genomeSize);
        writer.write(genomeVision);

        // Write behavior tracking (Phase 10 Agent 7)
        writer.write(age);
        writer.write(generation);

        // Write neural network weights (variable size)
        writer.write(static_cast<uint32_t>(neuralWeightsIH.size()));
        for (float w : neuralWeightsIH) writer.write(w);

        writer.write(static_cast<uint32_t>(neuralWeightsHO.size()));
        for (float w : neuralWeightsHO) writer.write(w);

        writer.write(static_cast<uint32_t>(neuralBiasH.size()));
        for (float b : neuralBiasH) writer.write(b);

        writer.write(static_cast<uint32_t>(neuralBiasO.size()));
        for (float b : neuralBiasO) writer.write(b);
    }

    void read(BinaryReader& reader) {
        id = reader.read<uint32_t>();
        type = reader.read<uint8_t>();
        posX = reader.read<float>();
        posY = reader.read<float>();
        posZ = reader.read<float>();
        rotation = reader.read<float>();
        health = reader.read<float>();
        energy = reader.read<float>();
        animPhase = reader.read<float>();
        colorR = reader.read<float>();
        colorG = reader.read<float>();
        colorB = reader.read<float>();
        size = reader.read<float>();

        // Read genome data
        genomeSpeed = reader.read<float>();
        genomeSize = reader.read<float>();
        genomeVision = reader.read<float>();

        // Read behavior tracking (Phase 10 Agent 7)
        age = reader.read<float>();
        generation = reader.read<int32_t>();

        // Read neural network weights
        uint32_t ihSize = reader.read<uint32_t>();
        neuralWeightsIH.resize(ihSize);
        for (uint32_t i = 0; i < ihSize; ++i) neuralWeightsIH[i] = reader.read<float>();

        uint32_t hoSize = reader.read<uint32_t>();
        neuralWeightsHO.resize(hoSize);
        for (uint32_t i = 0; i < hoSize; ++i) neuralWeightsHO[i] = reader.read<float>();

        uint32_t hSize = reader.read<uint32_t>();
        neuralBiasH.resize(hSize);
        for (uint32_t i = 0; i < hSize; ++i) neuralBiasH[i] = reader.read<float>();

        uint32_t oSize = reader.read<uint32_t>();
        neuralBiasO.resize(oSize);
        for (uint32_t i = 0; i < oSize; ++i) neuralBiasO[i] = reader.read<float>();
    }
};

// Snapshot of food at a point in time
struct FoodSnapshot {
    float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
    float energy = 20.0f;
    bool active = true;

    void write(BinaryWriter& writer) const {
        writer.write(posX);
        writer.write(posY);
        writer.write(posZ);
        writer.write(energy);
        writer.writeBool(active);
    }

    void read(BinaryReader& reader) {
        posX = reader.read<float>();
        posY = reader.read<float>();
        posZ = reader.read<float>();
        energy = reader.read<float>();
        active = reader.readBool();
    }
};

// Camera state for replay
struct CameraSnapshot {
    float posX = 0.0f, posY = 50.0f, posZ = 100.0f;
    float targetX = 0.0f, targetY = 0.0f, targetZ = 0.0f;
    float fov = 60.0f;

    void write(BinaryWriter& writer) const {
        writer.write(posX);
        writer.write(posY);
        writer.write(posZ);
        writer.write(targetX);
        writer.write(targetY);
        writer.write(targetZ);
        writer.write(fov);
    }

    void read(BinaryReader& reader) {
        posX = reader.read<float>();
        posY = reader.read<float>();
        posZ = reader.read<float>();
        targetX = reader.read<float>();
        targetY = reader.read<float>();
        targetZ = reader.read<float>();
        fov = reader.read<float>();
    }
};

// Statistics snapshot for UI display during replay
struct StatisticsSnapshot {
    uint32_t herbivoreCount = 0;
    uint32_t carnivoreCount = 0;
    uint32_t foodCount = 0;
    uint32_t generation = 0;
    float avgHerbivoreFitness = 0.0f;
    float avgCarnivoreFitness = 0.0f;

    void write(BinaryWriter& writer) const {
        writer.write(herbivoreCount);
        writer.write(carnivoreCount);
        writer.write(foodCount);
        writer.write(generation);
        writer.write(avgHerbivoreFitness);
        writer.write(avgCarnivoreFitness);
    }

    void read(BinaryReader& reader) {
        herbivoreCount = reader.read<uint32_t>();
        carnivoreCount = reader.read<uint32_t>();
        foodCount = reader.read<uint32_t>();
        generation = reader.read<uint32_t>();
        avgHerbivoreFitness = reader.read<float>();
        avgCarnivoreFitness = reader.read<float>();
    }
};

// A single frame in the replay
struct ReplayFrame {
    float timestamp = 0.0f;  // Simulation time when frame was captured
    std::vector<CreatureSnapshot> creatures;
    std::vector<FoodSnapshot> food;
    CameraSnapshot camera;
    StatisticsSnapshot stats;

    void write(BinaryWriter& writer) const {
        writer.write(timestamp);

        // Write creatures
        writer.write(static_cast<uint32_t>(creatures.size()));
        for (const auto& c : creatures) {
            c.write(writer);
        }

        // Write food
        writer.write(static_cast<uint32_t>(food.size()));
        for (const auto& f : food) {
            f.write(writer);
        }

        // Write camera and stats
        camera.write(writer);
        stats.write(writer);
    }

    void read(BinaryReader& reader) {
        timestamp = reader.read<float>();

        // Read creatures
        uint32_t creatureCount = reader.read<uint32_t>();
        creatures.resize(creatureCount);
        for (uint32_t i = 0; i < creatureCount; ++i) {
            creatures[i].read(reader);
        }

        // Read food
        uint32_t foodCount = reader.read<uint32_t>();
        food.resize(foodCount);
        for (uint32_t i = 0; i < foodCount; ++i) {
            food[i].read(reader);
        }

        // Read camera and stats
        camera.read(reader);
        stats.read(reader);
    }
};

// Replay file header
struct ReplayHeader {
    uint32_t magic = 0x52504C59;  // "RPLY"
    uint32_t version = 1;
    uint64_t timestamp = 0;       // When recording started
    uint32_t terrainSeed = 0;
    uint32_t frameCount = 0;
    float duration = 0.0f;        // Total replay duration
    float recordInterval = 1.0f;  // Seconds between frames

    void write(BinaryWriter& writer) const {
        writer.write(magic);
        writer.write(version);
        writer.write(timestamp);
        writer.write(terrainSeed);
        writer.write(frameCount);
        writer.write(duration);
        writer.write(recordInterval);
    }

    bool read(BinaryReader& reader) {
        magic = reader.read<uint32_t>();
        if (magic != 0x52504C59) return false;
        version = reader.read<uint32_t>();
        timestamp = reader.read<uint64_t>();
        terrainSeed = reader.read<uint32_t>();
        frameCount = reader.read<uint32_t>();
        duration = reader.read<float>();
        recordInterval = reader.read<float>();
        return true;
    }
};

// ============================================================================
// Replay Recorder
// ============================================================================

class ReplayRecorder {
public:
    ReplayRecorder() = default;

    // Configuration
    void setRecordInterval(float seconds) { m_recordInterval = seconds; }
    float getRecordInterval() const { return m_recordInterval; }
    void setMaxFrames(size_t maxFrames) {
        m_maxFrames = maxFrames;
        // Pre-allocate buffer for ring buffer efficiency
        m_frames.reserve(maxFrames);
    }

    // Recording control
    void startRecording(uint32_t terrainSeed);
    void stopRecording();
    bool isRecording() const { return m_isRecording; }

    // Call every frame during recording
    void update(float dt, float simulationTime);

    // Record a frame (called automatically based on interval)
    void recordFrame(const ReplayFrame& frame);

    // Manual frame recording (bypasses interval check)
    void forceRecordFrame(const ReplayFrame& frame);

    // Save recorded replay to file
    bool saveReplay(const std::string& filename);

    // Get recording info
    size_t getFrameCount() const { return m_frameCount; }
    float getDuration() const;

    // Clear all recorded data
    void clear();

    // Access frames for transfer to player (S-08: ring buffer aware)
    const ReplayFrame& getFrame(size_t index) const;
    const std::vector<ReplayFrame>& getFrames() const { return m_frames; }
    size_t getStartIndex() const { return m_startIndex; }

private:
    bool m_isRecording = false;
    float m_recordInterval = 1.0f;  // Record every second
    float m_timeSinceLastRecord = 0.0f;
    size_t m_maxFrames = 36000;  // 10 hours at 1fps

    uint32_t m_terrainSeed = 0;
    uint64_t m_startTimestamp = 0;

    // S-08 FIX: Ring buffer implementation for O(1) frame insertion
    std::vector<ReplayFrame> m_frames;
    size_t m_writeIndex = 0;   // Next position to write
    size_t m_startIndex = 0;   // Oldest frame position (for reading in order)
    size_t m_frameCount = 0;   // Actual number of frames stored
};

// ============================================================================
// Replay Player
// ============================================================================

class ReplayPlayer {
public:
    ReplayPlayer() = default;

    // Load replay from file
    bool loadReplay(const std::string& filename);

    // Load replay directly from recorder (for immediate playback)
    void loadFromRecorder(const ReplayRecorder& recorder);

    // Unload current replay
    void unloadReplay();
    bool hasReplay() const { return !m_frames.empty(); }

    // Playback control
    void play();
    void pause();
    void stop();
    void togglePause() { m_paused ? play() : pause(); }

    bool isPlaying() const { return m_isPlaying && !m_paused; }
    bool isPaused() const { return m_paused; }
    bool isStopped() const { return !m_isPlaying; }

    // Speed control
    void setSpeed(float speed) { m_playbackSpeed = std::clamp(speed, 0.1f, 10.0f); }
    float getSpeed() const { return m_playbackSpeed; }

    // Seeking
    void seek(float time);
    void seekToFrame(size_t frameIndex);
    void seekPercent(float percent);

    // Step controls
    void stepForward();
    void stepBackward();

    // Update playback (call every frame)
    void update(float dt);

    // Get current state
    float getCurrentTime() const { return m_playbackTime; }
    float getDuration() const { return m_header.duration; }
    float getProgress() const {
        return m_header.duration > 0 ? m_playbackTime / m_header.duration : 0.0f;
    }
    size_t getCurrentFrameIndex() const { return m_currentFrame; }
    size_t getTotalFrames() const { return m_frames.size(); }

    // Get interpolated frame at current playback time
    ReplayFrame getInterpolatedFrame() const;

    // Get current frame without interpolation
    const ReplayFrame& getCurrentFrame() const;

    // Get replay metadata
    const ReplayHeader& getHeader() const { return m_header; }
    uint32_t getTerrainSeed() const { return m_header.terrainSeed; }

private:
    ReplayHeader m_header;
    std::vector<ReplayFrame> m_frames;

    bool m_isPlaying = false;
    bool m_paused = false;
    float m_playbackTime = 0.0f;
    float m_playbackSpeed = 1.0f;
    size_t m_currentFrame = 0;

    // Find frame indices for interpolation
    void findFrameIndices(float time, size_t& prevIndex, size_t& nextIndex, float& t) const;

    // Interpolate between two creature snapshots
    static CreatureSnapshot interpolateCreature(const CreatureSnapshot& a,
                                                 const CreatureSnapshot& b,
                                                 float t);

    // Interpolate between two camera snapshots
    static CameraSnapshot interpolateCamera(const CameraSnapshot& a,
                                            const CameraSnapshot& b,
                                            float t);
};

// ============================================================================
// ReplayRecorder Implementation
// ============================================================================

inline void ReplayRecorder::startRecording(uint32_t terrainSeed) {
    clear();
    m_isRecording = true;
    m_terrainSeed = terrainSeed;
    m_timeSinceLastRecord = 0.0f;
    m_startTimestamp = static_cast<uint64_t>(
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())
    );
}

inline void ReplayRecorder::stopRecording() {
    m_isRecording = false;
}

inline void ReplayRecorder::update(float dt, float simulationTime) {
    if (!m_isRecording) return;

    m_timeSinceLastRecord += dt;
}

inline void ReplayRecorder::recordFrame(const ReplayFrame& frame) {
    if (!m_isRecording) return;

    // Check if enough time has passed
    if (m_timeSinceLastRecord < m_recordInterval && m_frameCount > 0) {
        return;
    }

    forceRecordFrame(frame);
    m_timeSinceLastRecord = 0.0f;
}

// S-08 FIX: O(1) ring buffer insertion instead of O(n) vector erase
inline void ReplayRecorder::forceRecordFrame(const ReplayFrame& frame) {
    if (m_frames.size() < m_maxFrames) {
        // Buffer not yet full - just append
        m_frames.push_back(frame);
        m_frameCount = m_frames.size();
    } else {
        // Buffer full - overwrite oldest frame (ring buffer behavior)
        m_frames[m_writeIndex] = frame;
        // Move start index to next oldest frame
        m_startIndex = (m_writeIndex + 1) % m_maxFrames;
    }
    // Advance write position
    m_writeIndex = (m_writeIndex + 1) % m_maxFrames;
}

inline float ReplayRecorder::getDuration() const {
    if (m_frameCount == 0) return 0.0f;
    // Get the last recorded frame's timestamp
    size_t lastIndex = (m_writeIndex == 0) ? m_frames.size() - 1 : m_writeIndex - 1;
    return m_frames[lastIndex].timestamp;
}

// S-08: Get frame by logical index (0 = oldest frame)
inline const ReplayFrame& ReplayRecorder::getFrame(size_t index) const {
    static ReplayFrame emptyFrame;
    if (index >= m_frameCount) return emptyFrame;

    // Map logical index to physical index in ring buffer
    size_t physicalIndex = (m_startIndex + index) % m_frames.size();
    return m_frames[physicalIndex];
}

inline bool ReplayRecorder::saveReplay(const std::string& filename) {
    if (m_frameCount == 0) return false;

    BinaryWriter writer;
    if (!writer.open(filename)) return false;

    try {
        // Create and write header
        ReplayHeader header;
        header.timestamp = m_startTimestamp;
        header.terrainSeed = m_terrainSeed;
        header.frameCount = static_cast<uint32_t>(m_frameCount);
        header.duration = getDuration();
        header.recordInterval = m_recordInterval;
        header.write(writer);

        // Write all frames in chronological order (ring buffer aware)
        for (size_t i = 0; i < m_frameCount; ++i) {
            getFrame(i).write(writer);
        }

        writer.close();
        return true;
    } catch (...) {
        return false;
    }
}

inline void ReplayRecorder::clear() {
    m_frames.clear();
    m_isRecording = false;
    m_timeSinceLastRecord = 0.0f;
    m_writeIndex = 0;
    m_startIndex = 0;
    m_frameCount = 0;
}

// ============================================================================
// ReplayPlayer Implementation
// ============================================================================

inline bool ReplayPlayer::loadReplay(const std::string& filename) {
    unloadReplay();

    BinaryReader reader;
    if (!reader.open(filename)) return false;

    try {
        // Read header
        if (!m_header.read(reader)) return false;

        // Read all frames
        m_frames.resize(m_header.frameCount);
        for (uint32_t i = 0; i < m_header.frameCount; ++i) {
            m_frames[i].read(reader);
        }

        reader.close();
        return true;
    } catch (...) {
        unloadReplay();
        return false;
    }
}

inline void ReplayPlayer::loadFromRecorder(const ReplayRecorder& recorder) {
    unloadReplay();

    size_t frameCount = recorder.getFrameCount();
    if (frameCount == 0) return;

    // Copy frames from recorder in chronological order
    m_frames.reserve(frameCount);
    for (size_t i = 0; i < frameCount; ++i) {
        m_frames.push_back(recorder.getFrame(i));
    }

    // Build header from recorder state
    m_header.frameCount = static_cast<uint32_t>(frameCount);
    m_header.duration = recorder.getDuration();
    m_header.recordInterval = recorder.getRecordInterval();
}

inline void ReplayPlayer::unloadReplay() {
    m_frames.clear();
    m_header = ReplayHeader();
    m_isPlaying = false;
    m_paused = false;
    m_playbackTime = 0.0f;
    m_currentFrame = 0;
}

inline void ReplayPlayer::play() {
    if (m_frames.empty()) return;
    m_isPlaying = true;
    m_paused = false;
}

inline void ReplayPlayer::pause() {
    m_paused = true;
}

inline void ReplayPlayer::stop() {
    m_isPlaying = false;
    m_paused = false;
    m_playbackTime = 0.0f;
    m_currentFrame = 0;
}

inline void ReplayPlayer::seek(float time) {
    m_playbackTime = std::clamp(time, 0.0f, m_header.duration);

    // Update current frame index
    for (size_t i = 0; i < m_frames.size(); ++i) {
        if (m_frames[i].timestamp > m_playbackTime) {
            m_currentFrame = i > 0 ? i - 1 : 0;
            return;
        }
    }
    m_currentFrame = m_frames.empty() ? 0 : m_frames.size() - 1;
}

inline void ReplayPlayer::seekToFrame(size_t frameIndex) {
    if (m_frames.empty()) return;
    m_currentFrame = std::min(frameIndex, m_frames.size() - 1);
    m_playbackTime = m_frames[m_currentFrame].timestamp;
}

inline void ReplayPlayer::seekPercent(float percent) {
    seek(std::clamp(percent, 0.0f, 1.0f) * m_header.duration);
}

inline void ReplayPlayer::stepForward() {
    if (m_currentFrame < m_frames.size() - 1) {
        ++m_currentFrame;
        m_playbackTime = m_frames[m_currentFrame].timestamp;
    }
}

inline void ReplayPlayer::stepBackward() {
    if (m_currentFrame > 0) {
        --m_currentFrame;
        m_playbackTime = m_frames[m_currentFrame].timestamp;
    }
}

inline void ReplayPlayer::update(float dt) {
    if (!m_isPlaying || m_paused || m_frames.empty()) return;

    m_playbackTime += dt * m_playbackSpeed;

    // Clamp and check for end
    if (m_playbackTime >= m_header.duration) {
        m_playbackTime = m_header.duration;
        m_isPlaying = false;
        m_currentFrame = m_frames.size() - 1;
        return;
    }

    // Update current frame index
    while (m_currentFrame < m_frames.size() - 1 &&
           m_frames[m_currentFrame + 1].timestamp <= m_playbackTime) {
        ++m_currentFrame;
    }
}

inline const ReplayFrame& ReplayPlayer::getCurrentFrame() const {
    static ReplayFrame emptyFrame;
    if (m_frames.empty()) return emptyFrame;
    return m_frames[m_currentFrame];
}

inline void ReplayPlayer::findFrameIndices(float time, size_t& prevIndex,
                                            size_t& nextIndex, float& t) const {
    if (m_frames.empty()) {
        prevIndex = nextIndex = 0;
        t = 0.0f;
        return;
    }

    prevIndex = 0;
    for (size_t i = 0; i < m_frames.size(); ++i) {
        if (m_frames[i].timestamp > time) {
            prevIndex = i > 0 ? i - 1 : 0;
            nextIndex = i;

            float prevTime = m_frames[prevIndex].timestamp;
            float nextTime = m_frames[nextIndex].timestamp;
            float duration = nextTime - prevTime;
            t = duration > 0 ? (time - prevTime) / duration : 0.0f;
            return;
        }
    }

    // Past the end
    prevIndex = nextIndex = m_frames.size() - 1;
    t = 0.0f;
}

inline CreatureSnapshot ReplayPlayer::interpolateCreature(const CreatureSnapshot& a,
                                                           const CreatureSnapshot& b,
                                                           float t) {
    CreatureSnapshot result;
    result.id = a.id;
    result.type = a.type;

    // Interpolate position
    result.posX = a.posX + (b.posX - a.posX) * t;
    result.posY = a.posY + (b.posY - a.posY) * t;
    result.posZ = a.posZ + (b.posZ - a.posZ) * t;

    // Interpolate rotation (shortest path)
    float angleDiff = b.rotation - a.rotation;
    while (angleDiff > 3.14159f) angleDiff -= 6.28318f;
    while (angleDiff < -3.14159f) angleDiff += 6.28318f;
    result.rotation = a.rotation + angleDiff * t;

    // Interpolate state
    result.health = a.health + (b.health - a.health) * t;
    result.energy = a.energy + (b.energy - a.energy) * t;
    result.animPhase = a.animPhase + (b.animPhase - a.animPhase) * t;

    // Keep visual properties from start frame
    result.colorR = a.colorR;
    result.colorG = a.colorG;
    result.colorB = a.colorB;
    result.size = a.size;

    return result;
}

inline CameraSnapshot ReplayPlayer::interpolateCamera(const CameraSnapshot& a,
                                                       const CameraSnapshot& b,
                                                       float t) {
    CameraSnapshot result;
    result.posX = a.posX + (b.posX - a.posX) * t;
    result.posY = a.posY + (b.posY - a.posY) * t;
    result.posZ = a.posZ + (b.posZ - a.posZ) * t;
    result.targetX = a.targetX + (b.targetX - a.targetX) * t;
    result.targetY = a.targetY + (b.targetY - a.targetY) * t;
    result.targetZ = a.targetZ + (b.targetZ - a.targetZ) * t;
    result.fov = a.fov + (b.fov - a.fov) * t;
    return result;
}

inline ReplayFrame ReplayPlayer::getInterpolatedFrame() const {
    if (m_frames.empty()) {
        return ReplayFrame();
    }

    size_t prevIdx, nextIdx;
    float t;
    findFrameIndices(m_playbackTime, prevIdx, nextIdx, t);

    const ReplayFrame& prevFrame = m_frames[prevIdx];
    const ReplayFrame& nextFrame = m_frames[nextIdx];

    ReplayFrame result;
    result.timestamp = m_playbackTime;

    // Interpolate camera
    result.camera = interpolateCamera(prevFrame.camera, nextFrame.camera, t);

    // Use stats from current frame (no interpolation needed)
    result.stats = prevFrame.stats;

    // Interpolate creatures (match by ID)
    for (const auto& prevCreature : prevFrame.creatures) {
        // Find matching creature in next frame
        const CreatureSnapshot* nextCreature = nullptr;
        for (const auto& nc : nextFrame.creatures) {
            if (nc.id == prevCreature.id) {
                nextCreature = &nc;
                break;
            }
        }

        if (nextCreature) {
            result.creatures.push_back(interpolateCreature(prevCreature, *nextCreature, t));
        } else {
            // Creature died between frames, fade out
            CreatureSnapshot fadingCreature = prevCreature;
            fadingCreature.energy *= (1.0f - t);  // Fade out
            result.creatures.push_back(fadingCreature);
        }
    }

    // Handle newly spawned creatures in next frame
    for (const auto& nextCreature : nextFrame.creatures) {
        bool found = false;
        for (const auto& c : result.creatures) {
            if (c.id == nextCreature.id) {
                found = true;
                break;
            }
        }
        if (!found) {
            // New creature, fade in
            CreatureSnapshot newCreature = nextCreature;
            newCreature.energy *= t;  // Fade in
            result.creatures.push_back(newCreature);
        }
    }

    // Food - just use current frame (no interpolation for food)
    result.food = prevFrame.food;

    return result;
}

} // namespace Forge
