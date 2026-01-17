# Architecture Roadmap
## OrganismEvolution - Evolution Simulator

### Document Version
- **Created**: January 2026
- **Last Updated**: January 2026
- **Scope**: Long-term architectural evolution path

---

## Current State Analysis

### Architecture Overview (As-Is)

```
┌─────────────────────────────────────────────────────────────────┐
│                        main.cpp                                  │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │   GLFW/Input    │  │  Render Loop    │  │   Dashboard     │ │
│  │   Handling      │  │  (Single Thread)│  │   UI (ImGui)    │ │
│  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘ │
│           │                    │                    │          │
└───────────┼────────────────────┼────────────────────┼──────────┘
            │                    │                    │
            v                    v                    v
┌───────────────────────────────────────────────────────────────┐
│                       Simulation                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐│
│  │   Terrain    │  │ VegetationMgr│  │  EcosystemManager    ││
│  │   (Heightmap)│  │  (L-Systems) │  │ ┌──────┐ ┌────────┐ ││
│  └──────────────┘  └──────────────┘  │ │Seasons│ │Metrics │ ││
│                                       │ └──────┘ └────────┘ ││
│  ┌──────────────────────────────────┐ └──────────────────────┘│
│  │        Creatures (Vector)        │                         │
│  │  ┌────────┐ ┌────────┐ ┌──────┐ │                         │
│  │  │Creature│ │Creature│ │ ...  │ │                         │
│  │  │+Genome │ │+Genome │ │      │ │                         │
│  │  │+Brain  │ │+Brain  │ │      │ │                         │
│  │  │+Sensory│ │+Sensory│ │      │ │                         │
│  │  └────────┘ └────────┘ └──────┘ │                         │
│  └──────────────────────────────────┘                         │
│                                                                │
│  ┌──────────────┐  ┌──────────────────────────────────────┐  │
│  │ SpatialGrid  │  │      Rendering Systems               │  │
│  │ (20x20 cells)│  │ ┌───────────┐ ┌─────────────────────┐│  │
│  └──────────────┘  │ │ Creature  │ │ Shadow Mapping      ││  │
│                    │ │ Renderer  │ │ (Single Cascade)    ││  │
│                    │ │(Instanced)│ └─────────────────────┘│  │
│                    │ └───────────┘                        │  │
│                    └──────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────┘
```

### Current Component Responsibilities

| Component | Responsibility | Coupling Level |
|-----------|---------------|----------------|
| `Simulation` | Central orchestrator, owns all entities | High (monolithic) |
| `Creature` | Entity with genome, brain, physics, rendering | High |
| `SpatialGrid` | O(1) neighbor queries, 20x20 fixed grid | Low |
| `CreatureRenderer` | Instanced rendering, batching by mesh | Medium |
| `EcosystemManager` | Season/metrics/population balance | Medium |
| `Terrain` | Heightmap generation, water detection | Low |

### Current Limitations Identified

#### 1. **Threading Model**
- **Issue**: Single-threaded simulation loop
- **Impact**: CPU bottleneck with >500 creatures
- **Evidence**: All creature updates sequential in `Simulation::update()`

#### 2. **Memory Layout**
- **Issue**: `std::vector<std::unique_ptr<Creature>>` - scattered heap allocations
- **Impact**: Cache misses during iteration, poor prefetching
- **Evidence**: Each creature is a separate heap allocation with internal pointers

#### 3. **Spatial Partitioning**
- **Issue**: Fixed 20x20 grid (15x15 unit cells)
- **Impact**: Uneven distribution in large worlds, O(n) rebuild per frame
- **Evidence**: `SpatialGrid::clear()` called every frame

#### 4. **Rendering Pipeline**
- **Issue**: Forward rendering with single shadow cascade
- **Impact**: Limited creature count before fill-rate bound
- **Evidence**: Fixed 2048x2048 shadow map, single sun light

#### 5. **Data Coupling**
- **Issue**: Creature owns genome, brain, physics, AND rendering data
- **Impact**: Cannot separate update threads from render threads
- **Evidence**: `Creature` class has 80+ member variables

---

