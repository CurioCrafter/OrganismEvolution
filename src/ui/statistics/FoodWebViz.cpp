#include "FoodWebViz.h"
#include <algorithm>
#include <cmath>

namespace ui {
namespace statistics {

// ============================================================================
// Constructor
// ============================================================================

FoodWebViz::FoodWebViz() {
    // Initialize resource nodes first (trophic levels 0-1)
    initResourceNodes();

    // Initialize with common creature types
    std::vector<CreatureType> types = {
        CreatureType::GRAZER,
        CreatureType::BROWSER,
        CreatureType::FRUGIVORE,
        CreatureType::SMALL_PREDATOR,
        CreatureType::APEX_PREDATOR,
        CreatureType::OMNIVORE,
        CreatureType::SCAVENGER
    };

    for (CreatureType type : types) {
        FoodWebNode node;
        node.type = type;
        node.nodeType = FoodWebNodeType::CREATURE;
        node.name = getTypeName(type);
        node.trophicLevel = getTrophicLevel(type);
        node.color = getColorForType(type);
        m_nodeIndices[type] = static_cast<int>(m_nodes.size());
        m_nodes.push_back(node);
    }

    // Define predator-prey relationships (edges)
    auto addEdge = [this](int sourceIdx, int targetIdx, float efficiency = 0.10f) {
        if (sourceIdx >= 0 && targetIdx >= 0) {
            FoodWebEdge edge;
            edge.sourceIdx = sourceIdx;
            edge.targetIdx = targetIdx;
            edge.efficiency = efficiency;
            m_edges.push_back(edge);
        }
    };

    auto addCreatureEdge = [this, &addEdge](CreatureType prey, CreatureType predator) {
        addEdge(m_nodeIndices[prey], m_nodeIndices[predator], 0.10f);
    };

    // ====== NEW: Resource to Creature edges ======

    // Producers -> Herbivores (20% efficiency for primary consumers)
    addEdge(m_producerNodeIdx, m_nodeIndices[CreatureType::GRAZER], 0.20f);
    addEdge(m_producerNodeIdx, m_nodeIndices[CreatureType::BROWSER], 0.20f);
    addEdge(m_producerNodeIdx, m_nodeIndices[CreatureType::FRUGIVORE], 0.20f);
    addEdge(m_producerNodeIdx, m_nodeIndices[CreatureType::OMNIVORE], 0.15f);

    // Plankton -> some herbivores (aquatic food)
    addEdge(m_planktonNodeIdx, m_nodeIndices[CreatureType::GRAZER], 0.15f);

    // Detritus -> Scavengers (high efficiency for carrion)
    addEdge(m_detritusNodeIdx, m_nodeIndices[CreatureType::SCAVENGER], 0.30f);

    // Detritus -> Decomposers
    addEdge(m_detritusNodeIdx, m_decomposerNodeIdx, 0.50f);

    // Decomposers -> Nutrients (recycling loop)
    addEdge(m_decomposerNodeIdx, m_nutrientNodeIdx, 0.60f);

    // Nutrients -> Producers (nutrient uptake)
    addEdge(m_nutrientNodeIdx, m_producerNodeIdx, 0.40f);

    // ====== Original Creature to Creature edges ======

    // Herbivores -> Small Predators
    addCreatureEdge(CreatureType::GRAZER, CreatureType::SMALL_PREDATOR);
    addCreatureEdge(CreatureType::BROWSER, CreatureType::SMALL_PREDATOR);
    addCreatureEdge(CreatureType::FRUGIVORE, CreatureType::SMALL_PREDATOR);

    // Herbivores -> Apex Predators
    addCreatureEdge(CreatureType::GRAZER, CreatureType::APEX_PREDATOR);
    addCreatureEdge(CreatureType::BROWSER, CreatureType::APEX_PREDATOR);

    // Small Predators -> Apex Predators
    addCreatureEdge(CreatureType::SMALL_PREDATOR, CreatureType::APEX_PREDATOR);

    // Herbivores -> Omnivores
    addCreatureEdge(CreatureType::FRUGIVORE, CreatureType::OMNIVORE);

    // Omnivores -> Apex Predators
    addCreatureEdge(CreatureType::OMNIVORE, CreatureType::APEX_PREDATOR);

    // ====== Death -> Detritus edges (corpses become detritus) ======
    // All creatures -> Detritus (when they die)
    for (auto& [type, idx] : m_nodeIndices) {
        addEdge(idx, m_detritusNodeIdx, 0.50f);  // 50% of biomass becomes detritus
    }
}

void FoodWebViz::initResourceNodes() {
    // Nutrients (Trophic Level 0) - soil nutrients
    {
        FoodWebNode node;
        node.nodeType = FoodWebNodeType::NUTRIENT;
        node.name = "Nutrients";
        node.trophicLevel = 0;
        node.color = glm::vec3(0.6f, 0.4f, 0.2f);  // Brown
        node.biomass = 100.0f;
        m_nutrientNodeIdx = static_cast<int>(m_nodes.size());
        m_nodes.push_back(node);
    }

    // Producers (Trophic Level 1) - plants, grass, trees
    {
        FoodWebNode node;
        node.nodeType = FoodWebNodeType::PRODUCER;
        node.name = "Producers";
        node.trophicLevel = 1;
        node.color = glm::vec3(0.2f, 0.7f, 0.2f);  // Green
        node.biomass = 500.0f;
        m_producerNodeIdx = static_cast<int>(m_nodes.size());
        m_nodes.push_back(node);
    }

    // Plankton (Trophic Level 1) - aquatic producers
    {
        FoodWebNode node;
        node.nodeType = FoodWebNodeType::PLANKTON;
        node.name = "Plankton";
        node.trophicLevel = 1;
        node.color = glm::vec3(0.3f, 0.6f, 0.8f);  // Light blue
        node.biomass = 100.0f;
        m_planktonNodeIdx = static_cast<int>(m_nodes.size());
        m_nodes.push_back(node);
    }

    // Detritus (Trophic Level 0.5) - dead organic matter
    {
        FoodWebNode node;
        node.nodeType = FoodWebNodeType::DETRITUS;
        node.name = "Detritus";
        node.trophicLevel = 0;  // Below producers
        node.color = glm::vec3(0.5f, 0.35f, 0.2f);  // Dark brown
        node.biomass = 50.0f;
        m_detritusNodeIdx = static_cast<int>(m_nodes.size());
        m_nodes.push_back(node);
    }

    // Decomposers (Trophic Level 1) - fungi, bacteria
    {
        FoodWebNode node;
        node.nodeType = FoodWebNodeType::DECOMPOSER;
        node.name = "Decomposers";
        node.trophicLevel = 1;
        node.color = glm::vec3(0.7f, 0.5f, 0.6f);  // Purple-brown (mushroom color)
        node.biomass = 100.0f;
        m_decomposerNodeIdx = static_cast<int>(m_nodes.size());
        m_nodes.push_back(node);
    }
}

// ============================================================================
// Update
// ============================================================================

void FoodWebViz::update(const Forge::FoodChainManager& foodChain,
                        const Forge::CreatureManager& creatures) {
    const auto& balance = foodChain.getPopulationBalance();
    const auto& energyStats = foodChain.getEnergyStats();

    // Update node populations and biomass for creature nodes
    for (auto& node : m_nodes) {
        if (node.nodeType != FoodWebNodeType::CREATURE) continue;

        auto it = balance.currentPopulation.find(node.type);
        if (it != balance.currentPopulation.end()) {
            node.population = it->second;
        } else {
            node.population = 0;
        }

        // Estimate biomass (simplified: population * avg size)
        node.biomass = static_cast<float>(node.population) * 1.0f;

        // Scale radius by population (log scale)
        float popScale = node.population > 0 ? std::log(1.0f + node.population) / std::log(500.0f) : 0.0f;
        node.radius = 10.0f + 30.0f * std::clamp(popScale, 0.0f, 1.0f);
    }

    // Update energy flow values
    for (auto& edge : m_edges) {
        if (edge.sourceIdx >= 0 && edge.targetIdx >= 0) {
            const auto& source = m_nodes[edge.sourceIdx];
            const auto& target = m_nodes[edge.targetIdx];

            // Use biomass for resource nodes, population for creatures
            float sourceValue = (source.nodeType == FoodWebNodeType::CREATURE)
                ? static_cast<float>(source.population) : source.biomass / 10.0f;
            float targetValue = (target.nodeType == FoodWebNodeType::CREATURE)
                ? static_cast<float>(target.population) : target.biomass / 10.0f;

            // Estimate energy transfer based on both endpoints
            edge.energyTransfer = std::min(sourceValue, targetValue) * edge.efficiency * 0.1f;
        }
    }
}

void FoodWebViz::updateResources(float producerBiomass, float detritusLevel,
                                 float decomposerActivity, float planktonLevel,
                                 float bloomMultiplier, int bloomType) {
    // Update producer node
    if (m_producerNodeIdx >= 0 && m_producerNodeIdx < static_cast<int>(m_nodes.size())) {
        auto& node = m_nodes[m_producerNodeIdx];
        node.biomass = producerBiomass;
        node.bloomMultiplier = bloomMultiplier;
        // Scale radius by biomass (log scale)
        float biomassScale = std::log(1.0f + producerBiomass) / std::log(1000.0f);
        node.radius = 15.0f + 35.0f * std::clamp(biomassScale, 0.0f, 1.0f);

        // Brighten color during bloom
        if (bloomMultiplier > 1.1f) {
            node.color = glm::vec3(0.3f, 0.85f, 0.3f);  // Brighter green
        } else {
            node.color = glm::vec3(0.2f, 0.7f, 0.2f);   // Normal green
        }
    }

    // Update detritus node
    if (m_detritusNodeIdx >= 0 && m_detritusNodeIdx < static_cast<int>(m_nodes.size())) {
        auto& node = m_nodes[m_detritusNodeIdx];
        node.biomass = detritusLevel * 100.0f;  // Scale to visible range
        float detritusScale = std::log(1.0f + node.biomass) / std::log(500.0f);
        node.radius = 10.0f + 25.0f * std::clamp(detritusScale, 0.0f, 1.0f);
    }

    // Update decomposer node
    if (m_decomposerNodeIdx >= 0 && m_decomposerNodeIdx < static_cast<int>(m_nodes.size())) {
        auto& node = m_nodes[m_decomposerNodeIdx];
        node.biomass = decomposerActivity * 100.0f;
        float decompScale = std::log(1.0f + node.biomass) / std::log(300.0f);
        node.radius = 10.0f + 25.0f * std::clamp(decompScale, 0.0f, 1.0f);

        // Fungal burst: brighten decomposer during fall mushroom season
        if (bloomType == 2) {  // Fungal burst
            node.color = glm::vec3(0.85f, 0.6f, 0.7f);  // Brighter purple
        } else {
            node.color = glm::vec3(0.7f, 0.5f, 0.6f);   // Normal
        }
    }

    // Update plankton node
    if (m_planktonNodeIdx >= 0 && m_planktonNodeIdx < static_cast<int>(m_nodes.size())) {
        auto& node = m_nodes[m_planktonNodeIdx];
        node.biomass = planktonLevel * 100.0f;
        float planktonScale = std::log(1.0f + node.biomass) / std::log(200.0f);
        node.radius = 8.0f + 20.0f * std::clamp(planktonScale, 0.0f, 1.0f);

        // Plankton bloom: brighten during winter plankton bloom
        if (bloomType == 3) {  // Plankton bloom
            node.color = glm::vec3(0.4f, 0.75f, 0.95f);  // Brighter blue
        } else {
            node.color = glm::vec3(0.3f, 0.6f, 0.8f);    // Normal
        }
    }

    // Update nutrient node (based on decomposer activity feeding back)
    if (m_nutrientNodeIdx >= 0 && m_nutrientNodeIdx < static_cast<int>(m_nodes.size())) {
        auto& node = m_nodes[m_nutrientNodeIdx];
        // Nutrients increase when decomposers are active
        node.biomass = 50.0f + decomposerActivity * 50.0f + detritusLevel * 30.0f;
        float nutrientScale = std::log(1.0f + node.biomass) / std::log(200.0f);
        node.radius = 10.0f + 20.0f * std::clamp(nutrientScale, 0.0f, 1.0f);
    }

    // Store bloom state for rendering
    m_currentBloomMult = bloomMultiplier;
    m_currentBloomType = bloomType;
}

// ============================================================================
// Layout
// ============================================================================

void FoodWebViz::layoutTrophicLevels(const ImVec2& canvasSize) {
    // Group nodes by trophic level
    std::map<int, std::vector<int>> levelNodes;
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        levelNodes[m_nodes[i].trophicLevel].push_back(static_cast<int>(i));
    }

