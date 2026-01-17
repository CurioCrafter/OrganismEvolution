#pragma once

/**
 * @file PhylogeneticTreeViz.h
 * @brief Enhanced phylogenetic tree visualization for statistics dashboard
 *
 * Provides:
 * - Radial, vertical, and horizontal tree layouts
 * - Species node rendering with population-based sizing
 * - Branch length visualization (evolutionary distance)
 * - Interactive zoom and pan
 * - Speciation/extinction event highlighting
 * - Timeline view of evolutionary history
 */

#include "imgui.h"
#include "StatisticsDataManager.h"
#include "../../entities/genetics/Species.h"
#include "../../entities/genetics/EvolutionaryHistory.h"
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include <memory>

namespace ui {
namespace statistics {

// ============================================================================
// Tree Layout Types
// ============================================================================

enum class TreeLayoutStyle {
    RADIAL,      // Root at center, branches radiate outward
    VERTICAL,    // Root at top, descendants below
    HORIZONTAL,  // Root at left, descendants right
    TIMELINE     // X-axis is time, Y-axis is species diversity
};

// ============================================================================
// Tree Node (for rendering)
// ============================================================================

struct PhyloNode {
    genetics::SpeciesId speciesId = 0;
    std::string name;
    glm::vec2 position{0.0f};
    float radius = 10.0f;
    glm::vec3 color{1.0f};
    int population = 0;
    int generation = 0;
    int depth = 0;
    bool isExtinct = false;
    bool isHovered = false;
    bool isSelected = false;

    PhyloNode* parent = nullptr;
    std::vector<PhyloNode*> children;
};

// ============================================================================
// Tree Branch (for rendering)
// ============================================================================

struct PhyloBranch {
    PhyloNode* parent = nullptr;
    PhyloNode* child = nullptr;
    float length = 0.0f;  // Evolutionary distance
    glm::vec3 color{0.5f, 0.5f, 0.5f};
};

// ============================================================================
// Phylogenetic Tree Visualizer
// ============================================================================

/**
 * @class PhylogeneticTreeViz
 * @brief Renders interactive phylogenetic tree visualization
 */
class PhylogeneticTreeViz {
public:
    PhylogeneticTreeViz();
    ~PhylogeneticTreeViz();

    /**
     * @brief Update tree from evolutionary history tracker
     */
    void updateFromHistory(const genetics::EvolutionaryHistoryTracker& history,
                          const genetics::SpeciationTracker& speciation);

    /**
     * @brief Render the tree visualization
     */
    void render(const ImVec2& canvasSize);

    /**
     * @brief Render compact tree overview
     */
    void renderCompact(const ImVec2& canvasSize);

    /**
     * @brief Render timeline view of evolutionary events
     */
    void renderTimeline(const StatisticsDataManager& data);

    // ========================================================================
    // Layout Control
    // ========================================================================

    void setLayoutStyle(TreeLayoutStyle style);
    TreeLayoutStyle getLayoutStyle() const { return m_layoutStyle; }

    void setZoom(float zoom) { m_zoom = glm::clamp(zoom, 0.1f, 5.0f); }
    float getZoom() const { return m_zoom; }

    void setPan(const glm::vec2& pan) { m_pan = pan; }
    glm::vec2 getPan() const { return m_pan; }

    void fitToCanvas(const ImVec2& canvasSize);
    void centerOnSpecies(genetics::SpeciesId id);

    // ========================================================================
    // Selection
    // ========================================================================

    genetics::SpeciesId getSelectedSpecies() const { return m_selectedSpeciesId; }
    void setSelectedSpecies(genetics::SpeciesId id) { m_selectedSpeciesId = id; }

    // ========================================================================
    // Configuration
    // ========================================================================

    void setShowExtinct(bool show) { m_showExtinct = show; }
    bool getShowExtinct() const { return m_showExtinct; }

    void setShowBranchLabels(bool show) { m_showBranchLabels = show; }
    void setNodeSizeByPopulation(bool enabled) { m_nodeSizeByPopulation = enabled; }

private:
    // Tree data
    std::vector<std::unique_ptr<PhyloNode>> m_nodes;
    std::vector<PhyloBranch> m_branches;
    std::map<genetics::SpeciesId, PhyloNode*> m_nodeMap;
    PhyloNode* m_root = nullptr;

    // View state
    TreeLayoutStyle m_layoutStyle = TreeLayoutStyle::VERTICAL;
    float m_zoom = 1.0f;
    glm::vec2 m_pan{0.0f};
    bool m_isDragging = false;
    glm::vec2 m_lastMousePos{0.0f};

    // Selection
    genetics::SpeciesId m_selectedSpeciesId = 0;
    genetics::SpeciesId m_hoveredSpeciesId = 0;

    // Configuration
    bool m_showExtinct = true;
    bool m_showBranchLabels = false;
    bool m_nodeSizeByPopulation = true;

    // Layout parameters
    float m_nodeSpacing = 40.0f;
    float m_levelSpacing = 60.0f;
    float m_minNodeRadius = 5.0f;
    float m_maxNodeRadius = 25.0f;

    // Layout methods
    void layoutTree();
    void layoutVertical(PhyloNode* node, float xMin, float xMax, float y, int depth);
    void layoutHorizontal(PhyloNode* node, float yMin, float yMax, float x, int depth);
    void layoutRadial(PhyloNode* node, float angleStart, float angleEnd, float radius, int depth);
    void layoutTimeline();

    int countLeaves(PhyloNode* node) const;
    int getMaxDepth(PhyloNode* node, int currentDepth = 0) const;

    // Rendering methods
    void renderNode(ImDrawList* drawList, PhyloNode* node, const ImVec2& canvasPos);
    void renderBranch(ImDrawList* drawList, const PhyloBranch& branch, const ImVec2& canvasPos);
    void renderTooltip(PhyloNode* node);
    void renderLegend(const ImVec2& canvasPos, const ImVec2& canvasSize);

    // Coordinate transformation
    ImVec2 worldToScreen(const glm::vec2& world, const ImVec2& canvasPos) const;
    glm::vec2 screenToWorld(const ImVec2& screen, const ImVec2& canvasPos) const;

    // Input handling
    void handleInput(const ImVec2& canvasPos, const ImVec2& canvasSize);
    PhyloNode* findNodeAtPosition(const glm::vec2& worldPos) const;

    // Color helpers
    ImU32 getNodeColor(PhyloNode* node) const;
    ImU32 getBranchColor(const PhyloBranch& branch) const;
};

} // namespace statistics
} // namespace ui
