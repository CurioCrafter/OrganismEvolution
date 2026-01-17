#pragma once

// ToolWheel - Radial menu for God Mode tool selection
// Provides quick access to all God Mode tools with visual feedback

#include "imgui.h"
#include <glm/glm.hpp>
#include <cmath>
#include <functional>
#include <vector>
#include <string>

namespace ui {

// Tool categories
enum class ToolCategory {
    SELECT,         // Selection tool
    SPAWN,          // Creature spawning
    TERRAFORM,      // Terrain modification
    ENVIRONMENT,    // Climate/weather
    MUTATION,       // Genetic manipulation
    TIME,           // Time controls
    MANIPULATE,     // Creature manipulation
    COUNT
};

// Tool wheel item
struct ToolWheelItem {
    ToolCategory category;
    std::string name;
    std::string icon;       // Unicode symbol or short text
    std::string tooltip;
    ImVec4 color;
    bool enabled = true;
};

class ToolWheel {
public:
    ToolWheel();
    ~ToolWheel() = default;

    // Update and render
    void update();
    void render();

    // Open/close wheel
    void open(const ImVec2& position);
    void close();
    bool isOpen() const { return m_isOpen; }

    // Toggle with hotkey
    void toggle(const ImVec2& position);

    // Get active tool
    ToolCategory getActiveTool() const { return m_activeTool; }
    void setActiveTool(ToolCategory tool);

    // Handle hotkeys (1-7 for tools)
    void handleHotkeys();

    // Callbacks
    using ToolSelectedCallback = std::function<void(ToolCategory)>;
    void setOnToolSelected(ToolSelectedCallback cb) { m_onToolSelected = cb; }

    // Settings
    void setRadius(float radius) { m_radius = radius; }
    float getRadius() const { return m_radius; }

    void setInnerRadius(float radius) { m_innerRadius = radius; }
    float getInnerRadius() const { return m_innerRadius; }

    // Visibility
    bool isVisible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }

    // Get tool info
    static const char* getToolName(ToolCategory tool);
    static const char* getToolIcon(ToolCategory tool);
    static ImVec4 getToolColor(ToolCategory tool);

private:
    // State
    bool m_isOpen = false;
    bool m_visible = true;
    ImVec2 m_centerPosition{400, 300};
    ToolCategory m_activeTool = ToolCategory::SELECT;
    ToolCategory m_hoveredTool = ToolCategory::COUNT;  // COUNT = none

    // Geometry
    float m_radius = 150.0f;
    float m_innerRadius = 50.0f;
    float m_openAnimation = 0.0f;

    // Items
    std::vector<ToolWheelItem> m_items;

    // Callbacks
    ToolSelectedCallback m_onToolSelected;

    // Animation
    float m_animationSpeed = 8.0f;

    // Helper methods
    void initializeItems();
    void renderWheel();
    void renderActiveToolIndicator();

    int getHoveredSegment(const ImVec2& mousePos);
    void drawSegment(ImDrawList* drawList, int segmentIndex, bool hovered, bool selected);
};

// ============================================================================
// Implementation
// ============================================================================

inline const char* ToolWheel::getToolName(ToolCategory tool) {
    switch (tool) {
        case ToolCategory::SELECT: return "Select";
        case ToolCategory::SPAWN: return "Spawn";
        case ToolCategory::TERRAFORM: return "Terraform";
        case ToolCategory::ENVIRONMENT: return "Environment";
        case ToolCategory::MUTATION: return "Mutation";
        case ToolCategory::TIME: return "Time";
        case ToolCategory::MANIPULATE: return "Manipulate";
        default: return "Unknown";
    }
}

inline const char* ToolWheel::getToolIcon(ToolCategory tool) {
    switch (tool) {
        case ToolCategory::SELECT: return "SEL";
        case ToolCategory::SPAWN: return "SPW";
        case ToolCategory::TERRAFORM: return "TER";
        case ToolCategory::ENVIRONMENT: return "ENV";
        case ToolCategory::MUTATION: return "MUT";
        case ToolCategory::TIME: return "TIM";
        case ToolCategory::MANIPULATE: return "MAN";
        default: return "?";
    }
}