    // Position nodes in layers
    float ySpacing = canvasSize.y / (levelNodes.size() + 1);
    int levelIdx = 0;

    for (auto& [level, indices] : levelNodes) {
        float y = 1.0f - (levelIdx + 1) * (1.0f / (levelNodes.size() + 1));
        float xSpacing = 1.0f / (indices.size() + 1);

        for (size_t i = 0; i < indices.size(); ++i) {
            int nodeIdx = indices[i];
            m_nodes[nodeIdx].position.x = (i + 1) * xSpacing;
            m_nodes[nodeIdx].position.y = y;
        }

        levelIdx++;
    }
}

void FoodWebViz::layoutForceDirected(float deltaTime) {
    applyForces();

    // Update positions based on velocity
    for (auto& node : m_nodes) {
        node.position += node.velocity * deltaTime;

        // Clamp to bounds
        node.position.x = std::clamp(node.position.x, 0.05f, 0.95f);
        node.position.y = std::clamp(node.position.y, 0.05f, 0.95f);

        // Apply damping
        node.velocity *= 0.9f;
    }
}

void FoodWebViz::applyForces() {
    // Repulsion between all nodes
    float repulsionStrength = 0.01f;
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        for (size_t j = i + 1; j < m_nodes.size(); ++j) {
            glm::vec2 diff = m_nodes[i].position - m_nodes[j].position;
            float dist = glm::length(diff);
            if (dist < 0.01f) dist = 0.01f;

            glm::vec2 force = (diff / dist) * repulsionStrength / (dist * dist);
            m_nodes[i].velocity += force;
            m_nodes[j].velocity -= force;
        }
    }

