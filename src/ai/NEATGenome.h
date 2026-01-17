#pragma once

#include "NeuralNetwork.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <memory>
#include <cmath>
#include <string>
#include <functional>
#include <deque>

namespace ai {

// Forward declarations
class NEATGenome;

// ============================================================================
// Innovation Record - Enhanced tracking for innovations
// ============================================================================

struct InnovationRecord {
    int innovationNumber;         // The innovation ID
    int generationCreated;        // When this innovation first appeared
    int spreadCount;              // How many genomes currently have this innovation
    float fitnessContribution;    // Average fitness contribution
    bool survivalStatus;          // Is this innovation still present in population?
    int fromNode;                 // Source node of connection
    int toNode;                   // Target node of connection

    // Historical tracking
    std::vector<float> fitnessHistory;  // Fitness over generations
    int peakGeneration;           // Generation with highest fitness
    float peakFitness;            // Highest fitness achieved

    InnovationRecord(int innov = 0, int gen = 0, int from = -1, int to = -1)
        : innovationNumber(innov), generationCreated(gen), spreadCount(1),
          fitnessContribution(0.0f), survivalStatus(true),
          fromNode(from), toNode(to), peakGeneration(gen), peakFitness(0.0f) {}

    void updateFitness(float fitness) {
        fitnessHistory.push_back(fitness);
        if (fitness > peakFitness) {
            peakFitness = fitness;
            peakGeneration = generationCreated + static_cast<int>(fitnessHistory.size()) - 1;
        }
        // Running average
        fitnessContribution = (fitnessContribution * (fitnessHistory.size() - 1) + fitness)
                              / fitnessHistory.size();
    }
};

// ============================================================================
// Brain Region - Cluster of nodes with functional identity
// ============================================================================

struct BrainRegion {
    int id;                           // Unique region identifier
    std::vector<int> nodeIds;         // Nodes belonging to this region
    std::vector<int> inputConnections;   // Innovation IDs of incoming connections
    std::vector<int> outputConnections;  // Innovation IDs of outgoing connections
    std::vector<int> internalConnections; // Innovation IDs within region

    // Functional characterization
    std::string function;             // "sensory", "motor", "integration", "memory"
    float modularity;                 // How self-contained the region is (0-1)
    float activity;                   // Average activation level
    float plasticity;                 // Average plasticity of region

    // Evolution tracking
    int generationFormed;             // When this region first appeared
    int parentRegionId;               // If split from another region
    std::vector<float> fitnessHistory; // How fitness changed as region evolved

    BrainRegion(int id_ = 0)
        : id(id_), function("unknown"), modularity(0.0f), activity(0.0f),
          plasticity(1.0f), generationFormed(0), parentRegionId(-1) {}

    int getNodeCount() const { return static_cast<int>(nodeIds.size()); }
    int getConnectionCount() const {
        return static_cast<int>(inputConnections.size() + outputConnections.size() +
                                internalConnections.size());
    }
};

// ============================================================================
// Modulatory Connection - Connections that modulate other connections
// ============================================================================

struct ModulatoryConnection {
    enum class ModulationType {
        GAIN,         // Multiplies the target weight
        GATE,         // Binary on/off based on modulator activation
        ADDITIVE,     // Adds to target weight
        PLASTICITY    // Modulates plasticity rate of target
    };

    int innovation;               // Innovation number
    int modulatorNodeId;          // The node doing the modulation
    int targetConnectionInnovation; // The connection being modulated
    float modulationStrength;     // How strongly this modulates (can be negative)
    ModulationType type;          // Type of modulation effect

    ModulatoryConnection(int innov, int modNode, int targetConn,
                         float strength = 1.0f,
                         ModulationType t = ModulationType::GAIN)
        : innovation(innov), modulatorNodeId(modNode),
          targetConnectionInnovation(targetConn),
          modulationStrength(strength), type(t) {}
};

// Bring enum out for easier access
using ModulationType = ModulatoryConnection::ModulationType;

// ============================================================================
// Brain Complexity Metrics Result
// ============================================================================

struct BrainMetrics {
    float complexity;         // Overall complexity score
    float modularity;         // Clustering coefficient
    float hierarchy;          // Network depth/layering
    float integration;        // Connection density
    float efficiency;         // Performance per neuron
    float cost;               // Metabolic cost estimate
    int nodeCount;
    int connectionCount;
    int regionCount;
    int maxDepth;
};

// ============================================================================
// NEAT Gene Structures
// ============================================================================

struct NodeGene {
    int id;
    NodeType type;
    ActivationType activation;
    float bias;
    int layer;

