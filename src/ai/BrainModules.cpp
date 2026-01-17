#include "BrainModules.h"
#include <algorithm>
#include <numeric>

namespace ai {

// ============================================================================
// Sensory Processor Implementation
// ============================================================================

SensoryProcessor::SensoryProcessor() {}

void SensoryProcessor::initialize(std::mt19937& rng) {
    // Create feedforward network: input -> hidden -> output
    std::uniform_real_distribution<float> weightDist(-1.0f, 1.0f);

    // Input nodes
    for (int i = 0; i < INPUT_SIZE; i++) {
        m_network.addNode(NodeType::INPUT, ActivationType::LINEAR);
    }

    // Bias node
    m_network.addNode(NodeType::BIAS, ActivationType::LINEAR);

    // Hidden layer (6 nodes)
    std::vector<int> hiddenIds;
    for (int i = 0; i < 6; i++) {
        int id = m_network.addNode(NodeType::HIDDEN, ActivationType::TANH);
        hiddenIds.push_back(id);
    }

    // Output nodes
    std::vector<int> outputIds;
    for (int i = 0; i < OUTPUT_SIZE; i++) {
        int id = m_network.addNode(NodeType::OUTPUT, ActivationType::TANH);
        outputIds.push_back(id);
    }

    // Connect input to hidden
    for (int i = 0; i < INPUT_SIZE + 1; i++) {  // +1 for bias
        for (int h : hiddenIds) {
            m_network.addConnection(i, h, weightDist(rng));
        }
    }

    // Connect hidden to output
    for (int h : hiddenIds) {
        for (int o : outputIds) {
            m_network.addConnection(h, o, weightDist(rng));
        }
    }
}

std::vector<float> SensoryProcessor::process(const SensoryInput& input) {
    return m_network.forward(input.toVector());
}

std::vector<float> SensoryProcessor::getWeights() const {
    return m_network.getWeights();
}

void SensoryProcessor::setWeights(const std::vector<float>& weights) {
    m_network.setWeights(weights);
}

size_t SensoryProcessor::getWeightCount() const {
    return m_network.getConnections().size();
}

// ============================================================================
// Emotional Module Implementation
// ============================================================================

EmotionalModule::EmotionalModule() {}

void EmotionalModule::initialize(std::mt19937& rng) {
    std::uniform_real_distribution<float> weightDist(-1.0f, 1.0f);

    // Input nodes
    for (int i = 0; i < INPUT_SIZE; i++) {
        m_network.addNode(NodeType::INPUT, ActivationType::LINEAR);
    }

    // Bias
    m_network.addNode(NodeType::BIAS, ActivationType::LINEAR);

    // Hidden layer (4 nodes - one per drive pathway)
    std::vector<int> hiddenIds;
    for (int i = 0; i < 4; i++) {
        int id = m_network.addNode(NodeType::HIDDEN, ActivationType::TANH);
        hiddenIds.push_back(id);
    }

    // Output nodes (one per drive)
    std::vector<int> outputIds;
    for (int i = 0; i < OUTPUT_SIZE; i++) {
        int id = m_network.addNode(NodeType::OUTPUT, ActivationType::SIGMOID);
        outputIds.push_back(id);
    }

    // Connect input to hidden
    for (int i = 0; i < INPUT_SIZE + 1; i++) {
        for (int h : hiddenIds) {
            m_network.addConnection(i, h, weightDist(rng));
        }
    }

    // Connect hidden to output
    for (int h : hiddenIds) {
        for (int o : outputIds) {
            m_network.addConnection(h, o, weightDist(rng));
        }
    }
}

EmotionalModule::Drives EmotionalModule::process(
    const std::vector<float>& sensoryFeatures,
    float energy, float health,
    const NeuromodulatorState& modulators) {

    // Build input vector
    std::vector<float> input;
    input.reserve(INPUT_SIZE);

    // Add sensory features (first 6)
    for (size_t i = 0; i < std::min(size_t(6), sensoryFeatures.size()); i++) {
        input.push_back(sensoryFeatures[i]);
    }
    while (input.size() < 6) input.push_back(0.0f);

    // Add internal state
    input.push_back(energy);
    input.push_back(health);

    // Add neuromodulator influences
    input.push_back(modulators.norepinephrine);  // Affects fear
    input.push_back(modulators.serotonin);       // Affects patience

    // Process through network
    auto output = m_network.forward(input);

    // Build drives
    Drives drives;
    if (output.size() > 0) drives.fear = output[0];
    if (output.size() > 1) drives.hunger = output[1];
    if (output.size() > 2) drives.curiosity = output[2];
    if (output.size() > 3) drives.social = output[3];

    // Modulate drives based on internal state
    // Low energy increases hunger
    drives.hunger = std::min(1.0f, drives.hunger + (1.0f - energy) * 0.3f);

    // High norepinephrine increases fear
    drives.fear = std::min(1.0f, drives.fear + modulators.norepinephrine * 0.2f);

    // High serotonin decreases fear, increases exploration
    drives.fear = std::max(0.0f, drives.fear - modulators.serotonin * 0.1f);
    drives.curiosity = std::min(1.0f, drives.curiosity + modulators.serotonin * 0.1f);

    m_currentDrives = drives;
    return drives;
}

std::vector<float> EmotionalModule::getWeights() const {
    return m_network.getWeights();
}

void EmotionalModule::setWeights(const std::vector<float>& weights) {
    m_network.setWeights(weights);
}

size_t EmotionalModule::getWeightCount() const {
    return m_network.getConnections().size();
}

// ============================================================================
// Decision Maker Implementation
// ============================================================================

DecisionMaker::DecisionMaker() {}

void DecisionMaker::initialize(std::mt19937& rng) {
    std::uniform_real_distribution<float> weightDist(-0.5f, 0.5f);

    // Input nodes
    for (int i = 0; i < INPUT_SIZE; i++) {
        m_network.addNode(NodeType::INPUT, ActivationType::LINEAR);
    }

    // Bias
    m_network.addNode(NodeType::BIAS, ActivationType::LINEAR);

    // Hidden layer with recurrent connections
    std::vector<int> hiddenIds;
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        int id = m_network.addNode(NodeType::HIDDEN, ActivationType::TANH);
        hiddenIds.push_back(id);
    }

