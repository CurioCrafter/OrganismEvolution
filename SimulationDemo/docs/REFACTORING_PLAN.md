# Refactoring Plan

**Project:** OrganismEvolution
**Version:** 1.0
**Priority System:** P1 (Critical) > P2 (High) > P3 (Medium) > P4 (Low)

---

## Overview

This document outlines a prioritized refactoring plan based on the Code Quality Audit findings. Each task includes complexity estimation, dependencies, and specific implementation guidance.

**Complexity Scale:**
- **XS:** < 1 hour, single file change
- **S:** 1-4 hours, 2-3 files
- **M:** 4-8 hours, multiple files, possible API changes
- **L:** 1-2 days, significant restructuring
- **XL:** 2-5 days, major architectural change

---

## Phase 1: Critical Fixes (P1)

### Task 1.1: Fix Raw Pointer Memory Leak ✅ COMPLETED
**Priority:** P1 | **Complexity:** XS | **Risk:** Low
**Status:** Completed on 2026-01-14

**Problem:** `PheromoneGrid* pheromoneGrid` in Simulation.h is a raw pointer with manual delete.

**Files to Modify:**
- [src/core/Simulation.h](../src/core/Simulation.h)
- [src/core/Simulation.cpp](../src/core/Simulation.cpp)

**Changes:**
```cpp
// Simulation.h:112
// BEFORE:
PheromoneGrid* pheromoneGrid;

// AFTER:
std::unique_ptr<PheromoneGrid> pheromoneGrid;

// Simulation.cpp - Remove manual delete in destructor
// BEFORE:
Simulation::~Simulation() {
    // ...
    if (pheromoneGrid) {
        delete pheromoneGrid;
        pheromoneGrid = nullptr;
    }
}

// AFTER:
Simulation::~Simulation() {
    // ...
    // pheromoneGrid automatically cleaned up by unique_ptr
}
```

**Dependencies:** None
**Testing:** Run simulation, verify no memory leaks with Valgrind/AddressSanitizer

---

### Task 1.2: Add RAII Wrappers for Initialization ✅ COMPLETED
**Priority:** P1 | **Complexity:** S | **Risk:** Low
**Status:** Completed on 2026-01-14

**Problem:** Initialization failures in main.cpp can leak resources.

**Files to Create:**
- `src/core/GLFWContext.h`
- `src/core/GLFWContext.cpp`

**Files to Modify:**
- [src/main.cpp](../src/main.cpp)

**Implementation:**
```cpp
// GLFWContext.h
#pragma once
#include <GLFW/glfw3.h>
#include <stdexcept>

class GLFWContext {
public:
    GLFWContext() {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }
    }

    ~GLFWContext() {
        glfwTerminate();
    }

    GLFWContext(const GLFWContext&) = delete;
    GLFWContext& operator=(const GLFWContext&) = delete;
};

class GLFWWindow {
public:
    GLFWWindow(int width, int height, const char* title);
    ~GLFWWindow();

    GLFWwindow* get() const { return window; }
    operator GLFWwindow*() const { return window; }

private:
    GLFWwindow* window;
};

// main.cpp usage:
int main() {
    try {
        GLFWContext glfwContext;  // RAII
        GLFWWindow window(WINDOW_WIDTH, WINDOW_HEIGHT, "OrganismEvolution");
        // ... rest of init
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
}
```

**Dependencies:** None
**Testing:** Test init failure scenarios (bad OpenGL version, missing GPU)

---

### Task 1.3: Consistent Spatial Grid Usage
**Priority:** P1 | **Complexity:** M | **Risk:** Medium

**Problem:** O(n²) neighbor queries in Creature methods don't use SpatialGrid.

**Files to Modify:**
- [src/entities/Creature.h](../src/entities/Creature.h)
- [src/entities/Creature.cpp](../src/entities/Creature.cpp)

**Changes:**
1. Modify `findNearestCreature()` to use SpatialGrid when available
2. Modify `getNeighborsOfType()` to use SpatialGrid
3. Store SpatialGrid pointer in Creature for queries

