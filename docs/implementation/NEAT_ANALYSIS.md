# NEAT Topology Evolution - Implementation Analysis

## Overview

This document provides a comprehensive analysis of the NEAT (NeuroEvolution of Augmenting Topologies) implementation in the OrganismEvolution simulator. NEAT allows neural network structure to evolve alongside weights, enabling creatures to develop increasingly complex brains over generations.

## Architecture Summary

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         NEAT ARCHITECTURE                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────────────┐  │
│  │ InnovationTracker │◄──│   NEATGenome    │◄──│    NEATPopulation       │  │
│  │   (Singleton)    │   │  (Gene Storage) │   │  (Evolution Manager)    │  │
│  └─────────────────┘    └─────────────────┘    └─────────────────────────┘  │
│           ▲                      ▲                         ▲                │
│           │                      │                         │                │
│           │              ┌───────┴───────┐                 │                │
│           │              │               │                 │                │
│  ┌────────┴────────┐  ┌──┴───┐       ┌───┴──┐   ┌─────────┴──────────────┐ │
│  │ Connection Gene │  │ Node │       │Species│   │   BrainEvolution       │ │
│  │  (innovation,   │  │ Gene │       │Manager│   │      Manager           │ │
│  │   weight, etc)  │  │      │       │       │   └──────────────────────┘  │
│  └─────────────────┘  └──────┘       └───────┘              ▲               │
│                                                              │               │
│  ┌─────────────────────────────────────────────────────────┐ │               │
│  │                ai::NeuralNetwork                         │ │               │
│  │           (Runtime Execution Engine)                     │ │               │
│  │  - buildFromGenome() converts genome to executable net   │ │               │
│  │  - forward() runs network with topological sorting       │ │               │
│  │  - Supports recurrent connections                        │ │               │
│  │  - Hebbian plasticity for online learning                │ │               │
│  └─────────────────────────────────────────────────────────┘ │               │
│                              ▲                                │               │
│                              │                                │               │
│  ┌───────────────────────────┴────────────────────────────────┴──────────┐   │
│  │                   CreatureBrainInterface                               │   │
│  │              (Bridge between NEAT and Creature)                        │   │
│  │  - Processes sensory input (15 channels)                               │   │
│  │  - Outputs motor commands (turn, speed, attack, flee)                  │   │
│  │  - Event callbacks (onFoodEaten, onDamageTaken, etc.)                  │   │
│  │  - Learning signal integration                                         │   │
│  └────────────────────────────────────────────────────────────────────────┘   │
│                              ▲                                                 │
│                              │                                                 │
│  ┌───────────────────────────┴────────────────────────────────────────────┐   │
│  │                         Creature                                        │   │
│  │  - m_neatBrain: optional NEAT-evolved brain                            │   │
│  │  - enableNEATBrain(): switch from simple NN to NEAT                    │   │
│  │  - updateNeuralBehavior(): uses NEAT for decision making               │   │
│  │  - Fitness propagates to genome for selection                          │   │
│  └────────────────────────────────────────────────────────────────────────┘   │
│                                                                                │
└────────────────────────────────────────────────────────────────────────────────┘
```

## Core Components

### 1. InnovationTracker (Singleton)

**Location:** `src/ai/NEATGenome.h:57-113`

The InnovationTracker ensures that the same structural innovation (adding a connection or node) receives the same innovation number across the population. This is critical for proper crossover alignment.

```cpp
class InnovationTracker {
public:
    static InnovationTracker& instance();

    // Get innovation number for a connection (creates new if first occurrence)
    int getConnectionInnovation(int from, int to);

    // Get node ID when splitting a connection (consistent across population)
    int getNodeInnovation(int splitConnectionId);

