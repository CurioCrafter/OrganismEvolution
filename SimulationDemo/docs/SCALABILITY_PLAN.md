# Scalability Plan
## OrganismEvolution - Performance and Scale Targets

### Document Version
- **Created**: January 2026
- **Last Updated**: January 2026
- **Scope**: Performance optimization and scalability strategies

---

## Current Performance Profile

### Measured Baselines (Estimated)

| Metric | Current Value | Measurement Method |
|--------|---------------|-------------------|
| **Max Creatures @ 60fps** | ~300-500 | Visual framerate drop |
| **Creature Update Cost** | ~0.5ms per 100 creatures | Sequential loop |
| **Spatial Query Cost** | O(creatures/400) average | 20x20 grid cells |
| **Render Calls per Frame** | 10-50 (batched) | Instanced rendering |
| **Memory per Creature** | ~2-3KB | Class size + heap allocs |
| **Frame Time Variance** | ±3-8ms | Due to GC-like spikes |

### Identified Bottlenecks

#### CPU Bottlenecks
1. **Sequential Creature Updates** - Single-threaded loop
2. **Neural Network Forward Pass** - Per-creature every frame
3. **Spatial Grid Rebuild** - Full clear + reinsert every frame
4. **Genome Fitness Calculation** - Per-creature, involves sqrt operations

#### GPU Bottlenecks
1. **Shadow Map Resolution** - Fixed 2048x2048, single cascade
2. **Fill Rate** - Forward rendering with many overlapping objects
3. **Instance Data Upload** - Per-frame buffer update

#### Memory Bottlenecks
1. **Scattered Allocations** - unique_ptr<Creature> pattern
2. **String Allocations** - Debug output, nametags
3. **Neural Network Weights** - 120 floats per creature (~480 bytes)

---

## Performance Targets

### Tier 1: Optimized Single-Thread (Immediate)
**Goal**: 1,000 creatures @ 60fps on single thread

| Optimization | Expected Gain | Effort |
|--------------|---------------|--------|
| SoA Position/Velocity | 2x iteration speed | Medium |
| SIMD neural forward pass | 4x brain update | Medium |
| Incremental spatial update | 3x query setup | Low |
| Object pooling | 1.5x allocation speed | Low |

### Tier 2: Multi-Threaded Simulation
**Goal**: 5,000 creatures @ 60fps with 4 cores

| Optimization | Expected Gain | Effort |
|--------------|---------------|--------|
| Parallel creature updates | 4x (core count) | High |
| Lock-free spatial queries | 2x contention reduction | High |
| Async render data prep | Overlap CPU/GPU | Medium |

### Tier 3: GPU Compute Integration
**Goal**: 50,000+ creatures @ 60fps

| Optimization | Expected Gain | Effort |
|--------------|---------------|--------|
| Compute shader neural nets | 100x throughput | Very High |
| GPU spatial hashing | Near-zero CPU cost | High |
| Indirect draw calls | 10x draw submission | High |

---

## Scalability Strategies

### 1. Spatial Partitioning Improvements

#### Current: Fixed Grid (20x20)
```
Pros: Simple, O(1) cell lookup
Cons: Fixed size, poor for non-uniform distribution
```

#### Recommended: Hierarchical Spatial Hash

```cpp
// Two-level spatial hash
struct SpatialHash {
    // Level 0: Fine grid (4x4 units per cell)
    std::unordered_map<uint64_t, std::vector<EntityID>> fineGrid;

    // Level 1: Coarse grid (16x16 units per cell)
    std::unordered_map<uint64_t, std::vector<EntityID>> coarseGrid;

    // Hash function
    uint64_t hash(float x, float z, float cellSize) {
        int32_t ix = static_cast<int32_t>(floor(x / cellSize));
        int32_t iz = static_cast<int32_t>(floor(z / cellSize));
        return (static_cast<uint64_t>(ix) << 32) | static_cast<uint32_t>(iz);
    }
};
```

**Benefits**:
- Adapts to any world size
- Fine grid for close queries, coarse for long-range
- Incremental update (only move changed entities)

#### Alternative: Loose Octree

For 3D queries (flying creatures, aquatic depth):
```
- Each node overlaps neighbors by 50%
- Entities never need to move between nodes for small movements
- O(log n) insertion, O(k) query where k = result count
```

