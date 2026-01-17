#include "DisasterSystem.h"
#include "disasters/Volcano.h"
#include "disasters/MeteorImpact.h"
#include "disasters/Disease.h"
#include "disasters/IceAge.h"
#include "disasters/Drought.h"
#include "disasters/Flood.h"
#include "../core/SimulationOrchestrator.h"
#include "../core/CreatureManager.h"
#include "VegetationManager.h"
#include "Terrain.h"
#include "ClimateSystem.h"
#include "WeatherSystem.h"
#include "../utils/SpatialGrid.h"

using namespace Forge;

#include <algorithm>
#include <cmath>

DisasterSystem::DisasterSystem()
    : m_volcano(std::make_unique<disasters::VolcanoDisaster>())
    , m_meteorImpact(std::make_unique<disasters::MeteorImpact>())
    , m_disease(std::make_unique<disasters::DiseaseOutbreak>())
    , m_iceAge(std::make_unique<disasters::IceAge>())
    , m_drought(std::make_unique<disasters::Drought>())
    , m_flood(std::make_unique<disasters::Flood>())
{
}

DisasterSystem::~DisasterSystem() = default;

void DisasterSystem::init(unsigned int seed) {
    m_rng.seed(seed);
    m_activeDisasters.clear();
    m_disasterHistory.clear();
    m_timeSinceLastDisaster = 0.0f;
    m_dayAccumulator = 0.0f;
}

void DisasterSystem::update(float deltaTime, SimulationOrchestrator& sim) {
    // Track time for random disaster checks
    m_timeSinceLastDisaster += deltaTime;
    m_dayAccumulator += deltaTime;

    // Check for natural disasters once per "day" (using 60 seconds as a day)
    const float dayLength = 60.0f;
    if (m_dayAccumulator >= dayLength) {
        m_dayAccumulator -= dayLength;
        if (m_randomDisastersEnabled) {
            checkNaturalDisasters(deltaTime);
        }
    }

    // Update all active disasters
    updateActiveDisasters(deltaTime, sim);

    // Clean up finished disasters
    cleanupFinishedDisasters();
}

void DisasterSystem::updateActiveDisasters(float deltaTime, SimulationOrchestrator& sim) {
    auto* creatures = sim.getCreatureManager();
    auto* terrain = sim.getTerrain();
    auto* climate = sim.getClimate();
    auto* weather = sim.getWeather();
    // Note: VegetationManager needs to be accessed through EcosystemManager or added to orchestrator
    VegetationManager* vegetation = nullptr;

    for (auto& disaster : m_activeDisasters) {
        if (!disaster.isActive) continue;

        disaster.elapsedTime += deltaTime;
        disaster.progress = std::min(1.0f, disaster.elapsedTime / disaster.duration);

        switch (disaster.type) {
            case DisasterType::VOLCANIC_ERUPTION:
                if (m_volcano && creatures && vegetation && terrain) {
                    m_volcano->update(deltaTime, *creatures, *vegetation, disaster);
                }
                break;

            case DisasterType::METEOR_IMPACT:
                if (m_meteorImpact && creatures && terrain && climate) {
                    m_meteorImpact->update(deltaTime, *creatures, *terrain, *climate, disaster);
                }
                break;

            case DisasterType::DISEASE_OUTBREAK:
                if (m_disease && creatures) {
                    m_disease->update(deltaTime, *creatures, disaster);
                }
                break;

            case DisasterType::ICE_AGE:
                if (m_iceAge && climate && vegetation && creatures) {
                    m_iceAge->update(deltaTime, *climate, *vegetation, *creatures, disaster);
                }
                break;

            case DisasterType::DROUGHT:
                if (m_drought && vegetation && creatures && terrain) {
                    m_drought->update(deltaTime, *vegetation, *creatures, *terrain, disaster);
                }
                break;

            case DisasterType::FLOOD:
                if (m_flood && creatures && terrain) {
                    m_flood->update(deltaTime, *creatures, *terrain, disaster);
                }
                break;

            default:
                break;
        }

        // Check if disaster has completed
        if (disaster.progress >= 1.0f) {
            disaster.isActive = false;
        }
    }
}

void DisasterSystem::checkNaturalDisasters(float deltaTime) {
    // Don't trigger if we're at max concurrent disasters
    if (getActiveDisasterCount() >= m_maxConcurrentDisasters) {
        return;
    }

    // Don't trigger if cooldown hasn't passed
    if (m_timeSinceLastDisaster < m_minDisasterCooldown) {
        return;
    }

    // Roll for disaster
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    if (dist(m_rng) < m_disasterProbability) {
        triggerRandomDisaster();
    }
}

