#include "PhylogeneticTreeVisualizer.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <climits>

namespace ui {

// ============================================================================
// PhylogeneticTreeVisualizer Implementation
// ============================================================================

PhylogeneticTreeVisualizer::PhylogeneticTreeVisualizer() = default;
PhylogeneticTreeVisualizer::~PhylogeneticTreeVisualizer() = default;

void PhylogeneticTreeVisualizer::buildFromTracker(const genetics::SpeciationTracker& tracker) {
    m_nodes.clear();
    m_speciesNodeMap.clear();
    m_root = nullptr;

    const auto& tree = tracker.getPhylogeneticTree();

    // Get all species and build nodes
    auto activeSpecies = tracker.getActiveSpecies();
    auto extinctSpecies = tracker.getExtinctSpecies();

    std::vector<const genetics::Species*> allSpecies;
    allSpecies.insert(allSpecies.end(), activeSpecies.begin(), activeSpecies.end());
    allSpecies.insert(allSpecies.end(), extinctSpecies.begin(), extinctSpecies.end());

    if (allSpecies.empty()) return;

    // Create nodes for each species
    for (const genetics::Species* sp : allSpecies) {
        auto node = std::make_unique<TreeRenderNode>();
        node->speciesId = sp->getId();
        node->name = sp->getName();
        node->population = sp->getStats().size;
        node->generation = sp->getFoundingGeneration();
        node->isExtinct = sp->isExtinct();
        node->color = sp->getColor();
        node->clusterColor = sp->getColor();  // Will be updated by setSimilaritySystem
        node->clusterId = -1;
        node->divergenceScore = 0.0f;
        node->isFiltered = true;

        // Scale radius by population (log scale for better visualization)
        float popFactor = sp->getStats().size > 0 ?
            std::log2(static_cast<float>(sp->getStats().size) + 1.0f) / 8.0f : 0.0f;
        node->radius = glm::mix(m_minNodeRadius, m_maxNodeRadius, glm::clamp(popFactor, 0.0f, 1.0f));

        // Gray out extinct species (base color only, cluster color may differ)
        if (node->isExtinct) {
            node->color = glm::vec3(0.5f);
            node->radius = m_minNodeRadius;
        }

        m_speciesNodeMap[sp->getId()] = node.get();
        m_nodes.push_back(std::move(node));
    }

    // Build parent-child relationships from phylogenetic tree
    for (const genetics::Species* sp : allSpecies) {
        const genetics::PhyloNode* phyloNode = tree.getNodeForSpecies(sp->getId());
        if (!phyloNode) continue;

        TreeRenderNode* renderNode = m_speciesNodeMap[sp->getId()];
        if (!renderNode) continue;

        // Find parent
        if (phyloNode->parentId != 0) {
            const genetics::PhyloNode* parentPhylo = tree.getNode(phyloNode->parentId);
            if (parentPhylo) {
                auto parentIt = m_speciesNodeMap.find(parentPhylo->speciesId);
                if (parentIt != m_speciesNodeMap.end()) {
                    renderNode->parent = parentIt->second;
                    parentIt->second->children.push_back(renderNode);
                }
            }
        }
    }

    // Find root (node with no parent)
    for (auto& node : m_nodes) {
        if (node->parent == nullptr) {
            m_root = node.get();
            break;
        }
    }

    // Layout the tree
    layoutTree();

    // Apply current filter
    applyFilter();
}

void PhylogeneticTreeVisualizer::setSimilaritySystem(const genetics::SpeciesSimilaritySystem* similarity) {
    m_similarity = similarity;
    updateNodeColors();
}

void PhylogeneticTreeVisualizer::updateNodeColors() {
    if (m_nodes.empty()) return;

    for (auto& node : m_nodes) {
        // Update cluster information if similarity system available
        if (m_similarity) {
            node->clusterId = m_similarity->getClusterId(node->speciesId);
            node->clusterColor = m_similarity->getClusterColor(node->speciesId);

            // Compute divergence from cluster centroid
            const auto* fv = m_similarity->getFeatureVector(node->speciesId);
            const auto* cluster = m_similarity->getCluster(node->clusterId);
            if (fv && cluster) {
                node->divergenceScore = fv->distanceTo(cluster->centroid,
                    genetics::SpeciesFeatureVector::getDefaultWeights());
            }
        }

        // Choose display color based on color mode
        glm::vec3 displayColor;
        switch (m_colorMode) {
            case ColorMode::SPECIES_ID:
                displayColor = node->color;
                break;

            case ColorMode::CLUSTER:
                displayColor = (node->clusterId >= 0) ? node->clusterColor : node->color;
                break;

            case ColorMode::FITNESS:
                // Green to red gradient based on assumed fitness (use divergence as proxy)
                {
                    float t = glm::clamp(node->divergenceScore / 0.5f, 0.0f, 1.0f);
                    displayColor = glm::mix(glm::vec3(0.2f, 0.8f, 0.2f),  // Green = fit
                                            glm::vec3(0.8f, 0.2f, 0.2f),  // Red = less fit
                                            t);
                }
                break;

            case ColorMode::AGE:
                // Blue (old) to yellow (new) gradient
                {
                    // Normalize generation to [0, 1] range (assuming max ~1000 generations)
                    float t = glm::clamp(static_cast<float>(node->generation) / 500.0f, 0.0f, 1.0f);
                    displayColor = glm::mix(glm::vec3(0.2f, 0.4f, 0.9f),  // Blue = old
                                            glm::vec3(0.9f, 0.9f, 0.2f),  // Yellow = new
                                            t);
                }
                break;
        }

        // Apply extinction dimming
        if (node->isExtinct && m_colorMode != ColorMode::SPECIES_ID) {
            displayColor = glm::mix(displayColor, glm::vec3(0.5f), 0.6f);
        }

        // Store final display color
        if (m_colorMode == ColorMode::CLUSTER) {
            node->clusterColor = displayColor;
        } else {
            node->color = displayColor;
        }
    }
}

void PhylogeneticTreeVisualizer::setFilter(const TreeFilterOptions& filter) {
    m_filter = filter;
    applyFilter();
}

void PhylogeneticTreeVisualizer::applyFilter() {
    for (auto& node : m_nodes) {
        node->isFiltered = nodePassesFilter(node.get());
    }
}

bool PhylogeneticTreeVisualizer::nodePassesFilter(const TreeRenderNode* node) const {
    // Extinction filter
    if (node->isExtinct && !m_filter.showExtinct) return false;
    if (!node->isExtinct && !m_filter.showExtant) return false;

    // Cluster filter
    if (m_filter.filterByClusterId >= 0 && node->clusterId != m_filter.filterByClusterId) {
        return false;
    }

    // Name filter (case-insensitive substring)
    if (!m_filter.nameFilter.empty()) {
        std::string nodeName = node->name;
        std::string filterText = m_filter.nameFilter;

        // Convert both to lowercase for case-insensitive comparison
        std::transform(nodeName.begin(), nodeName.end(), nodeName.begin(), ::tolower);
        std::transform(filterText.begin(), filterText.end(), filterText.begin(), ::tolower);

        if (nodeName.find(filterText) == std::string::npos) {
            return false;
        }
    }

    // Population filter
    if (node->population < m_filter.minPopulation) return false;

    // Generation filter
    if (node->generation > m_filter.maxGeneration) return false;

    return true;
}

std::set<int> PhylogeneticTreeVisualizer::getAvailableClusterIds() const {
    std::set<int> clusters;
    for (const auto& node : m_nodes) {
        if (node->clusterId >= 0) {
            clusters.insert(node->clusterId);
        }
    }
    return clusters;
}

std::vector<PhylogeneticTreeVisualizer::ClusterInfo> PhylogeneticTreeVisualizer::getClusterInfo() const {
    std::map<int, ClusterInfo> clusterMap;

    for (const auto& node : m_nodes) {
        if (node->clusterId >= 0) {
            auto& info = clusterMap[node->clusterId];
            info.clusterId = node->clusterId;
            info.color = node->clusterColor;
            info.memberCount++;
        }
    }

    std::vector<ClusterInfo> result;
    for (const auto& [id, info] : clusterMap) {
        result.push_back(info);
    }

    // Sort by cluster ID
    std::sort(result.begin(), result.end(),
              [](const ClusterInfo& a, const ClusterInfo& b) {
                  return a.clusterId < b.clusterId;
              });

    return result;
}

int PhylogeneticTreeVisualizer::countFilteredLeaves(TreeRenderNode* node) {
    if (!node->isFiltered) return 0;

    if (node->children.empty()) return 1;

    int count = 0;
    for (TreeRenderNode* child : node->children) {
        count += countFilteredLeaves(child);
    }
    return count > 0 ? count : 1;  // At least 1 for filtered internal nodes
}

void PhylogeneticTreeVisualizer::layoutTree() {
    if (!m_root) return;

    switch (m_layoutStyle) {
        case LayoutStyle::VERTICAL:
            layoutVertical(m_root, 0.0f, 800.0f, 50.0f, 0);
            break;
        case LayoutStyle::HORIZONTAL:
            layoutHorizontal(m_root, 0.0f, 600.0f, 50.0f, 0);
            break;
        case LayoutStyle::RADIAL:
            layoutRadial(m_root, 0.0f, 2.0f * 3.14159f, 50.0f, 0);
            break;
    }
}

int PhylogeneticTreeVisualizer::countLeaves(TreeRenderNode* node) {
    if (node->children.empty()) return 1;

    int count = 0;
    for (TreeRenderNode* child : node->children) {
        count += countLeaves(child);
    }
    return count;
}

void PhylogeneticTreeVisualizer::layoutVertical(TreeRenderNode* node, float xMin, float xMax, float y, int depth) {
    // Position this node at center of its x range
    node->position.x = (xMin + xMax) / 2.0f;
    node->position.y = y;

    if (node->children.empty()) return;

    // Distribute children across x range
    int totalLeaves = 0;
    std::vector<int> childLeaves;
    for (TreeRenderNode* child : node->children) {
        int leaves = countLeaves(child);
        childLeaves.push_back(leaves);
        totalLeaves += leaves;
    }

    float currentX = xMin;
    float width = xMax - xMin;

    for (size_t i = 0; i < node->children.size(); i++) {
        float childWidth = width * (static_cast<float>(childLeaves[i]) / totalLeaves);
        layoutVertical(node->children[i], currentX, currentX + childWidth, y + m_levelSpacing, depth + 1);
        currentX += childWidth;
    }
}

void PhylogeneticTreeVisualizer::layoutHorizontal(TreeRenderNode* node, float yMin, float yMax, float x, int depth) {
    node->position.x = x;
    node->position.y = (yMin + yMax) / 2.0f;

    if (node->children.empty()) return;

    int totalLeaves = 0;
    std::vector<int> childLeaves;
    for (TreeRenderNode* child : node->children) {
        int leaves = countLeaves(child);
        childLeaves.push_back(leaves);
        totalLeaves += leaves;
    }

    float currentY = yMin;
    float height = yMax - yMin;

    for (size_t i = 0; i < node->children.size(); i++) {
        float childHeight = height * (static_cast<float>(childLeaves[i]) / totalLeaves);
        layoutHorizontal(node->children[i], currentY, currentY + childHeight, x + m_levelSpacing, depth + 1);
        currentY += childHeight;
    }
}

void PhylogeneticTreeVisualizer::layoutRadial(TreeRenderNode* node, float angleStart, float angleEnd, float radius, int depth) {
    float angleMid = (angleStart + angleEnd) / 2.0f;
    node->position.x = 400.0f + radius * std::cos(angleMid);
    node->position.y = 300.0f + radius * std::sin(angleMid);

    if (node->children.empty()) return;

    int totalLeaves = 0;
    std::vector<int> childLeaves;
    for (TreeRenderNode* child : node->children) {
        int leaves = countLeaves(child);
        childLeaves.push_back(leaves);
        totalLeaves += leaves;
    }

    float currentAngle = angleStart;
    float angleRange = angleEnd - angleStart;

    for (size_t i = 0; i < node->children.size(); i++) {
        float childAngle = angleRange * (static_cast<float>(childLeaves[i]) / totalLeaves);
        layoutRadial(node->children[i], currentAngle, currentAngle + childAngle, radius + m_levelSpacing, depth + 1);
        currentAngle += childAngle;
    }
}

void PhylogeneticTreeVisualizer::render(const ImVec2& canvasSize) {
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();

    // Create canvas
    ImGui::InvisibleButton("##TreeCanvas", canvasSize,
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

    bool isHovered = ImGui::IsItemHovered();

    // Handle interaction
    handleInput(canvasPos, canvasSize);

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Draw background
    drawList->AddRectFilled(canvasPos,
                            ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                            IM_COL32(20, 25, 30, 255));

    // Draw border
    drawList->AddRect(canvasPos,
                      ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                      IM_COL32(60, 70, 80, 255));

    if (!m_root) {
        // Draw "No data" message
        const char* msg = "No species data";
        ImVec2 textSize = ImGui::CalcTextSize(msg);
        drawList->AddText(
            ImVec2(canvasPos.x + (canvasSize.x - textSize.x) / 2,
                   canvasPos.y + (canvasSize.y - textSize.y) / 2),
            IM_COL32(128, 128, 128, 255), msg);
        return;
    }

    // Reset hover state
    m_hoveredSpeciesId = 0;

    // Draw branches first (behind nodes)
    for (auto& node : m_nodes) {
        for (TreeRenderNode* child : node->children) {
            renderBranch(drawList, node.get(), child, canvasPos);
        }
    }

    // Draw nodes
    for (auto& node : m_nodes) {
        renderNode(drawList, node.get(), canvasPos);
    }

    // Draw tooltip for hovered node
    for (auto& node : m_nodes) {
        if (node->isHovered) {
            renderTooltip(node.get());
            break;
        }
    }

    // Draw legend
    renderLegend(drawList, canvasPos, canvasSize);

    // Draw zoom level
    char zoomText[32];
    snprintf(zoomText, sizeof(zoomText), "Zoom: %.0f%%", m_zoom * 100);
    ImVec2 zoomTextSize = ImGui::CalcTextSize(zoomText);
    drawList->AddText(
        ImVec2(canvasPos.x + canvasSize.x - zoomTextSize.x - 10, canvasPos.y + 10),
        IM_COL32(128, 128, 128, 255), zoomText);
}

void PhylogeneticTreeVisualizer::renderNode(ImDrawList* drawList, TreeRenderNode* node, const ImVec2& canvasPos) {
    // Skip filtered out nodes (render dimmed)
    if (!node->isFiltered) {
        ImVec2 screenPos = worldToScreen(node->position, canvasPos);
        float radius = node->radius * m_zoom * 0.5f;  // Smaller when filtered
        drawList->AddCircleFilled(screenPos, radius, IM_COL32(80, 80, 80, 100));
        return;
    }

    ImVec2 screenPos = worldToScreen(node->position, canvasPos);
    float radius = node->radius * m_zoom;

    // Check if mouse is hovering this node
    ImVec2 mousePos = ImGui::GetMousePos();
    float distSq = (mousePos.x - screenPos.x) * (mousePos.x - screenPos.x) +
                   (mousePos.y - screenPos.y) * (mousePos.y - screenPos.y);
    node->isHovered = distSq <= radius * radius;

    if (node->isHovered) {
        m_hoveredSpeciesId = node->speciesId;
    }

    // Handle click selection
    if (node->isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        m_selectedSpeciesId = node->speciesId;
    }

    node->isSelected = (node->speciesId == m_selectedSpeciesId);

    // Determine display color based on color mode
    glm::vec3 displayColor;
    switch (m_colorMode) {
        case ColorMode::CLUSTER:
            displayColor = node->clusterColor;
            break;
        default:
            displayColor = node->color;
            break;
    }

    // Determine colors
    ImU32 fillColor;
    ImU32 borderColor;

    if (node->isExtinct) {
        // Dimmed version of display color for extinct
        glm::vec3 extinctColor = glm::mix(displayColor, glm::vec3(0.5f), 0.5f);
        fillColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
            extinctColor.r, extinctColor.g, extinctColor.b, 0.7f));
        borderColor = IM_COL32(100, 100, 100, 200);
    } else {
        fillColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
            displayColor.r, displayColor.g, displayColor.b, 1.0f));

        if (node->isHovered) {
            borderColor = IM_COL32(255, 255, 255, 255);
            radius *= 1.2f;  // Enlarge on hover
        } else if (node->isSelected) {
            borderColor = IM_COL32(255, 220, 100, 255);
        } else {
            borderColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
                displayColor.r * 0.7f, displayColor.g * 0.7f, displayColor.b * 0.7f, 1.0f));
        }
    }

    // Draw node
    drawList->AddCircleFilled(screenPos, radius, fillColor);
    drawList->AddCircle(screenPos, radius, borderColor, 0, node->isSelected ? 3.0f : 1.5f);

    // Draw cluster indicator ring for cluster mode
    if (m_colorMode == ColorMode::CLUSTER && node->clusterId >= 0 && !node->isExtinct) {
        float outerRadius = radius * 1.15f;
        drawList->AddCircle(screenPos, outerRadius,
                            ImGui::ColorConvertFloat4ToU32(ImVec4(
                                node->clusterColor.r, node->clusterColor.g, node->clusterColor.b, 0.5f)),
                            0, 2.0f);
    }

    // Draw population indicator (small inner circle)
    if (node->population > 0 && !node->isExtinct) {
        float innerRadius = radius * 0.3f;
        drawList->AddCircleFilled(screenPos, innerRadius, IM_COL32(255, 255, 255, 100));
    }
}

