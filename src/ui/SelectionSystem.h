#pragma once

// SelectionSystem - Creature selection via raycasting and multi-selection
// Provides click-to-select and box-select functionality for God Mode tools

#include "imgui.h"
#include "../entities/Creature.h"
#include "../graphics/Camera.h"
#include "../core/CreatureManager.h"
#include <glm/glm.hpp>
#include <vector>
#include <functional>

namespace ui {

// Selection modes
enum class SelectionMode {
    SINGLE,         // Click to select one creature
    MULTI_ADD,      // Shift+click to add to selection
    BOX_SELECT,     // Drag to box-select multiple
    LASSO           // Future: freeform selection
};

// Selection change event data
struct SelectionChangedEvent {
    Creature* newSelection = nullptr;
    std::vector<Creature*> multiSelection;
    bool wasCleared = false;
};

class SelectionSystem {
public:
    SelectionSystem();
    ~SelectionSystem() = default;

    // Main update - handles mouse input and performs raycasting
    // Returns true if a creature was clicked this frame
    bool update(const Camera& camera, Forge::CreatureManager& creatures,
                float screenWidth, float screenHeight);

    // Get currently selected creature (primary selection)
    Creature* getSelectedCreature() const { return m_selectedCreature; }

    // Get all selected creatures (for multi-selection)
    const std::vector<Creature*>& getMultiSelection() const { return m_multiSelection; }

    // Check if a specific creature is selected
    bool isSelected(const Creature* creature) const;

    // Clear all selections
    void clearSelection();

    // Programmatically select a creature
    void select(Creature* creature);
    void addToSelection(Creature* creature);
    void removeFromSelection(Creature* creature);

    // Selection mode
    void setSelectionMode(SelectionMode mode) { m_mode = mode; }
    SelectionMode getSelectionMode() const { return m_mode; }

    // Visual feedback
    bool hasSelection() const { return m_selectedCreature != nullptr || !m_multiSelection.empty(); }
    glm::vec3 getSelectionCenter() const;

    // Box selection bounds (screen space, for rendering)
    bool isBoxSelecting() const { return m_isBoxSelecting; }
    glm::vec2 getBoxStart() const { return m_boxStart; }
    glm::vec2 getBoxEnd() const { return m_boxEnd; }

    // Hover (creature under mouse, not yet selected)
    Creature* getHoveredCreature() const { return m_hoveredCreature; }

    // Callbacks
    using SelectionChangedCallback = std::function<void(const SelectionChangedEvent&)>;
    void setOnSelectionChanged(SelectionChangedCallback cb) { m_onSelectionChanged = cb; }

    // Settings
    void setSelectionRadius(float radius) { m_selectionRadius = radius; }
    float getSelectionRadius() const { return m_selectionRadius; }

    // Render selection indicators (call after main render)
    void renderSelectionIndicators(const Camera& camera, float screenWidth, float screenHeight);

private:
    // Selection state
    Creature* m_selectedCreature = nullptr;
    std::vector<Creature*> m_multiSelection;
    Creature* m_hoveredCreature = nullptr;

    // Box selection state
    bool m_isBoxSelecting = false;
    glm::vec2 m_boxStart{0.0f};
    glm::vec2 m_boxEnd{0.0f};

    // Mode
    SelectionMode m_mode = SelectionMode::SINGLE;

    // Settings
    float m_selectionRadius = 2.0f;  // Sphere radius for raycast hit detection

    // Callbacks
    SelectionChangedCallback m_onSelectionChanged;

    // Input state tracking
    bool m_wasMouseDown = false;
    glm::vec2 m_mouseDownPos{0.0f};

    // Raycasting helpers
    Creature* raycastCreature(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                               Forge::CreatureManager& creatures);

    glm::vec3 screenToWorldRay(const glm::vec2& screenPos, const Camera& camera,
                                float screenWidth, float screenHeight);

    // Box selection helpers
    void performBoxSelection(Forge::CreatureManager& creatures, const Camera& camera,
                              float screenWidth, float screenHeight);

