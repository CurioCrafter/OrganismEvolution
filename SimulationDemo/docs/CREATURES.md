# SimulationDemo Creatures

Complete documentation of creature types, genetics, behavior systems, and reproduction.

## Creature Types

### Trophic Level Classification

```
Level 4 (Tertiary Consumers)
├── APEX_PREDATOR    - Hunts all herbivores and small predators
└── SCAVENGER        - Feeds on carrion

Level 3 (Secondary Consumers)
├── SMALL_PREDATOR   - Hunts small herbivores (frugivores)
└── OMNIVORE         - Eats plants and small creatures

Level 2 (Primary Consumers - Herbivores)
├── GRAZER           - Eats grass only
├── BROWSER          - Eats tree leaves and bushes
└── FRUGIVORE        - Eats fruits and berries

Level 1 (Producers)
└── Plants           - Not mobile creatures, food source

Special Types:
├── PARASITE         - Attaches to hosts, drains energy
└── CLEANER          - Symbiont that removes parasites
```

### Creature Type Properties

| Type | Diet | Social | Combat | Special |
|------|------|--------|--------|---------|
| GRAZER | Grass only | Herd animal | Flee-focused | High efficiency |
| BROWSER | Leaves, bushes | Semi-social | Moderate flee | Can climb |
| FRUGIVORE | Fruits, berries | Solitary | Fast flee | Color vision |
| SMALL_PREDATOR | Small prey | Pack hunter | Attack 10 dmg | Stalking |
| OMNIVORE | Plants + small prey | Flexible | Moderate | Adaptable |
| APEX_PREDATOR | All prey | Pack hunter | Attack 20 dmg | Territorial |
| SCAVENGER | Carrion | Loose groups | Low combat | Smell-focused |

## Genome System

### Trait Categories

#### Physical Traits
| Trait | Range | Description |
|-------|-------|-------------|
| `size` | 0.5 - 2.0 | Body scale multiplier |
| `speed` | 5.0 - 20.0 | Maximum movement speed |
| `efficiency` | 0.5 - 1.5 | Energy usage efficiency |
| `color` | RGB (0-1) | Body coloration |

#### Sensory Traits
| Trait | Range | Description |
|-------|-------|-------------|
| `visionRange` | 10 - 50 | Maximum sight distance |
| `visionFOV` | 1.0 - 6.0 rad | Field of view angle |
| `visionAcuity` | 0.0 - 1.0 | Detail perception |
| `colorPerception` | 0.0 - 1.0 | Color differentiation |
| `hearingRange` | 10 - 100 | Sound detection distance |
| `smellRange` | 10 - 150 | Scent detection distance |
| `camouflage` | 0.0 - 1.0 | Visibility reduction |

#### Neural Weights
| Component | Count | Purpose |
|-----------|-------|---------|
| Input weights | 4 | Sensory input processing |
| Hidden weights | 16 | Internal processing |
| Output weights | 4 | Movement decision |
| **Total** | **24** | Basic neural network |

### Diploid Genome (Advanced)

The advanced genetics system implements Mendelian inheritance:

```cpp
struct DiploidGenome {
    Chromosome maternal;    // From mother
    Chromosome paternal;    // From father

    // Trait expression uses dominance coefficients
    float expressedTrait =
        maternal.allele.effect * maternal.allele.dominance +
        paternal.allele.effect * paternal.allele.dominance;
};
```

#### Inheritance Patterns
- **Complete Dominance**: One allele fully masks the other
- **Codominance**: Both alleles expressed equally
- **Incomplete Dominance**: Blended expression

## Sensory System

### Perception Modalities

#### Vision
```cpp
struct VisionParams {
    float fov = 2.0f;           // Field of view (radians)
    float range = 30.0f;        // Maximum distance
    float acuity = 0.5f;        // Detail level (0-1)
    float colorPerception = 0.5f; // Color sensitivity
    float motionDetection = 0.7f; // Movement sensitivity
};
```

