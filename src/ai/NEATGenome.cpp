#include "NEATGenome.h"
#include <algorithm>
#include <queue>
#include <set>
#include <cmath>

namespace ai {

// ============================================================================
// Create Minimal Network
// ============================================================================

void NEATGenome::createMinimal(int numInputs, int numOutputs, std::mt19937& rng) {
    m_nodes.clear();
    m_connections.clear();
    m_inputCount = numInputs;
    m_outputCount = numOutputs;

    auto& tracker = InnovationTracker::instance();

    // Create input nodes
    for (int i = 0; i < numInputs; i++) {
        int id = tracker.getNextNodeId();
        m_nodes.emplace_back(id, NodeType::INPUT, ActivationType::LINEAR, 0.0f, 0);
    }

    // Create bias node
    int biasId = tracker.getNextNodeId();
    m_nodes.emplace_back(biasId, NodeType::BIAS, ActivationType::LINEAR, 0.0f, 0);

    // Create output nodes
    std::vector<int> outputIds;
    for (int i = 0; i < numOutputs; i++) {
        int id = tracker.getNextNodeId();
        m_nodes.emplace_back(id, NodeType::OUTPUT, ActivationType::TANH, 0.0f, 1);
        outputIds.push_back(id);
    }

    // Create initial connections (all inputs to all outputs)
    std::uniform_real_distribution<float> weightDist(-1.0f, 1.0f);

    for (const auto& inputNode : m_nodes) {
        if (inputNode.type == NodeType::INPUT || inputNode.type == NodeType::BIAS) {
            for (int outId : outputIds) {
                int innovation = tracker.getConnectionInnovation(inputNode.id, outId);
                float weight = weightDist(rng);
                m_connections.emplace_back(innovation, inputNode.id, outId, weight, true, false);
            }
        }
    }
}

// ============================================================================
// Mutation Operations
// ============================================================================

void NEATGenome::mutate(std::mt19937& rng, const MutationParams& params) {
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);

    // Weight mutation (most common)
    if (prob(rng) < params.mutateWeightProb) {
        mutateWeights(rng, params.weightPerturbProb, params.weightPerturbStrength, 1.0f - params.weightPerturbProb);
    }

    // Bias mutation
    if (prob(rng) < params.mutateBiasProb) {
        mutateBias(rng, params.biasPerturbStrength);
    }

    // Structural mutations
    if (prob(rng) < params.addConnectionProb) {
        mutateAddConnection(rng, params.allowRecurrent);
    }

    if (prob(rng) < params.addNodeProb) {
        mutateAddNode(rng);
    }

    // Toggle enable
    if (prob(rng) < params.toggleEnableProb) {
        mutateToggleEnable(rng);
    }

    // Activation mutation
    if (prob(rng) < params.mutateActivationProb) {
        mutateActivation(rng);
    }

    // Plasticity mutation
    if (prob(rng) < params.mutatePlasticityProb) {
        mutatePlasticity(rng);
    }

    // ========================================================================
    // Advanced Topology Evolution Mutations
    // ========================================================================

    // Add module (cluster of connected nodes)
    if (prob(rng) < params.addModuleProb) {
        mutateAddModule(rng, params.moduleSize);
    }

    // Add recurrent connection
    if (prob(rng) < params.addRecurrenceProb) {
        mutateAddRecurrence(rng);
    }

    // Split existing module
    if (prob(rng) < params.splitModuleProb) {
        mutateSplitModule(rng);
    }

    // Prune dead-end nodes
    if (prob(rng) < params.pruneDeadEndsProb) {
        mutatePruneDeadEnds();
    }

    // Add modulatory node
    if (prob(rng) < params.addModulatoryNodeProb) {
        addModulatoryNode(rng);
    }

    // Optimize for efficiency
    if (prob(rng) < params.optimizeEfficiencyProb) {
        optimizeForEfficiency(rng, params.efficiencyPruneThreshold);
    }
}

void NEATGenome::mutateWeights(std::mt19937& rng, float perturbChance,
                                float perturbStrength, float replaceChance) {
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);
    std::uniform_real_distribution<float> perturbDist(-perturbStrength, perturbStrength);
    std::uniform_real_distribution<float> weightDist(-2.0f, 2.0f);

    for (auto& conn : m_connections) {
        if (prob(rng) < perturbChance) {
            // Perturb existing weight
            conn.weight += perturbDist(rng);
            conn.weight = std::max(-5.0f, std::min(5.0f, conn.weight));
        } else if (prob(rng) < replaceChance) {
            // Replace with new random weight
            conn.weight = weightDist(rng);
        }
    }
}

void NEATGenome::mutateAddConnection(std::mt19937& rng, bool allowRecurrent) {
    auto validSources = getValidSourceNodes();
    auto validTargets = getValidTargetNodes();

    if (validSources.empty() || validTargets.empty()) return;

    // Try to find a valid new connection
    const int maxAttempts = 20;
    std::uniform_int_distribution<int> srcDist(0, static_cast<int>(validSources.size()) - 1);
    std::uniform_int_distribution<int> tgtDist(0, static_cast<int>(validTargets.size()) - 1);
    std::uniform_real_distribution<float> weightDist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);

    for (int attempt = 0; attempt < maxAttempts; attempt++) {
        int fromId = validSources[srcDist(rng)];
        int toId = validTargets[tgtDist(rng)];

        // Skip if same node
        if (fromId == toId) continue;

        // Skip if connection exists
        if (connectionExists(fromId, toId)) continue;

        // Check if this would be a recurrent connection
        const NodeGene* fromNode = getNode(fromId);
        const NodeGene* toNode = getNode(toId);

        if (!fromNode || !toNode) continue;

        bool isRecurrent = (fromNode->layer >= toNode->layer);

        // Skip recurrent connections if not allowed
        if (isRecurrent && !allowRecurrent) continue;

        // For non-recurrent, check for cycles
        if (!isRecurrent && wouldCreateCycle(fromId, toId)) {
            // Mark as recurrent instead
            isRecurrent = true;
            if (!allowRecurrent) continue;
        }

        // Add the connection
        auto& tracker = InnovationTracker::instance();
        int innovation = tracker.getConnectionInnovation(fromId, toId);
        float weight = weightDist(rng);
        m_connections.emplace_back(innovation, fromId, toId, weight, true, isRecurrent);
        return;
    }
}

void NEATGenome::mutateAddNode(std::mt19937& rng) {
    // Get enabled connections
    std::vector<size_t> enabledIndices;
    for (size_t i = 0; i < m_connections.size(); i++) {
        if (m_connections[i].enabled && !m_connections[i].recurrent) {
            enabledIndices.push_back(i);
        }
    }

    if (enabledIndices.empty()) return;

    // Select random connection to split
    std::uniform_int_distribution<size_t> dist(0, enabledIndices.size() - 1);
    size_t connIdx = enabledIndices[dist(rng)];
    ConnectionGene& oldConn = m_connections[connIdx];

    // Disable old connection
    oldConn.enabled = false;

    // Create new node
    auto& tracker = InnovationTracker::instance();
    int newNodeId = tracker.getNodeInnovation(oldConn.innovation);

    // Determine layer for new node
    const NodeGene* fromNode = getNode(oldConn.fromNode);
    const NodeGene* toNode = getNode(oldConn.toNode);
    int newLayer = fromNode ? (fromNode->layer + 1) : 1;
    if (toNode && newLayer >= toNode->layer) {
        // Need to shift layers of downstream nodes
        // For simplicity, just use midpoint layer concept
        newLayer = fromNode ? fromNode->layer + 1 : 1;
    }

    // Select random activation
    std::uniform_int_distribution<int> actDist(0, 4);
    ActivationType activations[] = {
        ActivationType::TANH,
        ActivationType::RELU,
        ActivationType::SIGMOID,
        ActivationType::LEAKY_RELU,
        ActivationType::ELU
    };
    ActivationType newAct = activations[actDist(rng)];

    m_nodes.emplace_back(newNodeId, NodeType::HIDDEN, newAct, 0.0f, newLayer);

    // Create two new connections
    // Connection 1: from original source to new node (weight = 1.0)
    int innov1 = tracker.getConnectionInnovation(oldConn.fromNode, newNodeId);
    m_connections.emplace_back(innov1, oldConn.fromNode, newNodeId, 1.0f, true, false);

    // Connection 2: from new node to original target (weight = old weight)
    int innov2 = tracker.getConnectionInnovation(newNodeId, oldConn.toNode);
    m_connections.emplace_back(innov2, newNodeId, oldConn.toNode, oldConn.weight, true, false);
}