```cpp
// Creature.h - Add cached spatial grid reference
class Creature {
private:
    const SpatialGrid* currentSpatialGrid = nullptr;

public:
    void setSpatialGrid(const SpatialGrid* grid) { currentSpatialGrid = grid; }
};

// Creature.cpp - Update findNearestCreature
Creature* Creature::findNearestCreature(
    const std::vector<Creature*>& creatures,
    CreatureType targetType,
    float maxRange) const
{
    // Use spatial grid if available
    if (currentSpatialGrid != nullptr) {
        auto nearby = currentSpatialGrid->queryRange(position, maxRange);
        Creature* nearest = nullptr;
        float nearestDist = maxRange;

        for (Creature* other : nearby) {
            if (other == this || !other->isAlive()) continue;
            if (other->getType() != targetType) continue;

            float dist = glm::length(other->getPosition() - position);
            if (dist < nearestDist) {
                nearestDist = dist;
                nearest = other;
            }
        }
        return nearest;
    }

    // Fallback to linear search
    // ... existing code
}
```

**Dependencies:** None
**Testing:** Profile before/after with 200+ creatures

---

## Phase 2: High Priority Improvements (P2)

### Task 2.1: Extract Simulation Responsibilities
**Priority:** P2 | **Complexity:** L | **Risk:** Medium

**Problem:** Simulation.cpp is 1400+ lines handling too many concerns.

**Files to Create:**
- `src/core/PopulationManager.h/cpp`
- `src/core/EnvironmentSystem.h/cpp`
- `src/core/RenderCoordinator.h/cpp`
- `src/debug/DebugVisualization.h/cpp`

**Proposed Structure:**

```cpp
// PopulationManager.h
class PopulationManager {
public:
    void spawnInitialPopulation();
    void spawnCreatures(CreatureType type, int count);
    void balancePopulation();
    void handleReproduction();

    int getHerbivoreCount() const;
    int getCarnivoreCount() const;
    int getAquaticCount() const;
    int getFlyingCount() const;

    std::vector<std::unique_ptr<Creature>>& getCreatures() { return creatures; }

private:
    std::vector<std::unique_ptr<Creature>> creatures;
    Terrain* terrain;  // Non-owning reference

    // Population parameters
    static constexpr int initialHerbivores = 60;
    // ... etc
};

// EnvironmentSystem.h
class EnvironmentSystem {
public:
    void update(float deltaTime);

    const EnvironmentConditions& getConditions() const;
    const SoundManager& getSoundManager() const;
    float getDayNightCycle() const;
    glm::vec3 getSunDirection() const;

private:
    EnvironmentConditions conditions;
    SoundManager soundManager;
    float dayNightCycle;
    float weatherCycle;
};

// RenderCoordinator.h
class RenderCoordinator {
public:
    void init();
    void renderFrame(const Camera& camera, const Simulation& sim);

private:
    void renderShadowPass(const Simulation& sim);
    void renderTerrain();
    void renderCreatures(const Camera& camera);
    void renderVegetation(const Camera& camera);
    void renderUI();

    std::unique_ptr<ShadowMap> shadowMap;
    std::unique_ptr<CreatureRenderer> creatureRenderer;
    // ... etc
};

// Simulation.h - Simplified
class Simulation {
public:
    void init();
    void update(float deltaTime);
    void render(Camera& camera);

private:
    std::unique_ptr<Terrain> terrain;
    std::unique_ptr<VegetationManager> vegetation;
    std::unique_ptr<PopulationManager> population;
    std::unique_ptr<EnvironmentSystem> environment;
    std::unique_ptr<RenderCoordinator> renderer;
    std::unique_ptr<SpatialGrid> spatialGrid;
};
```

**Dependencies:** Tasks 1.1, 1.2
**Testing:** Full regression test of simulation behavior

---

### Task 2.2: Add Exception Handling for Shaders
**Priority:** P2 | **Complexity:** S | **Risk:** Low

**Problem:** Shader loading in main.cpp has no exception handling.

**Files to Modify:**
- [src/main.cpp](../src/main.cpp)

