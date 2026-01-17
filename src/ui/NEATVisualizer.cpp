#include "NEATVisualizer.h"
#include "implot.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>

namespace ui {

// ============================================================================
// Neural Network Visualizer Implementation
// ============================================================================

ImU32 NeuralNetworkVisualizer::getNodeColor(ai::NodeType type, float activation) const {
    float intensity = m_showActivations ? std::abs(activation) : 0.5f;
    intensity = std::clamp(intensity, 0.0f, 1.0f);

    switch (type) {
        case ai::NodeType::INPUT:
            return IM_COL32(100, 150 + (int)(105 * intensity), 255, 255);  // Blue
        case ai::NodeType::OUTPUT:
            return IM_COL32(255, 100 + (int)(100 * intensity), 100, 255);  // Red
        case ai::NodeType::HIDDEN:
            return IM_COL32(100 + (int)(100 * intensity), 255, 100 + (int)(100 * intensity), 255);  // Green
        case ai::NodeType::BIAS:
            return IM_COL32(255, 255, 100, 255);  // Yellow
        default:
            return IM_COL32(200, 200, 200, 255);  // Gray
    }
}

ImU32 NeuralNetworkVisualizer::getConnectionColor(float weight, bool enabled) const {
    if (!enabled) {
        return IM_COL32(100, 100, 100, 100);  // Disabled: gray, semi-transparent
    }

    // Positive weights: green, Negative weights: red
    float absWeight = std::abs(weight);
    float normalized = std::min(1.0f, absWeight / 2.0f);  // Normalize to [0,1]
    int intensity = 100 + (int)(155 * normalized);

    if (weight > 0) {
        return IM_COL32(50, intensity, 50, 200);  // Green for positive
    } else {
        return IM_COL32(intensity, 50, 50, 200);  // Red for negative
    }
}

std::vector<NeuralNetworkVisualizer::NodeLayout> NeuralNetworkVisualizer::calculateLayout(
    const ai::NeuralNetwork* network,
    const ImVec2& canvasPos,
    const ImVec2& canvasSize) const {

    std::vector<NodeLayout> layout;
    if (!network) return layout;

    const auto& nodes = network->getNodes();
    if (nodes.empty()) return layout;

    // Group nodes by layer
    std::unordered_map<int, std::vector<size_t>> layerNodes;
    int minLayer = INT_MAX, maxLayer = INT_MIN;

    for (size_t i = 0; i < nodes.size(); ++i) {
        layerNodes[nodes[i].layer].push_back(i);
        minLayer = std::min(minLayer, nodes[i].layer);
        maxLayer = std::max(maxLayer, nodes[i].layer);
    }

    int numLayers = maxLayer - minLayer + 1;
    if (numLayers <= 0) return layout;

    float layerSpacing = canvasSize.x / (numLayers + 1);
    float margin = 30.0f;

    // Calculate positions for each node
    for (auto& [layer, indices] : layerNodes) {
        int numNodesInLayer = static_cast<int>(indices.size());
        float nodeSpacing = (canvasSize.y - 2 * margin) / (numNodesInLayer + 1);

        int layerIndex = layer - minLayer;
        float x = canvasPos.x + layerSpacing * (layerIndex + 1);

        for (size_t i = 0; i < indices.size(); ++i) {
            size_t nodeIdx = indices[i];
            float y = canvasPos.y + margin + nodeSpacing * (i + 1);

            NodeLayout nl;
            nl.position = ImVec2(x, y);
            nl.layer = nodes[nodeIdx].layer;
            nl.type = nodes[nodeIdx].type;
            nl.activation = nodes[nodeIdx].value;
            nl.id = nodes[nodeIdx].id;
            layout.push_back(nl);
        }
    }

    return layout;
}

void NeuralNetworkVisualizer::render(const ai::NeuralNetwork* network, const ImVec2& canvasSize) {
    if (!network) {
        ImGui::Text("No network to display");
        return;
    }

    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Draw background
    drawList->AddRectFilled(canvasPos,
                            ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                            IM_COL32(30, 30, 35, 255));
    drawList->AddRect(canvasPos,
                      ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                      IM_COL32(80, 80, 90, 255));

    // Calculate node layout
    auto layout = calculateLayout(network, canvasPos, canvasSize);

    // Build ID to position map for connection drawing
    std::unordered_map<int, ImVec2> nodePositions;
    std::unordered_map<int, const NodeLayout*> nodeLayouts;
    for (const auto& nl : layout) {
        nodePositions[nl.id] = nl.position;
        nodeLayouts[nl.id] = &nl;
    }

    // Draw connections first (behind nodes)
    const auto& connections = network->getConnections();
    for (const auto& conn : connections) {
        auto fromIt = nodePositions.find(conn.fromNode);
        auto toIt = nodePositions.find(conn.toNode);

        if (fromIt == nodePositions.end() || toIt == nodePositions.end()) continue;

        ImVec2 from = fromIt->second;
        ImVec2 to = toIt->second;

        ImU32 color = getConnectionColor(conn.weight, conn.enabled);
        float thickness = conn.enabled ? (1.0f + std::abs(conn.weight) * 0.5f) : 0.5f;

        // Draw curved connection for recurrent
        if (conn.recurrent) {
            // Draw as dashed or with different style
            ImVec2 mid = ImVec2((from.x + to.x) / 2, std::min(from.y, to.y) - 20);
            drawList->AddBezierQuadratic(from, mid, to, color, thickness);
        } else {
            drawList->AddLine(from, to, color, thickness);
        }

        // Draw weight text if enabled
        if (m_showWeights && conn.enabled) {
            ImVec2 mid = ImVec2((from.x + to.x) / 2, (from.y + to.y) / 2);
            char weightStr[16];
            snprintf(weightStr, sizeof(weightStr), "%.1f", conn.weight);
            drawList->AddText(mid, IM_COL32(180, 180, 180, 200), weightStr);
        }
    }

    // Draw nodes
    for (const auto& nl : layout) {
        ImU32 color = getNodeColor(nl.type, nl.activation);
        drawList->AddCircleFilled(nl.position, m_nodeRadius, color);
        drawList->AddCircle(nl.position, m_nodeRadius, IM_COL32(200, 200, 200, 255), 0, 2.0f);

        // Draw node ID
        char idStr[8];
        snprintf(idStr, sizeof(idStr), "%d", nl.id);
        ImVec2 textPos = ImVec2(nl.position.x - 4, nl.position.y - 5);
        drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), idStr);
    }

    // Draw legend
    float legendY = canvasPos.y + canvasSize.y - 60;
    float legendX = canvasPos.x + 10;

    drawList->AddCircleFilled(ImVec2(legendX, legendY), 5, getNodeColor(ai::NodeType::INPUT, 0.5f));
    drawList->AddText(ImVec2(legendX + 10, legendY - 5), IM_COL32(200, 200, 200, 255), "Input");

    drawList->AddCircleFilled(ImVec2(legendX, legendY + 15), 5, getNodeColor(ai::NodeType::HIDDEN, 0.5f));
    drawList->AddText(ImVec2(legendX + 10, legendY + 10), IM_COL32(200, 200, 200, 255), "Hidden");

    drawList->AddCircleFilled(ImVec2(legendX, legendY + 30), 5, getNodeColor(ai::NodeType::OUTPUT, 0.5f));
    drawList->AddText(ImVec2(legendX + 10, legendY + 25), IM_COL32(200, 200, 200, 255), "Output");

    // Reserve space
    ImGui::Dummy(canvasSize);
}

