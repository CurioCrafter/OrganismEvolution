# NEAT Neural Network Architecture

## System Overview

This document describes the architecture of the NEAT (NeuroEvolution of Augmenting Topologies) implementation for the OrganismEvolution simulation. The system enables creatures to evolve neural network brains that control their behavior.

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          NEAT EVOLUTION SYSTEM                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌─────────────────┐       ┌──────────────────┐       ┌─────────────────┐   │
│  │ InnovationTracker│       │   NEATPopulation │       │  SpeciesManager │   │
│  │   (Singleton)    │◄──────│                  │──────►│                 │   │
│  │                  │       │  - genomes[]     │       │  - species[]    │   │
│  │ - connectionMap  │       │  - bestGenome    │       │  - threshold    │   │
│  │ - nodeMap        │       │  - generation    │       │  - stagnation   │   │
│  └─────────────────┘       └────────┬─────────┘       └─────────────────┘   │
│                                      │                                       │
│                                      ▼                                       │
│                            ┌─────────────────┐                              │
│                            │   NEATGenome    │                              │
│                            │                 │                              │
│                            │ - nodes[]       │                              │
│                            │ - connections[] │                              │
│                            │ - fitness       │                              │
│                            │ - speciesId     │                              │
│                            └────────┬────────┘                              │
│                                     │                                        │
│                                     ▼                                        │
│                            ┌─────────────────┐                              │
│                            │  NeuralNetwork  │                              │
│                            │                 │                              │
│                            │ - forward()     │                              │
│                            │ - plasticity    │                              │
│                            │ - hebbian       │                              │
│                            └─────────────────┘                              │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                         CREATURE BRAIN SYSTEM                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌──────────────────────────────────────────────────────────────────────┐   │
│  │                     CreatureBrainInterface                            │   │
│  │                                                                        │   │
│  │  ┌───────────────┐   ┌──────────────┐   ┌──────────────────────────┐ │   │
│  │  │ LEGACY_STEERING│   │MODULAR_BRAIN │   │     NEAT_EVOLVED         │ │   │
│  │  │ (Hardcoded)    │   │              │   │                          │ │   │
│  │  └───────────────┘   │              │   │  ┌────────────────────┐  │ │   │
│  │                       │  Sensory    │   │  │   NEATGenome       │  │ │   │
│  │                       │  Emotional  │   │  │        ↓           │  │ │   │
│  │                       │  Decision   │   │  │  NeuralNetwork     │  │ │   │
│  │                       │  Motor      │   │  │        ↓           │  │ │   │
│  │                       │              │   │  │  outputs[]        │  │ │   │
│  │                       └──────────────┘   │  └────────────────────┘  │ │   │
│  │                                          └──────────────────────────┘ │   │
│  └──────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

## File Structure

```
src/ai/
├── NeuralNetwork.h/.cpp         # Core neural network execution
├── NEATGenome.h/.cpp            # NEAT genome with mutations/crossover
├── BrainModules.h/.cpp          # Modular brain components
├── CreatureBrainInterface.h/.cpp # Integration with creatures
├── BrainTests.cpp               # Unit tests
└── GPUSteeringCompute.h/.cpp    # GPU acceleration (optional)

src/ui/
├── NEATVisualizer.h/.cpp        # ImGui visualization panels
└── DashboardMetrics.h/.cpp      # Statistics tracking

docs/
├── NEAT_RESEARCH.md             # Algorithm details
└── NEAT_ARCHITECTURE.md         # This file
```

## Core Classes

### 1. NEATGenome (`src/ai/NEATGenome.h`)

The genetic representation of a neural network.

```cpp
class NEATGenome {
    std::vector<NodeGene> m_nodes;
    std::vector<ConnectionGene> m_connections;
    int m_inputCount, m_outputCount;
    float m_fitness, m_adjustedFitness;
    int m_speciesId;

public:
    // Create minimal topology
    void createMinimal(int numInputs, int numOutputs, std::mt19937& rng);

    // Mutation methods
    void mutate(std::mt19937& rng, const MutationParams& params);
    void mutateWeights(...);
    void mutateAddConnection(...);
    void mutateAddNode(...);
    void mutateToggleEnable(...);
    void mutateActivation(...);
    void mutateBias(...);
    void mutatePlasticity(...);

    // Crossover
    static NEATGenome crossover(const NEATGenome& fitter,
                                 const NEATGenome& other,
                                 std::mt19937& rng);

    // Species compatibility
    float compatibilityDistance(const NEATGenome& other, ...);

    // Build executable network
    std::unique_ptr<NeuralNetwork> buildNetwork() const;
};
```

### 2. NeuralNetwork (`src/ai/NeuralNetwork.h`)

Executable neural network with plasticity support.