    // Evolved plasticity parameters
    float plasticityCoef;     // How plastic connections from this node are
    bool canModulate;         // Can this node act as a neuromodulator?

    // Region membership
    int regionId;             // Which brain region this node belongs to (-1 = none)

    // Neuromodulation
    bool isModulatory;        // Does this node produce modulatory signals?
    float modulatoryOutput;   // Current modulatory signal strength

    NodeGene(int id_, NodeType type_, ActivationType act_ = ActivationType::TANH,
             float bias_ = 0.0f, int layer_ = 0)
        : id(id_), type(type_), activation(act_), bias(bias_), layer(layer_),
          plasticityCoef(1.0f), canModulate(false), regionId(-1),
          isModulatory(false), modulatoryOutput(0.0f) {}
};

struct ConnectionGene {
    int innovation;           // Historical marker for crossover alignment
    int fromNode;
    int toNode;
    float weight;
    bool enabled;
    bool recurrent;           // Is this a recurrent connection?

    // Plasticity parameters (evolved)
    bool plastic;             // Can this connection learn during lifetime?
    float plasticityRate;     // Per-connection learning rate multiplier

    ConnectionGene(int innov, int from, int to, float w = 0.0f,
                   bool enabled_ = true, bool recurrent_ = false)
        : innovation(innov), fromNode(from), toNode(to), weight(w),
          enabled(enabled_), recurrent(recurrent_),
          plastic(true), plasticityRate(1.0f) {}
};

// ============================================================================
// Innovation Tracker (Global for population) - Enhanced with history tracking
// ============================================================================

class InnovationTracker {
public:
    static InnovationTracker& instance() {
        static InnovationTracker tracker;
        return tracker;
    }

    // Get or create innovation number for a connection
    int getConnectionInnovation(int from, int to) {
        auto key = std::make_pair(from, to);
        auto it = m_connectionInnovations.find(key);
        if (it != m_connectionInnovations.end()) {
            return it->second;
        }
        int innov = m_nextConnectionInnovation++;
        m_connectionInnovations[key] = innov;

        // Create innovation record
        InnovationRecord record(innov, m_currentGeneration, from, to);
        m_innovationHistory[innov] = record;

        return innov;
    }

    // Get or create innovation number for a new node
    int getNodeInnovation(int splitConnectionId) {
        auto it = m_nodeInnovations.find(splitConnectionId);
        if (it != m_nodeInnovations.end()) {
            return it->second;
        }
        int innov = m_nextNodeId++;
        m_nodeInnovations[splitConnectionId] = innov;
        return innov;
    }

    int getNextNodeId() { return m_nextNodeId++; }

    void reset() {
        m_connectionInnovations.clear();
        m_nodeInnovations.clear();
        m_innovationHistory.clear();
        m_nextConnectionInnovation = 0;
        m_nextNodeId = 0;
        m_currentGeneration = 0;
    }

    // For setting up initial structure
    void setNextNodeId(int id) { m_nextNodeId = std::max(m_nextNodeId, id); }
    void setNextInnovation(int id) { m_nextConnectionInnovation = std::max(m_nextConnectionInnovation, id); }

    // ========================================================================
    // Enhanced Innovation Tracking
    // ========================================================================

    void setCurrentGeneration(int gen) { m_currentGeneration = gen; }
    int getCurrentGeneration() const { return m_currentGeneration; }

    // Track how widely an innovation has spread
    void trackInnovationSpread(int innovationId, int count) {
        auto it = m_innovationHistory.find(innovationId);
        if (it != m_innovationHistory.end()) {
            it->second.spreadCount = count;
            it->second.survivalStatus = (count > 0);
        }
    }

    // Update fitness contribution for an innovation
    void updateInnovationFitness(int innovationId, float fitness) {
        auto it = m_innovationHistory.find(innovationId);
        if (it != m_innovationHistory.end()) {
            it->second.updateFitness(fitness);
        }
    }

    // Get the most successful innovations (by fitness contribution)
    std::vector<InnovationRecord> getMostSuccessfulInnovations(int topN = 10) const {
        std::vector<InnovationRecord> records;
        records.reserve(m_innovationHistory.size());

        for (const auto& pair : m_innovationHistory) {
            if (pair.second.survivalStatus) {
                records.push_back(pair.second);
            }
        }

        std::sort(records.begin(), records.end(),
                  [](const InnovationRecord& a, const InnovationRecord& b) {
                      return a.fitnessContribution > b.fitnessContribution;
                  });

        if (static_cast<int>(records.size()) > topN) {
            records.resize(topN);
        }

        return records;
    }

