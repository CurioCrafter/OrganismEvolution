#include "PhylogeneticTreeViz.h"
#include <algorithm>
#include <cmath>

namespace ui {
namespace statistics {

// ============================================================================
// Constructor / Destructor
// ============================================================================

PhylogeneticTreeViz::PhylogeneticTreeViz() = default;
PhylogeneticTreeViz::~PhylogeneticTreeViz() = default;

// ============================================================================
// Update From Data Sources
// ============================================================================

void PhylogeneticTreeViz::updateFromHistory(const genetics::EvolutionaryHistoryTracker& history,
                                             const genetics::SpeciationTracker& speciation) {
    // Clear existing data
    m_nodes.clear();
    m_branches.clear();
    m_nodeMap.clear();
    m_root = nullptr;

    const auto& allSpecies = speciation.getAllSpecies();
    if (allSpecies.empty()) return;

    // Create nodes for all species
    for (const auto& species : allSpecies) {
        // Skip extinct species if not showing them
        if (!m_showExtinct && !species.isExtant()) continue;

        auto node = std::make_unique<PhyloNode>();
        node->speciesId = species.id;
        node->name = species.name;
        node->population = species.currentPopulation;
        node->generation = species.foundingGeneration;
        node->isExtinct = !species.isExtant();
        node->color = species.displayColor;

        // Scale radius by population
        if (m_nodeSizeByPopulation && species.currentPopulation > 0) {
            float popScale = std::log(1.0f + species.currentPopulation) / std::log(1000.0f);
            node->radius = m_minNodeRadius + (m_maxNodeRadius - m_minNodeRadius) * std::clamp(popScale, 0.0f, 1.0f);
        } else {
            node->radius = species.isExtant() ? m_minNodeRadius + 5.0f : m_minNodeRadius;
        }

        m_nodeMap[species.id] = node.get();
        m_nodes.push_back(std::move(node));
    }

    // Build parent-child relationships
    for (const auto& species : allSpecies) {
        if (!m_showExtinct && !species.isExtant()) continue;

        auto it = m_nodeMap.find(species.id);
        if (it == m_nodeMap.end()) continue;

        PhyloNode* childNode = it->second;

        if (species.parentId != 0) {
            auto parentIt = m_nodeMap.find(species.parentId);
            if (parentIt != m_nodeMap.end()) {
                PhyloNode* parentNode = parentIt->second;
                childNode->parent = parentNode;
                parentNode->children.push_back(childNode);

                // Create branch
                PhyloBranch branch;
                branch.parent = parentNode;
                branch.child = childNode;
                branch.length = static_cast<float>(species.foundingGeneration -
                               speciation.getSpecies(species.parentId)->foundingGeneration);
                branch.color = parentNode->color;
                m_branches.push_back(branch);
            }
        }
    }

    // Find root(s) - nodes without parents
    for (auto& node : m_nodes) {
        if (node->parent == nullptr) {
            if (m_root == nullptr) {
                m_root = node.get();
            }
            // If multiple roots, create a virtual root to connect them
            // For now, just use the first root found
        }
    }

    // Calculate depths
    std::function<void(PhyloNode*, int)> calculateDepths = [&](PhyloNode* node, int depth) {
        if (!node) return;
        node->depth = depth;
        for (PhyloNode* child : node->children) {
            calculateDepths(child, depth + 1);
        }
    };
    if (m_root) {
        calculateDepths(m_root, 0);
    }

    // Layout the tree
    layoutTree();
}

// ============================================================================
// Layout Methods
// ============================================================================

void PhylogeneticTreeViz::layoutTree() {
    if (!m_root) return;

    switch (m_layoutStyle) {
        case TreeLayoutStyle::VERTICAL:
            layoutVertical(m_root, -200.0f, 200.0f, 0.0f, 0);
            break;
        case TreeLayoutStyle::HORIZONTAL:
            layoutHorizontal(m_root, -200.0f, 200.0f, 0.0f, 0);
            break;
        case TreeLayoutStyle::RADIAL:
            layoutRadial(m_root, 0.0f, 2.0f * 3.14159265f, 50.0f, 0);
            break;
        case TreeLayoutStyle::TIMELINE:
            layoutTimeline();
            break;
    }
}

void PhylogeneticTreeViz::layoutVertical(PhyloNode* node, float xMin, float xMax, float y, int depth) {
    if (!node) return;

    // Position this node
    node->position.x = (xMin + xMax) / 2.0f;
    node->position.y = y;

    // Layout children
    int totalLeaves = countLeaves(node);
    if (totalLeaves == 0) totalLeaves = 1;

    float xPos = xMin;
    for (PhyloNode* child : node->children) {
        int childLeaves = countLeaves(child);
        if (childLeaves == 0) childLeaves = 1;

        float xWidth = (xMax - xMin) * static_cast<float>(childLeaves) / static_cast<float>(totalLeaves);

        layoutVertical(child, xPos, xPos + xWidth, y + m_levelSpacing, depth + 1);
        xPos += xWidth;
    }
}

void PhylogeneticTreeViz::layoutHorizontal(PhyloNode* node, float yMin, float yMax, float x, int depth) {
    if (!node) return;

    // Position this node
    node->position.x = x;
    node->position.y = (yMin + yMax) / 2.0f;

    // Layout children
    int totalLeaves = countLeaves(node);
    if (totalLeaves == 0) totalLeaves = 1;

    float yPos = yMin;
    for (PhyloNode* child : node->children) {
        int childLeaves = countLeaves(child);
        if (childLeaves == 0) childLeaves = 1;

        float yWidth = (yMax - yMin) * static_cast<float>(childLeaves) / static_cast<float>(totalLeaves);

        layoutHorizontal(child, yPos, yPos + yWidth, x + m_levelSpacing, depth + 1);
        yPos += yWidth;
    }
}

void PhylogeneticTreeViz::layoutRadial(PhyloNode* node, float angleStart, float angleEnd, float radius, int depth) {
    if (!node) return;

    // Position this node
    float angle = (angleStart + angleEnd) / 2.0f;
    node->position.x = radius * std::cos(angle);
    node->position.y = radius * std::sin(angle);

    // Layout children
    int totalLeaves = countLeaves(node);
    if (totalLeaves == 0) totalLeaves = 1;

    float currentAngle = angleStart;
    for (PhyloNode* child : node->children) {
        int childLeaves = countLeaves(child);
        if (childLeaves == 0) childLeaves = 1;

        float angleWidth = (angleEnd - angleStart) * static_cast<float>(childLeaves) / static_cast<float>(totalLeaves);

        layoutRadial(child, currentAngle, currentAngle + angleWidth, radius + m_levelSpacing * 0.8f, depth + 1);
        currentAngle += angleWidth;
    }
}

void PhylogeneticTreeViz::layoutTimeline() {
    // Sort nodes by generation
    std::vector<PhyloNode*> sortedNodes;
    for (auto& node : m_nodes) {
        sortedNodes.push_back(node.get());
    }

    std::sort(sortedNodes.begin(), sortedNodes.end(),
              [](PhyloNode* a, PhyloNode* b) {
                  return a->generation < b->generation;
              });

    // Find generation range
    int minGen = sortedNodes.empty() ? 0 : sortedNodes.front()->generation;
    int maxGen = sortedNodes.empty() ? 1 : sortedNodes.back()->generation;
    if (maxGen == minGen) maxGen = minGen + 1;

    float genRange = static_cast<float>(maxGen - minGen);

    // Position by generation (x) and vertical stacking (y)
    std::map<int, int> generationCounts;
    std::map<int, int> generationIndices;

    // Count species per generation
    for (PhyloNode* node : sortedNodes) {
        generationCounts[node->generation]++;
    }

    // Position each node
    for (PhyloNode* node : sortedNodes) {
        float x = (static_cast<float>(node->generation - minGen) / genRange) * 400.0f;

        int count = generationCounts[node->generation];
        int& idx = generationIndices[node->generation];
        float y = (static_cast<float>(idx) - static_cast<float>(count - 1) / 2.0f) * m_nodeSpacing;
        idx++;

        node->position.x = x;
        node->position.y = y;
    }
}

int PhylogeneticTreeViz::countLeaves(PhyloNode* node) const {
    if (!node) return 0;
    if (node->children.empty()) return 1;

    int count = 0;
    for (PhyloNode* child : node->children) {
        count += countLeaves(child);
    }
    return count;
}

int PhylogeneticTreeViz::getMaxDepth(PhyloNode* node, int currentDepth) const {
    if (!node) return currentDepth;

    int maxDepth = currentDepth;
    for (PhyloNode* child : node->children) {
        maxDepth = std::max(maxDepth, getMaxDepth(child, currentDepth + 1));
    }
    return maxDepth;
}

void PhylogeneticTreeViz::setLayoutStyle(TreeLayoutStyle style) {
    if (m_layoutStyle != style) {
        m_layoutStyle = style;
        layoutTree();
    }
}

void PhylogeneticTreeViz::fitToCanvas(const ImVec2& canvasSize) {
    if (m_nodes.empty()) return;

    // Find bounds
    float minX = 1e9f, maxX = -1e9f;
    float minY = 1e9f, maxY = -1e9f;

    for (const auto& node : m_nodes) {
        minX = std::min(minX, node->position.x - node->radius);
        maxX = std::max(maxX, node->position.x + node->radius);
        minY = std::min(minY, node->position.y - node->radius);
        maxY = std::max(maxY, node->position.y + node->radius);
    }

    float treeWidth = maxX - minX;
    float treeHeight = maxY - minY;

    if (treeWidth < 1.0f) treeWidth = 1.0f;
    if (treeHeight < 1.0f) treeHeight = 1.0f;

    // Calculate zoom to fit
    float margin = 50.0f;
    float zoomX = (canvasSize.x - margin * 2) / treeWidth;
    float zoomY = (canvasSize.y - margin * 2) / treeHeight;
    m_zoom = std::min(zoomX, zoomY);
    m_zoom = std::clamp(m_zoom, 0.1f, 5.0f);

    // Center the tree
    m_pan.x = -(minX + maxX) / 2.0f;
    m_pan.y = -(minY + maxY) / 2.0f;
}

void PhylogeneticTreeViz::centerOnSpecies(genetics::SpeciesId id) {
    auto it = m_nodeMap.find(id);
    if (it != m_nodeMap.end()) {
        m_pan = -it->second->position;
    }
}

// ============================================================================
// Rendering
// ============================================================================

void PhylogeneticTreeViz::render(const ImVec2& canvasSize) {
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background
    drawList->AddRectFilled(canvasPos,
                           ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                           IM_COL32(20, 22, 25, 255));

    // Render branches first (below nodes)
    for (const auto& branch : m_branches) {
        renderBranch(drawList, branch, canvasPos);
    }

    // Render nodes
    m_hoveredSpeciesId = 0;
    for (auto& node : m_nodes) {
        renderNode(drawList, node.get(), canvasPos);
    }

    // Render legend
    renderLegend(canvasPos, canvasSize);

    // Handle input
    ImGui::InvisibleButton("TreeCanvas", canvasSize);
    handleInput(canvasPos, canvasSize);

    // Render tooltip for hovered node
    if (m_hoveredSpeciesId != 0) {
        auto it = m_nodeMap.find(m_hoveredSpeciesId);
        if (it != m_nodeMap.end()) {
            renderTooltip(it->second);
        }
    }
}

void PhylogeneticTreeViz::renderCompact(const ImVec2& canvasSize) {
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background
    drawList->AddRectFilled(canvasPos,
                           ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                           IM_COL32(20, 22, 25, 255));

    // Auto-fit for compact view
    static bool firstFrame = true;
    if (firstFrame && !m_nodes.empty()) {
        fitToCanvas(canvasSize);
        firstFrame = false;
    }

    // Render branches
    for (const auto& branch : m_branches) {
        renderBranch(drawList, branch, canvasPos);
    }

    // Render nodes (smaller)
    float savedMinRadius = m_minNodeRadius;
    float savedMaxRadius = m_maxNodeRadius;
    m_minNodeRadius = 3.0f;
    m_maxNodeRadius = 12.0f;

    for (auto& node : m_nodes) {
        renderNode(drawList, node.get(), canvasPos);
    }

    m_minNodeRadius = savedMinRadius;
    m_maxNodeRadius = savedMaxRadius;

    ImGui::InvisibleButton("TreeCanvasCompact", canvasSize);
}

void PhylogeneticTreeViz::renderTimeline(const StatisticsDataManager& data) {
    const auto& speciationEvents = data.getSpeciationEvents();
    const auto& extinctionEvents = data.getExtinctionEvents();

    ImGui::Text("Evolutionary Timeline");

    if (ImPlot::BeginPlot("##Timeline", ImVec2(-1, 150), ImPlotFlags_NoMenus)) {
        ImPlot::SetupAxes("Time (s)", "Events");
        ImPlot::SetupAxisLimits(ImAxis_Y1, -1, 1);

        // Plot speciation events
        std::vector<float> specTimes;
        std::vector<float> specY;
        for (const auto& event : speciationEvents) {
            specTimes.push_back(event.time);
            specY.push_back(0.5f);
        }

        if (!specTimes.empty()) {
            ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(0.0f, 0.8f, 0.2f, 1.0f));
            ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, 8.0f);
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Up);
            ImPlot::PlotScatter("Speciation", specTimes.data(), specY.data(),
                               static_cast<int>(specTimes.size()));
            ImPlot::PopStyleVar();
            ImPlot::PopStyleColor();
        }