**Changes:**
```cpp
// main.cpp:714-718
// BEFORE:
std::cout << "Loading shaders..." << std::endl;
Shader shader("shaders/vertex.glsl", "shaders/fragment.glsl");
Shader creatureShader("shaders/creature_vertex.glsl", "shaders/creature_fragment.glsl");

// AFTER:
std::cout << "Loading shaders..." << std::endl;
std::unique_ptr<Shader> shader;
std::unique_ptr<Shader> creatureShader;

try {
    shader = std::make_unique<Shader>("shaders/vertex.glsl", "shaders/fragment.glsl");
    creatureShader = std::make_unique<Shader>("shaders/creature_vertex.glsl", "shaders/creature_fragment.glsl");
} catch (const std::exception& e) {
    std::cerr << "[FATAL] Failed to load shaders: " << e.what() << std::endl;
    glfwTerminate();
    return -1;
}
```

**Dependencies:** None
**Testing:** Test with missing/corrupt shader files

---

### Task 2.3: Pre-allocate Frame Vectors
**Priority:** P2 | **Complexity:** S | **Risk:** Low

**Problem:** Vectors allocated every frame in updateCreatures().

**Files to Modify:**
- [src/core/Simulation.h](../src/core/Simulation.h)
- [src/core/Simulation.cpp](../src/core/Simulation.cpp)

**Changes:**
```cpp
// Simulation.h - Add cached vectors
private:
    // Cached vectors for per-frame use (avoid allocations)
    std::vector<glm::vec3> cachedFoodPositions;
    std::vector<Creature*> cachedCreaturePtrs;

// Simulation.cpp:updateCreatures()
void Simulation::updateCreatures(float deltaTime) {
    // Reuse cached vectors
    cachedFoodPositions.clear();
    cachedFoodPositions.reserve(food.size() + vegetation->getTreeInstances().size());

    for (const auto& foodItem : food) {
        if (foodItem->isActive()) {
            cachedFoodPositions.push_back(foodItem->getPosition());
        }
    }

    // Add tree positions
    if (vegetation) {
        for (const auto& tree : vegetation->getTreeInstances()) {
            cachedFoodPositions.push_back(tree.position);
        }
    }

    cachedCreaturePtrs.clear();
    cachedCreaturePtrs.reserve(creatures.size());
    for (auto& creature : creatures) {
        if (creature->isAlive()) {
            cachedCreaturePtrs.push_back(creature.get());
        }
    }

    // ... rest of method using cached vectors
}
```

**Dependencies:** None
**Testing:** Profile memory allocations before/after

---

## Phase 3: Medium Priority (P3)

### Task 3.1: Extract Application Class from main.cpp
**Priority:** P3 | **Complexity:** M | **Risk:** Low

**Problem:** main.cpp is 1000+ lines with global state.

**Files to Create:**
- `src/core/Application.h`
- `src/core/Application.cpp`

**Proposed Structure:**
```cpp
// Application.h
class Application {
public:
    Application(int width, int height, const char* title);
    ~Application();

    void run();

    // Callbacks (static for GLFW)
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void windowFocusCallback(GLFWwindow* window, int focused);

private:
    void initGLFW();
    void initOpenGL();
    void processInput();
    void update(float deltaTime);
    void render();

    GLFWwindow* window;
    Camera camera;

    std::unique_ptr<Simulation> simulation;
    std::unique_ptr<CommandProcessor> commandProcessor;
    std::unique_ptr<DashboardMetrics> dashboardMetrics;
    std::unique_ptr<DashboardUI> dashboardUI;

    // Input state
    float lastX, lastY;
    bool firstMouse;
    bool mouseCaptured;
    bool windowFocused;

    // Timing
    float deltaTime;
    float lastFrame;
    float totalTime;

    // Static instance for callbacks
    static Application* instance;
};

// main.cpp - Simplified
int main() {
    try {
        Application app(1280, 720, "OrganismEvolution");
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}
```

**Dependencies:** Tasks 1.2, 2.2
**Testing:** Full application test

---

### Task 3.2: Replace Magic Numbers with Constants
**Priority:** P3 | **Complexity:** S | **Risk:** Low

**Problem:** Hard-coded values scattered throughout code.

**Files to Create:**
- `src/core/SimulationConstants.h`

**Files to Modify:**
- Multiple files

