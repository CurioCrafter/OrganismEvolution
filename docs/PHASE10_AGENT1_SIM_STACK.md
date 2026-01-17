# Phase 10 Agent 1: Simulation Stack Unification

**Goal:** Replace SimCreature/SimulationWorld with CreatureManager and Creature systems for unified runtime simulation.

**Status:** In Progress
**Date:** 2026-01-16

---

## 1. Field Mapping (SimCreature → Creature)

| SimCreature Field | Creature Equivalent | Notes |
|------------------|---------------------|-------|
| `glm::vec3 position` | `getPosition()` | Getter access, stored internally |
| `glm::vec3 velocity` | `getVelocity()` | Getter access, stored internally |
| `glm::vec3 facing` | Not directly stored | Derived from velocity direction |
| `float energy` | `getEnergy()` | Getter access, `setEnergy()`, `addEnergy()` for modification |
| `float fear` | `getFear()` | Part of creature's emotional state |
| `CreatureType type` | `getType()` | Immutable after spawn |
| `Genome genome` | `getGenome()`, `getDiploidGenome()` | Now supports sophisticated diploid genetics |
| `bool alive` | `isAlive()`, `isActive()` | Consistent lifecycle tracking |
| `u32 id` | `getID()`, `getId()` | Unique identifier |
| `bool pooled` | **REMOVED** | CreatureManager handles pooling internally |
| `u32 poolIndex` | **REMOVED** | Internal to CreatureManager |
| `u32 activeListIndex` | **REMOVED** | Internal to CreatureManager |

### Additional Creature Capabilities (Not in SimCreature)
- **Diploid Genetics:** Full genetic system with chromosomes, genes, alleles
- **Species Tracking:** `getSpeciesId()`, `getSpeciesDisplayName()`
- **NEAT Brain:** Evolved neural topology via `initializeNEATBrain()`
- **Animation System:** Skeletal animation, activity states, transitions
- **Sensory System:** Vision, hearing, smell, communication
- **Activity System:** Sleep, grooming, mating, parental care
- **Climate Response:** Migration, thermoregulation, stress
- **Amphibious Transitions:** Smooth land/water adaptation (Phase 7)

---

## 2. Current Architecture (Before Migration)

### SimCreature (src/main.cpp:190)
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

### SimulationWorld (src/main.cpp:574)
```cpp
class SimulationWorld {
public:
    std::vector<std::unique_ptr<SimCreature>> creatures;
    // ... spawning, update, food management
};
```

### AppState (src/main.cpp:1914)
```cpp
struct AppState {
    SimulationWorld world;  // ← Currently using SimCreature-based system
    // ...
};
```

### Main Loop Integration
- **Spawn:** `SimulationWorld::initialize()` spawns creatures using `spawnCreature` lambda
- **Update:** Inline update loops in `UpdateSimulation()` for each creature type
- **Render:** Builds `CreatureInput` arrays from `SimCreature` fields
- **Compute:** GPU steering uses `SimCreature` positions/velocities
- **GameplayManager:** Currently receives `nullptr` instead of `CreatureManager*`

---

## 3. Target Architecture (After Migration)

### CreatureManager Integration
```cpp
struct AppState {
    // Ecosystem Systems
    Terrain terrain;
    EcosystemManager ecosystem;

    // NEW: Unified creature management
    Forge::CreatureManager creatureManager;

    // OLD: Will be removed or gated behind compat flag
    SimulationWorld world;

    // Gameplay Systems
    Forge::GameplayManager gameplay;
};
```

### Initialization Sequence
1. **App Init:**
   ```cpp
   g_app.creatureManager = Forge::CreatureManager(500.0f, 500.0f);
   g_app.creatureManager.init(&g_app.terrain, &g_app.ecosystem, seed);
   ```

2. **Spawn Initial Population:**
   ```cpp
   // Replace SimulationWorld::initialize()
   for (int i = 0; i < herbivoreCount; ++i) {
       CreatureHandle handle = creatureManager.spawn(
           CreatureType::HERBIVORE,
           randomPosition,
           nullptr  // No parent genome
       );
   }
   ```

