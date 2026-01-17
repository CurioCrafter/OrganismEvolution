# Phase 10 Agent 3: Multi-Island Runtime Integration and Migration

**Status**: Completed
**Date**: 2026-01-16
**Agent**: Agent 3 - Multi-Island System Integration
**Project**: OrganismEvolution

---

## Executive Summary

This document describes the implementation of multi-island competition and migration runtime integration for the OrganismEvolution simulator. The system enables multiple islands (archipelagos) to run simultaneously with creatures migrating between them, creating evolutionary pressure through island biogeography effects.

### Key Features Implemented

- **Multi-Island Population System**: Each island has its own terrain, creature manager, and vegetation with deterministic seeding
- **Inter-Island Migration**: Creatures can migrate between islands via multiple migration types (coastal drift, flying, rafting, etc.)
- **Migration Event Tracking**: Comprehensive statistics and event callbacks for UI and camera integration
- **Enhanced UI**: Island comparison panels, migration feeds, and per-island statistics
- **Camera Integration**: Island-focused camera with transition support and migration following capability

---

## Architecture Overview

### System Components

```
MultiIslandManager
├── Island[0..N]
│   ├── Terrain (unique per island)
│   ├── CreatureManager (isolated population)
│   ├── VegetationManager
│   ├── IslandStats (population, fitness, diversity)
│   └── Climate (optional)
│
InterIslandMigration
├── Active Migrations (in-flight creatures)
├── Migration Statistics
├── Event Callbacks
└── Configuration

IslandComparisonUI
├── Overview Mode
├── Side-by-Side Mode
├── Gene Flow Diagram
├── Migration Map
└── Recent Events Feed

IslandCamera
├── Island View (focused on one island)
├── Overview (archipelago-wide)
├── Transition (smooth island switching)
└── Following (track migrating creatures)
```

---

## 1. Island Initialization

### Deterministic Per-Island Seeding

Each island receives a deterministic seed based on:
- Base archipelago seed
- Island index offset (e.g., `baseSeed + islandIndex * 10000`)

This ensures:
- Reproducible island populations
- Distinct genetic variation between islands
- Trackable divergence over time