void NEATGenome::mutateToggleEnable(std::mt19937& rng) {
    if (m_connections.empty()) return;

    std::uniform_int_distribution<size_t> dist(0, m_connections.size() - 1);
    size_t idx = dist(rng);

    // Don't disable if it would leave a node disconnected
    // (simplified check - just toggle)
    m_connections[idx].enabled = !m_connections[idx].enabled;
}

void NEATGenome::mutateActivation(std::mt19937& rng) {
    // Get hidden nodes
    std::vector<size_t> hiddenIndices;
    for (size_t i = 0; i < m_nodes.size(); i++) {
        if (m_nodes[i].type == NodeType::HIDDEN) {
            hiddenIndices.push_back(i);
        }
    }

    if (hiddenIndices.empty()) return;

    std::uniform_int_distribution<size_t> nodeDist(0, hiddenIndices.size() - 1);
    size_t nodeIdx = hiddenIndices[nodeDist(rng)];

    // Select new activation
    std::uniform_int_distribution<int> actDist(0, 7);
    ActivationType activations[] = {
        ActivationType::TANH,
        ActivationType::RELU,
        ActivationType::SIGMOID,
        ActivationType::LEAKY_RELU,
        ActivationType::ELU,
        ActivationType::GAUSSIAN,
        ActivationType::SINE,
        ActivationType::LINEAR
    };

    m_nodes[nodeIdx].activation = activations[actDist(rng)];
}

void NEATGenome::mutateBias(std::mt19937& rng, float strength) {
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);
    std::uniform_real_distribution<float> perturbDist(-strength, strength);

    for (auto& node : m_nodes) {
        if (node.type == NodeType::HIDDEN || node.type == NodeType::OUTPUT) {
            if (prob(rng) < 0.3f) {
                node.bias += perturbDist(rng);
                node.bias = std::max(-2.0f, std::min(2.0f, node.bias));
            }
        }
    }
}

void NEATGenome::mutatePlasticity(std::mt19937& rng) {
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);
    std::uniform_real_distribution<float> perturbDist(-0.2f, 0.2f);

    for (auto& conn : m_connections) {
        if (prob(rng) < 0.2f) {
            // Toggle plasticity
            conn.plastic = !conn.plastic;
        }
        if (conn.plastic && prob(rng) < 0.3f) {
            // Mutate plasticity rate
            conn.plasticityRate += perturbDist(rng);
            conn.plasticityRate = std::max(0.0f, std::min(2.0f, conn.plasticityRate));
        }
    }
}

// ============================================================================
// Advanced Topology Evolution Mutations
// ============================================================================

void NEATGenome::mutateAddModule(std::mt19937& rng, int moduleSize) {
    if (moduleSize < 2) moduleSize = 2;
    if (moduleSize > 10) moduleSize = 10;

    auto& tracker = InnovationTracker::instance();
    std::uniform_real_distribution<float> weightDist(-1.0f, 1.0f);
    std::uniform_int_distribution<int> actDist(0, 4);

    ActivationType activations[] = {
        ActivationType::TANH,
        ActivationType::RELU,
        ActivationType::SIGMOID,
        ActivationType::LEAKY_RELU,
        ActivationType::ELU
    };

    // Create new region for this module
    int regionId = m_nextRegionId++;
    BrainRegion region(regionId);
    region.generationFormed = m_generation;
    region.function = "integration";

    // Find the maximum layer of hidden nodes to place module after
    int baseLayer = 1;
    for (const auto& node : m_nodes) {
        if (node.type == NodeType::HIDDEN) {
            baseLayer = std::max(baseLayer, node.layer + 1);
        }
    }

    // Create module nodes
    std::vector<int> moduleNodeIds;
    for (int i = 0; i < moduleSize; i++) {
        int nodeId = tracker.getNextNodeId();
        ActivationType act = activations[actDist(rng)];
        m_nodes.emplace_back(nodeId, NodeType::HIDDEN, act, 0.0f, baseLayer);
        m_nodes.back().regionId = regionId;
        moduleNodeIds.push_back(nodeId);
        region.nodeIds.push_back(nodeId);
    }

    // Connect module nodes internally (create a small fully connected sub-network)
    for (int i = 0; i < moduleSize; i++) {
        for (int j = 0; j < moduleSize; j++) {
            if (i != j) {
                int fromId = moduleNodeIds[i];
                int toId = moduleNodeIds[j];
                int innovation = tracker.getConnectionInnovation(fromId, toId);
                float weight = weightDist(rng);
                m_connections.emplace_back(innovation, fromId, toId, weight, true, false);
                region.internalConnections.push_back(innovation);
            }
        }
    }

    // Connect module to existing network
    auto validSources = getValidSourceNodes();
    auto validTargets = getValidTargetNodes();

    // Remove module nodes from targets (we'll connect TO module, not from it to itself again)
    for (int modNodeId : moduleNodeIds) {
        validTargets.erase(std::remove(validTargets.begin(), validTargets.end(), modNodeId),
                          validTargets.end());
    }

    // Connect one random source to module input
    if (!validSources.empty()) {
        std::uniform_int_distribution<size_t> srcDist(0, validSources.size() - 1);
        int srcId = validSources[srcDist(rng)];
        int tgtId = moduleNodeIds[0];  // First module node is input
        int innovation = tracker.getConnectionInnovation(srcId, tgtId);
        m_connections.emplace_back(innovation, srcId, tgtId, weightDist(rng), true, false);
        region.inputConnections.push_back(innovation);
    }

    // Connect module output to one random target
    if (!validTargets.empty()) {
        std::uniform_int_distribution<size_t> tgtDist(0, validTargets.size() - 1);
        int srcId = moduleNodeIds.back();  // Last module node is output
        int tgtId = validTargets[tgtDist(rng)];
        if (srcId != tgtId) {
            int innovation = tracker.getConnectionInnovation(srcId, tgtId);
            m_connections.emplace_back(innovation, srcId, tgtId, weightDist(rng), true, false);
            region.outputConnections.push_back(innovation);
        }
    }

    m_regions.push_back(region);
}