void PhylogeneticTreeVisualizer::renderBranch(ImDrawList* drawList, TreeRenderNode* parent, TreeRenderNode* child, const ImVec2& canvasPos) {
    ImVec2 parentPos = worldToScreen(parent->position, canvasPos);
    ImVec2 childPos = worldToScreen(child->position, canvasPos);

    ImU32 branchColor;
    if (child->isExtinct) {
        branchColor = IM_COL32(100, 100, 100, 150);
    } else {
        branchColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
            child->color.r * 0.6f, child->color.g * 0.6f, child->color.b * 0.6f, 0.8f));
    }

    float thickness = child->isExtinct ? 1.0f : 2.0f;

    // Draw elbow connector (L-shaped branch)
    switch (m_layoutStyle) {
        case LayoutStyle::VERTICAL:
            drawList->AddLine(parentPos, ImVec2(parentPos.x, childPos.y), branchColor, thickness);
            drawList->AddLine(ImVec2(parentPos.x, childPos.y), childPos, branchColor, thickness);
            break;
        case LayoutStyle::HORIZONTAL:
            drawList->AddLine(parentPos, ImVec2(childPos.x, parentPos.y), branchColor, thickness);
            drawList->AddLine(ImVec2(childPos.x, parentPos.y), childPos, branchColor, thickness);
            break;
        case LayoutStyle::RADIAL:
            // Direct line for radial layout
            drawList->AddLine(parentPos, childPos, branchColor, thickness);
            break;
    }
}