        // Plot extinction events
        std::vector<float> extTimes;
        std::vector<float> extY;
        for (const auto& event : extinctionEvents) {
            extTimes.push_back(event.time);
            extY.push_back(-0.5f);
        }

        if (!extTimes.empty()) {
            ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, 8.0f);
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Down);
            ImPlot::PlotScatter("Extinction", extTimes.data(), extY.data(),
                               static_cast<int>(extTimes.size()));
            ImPlot::PopStyleVar();
            ImPlot::PopStyleColor();
        }

        ImPlot::EndPlot();
    }

    // Event counts
    ImGui::Columns(2, nullptr, false);
    ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.2f, 1.0f), "Speciations: %zu", speciationEvents.size());
    ImGui::NextColumn();
    ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "Extinctions: %zu", extinctionEvents.size());
    ImGui::Columns(1);
}

void PhylogeneticTreeViz::renderNode(ImDrawList* drawList, PhyloNode* node, const ImVec2& canvasPos) {
    ImVec2 screenPos = worldToScreen(node->position, canvasPos);

    float radius = node->radius * m_zoom;
    if (radius < 2.0f) radius = 2.0f;

    // Check if hovered
    ImVec2 mousePos = ImGui::GetMousePos();
    float dist = std::sqrt(std::pow(mousePos.x - screenPos.x, 2) +
                          std::pow(mousePos.y - screenPos.y, 2));
    node->isHovered = dist < radius + 5.0f;
    if (node->isHovered) {
        m_hoveredSpeciesId = node->speciesId;
    }

    // Node color
    ImU32 color = getNodeColor(node);

    // Draw node
    if (node->isExtinct) {
        // Extinct species - hollow circle
        drawList->AddCircle(screenPos, radius, color, 0, 2.0f);
    } else {
        // Living species - filled circle
        drawList->AddCircleFilled(screenPos, radius, color);

        // Add outline for selected
        if (node->speciesId == m_selectedSpeciesId) {
            drawList->AddCircle(screenPos, radius + 3.0f, IM_COL32(255, 255, 255, 255), 0, 2.0f);
        }
    }

    // Add highlight on hover
    if (node->isHovered) {
        drawList->AddCircle(screenPos, radius + 2.0f, IM_COL32(255, 255, 100, 200), 0, 2.0f);
    }

    // Draw label for larger nodes or hovered
    if ((radius > 8.0f || node->isHovered) && !node->name.empty()) {
        ImVec2 textPos(screenPos.x + radius + 4.0f, screenPos.y - 6.0f);
        drawList->AddText(textPos, IM_COL32(200, 200, 200, 255), node->name.c_str());
    }
}