#### Hearing
```cpp
struct HearingParams {
    float range = 50.0f;        // Detection distance
    float directionalAccuracy = 0.6f;
    bool echolocation = false;  // Active sonar capability
};
```

#### Smell
```cpp
struct SmellParams {
    float range = 75.0f;        // Scent detection range
    float sensitivity = 0.5f;   // Concentration threshold
    bool pheromoneReception = true;
};
```

#### Touch
```cpp
struct TouchParams {
    float range = 2.0f;         // Contact distance
    float vibrationSensitivity = 0.5f;
};
```

### Pheromone Types
| Type | Purpose | Decay Rate |
|------|---------|------------|
| FOOD_TRAIL | Mark food locations | Medium |
| ALARM | Warn of danger | Fast |
| MATING | Attract mates | Slow |
| TERRITORY | Mark boundaries | Very slow |
| AGGREGATION | Group formation | Medium |

### Memory System
```cpp
struct SpatialMemory {
    vector<MemoryLocation> foodSources;
    vector<MemoryLocation> dangers;
    vector<MemoryLocation> shelters;
    vector<MemoryLocation> territory;

    float decayRate = 0.01f;  // Memory fades over time
};
```

## Behavior Systems

### Steering Behaviors

#### Basic Behaviors
| Behavior | Description | Weight |
|----------|-------------|--------|
| `seek` | Move toward target | 1.0 |
| `flee` | Move away from threat | 1.5 |
| `arrive` | Slow approach to target | 1.0 |
| `wander` | Random exploration | 0.5 |
| `pursuit` | Intercept moving target | 1.2 |
| `evasion` | Avoid predicted intercept | 1.5 |

#### Flocking Behaviors
| Behavior | Description | Weight |
|----------|-------------|--------|
| `separation` | Avoid crowding neighbors | 1.5 |
| `alignment` | Match neighbor heading | 1.0 |
| `cohesion` | Move toward group center | 1.0 |

#### Environment Behaviors
| Behavior | Description |
|----------|-------------|
| `boundaryAvoidance` | Stay within world bounds |
| `obstacleAvoidance` | Navigate around terrain |
| `terrainFollowing` | Maintain height above ground |

### Creature-Specific AI

#### Herbivore Behavior
```
Priority Stack:
1. FLEE if predator detected (high fear)
2. SEEK food if hungry (energy < 50%)
3. SEEK mate if reproductive (energy > 80%)
4. FLOCK with herd members
5. WANDER if no other goals
```

#### Carnivore Behavior
```
Priority Stack:
1. HUNT if prey detected and hungry
2. PACK_COORDINATE if hunting with others
3. SEEK mate if reproductive
4. PATROL territory
5. REST if energy high
```

### Neural Network Control

```
Inputs (4):
├── [0] Nearest food distance (normalized)
├── [1] Nearest threat distance (normalized)
├── [2] Current energy level (0-1)
└── [3] Nearest mate distance (normalized)

Hidden Layer (4 neurons):
├── tanh activation
└── Fully connected

Outputs (2):
├── [0] Turn angle (-π to π)
└── [1] Speed multiplier (0-1)
```

### NEAT Evolution (Advanced)

The NeuroEvolution of Augmenting Topologies system allows:
- Dynamic node addition
- Dynamic connection addition
- Recurrent connections
- Topology crossover via innovation numbers

## Reproduction

### Requirements

#### Herbivore Reproduction
| Requirement | Value |
|-------------|-------|
| Energy threshold | 180 / 200 |
| Energy cost | 80 |
| Cooldown | 30 seconds |
| Minimum age | 60 seconds |

#### Carnivore Reproduction
| Requirement | Value |
|-------------|-------|
| Energy threshold | 170 / 200 |
| Energy cost | 100 |
| Minimum kills | 2 |
| Cooldown | 45 seconds |

