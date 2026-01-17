#include "NeuralNetwork.h"
#include <cmath>
#include <algorithm>

NeuralNetwork::NeuralNetwork(const std::vector<float>& weights)
    : weights(weights) {
    // Ensure we have enough weights for the network
    // Required: (inputCount * hiddenCount) + (hiddenCount * outputCount) = 8*8 + 8*6 = 112 weights
    const size_t requiredWeights = static_cast<size_t>(inputCount * hiddenCount + hiddenCount * outputCount);
    if (this->weights.size() < requiredWeights) {
        // Pad with small random-ish values based on index (deterministic)
        this->weights.resize(requiredWeights);
        for (size_t i = weights.size(); i < requiredWeights; i++) {
            // Simple deterministic initialization: small values based on index
            this->weights[i] = (static_cast<float>(i % 17) - 8.0f) * 0.1f;
        }
    }
}

// Legacy method for backward compatibility
void NeuralNetwork::process(const std::vector<float>& inputs, float& outAngle, float& outSpeed) {
    NeuralOutputs outputs = forward(inputs);
    outAngle = outputs.turnAngle * 3.14159f;  // Scale to -PI to PI
    outSpeed = (outputs.speedMultiplier + 1.0f) * 0.5f;  // Scale to 0-1
}

// NEW: Full neural network forward pass with all behavior outputs
NeuralOutputs NeuralNetwork::forward(const std::vector<float>& inputs) {
    NeuralOutputs result;

    // Pad or truncate inputs to match expected input count
    std::vector<float> paddedInputs(inputCount, 0.0f);
    for (size_t i = 0; i < std::min(inputs.size(), static_cast<size_t>(inputCount)); i++) {
        paddedInputs[i] = inputs[i];
    }

    // Input to hidden layer
    std::vector<float> hidden(hiddenCount, 0.0f);
    int weightIdx = 0;

    for (int h = 0; h < hiddenCount; h++) {
        float sum = 0.0f;
        for (int i = 0; i < inputCount; i++) {
            if (weightIdx < static_cast<int>(weights.size())) {
                sum += paddedInputs[i] * weights[weightIdx++];
            }
        }
        hidden[h] = tanh(sum);
    }

    // Hidden to output layer
    std::vector<float> output(outputCount, 0.0f);

    for (int o = 0; o < outputCount; o++) {
        float sum = 0.0f;
        for (int h = 0; h < hiddenCount; h++) {
            if (weightIdx < static_cast<int>(weights.size())) {
                sum += hidden[h] * weights[weightIdx++];
            }
        }
        output[o] = tanh(sum);  // All outputs in range [-1, 1]
    }

    // Map outputs to NeuralOutputs struct
    // Output 0: Turn angle (-1 to 1, will be scaled to radians by caller)
    result.turnAngle = output[0];

    // Output 1: Speed multiplier (-1 to 1, will be scaled to 0-1 by caller)
    result.speedMultiplier = output[1];

    // Output 2: Aggression modifier (-1 to 1)
    // Negative = more passive (flee sooner, attack less)
    // Positive = more aggressive (attack more, pursue longer)
    result.aggressionMod = output[2];

    // Output 3: Fear modifier (-1 to 1)
    // Negative = braver (flee later, less reactive to threats)
    // Positive = more fearful (flee sooner, more reactive)
    result.fearMod = output[3];

    // Output 4: Social modifier (-1 to 1)
    // Negative = more solitary (less flocking/herding)
    // Positive = more social (stronger flocking/herding behavior)
    result.socialMod = output[4];

    // Output 5: Exploration modifier (-1 to 1)
    // Negative = stay near current area
    // Positive = wander more, explore further
    result.explorationMod = output[5];

    return result;
}

float NeuralNetwork::sigmoid(float x) {
    return 1.0f / (1.0f + std::exp(-x));
}

float NeuralNetwork::tanh(float x) {
    return std::tanh(x);
}

// =============================================================================
// AQUATIC NEURAL NETWORK IMPLEMENTATION
// =============================================================================

AquaticNeuralNetwork::AquaticNeuralNetwork() {
    initializeRandom();
}

