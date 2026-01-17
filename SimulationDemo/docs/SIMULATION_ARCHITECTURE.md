# Simulation Architecture Documentation

## System Overview

This document analyzes the architecture of the OrganismEvolution simulation, providing a comprehensive overview of system design, data flow, and coupling analysis.

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                           MAIN APPLICATION LAYER                              │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │   GLFW      │  │   Input     │  │   Camera    │  │  Dashboard  │         │
│  │   Window    │──│  Processing │──│   Control   │──│    UI       │         │
│  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘         │
└──────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│                           SIMULATION CORE                                     │
│                                                                               │
│  ┌──────────────────────────────────────────────────────────────────────┐   │
│  │                        Simulation Class                               │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                   │   │
│  │  │   update()  │  │  render()   │  │   init()    │                   │   │
│  │  └──────┬──────┘  └──────┬──────┘  └─────────────┘                   │   │
│  │         │                │                                            │   │
│  │         ▼                ▼                                            │   │
│  │  ┌─────────────────────────────────────────────────────────────┐     │   │
│  │  │                    Subsystem Updates                         │     │   │
│  │  │  • updateEnvironmentConditions()                            │     │   │
│  │  │  • spatialGrid->clear() + insert()                          │     │   │
│  │  │  • updateCreatures()                                        │     │   │
│  │  │  • processCreatureSounds()                                  │     │   │
│  │  │  • updateFood()                                             │     │   │
│  │  │  • handleReproduction()                                     │     │   │
│  │  │  • handleEvolution()                                        │     │   │
│  │  │  • balancePopulation()                                      │     │   │
│  │  └─────────────────────────────────────────────────────────────┘     │   │
│  └──────────────────────────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────────────────────────┘
                                      │
                     ┌────────────────┼────────────────┐
                     ▼                ▼                ▼
┌─────────────────────────┐ ┌─────────────────────────┐ ┌─────────────────────────┐
│      ENTITY LAYER       │ │    ENVIRONMENT LAYER    │ │     GRAPHICS LAYER      │
│                         │ │                         │ │                         │
│  ┌───────────────────┐  │ │  ┌───────────────────┐  │ │  ┌───────────────────┐  │
│  │     Creature      │  │ │  │     Terrain       │  │ │  │  CreatureRenderer │  │
│  │  ┌─────────────┐  │  │ │  │  ┌─────────────┐  │  │ │  │  ┌─────────────┐  │  │
│  │  │   Genome    │  │  │ │  │  │ HeightMap   │  │  │ │  │  │ MeshCache   │  │  │
│  │  │   Brain     │  │  │ │  │  │ Water Zones │  │  │ │  │  │ Instancing  │  │  │
│  │  │  Sensory    │  │  │ │  │  └─────────────┘  │  │ │  │  └─────────────┘  │  │
│  │  │  Steering   │  │  │ │  └───────────────────┘  │ │  └───────────────────┘  │
│  │  └─────────────┘  │  │ │                         │ │                         │
│  └───────────────────┘  │ │  ┌───────────────────┐  │ │  ┌───────────────────┐  │
│                         │ │  │  VegetationMgr    │  │ │  │    ShadowMap      │  │
│  ┌───────────────────┐  │ │  │  ┌─────────────┐  │  │ │  │  ┌─────────────┐  │  │
│  │      Food         │  │ │  │  │ Trees/Grass │  │  │ │  │  │ Depth FBO   │  │  │
│  └───────────────────┘  │ │  │  │ L-Systems   │  │  │ │  │  │ Light Space │  │  │
│                         │ │  │  └─────────────┘  │  │ │  │  └─────────────┘  │  │
│  ┌───────────────────┐  │ │  └───────────────────┘  │ │  └───────────────────┘  │
│  │   SpatialGrid     │  │ │                         │ │                         │
│  │  ┌─────────────┐  │  │ │  ┌───────────────────┐  │ │  ┌───────────────────┐  │
│  │  │Cell-based   │  │  │ │  │  SoundManager     │  │ │  │    Shaders        │  │
│  │  │ queries     │  │  │ │  │  EnvironConditions│  │ │  │  ┌─────────────┐  │  │
│  │  └─────────────┘  │  │ │  └───────────────────┘  │ │  │  │ Creature    │  │  │
│  └───────────────────┘  │ │                         │ │  │  │ Vegetation  │  │  │
└─────────────────────────┘ └─────────────────────────┘ │  │  │ Shadow      │  │  │
                                                        │  │  │ Nametag     │  │  │
                                                        │  │  └─────────────┘  │  │
                                                        │  └───────────────────┘  │
                                                        └─────────────────────────┘