inline ImVec4 ToolWheel::getToolColor(ToolCategory tool) {
    switch (tool) {
        case ToolCategory::SELECT: return ImVec4(0.4f, 0.7f, 0.9f, 1.0f);      // Blue
        case ToolCategory::SPAWN: return ImVec4(0.3f, 0.8f, 0.3f, 1.0f);       // Green
        case ToolCategory::TERRAFORM: return ImVec4(0.7f, 0.5f, 0.3f, 1.0f);   // Brown
        case ToolCategory::ENVIRONMENT: return ImVec4(0.5f, 0.7f, 0.5f, 1.0f); // Teal
        case ToolCategory::MUTATION: return ImVec4(0.8f, 0.4f, 0.8f, 1.0f);    // Purple
        case ToolCategory::TIME: return ImVec4(0.9f, 0.8f, 0.3f, 1.0f);        // Yellow
        case ToolCategory::MANIPULATE: return ImVec4(0.9f, 0.5f, 0.3f, 1.0f);  // Orange
        default: return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    }
}

inline ToolWheel::ToolWheel() {
    initializeItems();
}

inline void ToolWheel::initializeItems() {
    m_items.clear();

    m_items.push_back({ToolCategory::SELECT, "Select", "SEL",
        "Click to select creatures (Shift+click for multi-select)",
        getToolColor(ToolCategory::SELECT)});

    m_items.push_back({ToolCategory::SPAWN, "Spawn", "SPW",
        "Spawn new creatures into the simulation",
        getToolColor(ToolCategory::SPAWN)});

    m_items.push_back({ToolCategory::TERRAFORM, "Terraform", "TER",
        "Modify terrain height and shape",
        getToolColor(ToolCategory::TERRAFORM)});

    m_items.push_back({ToolCategory::ENVIRONMENT, "Environment", "ENV",
        "Control weather, climate, and food spawning",
        getToolColor(ToolCategory::ENVIRONMENT)});

    m_items.push_back({ToolCategory::MUTATION, "Mutation", "MUT",
        "Apply genetic mutations to creatures",
        getToolColor(ToolCategory::MUTATION)});

    m_items.push_back({ToolCategory::TIME, "Time", "TIM",
        "Control simulation speed and time",
        getToolColor(ToolCategory::TIME)});

    m_items.push_back({ToolCategory::MANIPULATE, "Manipulate", "MAN",
        "Clone, kill, heal, and modify creatures",
        getToolColor(ToolCategory::MANIPULATE)});
}

inline void ToolWheel::open(const ImVec2& position) {
    m_centerPosition = position;
    m_isOpen = true;
}

inline void ToolWheel::close() {
    m_isOpen = false;
}

inline void ToolWheel::toggle(const ImVec2& position) {
    if (m_isOpen) {
        close();
    } else {
        open(position);
    }
}

inline void ToolWheel::setActiveTool(ToolCategory tool) {
    m_activeTool = tool;
    if (m_onToolSelected) {
        m_onToolSelected(tool);
    }
}

inline void ToolWheel::handleHotkeys() {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) return;

    // Tab to toggle wheel
    if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
        toggle(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f));
    }

    // Number keys for direct tool selection (when wheel is not open)
    if (!m_isOpen) {
        if (ImGui::IsKeyPressed(ImGuiKey_F1)) setActiveTool(ToolCategory::SELECT);
        if (ImGui::IsKeyPressed(ImGuiKey_F2)) setActiveTool(ToolCategory::SPAWN);
        if (ImGui::IsKeyPressed(ImGuiKey_F3)) setActiveTool(ToolCategory::TERRAFORM);
        if (ImGui::IsKeyPressed(ImGuiKey_F4)) setActiveTool(ToolCategory::ENVIRONMENT);
        if (ImGui::IsKeyPressed(ImGuiKey_F5)) setActiveTool(ToolCategory::MUTATION);
        if (ImGui::IsKeyPressed(ImGuiKey_F6)) setActiveTool(ToolCategory::TIME);
        if (ImGui::IsKeyPressed(ImGuiKey_F7)) setActiveTool(ToolCategory::MANIPULATE);
    }

    // Escape to close wheel
    if (m_isOpen && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        close();
    }
}

