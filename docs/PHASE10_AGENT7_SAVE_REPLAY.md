# Phase 10 Agent 7: Save/Load and Replay Integrity

## Executive Summary

This document describes the data model alignment and implementation strategy for capturing complete creature state in the save/load and replay systems. The goal is to eliminate placeholder data and ensure all creature attributes (age, generation, behaviors, genome, neural weights) are properly persisted.

## Current State Analysis

### SimCreature Data Model (main.cpp)

The runtime creature structure contains:

```cpp
struct SimCreature {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 facing;
    float energy;
    float fear;
    CreatureType type;
    Genome genome;
    bool alive;
    u32 id;
    bool pooled;
    u32 poolIndex;
    u32 activeListIndex;
};
```

**Missing Fields** (compared to full Creature class):
- `health` - Placeholder: set to 100.0f
- `age` - Placeholder: set to 0.0f
- `generation` - Placeholder: set to 1
- `foodEaten` - Placeholder: set to 0.0f
- `distanceTraveled` - Placeholder: set to 0.0f
- `successfulHunts` - Placeholder: set to 0
- `escapes` - Placeholder: set to 0
- `wanderAngle` - Placeholder: set to 0.0f
- `animPhase` - Placeholder: set to 0.0f

### CreatureSaveData Structure (Serializer.h)

Already contains comprehensive fields for full creature state:

```cpp
struct CreatureSaveData {
    // Identity
    uint32_t id, uint8_t type

    // Transform
    float posX, posY, posZ, velX, velY, velZ, rotation

    // State (CURRENTLY PLACEHOLDERS)
    float health, energy, age
    int32_t generation

    // Behavior tracking (CURRENTLY PLACEHOLDERS)
    float foodEaten, distanceTraveled
    int32_t successfulHunts, escapes
    float wanderAngle, animPhase

    // Genome
    float genomeSize, genomeSpeed, genomeVision, genomeEfficiency
    float genomeColorR, genomeColorG, genomeColorB
    float genomeMutationRate

    // Neural network
    std::vector<float> weightsIH, weightsHO, biasH, biasO
};
```

## Problem Statement

The current save system uses placeholder values for fields not present in `SimCreature`:

```cpp
// main.cpp:3244-3246
data.health = 100.0f;  // SimCreature doesn't track health separately
data.age = 0.0f;       // SimCreature doesn't track age
data.generation = 1;   // SimCreature doesn't track generation per-creature
```

This creates two issues:

1. **Data Loss**: Creature age, generation, and behavior statistics are not persisted
2. **Replay Inaccuracy**: Replays show generic creatures, not the actual evolved population

## Solution Architecture

### Phase 1: Extend SimCreature (Minimal Invasive Approach)

**Option A: Add Required Fields Directly**
```cpp
struct SimCreature {
    // Existing fields...
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 facing;
    float energy;
    float fear;
    CreatureType type;
    Genome genome;
    bool alive;
    u32 id;

    // NEW: Save/replay tracking fields
    float age = 0.0f;           // Time alive
    int32_t generation = 1;     // Evolutionary generation
    float foodEaten = 0.0f;     // Lifetime food consumed
    float distanceTraveled = 0.0f;
    int32_t successfulHunts = 0;
    int32_t escapes = 0;
    float wanderAngle = 0.0f;   // Current wander behavior angle
    float animPhase = 0.0f;     // Animation phase for continuity

    // Pooling fields (existing)
    bool pooled;
    u32 poolIndex;
    u32 activeListIndex;
};
```

**Option B: Embed Tracking Structure**
```cpp
struct CreatureTracking {
    float age = 0.0f;
    int32_t generation = 1;
    float foodEaten = 0.0f;
    float distanceTraveled = 0.0f;
    int32_t successfulHunts = 0;
    int32_t escapes = 0;
    float wanderAngle = 0.0f;
    float animPhase = 0.0f;
};

struct SimCreature {
    // Existing fields...
    CreatureTracking tracking;  // NEW
};
```

**Recommendation**: Option A (direct fields) for simplicity and zero overhead.

### Phase 2: Update SaveSystemIntegration.h

Modify `buildCreatureSaveData` to use real data:

```cpp
template<typename CreatureType, typename NeuralNetType>
CreatureSaveData buildCreatureSaveData(const CreatureType& creature, const NeuralNetType& brain) {
    CreatureSaveData data;

    // ... existing position/velocity/genome code ...

    // UPDATED: Use real creature data
    data.health = creature.energy;  // Use energy as health proxy (or add health field)
    data.energy = creature.energy;
    data.age = creature.age;                    // NOW REAL
    data.generation = creature.generation;      // NOW REAL

    data.foodEaten = creature.foodEaten;        // NOW REAL
    data.distanceTraveled = creature.distanceTraveled;  // NOW REAL
    data.successfulHunts = creature.successfulHunts;    // NOW REAL
    data.escapes = creature.escapes;            // NOW REAL
    data.wanderAngle = creature.wanderAngle;    // NOW REAL
    data.animPhase = creature.animPhase;        // NOW REAL

    // ... neural network weights ...

    return data;
}
```

### Phase 3: Replay Frame Enhancement

Ensure `CreatureSnapshot` (ReplaySystem.h) includes behavior state:

```cpp
struct CreatureSnapshot {
    // Existing fields...
    uint32_t id;
    uint8_t type;
    float posX, posY, posZ, rotation;
    float health, energy, animPhase;
    float colorR, colorG, colorB, size;

    // NEW: Genome and behavior data for accurate replay
    float genomeSpeed, genomeSize, genomeVision;

    // ENHANCED: Add age and generation for replay visualization
    float age = 0.0f;           // Display creature age in replay UI
    int32_t generation = 1;     // Show generation in replay stats

    // Neural network weights (already present - S-07 fix)
    std::vector<float> neuralWeightsIH;
    std::vector<float> neuralWeightsHO;
    std::vector<float> neuralBiasH;
    std::vector<float> neuralBiasO;
};
```

Update `buildCreatureSnapshot` helper:

```cpp
template<typename CreatureType>
CreatureSnapshot buildCreatureSnapshot(const CreatureType& creature) {
    CreatureSnapshot snap;

    snap.id = creature.id;
    snap.type = static_cast<uint8_t>(creature.type);
    snap.posX = creature.position.x;
    snap.posY = creature.position.y;
    snap.posZ = creature.position.z;
    snap.rotation = std::atan2(creature.facing.x, creature.facing.z);
    snap.health = creature.energy;  // Use energy as health
    snap.energy = creature.energy;
    snap.animPhase = creature.animPhase;
    snap.colorR = creature.genome.color.x;
    snap.colorG = creature.genome.color.y;
    snap.colorB = creature.genome.color.z;
    snap.size = creature.genome.size;

    // Genome data (already present)
    snap.genomeSpeed = creature.genome.speed;
    snap.genomeSize = creature.genome.size;
    snap.genomeVision = creature.genome.visionRange;

    // NEW: Behavior data for replay
    snap.age = creature.age;
    snap.generation = creature.generation;

    // Neural weights would be populated by caller if needed

    return snap;
}
```

### Phase 4: Backward Compatibility

Support loading older saves that lack new fields:

```cpp
// In CreatureSaveData::read()
void read(BinaryReader& reader, uint32_t version = SaveConstants::CURRENT_VERSION) {
    // Read standard fields...
    id = reader.read<uint32_t>();
    type = reader.read<uint8_t>();
    // ... position, velocity, rotation ...

    health = reader.read<float>();
    energy = reader.read<float>();
    age = reader.read<float>();
    generation = reader.read<int32_t>();

    foodEaten = reader.read<float>();
    distanceTraveled = reader.read<float>();
    successfulHunts = reader.read<int32_t>();
    escapes = reader.read<int32_t>();
    wanderAngle = reader.read<float>();
    animPhase = reader.read<float>();

    // Genome...
    // Neural weights...
}
```

Since the current format already allocates space for these fields, **no version bump is required**. Older saves will have zero/default values which is acceptable.

## Implementation Strategy

### Files to Modify

1. **src/main.cpp** (SimCreature definition)
   - Add age, generation, tracking fields to `SimCreature`
   - Update creature spawning code to initialize generation
   - Update creature update loops to increment age, track stats

2. **src/core/SaveSystemIntegration.h**
   - Update `buildCreatureSaveData` to use real values
   - Update `buildCreatureSnapshot` to include age/generation

3. **src/core/ReplaySystem.h** (if needed)
   - Add age/generation fields to `CreatureSnapshot`
   - Update write/read methods

4. **src/core/Serializer.h** (versioning if needed)
   - Already has all required fields
   - No changes needed unless adding new fields

### Execution Sequence