void NEATGenome::mutateAddRecurrence(std::mt19937& rng) {
    // Get all hidden and output nodes as potential targets for recurrence
    std::vector<int> candidates;
    for (const auto& node : m_nodes) {
        if (node.type == NodeType::HIDDEN || node.type == NodeType::OUTPUT) {
            candidates.push_back(node.id);
        }
    }

    if (candidates.size() < 2) return;

    const int maxAttempts = 20;
    std::uniform_int_distribution<size_t> nodeDist(0, candidates.size() - 1);
    std::uniform_real_distribution<float> weightDist(-1.0f, 1.0f);

    for (int attempt = 0; attempt < maxAttempts; attempt++) {
        int fromId = candidates[nodeDist(rng)];
        int toId = candidates[nodeDist(rng)];

        // Skip if same node (self-connection) - could be allowed but skip for now
        if (fromId == toId) continue;

        // Skip if connection exists
        if (connectionExists(fromId, toId)) continue;

        const NodeGene* fromNode = getNode(fromId);
        const NodeGene* toNode = getNode(toId);
        if (!fromNode || !toNode) continue;

        // Only create recurrent connection if from is same or higher layer than to
        if (fromNode->layer >= toNode->layer) {
            auto& tracker = InnovationTracker::instance();
            int innovation = tracker.getConnectionInnovation(fromId, toId);
            m_connections.emplace_back(innovation, fromId, toId, weightDist(rng), true, true);
            return;
        }
    }
}

void NEATGenome::mutateSplitModule(std::mt19937& rng) {
    if (m_regions.empty()) {
        // No modules to split, try to identify some first
        updateRegions();
        if (m_regions.empty()) return;
    }

    // Find a region with enough nodes to split
    std::vector<size_t> splittableRegions;
    for (size_t i = 0; i < m_regions.size(); i++) {
        if (m_regions[i].nodeIds.size() >= 4) {
            splittableRegions.push_back(i);
        }
    }

    if (splittableRegions.empty()) return;

    std::uniform_int_distribution<size_t> regionDist(0, splittableRegions.size() - 1);
    size_t regionIdx = splittableRegions[regionDist(rng)];
    BrainRegion& oldRegion = m_regions[regionIdx];

    // Split nodes into two groups
    int halfSize = static_cast<int>(oldRegion.nodeIds.size()) / 2;

    // Create new region
    int newRegionId = m_nextRegionId++;
    BrainRegion newRegion(newRegionId);
    newRegion.generationFormed = m_generation;
    newRegion.parentRegionId = oldRegion.id;
    newRegion.function = oldRegion.function;

    // Move half the nodes to new region
    for (int i = halfSize; i < static_cast<int>(oldRegion.nodeIds.size()); i++) {
        int nodeId = oldRegion.nodeIds[i];
        NodeGene* node = getNode(nodeId);
        if (node) {
            node->regionId = newRegionId;
            newRegion.nodeIds.push_back(nodeId);
        }
    }
    oldRegion.nodeIds.resize(halfSize);

    // Reclassify connections
    std::unordered_set<int> oldNodeSet(oldRegion.nodeIds.begin(), oldRegion.nodeIds.end());
    std::unordered_set<int> newNodeSet(newRegion.nodeIds.begin(), newRegion.nodeIds.end());

    oldRegion.internalConnections.clear();
    oldRegion.inputConnections.clear();
    oldRegion.outputConnections.clear();
    newRegion.internalConnections.clear();

    for (const auto& conn : m_connections) {
        if (!conn.enabled) continue;

        bool fromOld = oldNodeSet.count(conn.fromNode) > 0;
        bool toOld = oldNodeSet.count(conn.toNode) > 0;
        bool fromNew = newNodeSet.count(conn.fromNode) > 0;
        bool toNew = newNodeSet.count(conn.toNode) > 0;

        if (fromOld && toOld) {
            oldRegion.internalConnections.push_back(conn.innovation);
        } else if (fromNew && toNew) {
            newRegion.internalConnections.push_back(conn.innovation);
        } else if (!fromOld && !fromNew && toOld) {
            oldRegion.inputConnections.push_back(conn.innovation);
        } else if (!fromOld && !fromNew && toNew) {
            newRegion.inputConnections.push_back(conn.innovation);
        } else if (fromOld && !toOld && !toNew) {
            oldRegion.outputConnections.push_back(conn.innovation);
        } else if (fromNew && !toOld && !toNew) {
            newRegion.outputConnections.push_back(conn.innovation);
        }
    }

    m_regions.push_back(newRegion);
}

void NEATGenome::mutatePruneDeadEnds() {
    // Find nodes that have no path to output or from input
    std::unordered_set<int> inputNodeIds;
    std::unordered_set<int> outputNodeIds;
    std::unordered_set<int> hiddenNodeIds;

    for (const auto& node : m_nodes) {
        if (node.type == NodeType::INPUT || node.type == NodeType::BIAS) {
            inputNodeIds.insert(node.id);
        } else if (node.type == NodeType::OUTPUT) {
            outputNodeIds.insert(node.id);
        } else if (node.type == NodeType::HIDDEN) {
            hiddenNodeIds.insert(node.id);
        }
    }

    if (hiddenNodeIds.empty()) return;

    // Build adjacency lists
    std::unordered_map<int, std::vector<int>> forwardEdges;  // node -> nodes it sends to
    std::unordered_map<int, std::vector<int>> backwardEdges; // node -> nodes it receives from

    for (const auto& conn : m_connections) {
        if (!conn.enabled || conn.recurrent) continue;
        forwardEdges[conn.fromNode].push_back(conn.toNode);
        backwardEdges[conn.toNode].push_back(conn.fromNode);
    }

    // Find nodes reachable from inputs (forward pass)
    std::unordered_set<int> reachableFromInput;
    std::queue<int> queue;
    for (int inputId : inputNodeIds) {
        queue.push(inputId);
        reachableFromInput.insert(inputId);
    }
    while (!queue.empty()) {
        int current = queue.front();
        queue.pop();
        for (int next : forwardEdges[current]) {
            if (reachableFromInput.count(next) == 0) {
                reachableFromInput.insert(next);
                queue.push(next);
            }
        }
    }

    // Find nodes that can reach outputs (backward pass)
    std::unordered_set<int> canReachOutput;
    for (int outputId : outputNodeIds) {
        queue.push(outputId);
        canReachOutput.insert(outputId);
    }
    while (!queue.empty()) {
        int current = queue.front();
        queue.pop();
        for (int prev : backwardEdges[current]) {
            if (canReachOutput.count(prev) == 0) {
                canReachOutput.insert(prev);
                queue.push(prev);
            }
        }
    }

    // Hidden nodes that are either not reachable from input OR cannot reach output are dead ends
    std::unordered_set<int> deadEndNodes;
    for (int hiddenId : hiddenNodeIds) {
        if (reachableFromInput.count(hiddenId) == 0 || canReachOutput.count(hiddenId) == 0) {
            deadEndNodes.insert(hiddenId);
        }
    }

    if (deadEndNodes.empty()) return;

    // Remove dead end nodes
    m_nodes.erase(
        std::remove_if(m_nodes.begin(), m_nodes.end(),
                       [&deadEndNodes](const NodeGene& n) {
                           return deadEndNodes.count(n.id) > 0;
                       }),
        m_nodes.end());

    // Disable connections to/from dead end nodes
    for (auto& conn : m_connections) {
        if (deadEndNodes.count(conn.fromNode) > 0 || deadEndNodes.count(conn.toNode) > 0) {
            conn.enabled = false;
        }
    }

    // Update regions
    for (auto& region : m_regions) {
        region.nodeIds.erase(
            std::remove_if(region.nodeIds.begin(), region.nodeIds.end(),
                           [&deadEndNodes](int id) { return deadEndNodes.count(id) > 0; }),
            region.nodeIds.end());
    }
}

