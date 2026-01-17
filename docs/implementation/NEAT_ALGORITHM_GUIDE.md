# NEAT Algorithm Implementation Guide
## NeuroEvolution of Augmenting Topologies

### Overview

NEAT (NeuroEvolution of Augmenting Topologies) is a genetic algorithm for evolving artificial neural networks, developed by Kenneth O. Stanley and Risto Miikkulainen at the University of Texas at Austin in 2002. The algorithm is unique in that it evolves both the weights AND topology of neural networks simultaneously.

**Key Paper**: "Evolving Neural Networks through Augmenting Topologies" (Stanley & Miikkulainen, 2002)
- DOI: 10.1162/106365602320169811
- PDF: https://nn.cs.utexas.edu/downloads/papers/stanley.ec02.pdf

This paper received the Outstanding Paper of the Decade (2002-2012) award from the International Society for Artificial Life.

---

## Core Concepts

### 1. Minimal Initial Topology

NEAT begins evolution with the simplest possible networks - just input and output nodes with random connection weights. No hidden layers exist initially.

**Why This Matters:**
- Searches through minimal weight dimensions first
- Reduces generations needed to find solutions
- Complexity emerges only when beneficial
- Avoids bloated network topologies

**Implementation:**
```cpp
// Initial genome structure
struct InitialGenome {
    std::vector<NodeGene> inputNodes;   // Fixed count based on problem
    std::vector<NodeGene> outputNodes;  // Fixed count based on problem
    std::vector<ConnectionGene> connections; // Direct input->output connections
    // NO hidden nodes initially
};
```

**Starting Connectivity Options:**
- `unconnected` - No initial connections
- `full_direct` - All inputs connected to all outputs
- `partial_direct` - Subset of possible connections
- `fs_neat_nohidden` - FS-NEAT style (one random connection per output)

---

### 2. Genetic Encoding

NEAT uses a direct encoding with two types of genes:

#### Node Genes
```cpp
struct NodeGene {
    int nodeId;              // Unique identifier
    NodeType type;           // INPUT, OUTPUT, HIDDEN, BIAS
    ActivationFunction activation;  // sigmoid, tanh, relu, etc.
    float bias;              // Node bias value
    float response;          // Response multiplier (optional)
};
```

#### Connection Genes
```cpp
struct ConnectionGene {
    int innovationNumber;    // Historical marking (CRITICAL)
    int sourceNode;          // Input node ID
    int targetNode;          // Output node ID
    float weight;            // Connection weight
    bool enabled;            // Whether connection is active
};
```

**Key Insight:** The innovation number is the crucial element that enables meaningful crossover between different topologies.

---

### 3. Innovation Numbers (Historical Markings)

Innovation numbers solve the fundamental problem of how to perform crossover between neural networks with different structures.

**How It Works:**
1. A global innovation counter is maintained across the entire population
2. Each NEW structural mutation (add node or add connection) gets the next innovation number
3. If the same mutation occurs in multiple genomes within a generation, they get the SAME innovation number
4. Innovation numbers are inherited during crossover

**Implementation:**
```cpp
class InnovationTracker {
private:
    int globalInnovationNumber = 0;

    // Track innovations within current generation to avoid duplicates
    // Key: (sourceNode, targetNode), Value: innovationNumber
    std::map<std::pair<int,int>, int> currentGenerationConnections;

    // For node additions: Key: connectionInnovation, Value: newNodeId
    std::map<int, int> currentGenerationNodes;

public:
    int getConnectionInnovation(int source, int target) {
        auto key = std::make_pair(source, target);
        auto it = currentGenerationConnections.find(key);
        if (it != currentGenerationConnections.end()) {
            return it->second;  // Same mutation already happened
        }
        int innovation = ++globalInnovationNumber;
        currentGenerationConnections[key] = innovation;
        return innovation;
    }

    void resetGeneration() {
        currentGenerationConnections.clear();
        currentGenerationNodes.clear();
    }
};
```

**Critical Point:** Innovation numbers NEVER change once assigned. They represent the historical origin of each gene.

---

### 4. Mutation Types

NEAT employs several mutation operators:

#### 4.1 Weight Mutation
Most common mutation - adjusts existing connection weights.