void DisasterSystem::cleanupFinishedDisasters() {
    auto it = std::remove_if(m_activeDisasters.begin(), m_activeDisasters.end(),
        [this](const ActiveDisaster& disaster) {
            if (!disaster.isActive && disaster.progress >= 1.0f) {
                // Record the disaster before removing
                recordDisaster(disaster);
                return true;
            }
            return false;
        });
    m_activeDisasters.erase(it, m_activeDisasters.end());
}

void DisasterSystem::recordDisaster(const ActiveDisaster& disaster) {
    DisasterRecord record;
    record.type = disaster.type;
    record.severity = disaster.severity;
    record.epicenter = disaster.epicenter;
    record.simulationDay = m_dayAccumulator;
    record.totalKilled = disaster.creaturesKilled;
    record.totalAffected = disaster.creaturesAffected;
    record.duration = disaster.elapsedTime;
    record.summary = disaster.description + " - Killed: " + std::to_string(disaster.creaturesKilled);

    m_disasterHistory.push_back(record);

    // Trigger callback
    if (m_onDisasterEnd) {
        m_onDisasterEnd(record);
    }
}

void DisasterSystem::triggerDisaster(DisasterType type, const glm::vec3& epicenter,
                                     DisasterSeverity severity) {
    // Check if this type is already active (for some types, only one allowed)
    if (type == DisasterType::ICE_AGE || type == DisasterType::DROUGHT) {
        if (isDisasterTypeActive(type)) {
            return; // Don't stack global disasters
        }
    }

    ActiveDisaster disaster;
    disaster.type = type;
    disaster.severity = severity;
    disaster.epicenter = epicenter;
    disaster.radius = getBaseRadius(type, severity);
    disaster.duration = getBaseDuration(type, severity);
    disaster.progress = 0.0f;
    disaster.elapsedTime = 0.0f;
    disaster.creaturesAffected = 0;
    disaster.creaturesKilled = 0;
    disaster.vegetationDestroyed = 0;
    disaster.isActive = true;
    disaster.description = std::string(getDisasterTypeName(type)) + " (" +
                          getSeverityName(severity) + ")";

    // Initialize the specific disaster handler
    switch (type) {
        case DisasterType::VOLCANIC_ERUPTION:
            if (m_volcano) {
                m_volcano->trigger(epicenter, disaster.radius, severity);
            }
            break;

        case DisasterType::METEOR_IMPACT:
            if (m_meteorImpact) {
                m_meteorImpact->trigger(epicenter, disaster.radius, severity);
            }
            break;

        case DisasterType::DISEASE_OUTBREAK:
            if (m_disease) {
                m_disease->trigger(epicenter, severity);
            }
            break;

        case DisasterType::ICE_AGE:
            if (m_iceAge) {
                m_iceAge->trigger(severity);
            }
            break;

        case DisasterType::DROUGHT:
            if (m_drought) {
                m_drought->trigger(severity);
            }
            break;

        case DisasterType::FLOOD:
            if (m_flood) {
                m_flood->trigger(epicenter, severity);
            }
            break;

        default:
            break;
    }

    m_activeDisasters.push_back(disaster);
    m_timeSinceLastDisaster = 0.0f;

    // Trigger callback
    if (m_onDisasterStart) {
        m_onDisasterStart(disaster);
    }
}

void DisasterSystem::triggerRandomDisaster() {
    // Weighted random selection (some disasters are rarer)
    static const std::vector<std::pair<DisasterType, float>> disasterWeights = {
        { DisasterType::DROUGHT, 0.25f },
        { DisasterType::FLOOD, 0.20f },
        { DisasterType::DISEASE_OUTBREAK, 0.20f },
        { DisasterType::VOLCANIC_ERUPTION, 0.15f },
        { DisasterType::METEOR_IMPACT, 0.10f },
        { DisasterType::ICE_AGE, 0.10f }
    };

    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float roll = dist(m_rng);
    float cumulative = 0.0f;

    DisasterType selectedType = DisasterType::DROUGHT;
    for (const auto& [type, weight] : disasterWeights) {
        cumulative += weight;
        if (roll <= cumulative) {
            selectedType = type;
            break;
        }
    }

    // Random severity (biased towards moderate)
    static const std::vector<std::pair<DisasterSeverity, float>> severityWeights = {
        { DisasterSeverity::MINOR, 0.35f },
        { DisasterSeverity::MODERATE, 0.40f },
        { DisasterSeverity::MAJOR, 0.20f },
        { DisasterSeverity::CATASTROPHIC, 0.05f }
    };

    roll = dist(m_rng);
    cumulative = 0.0f;
    DisasterSeverity selectedSeverity = DisasterSeverity::MODERATE;
    for (const auto& [sev, weight] : severityWeights) {
        cumulative += weight;
        if (roll <= cumulative) {
            selectedSeverity = sev;
            break;
        }
    }

    // Random epicenter (for local disasters)
    std::uniform_real_distribution<float> posDist(-200.0f, 200.0f);
    glm::vec3 epicenter(posDist(m_rng), 0.0f, posDist(m_rng));

    triggerDisaster(selectedType, epicenter, selectedSeverity);
}