void NeuralNetworkVisualizer::renderCompact(const ai::NeuralNetwork* network, const ImVec2& canvasSize) {
    // Simplified rendering for smaller displays
    float savedRadius = m_nodeRadius;
    bool savedWeights = m_showWeights;

    m_nodeRadius = 6.0f;
    m_showWeights = false;

    render(network, canvasSize);

    m_nodeRadius = savedRadius;
    m_showWeights = savedWeights;
}

// ============================================================================
// NEAT Evolution Panel Implementation
// ============================================================================

void NEATEvolutionPanel::update(const ai::BrainEvolutionManager* manager) {
    if (!manager) return;

    m_generation = manager->getGeneration();
    m_speciesCount = manager->getSpeciesCount();
    m_bestFitness = manager->getBestFitness();
    m_avgFitness = manager->getAverageFitness();
    m_avgComplexity = manager->getAverageComplexity();

    const auto& bestGenome = manager->getBestGenome();
    m_bestGenomeNodes = static_cast<int>(bestGenome.getNodes().size());
    m_bestGenomeConnections = bestGenome.getEnabledConnectionCount();
    m_bestGenomeHiddenNodes = bestGenome.getHiddenCount();

    recordHistory();
}

void NEATEvolutionPanel::recordHistory() {
    m_fitnessHistory.push_back(m_avgFitness);
    m_bestFitnessHistory.push_back(m_bestFitness);
    m_speciesHistory.push_back(static_cast<float>(m_speciesCount));
    m_complexityHistory.push_back(m_avgComplexity);

    trimHistory();
}