    // Output nodes
    std::vector<int> outputIds;
    for (int i = 0; i < OUTPUT_SIZE; i++) {
        int id = m_network.addNode(NodeType::OUTPUT, ActivationType::TANH);
        outputIds.push_back(id);
    }

    // Input to hidden connections
    for (int i = 0; i < INPUT_SIZE + 1; i++) {
        for (int h : hiddenIds) {
            m_network.addConnection(i, h, weightDist(rng));
        }
    }

    // Hidden to hidden recurrent connections (partial)
    for (size_t i = 0; i < hiddenIds.size(); i++) {
        // Self-connection
        m_network.addConnection(hiddenIds[i], hiddenIds[i], weightDist(rng) * 0.5f, true);

        // Connect to next few neurons (creating recurrent memory)
        for (size_t j = 1; j <= 2 && i + j < hiddenIds.size(); j++) {
            m_network.addConnection(hiddenIds[i], hiddenIds[i + j], weightDist(rng) * 0.3f, true);
        }
    }

    // Hidden to output
    for (int h : hiddenIds) {
        for (int o : outputIds) {
            m_network.addConnection(h, o, weightDist(rng));
        }
    }

    // Mark connections as plastic
    for (auto& conn : m_network.getConnections()) {
        conn.plastic = true;
        conn.plasticityRate = 1.0f;
    }
}

std::vector<float> DecisionMaker::decide(
    const std::vector<float>& sensoryFeatures,
    const std::vector<float>& memoryState,
    const EmotionalModule::Drives& drives,
    const NeuromodulatorState& modulators) {

    // Build comprehensive input
    std::vector<float> input;
    input.reserve(INPUT_SIZE);

    // Sensory features (8)
    for (size_t i = 0; i < 8 && i < sensoryFeatures.size(); i++) {
        input.push_back(sensoryFeatures[i]);
    }
    while (input.size() < 8) input.push_back(0.0f);

    // Memory state (12)
    for (size_t i = 0; i < 12 && i < memoryState.size(); i++) {
        input.push_back(memoryState[i]);
    }
    while (input.size() < 20) input.push_back(0.0f);

    // Drives (4)
    auto driveVec = drives.toVector();
    input.insert(input.end(), driveVec.begin(), driveVec.end());

    // Modulators (4)
    input.push_back(modulators.dopamine);
    input.push_back(modulators.norepinephrine);
    input.push_back(modulators.serotonin);
    input.push_back(modulators.acetylcholine);

    // Forward pass
    m_lastOutput = m_network.forward(input);

    // Accumulate Hebbian correlations
    m_network.accumulateHebbian();

    return m_lastOutput;
}

void DecisionMaker::updatePlasticity(float reward, float learningRate) {
    m_network.updatePlasticity(reward, learningRate);
}

