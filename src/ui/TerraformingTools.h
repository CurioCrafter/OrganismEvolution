#pragma once

// TerraformingTools - Terrain modification tools for God Mode
// Provides raise, lower, smooth, flatten, and biome painting capabilities

#include "imgui.h"
#include "../environment/Terrain.h"
#include "../graphics/Camera.h"
#include <glm/glm.hpp>
#include <vector>
#include <deque>
#include <functional>

namespace ui {

// Terraforming tool types
enum class TerraformTool {
    RAISE,          // Raise terrain height
    LOWER,          // Lower terrain height
    SMOOTH,         // Smooth terrain (average neighbors)
    FLATTEN,        // Flatten to target height
    NOISE,          // Add random noise
    PLATEAU,        // Create flat plateau
    CRATER          // Create crater/depression
};

// Brush shape
enum class BrushShape {
    CIRCLE,         // Circular brush
    SQUARE,         // Square brush
    DIAMOND         // Diamond/rhombus brush
};

// Falloff type for brush edge
enum class BrushFalloff {
    LINEAR,         // Linear falloff
    SMOOTH,         // Smooth (cosine) falloff
    CONSTANT,       // No falloff (hard edge)
    GAUSSIAN        // Gaussian falloff
};

// Single terrain edit for undo
struct TerrainEdit {
    int x, z;                   // Grid position
    float previousHeight;       // Height before edit
    float newHeight;            // Height after edit
};

// Batch of edits (one brush stroke)
struct TerrainEditBatch {
    std::vector<TerrainEdit> edits;
    TerraformTool tool;
    std::string description;
    float timestamp;
};

class TerraformingTools {
public:
    TerraformingTools();
    ~TerraformingTools() = default;

    // Main update - handles input and applies brush
    void update(float deltaTime, const Camera& camera, float screenWidth, float screenHeight);

    // Render UI controls
    void renderUI();

    // Set terrain reference
    void setTerrain(Terrain* terrain) { m_terrain = terrain; }

    // Tool selection
    void setTool(TerraformTool tool) { m_currentTool = tool; }
    TerraformTool getTool() const { return m_currentTool; }

    // Brush settings
    void setBrushSize(float size) { m_brushSize = glm::clamp(size, 1.0f, 100.0f); }
    float getBrushSize() const { return m_brushSize; }

    void setBrushStrength(float strength) { m_brushStrength = glm::clamp(strength, 0.01f, 5.0f); }
    float getBrushStrength() const { return m_brushStrength; }

    void setBrushShape(BrushShape shape) { m_brushShape = shape; }
    BrushShape getBrushShape() const { return m_brushShape; }

    void setBrushFalloff(BrushFalloff falloff) { m_brushFalloff = falloff; }
    BrushFalloff getBrushFalloff() const { return m_brushFalloff; }

    // Flatten tool target height
    void setFlattenHeight(float height) { m_flattenHeight = height; }
    float getFlattenHeight() const { return m_flattenHeight; }

    // Undo/redo
    void undo();
    void redo();
    bool canUndo() const { return !m_undoStack.empty(); }
    bool canRedo() const { return !m_redoStack.empty(); }
    int getUndoCount() const { return static_cast<int>(m_undoStack.size()); }

    // Get brush position for visualization
    glm::vec3 getBrushPosition() const { return m_brushPosition; }
    bool isBrushActive() const { return m_brushActive; }

    // Is the tool currently being used
    bool isEditing() const { return m_isEditing; }

    // Panel visibility
    bool isVisible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }
    void toggleVisible() { m_visible = !m_visible; }

    // Settings
    void setUndoLimit(int limit) { m_undoLimit = limit; }
    void setTerrainModificationEnabled(bool enabled) { m_modificationEnabled = enabled; }

private:
    // Terrain reference
    Terrain* m_terrain = nullptr;

    // Tool state
    TerraformTool m_currentTool = TerraformTool::RAISE;
    BrushShape m_brushShape = BrushShape::CIRCLE;
    BrushFalloff m_brushFalloff = BrushFalloff::SMOOTH;

    // Brush parameters
    float m_brushSize = 15.0f;
    float m_brushStrength = 1.0f;
    float m_flattenHeight = 10.0f;