AquaticNeuralNetwork::AquaticNeuralNetwork(const std::vector<float>& weights)
    : m_weights(weights) {
    // Ensure we have enough weights
    if (m_weights.size() < static_cast<size_t>(WEIGHT_COUNT)) {
        m_weights.resize(WEIGHT_COUNT);
        // Initialize missing weights with small values
        for (size_t i = weights.size(); i < static_cast<size_t>(WEIGHT_COUNT); i++) {
            m_weights[i] = (static_cast<float>(i % 23) - 11.0f) * 0.05f;
        }
    }
}

void AquaticNeuralNetwork::initializeRandom() {
    m_weights.resize(WEIGHT_COUNT);

    // Xavier initialization: weights scaled by sqrt(2 / (fan_in + fan_out))
    // Input to Hidden1
    float scale1 = std::sqrt(2.0f / (INPUT_COUNT + HIDDEN1_COUNT));
    for (int i = 0; i < INPUT_COUNT * HIDDEN1_COUNT; i++) {
        // Pseudo-random based on index for deterministic initialization
        float r = (static_cast<float>((i * 7919) % 1000) / 500.0f) - 1.0f;
        m_weights[i] = r * scale1;
    }

    // Hidden1 to Hidden2
    float scale2 = std::sqrt(2.0f / (HIDDEN1_COUNT + HIDDEN2_COUNT));
    int offset = INPUT_COUNT * HIDDEN1_COUNT;
    for (int i = 0; i < HIDDEN1_COUNT * HIDDEN2_COUNT; i++) {
        float r = (static_cast<float>(((i + offset) * 6271) % 1000) / 500.0f) - 1.0f;
        m_weights[offset + i] = r * scale2;
    }

    // Hidden2 to Output
    float scale3 = std::sqrt(2.0f / (HIDDEN2_COUNT + OUTPUT_COUNT));
    offset += HIDDEN1_COUNT * HIDDEN2_COUNT;
    for (int i = 0; i < HIDDEN2_COUNT * OUTPUT_COUNT; i++) {
        float r = (static_cast<float>(((i + offset) * 4337) % 1000) / 500.0f) - 1.0f;
        m_weights[offset + i] = r * scale3;
    }
}

void AquaticNeuralNetwork::setWeights(const std::vector<float>& weights) {
    m_weights = weights;
    if (m_weights.size() < static_cast<size_t>(WEIGHT_COUNT)) {
        m_weights.resize(WEIGHT_COUNT, 0.0f);
    }
}

