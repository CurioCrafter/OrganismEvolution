#pragma once

#include "NeuralNetwork.h"
#include "NEATGenome.h"
#include <vector>
#include <memory>
#include <random>
#include <cmath>
#include <array>

namespace ai {

// ============================================================================
// Forward Declarations
// ============================================================================

class CreatureBrain;

// ============================================================================
// Sensory Input Structure
// ============================================================================

struct SensoryInput {
    // Vision - detected entities (8 inputs)
    float nearestFoodDistance = 1.0f;    // Normalized [0,1], 1 = at max vision range
    float nearestFoodAngle = 0.0f;       // Normalized [-1,1], angle relative to heading
    float nearestPredatorDistance = 1.0f;
    float nearestPredatorAngle = 0.0f;
    float nearestPreyDistance = 1.0f;    // For carnivores
    float nearestPreyAngle = 0.0f;
    float nearestAllyDistance = 1.0f;    // Same species
    float nearestAllyAngle = 0.0f;

    // Internal state (5 inputs)
    float energy = 1.0f;                 // Normalized [0,1]
    float health = 1.0f;                 // Normalized [0,1]
    float age = 0.0f;                    // Normalized by expected lifespan
    float currentSpeed = 0.0f;           // Current speed normalized to max
    float hungerLevel = 0.0f;            // How hungry (1-energy/maxEnergy)

    // Environmental (4 inputs)
    float terrainHeight = 0.0f;          // Local terrain height
    float waterProximity = 1.0f;         // Distance to water [0,1]
    float temperature = 0.5f;            // Environmental temperature normalized
    float dayNightCycle = 0.5f;          // Time of day (0=midnight, 0.5=noon, 1=midnight)

    // Social/situational awareness (4 inputs)
    float nearbyCreatureCount = 0.0f;    // Normalized count of nearby creatures
    float nearbyPredatorCount = 0.0f;    // Number of predators in range
    float nearbyPreyCount = 0.0f;        // Number of prey in range
    float nearbyAllyCount = 0.0f;        // Number of allies (same species) nearby

    // Memory/state inputs (4 inputs)
    float wasAttacked = 0.0f;            // Recently took damage
    float recentFoodEaten = 0.0f;        // Recently ate food
    float fear = 0.0f;                   // Current fear level [0,1]
    float timeSinceLastMeal = 0.0f;      // Normalized time since eating

    // Mate detection (2 inputs) - for reproduction decisions
    float nearestMateDistance = 1.0f;    // Distance to nearest potential mate
    float nearestMateAngle = 0.0f;       // Angle to nearest potential mate

    // Convert to vector for network input
    std::vector<float> toVector() const {
        return {
            // Vision (8)
            nearestFoodDistance, nearestFoodAngle,
            nearestPredatorDistance, nearestPredatorAngle,
            nearestPreyDistance, nearestPreyAngle,
            nearestAllyDistance, nearestAllyAngle,
            // Internal state (5)
            energy, health, age, currentSpeed, hungerLevel,
            // Environmental (4)
            terrainHeight, waterProximity, temperature, dayNightCycle,
            // Social (4)
            nearbyCreatureCount, nearbyPredatorCount, nearbyPreyCount, nearbyAllyCount,
            // Memory (4)
            wasAttacked, recentFoodEaten, fear, timeSinceLastMeal,
            // Mate (2)
            nearestMateDistance, nearestMateAngle
        };
    }

    static constexpr int size() { return 27; }
};

// ============================================================================
// Motor Output Structure
// ============================================================================

struct MotorOutput {
    // Movement control (3 outputs)
    float turnAngle = 0.0f;      // [-1, 1] -> [-PI, PI] - primary direction
    float speed = 0.0f;          // [0, 1] -> fraction of max speed
    float movementDirectionX = 0.0f;  // [-1, 1] -> direct X component of desired movement

