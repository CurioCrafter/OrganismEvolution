# PHASE 7 - Agent 3: Aquatic-to-Land Evolution Pipeline

## Overview

This document describes the gradual aquatic-to-land evolution system implemented for the OrganismEvolution project. The system enables creatures to progressively transition from fully aquatic forms to land-dwelling forms through meaningful intermediate stages, without instant type flips.

## State Transition Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        AMPHIBIOUS STAGE TRANSITIONS                         │
└─────────────────────────────────────────────────────────────────────────────┘

    ┌──────────────┐       Shore Exposure        ┌──────────────────┐
    │    FULLY     │ ───────────────────────────>│   TRANSITIONING  │
    │   AQUATIC    │       + Lung 0.2+           │                  │
    │              │       + Limb 0.15+          │  Can survive     │
    │ 100% water   │                             │  briefly on land │
    │ Dies on land │<─────────────────────────── │  with penalties  │
    └──────────────┘   Deep water regression     └──────────────────┘
          │                                               │
          │                                               │ Land Exposure
          │                                               │ + Lung 0.5+
          │                                               │ + Limb 0.4+
          │                                               ▼
          │                                      ┌──────────────────┐
          │                                      │    AMPHIBIOUS    │
          │                                      │                  │
          │                                      │  Thrives in both │
          │                                      │  environments    │
          │                                      │  Minimal penalty │
          │                                      └──────────────────┘
          │                                               │
          │                                               │ Extended Land
          │                                               │ + Lung 0.8+
          │                                               │ + Limb 0.7+
          │                                               ▼
          │                                      ┌──────────────────┐
          │                                      │   LAND_ADAPTED   │
          │                                      │                  │
          │                                      │  Primarily land  │
          │                                      │  Can enter water │
          │                                      │  briefly         │
          └──────────────────────────────────────└──────────────────┘
                        (No direct regression)
```

## Trait Variables and Thresholds

### Per-Creature Transition State (`AmphibiousTransitionState`)

| Variable | Type | Range | Description |
|----------|------|-------|-------------|
| `currentStage` | AmphibiousStage | enum | Current evolution stage |
| `transitionProgress` | float | 0.0-1.0 | Progress within current stage |
| `lungCapacity` | float | 0.0-1.0 | 0 = gills only, 1 = full lungs |
| `limbStrength` | float | 0.0-1.0 | 0 = fins only, 1 = strong legs |
| `skinMoisture` | float | 0.0-1.0 | 1 = needs water, 0 = dry-tolerant |
| `aquaticAffinity` | float | 0.0-1.0 | 1 = fully aquatic, 0 = terrestrial |
| `timeNearShore` | float | seconds | Accumulated shore exposure |
| `timeOnLand` | float | seconds | Accumulated land exposure |
| `environmentalStress` | float | 0.0-1.0 | Current stress level |

### Stage Transition Thresholds (`AmphibiousThresholds`)

| Threshold | Default Value | Description |
|-----------|---------------|-------------|
| `lungCapacityToTransition` | 0.2 | Min lungs to start transitioning |
| `limbStrengthToTransition` | 0.15 | Min limbs to start transitioning |
| `lungCapacityToAmphibious` | 0.5 | Min lungs for full amphibious |
| `limbStrengthToAmphibious` | 0.4 | Min limbs for full amphibious |
| `lungCapacityToLand` | 0.8 | Min lungs for land adaptation |
| `limbStrengthToLand` | 0.7 | Min limbs for land adaptation |
| `skinMoistureMinLand` | 0.1 | Max dryness tolerance for land |
| `aquaticAffinityMaxLand` | 0.3 | Max water dependency for land |

### Environmental Exposure Thresholds

| Threshold | Default | Description |
|-----------|---------|-------------|
| `shoreExposureToTransition` | 300s (5 min) | Shore time to start transitioning |
| `landExposureToAmphibious` | 600s (10 min) | Land time for amphibious stage |
| `landExposureToLandAdapted` | 1800s (30 min) | Land time for full land adaptation |
| `maxLandTimeAquatic` | 10s | Aquatic creature dies on land after |
| `maxWaterTimeLand` | 60s | Land creature drowns after |

## Formulas

### Trait Development Rates (per second)

```cpp
// Shore zone (optimal for transition)
lungCapacity += 0.001 * deltaTime;
limbStrength += 0.0015 * deltaTime;
skinMoisture -= 0.001 * deltaTime;
aquaticAffinity -= 0.001 * deltaTime;

