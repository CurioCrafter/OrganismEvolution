# Coding Standards Guide

**Project:** OrganismEvolution
**Version:** 1.0
**Based On:** Google C++ Style Guide, Unreal Engine Standards, C++ Core Guidelines

---

## Table of Contents

1. [Naming Conventions](#1-naming-conventions)
2. [File Organization](#2-file-organization)
3. [Code Formatting](#3-code-formatting)
4. [Memory Management](#4-memory-management)
5. [Error Handling](#5-error-handling)
6. [Documentation](#6-documentation)
7. [Performance Guidelines](#7-performance-guidelines)
8. [OpenGL/Graphics Guidelines](#8-openglgraphics-guidelines)

---

## 1. Naming Conventions

### 1.1 General Rules

| Element | Convention | Example |
|---------|------------|---------|
| Classes/Structs | PascalCase | `CreatureRenderer`, `SpatialGrid` |
| Functions/Methods | camelCase | `updateBehavior()`, `getPosition()` |
| Variables | camelCase | `deltaTime`, `creatureCount` |
| Constants | UPPER_SNAKE_CASE | `MAX_CREATURES`, `DEFAULT_SPEED` |
| Namespaces | lowercase | `genetics`, `ai` |
| File names | PascalCase | `Creature.cpp`, `SpatialGrid.h` |

### 1.2 Prefix Conventions

```cpp
// Global variables: g_ prefix
extern float g_deltaTime;
extern bool g_debugMode;

// Member variables: m_ prefix (optional but recommended for clarity)
class Creature {
private:
    glm::vec3 m_position;
    float m_energy;
};

// OR: no prefix with explicit this-> usage (current project style)
class Creature {
private:
    glm::vec3 position;
    float energy;

    void update() {
        this->position += velocity;  // Explicit this-> for clarity
    }
};

// Static member variables: s_ prefix
class Creature {
private:
    static int s_nextID;
};

// OpenGL handles: no prefix, but grouped
GLuint vao, vbo, ebo;  // Vertex data
GLuint fbo, rbo;       // Framebuffer objects
GLuint texture;        // Textures
```

### 1.3 Boolean Naming

```cpp
// Use is/has/can/should prefixes for booleans
bool isAlive;
bool hasFood;
bool canReproduce;
bool shouldRender;

// NOT:
bool alive;      // Ambiguous
bool food;       // Noun, not condition
bool reproduce;  // Verb, not condition
```

### 1.4 Function Naming

```cpp
// Getters: use get prefix or property-style
const glm::vec3& getPosition() const;
float energy() const;  // Property-style for simple accessors

// Setters: use set prefix
void setPosition(const glm::vec3& pos);

// Queries (return bool): use is/has/can
bool isAlive() const;
bool hasEnergy() const;
bool canReproduce() const;

// Actions: use verb
void update(float deltaTime);
void render(const Camera& camera);
void consumeFood(float amount);

// Factory methods: use create
static std::unique_ptr<Creature> createHerbivore(const glm::vec3& pos);
static std::unique_ptr<Creature> createFromGenome(const Genome& genome);
```

### 1.5 Type Aliases

```cpp
// Use using (not typedef)
using CreaturePtr = std::unique_ptr<Creature>;
using CreatureList = std::vector<CreaturePtr>;
using CreatureID = uint32_t;

// For template aliases
template<typename T>
using Vector3 = glm::tvec3<T>;
```

---

## 2. File Organization

### 2.1 Directory Structure

```
src/
├── core/           # Application, Simulation, State management
├── entities/       # Game objects (Creature, Food)
│   └── genetics/   # Genetic system components
├── environment/    # World systems (Terrain, Weather, Ecosystem)
├── graphics/       # Rendering systems
│   ├── mesh/       # Mesh data structures
│   ├── procedural/ # Procedural generation
│   └── rendering/  # Renderers, batching
├── ai/             # AI systems (Neural networks, Behaviors)
├── physics/        # Physics and movement
├── ui/             # User interface
└── utils/          # Utilities (Math, Random, Containers)

external/           # Third-party libraries
shaders/            # GLSL shaders
docs/               # Documentation
tests/              # Unit tests (recommended addition)
```

### 2.2 Header File Organization

```cpp
// Creature.h
#pragma once  // Use pragma once (widely supported)

// 1. Standard library includes (alphabetical)
#include <memory>
#include <vector>

// 2. Third-party includes (alphabetical)
#include <GL/glew.h>
#include <glm/glm.hpp>

// 3. Project includes (alphabetical)
#include "Genome.h"
#include "NeuralNetwork.h"

// 4. Forward declarations
class Terrain;
class SpatialGrid;

// 5. Class declaration
class Creature {
public:
    // Types and nested classes first
    enum class State { Idle, Moving, Eating, Fleeing };

    // Constructors/Destructors
    Creature(const glm::vec3& position, const Genome& genome);
    ~Creature();

    // Deleted/defaulted special members
    Creature(const Creature&) = delete;
    Creature& operator=(const Creature&) = delete;
    Creature(Creature&&) = default;
    Creature& operator=(Creature&&) = default;

    // Public methods (grouped by functionality)
    void update(float deltaTime);
    void render(const Camera& camera);

    // Getters
    const glm::vec3& getPosition() const;
    float getEnergy() const;

    // Setters
    void setPosition(const glm::vec3& pos);

private:
    // Private methods
    void updatePhysics(float deltaTime);
    void calculateFitness();

    // Member variables (grouped logically)
    // -- Transform
    glm::vec3 position;
    glm::vec3 velocity;
    float rotation;

    // -- State
    float energy;
    bool alive;
    State currentState;

    // -- Components
    Genome genome;
    std::unique_ptr<NeuralNetwork> brain;

    // -- Static members
    static int nextID;

    // -- Constants
    static constexpr float MAX_ENERGY = 200.0f;
};
```

### 2.3 Source File Organization

```cpp
// Creature.cpp

// 1. Corresponding header first
#include "Creature.h"

// 2. Other includes
#include "../environment/Terrain.h"
#include "../utils/Random.h"
#include <algorithm>
#include <iostream>

// 3. Anonymous namespace for file-local helpers
namespace {
    float clampEnergy(float value) {
        return std::clamp(value, 0.0f, 200.0f);
    }
}  // namespace

// 4. Static member initialization
int Creature::nextID = 1;

// 5. Implementation in declaration order
Creature::Creature(const glm::vec3& position, const Genome& genome)
    : position(position)
    , velocity(0.0f)
    , rotation(0.0f)
    , energy(100.0f)
    , alive(true)
    , currentState(State::Idle)
    , genome(genome)
    , brain(std::make_unique<NeuralNetwork>(genome.neuralWeights))
{
    // Constructor body
}
```

---

## 3. Code Formatting

### 3.1 Indentation and Spacing

```cpp
// Use 4 spaces (not tabs)
void Creature::update(float deltaTime) {
    if (alive) {
        // 4-space indent
        updatePhysics(deltaTime);
    }
}

// Opening brace on same line (K&R style)
if (condition) {
    // ...
}

// Exception: Function definitions may use either
void Creature::update(float deltaTime) {  // Same line
    // ...
}

// OR
void Creature::update(float deltaTime)
{  // New line
    // ...
}
```

### 3.2 Line Length

```cpp
// Maximum 120 characters per line
// Break long lines logically

// Function with many parameters
Creature::Creature(
    const glm::vec3& position,
    const Genome& genome,
    CreatureType type,
    int generation
)
    : position(position)
    , genome(genome)
    , type(type)
    , generation(generation)
{
}

// Long conditionals
if (creature->isAlive() &&
    creature->getEnergy() > REPRODUCTION_THRESHOLD &&
    creature->getAge() > MATURITY_AGE)
{
    // ...
}
```

### 3.3 Bracing

```cpp
// Always use braces, even for single statements
if (alive) {
    update();
}

// NOT:
if (alive)
    update();  // Dangerous: easy to introduce bugs

// Exception: Guard clauses may be single line
if (!alive) return;
if (energy <= 0.0f) return;
```

---

## 4. Memory Management

### 4.1 Smart Pointer Rules

```cpp
// Rule 1: Use unique_ptr for ownership
class Simulation {
    std::unique_ptr<Terrain> terrain;           // Owned by Simulation
    std::vector<std::unique_ptr<Creature>> creatures;  // Owned collection
};

// Rule 2: Use raw pointers for non-owning references (observers)
void updateCreatures(const std::vector<Creature*>& neighbors) {
    // neighbors doesn't own the creatures
}

// Rule 3: Use shared_ptr ONLY when ownership is truly shared
class Resource {
    std::shared_ptr<Texture> texture;  // Multiple objects share this texture
};

// Rule 4: Never use new/delete directly
// BAD:
Creature* c = new Creature();
delete c;

// GOOD:
auto c = std::make_unique<Creature>();
```

### 4.2 RAII Patterns

```cpp
// Use RAII for OpenGL resources
class VAOGuard {
public:
    VAOGuard(GLuint vao) { glBindVertexArray(vao); }
    ~VAOGuard() { glBindVertexArray(0); }

    VAOGuard(const VAOGuard&) = delete;
    VAOGuard& operator=(const VAOGuard&) = delete;
};

// Usage:
void render() {
    VAOGuard guard(vao);  // Binds VAO
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
}  // VAO unbound automatically

// RAII for GLFW
class GLFWContext {
public:
    GLFWContext() {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }
    }
    ~GLFWContext() { glfwTerminate(); }
};
```

### 4.3 Container Guidelines

```cpp
// Prefer reserve() for vectors with known size
std::vector<Creature*> neighbors;
neighbors.reserve(expectedCount);  // Avoid reallocations

// Use emplace_back for in-place construction
creatures.emplace_back(std::make_unique<Creature>(position, genome));

// Clear and reserve for reuse (avoid allocation)
cachedPositions.clear();
if (cachedPositions.capacity() < food.size()) {
    cachedPositions.reserve(food.size());
}
```

---

## 5. Error Handling

### 5.1 Error Handling Strategy

```cpp
// Use exceptions for unrecoverable errors
void Shader::compile() {
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        throw std::runtime_error(std::string("Shader compile failed: ") + infoLog);
    }
}

// Use return values for expected failures
std::optional<Creature*> findNearestPrey() {
    if (creatures.empty()) {
        return std::nullopt;
    }
    // ...
    return nearestPrey;
}

// Use assertions for programmer errors (debug only)
void Creature::update(float deltaTime) {
    assert(deltaTime >= 0.0f && "Negative delta time");
    assert(alive && "Updating dead creature");
    // ...
}
```

### 5.2 Logging Guidelines

```cpp
// Use log levels
enum class LogLevel { Debug, Info, Warning, Error };

// Format: [LEVEL][SYSTEM] Message
std::cout << "[INFO][Simulation] Spawned " << count << " creatures" << std::endl;
std::cerr << "[ERROR][Shader] Failed to compile: " << error << std::endl;
std::cout << "[DEBUG][Creature] Position: " << pos.x << ", " << pos.y << std::endl;

// Use conditional logging for performance
#ifdef _DEBUG
    std::cout << "[DEBUG] Expensive debug info: " << computeDebugInfo() << std::endl;
#endif
```

### 5.3 Graceful Degradation

```cpp
// Allow features to fail gracefully
void Simulation::initShadowMapping() {
    try {
        shadowMap = std::make_unique<ShadowMap>();
        shadowShader = std::make_unique<Shader>(/* ... */);
        shadowsEnabled = true;
    } catch (const std::exception& e) {
        std::cerr << "[WARNING] Shadow mapping unavailable: " << e.what() << std::endl;
        shadowMap.reset();
        shadowShader.reset();
        shadowsEnabled = false;
        // Continue without shadows
    }
}
```

---

## 6. Documentation

### 6.1 Header Comments

```cpp
/**
 * @file Creature.h
 * @brief Defines the Creature class representing a living organism in the simulation.
 *
 * Creatures are the core entities in the evolution simulation. Each creature has:
 * - A genome defining physical and behavioral traits
 * - A neural network brain for decision making
 * - Sensory systems for perceiving the environment
 * - Steering behaviors for movement
 */
```

### 6.2 Class Documentation

```cpp
/**
 * @class Creature
 * @brief A living organism in the evolution simulation.
 *
 * Creatures can be herbivores, carnivores, aquatic, or flying types.
 * They evolve through genetic algorithms with neural network brains.
 *
 * @note Thread Safety: Not thread-safe. All access must be from main thread.
 * @note Memory: Owns its NeuralNetwork brain via unique_ptr.
 *
 * @see Genome for genetic traits
 * @see SteeringBehaviors for movement
 * @see SensorySystem for perception
 */
class Creature { /* ... */ };
```

### 6.3 Function Documentation

```cpp
/**
 * @brief Updates the creature's state for one simulation step.
 *
 * Performs the following in order:
 * 1. Updates age and cooldowns
 * 2. Consumes energy based on metabolism
 * 3. Processes sensory input
 * 4. Executes behavior based on creature type
 * 5. Updates physics/position
 *
 * @param deltaTime Time elapsed since last update in seconds.
 * @param terrain Reference to terrain for height/water queries.
 * @param foodPositions Positions of all food items.
 * @param otherCreatures Non-owning pointers to other creatures.
 * @param spatialGrid Optional spatial partitioning for efficient queries.
 * @param envConditions Optional environment data (day/night, weather).
 * @param sounds Optional sound events for hearing.
 *
 * @pre deltaTime > 0
 * @pre this->isAlive() == true
 * @post energy may decrease
 * @post position may change
 */
void update(
    float deltaTime,
    const Terrain& terrain,
    const std::vector<glm::vec3>& foodPositions,
    const std::vector<Creature*>& otherCreatures,
    const SpatialGrid* spatialGrid = nullptr,
    const EnvironmentConditions* envConditions = nullptr,
    const std::vector<SoundEvent>* sounds = nullptr
);
```

### 6.4 Inline Comments

```cpp
void Creature::updateBehaviorHerbivore(float deltaTime) {
    glm::vec3 steeringForce(0.0f);

    // Priority 1: FLEE from predators (highest priority)
    Creature* predator = findNearestCreature(creatures, CreatureType::CARNIVORE);
    if (predator != nullptr) {
        float distance = glm::length(predator->getPosition() - position);

        if (distance < genome.visionRange * 0.8f) {
            // Use evasion to predict predator movement
            steeringForce += steering.evasion(position, velocity,
                predator->getPosition(), predator->getVelocity()) * 2.5f;

            // Adrenaline boost: temporarily increase max speed
            SteeringBehaviors::Config config = steering.getConfig();
            config.maxSpeed = genome.speed * 1.4f;  // 40% speed boost
            steering.setConfig(config);
        }
    }

    // ... rest of method
}
```

---

## 7. Performance Guidelines

### 7.1 Avoid Allocations in Hot Paths

```cpp
// BAD: Allocates every frame
void update() {
    std::vector<Creature*> neighbors;  // Allocation!
    findNeighbors(neighbors);
}

// GOOD: Reuse allocated memory
class Simulation {
    std::vector<Creature*> cachedNeighbors;  // Persistent

    void update() {
        cachedNeighbors.clear();  // No deallocation
        findNeighbors(cachedNeighbors);
    }
};
```

### 7.2 Use Spatial Partitioning

```cpp
// BAD: O(n²) for neighbor queries
Creature* findNearest(const std::vector<Creature*>& all) {
    for (Creature* other : all) {  // O(n)
        // ...
    }
}

// GOOD: O(1) average with spatial grid
Creature* findNearest(const SpatialGrid& grid, float range) {
    auto nearby = grid.query(position, range);  // O(1) average
    // ...
}
```

### 7.3 Cache OpenGL State

```cpp
// BAD: Redundant queries
void render() {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);  // Query
    renderTerrain();

    glGetIntegerv(GL_VIEWPORT, viewport);  // Same query again!
    renderCreatures();
}

// GOOD: Query once, cache
class Renderer {
    GLint cachedViewport[4];

    void beginFrame() {
        glGetIntegerv(GL_VIEWPORT, cachedViewport);
    }

    void render() {
        renderTerrain(cachedViewport);
        renderCreatures(cachedViewport);
    }
};
```

### 7.4 Batch OpenGL Calls

```cpp
// BAD: One draw call per creature
for (const auto& creature : creatures) {
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &creature->getModelMatrix()[0][0]);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);  // Many draw calls!
}

// GOOD: Instanced rendering
void CreatureRenderer::render(const std::vector<Creature*>& creatures) {
    updateInstanceBuffer(creatures);  // Upload all transforms at once
    glDrawArraysInstanced(GL_TRIANGLES, 0, vertexCount, creatures.size());  // Single call
}
```

---

## 8. OpenGL/Graphics Guidelines

### 8.1 Shader Uniform Management

```cpp
// Cache uniform locations
class Shader {
    std::unordered_map<std::string, GLint> uniformCache;

    GLint getUniformLocation(const std::string& name) {
        auto it = uniformCache.find(name);
        if (it != uniformCache.end()) {
            return it->second;
        }
        GLint loc = glGetUniformLocation(programID, name.c_str());
        uniformCache[name] = loc;
        return loc;
    }
};
```

### 8.2 Vertex Attribute Layout

```cpp
// Use consistent attribute locations
// Location 0: Position (always)
// Location 1: Normal
// Location 2: TexCoord/Color
// Location 3-6: Instance model matrix (mat4)
// Location 7+: Instance custom data

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in mat4 aInstanceModel;  // Uses 3, 4, 5, 6
layout (location = 7) in vec3 aInstanceColor;
```

### 8.3 Resource Cleanup

```cpp
// Always pair creation with cleanup
class Mesh {
public:
    Mesh() {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
    }

    ~Mesh() {
        if (vao != 0) {
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ebo);
        }
    }

    // Prevent copying (would create dangling handles)
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Allow moving
    Mesh(Mesh&& other) noexcept
        : vao(other.vao), vbo(other.vbo), ebo(other.ebo) {
        other.vao = other.vbo = other.ebo = 0;
    }

private:
    GLuint vao = 0, vbo = 0, ebo = 0;
};
```

---

## Appendix: Tool Configuration

### clang-format Configuration

```yaml
# .clang-format
BasedOnStyle: Google
IndentWidth: 4
ColumnLimit: 120
BreakBeforeBraces: Attach
AllowShortIfStatementsOnASingleLine: Never
AllowShortLoopsOnASingleLine: false
```

### clang-tidy Configuration

```yaml
# .clang-tidy
Checks: >
  -*,
  bugprone-*,
  cppcoreguidelines-*,
  modernize-*,
  performance-*,
  readability-*,
  -modernize-use-trailing-return-type,
  -readability-magic-numbers

CheckOptions:
  - { key: readability-identifier-naming.ClassCase, value: CamelCase }
  - { key: readability-identifier-naming.FunctionCase, value: camelBack }
  - { key: readability-identifier-naming.VariableCase, value: camelBack }
  - { key: readability-identifier-naming.GlobalConstantCase, value: UPPER_CASE }
```

---

## References

1. **Google C++ Style Guide**
   https://google.github.io/styleguide/cppguide.html

2. **Unreal Engine Coding Standards**
   https://docs.unrealengine.com/en-US/ProductionPipelines/DevelopmentSetup/CodingStandard/

3. **C++ Core Guidelines**
   https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines

4. **OpenGL Best Practices**
   https://www.khronos.org/opengl/wiki/Common_Mistakes

5. **Game Programming Patterns**
   https://gameprogrammingpatterns.com/