void NEATEvolutionPanel::trimHistory() {
    while (m_fitnessHistory.size() > HISTORY_SIZE) {
        m_fitnessHistory.erase(m_fitnessHistory.begin());
    }
    while (m_bestFitnessHistory.size() > HISTORY_SIZE) {
        m_bestFitnessHistory.erase(m_bestFitnessHistory.begin());
    }
    while (m_speciesHistory.size() > HISTORY_SIZE) {
        m_speciesHistory.erase(m_speciesHistory.begin());
    }
    while (m_complexityHistory.size() > HISTORY_SIZE) {
        m_complexityHistory.erase(m_complexityHistory.begin());
    }
}

void NEATEvolutionPanel::render() {
    if (ImGui::Begin("NEAT Evolution", nullptr, ImGuiWindowFlags_NoCollapse)) {

        // Header statistics
        ImGui::Text("Generation: %d", m_generation);
        ImGui::SameLine(150);
        ImGui::Text("Species: %d", m_speciesCount);
        ImGui::SameLine(280);
        ImGui::Text("Complexity: %.1f", m_avgComplexity);

        ImGui::Separator();

        // Fitness section
        if (ImGui::CollapsingHeader("Fitness", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Best Fitness: %.2f", m_bestFitness);
            ImGui::Text("Avg Fitness:  %.2f", m_avgFitness);

            renderFitnessGraph();
        }

        // Species section
        if (ImGui::CollapsingHeader("Species Distribution")) {
            renderSpeciesGraph();

            if (m_showSpeciesBreakdown) {
                renderSpeciesBreakdown();
            }
        }

        // Topology section
        if (ImGui::CollapsingHeader("Network Topology")) {
            renderComplexityGraph();
            renderBestGenomeInfo();
        }
    }
    ImGui::End();
}

void NEATEvolutionPanel::renderCompact() {
    ImGui::Text("Gen: %d  Species: %d", m_generation, m_speciesCount);
    ImGui::Text("Best: %.1f  Avg: %.1f", m_bestFitness, m_avgFitness);

    // Mini fitness graph
    if (!m_fitnessHistory.empty()) {
        ImGui::PlotLines("##fitness_mini", m_fitnessHistory.data(),
                         static_cast<int>(m_fitnessHistory.size()),
                         0, nullptr, 0.0f, FLT_MAX, ImVec2(0, 40));
    }
}

void NEATEvolutionPanel::renderFitnessGraph() {
    if (m_fitnessHistory.empty()) {
        ImGui::Text("No fitness data yet");
        return;
    }

    // Use ImPlot for better visualization
    if (ImPlot::BeginPlot("Fitness Over Generations", ImVec2(-1, 150))) {
        ImPlot::SetupAxes("Generation", "Fitness");

        // Generate x-axis data
        std::vector<float> xData(m_fitnessHistory.size());
        for (size_t i = 0; i < xData.size(); ++i) {
            xData[i] = static_cast<float>(i);
        }

        // Average fitness line
        ImPlot::SetNextLineStyle(ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
        ImPlot::PlotLine("Average", xData.data(), m_fitnessHistory.data(),
                        static_cast<int>(m_fitnessHistory.size()));

        // Best fitness line
        if (!m_bestFitnessHistory.empty()) {
            ImPlot::SetNextLineStyle(ImVec4(0.9f, 0.7f, 0.2f, 1.0f));
            ImPlot::PlotLine("Best", xData.data(), m_bestFitnessHistory.data(),
                            static_cast<int>(m_bestFitnessHistory.size()));
        }

        ImPlot::EndPlot();
    }
}

void NEATEvolutionPanel::renderSpeciesGraph() {
    if (m_speciesHistory.empty()) {
        ImGui::Text("No species data yet");
        return;
    }

    if (ImPlot::BeginPlot("Species Over Generations", ImVec2(-1, 120))) {
        ImPlot::SetupAxes("Generation", "Species Count");

        std::vector<float> xData(m_speciesHistory.size());
        for (size_t i = 0; i < xData.size(); ++i) {
            xData[i] = static_cast<float>(i);
        }

        ImPlot::SetNextLineStyle(ImVec4(0.5f, 0.5f, 0.9f, 1.0f));
        ImPlot::PlotLine("Species", xData.data(), m_speciesHistory.data(),
                        static_cast<int>(m_speciesHistory.size()));

        ImPlot::EndPlot();
    }
}

void NEATEvolutionPanel::renderComplexityGraph() {
    if (m_complexityHistory.empty()) {
        ImGui::Text("No complexity data yet");
        return;
    }

    if (ImPlot::BeginPlot("Network Complexity", ImVec2(-1, 120))) {
        ImPlot::SetupAxes("Generation", "Complexity");

        std::vector<float> xData(m_complexityHistory.size());
        for (size_t i = 0; i < xData.size(); ++i) {
            xData[i] = static_cast<float>(i);
        }

        ImPlot::SetNextLineStyle(ImVec4(0.9f, 0.5f, 0.5f, 1.0f));
        ImPlot::PlotLine("Complexity", xData.data(), m_complexityHistory.data(),
                        static_cast<int>(m_complexityHistory.size()));

        ImPlot::EndPlot();
    }
}

void NEATEvolutionPanel::renderSpeciesBreakdown() {
    if (m_speciesStats.empty()) {
        ImGui::Text("No species to display");
        return;
    }

    ImGui::Text("Species Breakdown:");

    if (ImGui::BeginTable("species_table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 40);
        ImGui::TableSetupColumn("Members", ImGuiTableColumnFlags_WidthFixed, 60);
        ImGui::TableSetupColumn("Avg Fit", ImGuiTableColumnFlags_WidthFixed, 60);
        ImGui::TableSetupColumn("Stagnant", ImGuiTableColumnFlags_WidthFixed, 60);
        ImGui::TableHeadersRow();

        for (const auto& species : m_speciesStats) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(species.color, "%d", species.id);

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d", species.memberCount);

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.1f", species.avgFitness);

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%d", species.stagnantGenerations);
        }

        ImGui::EndTable();
    }
}

void NEATEvolutionPanel::renderBestGenomeInfo() {
    ImGui::Text("Best Genome Topology:");
    ImGui::BulletText("Total Nodes: %d", m_bestGenomeNodes);
    ImGui::BulletText("Hidden Nodes: %d", m_bestGenomeHiddenNodes);
    ImGui::BulletText("Connections: %d", m_bestGenomeConnections);
}

// ============================================================================
// Genome Serializer Implementation
// ============================================================================

bool GenomeSerializer::saveGenome(const ai::NEATGenome& genome, const std::string& filepath) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;

    // Write header
    const char* magic = "NEAT";
    file.write(magic, 4);

    int version = 1;
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));

    // Write node count and nodes
    const auto& nodes = genome.getNodes();
    int nodeCount = static_cast<int>(nodes.size());
    file.write(reinterpret_cast<const char*>(&nodeCount), sizeof(nodeCount));

    for (const auto& node : nodes) {
        file.write(reinterpret_cast<const char*>(&node.id), sizeof(node.id));
        int typeInt = static_cast<int>(node.type);
        file.write(reinterpret_cast<const char*>(&typeInt), sizeof(typeInt));
        int actInt = static_cast<int>(node.activation);
        file.write(reinterpret_cast<const char*>(&actInt), sizeof(actInt));
        file.write(reinterpret_cast<const char*>(&node.bias), sizeof(node.bias));
        file.write(reinterpret_cast<const char*>(&node.layer), sizeof(node.layer));
    }

    // Write connection count and connections
    const auto& connections = genome.getConnections();
    int connCount = static_cast<int>(connections.size());
    file.write(reinterpret_cast<const char*>(&connCount), sizeof(connCount));

    for (const auto& conn : connections) {
        file.write(reinterpret_cast<const char*>(&conn.innovation), sizeof(conn.innovation));
        file.write(reinterpret_cast<const char*>(&conn.fromNode), sizeof(conn.fromNode));
        file.write(reinterpret_cast<const char*>(&conn.toNode), sizeof(conn.toNode));
        file.write(reinterpret_cast<const char*>(&conn.weight), sizeof(conn.weight));
        file.write(reinterpret_cast<const char*>(&conn.enabled), sizeof(conn.enabled));
        file.write(reinterpret_cast<const char*>(&conn.recurrent), sizeof(conn.recurrent));
    }

    // Write fitness
    float fitness = genome.getFitness();
    file.write(reinterpret_cast<const char*>(&fitness), sizeof(fitness));

    file.close();
    return true;
}