// Land zone (for non-fully-aquatic)
lungCapacity += 0.002 * deltaTime;  // 2x rate on land
limbStrength += 0.003 * deltaTime;  // 2x rate on land
skinMoisture -= 0.002 * deltaTime;
aquaticAffinity -= 0.002 * deltaTime;

// Deep water (regression)
limbStrength -= 0.00075 * deltaTime;  // 0.5x decay
skinMoisture += 0.002 * deltaTime;
aquaticAffinity += 0.001 * deltaTime;
```

### Environmental Penalties

#### Fully Aquatic on Land
```cpp
float survivalRatio = timeOnLand / maxLandTimeAquatic;  // 10 seconds
energyDrain = 5.0 * deltaTime * survivalRatio;
speedPenalty = 0.8;  // 80% speed reduction
if (survivalRatio > 0.5) {
    healthDamage = 10.0 * deltaTime * (survivalRatio - 0.5) * 2.0;
}
```

#### Transitioning on Land
```cpp
float maxTime = maxLandTimeAquatic * 3.0;  // 30 seconds
float survivalRatio = timeOnLand / maxTime;
energyDrain = 2.0 * deltaTime * survivalRatio;
speedPenalty = 0.5;  // 50% speed reduction
```

#### Land-Adapted Underwater
```cpp
float survivalRatio = timeSubmerged / maxWaterTimeLand;  // 60 seconds
energyDrain = 2.0 * deltaTime * survivalRatio;
speedPenalty = 0.4;  // 40% speed reduction
if (survivalRatio > 0.5) {
    healthDamage = 8.0 * deltaTime * (survivalRatio - 0.5) * 2.0;  // Drowning
}
```

### Locomotion Blend Factor

```cpp
float calculateLocomotionBlend(AmphibiousStage stage, float progress) {
    switch (stage) {
        case FULLY_AQUATIC:  return 0.0;
        case TRANSITIONING:  return progress * 0.4;        // 0-40%
        case AMPHIBIOUS:     return 0.4 + progress * 0.3;  // 40-70%
        case LAND_ADAPTED:   return 0.7 + progress * 0.3;  // 70-100%
    }
}
// Result: 0.0 = pure swim, 1.0 = pure walk
```

### Speed Multipliers by Zone

| Stage | Deep Water | Shallow | Shore | Land |
|-------|------------|---------|-------|------|
| Fully Aquatic | 1.0 | 1.0 | 0.7 | 0.2 |
| Transitioning | 0.9 | 1.0 | 0.8 | 0.5 |
| Amphibious | 0.8 | 0.9 | 1.0 | 0.9 |
| Land Adapted | 0.6 | 0.7 | 0.8 | 1.0 |

## Files Modified

| File | Changes |
|------|---------|
| `src/physics/Metamorphosis.h` | Added `AmphibiousStage` enum, `AmphibiousThresholds`, `AmphibiousTransitionState`, `AmphibiousUpdateResult`, `AmphibiousTransitionController` class |
| `src/physics/Metamorphosis.cpp` | Implemented `AmphibiousTransitionController` methods |
| `src/entities/SwimBehavior.h` | Added `AmphibiousLocomotionConfig`, `AmphibiousVelocityResult`, `AmphibiousLocomotion` class |
| `src/entities/SwimBehavior.cpp` | Implemented amphibious locomotion blending |
| `src/animation/Animation.h` | Added `initializeAmphibious()`, `setAmphibiousBlend()`, `m_amphibiousBlend` |
| `src/core/CreatureManager.h` | Added amphibious stats, transition controller management |
| `src/core/CreatureManager.cpp` | Implemented `updateAmphibiousTransitions()`, environment zone detection |

## Integration Notes for Agent 10 (UI)

### Available Statistics (`PopulationStats`)

```cpp
// Amphibious stage counts
stats.byAmphibiousStage[0]  // FULLY_AQUATIC count
stats.byAmphibiousStage[1]  // TRANSITIONING count
stats.byAmphibiousStage[2]  // AMPHIBIOUS count
stats.byAmphibiousStage[3]  // LAND_ADAPTED count

