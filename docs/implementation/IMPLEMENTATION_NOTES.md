# Neural Network Implementation Notes

## Overview

This document describes the implementation of the advanced neural network system for creature brains in OrganismEvolution. The system supports NEAT-style topology evolution, modular brain architecture, lifetime learning through Hebbian plasticity, and neuromodulation.

## Architecture

### File Structure

```
src/ai/
├── NeuralNetwork.h/cpp       # Core neural network with variable topology
├── NEATGenome.h/cpp          # NEAT genome with evolution operators
├── BrainModules.h/cpp        # Modular brain (Sensory, Emotional, Decision, Motor)
├── CreatureBrainInterface.h/cpp  # Bridge to existing creature code
└── BrainTests.cpp            # Verification tests
```

### Design Philosophy

The system was designed with these principles:

1. **Backward Compatibility**: Existing steering-behavior creatures continue to work
2. **Gradual Adoption**: Brain types can be switched per-creature
3. **Evolvability**: Both weights AND topology can evolve
4. **Lifetime Learning**: Creatures can improve within a single lifetime
5. **Biological Inspiration**: Neuromodulators and modular architecture

## Components

### 1. NeuralNetwork (Core)

The `NeuralNetwork` class supports:

- **Variable Topology**: Any number of nodes and connections
- **Multiple Activation Functions**: Tanh, ReLU, Sigmoid, Gaussian, Sine, etc.
- **Recurrent Connections**: For temporal memory
- **Plasticity**: Connections can learn during the creature's lifetime

Key features:
```cpp
// Activation types
enum class ActivationType {
    LINEAR, SIGMOID, TANH, RELU, LEAKY_RELU,
    ELU, GAUSSIAN, SINE, STEP
};

// Node with runtime state
struct Node {
    float value;        // Current activation
    float prevValue;    // For recurrent connections
    float activity;     // Running average (for homeostasis)
    float eligibility;  // For learning
};

// Connection with plasticity
struct Connection {
    bool plastic;           // Can learn?
    float plasticityRate;   // Learning rate multiplier
    float eligibility;      // Eligibility trace
    float hebbianTerm;      // Accumulated correlation
};
```

### 2. NEATGenome (Evolution)

Implements full NEAT algorithm:

**Gene Representation:**
```cpp
struct NodeGene {
    int id;
    NodeType type;        // INPUT, HIDDEN, OUTPUT, BIAS
    ActivationType activation;
    float bias;
};

struct ConnectionGene {
    int innovation;       // Historical marker
    int fromNode, toNode;
    float weight;
    bool enabled;
    bool recurrent;
};
```

**Mutation Operators:**
- `mutateAddConnection`: Connect previously unconnected nodes
- `mutateAddNode`: Split existing connection, insert node
- `mutateWeights`: Perturb or replace weights
- `mutateToggleEnable`: Enable/disable connections
- `mutateActivation`: Change node activation function
- `mutatePlasticity`: Evolve learning parameters

**Crossover:**
- Genes aligned by innovation number
- Matching genes: randomly from either parent
- Disjoint/excess: from fitter parent

**Speciation:**
- Compatibility distance based on excess, disjoint, and weight differences
- Fitness sharing within species
- Stagnation detection and elimination

### 3. BrainModules (Modular Architecture)

Four specialized modules:

**SensoryProcessor:**
- Inputs: 15 sensory features (distances, angles, internal state)
- Outputs: 8 processed features
- Architecture: Feedforward, feature extraction

**EmotionalModule:**
- Inputs: Processed sensory + internal state
- Outputs: 4 drives (fear, hunger, curiosity, social)
- Function: Motivation and priority weighting

**DecisionMaker:**
- Inputs: Sensory + Memory + Drives + Modulators
- Outputs: 8 action values
- Architecture: Recurrent with plasticity
- Central executive for action selection

**MotorController:**
- Inputs: Decision outputs
- Outputs: 4 motor commands (turn, speed, attack, flee)
- Function: Smooth motor command generation

### 4. Neuromodulation

Two primary modulators:

**Dopamine (Reward):**
```cpp
void onFoodEaten()     // +0.5 dopamine
void onSuccessfulHunt() // +0.8 dopamine
void onDamageTaken()    // -0.3 dopamine
```

**Norepinephrine (Arousal):**
```cpp
void onThreatDetected() // Increase arousal
void onNovelty()        // Increase attention
```

Effects on learning:
```cpp
float getLearningRate() {
    // High acetylcholine = higher learning
    // High norepinephrine = focused learning
    return 0.01f * (0.5f + acetylcholine) * (0.5f + 0.5f * norepinephrine);
}
```

### 5. Lifetime Learning

**Hebbian Rule:**
```cpp
correlation = preActivity * postActivity;
eligibility = 0.95 * eligibility + correlation;
```

**Three-Factor Learning:**
```cpp
deltaW = learningRate * eligibility * reward;
```

**Weight Update:**
```cpp
void updatePlasticity(float reward, float learningRate) {
    for (conn : connections) {
        if (conn.plastic) {
            deltaW = learningRate * conn.plasticityRate *
                     conn.eligibility * reward;
            deltaW -= 0.0001f * conn.weight;  // Weight decay
            conn.weight = clamp(conn.weight + deltaW, -5, 5);
        }
    }
}
```

## Integration Guide

### Using with Existing Creatures

1. **Add to Creature class:**
```cpp
#include "../ai/CreatureBrainInterface.h"

class Creature {
    std::unique_ptr<ai::CreatureBrainInterface> advancedBrain;
};
```