bool GenomeSerializer::loadGenome(ai::NEATGenome& genome, const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;

    // Read and verify header
    char magic[5] = {0};
    file.read(magic, 4);
    if (std::string(magic) != "NEAT") return false;

    int version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != 1) return false;

    // Read nodes
    int nodeCount;
    file.read(reinterpret_cast<char*>(&nodeCount), sizeof(nodeCount));

    auto& nodes = genome.getNodes();
    nodes.clear();
    nodes.reserve(nodeCount);

    for (int i = 0; i < nodeCount; ++i) {
        int id, typeInt, actInt, layer;
        float bias;

        file.read(reinterpret_cast<char*>(&id), sizeof(id));
        file.read(reinterpret_cast<char*>(&typeInt), sizeof(typeInt));
        file.read(reinterpret_cast<char*>(&actInt), sizeof(actInt));
        file.read(reinterpret_cast<char*>(&bias), sizeof(bias));
        file.read(reinterpret_cast<char*>(&layer), sizeof(layer));

        nodes.emplace_back(id, static_cast<ai::NodeType>(typeInt),
                           static_cast<ai::ActivationType>(actInt), bias, layer);
    }

    // Read connections
    int connCount;
    file.read(reinterpret_cast<char*>(&connCount), sizeof(connCount));

    auto& connections = genome.getConnections();
    connections.clear();
    connections.reserve(connCount);

    for (int i = 0; i < connCount; ++i) {
        int innovation, fromNode, toNode;
        float weight;
        bool enabled, recurrent;

        file.read(reinterpret_cast<char*>(&innovation), sizeof(innovation));
        file.read(reinterpret_cast<char*>(&fromNode), sizeof(fromNode));
        file.read(reinterpret_cast<char*>(&toNode), sizeof(toNode));
        file.read(reinterpret_cast<char*>(&weight), sizeof(weight));
        file.read(reinterpret_cast<char*>(&enabled), sizeof(enabled));
        file.read(reinterpret_cast<char*>(&recurrent), sizeof(recurrent));

        connections.emplace_back(innovation, fromNode, toNode, weight, enabled, recurrent);
    }

    // Read fitness
    float fitness;
    file.read(reinterpret_cast<char*>(&fitness), sizeof(fitness));
    genome.setFitness(fitness);

    file.close();
    return true;
}