    // Attraction along edges
    float attractionStrength = 0.001f;
    for (const auto& edge : m_edges) {
        if (edge.sourceIdx < 0 || edge.targetIdx < 0) continue;

        glm::vec2 diff = m_nodes[edge.targetIdx].position - m_nodes[edge.sourceIdx].position;
        float dist = glm::length(diff);

        glm::vec2 force = diff * attractionStrength;
        m_nodes[edge.sourceIdx].velocity += force;
        m_nodes[edge.targetIdx].velocity -= force;
    }

    // Trophic level constraint (vertical positioning)
    float levelForce = 0.005f;
    for (auto& node : m_nodes) {
        float targetY = 1.0f - (node.trophicLevel / 5.0f);
        node.velocity.y += (targetY - node.position.y) * levelForce;
    }
}

// ============================================================================
// Rendering
// ============================================================================

void FoodWebViz::render(const ImVec2& canvasSize) {
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background
    drawList->AddRectFilled(canvasPos,
                           ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                           IM_COL32(25, 28, 32, 255));

    // Layout if needed
    static bool firstFrame = true;
    if (firstFrame) {
        layoutTrophicLevels(canvasSize);
        firstFrame = false;
    }

    if (m_useForceLayout) {
        layoutForceDirected(0.016f);
    }

    // Draw trophic level labels (now including level 0 for nutrients/detritus)
    const char* levelLabels[] = {"Nutrients/Detritus", "Producers/Decomposers", "Primary Consumers", "Secondary Consumers", "Apex Predators"};
    for (int level = 0; level <= 4; ++level) {
        float y = canvasPos.y + canvasSize.y * (1.0f - level / 5.0f);
        drawList->AddText(ImVec2(canvasPos.x + 5, y - 8), IM_COL32(100, 100, 100, 150), levelLabels[level]);

        // Horizontal guide line
        drawList->AddLine(ImVec2(canvasPos.x, y), ImVec2(canvasPos.x + canvasSize.x, y),
                         IM_COL32(50, 50, 50, 100), 1.0f);
    }

    // Show bloom indicator if active
    if (m_currentBloomMult > 1.1f) {
        const char* bloomNames[] = {"", "Spring Bloom", "Fungal Burst", "Plankton Bloom"};
        const char* bloomName = (m_currentBloomType >= 0 && m_currentBloomType < 4)
            ? bloomNames[m_currentBloomType] : "";
        char bloomText[64];
        snprintf(bloomText, sizeof(bloomText), "%s (x%.1f)", bloomName, m_currentBloomMult);
        drawList->AddText(ImVec2(canvasPos.x + canvasSize.x - 150, canvasPos.y + 5),
                         IM_COL32(100, 200, 100, 255), bloomText);
    }

    // Draw edges first (below nodes)
    for (const auto& edge : m_edges) {
        renderEdge(drawList, edge, canvasPos, canvasSize);
    }

    // Draw nodes
    m_hoveredNode = -1;
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        renderNode(drawList, m_nodes[i], canvasPos, canvasSize);
        if (m_nodes[i].isHovered) {
            m_hoveredNode = static_cast<int>(i);
        }
    }

    // Handle input
    ImGui::InvisibleButton("FoodWebCanvas", canvasSize);
    handleInput(canvasPos, canvasSize);

    // Tooltip
    if (m_hoveredNode >= 0) {
        renderTooltip(m_nodes[m_hoveredNode]);
    }
}

