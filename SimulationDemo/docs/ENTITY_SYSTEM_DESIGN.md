# Entity System Design Document

## Overview

This document analyzes our current entity structure against Entity-Component-System (ECS) patterns and data-oriented design principles. We examine trade-offs between our object-oriented creature design and pure ECS approaches, with recommendations for scalability.

## Current Entity Structure Analysis

### Creature Class Structure

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           CREATURE CLASS                                     │
│  (Object-Oriented / Monolithic Design)                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐             │
│  │    Identity     │  │    Transform    │  │    Physics      │             │
│  │  ─────────────  │  │  ─────────────  │  │  ─────────────  │             │
│  │  int id         │  │  vec3 position  │  │  vec3 velocity  │             │
│  │  int generation │  │  float rotation │  │  (no mass/accel)│             │
│  │  CreatureType   │  │                 │  │                 │             │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘             │
│                                                                              │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐             │
│  │    Genetics     │  │      AI         │  │    Sensory      │             │
│  │  ─────────────  │  │  ─────────────  │  │  ─────────────  │             │
│  │  Genome         │  │  NeuralNetwork* │  │  SensorySystem  │             │
│  │  DiploidGenome  │  │  SteeringBehavs │  │  fear level     │             │
│  │  sterile flag   │  │  wanderTarget   │  │  beingHunted    │             │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘             │
│                                                                              │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐             │
│  │   Lifecycle     │  │    Combat       │  │    Fitness      │             │
│  │  ─────────────  │  │  ─────────────  │  │  ─────────────  │             │
│  │  float energy   │  │  huntCooldown   │  │  float fitness  │             │
│  │  float age      │  │  int killCount  │  │  int foodEaten  │             │
│  │  bool alive     │  │  attackRange    │  │  distTraveled   │             │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘             │
│                                                                              │
│  METHODS (behavior embedded in class):                                       │
│  ├── update(dt, terrain, food, creatures, grid, env, sounds)                │
│  ├── updatePhysics(dt, terrain)                                             │
│  ├── updateBehaviorHerbivore/Carnivore/Aquatic/Flying(...)                  │
│  ├── updateSensoryBehavior(dt, creatures)                                   │
│  ├── attack(target, dt)                                                     │
│  ├── takeDamage(damage)                                                     │
│  ├── consumeFood(amount)                                                    │
│  ├── reproduce(energyCost)                                                  │
│  ├── calculateFitness()                                                     │
│  └── emitAlarmCall/MatingCall(soundBuffer)                                  │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Memory Layout (Current OOP)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  ARRAY OF CREATURES (std::vector<std::unique_ptr<Creature>>)               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  Heap: [ ptr1 ]──▶[ Creature 1 data ............................. ]        │
│        [ ptr2 ]──▶[ Creature 2 data ............................. ]        │
│        [ ptr3 ]──▶[ Creature 3 data ............................. ]        │
│        [ ptr4 ]──▶[ Creature 4 data ............................. ]        │
│          ...                                                                 │
│                                                                              │
│  PROBLEM: Pointer chasing, poor cache locality                              │
│  Each creature access = potential cache miss                                │
│                                                                              │
│  Cache line (64 bytes) might contain:                                       │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │ [position.x][position.y][position.z][velocity.x][velocity.y]...     │  │
│  │ ...[rotation][wanderTarget.x]...[genome data spans multiple lines]  │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                                                              │
│  When iterating for physics ONLY, we load unused genome, AI, sensory data  │
└─────────────────────────────────────────────────────────────────────────────┘
```

## ECS Architecture Pattern

### Core Concept

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  ENTITY-COMPONENT-SYSTEM (ECS) ARCHITECTURE                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ENTITIES: Just IDs (uint32_t)                                              │
│  ┌───────────────────────────────────────────────────────────────┐         │
│  │  Entity 0 │ Entity 1 │ Entity 2 │ Entity 3 │ Entity 4 │ ...  │         │
│  └───────────────────────────────────────────────────────────────┘         │
│                                                                              │
│  COMPONENTS: Contiguous arrays of plain data                                │
│  ┌───────────────────────────────────────────────────────────────┐         │
│  │  Position[0] │ Position[1] │ Position[2] │ Position[3] │ ... │ Cache   │
│  └───────────────────────────────────────────────────────────────┘ Friendly│
│  ┌───────────────────────────────────────────────────────────────┐         │
│  │  Velocity[0] │ Velocity[1] │ Velocity[2] │ Velocity[3] │ ... │         │
│  └───────────────────────────────────────────────────────────────┘         │
│  ┌───────────────────────────────────────────────────────────────┐         │
│  │  Genome[0]   │ Genome[1]   │ Genome[2]   │ Genome[3]   │ ... │         │
│  └───────────────────────────────────────────────────────────────┘         │
│                                                                              │
│  SYSTEMS: Logic operating on components                                      │
│  ┌───────────────────────────────────────────────────────────────┐         │
│  │  PhysicsSystem::update()                                      │         │
│  │    for each entity with (Position, Velocity):                 │         │
│  │        position += velocity * dt                              │         │
│  └───────────────────────────────────────────────────────────────┘         │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### ECS Memory Layout (Optimal)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  STRUCTURE OF ARRAYS (SoA) - Cache Optimal                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  Position array (contiguous):                                               │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │ [p0.x p0.y p0.z] [p1.x p1.y p1.z] [p2.x p2.y p2.z] [p3.x p3.y p3.z] │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│  Cache line: [ pos0 | pos1 | pos2 | pos3 | pos4 | ... ]                    │
│  Perfect prefetching for linear iteration!                                  │
│                                                                              │
│  Velocity array (contiguous):                                               │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │ [v0.x v0.y v0.z] [v1.x v1.y v1.z] [v2.x v2.y v2.z] [v3.x v3.y v3.z] │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                                                              │
│  When PhysicsSystem iterates:                                               │
│    - Reads positions sequentially (cache hits)                              │
│    - Reads velocities sequentially (cache hits)                             │
│    - Writes positions sequentially (cache hits)                             │
│    - Never loads Genome, AI, or Sensory data                                │
│                                                                              │
│  D1 cache miss rate: ~0.8% (from Austin Morlan's 10,000 entity demo)       │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Component Recommendations

### Proposed Component Breakdown

```cpp
// IDENTITY (required by all)
struct EntityID {
    uint32_t id;
    uint16_t generation;  // For handle validation
    uint8_t type;         // CreatureType enum
    uint8_t flags;        // alive, sterile, etc.
};

