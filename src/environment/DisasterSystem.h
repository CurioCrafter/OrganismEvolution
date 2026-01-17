#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <random>

// Forward declarations
namespace Forge {
    class SimulationOrchestrator;
    class CreatureManager;
}

class VegetationManager;
class Terrain;
class ClimateSystem;
class WeatherSystem;
class SpatialGrid;

using Forge::SimulationOrchestrator;
using Forge::CreatureManager;

namespace disasters {
    class VolcanoDisaster;
    class MeteorImpact;
    class DiseaseOutbreak;
    class IceAge;
    class Drought;
    class Flood;
}

/**
 * @brief Types of catastrophic events that can occur in the simulation
 */
enum class DisasterType {
    VOLCANIC_ERUPTION,
    METEOR_IMPACT,
    DISEASE_OUTBREAK,
    ICE_AGE,
    DROUGHT,
    FLOOD,
    INVASIVE_SPECIES,
    COUNT
};

/**
 * @brief Severity levels for disasters
 */
enum class DisasterSeverity {
    MINOR,      // Affects small area, low mortality
    MODERATE,   // Medium area, moderate mortality
    MAJOR,      // Large area, high mortality
    CATASTROPHIC // Mass extinction event
};

/**
 * @brief Represents an active disaster in the simulation
 */
struct ActiveDisaster {
    DisasterType type;
    DisasterSeverity severity;
    glm::vec3 epicenter;
    float radius;
    float progress;         // 0.0 to 1.0
    float duration;         // Total duration in seconds
    float elapsedTime;      // Time since start
    int creaturesAffected;
    int creaturesKilled;
    int vegetationDestroyed;
    bool isActive;
    std::string description;

    float getTimeRemaining() const { return duration - elapsedTime; }
    float getProgressPercent() const { return progress * 100.0f; }
};

/**
 * @brief Historical record of a disaster event
 */
struct DisasterRecord {
    DisasterType type;
    DisasterSeverity severity;
    glm::vec3 epicenter;
    float simulationDay;
    int totalKilled;
    int totalAffected;
    float duration;
    std::string summary;
};

/**
 * @brief Callback signature for disaster events
 */
using DisasterEventCallback = std::function<void(const ActiveDisaster&)>;
using DisasterEndCallback = std::function<void(const DisasterRecord&)>;

/**
 * @brief Central disaster management system
 *
 * Coordinates catastrophic events including volcanic eruptions, meteor impacts,
 * disease outbreaks, ice ages, droughts, and floods. Designed to create dramatic
 * evolutionary bottlenecks while ensuring recovery is always possible.
 *
 * DESIGN PRINCIPLES:
 * - Gradual damage over time, never instant kills
 * - Always leave survivors for population recovery
 * - Clear visual feedback and progression
 * - Educational about evolutionary pressure
 */
class DisasterSystem {
public:
    DisasterSystem();
    ~DisasterSystem();

    // Non-copyable
    DisasterSystem(const DisasterSystem&) = delete;
    DisasterSystem& operator=(const DisasterSystem&) = delete;

    /**
     * @brief Initialize the disaster system
     * @param seed Random seed for disaster generation
     */
    void init(unsigned int seed);

    /**
     * @brief Main update function - call every frame
     * @param deltaTime Time since last frame
     * @param sim Reference to simulation orchestrator for system access
     */
    void update(float deltaTime, SimulationOrchestrator& sim);

    // === Disaster Triggering ===

    /**
     * @brief Manually trigger a disaster at a specific location
     * @param type Type of disaster to trigger
     * @param epicenter World position of disaster center
     * @param severity How severe the disaster should be
     */
    void triggerDisaster(DisasterType type, const glm::vec3& epicenter,
                         DisasterSeverity severity = DisasterSeverity::MODERATE);

    /**
     * @brief Trigger a random disaster somewhere in the world
     */
    void triggerRandomDisaster();

    /**
     * @brief Attempt to trigger a natural disaster based on probability
     * @return True if a disaster was triggered
     */
    bool attemptNaturalDisaster();

    // === Configuration ===

    /**
     * @brief Set the base probability of random disasters per simulation day
     * @param probability Value between 0.0 and 1.0
     */
    void setDisasterProbability(float probability);
    float getDisasterProbability() const { return m_disasterProbability; }

