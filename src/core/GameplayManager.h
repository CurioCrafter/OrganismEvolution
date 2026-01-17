#pragma once

/**
 * @file GameplayManager.h
 * @brief Central manager for gameplay features and fun factor systems
 *
 * Provides:
 * - Time controls (pause, slow-mo, fast-forward)
 * - Creature statistics tracking (births/deaths per minute, records)
 * - Achievements/milestones system
 * - Creature highlighting (predators, prey, hungry, reproducing)
 * - Event notifications with scrolling log
 * - Spotlight feature for interesting creatures
 * - Ambient environmental events (seasons, golden age, drought)
 */

#include "../entities/Creature.h"
#include "../entities/CreatureType.h"
#include <glm/glm.hpp>
#include <vector>
#include <deque>
#include <string>
#include <functional>
#include <map>
#include <set>
#include <cstdint>

namespace Forge {

class CreatureManager;
class SimulationOrchestrator;

// ============================================================================
// Time Control
// ============================================================================

struct TimeControl {
    float timeScale = 1.0f;          // Current time scale
    bool paused = false;              // Is simulation paused
    float targetTimeScale = 1.0f;    // Target for smooth transitions
    float transitionSpeed = 5.0f;    // How fast to reach target

    // Preset speeds
    static constexpr float SPEED_QUARTER = 0.25f;
    static constexpr float SPEED_HALF = 0.5f;
    static constexpr float SPEED_NORMAL = 1.0f;
    static constexpr float SPEED_DOUBLE = 2.0f;
    static constexpr float SPEED_QUAD = 4.0f;
    static constexpr float SPEED_OCTO = 8.0f;

    void setSpeed(float speed) {
        targetTimeScale = glm::clamp(speed, 0.1f, 10.0f);
    }

    void smoothUpdate(float dt) {
        if (timeScale != targetTimeScale) {
            float diff = targetTimeScale - timeScale;
            float change = transitionSpeed * dt;
            if (std::abs(diff) < change) {
                timeScale = targetTimeScale;
            } else {
                timeScale += (diff > 0 ? 1 : -1) * change;
            }
        }
    }
};

// ============================================================================
// Statistics Tracking
// ============================================================================

struct CreatureRecord {
    int creatureId = -1;
    std::string name;
    float value = 0.0f;
    float timestamp = 0.0f;
    genetics::SpeciesId speciesId = 0;
};

struct LiveStatistics {
    // Per-minute tracking
    int birthsThisMinute = 0;
    int deathsThisMinute = 0;
    float minuteTimer = 0.0f;

    // Rolling history (last 10 minutes)
    std::deque<int> birthsHistory;
    std::deque<int> deathsHistory;
    static constexpr int MAX_MINUTES = 10;

    // Records
    CreatureRecord oldestEver;
    CreatureRecord fastestEver;
    CreatureRecord largestEver;
    CreatureRecord mostOffspringEver;
    int peakPopulation = 0;
    float peakPopulationTime = 0.0f;
    int longestLineage = 0;  // Generations survived

    // Current records
    CreatureRecord currentOldest;
    CreatureRecord currentFastest;
    CreatureRecord currentLargest;
    CreatureRecord currentMostOffspring;

    // Species tracking
    std::map<genetics::SpeciesId, int> speciesOffspring;  // Total offspring per species
    genetics::SpeciesId mostSuccessfulSpecies = 0;
    int mostSuccessfulSpeciesOffspring = 0;

    void rollMinute() {
        birthsHistory.push_back(birthsThisMinute);
        deathsHistory.push_back(deathsThisMinute);
        if (birthsHistory.size() > MAX_MINUTES) {
            birthsHistory.pop_front();
            deathsHistory.pop_front();
        }
        birthsThisMinute = 0;
        deathsThisMinute = 0;
        minuteTimer = 0.0f;
    }

    float getAverageBirthsPerMinute() const {
        if (birthsHistory.empty()) return 0.0f;
        float sum = 0.0f;
        for (int b : birthsHistory) sum += b;
        return sum / birthsHistory.size();
    }