## Target State Architecture

### Phase 1: Data-Oriented Refactor (Foundation)

```
┌─────────────────────────────────────────────────────────────────┐
│                     Entity Component System                      │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                    ComponentArrays                        │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐   │  │
│  │  │ Position[]   │  │ Velocity[]   │  │ Genome[]     │   │  │
│  │  │ (contiguous) │  │ (contiguous) │  │ (contiguous) │   │  │
│  │  └──────────────┘  └──────────────┘  └──────────────┘   │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐   │  │
│  │  │ Brain[]      │  │ Energy[]     │  │ RenderData[] │   │  │
│  │  │ (contiguous) │  │ (contiguous) │  │ (contiguous) │   │  │
│  │  └──────────────┘  └──────────────┘  └──────────────┘   │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                     Systems                               │  │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────┐│  │
│  │  │Movement  │ │ AI       │ │ Physics  │ │ Reproduction ││  │
│  │  │ System   │ │ System   │ │ System   │ │ System       ││  │
│  │  └──────────┘ └──────────┘ └──────────┘ └──────────────┘│  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

**Benefits**:
- Cache-friendly iteration (Structure of Arrays)
- Systems can be parallelized independently
- Clean separation of concerns

### Phase 2: Job System Integration

```
┌─────────────────────────────────────────────────────────────────┐
│                        Job Scheduler                             │
│                                                                  │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐                │
│  │ Worker 0   │  │ Worker 1   │  │ Worker N   │                │
│  │ (Thread 0) │  │ (Thread 1) │  │ (Thread N) │                │
│  └─────┬──────┘  └─────┬──────┘  └─────┬──────┘                │
│        │               │               │                        │
│        └───────────────┼───────────────┘                        │
│                        │                                         │
│                        v                                         │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                   Job Queue (Lock-Free)                   │  │
│  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐  │  │
│  │  │Update│ │Update│ │Update│ │Update│ │Render│ │ Audio│  │  │
│  │  │AI 0-99│AI100-│ │Phys  │ │Sens  │ │Prep  │ │ Jobs │  │  │
│  │  └──────┘ └──────┘ └──────┘ └──────┘ └──────┘ └──────┘  │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│  Frame Dependencies:                                             │
│  [AI Update] ──> [Physics] ──> [Spatial Rebuild] ──> [Render]   │
└─────────────────────────────────────────────────────────────────┘
```

**Job Types**:
1. **Parallel Jobs**: Creature AI, physics updates (batch by N creatures)
2. **Sequential Jobs**: Spatial grid rebuild, render submission
3. **Async Jobs**: Save/load, terrain streaming

### Phase 3: Rendering Pipeline Upgrade

```
┌─────────────────────────────────────────────────────────────────┐
│                   Deferred Rendering Pipeline                    │
│                                                                  │
│  Pass 1: G-Buffer Generation                                    │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ Albedo  │ Normal  │ Roughness │ Metallic │ Depth        │  │
│  │ (RGBA8) │ (RG16F) │ (R8)      │ (R8)     │ (D32F)       │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│  Pass 2: Shadow Maps (Cascaded)                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ Cascade 0  │ Cascade 1  │ Cascade 2  │ Cascade 3        │  │
│  │ (0-50m)    │ (50-150m)  │ (150-400m) │ (400-1000m)      │  │
│  │ 2048x2048  │ 2048x2048  │ 1024x1024  │ 512x512          │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│  Pass 3: Lighting                                               │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ Sun Light │ Ambient Occlusion │ Screen Space Reflections │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│  Pass 4: Post-Processing                                        │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ Bloom │ Tone Mapping │ FXAA │ Color Grading              │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Migration Path

### Step 1: Introduce ECS Foundation (Non-Breaking)
**Priority**: High
**Complexity**: Medium

1. Create `EntityManager` class alongside existing `Simulation`
2. Implement component storage with SoA layout
3. Migrate one component at a time (start with Position/Velocity)
4. Keep `Creature` class as facade during transition