bool GenomeSerializer::savePopulation(const std::vector<ai::NEATGenome>& genomes, const std::string& filepath) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;

    // Write header
    const char* magic = "NPOP";
    file.write(magic, 4);

    int version = 1;
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));

    int popSize = static_cast<int>(genomes.size());
    file.write(reinterpret_cast<const char*>(&popSize), sizeof(popSize));

    file.close();

    // Save each genome
    for (int i = 0; i < popSize; ++i) {
        std::string genomePath = filepath + "." + std::to_string(i);
        if (!saveGenome(genomes[i], genomePath)) return false;
    }

    return true;
}

bool GenomeSerializer::loadPopulation(std::vector<ai::NEATGenome>& genomes, const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;

    // Read and verify header
    char magic[5] = {0};
    file.read(magic, 4);
    if (std::string(magic) != "NPOP") return false;

    int version;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != 1) return false;

    int popSize;
    file.read(reinterpret_cast<char*>(&popSize), sizeof(popSize));

    file.close();

    // Load each genome
    genomes.clear();
    genomes.resize(popSize);

    for (int i = 0; i < popSize; ++i) {
        std::string genomePath = filepath + "." + std::to_string(i);
        if (!loadGenome(genomes[i], genomePath)) return false;
    }

    return true;
}

