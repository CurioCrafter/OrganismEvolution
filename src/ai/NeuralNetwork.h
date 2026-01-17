#pragma once

#include <vector>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <random>
#include <memory>
#include <functional>

namespace ai {

// Forward declarations
class NeuralNetwork;
class NEATGenome;

// ============================================================================
// Enums and Constants
// ============================================================================

enum class NodeType {
    INPUT,
    HIDDEN,
    OUTPUT,
    BIAS
};

enum class ActivationType {
    LINEAR,
    SIGMOID,
    TANH,
    RELU,
    LEAKY_RELU,
    ELU,
    GAUSSIAN,
    SINE,
    STEP
};

// ============================================================================
// Node Definition
// ============================================================================

struct Node {
    int id;
    NodeType type;
    ActivationType activation;
    float bias;
    int layer;              // Topological layer (for feedforward ordering)

    // Runtime state
    float value;            // Current activation value
    float prevValue;        // Previous timestep value (for recurrent)
    float inputSum;         // Sum of inputs before activation

    // Plasticity state
    float activity;         // Recent average activity (for homeostasis)
    float eligibility;      // Eligibility trace for learning

    Node(int id_, NodeType type_, ActivationType act_ = ActivationType::TANH,
         float bias_ = 0.0f, int layer_ = 0)
        : id(id_), type(type_), activation(act_), bias(bias_), layer(layer_),
          value(0.0f), prevValue(0.0f), inputSum(0.0f),
          activity(0.0f), eligibility(0.0f) {}

    void reset() {
        value = 0.0f;
        prevValue = 0.0f;
        inputSum = 0.0f;
        activity = 0.0f;
        eligibility = 0.0f;
    }
};

// ============================================================================
// Connection Definition
// ============================================================================

struct Connection {
    int innovation;         // NEAT innovation number
    int fromNode;           // Source node ID
    int toNode;             // Target node ID
    float weight;           // Connection weight
    bool enabled;           // Is connection active?
    bool recurrent;         // Is this a recurrent connection?

    // Plasticity parameters
    bool plastic;           // Can this connection learn?
    float plasticityRate;   // Per-connection learning rate multiplier
    float eligibility;      // Eligibility trace
    float hebbianTerm;      // Accumulated Hebbian correlation

    Connection(int innov, int from, int to, float w = 0.0f,
               bool enabled_ = true, bool recurrent_ = false)
        : innovation(innov), fromNode(from), toNode(to), weight(w),
          enabled(enabled_), recurrent(recurrent_),
          plastic(true), plasticityRate(1.0f),
          eligibility(0.0f), hebbianTerm(0.0f) {}

    void resetPlasticity() {
        eligibility = 0.0f;
        hebbianTerm = 0.0f;
    }
};

// ============================================================================
// Activation Functions
// ============================================================================

inline float activate(float x, ActivationType type) {
    switch (type) {
        case ActivationType::LINEAR:
            return x;
        case ActivationType::SIGMOID:
            return 1.0f / (1.0f + std::exp(-x));
        case ActivationType::TANH:
            return std::tanh(x);
        case ActivationType::RELU:
            return std::max(0.0f, x);
        case ActivationType::LEAKY_RELU:
            return x > 0 ? x : 0.01f * x;
        case ActivationType::ELU:
            return x > 0 ? x : std::exp(x) - 1.0f;
        case ActivationType::GAUSSIAN:
            return std::exp(-x * x);
        case ActivationType::SINE:
            return std::sin(x);
        case ActivationType::STEP:
            return x > 0 ? 1.0f : 0.0f;
        default:
            return std::tanh(x);
    }
}

inline float activateDerivative(float x, float output, ActivationType type) {
    switch (type) {
        case ActivationType::LINEAR:
            return 1.0f;
        case ActivationType::SIGMOID:
            return output * (1.0f - output);
        case ActivationType::TANH:
            return 1.0f - output * output;
        case ActivationType::RELU:
            return x > 0 ? 1.0f : 0.0f;
        case ActivationType::LEAKY_RELU:
            return x > 0 ? 1.0f : 0.01f;
        case ActivationType::ELU:
            return x > 0 ? 1.0f : output + 1.0f;
        case ActivationType::GAUSSIAN:
            return -2.0f * x * output;
        case ActivationType::SINE:
            return std::cos(x);
        case ActivationType::STEP:
            return 0.0f;
        default:
            return 1.0f - output * output;
    }
}

// ============================================================================
// Neural Network Class
// ============================================================================

class NeuralNetwork {
public:
    NeuralNetwork() = default;

    // Build network from NEAT genome
    void buildFromGenome(const NEATGenome& genome);