void NEATGenome::mutateTransferModule(const NEATGenome& source, std::mt19937& rng) {
    // Get source regions
    const auto& sourceRegions = source.getRegions();
    if (sourceRegions.empty()) return;

    // Pick a random source region
    std::uniform_int_distribution<size_t> regionDist(0, sourceRegions.size() - 1);
    const BrainRegion& sourceRegion = sourceRegions[regionDist(rng)];

    if (sourceRegion.nodeIds.empty()) return;

    auto& tracker = InnovationTracker::instance();
    std::uniform_real_distribution<float> weightDist(-1.0f, 1.0f);

    // Create new region in this genome
    int newRegionId = m_nextRegionId++;
    BrainRegion newRegion(newRegionId);
    newRegion.generationFormed = m_generation;
    newRegion.function = sourceRegion.function;

    // Map old node IDs to new node IDs
    std::unordered_map<int, int> nodeIdMap;

    // Copy nodes from source region
    for (int oldNodeId : sourceRegion.nodeIds) {
        const NodeGene* sourceNode = source.getNode(oldNodeId);
        if (!sourceNode) continue;

        int newNodeId = tracker.getNextNodeId();
        nodeIdMap[oldNodeId] = newNodeId;

        NodeGene newNode(*sourceNode);
        newNode.id = newNodeId;
        newNode.regionId = newRegionId;
        m_nodes.push_back(newNode);
        newRegion.nodeIds.push_back(newNodeId);
    }

    // Copy internal connections
    for (const auto& conn : source.getConnections()) {
        if (!conn.enabled) continue;

        auto fromIt = nodeIdMap.find(conn.fromNode);
        auto toIt = nodeIdMap.find(conn.toNode);

        // Only copy connections that are internal to the transferred module
        if (fromIt != nodeIdMap.end() && toIt != nodeIdMap.end()) {
            int newFrom = fromIt->second;
            int newTo = toIt->second;
            int innovation = tracker.getConnectionInnovation(newFrom, newTo);
            m_connections.emplace_back(innovation, newFrom, newTo, conn.weight,
                                       true, conn.recurrent);
            newRegion.internalConnections.push_back(innovation);
        }
    }

    // Connect transferred module to existing network
    auto validSources = getValidSourceNodes();
    auto validTargets = getValidTargetNodes();

    // Remove transferred nodes from sources/targets
    for (const auto& pair : nodeIdMap) {
        validSources.erase(std::remove(validSources.begin(), validSources.end(), pair.second),
                          validSources.end());
        validTargets.erase(std::remove(validTargets.begin(), validTargets.end(), pair.second),
                          validTargets.end());
    }

    if (!validSources.empty() && !newRegion.nodeIds.empty()) {
        std::uniform_int_distribution<size_t> srcDist(0, validSources.size() - 1);
        int srcId = validSources[srcDist(rng)];
        int tgtId = newRegion.nodeIds[0];
        int innovation = tracker.getConnectionInnovation(srcId, tgtId);
        m_connections.emplace_back(innovation, srcId, tgtId, weightDist(rng), true, false);
        newRegion.inputConnections.push_back(innovation);
    }

    if (!validTargets.empty() && !newRegion.nodeIds.empty()) {
        std::uniform_int_distribution<size_t> tgtDist(0, validTargets.size() - 1);
        int srcId = newRegion.nodeIds.back();
        int tgtId = validTargets[tgtDist(rng)];
        int innovation = tracker.getConnectionInnovation(srcId, tgtId);
        m_connections.emplace_back(innovation, srcId, tgtId, weightDist(rng), true, false);
        newRegion.outputConnections.push_back(innovation);
    }

    m_regions.push_back(newRegion);
}

// ============================================================================
// Crossover
// ============================================================================

NEATGenome NEATGenome::crossover(const NEATGenome& fitter, const NEATGenome& other,
                                  std::mt19937& rng) {
    NEATGenome child;
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);

    // Copy all nodes from fitter parent
    child.m_nodes = fitter.m_nodes;
    child.m_inputCount = fitter.m_inputCount;
    child.m_outputCount = fitter.m_outputCount;

    // Add any unique hidden nodes from other parent
    std::set<int> childNodeIds;
    for (const auto& node : child.m_nodes) {
        childNodeIds.insert(node.id);
    }

    for (const auto& node : other.m_nodes) {
        if (node.type == NodeType::HIDDEN && childNodeIds.find(node.id) == childNodeIds.end()) {
            child.m_nodes.push_back(node);
            childNodeIds.insert(node.id);
        }
    }

    // Align genes by innovation number
    std::unordered_map<int, const ConnectionGene*> fitterGenes;
    std::unordered_map<int, const ConnectionGene*> otherGenes;

    for (const auto& conn : fitter.m_connections) {
        fitterGenes[conn.innovation] = &conn;
    }
    for (const auto& conn : other.m_connections) {
        otherGenes[conn.innovation] = &conn;
    }

    // Determine max innovation
    int maxInnov = 0;
    for (const auto& conn : fitter.m_connections) {
        maxInnov = std::max(maxInnov, conn.innovation);
    }
    for (const auto& conn : other.m_connections) {
        maxInnov = std::max(maxInnov, conn.innovation);
    }

    // Process each innovation number
    for (int innov = 0; innov <= maxInnov; innov++) {
        auto fitterIt = fitterGenes.find(innov);
        auto otherIt = otherGenes.find(innov);

        if (fitterIt != fitterGenes.end() && otherIt != otherGenes.end()) {
            // Matching gene - randomly choose parent
            const ConnectionGene* chosen = (prob(rng) < 0.5f) ?
                                           fitterIt->second : otherIt->second;
            child.m_connections.push_back(*chosen);

            // Handle disabled genes
            if (!fitterIt->second->enabled || !otherIt->second->enabled) {
                if (prob(rng) < 0.75f) {
                    child.m_connections.back().enabled = false;
                }
            }
        } else if (fitterIt != fitterGenes.end()) {
            // Disjoint/excess from fitter parent - always inherit
            child.m_connections.push_back(*fitterIt->second);
        }
        // Disjoint/excess from other parent are NOT inherited (fitter parent dominates)
    }

    return child;
}

// ============================================================================
// Compatibility Distance
// ============================================================================

float NEATGenome::compatibilityDistance(const NEATGenome& other,
                                         float c1, float c2, float c3) const {
    // Count excess, disjoint, and average weight difference

    std::set<int> thisInnovs, otherInnovs;
    for (const auto& conn : m_connections) {
        thisInnovs.insert(conn.innovation);
    }
    for (const auto& conn : other.m_connections) {
        otherInnovs.insert(conn.innovation);
    }

    int thisMax = thisInnovs.empty() ? 0 : *thisInnovs.rbegin();
    int otherMax = otherInnovs.empty() ? 0 : *otherInnovs.rbegin();
    int maxInnov = std::max(thisMax, otherMax);
    int minMax = std::min(thisMax, otherMax);

    int excess = 0;
    int disjoint = 0;
    float weightDiffSum = 0.0f;
    int matchingCount = 0;

    // Build weight maps for matching genes
    std::unordered_map<int, float> thisWeights, otherWeights;
    for (const auto& conn : m_connections) {
        thisWeights[conn.innovation] = conn.weight;
    }
    for (const auto& conn : other.m_connections) {
        otherWeights[conn.innovation] = conn.weight;
    }

    for (int i = 0; i <= maxInnov; i++) {
        bool inThis = thisInnovs.count(i) > 0;
        bool inOther = otherInnovs.count(i) > 0;

        if (inThis && inOther) {
            // Matching gene
            weightDiffSum += std::abs(thisWeights[i] - otherWeights[i]);
            matchingCount++;
        } else if (inThis || inOther) {
            if (i > minMax) {
                excess++;
            } else {
                disjoint++;
            }
        }
    }

    float avgWeightDiff = matchingCount > 0 ? weightDiffSum / matchingCount : 0.0f;

    // Normalize by larger genome size
    int N = std::max(m_connections.size(), other.m_connections.size());
    if (N < 20) N = 1;  // Don't normalize small genomes

    return (c1 * excess / N) + (c2 * disjoint / N) + (c3 * avgWeightDiff);
}