bool GenomeSerializer::exportToJSON(const ai::NEATGenome& genome, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "{\n";
    file << "  \"fitness\": " << genome.getFitness() << ",\n";
    file << "  \"inputCount\": " << genome.getInputCount() << ",\n";
    file << "  \"outputCount\": " << genome.getOutputCount() << ",\n";
    file << "  \"hiddenCount\": " << genome.getHiddenCount() << ",\n";
    file << "  \"connectionCount\": " << genome.getEnabledConnectionCount() << ",\n";
    file << "  \"complexity\": " << genome.getComplexity() << ",\n";

    // Nodes
    file << "  \"nodes\": [\n";
    const auto& nodes = genome.getNodes();
    for (size_t i = 0; i < nodes.size(); ++i) {
        const auto& node = nodes[i];
        file << "    { \"id\": " << node.id
             << ", \"type\": \"" << (node.type == ai::NodeType::INPUT ? "INPUT" :
                                      node.type == ai::NodeType::OUTPUT ? "OUTPUT" :
                                      node.type == ai::NodeType::HIDDEN ? "HIDDEN" : "BIAS")
             << "\", \"bias\": " << std::fixed << std::setprecision(4) << node.bias
             << ", \"layer\": " << node.layer << " }";
        if (i < nodes.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ],\n";

    // Connections
    file << "  \"connections\": [\n";
    const auto& connections = genome.getConnections();
    for (size_t i = 0; i < connections.size(); ++i) {
        const auto& conn = connections[i];
        file << "    { \"innovation\": " << conn.innovation
             << ", \"from\": " << conn.fromNode
             << ", \"to\": " << conn.toNode
             << ", \"weight\": " << std::fixed << std::setprecision(4) << conn.weight
             << ", \"enabled\": " << (conn.enabled ? "true" : "false")
             << ", \"recurrent\": " << (conn.recurrent ? "true" : "false") << " }";
        if (i < connections.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ]\n";

    file << "}\n";

    file.close();
    return true;
}

// ============================================================================
// Creature Brain Inspector Implementation
// ============================================================================

void CreatureBrainInspector::setTarget(const ai::CreatureBrainInterface* brain) {
    m_targetBrain = brain;
    if (brain) {
        m_brainType = brain->getBrainType();
    }
}

void CreatureBrainInspector::render() {
    if (!m_targetBrain) {
        ImGui::Text("No creature brain selected");
        return;
    }

    renderBrainType();
    ImGui::Separator();

    renderNeuromodulators();
    ImGui::Separator();

    renderDrives();
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Network Topology", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderNetworkTopology();
    }

    if (ImGui::CollapsingHeader("Weight Distribution")) {
        renderWeightDistribution();
    }
}

void CreatureBrainInspector::renderBrainType() {
    const char* typeStr = "Unknown";
    switch (m_brainType) {
        case ai::CreatureBrainInterface::BrainType::LEGACY_STEERING:
            typeStr = "Legacy Steering";
            break;
        case ai::CreatureBrainInterface::BrainType::MODULAR_BRAIN:
            typeStr = "Modular Brain";
            break;
        case ai::CreatureBrainInterface::BrainType::NEAT_EVOLVED:
            typeStr = "NEAT Evolved";
            break;
    }

    ImGui::Text("Brain Type: %s", typeStr);
    ImGui::Text("Complexity: %.1f", m_targetBrain->getComplexity());
    ImGui::Text("Weight Count: %zu", m_targetBrain->getWeightCount());
}

void CreatureBrainInspector::renderNeuromodulators() {
    const auto& mods = m_targetBrain->getNeuromodulators();

    ImGui::Text("Neuromodulators:");

    // Dopamine bar (reward signal)
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
    ImGui::ProgressBar((mods.dopamine + 1.0f) / 2.0f, ImVec2(-1, 0), "Dopamine");
    ImGui::PopStyleColor();

    // Norepinephrine bar (arousal)
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.4f, 0.2f, 1.0f));
    ImGui::ProgressBar(mods.norepinephrine, ImVec2(-1, 0), "Norepinephrine");
    ImGui::PopStyleColor();

    // Serotonin bar (mood)
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.4f, 0.4f, 0.9f, 1.0f));
    ImGui::ProgressBar(mods.serotonin, ImVec2(-1, 0), "Serotonin");
    ImGui::PopStyleColor();

    // Acetylcholine bar (learning)
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.9f, 0.9f, 0.2f, 1.0f));
    ImGui::ProgressBar(mods.acetylcholine, ImVec2(-1, 0), "Acetylcholine");
    ImGui::PopStyleColor();
}