void PhylogeneticTreeVisualizer::renderTooltip(TreeRenderNode* node) {
    ImGui::BeginTooltip();

    // Species name with color indicator (use cluster color if in cluster mode)
    glm::vec3 displayColor = (m_colorMode == ColorMode::CLUSTER) ? node->clusterColor : node->color;
    ImGui::TextColored(ImVec4(displayColor.r, displayColor.g, displayColor.b, 1.0f),
                       "%s", node->name.c_str());

    ImGui::Separator();

    ImGui::Text("Species ID: %u", node->speciesId);
    ImGui::Text("Population: %d", node->population);
    ImGui::Text("Founded: Gen %d", node->generation);

    // Cluster information
    if (node->clusterId >= 0) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(node->clusterColor.r, node->clusterColor.g, node->clusterColor.b, 1.0f),
                           "Cluster: %d", node->clusterId);

        // Show divergence score
        if (node->divergenceScore > 0.0f) {
            ImGui::Text("Divergence: %.3f", node->divergenceScore);
        }

        // Show related species count
        if (m_similarity) {
            auto related = m_similarity->getRelatedSpecies(node->speciesId);
            if (!related.empty()) {
                ImGui::Text("Related species: %zu", related.size());
            }
        }
    }

    if (node->isExtinct) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "EXTINCT");
    }

    if (!node->children.empty()) {
        ImGui::Text("Child species: %zu", node->children.size());
    }

    // Hint for interaction
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Click to select");

    ImGui::EndTooltip();
}