void DecisionMaker::accumulateEligibility() {
    m_network.accumulateHebbian();
}

std::vector<float> DecisionMaker::getWeights() const {
    return m_network.getWeights();
}

void DecisionMaker::setWeights(const std::vector<float>& weights) {
    m_network.setWeights(weights);
}

size_t DecisionMaker::getWeightCount() const {
    return m_network.getConnections().size();
}

// ============================================================================
// Motor Controller Implementation
// ============================================================================

MotorController::MotorController() {}

void MotorController::initialize(std::mt19937& rng) {
    std::uniform_real_distribution<float> weightDist(-1.0f, 1.0f);

    // Input nodes
    for (int i = 0; i < INPUT_SIZE; i++) {
        m_network.addNode(NodeType::INPUT, ActivationType::LINEAR);
    }

    // Bias
    m_network.addNode(NodeType::BIAS, ActivationType::LINEAR);

    // Small hidden layer
    std::vector<int> hiddenIds;
    for (int i = 0; i < 4; i++) {
        int id = m_network.addNode(NodeType::HIDDEN, ActivationType::TANH);
        hiddenIds.push_back(id);
    }

    // Output nodes
    std::vector<int> outputIds;
    for (int i = 0; i < OUTPUT_SIZE; i++) {
        int id = m_network.addNode(NodeType::OUTPUT, ActivationType::TANH);
        outputIds.push_back(id);
    }

    // Connections
    for (int i = 0; i < INPUT_SIZE + 1; i++) {
        for (int h : hiddenIds) {
            m_network.addConnection(i, h, weightDist(rng));
        }
    }

    for (int h : hiddenIds) {
        for (int o : outputIds) {
            m_network.addConnection(h, o, weightDist(rng));
        }
    }
}

MotorOutput MotorController::generateMotorCommands(const std::vector<float>& decisionOutput) {
    // Pad input if needed
    std::vector<float> input = decisionOutput;
    while (input.size() < INPUT_SIZE) {
        input.push_back(0.0f);
    }

    auto output = m_network.forward(input);
    return MotorOutput::fromVector(output);
}

std::vector<float> MotorController::getWeights() const {
    return m_network.getWeights();
}

void MotorController::setWeights(const std::vector<float>& weights) {
    m_network.setWeights(weights);
}

size_t MotorController::getWeightCount() const {
    return m_network.getConnections().size();
}

// ============================================================================
// Creature Brain Implementation
// ============================================================================

CreatureBrain::CreatureBrain() {}

void CreatureBrain::initialize(std::mt19937& rng) {
    m_sensory.initialize(rng);
    m_emotional.initialize(rng);
    m_decision.initialize(rng);
    m_motor.initialize(rng);
    m_memory.reset();
    m_modulators = NeuromodulatorState();
    m_useNEATGenome = false;
}

void CreatureBrain::initializeFromGenome(const NEATGenome& genome) {
    m_genome = genome;
    m_neatNetwork = genome.buildNetwork();
    m_useNEATGenome = true;
    m_memory.reset();
    m_modulators = NeuromodulatorState();
}

MotorOutput CreatureBrain::process(const SensoryInput& input, float deltaTime) {
    // Decay neuromodulators
    m_modulators.decay(deltaTime);

    if (m_useNEATGenome && m_neatNetwork) {
        // Use single NEAT-evolved network
        auto rawInput = input.toVector();

        // Add neuromodulator state to input
        rawInput.push_back(m_modulators.dopamine);
        rawInput.push_back(m_modulators.norepinephrine);
        rawInput.push_back(m_modulators.serotonin);
        rawInput.push_back(m_modulators.acetylcholine);

        // Add memory state
        auto memState = m_memory.read();
        for (size_t i = 0; i < 4 && i < memState.size(); i++) {
            rawInput.push_back(memState[i]);
        }

        auto output = m_neatNetwork->forward(rawInput);

        // Update memory with processed input
        m_memory.update(output, 0.3f);

        // Accumulate Hebbian traces
        m_neatNetwork->accumulateHebbian();

        return MotorOutput::fromVector(output);
    }

    // Modular brain processing pipeline

    // 1. Sensory processing
    auto sensoryFeatures = m_sensory.process(input);

    // 2. Emotional processing
    m_currentDrives = m_emotional.process(
        sensoryFeatures, input.energy, input.health, m_modulators);

    // 3. Update working memory
    m_memory.update(sensoryFeatures, 0.2f + 0.3f * m_modulators.acetylcholine);
    m_memory.decay(0.05f * deltaTime);

    // 4. Decision making
    auto memoryState = m_memory.read();
    m_lastDecision = m_decision.decide(
        sensoryFeatures, memoryState, m_currentDrives, m_modulators);

    // 5. Motor output generation
    auto motorOutput = m_motor.generateMotorCommands(m_lastDecision);

    // Track for learning
    m_stepsSinceLearn++;

    return motorOutput;
}