AquaticNeuralOutputs AquaticNeuralNetwork::forward(const std::vector<float>& inputs) {
    AquaticNeuralOutputs result;

    // Pad inputs to expected size
    std::vector<float> paddedInputs(INPUT_COUNT, 0.0f);
    for (size_t i = 0; i < std::min(inputs.size(), static_cast<size_t>(INPUT_COUNT)); i++) {
        paddedInputs[i] = inputs[i];
    }

    // First hidden layer: input -> hidden1
    std::vector<float> hidden1(HIDDEN1_COUNT, 0.0f);
    int weightIdx = 0;

    for (int h = 0; h < HIDDEN1_COUNT; h++) {
        float sum = 0.0f;
        for (int i = 0; i < INPUT_COUNT; i++) {
            sum += paddedInputs[i] * m_weights[weightIdx++];
        }
        hidden1[h] = leakyRelu(sum);  // LeakyReLU for hidden layers
    }

    // Second hidden layer: hidden1 -> hidden2
    std::vector<float> hidden2(HIDDEN2_COUNT, 0.0f);

    for (int h = 0; h < HIDDEN2_COUNT; h++) {
        float sum = 0.0f;
        for (int i = 0; i < HIDDEN1_COUNT; i++) {
            sum += hidden1[i] * m_weights[weightIdx++];
        }
        hidden2[h] = leakyRelu(sum);
    }

    // Output layer: hidden2 -> output
    std::vector<float> output(OUTPUT_COUNT, 0.0f);

    for (int o = 0; o < OUTPUT_COUNT; o++) {
        float sum = 0.0f;
        for (int h = 0; h < HIDDEN2_COUNT; h++) {
            sum += hidden2[h] * m_weights[weightIdx++];
        }
        // Use tanh for outputs that need [-1, 1] range
        // Use sigmoid for outputs that need [0, 1] range
        output[o] = tanh(sum);
    }

    // Map outputs to AquaticNeuralOutputs struct
    // Base outputs (inherited from NeuralOutputs)
    result.turnAngle = output[AquaticOutputs::TURN_ANGLE];
    result.speedMultiplier = (output[AquaticOutputs::SPEED_MULTIPLIER] + 1.0f) * 0.5f;  // Scale to [0, 1]
    result.aggressionMod = output[AquaticOutputs::AGGRESSION];
    result.fearMod = output[AquaticOutputs::FEAR];
    result.socialMod = output[AquaticOutputs::SOCIAL];
    result.explorationMod = output[AquaticOutputs::EXPLORATION];

    // Aquatic-specific outputs
    result.depthChange = output[AquaticOutputs::DEPTH_CHANGE];
    result.verticalSpeed = (output[AquaticOutputs::VERTICAL_SPEED] + 1.0f) * 0.5f;

    result.schoolingWeight = output[AquaticOutputs::SCHOOLING_WEIGHT];
    result.separationMod = output[AquaticOutputs::SEPARATION_MOD];
    result.alignmentMod = output[AquaticOutputs::ALIGNMENT_MOD];

    result.burstSwim = (output[AquaticOutputs::BURST_SWIM] + 1.0f) * 0.5f;
    result.breachAttempt = (output[AquaticOutputs::BREACH_ATTEMPT] + 1.0f) * 0.5f;
    result.hideAttempt = (output[AquaticOutputs::HIDE_ATTEMPT] + 1.0f) * 0.5f;

    result.huntIntensity = (output[AquaticOutputs::HUNT_INTENSITY] + 1.0f) * 0.5f;
    result.feedingUrgency = (output[AquaticOutputs::FEEDING_URGENCY] + 1.0f) * 0.5f;

    result.inkRelease = (output[AquaticOutputs::INK_RELEASE] + 1.0f) * 0.5f;
    result.electricDischarge = (output[AquaticOutputs::ELECTRIC_DISCHARGE] + 1.0f) * 0.5f;
    result.venomStrike = (output[AquaticOutputs::VENOM_STRIKE] + 1.0f) * 0.5f;

    result.biolumActivation = (output[AquaticOutputs::BIOLUM_ACTIVATION] + 1.0f) * 0.5f;
    result.biolumFlash = (output[AquaticOutputs::BIOLUM_FLASH] + 1.0f) * 0.5f;

    return result;
}

NeuralOutputs AquaticNeuralNetwork::forwardBase(const std::vector<float>& inputs) {
    AquaticNeuralOutputs aquaticOutputs = forward(inputs);

    // Convert to base NeuralOutputs (slice off aquatic-specific fields)
    NeuralOutputs result;
    result.turnAngle = aquaticOutputs.turnAngle;
    result.speedMultiplier = aquaticOutputs.speedMultiplier;
    result.aggressionMod = aquaticOutputs.aggressionMod;
    result.fearMod = aquaticOutputs.fearMod;
    result.socialMod = aquaticOutputs.socialMod;
    result.explorationMod = aquaticOutputs.explorationMod;

    return result;
}

void AquaticNeuralNetwork::mutate(float mutationRate, float mutationStrength) {
    for (size_t i = 0; i < m_weights.size(); i++) {
        // Use deterministic pseudo-random based on weight index and current value
        float hash = std::sin(static_cast<float>(i) * 12.9898f + m_weights[i] * 78.233f);
        float random = hash - std::floor(hash);

        if (random < mutationRate) {
            // Apply mutation
            float mutationHash = std::sin(static_cast<float>(i) * 43.758f + m_weights[i] * 93.527f);
            float mutation = (mutationHash - std::floor(mutationHash)) * 2.0f - 1.0f;
            m_weights[i] += mutation * mutationStrength;

            // Clamp weights to reasonable range
            m_weights[i] = std::clamp(m_weights[i], -3.0f, 3.0f);
        }
    }
}

