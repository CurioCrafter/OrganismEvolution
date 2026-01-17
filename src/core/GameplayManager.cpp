#include "GameplayManager.h"
#include "CreatureManager.h"
#include <algorithm>
#include <cmath>

namespace Forge {

// ============================================================================
// Constructor / Initialization
// ============================================================================

GameplayManager::GameplayManager() {
    initializeAchievements();
}

void GameplayManager::initialize() {
    m_timeControl = TimeControl{};
    m_statistics = LiveStatistics{};
    m_highlightSettings = HighlightSettings{};
    m_spotlight = SpotlightCreature{};
    m_currentEnvironmentEvent = EnvironmentEvent{};
    m_eventLog.clear();

    addEvent("Simulation started", GameEventType::INFO);
}

void GameplayManager::shutdown() {
    m_eventLog.clear();
    m_spotlight.creature = nullptr;
}

void GameplayManager::initializeAchievements() {
    m_achievements.clear();
    m_achievements.reserve(static_cast<size_t>(AchievementType::COUNT));

    auto addAch = [this](AchievementType type, const std::string& name, const std::string& desc) {
        Achievement ach;
        ach.type = type;
        ach.name = name;
        ach.description = desc;
        m_achievements.push_back(ach);
    };

    // Predation achievements
    addAch(AchievementType::FIRST_PREDATION, "First Blood", "A predator makes its first kill");
    addAch(AchievementType::APEX_PREDATOR, "Apex Predator", "A single predator reaches 100 kills");
    addAch(AchievementType::HUNTING_DYNASTY, "Hunting Dynasty", "A predator lineage survives 10 generations");

    // Population achievements
    addAch(AchievementType::POPULATION_BOOM, "Population Boom", "Reach 500 creatures");
    addAch(AchievementType::POPULATION_EXPLOSION, "Population Explosion", "Reach 1000 creatures");
    addAch(AchievementType::MASS_EXTINCTION, "Mass Extinction", "Population drops below 50");

    // Evolution achievements
    addAch(AchievementType::BIODIVERSITY, "Biodiversity", "20+ unique species active simultaneously");
    addAch(AchievementType::ANCIENT_LINEAGE, "Ancient Lineage", "A species survives 50 generations");
    addAch(AchievementType::RAPID_SPECIATION, "Rapid Speciation", "5 new species emerge in one minute");

    // Survival achievements
    addAch(AchievementType::ELDER, "Elder", "A creature survives to age 100");
    addAch(AchievementType::CENTENARIAN, "Centenarian", "A creature survives to age 200");
    addAch(AchievementType::SURVIVOR, "Survivor", "Population recovers after dropping below 100");

    // Ecosystem achievements
    addAch(AchievementType::ECOSYSTEM_BALANCE, "Ecosystem Balance", "Maintain 40-60% predator/prey ratio for 5 minutes");
    addAch(AchievementType::GOLDEN_AGE, "Golden Age", "90%+ creatures have high energy for 2 minutes");
    addAch(AchievementType::GREAT_FAMINE, "Great Famine", "80%+ creatures are starving");

    // Milestone achievements
    addAch(AchievementType::FIRST_GENERATION, "New Generation", "First generation is born");
    addAch(AchievementType::TEN_GENERATIONS, "Ten Generations", "Reach 10 generations");
    addAch(AchievementType::HUNDRED_GENERATIONS, "Century of Evolution", "Reach 100 generations");
    addAch(AchievementType::FIRST_SPECIES, "Speciation", "First speciation event occurs");
    addAch(AchievementType::AQUATIC_DOMINANCE, "Aquatic Dominance", "50%+ creatures are aquatic");
    addAch(AchievementType::AERIAL_DOMINANCE, "Aerial Dominance", "50%+ creatures are flying");
}

// ============================================================================
// Main Update
// ============================================================================

void GameplayManager::update(float deltaTime, float simulationTime, CreatureManager* creatures) {
    // Update time control smooth transitions
    m_timeControl.smoothUpdate(deltaTime);

    // Don't update gameplay systems if paused
    if (m_timeControl.paused) {
        return;
    }

    float scaledDelta = deltaTime * m_timeControl.timeScale;

    updateStatistics(scaledDelta, simulationTime, creatures);
    updateEvents(deltaTime);  // Events use real time
    updateSpotlight(deltaTime, creatures);
    updateEnvironmentEvent(scaledDelta);
    updateRecords(creatures, simulationTime);

    // Check for achievements
    checkAchievements(simulationTime, creatures);

    m_lastSimulationTime = simulationTime;
}

// ============================================================================
// Statistics
// ============================================================================

void GameplayManager::updateStatistics(float deltaTime, float simulationTime, CreatureManager* creatures) {
    m_statistics.minuteTimer += deltaTime;

    // Roll over every minute
    if (m_statistics.minuteTimer >= 60.0f) {
        m_statistics.rollMinute();
    }

    // If creatures manager is not available, skip population tracking
    // (main.cpp uses a different creature system)
    if (!creatures) return;

    // Update population tracking
    int currentPop = creatures->getTotalPopulation();
    if (currentPop > m_statistics.peakPopulation) {
        m_statistics.peakPopulation = currentPop;
        m_statistics.peakPopulationTime = simulationTime;

        // Notify on population milestones
        if (currentPop >= 100 && m_lastPopulation < 100) {
            addEvent("Population reached 100!", GameEventType::MILESTONE, true);
        }
        if (currentPop >= 500 && m_lastPopulation < 500) {
            addEvent("Population reached 500!", GameEventType::MILESTONE, true);
        }
        if (currentPop >= 1000 && m_lastPopulation < 1000) {
            addEvent("Population record broken: 1000 creatures!", GameEventType::MILESTONE, true);
        }
    }

    m_lastPopulation = currentPop;
}

void GameplayManager::updateRecords(CreatureManager* creatures, float simulationTime) {
    // If creatures manager is not available, skip record tracking
    if (!creatures) return;

    // Reset current records
    m_statistics.currentOldest = CreatureRecord{};
    m_statistics.currentFastest = CreatureRecord{};
    m_statistics.currentLargest = CreatureRecord{};

    const auto& creatureList = creatures->getAllCreatures();
    for (const auto& creature : creatureList) {
        if (!creature || !creature->isAlive()) continue;

        // Oldest
        if (creature->getAge() > m_statistics.currentOldest.value) {
            m_statistics.currentOldest.creatureId = creature->getID();
            m_statistics.currentOldest.value = creature->getAge();
            m_statistics.currentOldest.speciesId = creature->getSpeciesId();
            m_statistics.currentOldest.timestamp = simulationTime;
        }

        // Fastest
        float speed = creature->getSpeed();
        if (speed > m_statistics.currentFastest.value) {
            m_statistics.currentFastest.creatureId = creature->getID();
            m_statistics.currentFastest.value = speed;
            m_statistics.currentFastest.speciesId = creature->getSpeciesId();
            m_statistics.currentFastest.timestamp = simulationTime;
        }

        // Largest
        float size = creature->getSize();
        if (size > m_statistics.currentLargest.value) {
            m_statistics.currentLargest.creatureId = creature->getID();
            m_statistics.currentLargest.value = size;
            m_statistics.currentLargest.speciesId = creature->getSpeciesId();
            m_statistics.currentLargest.timestamp = simulationTime;
        }

        // Update all-time records
        if (creature->getAge() > m_statistics.oldestEver.value) {
            m_statistics.oldestEver = m_statistics.currentOldest;
        }
        if (speed > m_statistics.fastestEver.value) {
            m_statistics.fastestEver = m_statistics.currentFastest;
        }
        if (size > m_statistics.largestEver.value) {
            m_statistics.largestEver = m_statistics.currentLargest;
        }
    }
}

void GameplayManager::recordBirth(const Creature& creature) {
    m_statistics.birthsThisMinute++;

    // Track species offspring
    auto speciesId = creature.getSpeciesId();
    m_statistics.speciesOffspring[speciesId]++;

    // Check for most successful species
    int offspring = m_statistics.speciesOffspring[speciesId];
    if (offspring > m_statistics.mostSuccessfulSpeciesOffspring) {
        m_statistics.mostSuccessfulSpeciesOffspring = offspring;
        m_statistics.mostSuccessfulSpecies = speciesId;
    }

    // Track generation
    int gen = creature.getGeneration();
    if (gen > m_statistics.longestLineage) {
        m_statistics.longestLineage = gen;
    }
}

void GameplayManager::recordDeath(const Creature& creature, const std::string& cause) {
    m_statistics.deathsThisMinute++;

    // Check if this was a record-holder
    if (creature.getID() == m_statistics.currentOldest.creatureId) {
        std::string msg = "The oldest creature has died (age " +
                          std::to_string(static_cast<int>(creature.getAge())) + ")";
        addEvent(msg, GameEventType::DEATH);
    }
}

void GameplayManager::updatePopulation(int totalCreatures, int herbivores, int carnivores, int aquatic, int flying) {
    // Update population tracking for SimulationWorld
    if (totalCreatures > m_statistics.peakPopulation) {
        m_statistics.peakPopulation = totalCreatures;
        m_statistics.peakPopulationTime = m_lastSimulationTime;

        // Notify on population milestones
        if (totalCreatures >= 100 && m_lastPopulation < 100) {
            addEvent("Population reached 100!", GameEventType::MILESTONE, true);
        }
        if (totalCreatures >= 500 && m_lastPopulation < 500) {
            addEvent("Population reached 500!", GameEventType::MILESTONE, true);
        }
        if (totalCreatures >= 1000 && m_lastPopulation < 1000) {
            addEvent("Population record broken: 1000 creatures!", GameEventType::MILESTONE, true);
        }
    }

    // Check for mass extinction
    if (totalCreatures < 50 && m_lastPopulation >= 50) {
        addEvent("Mass extinction! Population below 50!", GameEventType::WARNING, true);
    }

    m_lastPopulation = totalCreatures;
}

void GameplayManager::recordSimpleBirth() {
    m_statistics.birthsThisMinute++;
}

void GameplayManager::recordSimpleDeath(const std::string& cause) {
    m_statistics.deathsThisMinute++;
    if (!cause.empty()) {
        addEvent("Creature died: " + cause, GameEventType::DEATH);
    }
}

// ============================================================================
// Achievements
// ============================================================================

void GameplayManager::checkAchievements(float simulationTime, CreatureManager* creatures) {
    // If creatures manager is not available, skip achievement checking
    if (!creatures) return;

    const auto& creatureList = creatures->getAllCreatures();
    int totalCreatures = 0;
    int predatorCount = 0;
    int herbivoreCount = 0;
    int aquaticCount = 0;
    int flyingCount = 0;
    int highEnergyCount = 0;
    int lowEnergyCount = 0;
    int maxKills = 0;
    float maxAge = 0.0f;
    int maxGeneration = 0;

    std::set<genetics::SpeciesId> activeSpecies;

    for (const auto& creature : creatureList) {
        if (!creature || !creature->isAlive()) continue;

        totalCreatures++;
        activeSpecies.insert(creature->getSpeciesId());

        if (isPredator(creature->getType())) {
            predatorCount++;
            maxKills = std::max(maxKills, creature->getKillCount());
        }
        if (isHerbivore(creature->getType())) {
            herbivoreCount++;
        }
        if (isAquatic(creature->getType())) {
            aquaticCount++;
        }
        if (isFlying(creature->getType())) {
            flyingCount++;
        }

        if (creature->getEnergy() > 0.5f * creature->getMaxEnergy()) {
            highEnergyCount++;
        }
        if (creature->getEnergy() < 0.3f * creature->getMaxEnergy()) {
            lowEnergyCount++;
        }

        maxAge = std::max(maxAge, creature->getAge());
        maxGeneration = std::max(maxGeneration, creature->getGeneration());
    }

    // Check population achievements
    if (totalCreatures >= 500) {
        unlockAchievement(AchievementType::POPULATION_BOOM, simulationTime);
    }
    if (totalCreatures >= 1000) {
        unlockAchievement(AchievementType::POPULATION_EXPLOSION, simulationTime);
    }
    if (totalCreatures < 50 && totalCreatures > 0) {
        unlockAchievement(AchievementType::MASS_EXTINCTION, simulationTime);
    }

    // Check biodiversity
    if (activeSpecies.size() >= 20) {
        unlockAchievement(AchievementType::BIODIVERSITY, simulationTime);
    }

    // Check survival achievements
    if (maxAge >= 100.0f) {
        unlockAchievement(AchievementType::ELDER, simulationTime);
    }
    if (maxAge >= 200.0f) {
        unlockAchievement(AchievementType::CENTENARIAN, simulationTime);
    }

    // Check generation milestones
    if (maxGeneration >= 1) {
        unlockAchievement(AchievementType::FIRST_GENERATION, simulationTime);
    }
    if (maxGeneration >= 10) {
        unlockAchievement(AchievementType::TEN_GENERATIONS, simulationTime);
    }
    if (maxGeneration >= 100) {
        unlockAchievement(AchievementType::HUNDRED_GENERATIONS, simulationTime);
    }

    // Check predation achievements
    if (maxKills >= 1) {
        unlockAchievement(AchievementType::FIRST_PREDATION, simulationTime);
    }
    if (maxKills >= 100) {
        unlockAchievement(AchievementType::APEX_PREDATOR, simulationTime);
    }

    // Check dominance achievements
    if (totalCreatures > 0) {
        float aquaticRatio = static_cast<float>(aquaticCount) / totalCreatures;
        float flyingRatio = static_cast<float>(flyingCount) / totalCreatures;

        if (aquaticRatio >= 0.5f) {
            unlockAchievement(AchievementType::AQUATIC_DOMINANCE, simulationTime);
        }
        if (flyingRatio >= 0.5f) {
            unlockAchievement(AchievementType::AERIAL_DOMINANCE, simulationTime);
        }

        // Ecosystem balance tracking
        float predatorRatio = static_cast<float>(predatorCount) / totalCreatures;
        if (predatorRatio >= 0.4f && predatorRatio <= 0.6f) {
            m_ecosystemBalanceTimer += 1.0f / 60.0f;  // Assuming 60 FPS
            if (m_ecosystemBalanceTimer >= 300.0f) {  // 5 minutes
                unlockAchievement(AchievementType::ECOSYSTEM_BALANCE, simulationTime);
            }
        } else {
            m_ecosystemBalanceTimer = 0.0f;
        }

        // Golden age tracking
        float highEnergyRatio = static_cast<float>(highEnergyCount) / totalCreatures;
        if (highEnergyRatio >= 0.9f) {
            m_goldenAgeTimer += 1.0f / 60.0f;
            if (m_goldenAgeTimer >= 120.0f) {  // 2 minutes
                unlockAchievement(AchievementType::GOLDEN_AGE, simulationTime);
            }
        } else {
            m_goldenAgeTimer = 0.0f;
        }

        // Great famine check
        float lowEnergyRatio = static_cast<float>(lowEnergyCount) / totalCreatures;
        if (lowEnergyRatio >= 0.8f) {
            unlockAchievement(AchievementType::GREAT_FAMINE, simulationTime);
        }
    }
}

void GameplayManager::unlockAchievement(AchievementType type, float time) {
    size_t index = static_cast<size_t>(type);
    if (index >= m_achievements.size()) return;

    Achievement& ach = m_achievements[index];
    if (ach.unlocked) return;

    ach.unlocked = true;
    ach.unlockTime = time;

    // Create event
    std::string text = "Achievement Unlocked: " + ach.name;
    addEvent(text, GameEventType::ACHIEVEMENT, true);

    // Callback
    if (m_achievementCallback) {
        m_achievementCallback(ach);
    }
}

int GameplayManager::getUnlockedAchievementCount() const {
    int count = 0;
    for (const auto& ach : m_achievements) {
        if (ach.unlocked) count++;
    }
    return count;
}

const Achievement* GameplayManager::getLatestAchievement() const {
    const Achievement* latest = nullptr;
    float latestTime = -1.0f;

    for (const auto& ach : m_achievements) {
        if (ach.unlocked && !ach.notified && ach.unlockTime > latestTime) {
            latest = &ach;
            latestTime = ach.unlockTime;
        }
    }

    return latest;
}

// ============================================================================
// Highlighting
// ============================================================================

bool GameplayManager::shouldHighlightCreature(const Creature& creature) const {
    if (m_highlightSettings.mode == HighlightMode::NONE) return false;
    if (!creature.isAlive()) return false;

    switch (m_highlightSettings.mode) {
        case HighlightMode::PREDATORS:
            return isPredator(creature.getType());

        case HighlightMode::PREY:
            return isHerbivore(creature.getType());

        case HighlightMode::HUNGRY:
            return creature.getEnergy() < m_highlightSettings.hungerThreshold * creature.getMaxEnergy();

        case HighlightMode::REPRODUCING:
            return creature.canReproduce();

        case HighlightMode::SELECTED_SPECIES:
            return creature.getSpeciesId() == m_highlightSettings.selectedSpecies;

        case HighlightMode::OLDEST:
            return creature.getAge() > 50.0f;  // Highlight creatures older than 50

        case HighlightMode::YOUNGEST:
            return creature.getAge() < 5.0f;  // Highlight creatures younger than 5

        default:
            return false;
    }
}

glm::vec3 GameplayManager::getCreatureHighlightColor(const Creature& creature, float time) const {
    if (!shouldHighlightCreature(creature)) {
        return glm::vec3(0.0f);
    }

    // Calculate pulse
    float pulse = 1.0f + m_highlightSettings.pulseIntensity *
                  std::sin(time * m_highlightSettings.pulseSpeed * 3.14159f * 2.0f);

    glm::vec3 baseColor;
    switch (m_highlightSettings.mode) {
        case HighlightMode::PREDATORS:
            baseColor = m_highlightSettings.predatorColor;
            break;
        case HighlightMode::PREY:
            baseColor = m_highlightSettings.preyColor;
            break;
        case HighlightMode::HUNGRY:
            baseColor = m_highlightSettings.hungryColor;
            break;
        case HighlightMode::REPRODUCING:
            baseColor = m_highlightSettings.reproducingColor;
            break;
        case HighlightMode::SELECTED_SPECIES:
            baseColor = m_highlightSettings.speciesColor;
            break;
        case HighlightMode::OLDEST:
            baseColor = m_highlightSettings.oldestColor;
            break;
        case HighlightMode::YOUNGEST:
            baseColor = m_highlightSettings.youngestColor;
            break;
        default:
            baseColor = glm::vec3(1.0f);
    }

    return baseColor * pulse;
}

// ============================================================================
// Event Log
// ============================================================================

void GameplayManager::addEvent(const std::string& text, GameEventType type, bool important) {
    GameEvent event;
    event.text = text;
    event.type = type;
    event.timestamp = m_lastSimulationTime;
    event.duration = important ? IMPORTANT_EVENT_DURATION : DEFAULT_EVENT_DURATION;
    event.color = getEventColor(type);
    event.important = important;

    m_eventLog.push_front(event);

    // Trim log
    while (m_eventLog.size() > MAX_EVENT_LOG) {
        m_eventLog.pop_back();
    }

    // Callback
    if (m_eventCallback) {
        m_eventCallback(event);
    }
}

void GameplayManager::updateEvents(float deltaTime) {
    // Update event elapsed times
    for (auto& event : m_eventLog) {
        event.elapsed += deltaTime;
    }
}

glm::vec3 GameplayManager::getEventColor(GameEventType type) {
    switch (type) {
        case GameEventType::INFO:
            return glm::vec3(1.0f, 1.0f, 1.0f);
        case GameEventType::BIRTH:
            return glm::vec3(0.4f, 1.0f, 0.4f);
        case GameEventType::DEATH:
            return glm::vec3(1.0f, 0.4f, 0.4f);
        case GameEventType::SPECIATION:
            return glm::vec3(0.4f, 0.8f, 1.0f);
        case GameEventType::EXTINCTION:
            return glm::vec3(0.8f, 0.2f, 0.2f);
        case GameEventType::ACHIEVEMENT:
            return glm::vec3(1.0f, 0.8f, 0.2f);
        case GameEventType::MILESTONE:
            return glm::vec3(0.8f, 0.6f, 1.0f);
        case GameEventType::ENVIRONMENT:
            return glm::vec3(0.5f, 0.9f, 0.7f);
        case GameEventType::WARNING:
            return glm::vec3(1.0f, 0.6f, 0.2f);
        default:
            return glm::vec3(1.0f);
    }
}

// ============================================================================
// Spotlight
// ============================================================================

void GameplayManager::updateSpotlight(float deltaTime, CreatureManager* creatures) {
    m_spotlightTimer += deltaTime;

    // If creatures manager is not available, skip spotlight
    if (!creatures) return;

    // Check if current spotlight creature is still valid
    if (m_spotlight.creature) {
        if (!m_spotlight.creature->isAlive()) {
            m_spotlight.creature = nullptr;
        } else {
            m_spotlight.showTime += deltaTime;
            if (m_spotlight.showTime >= m_spotlight.maxShowTime) {
                // Time to pick a new spotlight
                m_spotlight.creature = nullptr;
            }
        }
    }

    // Auto-select new spotlight periodically (every 15 seconds if no manual selection)
    if (!m_spotlight.creature && m_spotlightTimer >= 15.0f) {
        // Rotate through categories
        static int categoryIndex = 0;
        SpotlightCategory categories[] = {
            SpotlightCategory::OLDEST,
            SpotlightCategory::FASTEST,
            SpotlightCategory::LARGEST,
            SpotlightCategory::HIGHEST_FITNESS
        };

        setSpotlightCategory(categories[categoryIndex % 4]);
        categoryIndex++;
        m_spotlightTimer = 0.0f;
    }
}

void GameplayManager::setSpotlightCategory(SpotlightCategory category) {
    m_spotlight.category = category;
    m_spotlight.showTime = 0.0f;
    m_spotlight.followCamera = false;

    // Note: Actual creature selection happens in update when we have access to CreatureManager
}

void GameplayManager::nextSpotlight() {
    int next = (static_cast<int>(m_spotlight.category) + 1) % static_cast<int>(SpotlightCategory::RANDOM_INTERESTING);
    if (next == 0) next = 1;  // Skip NONE
    setSpotlightCategory(static_cast<SpotlightCategory>(next));
}

const Creature* GameplayManager::findOldestCreature(CreatureManager* creatures) const {
    if (!creatures) return nullptr;

    const Creature* oldest = nullptr;
    float maxAge = 0.0f;

    for (const auto& creature : creatures->getAllCreatures()) {
        if (!creature || !creature->isAlive()) continue;
        if (creature->getAge() > maxAge) {
            maxAge = creature->getAge();
            oldest = creature.get();
        }
    }

    return oldest;
}

const Creature* GameplayManager::findFastestCreature(CreatureManager* creatures) const {
    if (!creatures) return nullptr;

    const Creature* fastest = nullptr;
    float maxSpeed = 0.0f;

    for (const auto& creature : creatures->getAllCreatures()) {
        if (!creature || !creature->isAlive()) continue;
        if (creature->getSpeed() > maxSpeed) {
            maxSpeed = creature->getSpeed();
            fastest = creature.get();
        }
    }

    return fastest;
}

const Creature* GameplayManager::findLargestCreature(CreatureManager* creatures) const {
    if (!creatures) return nullptr;

    const Creature* largest = nullptr;
    float maxSize = 0.0f;

    for (const auto& creature : creatures->getAllCreatures()) {
        if (!creature || !creature->isAlive()) continue;
        if (creature->getSize() > maxSize) {
            maxSize = creature->getSize();
            largest = creature.get();
        }
    }

    return largest;
}

const Creature* GameplayManager::findHighestFitnessCreature(CreatureManager* creatures) const {
    if (!creatures) return nullptr;

    const Creature* fittest = nullptr;
    float maxFitness = 0.0f;

    for (const auto& creature : creatures->getAllCreatures()) {
        if (!creature || !creature->isAlive()) continue;
        if (creature->getFitness() > maxFitness) {
            maxFitness = creature->getFitness();
            fittest = creature.get();
        }
    }

    return fittest;
}

// ============================================================================
// Environmental Events
// ============================================================================

void GameplayManager::updateEnvironmentEvent(float deltaTime) {
    if (!m_currentEnvironmentEvent.isActive()) {
        // Random chance to trigger an event
        m_environmentEventTimer += deltaTime;
        if (m_environmentEventTimer >= 120.0f) {  // Check every 2 minutes
            m_environmentEventTimer = 0.0f;

            // 20% chance of an event
            if ((rand() % 100) < 20) {
                // Pick random event type
                EnvironmentEventType types[] = {
                    EnvironmentEventType::GOLDEN_AGE,
                    EnvironmentEventType::DROUGHT,
                    EnvironmentEventType::FOOD_BLOOM,
                    EnvironmentEventType::BREEDING_SEASON
                };
                int idx = rand() % 4;
                triggerEnvironmentEvent(types[idx], 60.0f + (rand() % 60), 0.5f + (rand() % 50) / 100.0f);
            }
        }
        return;
    }

    m_currentEnvironmentEvent.elapsed += deltaTime;

    // Check if event ended
    if (m_currentEnvironmentEvent.elapsed >= m_currentEnvironmentEvent.duration) {
        addEvent(m_currentEnvironmentEvent.name + " has ended", GameEventType::ENVIRONMENT);
        m_currentEnvironmentEvent.type = EnvironmentEventType::NONE;
    }
}

void GameplayManager::triggerEnvironmentEvent(EnvironmentEventType type, float duration, float intensity) {
    m_currentEnvironmentEvent.type = type;
    m_currentEnvironmentEvent.duration = duration;
    m_currentEnvironmentEvent.elapsed = 0.0f;
    m_currentEnvironmentEvent.intensity = glm::clamp(intensity, 0.1f, 2.0f);

    switch (type) {
        case EnvironmentEventType::GOLDEN_AGE:
            m_currentEnvironmentEvent.name = "Golden Age";
            m_currentEnvironmentEvent.description = "Food is abundant and conditions are ideal";
            break;
        case EnvironmentEventType::DROUGHT:
            m_currentEnvironmentEvent.name = "Drought";
            m_currentEnvironmentEvent.description = "Food is scarce and survival is harder";
            break;
        case EnvironmentEventType::FOOD_BLOOM:
            m_currentEnvironmentEvent.name = "Food Bloom";
            m_currentEnvironmentEvent.description = "A sudden explosion of food sources";
            break;
        case EnvironmentEventType::HARSH_WINTER:
            m_currentEnvironmentEvent.name = "Harsh Winter";
            m_currentEnvironmentEvent.description = "Energy drain is increased";
            break;
        case EnvironmentEventType::MIGRATION_SEASON:
            m_currentEnvironmentEvent.name = "Migration Season";
            m_currentEnvironmentEvent.description = "Creatures are restless and tend to move";
            break;
        case EnvironmentEventType::BREEDING_SEASON:
            m_currentEnvironmentEvent.name = "Breeding Season";
            m_currentEnvironmentEvent.description = "Reproduction is more likely";
            break;
        case EnvironmentEventType::PLAGUE:
            m_currentEnvironmentEvent.name = "Plague";
            m_currentEnvironmentEvent.description = "A mysterious illness spreads";
            break;
        default:
            m_currentEnvironmentEvent.name = "Unknown Event";
            m_currentEnvironmentEvent.description = "";
    }

    addEvent(m_currentEnvironmentEvent.name + " begins!", GameEventType::ENVIRONMENT, true);
}

void GameplayManager::clearEnvironmentEvent() {
    if (m_currentEnvironmentEvent.isActive()) {
        addEvent(m_currentEnvironmentEvent.name + " was ended early", GameEventType::ENVIRONMENT);
    }
    m_currentEnvironmentEvent.type = EnvironmentEventType::NONE;
}

float GameplayManager::getFoodModifier() const {
    if (!m_currentEnvironmentEvent.isActive()) return 1.0f;

    switch (m_currentEnvironmentEvent.type) {
        case EnvironmentEventType::GOLDEN_AGE:
            return 1.0f + 0.5f * m_currentEnvironmentEvent.intensity;
        case EnvironmentEventType::DROUGHT:
            return 1.0f - 0.4f * m_currentEnvironmentEvent.intensity;
        case EnvironmentEventType::FOOD_BLOOM:
            return 1.0f + 1.0f * m_currentEnvironmentEvent.intensity;
        case EnvironmentEventType::HARSH_WINTER:
            return 1.0f - 0.3f * m_currentEnvironmentEvent.intensity;
        default:
            return 1.0f;
    }
}

float GameplayManager::getEnergyDrainModifier() const {
    if (!m_currentEnvironmentEvent.isActive()) return 1.0f;

    switch (m_currentEnvironmentEvent.type) {
        case EnvironmentEventType::GOLDEN_AGE:
            return 1.0f - 0.2f * m_currentEnvironmentEvent.intensity;
        case EnvironmentEventType::DROUGHT:
            return 1.0f + 0.3f * m_currentEnvironmentEvent.intensity;
        case EnvironmentEventType::HARSH_WINTER:
            return 1.0f + 0.5f * m_currentEnvironmentEvent.intensity;
        case EnvironmentEventType::PLAGUE:
            return 1.0f + 0.4f * m_currentEnvironmentEvent.intensity;
        default:
            return 1.0f;
    }
}

float GameplayManager::getReproductionModifier() const {
    if (!m_currentEnvironmentEvent.isActive()) return 1.0f;

    switch (m_currentEnvironmentEvent.type) {
        case EnvironmentEventType::GOLDEN_AGE:
            return 1.0f + 0.3f * m_currentEnvironmentEvent.intensity;
        case EnvironmentEventType::DROUGHT:
            return 1.0f - 0.3f * m_currentEnvironmentEvent.intensity;
        case EnvironmentEventType::BREEDING_SEASON:
            return 1.0f + 0.5f * m_currentEnvironmentEvent.intensity;
        case EnvironmentEventType::PLAGUE:
            return 1.0f - 0.5f * m_currentEnvironmentEvent.intensity;
        default:
            return 1.0f;
    }
}

} // namespace Forge