// ============================================================================
// Enhanced Compatibility Distance
// ============================================================================

float NEATGenome::compatibilityDistanceEnhanced(const NEATGenome& other,
                                                 float c1, float c2, float c3,
                                                 float c4_structure) const {
    // Base distance calculation
    float baseDistance = compatibilityDistance(other, c1, c2, c3);

    // Add structure-based distance
    float structureDistance = calculateBrainStructureDistance(other);

    return baseDistance + c4_structure * structureDistance;
}

// ============================================================================
// Brain Speciation - Structure and Functional Distance
// ============================================================================

float NEATGenome::calculateBrainStructureDistance(const NEATGenome& other) const {
    float distance = 0.0f;

    // Compare number of nodes
    int thisNodes = static_cast<int>(m_nodes.size());
    int otherNodes = static_cast<int>(other.m_nodes.size());
    float nodeDiff = std::abs(thisNodes - otherNodes) /
                     static_cast<float>(std::max(thisNodes, otherNodes) + 1);
    distance += nodeDiff;

    // Compare number of connections
    int thisConns = getEnabledConnectionCount();
    int otherConns = other.getEnabledConnectionCount();
    float connDiff = std::abs(thisConns - otherConns) /
                     static_cast<float>(std::max(thisConns, otherConns) + 1);
    distance += connDiff;

    // Compare layer depth
    int thisDepth = getMaxLayer();
    int otherDepth = other.getMaxLayer();
    float depthDiff = std::abs(thisDepth - otherDepth) /
                      static_cast<float>(std::max(thisDepth, otherDepth) + 1);
    distance += depthDiff;

    // Compare recurrent connection ratio
    float thisRecurrent = getRecurrentConnectionCount() /
                          static_cast<float>(std::max(1, thisConns));
    float otherRecurrent = other.getRecurrentConnectionCount() /
                           static_cast<float>(std::max(1, otherConns));
    distance += std::abs(thisRecurrent - otherRecurrent);

    // Compare modularity
    float thisModularity = calculateModularity();
    float otherModularity = other.calculateModularity();
    distance += std::abs(thisModularity - otherModularity);

    return distance / 5.0f;  // Normalize by number of factors
}

float NEATGenome::calculateFunctionalDistance(const NEATGenome& other,
                                               const std::vector<std::vector<float>>& testInputs) const {
    if (testInputs.empty()) return 0.0f;

    // Build networks
    auto thisNetwork = buildNetwork();
    auto otherNetwork = other.buildNetwork();

    if (!thisNetwork || !otherNetwork) return 1.0f;

    float totalDistance = 0.0f;

    for (const auto& input : testInputs) {
        // Get outputs from both networks
        auto thisOutput = thisNetwork->forward(input);
        auto otherOutput = otherNetwork->forward(input);

        // Calculate Euclidean distance between outputs
        float dist = 0.0f;
        size_t minSize = std::min(thisOutput.size(), otherOutput.size());
        for (size_t i = 0; i < minSize; i++) {
            float diff = thisOutput[i] - otherOutput[i];
            dist += diff * diff;
        }
        totalDistance += std::sqrt(dist) / static_cast<float>(minSize + 1);
    }

    return totalDistance / static_cast<float>(testInputs.size());
}

// ============================================================================
// Brain Complexity Metrics
// ============================================================================

float NEATGenome::calculateBrainComplexity() const {
    // Nodes contribute based on their type
    float nodeScore = 0.0f;
    for (const auto& node : m_nodes) {
        if (node.type == NodeType::HIDDEN) {
            nodeScore += 2.0f;  // Hidden nodes are more valuable
        } else if (node.type == NodeType::OUTPUT) {
            nodeScore += 1.0f;
        }
        if (node.isModulatory) {
            nodeScore += 1.5f;  // Modulatory nodes add complexity
        }
    }

    // Connections contribute weighted by their properties
    float connScore = 0.0f;
    for (const auto& conn : m_connections) {
        if (!conn.enabled) continue;
        float weight = 1.0f;
        if (conn.recurrent) weight += 0.5f;  // Recurrent adds complexity
        if (conn.plastic) weight += 0.3f;    // Plastic connections are more complex
        weight *= std::abs(conn.weight);     // Stronger connections contribute more
        connScore += weight;
    }

    // Modulatory connections add additional complexity
    float modScore = m_modulatoryConnections.size() * 1.5f;

    return nodeScore + connScore + modScore;
}

float NEATGenome::calculateModularity() const {
    // Calculate average local clustering coefficient
    // High clustering coefficient indicates modular structure

    std::vector<int> nodeIds;
    for (const auto& node : m_nodes) {
        if (node.type == NodeType::HIDDEN) {
            nodeIds.push_back(node.id);
        }
    }

    if (nodeIds.size() < 2) return 0.0f;

    float totalClusteringCoef = 0.0f;
    int validNodes = 0;

    for (int nodeId : nodeIds) {
        float coef = calculateLocalClusteringCoefficient(nodeId);
        if (coef >= 0.0f) {
            totalClusteringCoef += coef;
            validNodes++;
        }
    }

    return validNodes > 0 ? totalClusteringCoef / validNodes : 0.0f;
}

float NEATGenome::calculateLocalClusteringCoefficient(int nodeId) const {
    auto neighbors = getNeighbors(nodeId);
    int k = static_cast<int>(neighbors.size());

    if (k < 2) return 0.0f;  // Need at least 2 neighbors

    // Count edges between neighbors
    int edges = 0;
    for (int n1 : neighbors) {
        for (int n2 : neighbors) {
            if (n1 >= n2) continue;  // Avoid double counting
            if (connectionExists(n1, n2) || connectionExists(n2, n1)) {
                edges++;
            }
        }
    }

    // Maximum possible edges = k(k-1)/2
    int maxEdges = k * (k - 1) / 2;
    return maxEdges > 0 ? static_cast<float>(edges) / maxEdges : 0.0f;
}

float NEATGenome::calculateHierarchy() const {
    // Calculate hierarchy based on layer distribution and depth
    int maxLayer = getMaxLayer();
    if (maxLayer <= 1) return 0.0f;

    // Count nodes per layer
    std::unordered_map<int, int> nodesPerLayer;
    for (const auto& node : m_nodes) {
        if (node.type == NodeType::HIDDEN) {
            nodesPerLayer[node.layer]++;
        }
    }

    // Calculate layer utilization
    int usedLayers = static_cast<int>(nodesPerLayer.size());

    // Hierarchy is based on depth and even distribution
    float depthScore = static_cast<float>(maxLayer) / 10.0f;  // Normalize
    float utilizationScore = static_cast<float>(usedLayers) / (maxLayer + 1);

    return (depthScore + utilizationScore) / 2.0f;
}

float NEATGenome::calculateIntegration() const {
    // Connection density - how connected is the network?
    int nodeCount = static_cast<int>(m_nodes.size());
    int enabledConns = getEnabledConnectionCount();

    // Maximum possible connections (excluding input-to-input and output-to-output)
    int maxPossible = nodeCount * nodeCount;  // Upper bound

    if (maxPossible == 0) return 0.0f;

    return static_cast<float>(enabledConns) / maxPossible;
}