```

## Data Flow Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                            FRAME UPDATE FLOW                                 │
└─────────────────────────────────────────────────────────────────────────────┘

    Time                    Actions                         Data Flow
    ────                    ───────                         ─────────
      │
      ▼
 ┌────────┐
 │ Frame  │─────────────────────────────────────────────────────────────────┐
 │ Start  │                                                                 │
 └────┬───┘                                                                 │
      │                                                                     │
      ▼                                                                     │
 ┌────────────────┐    ┌──────────────────────────────────┐                │
 │ Process Input  │───▶│ Camera position/rotation updated │                │
 │ (non-blocking) │    │ Simulation speed adjusted        │                │
 └────────┬───────┘    └──────────────────────────────────┘                │
          │                                                                 │
          ▼                                                                 │
 ┌────────────────────┐    ┌────────────────────────────────────────────┐  │
 │ Update Environment │───▶│ Day/night cycle → visibility, temperature  │  │
 │ Conditions         │    │ Weather cycle → wind direction/speed       │  │
 └────────┬───────────┘    └────────────────────────────────────────────┘  │
          │                                                                 │
          ▼                                                                 │
 ┌────────────────────┐    ┌────────────────────────────────────────────┐  │
 │ Rebuild Spatial    │───▶│ Clear grid → Insert all alive creatures    │  │
 │ Grid               │    │ O(n) rebuild, enables O(1) neighbor query  │  │
 └────────┬───────────┘    └────────────────────────────────────────────┘  │
          │                                                                 │
          ▼                                                                 │
 ┌────────────────────┐                                                    │
 │ Update Creatures   │    ┌──────────────────────────────────────────────┐│
 │ ├─ Herbivores      │───▶│ For each creature:                          ││
 │ ├─ Carnivores      │    │   1. Query nearby creatures (via grid)      ││
 │ ├─ Aquatic         │    │   2. Process sensory input                  ││
 │ └─ Flying          │    │   3. Run behavior AI (steering, hunting)    ││
 └────────┬───────────┘    │   4. Update physics (position, velocity)    ││
          │                │   5. Calculate fitness                       ││
          │                │   6. Check energy/death                      ││
          │                └──────────────────────────────────────────────┘│
          ▼                                                                 │
 ┌────────────────────┐    ┌────────────────────────────────────────────┐  │
 │ Process Creature   │───▶│ Alarm calls → Sound events added           │  │
 │ Sounds             │    │ Mating calls → Sound propagation           │  │
 └────────┬───────────┘    └────────────────────────────────────────────┘  │
          │                                                                 │
          ▼                                                                 │
 ┌────────────────────┐    ┌────────────────────────────────────────────┐  │
 │ Handle Reproduction│───▶│ Check canReproduce() → Create offspring    │  │
 │                    │    │ Genome mutation → New creature added       │  │
 └────────┬───────────┘    └────────────────────────────────────────────┘  │
          │                                                                 │
          ▼                                                                 │
 ┌────────────────────┐    ┌────────────────────────────────────────────┐  │
 │ Balance Population │───▶│ Lotka-Volterra dynamics                    │  │
 │                    │    │ Spawn/cull to maintain equilibrium         │  │
 └────────┬───────────┘    └────────────────────────────────────────────┘  │
          │                                                                 │
          ▼                                                                 │
 ┌────────────────────┐                                                    │
 │ RENDER PHASE       │    ┌──────────────────────────────────────────────┐│
 │ ├─ Shadow Pass     │───▶│ Render depth from light perspective        ││
 │ ├─ Terrain         │    │ Terrain with shadow receiving              ││
 │ ├─ Vegetation      │    │ Trees, bushes, grass with shadows          ││
 │ ├─ Creatures       │    │ Instanced rendering with animation         ││
 │ ├─ Nametags        │    │ Billboard quads with IDs                   ││
 │ └─ Debug Markers   │    │ Optional position/camera lines             ││
 └────────┬───────────┘    └──────────────────────────────────────────────┘│
          │                                                                 │
          ▼                                                                 │
 ┌────────────────────┐    ┌────────────────────────────────────────────┐  │
 │ Dashboard UI       │───▶│ ImGui overlay with metrics                 │  │
 │ (ImGui)            │    │ Population graphs, creature inspector      │  │
 └────────┬───────────┘    └────────────────────────────────────────────┘  │
          │                                                                 │
          ▼                                                                 │
 ┌────────┐                                                                 │
 │ Swap   │◀────────────────────────────────────────────────────────────────┘
 │Buffers │
 └────────┘
```

## System Responsibilities

### 1. Main Application (`main.cpp`)

**Responsibilities:**
- Window creation and OpenGL context management
- Input processing and camera control
- Frame timing with delta capping
- Coordinating update and render calls
- Debug visualization controls (F1-F5 keys)
- MCP command processing integration

**Key Design Decisions:**
- Uses chrono for timing instead of glfwGetTime for better precision
- Caps deltaTime to 100ms to prevent physics explosion after window unfocus
- Separates test mode rendering for debugging shader issues

