#include "Disease.h"
#include "../../core/CreatureManager.h"
#include "../../entities/Creature.h"

#include <algorithm>
#include <cmath>

using namespace Forge;

namespace disasters {

DiseaseOutbreak::DiseaseOutbreak() {
    m_infections.reserve(1000);
}

void DiseaseOutbreak::trigger(const glm::vec3& epicenter, DisasterSeverity severity) {
    m_active = true;
    m_epicenter = epicenter;
    m_elapsedTime = 0.0f;

    std::random_device rd;
    m_rng.seed(rd());

    // Clear previous state
    m_infections.clear();
    m_immuneCreatures.clear();

    // Initialize disease strain based on severity
    initializeStrain(severity);

    // Reset statistics
    m_stats = DiseaseStats{};
}

void DiseaseOutbreak::infectPatientZero(Creature* patientZero) {
    if (!patientZero || !patientZero->isAlive()) return;

    infectCreature(patientZero);
    m_epicenter = patientZero->getPosition();
}

void DiseaseOutbreak::initializeStrain(DisasterSeverity severity) {
    // Base strain properties
    m_strain.name = "Unknown Pathogen";
    m_strain.transmissionRange = 5.0f;
    m_strain.mutationRate = 0.01f;
    m_strain.affectsYoung = true;
    m_strain.affectsOld = true;

    switch (severity) {
        case DisasterSeverity::MINOR:
            m_strain.name = "Mild Flu";
            m_strain.transmissionRate = 0.15f;
            m_strain.incubationMin = 3.0f;
            m_strain.incubationMax = 8.0f;
            m_strain.symptomDuration = 15.0f;
            m_strain.baseMortalityRate = 0.02f;
            m_strain.recoveryRate = 0.1f;
            break;

        case DisasterSeverity::MODERATE:
            m_strain.name = "Viral Infection";
            m_strain.transmissionRate = 0.25f;
            m_strain.incubationMin = 5.0f;
            m_strain.incubationMax = 12.0f;
            m_strain.symptomDuration = 25.0f;
            m_strain.baseMortalityRate = 0.08f;
            m_strain.recoveryRate = 0.06f;
            break;

        case DisasterSeverity::MAJOR:
            m_strain.name = "Deadly Plague";
            m_strain.transmissionRate = 0.35f;
            m_strain.incubationMin = 2.0f;
            m_strain.incubationMax = 7.0f;
            m_strain.symptomDuration = 40.0f;
            m_strain.baseMortalityRate = 0.20f;
            m_strain.recoveryRate = 0.04f;
            break;

        case DisasterSeverity::CATASTROPHIC:
            m_strain.name = "Extinction Plague";
            m_strain.transmissionRate = 0.50f;
            m_strain.incubationMin = 1.0f;
            m_strain.incubationMax = 5.0f;
            m_strain.symptomDuration = 60.0f;
            m_strain.baseMortalityRate = 0.35f;
            m_strain.recoveryRate = 0.03f;
            break;
    }
}

void DiseaseOutbreak::findPatientZero(CreatureManager& creatures) {
    // Find a random creature near epicenter to be patient zero
    const auto& nearbyCreatures = creatures.queryNearby(m_epicenter, 50.0f);

    if (nearbyCreatures.empty()) {
        // No creatures nearby, expand search
        const auto& allCreatures = creatures.queryNearby(m_epicenter, 200.0f);
        if (!allCreatures.empty()) {
            std::uniform_int_distribution<size_t> dist(0, allCreatures.size() - 1);
            infectCreature(allCreatures[dist(m_rng)]);
        }
    } else {
        std::uniform_int_distribution<size_t> dist(0, nearbyCreatures.size() - 1);
        infectCreature(nearbyCreatures[dist(m_rng)]);
    }
}

void DiseaseOutbreak::infectCreature(Creature* creature) {
    if (!creature || !creature->isAlive()) return;

    uint32_t id = creature->getID();

    // Check if already infected or immune
    if (m_infections.find(id) != m_infections.end()) return;
    if (m_immuneCreatures.find(id) != m_immuneCreatures.end()) return;

    InfectionData infection;
    infection.creatureId = id;
    infection.state = InfectionState::EXPOSED;
    infection.infectionTime = 0.0f;

    // Random incubation period
    std::uniform_real_distribution<float> incubationDist(m_strain.incubationMin, m_strain.incubationMax);
    infection.incubationPeriod = incubationDist(m_rng);

    // Severity influenced by creature traits (would use genome data if available)
    std::uniform_real_distribution<float> severityDist(0.3f, 1.0f);
    infection.severity = severityDist(m_rng);

    // Immune response - some creatures naturally more resistant
    infection.immuneResponse = calculateSusceptibility(*creature);
    infection.isContagious = false;
    infection.infectedOthers = 0;

    m_infections[id] = infection;
    m_stats.totalCases++;
}

void DiseaseOutbreak::update(float deltaTime, CreatureManager& creatures,
                             ActiveDisaster& disaster) {
    if (!m_active) return;

    m_elapsedTime += deltaTime;

    // Find patient zero if we don't have any infections yet
    if (m_infections.empty()) {
        findPatientZero(creatures);
    }

    // Update all infections
    updateInfections(deltaTime, creatures, disaster);

    // Attempt transmission periodically (not every frame for performance)
    m_lastTransmissionCheck += deltaTime;
    if (m_lastTransmissionCheck >= TRANSMISSION_CHECK_INTERVAL) {
        for (auto& [id, infection] : m_infections) {
            if (!infection.isContagious) continue;

            Creature* creature = creatures.getCreatureByID(id);
            if (creature && creature->isAlive()) {
                attemptTransmission(*creature, creatures, m_lastTransmissionCheck, disaster);
            }
        }
        m_lastTransmissionCheck = 0.0f;
    }

    // Update statistics
    updateStatistics(creatures);

    // Update disaster info
    disaster.creaturesAffected = m_stats.totalCases;
    disaster.creaturesKilled = m_stats.dead;
    disaster.description = m_strain.name + " - Active: " + std::to_string(m_stats.activeCases) +
                          " Dead: " + std::to_string(m_stats.dead) +
                          " R0: " + std::to_string(static_cast<int>(m_stats.rNaught * 10) / 10.0f);

    // Disease ends when no more active cases
    if (m_stats.activeCases == 0 && m_stats.totalCases > 0) {
        disaster.progress = 1.0f;
        m_active = false;
    }
}

void DiseaseOutbreak::updateInfections(float deltaTime, CreatureManager& creatures,
                                        ActiveDisaster& disaster) {
    std::vector<uint32_t> toRemove;

    for (auto& [id, infection] : m_infections) {
        // Skip dead or recovered
        if (infection.state == InfectionState::DEAD ||
            infection.state == InfectionState::RECOVERED) {
            continue;
        }

        Creature* creature = creatures.getCreatureByID(id);
        if (!creature || !creature->isAlive()) {
            // Creature died from other causes
            infection.state = InfectionState::DEAD;
            continue;
        }

        progressInfection(infection, creature, deltaTime, disaster);
    }

    // Move recovered to immune list
    for (auto it = m_infections.begin(); it != m_infections.end(); ) {
        if (it->second.state == InfectionState::RECOVERED) {
            m_immuneCreatures.insert(it->first);
            it = m_infections.erase(it);
        } else if (it->second.state == InfectionState::DEAD) {
            it = m_infections.erase(it);
        } else {
            ++it;
        }
    }
}

void DiseaseOutbreak::progressInfection(InfectionData& infection, Creature* creature,
                                         float deltaTime, ActiveDisaster& disaster) {
    infection.infectionTime += deltaTime;

    switch (infection.state) {
        case InfectionState::EXPOSED:
            // Incubation period - not yet symptomatic
            if (infection.infectionTime >= infection.incubationPeriod) {
                infection.state = InfectionState::SYMPTOMATIC;
                infection.isContagious = true;
                infection.infectionTime = 0.0f; // Reset for symptom phase
            }
            break;

        case InfectionState::SYMPTOMATIC:
            {
                // Gradual health drain
                float healthDrain = infection.severity * 2.0f * deltaTime;
                creature->takeDamage(healthDrain);

                // Check for progression to critical
                float criticalChance = infection.severity * 0.01f * deltaTime;
                std::uniform_real_distribution<float> dist(0.0f, 1.0f);
                if (dist(m_rng) < criticalChance) {
                    infection.state = InfectionState::CRITICAL;
                    infection.infectionTime = 0.0f;
                }

                // Check for recovery
                float recoveryChance = m_strain.recoveryRate * infection.immuneResponse * deltaTime;
                if (dist(m_rng) < recoveryChance) {
                    infection.state = InfectionState::RECOVERING;
                    infection.infectionTime = 0.0f;
                }

                // Duration check
                if (infection.infectionTime >= m_strain.symptomDuration) {
                    infection.state = InfectionState::RECOVERING;
                    infection.infectionTime = 0.0f;
                }
            }
            break;

        case InfectionState::CRITICAL:
            {
                // Severe health drain
                float healthDrain = infection.severity * 5.0f * deltaTime;
                creature->takeDamage(healthDrain);

                // Mortality check
                float mortalityChance = calculateMortality(infection, *creature) * deltaTime;
                std::uniform_real_distribution<float> dist(0.0f, 1.0f);

                if (!creature->isAlive() || dist(m_rng) < mortalityChance) {
                    infection.state = InfectionState::DEAD;
                    infection.isContagious = false;
                    m_stats.dead++;
                    disaster.creaturesKilled++;
                    // Actually kill the creature
                    creature->takeDamage(1000.0f); // Ensure death
                } else if (dist(m_rng) < m_strain.recoveryRate * 0.5f * infection.immuneResponse) {
                    // Can still recover from critical
                    infection.state = InfectionState::RECOVERING;
                    infection.infectionTime = 0.0f;
                }
            }
            break;

        case InfectionState::RECOVERING:
            {
                // Gradual recovery
                infection.severity -= deltaTime * 0.1f;
                infection.isContagious = false; // No longer spreading

                if (infection.severity <= 0.0f || infection.infectionTime > 10.0f) {
                    infection.state = InfectionState::RECOVERED;
                    m_stats.recovered++;
                }
            }
            break;

        default:
            break;
    }
}

void DiseaseOutbreak::attemptTransmission(const Creature& infected, CreatureManager& creatures,
                                           float deltaTime, ActiveDisaster& disaster) {
    auto it = m_infections.find(infected.getID());
    if (it == m_infections.end() || !it->second.isContagious) return;

    const auto& nearbyCreatures = creatures.queryNearby(infected.getPosition(),
                                                         m_strain.transmissionRange);

    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (auto* nearby : nearbyCreatures) {
        if (!nearby || nearby == &infected || !nearby->isAlive()) continue;

        uint32_t nearbyId = nearby->getID();

        // Skip already infected or immune
        if (m_infections.find(nearbyId) != m_infections.end()) continue;
        if (m_immuneCreatures.find(nearbyId) != m_immuneCreatures.end()) continue;

        // Calculate transmission chance
        float susceptibility = calculateSusceptibility(*nearby);
        float transmissionChance = m_strain.transmissionRate * susceptibility * deltaTime;

        if (dist(m_rng) < transmissionChance) {
            infectCreature(nearby);
            it->second.infectedOthers++;
            disaster.creaturesAffected++;
        }
    }
}

float DiseaseOutbreak::calculateSusceptibility(const Creature& creature) const {
    // Base susceptibility
    float susceptibility = 1.0f;

    // Age effects (would use actual age if available)
    float energy = creature.getEnergy();
    if (energy < 30.0f) {
        // Low energy = weakened immune system
        susceptibility *= 1.3f;
    } else if (energy > 80.0f) {
        // High energy = stronger immune system
        susceptibility *= 0.8f;
    }

    // Species variation (random but consistent per creature based on ID)
    uint32_t id = creature.getId();
    float geneticFactor = 0.7f + (static_cast<float>(id % 100) / 100.0f) * 0.6f;
    susceptibility *= geneticFactor;

    return std::clamp(susceptibility, 0.3f, 2.0f);
}

float DiseaseOutbreak::calculateMortality(const InfectionData& infection,
                                           const Creature& creature) const {
    float mortality = m_strain.baseMortalityRate;

    // Severity amplifies mortality
    mortality *= infection.severity;

    // Poor immune response increases mortality
    mortality *= (2.0f - infection.immuneResponse);

    // Energy state matters
    float energy = creature.getEnergy();
    if (energy < 20.0f) {
        mortality *= 1.5f;
    }

    return std::clamp(mortality, 0.01f, 0.8f);
}

void DiseaseOutbreak::updateStatistics(CreatureManager& creatures) {
    m_stats.activeCases = 0;

    int totalInfected = 0;
    int totalInfectedOthers = 0;

    for (const auto& [id, infection] : m_infections) {
        if (infection.state != InfectionState::DEAD &&
            infection.state != InfectionState::RECOVERED) {
            m_stats.activeCases++;
        }
        totalInfected++;
        totalInfectedOthers += infection.infectedOthers;
    }

    // Calculate R0 (average infections per case)
    if (totalInfected > 0) {
        m_stats.rNaught = static_cast<float>(totalInfectedOthers) / totalInfected;
    }

    // Track peak
    m_stats.peakInfected = std::max(m_stats.peakInfected,
                                     static_cast<float>(m_stats.activeCases));

    // Calculate actual mortality rate
    if (m_stats.totalCases > 0) {
        m_stats.mortalityRate = static_cast<float>(m_stats.dead) / m_stats.totalCases;
    }

    m_stats.daysSinceStart = m_elapsedTime / 60.0f; // Assuming 60s per day
}

void DiseaseOutbreak::reset() {
    m_active = false;
    m_infections.clear();
    m_immuneCreatures.clear();
    m_stats = DiseaseStats{};
    m_elapsedTime = 0.0f;
}

bool DiseaseOutbreak::isInfected(uint32_t creatureId) const {
    auto it = m_infections.find(creatureId);
    if (it == m_infections.end()) return false;
    return it->second.state != InfectionState::RECOVERED &&
           it->second.state != InfectionState::DEAD;
}

bool DiseaseOutbreak::isImmune(uint32_t creatureId) const {
    return m_immuneCreatures.find(creatureId) != m_immuneCreatures.end();
}

InfectionState DiseaseOutbreak::getInfectionState(uint32_t creatureId) const {
    auto it = m_infections.find(creatureId);
    if (it == m_infections.end()) {
        if (isImmune(creatureId)) {
            return InfectionState::RECOVERED;
        }
        return InfectionState::HEALTHY;
    }
    return it->second.state;
}

std::vector<uint32_t> DiseaseOutbreak::getInfectedCreatures() const {
    std::vector<uint32_t> result;
    for (const auto& [id, infection] : m_infections) {
        if (infection.state != InfectionState::DEAD &&
            infection.state != InfectionState::RECOVERED) {
            result.push_back(id);
        }
    }
    return result;
}

} // namespace disasters
