# Neural Networks for Creature Brains: Research & Implementation

## Executive Summary

This document surveys modern neuroevolution techniques and documents the implementation of an advanced neural network system for the OrganismEvolution simulation. The goal is to replace hardcoded steering behaviors with evolved neural networks capable of learning and adapting both across generations (evolution) and within a single lifetime (plasticity).

---

## Part 1: Survey of Neuroevolution Techniques

### 1.1 NEAT (NeuroEvolution of Augmenting Topologies)

**Core Concept**: NEAT evolves both the weights AND topology of neural networks, starting from minimal structures and complexifying over time.

**Key Innovations**:

1. **Historical Markings (Innovation Numbers)**
   - Each gene (connection) receives a unique ID when created
   - Enables meaningful crossover between different topologies
   - Solves the "competing conventions" problem

2. **Speciation**
   - Networks are grouped into species based on topological similarity
   - Fitness sharing within species prevents premature convergence
   - Protects novel innovations from competition with optimized solutions

3. **Complexification**
   - Networks start minimal (inputs directly connected to outputs)
   - Add nodes by splitting existing connections
   - Add connections between previously unconnected nodes
   - Natural selection prunes unnecessary complexity

**Mutation Operators**:
```
- Add Connection: Connect two previously unconnected nodes
- Add Node: Split existing connection, insert node in middle
- Mutate Weight: Perturb or replace connection weight
- Toggle Enable: Enable/disable a connection
```

**Crossover**:
- Align genes by innovation number
- Matching genes: randomly inherit from either parent
- Disjoint/excess genes: inherit from fitter parent

**Pros**: Discovers minimal architectures, handles competing conventions
**Cons**: Computationally expensive speciation, slow for large networks

### 1.2 HyperNEAT (Hypercube-based NEAT)

**Core Concept**: Uses NEAT to evolve a Compositional Pattern Producing Network (CPPN) that generates the weights of a larger substrate network.

**How It Works**:
1. Define a substrate geometry (e.g., 2D grid of neurons)
2. CPPN takes coordinates (x1,y1,x2,y2) as input
3. CPPN outputs the weight between neurons at those coordinates
4. Geometric regularities emerge naturally (symmetry, repetition)

**Advantages**:
- Scales to very large networks (millions of connections)
- Exploits geometric regularities in the problem
- Produces regular, interpretable weight patterns

**Disadvantages**:
- Requires careful substrate design
- Indirect encoding can be harder to evolve
- May miss fine-grained solutions

### 1.3 Evolution Strategies (ES)

**Core Concept**: Black-box optimization using populations of perturbed parameter vectors.

**OpenAI ES Algorithm**:
```
1. Sample n perturbations ε_i from N(0, I)
2. Evaluate fitness F(θ + σ*ε_i) for each perturbation
3. Update: θ ← θ + α * (1/nσ) * Σ F_i * ε_i
```

**Key Properties**:
- Highly parallelizable (embarrassingly parallel)
- No backpropagation needed
- Works with non-differentiable fitness functions
- Can handle sparse/delayed rewards

**CMA-ES (Covariance Matrix Adaptation)**:
- Maintains full covariance matrix of search distribution
- Adapts step sizes per dimension
- State-of-the-art for continuous optimization

**Advantages**:
- Scales to millions of parameters
- Simple to implement and parallelize
- Robust to noisy fitness evaluations

**Disadvantages**:
- Sample inefficient compared to gradient methods
- Struggles with high-dimensional problems without structure

### 1.4 Novelty Search

**Core Concept**: Reward behavioral novelty rather than fitness. Novel behaviors step-stones to distant fitness peaks.

**Algorithm**:
```
1. Define behavior characterization (e.g., final position, trajectory)
2. Maintain archive of novel behaviors
3. Novelty = average distance to k-nearest behaviors in archive
4. Select based on novelty, not fitness
```

**Behavioral Characterization Examples**:
- Final (x,y) position
- Trajectory through environment
- Sequence of actions taken
- Internal state patterns

**Novelty + Fitness Hybrid**:
```
score = α * novelty + (1-α) * fitness
```
Often outperforms pure novelty or pure fitness search.

**Advantages**:
- Escapes local optima and deceptive fitness landscapes
- Discovers diverse solution repertoire
- Enables open-ended evolution

**Disadvantages**:
- Behavior space must be carefully defined
- Archive growth can be expensive
- May explore irrelevant regions

### 1.5 Quality Diversity (QD) Algorithms

**MAP-Elites**:
- Discretize behavior space into grid cells
- Maintain highest-fitness solution per cell
- Generate offspring, place in appropriate cell if better