    /**
     * @brief Enable or disable automatic random disasters
     */
    void setRandomDisastersEnabled(bool enabled) { m_randomDisastersEnabled = enabled; }
    bool areRandomDisastersEnabled() const { return m_randomDisastersEnabled; }

    /**
     * @brief Set minimum time between random disasters
     */
    void setMinDisasterCooldown(float seconds) { m_minDisasterCooldown = seconds; }
    float getMinDisasterCooldown() const { return m_minDisasterCooldown; }

    /**
     * @brief Set maximum concurrent disasters allowed
     */
    void setMaxConcurrentDisasters(int max) { m_maxConcurrentDisasters = max; }
    int getMaxConcurrentDisasters() const { return m_maxConcurrentDisasters; }

    // === Queries ===

    /**
     * @brief Get all currently active disasters
     */
    const std::vector<ActiveDisaster>& getActiveDisasters() const { return m_activeDisasters; }

    /**
     * @brief Check if any disasters are currently active
     */
    bool hasActiveDisasters() const;

    /**
     * @brief Get count of active disasters
     */
    int getActiveDisasterCount() const;

    /**
     * @brief Check if a specific disaster type is active
     */
    bool isDisasterTypeActive(DisasterType type) const;

    /**
     * @brief Get historical disaster records
     */
    const std::vector<DisasterRecord>& getDisasterHistory() const { return m_disasterHistory; }

    /**
     * @brief Get total deaths across all historical disasters
     */
    int getTotalHistoricalDeaths() const;

    /**
     * @brief Check if a position is within any active disaster zone
     * @param position World position to check
     * @return Danger level 0.0 to 1.0
     */
    float getDangerLevel(const glm::vec3& position) const;

    // === Callbacks ===

    /**
     * @brief Set callback for when disasters start
     */
    void setOnDisasterStart(DisasterEventCallback callback) { m_onDisasterStart = callback; }

    /**
     * @brief Set callback for when disasters end
     */
    void setOnDisasterEnd(DisasterEndCallback callback) { m_onDisasterEnd = callback; }

    // === Utility ===

    /**
     * @brief Get human-readable name for a disaster type
     */
    static const char* getDisasterTypeName(DisasterType type);

    /**
     * @brief Get description for a disaster type
     */
    static const char* getDisasterTypeDescription(DisasterType type);

    /**
     * @brief Get severity name
     */
    static const char* getSeverityName(DisasterSeverity severity);

    /**
     * @brief Force end all active disasters (for debugging/testing)
     */
    void endAllDisasters();

private:
    // === Internal Methods ===
    void updateActiveDisasters(float deltaTime, SimulationOrchestrator& sim);
    void checkNaturalDisasters(float deltaTime);
    void cleanupFinishedDisasters();
    void recordDisaster(const ActiveDisaster& disaster);

    glm::vec3 findRandomEpicenter(DisasterType type, const Terrain& terrain) const;
    float getBaseDuration(DisasterType type, DisasterSeverity severity) const;
    float getBaseRadius(DisasterType type, DisasterSeverity severity) const;

    // === Individual Disaster Handlers ===
    std::unique_ptr<disasters::VolcanoDisaster> m_volcano;
    std::unique_ptr<disasters::MeteorImpact> m_meteorImpact;
    std::unique_ptr<disasters::DiseaseOutbreak> m_disease;
    std::unique_ptr<disasters::IceAge> m_iceAge;
    std::unique_ptr<disasters::Drought> m_drought;
    std::unique_ptr<disasters::Flood> m_flood;

    // === State ===
    std::vector<ActiveDisaster> m_activeDisasters;
    std::vector<DisasterRecord> m_disasterHistory;

    // === Configuration ===
    float m_disasterProbability = 0.001f;   // Per-day probability
    bool m_randomDisastersEnabled = true;
    float m_minDisasterCooldown = 60.0f;    // Minimum seconds between random disasters
    int m_maxConcurrentDisasters = 2;       // Max simultaneous disasters

    // === Timing ===
    float m_timeSinceLastDisaster = 0.0f;
    float m_dayAccumulator = 0.0f;

    // === Random Generation ===
    mutable std::mt19937 m_rng;

    // === Callbacks ===
    DisasterEventCallback m_onDisasterStart;
    DisasterEndCallback m_onDisasterEnd;
};