    // Action intents (5 outputs)
    float attackIntent = 0.0f;   // [0, 1] -> threshold for attacking
    float fleeIntent = 0.0f;     // [0, 1] -> urgency of fleeing
    float eatIntent = 0.0f;      // [0, 1] -> priority of eating when food nearby
    float mateIntent = 0.0f;     // [0, 1] -> desire to reproduce
    float restIntent = 0.0f;     // [0, 1] -> desire to rest/conserve energy

    // Social behavior (2 outputs)
    float socialAttraction = 0.0f;  // [-1, 1] -> attract to allies (+) or go solo (-)
    float aggressionLevel = 0.0f;   // [0, 1] -> overall aggression/boldness

    static MotorOutput fromVector(const std::vector<float>& v) {
        MotorOutput out;
        // Movement (3)
        if (v.size() > 0) out.turnAngle = std::tanh(v[0]);
        if (v.size() > 1) out.speed = (std::tanh(v[1]) + 1.0f) * 0.5f;
        if (v.size() > 2) out.movementDirectionX = std::tanh(v[2]);
        // Actions (5)
        if (v.size() > 3) out.attackIntent = (std::tanh(v[3]) + 1.0f) * 0.5f;
        if (v.size() > 4) out.fleeIntent = (std::tanh(v[4]) + 1.0f) * 0.5f;
        if (v.size() > 5) out.eatIntent = (std::tanh(v[5]) + 1.0f) * 0.5f;
        if (v.size() > 6) out.mateIntent = (std::tanh(v[6]) + 1.0f) * 0.5f;
        if (v.size() > 7) out.restIntent = (std::tanh(v[7]) + 1.0f) * 0.5f;
        // Social (2)
        if (v.size() > 8) out.socialAttraction = std::tanh(v[8]);
        if (v.size() > 9) out.aggressionLevel = (std::tanh(v[9]) + 1.0f) * 0.5f;
        return out;
    }

    static constexpr int size() { return 10; }
};

// ============================================================================
// Neuromodulator State
// ============================================================================

struct NeuromodulatorState {
    float dopamine = 0.0f;       // Reward/pleasure signal
    float norepinephrine = 0.5f; // Arousal/alertness
    float serotonin = 0.5f;      // Mood/patience
    float acetylcholine = 0.5f;  // Attention/learning rate

    // Homeostatic baselines
    float dopamineBaseline = 0.0f;
    float arousalBaseline = 0.5f;

    // Update neuromodulators based on events
    void onFoodEaten(float amount = 1.0f) {
        dopamine = std::min(1.0f, dopamine + 0.5f * amount);
        serotonin = std::min(1.0f, serotonin + 0.1f);
    }

    void onDamageTaken(float amount = 1.0f) {
        dopamine = std::max(-1.0f, dopamine - 0.3f * amount);
        norepinephrine = std::min(1.0f, norepinephrine + 0.4f);
    }

    void onThreatDetected(float threatLevel) {
        norepinephrine = std::min(1.0f, norepinephrine + 0.3f * threatLevel);
        acetylcholine = std::min(1.0f, acetylcholine + 0.2f * threatLevel);
    }

    void onSuccessfulHunt() {
        dopamine = std::min(1.0f, dopamine + 0.8f);
        serotonin = std::min(1.0f, serotonin + 0.2f);
    }

    void onNovelty(float noveltyLevel) {
        acetylcholine = std::min(1.0f, acetylcholine + 0.2f * noveltyLevel);
        dopamine += 0.1f * noveltyLevel;  // Curiosity reward
    }

    void decay(float dt) {
        // Decay towards baselines
        float decayRate = 2.0f * dt;
        dopamine += (dopamineBaseline - dopamine) * decayRate;
        norepinephrine += (arousalBaseline - norepinephrine) * decayRate;
        serotonin += (0.5f - serotonin) * decayRate * 0.5f;
        acetylcholine += (0.5f - acetylcholine) * decayRate;

        // Clamp
        dopamine = std::max(-1.0f, std::min(1.0f, dopamine));
        norepinephrine = std::max(0.0f, std::min(1.0f, norepinephrine));
        serotonin = std::max(0.0f, std::min(1.0f, serotonin));
        acetylcholine = std::max(0.0f, std::min(1.0f, acetylcholine));
    }