```
1. Add fields to SimCreature struct
   └─> Update constructors and Reset() method
       └─> Initialize age=0.0f, generation=1

2. Update creature spawning logic
   └─> Set generation = parent.generation + 1 for offspring
   └─> Initialize age = 0.0f for newborns

3. Update creature update loop
   └─> Increment age += deltaTime
   └─> Track foodEaten += amount when eating
   └─> Track distanceTraveled += distance each frame
   └─> Track successfulHunts++ when kill occurs
   └─> Track escapes++ when evading predator

4. Update SaveSystemIntegration.h
   └─> Remove placeholder comments
   └─> Map SimCreature fields to CreatureSaveData

5. Test save/load
   └─> Create population with varied generations
   └─> Save game
   └─> Reload and verify age/generation persist

6. Test replay
   └─> Record replay session
   └─> Play back and verify creature stats display
```

## Data Flow Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                     Runtime Simulation                       │
│  SimCreature {                                               │
│    position, velocity, energy, fear,                         │
│    age, generation, foodEaten, ...  ← UPDATED                │
│  }                                                            │
└───────────────┬─────────────────────────────────────────────┘
                │
                ├───── Save Game ─────┐
                │                     │
                v                     v
    ┌───────────────────────┐  ┌─────────────────────────┐
    │ buildCreatureSaveData │  │ buildCreatureSnapshot   │
    │  (SaveSystemInt.h)    │  │  (SaveSystemInt.h)      │
    └───────┬───────────────┘  └───────┬─────────────────┘
            │                          │
            v                          v
    ┌───────────────────┐      ┌──────────────────┐
    │ CreatureSaveData  │      │ CreatureSnapshot │
    │  .age = real      │      │  .age = real     │
    │  .generation=real │      │  .gen = real     │
    └───────┬───────────┘      └───────┬──────────┘
            │                          │
            v                          v
    ┌───────────────────┐      ┌──────────────────┐
    │  SaveManager      │      │  ReplayRecorder  │
    │  writes to disk   │      │  writes to disk  │
    └───────────────────┘      └──────────────────┘
```

## Validation Plan

### Test Case 1: Save/Load Persistence

1. Start new simulation
2. Wait for generation 5+ creatures
3. Select oldest creature, note its age
4. Quick-save (F5)
5. Quick-load (F9)
6. Verify:
   - Creature age matches pre-save value
   - Generation numbers preserved
   - Food eaten counts preserved

### Test Case 2: Replay Accuracy

1. Start recording replay
2. Run simulation for 5 minutes
3. Note generation distribution in population
4. Stop recording
5. Play back replay
6. Verify:
   - Creature ages increase over replay time
   - Generation labels show correct values
   - Oldest creatures display high age values

### Test Case 3: Backward Compatibility

1. Load a save file from before this change
2. Verify:
   - Creatures load without crash
   - Missing fields default to 0/1 appropriately
   - Simulation runs normally
   - New tracking starts accumulating from load point

## Performance Considerations

### Memory Impact

```
Before: sizeof(SimCreature) ≈ 64 bytes
After:  sizeof(SimCreature) ≈ 96 bytes (+50%)

For 10,000 creatures:
  Memory increase: ~320 KB (negligible)