void PhylogeneticTreeVisualizer::renderLegend(ImDrawList* drawList, const ImVec2& canvasPos, const ImVec2& canvasSize) {
    float legendY = canvasPos.y + 10;
    float legendX = canvasPos.x + 10;

    // Title based on color mode
    const char* modeLabel = "Species Colors";
    switch (m_colorMode) {
        case ColorMode::SPECIES_ID: modeLabel = "By Species ID"; break;
        case ColorMode::CLUSTER: modeLabel = "By Similarity Cluster"; break;
        case ColorMode::FITNESS: modeLabel = "By Fitness"; break;
        case ColorMode::AGE: modeLabel = "By Age"; break;
    }
    drawList->AddText(ImVec2(legendX, legendY), IM_COL32(200, 200, 200, 255), modeLabel);
    legendY += 20;

    if (m_colorMode == ColorMode::CLUSTER) {
        // Show cluster legend
        auto clusterInfos = getClusterInfo();
        int maxClusters = std::min(static_cast<int>(clusterInfos.size()), 8);  // Limit to 8 in legend

        for (int i = 0; i < maxClusters; ++i) {
            const auto& info = clusterInfos[i];
            drawList->AddCircleFilled(ImVec2(legendX + 5, legendY + 5), 5,
                ImGui::ColorConvertFloat4ToU32(ImVec4(info.color.r, info.color.g, info.color.b, 1.0f)));

            char label[32];
            snprintf(label, sizeof(label), "Cluster %d (%d)", info.clusterId, info.memberCount);
            drawList->AddText(ImVec2(legendX + 15, legendY), IM_COL32(180, 180, 180, 255), label);
            legendY += 15;
        }

        if (clusterInfos.size() > 8) {
            char moreLabel[32];
            snprintf(moreLabel, sizeof(moreLabel), "+%zu more...", clusterInfos.size() - 8);
            drawList->AddText(ImVec2(legendX + 15, legendY), IM_COL32(128, 128, 128, 255), moreLabel);
            legendY += 15;
        }
    } else if (m_colorMode == ColorMode::FITNESS) {
        // Gradient legend for fitness
        drawList->AddCircleFilled(ImVec2(legendX + 5, legendY + 5), 5, IM_COL32(51, 204, 51, 255));
        drawList->AddText(ImVec2(legendX + 15, legendY), IM_COL32(150, 150, 150, 255), "High Fitness");
        legendY += 15;
        drawList->AddCircleFilled(ImVec2(legendX + 5, legendY + 5), 5, IM_COL32(204, 51, 51, 255));
        drawList->AddText(ImVec2(legendX + 15, legendY), IM_COL32(150, 150, 150, 255), "Low Fitness");
        legendY += 15;
    } else if (m_colorMode == ColorMode::AGE) {
        // Gradient legend for age
        drawList->AddCircleFilled(ImVec2(legendX + 5, legendY + 5), 5, IM_COL32(51, 102, 230, 255));
        drawList->AddText(ImVec2(legendX + 15, legendY), IM_COL32(150, 150, 150, 255), "Old Species");
        legendY += 15;
        drawList->AddCircleFilled(ImVec2(legendX + 5, legendY + 5), 5, IM_COL32(230, 230, 51, 255));
        drawList->AddText(ImVec2(legendX + 15, legendY), IM_COL32(150, 150, 150, 255), "New Species");
        legendY += 15;
    }

    // Status legend (always show)
    legendY += 5;
    drawList->AddCircleFilled(ImVec2(legendX + 5, legendY + 5), 5, IM_COL32(100, 200, 100, 255));
    drawList->AddText(ImVec2(legendX + 15, legendY), IM_COL32(150, 150, 150, 255), "Active");
    legendY += 15;
    drawList->AddCircleFilled(ImVec2(legendX + 5, legendY + 5), 5, IM_COL32(128, 128, 128, 180));
    drawList->AddText(ImVec2(legendX + 15, legendY), IM_COL32(150, 150, 150, 255), "Extinct");
}