```cpp
void mutateWeights(Genome& genome, float mutateRate, float perturbPower, float replaceRate) {
    for (auto& conn : genome.connections) {
        if (randomFloat() < mutateRate) {
            if (randomFloat() < replaceRate) {
                // Complete replacement with new random weight
                conn.weight = randomFloat(-1.0f, 1.0f);
            } else {
                // Gaussian perturbation
                conn.weight += randomGaussian() * perturbPower;
                conn.weight = clamp(conn.weight, -8.0f, 8.0f);
            }
        }
    }
}
```

**Recommended Rates:**
- `weight_mutate_rate`: 0.8 (80% chance per connection)
- `weight_mutate_power`: 0.5 (standard deviation)
- `weight_replace_rate`: 0.1 (10% chance of full replacement)

#### 4.2 Add Connection Mutation
Creates a new connection between previously unconnected nodes.

```cpp
void mutateAddConnection(Genome& genome, InnovationTracker& tracker) {
    // Select random source (not output) and target (not input)
    int source = selectRandomNonOutputNode(genome);
    int target = selectRandomNonInputNode(genome);

    // Check if connection already exists
    if (connectionExists(genome, source, target)) {
        // Optionally re-enable if disabled
        return;
    }

    // Check for cycles in feed-forward networks
    if (genome.feedForward && wouldCreateCycle(genome, source, target)) {
        return;
    }

    int innovation = tracker.getConnectionInnovation(source, target);
    genome.connections.push_back({
        .innovationNumber = innovation,
        .sourceNode = source,
        .targetNode = target,
        .weight = randomFloat(-1.0f, 1.0f),
        .enabled = true
    });
}
```

**Recommended Rate:** `conn_add_prob`: 0.5

#### 4.3 Add Node Mutation
Splits an existing connection by inserting a new node.

```cpp
void mutateAddNode(Genome& genome, InnovationTracker& tracker) {
    // Select random enabled connection
    ConnectionGene* conn = selectRandomEnabledConnection(genome);
    if (!conn) return;

    // Disable original connection
    conn->enabled = false;

    // Create new node
    int newNodeId = tracker.getNewNodeId(conn->innovationNumber);
    genome.nodes.push_back({
        .nodeId = newNodeId,
        .type = HIDDEN,
        .activation = DEFAULT_ACTIVATION,
        .bias = 0.0f
    });

    // Create two new connections
    // Connection 1: source -> new node (weight = 1.0 to minimize disruption)
    int innov1 = tracker.getConnectionInnovation(conn->sourceNode, newNodeId);
    genome.connections.push_back({
        .innovationNumber = innov1,
        .sourceNode = conn->sourceNode,
        .targetNode = newNodeId,
        .weight = 1.0f,  // IMPORTANT: Weight of 1 minimizes initial disruption
        .enabled = true
    });

    // Connection 2: new node -> target (inherits original weight)
    int innov2 = tracker.getConnectionInnovation(newNodeId, conn->targetNode);
    genome.connections.push_back({
        .innovationNumber = innov2,
        .sourceNode = newNodeId,
        .targetNode = conn->targetNode,
        .weight = conn->weight,  // IMPORTANT: Inherit original weight
        .enabled = true
    });
}
```

**Why Weight=1.0 on Input Connection:**
- The new node initially acts nearly as an identity function
- Introduces nonlinearity gradually
- Minimizes disruption to existing behavior
- Gives the mutation time to prove useful

**Recommended Rate:** `node_add_prob`: 0.2

#### 4.4 Enable/Disable Mutation
Toggles the enabled status of connections.

```cpp
void mutateToggleEnable(Genome& genome, float rate) {
    for (auto& conn : genome.connections) {
        if (randomFloat() < rate) {
            conn.enabled = !conn.enabled;
        }
    }
}
```

**Recommended Rate:** `enabled_mutate_rate`: 0.01

---

### 5. Crossover Between Genomes

NEAT's crossover is made possible by innovation numbers, which allow alignment of genes from different topologies.

#### Gene Classification During Crossover:
- **Matching genes**: Same innovation number in both parents
- **Disjoint genes**: Innovation numbers within the range of both parents, but only in one
- **Excess genes**: Innovation numbers beyond the range of one parent