    // Get effective learning rate based on neuromodulator state
    float getLearningRate() const {
        // High acetylcholine = higher learning rate
        // High norepinephrine = focused learning
        return 0.01f * (0.5f + acetylcholine) * (0.5f + 0.5f * norepinephrine);
    }

    // Get reward signal for plasticity
    float getRewardSignal() const {
        return dopamine;
    }
};

// ============================================================================
// Working Memory Module
// ============================================================================

class WorkingMemory {
public:
    static constexpr int MEMORY_SIZE = 8;
    static constexpr int CONTEXT_SIZE = 4;

    WorkingMemory() {
        m_memory.fill(0.0f);
        m_context.fill(0.0f);
    }

    void update(const std::vector<float>& input, float gateStrength) {
        // Gated memory update
        // gateStrength [0,1] determines how much new info overwrites old
        for (size_t i = 0; i < std::min(input.size(), m_memory.size()); i++) {
            m_memory[i] = (1.0f - gateStrength) * m_memory[i] + gateStrength * input[i];
        }
    }

    void setContext(const std::array<float, CONTEXT_SIZE>& ctx) {
        m_context = ctx;
    }

    std::vector<float> read() const {
        std::vector<float> out(m_memory.begin(), m_memory.end());
        out.insert(out.end(), m_context.begin(), m_context.end());
        return out;
    }

    void reset() {
        m_memory.fill(0.0f);
        m_context.fill(0.0f);
    }

    // Decay memory over time (forgetting)
    void decay(float rate) {
        for (auto& m : m_memory) {
            m *= (1.0f - rate);
        }
    }

private:
    std::array<float, MEMORY_SIZE> m_memory;
    std::array<float, CONTEXT_SIZE> m_context;
};

// ============================================================================
// Sensory Processor Module
// ============================================================================

class SensoryProcessor {
public:
    static constexpr int INPUT_SIZE = SensoryInput::size();
    static constexpr int OUTPUT_SIZE = 8;

    SensoryProcessor();

    void initialize(std::mt19937& rng);
    std::vector<float> process(const SensoryInput& input);

    // For evolution
    std::vector<float> getWeights() const;
    void setWeights(const std::vector<float>& weights);
    size_t getWeightCount() const;

    void reset() { m_network.reset(); }

private:
    NeuralNetwork m_network;
};

// ============================================================================
// Emotional Module (Drives and Motivations)
// ============================================================================

class EmotionalModule {
public:
    static constexpr int INPUT_SIZE = 10;  // Processed sensory + internal
    static constexpr int OUTPUT_SIZE = 4;  // Fear, hunger, curiosity, social

    EmotionalModule();

    void initialize(std::mt19937& rng);

    struct Drives {
        float fear = 0.0f;       // [0,1] flee motivation
        float hunger = 0.0f;     // [0,1] food seeking motivation
        float curiosity = 0.0f;  // [0,1] exploration motivation
        float social = 0.0f;     // [0,1] approach allies motivation

        std::vector<float> toVector() const {
            return {fear, hunger, curiosity, social};
        }
    };

    Drives process(const std::vector<float>& sensoryFeatures,
                   float energy, float health,
                   const NeuromodulatorState& modulators);

    // For evolution
    std::vector<float> getWeights() const;
    void setWeights(const std::vector<float>& weights);
    size_t getWeightCount() const;

    void reset() { m_network.reset(); }

private:
    NeuralNetwork m_network;
    Drives m_currentDrives;
};

// ============================================================================
// Decision Maker Module (Central Executive)
// ============================================================================

class DecisionMaker {
public:
    // Inputs: sensory features + memory + drives + modulators
    static constexpr int INPUT_SIZE = 8 + 12 + 4 + 4;  // 28
    static constexpr int HIDDEN_SIZE = 16;
    static constexpr int OUTPUT_SIZE = 8;  // Action values

