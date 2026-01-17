#pragma once

/**
 * @file FoodWebViz.h
 * @brief Food web (trophic network) visualization using force-directed layout
 *
 * Displays:
 * - Trophic levels as horizontal layers
 * - Energy flow between levels as directed edges
 * - Node size by biomass/population
 * - Edge thickness by energy transfer rate
 * - Interactive highlighting of predator-prey relationships
 */

#include "imgui.h"
#include "implot.h"
#include "../../core/FoodChainManager.h"
#include "../../core/CreatureManager.h"
#include "../../entities/CreatureType.h"
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include <string>

namespace ui {
namespace statistics {

// ============================================================================
// Food Web Node
// ============================================================================

// Extended node type to include resources (not just creatures)
enum class FoodWebNodeType {
    CREATURE,      // Standard creature type
    PRODUCER,      // Plants/producers
    DETRITUS,      // Dead organic matter
    PLANKTON,      // Aquatic primary producer
    DECOMPOSER,    // Fungi/decomposers
    NUTRIENT       // Soil nutrients (abstract)
};

struct FoodWebNode {
    CreatureType type;        // Only valid if nodeType == CREATURE
    FoodWebNodeType nodeType = FoodWebNodeType::CREATURE;
    std::string name;
    int trophicLevel = 0;     // 0 = nutrient, 1 = producer, 2 = herbivore, 3 = carnivore, etc.
    int population = 0;       // For creatures
    float biomass = 0.0f;     // For resources
    float energyFlow = 0.0f;  // Energy flowing through this node
    glm::vec2 position{0.0f};
    glm::vec2 velocity{0.0f};
    float radius = 20.0f;
    glm::vec3 color{0.5f, 0.5f, 0.5f};
    bool isHovered = false;
    bool isSelected = false;

    // For seasonal bloom indication
    float bloomMultiplier = 1.0f;  // > 1 during active bloom
};

// ============================================================================
// Food Web Edge
// ============================================================================

struct FoodWebEdge {
    int sourceIdx = -1;       // Index of prey/producer node
    int targetIdx = -1;       // Index of predator/consumer node
    float energyTransfer = 0.0f;  // Energy flowing along this edge
    float efficiency = 0.0f;  // Transfer efficiency
};

// ============================================================================
// Food Web Visualizer
// ============================================================================

/**
 * @class FoodWebViz
 * @brief Renders interactive food web visualization
 */
class FoodWebViz {
public:
    FoodWebViz();
    ~FoodWebViz() = default;

    /**
     * @brief Update food web from simulation data
     */
    void update(const Forge::FoodChainManager& foodChain,
               const Forge::CreatureManager& creatures);

    /**
     * @brief Update resource nodes (detritus, producers, decomposers)
     * Call this with ecosystem data to show non-creature resources
     */
    void updateResources(float producerBiomass, float detritusLevel,
                        float decomposerActivity, float planktonLevel,
                        float bloomMultiplier, int bloomType);

    /**
     * @brief Render the food web visualization
     */
    void render(const ImVec2& canvasSize);

    /**
     * @brief Render compact energy pyramid view
     */
    void renderPyramid(const ImVec2& canvasSize);

    /**
     * @brief Render energy flow Sankey diagram style
     */
    void renderEnergyFlow();

    // ========================================================================
    // Configuration
    // ========================================================================

    void setUseForceLayout(bool use) { m_useForceLayout = use; }
    void setShowEnergyFlow(bool show) { m_showEnergyFlow = show; }
    void setShowLabels(bool show) { m_showLabels = show; }

private:
    // Nodes and edges
    std::vector<FoodWebNode> m_nodes;
    std::vector<FoodWebEdge> m_edges;
    std::map<CreatureType, int> m_nodeIndices;

    // Resource node indices (for non-creature nodes)
    int m_producerNodeIdx = -1;
    int m_detritusNodeIdx = -1;
    int m_decomposerNodeIdx = -1;
    int m_planktonNodeIdx = -1;
    int m_nutrientNodeIdx = -1;

