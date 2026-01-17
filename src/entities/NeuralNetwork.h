#pragma once

#include <vector>

// Neural network outputs for behavior modulation
struct NeuralOutputs {
    // Movement outputs (original)
    float turnAngle;        // -1 to 1, scaled to -PI to PI
    float speedMultiplier;  // 0 to 1

    // Behavior modulation outputs (NEW - enable evolution)
    float aggressionMod;    // -1 to 1: negative = more passive, positive = more aggressive
    float fearMod;          // -1 to 1: negative = braver, positive = more fearful
    float socialMod;        // -1 to 1: negative = solitary, positive = more social/herding
    float explorationMod;   // -1 to 1: negative = stay near, positive = explore more

    NeuralOutputs() : turnAngle(0.0f), speedMultiplier(0.5f),
                      aggressionMod(0.0f), fearMod(0.0f),
                      socialMod(0.0f), explorationMod(0.0f) {}
};

// Extended outputs for aquatic creatures
struct AquaticNeuralOutputs : public NeuralOutputs {
    // Vertical movement
    float depthChange;      // -1 to 1: negative = surface, positive = dive deeper
    float verticalSpeed;    // 0 to 1: how fast to change depth

    // Schooling behavior
    float schoolingWeight;  // -1 to 1: how strongly to follow school vs individual behavior
    float separationMod;    // -1 to 1: preferred distance to neighbors
    float alignmentMod;     // -1 to 1: how much to match school direction

    // Special behaviors
    float burstSwim;        // 0 to 1: burst swimming trigger (high energy cost)
    float breachAttempt;    // 0 to 1: attempt to breach surface
    float hideAttempt;      // 0 to 1: attempt to hide (camouflage, reef, etc.)

    // Hunting/Feeding
    float huntIntensity;    // 0 to 1: how aggressively pursuing prey
    float feedingUrgency;   // 0 to 1: priority on finding food

    // Defense
    float inkRelease;       // 0 to 1: release ink (cephalopods)
    float electricDischarge;// 0 to 1: electric shock (electric eel)
    float venomStrike;      // 0 to 1: use venom (jellyfish, stonefish)

    // Bioluminescence
    float biolumActivation; // 0 to 1: activate bioluminescence
    float biolumFlash;      // 0 to 1: trigger flash pattern

    AquaticNeuralOutputs() : NeuralOutputs(),
        depthChange(0.0f), verticalSpeed(0.5f),
        schoolingWeight(0.5f), separationMod(0.0f), alignmentMod(0.0f),
        burstSwim(0.0f), breachAttempt(0.0f), hideAttempt(0.0f),
        huntIntensity(0.5f), feedingUrgency(0.5f),
        inkRelease(0.0f), electricDischarge(0.0f), venomStrike(0.0f),
        biolumActivation(0.0f), biolumFlash(0.0f) {}
};

// Input indices for aquatic neural network
namespace AquaticInputs {
    // Basic sensory inputs (0-7, compatible with base network)
    constexpr int FOOD_DISTANCE = 0;      // Normalized distance to nearest food
    constexpr int FOOD_ANGLE = 1;         // Angle to nearest food (-1 to 1)
    constexpr int THREAT_DISTANCE = 2;    // Normalized distance to nearest predator
    constexpr int THREAT_ANGLE = 3;       // Angle to nearest predator
    constexpr int ENERGY_LEVEL = 4;       // Current energy (0-1)
    constexpr int CURRENT_SPEED = 5;      // Current speed normalized to max
    constexpr int ALLIES_NEARBY = 6;      // Number of nearby allies (normalized)
    constexpr int FEAR_LEVEL = 7;         // Current fear/panic level

    // Aquatic-specific inputs (8+)
    constexpr int CURRENT_DEPTH = 8;      // Current depth (0-1, 0=surface)
    constexpr int TARGET_DEPTH = 9;       // Preferred/safe depth
    constexpr int DEPTH_PRESSURE = 10;    // Pressure stress level (0=comfortable)
    constexpr int OXYGEN_LEVEL = 11;      // For air-breathers: remaining breath (0-1)

    constexpr int WATER_CURRENT_X = 12;   // Water current direction X
    constexpr int WATER_CURRENT_Y = 13;   // Water current direction Y (vertical)
    constexpr int WATER_CURRENT_Z = 14;   // Water current direction Z
    constexpr int CURRENT_STRENGTH = 15;  // Current strength (0-1)

    constexpr int SCHOOL_CENTER_DIST = 16;// Distance to school center
    constexpr int SCHOOL_CENTER_ANGLE = 17;// Angle to school center
    constexpr int SCHOOL_SIZE = 18;       // Number of fish in school (normalized)
    constexpr int SCHOOL_PANIC = 19;      // School's overall panic level

    constexpr int NEAREST_PREY_DIST = 20; // For predators: distance to prey
    constexpr int NEAREST_PREY_ANGLE = 21;
    constexpr int PREY_SIZE = 22;         // Size of nearest prey (relative to self)

    constexpr int LIGHT_LEVEL = 23;       // Ambient light (0=dark, 1=bright surface)
    constexpr int TEMPERATURE = 24;       // Water temperature relative to preferred
    constexpr int VISIBILITY = 25;        // Visual range factor (turbidity)

    constexpr int LATERAL_LINE_FRONT = 26;// Vibration sensing front
    constexpr int LATERAL_LINE_LEFT = 27; // Vibration sensing left
    constexpr int LATERAL_LINE_RIGHT = 28;// Vibration sensing right
    constexpr int LATERAL_LINE_REAR = 29; // Vibration sensing rear

    constexpr int ECHOLOCATION_PING = 30; // Echolocation return signal
    constexpr int ECHOLOCATION_DIST = 31; // Distance from echolocation