**NSGA-II for Neuroevolution**:
- Multi-objective optimization
- Pareto front of fitness vs. complexity vs. novelty
- Non-dominated sorting + crowding distance

---

## Part 2: Network Topologies Comparison

### 2.1 Feedforward Networks

**Architecture**:
```
Input → Hidden₁ → Hidden₂ → ... → Output
```

**Properties**:
- Deterministic input-output mapping
- No internal state between timesteps
- Easy to train and analyze
- Cannot handle temporal dependencies

**Use Cases**:
- Classification tasks
- Immediate sensory-motor mapping
- Reflex behaviors

### 2.2 Recurrent Neural Networks (RNNs)

**Architecture**:
```
       ┌──────────────────┐
       ↓                  │
Input → Hidden ──────→ Output
           │
           └──→ (next timestep)
```

**Variants**:

**Simple RNN**:
```
h_t = tanh(W_hh * h_{t-1} + W_xh * x_t + b)
y_t = W_hy * h_t
```

**LSTM (Long Short-Term Memory)**:
```
f_t = σ(W_f * [h_{t-1}, x_t] + b_f)     // forget gate
i_t = σ(W_i * [h_{t-1}, x_t] + b_i)     // input gate
c̃_t = tanh(W_c * [h_{t-1}, x_t] + b_c)  // candidate
c_t = f_t ⊙ c_{t-1} + i_t ⊙ c̃_t        // cell state
o_t = σ(W_o * [h_{t-1}, x_t] + b_o)     // output gate
h_t = o_t ⊙ tanh(c_t)                   // hidden state
```

**GRU (Gated Recurrent Unit)**:
```
z_t = σ(W_z * [h_{t-1}, x_t])     // update gate
r_t = σ(W_r * [h_{t-1}, x_t])     // reset gate
h̃_t = tanh(W * [r_t ⊙ h_{t-1}, x_t])
h_t = (1-z_t) ⊙ h_{t-1} + z_t ⊙ h̃_t
```

**Advantages**:
- Captures temporal dependencies
- Internal memory/state
- Can learn sequences and patterns

**Disadvantages**:
- Vanishing/exploding gradients (mitigated by LSTM/GRU)
- Harder to evolve (more parameters)
- Potentially unstable dynamics

### 2.3 Attention Mechanisms

**Self-Attention (Transformer-style)**:
```
Q = X * W_Q   // queries
K = X * W_K   // keys
V = X * W_V   // values
Attention = softmax(Q * K^T / √d_k) * V
```

**Properties**:
- Dynamic weighting of inputs
- Can attend to relevant features
- Parallelizable (unlike RNNs)
- Computationally expensive for long sequences

**Multi-Head Attention**:
- Multiple attention mechanisms in parallel
- Each head can learn different relationships
- Concatenated and projected

**Applications for Creatures**:
- Attend to relevant environmental features
- Focus on threatening predators vs. distant ones
- Dynamic priority of sensory inputs

### 2.4 Recommended Topology for This Project

**Hybrid Recurrent Network with Gating**:

```
Sensory Input (8-12 features)
       │
       ↓
┌─────────────────────────────────┐
│     SENSORY PROCESSOR           │
│  (feedforward, feature extract) │
└─────────────────────────────────┘
       │
       ↓
┌─────────────────────────────────┐
│     DECISION MAKER              │
│  (recurrent, with gates)        │ ←── Memory State
│  (modulated by neuromodulators) │
└─────────────────────────────────┘
       │
       ↓
┌─────────────────────────────────┐
│     MOTOR CONTROLLER            │
│  (feedforward, action output)   │
└─────────────────────────────────┘
       │
       ↓
Motor Output (angle, speed, actions)
```

---

## Part 3: Real-Time Learning During Lifetime

### 3.1 Hebbian Learning

**Principle**: "Neurons that fire together, wire together"

**Basic Hebbian Rule**:
```
Δw_ij = η * x_i * y_j
```
Where:
- η = learning rate
- x_i = presynaptic activity
- y_j = postsynaptic activity

**Problem**: Unbounded weight growth

**BCM Rule (Bienenstock-Cooper-Munro)**:
```
Δw_ij = η * x_i * y_j * (y_j - θ)
```
Where θ is a sliding threshold based on recent activity.

**Oja's Rule (Normalized Hebbian)**:
```
Δw_ij = η * y_j * (x_i - y_j * w_ij)
```
Self-normalizing, prevents unbounded growth.

### 3.2 STDP (Spike-Timing-Dependent Plasticity)