void FoodWebViz::renderNode(ImDrawList* drawList, const FoodWebNode& node,
                            const ImVec2& canvasPos, const ImVec2& canvasSize) {
    ImVec2 screenPos = worldToScreen(node.position, canvasPos, canvasSize);
    float radius = node.radius * m_zoom;

    // Check hover
    ImVec2 mousePos = ImGui::GetMousePos();
    float dist = std::sqrt(std::pow(mousePos.x - screenPos.x, 2) +
                          std::pow(mousePos.y - screenPos.y, 2));
    const_cast<FoodWebNode&>(node).isHovered = dist < radius + 5.0f;

    // Determine alpha based on population (creatures) or biomass (resources)
    float alpha = 1.0f;
    if (node.nodeType == FoodWebNodeType::CREATURE) {
        alpha = node.population > 0 ? 1.0f : 0.3f;
    } else {
        alpha = node.biomass > 1.0f ? 1.0f : 0.5f;
    }

    // Node color
    ImU32 color = IM_COL32(
        static_cast<int>(node.color.r * 255),
        static_cast<int>(node.color.g * 255),
        static_cast<int>(node.color.b * 255),
        static_cast<int>(255 * alpha)
    );

    // Draw filled circle (resources get different shapes)
    if (node.nodeType == FoodWebNodeType::CREATURE) {
        drawList->AddCircleFilled(screenPos, radius, color);
    } else if (node.nodeType == FoodWebNodeType::NUTRIENT || node.nodeType == FoodWebNodeType::DETRITUS) {
        // Square shape for abiotic/detritus
        drawList->AddRectFilled(
            ImVec2(screenPos.x - radius, screenPos.y - radius),
            ImVec2(screenPos.x + radius, screenPos.y + radius),
            color, radius * 0.3f);  // Rounded corners
    } else {
        // Diamond shape for producers/decomposers
        ImVec2 points[4] = {
            ImVec2(screenPos.x, screenPos.y - radius),
            ImVec2(screenPos.x + radius, screenPos.y),
            ImVec2(screenPos.x, screenPos.y + radius),
            ImVec2(screenPos.x - radius, screenPos.y)
        };
        drawList->AddConvexPolyFilled(points, 4, color);
    }

    // Outline (brighter during bloom for resource nodes)
    ImU32 outlineColor;
    if (node.isHovered) {
        outlineColor = IM_COL32(255, 255, 200, 255);
    } else if (node.nodeType != FoodWebNodeType::CREATURE && node.bloomMultiplier > 1.1f) {
        // Glowing outline during bloom
        outlineColor = IM_COL32(255, 255, 150, 200);
    } else {
        outlineColor = IM_COL32(255, 255, 255, static_cast<int>(100 * alpha));
    }

    if (node.nodeType == FoodWebNodeType::CREATURE) {
        drawList->AddCircle(screenPos, radius, outlineColor, 0, 2.0f);
    } else if (node.nodeType == FoodWebNodeType::NUTRIENT || node.nodeType == FoodWebNodeType::DETRITUS) {
        drawList->AddRect(
            ImVec2(screenPos.x - radius, screenPos.y - radius),
            ImVec2(screenPos.x + radius, screenPos.y + radius),
            outlineColor, radius * 0.3f, 0, 2.0f);
    } else {
        ImVec2 points[4] = {
            ImVec2(screenPos.x, screenPos.y - radius),
            ImVec2(screenPos.x + radius, screenPos.y),
            ImVec2(screenPos.x, screenPos.y + radius),
            ImVec2(screenPos.x - radius, screenPos.y)
        };
        for (int i = 0; i < 4; ++i) {
            drawList->AddLine(points[i], points[(i + 1) % 4], outlineColor, 2.0f);
        }
    }

    // Label
    if (m_showLabels) {
        char label[64];
        if (node.nodeType == FoodWebNodeType::CREATURE) {
            if (node.population > 0) {
                snprintf(label, sizeof(label), "%s (%d)", node.name.c_str(), node.population);
            } else {
                return;  // Don't label creatures with 0 population
            }
        } else {
            // Show biomass for resource nodes
            snprintf(label, sizeof(label), "%s (%.0f)", node.name.c_str(), node.biomass);
        }

        ImVec2 textSize = ImGui::CalcTextSize(label);
        ImVec2 textPos(screenPos.x - textSize.x / 2.0f, screenPos.y + radius + 4.0f);
        drawList->AddText(textPos, IM_COL32(200, 200, 200, static_cast<int>(255 * alpha)), label);
    }
}