    bool isCreatureInScreenBox(const Creature* creature, const Camera& camera,
                                const glm::vec2& boxMin, const glm::vec2& boxMax,
                                float screenWidth, float screenHeight);

    // Notification
    void notifySelectionChanged(bool wasCleared = false);
};

// ============================================================================
// Implementation
// ============================================================================

inline SelectionSystem::SelectionSystem() {
    m_multiSelection.reserve(100);
}

inline bool SelectionSystem::isSelected(const Creature* creature) const {
    if (creature == nullptr) return false;
    if (creature == m_selectedCreature) return true;
    for (const auto* c : m_multiSelection) {
        if (c == creature) return true;
    }
    return false;
}

inline void SelectionSystem::clearSelection() {
    m_selectedCreature = nullptr;
    m_multiSelection.clear();
    notifySelectionChanged(true);
}

inline void SelectionSystem::select(Creature* creature) {
    m_multiSelection.clear();
    m_selectedCreature = creature;
    if (creature) {
        m_multiSelection.push_back(creature);
    }
    notifySelectionChanged();
}

inline void SelectionSystem::addToSelection(Creature* creature) {
    if (creature == nullptr) return;
    if (isSelected(creature)) return;

    m_multiSelection.push_back(creature);
    if (m_selectedCreature == nullptr) {
        m_selectedCreature = creature;
    }
    notifySelectionChanged();
}

inline void SelectionSystem::removeFromSelection(Creature* creature) {
    if (creature == nullptr) return;

    auto it = std::find(m_multiSelection.begin(), m_multiSelection.end(), creature);
    if (it != m_multiSelection.end()) {
        m_multiSelection.erase(it);
    }

    if (m_selectedCreature == creature) {
        m_selectedCreature = m_multiSelection.empty() ? nullptr : m_multiSelection[0];
    }
    notifySelectionChanged();
}

inline glm::vec3 SelectionSystem::getSelectionCenter() const {
    if (m_multiSelection.empty()) {
        return glm::vec3(0.0f);
    }

    glm::vec3 center(0.0f);
    for (const auto* creature : m_multiSelection) {
        center += creature->getPosition();
    }
    return center / static_cast<float>(m_multiSelection.size());
}

inline void SelectionSystem::notifySelectionChanged(bool wasCleared) {
    if (m_onSelectionChanged) {
        SelectionChangedEvent event;
        event.newSelection = m_selectedCreature;
        event.multiSelection = m_multiSelection;
        event.wasCleared = wasCleared;
        m_onSelectionChanged(event);
    }
}

inline glm::vec3 SelectionSystem::screenToWorldRay(const glm::vec2& screenPos,
                                                     const Camera& camera,
                                                     float screenWidth, float screenHeight) {
    // Convert screen coords to normalized device coords (-1 to 1)
    float x = (2.0f * screenPos.x) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y) / screenHeight;  // Flip Y

    // Get view and projection matrices
    glm::mat4 view = const_cast<Camera&>(camera).getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix(screenWidth / screenHeight);

    // Inverse view-projection
    glm::mat4 invVP = glm::inverse(proj * view);

    // Near and far points in clip space
    glm::vec4 nearPoint(x, y, -1.0f, 1.0f);
    glm::vec4 farPoint(x, y, 1.0f, 1.0f);

    // Transform to world space
    glm::vec4 nearWorld = invVP * nearPoint;
    glm::vec4 farWorld = invVP * farPoint;

    nearWorld /= nearWorld.w;
    farWorld /= farWorld.w;

    // Ray direction
    return glm::normalize(glm::vec3(farWorld) - glm::vec3(nearWorld));
}

inline Creature* SelectionSystem::raycastCreature(const glm::vec3& rayOrigin,
                                                    const glm::vec3& rayDir,
                                                    Forge::CreatureManager& creatures) {
    Creature* closest = nullptr;
    float closestDist = std::numeric_limits<float>::max();

    creatures.forEach([&](Creature& creature, size_t) {
        if (!creature.isAlive()) return;

        glm::vec3 pos = creature.getPosition();
        float radius = creature.getGenome().size * m_selectionRadius;

        // Ray-sphere intersection
        glm::vec3 oc = rayOrigin - pos;
        float a = glm::dot(rayDir, rayDir);
        float b = 2.0f * glm::dot(oc, rayDir);
        float c = glm::dot(oc, oc) - radius * radius;
        float discriminant = b * b - 4.0f * a * c;

        if (discriminant >= 0) {
            float t = (-b - sqrt(discriminant)) / (2.0f * a);
            if (t > 0 && t < closestDist) {
                closestDist = t;
                closest = &creature;
            }
        }
    });

    return closest;
}

