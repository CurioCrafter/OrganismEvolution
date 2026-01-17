# Phase 7: Behavior Variety Expansion

## Overview

This document describes the behavior variety system implemented in Phase 7, Agent 4. The goal is to make creatures feel unpredictable and alive through diverse behaviors with clear triggers, smooth transitions, and per-creature personality differences.

## New Files Created

| File | Purpose |
|------|---------|
| `src/entities/behaviors/VarietyBehaviors.h` | Header defining new behavior types, memory system, personality traits, and state machine |
| `src/entities/behaviors/VarietyBehaviors.cpp` | Implementation of variety behaviors and aquatic group dynamics |

## Modified Files

| File | Changes |
|------|---------|
| `src/entities/behaviors/BehaviorCoordinator.h` | Added VarietyBehaviorManager integration, new stats, enable/disable |
| `src/entities/behaviors/BehaviorCoordinator.cpp` | Integrated variety forces into behavior calculation pipeline |
| `src/entities/aquatic/FishSchooling.h` | Added SchoolDynamics for group split/rejoin, panic waves, leader following |
| `src/entities/aquatic/FishSchooling.cpp` | Implementation of extended schooling dynamics |

## New Behaviors

### 1. Curiosity/Exploration

**Trigger Conditions:**
- Novelty score > 0.3 (position not recently visited)
- Energy ratio > 40% (not too hungry)
- Personality curiosity trait influences likelihood

**Behavior Phases:**
1. `CURIOSITY_APPROACH` - Move toward novel stimulus
2. `CURIOSITY_INSPECT` - Circle around target while observing

**Exit Conditions:**
- Duration expires (8-12 seconds based on curiosity trait)
- Target reached and inspection complete
- Higher priority behavior triggers

**Energy Cost:** Low (cautious movement speed)

---

### 2. Mating Ritual/Display

**Trigger Conditions:**
- Creature can reproduce (energy/age requirements met)
- Potential mate detected in vision range
- Same species and compatible for mating

**Behavior Phases:**
1. `MATING_DISPLAY` - Perform display dance (circular movement pattern with bobbing)
2. `MATING_APPROACH` - Move toward mate after display completes

**Exit Conditions:**
- Display completes (5-8 seconds based on patience trait)
- Mate no longer available
- Close enough to mate (distance < 2 units)

**Energy Cost:** Moderate (0.5 energy/second during display)

---

### 3. Scavenging

**Trigger Conditions:**
- Energy ratio < 60% (hungry)
- Carcass detected in memory or vision range
- Lower aggression creatures more likely to scavenge

**Behavior Phases:**
1. `SCAVENGING_SEEK` - Move toward carcass location
2. `SCAVENGING_FEED` - Consume food at carcass site

**Exit Conditions:**
- Arrival at carcass
- Carcass depleted
- Higher priority behavior (predator detected)

**Energy Cost:** Movement cost only during seek phase

---

### 4. Playing (Juvenile Behavior)

**Trigger Conditions:**
- Age < 30 seconds
- Sociability trait > 0.5
- Energy ratio > 60%

**Behavior Pattern:**
- Figure-eight movement pattern
- Quick, unpredictable direction changes
- Occasional vertical jumps

**Exit Conditions:**
- Duration expires (10-15 seconds)
- Cooldown: 15 seconds before can play again

---

### 5. Resting

**Trigger Conditions:**
- Energy ratio < 30% (exhausted)
- No immediate threats

**Behavior Pattern:**
- Minimal movement (slight random shifts)
- Energy conservation mode

**Exit Conditions:**
- Energy recovered or threat detected
- Duration: 5-15 seconds based on energy deficit

---

## Memory System

### CreatureMemory Structure

```cpp
struct CreatureMemory {
    // Food memory (decays in 30 seconds)
    glm::vec3 lastFoodLocation;
    float lastFoodTime;

    // Threat memory (decays in 15 seconds)
    glm::vec3 lastThreatLocation;
    uint32_t lastThreatId;
    float lastThreatTime;

    // Mate memory (decays in 60 seconds)
    glm::vec3 lastMateLocation;
    uint32_t lastMateId;

    // Exploration memory (up to 10 recent locations)
    std::vector<glm::vec3> recentLocations;

    // Carcass memory (decays in 45 seconds)
    glm::vec3 lastCarcassLocation;
    float lastCarcassTime;
};
```

### Novelty Score Calculation

The novelty score determines how "new" a location is:

```
novelty = min(1.0, minDistanceToKnownLocations / 50.0)
```

Higher novelty encourages curiosity behavior.

---

## Personality System

### BehaviorPersonality Traits

| Trait | Range | Influences |
|-------|-------|------------|
| `curiosity` | 0.0-1.0 | Exploration likelihood, inspection duration |
| `aggression` | 0.0-1.0 | Scavenging preference (inverse), attack threshold |
| `sociability` | 0.0-1.0 | Play behavior, group cohesion |
| `boldness` | 0.0-1.0 | Risk tolerance, flee threshold |
| `patience` | 0.0-1.0 | Mating display duration, waiting behaviors |

### Trait Derivation from Genome