void FoodWebViz::renderEdge(ImDrawList* drawList, const FoodWebEdge& edge,
                            const ImVec2& canvasPos, const ImVec2& canvasSize) {
    if (edge.sourceIdx < 0 || edge.targetIdx < 0) return;

    const FoodWebNode& source = m_nodes[edge.sourceIdx];
    const FoodWebNode& target = m_nodes[edge.targetIdx];

    // Skip edges with no population at either end
    if (source.population == 0 || target.population == 0) return;

    ImVec2 startPos = worldToScreen(source.position, canvasPos, canvasSize);
    ImVec2 endPos = worldToScreen(target.position, canvasPos, canvasSize);

    // Calculate direction for arrow
    glm::vec2 dir = glm::normalize(target.position - source.position);

    // Offset start/end by node radii
    startPos.x += dir.x * source.radius * m_zoom;
    startPos.y += dir.y * source.radius * m_zoom;
    endPos.x -= dir.x * target.radius * m_zoom;
    endPos.y -= dir.y * target.radius * m_zoom;

    // Edge thickness based on energy transfer
    float thickness = 1.0f + edge.energyTransfer * 0.5f;
    thickness = std::clamp(thickness, 1.0f, 5.0f);

    // Color gradient from source to target
    ImU32 colorStart = IM_COL32(
        static_cast<int>(source.color.r * 200),
        static_cast<int>(source.color.g * 200),
        static_cast<int>(source.color.b * 200),
        150
    );
    ImU32 colorEnd = IM_COL32(
        static_cast<int>(target.color.r * 200),
        static_cast<int>(target.color.g * 200),
        static_cast<int>(target.color.b * 200),
        150
    );

    // Draw line
    drawList->AddLine(startPos, endPos, colorStart, thickness);

    // Draw arrowhead
    float arrowSize = 8.0f;
    glm::vec2 arrowDir(-dir.y, dir.x);  // Perpendicular

    ImVec2 arrow1(endPos.x - dir.x * arrowSize + arrowDir.x * arrowSize * 0.5f,
                  endPos.y - dir.y * arrowSize + arrowDir.y * arrowSize * 0.5f);
    ImVec2 arrow2(endPos.x - dir.x * arrowSize - arrowDir.x * arrowSize * 0.5f,
                  endPos.y - dir.y * arrowSize - arrowDir.y * arrowSize * 0.5f);

    drawList->AddTriangleFilled(endPos, arrow1, arrow2, colorEnd);

    // Energy flow label
    if (m_showEnergyFlow && edge.energyTransfer > 0.1f) {
        ImVec2 midPoint((startPos.x + endPos.x) / 2.0f, (startPos.y + endPos.y) / 2.0f);
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f", edge.energyTransfer);
        drawList->AddText(midPoint, IM_COL32(200, 200, 100, 200), buf);
    }
}