**Implementation:**
```cpp
// SimulationConstants.h
#pragma once

namespace SimConstants {
    // Terrain
    constexpr int TERRAIN_WIDTH = 300;
    constexpr int TERRAIN_HEIGHT = 300;
    constexpr float TERRAIN_SCALE = 1.5f;

    // Water
    constexpr float WATER_LEVEL = -5.0f;

    // Population Limits
    constexpr int INITIAL_HERBIVORES = 60;
    constexpr int INITIAL_CARNIVORES = 15;
    constexpr int MIN_HERBIVORES = 30;
    constexpr int MAX_HERBIVORES = 150;
    // ... etc

    // Physics
    constexpr float MAX_DELTA_TIME = 0.1f;
    constexpr float DEFAULT_DELTA_TIME = 0.016f;

    // Rendering
    constexpr int WINDOW_WIDTH = 1280;
    constexpr int WINDOW_HEIGHT = 720;
    constexpr int SHADOW_MAP_SIZE = 2048;
}
```

**Dependencies:** None
**Testing:** Compile test, visual verification

---

### Task 3.3: Fix Static Variable in Aquatic Behavior
**Priority:** P3 | **Complexity:** XS | **Risk:** Low

**Problem:** Static `wanderAngle` shared across all aquatic creatures.

**Files to Modify:**
- [src/entities/Creature.h](../src/entities/Creature.h)
- [src/entities/Creature.cpp](../src/entities/Creature.cpp)

**Changes:**
```cpp
// Creature.h - Add member
private:
    float aquaticWanderAngle = 0.0f;

// Creature.cpp:494-495
// BEFORE:
void Creature::updateBehaviorAquatic(/* ... */) {
    // ...
    static float wanderAngle = 0.0f;
    wanderAngle += (Random::range(-0.5f, 0.5f)) * deltaTime * 3.0f;

// AFTER:
void Creature::updateBehaviorAquatic(/* ... */) {
    // ...
    aquaticWanderAngle += (Random::range(-0.5f, 0.5f)) * deltaTime * 3.0f;
```

**Dependencies:** None
**Testing:** Observe aquatic creature behavior diversity

---

## Phase 4: Polish (P4)

### Task 4.1: Standardize Boolean Naming
**Priority:** P4 | **Complexity:** S | **Risk:** Low

**Files to Modify:**
- [src/entities/Creature.h](../src/entities/Creature.h)
- [src/entities/Creature.cpp](../src/entities/Creature.cpp)

**Changes:**
```cpp
// Creature.h
// BEFORE:
bool alive;
bool sterile;
bool beingHunted;

// AFTER:
bool isAlive;  // Or keep 'alive' with getter isAlive()
bool isSterile;
bool isBeingHunted;

// Update all usages in Creature.cpp
```

**Dependencies:** None
**Testing:** Compile test

---

### Task 4.2: Add Member Variable Prefixes (Optional)
**Priority:** P4 | **Complexity:** M | **Risk:** Low

**Note:** This is optional based on team preference. The current style (no prefix) is valid.

If adopting `m_` prefix:

**Files to Modify:**
- All header files with member variables

**Example:**
```cpp
// Creature.h
private:
    glm::vec3 m_position;
    glm::vec3 m_velocity;
    float m_energy;
    bool m_isAlive;
```

**Dependencies:** None
**Testing:** Compile test

---

### Task 4.3: Cache OpenGL Viewport Queries
**Priority:** P4 | **Complexity:** XS | **Risk:** Low

**Files to Modify:**
- [src/core/Simulation.h](../src/core/Simulation.h)
- [src/core/Simulation.cpp](../src/core/Simulation.cpp)

**Changes:**
```cpp
// Simulation.h
private:
    GLint cachedViewport[4];

// Simulation.cpp:render()
void Simulation::render(Camera& camera) {
    // Query once per frame
    glGetIntegerv(GL_VIEWPORT, cachedViewport);

    // Use cachedViewport throughout render methods
    float aspectRatio = (cachedViewport[3] > 0)
        ? (float)cachedViewport[2] / (float)cachedViewport[3]
        : 16.0f / 9.0f;
}
```

**Dependencies:** None
**Testing:** Visual verification

---

