# SimulationDemo Architecture

SimulationDemo is a DirectX 12 port of OrganismEvolution, leveraging ModularGameEngine's RHI, Platform, and Shaders modules for cross-platform graphics abstraction.

## High-Level System Diagram

```
+-----------------------------------------------------------------------------------+
|                                  APPLICATION LAYER                                 |
+-----------------------------------------------------------------------------------+
|  Platform Module (ModularGameEngine)                                               |
|  +-----------------------------------------------------------------------------+  |
|  |  Window Management  |  Input Handling  |  Event System  |  Time Management  |  |
|  +-----------------------------------------------------------------------------+  |
+-----------------------------------------------------------------------------------+
                                        |
                                        v
+-----------------------------------------------------------------------------------+
|                                  SIMULATION CORE                                   |
+-----------------------------------------------------------------------------------+
|  Simulation                                                                        |
|  +-----------------------------------------------------------------------------+  |
|  |  Entity Management  |  Population Balance  |  Evolution  |  Environment     |  |
|  +-----------------------------------------------------------------------------+  |
|                                                                                    |
|  +------------------+  +------------------+  +------------------+                  |
|  |  EcosystemMgr    |  |  SeasonManager   |  |  SpatialGrid     |                  |
|  +------------------+  +------------------+  +------------------+                  |
+-----------------------------------------------------------------------------------+
                                        |
                                        v
+-----------------------------------------------------------------------------------+
|                                   ENTITY LAYER                                     |
+-----------------------------------------------------------------------------------+
|  +------------------+  +------------------+  +------------------+                  |
|  |    Creature      |  |     Genome       |  |  NeuralNetwork   |                  |
|  |  - Position      |  |  - Size/Speed    |  |  - 4 inputs      |                  |
|  |  - Energy        |  |  - Efficiency    |  |  - 4 hidden      |                  |
|  |  - Age/Gen       |  |  - Sensory       |  |  - 2 outputs     |                  |
|  +------------------+  +------------------+  +------------------+                  |
|           |                     |                     |                            |
|           +---------------------+---------------------+                            |
|                                 |                                                  |
|  +------------------+  +------------------+  +------------------+                  |
|  | SensorySystem    |  | SteeringBehaviors|  |EcosystemBehaviors|                  |
|  | - Vision         |  | - Seek/Flee      |  | - Herbivore AI   |                  |
|  | - Hearing        |  | - Flocking       |  | - Carnivore AI   |                  |
|  | - Smell/Touch    |  | - Wander         |  | - Pack Hunting   |                  |
|  +------------------+  +------------------+  +------------------+                  |
+-----------------------------------------------------------------------------------+
                                        |
                                        v
+-----------------------------------------------------------------------------------+
|                                 GRAPHICS LAYER                                     |
+-----------------------------------------------------------------------------------+
|  RHI Module (ModularGameEngine)                                                    |
|  +-----------------------------------------------------------------------------+  |
|  |  Device  |  CommandQueue  |  SwapChain  |  Pipeline State  |  Root Signature|  |
|  +-----------------------------------------------------------------------------+  |
|                                                                                    |
|  Shaders Module (ModularGameEngine)                                                |
|  +-----------------------------------------------------------------------------+  |
|  |  ShaderCompiler  |  ShaderReflection  |  HLSL Management                    |  |
|  +-----------------------------------------------------------------------------+  |
|                                                                                    |
|  Rendering Components                                                              |
|  +------------------+  +------------------+  +------------------+                  |
|  | TerrainRenderer  |  |CreatureRenderer  |  |VegetationRenderer|                  |
|  +------------------+  +------------------+  +------------------+                  |
+-----------------------------------------------------------------------------------+
```

## Module Dependencies

### Core Dependencies

```
SimulationDemo
    |
    +-- ModularGameEngine::Platform
    |       |-- Window creation and management
    |       |-- Input processing (keyboard, mouse)
    |       |-- Event dispatch system
    |       +-- Timer and delta time
    |
    +-- ModularGameEngine::RHI (Render Hardware Interface)
    |       |-- Device abstraction (DX12 backend)
    |       |-- Command list management
    |       |-- Resource creation (buffers, textures)
    |       |-- Pipeline state objects
    |       +-- Descriptor heap management
    |
    +-- ModularGameEngine::Shaders
            |-- HLSL shader compilation
            |-- Shader reflection
            +-- Root signature generation
```