void CreatureBrain::onFoodEaten(float amount) {
    m_modulators.onFoodEaten(amount);
    m_accumulatedReward += amount;
}

void CreatureBrain::onDamageTaken(float amount) {
    m_modulators.onDamageTaken(amount);
    m_accumulatedReward -= amount * 0.5f;
}

void CreatureBrain::onThreatDetected(float level) {
    m_modulators.onThreatDetected(level);
}

void CreatureBrain::onSuccessfulHunt() {
    m_modulators.onSuccessfulHunt();
    m_accumulatedReward += 2.0f;
}

void CreatureBrain::learn(float reward) {
    m_accumulatedReward += reward;

    float effectiveReward = m_modulators.getRewardSignal() + reward * 0.5f;
    float learningRate = m_modulators.getLearningRate();

    if (m_useNEATGenome && m_neatNetwork) {
        m_neatNetwork->updatePlasticity(effectiveReward, learningRate);
        m_neatNetwork->decayEligibility(0.95f);
    } else {
        m_decision.updatePlasticity(effectiveReward, learningRate);
    }

    m_stepsSinceLearn = 0;
    m_accumulatedReward = 0.0f;
}

void CreatureBrain::reset() {
    if (m_useNEATGenome && m_neatNetwork) {
        m_neatNetwork->reset();
    } else {
        m_sensory.reset();
        m_emotional.reset();
        m_decision.reset();
        m_motor.reset();
    }

    m_memory.reset();
    m_modulators = NeuromodulatorState();
    m_currentDrives = EmotionalModule::Drives();
    m_accumulatedReward = 0.0f;
    m_stepsSinceLearn = 0;
}

std::vector<float> CreatureBrain::getAllWeights() const {
    if (m_useNEATGenome && m_neatNetwork) {
        return m_neatNetwork->getWeights();
    }

    std::vector<float> weights;

    auto sensoryW = m_sensory.getWeights();
    auto emotionalW = m_emotional.getWeights();
    auto decisionW = m_decision.getWeights();
    auto motorW = m_motor.getWeights();

    weights.insert(weights.end(), sensoryW.begin(), sensoryW.end());
    weights.insert(weights.end(), emotionalW.begin(), emotionalW.end());
    weights.insert(weights.end(), decisionW.begin(), decisionW.end());
    weights.insert(weights.end(), motorW.begin(), motorW.end());

    return weights;
}

void CreatureBrain::setAllWeights(const std::vector<float>& weights) {
    if (m_useNEATGenome && m_neatNetwork) {
        m_neatNetwork->setWeights(weights);
        return;
    }

    size_t offset = 0;

    auto setModuleWeights = [&](auto& module) {
        size_t count = module.getWeightCount();
        if (offset + count <= weights.size()) {
            std::vector<float> moduleWeights(
                weights.begin() + offset,
                weights.begin() + offset + count);
            module.setWeights(moduleWeights);
            offset += count;
        }
    };

    setModuleWeights(m_sensory);
    setModuleWeights(m_emotional);
    setModuleWeights(m_decision);
    setModuleWeights(m_motor);
}

size_t CreatureBrain::getTotalWeightCount() const {
    if (m_useNEATGenome && m_neatNetwork) {
        return m_neatNetwork->getConnections().size();
    }

    return m_sensory.getWeightCount() +
           m_emotional.getWeightCount() +
           m_decision.getWeightCount() +
           m_motor.getWeightCount();
}

float CreatureBrain::getAverageActivity() const {
    if (m_useNEATGenome && m_neatNetwork) {
        return m_neatNetwork->getAverageActivity();
    }
    return 0.0f;  // Not tracked for modular brain
}

float CreatureBrain::getComplexity() const {
    if (m_useNEATGenome && m_neatNetwork) {
        return m_neatNetwork->getNetworkComplexity();
    }
    return static_cast<float>(getTotalWeightCount());
}

void CreatureBrain::setNEATGenome(const NEATGenome& genome) {
    m_genome = genome;
    m_neatNetwork = genome.buildNetwork();
    m_useNEATGenome = true;
}

} // namespace ai
