# NEAT Algorithm Research and Implementation Guide

## Overview

NEAT (NeuroEvolution of Augmenting Topologies) is a genetic algorithm for evolving artificial neural networks, developed by Kenneth O. Stanley and Risto Miikkulainen at the University of Texas at Austin in 2002.

## Key Innovations of NEAT

### 1. Genetic Encoding with Historical Markings

NEAT uses **innovation numbers** to track the historical origin of every gene. When a new structural mutation occurs (adding a node or connection), it receives a globally unique innovation number.

**Benefits:**
- Enables meaningful crossover between networks of different topologies
- Genes with the same innovation number represent the same structural component
- Solves the "competing conventions" problem in neuroevolution

### 2. Protecting Innovation Through Speciation

New structural innovations often initially harm fitness before optimization can occur. NEAT protects these innovations by:
- Grouping similar networks into **species**
- Implementing **fitness sharing** within species
- Allowing innovations time to optimize before competing globally

**Compatibility Distance Formula:**
```
δ = (c1 * E) / N + (c2 * D) / N + c3 * W̄
```
Where:
- E = number of excess genes
- D = number of disjoint genes
- W̄ = average weight difference of matching genes
- c1, c2, c3 = coefficients (typically c1=1.0, c2=1.0, c3=0.4)
- N = number of genes in larger genome (set to 1 for small genomes)

### 3. Starting from Minimal Structure

Unlike other neuroevolution methods that start with random topologies, NEAT begins with **minimal networks** - just input nodes directly connected to output nodes.

**Benefits:**
- Search starts in a lower-dimensional space
- Complexity grows only as needed
- More efficient than pruning a complex network

## Core Components

### Gene Types

#### Node Gene
```cpp
struct NodeGene {
    int id;                    // Unique identifier
    NodeType type;             // INPUT, HIDDEN, OUTPUT, BIAS
    ActivationType activation; // SIGMOID, TANH, RELU, etc.
    float bias;                // Node bias value
    int layer;                 // Topological layer (for ordering)
};
```

#### Connection Gene
```cpp
struct ConnectionGene {
    int innovation;  // Historical marker
    int fromNode;    // Source node ID
    int toNode;      // Target node ID
    float weight;    // Connection weight
    bool enabled;    // Active or disabled
    bool recurrent;  // Connects to earlier layer
};
```

### Innovation Tracking

The **InnovationTracker** maintains a global registry of structural mutations:
- Maps (from_node, to_node) pairs to innovation numbers for connections
- Maps split connection IDs to new node IDs
- Ensures identical mutations receive the same innovation number

### Species Management

A **Species** groups genomes with similar topology:
- Representative genome defines the species
- Members must be within compatibility threshold of representative
- Fitness sharing divides fitness by species size
- Stagnant species (no improvement) are eventually removed

## Mutation Operations

### 1. Weight Mutation (Most Common)
- **Perturbation**: Add small random value to existing weights
- **Replacement**: Replace weight with new random value
- Typical probabilities: 80% perturb, 10% replace

### 2. Add Connection Mutation
- Select random valid source and target nodes
- Check if connection already exists
- Detect if connection would create a cycle
- Mark recurrent connections appropriately
- Assign innovation number via tracker

### 3. Add Node Mutation
- Select random enabled non-recurrent connection
- Disable the old connection
- Create new hidden node
- Add connection: old_source → new_node (weight = 1.0)
- Add connection: new_node → old_target (weight = old_weight)

### 4. Toggle Enable Mutation
- Randomly enable/disable connections
- Low probability (~1%) to maintain stability

### 5. Activation Mutation
- Change activation function of hidden nodes
- Supports: SIGMOID, TANH, RELU, LEAKY_RELU, ELU, GAUSSIAN, SINE

### 6. Bias Mutation
- Perturb bias values of hidden and output nodes
- Similar to weight mutation

## Crossover Algorithm

NEAT crossover aligns genes by innovation number:

```
1. Identify matching, disjoint, and excess genes
2. For matching genes: randomly select from either parent
3. For disjoint/excess genes: inherit from fitter parent only
4. Handle disabled genes: 75% chance to remain disabled if either parent has it disabled
```

**Key Points:**
- Fitter parent contributes disjoint/excess genes
- Matching genes can come from either parent
- No bloat from less-fit parent's extra complexity

## Fitness Evaluation