**Principle**: Precise timing between pre- and post-synaptic spikes determines weight change.

**STDP Window**:
```
If pre fires before post (Δt > 0): LTP (strengthen)
If post fires before pre (Δt < 0): LTD (weaken)

Δw = A+ * exp(-Δt/τ+)  if Δt > 0
Δw = -A- * exp(Δt/τ-)  if Δt < 0
```

**Parameters**:
- A+, A- = amplitude of potentiation/depression
- τ+, τ- = time constants (typically 10-20ms)

**Simplified STDP for Simulation**:
Since we don't have spikes, use rate-based approximation:
```
Δw_ij = η * (x_i^{t} * y_j^{t} - x_i^{t-1} * y_j^{t})
```

### 3.3 Modulated Hebbian Learning

**Concept**: Weight changes only occur in presence of neuromodulatory signal.

**Three-Factor Rule**:
```
Δw_ij = η * x_i * y_j * m
```
Where m is a modulatory signal (e.g., dopamine-like reward).

**Eligibility Traces**:
```
e_ij = γ * e_ij + x_i * y_j           // accumulate correlations
Δw_ij = η * e_ij * m                   // apply when reward arrives
```

This enables credit assignment over time - correlations are remembered until reward signal indicates their value.

### 3.4 Metaplasticity

**Concept**: The plasticity rules themselves can be evolved.

**Evolved Plasticity Parameters**:
- Learning rate η
- Eligibility trace decay γ
- Modulatory sensitivity
- Per-connection plasticity type

**Differentiable Plasticity (Miconi et al. 2018)**:
```
y = tanh(W * x + α ⊙ H * x)
H ← H + η * (y * x^T)
```
Where:
- W = fixed weights (evolved)
- H = plastic weights (learned during lifetime)
- α = per-connection plasticity coefficients (evolved)

---

## Part 4: Modular Brain Architecture

### 4.1 Biological Inspiration

**Vertebrate Brain Regions**:
- **Brainstem**: Basic reflexes, autonomic functions
- **Cerebellum**: Motor coordination, timing
- **Limbic System**: Emotion, motivation, memory
- **Cortex**: Higher cognition, planning

**Key Principles**:
- Functional specialization
- Hierarchical organization
- Parallel processing pathways
- Feedback loops between regions

### 4.2 Proposed Module Architecture

```
┌────────────────────────────────────────────────────────────┐
│                       CREATURE BRAIN                        │
├────────────────────────────────────────────────────────────┤
│                                                            │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐ │
│  │   SENSORY    │    │   MEMORY     │    │  EMOTIONAL   │ │
│  │  PROCESSOR   │───>│   MODULE     │<───│   MODULE     │ │
│  │              │    │  (Working +  │    │ (Fear, Need, │ │
│  │ • Vision     │    │   Episodic)  │    │  Curiosity)  │ │
│  │ • Proximity  │    │              │    │              │ │
│  │ • Internal   │    └──────────────┘    └──────────────┘ │
│  └──────────────┘            │                  │         │
│          │                   ↓                  ↓         │
│          │          ┌──────────────────────────────────┐  │
│          └─────────>│        DECISION MAKER            │  │
│                     │    (Central Pattern Generator)   │  │
│                     │    • Action selection            │  │
│                     │    • Priority weighting          │  │
│                     │    • Planning/prediction         │  │
│                     └──────────────────────────────────┘  │
│                                    │                       │
│                                    ↓                       │
│                     ┌──────────────────────────────────┐  │
│                     │       MOTOR CONTROLLER           │  │
│                     │   • Angle output                 │  │
│                     │   • Speed modulation             │  │
│                     │   • Action execution             │  │
│                     └──────────────────────────────────┘  │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

### 4.3 Module Specifications

**SensoryProcessor**:
- **Inputs**: Raw sensory data (distances, angles, internal state)
- **Outputs**: Processed features (threat level, food attractiveness, etc.)
- **Architecture**: Feedforward, learned feature extraction
- **Size**: 12 inputs → 8 hidden → 6 outputs

**MemoryModule**:
- **Inputs**: Current processed sensory state
- **Outputs**: Memory-enhanced representation
- **Architecture**: Recurrent with gating (simplified LSTM)
- **Function**: Working memory, recent experience buffer
- **Size**: 6 inputs + 4 hidden state → 4 outputs

**EmotionalModule**:
- **Inputs**: Processed sensory state, internal needs
- **Outputs**: Emotional/motivational signals
- **Components**:
  - Fear level (0-1)
  - Hunger drive (0-1)
  - Curiosity/exploration drive (0-1)
  - Social affiliation (0-1)
- **Architecture**: Feedforward with learned drive curves
- **Size**: 6 inputs → 4 outputs

**DecisionMaker**:
- **Inputs**: Sensory features + Memory + Emotional state
- **Outputs**: Action probabilities or values
- **Architecture**: Recurrent, receives neuromodulation
- **Function**: Central executive, action selection
- **Size**: 14 inputs → 8 hidden (recurrent) → 4 outputs

**MotorController**:
- **Inputs**: Decision outputs
- **Outputs**: Actual motor commands (angle, speed)
- **Architecture**: Feedforward, possibly evolved pattern generators
- **Size**: 4 inputs → 4 hidden → 2 outputs

### 4.4 Inter-Module Communication

**Information Flow**:
```
Sensory → SensoryProcessor → [features]
                              ↓
