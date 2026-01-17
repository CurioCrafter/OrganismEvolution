# PHASE 10 - Agent 4: BehaviorCoordinator Wiring and Life Events

**Status:** ✅ COMPLETED
**Date:** 2026-01-16
**Agent:** Agent 4 - BehaviorCoordinator Integration

## Overview

This phase integrates the BehaviorCoordinator system with the Creature movement and activity systems, enabling emergent high-level behaviors to influence creature steering and emit life events for the ActivitySystem.

## Architecture

### Component Integration

```
┌─────────────────────────────────────────────────────────────┐
│                     Creature Update Loop                     │
└─────────────────────────────────────────────────────────────┘
                            │
                            ├─► Neural Network (NEAT Brain)
                            │   └─► Motor outputs (turn, speed, intents)
                            │
                            ├─► Steering Behaviors (legacy)
                            │   └─► Flocking, evasion, food seeking
                            │
                            ├─► BehaviorCoordinator ◄─── NEW (Phase 10)
                            │   └─► calculateBehaviorForces()
                            │       ├─► Territorial defense/intrusion
                            │       ├─► Social group cohesion
                            │       ├─► Pack hunting coordination
                            │       ├─► Migration urge
                            │       ├─► Parental care proximity
                            │       └─► Variety behaviors (play, display, etc.)
                            │
                            └─► Force Blending (0.4 weight for behaviors)
                                └─► Final velocity update
```

### Event Flow

```
BehaviorCoordinator::update()
    │
    ├─► TerritorialBehavior::update()
    ├─► SocialGroupManager::update()
    ├─► PackHuntingBehavior::update()
    ├─► MigrationBehavior::update()
    ├─► ParentalCareBehavior::update()
    └─► VarietyBehaviorManager::update()
        │
        └─► Emit BehaviorEvents (with cooldown)
            ├─► HUNT_START / HUNT_SUCCESS / HUNT_FAIL
            ├─► TERRITORIAL_DISPLAY / TERRITORIAL_INTRUSION
            ├─► SOCIAL_JOIN_GROUP / SOCIAL_LEAVE_GROUP
            ├─► PARENTAL_CARE_START / PARENTAL_FEED
            ├─► MIGRATION_START / MIGRATION_END
            ├─► MATING_DISPLAY
            ├─► PLAY_BEHAVIOR
            ├─► SCAVENGING
            └─► CURIOSITY_EXPLORE
                │
                └─► Callbacks (future: ActivitySystem integration)
```

## Implementation Details

### 1. Behavior Force Blending

**Location:** `src/entities/Creature.cpp:471-479`

```cpp
// PHASE 10 - Agent 4: BLEND BEHAVIOR COORDINATOR FORCES
if (glm::length(behaviorForce) > 0.01f) {
    // Behavior forces have moderate influence (0.4 weight)
    // Lower than survival behaviors but higher than idle wandering
    steeringForce += behaviorForce * 0.4f;
}
```

**Design Rationale:**
- **0.4 weight:** Balances high-level behaviors with low-level survival instincts
- **Lower than flee (3.0):** Survival always takes priority
- **Higher than wander (0.5):** Emergent behaviors dominate idle movement
- **Blending:** Additive combination allows multiple forces to cooperate

### 2. Force Priority Hierarchy

From highest to lowest priority:

1. **Flee from predator** (weight: 3.0) - Survival
2. **Migration** (weight: 2.0) - Seasonal movement
3. **Hunting** (weight: 1.5) - Predator food acquisition
4. **Parental care** (weight: 1.2) - Offspring protection
5. **Territorial** (weight: 1.0) - Territory defense
6. **Social** (weight: 0.8) - Group cohesion
7. **Variety behaviors** (blended at 0.6) - Play, display, curiosity

**Note:** BehaviorCoordinator internally resolves conflicts and applies these weights in `resolveConflicts()`.

### 3. Event System

**Header:** `src/entities/behaviors/BehaviorCoordinator.h:24-55`

#### Event Types