3. **Per-Frame Update:**
   ```cpp
   // Replace inline SimCreature update loops
   creatureManager.forEach([&](Creature& creature, size_t index) {
       creature.update(deltaTime, terrain, foodPositions,
                      otherCreatures, spatialGrid, envConditions, sounds);
   });

   creatureManager.update(deltaTime);  // Process deaths, stats
   creatureManager.rebuildSpatialGrids();
   ```

4. **Render Adapter:**
   ```cpp
   std::vector<CreatureInput> inputs;
   creatureManager.forEach([&](Creature& c, size_t i) {
       CreatureInput input;
       input.position = c.getPosition();
       input.velocity = c.getVelocity();
       input.energy = c.getEnergy();
       input.fear = c.getFear();
       input.type = static_cast<int>(c.getType());
       inputs.push_back(input);
   });
   ```

5. **GameplayManager Connection:**
   ```cpp
   gameplay.update(deltaTime, simulationTime, &creatureManager);  // ← No longer nullptr!
   ```

---

## 4. Migration Phases

### Phase 1: CreatureManager Bootstrap ✅
**Goal:** Add CreatureManager to app state and wire up initialization.

**Changes:**
- Add `Forge::CreatureManager creatureManager` to `AppState`
- Initialize after Terrain/EcosystemManager creation
- Keep `SimulationWorld` alongside (compatibility mode)

**Files Modified:**
- `src/main.cpp` (AppState struct, initialization)

**Validation:**
- App starts without crashes
- Both systems coexist (SimulationWorld still active)

---

### Phase 2: Spawn Migration ⏳
**Goal:** Replace SimulationWorld spawning with CreatureManager::spawn.

**Changes:**
- Replace `spawnCreature` lambda in `SimulationWorld::initialize()`
- Use `creatureManager.spawn()` for all creature types
- Ensure aquatic/flying creatures spawn in valid locations

**Files Modified:**
- `src/main.cpp` (SimulationWorld::initialize)

**Validation:**
- Creatures spawn at correct positions (land, water, air)
- Initial population matches expectations
- No double-spawning (disable SimulationWorld spawn if both active)

---

### Phase 3: Update Loop Migration ⏳
**Goal:** Replace inline SimCreature update with Creature::update.

**Changes:**
- Replace per-type update loops with `creatureManager.forEach()`
- Call `Creature::update()` with full environment context
- Remove SimCreature update code or gate behind flag

**Files Modified:**
- `src/main.cpp` (UpdateSimulation function)

**Validation:**
- Creatures move, eat, reproduce, die correctly
- No performance regression (should be faster with spatial grid)
- 5-10 minute continuous run without crashes

---

### Phase 4: Render/Compute Adapter ⏳
**Goal:** Build CreatureInput arrays from CreatureManager for GPU systems.

**Changes:**
- Adapter function: `creatureManager → CreatureInput[]`
- Update GPU compute input building
- Update instanced rendering input

**Files Modified:**
- `src/main.cpp` (render/compute input preparation)

**Validation:**
- Creatures render correctly
- GPU steering works (if enabled)
- Visual fidelity matches previous behavior

---

### Phase 5: GameplayManager Integration ⏳
**Goal:** Connect GameplayManager to CreatureManager.

**Changes:**
- Pass `&creatureManager` instead of `nullptr` to `gameplay.update()`
- Enable all gameplay features (achievements, spotlight, records)
- Remove `updatePopulation()` manual calls

**Files Modified:**
- `src/main.cpp` (GameplayManager update calls)

**Validation:**
- Achievements unlock correctly
- Statistics track properly
- Spotlight feature works

---

### Phase 6: Cleanup & Compatibility ⏳
**Goal:** Remove unused code and add rollback mechanism.

**Changes:**
- Remove `SimulationWorld` or gate behind `#define USE_LEGACY_SIM_WORLD`
- Clean up unused SimCreature update paths
- Add logging: creature count, update time

**Files Modified:**
- `src/main.cpp` (cleanup, logging)

**Validation:**
- No dead code paths
- Clean compile with no warnings
- Rollback flag works if needed

---

## 5. Call Chain Diagram

### Before (SimCreature-based):
```
main()
├─ InitializeApp()
│  └─ world.initialize() → spawn SimCreatures
├─ UpdateSimulation()
│  ├─ for (herbivores) { update inline }
│  ├─ for (carnivores) { update inline }
│  ├─ for (aquatic) { update inline }
│  └─ for (flying) { update inline }
├─ BuildCreatureInputs()
│  └─ SimCreature → CreatureInput
└─ RenderCreatures()
   └─ Use CreatureInput array
```