inline bool SelectionSystem::isCreatureInScreenBox(const Creature* creature,
                                                     const Camera& camera,
                                                     const glm::vec2& boxMin,
                                                     const glm::vec2& boxMax,
                                                     float screenWidth, float screenHeight) {
    if (!creature || !creature->isAlive()) return false;

    glm::vec3 worldPos = creature->getPosition();
    glm::mat4 view = const_cast<Camera&>(camera).getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix(screenWidth / screenHeight);
    glm::mat4 vp = proj * view;

    glm::vec4 clipPos = vp * glm::vec4(worldPos, 1.0f);

    // Behind camera check
    if (clipPos.w <= 0) return false;

    // Perspective divide
    glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;

    // Convert to screen coords
    float screenX = (ndc.x + 1.0f) * 0.5f * screenWidth;
    float screenY = (1.0f - ndc.y) * 0.5f * screenHeight;  // Flip Y

    return screenX >= boxMin.x && screenX <= boxMax.x &&
           screenY >= boxMin.y && screenY <= boxMax.y;
}

inline void SelectionSystem::performBoxSelection(Forge::CreatureManager& creatures,
                                                   const Camera& camera,
                                                   float screenWidth, float screenHeight) {
    glm::vec2 boxMin(std::min(m_boxStart.x, m_boxEnd.x), std::min(m_boxStart.y, m_boxEnd.y));
    glm::vec2 boxMax(std::max(m_boxStart.x, m_boxEnd.x), std::max(m_boxStart.y, m_boxEnd.y));

    // Minimum box size check
    if (glm::length(boxMax - boxMin) < 5.0f) return;

    m_multiSelection.clear();
    m_selectedCreature = nullptr;

    creatures.forEach([&](Creature& creature, size_t) {
        if (isCreatureInScreenBox(&creature, camera, boxMin, boxMax, screenWidth, screenHeight)) {
            m_multiSelection.push_back(&creature);
            if (m_selectedCreature == nullptr) {
                m_selectedCreature = &creature;
            }
        }
    });

    notifySelectionChanged();
}

inline bool SelectionSystem::update(const Camera& camera,
                                      Forge::CreatureManager& creatures,
                                      float screenWidth, float screenHeight) {
    // Check if ImGui wants the mouse
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        m_hoveredCreature = nullptr;
        m_isBoxSelecting = false;
        return false;
    }

    glm::vec2 mousePos(io.MousePos.x, io.MousePos.y);
    bool mouseDown = io.MouseDown[0];  // Left mouse button
    bool shiftHeld = io.KeyShift;

    // Calculate ray from mouse position
    glm::vec3 rayDir = screenToWorldRay(mousePos, camera, screenWidth, screenHeight);
    glm::vec3 rayOrigin = camera.Position;

    // Update hover
    m_hoveredCreature = raycastCreature(rayOrigin, rayDir, creatures);

    // Handle mouse input
    if (mouseDown && !m_wasMouseDown) {
        // Mouse just pressed
        m_mouseDownPos = mousePos;
        m_boxStart = mousePos;
        m_boxEnd = mousePos;
    }
    else if (mouseDown && m_wasMouseDown) {
        // Mouse held - check for box select drag
        float dragDist = glm::length(mousePos - m_mouseDownPos);
        if (dragDist > 10.0f) {
            m_isBoxSelecting = true;
            m_boxEnd = mousePos;
        }
    }
    else if (!mouseDown && m_wasMouseDown) {
        // Mouse released
        if (m_isBoxSelecting) {
            // Finish box selection
            m_boxEnd = mousePos;
            performBoxSelection(creatures, camera, screenWidth, screenHeight);
            m_isBoxSelecting = false;
        }
        else {
            // Single click selection
            Creature* clicked = raycastCreature(rayOrigin, rayDir, creatures);

            if (clicked) {
                if (shiftHeld && m_mode == SelectionMode::SINGLE) {
                    // Shift+click: toggle in multi-selection
                    if (isSelected(clicked)) {
                        removeFromSelection(clicked);
                    } else {
                        addToSelection(clicked);
                    }
                } else {
                    // Normal click: select single
                    select(clicked);
                }
                m_wasMouseDown = mouseDown;
                return true;
            } else {
                // Clicked empty space - clear selection
                clearSelection();
            }
        }
    }

    m_wasMouseDown = mouseDown;
    return false;
}