```

### CPU Impact

- Age increment: `age += deltaTime` → 1 FLOP per creature per frame
- Distance tracking: `dist += length(vel) * dt` → ~5 FLOPs per creature per frame
- Total overhead: <0.1% on modern CPUs

**Verdict**: Performance impact is negligible.

## Handoff Notes for Agent 1 (Runtime Wiring)

### Required Changes in main.cpp

1. **Add fields to SimCreature** (line 190)
2. **Update Reset() method** to initialize tracking fields
3. **Update creature spawn logic** (CreaturePool::acquire):
   ```cpp
   creature->age = 0.0f;
   creature->generation = parentGeneration + 1;  // Track lineage
   ```

4. **Update SimulationWorld::Update()**:
   ```cpp
   // In creature update loop:
   creature->age += deltaTime;

   // When creature eats food:
   creature->foodEaten += foodAmount;

   // Each frame:
   glm::vec3 movement = creature->velocity * deltaTime;
   creature->distanceTraveled += glm::length(movement);

   // When predator kills prey:
   predator->successfulHunts++;

   // When prey escapes:
   prey->escapes++;
   ```

5. **Update save/load calls** (already using SaveSystemIntegration.h):
   - No changes needed - templates will pick up new fields automatically

### Integration Points

```cpp
// main.cpp save function (currently at line ~3220)
bool SimulationWorld::SaveGame(const std::string& filename) {
    // ... existing setup code ...

    for (u32 i = 0; i < m_activeCreatures.size(); ++i) {
        SimCreature* c = m_activeCreatures[i];
        if (!c || !c->alive) continue;

        // buildCreatureSaveData will now use real age/generation
        auto data = buildCreatureSaveData(*c, /* neural net */);
        creatureData.push_back(data);
    }

    // ... rest of save code ...
}
```

No code changes required - the template function updates automatically.

## File Manifest

### Modified Files

| File | Purpose | Changes |
|------|---------|---------|
| `src/main.cpp` | Add tracking to SimCreature | Add 8 new fields, update Reset(), update loops |
| `src/core/SaveSystemIntegration.h` | Remove placeholders | Update buildCreatureSaveData template |
| `src/core/ReplaySystem.h` | Add replay metadata | Add age/generation to CreatureSnapshot |
| `docs/PHASE10_AGENT7_SAVE_REPLAY.md` | Documentation | This file |

### Unmodified Files (Design-Ready)

| File | Status | Notes |
|------|--------|-------|
| `src/core/SaveManager.h` | No changes needed | Already supports all fields |
| `src/core/Serializer.h` | No changes needed | CreatureSaveData already complete |
| `src/entities/Creature.h` | Reference only | Full creature has all fields |

## Versioning Strategy

### Current Save Format Version

```cpp
// SaveConstants::CURRENT_VERSION = 2
// Version 2 already includes:
//   - RNG state
//   - maxGeneration
//   - nextCreatureId
```

### No Version Bump Required

The current `CreatureSaveData` structure already allocates all the fields we need. Older saves will simply have default values (0 for age, 1 for generation), which is semantically correct for "data not tracked at the time."

### Future Versioning

If we need to add **new** fields beyond what's in `CreatureSaveData`:

1. Increment `SaveConstants::CURRENT_VERSION` to 3
2. Add version-aware read:
   ```cpp
   void CreatureSaveData::read(BinaryReader& reader, uint32_t version) {
       // Read existing fields...

       if (version >= 3) {
           // Read new fields introduced in v3
           newField = reader.read<float>();
       } else {
           // Default for older saves
           newField = defaultValue;
       }
   }
   ```

## Migration Path

### For Existing Saves

1. Old saves load with default age=0, generation=1
2. Simulation resumes normally
3. New tracking starts accumulating from load point
4. Re-save updates to full tracking data

### For Existing Replays

1. Old replays display age=0, generation=1 for all creatures
2. Playback functions normally
3. New recordings capture full tracking data

## Success Criteria

✅ **Data Integrity**
- All creature fields persist across save/load
- No data loss for age, generation, behavior stats

✅ **Replay Accuracy**
- Replay shows actual creature ages
- Generation numbers visible in replay UI
- Behavior statistics available for analysis

✅ **Backward Compatibility**
- Old saves load without errors
- Old replays play without crashes
- Default values are sensible

✅ **Performance**
- <1% CPU overhead for tracking
- <500KB memory overhead for 10K creatures
- No frame time regression

## Next Steps (For Agent 1)

1. Review this document for architecture approval
2. Implement SimCreature field additions
3. Wire up tracking code in update loops
4. Test save/load with new data
5. Verify replay displays correct metadata
6. Validate backward compatibility with old saves

## Appendix: Field Mapping Reference

| Runtime (SimCreature) | Save (CreatureSaveData) | Replay (CreatureSnapshot) |
|-----------------------|-------------------------|---------------------------|
| `id` | `id` | `id` |
| `type` | `type` | `type` |
| `position` | `posX, posY, posZ` | `posX, posY, posZ` |
| `velocity` | `velX, velY, velZ` | (not stored) |
| `facing` | `rotation` (computed) | `rotation` (computed) |
| `energy` | `energy` | `energy` |
| `energy` (proxy) | `health` | `health` |
| **`age`** ✨ | `age` | `age` ✨ |
| **`generation`** ✨ | `generation` | `generation` ✨ |
| **`foodEaten`** ✨ | `foodEaten` | (not stored) |
| **`distanceTraveled`** ✨ | `distanceTraveled` | (not stored) |
| **`successfulHunts`** ✨ | `successfulHunts` | (not stored) |
| **`escapes`** ✨ | `escapes` | (not stored) |
| **`wanderAngle`** ✨ | `wanderAngle` | (not stored) |
| **`animPhase`** ✨ | `animPhase` | `animPhase` |
| `genome.*` | `genome*` fields | `genome*` fields (partial) |

✨ = NEW TRACKING (currently placeholder)

---

**Document Status**: Ready for Implementation
**Target Milestone**: Phase 10 - Save/Replay Integrity
**Dependencies**: None (standalone enhancement)
**Risk Level**: Low (additive changes, no breaking modifications)