    // Brush state
    glm::vec3 m_brushPosition{0.0f};
    bool m_brushActive = false;
    bool m_isEditing = false;

    // Current edit batch (accumulated while mouse is held)
    TerrainEditBatch m_currentBatch;

    // Undo/redo stacks
    std::deque<TerrainEditBatch> m_undoStack;
    std::deque<TerrainEditBatch> m_redoStack;
    int m_undoLimit = 30;

    // UI state
    bool m_visible = true;
    bool m_showAdvanced = false;

    // Modification enabled flag
    bool m_modificationEnabled = true;

    // Input tracking
    bool m_wasMouseDown = false;

    // Apply brush at position
    void applyBrush(const glm::vec3& worldPosition);

    // Calculate brush intensity at point
    float calculateBrushIntensity(float distanceFromCenter) const;

    // Get terrain position from screen coordinates
    bool getTerrainPosition(const Camera& camera, float screenX, float screenY,
                             float screenWidth, float screenHeight, glm::vec3& outPosition);

    // Terrain modification functions
    void modifyHeight(int gridX, int gridZ, float amount);
    float getHeight(int gridX, int gridZ) const;
    void setHeight(int gridX, int gridZ, float height);

    // Tool-specific operations
    void applyRaise(int centerX, int centerZ);
    void applyLower(int centerX, int centerZ);
    void applySmooth(int centerX, int centerZ);
    void applyFlatten(int centerX, int centerZ);
    void applyNoise(int centerX, int centerZ);
    void applyPlateau(int centerX, int centerZ);
    void applyCrater(int centerX, int centerZ);

    // Undo helpers
    void pushUndoBatch();
    void applyBatch(const TerrainEditBatch& batch, bool reverse);

    // Tool name helpers
    static const char* getToolName(TerraformTool tool);
    static const char* getShapeName(BrushShape shape);
    static const char* getFalloffName(BrushFalloff falloff);
};

// ============================================================================
// Implementation
// ============================================================================

inline TerraformingTools::TerraformingTools() {
    m_currentBatch.edits.reserve(1000);
}

inline const char* TerraformingTools::getToolName(TerraformTool tool) {
    switch (tool) {
        case TerraformTool::RAISE: return "Raise";
        case TerraformTool::LOWER: return "Lower";
        case TerraformTool::SMOOTH: return "Smooth";
        case TerraformTool::FLATTEN: return "Flatten";
        case TerraformTool::NOISE: return "Add Noise";
        case TerraformTool::PLATEAU: return "Plateau";
        case TerraformTool::CRATER: return "Crater";
        default: return "Unknown";
    }
}

inline const char* TerraformingTools::getShapeName(BrushShape shape) {
    switch (shape) {
        case BrushShape::CIRCLE: return "Circle";
        case BrushShape::SQUARE: return "Square";
        case BrushShape::DIAMOND: return "Diamond";
        default: return "Unknown";
    }
}

inline const char* TerraformingTools::getFalloffName(BrushFalloff falloff) {
    switch (falloff) {
        case BrushFalloff::LINEAR: return "Linear";
        case BrushFalloff::SMOOTH: return "Smooth";
        case BrushFalloff::CONSTANT: return "Hard Edge";
        case BrushFalloff::GAUSSIAN: return "Gaussian";
        default: return "Unknown";
    }
}

inline float TerraformingTools::calculateBrushIntensity(float distanceFromCenter) const {
    float normalizedDist = distanceFromCenter / m_brushSize;
    if (normalizedDist > 1.0f) return 0.0f;

    switch (m_brushFalloff) {
        case BrushFalloff::LINEAR:
            return 1.0f - normalizedDist;

        case BrushFalloff::SMOOTH:
            return 0.5f * (1.0f + cos(normalizedDist * 3.14159f));

        case BrushFalloff::CONSTANT:
            return 1.0f;

        case BrushFalloff::GAUSSIAN:
            return exp(-4.0f * normalizedDist * normalizedDist);

        default:
            return 1.0f - normalizedDist;
    }
}

