# OrganismEvolution Developer Guide

This comprehensive guide provides developers with the information needed to understand, modify, and extend the OrganismEvolution simulation. The project implements a real-time ecosystem simulation with evolved neural network-driven creatures, procedural terrain, and a DirectX 12 rendering pipeline.

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [Code Organization](#2-code-organization)
3. [Core Systems](#3-core-systems)
4. [Adding New Creature Types](#4-adding-new-creature-types)
5. [Adding New Behaviors](#5-adding-new-behaviors)
6. [Shader Modification Guide](#6-shader-modification-guide)
7. [Build Instructions](#7-build-instructions)
8. [Debugging](#8-debugging)
9. [Testing](#9-testing)
10. [Contribution Guidelines](#10-contribution-guidelines)

---

## 1. Architecture Overview

### High-Level System Diagram

```
+------------------------------------------------------------------+
|                        APPLICATION LAYER                          |
|  +------------------+  +------------------+  +------------------+  |
|  |    main.cpp      |  |  SimulationDash  |  |   DayNightCycle  |  |
|  | (Entry Point +   |  |  (ImGui Debug)   |  |   (Time System)  |  |
|  |  Game Loop)      |  +------------------+  +------------------+  |
+------------------------------------------------------------------+
                               |
+------------------------------------------------------------------+
|                        SIMULATION LAYER                           |
|  +------------------+  +------------------+  +------------------+  |
|  |    Creature      |  |     Terrain      |  |    SpatialGrid   |  |
|  | (Entity Logic)   |  | (World Geometry) |  | (Spatial Queries)|  |
|  +------------------+  +------------------+  +------------------+  |
|  +------------------+  +------------------+  +------------------+  |
|  |   NeuralNetwork  |  | SteeringBehaviors|  |  SensorySystem   |  |
|  | (AI Decision)    |  | (Movement Logic) |  |  (Perception)    |  |
|  +------------------+  +------------------+  +------------------+  |
+------------------------------------------------------------------+
                               |
+------------------------------------------------------------------+
|                        GENETICS LAYER                             |
|  +------------------+  +------------------+  +------------------+  |
|  |     Genome       |  |  DiploidGenome   |  |  GeneticsManager |  |
|  | (Simple Traits)  |  | (Full Genetics)  |  | (Population Mgmt)|  |
|  +------------------+  +------------------+  +------------------+  |
+------------------------------------------------------------------+
                               |
+------------------------------------------------------------------+
|                        GRAPHICS LAYER                             |
|  +------------------+  +------------------+  +------------------+  |
|  |  ForgeEngine RHI |  |  HLSL Shaders    |  |    Animation     |  |
|  | (DX12 Abstraction|  | (GPU Programs)   |  |  (Skeletal/Proc) |  |
|  +------------------+  +------------------+  +------------------+  |
|  +------------------+  +------------------+                       |
|  |  CreatureBuilder |  |  MarchingCubes   |                       |
|  | (Metaball Mesh)  |  | (Isosurface Gen) |                       |
|  +------------------+  +------------------+                       |
+------------------------------------------------------------------+
```

### Core Components and Their Relationships

| Component | Responsibility | Dependencies |
|-----------|---------------|--------------|
| `main.cpp` | Application entry, game loop, DX12 setup | ForgeEngine, All systems |
| `Creature` | Entity state, behavior updates, physics | Genome, NeuralNetwork, Steering |
| `Terrain` | World geometry, height queries, biomes | DX12Device, Perlin noise |
| `NeuralNetwork` | AI decision making, evolved weights | None (self-contained) |
| `SpatialGrid` | O(1) neighbor queries, collision | Creature pointers |
| `Genome` | Genetic traits, mutation, crossover | None |
| `SteeringBehaviors` | Reynolds' flocking, seek, flee | GLM vectors |
| `ForgeEngine RHI` | DirectX 12 abstraction layer | Windows SDK, D3D12 |

### Data Flow in the Simulation

```
Frame Start
    |
    v
+-------------------+
| Input Processing  |  <-- Keyboard/Mouse (Camera, Debug toggles)
+-------------------+
    |
    v
+-------------------+
| Physics Update    |  <-- deltaTime
|   - Terrain query |
|   - Position clamp|
+-------------------+
    |
    v
+-------------------+
| AI Update         |
|   - Gather inputs |  <-- SensorySystem, SpatialGrid
|   - Neural forward|  <-- NeuralNetwork weights
|   - Get outputs   |  --> turnAmount, speed, eat, flee
+-------------------+
    |
    v
+-------------------+
| Behavior Update   |
|   - Steering calc |  <-- SteeringBehaviors
|   - Apply velocity|
|   - Energy update |
+-------------------+
    |
    v
+-------------------+
| Reproduction      |
|   - Fitness eval  |
|   - Crossover     |  <-- Genome, DiploidGenome
|   - Mutation      |
+-------------------+
    |
    v
+-------------------+
| Render            |
|   - Update CBs    |  <-- Constants buffer
|   - Draw terrain  |  <-- Terrain.hlsl
|   - Draw creatures|  <-- Creature.hlsl (instanced)
|   - Draw UI       |  <-- ImGui
+-------------------+
    |
    v
Frame End (Present)
```

### DirectX 12 Rendering Pipeline

The rendering system uses the ForgeEngine RHI abstraction layer, which wraps DirectX 12:

```
+----------------+     +----------------+     +----------------+
| IDevice        | --> | ICommandList   | --> | ISwapchain     |
| (DX12 Context) |     | (Draw Commands)|     | (Frame Buffers)|
+----------------+     +----------------+     +----------------+
        |                     |
        v                     v
+----------------+     +----------------+
| IPipeline      |     | IBuffer        |
| (PSO + Shaders)|     | (VB, IB, CB)   |
+----------------+     +----------------+
```

**Key Pipeline Stages:**

1. **Frame Begin**: Acquire backbuffer, reset command list
2. **Shadow Pass**: Render scene to Cascaded Shadow Maps (CSM)
3. **Main Pass**: Render terrain, creatures (instanced), vegetation
4. **UI Pass**: ImGui overlay rendering
5. **Present**: Swap buffers, sync with GPU

---

## 2. Code Organization

### Directory Structure

```
OrganismEvolution/
|-- src/
|   |-- main.cpp                    # Entry point + game loop (~3600 lines)
|   |-- core/
|   |   |-- DayNightCycle.h         # Time-of-day system
|   |   |-- SaveManager.h           # Save/load functionality
|   |   |-- Serializer.h            # Data serialization
|   |-- entities/
|   |   |-- Creature.h/.cpp         # Main entity class
|   |   |-- CreatureType.h          # Type enums and traits
|   |   |-- CreatureTraits.cpp      # Per-type configurations
|   |   |-- Genome.h/.cpp           # Simple genetic system
|   |   |-- SteeringBehaviors.h/.cpp# Movement algorithms
|   |   |-- SensorySystem.h/.cpp    # Perception system
|   |   |-- genetics/               # Advanced genetics
|   |       |-- DiploidGenome.h/.cpp
|   |       |-- Chromosome.h/.cpp
|   |       |-- Gene.h/.cpp
|   |       |-- Allele.h/.cpp
|   |       |-- Species.h/.cpp
|   |-- ai/
|   |   |-- NeuralNetwork.h/.cpp    # NEAT-style neural nets
|   |   |-- NEATGenome.h/.cpp       # NEAT genome encoding
|   |   |-- BrainModules.h/.cpp     # Modular brain components
|   |   |-- CreatureBrainInterface.h/.cpp
|   |-- environment/
|   |   |-- Terrain.h/.cpp          # Terrain generation/queries
|   |   |-- Food.h/.cpp             # Food entities
|   |   |-- VegetationManager.h/.cpp# Trees, grass, plants
|   |   |-- WeatherSystem.h/.cpp    # Weather effects
|   |   |-- SeasonManager.h/.cpp    # Seasonal changes
|   |-- graphics/
|   |   |-- Camera.h/.cpp           # Camera controller
|   |   |-- Frustum.h/.cpp          # View frustum culling
|   |   |-- procedural/
|   |       |-- CreatureBuilder.h/.cpp   # Metaball creatures
|   |       |-- MarchingCubes.h/.cpp     # Isosurface extraction
|   |       |-- MorphologyBuilder.h/.cpp # Body part assembly
|   |   |-- rendering/
|   |       |-- CreatureMeshCache.h/.cpp # Mesh caching
|   |-- animation/
|   |   |-- Animation.h             # Main animation header
|   |   |-- Skeleton.h              # Bone hierarchy
|   |   |-- Pose.h                  # Animation poses
|   |   |-- IKSolver.h              # Inverse kinematics
|   |   |-- ProceduralLocomotion.h  # Walk/run cycles
|   |-- ui/
|   |   |-- SimulationDashboard.h/.cpp  # ImGui debug panel
|   |   |-- DashboardMetrics.h/.cpp     # Statistics
|   |   |-- NEATVisualizer.h/.cpp       # Neural net display
|   |-- utils/
|   |   |-- SpatialGrid.h/.cpp      # Spatial partitioning
|   |   |-- Random.h/.cpp           # RNG utilities
|   |   |-- PerlinNoise.h/.cpp      # Noise generation
|   |   |-- CommandProcessor.h/.cpp # Console commands
|   |-- physics/
|       |-- Morphology.h/.cpp       # Body physics
|       |-- Locomotion.h/.cpp       # Movement physics
|       |-- FitnessLandscape.h/.cpp # Evolution fitness
|
|-- ForgeEngine/                    # DirectX 12 RHI
|   |-- Core/Public/                # Core types
|   |-- RHI/Public/                 # RHI interfaces
|   |-- RHI/DX12/                   # DX12 implementation
|   |-- Platform/                   # Window management
|   |-- Shaders/                    # Shader compiler
|
|-- shaders/hlsl/                   # HLSL shader files
|   |-- Common.hlsl                 # Shared functions
|   |-- Terrain.hlsl                # Terrain rendering
|   |-- Creature.hlsl               # Creature rendering
|   |-- CreatureSkinned.hlsl        # Skeletal animation
|   |-- Vegetation.hlsl             # Plants/trees
|   |-- Shadow.hlsl                 # Shadow mapping
|   |-- SSAO.hlsl                   # Ambient occlusion
|   |-- Bloom.hlsl                  # Post-process bloom
|
|-- external/                       # Third-party libraries
|   |-- imgui/                      # Dear ImGui
|   |-- implot/                     # ImPlot graphing
|   |-- glm/                        # GLM math library
|
|-- tests/                          # Unit tests
|   |-- test_genome.cpp
|   |-- test_neural_network.cpp
|   |-- test_spatial_grid.cpp
|   |-- animation/
|       |-- test_skeleton.cpp
|       |-- test_pose.cpp
|
|-- Runtime/Shaders/                # Compiled shaders (runtime)
|-- docs/                           # Documentation
```

### Key Source Files and Their Purposes

| File | Purpose | Lines (approx) |
|------|---------|----------------|
| `src/main.cpp` | Complete simulation: DX12 init, game loop, rendering | 3600+ |
| `src/entities/Creature.cpp` | Entity logic, behavior, physics, reproduction | 600+ |
| `src/entities/CreatureType.h` | All creature types, traits, helper functions | 280+ |
| `src/ai/NeuralNetwork.cpp` | Neural network forward pass, plasticity | 300+ |
| `src/utils/SpatialGrid.cpp` | Spatial hashing for O(1) neighbor queries | 150+ |
| `shaders/hlsl/Creature.hlsl` | Instanced creature rendering, PBR lighting | 490+ |
| `shaders/hlsl/Terrain.hlsl` | Terrain with biomes, water, shadows | 400+ |

### Header vs Implementation Files

**Header Files (`.h`):**
- Class declarations and interfaces
- Inline functions (performance-critical code)
- Template definitions
- Constants and enums
- Include guards using `#pragma once`

**Implementation Files (`.cpp`):**
- Function implementations
- Static member initialization
- Platform-specific code
- Heavy template instantiations

**Convention Example:**
```cpp
// Creature.h - Declaration
class Creature {
public:
    void update(float deltaTime, const Terrain& terrain, ...);
    bool isAlive() const { return alive; }  // Inline for performance
private:
    void updateBehaviorHerbivore(...);  // Implemented in .cpp
};

// Creature.cpp - Implementation
void Creature::update(...) { /* heavy logic */ }
void Creature::updateBehaviorHerbivore(...) { /* behavior logic */ }
```

### Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Classes | PascalCase | `Creature`, `NeuralNetwork`, `SpatialGrid` |
| Functions | camelCase | `updateBehavior()`, `getHeight()`, `isAlive()` |
| Member variables | camelCase (no prefix) | `position`, `velocity`, `energy` |
| Private members | m_ prefix | `m_animator`, `m_terrainPtr` |
| Constants | UPPER_SNAKE | `MAX_CREATURES`, `WATER_LEVEL` |
| Enums | PascalCase | `CreatureType::HERBIVORE` |
| Namespaces | lowercase | `animation::`, `genetics::`, `ai::` |
| Files | PascalCase | `Creature.cpp`, `NeuralNetwork.h` |
| HLSL | PascalCase functions | `VSMain`, `PSMain`, `calculatePBR` |

---

## 3. Core Systems

### Entity System (Creature, Food)

The entity system manages all simulation entities. The primary entity is `Creature`.

**Creature Class Structure:**
```cpp
class Creature {
public:
    // Constructors
    Creature(const glm::vec3& position, const Genome& genome, CreatureType type);
    Creature(const glm::vec3& position, const genetics::DiploidGenome& parent1,
             const genetics::DiploidGenome& parent2, CreatureType type);

    // Main update loop
    void update(float deltaTime, const Terrain& terrain,
                const std::vector<glm::vec3>& foodPositions,
                const std::vector<Creature*>& otherCreatures,
                const SpatialGrid* spatialGrid = nullptr,
                const EnvironmentConditions* envConditions = nullptr,
                const std::vector<SoundEvent>* sounds = nullptr);

private:
    // State
    glm::vec3 position, velocity;
    float energy, age;
    bool alive;

    // Systems
    Genome genome;
    genetics::DiploidGenome diploidGenome;
    std::unique_ptr<NeuralNetwork> brain;
    SteeringBehaviors steering;
    SensorySystem sensory;
    animation::CreatureAnimator m_animator;

    // Type-specific behavior
    void updateBehaviorHerbivore(...);
    void updateBehaviorCarnivore(...);
    void updateBehaviorAquatic(...);
    void updateBehaviorFlying(...);
};
```

**Update Flow:**
1. Age and energy decay
2. Sensory system perceives environment
3. Neural network produces action outputs
4. Behavior update based on creature type
5. Physics update (position, terrain collision)
6. Animation update
7. Fitness calculation

### Genome and Genetics

**Simple Genome (`Genome` class):**
- Direct trait storage (size, speed, vision, etc.)
- Simple crossover (averaging or random selection)
- Gaussian mutation

```cpp
class Genome {
public:
    // Physical traits
    float size;           // 0.5 to 2.0
    float speed;          // 5.0 to 20.0
    float visionRange;    // 10.0 to 50.0
    float efficiency;     // 0.5 to 1.5

    // Visual traits
    glm::vec3 color;

    // Neural network weights
    std::vector<float> neuralWeights;

    // Sensory traits (evolvable)
    float visionFOV, visionAcuity, colorPerception;
    float hearingRange, smellRange;
    float camouflageLevel;

    // Flying/Aquatic traits
    float wingSpan, flapFrequency, glideRatio;
    float finSize, tailSize, swimFrequency;

    void mutate(float mutationRate, float mutationStrength);
    void randomize();
};
```

**Diploid Genome System (`genetics::DiploidGenome`):**
- Chromosome pairs with dominance
- Mendelian inheritance
- Genetic load (inbreeding depression)
- Species identification

### Neural Network (NeuralNetwork.h/cpp)

The AI system uses NEAT-style neural networks with evolvable topology.

**Architecture:**
```
INPUTS (10):                    HIDDEN (12):        OUTPUTS (4):
- Target distance      \        +--------+          - Turn amount
- Target angle          \       | Neuron |-----+    - Speed multiplier
- Threat distance        >----->| (tanh) |     +--->- Eat intention
- Threat angle          /       +--------+     |    - Flee urgency
- Secondary distance   /                       |
- Secondary angle                              |
- Energy level                                 |
- Health level                                 |
- Terrain height                               |
- Current speed                                v
```

**Key Features:**
- Xavier/He weight initialization
- Tanh activation (hidden), Sigmoid (output)
- Mutation with weight clipping
- Crossover for sexual reproduction

```cpp
// Forward pass
std::vector<f32> NeuralNetwork::Forward(const std::vector<f32>& inputs) {
    // Input -> Hidden (Tanh)
    for (i32 h = 0; h < HIDDEN_SIZE; h++) {
        f32 sum = biasH[h];
        for (i32 i = 0; i < INPUT_SIZE; i++)
            sum += inputs[i] * weightsIH[i * HIDDEN_SIZE + h];
        hidden[h] = Tanh(sum);
    }

    // Hidden -> Output (Sigmoid for 0-1 range)
    for (i32 o = 0; o < OUTPUT_SIZE; o++) {
        f32 sum = biasO[o];
        for (i32 h = 0; h < HIDDEN_SIZE; h++)
            sum += hidden[h] * weightsHO[h * OUTPUT_SIZE + o];
        outputs[o] = Sigmoid(sum);
    }
    return outputs;
}
```

### Spatial Grid (SpatialGrid.h/cpp)

The spatial grid provides O(1) neighbor queries through spatial hashing.

**Key Features:**
- Fixed-size cell arrays (no dynamic allocation per query)
- Reusable query buffer
- Squared distance comparisons (avoid sqrt)
- Type-filtered queries for predator-prey

```cpp
class SpatialGrid {
public:
    static constexpr int MAX_PER_CELL = 64;

    void clear();
    void insert(Creature* creature);

    // O(1) neighbor query (returns internal buffer reference)
    const std::vector<Creature*>& query(const glm::vec3& position, float radius);

    // Type-filtered query for predator-prey optimization
    const std::vector<Creature*>& queryByType(const glm::vec3& position,
                                               float radius, int creatureType);
private:
    struct Cell {
        Creature* creatures[MAX_PER_CELL];
        int count = 0;
    };

    std::vector<Cell> cells;  // Flat array for cache locality
    float invCellWidth;       // Precomputed for fast division
};
```

**Usage:**
```cpp
// Each frame
spatialGrid.clear();
for (auto& creature : creatures)
    spatialGrid.insert(creature.get());

// In creature update
const auto& neighbors = spatialGrid.query(position, visionRange);
for (Creature* neighbor : neighbors) {
    // Process neighbor
}
```

### Animation System

The animation system supports multiple locomotion types with procedural animation.

**Key Classes:**
- `Skeleton`: Bone hierarchy with transforms
- `SkeletonPose`: Current pose (bone transforms)
- `IKSolver`: Foot placement, look-at
- `ProceduralLocomotion`: Walk cycles, swimming, flying

```cpp
namespace animation {

class CreatureAnimator {
public:
    void initializeBiped(float height);
    void initializeQuadruped(float length, float height);
    void initializeFlying(float wingspan);
    void initializeAquatic(float length);

    void update(float deltaTime);
    void setVelocity(const glm::vec3& velocity);

    const std::vector<glm::mat4>& getSkinningMatrices() const;

private:
    Skeleton m_skeleton;
    SkeletonPose m_pose;
    ProceduralLocomotion m_locomotion;
    IKSystem m_ikSystem;
};

}
```

---

## 4. Adding New Creature Types

### Step-by-Step Guide

Follow these steps to add a new creature type to the simulation.

### Step 1: Modify CreatureType.h

Add your new type to the `CreatureType` enum:

```cpp
// src/entities/CreatureType.h

enum class CreatureType {
    // Existing types...
    APEX_PREDATOR,
    SCAVENGER,

    // Add your new type
    BURROWING,      // New: Underground creature

    // Keep legacy aliases at the end
    HERBIVORE = GRAZER,
    CARNIVORE = APEX_PREDATOR
};
```

Add helper functions if needed:

```cpp
// Helper to check if creature can burrow
inline bool canBurrow(CreatureType type) {
    return type == CreatureType::BURROWING;
}
```

### Step 2: Add Traits in CreatureTraits.cpp

Configure the traits for your new creature type:

```cpp
// src/entities/CreatureTraits.cpp

CreatureTraits CreatureTraits::getTraitsFor(CreatureType type) {
    CreatureTraits traits;
    traits.type = type;

    switch (type) {
        // Existing cases...

        case CreatureType::BURROWING:
            traits.diet = DietType::OMNIVORE_FLEX;
            traits.trophicLevel = TrophicLevel::PRIMARY_CONSUMER;
            traits.attackRange = 1.5f;
            traits.attackDamage = 8.0f;
            traits.fleeDistance = 15.0f;  // Quick to hide
            traits.huntingEfficiency = 0.06f;
            traits.minPreySize = 0.1f;
            traits.maxPreySize = 0.5f;
            traits.isPackHunter = false;
            traits.isHerdAnimal = false;
            traits.isTerritorial = true;  // Defends burrow
            traits.canClimb = false;
            traits.canDigest[0] = true;   // Can eat grass/roots
            traits.canDigest[1] = true;
            traits.canDigest[2] = true;
            traits.parasiteResistance = 0.6f;
            break;

        default:
            break;
    }

    return traits;
}
```

### Step 3: Add Behavior in Creature.cpp

Create a new behavior update function:

```cpp
// src/entities/Creature.cpp

// Add declaration in Creature.h first:
// void updateBehaviorBurrowing(float deltaTime, const Terrain& terrain,
//                              const SpatialGrid* grid);

void Creature::updateBehaviorBurrowing(float deltaTime, const Terrain& terrain,
                                        const SpatialGrid* grid) {
    glm::vec3 steeringForce(0.0f);

    // Check if currently underground
    float terrainHeight = terrain.getHeight(position.x, position.z);
    bool isUnderground = position.y < terrainHeight - 0.5f;

    if (isUnderground) {
        // Underground behavior: move toward food sources
        // Reduced vision, rely on vibration sensing
        float detectRange = genome.vibrationSensitivity * 20.0f;

        // Wander underground
        steeringForce += steering.wander(position, velocity, wanderTarget) * 0.5f;

        // Surface occasionally to find food
        if (energy < maxEnergy * 0.3f) {
            glm::vec3 surfaceTarget = position;
            surfaceTarget.y = terrainHeight + 1.0f;
            steeringForce += steering.seek(position, velocity, surfaceTarget) * 2.0f;
        }
    } else {
        // Above ground: act like small herbivore
        updateBehaviorHerbivore(deltaTime, {}, {}, grid);
        return;
    }

    // Apply steering
    velocity = steering.applyForce(velocity, steeringForce, deltaTime);
}
```

Update the main update function to call your new behavior:

```cpp
void Creature::update(float deltaTime, ...) {
    // ... existing code ...

    // Update behavior based on creature type
    if (type == CreatureType::BURROWING) {
        updateBehaviorBurrowing(deltaTime, terrain, spatialGrid);
        updatePhysics(deltaTime, terrain);
    } else if (type == CreatureType::HERBIVORE) {
        // ... existing ...
    }
}
```

### Step 4: Add Animation Support

Initialize the appropriate animation type:

```cpp
// src/entities/Creature.cpp

void Creature::initializeAnimation() {
    if (!m_animationEnabled) return;

    switch (type) {
        // Existing cases...

        case CreatureType::BURROWING:
            // Quadruped with short legs
            m_animator.initializeQuadruped(genome.size * 0.8f, genome.size * 0.3f);
            break;

        default:
            m_animator.initializeBiped(genome.size);
            break;
    }
}
```

### Step 5: Add Genome Randomization (Optional)

If your creature needs special genetic traits:

```cpp
// src/entities/Genome.cpp

void Genome::randomizeBurrowing() {
    randomize();  // Base randomization

    // Burrowing-specific traits
    size = Random::Float(0.3f, 0.8f);      // Small
    speed = Random::Float(3.0f, 8.0f);     // Moderate speed
    vibrationSensitivity = Random::Float(0.7f, 1.0f);  // High sensitivity
    visionRange = Random::Float(5.0f, 15.0f);  // Limited vision
    efficiency = Random::Float(0.8f, 1.2f);    // Efficient metabolism
}
```

### Step 6: Update Creature Rendering (Optional)

If your creature needs a distinct appearance, modify the creature builder:

```cpp
// In main.cpp or CreatureBuilder.cpp

void CreatureBuilder::BuildCreatureMetaballs(MetaballSystem& metaballs,
                                             f32 size, f32 speed,
                                             CreatureType type) {
    metaballs.Clear();

    if (type == CreatureType::BURROWING) {
        // Compact, streamlined body for burrowing
        metaballs.AddMetaball(Vec3(0, 0, 0), 0.8f * size, 1.0f);  // Body
        metaballs.AddMetaball(Vec3(0.6f * size, -0.1f * size, 0),
                              0.4f * size, 0.7f);  // Head (forward, down)
        metaballs.AddMetaball(Vec3(-0.5f * size, 0, 0),
                              0.3f * size, 0.5f);  // Short tail

        // Strong front claws
        metaballs.AddMetaball(Vec3(0.3f * size, -0.3f * size, 0.3f * size),
                              0.15f * size, 0.4f);
        metaballs.AddMetaball(Vec3(0.3f * size, -0.3f * size, -0.3f * size),
                              0.15f * size, 0.4f);
    } else {
        // ... existing creature types ...
    }
}
```

---

## 5. Adding New Behaviors

### Understanding Steering Behaviors

The steering system implements Craig Reynolds' classic steering behaviors:

```cpp
class SteeringBehaviors {
public:
    // Individual behaviors return steering force vectors
    glm::vec3 seek(pos, vel, target);      // Move toward target
    glm::vec3 flee(pos, vel, threat);      // Move away from threat
    glm::vec3 arrive(pos, vel, target);    // Seek with deceleration
    glm::vec3 wander(pos, vel, wanderTarget); // Random movement
    glm::vec3 pursuit(pos, vel, targetPos, targetVel);  // Intercept
    glm::vec3 evasion(pos, vel, pursuerPos, pursuerVel); // Escape

    // Flocking behaviors (Reynolds' Boids)
    glm::vec3 separate(pos, vel, neighbors); // Avoid crowding
    glm::vec3 align(pos, vel, neighbors);    // Match velocity
    glm::vec3 cohesion(pos, vel, neighbors); // Move to center

    // Combined
    glm::vec3 flock(pos, vel, neighbors,
                    separationWeight, alignmentWeight, cohesionWeight);

    // Apply with limits
    glm::vec3 applyForce(velocity, force, deltaTime);
};
```

### Modifying updateBehavior Functions

To add a new behavior pattern, modify the appropriate `updateBehavior*` function:

```cpp
void Creature::updateBehaviorHerbivore(float deltaTime,
                                        const std::vector<glm::vec3>& foodPositions,
                                        const std::vector<Creature*>& otherCreatures,
                                        const SpatialGrid* grid) {
    glm::vec3 steeringForce(0.0f);

    // Neural network modulation (evolved behavior)
    float fearModifier = 1.0f + m_neuralOutputs.fearMod * 0.5f;
    float socialModifier = 1.0f + m_neuralOutputs.socialMod * 0.3f;

    // === ADD YOUR NEW BEHAVIOR HERE ===

    // Example: Curiosity behavior - investigate novel objects
    if (m_neuralOutputs.explorationMod > 0.5f) {
        // Find unexplored areas using spatial memory
        glm::vec3 curiosityTarget = sensory.getUnexploredDirection(position);
        if (glm::length(curiosityTarget) > 0.1f) {
            steeringForce += steering.seek(position, velocity, curiosityTarget)
                           * m_neuralOutputs.explorationMod * 0.8f;
        }
    }

    // === END NEW BEHAVIOR ===

    // Existing behaviors...
    // 1. Flee from predators
    Creature* nearestPredator = findNearestCreature(otherCreatures,
                                                     CreatureType::CARNIVORE,
                                                     genome.visionRange * fearModifier,
                                                     grid);
    if (nearestPredator) {
        float dist = glm::distance(position, nearestPredator->getPosition());
        float urgency = 1.0f - (dist / (genome.visionRange * fearModifier));
        steeringForce += steering.flee(position, velocity,
                                       nearestPredator->getPosition())
                       * urgency * 3.0f;
        fear = std::min(1.0f, fear + deltaTime * 2.0f);
    }

    // 2. Seek food when hungry
    if (energy < maxEnergy * 0.7f && !foodPositions.empty()) {
        glm::vec3 nearestFood = findNearestFood(foodPositions);
        steeringForce += steering.arrive(position, velocity, nearestFood) * 1.5f;
    }

    // 3. Flock with nearby herbivores
    auto neighbors = getNeighborsOfType(otherCreatures, CreatureType::HERBIVORE,
                                        genome.visionRange * 0.5f, grid);
    if (!neighbors.empty()) {
        steeringForce += steering.flock(position, velocity, neighbors,
                                        1.5f * socialModifier,  // separation
                                        1.0f * socialModifier,  // alignment
                                        1.0f * socialModifier)  // cohesion
                       * 0.8f;
    }

    // 4. Wander when no other goals
    if (glm::length(steeringForce) < 0.1f) {
        steeringForce += steering.wander(position, velocity, wanderTarget) * 0.5f;
    }

    // Apply accumulated steering force
    velocity = steering.applyForce(velocity, steeringForce, deltaTime);
}
```

### Using Neural Network Outputs

Neural network outputs modulate behavior. To add new output dimensions:

1. **Increase OUTPUT_SIZE** in neural network:
```cpp
// main.cpp NeuralNetwork class
static constexpr i32 OUTPUT_SIZE = 5;  // Was 4, now 5
```

2. **Add to CreatureActions struct:**
```cpp
struct CreatureActions {
    f32 turnAmount = 0.0f;
    f32 speedMultiplier = 0.5f;
    f32 eatIntention = 0.0f;
    f32 fleeUrgency = 0.0f;
    f32 curiosityLevel = 0.5f;  // NEW: 0=ignore novel, 1=investigate
};
```

3. **Map output in behavior update:**
```cpp
void Creature::updateNeuralBehavior(const std::vector<glm::vec3>& foodPositions,
                                     const std::vector<Creature*>& otherCreatures) {
    auto inputs = gatherNeuralInputs(foodPositions, otherCreatures);
    auto outputs = brain->Forward(inputs);

    m_neuralOutputs.turnMod = outputs[0] * 2.0f - 1.0f;  // -1 to 1
    m_neuralOutputs.speedMod = outputs[1];               // 0 to 1
    m_neuralOutputs.eatMod = outputs[2];                 // 0 to 1
    m_neuralOutputs.fearMod = outputs[3] * 2.0f - 1.0f;  // -1 to 1
    m_neuralOutputs.curiosityMod = outputs[4];           // NEW: 0 to 1
}
```

### Testing New Behaviors

1. **Console logging:**
```cpp
#if SIMULATION_DEBUG
    DEBUG_LOG("Creature %d: curiosity=%.2f, exploring=%s\n",
              id, m_neuralOutputs.curiosityMod,
              (m_neuralOutputs.explorationMod > 0.5f) ? "yes" : "no");
#endif
```

2. **Visual debugging:**
   - Press F3 to open debug panel
   - Select a creature to see its neural outputs
   - Watch behavior state in real-time

3. **Fitness testing:**
   - Track if new behavior improves survival
   - Monitor reproduction rates
   - Check energy efficiency

---

## 6. Shader Modification Guide

### Shader File Locations

All HLSL shaders are located in `shaders/hlsl/`:

| Shader | Purpose |
|--------|---------|
| `Common.hlsl` | Shared functions, constants |
| `Terrain.hlsl` | Terrain rendering with biomes |
| `Creature.hlsl` | Instanced creature rendering |
| `CreatureSkinned.hlsl` | Skeletal animation |
| `Vegetation.hlsl` | Trees, grass, plants |
| `Shadow.hlsl` | Shadow map generation |
| `SSAO.hlsl` | Screen-space ambient occlusion |
| `Bloom.hlsl` | Post-process bloom |
| `ToneMapping.hlsl` | HDR to LDR conversion |

### HLSL Syntax Basics

**Shader structure:**
```hlsl
// Constant buffer (matches C++ struct layout)
cbuffer FrameConstants : register(b0)
{
    float4x4 viewProjection;  // 64 bytes, 16-byte aligned
    float3 viewPos;           // 12 bytes
    float time;               // 4 bytes (packs after viewPos)
};

// Input structure
struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

// Output structure
struct PSInput
{
    float4 position : SV_POSITION;
    float3 fragPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
};

// Vertex shader
PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = mul(viewProjection, float4(input.position, 1.0));
    output.fragPos = input.position;
    output.normal = input.normal;
    return output;
}

// Pixel shader
float4 PSMain(PSInput input) : SV_TARGET
{
    float3 color = calculateLighting(input.normal, input.fragPos);
    return float4(color, 1.0);
}
```

### Common Modifications

**1. Changing creature color based on type:**
```hlsl
// In Creature.hlsl PSMain()

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 baseColor = input.color;

    // Get creature type from instance data
    float creatureType = input.creatureType;

    // Modify color based on type
    if (creatureType == 1.0) {  // Carnivore
        baseColor = lerp(baseColor, float3(1.0, 0.3, 0.3), 0.3);  // Reddish tint
    } else if (creatureType == 2.0) {  // Flying
        baseColor = lerp(baseColor, float3(0.3, 0.3, 1.0), 0.2);  // Bluish tint
    }

    // Continue with lighting...
    float3 skinColor = generateSkinPattern(input.triplanarUV, input.normal, baseColor);
    // ...
}
```

**2. Adding a new procedural pattern:**
```hlsl
// Add to Creature.hlsl

// Stripe pattern for specific creature types
float3 generateStripePattern(float3 pos, float3 baseColor, float stripeWidth)
{
    // Create stripes along body length (X axis)
    float stripe = sin(pos.x * 3.14159 / stripeWidth);
    stripe = smoothstep(0.3, 0.7, stripe);

    float3 stripeColor = baseColor * 0.6;  // Darker stripe
    return lerp(stripeColor, baseColor, stripe);
}

// Use in PSMain:
float3 skinColor = baseColor;
if (hasStripes) {
    skinColor = generateStripePattern(input.triplanarUV, baseColor, 0.5);
}
```

**3. Modifying lighting:**
```hlsl
// In calculatePBR function, adjust roughness for wet creatures
float adjustedRoughness = roughness;
if (isWet) {
    adjustedRoughness *= 0.5;  // Wet surfaces are shinier
}
```

### Recompiling Shaders

Shaders are compiled at runtime by the ForgeEngine shader compiler. To recompile:

1. **Automatic**: Modify `.hlsl` file, restart application
2. **Manual rebuild**: Delete `build_dx12/` directory, rebuild project

The shader compiler is in `ForgeEngine/Shaders/Private/ShaderCompiler.cpp`.

**Shader loading in main.cpp:**
```cpp
// Shaders are loaded from Runtime/Shaders/
// The build copies shaders to the executable directory
auto vertexShader = device->CreateShader({
    .type = ShaderType::Vertex,
    .source = LoadShaderSource("Runtime/Shaders/Creature.hlsl"),
    .entryPoint = "VSMain"
});
```

### Debugging Shaders

**1. Use debug colors:**
```hlsl
// Visualize normals
return float4(input.normal * 0.5 + 0.5, 1.0);

// Visualize UV coordinates
return float4(input.texCoord, 0.0, 1.0);

// Visualize depth
float depth = input.position.z / input.position.w;
return float4(depth, depth, depth, 1.0);
```

**2. Cascade shadow debugging:**
```hlsl
// In Terrain.hlsl, enable cascade visualization
static const bool SHOW_CASCADE_DEBUG = true;

// Shows red/green/blue/yellow for each shadow cascade
```

**3. PIX/RenderDoc capture:**
- Use PIX for Windows or RenderDoc
- Capture a frame and inspect shader inputs/outputs
- Step through shader execution

**4. Validation layers:**
```cpp
// Enable in DeviceConfig
DeviceConfig config;
config.enableValidation = true;
config.enableGPUValidation = true;  // Slower but catches more errors
```

---

## 7. Build Instructions

### Prerequisites

- **Windows 10/11** (DirectX 12 required)
- **Visual Studio 2019+** with C++ workload
- **Windows SDK** (10.0.19041.0 or later)
- **CMake 3.15+**
- **DirectX 12 compatible GPU**

### MSYS2 Setup (Alternative Build)

For MinGW-based builds (legacy OpenGL support):

```bash
# Install MSYS2 from https://www.msys2.org/

# Open MSYS2 MinGW 64-bit terminal
pacman -Syu
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-glfw
pacman -S mingw-w64-x86_64-glew
```

### CMake Configuration

**Visual Studio (Recommended):**
```powershell
cd C:\Users\andre\Desktop\OrganismEvolution
mkdir build_dx12
cd build_dx12
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

**Command Line (MSVC):**
```powershell
# Open "Developer Command Prompt for VS"
cd C:\Users\andre\Desktop\OrganismEvolution
mkdir build_dx12
cd build_dx12
cmake .. -G "Ninja"
cmake --build . --config Release
```

### Debug vs Release Builds

| Build Type | Flags | Use Case |
|------------|-------|----------|
| Debug | `/Od /Zi /DFORGE_DEBUG=1` | Development, debugging |
| Release | `/O3 /DNDEBUG` | Performance testing, distribution |
| RelWithDebInfo | `/O2 /Zi` | Profiling with symbols |

**Switching builds:**
```powershell
# Debug
cmake --build . --config Debug

# Release
cmake --build . --config Release
```

### Common Build Errors

**1. "D3D12CreateDevice failed"**
- Cause: GPU doesn't support DirectX 12
- Fix: Update GPU drivers, check GPU compatibility

**2. "Cannot find d3d12.lib"**
- Cause: Windows SDK not installed
- Fix: Install Windows SDK via Visual Studio Installer

**3. "Shader compilation failed"**
- Cause: HLSL syntax error
- Fix: Check shader file, look at error message for line number

**4. "LNK2019: unresolved external symbol"**
- Cause: Missing library link
- Fix: Check CMakeLists.txt `target_link_libraries`

**5. "glm/glm.hpp not found"**
- Cause: GLM not in include path
- Fix: Ensure `external/glm/` exists, check CMake include paths

---

## 8. Debugging

### Using the Debug Overlay (F3)

Press **F3** to toggle the debug panel. Features:

**Left Panel:**
- Population statistics (herbivores, carnivores, etc.)
- Generation counter
- Time of day
- Simulation speed controls
- Camera controls

**Right Panel:**
- Selected creature info
- Neural network visualization
- Genome traits
- Energy/health bars
- Behavior state

**Keyboard shortcuts:**
| Key | Action |
|-----|--------|
| F3 | Toggle debug panel |
| F1 | Toggle help overlay |
| Space | Pause/resume simulation |
| +/- | Adjust simulation speed |
| Tab | Cycle selected creature |
| R | Reset camera |

### Console Output

Debug logging is controlled by preprocessor macros:

```cpp
// Enabled in Debug builds automatically
#if SIMULATION_DEBUG
    #define DEBUG_LOG(fmt, ...) printf(fmt, ##__VA_ARGS__)
    #define DEBUG_WARNING(fmt, ...) printf("WARNING: " fmt, ##__VA_ARGS__)
    #define DEBUG_ERROR(fmt, ...) printf("ERROR: " fmt, ##__VA_ARGS__)
#else
    #define DEBUG_LOG(fmt, ...) ((void)0)
    #define DEBUG_WARNING(fmt, ...) ((void)0)
    #define DEBUG_ERROR(fmt, ...) ((void)0)
#endif

// Always-on logging (even in Release)
#define LOG_INFO(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) printf("ERROR: " fmt, ##__VA_ARGS__)
```

**Usage:**
```cpp
DEBUG_LOG("Creature %d: pos=(%.2f, %.2f, %.2f), energy=%.1f\n",
          creature->getID(),
          pos.x, pos.y, pos.z,
          creature->getEnergy());
```

### Performance Profiling

**Built-in metrics:**
```cpp
// In SimulationDashboard
m_metrics.recordFrameTime(deltaTime * 1000.0f);  // ms

// Display in debug panel
ImGui::Text("Frame time: %.2f ms", m_metrics.getAverageFrameTime());
ImGui::Text("FPS: %.1f", 1000.0f / m_metrics.getAverageFrameTime());
```

**SpatialGrid statistics:**
```cpp
size_t totalCreatures = spatialGrid.getTotalCreatures();
size_t maxOccupancy = spatialGrid.getMaxCellOccupancy();
size_t queryCount = spatialGrid.getQueryCount();

DEBUG_LOG("SpatialGrid: %zu creatures, max %zu per cell, %zu queries\n",
          totalCreatures, maxOccupancy, queryCount);
```

**GPU profiling:**
- Use PIX for Windows for GPU profiling
- Enable timestamp queries in ForgeEngine (advanced)

### Common Pitfalls

**1. Creatures stuck at spawn:**
- Check: Neural network outputting zeros
- Fix: Verify weight initialization, add bias

**2. All creatures die immediately:**
- Check: Energy consumption rate
- Fix: Adjust `baseConsumption` in Creature::update()

**3. Creatures not finding food:**
- Check: SpatialGrid query radius vs food positions
- Fix: Verify food spawn positions are within grid bounds

**4. Frame rate drops:**
- Check: SpatialGrid cell count, creature count
- Fix: Increase grid resolution, reduce `MAX_CREATURES`

**5. Rendering artifacts:**
- Check: Vertex buffer stride, constant buffer alignment
- Fix: Verify `static_assert` in vertex structs, check cbuffer padding

---

## 9. Testing

### Running Unit Tests

Tests are located in the `tests/` directory:

```powershell
# Build tests
cd build_dx12
cmake .. -DBUILD_TESTS=ON
cmake --build . --config Debug

# Run all tests
ctest -C Debug --output-on-failure

# Run specific test
.\tests\Debug\test_genome.exe
.\tests\Debug\test_neural_network.exe
.\tests\Debug\test_spatial_grid.exe
```

### Adding New Tests

**Create a new test file:**
```cpp
// tests/test_my_feature.cpp

#include "../src/my_feature/MyFeature.h"
#include <cassert>
#include <iostream>

void testBasicFunctionality() {
    std::cout << "Testing basic functionality..." << std::endl;

    MyFeature feature;
    feature.initialize();

    assert(feature.isValid());
    assert(feature.getValue() > 0);

    std::cout << "  Basic functionality test passed!" << std::endl;
}

void testEdgeCases() {
    std::cout << "Testing edge cases..." << std::endl;

    MyFeature feature;

    // Test with extreme values
    feature.setValue(0.0f);
    assert(feature.getValue() >= 0.0f);

    feature.setValue(1000000.0f);
    assert(feature.getValue() <= MAX_VALUE);

    std::cout << "  Edge cases test passed!" << std::endl;
}

int main() {
    std::cout << "=== MyFeature Unit Tests ===" << std::endl;

    testBasicFunctionality();
    testEdgeCases();

    std::cout << "\n=== All tests passed! ===" << std::endl;
    return 0;
}
```

**Add to CMakeLists.txt:**
```cmake
# In tests/CMakeLists.txt
add_executable(test_my_feature test_my_feature.cpp)
target_link_libraries(test_my_feature ${PROJECT_LIBS})
add_test(NAME MyFeatureTests COMMAND test_my_feature)
```

### Performance Benchmarks

**Genome performance:**
```cpp
// tests/test_performance.cpp

void benchmarkGenomeCrossover() {
    const int iterations = 10000;
    auto start = std::chrono::high_resolution_clock::now();

    Genome parent1, parent2;
    parent1.randomize();
    parent2.randomize();

    for (int i = 0; i < iterations; i++) {
        Genome child(parent1, parent2);
        (void)child;  // Prevent optimization
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Crossover: " << iterations << " iterations in "
              << duration.count() << " us ("
              << (duration.count() / (float)iterations) << " us/op)" << std::endl;
}

void benchmarkSpatialGridQuery() {
    SpatialGrid grid(500.0f, 500.0f, 50);  // 50x50 cells

    // Insert 1000 creatures
    std::vector<std::unique_ptr<Creature>> creatures;
    for (int i = 0; i < 1000; i++) {
        glm::vec3 pos(Random::Float(-250, 250), 0, Random::Float(-250, 250));
        creatures.push_back(std::make_unique<Creature>(pos, Genome(), CreatureType::HERBIVORE));
        grid.insert(creatures.back().get());
    }

    const int queries = 10000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < queries; i++) {
        glm::vec3 queryPos(Random::Float(-250, 250), 0, Random::Float(-250, 250));
        const auto& results = grid.query(queryPos, 30.0f);
        (void)results;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "SpatialGrid query: " << queries << " queries in "
              << duration.count() << " us ("
              << (duration.count() / (float)queries) << " us/query)" << std::endl;
}
```

**Expected benchmarks:**
- Genome crossover: < 5 us per operation
- Neural network forward: < 10 us per pass
- SpatialGrid query: < 1 us per query (with 1000 creatures)

---

## 10. Contribution Guidelines

### Code Style

**General:**
- Use C++20 features where appropriate
- Prefer `const` correctness
- Use RAII for resource management
- Avoid raw `new`/`delete`, use smart pointers

**Formatting:**
```cpp
// Braces on same line for functions
void MyClass::myFunction() {
    if (condition) {
        // Code
    } else {
        // Code
    }
}

// Member initialization lists
MyClass::MyClass(int value)
    : m_value(value)
    , m_initialized(true)
{
}

// Constants in UPPER_SNAKE_CASE
static constexpr float MAX_ENERGY = 200.0f;

// Enums with PascalCase
enum class MyEnum {
    FirstValue,
    SecondValue
};
```

**Comments:**
```cpp
/**
 * @brief Brief description of function.
 *
 * Detailed description if needed.
 *
 * @param param1 Description of parameter
 * @return Description of return value
 */
float calculateSomething(float param1);

// Single-line comments for implementation details
float result = complexCalculation();  // Normalize to 0-1 range
```

### Pull Request Process

1. **Fork the repository**
2. **Create a feature branch:**
   ```bash
   git checkout -b feature/my-new-feature
   ```

3. **Make changes following code style**

4. **Add tests for new functionality**

5. **Run existing tests:**
   ```powershell
   ctest -C Debug --output-on-failure
   ```

6. **Commit with descriptive message:**
   ```bash
   git commit -m "Add new creature type: Burrowing

   - Added BURROWING to CreatureType enum
   - Implemented underground behavior
   - Added quadruped animation for burrowers
   - Added unit tests for new type"
   ```

7. **Push and create PR:**
   ```bash
   git push origin feature/my-new-feature
   ```

8. **Address review feedback**

### Documentation Requirements

**For new features:**
- Update relevant section in DEVELOPER_GUIDE.md
- Add inline code comments explaining complex logic
- Include example usage in PR description

**For bug fixes:**
- Reference issue number in commit message
- Add test case that reproduces the bug
- Document the root cause in PR description

**For API changes:**
- Update header file documentation
- Note breaking changes in PR description
- Provide migration guide if needed

### Testing Requirements

- All new code must have unit tests
- Tests must pass in both Debug and Release
- Performance-critical code needs benchmarks
- Edge cases must be tested (null, empty, boundary values)

### Code Review Checklist

Before submitting:
- [ ] Code compiles without warnings
- [ ] All tests pass
- [ ] No memory leaks (run with validation enabled)
- [ ] Performance is acceptable
- [ ] Documentation is updated
- [ ] Commit messages are descriptive
- [ ] PR description explains the changes

---

## Appendix: Quick Reference

### Important Constants

```cpp
// main.cpp
static constexpr u32 MAX_CREATURES = 2000;
Terrain::WIDTH = 300;
Terrain::DEPTH = 300;
Terrain::SCALE = 1.5f;
Terrain::WATER_LEVEL = 0.0f;

// Creature.h
static constexpr float maxEnergy = 200.0f;
static constexpr float herbivoreReproductionThreshold = 180.0f;
static constexpr float attackRange = 2.5f;
static constexpr float killEnergyGain = 120.0f;

// NeuralNetwork
static constexpr i32 INPUT_SIZE = 10;
static constexpr i32 HIDDEN_SIZE = 12;
static constexpr i32 OUTPUT_SIZE = 4;
```

### File Quick Reference

| Need to... | Edit file |
|------------|-----------|
| Add creature type | `CreatureType.h`, `CreatureTraits.cpp`, `Creature.cpp` |
| Change behavior | `Creature.cpp` (updateBehavior* functions) |
| Modify neural network | `main.cpp` (NeuralNetwork class) |
| Change terrain | `main.cpp` (Terrain class) |
| Modify creature appearance | `main.cpp` (CreatureBuilder), `Creature.hlsl` |
| Add UI element | `SimulationDashboard.cpp` |
| Change lighting | `Terrain.hlsl`, `Creature.hlsl` |

### Debug Keys

| Key | Action |
|-----|--------|
| F3 | Toggle debug panel |
| F1 | Toggle help |
| Space | Pause/resume |
| Tab | Cycle creature selection |
| +/- | Adjust speed |
| R | Reset camera |
| Escape | Exit |

---

*Last updated: January 2026*
*Version: 1.0.0*