    int getNextNodeId();
    void reset();  // Call at start of new population
};
```

**Key Features:**
- Global singleton pattern ensures consistency
- Connection innovations tracked by (from, to) pairs
- Node innovations tracked by split connection ID
- Prevents duplicate innovation numbers in same generation

### 2. Gene Structures

**NodeGene** (`src/ai/NEATGenome.h:17-32`):
```cpp
struct NodeGene {
    int id;                     // Unique identifier
    NodeType type;              // INPUT, HIDDEN, OUTPUT, BIAS
    ActivationType activation;  // TANH, RELU, SIGMOID, etc. (9 types)
    float bias;                 // Node bias
    int layer;                  // Topological layer for ordering
    float plasticityCoef;       // Evolved plasticity parameter
    bool canModulate;           // Neuromodulator capability
};
```

**ConnectionGene** (`src/ai/NEATGenome.h:34-51`):
```cpp
struct ConnectionGene {
    int innovation;      // Historical marker for crossover
    int fromNode;        // Source node ID
    int toNode;          // Target node ID
    float weight;        // Connection weight [-5, 5]
    bool enabled;        // Can be disabled without deletion
    bool recurrent;      // Recurrent connections support temporal memory
    bool plastic;        // Can this connection learn during lifetime?
    float plasticityRate; // Per-connection learning rate
};
```

### 3. NEATGenome

**Location:** `src/ai/NEATGenome.h:119-217`, `src/ai/NEATGenome.cpp`

The genome stores the complete neural network topology and provides mutation/crossover operations.

**Mutation Operations:**

| Method | Description | Default Rate |
|--------|-------------|--------------|
| `mutateAddConnection()` | Add new connection between valid nodes | 5% |
| `mutateAddNode()` | Split existing connection with new node | 3% |
| `mutateWeights()` | Perturb or replace connection weights | 80% |
| `mutateBias()` | Perturb node biases | 30% |
| `mutateActivation()` | Change hidden node activation function | 5% |
| `mutateToggleEnable()` | Enable/disable connections | 1% |
| `mutatePlasticity()` | Evolve per-connection learning rates | 10% |

**Crossover (`NEATGenome::crossover()`):**
1. Align genes by innovation number
2. Matching genes: randomly choose from either parent
3. Disjoint/excess genes: inherit from fitter parent only
4. 75% chance to re-enable disabled genes
5. Nodes copied from fitter parent + unique hidden nodes from other

**Compatibility Distance:**
```cpp
float distance = (c1 * excess / N) + (c2 * disjoint / N) + (c3 * avgWeightDiff);
// Default coefficients: c1=1.0, c2=1.0, c3=0.4
// N = max(genome1.size, genome2.size), min 20 for normalization
```

### 4. Speciation System

**Location:** `src/ai/NEATGenome.h:253-301`, `src/ai/NEATGenome.cpp:536-577`

Species protect innovation by grouping similar topologies.

```cpp
class Species {
    int id;
    std::vector<NEATGenome*> members;
    NEATGenome representative;
    float totalAdjustedFitness;
    int stagnantGenerations;
    float bestFitness;