inline bool TerraformingTools::getTerrainPosition(const Camera& camera, float screenX, float screenY,
                                                    float screenWidth, float screenHeight,
                                                    glm::vec3& outPosition) {
    if (!m_terrain) return false;

    // Convert screen coords to normalized device coords
    float x = (2.0f * screenX) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * screenY) / screenHeight;

    // Get view and projection matrices
    glm::mat4 view = const_cast<Camera&>(camera).getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix(screenWidth / screenHeight);
    glm::mat4 invVP = glm::inverse(proj * view);

    // Get ray
    glm::vec4 nearPoint(x, y, -1.0f, 1.0f);
    glm::vec4 farPoint(x, y, 1.0f, 1.0f);

    glm::vec4 nearWorld = invVP * nearPoint;
    glm::vec4 farWorld = invVP * farPoint;

    nearWorld /= nearWorld.w;
    farWorld /= farWorld.w;

    glm::vec3 rayOrigin(nearWorld);
    glm::vec3 rayDir = glm::normalize(glm::vec3(farWorld) - glm::vec3(nearWorld));

    // Raycast against terrain (simple iterative approach)
    float t = 0.0f;
    float maxDist = 1000.0f;
    float step = 1.0f;

    while (t < maxDist) {
        glm::vec3 point = rayOrigin + rayDir * t;

        if (m_terrain->isInBounds(point.x, point.z)) {
            float terrainHeight = m_terrain->getHeight(point.x, point.z);

            if (point.y <= terrainHeight) {
                // Hit terrain - refine with smaller steps
                t -= step;
                step *= 0.5f;

                if (step < 0.1f) {
                    outPosition = point;
                    outPosition.y = terrainHeight;
                    return true;
                }
            }
        }

        t += step;
    }

    return false;
}

inline void TerraformingTools::update(float deltaTime, const Camera& camera,
                                        float screenWidth, float screenHeight) {
    if (!m_terrain || !m_modificationEnabled) {
        m_brushActive = false;
        return;
    }

    // Check if ImGui wants mouse
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        m_brushActive = false;
        m_isEditing = false;
        return;
    }

    glm::vec2 mousePos(io.MousePos.x, io.MousePos.y);
    bool mouseDown = io.MouseDown[0];

    // Update brush position
    m_brushActive = getTerrainPosition(camera, mousePos.x, mousePos.y,
                                        screenWidth, screenHeight, m_brushPosition);

    // Handle mouse input
    if (mouseDown && m_brushActive) {
        if (!m_wasMouseDown) {
            // Start new edit batch
            m_currentBatch.edits.clear();
            m_currentBatch.tool = m_currentTool;
            m_currentBatch.description = getToolName(m_currentTool);
            m_currentBatch.timestamp = 0.0f;  // Would use actual time
        }

        // Apply brush
        applyBrush(m_brushPosition);
        m_isEditing = true;
    }
    else if (!mouseDown && m_wasMouseDown && m_isEditing) {
        // Finished editing - push to undo stack
        if (!m_currentBatch.edits.empty()) {
            pushUndoBatch();
        }
        m_isEditing = false;
    }

    m_wasMouseDown = mouseDown;
}

inline void TerraformingTools::applyBrush(const glm::vec3& worldPosition) {
    if (!m_terrain) return;

    // Convert world position to grid coordinates
    float scale = m_terrain->getScale();
    int centerX = static_cast<int>(worldPosition.x / scale);
    int centerZ = static_cast<int>(worldPosition.z / scale);

    // Apply tool
    switch (m_currentTool) {
        case TerraformTool::RAISE:
            applyRaise(centerX, centerZ);
            break;
        case TerraformTool::LOWER:
            applyLower(centerX, centerZ);
            break;
        case TerraformTool::SMOOTH:
            applySmooth(centerX, centerZ);
            break;
        case TerraformTool::FLATTEN:
            applyFlatten(centerX, centerZ);
            break;
        case TerraformTool::NOISE:
            applyNoise(centerX, centerZ);
            break;
        case TerraformTool::PLATEAU:
            applyPlateau(centerX, centerZ);
            break;
        case TerraformTool::CRATER:
            applyCrater(centerX, centerZ);
            break;
    }
}