AquaticNeuralNetwork AquaticNeuralNetwork::crossover(
    const AquaticNeuralNetwork& parent1,
    const AquaticNeuralNetwork& parent2
) {
    std::vector<float> childWeights(WEIGHT_COUNT);

    const auto& w1 = parent1.getWeights();
    const auto& w2 = parent2.getWeights();

    // Uniform crossover with occasional segment swaps
    for (int i = 0; i < WEIGHT_COUNT; i++) {
        // Deterministic selection based on index
        float hash = std::sin(static_cast<float>(i) * 7.9393f);
        float selector = hash - std::floor(hash);

        if (selector < 0.5f) {
            childWeights[i] = w1[i];
        } else {
            childWeights[i] = w2[i];
        }

        // Occasionally interpolate between parents for smoother evolution
        if (selector > 0.45f && selector < 0.55f) {
            childWeights[i] = (w1[i] + w2[i]) * 0.5f;
        }
    }

    return AquaticNeuralNetwork(childWeights);
}

float AquaticNeuralNetwork::relu(float x) const {
    return std::max(0.0f, x);
}

float AquaticNeuralNetwork::leakyRelu(float x, float alpha) const {
    return x >= 0.0f ? x : alpha * x;
}

float AquaticNeuralNetwork::tanh(float x) const {
    return std::tanh(x);
}

float AquaticNeuralNetwork::sigmoid(float x) const {
    return 1.0f / (1.0f + std::exp(-x));
}

// =============================================================================
// SPECIALIZED AQUATIC NETWORKS
// =============================================================================