stats.totalTransitions       // Total stage changes this session
stats.transitionsThisFrame   // Transitions this frame
stats.avgTransitionProgress  // Average progress of transitioning creatures
```

### Per-Creature UI Data

```cpp
// Get transition controller for selected creature
const AmphibiousTransitionController* ctrl = creatureManager.getTransitionController(handle);
if (ctrl) {
    const auto& state = ctrl->getState();

    // Display these in UI panel:
    // - Current stage: getAmphibiousStageName(state.currentStage)
    // - Progress bar: state.transitionProgress (0-1)
    // - Lung icon fill: state.lungCapacity (0-1)
    // - Limb icon fill: state.limbStrength (0-1)
    // - Moisture icon: state.skinMoisture (0-1)
    // - Stress meter: state.environmentalStress (0-1)
}
```

### Suggested UI Elements

1. **Evolution Panel** (for selected creature):
   - Stage indicator with icon (fish -> fish-frog -> frog -> frog-land)
   - Progress bar showing distance to next stage
   - Trait bars: Lungs, Limbs, Moisture Tolerance
   - Environmental stress warning indicator

2. **Population Overview**:
   - Pie chart or bar graph of creatures by amphibious stage
   - "Transitions Today" counter
   - List of recent stage change events

3. **Debug Overlay** (when enabled):
   - Show environment zone under cursor (DEEP/SHALLOW/SHORE/LAND)
   - Highlight creatures by stage with different colors
   - Show trait development rates in real-time

## Testing Procedure

### Manual Test: Shore Transition

1. Spawn aquatic creature near shore:
   ```cpp
   // In game code or console
   CreatureHandle handle = creatureManager.spawn(CreatureType::AQUATIC, shorePosition);
   creatureManager.setAmphibiousDebugLogging(true);
   ```

2. Wait ~5 minutes of simulation time near shore

3. Verify console output shows:
   ```
   [AMPHIBIOUS] Stage change: Fully Aquatic -> Transitioning (Progress: 0.0)
   ```

4. Continue waiting on land (~10 more minutes)

5. Verify:
   ```
   [AMPHIBIOUS] Stage change: Transitioning -> Amphibious (Progress: 0.0)
   ```

### Forced Test

```cpp
// Force immediate transition for testing
CreatureHandle handle = /* get creature handle */;
creatureManager.forceTransitionStage(handle, AmphibiousStage::AMPHIBIOUS);
```

## Handoff Note for Agent 5 (Genome)

The current implementation uses per-creature state (`AmphibiousTransitionState`) that evolves during a creature's lifetime. For **inheritable evolution**, the following changes would be needed in `src/entities/Genome.h`:

1. Add base trait genes:
   ```cpp
   float baseLungCapacity = 0.0f;    // Starting lung development
   float baseLimbStrength = 0.0f;    // Starting limb development
   float amphibiousPotential = 0.0f; // How easily creature can transition (0-1)
   float landAdaptability = 0.0f;    // Genetic predisposition to land life
   ```

2. These would be inherited and mutated, giving offspring a head start on transition

3. The `AmphibiousTransitionController::initialize()` would read from genome:
   ```cpp
   void initialize(const Genome& genome) {
       m_state.lungCapacity = genome.baseLungCapacity;
       m_state.limbStrength = genome.baseLimbStrength;
       // Set starting stage based on inherited traits
   }
   ```

**DO NOT EDIT**: `src/entities/Genome.*` - please coordinate with Agent 5 for these changes.