// TRANSFORM (required by all)
struct Transform {
    glm::vec3 position;
    float rotation;       // Y-axis rotation
};

// PHYSICS (all moving entities)
struct Physics {
    glm::vec3 velocity;
    glm::vec3 acceleration;
    float mass;
    float drag;
};

// RENDER DATA (all visible entities)
struct RenderData {
    glm::vec3 color;
    float size;
    uint8_t meshType;     // Index into mesh cache
    uint8_t animPhase;
};

// GENOME (creatures only)
struct GeneticTraits {
    float speed;
    float visionRange;
    float size;
    float metabolicRate;
    // ... other heritable traits
};

// LIFECYCLE (creatures only)
struct Lifecycle {
    float energy;
    float age;
    float maxAge;
    float fitness;
};

// AI STATE (creatures with behavior)
struct AIState {
    glm::vec3 targetPosition;
    uint32_t targetEntity;    // Entity ID of prey/mate
    uint8_t behaviorState;    // Wandering, Hunting, Fleeing, etc.
    float stateTimer;
};

// SENSORY (creatures with perception)
struct SensoryState {
    float fear;
    float visibility;
    uint8_t nearbyThreats;
    uint8_t nearbyFood;
};

// COMBAT (predators and fighters)
struct CombatState {
    float attackCooldown;
    uint16_t killCount;
    float attackDamage;
    float attackRange;
};

// REPRODUCTION (breeding creatures)
struct ReproductionState {
    float matingCooldown;
    uint16_t offspringCount;
    bool readyToMate;
};
```

### System Definitions

```cpp
// Each system operates on specific component combinations
class PhysicsSystem {
    // Requires: Transform, Physics
    void update(float dt) {
        for (auto entity : view<Transform, Physics>()) {
            auto& pos = get<Transform>(entity);
            auto& phys = get<Physics>(entity);

            phys.velocity += phys.acceleration * dt;
            phys.velocity *= (1.0f - phys.drag * dt);
            pos.position += phys.velocity * dt;
        }
    }
};

class AISystem {
    // Requires: Transform, AIState, (optional) SensoryState
    void update(float dt, SpatialGrid& grid) {
        for (auto entity : view<Transform, AIState>()) {
            auto& pos = get<Transform>(entity);
            auto& ai = get<AIState>(entity);

            switch (ai.behaviorState) {
                case WANDERING: updateWander(entity, dt); break;
                case HUNTING:   updateHunt(entity, dt, grid); break;
                case FLEEING:   updateFlee(entity, dt, grid); break;
                case EATING:    updateEat(entity, dt); break;
            }
        }
    }
};

class CombatSystem {
    // Requires: Transform, CombatState, Lifecycle
    void update(float dt, SpatialGrid& grid) {
        for (auto entity : view<Transform, CombatState, Lifecycle>()) {
            auto& combat = get<CombatState>(entity);
            combat.attackCooldown -= dt;

            if (combat.attackCooldown <= 0.0f) {
                tryAttackNearby(entity, grid);
            }
        }
    }
};