inline void SelectionSystem::renderSelectionIndicators(const Camera& camera,
                                                         float screenWidth, float screenHeight) {
    // Render box selection rectangle
    if (m_isBoxSelecting) {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();

        glm::vec2 boxMin(std::min(m_boxStart.x, m_boxEnd.x), std::min(m_boxStart.y, m_boxEnd.y));
        glm::vec2 boxMax(std::max(m_boxStart.x, m_boxEnd.x), std::max(m_boxStart.y, m_boxEnd.y));

        // Fill
        drawList->AddRectFilled(
            ImVec2(boxMin.x, boxMin.y),
            ImVec2(boxMax.x, boxMax.y),
            IM_COL32(100, 150, 255, 50)
        );

        // Border
        drawList->AddRect(
            ImVec2(boxMin.x, boxMin.y),
            ImVec2(boxMax.x, boxMax.y),
            IM_COL32(100, 150, 255, 200),
            0.0f, 0, 2.0f
        );
    }

    // Render selection circles around selected creatures
    ImDrawList* drawList = ImGui::GetForegroundDrawList();

    for (const auto* creature : m_multiSelection) {
        if (!creature || !creature->isAlive()) continue;

        glm::vec3 worldPos = creature->getPosition();
        glm::mat4 view = const_cast<Camera&>(camera).getViewMatrix();
        glm::mat4 proj = camera.getProjectionMatrix(screenWidth / screenHeight);
        glm::mat4 vp = proj * view;

        glm::vec4 clipPos = vp * glm::vec4(worldPos, 1.0f);
        if (clipPos.w <= 0) continue;

        glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
        float screenX = (ndc.x + 1.0f) * 0.5f * screenWidth;
        float screenY = (1.0f - ndc.y) * 0.5f * screenHeight;

        // Selection indicator
        ImU32 color = (creature == m_selectedCreature)
            ? IM_COL32(255, 200, 50, 200)   // Primary selection: gold
            : IM_COL32(100, 200, 255, 150); // Multi-selection: blue

        float radius = 20.0f / (clipPos.w * 0.1f);  // Scale with distance
        radius = glm::clamp(radius, 10.0f, 40.0f);

        drawList->AddCircle(ImVec2(screenX, screenY), radius, color, 32, 2.0f);
    }

    // Render hover indicator
    if (m_hoveredCreature && !isSelected(m_hoveredCreature)) {
        glm::vec3 worldPos = m_hoveredCreature->getPosition();
        glm::mat4 view = const_cast<Camera&>(camera).getViewMatrix();
        glm::mat4 proj = camera.getProjectionMatrix(screenWidth / screenHeight);
        glm::mat4 vp = proj * view;

        glm::vec4 clipPos = vp * glm::vec4(worldPos, 1.0f);
        if (clipPos.w > 0) {
            glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
            float screenX = (ndc.x + 1.0f) * 0.5f * screenWidth;
            float screenY = (1.0f - ndc.y) * 0.5f * screenHeight;

            float radius = 20.0f / (clipPos.w * 0.1f);
            radius = glm::clamp(radius, 10.0f, 40.0f);

            drawList->AddCircle(ImVec2(screenX, screenY), radius, IM_COL32(255, 255, 255, 100), 32, 1.5f);
        }
    }
}

} // namespace ui