inline float TerraformingTools::getHeight(int gridX, int gridZ) const {
    if (!m_terrain) return 0.0f;

    int width = m_terrain->getWidth();
    int depth = m_terrain->getDepth();

    if (gridX < 0 || gridX >= width || gridZ < 0 || gridZ >= depth) {
        return 0.0f;
    }

    float scale = m_terrain->getScale();
    return m_terrain->getHeight(gridX * scale, gridZ * scale);
}

inline void TerraformingTools::setHeight(int gridX, int gridZ, float height) {
    // Note: The current Terrain class doesn't expose direct height modification
    // This would need to be added to Terrain.h for full functionality
    // For now, we record the edits for undo but actual modification requires Terrain API extension
}

inline void TerraformingTools::modifyHeight(int gridX, int gridZ, float amount) {
    float currentHeight = getHeight(gridX, gridZ);
    float newHeight = currentHeight + amount;

    // Record edit
    TerrainEdit edit;
    edit.x = gridX;
    edit.z = gridZ;
    edit.previousHeight = currentHeight;
    edit.newHeight = newHeight;
    m_currentBatch.edits.push_back(edit);

    // Apply change (requires Terrain API extension)
    setHeight(gridX, gridZ, newHeight);
}

inline void TerraformingTools::applyRaise(int centerX, int centerZ) {
    int radius = static_cast<int>(m_brushSize / m_terrain->getScale());

    for (int dx = -radius; dx <= radius; ++dx) {
        for (int dz = -radius; dz <= radius; ++dz) {
            float dist = sqrt(static_cast<float>(dx * dx + dz * dz)) * m_terrain->getScale();

            if (m_brushShape == BrushShape::SQUARE ||
                (m_brushShape == BrushShape::CIRCLE && dist <= m_brushSize) ||
                (m_brushShape == BrushShape::DIAMOND && abs(dx) + abs(dz) <= radius)) {

                float intensity = calculateBrushIntensity(dist);
                modifyHeight(centerX + dx, centerZ + dz, m_brushStrength * intensity * 0.1f);
            }
        }
    }
}

inline void TerraformingTools::applyLower(int centerX, int centerZ) {
    int radius = static_cast<int>(m_brushSize / m_terrain->getScale());

    for (int dx = -radius; dx <= radius; ++dx) {
        for (int dz = -radius; dz <= radius; ++dz) {
            float dist = sqrt(static_cast<float>(dx * dx + dz * dz)) * m_terrain->getScale();

            if (m_brushShape == BrushShape::SQUARE ||
                (m_brushShape == BrushShape::CIRCLE && dist <= m_brushSize) ||
                (m_brushShape == BrushShape::DIAMOND && abs(dx) + abs(dz) <= radius)) {

                float intensity = calculateBrushIntensity(dist);
                modifyHeight(centerX + dx, centerZ + dz, -m_brushStrength * intensity * 0.1f);
            }
        }
    }
}

inline void TerraformingTools::applySmooth(int centerX, int centerZ) {
    int radius = static_cast<int>(m_brushSize / m_terrain->getScale());

    for (int dx = -radius; dx <= radius; ++dx) {
        for (int dz = -radius; dz <= radius; ++dz) {
            float dist = sqrt(static_cast<float>(dx * dx + dz * dz)) * m_terrain->getScale();

            if (dist <= m_brushSize) {
                // Average neighboring heights
                float avg = 0.0f;
                int count = 0;

                for (int nx = -1; nx <= 1; ++nx) {
                    for (int nz = -1; nz <= 1; ++nz) {
                        avg += getHeight(centerX + dx + nx, centerZ + dz + nz);
                        ++count;
                    }
                }

                if (count > 0) {
                    avg /= count;
                    float current = getHeight(centerX + dx, centerZ + dz);
                    float intensity = calculateBrushIntensity(dist);
                    float newHeight = current + (avg - current) * intensity * m_brushStrength * 0.1f;

                    TerrainEdit edit;
                    edit.x = centerX + dx;
                    edit.z = centerZ + dz;
                    edit.previousHeight = current;
                    edit.newHeight = newHeight;
                    m_currentBatch.edits.push_back(edit);

                    setHeight(centerX + dx, centerZ + dz, newHeight);
                }
            }
        }
    }
}