### Internal Module Dependencies

```
main.cpp (Entry Point)
    |
    +-- Simulation (Core orchestrator)
    |       |-- Creature (entity management)
    |       |       |-- Genome (genetics)
    |       |       |-- NeuralNetwork (AI)
    |       |       |-- SensorySystem (perception)
    |       |       +-- SteeringBehaviors (movement)
    |       |
    |       |-- EcosystemManager
    |       |       |-- ProducerSystem
    |       |       |-- DecomposerSystem
    |       |       +-- EcosystemMetrics
    |       |
    |       |-- SeasonManager
    |       +-- SpatialGrid
    |
    +-- Renderers
            |-- TerrainRenderer
            |-- CreatureRenderer
            |       |-- CreatureMeshCache
            |       +-- CreatureBuilder
            +-- VegetationRenderer
```

## Data Flow

### Input -> Simulation -> Rendering Pipeline

```
+-------------+     +------------------+     +-------------------+
|   INPUT     |     |   SIMULATION     |     |    RENDERING      |
+-------------+     +------------------+     +-------------------+
      |                     |                        |
      v                     v                        v
+-------------+     +------------------+     +-------------------+
| Platform    |     | Update Loop      |     | Frame Begin       |
| Module      |---->| (per frame)      |---->| (clear, set RT)   |
+-------------+     +------------------+     +-------------------+
      |                     |                        |
      v                     v                        v
+-------------+     +------------------+     +-------------------+
| Keyboard:   |     | 1. Env Update    |     | Terrain Pass      |
| W/A/S/D     |     |    - Seasons     |     | - Height map      |
| +/- Speed   |     |    - Weather     |     | - Biome colors    |
| P Pause     |     |                  |     | - Lighting        |
+-------------+     +------------------+     +-------------------+
      |                     |                        |
      v                     v                        v
+-------------+     +------------------+     +-------------------+
| Mouse:      |     | 2. Grid Rebuild  |     | Vegetation Pass   |
| Look around |     |    - Spatial     |     | - Trees           |
| Scroll zoom |     |      partitioning|     | - Bushes          |
+-------------+     +------------------+     +-------------------+
      |                     |                        |
      v                     v                        v
+-------------+     +------------------+     +-------------------+
| Camera      |     | 3. Creature      |     | Creature Pass     |
| Update      |     |    Update        |     | - Instanced draw  |
|             |     |    - Physics     |     | - Per-creature    |
|             |     |    - Sensory     |     |   color/animation |
|             |     |    - AI/Neural   |     |                   |
|             |     |    - Energy      |     |                   |
+-------------+     +------------------+     +-------------------+
                            |                        |
                            v                        v
                    +------------------+     +-------------------+
                    | 4. Reproduction  |     | UI/Nametag Pass   |
                    |    - Crossover   |     |                   |
                    |    - Mutation    |     |                   |
                    +------------------+     +-------------------+
                            |                        |
                            v                        v
                    +------------------+     +-------------------+
                    | 5. Population    |     | Frame End         |
                    |    Balance       |     | (present)         |
                    +------------------+     +-------------------+
```

### Detailed Simulation Update Cycle

```cpp
void Simulation::update(float deltaTime) {
    // 1. Apply time scaling
    float scaledDelta = deltaTime * simulationSpeed;

    // 2. Update environment
    environmentConditions.update(scaledDelta);
    seasonManager.update(scaledDelta);

    // 3. Rebuild spatial partitioning
    spatialGrid.clear();
    for (auto& creature : creatures) {
        if (creature.isAlive()) {
            spatialGrid.insert(creature);
        }
    }

    // 4. Update all creatures
    for (auto& creature : creatures) {
        // Get nearby entities for sensory/behavior
        auto nearby = spatialGrid.getNeighbors(creature.position, creature.visionRange);

        creature.updatePhysics(scaledDelta);
        creature.updateSensory(nearby, environmentConditions);
        creature.updateBehavior(scaledDelta);
        creature.updateEnergy(scaledDelta);
    }

    // 5. Handle reproduction
    processReproduction();

    // 6. Balance populations (Lotka-Volterra)
    balancePopulations();

    // 7. Spawn resources
    spawnFood(scaledDelta);
}
```