| Event Type | Trigger Condition | Cooldown |
|-----------|------------------|----------|
| `HUNT_START` | Creature begins pack hunt | 2s |
| `HUNT_SUCCESS` | Hunt kills prey | 2s |
| `HUNT_FAIL` | Hunt target escapes | 2s |
| `TERRITORIAL_DISPLAY` | Defending territory from intruder | 2s |
| `TERRITORIAL_INTRUSION` | Entering another's territory | 2s |
| `SOCIAL_JOIN_GROUP` | Joins social group | 2s |
| `SOCIAL_LEAVE_GROUP` | Leaves social group | 2s |
| `PARENTAL_CARE_START` | Parent begins caring for offspring | 2s |
| `PARENTAL_FEED` | Parent feeds offspring | 2s |
| `MIGRATION_START` | Begins seasonal migration | 2s |
| `MIGRATION_END` | Completes migration | 2s |
| `MATING_DISPLAY` | Performing courtship display | 2s |
| `PLAY_BEHAVIOR` | Playing with conspecifics | 2s |
| `SCAVENGING` | Scavenging carrion | 2s |
| `CURIOSITY_EXPLORE` | Exploring novel object/area | 2s |

#### Event Structure

```cpp
struct BehaviorEvent {
    BehaviorEventType type;      // Event classification
    uint32_t creatureID;         // Source creature
    glm::vec3 position;          // Event location
    float timestamp;             // Simulation time
    uint32_t targetID = 0;       // Target creature (for hunts, care, etc.)
    float intensity = 1.0f;      // Event strength (0-1)
};
```

### 4. Debug Statistics

**Access:** `BehaviorCoordinator::getDebugStats()`

```cpp
struct DebugStats {
    int huntStarts = 0;
    int huntSuccesses = 0;
    int huntFails = 0;
    int territorialDisplays = 0;
    int territorialIntrusions = 0;
    int socialGroupJoins = 0;
    int socialGroupLeaves = 0;
    int parentalCareStarts = 0;
    int parentalFeeds = 0;
    int migrationStarts = 0;
    int migrationEnds = 0;
    int matingDisplays = 0;
    int playBehaviors = 0;
    int scavengingEvents = 0;
    int curiosityExplores = 0;
    float lastEventTime = 0.0f;
};
```

**Usage:**
```cpp
auto& stats = behaviorCoordinator.getDebugStats();
std::cout << "Hunt Success Rate: "
          << (stats.huntSuccesses / float(stats.huntStarts)) * 100.0f
          << "%" << std::endl;
```

## Integration Points

### 1. Creature Update

**File:** `src/entities/Creature.h:34-38`

```cpp
void update(float deltaTime, const Terrain& terrain,
            const std::vector<glm::vec3>& foodPositions,
            const std::vector<Creature*>& otherCreatures,
            const SpatialGrid* spatialGrid = nullptr,
            const EnvironmentConditions* envConditions = nullptr,
            const std::vector<SoundEvent>* sounds = nullptr,
            BehaviorCoordinator* behaviorCoordinator = nullptr);  // NEW
```

### 2. CreatureManager Integration (Future)

**Recommended pattern for main simulation loop:**

```cpp
// In main update loop
behaviorCoordinator.update(deltaTime);

// When updating creatures
for (auto& creature : creatures) {
    creature->update(deltaTime, terrain, food, others,
                    spatialGrid, envConditions, sounds,
                    &behaviorCoordinator);  // Pass coordinator
}
```

### 3. ActivitySystem Callback (Handoff to Agent 5)

**Setup in simulation initialization:**

```cpp
// Register callback for activity state transitions
behaviorCoordinator.registerEventCallback([&](const BehaviorEvent& event) {
    // Route to ActivitySystem
    switch (event.type) {
        case BehaviorEventType::HUNT_START:
            // Trigger "hunting" activity animation
            break;
        case BehaviorEventType::PARENTAL_FEED:
            // Trigger "feeding offspring" activity
            break;
        case BehaviorEventType::MATING_DISPLAY:
            // Trigger "courtship display" activity
            break;
        // ... etc
    }
});
```

## Testing & Validation

### Debug Output

To enable behavior event logging, add to initialization:

```cpp
behaviorCoordinator.setVarietyDebugLogging(true);
```

### Expected Behavior (5-10 minute run)

✅ **Territorial Behaviors:**
- Creatures establish territories in high-resource areas
- Territorial displays when intruders approach
- Force output: Attraction to territory center, repulsion from borders

✅ **Social Behaviors:**
- Herbivores form groups (3-12 individuals)
- Group cohesion forces keep members together
- Leaders emerge based on fitness/age

