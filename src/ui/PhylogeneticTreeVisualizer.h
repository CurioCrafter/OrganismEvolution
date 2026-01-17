#pragma once

#include "imgui.h"
#include "../entities/genetics/Species.h"
#include "../entities/genetics/SpeciesSimilarity.h"
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <functional>
#include <string>

namespace ui {

// Visualization node for phylogenetic tree rendering
struct TreeRenderNode {
    uint64_t nodeId;
    genetics::SpeciesId speciesId;
    glm::vec2 position;
    float radius;
    glm::vec3 color;           // Base species/cluster color
    glm::vec3 clusterColor;    // Color from similarity clustering
    std::string name;
    int population;
    int generation;
    int clusterId;             // Similarity cluster ID (-1 if unassigned)
    float divergenceScore;     // Divergence from cluster centroid
    bool isExtinct;
    bool isHovered;
    bool isSelected;
    bool isFiltered;           // True if passes current filter
    std::vector<TreeRenderNode*> children;
    TreeRenderNode* parent;

    TreeRenderNode()
        : nodeId(0), speciesId(0), position(0.0f), radius(8.0f),
          color(1.0f), clusterColor(1.0f), population(0), generation(0),
          clusterId(-1), divergenceScore(0.0f),
          isExtinct(false), isHovered(false), isSelected(false),
          isFiltered(true), parent(nullptr) {}
};

// =============================================================================
// FILTER OPTIONS
// =============================================================================
// Controls for filtering visible species in the tree.

struct TreeFilterOptions {
    bool showExtinct = true;           // Show extinct species
    bool showExtant = true;            // Show extant species
    int filterByClusterId = -1;        // -1 = show all clusters
    std::string nameFilter;            // Substring filter for species names
    int minPopulation = 0;             // Minimum population to show
    int maxGeneration = INT_MAX;       // Only show species founded before this

    void reset() {
        showExtinct = true;
        showExtant = true;
        filterByClusterId = -1;
        nameFilter.clear();
        minPopulation = 0;
        maxGeneration = INT_MAX;
    }
};

// ============================================================================
// PhylogeneticTreeVisualizer - Interactive tree visualization
// ============================================================================
//
// Renders an interactive phylogenetic tree showing:
// - Species relationships (parent-child branches)
// - Population sizes (node radius)
// - Extinction status (grayed out nodes)
// - Speciation events (branch points)
// - Hoverable tooltips with species details
//
class PhylogeneticTreeVisualizer {
public:
    PhylogeneticTreeVisualizer();
    ~PhylogeneticTreeVisualizer();

    // Build tree from speciation tracker data
    void buildFromTracker(const genetics::SpeciationTracker& tracker);

    // Set similarity system for cluster coloring (optional, called after buildFromTracker)
    void setSimilaritySystem(const genetics::SpeciesSimilaritySystem* similarity);

    // Main render function - call within an ImGui window context
    void render(const ImVec2& canvasSize);

    // Get currently selected species (for external use)
    genetics::SpeciesId getSelectedSpecies() const { return m_selectedSpeciesId; }
    void setSelectedSpecies(genetics::SpeciesId id) { m_selectedSpeciesId = id; }

    // Configuration
    void setZoom(float zoom) { m_zoom = glm::clamp(zoom, 0.1f, 5.0f); }
    void setPan(const glm::vec2& pan) { m_pan = pan; }
    float getZoom() const { return m_zoom; }
    glm::vec2 getPan() const { return m_pan; }

    // Auto-fit tree to canvas
    void fitToCanvas(const ImVec2& canvasSize);

    // Tree layout style
    enum class LayoutStyle {
        VERTICAL,       // Root at top, descendants below
        HORIZONTAL,     // Root at left, descendants right
        RADIAL          // Root at center, descendants outward
    };
    void setLayoutStyle(LayoutStyle style) { m_layoutStyle = style; }
    LayoutStyle getLayoutStyle() const { return m_layoutStyle; }

    // Color mode
    enum class ColorMode {
        SPECIES_ID,     // Color by species ID (original behavior)
        CLUSTER,        // Color by similarity cluster
        FITNESS,        // Color by fitness gradient
        AGE             // Color by founding generation
    };
    void setColorMode(ColorMode mode) { m_colorMode = mode; updateNodeColors(); }
    ColorMode getColorMode() const { return m_colorMode; }

    // Filtering
    void setFilter(const TreeFilterOptions& filter);
    const TreeFilterOptions& getFilter() const { return m_filter; }
    void clearFilter() { m_filter.reset(); applyFilter(); }

    // Get available cluster IDs for filter UI
    std::set<int> getAvailableClusterIds() const;

    // Get cluster info for legend
    struct ClusterInfo {
        int clusterId;
        glm::vec3 color;
        int memberCount;
    };
    std::vector<ClusterInfo> getClusterInfo() const;

private:
    // Tree data
    std::vector<std::unique_ptr<TreeRenderNode>> m_nodes;
    std::map<genetics::SpeciesId, TreeRenderNode*> m_speciesNodeMap;
    TreeRenderNode* m_root = nullptr;