void FoodWebViz::renderTooltip(const FoodWebNode& node) {
    ImGui::BeginTooltip();
    ImGui::TextColored(ImVec4(node.color.r, node.color.g, node.color.b, 1.0f),
                       "%s", node.name.c_str());
    ImGui::Separator();

    if (node.nodeType == FoodWebNodeType::CREATURE) {
        ImGui::Text("Population: %d", node.population);
        ImGui::Text("Trophic Level: %d", node.trophicLevel);
        ImGui::Text("Biomass: %.1f", node.biomass);
    } else {
        ImGui::Text("Biomass: %.1f", node.biomass);
        ImGui::Text("Trophic Level: %d", node.trophicLevel);

        // Show node type
        const char* typeNames[] = {"Creature", "Producer", "Detritus", "Plankton", "Decomposer", "Nutrient"};
        int typeIdx = static_cast<int>(node.nodeType);
        if (typeIdx >= 0 && typeIdx < 6) {
            ImGui::Text("Type: %s", typeNames[typeIdx]);
        }

        // Show bloom indicator if applicable
        if (node.bloomMultiplier > 1.1f) {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "BLOOMING (x%.1f)", node.bloomMultiplier);
        }
    }

    // Find this node's index
    int thisIdx = -1;
    if (node.nodeType == FoodWebNodeType::CREATURE) {
        auto it = m_nodeIndices.find(node.type);
        if (it != m_nodeIndices.end()) {
            thisIdx = it->second;
        }
    } else {
        // Find by matching pointer (resource nodes)
        for (size_t i = 0; i < m_nodes.size(); ++i) {
            if (&m_nodes[i] == &node) {
                thisIdx = static_cast<int>(i);
                break;
            }
        }
    }

    if (thisIdx >= 0) {
        // List consumers (what eats this)
        ImGui::Separator();
        ImGui::Text("Consumed by:");
        bool hasConsumers = false;
        for (const auto& edge : m_edges) {
            if (edge.sourceIdx == thisIdx) {
                ImGui::BulletText("%s (%.0f%%)", m_nodes[edge.targetIdx].name.c_str(),
                                 edge.efficiency * 100.0f);
                hasConsumers = true;
            }
        }
        if (!hasConsumers) ImGui::TextDisabled("  (none)");

        // List food sources (what this consumes)
        ImGui::Text("Consumes:");
        bool hasFood = false;
        for (const auto& edge : m_edges) {
            if (edge.targetIdx == thisIdx) {
                ImGui::BulletText("%s", m_nodes[edge.sourceIdx].name.c_str());
                hasFood = true;
            }
        }
        if (!hasFood) ImGui::TextDisabled("  (none)");
    }

    ImGui::EndTooltip();
}