void PhylogeneticTreeVisualizer::renderFilterControls() {
    // This is called from SpeciesEvolutionPanel - provides filter UI
    // Left empty as controls are rendered in the panel
}

ImVec2 PhylogeneticTreeVisualizer::worldToScreen(const glm::vec2& world, const ImVec2& canvasPos) const {
    return ImVec2(
        canvasPos.x + (world.x + m_pan.x) * m_zoom,
        canvasPos.y + (world.y + m_pan.y) * m_zoom
    );
}

glm::vec2 PhylogeneticTreeVisualizer::screenToWorld(const ImVec2& screen, const ImVec2& canvasPos) const {
    return glm::vec2(
        (screen.x - canvasPos.x) / m_zoom - m_pan.x,
        (screen.y - canvasPos.y) / m_zoom - m_pan.y
    );
}

void PhylogeneticTreeVisualizer::handleInput(const ImVec2& canvasPos, const ImVec2& canvasSize) {
    ImGuiIO& io = ImGui::GetIO();

    // Check if mouse is in canvas
    ImVec2 mousePos = io.MousePos;
    bool inCanvas = mousePos.x >= canvasPos.x && mousePos.x <= canvasPos.x + canvasSize.x &&
                    mousePos.y >= canvasPos.y && mousePos.y <= canvasPos.y + canvasSize.y;

    if (!inCanvas) {
        m_isDragging = false;
        return;
    }

    // Zoom with scroll wheel
    if (io.MouseWheel != 0) {
        float zoomDelta = io.MouseWheel * 0.1f;
        m_zoom = glm::clamp(m_zoom + zoomDelta, 0.2f, 3.0f);
    }

    // Pan with right mouse button
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        m_isDragging = true;
        m_lastMousePos = glm::vec2(mousePos.x, mousePos.y);
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        m_isDragging = false;
    }

    if (m_isDragging) {
        glm::vec2 currentMousePos(mousePos.x, mousePos.y);
        glm::vec2 delta = currentMousePos - m_lastMousePos;
        m_pan += delta / m_zoom;
        m_lastMousePos = currentMousePos;
    }
}