void CreatureBrainInspector::renderDrives() {
    const auto& drives = m_targetBrain->getDrives();

    ImGui::Text("Emotional Drives:");

    // Fear bar (red)
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
    ImGui::ProgressBar(drives.fear, ImVec2(-1, 0), "Fear");
    ImGui::PopStyleColor();

    // Hunger bar (orange)
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.9f, 0.6f, 0.2f, 1.0f));
    ImGui::ProgressBar(drives.hunger, ImVec2(-1, 0), "Hunger");
    ImGui::PopStyleColor();

    // Curiosity bar (cyan)
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.8f, 0.9f, 1.0f));
    ImGui::ProgressBar(drives.curiosity, ImVec2(-1, 0), "Curiosity");
    ImGui::PopStyleColor();

    // Social bar (purple)
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.7f, 0.3f, 0.9f, 1.0f));
    ImGui::ProgressBar(drives.social, ImVec2(-1, 0), "Social");
    ImGui::PopStyleColor();
}

void CreatureBrainInspector::renderNetworkTopology() {
    // Get the genome and build network for visualization
    const auto& genome = m_targetBrain->getGenome();
    auto network = genome.buildNetwork();

    if (network) {
        m_visualizer.renderCompact(network.get(), ImVec2(300, 200));
    } else {
        ImGui::Text("Could not build network visualization");
    }
}

void CreatureBrainInspector::renderWeightDistribution() {
    const auto& weights = m_targetBrain->getWeights();

    if (weights.empty()) {
        ImGui::Text("No weights available");
        return;
    }

    // Use ImPlot for histogram
    if (ImPlot::BeginPlot("Weight Distribution", ImVec2(-1, 120))) {
        ImPlot::SetupAxes("Weight", "Count");
        ImPlot::PlotHistogram("Weights", weights.data(), static_cast<int>(weights.size()), 20);
        ImPlot::EndPlot();
    }

    // Statistics
    float sum = 0.0f, sumSq = 0.0f;
    float minW = weights[0], maxW = weights[0];
    for (float w : weights) {
        sum += w;
        sumSq += w * w;
        minW = std::min(minW, w);
        maxW = std::max(maxW, w);
    }
    float mean = sum / weights.size();
    float variance = (sumSq / weights.size()) - (mean * mean);
    float stddev = std::sqrt(std::max(0.0f, variance));

    ImGui::Text("Count: %zu | Min: %.2f | Max: %.2f", weights.size(), minW, maxW);
    ImGui::Text("Mean: %.3f | Std: %.3f", mean, stddev);
}

} // namespace ui