```cpp
class NeuralNetwork {
    std::vector<Node> m_nodes;
    std::vector<Connection> m_connections;
    std::vector<int> m_executionOrder;

public:
    // Build from genome
    void buildFromGenome(const NEATGenome& genome);

    // Execution
    std::vector<float> forward(const std::vector<float>& inputs);
    void reset();

    // Plasticity
    void accumulateHebbian();
    void updatePlasticity(float reward, float learningRate);
    void decayEligibility(float decay);
};
```

### 3. CreatureBrain (`src/ai/BrainModules.h`)

High-level brain abstraction with modular components.

```cpp
class CreatureBrain {
    // Modular components
    SensoryProcessor m_sensory;
    EmotionalModule m_emotional;
    DecisionMaker m_decision;
    MotorController m_motor;
    WorkingMemory m_memory;

    // NEAT brain (alternative)
    NEATGenome m_genome;
    std::unique_ptr<NeuralNetwork> m_neatNetwork;
    bool m_useNEATGenome;

    // Neuromodulators
    NeuromodulatorState m_modulators;

public:
    // Initialize
    void initialize(std::mt19937& rng);
    void initializeFromGenome(const NEATGenome& genome);

    // Processing
    MotorOutput process(const SensoryInput& input, float deltaTime);

    // Events
    void onFoodEaten(float amount);
    void onDamageTaken(float amount);
    void onThreatDetected(float level);

    // Learning
    void learn(float reward);
};
```

### 4. CreatureBrainInterface (`src/ai/CreatureBrainInterface.h`)

Bridge between AI system and creature entities.

```cpp
class CreatureBrainInterface {
public:
    enum class BrainType {
        LEGACY_STEERING,  // Original hardcoded behaviors
        MODULAR_BRAIN,    // Separate neural modules
        NEAT_EVOLVED      // Full NEAT evolution
    };

    void initialize(BrainType type, int inputSize, int outputSize);
    void initializeFromGenome(const NEATGenome& genome);

    MotorOutput process(/* sensory inputs */, float deltaTime);

    // Event callbacks
    void onFoodEaten(float amount);
    void onDamageTaken(float amount);
    void onThreatDetected(float level);
    void onSuccessfulHunt();
    void onSuccessfulEscape();

    // Learning
    void learn(float reward);

    // Genome access
    NEATGenome& getGenome();
    void setGenome(const NEATGenome& genome);
};
```

### 5. BrainEvolutionManager (`src/ai/CreatureBrainInterface.h`)

Population-level evolution management.

```cpp
class BrainEvolutionManager {
    std::vector<NEATGenome> m_genomes;
    std::vector<Species> m_species;
    MutationParams m_mutationParams;

public:
    // Create brains
    std::unique_ptr<CreatureBrainInterface> createBrain();
    std::unique_ptr<CreatureBrainInterface> createOffspringBrain(
        const CreatureBrainInterface& parent);
    std::unique_ptr<CreatureBrainInterface> createOffspringBrain(
        const CreatureBrainInterface& parent1,
        const CreatureBrainInterface& parent2);

    // Evolution
    void reportFitness(const NEATGenome& genome, float fitness);
    void evolveGeneration();

    // Statistics
    int getGeneration() const;
    int getSpeciesCount() const;
    float getAverageFitness() const;
    float getBestFitness() const;
    const NEATGenome& getBestGenome() const;
};
```

## Data Flow

### 1. Sensory Input Pipeline

```
World State
    │
    ▼
SensorySystem::sense()
    │
    ├── Vision (raycasts, FOV)
    ├── Hearing (sound events)
    ├── Smell (pheromones, food)
    └── Touch (proximity)
    │
    ▼
SensoryInput struct
    │
    ├── nearestFoodDistance/Angle
    ├── nearestPredatorDistance/Angle
    ├── nearestPreyDistance/Angle
    ├── energy, health, age
    └── environmental data
    │
    ▼
Neural Network Input (15 floats + modulator state)
```

### 2. Decision Pipeline

```
Neural Network Input
    │
    ▼
NeuralNetwork::forward()
    │
    ├── Set input node values
    ├── Process in topological order
    │   ├── Sum weighted inputs
    │   ├── Add bias
    │   ├── Apply activation
    │   └── Handle recurrent connections
    └── Collect output values
    │
    ▼
MotorOutput struct
    │
    ├── turnAngle (-1 to 1)
    ├── speed (0 to 1)
    ├── attackIntent (0 to 1)
    └── fleeIntent (0 to 1)
    │
    ▼
Creature behavior update
```

### 3. Evolution Pipeline

```
Creature lifetime
    │
    ├── Accumulate fitness metrics
    │   ├── Survival time
    │   ├── Food eaten
    │   ├── Offspring produced
    │   └── Kills (carnivores)
    │
    ▼
On creature death
    │
    ├── Calculate final fitness
    ├── Report to BrainEvolutionManager
    │
    ▼
End of generation
    │
    ├── Speciate population
    ├── Calculate adjusted fitness
    ├── Remove stagnant species
    ├── Reproduce (crossover + mutation)
    └── Next generation
```