void PhylogeneticTreeVisualizer::fitToCanvas(const ImVec2& canvasSize) {
    if (m_nodes.empty()) return;

    // Find bounds of all nodes
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();

    for (const auto& node : m_nodes) {
        minX = std::min(minX, node->position.x);
        maxX = std::max(maxX, node->position.x);
        minY = std::min(minY, node->position.y);
        maxY = std::max(maxY, node->position.y);
    }

    float treeWidth = maxX - minX + 100;  // Add padding
    float treeHeight = maxY - minY + 100;

    // Calculate zoom to fit
    float zoomX = canvasSize.x / treeWidth;
    float zoomY = canvasSize.y / treeHeight;
    m_zoom = std::min(zoomX, zoomY);
    m_zoom = glm::clamp(m_zoom, 0.2f, 2.0f);

    // Center the tree
    float centerX = (minX + maxX) / 2.0f;
    float centerY = (minY + maxY) / 2.0f;
    m_pan.x = (canvasSize.x / (2.0f * m_zoom)) - centerX;
    m_pan.y = (canvasSize.y / (2.0f * m_zoom)) - centerY;
}

// ============================================================================
// SpeciesEvolutionPanel Implementation
// ============================================================================

SpeciesEvolutionPanel::SpeciesEvolutionPanel() = default;