    constexpr int TOTAL_INPUTS = 32;
}

// Output indices for aquatic neural network
namespace AquaticOutputs {
    // Base movement outputs (0-5, compatible with base network)
    constexpr int TURN_ANGLE = 0;
    constexpr int SPEED_MULTIPLIER = 1;
    constexpr int AGGRESSION = 2;
    constexpr int FEAR = 3;
    constexpr int SOCIAL = 4;
    constexpr int EXPLORATION = 5;

    // Aquatic-specific outputs (6+)
    constexpr int DEPTH_CHANGE = 6;       // Vertical movement intent
    constexpr int VERTICAL_SPEED = 7;
    constexpr int SCHOOLING_WEIGHT = 8;
    constexpr int SEPARATION_MOD = 9;
    constexpr int ALIGNMENT_MOD = 10;
    constexpr int BURST_SWIM = 11;
    constexpr int BREACH_ATTEMPT = 12;
    constexpr int HIDE_ATTEMPT = 13;
    constexpr int HUNT_INTENSITY = 14;
    constexpr int FEEDING_URGENCY = 15;
    constexpr int INK_RELEASE = 16;
    constexpr int ELECTRIC_DISCHARGE = 17;
    constexpr int VENOM_STRIKE = 18;
    constexpr int BIOLUM_ACTIVATION = 19;
    constexpr int BIOLUM_FLASH = 20;

    constexpr int TOTAL_OUTPUTS = 21;
}

class NeuralNetwork {
public:
    NeuralNetwork(const std::vector<float>& weights);

    // Legacy method - Returns movement direction (angle) and speed multiplier
    void process(const std::vector<float>& inputs, float& outAngle, float& outSpeed);

    // NEW: Full neural processing with behavior modulation outputs
    NeuralOutputs forward(const std::vector<float>& inputs);

    // Get the expected input count for this network
    int getInputCount() const { return inputCount; }
    int getOutputCount() const { return outputCount; }

private:
    std::vector<float> weights;
    static constexpr int inputCount = 8;    // Expanded: food dist/angle, threat dist/angle, energy, speed, allies nearby, fear level
    static constexpr int hiddenCount = 8;   // Hidden layer neurons (expanded for more complex behavior)
    static constexpr int outputCount = 6;   // turn, speed, aggression, fear, social, exploration

    float sigmoid(float x);
    float tanh(float x);
};

// =============================================================================
// AQUATIC NEURAL NETWORK - Extended neural network for aquatic creatures
// =============================================================================
// Supports the full range of aquatic-specific inputs (32) and outputs (21)
// Uses a deeper network architecture for more complex underwater decision-making

class AquaticNeuralNetwork {
public:
    AquaticNeuralNetwork();
    explicit AquaticNeuralNetwork(const std::vector<float>& weights);

    // Full aquatic neural processing
    AquaticNeuralOutputs forward(const std::vector<float>& inputs);

    // Convert to base NeuralOutputs for compatibility
    NeuralOutputs forwardBase(const std::vector<float>& inputs);

    // Get network configuration
    int getInputCount() const { return INPUT_COUNT; }
    int getOutputCount() const { return OUTPUT_COUNT; }
    int getHidden1Count() const { return HIDDEN1_COUNT; }
    int getHidden2Count() const { return HIDDEN2_COUNT; }
    int getWeightCount() const { return WEIGHT_COUNT; }

    // Get/set weights for evolution
    const std::vector<float>& getWeights() const { return m_weights; }
    void setWeights(const std::vector<float>& weights);

    // Mutate weights for evolution
    void mutate(float mutationRate, float mutationStrength);

    // Create offspring through crossover
    static AquaticNeuralNetwork crossover(
        const AquaticNeuralNetwork& parent1,
        const AquaticNeuralNetwork& parent2
    );

    // Network architecture constants
    static constexpr int INPUT_COUNT = AquaticInputs::TOTAL_INPUTS;     // 32
    static constexpr int HIDDEN1_COUNT = 24;  // First hidden layer
    static constexpr int HIDDEN2_COUNT = 16;  // Second hidden layer
    static constexpr int OUTPUT_COUNT = AquaticOutputs::TOTAL_OUTPUTS;  // 21

    // Total weights: (32*24) + (24*16) + (16*21) = 768 + 384 + 336 = 1488
    static constexpr int WEIGHT_COUNT =
        (INPUT_COUNT * HIDDEN1_COUNT) +
        (HIDDEN1_COUNT * HIDDEN2_COUNT) +
        (HIDDEN2_COUNT * OUTPUT_COUNT);

private:
    std::vector<float> m_weights;

    // Activation functions
    float relu(float x) const;
    float leakyRelu(float x, float alpha = 0.01f) const;
    float tanh(float x) const;
    float sigmoid(float x) const;

    // Initialize with random weights
    void initializeRandom();
};

// =============================================================================
// SPECIALIZED AQUATIC NETWORKS - Pre-configured for specific creature types
// =============================================================================

namespace AquaticNetworks {
    // Create network optimized for schooling fish behavior
    AquaticNeuralNetwork createSchoolingFishNetwork();

    // Create network optimized for predatory behavior
    AquaticNeuralNetwork createPredatorNetwork();

    // Create network optimized for deep-sea creatures
    AquaticNeuralNetwork createDeepSeaNetwork();

    // Create network optimized for jellyfish (simple drifting)
    AquaticNeuralNetwork createJellyfishNetwork();

    // Create network optimized for air-breathing marine mammals
    AquaticNeuralNetwork createMarineMammalNetwork();

    // Create network optimized for cephalopods (octopus/squid)
    AquaticNeuralNetwork createCephalopodNetwork();
}