void PhylogeneticTreeViz::renderBranch(ImDrawList* drawList, const PhyloBranch& branch,
                                        const ImVec2& canvasPos) {
    if (!branch.parent || !branch.child) return;

    ImVec2 startPos = worldToScreen(branch.parent->position, canvasPos);
    ImVec2 endPos = worldToScreen(branch.child->position, canvasPos);

    ImU32 color = getBranchColor(branch);

    // Different branch styles based on layout
    if (m_layoutStyle == TreeLayoutStyle::RADIAL) {
        // Direct line for radial
        drawList->AddLine(startPos, endPos, color, 1.5f);
    } else if (m_layoutStyle == TreeLayoutStyle::VERTICAL) {
        // L-shaped connector
        ImVec2 midPoint(startPos.x, (startPos.y + endPos.y) / 2.0f);
        ImVec2 midPoint2(endPos.x, midPoint.y);
        drawList->AddLine(startPos, midPoint, color, 1.5f);
        drawList->AddLine(midPoint, midPoint2, color, 1.5f);
        drawList->AddLine(midPoint2, endPos, color, 1.5f);
    } else if (m_layoutStyle == TreeLayoutStyle::HORIZONTAL) {
        // L-shaped connector (horizontal)
        ImVec2 midPoint((startPos.x + endPos.x) / 2.0f, startPos.y);
        ImVec2 midPoint2(midPoint.x, endPos.y);
        drawList->AddLine(startPos, midPoint, color, 1.5f);
        drawList->AddLine(midPoint, midPoint2, color, 1.5f);
        drawList->AddLine(midPoint2, endPos, color, 1.5f);
    } else {
        // Timeline - direct line
        drawList->AddLine(startPos, endPos, color, 1.5f);
    }
}