    // Get complete innovation history
    const std::unordered_map<int, InnovationRecord>& getInnovationHistory() const {
        return m_innovationHistory;
    }

    // Get specific innovation record
    const InnovationRecord* getInnovationRecord(int innovationId) const {
        auto it = m_innovationHistory.find(innovationId);
        if (it != m_innovationHistory.end()) {
            return &it->second;
        }
        return nullptr;
    }

    // Get innovations created in a specific generation range
    std::vector<InnovationRecord> getInnovationsFromGeneration(int startGen, int endGen = -1) const {
        std::vector<InnovationRecord> records;
        if (endGen < 0) endGen = m_currentGeneration;

        for (const auto& pair : m_innovationHistory) {
            if (pair.second.generationCreated >= startGen &&
                pair.second.generationCreated <= endGen) {
                records.push_back(pair.second);
            }
        }

        std::sort(records.begin(), records.end(),
                  [](const InnovationRecord& a, const InnovationRecord& b) {
                      return a.generationCreated < b.generationCreated;
                  });

        return records;
    }

    // Get innovation spread statistics
    float getAverageInnovationSpread() const {
        if (m_innovationHistory.empty()) return 0.0f;
        float total = 0.0f;
        for (const auto& pair : m_innovationHistory) {
            total += pair.second.spreadCount;
        }
        return total / m_innovationHistory.size();
    }

    int getActiveInnovationCount() const {
        int count = 0;
        for (const auto& pair : m_innovationHistory) {
            if (pair.second.survivalStatus) count++;
        }
        return count;
    }

private:
    InnovationTracker() = default;

    struct PairHash {
        size_t operator()(const std::pair<int, int>& p) const {
            return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 16);
        }
    };

    std::unordered_map<std::pair<int, int>, int, PairHash> m_connectionInnovations;
    std::unordered_map<int, int> m_nodeInnovations;  // split connection ID -> node ID
    std::unordered_map<int, InnovationRecord> m_innovationHistory;  // Full history
    int m_nextConnectionInnovation = 0;
    int m_nextNodeId = 0;
    int m_currentGeneration = 0;
};

// ============================================================================
// NEAT Genome
// ============================================================================

class NEATGenome {
public:
    NEATGenome() = default;

    // Create minimal network (inputs directly connected to outputs)
    void createMinimal(int numInputs, int numOutputs, std::mt19937& rng);

    // Create from existing genome (copy)
    NEATGenome(const NEATGenome& other) = default;
    NEATGenome& operator=(const NEATGenome& other) = default;

    // ========================================================================
    // Mutation Operations
    // ========================================================================

    void mutate(std::mt19937& rng, const struct MutationParams& params);

    // Individual mutation types
    void mutateWeights(std::mt19937& rng, float perturbChance, float perturbStrength,
                       float replaceChance);
    void mutateAddConnection(std::mt19937& rng, bool allowRecurrent = true);
    void mutateAddNode(std::mt19937& rng);
    void mutateToggleEnable(std::mt19937& rng);
    void mutateActivation(std::mt19937& rng);
    void mutateBias(std::mt19937& rng, float strength);
    void mutatePlasticity(std::mt19937& rng);

    // ========================================================================
    // Advanced Topology Evolution Mutations
    // ========================================================================

    // Add a cluster of connected nodes (module)
    void mutateAddModule(std::mt19937& rng, int moduleSize = 3);

    // Add a recurrent connection specifically
    void mutateAddRecurrence(std::mt19937& rng);

    // Split an existing module into two smaller modules
    void mutateSplitModule(std::mt19937& rng);

    // Remove disconnected or dead-end nodes that don't contribute
    void mutatePruneDeadEnds();

    // Transfer (copy) a module from another genome
    void mutateTransferModule(const NEATGenome& source, std::mt19937& rng);

    // ========================================================================
    // Crossover
    // ========================================================================

    static NEATGenome crossover(const NEATGenome& fitter, const NEATGenome& other,
                                std::mt19937& rng);

    // ========================================================================
    // Species Distance - Enhanced
    // ========================================================================

    float compatibilityDistance(const NEATGenome& other,
                                float c1_excess, float c2_disjoint, float c3_weight) const;

    // Enhanced with structure weighting
    float compatibilityDistanceEnhanced(const NEATGenome& other,
                                        float c1_excess, float c2_disjoint, float c3_weight,
                                        float c4_structure) const;

