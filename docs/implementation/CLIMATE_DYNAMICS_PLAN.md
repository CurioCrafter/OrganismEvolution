# Climate Dynamics Implementation Plan

## Overview

This document outlines the implementation of a dynamic climate system that evolves over time, affecting biomes and creature behavior. The system addresses issues E-03 (ClimateSystem.update() empty) and E-06 (Biome blend unused).

## Current State Analysis

### Existing Components
- **ClimateSystem**: Has biome classification based on Whittaker diagram (temperature vs precipitation)
- **SeasonManager**: Provides seasonal temperature/growth modifiers, day length calculations
- **ClimateData**: Contains temperature, moisture, elevation, slope, distance to water, latitude
- **BiomeBlend**: Structure exists but transitions not animated over time

### Issues Identified
1. `ClimateSystem::update()` is essentially empty (just a TODO comment)
2. BiomeBlend is calculated but not used for dynamic transitions
3. No long-term climate cycles (ice ages, etc.)
4. No climate events (droughts, volcanic winters)
5. Creatures don't respond to climate stress

## Implementation Design

### Phase 1: Climate Update System

#### Long-term Temperature Cycles (Milankovitch-inspired)
```
Global Temperature = Base + LongCycle + MediumCycle + SeasonalOffset + EventModifier

Where:
- LongCycle: sin(time * 0.0001) * 5.0  // ~1000 game-year period, ±5°C
- MediumCycle: sin(time * 0.001) * 2.0  // ~100 game-year period, ±2°C
- SeasonalOffset: from SeasonManager
- EventModifier: active climate event effects
```

#### New Member Variables for ClimateSystem
```cpp
float m_simulationTime = 0.0f;
float m_globalTemperatureOffset = 0.0f;
ClimateEvent m_activeEvent = ClimateEvent::NONE;
float m_eventDuration = 0.0f;
float m_eventTimeRemaining = 0.0f;

// Climate grid for tracking transitions
std::vector<std::vector<ClimateGridCell>> m_climateGrid;
std::vector<std::vector<float>> m_baseTemperatureGrid;
```

### Phase 2: Moisture Transport

Simulate wind-driven moisture transport with rain shadow effects:
1. Water bodies have moisture = 1.0
2. Prevailing winds carry moisture inland
3. Mountains block moisture (rain shadow)
4. Moisture dissipates with distance from source

### Phase 3: Biome Transitions

When climate changes cause biome reclassification:
1. Track previous biome as secondary
2. Gradually blend from old to new over time
3. Update vegetation accordingly
4. Notify affected creatures

### Phase 4: Climate Events

| Event | Temperature Effect | Duration | Probability |
|-------|-------------------|----------|-------------|
| VOLCANIC_WINTER | -3°C | 50 days | 15% of events |
| SOLAR_MAXIMUM | +2°C | 100 days | 10% of events |
| DROUGHT | -50% moisture | 30 days | 20% of events |
| MONSOON | +100% moisture | 20 days | 20% of events |
| ICE_AGE_START | Gradual cooling | Long-term | Rare (cycle-based) |
| ICE_AGE_END | Gradual warming | Long-term | Rare (cycle-based) |

### Phase 5: Creature Climate Response

Creatures respond to climate based on their genome:
1. **Optimal Temperature**: Derived from genome traits
2. **Temperature Stress**: Energy drain when outside comfort zone
3. **Behavioral Adaptation**: Seek water when hot, huddle when cold
4. **Migration**: Move towards more suitable climate when stressed

## Data Flow

```
SeasonManager.update()
       ↓
ClimateSystem.update()
       ├── updateGlobalTemperature()
       ├── updateMoisturePatterns()
       ├── triggerRandomEvent()
       ├── applyClimateEvent()
       └── updateBiomeTransitions()
              ↓
    ClimateData (per-position)
              ↓
    Creature.updateClimateResponse()
              ├── calculateClimateStress()
              ├── adjustBehavior()
              └── considerMigration()
```

## UI Requirements

### Climate Panel
- Global temperature display
- Current season
- Active climate event (if any)
- Local climate at camera position
- Temperature history graph

## Testing Criteria

1. Temperature cycles visible over extended simulation
2. Climate events trigger and have visible effects
3. Biomes transition smoothly when climate changes
4. Creatures migrate away from hostile climate zones
5. UI displays accurate climate information

## Files Modified

- `src/environment/ClimateSystem.h` - Add new enums, member variables
- `src/environment/ClimateSystem.cpp` - Implement update functions
- `src/entities/Creature.h` - Add climate response methods
- `src/entities/Creature.cpp` - Implement climate behavior
- `src/main.cpp` - Add climate UI panel
