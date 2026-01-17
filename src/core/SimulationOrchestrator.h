#pragma once

// Lightweight SimulationOrchestrator adapter for UI systems.
// Provides time control hooks and access to core simulation pointers.

#include <functional>
#include <cstdint>

class WeatherSystem;

namespace Forge {

class CreatureManager;
class CameraController;
class Terrain;

enum class SimulationState {
    RUNNING,
    PAUSED
};

struct SimulationStats {
    int dayCount = 0;
    int totalCreatures = 0;
    int maxGeneration = 0;
    float simulationTime = 0.0f;
};

class SimulationOrchestrator {
public:
    // Bind pointers to core simulation state (owned elsewhere)
    void bindTimeState(bool* paused, float* timeScale, float* simTime) {
        m_paused = paused;
        m_timeScale = timeScale;
        m_simTime = simTime;
    }

    void bindDayCount(int* dayCount) { m_dayCount = dayCount; }
    void bindMaxGeneration(uint32_t* maxGen) { m_maxGeneration = maxGen; }

    void setCreatureManager(CreatureManager* manager) { m_creatureManager = manager; }
    void setTerrain(Terrain* terrain) { m_terrain = terrain; }
    void setWeather(::WeatherSystem* weather) { m_weather = weather; }
    void setCameraController(CameraController* controller) { m_cameraController = controller; }

    void setStepFramesCallback(std::function<void(int)> cb) { m_stepFrames = std::move(cb); }
    void setSkipGenerationsCallback(std::function<void(int)> cb) { m_skipGenerations = std::move(cb); }
    void setSkipToGenerationCallback(std::function<void(int)> cb) { m_skipToGeneration = std::move(cb); }

    // Time controls
    void pause() { if (m_paused) *m_paused = true; }
    void resume() { if (m_paused) *m_paused = false; }

    void setTimeScale(float scale) {
        if (m_timeScale) {
            *m_timeScale = scale;
        }
    }

    float getTimeScale() const {
        return m_timeScale ? *m_timeScale : 1.0f;
    }

    SimulationState getState() const {
        return (m_paused && *m_paused) ? SimulationState::PAUSED : SimulationState::RUNNING;
    }

    void stepFrame() { stepFrames(1); }

    void stepFrames(int count) {
        if (m_stepFrames) {
            m_stepFrames(count);
        }
    }

    void skipGenerations(int count) {
        if (m_skipGenerations) {
            m_skipGenerations(count);
        }
    }

    void skipToGeneration(int target) {
        if (m_skipToGeneration) {
            m_skipToGeneration(target);
        }
    }

    float getSimulationTime() const {
        return m_simTime ? *m_simTime : 0.0f;
    }

    int getCurrentDay() const {
        return m_dayCount ? *m_dayCount : 0;
    }

    int getMaxGeneration() const {
        return m_maxGeneration ? static_cast<int>(*m_maxGeneration) : 0;
    }

    // Accessors for tools
    CreatureManager* getCreatureManager() const { return m_creatureManager; }
    Terrain* getTerrain() const { return m_terrain; }
    ::WeatherSystem* getWeather() const { return m_weather; }
    CameraController* getCameraController() const { return m_cameraController; }

    const SimulationStats& getStats() const { return m_stats; }
    void updateStats(const SimulationStats& stats) { m_stats = stats; }

private:
    bool* m_paused = nullptr;
    float* m_timeScale = nullptr;
    float* m_simTime = nullptr;
    int* m_dayCount = nullptr;
    uint32_t* m_maxGeneration = nullptr;

    CreatureManager* m_creatureManager = nullptr;
    Terrain* m_terrain = nullptr;
    ::WeatherSystem* m_weather = nullptr;
    CameraController* m_cameraController = nullptr;

    SimulationStats m_stats;

    std::function<void(int)> m_stepFrames;
    std::function<void(int)> m_skipGenerations;
    std::function<void(int)> m_skipToGeneration;
};

} // namespace Forge