    // ========================================================================
    // Brain Speciation - Structure and Functional Distance
    // ========================================================================

    // Topology comparison - how different are the network structures?
    float calculateBrainStructureDistance(const NEATGenome& other) const;

    // Behavioral similarity - how similar are the network responses?
    float calculateFunctionalDistance(const NEATGenome& other,
                                      const std::vector<std::vector<float>>& testInputs) const;

    // ========================================================================
    // Brain Complexity Metrics
    // ========================================================================

    // Overall complexity: nodes + weighted connections
    float calculateBrainComplexity() const;

    // Clustering coefficient - how modular is the network?
    float calculateModularity() const;

    // Network depth - how hierarchical is the processing?
    float calculateHierarchy() const;

    // Connection density - how integrated is the network?
    float calculateIntegration() const;

    // Get all brain metrics at once
    BrainMetrics calculateAllMetrics() const;

    // ========================================================================
    // Brain Regions/Modules
    // ========================================================================

    // Identify distinct regions via cluster analysis
    std::vector<BrainRegion> identifyRegions() const;

    // Get what function a region serves (sensory/motor/integration/memory)
    std::string getRegionFunction(int regionId) const;

    // Track how regions have evolved over generations
    void trackRegionEvolution(std::vector<BrainRegion>& regions) const;

    // Get regions accessor
    const std::vector<BrainRegion>& getRegions() const { return m_regions; }
    std::vector<BrainRegion>& getRegions() { return m_regions; }

    // Update internal region tracking
    void updateRegions();

    // ========================================================================
    // Neuromodulation
    // ========================================================================

    // Add a node that modulates other connections
    void addModulatoryNode(std::mt19937& rng, int targetRegionId = -1);

    // Calculate how plastic a brain region is
    float calculatePlasticity(int regionId) const;

    // Get modulatory connections
    const std::vector<ModulatoryConnection>& getModulatoryConnections() const {
        return m_modulatoryConnections;
    }

    // Add a modulatory connection
    void addModulatoryConnection(int modulatorNode, int targetConnectionInnovation,
                                 float strength, ModulationType type);

    // ========================================================================
    // Brain Fitness / Efficiency
    // ========================================================================

    // Performance per neuron
    float calculateBrainEfficiency() const;

    // Metabolic cost estimate
    float calculateBrainCost() const;

    // Prune unnecessary complexity while maintaining function
    void optimizeForEfficiency(std::mt19937& rng, float pruneThreshold = 0.1f);

    // ========================================================================
    // Accessors
    // ========================================================================

    const std::vector<NodeGene>& getNodes() const { return m_nodes; }
    const std::vector<ConnectionGene>& getConnections() const { return m_connections; }

    std::vector<NodeGene>& getNodes() { return m_nodes; }
    std::vector<ConnectionGene>& getConnections() { return m_connections; }

    int getInputCount() const { return m_inputCount; }
    int getOutputCount() const { return m_outputCount; }
    int getHiddenCount() const;
    int getEnabledConnectionCount() const;
    int getRecurrentConnectionCount() const;
    int getModulatoryNodeCount() const;

    float getFitness() const { return m_fitness; }
    void setFitness(float f) { m_fitness = f; }

    float getAdjustedFitness() const { return m_adjustedFitness; }
    void setAdjustedFitness(float f) { m_adjustedFitness = f; }

    int getSpeciesId() const { return m_speciesId; }
    void setSpeciesId(int id) { m_speciesId = id; }

    int getGeneration() const { return m_generation; }
    void setGeneration(int gen) { m_generation = gen; }

    // ========================================================================
    // Complexity metrics (legacy + new)
    // ========================================================================

    float getComplexity() const;
    int getMaxInnovation() const;
    int getMaxLayer() const;

    // ========================================================================
    // Build neural network from this genome
    // ========================================================================

    std::unique_ptr<NeuralNetwork> buildNetwork() const;

private:
    std::vector<NodeGene> m_nodes;
    std::vector<ConnectionGene> m_connections;
    std::vector<BrainRegion> m_regions;
    std::vector<ModulatoryConnection> m_modulatoryConnections;

    int m_inputCount = 0;
    int m_outputCount = 0;
    int m_generation = 0;

    float m_fitness = 0.0f;
    float m_adjustedFitness = 0.0f;
    int m_speciesId = -1;
    int m_nextRegionId = 0;