### Multi-objective Fitness
Our implementation uses composite fitness:
```cpp
float calculateFitness(const Creature& creature) {
    float fitness = 0.0f;

    // Survival (weighted by age)
    fitness += creature.age * 0.1f;

    // Resource acquisition
    fitness += creature.foodEaten * 10.0f;

    // Reproduction success
    fitness += creature.offspringCount * 50.0f;

    // Exploration (novelty)
    fitness += creature.distanceTraveled * 0.01f;

    // Efficiency
    fitness += creature.energyEfficiency * 5.0f;

    // Type-specific bonuses
    if (creature.type == CARNIVORE) {
        fitness += creature.kills * 30.0f;
    }

    return fitness;
}
```

### Adjusted Fitness
Fitness sharing prevents any single species from dominating:
```cpp
adjustedFitness = rawFitness / speciesSize
```

## Evolution Cycle

### Per Generation:
1. **Evaluate fitness** for all genomes
2. **Speciate** population (assign to species by compatibility)
3. **Calculate adjusted fitness** (fitness sharing)
4. **Update stagnation** counters for species
5. **Remove stagnant species** (except best-performing)
6. **Allocate offspring** proportional to species adjusted fitness
7. **Reproduce** via selection, crossover, and mutation
8. **Elitism**: Keep best genome from large species unchanged

## Neural Network Execution

### Forward Pass Algorithm

1. Set input node values from sensory data
2. Set bias node value to 1.0
3. **For each layer (sorted order):**
   - Sum weighted inputs from enabled connections
   - Add node bias
   - Apply activation function
   - Store for recurrent connections: `prevValue = value`
4. Collect output node values

### Recurrent Connection Handling
- Store previous timestep values
- Use `prevValue` for recurrent inputs (temporal delay)
- Enables memory and temporal processing

## Plasticity Extensions

Our implementation extends basic NEAT with synaptic plasticity:

### Hebbian Learning
```cpp
hebbianTerm = preActivity * postActivity
eligibility = 0.95 * eligibility + hebbianTerm
```

### Three-Factor Learning Rule
```cpp
deltaWeight = learningRate * plasticityRate * eligibility * reward
```

This allows networks to learn during their lifetime, not just through evolution.

## Neuromodulation

Inspired by biological neurotransmitters:
- **Dopamine**: Reward/pleasure signal, modulates learning
- **Norepinephrine**: Arousal/alertness, affects fear response
- **Serotonin**: Mood/patience, reduces impulsivity
- **Acetylcholine**: Attention/learning rate modulation

## Parameter Guidelines

### Mutation Rates
| Parameter | Typical Value | Notes |
|-----------|---------------|-------|
| Weight mutation prob | 0.8 | Most common mutation |
| Weight perturb prob | 0.9 | vs. replacement |
| Weight perturb strength | 0.5 | Gaussian sigma |
| Add connection prob | 0.05 | Structural |
| Add node prob | 0.03 | Structural |
| Toggle enable prob | 0.01 | Rare |
| Activation mutation prob | 0.05 | Rare |

### Speciation Parameters
| Parameter | Typical Value | Notes |
|-----------|---------------|-------|
| Compatibility threshold | 3.0 | Adjust dynamically |
| c1 (excess) | 1.0 | Standard |
| c2 (disjoint) | 1.0 | Standard |
| c3 (weight diff) | 0.4 | Lower than structural |
| Stagnation limit | 15 | Generations |
| Survival threshold | 0.2 | Top 20% reproduce |

## References

1. Stanley, K. O., & Miikkulainen, R. (2002). Evolving Neural Networks through Augmenting Topologies. *Evolutionary Computation*, 10(2), 99-127.

2. Stanley, K. O., & Miikkulainen, R. (2002). Efficient Evolution of Neural Network Topologies. *Proceedings of the 2002 Congress on Evolutionary Computation*, 1757-1762.

3. Stanley, K. O. (2004). *Efficient Evolution of Neural Networks Through Complexification*. PhD dissertation, The University of Texas at Austin.

## Implementation Notes

### Thread Safety
- InnovationTracker uses singleton pattern
- Mutations should use thread-local random generators
- Population evolution is sequential; fitness evaluation can be parallel

### Memory Efficiency
- Use vectors instead of maps for small genomes
- Reserve capacity for expected growth
- Consider pooling for frequently created/destroyed networks

### Performance Optimization
- Pre-compute execution order after topology changes
- Use node ID lookup map for O(1) access
- Batch weight mutations for cache efficiency