    float getAverageDeathsPerMinute() const {
        if (deathsHistory.empty()) return 0.0f;
        float sum = 0.0f;
        for (int d : deathsHistory) sum += d;
        return sum / deathsHistory.size();
    }
};

// ============================================================================
// Achievements System
// ============================================================================

enum class AchievementType {
    // Predation
    FIRST_PREDATION,            // First hunting kill
    APEX_PREDATOR,              // 100 kills by one creature
    HUNTING_DYNASTY,            // One predator lineage survives 10 generations

    // Population
    POPULATION_BOOM,            // Reach 500 creatures
    POPULATION_EXPLOSION,       // Reach 1000 creatures
    MASS_EXTINCTION,            // Population drops below 50

    // Evolution
    BIODIVERSITY,               // 20+ unique species active
    ANCIENT_LINEAGE,            // A species survives 50 generations
    RAPID_SPECIATION,           // 5 new species in one minute

    // Survival
    ELDER,                      // Creature survives to age 100
    CENTENARIAN,                // Creature survives to age 200
    SURVIVOR,                   // Population recovers after dropping below 100

    // Ecosystem
    ECOSYSTEM_BALANCE,          // Maintain 40-60% predator/prey ratio for 5 minutes
    GOLDEN_AGE,                 // 90%+ creatures above 50% energy for 2 minutes
    GREAT_FAMINE,               // 80%+ creatures below 30% energy

    // Milestones
    FIRST_GENERATION,           // First generation born
    TEN_GENERATIONS,            // Reach 10 generations
    HUNDRED_GENERATIONS,        // Reach 100 generations
    FIRST_SPECIES,              // First speciation event
    AQUATIC_DOMINANCE,          // 50%+ creatures are aquatic
    AERIAL_DOMINANCE,           // 50%+ creatures are flying

    COUNT
};

struct Achievement {
    AchievementType type;
    std::string name;
    std::string description;
    bool unlocked = false;
    float unlockTime = 0.0f;
    bool notified = false;
};

// ============================================================================
// Creature Highlighting
// ============================================================================

enum class HighlightMode {
    NONE,
    PREDATORS,          // Red tint for predators
    PREY,               // Green tint for herbivores
    HUNGRY,             // Yellow for low energy
    REPRODUCING,        // Pink for ready to reproduce
    SELECTED_SPECIES,   // Highlight all of a species
    OLDEST,             // Highlight oldest creatures
    YOUNGEST,           // Highlight youngest creatures
    COUNT
};

struct HighlightSettings {
    HighlightMode mode = HighlightMode::NONE;
    genetics::SpeciesId selectedSpecies = 0;
    float hungerThreshold = 0.3f;    // Below this energy = hungry
    float pulseSpeed = 2.0f;         // Highlight pulse speed
    float pulseIntensity = 0.3f;     // How much to modulate color