BrainMetrics NEATGenome::calculateAllMetrics() const {
    BrainMetrics metrics;

    metrics.complexity = calculateBrainComplexity();
    metrics.modularity = calculateModularity();
    metrics.hierarchy = calculateHierarchy();
    metrics.integration = calculateIntegration();
    metrics.efficiency = calculateBrainEfficiency();
    metrics.cost = calculateBrainCost();
    metrics.nodeCount = static_cast<int>(m_nodes.size());
    metrics.connectionCount = getEnabledConnectionCount();
    metrics.regionCount = static_cast<int>(m_regions.size());
    metrics.maxDepth = getMaxLayer();

    return metrics;
}

// ============================================================================
// Brain Regions/Modules
// ============================================================================

std::vector<BrainRegion> NEATGenome::identifyRegions() const {
    std::vector<BrainRegion> regions;

    // Get all hidden nodes
    std::vector<int> hiddenNodes;
    for (const auto& node : m_nodes) {
        if (node.type == NodeType::HIDDEN) {
            hiddenNodes.push_back(node.id);
        }
    }

    if (hiddenNodes.empty()) return regions;

    // Simple clustering based on connectivity
    std::unordered_set<int> unassigned(hiddenNodes.begin(), hiddenNodes.end());
    int regionId = 0;

    while (!unassigned.empty()) {
        BrainRegion region(regionId++);
        region.generationFormed = m_generation;

        // Start with first unassigned node
        std::queue<int> toProcess;
        int startNode = *unassigned.begin();
        toProcess.push(startNode);
        unassigned.erase(startNode);
        region.nodeIds.push_back(startNode);

        // BFS to find connected nodes with high local connectivity
        while (!toProcess.empty()) {
            int current = toProcess.front();
            toProcess.pop();

            auto neighbors = getNeighbors(current);
            for (int neighbor : neighbors) {
                if (unassigned.count(neighbor) > 0) {
                    // Check if neighbor is strongly connected to existing region
                    int connectionsToRegion = 0;
                    auto neighborNeighbors = getNeighbors(neighbor);
                    for (int nn : neighborNeighbors) {
                        for (int rn : region.nodeIds) {
                            if (nn == rn) connectionsToRegion++;
                        }
                    }

                    // Add to region if well-connected (at least connected to 1 other node)
                    if (connectionsToRegion >= 1 || region.nodeIds.size() < 3) {
                        unassigned.erase(neighbor);
                        region.nodeIds.push_back(neighbor);
                        toProcess.push(neighbor);
                    }
                }
            }

            // Limit region size to keep regions meaningful
            if (region.nodeIds.size() >= 10) break;
        }

        // Classify region connections
        std::unordered_set<int> regionNodeSet(region.nodeIds.begin(), region.nodeIds.end());
        for (const auto& conn : m_connections) {
            if (!conn.enabled) continue;

            bool fromRegion = regionNodeSet.count(conn.fromNode) > 0;
            bool toRegion = regionNodeSet.count(conn.toNode) > 0;

            if (fromRegion && toRegion) {
                region.internalConnections.push_back(conn.innovation);
            } else if (!fromRegion && toRegion) {
                region.inputConnections.push_back(conn.innovation);
            } else if (fromRegion && !toRegion) {
                region.outputConnections.push_back(conn.innovation);
            }
        }

        // Calculate region modularity
        float internal = static_cast<float>(region.internalConnections.size());
        float external = static_cast<float>(region.inputConnections.size() +
                                            region.outputConnections.size());
        region.modularity = (internal > 0 || external > 0) ?
                            internal / (internal + external) : 0.0f;

        // Determine region function
        region.function = getRegionFunction(region.id);

        regions.push_back(region);
    }

    return regions;
}

std::string NEATGenome::getRegionFunction(int regionId) const {
    // Find the region
    const BrainRegion* region = nullptr;
    for (const auto& r : m_regions) {
        if (r.id == regionId) {
            region = &r;
            break;
        }
    }

    if (!region || region->nodeIds.empty()) return "unknown";

    // Analyze connectivity to determine function
    std::unordered_set<int> inputNodeIds;
    std::unordered_set<int> outputNodeIds;
    for (const auto& node : m_nodes) {
        if (node.type == NodeType::INPUT || node.type == NodeType::BIAS) {
            inputNodeIds.insert(node.id);
        } else if (node.type == NodeType::OUTPUT) {
            outputNodeIds.insert(node.id);
        }
    }

    int inputConnections = 0;
    int outputConnections = 0;

    for (const auto& conn : m_connections) {
        if (!conn.enabled) continue;

        for (int nodeId : region->nodeIds) {
            if (conn.toNode == nodeId && inputNodeIds.count(conn.fromNode) > 0) {
                inputConnections++;
            }
            if (conn.fromNode == nodeId && outputNodeIds.count(conn.toNode) > 0) {
                outputConnections++;
            }
        }
    }

    // Classify based on connections
    if (inputConnections > outputConnections * 2) {
        return "sensory";
    } else if (outputConnections > inputConnections * 2) {
        return "motor";
    } else if (region->getConnectionCount() > 10 && region->modularity > 0.7f) {
        return "memory";
    } else {
        return "integration";
    }
}

void NEATGenome::trackRegionEvolution(std::vector<BrainRegion>& regions) const {
    // Update fitness history for each region
    for (auto& region : regions) {
        region.fitnessHistory.push_back(m_fitness);

        // Calculate region plasticity
        float totalPlasticity = 0.0f;
        int plasticConns = 0;
        for (const auto& conn : m_connections) {
            if (!conn.enabled) continue;

            std::unordered_set<int> regionNodeSet(region.nodeIds.begin(), region.nodeIds.end());
            if (regionNodeSet.count(conn.fromNode) > 0 || regionNodeSet.count(conn.toNode) > 0) {
                if (conn.plastic) {
                    totalPlasticity += conn.plasticityRate;
                    plasticConns++;
                }
            }
        }
        region.plasticity = plasticConns > 0 ? totalPlasticity / plasticConns : 0.0f;
    }
}

void NEATGenome::updateRegions() {
    m_regions = identifyRegions();

    // Update node region assignments
    for (auto& region : m_regions) {
        for (int nodeId : region.nodeIds) {
            NodeGene* node = getNode(nodeId);
            if (node) {
                node->regionId = region.id;
            }
        }
    }
}

void NEATGenome::assignNodeToRegion(int nodeId, int regionId) {
    NodeGene* node = getNode(nodeId);
    if (node) {
        node->regionId = regionId;
    }

    // Update region's node list
    for (auto& region : m_regions) {
        if (region.id == regionId) {
            if (std::find(region.nodeIds.begin(), region.nodeIds.end(), nodeId) == region.nodeIds.end()) {
                region.nodeIds.push_back(nodeId);
            }
            break;
        }
    }
}

// ============================================================================
// Neuromodulation
// ============================================================================