### 2. Simulation Core (`Simulation.cpp/.h`)

**Responsibilities:**
- Owns all game entities (creatures, food, terrain)
- Orchestrates update sequence
- Manages population dynamics (Lotka-Volterra balancing)
- Coordinates rendering pipeline

**Key Data Members:**
```cpp
std::unique_ptr<Terrain> terrain;
std::unique_ptr<VegetationManager> vegetation;
std::vector<std::unique_ptr<Creature>> creatures;
std::vector<std::unique_ptr<Food>> food;
std::unique_ptr<SpatialGrid> spatialGrid;
```

### 3. Creature Entity (`Creature.cpp/.h`)

**Responsibilities:**
- Individual creature state and behavior
- Physics integration
- AI decision-making via behavior type
- Energy/fitness tracking
- Combat (predator-prey interactions)

**Components:**
- `Genome` - Genetic traits (speed, vision, color, size)
- `DiploidGenome` - Advanced genetic system with alleles
- `NeuralNetwork` - Brain for decision-making
- `SteeringBehaviors` - Movement behaviors (seek, flee, wander)
- `SensorySystem` - Perception of environment

### 4. Spatial Grid (`SpatialGrid.cpp/.h`)

**Responsibilities:**
- O(1) spatial queries for nearby creatures
- Cell-based partitioning of world space
- Optimized neighbor lookups for behavior AI

**Interface:**
```cpp
void clear();
void insert(Creature* creature);
std::vector<Creature*> query(const glm::vec3& pos, float radius) const;
Creature* findNearest(const glm::vec3& pos, float maxRadius, int typeFilter) const;
```

### 5. Rendering Systems

**CreatureRenderer:**
- GPU instanced rendering of creatures
- Per-type mesh caching
- Animation via shader uniforms

**ShadowMap:**
- Cascaded shadow mapping from sun direction
- Depth buffer for shadow testing
- Light-space matrix calculation

## Coupling Analysis

### Tight Coupling (Areas for Improvement)

1. **Creature ↔ Simulation**
   - Creatures receive `std::vector<Creature*>` directly
   - Requires knowledge of other creature internals for hunting/fleeing
   - **Recommendation:** Use spatial queries returning abstract interfaces

2. **main.cpp ↔ Simulation**
   - Main directly accesses simulation internals via getters
   - Dashboard metrics pulls raw creature/food vectors
   - **Recommendation:** Event-based notification system

3. **Rendering ↔ Entity State**
   - Shader uniforms tightly coupled to Genome structure
   - Color, size, animation phase passed per-instance
   - **Recommendation:** Render component separation

### Loose Coupling (Good Patterns)

1. **SpatialGrid ↔ Creatures**
   - Only stores pointers, doesn't own entities
   - Rebuilt each frame - stateless design

2. **Shaders ↔ Application**
   - Generic uniform setters via Shader class
   - No shader code in entity classes

3. **VegetationManager ↔ Terrain**
   - Queries terrain for placement decisions
   - Owns its own rendering resources

## Interface Definitions

### Simulation Public Interface

```cpp
// Lifecycle
void init();
void update(float deltaTime);
void render(Camera& camera);

// Control
void togglePause();
void increaseSpeed();
void decreaseSpeed();

// Statistics
int getPopulation() const;
int getHerbivoreCount() const;
int getCarnivoreCount() const;
float getAverageFitness() const;

// Data Access (for UI/metrics)
std::vector<Creature*> getCreatures() const;
std::vector<Food*> getFood() const;
const Terrain* getTerrain() const;
```

### Creature Update Interface

```cpp
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

## Performance Considerations

### Current Bottlenecks

1. **Creature Update Loop** - O(n) per creature for neighbor queries without spatial grid
2. **Spatial Grid Rebuild** - O(n) clear and insert every frame
3. **Rendering** - Instanced batching helps, but per-creature uniform updates
4. **Population Balance** - Sorting creatures by fitness for culling

### Current Optimizations

1. **Spatial Partitioning** - Grid reduces neighbor queries from O(n²) to O(n*k) where k is local density
2. **Instanced Rendering** - Single draw call per creature type
3. **Shadow Map Caching** - Reuses depth buffer across frames
4. **Mesh Caching** - Preloaded archetype meshes in CreatureMeshCache

## External References

- [Game Programming Patterns - Game Loop](https://gameprogrammingpatterns.com/game-loop.html)
- [Game Programming Patterns - Spatial Partition](https://gameprogrammingpatterns.com/spatial-partition.html)
- [Game Programming Patterns - Update Method](https://gameprogrammingpatterns.com/update-method.html)
- [Fix Your Timestep! - Gaffer On Games](https://gafferongames.com/post/fix_your_timestep/)
- [ECS FAQ - Sander Mertens](https://github.com/SanderMertens/ecs-faq)