    // Colors for each mode
    glm::vec3 predatorColor = glm::vec3(1.0f, 0.3f, 0.3f);
    glm::vec3 preyColor = glm::vec3(0.3f, 1.0f, 0.3f);
    glm::vec3 hungryColor = glm::vec3(1.0f, 1.0f, 0.3f);
    glm::vec3 reproducingColor = glm::vec3(1.0f, 0.5f, 0.8f);
    glm::vec3 speciesColor = glm::vec3(0.5f, 0.8f, 1.0f);
    glm::vec3 oldestColor = glm::vec3(0.8f, 0.6f, 0.2f);
    glm::vec3 youngestColor = glm::vec3(0.6f, 1.0f, 0.9f);
};

// ============================================================================
// Event Notifications
// ============================================================================

enum class GameEventType {
    INFO,
    BIRTH,
    DEATH,
    SPECIATION,
    EXTINCTION,
    ACHIEVEMENT,
    MILESTONE,
    ENVIRONMENT,
    WARNING
};

struct GameEvent {
    std::string text;
    GameEventType type;
    float timestamp;
    float duration;
    float elapsed = 0.0f;
    glm::vec3 color;
    bool important = false;  // Important events stay longer
};

// ============================================================================
// Spotlight Feature
// ============================================================================

enum class SpotlightCategory {
    NONE,
    OLDEST,
    FASTEST,
    LARGEST,
    MOST_OFFSPRING,
    MOST_KILLS,
    HIGHEST_FITNESS,
    RANDOM_INTERESTING
};

struct SpotlightCreature {
    const Creature* creature = nullptr;
    SpotlightCategory category = SpotlightCategory::NONE;
    float showTime = 0.0f;
    float maxShowTime = 10.0f;
    std::string bioText;
    bool followCamera = false;
};

// ============================================================================
// Environmental Events
// ============================================================================

enum class EnvironmentEventType {
    NONE,
    GOLDEN_AGE,        // Abundant food, perfect weather
    DROUGHT,           // Less food, harder survival
    FOOD_BLOOM,        // Sudden food abundance
    HARSH_WINTER,      // Energy drain increased
    MIGRATION_SEASON,  // Creatures tend to move
    BREEDING_SEASON,   // Reproduction more likely
    PLAGUE             // Random deaths
};

struct EnvironmentEvent {
    EnvironmentEventType type = EnvironmentEventType::NONE;
    float duration = 0.0f;
    float elapsed = 0.0f;
    float intensity = 1.0f;
    std::string name;
    std::string description;

    bool isActive() const { return type != EnvironmentEventType::NONE && elapsed < duration; }
    float getProgress() const { return duration > 0 ? elapsed / duration : 0.0f; }
    float getRemainingTime() const { return std::max(0.0f, duration - elapsed); }
};

// ============================================================================
// Gameplay Manager
// ============================================================================

class GameplayManager {
public:
    GameplayManager();
    ~GameplayManager() = default;

    // ========================================================================
    // Initialization
    // ========================================================================

    void initialize();
    void shutdown();

    // ========================================================================
    // Update
    // ========================================================================

    void update(float deltaTime, float simulationTime, CreatureManager* creatures);

    // ========================================================================
    // Time Control
    // ========================================================================

    TimeControl& getTimeControl() { return m_timeControl; }
    const TimeControl& getTimeControl() const { return m_timeControl; }

    void togglePause() { m_timeControl.paused = !m_timeControl.paused; }
    void setPaused(bool paused) { m_timeControl.paused = paused; }
    bool isPaused() const { return m_timeControl.paused; }

    void setTimeScale(float scale) { m_timeControl.setSpeed(scale); }
    float getTimeScale() const { return m_timeControl.timeScale; }
    float getEffectiveTimeScale() const { return m_timeControl.paused ? 0.0f : m_timeControl.timeScale; }

    // ========================================================================
    // Statistics
    // ========================================================================

    const LiveStatistics& getStatistics() const { return m_statistics; }
    LiveStatistics& getStatistics() { return m_statistics; }

    void recordBirth(const Creature& creature);
    void recordDeath(const Creature& creature, const std::string& cause);

    // Manual population update (for use with SimulationWorld instead of CreatureManager)
    void updatePopulation(int totalCreatures, int herbivores, int carnivores, int aquatic, int flying);
    void recordSimpleBirth();
    void recordSimpleDeath(const std::string& cause = "");

    // ========================================================================
    // Achievements
    // ========================================================================

    const std::vector<Achievement>& getAchievements() const { return m_achievements; }
    int getUnlockedAchievementCount() const;
    const Achievement* getLatestAchievement() const;
    void checkAchievements(float simulationTime, CreatureManager* creatures);

    // ========================================================================
    // Highlighting
    // ========================================================================

    HighlightSettings& getHighlightSettings() { return m_highlightSettings; }
    const HighlightSettings& getHighlightSettings() const { return m_highlightSettings; }

    void setHighlightMode(HighlightMode mode) { m_highlightSettings.mode = mode; }
    HighlightMode getHighlightMode() const { return m_highlightSettings.mode; }