```cpp
// Example: Phase 1 component storage
class ComponentStorage {
    std::vector<glm::vec3> positions;      // Cache-friendly
    std::vector<glm::vec3> velocities;
    std::vector<float> energies;
    std::vector<uint32_t> entityToIndex;   // Sparse set
    std::vector<uint32_t> indexToEntity;
};
```

### Step 2: Implement Basic Job System
**Priority**: High
**Complexity**: High

1. Create thread pool with N-1 worker threads (N = core count)
2. Implement lock-free job queue (consider: `moodycamel::ConcurrentQueue`)
3. Start with embarrassingly parallel jobs (creature AI updates)
4. Add job dependencies using atomic counters

```cpp
// Example: Job interface
struct Job {
    void (*function)(void* data, uint32_t start, uint32_t end);
    void* data;
    std::atomic<uint32_t>* counter;  // Dependencies
};
```

### Step 3: Upgrade Spatial Partitioning
**Priority**: Medium
**Complexity**: Medium

1. Replace fixed grid with **Bounding Volume Hierarchy (BVH)** or **Loose Octree**
2. Implement incremental updates (only move changed entities)
3. Add frustum culling integration
4. Consider spatial hashing for truly dynamic worlds

### Step 4: Rendering Pipeline Modernization
**Priority**: Medium
**Complexity**: High

1. Implement G-Buffer pass
2. Add cascaded shadow maps (4 cascades)
3. Implement GPU-driven culling (compute shader)
4. Add LOD system for creatures

### Step 5: Asset Streaming
**Priority**: Low
**Complexity**: High

1. Implement chunk-based world loading
2. Background thread for mesh generation
3. Priority queue based on camera distance
4. Memory budget manager

---

## Architecture Decision Records

### ADR-001: ECS vs Traditional OOP
**Decision**: Hybrid approach (ECS for hot path, OOP for complex behaviors)
**Rationale**:
- Pure ECS is verbose for complex creature behaviors
- Hot loop (position/velocity updates) benefits from SoA
- Brain/Genome can remain object-oriented

### ADR-002: Job System Selection
**Decision**: Custom lightweight job system over EnTT/Flecs schedulers
**Rationale**:
- Full control over job granularity
- No external dependencies
- Tailored to simulation tick pattern

### ADR-003: Rendering API
**Decision**: Remain on OpenGL 4.3+, defer DX12/Vulkan
**Rationale**:
- OpenGL sufficient for current scale (1000s of creatures)
- Cross-platform benefits
- DX12/Vulkan adds complexity without current need
- Revisit when GPU-driven rendering becomes essential

---

## Success Metrics

| Metric | Current | Phase 1 Target | Phase 3 Target |
|--------|---------|----------------|----------------|
| Max Creatures | ~300 @ 60fps | 1,000 @ 60fps | 10,000 @ 60fps |
| Frame Time Variance | ±5ms | ±2ms | ±0.5ms |
| Memory per Creature | ~2KB | ~500B | ~300B |
| Spatial Query Time | O(n/400) | O(log n) | O(1) amortized |
| Shadow Quality | 1 cascade | 2 cascades | 4 cascades |

---

## References

### Architecture Patterns
- [Game Programming Patterns - Data Locality](http://gameprogrammingpatterns.com/data-locality.html)
- [GDC 2015 - Parallelizing the Naughty Dog Engine](https://www.gdcvault.com/play/1022186/Parallelizing-the-Naughty-Dog-Engine)
- [Our Machinery - The Truth (Data-Oriented Engine)](https://ourmachinery.com/)

### Similar Projects
- [Creatures (Steve Grand)](https://en.wikipedia.org/wiki/Creatures_(video_game_series)) - Artificial life simulation
- [Dwarf Fortress](https://www.bay12games.com/dwarves/) - Complex simulation at scale
- [Species: ALRE](https://store.steampowered.com/app/774541/Species_Artificial_Life_Real_Evolution/) - Evolution simulator

### Technical Resources
- [EnTT ECS Library](https://github.com/skypjack/entt)
- [Flecs ECS Library](https://github.com/SanderMertens/flecs)
- [Bitsquid (Stingray) Architecture](http://bitsquid.blogspot.com/)
