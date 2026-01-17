# Ecosystem Implementation Guide

This document describes the implementation of the multi-trophic ecosystem system for OrganismEvolution.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    EcosystemManager                          │
│  (Central coordinator for all ecosystem subsystems)         │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │
│  │ProducerSystem│  │DecomposerSys │  │SeasonManager │       │
│  │              │  │              │  │              │       │
│  │ - Grass      │  │ - Corpses    │  │ - Spring     │       │
│  │ - Bushes     │  │ - Nutrients  │  │ - Summer     │       │
│  │ - Trees      │  │ - Soil cycle │  │ - Fall       │       │
│  │ - Soil grid  │  │              │  │ - Winter     │       │
│  └──────────────┘  └──────────────┘  └──────────────┘       │
│                                                              │
│  ┌──────────────────────────────────────────────────┐       │
│  │               EcosystemMetrics                    │       │
│  │                                                   │       │
│  │  - Population tracking    - Energy flow          │       │
│  │  - Species diversity      - Trophic balance      │       │
│  │  - Health score           - Warnings             │       │
│  └──────────────────────────────────────────────────┘       │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

## Creature Types

### Trophic Level 2: Primary Consumers (Herbivores)

| Type | Diet | Behavior | Special |
|------|------|----------|---------|
| GRAZER | Grass only | Herding | Efficient grass digestion |
| BROWSER | Leaves, bushes | Small groups | Reaches tree food |
| FRUGIVORE | Fruit, berries | Solitary | Can climb |

### Trophic Level 3: Secondary Consumers

| Type | Diet | Behavior | Special |
|------|------|----------|---------|
| SMALL_PREDATOR | Small herbivores | Solitary, ambush | Territorial |
| OMNIVORE | Plants + small prey | Diet switching | Type III response |
| SCAVENGER | Carrion | Follow predators | Immune to parasites |

### Trophic Level 4: Tertiary Consumers

| Type | Diet | Behavior | Special |
|------|------|----------|---------|
| APEX_PREDATOR | All herbivores + small predators | Pack hunting | Top of food chain |

### Special Types

| Type | Role | Behavior | Special |
|------|------|----------|---------|
| PARASITE | Energy drain | Attach to hosts | Sustained damage |
| CLEANER | Remove parasites | Symbiotic | Mutual benefit |

## File Structure

```
src/
├── entities/
│   ├── CreatureType.h        # Expanded enum + helpers
│   ├── CreatureTraits.cpp    # Factory for type-specific traits
│   ├── EcosystemBehaviors.h  # Ecosystem-aware AI behaviors
│   └── EcosystemBehaviors.cpp
│
└── environment/
    ├── ProducerSystem.h      # Plant growth and soil
    ├── ProducerSystem.cpp
    ├── DecomposerSystem.h    # Corpse decomposition
    ├── DecomposerSystem.cpp
    ├── SeasonManager.h       # Seasonal cycles
    ├── SeasonManager.cpp
    ├── EcosystemMetrics.h    # Health tracking
    ├── EcosystemMetrics.cpp
    ├── EcosystemManager.h    # Central coordinator
    └── EcosystemManager.cpp
```

## Integration Guide

### Step 1: Add EcosystemManager to Simulation

```cpp
// In Simulation.h
#include "../environment/EcosystemManager.h"

class Simulation {
private:
    std::unique_ptr<EcosystemManager> ecosystem;
    // ...
};

// In Simulation.cpp init()
ecosystem = std::make_unique<EcosystemManager>(terrain.get());
ecosystem->init(seed);

// In Simulation.cpp update()
ecosystem->update(deltaTime, creatures);
```

### Step 2: Use EcosystemManager for Food

```cpp
// Replace getFoodPositions() with ecosystem-aware version
std::vector<glm::vec3> Simulation::getFoodPositions() const {
    // Legacy food positions
    auto positions = getLegacyFoodPositions();

    // Add producer system food
    if (ecosystem && ecosystem->getProducers()) {
        auto producerFood = ecosystem->getProducers()->getAllFoodPositions();
        positions.insert(positions.end(), producerFood.begin(), producerFood.end());
    }

    return positions;
}
```

### Step 3: Handle Creature Deaths