**Implementation**: [MultiIslandManager.cpp:125-191](../src/core/MultiIslandManager.cpp#L125-L191)

```cpp
void MultiIslandManager::populateIsland(Island& island, unsigned int seed) {
    // Deterministic per-island seed
    island.creatures->init(island.terrain.get(), nullptr, seed);

    // Calculate population based on island size
    int basePopulation = static_cast<int>(50 * island.size);

    // Spawn with genetic variation
    std::mt19937 rng(seed);
    // ... spawn herbivores, carnivores, aquatic creatures
}
```

### Island-Specific Configuration

Each island maintains:
- **Island ID**: Unique identifier (0-based index)
- **Name**: Human-readable name (e.g., "Island Alpha")
- **World Position**: 2D coordinates in archipelago space
- **Size Multiplier**: Affects terrain size and base population
- **IslandConfig**: Biome types, difficulty, resource density

**Code Reference**: [MultiIslandManager.h:54-116](../src/core/MultiIslandManager.h#L54-L116)

---

## 2. Migration System

### Migration Types

The system supports 8 migration types with different characteristics:

| Type | Base Survival | Speed | Energy Cost | Typical Creatures |
|------|--------------|-------|-------------|-------------------|
| COASTAL_DRIFT | 30% | 5 u/s | 0.5/u | Aquatic, swimmers |
| FLYING | 70% | 15 u/s | 0.3/u | Flying creatures |
| FLOATING_DEBRIS | 20% | 2 u/s | 0.1/u | Land creatures (rafting) |
| SEASONAL | 60% | 5 u/s | 0.5/u | Triggered by environmental cues |
| POPULATION_PRESSURE | 40% | 5 u/s | 0.5/u | Overcrowding trigger |
| FOOD_SCARCITY | 30% | 5 u/s | 0.5/u | Low energy trigger |
| MATE_SEEKING | 50% | 7 u/s | 0.4/u | Reproductive trigger |
| RANDOM_DISPERSAL | 25% | 3 u/s | 0.5/u | Chance events |

**Configuration**: [InterIslandMigration.h:116-151](../src/entities/behaviors/InterIslandMigration.h#L116-L151)

### Migration Triggers

Migrations are triggered by:

1. **Population Pressure**: When island population > 80% capacity
2. **Coastal Proximity**: Creatures near coast (>70% from center) have 3x higher migration chance
3. **Energy Scarcity**: Creatures with <30% energy seek new habitat
4. **Base Chance**: Very low (0.0001 per creature per update) with modifiers

**Implementation**: [InterIslandMigration.cpp:36-92](../src/entities/behaviors/InterIslandMigration.cpp#L36-L92)

### Migration Event Lifecycle

```
1. INITIATING
   ↓ (creature removed from source island)
2. IN_TRANSIT
   ↓ (energy consumed, survival checks)
3. ARRIVING (progress > 90%)
   ↓
4. COMPLETED → spawn on destination
   OR
   FAILED → creature lost at sea
```

### Survival Calculation

Survival chance is computed from:
- **Base survival** (by migration type)
- **Distance penalty**: -0.5 per 1000 units
- **Fitness bonus**: +0.2 per fitness point (swimming) or +0.1 (flying)
- **Energy bonus**: +0.1 * (energy/200)
- **Ocean current bonus**: +0.15 * current strength (water migrations)
- **Creature type bonus**: +0.2 for flying on FLYING migration, +0.3 for aquatic on COASTAL_DRIFT

Final survival clamped to [0.05, 0.95]

**Code**: [InterIslandMigration.cpp:339-382](../src/entities/behaviors/InterIslandMigration.cpp#L339-L382)

---

## 3. Migration Statistics

### Per-Island Stats

Each island tracks:
- `emigrations`: Creatures leaving the island
- `immigrations`: Creatures arriving on the island
- Net migration: `immigrations - emigrations`

**Updated in**: [InterIslandMigration.cpp:261-266](../src/entities/behaviors/InterIslandMigration.cpp#L261-L266)

### Global Migration Stats

The InterIslandMigration system tracks:
- `totalAttempts`: All migration attempts
- `successfulMigrations`: Arrivals
- `failedMigrations`: Lost at sea
- `inProgressMigrations`: Currently traveling
- `attemptsByType[8]`: Attempts per migration type
- `successesByType[8]`: Successes per migration type
- `migrationsBetweenIslands`: Map of (source, dest) → count
- `avgSurvivalRate`: Weighted average survival
- `avgTravelTime`: Weighted average duration

**Structure**: [InterIslandMigration.h:85-112](../src/entities/behaviors/InterIslandMigration.h#L85-L112)

---

## 4. UI Integration

### Island Comparison UI

The `IslandComparisonUI` provides 5 viewing modes:

#### Overview Mode
- Global archipelago statistics
- Per-island summary cards
- Population, species, fitness, diversity per island
- Migration counts (arrivals/departures)
- Collapsible island cards with progress bars

**Screenshot/Notes**: Shows archipelago-wide stats and individual island breakdowns

#### Side-by-Side Mode
- Compare two selected islands directly
- Genetic distance calculation
- Physical distance
- Divergence status (LOW/MODERATE/HIGH)
- Comparison bars for population, fitness, diversity

#### Gene Flow Diagram
- Genetic distance matrix (island × island)
- Migration summary (flows between islands)
- Active migration count

#### Divergence Tree
- Divergence from reference island
- Speciation predictions (genetic distance > 0.7)

#### Migration Map
- Active migration details:
  - Source → Destination
  - Migration type (Coastal Drift, Flying, etc.)
  - Progress bar with percentage and elapsed time
  - Survival chance and energy remaining
  - Energy bar (color-coded: green/yellow/red)
- Recent events feed:
  - Migration arrivals/departures
  - Population booms/crashes
  - Color-coded by event type

**Implementation**: [IslandComparisonUI.cpp](../src/ui/IslandComparisonUI.cpp)

### Region Overview Panel

Pre-existing component (Phase 8) that displays:
- Per-island population by creature type
- Biome distribution
- Ecosystem health indicator
- Jump to island button (camera integration)

**Code**: [RegionOverviewPanel.h](../src/ui/RegionOverviewPanel.h)

---

## 5. Camera System

### Island Camera Modes

The `IslandCamera` supports:

1. **ISLAND_VIEW**: Focused on active island
   - Orbitable camera (yaw/pitch control)
   - Zoom (0.5x - 3.0x)
   - Pan within island bounds

2. **OVERVIEW**: Bird's eye view of archipelago
   - Height based on archipelago size
   - Shows all islands simultaneously

3. **TRANSITION**: Smooth island switching
   - Arc interpolation (configurable arc height)
   - Eased cubic interpolation
   - Configurable duration (default 2s)

4. **FOLLOWING**: Track migrating creatures
   - Camera follows creature position
   - Offset based on velocity direction
   - Smooth damping

### Camera Transitions

When switching islands:
```cpp
// Calculate ideal position for target island
glm::vec3 targetPos = calculateIslandCameraPosition(targetIsland);
glm::vec3 targetCenter = calculateIslandCenter(targetIsland);

// Start smooth transition with arc
startTransition(targetPos, targetCenter, 2.0f);
```

Arc interpolation adds height during transition for cinematic effect.

**Code**: [IslandCamera.cpp:364-386](../src/graphics/IslandCamera.cpp#L364-L386)

---

## 6. Runtime Hookup (Handoff to Agent 1)

### Required Integration Points

Agent 1 (Runtime/Main Integration) needs to connect:

#### 1. Main Loop Integration

```cpp
// In main.cpp or simulation update loop:

// After world generation
archipelagoGenerator.generate(seed);
multiIslandManager.init(archipelagoGenerator);
multiIslandManager.generateAll(seed);

// Per frame
multiIslandManager.update(deltaTime);
interIslandMigration.update(deltaTime, multiIslandManager);
multiIslandManager.updateStatistics();

// UI update (every 0.5s or so)
islandComparisonUI.recordHistory(multiIslandManager);
islandComparisonUI.render(multiIslandManager, &interIslandMigration);

// Camera update
islandCamera.update(deltaTime);
```

#### 2. Event Callbacks

```cpp
// Register migration event callbacks
interIslandMigration.registerCallback([&](const MigrationEvent& event) {
    if (event.state == MigrationState::COMPLETED) {
        // UI notification, camera cue, etc.
        islandCamera.followMigration(event, multiIslandManager);
    }
});

// Register island event callbacks
multiIslandManager.registerEventCallback([&](const IslandEvent& event) {
    if (event.type == IslandEvent::Type::POPULATION_BOOM) {
        // UI alert
    }
});
```

#### 3. DX12 Initialization

```cpp
// After DX12 device and PSO creation
multiIslandManager.initializeDX12(dx12Device, terrainPSO, rootSignature);
```

#### 4. Rendering

```cpp
// In render pass
multiIslandManager.render(camera, commandList);

// For shadows (optional, performance optimization)
multiIslandManager.renderForShadow(camera, shadowCommandList);
```

### Configuration

Recommended settings for 5-10 minute runs:

```cpp
// Migration config
MigrationConfig config;
config.baseMigrationChance = 0.0002f;  // Slightly higher for testing
config.coastalProximityBonus = 3.0f;
config.populationPressureThreshold = 0.8f;

// Island manager config
multiIslandManager.setMaxCreaturesPerIsland(2048);
multiIslandManager.setAlwaysUpdateActiveIsland(true);
multiIslandManager.setUpdateRadiusForInactiveIslands(500.0f);

// UI config
islandComparisonUI.setVisible(true);
islandComparisonUI.setMode(ComparisonMode::OVERVIEW);
```

---

## 7. Validation Checklist

For 5-10 minute test runs with 3-6 islands:

- [ ] Each island generates distinct terrain
- [ ] Initial populations spawn correctly (50-150 creatures per island based on size)
- [ ] Population counts are tracked independently per island
- [ ] Migrations occur (check migration stats in UI)
- [ ] At least one successful migration per island pair observed
- [ ] Migration survival rates are reasonable (20-70% depending on type)
- [ ] Island statistics update correctly:
  - [ ] Population curves diverge between islands
  - [ ] Species counts vary per island
  - [ ] Genetic diversity tracked independently
  - [ ] Emigration/immigration counts match
- [ ] UI displays all islands correctly
- [ ] Camera can switch between islands smoothly
- [ ] No crashes or memory leaks during extended runs

### Expected Behavior

After 5-10 minutes:
- **Population Divergence**: Each island should show distinct population curves
- **Migration Events**: 10-50 migration attempts across all islands
- **Survival Rate**: 30-50% average across all migration types
- **Genetic Divergence**: Low at start, increasing over time
- **Species Variation**: Different species counts per island

---

## 8. Code Locations

### Core Files (Agent 3 Owned)

| File | Purpose | Key Functions |
|------|---------|--------------|
| [src/core/MultiIslandManager.h](../src/core/MultiIslandManager.h) | Multi-island manager interface | `generateAll()`, `update()`, `transferCreature()` |
| [src/core/MultiIslandManager.cpp](../src/core/MultiIslandManager.cpp) | Implementation | `populateIsland()`, `updateIslandStats()` |
| [src/entities/behaviors/InterIslandMigration.h](../src/entities/behaviors/InterIslandMigration.h) | Migration system interface | `update()`, `attemptMigration()` |
| [src/entities/behaviors/InterIslandMigration.cpp](../src/entities/behaviors/InterIslandMigration.cpp) | Migration implementation | `checkMigrationTriggers()`, `completeMigration()` |
| [src/ui/IslandComparisonUI.h](../src/ui/IslandComparisonUI.h) | Island UI interface | `render()`, `renderMigrationMap()` |
| [src/ui/IslandComparisonUI.cpp](../src/ui/IslandComparisonUI.cpp) | UI implementation | `renderOverview()`, `renderSideBySide()` |
| [src/ui/RegionOverviewPanel.h](../src/ui/RegionOverviewPanel.h) | Per-island stats panel | `updateStats()`, `renderRegionCard()` |
| [src/graphics/IslandCamera.h](../src/graphics/IslandCamera.h) | Island camera interface | `setActiveIsland()`, `followMigration()` |
| [src/graphics/IslandCamera.cpp](../src/graphics/IslandCamera.cpp) | Camera implementation | `updateTransition()`, `updateFollowing()` |

### Files NOT Modified (Parallel Safety)

- `src/main.cpp` - Agent 1's responsibility for runtime hookup
- `src/environment/ProceduralWorld.*` - World generation (Phase 9)
- `src/ui/*` (except IslandComparisonUI and RegionOverviewPanel)

---

## 9. Tuning Parameters

### Migration Tuning

To increase migration frequency:
```cpp
config.baseMigrationChance = 0.001f;  // 10x higher (default 0.0001)
config.coastalProximityBonus = 5.0f;  // Stronger coastal effect
```

To reduce migration difficulty:
```cpp
config.baseSwimSurvival = 0.5f;  // 50% instead of 30%
config.baseFlyingSurvival = 0.85f;  // 85% instead of 70%
```

To speed up migrations:
```cpp
config.swimSpeed = 10.0f;  // 2x faster
config.flyingSpeed = 25.0f;
```

### Island Population Tuning

To adjust island population density:
```cpp
// In MultiIslandManager::populateIsland()
int basePopulation = static_cast<int>(100 * island.size);  // Double population
```

To modify creature type distribution:
```cpp
int herbivoreCount = static_cast<int>(basePopulation * 0.8f);  // 80% herbivores
int carnivoreCount = static_cast<int>(basePopulation * 0.1f);  // 10% carnivores
```

---

## 10. Future Enhancements

Potential improvements for future phases:

1. **Adaptive Island Effects**: Environmental pressure based on island size and resources
2. **Wind and Current Visualization**: Real-time ocean current display
3. **Migration Pathways**: Preferred routes based on historical success
4. **Seasonal Migration**: Scheduled mass migrations at specific simulation times
5. **Island Extinction/Recolonization**: Allow islands to go extinct and be recolonized
6. **Genetic Founder Effects**: Track genetic bottlenecks during migration events
7. **Allopatric Speciation Alerts**: Automatic detection of reproductive isolation
8. **Migration Animation**: Visual trails showing migrating creatures in flight

---

## 11. Handoff Notes for Agent 1

### Integration Priority

1. **High Priority**: Main loop hookups (init, update, render)
2. **Medium Priority**: Event callbacks for UI feedback
3. **Low Priority**: Camera integration (can default to first island)

### Testing Recommendations

1. Start with 2 islands to verify basic migration
2. Scale to 4-6 islands for archipelago testing
3. Run 5-10 minute sessions to observe divergence
4. Monitor migration statistics in UI
5. Verify memory usage stays stable

### Known Limitations

- Inactive islands update at reduced rate (4x slower) for performance
- Migration survival is probabilistic (some runs may have low migration counts)
- Camera following mode requires active migration event reference
- Genetic distance calculation is simplified (uses fitness/diversity proxy)

### Support

For questions or issues with the multi-island system, refer to:
- This documentation
- Code comments in owned files
- [MultiIslandManager.h](../src/core/MultiIslandManager.h) interface documentation

---

**Document Version**: 1.0
**Last Updated**: 2026-01-16
**Agent**: Agent 3 (Multi-Island Integration)
**Status**: Ready for Agent 1 Integration