```cpp
Genome crossover(const Genome& parent1, const Genome& parent2,
                  float fitness1, float fitness2) {
    Genome offspring;

    // Determine more fit parent (or equal fitness)
    bool parent1MoreFit = fitness1 > fitness2;
    bool equalFitness = (fitness1 == fitness2);

    // Create map of parent2 genes by innovation number
    std::map<int, ConnectionGene*> parent2Genes;
    for (auto& conn : parent2.connections) {
        parent2Genes[conn.innovationNumber] = &conn;
    }

    // Iterate through parent1 genes
    for (const auto& gene1 : parent1.connections) {
        auto it = parent2Genes.find(gene1.innovationNumber);

        if (it != parent2Genes.end()) {
            // MATCHING GENE - randomly choose from either parent
            if (randomFloat() < 0.5f) {
                offspring.connections.push_back(gene1);
            } else {
                offspring.connections.push_back(*it->second);
            }

            // Handle disabled genes - 25% chance to re-enable
            if (!gene1.enabled || !it->second->enabled) {
                if (randomFloat() < 0.25f) {
                    offspring.connections.back().enabled = true;
                }
            }
        } else {
            // DISJOINT/EXCESS from parent1
            if (parent1MoreFit || equalFitness) {
                offspring.connections.push_back(gene1);
            }
        }
    }

    // If equal fitness or parent2 more fit, also inherit its unique genes
    if (!parent1MoreFit) {
        for (const auto& gene2 : parent2.connections) {
            // Check if gene2's innovation exists in parent1
            bool existsInP1 = false;
            for (const auto& g1 : parent1.connections) {
                if (g1.innovationNumber == gene2.innovationNumber) {
                    existsInP1 = true;
                    break;
                }
            }
            if (!existsInP1) {
                if (!parent1MoreFit && (equalFitness || randomFloat() < 0.5f)) {
                    offspring.connections.push_back(gene2);
                }
            }
        }
    }

    // Collect required nodes from connections
    collectNodesFromConnections(offspring);

    return offspring;
}
```

**Key Points:**
- Matching genes are randomly inherited from either parent
- Disjoint/excess genes come from the MORE FIT parent
- If fitness is equal, disjoint/excess may come from both
- Disabled genes have 25% chance of being re-enabled

---

### 6. Speciation and Compatibility Distance

Species protect structural innovations by grouping similar genomes together.

#### Compatibility Distance Formula:

```
δ = (c1 * E / N) + (c2 * D / N) + (c3 * W̄)
```

Where:
- `E` = Number of excess genes
- `D` = Number of disjoint genes
- `N` = Number of genes in the larger genome (use 1 if N < threshold)
- `W̄` = Average weight difference of matching genes
- `c1, c2, c3` = Tunable coefficients

```cpp
float compatibilityDistance(const Genome& g1, const Genome& g2,
                            float c1, float c2, float c3) {
    int maxInnovG1 = getMaxInnovation(g1);
    int maxInnovG2 = getMaxInnovation(g2);
    int maxInnov = std::max(maxInnovG1, maxInnovG2);
    int minMaxInnov = std::min(maxInnovG1, maxInnovG2);

    int matching = 0;
    int disjoint = 0;
    int excess = 0;
    float weightDiff = 0.0f;

    // Create map for g2
    std::map<int, float> g2Weights;
    for (const auto& conn : g2.connections) {
        g2Weights[conn.innovationNumber] = conn.weight;
    }

    // Count genes from g1
    for (const auto& conn : g1.connections) {
        auto it = g2Weights.find(conn.innovationNumber);
        if (it != g2Weights.end()) {
            matching++;
            weightDiff += std::abs(conn.weight - it->second);
        } else if (conn.innovationNumber > minMaxInnov) {
            excess++;
        } else {
            disjoint++;
        }
    }

    // Count non-matching genes from g2
    std::set<int> g1Innovations;
    for (const auto& conn : g1.connections) {
        g1Innovations.insert(conn.innovationNumber);
    }
    for (const auto& conn : g2.connections) {
        if (g1Innovations.find(conn.innovationNumber) == g1Innovations.end()) {
            if (conn.innovationNumber > minMaxInnov) {
                excess++;
            } else {
                disjoint++;
            }
        }
    }

    float N = std::max(g1.connections.size(), g2.connections.size());
    if (N < 20) N = 1;  // Normalize only for larger genomes

    float avgWeightDiff = (matching > 0) ? weightDiff / matching : 0;

    return (c1 * excess / N) + (c2 * disjoint / N) + (c3 * avgWeightDiff);
}
```