```cpp
// When a creature dies
if (!creature->isAlive()) {
    ecosystem->onCreatureDeath(*creature);
}
```

### Step 4: Spawn Diverse Creature Types

```cpp
void Simulation::spawnCreatures(CreatureType type, int count) {
    CreatureTraits traits = CreatureTraits::getTraitsFor(type);

    for (int i = 0; i < count; i++) {
        Genome genome = createGenomeForType(type, traits);
        glm::vec3 pos = findValidSpawnPosition();

        auto creature = std::make_unique<Creature>(pos, genome, type);
        creatures.push_back(std::move(creature));
    }
}

void Simulation::spawnInitialPopulation() {
    // Follow trophic pyramid ratios
    spawnCreatures(CreatureType::GRAZER, 30);
    spawnCreatures(CreatureType::BROWSER, 20);
    spawnCreatures(CreatureType::FRUGIVORE, 15);
    spawnCreatures(CreatureType::SMALL_PREDATOR, 8);
    spawnCreatures(CreatureType::OMNIVORE, 6);
    spawnCreatures(CreatureType::APEX_PREDATOR, 4);
    spawnCreatures(CreatureType::SCAVENGER, 5);
}
```

### Step 5: Use Spawn Recommendations

```cpp
void Simulation::balancePopulation() {
    auto recommendations = ecosystem->getSpawnRecommendations();

    for (const auto& rec : recommendations) {
        spawnCreatures(rec.type, rec.count);
        std::cout << "Spawning " << rec.count << " "
                  << getCreatureTypeName(rec.type)
                  << ": " << rec.reason << std::endl;
    }
}
```

## Energy Flow

Energy flows through the system following the 10% rule:

```
Sunlight (100%)
    ↓
Producers (10% captured as biomass)
    ↓
Primary Consumers / Herbivores (10-15% of plant energy)
    ↓
Secondary Consumers (8-12% of herbivore energy)
    ↓
Tertiary Consumers (5-10% of secondary consumer energy)
    ↓
Decomposers → Return nutrients to soil → Producers
```

Implementation in `EcosystemBehaviors::consumeProducerFood()`:
```cpp
// Apply trophic efficiency
energyGained *= 0.12f;  // 12% efficiency
```

## Seasonal Effects

| Season | Growth | Berries | Fruit | Leaves | Reproduction |
|--------|--------|---------|-------|--------|--------------|
| Spring | 1.5x | 0.2x | 0.1x | 0.8x | 1.5x |
| Summer | 1.0x | 1.0x | 1.2x | 1.0x | 1.0x |
| Fall | 0.5x | 1.5x | 1.0x→0.2x | 1.0x→0.3x | 0.5x |
| Winter | 0.1x | 0.0x | 0.0x | 0.1x | 0.2x |

## Population Balance

The system maintains population balance through:

1. **Carrying Capacity**: Each species has a seasonal carrying capacity
2. **Trophic Ratios**: ~10:1 ratio between adjacent trophic levels
3. **Spawn Recommendations**: When populations fall below critical thresholds
4. **Health Monitoring**: EcosystemMetrics tracks and warns about imbalances

## Nutrient Cycling

```
Dead Creature
    ↓
Corpse (50% of energy as biomass)
    ↓
    ├── Scavengers consume (high energy return)
    │
    └── Decomposition
            ↓
        Nutrients released to soil
            ↓
        ├── Nitrogen (30% of decomposed)
        ├── Phosphorus (10% of decomposed)
        └── Organic Matter (50% of decomposed)
            ↓
        Enhanced plant growth
```

## Testing the System

1. **Build the project**: `cmake --build build`
2. **Run simulation**: `./build/OrganismEvolution`
3. **Monitor health**: Check `ecosystem->getEcosystemStatus()` in console
4. **Verify balance**: Population counts should follow 10:1 trophic ratios
5. **Test seasons**: Observe plant growth changes every ~90 game days

## Future Enhancements

1. **Pack/Herd Behavior**: Implement group coordination
2. **Territory Defense**: Add territorial conflicts
3. **Migration**: Seasonal movement patterns
4. **Disease Spread**: Epidemic simulation
5. **Climate Events**: Droughts, floods affecting ecosystem