void PhylogeneticTreeViz::renderTooltip(PhyloNode* node) {
    ImGui::BeginTooltip();
    ImGui::Text("%s", node->name.c_str());
    ImGui::Separator();
    ImGui::Text("Population: %d", node->population);
    ImGui::Text("Generation: %d", node->generation);
    ImGui::Text("Children: %zu", node->children.size());
    if (node->isExtinct) {
        ImGui::TextColored(ImVec4(0.8f, 0.3f, 0.3f, 1.0f), "EXTINCT");
    }
    ImGui::EndTooltip();
}

void PhylogeneticTreeViz::renderLegend(const ImVec2& canvasPos, const ImVec2& canvasSize) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    float x = canvasPos.x + 10.0f;
    float y = canvasPos.y + canvasSize.y - 60.0f;

    // Background
    drawList->AddRectFilled(ImVec2(x - 5, y - 5), ImVec2(x + 100, y + 55),
                           IM_COL32(30, 32, 35, 200), 4.0f);

    // Legend items
    drawList->AddCircleFilled(ImVec2(x + 8, y + 8), 6.0f, IM_COL32(100, 180, 100, 255));
    drawList->AddText(ImVec2(x + 20, y), IM_COL32(200, 200, 200, 255), "Living");

    drawList->AddCircle(ImVec2(x + 8, y + 28), 6.0f, IM_COL32(150, 150, 150, 255), 0, 2.0f);
    drawList->AddText(ImVec2(x + 20, y + 20), IM_COL32(200, 200, 200, 255), "Extinct");

    // Node count
    char buf[32];
    snprintf(buf, sizeof(buf), "Species: %zu", m_nodes.size());
    drawList->AddText(ImVec2(x, y + 40), IM_COL32(150, 150, 150, 255), buf);
}