void SpeciesEvolutionPanel::render() {
    if (!m_tracker) {
        ImGui::Text("Species tracker not connected");
        return;
    }

    // Build tree visualization from current data
    m_treeVisualizer.buildFromTracker(*m_tracker);

    // Update similarity system if available
    if (m_similarity) {
        m_treeVisualizer.setSimilaritySystem(m_similarity);
    }

    // Render sections
    if (ImGui::CollapsingHeader("Species Overview", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderOverview();
    }

    if (m_similarity && ImGui::CollapsingHeader("Similarity Clusters")) {
        renderSimilarityMetrics();
    }

    if (ImGui::CollapsingHeader("Active Species", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderActiveSpeciesList();
    }

    if (m_showExtinct && ImGui::CollapsingHeader("Extinct Species")) {
        renderExtinctSpeciesList();
    }

    if (ImGui::CollapsingHeader("Phylogenetic Tree", ImGuiTreeNodeFlags_DefaultOpen)) {
        renderPhylogeneticTree();
    }
}

void SpeciesEvolutionPanel::renderOverview() {
    ImGui::Text("Active Species: %d", m_tracker->getActiveSpeciesCount());
    ImGui::Text("Total Species (inc. extinct): %d", m_tracker->getTotalSpeciesCount());
    ImGui::Text("Speciation Events: %d", m_tracker->getSpeciationEventCount());
    ImGui::Text("Extinctions: %d", m_tracker->getExtinctionEventCount());

    ImGui::Separator();

    // Show/hide controls
    ImGui::Checkbox("Show Extinct Species", &m_showExtinct);
}

void SpeciesEvolutionPanel::renderActiveSpeciesList() {
    auto activeSpecies = m_tracker->getActiveSpecies();

    if (activeSpecies.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No active species");
        return;
    }

    for (const genetics::Species* sp : activeSpecies) {
        glm::vec3 color = sp->getColor();
        ImGui::PushID(sp->getId());

        // Color indicator
        ImGui::ColorButton("##color",
                           ImVec4(color.r, color.g, color.b, 1.0f),
                           ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
                           ImVec2(16, 16));
        ImGui::SameLine();

        // Species tree node
        bool isOpen = ImGui::TreeNode("##species", "%s", sp->getName().c_str());

        // Selection highlight
        if (m_treeVisualizer.getSelectedSpecies() == sp->getId()) {
            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddRectFilled(
                min, max, IM_COL32(255, 220, 100, 30));
        }

        // Click to select
        if (ImGui::IsItemClicked()) {
            m_treeVisualizer.setSelectedSpecies(sp->getId());
            if (m_onSpeciesSelected) {
                m_onSpeciesSelected(sp->getId());
            }
        }

        if (isOpen) {
            renderSpeciesDetails(sp);
            ImGui::TreePop();
        }

        ImGui::PopID();
    }
}

void SpeciesEvolutionPanel::renderExtinctSpeciesList() {
    auto extinctSpecies = m_tracker->getExtinctSpecies();

    if (extinctSpecies.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No extinct species");
        return;
    }

    for (const genetics::Species* sp : extinctSpecies) {
        ImGui::PushID(sp->getId());

        // Gray color for extinct
        ImGui::ColorButton("##color",
                           ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                           ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
                           ImVec2(16, 16));
        ImGui::SameLine();

        if (ImGui::TreeNode("##species", "%s (Extinct)", sp->getName().c_str())) {
            ImGui::Text("Founded: Gen %d", sp->getFoundingGeneration());
            ImGui::Text("Extinct: Gen %d", sp->getExtinctionGeneration());
            ImGui::TreePop();
        }

        ImGui::PopID();
    }
}

void SpeciesEvolutionPanel::renderPhylogeneticTree() {
    // Layout and color mode controls on same row
    ImGui::PushItemWidth(120);

    static int layoutChoice = 0;
    if (ImGui::Combo("Layout", &layoutChoice, "Vertical\0Horizontal\0Radial\0")) {
        m_treeVisualizer.setLayoutStyle(static_cast<PhylogeneticTreeVisualizer::LayoutStyle>(layoutChoice));
    }

    ImGui::SameLine();

    if (ImGui::Combo("Color By", &m_colorModeIndex, "Species ID\0Cluster\0Fitness\0Age\0")) {
        m_treeVisualizer.setColorMode(static_cast<PhylogeneticTreeVisualizer::ColorMode>(m_colorModeIndex));
    }

    ImGui::PopItemWidth();

    // Fit to canvas and filter toggle
    ImVec2 canvasSize(ImGui::GetContentRegionAvail().x, 350);
    if (ImGui::Button("Fit to View")) {
        m_treeVisualizer.fitToCanvas(canvasSize);
    }
    ImGui::SameLine();
    ImGui::Checkbox("Filters", &m_showFilters);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(Scroll to zoom, Right-drag to pan)");

    // Filter panel (collapsible)
    if (m_showFilters) {
        renderFilterPanel();
    }

    // Render tree canvas
    m_treeVisualizer.render(canvasSize);

    // Cluster legend below tree
    if (m_showClusterLegend && m_colorModeIndex == 1) {
        renderClusterLegend();
    }
}

void SpeciesEvolutionPanel::renderFilterPanel() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.12f, 1.0f));
    ImGui::BeginChild("FilterPanel", ImVec2(0, 80), true);

    ImGui::Text("Filter Species:");

    // Get current filter state
    TreeFilterOptions filter = m_treeVisualizer.getFilter();
    bool filterChanged = false;

    // Row 1: Extinction filters and name filter
    if (ImGui::Checkbox("Extant", &filter.showExtant)) filterChanged = true;
    ImGui::SameLine();
    if (ImGui::Checkbox("Extinct", &filter.showExtinct)) filterChanged = true;

    ImGui::SameLine();
    ImGui::PushItemWidth(150);
    if (ImGui::InputText("Name", m_nameFilterBuf, sizeof(m_nameFilterBuf))) {
        filter.nameFilter = m_nameFilterBuf;
        filterChanged = true;
    }
    ImGui::PopItemWidth();

    if (filterChanged) {
        m_treeVisualizer.setFilter(filter);
    }

    // Row 2: Cluster filter
    auto clusterIds = m_treeVisualizer.getAvailableClusterIds();
    if (!clusterIds.empty()) {
        ImGui::Text("Cluster:");
        ImGui::SameLine();

        TreeFilterOptions filter = m_treeVisualizer.getFilter();

        if (ImGui::Button("All")) {
            filter.filterByClusterId = -1;
            m_treeVisualizer.setFilter(filter);
        }

        for (int clusterId : clusterIds) {
            ImGui::SameLine();
            char label[16];
            snprintf(label, sizeof(label), "%d", clusterId);

            bool isSelected = (filter.filterByClusterId == clusterId);
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
            }

            if (ImGui::Button(label)) {
                filter.filterByClusterId = clusterId;
                m_treeVisualizer.setFilter(filter);
            }

            if (isSelected) {
                ImGui::PopStyleColor();
            }
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void SpeciesEvolutionPanel::renderClusterLegend() {
    auto clusterInfo = m_treeVisualizer.getClusterInfo();
    if (clusterInfo.empty()) return;

    ImGui::Text("Clusters:");
    ImGui::SameLine();

    for (const auto& info : clusterInfo) {
        ImVec4 color(info.color.r, info.color.g, info.color.b, 1.0f);
        ImGui::ColorButton("##cluster", color, ImGuiColorEditFlags_NoTooltip, ImVec2(12, 12));

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Cluster %d: %d species", info.clusterId, info.memberCount);
            ImGui::EndTooltip();
        }

        ImGui::SameLine();
    }

    ImGui::NewLine();
}