bool DisasterSystem::attemptNaturalDisaster() {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    if (dist(m_rng) < m_disasterProbability) {
        triggerRandomDisaster();
        return true;
    }
    return false;
}

void DisasterSystem::setDisasterProbability(float probability) {
    m_disasterProbability = std::clamp(probability, 0.0f, 1.0f);
}

bool DisasterSystem::hasActiveDisasters() const {
    return !m_activeDisasters.empty();
}

int DisasterSystem::getActiveDisasterCount() const {
    return static_cast<int>(std::count_if(m_activeDisasters.begin(), m_activeDisasters.end(),
        [](const ActiveDisaster& d) { return d.isActive; }));
}

bool DisasterSystem::isDisasterTypeActive(DisasterType type) const {
    return std::any_of(m_activeDisasters.begin(), m_activeDisasters.end(),
        [type](const ActiveDisaster& d) { return d.isActive && d.type == type; });
}

int DisasterSystem::getTotalHistoricalDeaths() const {
    int total = 0;
    for (const auto& record : m_disasterHistory) {
        total += record.totalKilled;
    }
    return total;
}

float DisasterSystem::getDangerLevel(const glm::vec3& position) const {
    float maxDanger = 0.0f;

    for (const auto& disaster : m_activeDisasters) {
        if (!disaster.isActive) continue;

        float distance = glm::length(position - disaster.epicenter);
        if (distance < disaster.radius) {
            // Danger decreases with distance from epicenter
            float normalizedDist = distance / disaster.radius;
            float danger = 1.0f - normalizedDist;

            // Amplify based on severity
            switch (disaster.severity) {
                case DisasterSeverity::MINOR: danger *= 0.5f; break;
                case DisasterSeverity::MODERATE: danger *= 0.75f; break;
                case DisasterSeverity::MAJOR: danger *= 1.0f; break;
                case DisasterSeverity::CATASTROPHIC: danger *= 1.5f; break;
            }

            maxDanger = std::max(maxDanger, danger);
        }
    }

    return std::clamp(maxDanger, 0.0f, 1.0f);
}

const char* DisasterSystem::getDisasterTypeName(DisasterType type) {
    switch (type) {
        case DisasterType::VOLCANIC_ERUPTION: return "Volcanic Eruption";
        case DisasterType::METEOR_IMPACT: return "Meteor Impact";
        case DisasterType::DISEASE_OUTBREAK: return "Disease Outbreak";
        case DisasterType::ICE_AGE: return "Ice Age";
        case DisasterType::DROUGHT: return "Drought";
        case DisasterType::FLOOD: return "Flood";
        case DisasterType::INVASIVE_SPECIES: return "Invasive Species";
        default: return "Unknown Disaster";
    }
}

const char* DisasterSystem::getDisasterTypeDescription(DisasterType type) {
    switch (type) {
        case DisasterType::VOLCANIC_ERUPTION:
            return "Molten lava and pyroclastic flows devastate the area. "
                   "Survivors must flee or face extreme heat damage.";
        case DisasterType::METEOR_IMPACT:
            return "A celestial body strikes the ground, creating a crater "
                   "and triggering a nuclear winter effect.";
        case DisasterType::DISEASE_OUTBREAK:
            return "A pathogen spreads through the population. Creatures with "
                   "stronger immune systems are more likely to survive.";
        case DisasterType::ICE_AGE:
            return "Global temperatures plummet, reducing plant growth and "
                   "favoring cold-adapted creatures.";
        case DisasterType::DROUGHT:
            return "Water becomes scarce, plants wither, and creatures must "
                   "adapt to survive the harsh conditions.";
        case DisasterType::FLOOD:
            return "Rising water levels threaten low-lying areas. Aquatic and "
                   "amphibious creatures thrive while others struggle.";
        case DisasterType::INVASIVE_SPECIES:
            return "A new species disrupts the ecosystem, competing for "
                   "resources and altering predator-prey dynamics.";
        default:
            return "An unknown catastrophe threatens the ecosystem.";
    }
}

