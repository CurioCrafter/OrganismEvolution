#pragma once

#include "../ai/NeuralNetwork.h"
#include "../ai/NEATGenome.h"
#include "../ai/CreatureBrainInterface.h"
#include "imgui.h"
#include <vector>
#include <string>
#include <unordered_map>

namespace ui {

// ============================================================================
// Neural Network Topology Visualizer
// ============================================================================

class NeuralNetworkVisualizer {
public:
    NeuralNetworkVisualizer() = default;

    // Render the neural network topology visualization
    void render(const ai::NeuralNetwork* network, const ImVec2& canvasSize = ImVec2(400, 300));

    // Render compact version for creature inspector
    void renderCompact(const ai::NeuralNetwork* network, const ImVec2& canvasSize = ImVec2(200, 150));

    // Configuration
    void setShowWeights(bool show) { m_showWeights = show; }
    void setShowActivations(bool show) { m_showActivations = show; }
    void setNodeRadius(float radius) { m_nodeRadius = radius; }

private:
    bool m_showWeights = true;
    bool m_showActivations = true;
    float m_nodeRadius = 12.0f;

    // Color helpers
    ImU32 getNodeColor(ai::NodeType type, float activation) const;
    ImU32 getConnectionColor(float weight, bool enabled) const;

    // Layout calculation
    struct NodeLayout {
        ImVec2 position;
        int layer;
        ai::NodeType type;
        float activation;
        int id;
    };

    std::vector<NodeLayout> calculateLayout(const ai::NeuralNetwork* network,
                                             const ImVec2& canvasPos,
                                             const ImVec2& canvasSize) const;
};

// ============================================================================
// NEAT Evolution Statistics Panel
// ============================================================================

class NEATEvolutionPanel {
public:
    NEATEvolutionPanel() = default;

    // Update statistics from evolution manager
    void update(const ai::BrainEvolutionManager* manager);

    // Render the full evolution statistics panel
    void render();

    // Render compact statistics (for sidebar)
    void renderCompact();

    // History for graphs
    static constexpr int HISTORY_SIZE = 300;

private:
    // Current statistics
    int m_generation = 0;
    int m_speciesCount = 0;
    float m_bestFitness = 0.0f;
    float m_avgFitness = 0.0f;
    float m_avgComplexity = 0.0f;
    int m_populationSize = 0;

    // Best genome info
    int m_bestGenomeNodes = 0;
    int m_bestGenomeConnections = 0;
    int m_bestGenomeHiddenNodes = 0;

    // History for graphs
    std::vector<float> m_fitnessHistory;
    std::vector<float> m_speciesHistory;
    std::vector<float> m_complexityHistory;
    std::vector<float> m_bestFitnessHistory;

    // Species tracking
    struct SpeciesStats {
        int id;
        int memberCount;
        float avgFitness;
        float bestFitness;
        int stagnantGenerations;
        ImVec4 color;
    };
    std::vector<SpeciesStats> m_speciesStats;

    // UI state
    bool m_showSpeciesBreakdown = true;
    bool m_showTopologyEvolution = true;
    int m_selectedSpecies = -1;

    // Helper to record history
    void recordHistory();
    void trimHistory();

    // Rendering helpers
    void renderFitnessGraph();
    void renderSpeciesGraph();
    void renderComplexityGraph();
    void renderSpeciesBreakdown();
    void renderBestGenomeInfo();
};

// ============================================================================
// Genome Serialization Helper
// ============================================================================

class GenomeSerializer {
public:
    // Save genome to file (binary format)
    static bool saveGenome(const ai::NEATGenome& genome, const std::string& filepath);

    // Load genome from file
    static bool loadGenome(ai::NEATGenome& genome, const std::string& filepath);

    // Save population to file
    static bool savePopulation(const std::vector<ai::NEATGenome>& genomes, const std::string& filepath);

    // Load population from file
    static bool loadPopulation(std::vector<ai::NEATGenome>& genomes, const std::string& filepath);

    // Export genome to human-readable JSON
    static bool exportToJSON(const ai::NEATGenome& genome, const std::string& filepath);

    // Get file dialog filter
    static const char* getFileFilter() { return "NEAT Genome (*.genome)\0*.genome\0All Files (*.*)\0*.*\0"; }
};

// ============================================================================
// Creature Brain Inspector
// ============================================================================

class CreatureBrainInspector {
public:
    CreatureBrainInspector() = default;

    // Set the brain to inspect
    void setTarget(const ai::CreatureBrainInterface* brain);

    // Render the inspector panel
    void render();

    // Check if inspecting a brain
    bool hasTarget() const { return m_targetBrain != nullptr; }

private:
    const ai::CreatureBrainInterface* m_targetBrain = nullptr;
    NeuralNetworkVisualizer m_visualizer;

    // Cached data
    ai::CreatureBrainInterface::BrainType m_brainType;
    std::vector<float> m_lastInputs;
    std::vector<float> m_lastOutputs;

    // Rendering helpers
    void renderBrainType();
    void renderNeuromodulators();
    void renderDrives();
    void renderNetworkTopology();
    void renderWeightDistribution();
};

} // namespace ui