[features] → MemoryModule → [memory-enhanced]
                              ↓
[features] → EmotionalModule → [drives]
                              ↓
[memory-enhanced, drives] → DecisionMaker → [action values]
                              ↓
[action values] → MotorController → [angle, speed]
```

**Neuromodulation Targets**:
- DecisionMaker plasticity (learning from rewards)
- Emotional module thresholds
- Memory retention/forgetting

---

## Part 5: Neuromodulation System

### 5.1 Biological Neuromodulators

**Dopamine**:
- Reward prediction error signaling
- Motivation and reinforcement learning
- High = reward better than expected
- Low = punishment or disappointment

**Norepinephrine**:
- Arousal and alertness
- Attention and vigilance
- Released during stress/threat

**Serotonin**:
- Mood and patience
- Inhibition and behavioral regulation
- Long-term value estimation

**Acetylcholine**:
- Attention and memory encoding
- Learning rate modulation
- High during novel situations

### 5.2 Simplified Neuromodulator Model

For this simulation, implement two primary modulators:

**Reward Signal (Dopamine-like)**:
```cpp
class RewardModulator {
    float baseline = 0.0f;
    float currentLevel = 0.0f;
    float decayRate = 0.9f;

    void update(float reward) {
        float prediction_error = reward - baseline;
        currentLevel = prediction_error;
        baseline = decayRate * baseline + (1-decayRate) * reward;
    }
};
```

**Triggers**:
- Eating food: +1.0 reward
- Getting attacked: -1.0 reward
- Successful hunt: +2.0 reward
- Energy critically low: -0.5 reward

**Arousal Signal (Norepinephrine-like)**:
```cpp
class ArousalModulator {
    float level = 0.5f;  // baseline arousal
    float targetLevel = 0.5f;
    float adaptRate = 0.1f;

    void update(float threat_level, float novelty) {
        targetLevel = 0.3f + 0.4f * threat_level + 0.3f * novelty;
        level += adaptRate * (targetLevel - level);
    }
};
```

**Effects**:
- High arousal → increased learning rate, faster decisions
- Low arousal → exploration, slow decisions
- Extreme arousal → reflex behaviors dominate

### 5.3 Modulated Plasticity Implementation

```cpp
void updateWeights(float reward, float arousal) {
    for (auto& synapse : synapses) {
        if (synapse.plastic) {
            // Hebbian correlation
            float correlation = synapse.preActivity * synapse.postActivity;

            // Update eligibility trace
            synapse.eligibility = traceDecay * synapse.eligibility + correlation;

            // Modulated weight update
            float deltaW = learningRate * synapse.eligibility * reward * arousal;
            synapse.weight = clamp(synapse.weight + deltaW, -maxWeight, maxWeight);
        }
    }
}
```

---

## Part 6: NEAT Implementation Details

### 6.1 Gene Representation

```cpp
struct ConnectionGene {
    int innovation;     // Historical marker
    int fromNode;       // Source node ID
    int toNode;         // Target node ID
    float weight;       // Connection weight
    bool enabled;       // Is connection active?
    bool plastic;       // Can learn during lifetime?
    float plasticityCoef; // Learning rate coefficient
};