    // Helper methods
    bool connectionExists(int from, int to) const;
    bool wouldCreateCycle(int from, int to) const;
    NodeGene* getNode(int id);
    const NodeGene* getNode(int id) const;

    std::vector<int> getNodeIds(NodeType type) const;
    std::vector<int> getValidSourceNodes() const;
    std::vector<int> getValidTargetNodes() const;

    // Region helper methods
    std::unordered_set<int> getNeighbors(int nodeId) const;
    float calculateLocalClusteringCoefficient(int nodeId) const;
    int findNodeRegion(int nodeId) const;
    void assignNodeToRegion(int nodeId, int regionId);
};

// ============================================================================
// Mutation Parameters
// ============================================================================

struct MutationParams {
    // Topology mutations
    float addConnectionProb = 0.05f;
    float addNodeProb = 0.03f;
    float toggleEnableProb = 0.01f;

    // Weight mutations
    float mutateWeightProb = 0.8f;
    float weightPerturbProb = 0.9f;      // vs complete replacement
    float weightPerturbStrength = 0.5f;

    // Bias mutations
    float mutateBiasProb = 0.3f;
    float biasPerturbStrength = 0.2f;

    // Activation mutations
    float mutateActivationProb = 0.05f;

    // Plasticity mutations
    float mutatePlasticityProb = 0.1f;

    // Recurrent connections
    bool allowRecurrent = true;
    float recurrentProb = 0.2f;  // Probability new connection is recurrent

    // ========================================================================
    // Advanced Topology Evolution Parameters
    // ========================================================================

    // Module mutations
    float addModuleProb = 0.02f;         // Add a cluster of connected nodes
    int moduleSize = 3;                   // Default size of new modules
    float splitModuleProb = 0.01f;        // Split existing module into two
    float transferModuleProb = 0.005f;    // Copy module from another genome

    // Recurrence mutations
    float addRecurrenceProb = 0.03f;      // Add recurrent connection

    // Pruning
    float pruneDeadEndsProb = 0.02f;      // Remove disconnected nodes

    // Neuromodulation
    float addModulatoryNodeProb = 0.01f;  // Add a neuromodulatory node
    float modulatoryConnectionProb = 0.02f; // Add modulatory connection

    // Efficiency optimization
    float optimizeEfficiencyProb = 0.01f; // Prune for efficiency
    float efficiencyPruneThreshold = 0.1f; // Threshold for pruning

    // Brain speciation parameters
    float structureDistanceWeight = 0.3f; // Weight for structure in distance calc
};

// ============================================================================
// Species for NEAT Speciation
// ============================================================================

class Species {
public:
    int id;
    std::vector<NEATGenome*> members;
    NEATGenome representative;
    float totalAdjustedFitness = 0.0f;
    int stagnantGenerations = 0;
    float bestFitness = 0.0f;

    Species(int id_, const NEATGenome& rep)
        : id(id_), representative(rep) {}

    void updateRepresentative(std::mt19937& rng) {
        if (!members.empty()) {
            std::uniform_int_distribution<int> dist(0, static_cast<int>(members.size()) - 1);
            representative = *members[dist(rng)];
        }
    }

    void calculateAdjustedFitness() {
        totalAdjustedFitness = 0.0f;
        if (members.empty()) return;

        float speciesSize = static_cast<float>(members.size());
        for (auto* genome : members) {
            genome->setAdjustedFitness(genome->getFitness() / speciesSize);
            totalAdjustedFitness += genome->getAdjustedFitness();
        }
    }

    void updateStagnation() {
        float currentBest = 0.0f;
        for (auto* genome : members) {
            currentBest = std::max(currentBest, genome->getFitness());
        }

        if (currentBest > bestFitness) {
            bestFitness = currentBest;
            stagnantGenerations = 0;
        } else {
            stagnantGenerations++;
        }
    }

    void clear() {
        members.clear();
        totalAdjustedFitness = 0.0f;
    }
};

// ============================================================================
// NEAT Population Manager
// ============================================================================

class NEATPopulation {
public:
    NEATPopulation(int populationSize, int numInputs, int numOutputs);

    void evaluateFitness(std::function<float(const NEATGenome&)> fitnessFunc);
    void evolve();

    // Accessors
    std::vector<NEATGenome>& getGenomes() { return m_genomes; }
    const std::vector<NEATGenome>& getGenomes() const { return m_genomes; }
    const NEATGenome& getBestGenome() const { return m_bestGenome; }
    int getGeneration() const { return m_generation; }
    int getSpeciesCount() const { return static_cast<int>(m_species.size()); }