### Rendering Data Flow

```
+-------------------+     +-------------------+     +-------------------+
|  Simulation State |     |  Per-Frame Data   |     |   GPU Resources   |
+-------------------+     +-------------------+     +-------------------+
        |                         |                         |
        v                         v                         v
+-------------------+     +-------------------+     +-------------------+
| Creature List     |---->| Instance Buffer   |---->| Structured Buffer |
| - Position        |     | - World Matrix    |     | (SRV)             |
| - Color           |     | - Color           |     |                   |
| - Animation Phase |     | - AnimPhase       |     |                   |
+-------------------+     +-------------------+     +-------------------+
        |                         |                         |
        v                         v                         v
+-------------------+     +-------------------+     +-------------------+
| Terrain Height    |---->| Constant Buffer   |---->| CBV               |
| Camera Position   |     | - ViewProj Matrix |     |                   |
| Light Direction   |     | - LightDir        |     |                   |
+-------------------+     +-------------------+     +-------------------+
```

## Key Configuration Constants

| Parameter | Value | Description |
|-----------|-------|-------------|
| Initial Herbivores | 60 | Starting herbivore population |
| Initial Carnivores | 15 | Starting carnivore population |
| Max Food Items | 150 | Maximum food entities |
| Mutation Rate | 10% | Probability of gene mutation |
| Mutation Strength | 0.15 | Magnitude of mutations |
| Simulation Speed | 0.25x - 8.0x | Time scale multiplier |
| World Size | 300x300 | Terrain grid dimensions |

## Threading Model

```
+------------------+     +------------------+     +------------------+
|   Main Thread    |     |   Render Thread  |     |  Worker Threads  |
+------------------+     +------------------+     +------------------+
| - Window events  |     | - Command list   |     | - Spatial grid   |
| - Input polling  |     |   recording      |     |   updates        |
| - Simulation     |     | - Resource       |     | - Creature AI    |
|   orchestration  |     |   uploads        |     |   (future)       |
| - Frame timing   |     | - Present        |     |                  |
+------------------+     +------------------+     +------------------+
```

## File Organization

```
SimulationDemo/
├── src/
│   ├── main.cpp                    # Entry point, window setup
│   ├── core/
│   │   └── Simulation.h/cpp        # Main simulation orchestrator
│   ├── entities/
│   │   ├── Creature.h/cpp          # Creature entity
│   │   ├── Genome.h/cpp            # Genetic traits
│   │   ├── NeuralNetwork.h/cpp     # AI brain
│   │   ├── SensorySystem.h/cpp     # Perception
│   │   └── SteeringBehaviors.h/cpp # Movement
│   ├── environment/
│   │   ├── Terrain.h/cpp           # World terrain
│   │   ├── EcosystemManager.h/cpp  # Ecosystem coordination
│   │   └── SeasonManager.h/cpp     # Seasonal changes
│   ├── graphics/
│   │   ├── rendering/
│   │   │   ├── TerrainRenderer.h/cpp
│   │   │   └── CreatureRenderer.h/cpp
│   │   └── mesh/
│   │       └── MeshData.h          # Vertex definitions
│   └── utils/
│       ├── SpatialGrid.h/cpp       # Spatial partitioning
│       └── CommandProcessor.h/cpp  # Debug commands
├── shaders/
│   ├── terrain.hlsl                # Terrain rendering
│   ├── creature.hlsl               # Creature rendering
│   └── vegetation.hlsl             # Tree/bush rendering
└── docs/
    ├── ARCHITECTURE.md             # This file
    ├── CONTROLS.md                 # Input documentation
    ├── CREATURES.md                # Creature system docs
    ├── RENDERING.md                # Graphics pipeline docs
    └── KNOWN_ISSUES.md             # Bug tracking
```