struct NodeGene {
    int id;
    NodeType type;      // INPUT, HIDDEN, OUTPUT, BIAS
    ActivationType activation; // TANH, RELU, SIGMOID, etc.
    float bias;
    int layer;          // For feedforward ordering
};
```

### 6.2 Mutation Operations

**Add Connection**:
```cpp
void mutateAddConnection(Genome& g) {
    // Find two unconnected nodes where connection is valid
    Node* from = selectRandomNode(g, {INPUT, HIDDEN, BIAS});
    Node* to = selectRandomNode(g, {HIDDEN, OUTPUT});

    if (!connectionExists(g, from, to) && !wouldCreateCycle(g, from, to)) {
        int innov = getInnovationNumber(from->id, to->id);
        g.connections.push_back({innov, from->id, to->id, randomWeight(), true});
    }
}
```

**Add Node**:
```cpp
void mutateAddNode(Genome& g) {
    // Select random enabled connection
    ConnectionGene& conn = selectRandomEnabled(g.connections);

    // Disable old connection
    conn.enabled = false;

    // Create new node
    int newNodeId = g.nextNodeId++;
    g.nodes.push_back({newNodeId, HIDDEN, TANH, 0.0f});

    // Create two new connections
    // Old connection: A → B becomes A → New → B
    int innov1 = getInnovationNumber(conn.fromNode, newNodeId);
    int innov2 = getInnovationNumber(newNodeId, conn.toNode);

    g.connections.push_back({innov1, conn.fromNode, newNodeId, 1.0f, true});
    g.connections.push_back({innov2, newNodeId, conn.toNode, conn.weight, true});
}
```

### 6.3 Speciation

**Compatibility Distance**:
```cpp
float compatibility(Genome& g1, Genome& g2) {
    int excess = countExcess(g1, g2);
    int disjoint = countDisjoint(g1, g2);
    float weightDiff = averageWeightDifference(g1, g2);

    int N = max(g1.connections.size(), g2.connections.size());
    if (N < 20) N = 1;  // Small genome normalization

    return (c1 * excess / N) + (c2 * disjoint / N) + (c3 * weightDiff);
}
```

**Species Assignment**:
```cpp
void assignSpecies(Population& pop) {
    for (auto& genome : pop.genomes) {
        bool found = false;
        for (auto& species : pop.species) {
            if (compatibility(genome, species.representative) < threshold) {
                species.members.push_back(&genome);
                found = true;
                break;
            }
        }
        if (!found) {
            // Create new species with this genome as representative
            pop.species.push_back(Species{&genome, {&genome}});
        }
    }
}
```

### 6.4 Fitness Sharing

```cpp
void adjustFitness(Population& pop) {
    for (auto& species : pop.species) {
        int speciesSize = species.members.size();
        for (auto* genome : species.members) {
            genome->adjustedFitness = genome->fitness / speciesSize;
        }
    }
}
```

---

## Part 7: Implementation Plan

### Phase 1: Core Neural Network (src/ai/NeuralNetwork.h/cpp)
- Flexible node/connection representation
- Support for recurrent connections
- Multiple activation functions
- Forward pass with proper ordering

### Phase 2: NEAT Evolution (src/ai/NEATGenome.h/cpp)
- Gene representation with innovation numbers
- Mutation operators
- Crossover with gene alignment
- Speciation system

### Phase 3: Brain Modules (src/ai/BrainModules.h/cpp)
- SensoryProcessor implementation
- MemoryModule with recurrence
- EmotionalModule for drives
- DecisionMaker with modulation
- MotorController

### Phase 4: Plasticity System (src/ai/Plasticity.h/cpp)
- Hebbian learning rules
- Eligibility traces
- Neuromodulation integration

### Phase 5: Integration
- Connect brain to creature sensory inputs
- Replace steering behaviors with NN outputs
- Integrate with existing evolution system

### Phase 6: Testing & Validation
- Unit tests for NN operations
- Evolution experiments
- Learning verification

---

## References

1. Stanley, K.O. & Miikkulainen, R. (2002). Evolving Neural Networks through Augmenting Topologies. Evolutionary Computation, 10(2), 99-127.

2. Stanley, K.O., D'Ambrosio, D.B., & Gauci, J. (2009). A Hypercube-Based Encoding for Evolving Large-Scale Neural Networks. Artificial Life, 15(2), 185-212.

3. Salimans, T., et al. (2017). Evolution Strategies as a Scalable Alternative to Reinforcement Learning. arXiv:1703.03864.

4. Lehman, J. & Stanley, K.O. (2011). Abandoning Objectives: Evolution Through the Search for Novelty Alone. Evolutionary Computation, 19(2), 189-223.

5. Miconi, T., et al. (2018). Differentiable Plasticity: Training Plastic Neural Networks with Backpropagation. ICML 2018.

6. Soltoggio, A., Stanley, K.O., & Risi, S. (2018). Born to Learn: the Inspiration, Progress, and Future of Evolved Plastic Artificial Neural Networks. Neural Networks, 108, 48-67.

7. Mouret, J.B. & Clune, J. (2015). Illuminating search spaces by mapping elites. arXiv:1504.04909.