const char* DisasterSystem::getSeverityName(DisasterSeverity severity) {
    switch (severity) {
        case DisasterSeverity::MINOR: return "Minor";
        case DisasterSeverity::MODERATE: return "Moderate";
        case DisasterSeverity::MAJOR: return "Major";
        case DisasterSeverity::CATASTROPHIC: return "Catastrophic";
        default: return "Unknown";
    }
}

void DisasterSystem::endAllDisasters() {
    for (auto& disaster : m_activeDisasters) {
        if (disaster.isActive) {
            disaster.isActive = false;
            disaster.progress = 1.0f;
            recordDisaster(disaster);
        }
    }
    m_activeDisasters.clear();

    // Reset specific disaster handlers
    if (m_volcano) m_volcano->reset();
    if (m_meteorImpact) m_meteorImpact->reset();
    if (m_disease) m_disease->reset();
    if (m_iceAge) m_iceAge->reset();
    if (m_drought) m_drought->reset();
    if (m_flood) m_flood->reset();
}

glm::vec3 DisasterSystem::findRandomEpicenter(DisasterType type, const Terrain& terrain) const {
    std::uniform_real_distribution<float> dist(-200.0f, 200.0f);

    for (int attempts = 0; attempts < 20; ++attempts) {
        glm::vec3 pos(dist(m_rng), 0.0f, dist(m_rng));

        float height = terrain.getHeight(pos.x, pos.z);
        pos.y = height;

        // Type-specific placement
        switch (type) {
            case DisasterType::VOLCANIC_ERUPTION:
                // Prefer higher terrain (mountains)
                if (height > 30.0f) return pos;
                break;

            case DisasterType::FLOOD:
                // Prefer lower terrain (valleys)
                if (height < 20.0f) return pos;
                break;

            default:
                return pos;
        }
    }

    // Fallback to random position
    return glm::vec3(dist(m_rng), 0.0f, dist(m_rng));
}

float DisasterSystem::getBaseDuration(DisasterType type, DisasterSeverity severity) const {
    float baseDuration = 60.0f; // 1 minute base

    // Type-specific durations
    switch (type) {
        case DisasterType::VOLCANIC_ERUPTION:
            baseDuration = 120.0f; // 2 minutes
            break;
        case DisasterType::METEOR_IMPACT:
            baseDuration = 180.0f; // 3 minutes (includes aftermath)
            break;
        case DisasterType::DISEASE_OUTBREAK:
            baseDuration = 240.0f; // 4 minutes
            break;
        case DisasterType::ICE_AGE:
            baseDuration = 600.0f; // 10 minutes
            break;
        case DisasterType::DROUGHT:
            baseDuration = 300.0f; // 5 minutes
            break;
        case DisasterType::FLOOD:
            baseDuration = 150.0f; // 2.5 minutes
            break;
        default:
            break;
    }

    // Severity multiplier
    switch (severity) {
        case DisasterSeverity::MINOR: return baseDuration * 0.5f;
        case DisasterSeverity::MODERATE: return baseDuration;
        case DisasterSeverity::MAJOR: return baseDuration * 1.5f;
        case DisasterSeverity::CATASTROPHIC: return baseDuration * 2.0f;
    }

    return baseDuration;
}

float DisasterSystem::getBaseRadius(DisasterType type, DisasterSeverity severity) const {
    float baseRadius = 50.0f;

    // Type-specific radii
    switch (type) {
        case DisasterType::VOLCANIC_ERUPTION:
            baseRadius = 60.0f;
            break;
        case DisasterType::METEOR_IMPACT:
            baseRadius = 40.0f; // Crater is smaller, but effects spread
            break;
        case DisasterType::DISEASE_OUTBREAK:
            baseRadius = 100.0f; // Can spread widely
            break;
        case DisasterType::ICE_AGE:
            baseRadius = 500.0f; // Global effect
            break;
        case DisasterType::DROUGHT:
            baseRadius = 200.0f; // Regional effect
            break;
        case DisasterType::FLOOD:
            baseRadius = 80.0f;
            break;
        default:
            break;
    }

    // Severity multiplier
    switch (severity) {
        case DisasterSeverity::MINOR: return baseRadius * 0.6f;
        case DisasterSeverity::MODERATE: return baseRadius;
        case DisasterSeverity::MAJOR: return baseRadius * 1.4f;
        case DisasterSeverity::CATASTROPHIC: return baseRadius * 2.0f;
    }

    return baseRadius;
}