### 2. Level of Detail (LOD) Systems

#### Simulation LOD
```
Distance Bands:
  0-50m:   Full simulation (AI every frame, full physics)
  50-150m: Reduced simulation (AI every 4 frames, simplified physics)
  150m+:   Statistical simulation (movement noise, no AI)
```

#### Rendering LOD
```
LOD 0: Full mesh (< 30m from camera)
       - All body parts, full animation
       - Per-vertex lighting

LOD 1: Simplified mesh (30-100m)
       - Body + limbs only
       - Vertex colors instead of textures

LOD 2: Billboard (100-300m)
       - Screen-aligned quad
       - Baked animation frames

LOD 3: Point (300m+)
       - Single colored point
       - No animation
```

### 3. Streaming Architecture

#### World Chunking
```
Chunk Size: 64x64 units (adjustable)
Active Radius: 3 chunks around camera (192x192 active area)
Streaming Distance: 5 chunks (load ahead)

Chunk States:
  - Unloaded: No data in memory
  - Loading: Background thread generating/loading
  - Active: Full simulation
  - Hibernating: Simplified simulation (creatures frozen in place)
  - Unloading: Saving state, freeing memory
```

#### Creature Migration
```cpp
struct ChunkMigration {
    void update() {
        for (auto& creature : activeCreatures) {
            ChunkCoord newChunk = worldToChunk(creature.position);
            if (newChunk != creature.currentChunk) {
                migrateCreature(creature, newChunk);
            }
        }
    }

    void migrateCreature(Creature& c, ChunkCoord dest) {
        // Remove from source chunk
        sourceChunk.creatures.erase(c.id);

        // Add to destination (may trigger chunk load)
        if (!isChunkLoaded(dest)) {
            loadChunkAsync(dest);
        }
        destChunk.creatures.insert(c.id, c);
        c.currentChunk = dest;
    }
};
```

### 4. Multi-Threading Architecture

#### Job System Design
```
┌─────────────────────────────────────────────────────────────┐
│                    Frame Timeline                            │
│                                                              │
│ ├─── Update Phase ────┼── Render Prep ─┼── GPU Submit ──┤   │
│                                                              │
│ Thread 0 (Main):                                             │
│ [Input] [Wait] [Render Submit] [Swap Buffers]               │
│                                                              │
│ Thread 1-N (Workers):                                        │
│ [AI Batch 0] [AI Batch 1] ... [Physics] [Spatial Rebuild]   │
│                                                              │
│ Synchronization Points:                                      │
│ - After all AI jobs complete → Physics can start            │
│ - After physics complete → Spatial rebuild                  │
│ - After spatial → Render data copy                          │
└─────────────────────────────────────────────────────────────┘
```

#### Thread-Safe Patterns
```cpp
// Double-buffered component storage
template<typename T>
class DoubleBuffer {
    std::vector<T> buffers[2];
    std::atomic<int> readIndex{0};

public:
    const std::vector<T>& read() { return buffers[readIndex]; }
    std::vector<T>& write() { return buffers[1 - readIndex]; }
    void swap() { readIndex = 1 - readIndex; }
};

// Usage: Render thread reads, simulation threads write
DoubleBuffer<glm::mat4> transforms;
```

### 5. Memory Management

#### Object Pooling
```cpp
template<typename T, size_t BlockSize = 64>
class Pool {
    struct Block {
        alignas(T) char data[sizeof(T) * BlockSize];
        std::bitset<BlockSize> used;
        Block* next;
    };

    Block* head;
    std::vector<Block*> allBlocks;

public:
    T* allocate() {
        // Find first free slot in current block
        // Allocate new block if needed
    }

    void deallocate(T* ptr) {
        // Mark slot as free (don't call destructor yet)
    }
};
```

#### Memory Budget System
```cpp
class MemoryBudget {
    static constexpr size_t CREATURE_BUDGET = 100 * 1024 * 1024;  // 100MB
    static constexpr size_t TERRAIN_BUDGET = 50 * 1024 * 1024;    // 50MB
    static constexpr size_t VEGETATION_BUDGET = 30 * 1024 * 1024; // 30MB

    std::atomic<size_t> creatureUsage{0};
    std::atomic<size_t> terrainUsage{0};
    std::atomic<size_t> vegetationUsage{0};

public:
    bool canAllocateCreature(size_t size) {
        return creatureUsage + size <= CREATURE_BUDGET;
    }

    void reportAllocation(Category cat, size_t size);
    void reportDeallocation(Category cat, size_t size);
};
```