namespace AquaticNetworks {

// Helper to set specific weight regions for biasing behavior
static void biasWeights(
    std::vector<float>& weights,
    int inputIdx,
    int outputIdx,
    float bias,
    int inputCount,
    int hidden1Count,
    int hidden2Count
) {
    // This is a simplified bias - in reality we'd need to trace paths through the network
    // For now, we modify the input-to-hidden1 weights for the relevant input
    int weightStart = inputIdx * hidden1Count;
    for (int h = 0; h < hidden1Count; h++) {
        weights[weightStart + h] *= (1.0f + bias * 0.5f);
    }
}

AquaticNeuralNetwork createSchoolingFishNetwork() {
    AquaticNeuralNetwork network;
    std::vector<float> weights = network.getWeights();

    // Bias toward schooling behavior
    // Increase sensitivity to school center distance/angle
    biasWeights(weights, AquaticInputs::SCHOOL_CENTER_DIST, AquaticOutputs::SCHOOLING_WEIGHT,
                0.8f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    // Increase fear response to threats
    biasWeights(weights, AquaticInputs::THREAT_DISTANCE, AquaticOutputs::FEAR,
                0.6f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    // Bias alignment behavior
    biasWeights(weights, AquaticInputs::SCHOOL_CENTER_ANGLE, AquaticOutputs::ALIGNMENT_MOD,
                0.7f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    network.setWeights(weights);
    return network;
}

AquaticNeuralNetwork createPredatorNetwork() {
    AquaticNeuralNetwork network;
    std::vector<float> weights = network.getWeights();

    // Bias toward hunting behavior
    biasWeights(weights, AquaticInputs::NEAREST_PREY_DIST, AquaticOutputs::HUNT_INTENSITY,
                1.0f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    // Increase aggression
    biasWeights(weights, AquaticInputs::PREY_SIZE, AquaticOutputs::AGGRESSION,
                0.8f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    // Reduce fear
    biasWeights(weights, AquaticInputs::THREAT_DISTANCE, AquaticOutputs::FEAR,
                -0.5f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    // Enable burst swim for chasing
    biasWeights(weights, AquaticInputs::NEAREST_PREY_DIST, AquaticOutputs::BURST_SWIM,
                0.6f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    network.setWeights(weights);
    return network;
}

AquaticNeuralNetwork createDeepSeaNetwork() {
    AquaticNeuralNetwork network;
    std::vector<float> weights = network.getWeights();

    // Bias toward staying at preferred depth
    biasWeights(weights, AquaticInputs::DEPTH_PRESSURE, AquaticOutputs::DEPTH_CHANGE,
                0.5f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    // Enable bioluminescence in darkness
    biasWeights(weights, AquaticInputs::LIGHT_LEVEL, AquaticOutputs::BIOLUM_ACTIVATION,
                -0.8f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    // Use echolocation for navigation
    biasWeights(weights, AquaticInputs::ECHOLOCATION_PING, AquaticOutputs::EXPLORATION,
                0.6f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    // Reduce social behavior (deep sea creatures are often solitary)
    biasWeights(weights, AquaticInputs::SCHOOL_SIZE, AquaticOutputs::SOCIAL,
                -0.7f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    network.setWeights(weights);
    return network;
}

AquaticNeuralNetwork createJellyfishNetwork() {
    AquaticNeuralNetwork network;
    std::vector<float> weights = network.getWeights();

    // Jellyfish mostly drift with currents - reduce active swimming
    biasWeights(weights, AquaticInputs::CURRENT_STRENGTH, AquaticOutputs::SPEED_MULTIPLIER,
                -0.6f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    // Enable venom when threatened
    biasWeights(weights, AquaticInputs::THREAT_DISTANCE, AquaticOutputs::VENOM_STRIKE,
                0.8f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    // Bioluminescence for defense/luring
    biasWeights(weights, AquaticInputs::LIGHT_LEVEL, AquaticOutputs::BIOLUM_FLASH,
                0.5f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    // Minimal schooling
    biasWeights(weights, AquaticInputs::SCHOOL_CENTER_DIST, AquaticOutputs::SCHOOLING_WEIGHT,
                -0.8f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    network.setWeights(weights);
    return network;
}

AquaticNeuralNetwork createMarineMammalNetwork() {
    AquaticNeuralNetwork network;
    std::vector<float> weights = network.getWeights();

    // Critical: surface for air based on oxygen level
    biasWeights(weights, AquaticInputs::OXYGEN_LEVEL, AquaticOutputs::DEPTH_CHANGE,
                -1.2f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    // Enable breaching behavior
    biasWeights(weights, AquaticInputs::CURRENT_DEPTH, AquaticOutputs::BREACH_ATTEMPT,
                0.4f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    // Strong social behavior (pods)
    biasWeights(weights, AquaticInputs::SCHOOL_CENTER_DIST, AquaticOutputs::SOCIAL,
                0.9f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    // Use echolocation
    biasWeights(weights, AquaticInputs::ECHOLOCATION_PING, AquaticOutputs::HUNT_INTENSITY,
                0.7f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    // High exploration tendency
    biasWeights(weights, AquaticInputs::ENERGY_LEVEL, AquaticOutputs::EXPLORATION,
                0.6f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    network.setWeights(weights);
    return network;
}

AquaticNeuralNetwork createCephalopodNetwork() {
    AquaticNeuralNetwork network;
    std::vector<float> weights = network.getWeights();

    // Enable ink release when threatened
    biasWeights(weights, AquaticInputs::THREAT_DISTANCE, AquaticOutputs::INK_RELEASE,
                1.0f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    // Hiding behavior
    biasWeights(weights, AquaticInputs::THREAT_DISTANCE, AquaticOutputs::HIDE_ATTEMPT,
                0.8f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    // Intelligent hunting
    biasWeights(weights, AquaticInputs::NEAREST_PREY_DIST, AquaticOutputs::HUNT_INTENSITY,
                0.7f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    // High exploration (octopi are curious)
    biasWeights(weights, AquaticInputs::VISIBILITY, AquaticOutputs::EXPLORATION,
                0.8f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    // Solitary behavior
    biasWeights(weights, AquaticInputs::ALLIES_NEARBY, AquaticOutputs::SOCIAL,
                -0.7f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    // Burst swimming for escape
    biasWeights(weights, AquaticInputs::THREAT_DISTANCE, AquaticOutputs::BURST_SWIM,
                0.9f, AquaticNeuralNetwork::INPUT_COUNT, AquaticNeuralNetwork::HIDDEN1_COUNT,
                AquaticNeuralNetwork::HIDDEN2_COUNT);

    network.setWeights(weights);
    return network;
}

} // namespace AquaticNetworks