// ============================================================================
// Coordinate Transformation
// ============================================================================

ImVec2 PhylogeneticTreeViz::worldToScreen(const glm::vec2& world, const ImVec2& canvasPos) const {
    float x = (world.x + m_pan.x) * m_zoom;
    float y = (world.y + m_pan.y) * m_zoom;

    // Add canvas offset and center
    return ImVec2(canvasPos.x + ImGui::GetContentRegionAvail().x / 2.0f + x,
                  canvasPos.y + 150.0f + y);  // 150 = approximate center y
}

glm::vec2 PhylogeneticTreeViz::screenToWorld(const ImVec2& screen, const ImVec2& canvasPos) const {
    float x = screen.x - canvasPos.x - ImGui::GetContentRegionAvail().x / 2.0f;
    float y = screen.y - canvasPos.y - 150.0f;

    return glm::vec2((x / m_zoom) - m_pan.x, (y / m_zoom) - m_pan.y);
}

// ============================================================================
// Input Handling
// ============================================================================

void PhylogeneticTreeViz::handleInput(const ImVec2& canvasPos, const ImVec2& canvasSize) {
    ImGuiIO& io = ImGui::GetIO();

    // Only handle input if mouse is over canvas
    ImVec2 mousePos = ImGui::GetMousePos();
    bool isOverCanvas = mousePos.x >= canvasPos.x && mousePos.x < canvasPos.x + canvasSize.x &&
                        mousePos.y >= canvasPos.y && mousePos.y < canvasPos.y + canvasSize.y;

    if (!isOverCanvas) return;

    // Zoom with scroll wheel
    if (std::abs(io.MouseWheel) > 0.0f) {
        float zoomDelta = io.MouseWheel * 0.1f;
        m_zoom = std::clamp(m_zoom + zoomDelta, 0.1f, 5.0f);
    }

    // Pan with middle mouse button or left mouse + ctrl
    bool shouldDrag = ImGui::IsMouseDown(ImGuiMouseButton_Middle) ||
                     (ImGui::IsMouseDown(ImGuiMouseButton_Left) && io.KeyCtrl);

    if (shouldDrag) {
        if (!m_isDragging) {
            m_isDragging = true;
            m_lastMousePos = glm::vec2(mousePos.x, mousePos.y);
        } else {
            glm::vec2 delta(mousePos.x - m_lastMousePos.x, mousePos.y - m_lastMousePos.y);
            m_pan += delta / m_zoom;
            m_lastMousePos = glm::vec2(mousePos.x, mousePos.y);
        }
    } else {
        m_isDragging = false;
    }

    // Selection with left click
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.KeyCtrl) {
        glm::vec2 worldPos = screenToWorld(mousePos, canvasPos);
        PhyloNode* clickedNode = findNodeAtPosition(worldPos);

        if (clickedNode) {
            m_selectedSpeciesId = clickedNode->speciesId;
        } else {
            m_selectedSpeciesId = 0;
        }
    }
}