---

## Implementation Phases

### Phase 1: Foundation Optimizations
**Timeline**: First iteration

1. **Convert hot data to SoA layout**
   - Position, Velocity, Energy arrays
   - Keep Genome/Brain as objects (complex structure)

2. **Implement incremental spatial updates**
   - Track moved entities with dirty flags
   - Only reinsert entities that moved

3. **Add object pooling for creatures**
   - Pre-allocate pool for max expected creatures
   - Recycle dead creature memory

### Phase 2: Parallelization
**Timeline**: Second iteration

1. **Implement job system**
   - Thread pool with work stealing
   - Batch creature updates by index range

2. **Parallelize creature AI**
   - Each job processes N creatures
   - Read-only access to spatial grid

3. **Double-buffer render data**
   - Simulation writes to back buffer
   - Renderer reads from front buffer

### Phase 3: Advanced Scaling
**Timeline**: Third iteration

1. **Implement world streaming**
   - Chunk-based world loading
   - Background terrain generation

2. **Add simulation LOD**
   - Distance-based update frequency
   - Statistical simulation for distant creatures

3. **GPU compute for neural networks**
   - Batch all brains into single dispatch
   - Read results back for decision making

---

## Measurement & Profiling

### Key Metrics to Track
```cpp
struct FrameMetrics {
    float totalFrameTime;
    float updateTime;      // All creature updates
    float renderPrepTime;  // Building instance buffers
    float renderTime;      // GPU draw calls
    float spatialTime;     // Grid rebuild

    int creaturesUpdated;
    int drawCalls;
    int trianglesRendered;

    size_t memoryUsage;
    int cacheLinesCrossed;  // If profiling data locality
};
```

### Profiling Tools
- **CPU**: Tracy, Intel VTune, Visual Studio Profiler
- **GPU**: RenderDoc, NSight Graphics
- **Memory**: Valgrind (Linux), Visual Studio Memory Profiler

### Performance Regression Tests
```cpp
TEST(Performance, CreatureUpdateScaling) {
    for (int count : {100, 500, 1000, 5000}) {
        auto sim = createSimulationWithCreatures(count);

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 100; i++) {
            sim.update(1.0f / 60.0f);
        }
        auto duration = std::chrono::high_resolution_clock::now() - start;

        float msPerUpdate = duration.count() / 100.0f;
        float msPerCreature = msPerUpdate / count;

        // Assert linear or sub-linear scaling
        EXPECT_LT(msPerCreature, 0.05f);  // < 50µs per creature
    }
}
```

---

## Case Studies & References

### Dwarf Fortress
- **Scale**: 200+ dwarves with complex AI
- **Strategy**: Pathfinding caching, job priority queues
- **Lesson**: Complex simulation possible with smart scheduling

### Rimworld
- **Scale**: 50+ colonists, 1000s of items
- **Strategy**: Tick-based updates, lazy evaluation
- **Lesson**: Not everything needs per-frame updates

### Cities: Skylines
- **Scale**: 1M+ citizens
- **Strategy**: Agent groups, statistical simulation
- **Lesson**: Individual simulation not always necessary

### GDC Talks
- [Parallelizing the Naughty Dog Engine (GDC 2015)](https://www.gdcvault.com/play/1022186/)
- [Multithreading for Games (GDC 2019)](https://www.gdcvault.com/browse/gdc-19/play/1025920/)
- [Data-Oriented Design in Practice (GDC 2014)](https://www.gdcvault.com/play/1020791/Data-Oriented-Design-and-C)

---

## Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Thread synchronization bugs | High | High | Extensive testing, data race sanitizers |
| Performance regression | Medium | Medium | Automated benchmarks, gradual rollout |
| Memory fragmentation | Medium | Low | Object pooling, memory arenas |
| GPU driver compatibility | Low | High | Test on multiple vendors, fallback paths |
| Complexity explosion | Medium | High | Incremental refactoring, documentation |