void NEATGenome::addModulatoryNode(std::mt19937& rng, int targetRegionId) {
    auto& tracker = InnovationTracker::instance();
    std::uniform_real_distribution<float> weightDist(-1.0f, 1.0f);

    // Create new modulatory node
    int nodeId = tracker.getNextNodeId();
    int layer = 1;

    // Place in middle layer
    for (const auto& node : m_nodes) {
        if (node.type == NodeType::HIDDEN) {
            layer = std::max(layer, node.layer);
        }
    }
    layer = (layer + 1) / 2;  // Middle layer

    m_nodes.emplace_back(nodeId, NodeType::HIDDEN, ActivationType::SIGMOID, 0.0f, layer);
    NodeGene& newNode = m_nodes.back();
    newNode.isModulatory = true;
    newNode.canModulate = true;
    newNode.regionId = targetRegionId;

    // Connect from random input/hidden node
    auto validSources = getValidSourceNodes();
    if (!validSources.empty()) {
        std::uniform_int_distribution<size_t> srcDist(0, validSources.size() - 1);
        int srcId = validSources[srcDist(rng)];
        int innovation = tracker.getConnectionInnovation(srcId, nodeId);
        m_connections.emplace_back(innovation, srcId, nodeId, weightDist(rng), true, false);
    }

    // Create modulatory connections to target region or random connections
    std::vector<int> targetConnections;
    if (targetRegionId >= 0) {
        // Find connections in target region
        for (const auto& region : m_regions) {
            if (region.id == targetRegionId) {
                targetConnections.insert(targetConnections.end(),
                                        region.internalConnections.begin(),
                                        region.internalConnections.end());
                break;
            }
        }
    }

    if (targetConnections.empty()) {
        // Use random enabled connections
        for (const auto& conn : m_connections) {
            if (conn.enabled) {
                targetConnections.push_back(conn.innovation);
            }
        }
    }

    // Add modulatory connections to a few target connections
    std::uniform_int_distribution<size_t> targetDist(0, std::max(size_t(1), targetConnections.size()) - 1);
    std::uniform_int_distribution<int> typeDist(0, 3);

    int numModulations = std::min(3, static_cast<int>(targetConnections.size()));
    for (int i = 0; i < numModulations; i++) {
        if (targetConnections.empty()) break;
        int targetConn = targetConnections[targetDist(rng)];
        ModulationType type = static_cast<ModulationType>(typeDist(rng));
        addModulatoryConnection(nodeId, targetConn, weightDist(rng), type);
    }
}

float NEATGenome::calculatePlasticity(int regionId) const {
    const BrainRegion* region = nullptr;
    for (const auto& r : m_regions) {
        if (r.id == regionId) {
            region = &r;
            break;
        }
    }

    if (!region) return 0.0f;

    std::unordered_set<int> regionNodeSet(region->nodeIds.begin(), region->nodeIds.end());

    float totalPlasticity = 0.0f;
    int connectionCount = 0;

    for (const auto& conn : m_connections) {
        if (!conn.enabled) continue;

        bool involvesRegion = regionNodeSet.count(conn.fromNode) > 0 ||
                              regionNodeSet.count(conn.toNode) > 0;

        if (involvesRegion && conn.plastic) {
            totalPlasticity += conn.plasticityRate;
            connectionCount++;
        }
    }

    // Check for modulatory influence on region
    float modulatoryInfluence = 0.0f;
    for (const auto& modConn : m_modulatoryConnections) {
        // Check if target connection involves region
        for (const auto& conn : m_connections) {
            if (conn.innovation == modConn.targetConnectionInnovation) {
                if (regionNodeSet.count(conn.fromNode) > 0 ||
                    regionNodeSet.count(conn.toNode) > 0) {
                    modulatoryInfluence += std::abs(modConn.modulationStrength);
                }
                break;
            }
        }
    }

    float basePlasticity = connectionCount > 0 ? totalPlasticity / connectionCount : 0.0f;
    return basePlasticity * (1.0f + modulatoryInfluence * 0.5f);
}

void NEATGenome::addModulatoryConnection(int modulatorNode, int targetConnectionInnovation,
                                          float strength, ModulationType type) {
    // Verify modulator node exists and is modulatory
    NodeGene* node = getNode(modulatorNode);
    if (!node || !node->canModulate) return;

    // Verify target connection exists
    bool targetExists = false;
    for (const auto& conn : m_connections) {
        if (conn.innovation == targetConnectionInnovation) {
            targetExists = true;
            break;
        }
    }
    if (!targetExists) return;

    // Get innovation number for modulatory connection
    auto& tracker = InnovationTracker::instance();
    int innovation = tracker.getConnectionInnovation(modulatorNode, -targetConnectionInnovation);

    m_modulatoryConnections.emplace_back(innovation, modulatorNode, targetConnectionInnovation,
                                          strength, type);
}

// ============================================================================
// Brain Fitness / Efficiency
// ============================================================================

float NEATGenome::calculateBrainEfficiency() const {
    // Efficiency = fitness / cost
    float cost = calculateBrainCost();
    if (cost <= 0.0f) return 0.0f;

    return m_fitness / cost;
}

float NEATGenome::calculateBrainCost() const {
    // Metabolic cost model
    float nodeCost = 0.0f;
    for (const auto& node : m_nodes) {
        float baseCost = 1.0f;

        // Different node types have different costs
        if (node.type == NodeType::HIDDEN) {
            baseCost = 2.0f;

            // Complex activations cost more
            if (node.activation == ActivationType::GAUSSIAN ||
                node.activation == ActivationType::SINE) {
                baseCost *= 1.5f;
            }

            // Modulatory nodes are expensive
            if (node.isModulatory) {
                baseCost *= 2.0f;
            }
        }

        // Plasticity adds cost
        baseCost *= (1.0f + node.plasticityCoef * 0.2f);

        nodeCost += baseCost;
    }

    // Connection cost
    float connectionCost = 0.0f;
    for (const auto& conn : m_connections) {
        if (!conn.enabled) continue;

        float baseCost = 0.5f;

        // Stronger connections cost more
        baseCost *= (1.0f + std::abs(conn.weight) * 0.1f);

        // Recurrent connections cost more
        if (conn.recurrent) baseCost *= 1.5f;

        // Plastic connections cost more
        if (conn.plastic) baseCost *= (1.0f + conn.plasticityRate * 0.3f);

        connectionCost += baseCost;
    }

    // Modulatory connections cost
    float modCost = m_modulatoryConnections.size() * 1.0f;

    return nodeCost + connectionCost + modCost;
}

void NEATGenome::optimizeForEfficiency(std::mt19937& rng, float pruneThreshold) {
    // Identify and remove inefficient components

    // 1. Disable weak connections
    for (auto& conn : m_connections) {
        if (conn.enabled && std::abs(conn.weight) < pruneThreshold) {
            conn.enabled = false;
        }
    }

    // 2. Remove dead ends
    mutatePruneDeadEnds();

    // 3. Remove unused modulatory connections
    std::unordered_set<int> activeConnectionInnovs;
    for (const auto& conn : m_connections) {
        if (conn.enabled) {
            activeConnectionInnovs.insert(conn.innovation);
        }
    }

    m_modulatoryConnections.erase(
        std::remove_if(m_modulatoryConnections.begin(), m_modulatoryConnections.end(),
                       [&activeConnectionInnovs](const ModulatoryConnection& mc) {
                           return activeConnectionInnovs.count(mc.targetConnectionInnovation) == 0;
                       }),
        m_modulatoryConnections.end());

    // 4. Remove empty regions
    m_regions.erase(
        std::remove_if(m_regions.begin(), m_regions.end(),
                       [](const BrainRegion& r) { return r.nodeIds.empty(); }),
        m_regions.end());

    // 5. Potentially simplify activation functions
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);
    for (auto& node : m_nodes) {
        if (node.type == NodeType::HIDDEN && prob(rng) < 0.1f) {
            // Expensive activations might be simplified
            if (node.activation == ActivationType::GAUSSIAN ||
                node.activation == ActivationType::SINE) {
                node.activation = ActivationType::TANH;
            }
        }
    }
}