void FoodWebViz::renderPyramid(const ImVec2& canvasSize) {
    // Ecological pyramid view
    // Group by trophic level and show biomass as bar width

    std::map<int, float> levelBiomass;
    std::map<int, int> levelCount;

    for (const auto& node : m_nodes) {
        levelBiomass[node.trophicLevel] += node.biomass;
        levelCount[node.trophicLevel] += node.population;
    }

    // Render as horizontal bars
    ImGui::Text("Ecological Pyramid");

    float maxBiomass = 0.0f;
    for (const auto& [level, biomass] : levelBiomass) {
        maxBiomass = std::max(maxBiomass, biomass);
    }

    if (maxBiomass < 0.01f) maxBiomass = 1.0f;

    const char* levelNames[] = {"", "Producers", "Herbivores", "Predators", "Apex"};
    ImVec4 levelColors[] = {
        ImVec4(0, 0, 0, 0),
        ImVec4(0.2f, 0.8f, 0.2f, 1.0f),  // Green
        ImVec4(0.8f, 0.8f, 0.2f, 1.0f),  // Yellow
        ImVec4(0.8f, 0.5f, 0.2f, 1.0f),  // Orange
        ImVec4(0.8f, 0.2f, 0.2f, 1.0f),  // Red
    };

    for (int level = 4; level >= 2; --level) {
        float biomass = levelBiomass[level];
        float fraction = biomass / maxBiomass;
        int count = levelCount[level];

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, levelColors[level]);

        char overlay[64];
        snprintf(overlay, sizeof(overlay), "%s: %d (%.0f)", levelNames[level], count, biomass);

        // Center the bar
        float indent = (1.0f - fraction) * 0.5f * canvasSize.x * 0.8f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);
        ImGui::ProgressBar(fraction, ImVec2(canvasSize.x * 0.8f * fraction, 25), overlay);

        ImGui::PopStyleColor();
    }
}