2. **Initialize in constructor:**
```cpp
Creature::Creature(...) {
    advancedBrain = std::make_unique<ai::CreatureBrainInterface>();
    advancedBrain->initialize(ai::CreatureBrainInterface::BrainType::NEAT_EVOLVED);
}
```

3. **Process in update:**
```cpp
void Creature::update(float dt, ...) {
    // Gather sensory info
    float foodDist = findNearestFood();
    float predatorDist = findNearestPredator();
    // etc...

    // Process through brain
    auto output = advancedBrain->process(
        foodDist, foodAngle,
        predatorDist, predatorAngle,
        preyDist, preyAngle,
        allyDist, allyAngle,
        energy / maxEnergy, health, age / maxAge,
        terrainHeight, waterDist,
        recentlyAttacked, recentlyAte,
        dt
    );

    // Use output
    rotation += output.turnAngle * PI;
    float speed = output.speed * maxSpeed;
    velocity = glm::vec3(cos(rotation), 0, sin(rotation)) * speed;

    if (output.attackIntent > 0.5f && nearestPrey) {
        attack(nearestPrey);
    }
}
```

4. **Handle events:**
```cpp
void Creature::consumeFood(float amount) {
    energy += amount;
    advancedBrain->onFoodEaten(amount / 40.0f);  // Normalize
}

void Creature::takeDamage(float damage) {
    energy -= damage;
    advancedBrain->onDamageTaken(damage / maxDamage);
}
```

5. **Evolution in Simulation:**
```cpp
// Create evolution manager
ai::BrainEvolutionManager brainEvolver(100, 23, 4);

// On reproduction
auto childBrain = brainEvolver.createOffspringBrain(
    parent1->advancedBrain,
    parent2->advancedBrain
);

// At generation end
brainEvolver.evolveGeneration();
```

### Configuration Options

**Mutation Rates:**
```cpp
MutationParams params;
params.addConnectionProb = 0.05f;  // 5% chance to add connection
params.addNodeProb = 0.03f;        // 3% chance to add node
params.mutateWeightProb = 0.8f;    // 80% chance to mutate weights
params.weightPerturbStrength = 0.5f;
params.allowRecurrent = true;      // Allow recurrent connections
```

**Speciation:**
```cpp
population.compatibilityThreshold = 3.0f;
population.c1_excess = 1.0f;
population.c2_disjoint = 1.0f;
population.c3_weight = 0.4f;
population.stagnationLimit = 15;  // Generations before removal
```

## Performance Considerations

### Memory Usage

- Each `NeuralNetwork`: ~100-500 bytes base + ~20 bytes per connection
- Each `NEATGenome`: Similar to network + evolution metadata
- `CreatureBrain` (modular): ~2KB per creature
- `CreatureBrain` (NEAT): ~500 bytes - 5KB depending on evolution

### CPU Usage

- Forward pass: O(E) where E = enabled connections
- NEAT crossover: O(max genes)
- Speciation: O(N * S) where N = population, S = species
- Plasticity update: O(E) per learning event

### Optimization Tips

1. **Batch processing**: Process multiple creatures together
2. **Spatial locality**: Keep networks contiguous in memory
3. **Lazy evaluation**: Only compute unused modules on demand
4. **Connection pruning**: Periodically remove zero-weight connections

## Testing

### Running Tests

```bash
# Compile test executable
g++ -std=c++17 -DBRAIN_TESTS_MAIN -o brain_tests \
    src/ai/NeuralNetwork.cpp \
    src/ai/NEATGenome.cpp \
    src/ai/BrainModules.cpp \
    src/ai/CreatureBrainInterface.cpp \
    src/ai/BrainTests.cpp

# Run tests
./brain_tests
```

### Test Coverage

- Neural network construction and forward pass
- Recurrent connection behavior
- NEAT mutation operators
- NEAT crossover
- Compatibility distance
- Brain modules (sensory, emotional, decision, motor)
- Neuromodulator dynamics
- Hebbian learning
- Eligibility traces
- Evolution fitness improvement
- Speciation
- Full brain interface

## Future Enhancements

### Planned

1. **HyperNEAT**: Indirect encoding for large networks
2. **Novelty Search**: Reward behavioral diversity
3. **Working Memory**: LSTM-style gating
4. **Attention Mechanism**: Dynamic input weighting

### Research Directions

1. **Meta-learning**: Evolve learning rules themselves
2. **Modular Evolution**: Evolve module connectivity
3. **Social Learning**: Creatures share knowledge
4. **Hierarchical Behavior**: Multiple timescale decisions

## Troubleshooting

### Common Issues

**Network produces constant output:**
- Check input normalization (should be roughly [-1, 1])
- Verify connections are enabled
- Check for exploding/vanishing weights

**No evolution progress:**
- Increase population size
- Lower compatibility threshold (more species)
- Increase mutation rates

**Learning doesn't improve behavior:**
- Verify reward signal reaches learning function
- Check eligibility trace decay rate
- Ensure plasticity is enabled on relevant connections

**Performance issues:**
- Profile to find bottleneck
- Reduce network complexity
- Consider spatial hashing for creature queries

## References

1. Stanley, K.O. & Miikkulainen, R. (2002). NEAT: Evolving Neural Networks through Augmenting Topologies.
2. Soltoggio, A., Stanley, K.O., & Risi, S. (2018). Born to Learn: Evolved Plastic Neural Networks.
3. Miconi, T., et al. (2018). Differentiable Plasticity.