    void calculateAdjustedFitness();  // fitness / species_size
    void updateStagnation();          // Track generations without improvement
    void updateRepresentative();      // Random member becomes new rep
};
```

**Speciation Algorithm:**
1. Clear species memberships
2. For each genome:
   - Calculate compatibility distance to each species representative
   - Add to first species within threshold (default: 3.0)
   - Create new species if no match
3. Remove empty species
4. Update representatives (random member)

**Stagnation Handling:**
- Track generations without fitness improvement
- Penalize species after limit (default: 15 generations)
- Never remove best-performing species

### 5. NEATPopulation

**Location:** `src/ai/NEATGenome.h:307-350`, `src/ai/NEATGenome.cpp:461-683`

Manages population-level evolution with fitness-proportionate selection.

**Evolution Cycle (`evolve()`):**
1. **Speciate:** Assign genomes to species
2. **Adjusted Fitness:** Divide by species size (fitness sharing)
3. **Stagnation Check:** Penalize non-improving species
4. **Reproduction:**
   - Calculate offspring per species (proportional to adjusted fitness)
   - Champion preservation (best genome copied unchanged)
   - 75% crossover + mutation, 25% asexual mutation
5. **Selection:** Top 20% of species can reproduce

### 6. Neural Network Runtime

**Location:** `src/ai/NeuralNetwork.h`, `src/ai/NeuralNetwork.cpp`

The `ai::NeuralNetwork` class executes networks built from genomes.

**Key Methods:**
```cpp
void buildFromGenome(const NEATGenome& genome);  // Convert genome to network
std::vector<float> forward(const std::vector<float>& inputs);  // Evaluate
void updatePlasticity(float reward, float learningRate);  // Hebbian learning
```

**Execution Order:**
1. Compute topological layers from connections
2. Sort nodes by layer
3. Process in order (inputs → hidden → outputs)
4. Recurrent connections use previous timestep values

**Plasticity (Three-Factor Learning):**
```cpp
// deltaW = learningRate * plasticityRate * eligibility * reward
conn.weight += learningRate * conn.plasticityRate * conn.eligibility * reward;
conn.weight = clamp(conn.weight, -5.0f, 5.0f);
```

### 7. Creature Integration

**Location:** `src/entities/Creature.h`, `src/entities/Creature.cpp`

Creatures can use either:
- Simple fixed-topology NN (default)
- NEAT-evolved brain (when `m_useNEATBrain = true`)

**Enabling NEAT Brain:**
```cpp
creature->initializeNEATBrain();  // Create minimal NEAT brain
// OR
creature->initializeNEATBrain(genome);  // From specific genome
creature->enableNEATBrain(true);  // Activate NEAT processing
```

**Neural Processing Flow:**
```
Creature::update()
    → updateNeuralBehavior()
        → if (m_useNEATBrain && m_neatBrain)
            → m_neatBrain->process(sensory inputs...)
            → Convert MotorOutput to NeuralOutputs
        → else
            → brain->forward(simple inputs)
    → updateBehaviorHerbivore/Carnivore/Aquatic/Flying()
        → Uses m_neuralOutputs to modulate behavior
```

**Fitness Propagation:**
```cpp
void Creature::calculateFitness() {
    fitness = age * 0.5f + foodEaten * 10.0f + distanceTraveled * 0.01f;
    // ... type-specific bonuses

    if (m_useNEATBrain && m_neatBrain) {
        m_neatBrain->getGenome().setFitness(fitness);  // For evolution
        m_neatBrain->learn(fitness * 0.001f);  // Online learning
    }
}
```

## Activation Functions

The system supports 9 activation functions:

| Type | Formula | Use Case |
|------|---------|----------|
| LINEAR | f(x) = x | Direct passthrough |
| SIGMOID | f(x) = 1/(1+e^-x) | Probability outputs |
| TANH | f(x) = tanh(x) | Symmetric activation (default) |
| RELU | f(x) = max(0, x) | Sparse activation |
| LEAKY_RELU | f(x) = x if x>0 else 0.01x | Avoid dead neurons |
| ELU | f(x) = x if x>0 else e^x-1 | Smooth negative region |
| GAUSSIAN | f(x) = e^(-x²) | Radial basis |
| SINE | f(x) = sin(x) | Periodic patterns |
| STEP | f(x) = x>0 ? 1 : 0 | Binary decision |

## Usage Examples

### Creating a NEAT Population
```cpp
// Create population of 100 creatures with 8 inputs, 4 outputs
ai::NEATPopulation population(100, 8, 4);

// Configure mutation rates
population.mutationParams.addConnectionProb = 0.1f;
population.mutationParams.addNodeProb = 0.05f;

// Run evolution
for (int gen = 0; gen < 1000; gen++) {
    population.evaluateFitness([](const ai::NEATGenome& genome) {
        auto network = genome.buildNetwork();
        // Evaluate creature performance...
        return fitness;
    });
    population.evolve();
}
```

### Enabling NEAT on Creatures
```cpp
// Create creature with NEAT brain
Creature* creature = new Creature(position, genome, CreatureType::HERBIVORE);
creature->initializeNEATBrain();
creature->enableNEATBrain(true);