class ReproductionSystem {
    // Requires: Lifecycle, ReproductionState, GeneticTraits
    void update(float dt) {
        for (auto entity : view<Lifecycle, ReproductionState, GeneticTraits>()) {
            auto& life = get<Lifecycle>(entity);
            auto& repro = get<ReproductionState>(entity);

            repro.matingCooldown -= dt;
            repro.readyToMate = (life.energy > REPRO_THRESHOLD)
                             && (repro.matingCooldown <= 0.0f);
        }
    }
};

class RenderSystem {
    // Requires: Transform, RenderData
    void render(Camera& cam, float alpha) {
        for (auto entity : view<Transform, RenderData>()) {
            auto& tf = get<Transform>(entity);
            auto& rd = get<RenderData>(entity);

            // Batch by mesh type for instanced rendering
            instanceBatches[rd.meshType].add(tf.position, rd.color, rd.size);
        }

        for (auto& batch : instanceBatches) {
            batch.render(cam);
        }
    }
};
```

## Data Layout Optimization

### Array of Structures vs Structure of Arrays

```cpp
// CURRENT: Array of Structures (AoS)
struct Creature {
    glm::vec3 position;
    glm::vec3 velocity;
    Genome genome;
    float energy;
    // ... all data together
};
std::vector<Creature> creatures;

// Access pattern: creatures[i].position.x

// OPTIMIZED: Structure of Arrays (SoA)
struct CreatureData {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> velocities;
    std::vector<Genome> genomes;
    std::vector<float> energies;
    // ... arrays of each field
};

// Access pattern: data.positions[i].x
```

### Hybrid Approach (Recommended)

```cpp
// Group frequently-accessed-together data
struct CreatureHot {
    glm::vec3 position;
    glm::vec3 velocity;
    float rotation;
    float size;
};  // 40 bytes - fits in cache line

struct CreatureCold {
    Genome genome;
    DiploidGenome diploidGenome;
    std::unique_ptr<NeuralNetwork> brain;
};  // Larger, accessed less frequently

struct CreatureStore {
    std::vector<CreatureHot> hot;   // Contiguous, cache-friendly
    std::vector<CreatureCold> cold; // Separate allocation
    std::vector<uint8_t> alive;     // Bitfield for alive status
};
```

## Cache-Friendly Iteration Patterns

### Good: Linear Iteration

```cpp
// Excellent cache behavior
void updateAllPositions(float dt) {
    for (size_t i = 0; i < positions.size(); i++) {
        positions[i] += velocities[i] * dt;
    }
}

// SIMD-friendly version
void updateAllPositionsSIMD(float dt) {
    // Process 4 vec3s at a time with AVX
    for (size_t i = 0; i < positions.size(); i += 4) {
        __m256 pos = _mm256_load_ps(&positions[i].x);
        __m256 vel = _mm256_load_ps(&velocities[i].x);
        __m256 dtv = _mm256_set1_ps(dt);
        pos = _mm256_fmadd_ps(vel, dtv, pos);
        _mm256_store_ps(&positions[i].x, pos);
    }
}
```

### Bad: Random Access via Pointers

```cpp
// Poor cache behavior
void updateViaPointers() {
    for (Creature* c : creaturePointers) {
        c->position += c->velocity * dt;  // Cache miss per creature
    }
}
```

### Good: Index-Based Access

```cpp
// Better cache behavior with indices
void updateViaIndices(const std::vector<uint32_t>& activeIndices) {
    for (uint32_t idx : activeIndices) {
        positions[idx] += velocities[idx] * dt;
    }
}
```

## Scalability Analysis

### Current Performance Characteristics

| Metric | Current Value | Notes |
|--------|---------------|-------|
| Max creatures | ~300 | Before noticeable slowdown |
| Update complexity | O(n²) | Without spatial grid |
| Update complexity | O(n·k) | With spatial grid (k = local density) |
| Memory per creature | ~2KB | Including all components |
| Cache efficiency | Low | Pointer chasing |

### Projected with ECS

| Metric | Projected Value | Improvement |
|--------|-----------------|-------------|
| Max creatures | ~10,000+ | 30x+ |
| Update complexity | O(n) | Per system |
| Memory per creature | ~200-500B | Depends on components |
| Cache efficiency | High | Linear iteration |

### Bottleneck Comparison

```
CURRENT BOTTLENECKS:
┌─────────────────────────────────────────────────────────────────────────────┐
│  1. Spatial Grid Rebuild: O(n) per frame                                    │
│  2. Neighbor Queries: O(k) per creature, k = nearby count                   │
│  3. Rendering: O(n) draw calls if not instanced                             │
│  4. Memory Access: Random due to pointer-based storage                      │
└─────────────────────────────────────────────────────────────────────────────┘