    // Parameters
    MutationParams mutationParams;

    // Speciation parameters
    float compatibilityThreshold = 3.0f;
    float c1_excess = 1.0f;
    float c2_disjoint = 1.0f;
    float c3_weight = 0.4f;

    // Survival parameters
    float survivalThreshold = 0.2f;  // Top % that can reproduce
    int stagnationLimit = 15;         // Generations before species is penalized

private:
    std::vector<NEATGenome> m_genomes;
    std::vector<Species> m_species;
    NEATGenome m_bestGenome;

    int m_populationSize;
    int m_numInputs;
    int m_numOutputs;
    int m_generation = 0;
    int m_nextSpeciesId = 0;

    std::mt19937 m_rng;

    void speciate();
    void reproduceSpecies();
    NEATGenome reproduce(Species& species);
};

// ============================================================================
// Inline Implementations
// ============================================================================

inline int NEATGenome::getHiddenCount() const {
    int count = 0;
    for (const auto& node : m_nodes) {
        if (node.type == NodeType::HIDDEN) count++;
    }
    return count;
}

inline int NEATGenome::getEnabledConnectionCount() const {
    int count = 0;
    for (const auto& conn : m_connections) {
        if (conn.enabled) count++;
    }
    return count;
}

inline int NEATGenome::getRecurrentConnectionCount() const {
    int count = 0;
    for (const auto& conn : m_connections) {
        if (conn.enabled && conn.recurrent) count++;
    }
    return count;
}

inline int NEATGenome::getModulatoryNodeCount() const {
    int count = 0;
    for (const auto& node : m_nodes) {
        if (node.isModulatory) count++;
    }
    return count;
}

inline float NEATGenome::getComplexity() const {
    return static_cast<float>(getEnabledConnectionCount()) +
           static_cast<float>(getHiddenCount()) * 2.0f;
}

inline int NEATGenome::getMaxInnovation() const {
    int maxInnov = 0;
    for (const auto& conn : m_connections) {
        maxInnov = std::max(maxInnov, conn.innovation);
    }
    return maxInnov;
}

inline int NEATGenome::getMaxLayer() const {
    int maxLayer = 0;
    for (const auto& node : m_nodes) {
        maxLayer = std::max(maxLayer, node.layer);
    }
    return maxLayer;
}

inline bool NEATGenome::connectionExists(int from, int to) const {
    for (const auto& conn : m_connections) {
        if (conn.fromNode == from && conn.toNode == to) {
            return true;
        }
    }
    return false;
}

inline NodeGene* NEATGenome::getNode(int id) {
    for (auto& node : m_nodes) {
        if (node.id == id) return &node;
    }
    return nullptr;
}

inline const NodeGene* NEATGenome::getNode(int id) const {
    for (const auto& node : m_nodes) {
        if (node.id == id) return &node;
    }
    return nullptr;
}

inline std::vector<int> NEATGenome::getNodeIds(NodeType type) const {
    std::vector<int> ids;
    for (const auto& node : m_nodes) {
        if (node.type == type) {
            ids.push_back(node.id);
        }
    }
    return ids;
}

inline std::vector<int> NEATGenome::getValidSourceNodes() const {
    std::vector<int> ids;
    for (const auto& node : m_nodes) {
        if (node.type != NodeType::OUTPUT) {
            ids.push_back(node.id);
        }
    }
    return ids;
}

inline std::vector<int> NEATGenome::getValidTargetNodes() const {
    std::vector<int> ids;
    for (const auto& node : m_nodes) {
        if (node.type != NodeType::INPUT && node.type != NodeType::BIAS) {
            ids.push_back(node.id);
        }
    }
    return ids;
}

inline std::unordered_set<int> NEATGenome::getNeighbors(int nodeId) const {
    std::unordered_set<int> neighbors;
    for (const auto& conn : m_connections) {
        if (!conn.enabled) continue;
        if (conn.fromNode == nodeId) {
            neighbors.insert(conn.toNode);
        }
        if (conn.toNode == nodeId) {
            neighbors.insert(conn.fromNode);
        }
    }
    return neighbors;
}

inline int NEATGenome::findNodeRegion(int nodeId) const {
    const NodeGene* node = getNode(nodeId);
    if (node) return node->regionId;
    return -1;
}

inline std::unique_ptr<NeuralNetwork> NEATGenome::buildNetwork() const {
    auto network = std::make_unique<NeuralNetwork>();
    network->buildFromGenome(*this);
    return network;
}

} // namespace ai