PhyloNode* PhylogeneticTreeViz::findNodeAtPosition(const glm::vec2& worldPos) const {
    for (const auto& node : m_nodes) {
        float dist = glm::length(node->position - worldPos);
        float hitRadius = node->radius / m_zoom + 5.0f;

        if (dist < hitRadius) {
            return node.get();
        }
    }
    return nullptr;
}

// ============================================================================
// Color Helpers
// ============================================================================

ImU32 PhylogeneticTreeViz::getNodeColor(PhyloNode* node) const {
    if (node->isExtinct) {
        return IM_COL32(100, 100, 100, 180);
    }

    // Use species color
    return IM_COL32(
        static_cast<int>(node->color.r * 255),
        static_cast<int>(node->color.g * 255),
        static_cast<int>(node->color.b * 255),
        255
    );
}

ImU32 PhylogeneticTreeViz::getBranchColor(const PhyloBranch& branch) const {
    // Gradient from parent to child color
    if (branch.child && branch.child->isExtinct) {
        return IM_COL32(80, 80, 80, 150);
    }

    return IM_COL32(
        static_cast<int>(branch.color.r * 200),
        static_cast<int>(branch.color.g * 200),
        static_cast<int>(branch.color.b * 200),
        200
    );
}

} // namespace statistics
} // namespace ui
