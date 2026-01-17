#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <string>
#include "../DisasterSystem.h"

// Forward declarations
namespace Forge {
    class CreatureManager;
}
class Creature;
struct ActiveDisaster;

using Forge::CreatureManager;

namespace disasters {

/**
 * @brief Disease infection state for a creature
 */
enum class InfectionState {
    HEALTHY,        // Not infected
    EXPOSED,        // Infected but not yet symptomatic
    SYMPTOMATIC,    // Showing symptoms, contagious
    CRITICAL,       // Severe symptoms, may die
    RECOVERING,     // Getting better
    RECOVERED,      // Immune
    DEAD            // Died from disease
};

/**
 * @brief Individual creature infection data
 */
struct InfectionData {
    uint32_t creatureId;
    InfectionState state;
    float infectionTime;        // Time since infection
    float incubationPeriod;     // Time before symptoms
    float severity;             // 0-1 how bad the case is
    float immuneResponse;       // Creature's resistance
    bool isContagious;
    int infectedOthers;         // Number this creature infected
};

/**
 * @brief Disease statistics for tracking
 */
struct DiseaseStats {
    int totalCases;
    int activeCases;
    int recovered;
    int dead;
    int susceptible;            // Creatures that can still be infected
    float rNaught;              // Reproduction number
    float mortalityRate;        // Actual death rate
    float peakInfected;         // Maximum simultaneous infections
    float daysSinceStart;
};

/**
 * @brief Disease strain characteristics
 */
struct DiseaseStrain {
    std::string name;
    float transmissionRate;     // Base chance of spread per contact
    float incubationMin;        // Minimum incubation (seconds)
    float incubationMax;        // Maximum incubation (seconds)
    float symptomDuration;      // How long symptoms last
    float baseMortalityRate;    // Base death rate
    float recoveryRate;         // Base recovery rate
    float mutationRate;         // Chance of strain mutation
    bool affectsYoung;          // Extra dangerous to young
    bool affectsOld;            // Extra dangerous to old
    float transmissionRange;    // How close for transmission
};

/**
 * @brief Disease outbreak disaster handler
 *
 * Simulates epidemic/pandemic with:
 * - Patient zero infection and spread
 * - Incubation period before symptoms
 * - Contact-based transmission
 * - Variable severity based on creature traits
 * - Immunity after recovery
 * - Natural selection for disease resistance
 *
 * Evolutionary pressure:
 * - Favors creatures with stronger immune systems
 * - Creates bottleneck selecting for resistance genes
 * - Survivors more likely to pass disease resistance
 */
class DiseaseOutbreak {
public:
    DiseaseOutbreak();
    ~DiseaseOutbreak() = default;

    /**
     * @brief Trigger a new disease outbreak
     * @param epicenter Starting location
     * @param severity How severe the disease is
     */
    void trigger(const glm::vec3& epicenter, DisasterSeverity severity);

    /**
     * @brief Manually infect a specific creature (patient zero)
     * @param patientZero First infected creature
     */
    void infectPatientZero(Creature* patientZero);

    /**
     * @brief Update the disease simulation
     */
    void update(float deltaTime, CreatureManager& creatures, ActiveDisaster& disaster);

    /**
     * @brief Reset to inactive state
     */
    void reset();

    // === Accessors ===
    bool isActive() const { return m_active; }
    const DiseaseStats& getStats() const { return m_stats; }
    const DiseaseStrain& getStrain() const { return m_strain; }

    /**
     * @brief Check if a creature is infected
     */
    bool isInfected(uint32_t creatureId) const;

    /**
     * @brief Check if a creature is immune
     */
    bool isImmune(uint32_t creatureId) const;

    /**
     * @brief Get infection state for a creature
     */
    InfectionState getInfectionState(uint32_t creatureId) const;

    /**
     * @brief Get list of all infected creature IDs
     */
    std::vector<uint32_t> getInfectedCreatures() const;

    // === Configuration ===
    void setTransmissionRate(float rate) { m_strain.transmissionRate = rate; }
    void setMortalityRate(float rate) { m_strain.baseMortalityRate = rate; }
    void setRecoveryRate(float rate) { m_strain.recoveryRate = rate; }

private:
    // === Internal Methods ===
    void initializeStrain(DisasterSeverity severity);
    void findPatientZero(CreatureManager& creatures);
    void infectCreature(Creature* creature);
    void updateInfections(float deltaTime, CreatureManager& creatures, ActiveDisaster& disaster);
    void attemptTransmission(const Creature& infected, CreatureManager& creatures,
                             float deltaTime, ActiveDisaster& disaster);
    void progressInfection(InfectionData& infection, Creature* creature,
                           float deltaTime, ActiveDisaster& disaster);

    float calculateSusceptibility(const Creature& creature) const;
    float calculateMortality(const InfectionData& infection, const Creature& creature) const;
    void updateStatistics(CreatureManager& creatures);

    // === State ===
    bool m_active = false;
    glm::vec3 m_epicenter;
    DiseaseStrain m_strain;
    DiseaseStats m_stats;
    float m_elapsedTime = 0.0f;

    // === Infection Tracking ===
    std::unordered_map<uint32_t, InfectionData> m_infections;
    std::unordered_set<uint32_t> m_immuneCreatures;

    // === Timing ===
    float m_lastTransmissionCheck = 0.0f;
    static constexpr float TRANSMISSION_CHECK_INTERVAL = 0.5f; // Check every 0.5s

    // === Random Generation ===
    std::mt19937 m_rng;
};

} // namespace disasters
