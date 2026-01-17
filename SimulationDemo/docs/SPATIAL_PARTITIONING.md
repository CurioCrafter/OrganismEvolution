# Spatial Partitioning System

## Overview

The spatial partitioning system uses a uniform grid to accelerate creature neighbor queries from O(n²) to O(1) average-case complexity. This is critical for simulation performance as creature count increases.

## Implementation Status

| Feature | Status | Notes |
|---------|--------|-------|
| SpatialGrid class | Complete | `src/utils/SpatialGrid.h/cpp` |
| Grid initialization | Complete | 20x20 cells covering world bounds |
| Per-frame rebuild | Complete | `Simulation::update()` clears and rebuilds each frame |
| `findNearestCreature()` integration | Complete | Uses `SpatialGrid::findNearest()` when available |
| `getNeighborsOfType()` integration | Complete | Uses `SpatialGrid::queryByType()` when available |
| Fallback behavior | Complete | O(n) linear scan when grid unavailable |

## Architecture

### Grid Structure

```
SpatialGrid
├── worldWidth, worldDepth    // World dimensions
├── gridSize                  // Number of cells per axis (default: 20)
├── cellWidth, cellDepth      // Computed cell dimensions
└── cells[gridSize][gridSize] // 2D array of creature lists
```

### Coordinate Mapping

World coordinates are centered at origin. The grid maps world position `(x, z)` to cell indices:

```cpp
float normalizedX = (x + worldWidth * 0.5f) / worldWidth;
float normalizedZ = (z + worldDepth * 0.5f) / worldDepth;
cellX = clamp(int(normalizedX * gridSize), 0, gridSize - 1);
cellZ = clamp(int(normalizedZ * gridSize), 0, gridSize - 1);
```

### Query Algorithm

For radius queries, the grid:
1. Computes the bounding box of cells that could contain creatures within radius
2. Iterates only those cells (typically 1-4 cells for typical query radii)
3. Performs distance check on creatures in those cells

This reduces O(n) comparisons to O(k) where k is the average creatures per cell.

## API Reference

### Core Methods

```cpp
// Clear grid for new frame
void clear();

// Insert creature into appropriate cell
void insert(Creature* creature);

// Query all creatures within radius
std::vector<Creature*> query(const glm::vec3& position, float radius) const;

// Query creatures of specific type within radius
std::vector<Creature*> queryByType(const glm::vec3& position, float radius, int creatureType) const;

// Find nearest creature (optionally filtered by type)
Creature* findNearest(const glm::vec3& position, float maxRadius, int typeFilter = -1) const;

// Count creatures within radius
int countNearby(const glm::vec3& position, float radius) const;
```

### Integration Points

The SpatialGrid is used by the following Creature methods:

```cpp
// Uses grid->findNearest() when available
Creature* findNearestCreature(const std::vector<Creature*>& creatures,
                              CreatureType targetType, float maxRange,
                              const SpatialGrid* grid = nullptr) const;

// Uses grid->queryByType() when available
std::vector<Creature*> getNeighborsOfType(const std::vector<Creature*>& creatures,
                                          CreatureType targetType, float range,
                                          const SpatialGrid* grid = nullptr) const;
```

## Per-Frame Update Cycle

```cpp
void Simulation::update(float deltaTime) {
    // 1. Rebuild spatial grid
    if (spatialGrid) {
        spatialGrid->clear();
        for (auto& creature : creatures) {
            if (creature->isAlive()) {
                spatialGrid->insert(creature.get());
            }
        }
    }

    // 2. Update creatures (they use the grid for queries)
    updateCreatures(adjustedDelta);

    // ...
}
```

## Performance Characteristics

### Time Complexity

| Operation | Without Grid | With Grid |
|-----------|-------------|-----------|
| Find nearest creature | O(n) | O(k) where k = creatures/cell |
| Get all neighbors in radius | O(n) | O(k * cells_in_radius) |
| Grid rebuild per frame | N/A | O(n) |

### Space Complexity

- Grid storage: O(gridSize²) cell vectors
- Per-creature: O(1) (pointer stored in one cell)

### Optimal Grid Size

The default 20x20 grid provides good balance:
- Too few cells: Many creatures per cell, queries approach O(n)
- Too many cells: More memory, rebuild overhead, cache misses
- 20x20 with 300x300 world = 15x15 unit cells = good for typical vision ranges (20-50 units)

## Usage Examples

### Herbivore Behavior (Predator Detection)

```cpp
void Creature::updateBehaviorHerbivore(float deltaTime, ..., const SpatialGrid* grid) {
    // O(1) predator detection using spatial grid
    Creature* nearestPredator = findNearestCreature(
        otherCreatures, CreatureType::CARNIVORE, genome.visionRange, grid);

    if (nearestPredator != nullptr) {
        // Flee behavior
    }
}
```

### Flocking Behavior

```cpp
// O(1) neighbor query for flocking
std::vector<Creature*> herbivoreNeighbors = getNeighborsOfType(
    otherCreatures, CreatureType::HERBIVORE, genome.visionRange * 0.6f, grid);

if (!herbivoreNeighbors.empty()) {
    glm::vec3 flockForce = steering.flock(position, velocity, herbivoreNeighbors, ...);
}
```

## Future Enhancements

1. **Hierarchical Grid**: Quad-tree for varying creature densities
2. **Persistent Grid**: Update only moved creatures instead of full rebuild
3. **Parallel Queries**: Thread-safe query methods for multi-threaded updates
4. **3D Grid**: For flying creatures with significant altitude variation

## Related Documentation

- [ARCHITECTURE.md](ARCHITECTURE.md) - Overall system design
- [SIMULATION_ARCHITECTURE.md](SIMULATION_ARCHITECTURE.md) - Simulation loop details
- [SCALABILITY_PLAN.md](SCALABILITY_PLAN.md) - Performance scaling strategies