```cpp
void BehaviorPersonality::initFromGenome(float genomeAggression, float genomeSize, float genomeSpeed) {
    aggression = genomeAggression;
    boldness = clamp(0.3f + genomeSize * 0.5f, 0.0f, 1.0f);  // Larger = bolder
    curiosity = clamp(0.4f + genomeSpeed * 0.3f, 0.0f, 1.0f); // Faster = more curious
    sociability = clamp(0.6f - genomeAggression * 0.3f, 0.0f, 1.0f);
    patience = clamp(0.7f - genomeAggression * 0.4f, 0.0f, 1.0f);
}
```

Each creature also receives ±10% random variation on all traits for individuality.

---

## Behavior Priority System

### Priority Levels (Higher = More Important)

| Priority | Value | Behaviors |
|----------|-------|-----------|
| SURVIVAL | 10.0 | Flee, avoid danger |
| MATING | 6.0 | Reproduction |
| HUNGER | 5.0 | Scavenging when hungry |
| CURIOSITY | 3.0 | Exploration |
| SOCIAL | 2.0 | Playing, grooming |
| IDLE | 1.0 | Wandering |

### Conflict Resolution

Survival behaviors always take precedence. Variety behaviors only contribute force when:
- Not actively fleeing (flee magnitude < 0.1)
- Not actively hunting (hunting magnitude < 0.1)

Variety force weight: 0.6 (moderate influence on movement)

---

## Cooldown Timers

| Behavior | Cooldown |
|----------|----------|
| Curiosity | 8 seconds |
| Mating Display | 20 seconds |
| Scavenging | 10 seconds |
| Playing | 15 seconds |

---

## Aquatic Group Dynamics

### SchoolDynamics States

| State | Description | Intensity |
|-------|-------------|-----------|
| CRUISING | Normal movement | 1.0 |
| FEEDING_FRENZY | Aggressive feeding | 0.7 (looser) |
| PANIC_SCATTER | Split due to predator | 0.3 (minimum) |
| REFORMING | Rejoining after scatter | Increasing |
| LEADER_FOLLOWING | Following designated leader | 1.5 (tighter) |
| DEPTH_MIGRATION | Vertical movement | 1.0 |

### Panic Wave Propagation

When a predator is detected:
1. Panic wave originates at predator position
2. Wave expands at 15 units/second
3. Fish near wavefront flee outward
4. Wave dissipates after reaching 100 units radius
5. School enters REFORMING state

### Leader Selection

Leader score calculation:
```cpp
float score = (age/100 * 0.3) + (size/2 * 0.3) + (energy * 0.2) + (survivalTime/300 * 0.2);
```

Fish with highest score becomes leader; others follow at configurable distance.

---

## Integration Points for Agent 10 (UI Debug)

### Available Debug Hooks

1. **Behavior State Query:**
   ```cpp
   const BehaviorState* state = coordinator.getVarietyBehaviors().getBehaviorState(creatureId);
   std::string stateName = state->getStateName();
   ```

2. **Memory Inspection:**
   ```cpp
   const CreatureMemory* memory = coordinator.getVarietyBehaviors().getMemory(creatureId);
   bool hasFood = memory->hasValidFoodMemory;
   ```

3. **Personality Query:**
   ```cpp
   const BehaviorPersonality* personality = coordinator.getVarietyBehaviors().getPersonality(creatureId);
   float curiosity = personality->curiosity;
   ```

4. **Statistics:**
   ```cpp
   BehaviorCoordinator::BehaviorStats stats = coordinator.getStats();
   int curiosityCount = stats.curiosityBehaviors;
   int matingDisplays = stats.matingDisplays;
   ```

5. **Debug Logging Toggle:**
   ```cpp
   coordinator.setVarietyDebugLogging(true);  // Logs all behavior transitions
   ```

---

## Sensory Input to Behavior Weight Mapping

| Sensory Input | Behavior Affected | Weight Modification |
|---------------|-------------------|---------------------|
| nearestFoodDistance < 0.3 | Scavenging | +0.3 urgency |
| nearestPredatorDistance < 0.5 | All variety | Suppressed (flee priority) |
| nearestMateDistance < 0.6 | Mating Display | +0.4 urgency |
| energy < 0.4 | Resting | +0.5 urgency |
| energy < 0.6 | Scavenging | +(1-energy) urgency |
| noveltyScore > 0.5 | Curiosity | +noveltyScore * 0.5 |
| age < 30s | Playing | +(1 - age/60) * sociability |

---

## Validation Checklist

- [x] Behaviors have clear trigger conditions
- [x] Exit conditions prevent stuck states
- [x] Cooldowns prevent rapid flipping
- [x] Priority system resolves conflicts
- [x] Personality creates individual variation
- [x] Memory decays over time
- [x] Aquatic group dynamics implemented
- [x] Debug hooks available for UI
- [x] Statistics tracking implemented

---

## Testing Recommendations

1. **Variety Observation (2-3 minutes):**
   - Spawn 20-30 creatures
   - Enable debug logging
   - Observe behavior transitions in console
   - Verify variety (not all creatures doing same thing)

2. **Personality Consistency:**
   - Track specific creature over time
   - Verify consistent personality (curious creature stays curious)
   - Confirm random variation creates differences

3. **Priority Conflicts:**
   - Introduce predator near curious creature
   - Verify immediate transition to flee
   - Confirm curiosity cooldown applied

4. **Aquatic Group Split/Rejoin:**
   - Spawn fish school
   - Add predator to trigger panic
   - Verify scatter behavior
   - Observe reformation after predator leaves