### After (CreatureManager-based):
```
main()
├─ InitializeApp()
│  ├─ terrain.init()
│  ├─ ecosystem.init()
│  ├─ creatureManager.init(&terrain, &ecosystem)
│  └─ SpawnInitialPopulation()
│     └─ creatureManager.spawn(type, pos, genome)
├─ UpdateSimulation()
│  ├─ creatureManager.forEach([](Creature& c) {
│  │     c.update(dt, terrain, food, others, grid, env, sounds)
│  │  })
│  ├─ creatureManager.update(dt)  // Process deaths
│  └─ creatureManager.rebuildSpatialGrids()
├─ gameplay.update(dt, time, &creatureManager)
├─ BuildCreatureInputsFromManager()
│  └─ Creature → CreatureInput
└─ RenderCreatures()
   └─ Use CreatureInput array
```

---

## 6. Compatibility Flags

### Full Replacement (Default)
```cpp
#define USE_CREATURE_MANAGER 1
```

### Adapter Layer (Fallback)
```cpp
#define USE_LEGACY_SIM_WORLD 1
// Keep SimulationWorld active, sync to CreatureManager for features
```

### Rollback Plan
If critical issues arise:
1. Set `USE_LEGACY_SIM_WORLD = 1`
2. Comment out CreatureManager update calls
3. Revert to SimCreature-based update loops
4. File issue for investigation

---

## 7. Performance Expectations

| Metric | Before (SimCreature) | After (CreatureManager) | Notes |
|--------|---------------------|-------------------------|-------|
| Update O() | O(n²) nearby search | O(1) spatial grid | Major improvement |
| Memory | Fragmented vectors | Pooled allocation | Better cache locality |
| Creature Cap | ~1000 practical | 65536 theoretical | Scalable architecture |
| Features | Basic simulation | Full ecosystem | Animation, AI, genetics |

---

## 8. Validation Checklist

- [ ] **Build Success:** Project compiles without errors
- [ ] **Spawn Test:** All creature types spawn in valid locations
- [ ] **Movement Test:** Creatures move naturally (land, water, air)
- [ ] **Reproduction Test:** Creatures reproduce with genetic crossover
- [ ] **Death Test:** Creatures die from starvation, predation, age
- [ ] **Rendering Test:** All creatures visible with correct appearance
- [ ] **GPU Compute Test:** GPU steering works (if enabled)
- [ ] **GameplayManager Test:** Achievements unlock, stats track
- [ ] **5-Minute Run:** No crashes, memory leaks, or freezes
- [ ] **Performance Test:** FPS >= baseline (preferably better)

---

## 9. Known Integration Points (Handoff Notes)

### For Animation Agent (Agent 7, 8, 10):
- `Creature::getAnimator()` provides full animation system
- `ActivityStateMachine` drives contextual behaviors
- No changes needed to animation code

### For UI Agent (Agent 8):
- `CreatureManager::getSelectedCreature()` provides inspection target
- `PopulationStats` updated automatically each frame
- Existing UI can query Creature methods directly

### For Environment Agent (Agent 4, 5, 6):
- `Creature::updateClimateResponse()` already integrated
- Migration system uses `CreatureManager::forEach()`
- No changes needed to climate/terrain code

### For Physics Agent (Agent 2, 3):
- `Creature::update()` handles physics internally
- Amphibious transitions managed by `CreatureManager::updateAmphibiousTransitions()`
- No changes needed to physics code

---

## 10. Testing Notes

### What I Saw:
- (To be filled after implementation and testing)

### Issues Encountered:
- (To be filled if any)

### Performance Measurements:
- (To be filled after benchmarking)

---

## 11. Next Steps (After This Agent)

1. **Agent 2:** Ensure physics integration is correct
2. **Agent 3:** Verify amphibious transitions work in live sim
3. **Agent 7, 8, 10:** Validate animation and activity systems
4. **Agent 26:** Enable full GameplayManager features
5. **Integration Test:** Run extended simulation (30+ minutes)

---

**Last Updated:** 2026-01-16
**Agent:** Phase 10 Agent 1 (SimStack Unification)