    void selectSpeciesForHighlight(genetics::SpeciesId id) { m_highlightSettings.selectedSpecies = id; }

    // Get highlight color for a creature (returns (0,0,0) if no highlight)
    glm::vec3 getCreatureHighlightColor(const Creature& creature, float time) const;
    bool shouldHighlightCreature(const Creature& creature) const;

    // ========================================================================
    // Event Log
    // ========================================================================

    const std::deque<GameEvent>& getEventLog() const { return m_eventLog; }
    void addEvent(const std::string& text, GameEventType type, bool important = false);
    void clearEventLog() { m_eventLog.clear(); }

    // ========================================================================
    // Spotlight
    // ========================================================================

    const SpotlightCreature& getSpotlight() const { return m_spotlight; }
    bool hasSpotlight() const { return m_spotlight.creature != nullptr; }

    void setSpotlightCategory(SpotlightCategory category);
    void clearSpotlight() { m_spotlight.creature = nullptr; m_spotlight.category = SpotlightCategory::NONE; }
    void nextSpotlight();
    void toggleSpotlightFollow() { m_spotlight.followCamera = !m_spotlight.followCamera; }

    // ========================================================================
    // Environmental Events
    // ========================================================================

    const EnvironmentEvent& getCurrentEnvironmentEvent() const { return m_currentEnvironmentEvent; }
    bool hasEnvironmentEvent() const { return m_currentEnvironmentEvent.isActive(); }

    void triggerEnvironmentEvent(EnvironmentEventType type, float duration, float intensity = 1.0f);
    void clearEnvironmentEvent();

    // Get modifiers from current environment event
    float getFoodModifier() const;
    float getEnergyDrainModifier() const;
    float getReproductionModifier() const;

    // ========================================================================
    // Callbacks
    // ========================================================================

    using AchievementCallback = std::function<void(const Achievement&)>;
    using EventCallback = std::function<void(const GameEvent&)>;

    void setAchievementCallback(AchievementCallback cb) { m_achievementCallback = cb; }
    void setEventCallback(EventCallback cb) { m_eventCallback = cb; }

private:
    // Systems
    TimeControl m_timeControl;
    LiveStatistics m_statistics;
    HighlightSettings m_highlightSettings;
    SpotlightCreature m_spotlight;
    EnvironmentEvent m_currentEnvironmentEvent;

    // Achievements
    std::vector<Achievement> m_achievements;
    void initializeAchievements();
    void unlockAchievement(AchievementType type, float time);

    // Event log
    std::deque<GameEvent> m_eventLog;
    static constexpr size_t MAX_EVENT_LOG = 100;
    static constexpr float DEFAULT_EVENT_DURATION = 5.0f;
    static constexpr float IMPORTANT_EVENT_DURATION = 10.0f;

    // Callbacks
    AchievementCallback m_achievementCallback;
    EventCallback m_eventCallback;

    // Internal tracking
    float m_lastSimulationTime = 0.0f;
    int m_lastPopulation = 0;
    int m_lastGeneration = 0;
    float m_ecosystemBalanceTimer = 0.0f;
    float m_goldenAgeTimer = 0.0f;
    float m_environmentEventTimer = 0.0f;
    float m_spotlightTimer = 0.0f;

    // Update helpers
    void updateStatistics(float deltaTime, float simulationTime, CreatureManager* creatures);
    void updateEvents(float deltaTime);
    void updateSpotlight(float deltaTime, CreatureManager* creatures);
    void updateEnvironmentEvent(float deltaTime);
    void updateRecords(CreatureManager* creatures, float simulationTime);

    // Find interesting creatures for spotlight
    const Creature* findOldestCreature(CreatureManager* creatures) const;
    const Creature* findFastestCreature(CreatureManager* creatures) const;
    const Creature* findLargestCreature(CreatureManager* creatures) const;
    const Creature* findHighestFitnessCreature(CreatureManager* creatures) const;

    // Event color helpers
    static glm::vec3 getEventColor(GameEventType type);
};

} // namespace Forge
