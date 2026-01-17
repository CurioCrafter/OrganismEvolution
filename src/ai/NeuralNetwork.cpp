#include "NeuralNetwork.h"
#include "NEATGenome.h"
#include <queue>
#include <set>
#include <algorithm>
#include <stdexcept>

namespace ai {

// ============================================================================
// Build from NEAT Genome
// ============================================================================

void NeuralNetwork::buildFromGenome(const NEATGenome& genome) {
    m_nodes.clear();
    m_connections.clear();
    m_nodeIndex.clear();
    m_executionOrder.clear();
    m_inputCount = 0;
    m_outputCount = 0;
    m_nextNodeId = 0;

    // Copy nodes from genome
    for (const auto& geneNode : genome.getNodes()) {
        m_nodes.emplace_back(geneNode.id, geneNode.type, geneNode.activation,
                             geneNode.bias, geneNode.layer);
        m_nodeIndex[geneNode.id] = m_nodes.size() - 1;

        if (geneNode.id >= m_nextNodeId) {
            m_nextNodeId = geneNode.id + 1;
        }

        if (geneNode.type == NodeType::INPUT) {
            m_inputCount++;
        } else if (geneNode.type == NodeType::OUTPUT) {
            m_outputCount++;
        }
    }

    // Copy connections from genome
    for (const auto& geneConn : genome.getConnections()) {
        Connection conn(geneConn.innovation, geneConn.fromNode, geneConn.toNode,
                       geneConn.weight, geneConn.enabled, geneConn.recurrent);
        conn.plastic = geneConn.plastic;
        conn.plasticityRate = geneConn.plasticityRate;
        m_connections.push_back(conn);

        if (geneConn.innovation >= m_nextInnovation) {
            m_nextInnovation = geneConn.innovation + 1;
        }
    }

    // Compute execution order
    computeLayers();
    computeExecutionOrder();
}

// ============================================================================
// Forward Pass
// ============================================================================

std::vector<float> NeuralNetwork::forward(const std::vector<float>& inputs) {
    if (m_executionOrder.empty()) {
        computeLayers();
        computeExecutionOrder();
    }

    // Store previous values for recurrent connections
    for (auto& node : m_nodes) {
        node.prevValue = node.value;
        node.inputSum = 0.0f;
    }

    // Set input values
    int inputIdx = 0;
    for (auto& node : m_nodes) {
        if (node.type == NodeType::INPUT) {
            if (inputIdx < static_cast<int>(inputs.size())) {
                node.value = inputs[inputIdx++];
            } else {
                node.value = 0.0f;
            }
        } else if (node.type == NodeType::BIAS) {
            node.value = 1.0f;
        }
    }

    // Process nodes in execution order
    for (int nodeId : m_executionOrder) {
        auto it = m_nodeIndex.find(nodeId);
        if (it == m_nodeIndex.end()) continue;

        Node& node = m_nodes[it->second];

        // Skip input and bias nodes
        if (node.type == NodeType::INPUT || node.type == NodeType::BIAS) {
            continue;
        }

        // Sum inputs from all incoming connections
        float sum = node.bias;

        for (const auto& conn : m_connections) {
            if (conn.toNode != nodeId || !conn.enabled) continue;

            auto fromIt = m_nodeIndex.find(conn.fromNode);
            if (fromIt == m_nodeIndex.end()) continue;

            const Node& fromNode = m_nodes[fromIt->second];

            // Use previous value for recurrent connections
            float inputValue = conn.recurrent ? fromNode.prevValue : fromNode.value;
            sum += inputValue * conn.weight;
        }

        node.inputSum = sum;
        node.value = activate(sum, node.activation);

        // Update activity tracking (exponential moving average)
        node.activity = 0.95f * node.activity + 0.05f * std::abs(node.value);
    }

    // Collect outputs
    std::vector<float> outputs;
    outputs.reserve(m_outputCount);

    for (const auto& node : m_nodes) {
        if (node.type == NodeType::OUTPUT) {
            outputs.push_back(node.value);
        }
    }

    return outputs;
}

// ============================================================================
// Reset
// ============================================================================

void NeuralNetwork::reset() {
    for (auto& node : m_nodes) {
        node.reset();
    }
    for (auto& conn : m_connections) {
        conn.resetPlasticity();
    }
}

// ============================================================================
// Plasticity and Learning
// ============================================================================

void NeuralNetwork::accumulateHebbian() {
    for (auto& conn : m_connections) {
        if (!conn.enabled || !conn.plastic) continue;

        auto fromIt = m_nodeIndex.find(conn.fromNode);
        auto toIt = m_nodeIndex.find(conn.toNode);
        if (fromIt == m_nodeIndex.end() || toIt == m_nodeIndex.end()) continue;

        const Node& fromNode = m_nodes[fromIt->second];
        const Node& toNode = m_nodes[toIt->second];

        // Hebbian correlation: pre * post
        float preActivity = conn.recurrent ? fromNode.prevValue : fromNode.value;
        float correlation = preActivity * toNode.value;

        // Accumulate in eligibility trace
        conn.eligibility = 0.95f * conn.eligibility + correlation;
        conn.hebbianTerm = correlation;
    }
}

void NeuralNetwork::updatePlasticity(float reward, float learningRate) {
    for (auto& conn : m_connections) {
        if (!conn.enabled || !conn.plastic) continue;

        // Three-factor learning rule: eligibility * reward * learning_rate
        float deltaW = learningRate * conn.plasticityRate *
                       conn.eligibility * reward;

        // Weight decay (regularization)
        deltaW -= 0.0001f * conn.weight;

        // Update weight with clipping
        conn.weight += deltaW;
        conn.weight = std::max(-5.0f, std::min(5.0f, conn.weight));
    }
}

void NeuralNetwork::decayEligibility(float decay) {
    for (auto& conn : m_connections) {
        conn.eligibility *= decay;
    }
}

// ============================================================================
// Topology Computation
// ============================================================================

void NeuralNetwork::computeLayers() {
    // Assign layers based on longest path from inputs
    // Input/Bias nodes are layer 0
    // Output nodes get the highest layer number

    // Initialize layers
    for (auto& node : m_nodes) {
        if (node.type == NodeType::INPUT || node.type == NodeType::BIAS) {
            node.layer = 0;
        } else {
            node.layer = -1;  // Unassigned
        }
    }

    // Build adjacency lists (excluding recurrent connections)
    std::unordered_map<int, std::vector<int>> incoming;
    for (const auto& conn : m_connections) {
        if (conn.enabled && !conn.recurrent) {
            incoming[conn.toNode].push_back(conn.fromNode);
        }
    }

    // Iteratively compute layers
    bool changed = true;
    int maxIterations = static_cast<int>(m_nodes.size()) + 1;
    int iteration = 0;

    while (changed && iteration < maxIterations) {
        changed = false;
        iteration++;

        for (auto& node : m_nodes) {
            if (node.layer >= 0) continue;  // Already assigned

            // Check if all incoming nodes have layers assigned
            auto it = incoming.find(node.id);
            if (it == incoming.end() || it->second.empty()) {
                // No incoming connections (orphan hidden node)
                node.layer = 1;
                changed = true;
                continue;
            }

            int maxIncomingLayer = -1;
            bool allAssigned = true;

            for (int fromId : it->second) {
                auto fromIt = m_nodeIndex.find(fromId);
                if (fromIt != m_nodeIndex.end()) {
                    int fromLayer = m_nodes[fromIt->second].layer;
                    if (fromLayer < 0) {
                        allAssigned = false;
                        break;
                    }
                    maxIncomingLayer = std::max(maxIncomingLayer, fromLayer);
                }
            }

            if (allAssigned) {
                node.layer = maxIncomingLayer + 1;
                changed = true;
            }
        }
    }

    // Handle any remaining unassigned nodes (cycles in non-recurrent connections)
    for (auto& node : m_nodes) {
        if (node.layer < 0) {
            node.layer = 1;  // Default to hidden layer 1
        }
    }

    // Find max layer (excluding output nodes)
    int maxHiddenLayer = 0;
    for (const auto& node : m_nodes) {
        if (node.type != NodeType::OUTPUT) {
            maxHiddenLayer = std::max(maxHiddenLayer, node.layer);
        }
    }

    // Set output nodes to be one layer higher than the highest hidden layer
    int outputLayer = maxHiddenLayer + 1;
    for (auto& node : m_nodes) {
        if (node.type == NodeType::OUTPUT) {
            node.layer = outputLayer;
        }
    }
}

void NeuralNetwork::computeExecutionOrder() {
    m_executionOrder.clear();

    // Sort nodes by layer
    std::vector<std::pair<int, int>> nodeLayerPairs;  // (layer, nodeId)
    for (const auto& node : m_nodes) {
        nodeLayerPairs.emplace_back(node.layer, node.id);
    }

    std::sort(nodeLayerPairs.begin(), nodeLayerPairs.end());

    // Build execution order
    for (const auto& pair : nodeLayerPairs) {
        m_executionOrder.push_back(pair.second);
    }
}

bool NeuralNetwork::wouldCreateCycle(int from, int to) const {
    // BFS from 'to' to see if we can reach 'from'
    // If we can, adding from->to would create a cycle

    std::set<int> visited;
    std::queue<int> queue;
    queue.push(to);

    while (!queue.empty()) {
        int current = queue.front();
        queue.pop();

        if (current == from) {
            return true;  // Would create cycle
        }

        if (visited.count(current)) continue;
        visited.insert(current);

        // Find all nodes that 'current' connects to
        for (const auto& conn : m_connections) {
            if (conn.fromNode == current && conn.enabled && !conn.recurrent) {
                queue.push(conn.toNode);
            }
        }
    }

    return false;
}

} // namespace ai