inline void TerraformingTools::applyFlatten(int centerX, int centerZ) {
    int radius = static_cast<int>(m_brushSize / m_terrain->getScale());

    for (int dx = -radius; dx <= radius; ++dx) {
        for (int dz = -radius; dz <= radius; ++dz) {
            float dist = sqrt(static_cast<float>(dx * dx + dz * dz)) * m_terrain->getScale();

            if (dist <= m_brushSize) {
                float current = getHeight(centerX + dx, centerZ + dz);
                float intensity = calculateBrushIntensity(dist);
                float newHeight = current + (m_flattenHeight - current) * intensity * m_brushStrength * 0.1f;

                TerrainEdit edit;
                edit.x = centerX + dx;
                edit.z = centerZ + dz;
                edit.previousHeight = current;
                edit.newHeight = newHeight;
                m_currentBatch.edits.push_back(edit);

                setHeight(centerX + dx, centerZ + dz, newHeight);
            }
        }
    }
}

inline void TerraformingTools::applyNoise(int centerX, int centerZ) {
    int radius = static_cast<int>(m_brushSize / m_terrain->getScale());

    for (int dx = -radius; dx <= radius; ++dx) {
        for (int dz = -radius; dz <= radius; ++dz) {
            float dist = sqrt(static_cast<float>(dx * dx + dz * dz)) * m_terrain->getScale();

            if (dist <= m_brushSize) {
                float intensity = calculateBrushIntensity(dist);
                float noise = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f;
                modifyHeight(centerX + dx, centerZ + dz, noise * m_brushStrength * intensity * 0.1f);
            }
        }
    }
}

inline void TerraformingTools::applyPlateau(int centerX, int centerZ) {
    // Flatten inner area, raise edges
    int radius = static_cast<int>(m_brushSize / m_terrain->getScale());
    int innerRadius = radius * 2 / 3;

    for (int dx = -radius; dx <= radius; ++dx) {
        for (int dz = -radius; dz <= radius; ++dz) {
            float dist = sqrt(static_cast<float>(dx * dx + dz * dz)) * m_terrain->getScale();
            float gridDist = sqrt(static_cast<float>(dx * dx + dz * dz));

            if (dist <= m_brushSize) {
                float current = getHeight(centerX + dx, centerZ + dz);
                float targetHeight = m_flattenHeight;

                if (gridDist < innerRadius) {
                    // Inner plateau - flatten
                    float intensity = 0.5f;
                    float newHeight = current + (targetHeight - current) * intensity * m_brushStrength * 0.1f;

                    TerrainEdit edit{centerX + dx, centerZ + dz, current, newHeight};
                    m_currentBatch.edits.push_back(edit);
                    setHeight(centerX + dx, centerZ + dz, newHeight);
                }
            }
        }
    }
}

inline void TerraformingTools::applyCrater(int centerX, int centerZ) {
    int radius = static_cast<int>(m_brushSize / m_terrain->getScale());

    for (int dx = -radius; dx <= radius; ++dx) {
        for (int dz = -radius; dz <= radius; ++dz) {
            float dist = sqrt(static_cast<float>(dx * dx + dz * dz)) * m_terrain->getScale();

            if (dist <= m_brushSize) {
                float normalizedDist = dist / m_brushSize;

                // Crater profile: deep in center, raised rim
                float profile;
                if (normalizedDist < 0.7f) {
                    profile = -cos(normalizedDist * 3.14159f / 0.7f) * 0.5f - 0.5f;  // Depression
                } else {
                    float rimPos = (normalizedDist - 0.7f) / 0.3f;
                    profile = sin(rimPos * 3.14159f) * 0.3f;  // Raised rim
                }

                modifyHeight(centerX + dx, centerZ + dz, profile * m_brushStrength);
            }
        }
    }
}

inline void TerraformingTools::pushUndoBatch() {
    if (m_currentBatch.edits.empty()) return;

    m_undoStack.push_back(m_currentBatch);
    m_redoStack.clear();

    // Trim undo stack
    while (static_cast<int>(m_undoStack.size()) > m_undoLimit) {
        m_undoStack.pop_front();
    }
}

inline void TerraformingTools::undo() {
    if (m_undoStack.empty()) return;

    TerrainEditBatch batch = m_undoStack.back();
    m_undoStack.pop_back();

    applyBatch(batch, true);  // Reverse
    m_redoStack.push_back(batch);
}