## Proposed File Structure After Refactoring

```
src/
├── core/
│   ├── Application.h/cpp          # NEW: Main application class
│   ├── GLFWContext.h/cpp          # NEW: RAII wrappers
│   ├── Simulation.h/cpp           # SIMPLIFIED
│   ├── PopulationManager.h/cpp    # NEW: Extracted from Simulation
│   ├── EnvironmentSystem.h/cpp    # NEW: Extracted from Simulation
│   ├── RenderCoordinator.h/cpp    # NEW: Extracted from Simulation
│   └── SimulationConstants.h      # NEW: Shared constants
├── debug/
│   ├── DebugVisualization.h/cpp   # NEW: Extracted from Simulation
│   └── CommandSystem.h/cpp        # NEW: Extracted from main.cpp
├── entities/
│   ├── Creature.h/cpp             # MODIFIED: Use spatial grid
│   ├── Genome.h/cpp
│   └── genetics/
├── environment/
├── graphics/
├── ai/
├── physics/
├── ui/
└── utils/
```

---

## Implementation Schedule

### Sprint 1 (Week 1): Critical Fixes
- [x] Task 1.1: Fix PheromoneGrid raw pointer ✅
- [x] Task 1.2: Add RAII wrappers ✅
- [ ] Task 1.3: Spatial grid integration

### Sprint 2 (Week 2): High Priority
- [ ] Task 2.1: Extract Simulation responsibilities (start)
- [ ] Task 2.2: Shader exception handling
- [ ] Task 2.3: Pre-allocate frame vectors

### Sprint 3 (Week 3): High Priority (continued)
- [ ] Task 2.1: Extract Simulation responsibilities (complete)
- [ ] Task 3.1: Extract Application class (start)

### Sprint 4 (Week 4): Medium Priority
- [ ] Task 3.1: Extract Application class (complete)
- [ ] Task 3.2: Replace magic numbers
- [ ] Task 3.3: Fix static variable

### Sprint 5 (Week 5): Polish
- [ ] Task 4.1: Boolean naming
- [ ] Task 4.2: Member prefixes (optional)
- [ ] Task 4.3: Cache viewport queries
- [ ] Documentation updates

---

## Verification Checklist

After each task:

- [ ] Code compiles without warnings
- [ ] All existing tests pass
- [ ] Visual verification: simulation runs correctly
- [ ] No memory leaks (Valgrind/AddressSanitizer)
- [ ] Performance not degraded (profile if needed)
- [ ] Code follows CODING_STANDARDS.md

---

## Rollback Plan

Each task should be committed separately with descriptive commit messages:

```
git commit -m "refactor(core): Convert PheromoneGrid to unique_ptr

- Fixes memory leak risk in Simulation destructor
- Removes manual delete call
- Ref: CODE_QUALITY_AUDIT.md Issue 3.1"
```

If issues are found:
1. `git revert <commit>` for single task rollback
2. Create issue documenting the problem
3. Re-approach with smaller incremental changes

---

## Dependencies Graph

```
Task 1.1 ─────────────────────────────────────┐
                                              │
Task 1.2 ─────────────────┐                   │
                          │                   │
Task 1.3 ─────────────────┼───────────────────┤
                          │                   │
Task 2.2 ─────────────────┤                   │
                          │                   │
Task 2.3 ─────────────────┤                   │
                          ▼                   ▼
                     Task 2.1 ────────► Task 3.1
                          │
                          │
                          ▼
               Tasks 3.2, 3.3, 4.x (independent)
```

---

## Success Metrics

| Metric | Before | Target | Measure |
|--------|--------|--------|---------|
| Simulation.cpp lines | ~1400 | < 300 | `wc -l` |
| main.cpp lines | ~1060 | < 100 | `wc -l` |
| Memory leaks | Unknown | 0 | Valgrind |
| Neighbor query complexity | O(n²) | O(n log n) | Profile |
| Frame allocations | Multiple | 0 | Profiler |

---

## References

- [CODE_QUALITY_AUDIT.md](CODE_QUALITY_AUDIT.md) - Detailed issue descriptions
- [CODING_STANDARDS.md](CODING_STANDARDS.md) - Style guide for new code
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
