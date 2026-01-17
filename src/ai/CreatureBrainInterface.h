#pragma once

#include "BrainModules.h"
#include "NEATGenome.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <random>

namespace ai {

// Forward declaration
class CreatureBrain;

/**
 * CreatureBrainInterface - Bridge between the AI system and existing Creature class
 *
 * This interface allows creatures to use either:
 * 1. The new NEAT-evolved neural network brain
 * 2. The modular brain architecture
 * 3. Legacy steering behaviors (for comparison)
 */
class CreatureBrainInterface {
public:
    enum class BrainType {
        LEGACY_STEERING,    // Original hardcoded behaviors (default)
        MODULAR_BRAIN,      // Separate modules for sensory, emotional, decision, motor
        NEAT_EVOLVED        // NEAT-evolved topology
    };

    CreatureBrainInterface();
    ~CreatureBrainInterface() = default;

    // Initialize brain of specified type
    void initialize(BrainType type, int inputSize = SensoryInput::size(),
                    int outputSize = MotorOutput::size());

    // Initialize from NEAT genome
    void initializeFromGenome(const NEATGenome& genome);

    // Main processing - takes world state, returns motor commands (LEGACY - 15 inputs)
    MotorOutput process(
        // Vision inputs - normalized distances and angles
        float nearestFoodDist, float nearestFoodAngle,
        float nearestPredatorDist, float nearestPredatorAngle,
        float nearestPreyDist, float nearestPreyAngle,
        float nearestAllyDist, float nearestAllyAngle,
        // Internal state
        float energy, float health, float age,
        // Environmental
        float terrainHeight, float waterProximity,
        // Recent events
        bool wasRecentlyAttacked, bool recentlyAteFood,
        // Delta time for updates
        float deltaTime
    );

    // NEW: Process with expanded SensoryInput (27 inputs) -> MotorOutput (10 outputs)
    // This is the PRIMARY method for neural-driven behavior
    MotorOutput processExpanded(const SensoryInput& input, float deltaTime);

    // Convert to/from glm vectors for existing creature code
    struct MovementCommand {
        float turnAngle;    // Radians, relative to current heading
        float speed;        // 0-1, fraction of max speed
        float attackIntent; // 0-1, likelihood of attacking if able
        float fleeIntent;   // 0-1, urgency of fleeing

        glm::vec3 toVelocityDelta(const glm::vec3& currentVelocity, float maxSpeed) const;
    };

    MovementCommand getMovementCommand() const;

    // Event handlers - call these to update internal state
    void onFoodEaten(float amount = 1.0f);
    void onDamageTaken(float amount = 1.0f);
    void onThreatDetected(float level = 1.0f);
    void onSuccessfulHunt();
    void onSuccessfulEscape();  // Escaped from predator

    // Learning - call periodically with reward signal
    void learn(float reward);

    // Reset all state (for new episode/generation)
    void reset();

    // Access internal state for display/debugging
    const NeuromodulatorState& getNeuromodulators() const;
    const EmotionalModule::Drives& getDrives() const;
    float getBrainActivity() const;
    float getComplexity() const;

    // Get/set genome for evolution
    NEATGenome& getGenome();
    const NEATGenome& getGenome() const;
    void setGenome(const NEATGenome& genome);

    // Get weights as flat vector (for legacy compatibility)
    std::vector<float> getWeights() const;
    void setWeights(const std::vector<float>& weights);
    size_t getWeightCount() const;

    BrainType getBrainType() const { return m_brainType; }

    // Statistics tracking
    struct Statistics {
        float totalReward = 0.0f;
        int learningEvents = 0;
        float averageActivity = 0.0f;
        int decisionsMode = 0;
        float fearTime = 0.0f;
    };

    const Statistics& getStatistics() const { return m_stats; }

private:
    BrainType m_brainType = BrainType::LEGACY_STEERING;
    std::unique_ptr<CreatureBrain> m_brain;
    NEATGenome m_genome;

    // Cached outputs
    MotorOutput m_lastOutput;
    SensoryInput m_lastInput;

    // Statistics
    Statistics m_stats;

    // Internal helpers
    SensoryInput buildSensoryInput(
        float nearestFoodDist, float nearestFoodAngle,
        float nearestPredatorDist, float nearestPredatorAngle,
        float nearestPreyDist, float nearestPreyAngle,
        float nearestAllyDist, float nearestAllyAngle,
        float energy, float health, float age,
        float terrainHeight, float waterProximity,
        bool wasRecentlyAttacked, bool recentlyAteFood
    );
};

// ============================================================================
// Evolution Manager - Handles population-level NEAT evolution
// ============================================================================

class BrainEvolutionManager {
public:
    BrainEvolutionManager(int populationSize, int inputSize, int outputSize);

    // Create a new brain for a creature
    std::unique_ptr<CreatureBrainInterface> createBrain();

    // Create offspring brain from parent
    std::unique_ptr<CreatureBrainInterface> createOffspringBrain(
        const CreatureBrainInterface& parent);

    // Create offspring brain from two parents (crossover)
    std::unique_ptr<CreatureBrainInterface> createOffspringBrain(
        const CreatureBrainInterface& parent1,
        const CreatureBrainInterface& parent2);

    // Report fitness for a brain (by creature ID or genome)
    void reportFitness(const NEATGenome& genome, float fitness);

    // Advance to next generation (selection + reproduction)
    void evolveGeneration();

    // Get best performing genome
    const NEATGenome& getBestGenome() const;

    // Statistics
    int getGeneration() const { return m_generation; }
    int getSpeciesCount() const { return static_cast<int>(m_species.size()); }
    float getAverageFitness() const;
    float getBestFitness() const;
    float getAverageComplexity() const;

    // Configure evolution parameters
    MutationParams& getMutationParams() { return m_mutationParams; }
    void setCompatibilityThreshold(float thresh) { m_compatibilityThreshold = thresh; }

private:
    int m_populationSize;
    int m_inputSize;
    int m_outputSize;
    int m_generation = 0;

    std::vector<NEATGenome> m_genomes;
    std::vector<Species> m_species;
    NEATGenome m_bestGenome;
    float m_bestFitness = 0.0f;

    MutationParams m_mutationParams;
    float m_compatibilityThreshold = 3.0f;

    std::mt19937 m_rng;

    void speciate();
    NEATGenome reproduce(Species& species);
};

// ============================================================================
// Inline Implementations
// ============================================================================

inline glm::vec3 CreatureBrainInterface::MovementCommand::toVelocityDelta(
    const glm::vec3& currentVelocity, float maxSpeed) const {

    // Calculate desired direction
    float currentAngle = atan2(currentVelocity.z, currentVelocity.x);
    float desiredAngle = currentAngle + turnAngle * 3.14159f;

    // Calculate desired speed
    float desiredSpeed = speed * maxSpeed;

    // Create velocity vector
    glm::vec3 desired(
        cos(desiredAngle) * desiredSpeed,
        0.0f,
        sin(desiredAngle) * desiredSpeed
    );

    return desired - currentVelocity;
}

} // namespace ai