inline void ToolWheel::update() {
    // Update animation
    float targetAnimation = m_isOpen ? 1.0f : 0.0f;
    float dt = ImGui::GetIO().DeltaTime;
    m_openAnimation += (targetAnimation - m_openAnimation) * m_animationSpeed * dt;

    if (!m_isOpen && m_openAnimation < 0.01f) {
        m_openAnimation = 0.0f;
        return;
    }

    // Handle mouse input when open
    if (m_isOpen) {
        ImVec2 mousePos = ImGui::GetMousePos();
        m_hoveredTool = static_cast<ToolCategory>(getHoveredSegment(mousePos));

        // Click to select
        if (ImGui::IsMouseClicked(0) && m_hoveredTool != ToolCategory::COUNT) {
            setActiveTool(m_hoveredTool);
            close();
        }

        // Right click or click outside to cancel
        if (ImGui::IsMouseClicked(1)) {
            close();
        }

        // Check if mouse moved outside wheel
        float distFromCenter = sqrtf(
            (mousePos.x - m_centerPosition.x) * (mousePos.x - m_centerPosition.x) +
            (mousePos.y - m_centerPosition.y) * (mousePos.y - m_centerPosition.y)
        );

        if (ImGui::IsMouseClicked(0) && distFromCenter > m_radius * 1.2f) {
            close();
        }
    }
}

inline int ToolWheel::getHoveredSegment(const ImVec2& mousePos) {
    float dx = mousePos.x - m_centerPosition.x;
    float dy = mousePos.y - m_centerPosition.y;
    float dist = sqrtf(dx * dx + dy * dy);

    // Check if within wheel bounds
    if (dist < m_innerRadius || dist > m_radius) {
        return static_cast<int>(ToolCategory::COUNT);
    }

    // Calculate angle
    float angle = atan2f(dy, dx);
    if (angle < 0) angle += 2.0f * 3.14159f;

    // Rotate to start at top
    angle += 3.14159f * 0.5f;
    if (angle >= 2.0f * 3.14159f) angle -= 2.0f * 3.14159f;

    // Determine segment
    int numSegments = static_cast<int>(m_items.size());
    float segmentAngle = 2.0f * 3.14159f / numSegments;
    int segment = static_cast<int>(angle / segmentAngle);

    return segment % numSegments;
}

inline void ToolWheel::drawSegment(ImDrawList* drawList, int segmentIndex, bool hovered, bool selected) {
    if (segmentIndex < 0 || segmentIndex >= static_cast<int>(m_items.size())) return;

    const auto& item = m_items[segmentIndex];
    int numSegments = static_cast<int>(m_items.size());

    float segmentAngle = 2.0f * 3.14159f / numSegments;
    float startAngle = segmentIndex * segmentAngle - 3.14159f * 0.5f;  // Start at top
    float endAngle = startAngle + segmentAngle;

    // Animated radius
    float animRadius = m_radius * m_openAnimation;
    float animInnerRadius = m_innerRadius * m_openAnimation;

    // Colors
    ImVec4 baseColor = item.color;
    float alpha = m_openAnimation * (hovered ? 0.9f : (selected ? 0.7f : 0.5f));
    ImU32 fillColor = ImGui::ColorConvertFloat4ToU32(
        ImVec4(baseColor.x, baseColor.y, baseColor.z, alpha));

    ImU32 borderColor = ImGui::ColorConvertFloat4ToU32(
        ImVec4(baseColor.x, baseColor.y, baseColor.z, m_openAnimation));

    // Draw arc segment
    const int arcSegments = 20;
    std::vector<ImVec2> points;

    // Outer arc
    for (int i = 0; i <= arcSegments; ++i) {
        float a = startAngle + (endAngle - startAngle) * i / arcSegments;
        points.push_back(ImVec2(
            m_centerPosition.x + cosf(a) * animRadius,
            m_centerPosition.y + sinf(a) * animRadius
        ));
    }

    // Inner arc (reversed)
    for (int i = arcSegments; i >= 0; --i) {
        float a = startAngle + (endAngle - startAngle) * i / arcSegments;
        points.push_back(ImVec2(
            m_centerPosition.x + cosf(a) * animInnerRadius,
            m_centerPosition.y + sinf(a) * animInnerRadius
        ));
    }

    // Draw filled polygon
    drawList->AddConvexPolyFilled(points.data(), static_cast<int>(points.size()), fillColor);

    // Draw border
    for (size_t i = 0; i < points.size() / 2; ++i) {
        drawList->AddLine(points[i], points[i + 1], borderColor, 2.0f);
    }

    // Draw icon/text
    float midAngle = (startAngle + endAngle) * 0.5f;
    float iconRadius = (animRadius + animInnerRadius) * 0.5f;
    ImVec2 iconPos(
        m_centerPosition.x + cosf(midAngle) * iconRadius,
        m_centerPosition.y + sinf(midAngle) * iconRadius
    );

    // Icon background
    if (hovered || selected) {
        drawList->AddCircleFilled(iconPos, 20.0f * m_openAnimation,
            IM_COL32(0, 0, 0, static_cast<int>(100 * m_openAnimation)));
    }

    // Icon text
    ImVec2 textSize = ImGui::CalcTextSize(item.icon.c_str());
    drawList->AddText(
        ImVec2(iconPos.x - textSize.x * 0.5f, iconPos.y - textSize.y * 0.5f),
        ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, m_openAnimation)),
        item.icon.c_str()
    );
}