    DecisionMaker();

    void initialize(std::mt19937& rng);

    std::vector<float> decide(const std::vector<float>& sensoryFeatures,
                               const std::vector<float>& memoryState,
                               const EmotionalModule::Drives& drives,
                               const NeuromodulatorState& modulators);

    // Plasticity
    void updatePlasticity(float reward, float learningRate);
    void accumulateEligibility();

    // For evolution
    std::vector<float> getWeights() const;
    void setWeights(const std::vector<float>& weights);
    size_t getWeightCount() const;

    void reset() { m_network.reset(); }

private:
    NeuralNetwork m_network;
    std::vector<float> m_lastOutput;
};

// ============================================================================
// Motor Controller Module
// ============================================================================

class MotorController {
public:
    static constexpr int INPUT_SIZE = 8;   // Decision outputs
    static constexpr int OUTPUT_SIZE = MotorOutput::size();

    MotorController();

    void initialize(std::mt19937& rng);

    MotorOutput generateMotorCommands(const std::vector<float>& decisionOutput);

    // For evolution
    std::vector<float> getWeights() const;
    void setWeights(const std::vector<float>& weights);
    size_t getWeightCount() const;

    void reset() { m_network.reset(); }

private:
    NeuralNetwork m_network;
};

// ============================================================================
// Complete Creature Brain
// ============================================================================

class CreatureBrain {
public:
    CreatureBrain();

    void initialize(std::mt19937& rng);
    void initializeFromGenome(const NEATGenome& genome);

    // Main processing pipeline
    MotorOutput process(const SensoryInput& input, float deltaTime);

    // Event handlers (update neuromodulators)
    void onFoodEaten(float amount = 1.0f);
    void onDamageTaken(float amount = 1.0f);
    void onThreatDetected(float level);
    void onSuccessfulHunt();

    // Learning
    void learn(float reward);

    // Reset all state (new episode)
    void reset();

    // Access modules for evolution/inspection
    SensoryProcessor& getSensoryProcessor() { return m_sensory; }
    EmotionalModule& getEmotionalModule() { return m_emotional; }
    DecisionMaker& getDecisionMaker() { return m_decision; }
    MotorController& getMotorController() { return m_motor; }
    WorkingMemory& getWorkingMemory() { return m_memory; }
    NeuromodulatorState& getNeuromodulators() { return m_modulators; }

    const NeuromodulatorState& getNeuromodulators() const { return m_modulators; }
    const EmotionalModule::Drives& getCurrentDrives() const { return m_currentDrives; }

    // Serialization for evolution
    std::vector<float> getAllWeights() const;
    void setAllWeights(const std::vector<float>& weights);
    size_t getTotalWeightCount() const;

    // Statistics
    float getAverageActivity() const;
    float getComplexity() const;

    // NEAT genome for full topology evolution
    void setNEATGenome(const NEATGenome& genome);
    const NEATGenome& getNEATGenome() const { return m_genome; }
    NEATGenome& getNEATGenome() { return m_genome; }
    bool usesNEATGenome() const { return m_useNEATGenome; }

private:
    // Modules
    SensoryProcessor m_sensory;
    EmotionalModule m_emotional;
    DecisionMaker m_decision;
    MotorController m_motor;
    WorkingMemory m_memory;

    // State
    NeuromodulatorState m_modulators;
    EmotionalModule::Drives m_currentDrives;
    std::vector<float> m_lastDecision;

    // NEAT genome for full brain evolution
    NEATGenome m_genome;
    std::unique_ptr<NeuralNetwork> m_neatNetwork;
    bool m_useNEATGenome = false;

    // Internal
    float m_accumulatedReward = 0.0f;
    int m_stepsSinceLearn = 0;
};

} // namespace ai