    // Current bloom state for visual indication
    float m_currentBloomMult = 1.0f;
    int m_currentBloomType = 0;

    // Initialize resource nodes
    void initResourceNodes();

    // Layout options
    bool m_useForceLayout = false;
    bool m_showEnergyFlow = true;
    bool m_showLabels = true;

    // View state
    float m_zoom = 1.0f;
    glm::vec2 m_pan{0.0f};
    int m_selectedNode = -1;
    int m_hoveredNode = -1;

    // Layout helpers
    void layoutTrophicLevels(const ImVec2& canvasSize);
    void layoutForceDirected(float deltaTime);
    void applyForces();

    // Rendering helpers
    void renderNode(ImDrawList* drawList, const FoodWebNode& node,
                   const ImVec2& canvasPos, const ImVec2& canvasSize);
    void renderEdge(ImDrawList* drawList, const FoodWebEdge& edge,
                   const ImVec2& canvasPos, const ImVec2& canvasSize);
    void renderTooltip(const FoodWebNode& node);

    // Coordinate helpers
    ImVec2 worldToScreen(const glm::vec2& world, const ImVec2& canvasPos,
                        const ImVec2& canvasSize) const;

    // Input handling
    void handleInput(const ImVec2& canvasPos, const ImVec2& canvasSize);

    // Predefined colors for creature types
    static glm::vec3 getColorForType(CreatureType type);
    static int getTrophicLevel(CreatureType type);
    static const char* getTypeName(CreatureType type);
};

// ============================================================================
// Helper Functions
// ============================================================================

inline glm::vec3 FoodWebViz::getColorForType(CreatureType type) {
    switch (type) {
        case CreatureType::GRAZER:
        case CreatureType::BROWSER:
        case CreatureType::FRUGIVORE:
            return glm::vec3(0.0f, 0.8f, 0.4f);  // Green for herbivores

        case CreatureType::SMALL_PREDATOR:
            return glm::vec3(0.9f, 0.5f, 0.2f);  // Orange for small predators

        case CreatureType::APEX_PREDATOR:
            return glm::vec3(0.9f, 0.2f, 0.2f);  // Red for apex predators

        case CreatureType::OMNIVORE:
            return glm::vec3(0.8f, 0.6f, 0.2f);  // Yellow-orange for omnivores

        case CreatureType::SCAVENGER:
            return glm::vec3(0.6f, 0.4f, 0.3f);  // Brown for scavengers

        default:
            return glm::vec3(0.5f, 0.5f, 0.5f);  // Gray default
    }
}

inline int FoodWebViz::getTrophicLevel(CreatureType type) {
    switch (type) {
        case CreatureType::GRAZER:
        case CreatureType::BROWSER:
        case CreatureType::FRUGIVORE:
            return 2;  // Primary consumers

        case CreatureType::SMALL_PREDATOR:
        case CreatureType::OMNIVORE:
            return 3;  // Secondary consumers

        case CreatureType::APEX_PREDATOR:
            return 4;  // Tertiary consumers

        case CreatureType::SCAVENGER:
            return 3;  // Decomposers (variable level)

        default:
            return 2;
    }
}

inline const char* FoodWebViz::getTypeName(CreatureType type) {
    switch (type) {
        case CreatureType::GRAZER: return "Grazer";
        case CreatureType::BROWSER: return "Browser";
        case CreatureType::FRUGIVORE: return "Frugivore";
        case CreatureType::SMALL_PREDATOR: return "Small Predator";
        case CreatureType::APEX_PREDATOR: return "Apex Predator";
        case CreatureType::OMNIVORE: return "Omnivore";
        case CreatureType::SCAVENGER: return "Scavenger";
        default: return "Unknown";
    }
}

} // namespace statistics
} // namespace ui