inline void TerraformingTools::redo() {
    if (m_redoStack.empty()) return;

    TerrainEditBatch batch = m_redoStack.back();
    m_redoStack.pop_back();

    applyBatch(batch, false);  // Forward
    m_undoStack.push_back(batch);
}

inline void TerraformingTools::applyBatch(const TerrainEditBatch& batch, bool reverse) {
    for (const auto& edit : batch.edits) {
        float height = reverse ? edit.previousHeight : edit.newHeight;
        setHeight(edit.x, edit.z, height);
    }
}

inline void TerraformingTools::renderUI() {
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(320, 450), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Terraforming Tools", &m_visible)) {
        // Tool selection
        ImGui::Text("Tool:");
        ImGui::Separator();

        // Tool buttons
        const TerraformTool tools[] = {
            TerraformTool::RAISE, TerraformTool::LOWER,
            TerraformTool::SMOOTH, TerraformTool::FLATTEN,
            TerraformTool::NOISE, TerraformTool::CRATER
        };

        for (int i = 0; i < 6; ++i) {
            bool selected = (m_currentTool == tools[i]);

            if (selected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.6f, 0.8f, 1.0f));
            }

            if (ImGui::Button(getToolName(tools[i]), ImVec2(95, 30))) {
                m_currentTool = tools[i];
            }

            if (selected) {
                ImGui::PopStyleColor();
            }

            if ((i + 1) % 2 != 0) ImGui::SameLine();
        }

        ImGui::Separator();

        // Brush settings
        ImGui::Text("Brush Settings:");

        ImGui::SliderFloat("Size", &m_brushSize, 1.0f, 100.0f, "%.1f");
        ImGui::SliderFloat("Strength", &m_brushStrength, 0.01f, 5.0f, "%.2f");

        // Shape selection
        const char* shapes[] = {"Circle", "Square", "Diamond"};
        int shapeIdx = static_cast<int>(m_brushShape);
        if (ImGui::Combo("Shape", &shapeIdx, shapes, 3)) {
            m_brushShape = static_cast<BrushShape>(shapeIdx);
        }

        // Falloff selection
        const char* falloffs[] = {"Linear", "Smooth", "Hard Edge", "Gaussian"};
        int falloffIdx = static_cast<int>(m_brushFalloff);
        if (ImGui::Combo("Falloff", &falloffIdx, falloffs, 4)) {
            m_brushFalloff = static_cast<BrushFalloff>(falloffIdx);
        }

        // Flatten height (for flatten tool)
        if (m_currentTool == TerraformTool::FLATTEN || m_currentTool == TerraformTool::PLATEAU) {
            ImGui::Separator();
            ImGui::SliderFloat("Target Height", &m_flattenHeight, 0.0f, 50.0f, "%.1f");

            if (ImGui::Button("Sample Height")) {
                // Would sample terrain height at brush position
                if (m_brushActive) {
                    m_flattenHeight = m_brushPosition.y;
                }
            }
        }

        ImGui::Separator();

        // Undo/redo buttons
        ImGui::Text("History:");

        ImGui::BeginDisabled(!canUndo());
        if (ImGui::Button("Undo", ImVec2(80, 0))) {
            undo();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!canRedo());
        if (ImGui::Button("Redo", ImVec2(80, 0))) {
            redo();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::Text("(%d / %d)", getUndoCount(), m_undoLimit);

        ImGui::Separator();

        // Brush preview info
        if (m_brushActive) {
            ImGui::Text("Brush Position: (%.1f, %.1f, %.1f)",
                m_brushPosition.x, m_brushPosition.y, m_brushPosition.z);
            ImGui::Text("Terrain Height: %.1f", m_brushPosition.y);
        } else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                "Move mouse over terrain");
        }

        // Status
        if (!m_modificationEnabled) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
                "Terrain modification disabled!");
        }

        // Note about API
        ImGui::Separator();
        ImGui::TextWrapped("Note: Full terrain modification requires Terrain API extension. "
                           "Edits are recorded but visual changes depend on Terrain support.");
    }
    ImGui::End();
}

} // namespace ui