    // Manual network construction
    int addNode(NodeType type, ActivationType activation = ActivationType::TANH,
                float bias = 0.0f);
    void addConnection(int from, int to, float weight, bool recurrent = false);
    void setConnection(int from, int to, float weight);

    // Network execution
    std::vector<float> forward(const std::vector<float>& inputs);
    void reset();  // Reset all node states

    // Plasticity and learning
    void updatePlasticity(float reward, float learningRate);
    void accumulateHebbian();  // Accumulate Hebbian correlations
    void decayEligibility(float decay);

    // Accessors
    int getInputCount() const { return m_inputCount; }
    int getOutputCount() const { return m_outputCount; }
    int getNodeCount() const { return static_cast<int>(m_nodes.size()); }
    int getConnectionCount() const { return static_cast<int>(m_connections.size()); }
    int getEnabledConnectionCount() const;

    const std::vector<Node>& getNodes() const { return m_nodes; }
    const std::vector<Connection>& getConnections() const { return m_connections; }
    std::vector<Node>& getNodes() { return m_nodes; }
    std::vector<Connection>& getConnections() { return m_connections; }

    Node* getNode(int id);
    Connection* getConnection(int from, int to);

    // Serialization helpers
    std::vector<float> getWeights() const;
    void setWeights(const std::vector<float>& weights);

    // Statistics
    float getAverageActivity() const;
    float getNetworkComplexity() const;

private:
    std::vector<Node> m_nodes;
    std::vector<Connection> m_connections;
    std::vector<int> m_executionOrder;  // Topologically sorted node IDs
    std::unordered_map<int, size_t> m_nodeIndex;  // Node ID -> index in m_nodes

    int m_inputCount = 0;
    int m_outputCount = 0;
    int m_nextNodeId = 0;
    int m_nextInnovation = 0;

    void computeExecutionOrder();
    void computeLayers();
    bool wouldCreateCycle(int from, int to) const;
};

// ============================================================================
// Inline Implementations
// ============================================================================

inline int NeuralNetwork::addNode(NodeType type, ActivationType activation, float bias) {
    int id = m_nextNodeId++;
    int layer = 0;

    if (type == NodeType::INPUT || type == NodeType::BIAS) {
        layer = 0;
        if (type == NodeType::INPUT) m_inputCount++;
    } else if (type == NodeType::OUTPUT) {
        layer = 9999;  // Will be adjusted in computeLayers
        m_outputCount++;
    } else {
        layer = 1;  // Hidden nodes start in layer 1
    }

    m_nodes.emplace_back(id, type, activation, bias, layer);
    m_nodeIndex[id] = m_nodes.size() - 1;
    return id;
}

inline void NeuralNetwork::addConnection(int from, int to, float weight, bool recurrent) {
    int innovation = m_nextInnovation++;
    m_connections.emplace_back(innovation, from, to, weight, true, recurrent);
}

inline void NeuralNetwork::setConnection(int from, int to, float weight) {
    for (auto& conn : m_connections) {
        if (conn.fromNode == from && conn.toNode == to) {
            conn.weight = weight;
            return;
        }
    }
    // Connection doesn't exist, add it
    addConnection(from, to, weight, false);
}

inline Node* NeuralNetwork::getNode(int id) {
    auto it = m_nodeIndex.find(id);
    if (it != m_nodeIndex.end()) {
        return &m_nodes[it->second];
    }
    return nullptr;
}

inline Connection* NeuralNetwork::getConnection(int from, int to) {
    for (auto& conn : m_connections) {
        if (conn.fromNode == from && conn.toNode == to) {
            return &conn;
        }
    }
    return nullptr;
}

inline int NeuralNetwork::getEnabledConnectionCount() const {
    int count = 0;
    for (const auto& conn : m_connections) {
        if (conn.enabled) count++;
    }
    return count;
}

inline std::vector<float> NeuralNetwork::getWeights() const {
    std::vector<float> weights;
    weights.reserve(m_connections.size());
    for (const auto& conn : m_connections) {
        weights.push_back(conn.weight);
    }
    return weights;
}

inline void NeuralNetwork::setWeights(const std::vector<float>& weights) {
    size_t n = std::min(weights.size(), m_connections.size());
    for (size_t i = 0; i < n; i++) {
        m_connections[i].weight = weights[i];
    }
}

inline float NeuralNetwork::getAverageActivity() const {
    if (m_nodes.empty()) return 0.0f;
    float sum = 0.0f;
    for (const auto& node : m_nodes) {
        sum += std::abs(node.value);
    }
    return sum / m_nodes.size();
}

inline float NeuralNetwork::getNetworkComplexity() const {
    return static_cast<float>(getEnabledConnectionCount()) +
           static_cast<float>(m_nodes.size()) * 0.5f;
}

} // namespace ai