**Recommended Coefficients:**
- `c1` (excess): 1.0
- `c2` (disjoint): 1.0
- `c3` (weight): 0.4
- `compatibility_threshold`: 3.0

#### Species Assignment Algorithm:

```cpp
void assignToSpecies(std::vector<Genome>& population,
                      std::vector<Species>& species,
                      float threshold) {
    for (auto& genome : population) {
        bool found = false;

        for (auto& sp : species) {
            // Compare with species representative
            float distance = compatibilityDistance(
                genome, sp.representative, c1, c2, c3);

            if (distance < threshold) {
                sp.members.push_back(&genome);
                found = true;
                break;
            }
        }

        if (!found) {
            // Create new species with this genome as representative
            Species newSpecies;
            newSpecies.representative = genome;
            newSpecies.members.push_back(&genome);
            species.push_back(newSpecies);
        }
    }

    // Update representatives for next generation (random member)
    for (auto& sp : species) {
        if (!sp.members.empty()) {
            int idx = randomInt(0, sp.members.size() - 1);
            sp.representative = *sp.members[idx];
        }
    }
}
```

---

### 7. Fitness Sharing

Fitness sharing prevents any single species from dominating the population.

#### Adjusted Fitness Formula:

```
f'_i = f_i / |S_i|
```

Where `|S_i|` is the number of members in genome i's species.

```cpp
void calculateAdjustedFitness(std::vector<Species>& allSpecies) {
    for (auto& species : allSpecies) {
        int speciesSize = species.members.size();
        for (auto* genome : species.members) {
            genome->adjustedFitness = genome->fitness / speciesSize;
        }
    }
}
```

#### Offspring Allocation:

```cpp
void allocateOffspring(std::vector<Species>& species, int populationSize) {
    // Calculate total adjusted fitness
    float totalAdjustedFitness = 0;
    for (const auto& sp : species) {
        for (const auto* genome : sp.members) {
            totalAdjustedFitness += genome->adjustedFitness;
        }
    }

    // Allocate offspring proportionally
    int allocated = 0;
    for (auto& sp : species) {
        float speciesFitness = 0;
        for (const auto* genome : sp.members) {
            speciesFitness += genome->adjustedFitness;
        }

        sp.allowedOffspring = static_cast<int>(
            (speciesFitness / totalAdjustedFitness) * populationSize
        );
        allocated += sp.allowedOffspring;
    }

    // Handle rounding errors
    while (allocated < populationSize) {
        species[randomInt(0, species.size()-1)].allowedOffspring++;
        allocated++;
    }
}
```

---

### 8. Complete Evolution Loop

```cpp
class NEAT {
private:
    std::vector<Genome> population;
    std::vector<Species> species;
    InnovationTracker innovationTracker;
    Config config;

public:
    void evolve() {
        // Initialize population with minimal topology
        initializePopulation();

        for (int gen = 0; gen < config.maxGenerations; gen++) {
            // 1. Evaluate fitness of all genomes
            evaluateFitness();

            // 2. Check for solution
            if (getBestFitness() >= config.fitnessThreshold) {
                break;
            }

            // 3. Speciate population
            assignToSpecies(population, species, config.compatibilityThreshold);

            // 4. Calculate adjusted fitness
            calculateAdjustedFitness(species);

            // 5. Remove stagnant species
            removeStagnantSpecies();

            // 6. Allocate offspring per species
            allocateOffspring(species, config.populationSize);

            // 7. Reproduce within species
            std::vector<Genome> newPopulation;
            for (auto& sp : species) {
                // Sort by fitness
                sp.sortByFitness();

                // Elitism - keep best genome
                if (config.elitism > 0 && sp.members.size() > 0) {
                    newPopulation.push_back(*sp.members[0]);
                    sp.allowedOffspring--;
                }

                // Select parents and create offspring
                int survivors = sp.members.size() * config.survivalThreshold;
                survivors = std::max(survivors, 1);

                for (int i = 0; i < sp.allowedOffspring; i++) {
                    Genome* parent1 = sp.members[randomInt(0, survivors-1)];
                    Genome* parent2 = sp.members[randomInt(0, survivors-1)];

                    Genome offspring = crossover(*parent1, *parent2,
                                                  parent1->fitness, parent2->fitness);

                    // Apply mutations
                    mutateWeights(offspring, config.weightMutateRate,
                                  config.weightMutatePower, config.weightReplaceRate);

                    if (randomFloat() < config.addConnectionProb) {
                        mutateAddConnection(offspring, innovationTracker);
                    }
                    if (randomFloat() < config.addNodeProb) {
                        mutateAddNode(offspring, innovationTracker);
                    }

                    newPopulation.push_back(offspring);
                }
            }

            population = newPopulation;

            // 8. Reset innovation tracker for new generation
            innovationTracker.resetGeneration();
        }
    }
};
```