void SpeciesEvolutionPanel::renderSimilarityMetrics() {
    if (!m_similarity) return;

    const auto& metrics = m_similarity->getMetrics();

    ImGui::Text("Clusters: %d", metrics.clusterCount);
    ImGui::SameLine();
    ImGui::Text("| Species: %d", metrics.speciesCount);
    ImGui::SameLine();
    ImGui::Text("| Avg Size: %.1f", metrics.averageClusterSize);

    // Quality indicators
    ImGui::Text("Cohesion: %.3f", metrics.averageIntraDistance);
    ImGui::SameLine();
    ImGui::Text("| Separation: %.3f", metrics.averageInterDistance);

    // Silhouette score with color indicator
    float silhouette = metrics.silhouetteScore;
    ImVec4 silhouetteColor;
    if (silhouette > 0.5f) {
        silhouetteColor = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);  // Good
    } else if (silhouette > 0.25f) {
        silhouetteColor = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);  // Acceptable
    } else {
        silhouetteColor = ImVec4(0.8f, 0.3f, 0.2f, 1.0f);  // Poor
    }
    ImGui::TextColored(silhouetteColor, "Silhouette: %.3f", silhouette);

    ImGui::Text("Threshold: %.3f", m_similarity->getClusterThreshold());
    ImGui::SameLine();
    ImGui::Text("| Compute: %.2fms", metrics.computeTimeMs);
}

void SpeciesEvolutionPanel::renderEventLog() {
    // Placeholder for event log - could track speciation/extinction events with timestamps
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Event log coming soon...");
}

void SpeciesEvolutionPanel::renderSpeciesDetails(const genetics::Species* species) {
    const auto& stats = species->getStats();

    ImGui::Text("Population: %d", stats.size);
    if (stats.size > 0) {
        ImGui::Text("Peak Population: %d", stats.historicalMinimum != INT_MAX ?
                    std::max(stats.size, stats.historicalMinimum) : stats.size);
    }

    ImGui::Text("Founded: Generation %d", species->getFoundingGeneration());

    ImGui::Separator();

    // Genetic statistics
    ImGui::Text("Avg Heterozygosity: %.3f", stats.averageHeterozygosity);
    ImGui::Text("Avg Fitness: %.2f", stats.averageFitness);
    ImGui::Text("Genetic Load: %.3f", stats.averageGeneticLoad);
    ImGui::Text("Effective Pop. Size: %.1f", stats.effectivePopulationSize);

    ImGui::Separator();

    // Ecological niche
    const auto& niche = species->getNiche();
    ImGui::Text("Niche:");
    ImGui::Text("  Diet Specialization: %.2f", niche.dietSpecialization);
    ImGui::Text("  Habitat Preference: %.2f", niche.habitatPreference);
    ImGui::Text("  Activity Time: %.2f", niche.activityTime);

    // Focus camera button
    if (stats.size > 0) {
        if (ImGui::Button("Focus Camera")) {
            if (m_onSpeciesSelected) {
                m_onSpeciesSelected(species->getId());
            }
        }
    }
}

} // namespace ui