### Genetic Crossover

```cpp
Genome crossover(Genome& parent1, Genome& parent2) {
    Genome child;

    for (int i = 0; i < TRAIT_COUNT; i++) {
        // Random selection from either parent
        if (random() < 0.5f) {
            child.traits[i] = parent1.traits[i];
        } else {
            child.traits[i] = parent2.traits[i];
        }
    }

    // Apply mutations
    child.mutate(mutationRate, mutationStrength);

    return child;
}
```

### Mutation

```cpp
void Genome::mutate(float rate, float strength) {
    for (int i = 0; i < TRAIT_COUNT; i++) {
        if (random() < rate) {
            // Gaussian mutation
            float delta = gaussianRandom() * strength;
            traits[i] += delta;
            traits[i] = clamp(traits[i], traitMin[i], traitMax[i]);
        }
    }
}
```

**Default Parameters:**
- Mutation Rate: 10% per trait
- Mutation Strength: 0.15 (15% of trait range)

### Sexual Selection

```cpp
struct MateSelector {
    float choosiness = 0.5f;      // How selective
    float preferredSize = 1.0f;   // Size preference
    float preferredColor[3];      // Color preference

    float evaluateMate(Creature& candidate) {
        float score = 0.0f;
        score += fitnessWeight * candidate.fitness;
        score += sizeWeight * sizeSimilarity(candidate);
        score += colorWeight * colorSimilarity(candidate);
        return score;
    }
};
```

### Species & Speciation

Creatures can diverge into separate species based on:
- Genetic distance threshold
- Reproductive isolation
- Hybrid sterility penalties

```cpp
bool canInterbreed(Creature& a, Creature& b) {
    float geneticDistance = a.genome.distanceTo(b.genome);
    return geneticDistance < SPECIATION_THRESHOLD;
}
```

## Energy System

### Energy Sources
| Source | Energy Gain | Creature Type |
|--------|-------------|---------------|
| Grass | 20 | Grazer |
| Bush berry | 30 | Browser, Frugivore |
| Tree fruit | 40 | Browser, Frugivore |
| Small prey kill | 80 | Small Predator |
| Large prey kill | 120 | Apex Predator |
| Carrion | 60 | Scavenger |

### Energy Costs
| Activity | Cost/Second |
|----------|-------------|
| Base metabolism | 0.5 |
| Movement | speed * 0.1 |
| Sprinting | speed * 0.3 |
| Attack | 5.0 per attack |
| Reproduction | 80-100 (one-time) |

### Death Conditions
- Energy reaches 0
- Age exceeds maximum lifespan
- Killed by predator (energy transferred)

## Fitness Calculation

```cpp
float Creature::calculateFitness() {
    float fitness = 0.0f;

    // Survival time
    fitness += age * 0.1f;

    // Offspring produced
    fitness += offspringCount * 10.0f;

    // Energy efficiency
    fitness += totalEnergyGained / totalEnergySpent;

    // For carnivores: hunting success
    if (isCarnivore) {
        fitness += killCount * 5.0f;
    }

    return fitness;
}
```

## Population Dynamics

### Lotka-Volterra Balance

The simulation maintains predator-prey balance:

```cpp
void balancePopulations() {
    int herbivores = countType(HERBIVORE);
    int carnivores = countType(CARNIVORE);

    // Target ratio: 4:1 herbivore:carnivore
    float ratio = (float)herbivores / max(1, carnivores);

    if (ratio < 3.0f) {
        // Too few herbivores, spawn more
        spawnHerbivores(5);
    }
    if (ratio > 6.0f && carnivores < MIN_CARNIVORES) {
        // Too few carnivores, spawn more
        spawnCarnivores(2);
    }
}
```

### Population Limits
| Type | Minimum | Maximum |
|------|---------|---------|
| Herbivores | 20 | 200 |
| Carnivores | 5 | 50 |
| Total | 30 | 250 |