void FoodWebViz::renderEnergyFlow() {
    // Energy flow statistics
    ImGui::Text("Energy Flow Statistics");
    ImGui::Separator();

    // Calculate total energy at each level
    std::map<int, float> levelEnergy;
    for (const auto& node : m_nodes) {
        levelEnergy[node.trophicLevel] += node.population * 10.0f;  // Simplified energy estimate
    }

    // Show transfer efficiencies
    for (int level = 2; level <= 4; ++level) {
        float current = levelEnergy[level];
        float previous = levelEnergy[level - 1];
        float efficiency = previous > 0 ? (current / previous) * 100.0f : 0.0f;

        ImGui::Text("Level %d -> %d: %.1f%% efficiency", level - 1, level, efficiency);
    }
}

// ============================================================================
// Coordinate Transformation
// ============================================================================

ImVec2 FoodWebViz::worldToScreen(const glm::vec2& world, const ImVec2& canvasPos,
                                  const ImVec2& canvasSize) const {
    float x = canvasPos.x + world.x * canvasSize.x;
    float y = canvasPos.y + (1.0f - world.y) * canvasSize.y;  // Flip Y
    return ImVec2(x, y);
}

// ============================================================================
// Input Handling
// ============================================================================

void FoodWebViz::handleInput(const ImVec2& canvasPos, const ImVec2& canvasSize) {
    ImGuiIO& io = ImGui::GetIO();

    // Zoom with scroll
    if (std::abs(io.MouseWheel) > 0.0f) {
        m_zoom = std::clamp(m_zoom + io.MouseWheel * 0.1f, 0.5f, 3.0f);
    }

    // Selection on click
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && m_hoveredNode >= 0) {
        m_selectedNode = m_hoveredNode;
    }
}

} // namespace statistics
} // namespace ui