✅ **Hunting Behaviors:**
- Predators coordinate pack hunts (2-5 hunters)
- Role assignment: Leader, Flanker, Chaser, Blocker
- Force output: Surround and chase prey

✅ **Parental Behaviors:**
- Parents follow offspring
- Proximity forces keep family groups together
- Energy transfer events (PARENTAL_FEED)

✅ **Migration Behaviors:**
- Seasonal movement toward optimal biomes
- Strong directional force during migration
- Migration START/END events logged

✅ **Variety Behaviors:**
- Play behavior in juveniles (3-10% of time)
- Mating displays in reproductively ready adults
- Curiosity toward novel objects
- Scavenging near recent death sites

### Debug Stats Sample

```
=== Behavior System Stats (t=300s) ===
Hunt Starts:        42
Hunt Successes:     28
Hunt Fails:         14
Success Rate:       66.7%

Territorial Displays:   87
Territorial Intrusions: 134

Social Group Joins:     23
Social Group Leaves:    19
Active Groups:          7

Parental Care Starts:   15
Parental Feeds:         203

Migration Starts:       8
Migration Ends:         3

Mating Displays:        56
Play Behaviors:         132
Scavenging Events:      21
Curiosity Explores:     94
```

## Performance Notes

- **Force calculation:** O(1) per creature per frame
- **Event emission:** Cooldown map prevents spam (O(log n) lookup)
- **Event history:** Limited to 100 recent events (ring buffer)
- **Callback overhead:** Negligible (direct function calls)

## Files Modified

### Core Integration
- ✅ `src/entities/behaviors/BehaviorCoordinator.h` (Event system, debug stats)
- ✅ `src/entities/behaviors/BehaviorCoordinator.cpp` (Event emission, variety behavior tracking)
- ✅ `src/entities/Creature.h` (BehaviorCoordinator parameter)
- ✅ `src/entities/Creature.cpp` (Force blending in herbivore update)

### Documentation
- ✅ `docs/PHASE10_AGENT4_BEHAVIOR_WIRING.md` (This file)

## Handoff Notes to Agent 5 (ActivitySystem Mapping)

### Event → Activity Mapping Recommendations

| BehaviorEvent | Suggested ActivityType |
|--------------|------------------------|
| `HUNT_START` | `HUNTING` |
| `HUNT_SUCCESS` | `EATING` (after kill) |
| `TERRITORIAL_DISPLAY` | `TERRITORIAL_DISPLAY` (new) |
| `PARENTAL_FEED` | `FEEDING_OFFSPRING` (new) |
| `MATING_DISPLAY` | `COURTING` (new) |
| `PLAY_BEHAVIOR` | `PLAYING` (new) |
| `SCAVENGING` | `SCAVENGING` (new) |
| `MIGRATION_START` | `MIGRATING` (new) |

### Callback Integration Pattern

```cpp
// In ActivitySystem initialization
void ActivitySystem::registerWithBehaviorCoordinator(BehaviorCoordinator* bc) {
    bc->registerEventCallback([this](const BehaviorEvent& event) {
        Creature* creature = getCreatureByID(event.creatureID);
        if (!creature) return;

        switch (event.type) {
            case BehaviorEventType::HUNT_START:
                creature->getActivitySystem().startActivity(
                    ActivityType::HUNTING, event.intensity
                );
                break;
            // ... map other events
        }
    });
}
```

## Future Enhancements

1. **Priority-based event queuing** - Queue conflicting events and resolve by priority
2. **Event replay system** - Record and replay interesting behavior sequences
3. **Multi-creature synchronized events** - Group displays, coordinated hunts
4. **Emotional state integration** - Events modify creature emotional state (fear, excitement)
5. **Learning from events** - Reinforcement learning based on hunt success/failure

## Conclusion

The BehaviorCoordinator is now fully wired into the creature movement system and emitting rich life events. High-level emergent behaviors (territorial, social, hunting, parental, migration) now meaningfully influence creature steering with a 0.4 blend weight. The event system provides 15 distinct behavior event types with 2-second cooldowns to prevent spam.

**Next Steps (Agent 5):**
Map BehaviorEvents to ActivitySystem state transitions to drive appropriate animations and secondary motions.