// ============================================================================
// Cycle Detection
// ============================================================================

bool NEATGenome::wouldCreateCycle(int from, int to) const {
    // BFS from 'to' to see if we can reach 'from'
    std::set<int> visited;
    std::queue<int> queue;
    queue.push(to);

    while (!queue.empty()) {
        int current = queue.front();
        queue.pop();

        if (current == from) {
            return true;
        }

        if (visited.count(current)) continue;
        visited.insert(current);

        for (const auto& conn : m_connections) {
            if (conn.fromNode == current && conn.enabled && !conn.recurrent) {
                queue.push(conn.toNode);
            }
        }
    }

    return false;
}

// ============================================================================
// NEAT Population
// ============================================================================

NEATPopulation::NEATPopulation(int populationSize, int numInputs, int numOutputs)
    : m_populationSize(populationSize)
    , m_numInputs(numInputs)
    , m_numOutputs(numOutputs)
    , m_rng(std::random_device{}())
{
    // Reset innovation tracker for new population
    InnovationTracker::instance().reset();

    // Create initial population of minimal networks
    m_genomes.reserve(populationSize);
    for (int i = 0; i < populationSize; i++) {
        NEATGenome genome;
        genome.createMinimal(numInputs, numOutputs, m_rng);
        m_genomes.push_back(std::move(genome));
    }

    m_bestGenome = m_genomes[0];
}

void NEATPopulation::evaluateFitness(std::function<float(const NEATGenome&)> fitnessFunc) {
    float bestFitness = -std::numeric_limits<float>::max();

    for (auto& genome : m_genomes) {
        float fitness = fitnessFunc(genome);
        genome.setFitness(fitness);

        if (fitness > bestFitness) {
            bestFitness = fitness;
            m_bestGenome = genome;
        }
    }
}

void NEATPopulation::evolve() {
    m_generation++;

    // Speciate
    speciate();

    // Calculate adjusted fitness
    for (auto& species : m_species) {
        species.calculateAdjustedFitness();
        species.updateStagnation();
    }

    // Remove stagnant species (except the best one)
    float bestSpeciesFitness = 0.0f;
    int bestSpeciesIdx = 0;
    for (size_t i = 0; i < m_species.size(); i++) {
        if (m_species[i].bestFitness > bestSpeciesFitness) {
            bestSpeciesFitness = m_species[i].bestFitness;
            bestSpeciesIdx = static_cast<int>(i);
        }
    }

    for (size_t i = 0; i < m_species.size(); ) {
        if (m_species[i].stagnantGenerations > stagnationLimit &&
            static_cast<int>(i) != bestSpeciesIdx) {
            m_species.erase(m_species.begin() + i);
            if (static_cast<int>(i) < bestSpeciesIdx) bestSpeciesIdx--;
        } else {
            i++;
        }
    }

    // Reproduce
    reproduceSpecies();

    // Clear species members for next generation
    for (auto& species : m_species) {
        species.clear();
    }
}

void NEATPopulation::speciate() {
    // Clear existing species memberships
    for (auto& species : m_species) {
        species.clear();
    }

    // Assign each genome to a species
    for (auto& genome : m_genomes) {
        bool foundSpecies = false;

        for (auto& species : m_species) {
            float distance = genome.compatibilityDistance(
                species.representative, c1_excess, c2_disjoint, c3_weight);

            if (distance < compatibilityThreshold) {
                species.members.push_back(&genome);
                genome.setSpeciesId(species.id);
                foundSpecies = true;
                break;
            }
        }

        if (!foundSpecies) {
            // Create new species
            Species newSpecies(m_nextSpeciesId++, genome);
            newSpecies.members.push_back(&genome);
            genome.setSpeciesId(newSpecies.id);
            m_species.push_back(newSpecies);
        }
    }

    // Remove empty species
    m_species.erase(
        std::remove_if(m_species.begin(), m_species.end(),
                       [](const Species& s) { return s.members.empty(); }),
        m_species.end());

    // Update representatives
    for (auto& species : m_species) {
        species.updateRepresentative(m_rng);
    }
}

void NEATPopulation::reproduceSpecies() {
    // Calculate total adjusted fitness
    float totalAdjustedFitness = 0.0f;
    for (const auto& species : m_species) {
        totalAdjustedFitness += species.totalAdjustedFitness;
    }

    if (totalAdjustedFitness <= 0.0f) {
        totalAdjustedFitness = 1.0f;
    }

    // Calculate offspring per species
    std::vector<int> offspringCounts;
    int totalOffspring = 0;

    for (const auto& species : m_species) {
        float proportion = species.totalAdjustedFitness / totalAdjustedFitness;
        int count = static_cast<int>(std::round(proportion * m_populationSize));
        count = std::max(1, count);  // Ensure at least 1 offspring
        offspringCounts.push_back(count);
        totalOffspring += count;
    }

    // Adjust to match population size
    while (totalOffspring > m_populationSize) {
        // Remove from random species
        std::uniform_int_distribution<size_t> dist(0, offspringCounts.size() - 1);
        size_t idx = dist(m_rng);
        if (offspringCounts[idx] > 1) {
            offspringCounts[idx]--;
            totalOffspring--;
        }
    }
    while (totalOffspring < m_populationSize) {
        // Add to random species
        std::uniform_int_distribution<size_t> dist(0, offspringCounts.size() - 1);
        offspringCounts[dist(m_rng)]++;
        totalOffspring++;
    }

    // Generate new population
    std::vector<NEATGenome> newGenomes;
    newGenomes.reserve(m_populationSize);

    for (size_t speciesIdx = 0; speciesIdx < m_species.size(); speciesIdx++) {
        Species& species = m_species[speciesIdx];
        int count = offspringCounts[speciesIdx];

        if (species.members.empty()) continue;

        // Sort by fitness
        std::sort(species.members.begin(), species.members.end(),
                  [](const NEATGenome* a, const NEATGenome* b) {
                      return a->getFitness() > b->getFitness();
                  });

        // Keep champion unchanged
        if (count > 0 && species.members.size() >= 5) {
            newGenomes.push_back(*species.members[0]);
            count--;
        }

        // Produce offspring
        for (int i = 0; i < count; i++) {
            NEATGenome offspring = reproduce(species);
            newGenomes.push_back(std::move(offspring));
        }
    }

    m_genomes = std::move(newGenomes);
}

NEATGenome NEATPopulation::reproduce(Species& species) {
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);

    // Select parents from top performers
    int survivalCount = std::max(1, static_cast<int>(species.members.size() * survivalThreshold));
    std::uniform_int_distribution<int> parentDist(0, survivalCount - 1);

    NEATGenome* parent1 = species.members[parentDist(m_rng)];

    // 75% chance of crossover with another parent
    NEATGenome offspring;
    if (prob(m_rng) < 0.75f && species.members.size() > 1) {
        NEATGenome* parent2 = species.members[parentDist(m_rng)];
        while (parent2 == parent1 && survivalCount > 1) {
            parent2 = species.members[parentDist(m_rng)];
        }

        // Fitter parent is first argument
        if (parent1->getFitness() >= parent2->getFitness()) {
            offspring = NEATGenome::crossover(*parent1, *parent2, m_rng);
        } else {
            offspring = NEATGenome::crossover(*parent2, *parent1, m_rng);
        }
    } else {
        // Asexual reproduction
        offspring = *parent1;
    }

    // Mutate
    offspring.mutate(m_rng, mutationParams);

    return offspring;
}

} // namespace ai