ECS BOTTLENECKS (shifted):
┌─────────────────────────────────────────────────────────────────────────────┐
│  1. Entity Lookup: O(1) with dense arrays, O(log n) with sparse sets       │
│  2. Component Addition/Removal: May require array reorganization            │
│  3. Cross-System Dependencies: Requires careful ordering                    │
│  4. Debugging: Harder to inspect entity state                               │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Migration Strategy

### Phase 1: Data Separation (Non-Breaking)

```cpp
// Keep Creature class but reorganize internal storage
class Creature {
private:
    // Hot data - accessed every frame
    struct HotData {
        glm::vec3 position;
        glm::vec3 velocity;
        float rotation;
        float energy;
        bool alive;
    } hot;

    // Cold data - accessed occasionally
    struct ColdData {
        Genome genome;
        DiploidGenome diploidGenome;
        std::unique_ptr<NeuralNetwork> brain;
        SteeringBehaviors steering;
        SensorySystem sensory;
    } cold;

public:
    // Existing interface unchanged
    const glm::vec3& getPosition() const { return hot.position; }
    // ...
};
```

### Phase 2: System Extraction (Gradual)

```cpp
// Extract update logic into system classes
class PhysicsSystem {
public:
    static void updateCreature(Creature& c, float dt, const Terrain& terrain) {
        // Move physics logic from Creature::updatePhysics
    }
};

// In Simulation::updateCreatures
void Simulation::updateCreatures(float dt) {
    for (auto& creature : creatures) {
        if (!creature->isAlive()) continue;
        PhysicsSystem::updateCreature(*creature, dt, *terrain);
        AISystem::updateCreature(*creature, dt, *spatialGrid);
    }
}
```

### Phase 3: Component Arrays (Breaking Change)

```cpp
// Full ECS implementation
class World {
    ComponentArray<Transform> transforms;
    ComponentArray<Physics> physics;
    ComponentArray<Lifecycle> lifecycles;
    ComponentArray<AIState> aiStates;
    // ...

    std::vector<Entity> entities;
    std::unordered_map<Entity, Signature> signatures;

public:
    Entity createCreature(CreatureType type) {
        Entity e = createEntity();
        addComponent<Transform>(e, Transform{});
        addComponent<Physics>(e, Physics{});
        addComponent<Lifecycle>(e, Lifecycle{});
        // ...
        return e;
    }
};
```

## Comparison: Games Using Similar Patterns

### Dwarf Fortress

- Initially OOP, migrated to more data-oriented approach
- Handles thousands of entities with complex behavior
- Uses spatial partitioning for pathfinding and interactions

### RimWorld

- Unity-based but custom entity system
- Components for health, needs, skills, etc.
- Behavior trees for AI, separate from entity data

### Factorio

- Highly optimized ECS-like architecture
- Handles millions of entities (belts, items, etc.)
- Custom spatial data structures for belt networks

## External References

### Tutorials and Documentation

- [A Simple ECS in C++ - Austin Morlan](https://austinmorlan.com/posts/entity_component_system/)
- [ECS FAQ - Sander Mertens](https://github.com/SanderMertens/ecs-faq)
- [Let's Build an ECS - Hexops Devlog](https://devlog.hexops.com/2022/lets-build-ecs-part-1/)
- [Deep-diving into ECS - PRDeving](https://prdeving.wordpress.com/2023/12/14/deep-diving-into-entity-component-system-ecs-architecture-and-data-oriented-programming/)

### Libraries to Consider

- [EnTT](https://github.com/skypjack/entt) - Header-only C++ ECS
- [FLECS](https://github.com/SanderMertens/flecs) - Fast C/C++ ECS with queries
- [Bevy ECS](https://bevyengine.org/) - Rust game engine with excellent ECS

### Data-Oriented Design Resources

- [Data-Oriented Design - Wikipedia](https://en.wikipedia.org/wiki/Data-oriented_design)
- [CppCon: Data-Oriented Design and C++](https://www.youtube.com/watch?v=rX0ItVEVjHc)
- [Mike Acton's CppCon Talk](https://www.youtube.com/watch?v=rX0ItVEVjHc)

## Summary

| Aspect | Current OOP | Proposed ECS |
|--------|-------------|--------------|
| Entity representation | Class with all data | ID + component composition |
| Memory layout | AoS (heap allocated) | SoA (contiguous arrays) |
| Behavior location | Methods in Creature class | Separate System classes |
| Scalability | ~300 entities | ~10,000+ entities |
| Code organization | Intuitive, familiar | Requires paradigm shift |
| Debugging | Easy object inspection | Requires specialized tools |
| Flexibility | Inheritance-based | Composition-based |

**Recommendation:** For a simulation of this scope, a hybrid approach works well:
1. Keep the `Creature` class for API familiarity
2. Internally reorganize data into hot/cold separation
3. Extract update logic into system-like functions
4. Use contiguous storage for frequently-accessed data
5. Full ECS migration only if scaling to 1000+ entities becomes a requirement