// Or create from existing NEAT genome
ai::NEATGenome neatGenome;
neatGenome.createMinimal(8, 4, rng);
creature->initializeNEATBrain(neatGenome);
```

### Reproducing NEAT Creatures
```cpp
// Get parent genomes
const ai::NEATGenome& parent1 = creature1->getNEATGenome();
const ai::NEATGenome& parent2 = creature2->getNEATGenome();

// Crossover (fitter parent first)
ai::NEATGenome childGenome = ai::NEATGenome::crossover(parent1, parent2, rng);
childGenome.mutate(rng, mutationParams);

// Create offspring
Creature* offspring = new Creature(position, parentGenome, CreatureType::HERBIVORE);
offspring->initializeNEATBrain(childGenome);
```

## Observable Evolution Behaviors

Over generations, you should observe:

1. **Topology Growth:** Hidden nodes and connections increase
2. **Speciation:** Population splits into distinct species
3. **Specialization:** Different species develop different strategies
4. **Complexity-Fitness Correlation:** More complex brains for better fitness
5. **Activation Diversity:** Various activation functions for different computations

## Configuration Parameters

### MutationParams
```cpp
struct MutationParams {
    float addConnectionProb = 0.05f;
    float addNodeProb = 0.03f;
    float toggleEnableProb = 0.01f;
    float mutateWeightProb = 0.8f;
    float weightPerturbProb = 0.9f;
    float weightPerturbStrength = 0.5f;
    float mutateBiasProb = 0.3f;
    float biasPerturbStrength = 0.2f;
    float mutateActivationProb = 0.05f;
    float mutatePlasticityProb = 0.1f;
    bool allowRecurrent = true;
    float recurrentProb = 0.2f;
};
```

### Speciation Parameters
```cpp
float compatibilityThreshold = 3.0f;
float c1_excess = 1.0f;
float c2_disjoint = 1.0f;
float c3_weight = 0.4f;
float survivalThreshold = 0.2f;  // Top 20% reproduce
int stagnationLimit = 15;
```

## Files Modified for Integration

| File | Changes |
|------|---------|
| `src/entities/Creature.h` | Added NEAT brain members and methods |
| `src/entities/Creature.cpp` | NEAT brain initialization, processing, fitness propagation |
| `src/ai/NEATGenome.h` | Core NEAT data structures (already complete) |
| `src/ai/NEATGenome.cpp` | Mutation, crossover, speciation (already complete) |
| `src/ai/NeuralNetwork.h` | Runtime execution engine (already complete) |
| `src/ai/NeuralNetwork.cpp` | buildFromGenome, forward, plasticity (already complete) |
| `src/ai/CreatureBrainInterface.h` | Bridge between NEAT and creature (already complete) |
| `src/ai/CreatureBrainInterface.cpp` | Event handling, learning (already complete) |

## Deliverables Checklist

- [x] `docs/NEAT_ANALYSIS.md` - This document
- [x] InnovationTracker singleton - `src/ai/NEATGenome.h:57-113`
- [x] `mutateAddConnection()` with cycle detection - `src/ai/NEATGenome.cpp:113-161`
- [x] `mutateAddNode()` with connection splitting - `src/ai/NEATGenome.cpp:163-217`
- [x] Compatibility distance calculation - `src/ai/NEATGenome.cpp:369-424`
- [x] SpeciesManager with speciation - `src/ai/NEATGenome.cpp:536-577`
- [x] NEAT crossover with gene alignment - `src/ai/NEATGenome.cpp:295-363`
- [x] Integration with Creature brain - `src/entities/Creature.cpp:1054-1134`
- [x] Observable topology growth over generations - Via NEATPopulation::evolve()

## Future Enhancements

1. **GPU Acceleration:** Batch network evaluations on GPU
2. **Visualization:** Real-time network topology display
3. **Novelty Search:** Alternative fitness landscape exploration
4. **Modular NEAT:** Evolve reusable network modules
5. **HyperNEAT:** Evolve patterns that generate networks