inline void ToolWheel::render() {
    if (!m_visible) return;

    // Always render active tool indicator
    renderActiveToolIndicator();

    // Only render wheel if animating or open
    if (m_openAnimation > 0.01f) {
        renderWheel();
    }
}

inline void ToolWheel::renderWheel() {
    ImDrawList* drawList = ImGui::GetForegroundDrawList();

    // Background circle
    float animRadius = m_radius * m_openAnimation;
    drawList->AddCircleFilled(m_centerPosition, animRadius,
        IM_COL32(20, 20, 20, static_cast<int>(180 * m_openAnimation)), 64);

    // Inner circle (cancel zone)
    float animInnerRadius = m_innerRadius * m_openAnimation;
    drawList->AddCircleFilled(m_centerPosition, animInnerRadius,
        IM_COL32(40, 40, 40, static_cast<int>(200 * m_openAnimation)), 32);

    // Draw segments
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        bool hovered = (static_cast<int>(m_hoveredTool) == i);
        bool selected = (static_cast<int>(m_activeTool) == i);
        drawSegment(drawList, i, hovered, selected);
    }

    // Center text
    const char* centerText = m_hoveredTool != ToolCategory::COUNT
        ? m_items[static_cast<int>(m_hoveredTool)].name.c_str()
        : "Cancel";

    ImVec2 textSize = ImGui::CalcTextSize(centerText);
    drawList->AddText(
        ImVec2(m_centerPosition.x - textSize.x * 0.5f,
               m_centerPosition.y - textSize.y * 0.5f),
        ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, m_openAnimation)),
        centerText
    );

    // Tooltip for hovered segment
    if (m_hoveredTool != ToolCategory::COUNT && m_openAnimation > 0.9f) {
        ImGui::BeginTooltip();
        ImGui::Text("%s", m_items[static_cast<int>(m_hoveredTool)].name.c_str());
        ImGui::TextDisabled("%s", m_items[static_cast<int>(m_hoveredTool)].tooltip.c_str());
        ImGui::EndTooltip();
    }
}

inline void ToolWheel::renderActiveToolIndicator() {
    // Small indicator in corner showing active tool
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    ImVec2 indicatorPos(displaySize.x - 100, 50);

    // Background
    drawList->AddRectFilled(
        ImVec2(indicatorPos.x - 40, indicatorPos.y - 15),
        ImVec2(indicatorPos.x + 40, indicatorPos.y + 15),
        IM_COL32(30, 30, 30, 200),
        5.0f
    );

    // Tool color bar
    ImVec4 toolColor = getToolColor(m_activeTool);
    drawList->AddRectFilled(
        ImVec2(indicatorPos.x - 40, indicatorPos.y + 10),
        ImVec2(indicatorPos.x + 40, indicatorPos.y + 15),
        ImGui::ColorConvertFloat4ToU32(toolColor),
        0.0f, ImDrawFlags_RoundCornersBottom
    );

    // Tool name
    const char* toolName = getToolName(m_activeTool);
    ImVec2 textSize = ImGui::CalcTextSize(toolName);
    drawList->AddText(
        ImVec2(indicatorPos.x - textSize.x * 0.5f, indicatorPos.y - textSize.y * 0.5f),
        IM_COL32(255, 255, 255, 255),
        toolName
    );

    // Hint text
    const char* hint = "Tab: Tool Wheel";
    ImVec2 hintSize = ImGui::CalcTextSize(hint);
    drawList->AddText(
        ImVec2(indicatorPos.x - hintSize.x * 0.5f, indicatorPos.y + 20),
        IM_COL32(150, 150, 150, 200),
        hint
    );
}

} // namespace ui