## Neural Network Inputs (23 total)

| Index | Name | Range | Description |
|-------|------|-------|-------------|
| 0-1 | Food dist/angle | 0-1, -1-1 | Nearest food source |
| 2-3 | Predator dist/angle | 0-1, -1-1 | Nearest threat |
| 4-5 | Prey dist/angle | 0-1, -1-1 | Nearest prey (carnivores) |
| 6-7 | Ally dist/angle | 0-1, -1-1 | Nearest conspecific |
| 8 | Energy | 0-1 | Current energy level |
| 9 | Health | 0-1 | Current health |
| 10 | Age | 0-1 | Normalized by lifespan |
| 11 | Terrain height | -1-1 | Local elevation |
| 12 | Water proximity | 0-1 | Distance to water |
| 13 | Was attacked | 0-1 | Recent damage flag |
| 14 | Recent food | 0-1 | Recently ate flag |
| 15-18 | Neuromodulators | -1-1 | DA, NE, 5-HT, ACh |
| 19-22 | Memory state | various | Working memory |

## Neural Network Outputs (4-8)

| Index | Name | Range | Description |
|-------|------|-------|-------------|
| 0 | Turn angle | -1-1 | Desired heading change |
| 1 | Speed | 0-1 | Movement speed |
| 2 | Attack intent | 0-1 | Attack threshold |
| 3 | Flee intent | 0-1 | Flee urgency |
| 4-7 | Optional | various | Memory gate, vocalization, etc. |

## Species and Speciation

### Compatibility Threshold Adjustment

The compatibility threshold is dynamically adjusted to maintain target species count:

```cpp
if (speciesCount < targetSpecies) {
    threshold *= 0.95f;  // Decrease to split more
} else if (speciesCount > targetSpecies) {
    threshold *= 1.05f;  // Increase to merge more
}
```

### Stagnation Handling

```cpp
if (species.stagnantGenerations > stagnationLimit) {
    if (species.id != bestSpeciesId) {
        removeSpecies(species);
    }
}
```

## Visualization Components

### NEATVisualizer (`src/ui/NEATVisualizer.h`)

```cpp
class NeuralNetworkVisualizer {
    // Render network topology with nodes and connections
    void render(const NeuralNetwork* network, ImVec2 canvasSize);

    // Compact version for inspector
    void renderCompact(const NeuralNetwork* network, ImVec2 canvasSize);
};

class NEATEvolutionPanel {
    // Statistics graphs
    void renderFitnessGraph();
    void renderSpeciesGraph();
    void renderComplexityGraph();

    // Current state
    void renderBestGenomeInfo();
    void renderSpeciesBreakdown();
};

class CreatureBrainInspector {
    // Real-time brain state
    void renderNeuromodulators();
    void renderDrives();
    void renderNetworkTopology();
    void renderWeightDistribution();
};
```

## Save/Load System

### Genome Serialization

Binary format for efficient storage:
```
Header:
  - Magic: "NEAT" (4 bytes)
  - Version: int32

Nodes:
  - Count: int32
  - For each node:
    - id, type, activation, bias, layer

Connections:
  - Count: int32
  - For each connection:
    - innovation, from, to, weight, enabled, recurrent

Metadata:
  - fitness: float
```

### JSON Export

Human-readable format for analysis:
```json
{
  "fitness": 123.45,
  "inputCount": 15,
  "outputCount": 4,
  "nodes": [...],
  "connections": [...]
}
```

## Performance Considerations

### Memory Pooling
- Pre-allocate node and connection vectors
- Use move semantics in crossover
- Pool NeuralNetwork instances for evaluation

### Parallel Evaluation
- Fitness evaluation is embarrassingly parallel
- Use thread pool for large populations
- Speciation is sequential (shared state)

### GPU Acceleration
- Network forward pass can be batched
- Weight matrix representation for GPU
- Sparse matrix for variable topology

## Extension Points

### Custom Activation Functions
```cpp
enum class ActivationType {
    // Existing
    SIGMOID, TANH, RELU, LEAKY_RELU, ELU, GAUSSIAN, SINE,
    // Add new
    CUSTOM_1, CUSTOM_2, ...
};
```

### Custom Fitness Components
```cpp
// In fitness calculation
fitness += customComponent(creature) * weight;
```

### Alternative Selection
```cpp
// Replace tournament selection
NEATGenome* select(Species& species) {
    // Implement different selection strategy
}
```

## Testing

Unit tests in `src/ai/BrainTests.cpp`:
- Genome mutation consistency
- Crossover alignment
- Network forward pass
- Plasticity updates
- Speciation correctness
- Serialization round-trip