    // Similarity system reference (not owned)
    const genetics::SpeciesSimilaritySystem* m_similarity = nullptr;

    // Interaction state
    genetics::SpeciesId m_selectedSpeciesId = 0;
    genetics::SpeciesId m_hoveredSpeciesId = 0;

    // View state
    float m_zoom = 1.0f;
    glm::vec2 m_pan = glm::vec2(0.0f);
    bool m_isDragging = false;
    glm::vec2 m_lastMousePos;

    // Layout settings
    LayoutStyle m_layoutStyle = LayoutStyle::VERTICAL;
    ColorMode m_colorMode = ColorMode::CLUSTER;  // Default to cluster coloring
    float m_nodeSpacing = 60.0f;
    float m_levelSpacing = 80.0f;
    float m_minNodeRadius = 5.0f;
    float m_maxNodeRadius = 25.0f;

    // Filtering
    TreeFilterOptions m_filter;

    // Layout algorithm
    void layoutTree();
    void layoutVertical(TreeRenderNode* node, float xMin, float xMax, float y, int depth);
    void layoutHorizontal(TreeRenderNode* node, float yMin, float yMax, float x, int depth);
    void layoutRadial(TreeRenderNode* node, float angleStart, float angleEnd, float radius, int depth);

    // Calculate subtree width (number of visible leaves)
    int countLeaves(TreeRenderNode* node);
    int countFilteredLeaves(TreeRenderNode* node);

    // Rendering helpers
    void renderNode(ImDrawList* drawList, TreeRenderNode* node, const ImVec2& canvasPos);
    void renderBranch(ImDrawList* drawList, TreeRenderNode* parent, TreeRenderNode* child, const ImVec2& canvasPos);
    void renderTooltip(TreeRenderNode* node);
    void renderLegend(ImDrawList* drawList, const ImVec2& canvasPos, const ImVec2& canvasSize);
    void renderFilterControls();

    // Coordinate transformations
    ImVec2 worldToScreen(const glm::vec2& world, const ImVec2& canvasPos) const;
    glm::vec2 screenToWorld(const ImVec2& screen, const ImVec2& canvasPos) const;

    // Handle user interaction
    void handleInput(const ImVec2& canvasPos, const ImVec2& canvasSize);

    // Color and filter updates
    void updateNodeColors();
    void applyFilter();
    bool nodePassesFilter(const TreeRenderNode* node) const;
};

// ============================================================================
// SpeciesEvolutionPanel - Complete species & evolution UI
// ============================================================================
//
// Comprehensive panel combining:
// - Species statistics overview
// - Active species list with details
// - Phylogenetic tree visualization
// - Speciation/extinction event log
//
class SpeciesEvolutionPanel {
public:
    SpeciesEvolutionPanel();

    // Set the speciation tracker to visualize
    void setSpeciationTracker(genetics::SpeciationTracker* tracker) {
        m_tracker = tracker;
    }

    // Set the similarity system for cluster coloring
    void setSimilaritySystem(genetics::SpeciesSimilaritySystem* similarity) {
        m_similarity = similarity;
    }

    // Main render function
    void render();

    // Get selected species for camera focus
    genetics::SpeciesId getSelectedSpecies() const {
        return m_treeVisualizer.getSelectedSpecies();
    }

    // Callbacks for species selection
    using SpeciesSelectedCallback = std::function<void(genetics::SpeciesId)>;
    void setSpeciesSelectedCallback(SpeciesSelectedCallback cb) {
        m_onSpeciesSelected = cb;
    }

    // Access tree visualizer for external configuration
    PhylogeneticTreeVisualizer& getTreeVisualizer() { return m_treeVisualizer; }
    const PhylogeneticTreeVisualizer& getTreeVisualizer() const { return m_treeVisualizer; }

private:
    genetics::SpeciationTracker* m_tracker = nullptr;
    genetics::SpeciesSimilaritySystem* m_similarity = nullptr;
    PhylogeneticTreeVisualizer m_treeVisualizer;
    SpeciesSelectedCallback m_onSpeciesSelected;

    // UI state
    bool m_showExtinct = true;
    bool m_showTree = true;
    bool m_showFilters = false;
    bool m_showClusterLegend = true;
    int m_sortBy = 0;  // 0=population, 1=generation, 2=fitness
    int m_colorModeIndex = 1;  // 0=species, 1=cluster, 2=fitness, 3=age
    char m_nameFilterBuf[64] = "";

    // Render components
    void renderOverview();
    void renderActiveSpeciesList();
    void renderExtinctSpeciesList();
    void renderPhylogeneticTree();
    void renderEventLog();
    void renderSpeciesDetails(const genetics::Species* species);
    void renderFilterPanel();
    void renderClusterLegend();
    void renderSimilarityMetrics();
};

} // namespace ui