---

## Recommended Configuration Parameters

### Population Settings
| Parameter | Recommended Value | Description |
|-----------|-------------------|-------------|
| `pop_size` | 150 | Population size |
| `elitism` | 2 | Best genomes preserved unchanged |
| `survival_threshold` | 0.2 | Fraction allowed to reproduce |

### Mutation Rates
| Parameter | Recommended Value | Description |
|-----------|-------------------|-------------|
| `weight_mutate_rate` | 0.8 | Per-connection weight mutation |
| `weight_mutate_power` | 0.5 | Weight perturbation std dev |
| `weight_replace_rate` | 0.1 | Full weight replacement |
| `conn_add_prob` | 0.5 | Add connection mutation |
| `node_add_prob` | 0.2 | Add node mutation |
| `enabled_mutate_rate` | 0.01 | Toggle connection enabled |

### Compatibility Distance
| Parameter | Recommended Value | Description |
|-----------|-------------------|-------------|
| `c1` (excess) | 1.0 | Excess gene coefficient |
| `c2` (disjoint) | 1.0 | Disjoint gene coefficient |
| `c3` (weight) | 0.4 | Weight difference coefficient |
| `compatibility_threshold` | 3.0 | Species membership threshold |

### Stagnation
| Parameter | Recommended Value | Description |
|-----------|-------------------|-------------|
| `max_stagnation` | 15 | Generations before species removal |
| `species_elitism` | 2 | Top species protected from stagnation |

---

## Open Source Implementations

### C++ Implementations
1. **Original NEAT C++** (Ken Stanley): https://nn.cs.utexas.edu/?neat-c
2. **EvolutionNet**: https://github.com/BiagioFesta/EvolutionNet
3. **TinyAI**: https://github.com/hav4ik/tinyai
4. **abadiet/NEAT**: https://github.com/abadiet/NEAT
5. **HyperNEAT**: https://github.com/MisterTea/HyperNEAT

### Other Languages
- **NEAT-Python**: https://neat-python.readthedocs.io/
- **SharpNEAT** (C#): https://github.com/colgreen/sharpneat
- **goNEAT** (Go): https://github.com/yaricom/goNEAT

---

## Best Practices for Implementation

### 1. Start Simple
- Begin with minimal topology (no hidden nodes)
- Use direct input-to-output connections initially
- Let complexity emerge through evolution

### 2. Innovation Number Management
- Use a global counter that persists across generations
- Track same-generation mutations to assign identical numbers
- Never modify innovation numbers once assigned

### 3. Mutation Balance
- Weight mutations should be much more frequent than structural
- Too many structural mutations = bloated networks
- Too few = slow adaptation

### 4. Speciation Tuning
- If too few species: lower compatibility threshold
- If too many species: raise compatibility threshold
- Monitor species diversity over generations

### 5. Performance Optimization
- Cache compatibility distances (they're expensive)
- Use spatial data structures for genome comparisons
- Parallelize fitness evaluation

### 6. Common Pitfalls
- Forgetting to disable the original connection when adding a node
- Not handling recurrent connections properly in feed-forward mode
- Innovation number collisions across generations
- Not normalizing compatibility distance for genome size

---

## References

1. Stanley, K.O. and Miikkulainen, R. (2002). "Evolving Neural Networks through Augmenting Topologies". Evolutionary Computation 10(2):99-127.
2. NEAT Users Page: https://www.cs.ucf.edu/~kstanley/neat.html
3. NEAT-Python Documentation: https://neat-python.readthedocs.io/
4. SharpNEAT Research: https://sharpneat.sourceforge.io/research/
