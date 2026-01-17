# OrganismEvolution - Comprehensive Documentation

## A 3D Genetic Evolution Simulator

**Version**: 1.0.0
**Language**: C++17
**Platform**: Windows (MSYS2/MinGW-w64)
**Graphics**: OpenGL 3.3+

---

## Table of Contents

1. [Overview](#1-overview)
   - [1.1 Project Description](#11-project-description)
   - [1.2 Key Features](#12-key-features)
   - [1.3 Technical Specifications](#13-technical-specifications)

2. [Quick Start](#2-quick-start)
   - [2.1 Prerequisites](#21-prerequisites)
   - [2.2 Building the Project](#22-building-the-project)
   - [2.3 Running the Simulation](#23-running-the-simulation)
   - [2.4 Controls](#24-controls)

3. [Architecture Overview](#3-architecture-overview)
   - [3.1 System Diagram](#31-system-diagram)
   - [3.2 Directory Structure](#32-directory-structure)
   - [3.3 Core Components](#33-core-components)
   - [3.4 Data Flow](#34-data-flow)

4. [Core Systems](#4-core-systems)
   - [4.1 Simulation Engine](#41-simulation-engine)
   - [4.2 Entity System](#42-entity-system)
   - [4.3 Update Loop](#43-update-loop)

5. [Genetics System](#5-genetics-system)
   - [5.1 Diploid Genome](#51-diploid-genome)
   - [5.2 Chromosomes and Genes](#52-chromosomes-and-genes)
   - [5.3 Alleles and Dominance](#53-alleles-and-dominance)
   - [5.4 Mutation Types](#54-mutation-types)
   - [5.5 Sexual Reproduction](#55-sexual-reproduction)
   - [5.6 Speciation](#56-speciation)
   - [5.7 Mate Selection](#57-mate-selection)
   - [5.8 Hybrid Zones](#58-hybrid-zones)

6. [Neural Network & AI](#6-neural-network--ai)
   - [6.1 NEAT Implementation](#61-neat-implementation)
   - [6.2 Modular Brain Architecture](#62-modular-brain-architecture)
   - [6.3 Neuromodulation System](#63-neuromodulation-system)
   - [6.4 Sensory Processing](#64-sensory-processing)
   - [6.5 Steering Behaviors](#65-steering-behaviors)
   - [6.6 Brain Evolution](#66-brain-evolution)

7. [Ecosystem & Environment](#7-ecosystem--environment)
   - [7.1 Terrain Generation](#71-terrain-generation)
   - [7.2 Vegetation System](#72-vegetation-system)
   - [7.3 L-System Trees](#73-l-system-trees)
   - [7.4 Seasonal Cycles](#74-seasonal-cycles)
   - [7.5 Producer System](#75-producer-system)
   - [7.6 Decomposer System](#76-decomposer-system)
   - [7.7 Trophic Dynamics](#77-trophic-dynamics)

8. [Graphics & Rendering](#8-graphics--rendering)
   - [8.1 Rendering Pipeline](#81-rendering-pipeline)
   - [8.2 Procedural Generation](#82-procedural-generation)
   - [8.3 Metaball System](#83-metaball-system)
   - [8.4 Marching Cubes](#84-marching-cubes)
   - [8.5 Creature Building](#85-creature-building)
   - [8.6 Instanced Rendering](#86-instanced-rendering)
   - [8.7 Shader System](#87-shader-system)

9. [Physics & Morphology](#9-physics--morphology)
   - [9.1 Locomotion System](#91-locomotion-system)
   - [9.2 Body Plans](#92-body-plans)
   - [9.3 Metamorphosis](#93-metamorphosis)
   - [9.4 Fitness Landscape](#94-fitness-landscape)
   - [9.5 Visual Indicators](#95-visual-indicators)

10. [Configuration & Parameters](#10-configuration--parameters)
    - [10.1 Simulation Parameters](#101-simulation-parameters)
    - [10.2 Evolution Parameters](#102-evolution-parameters)
    - [10.3 Environment Parameters](#103-environment-parameters)
    - [10.4 Performance Tuning](#104-performance-tuning)

11. [API Reference](#11-api-reference)
    - [11.1 Core Classes](#111-core-classes)
    - [11.2 Entity Classes](#112-entity-classes)
    - [11.3 Graphics Classes](#113-graphics-classes)
    - [11.4 Utility Classes](#114-utility-classes)

12. [Appendices](#12-appendices)
    - [12.1 Gene Type Reference](#121-gene-type-reference)
    - [12.2 Creature Type Reference](#122-creature-type-reference)
    - [12.3 Shader Reference](#123-shader-reference)
    - [12.4 Troubleshooting](#124-troubleshooting)

---

## 1. Overview

### 1.1 Project Description

**OrganismEvolution** is a sophisticated 3D evolution simulator that models biological evolution, natural selection, and ecosystem dynamics in a procedurally generated environment. Creatures with randomly generated genetic traits compete for survival, and successful traits are passed to future generations through realistic genetic mechanisms including diploid inheritance, chromosomal crossover, and speciation.

The simulation demonstrates:
- **Darwinian natural selection** through differential survival and reproduction
- **Genetic inheritance** with diploid chromosomes, allelic dominance, and crossover
- **Speciation** through genetic drift and reproductive isolation
- **Ecosystem dynamics** including predator-prey relationships and trophic pyramids
- **Neural evolution** using NEAT (NeuroEvolution of Augmenting Topologies)

### 1.2 Key Features

| Category | Features |
|----------|----------|
| **Genetics** | Diploid genomes, 26 gene types, 4 chromosomes, sexual reproduction, speciation tracking |
| **Neural Networks** | NEAT evolution, modular brains, neuromodulation, plasticity, 4 activation functions |
| **Ecosystem** | 12 creature types, 4 seasons, trophic pyramids, nutrient cycling, carrying capacity |
| **Graphics** | Procedural textures, metaball creatures, L-system trees, instanced rendering |
| **Physics** | 10 locomotion gaits, inverse kinematics, CPG coordination, metamorphosis |
| **Sensory** | Vision, hearing, smell, touch, spatial memory, pheromone communication |

### 1.3 Technical Specifications

```
Language:        C++17
Build System:    CMake 3.15+
Graphics API:    OpenGL 3.3+
Dependencies:    GLFW3, GLEW, GLM
Platform:        Windows (MSYS2/MinGW-w64)
Lines of Code:   17,600+ (source) + 5,000+ (documentation)
```

**Performance Targets:**
- 60+ FPS with 150 creatures
- 300×300 terrain grid (90,000 vertices)
- Real-time procedural mesh generation
- Instanced rendering for efficiency

---

## 2. Quick Start

### 2.1 Prerequisites

**Required Software:**
- MSYS2 (https://www.msys2.org/)
- MinGW-w64 toolchain
- CMake 3.15+
- Git

**Install dependencies in MSYS2 MINGW64 terminal:**
```bash
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-glfw
pacman -S mingw-w64-x86_64-glew
pacman -S mingw-w64-x86_64-glm
```

### 2.2 Building the Project

```bash
# Clone the repository
git clone https://github.com/yourusername/OrganismEvolution.git
cd OrganismEvolution

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -G "MinGW Makefiles"

# Build
cmake --build . -j4
```

### 2.3 Running the Simulation

```bash
./OrganismEvolution.exe
```

### 2.4 Controls

| Key | Action |
|-----|--------|
| `W/A/S/D` | Move camera forward/left/backward/right |
| `SPACE` | Move camera up |
| `CTRL` | Move camera down |
| `Mouse` | Look around |
| `Scroll` | Zoom in/out |
| `P` | Pause/Resume simulation |
| `+/-` | Increase/Decrease simulation speed |
| `ESC` | Exit |

---

## 3. Architecture Overview

### 3.1 System Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                         MAIN LOOP                                │
│  ┌──────────────┐   ┌──────────────┐   ┌──────────────┐        │
│  │    INPUT     │ → │    UPDATE    │ → │    RENDER    │        │
│  └──────────────┘   └──────────────┘   └──────────────┘        │
└─────────────────────────────────────────────────────────────────┘
                              │
         ┌────────────────────┼────────────────────┐
         ▼                    ▼                    ▼
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│   SIMULATION    │  │   ECOSYSTEM     │  │   GRAPHICS      │
│                 │  │                 │  │                 │
│ - Creatures     │  │ - Terrain       │  │ - Shaders       │
│ - Spatial Grid  │  │ - Vegetation    │  │ - Meshes        │
│ - Food          │  │ - Seasons       │  │ - Instancing    │
│ - Population    │  │ - Nutrients     │  │ - Camera        │
└────────┬────────┘  └────────┬────────┘  └─────────────────┘
         │                    │
         ▼                    ▼
┌─────────────────┐  ┌─────────────────┐
│   CREATURE      │  │   ENVIRONMENT   │
│                 │  │                 │
│ - Genome        │  │ - Producer      │
│ - Brain         │  │ - Decomposer    │
│ - Sensory       │  │ - Season Mgr    │
│ - Steering      │  │ - Metrics       │
└────────┬────────┘  └─────────────────┘
         │
    ┌────┴────┐
    ▼         ▼
┌────────┐ ┌────────┐
│GENETICS│ │   AI   │
│        │ │        │
│-Diploid│ │- NEAT  │
│-Species│ │- Brain │
│-Hybrid │ │- Neuro │
└────────┘ └────────┘
```

### 3.2 Directory Structure

```
OrganismEvolution/
├── src/                           # Source code
│   ├── main.cpp                   # Entry point
│   ├── core/                      # Simulation engine
│   │   └── Simulation.h/cpp       # Main coordinator
│   ├── entities/                  # Creature system
│   │   ├── Creature.h/cpp         # Individual organism
│   │   ├── Genome.h/cpp           # Simple genome
│   │   ├── CreatureType.h         # Type enumeration
│   │   ├── SensorySystem.h/cpp    # Multi-modal sensing
│   │   ├── SteeringBehaviors.h/cpp# Movement AI
│   │   └── genetics/              # Advanced genetics
│   │       ├── Allele.h/cpp       # Allele variants
│   │       ├── Gene.h/cpp         # Diploid genes
│   │       ├── Chromosome.h/cpp   # Gene organization
│   │       ├── DiploidGenome.h/cpp# Complete genome
│   │       ├── Species.h/cpp      # Species tracking
│   │       ├── MateSelector.h/cpp # Sexual selection
│   │       ├── HybridZone.h/cpp   # Hybrid dynamics
│   │       └── GeneticsManager.h/cpp # Integration
│   ├── environment/               # World systems
│   │   ├── Terrain.h/cpp          # Height map
│   │   ├── Food.h/cpp             # Food entities
│   │   ├── VegetationManager.h/cpp# Plant placement
│   │   ├── TreeGenerator.h/cpp    # L-system trees
│   │   ├── LSystem.h/cpp          # Grammar engine
│   │   ├── ProducerSystem.h/cpp   # Plant production
│   │   ├── DecomposerSystem.h/cpp # Nutrient cycling
│   │   ├── SeasonManager.h/cpp    # Seasonal cycles
│   │   ├── EcosystemManager.h/cpp # Central coordinator
│   │   └── EcosystemMetrics.h/cpp # Health tracking
│   ├── graphics/                  # Rendering
│   │   ├── Camera.h/cpp           # View control
│   │   ├── Shader.h/cpp           # Shader management
│   │   ├── procedural/            # Mesh generation
│   │   │   ├── MetaballSystem.h   # Implicit surfaces
│   │   │   ├── MarchingCubes.h/cpp# Isosurface extraction
│   │   │   ├── CreatureBuilder.h/cpp # Body construction
│   │   │   └── MorphologyBuilder.h# Body plans
│   │   ├── rendering/             # Optimization
│   │   │   ├── CreatureMeshCache.h/cpp # Mesh reuse
│   │   │   └── CreatureRenderer.h/cpp  # Instancing
│   │   └── mesh/
│   │       └── MeshData.h         # Vertex structures
│   ├── physics/                   # Movement & morphology
│   │   ├── Locomotion.h/cpp       # Gait physics
│   │   ├── Morphology.h/cpp       # Body structure
│   │   ├── Metamorphosis.h/cpp    # Life stages
│   │   ├── FitnessLandscape.h/cpp # Fitness calculation
│   │   └── VisualIndicators.h/cpp # State visualization
│   ├── ai/                        # Neural systems
│   │   ├── NeuralNetwork.h/cpp    # Network implementation
│   │   ├── NEATGenome.h/cpp       # Topology evolution
│   │   ├── CreatureBrainInterface.h/cpp # Integration
│   │   └── BrainModules.h/cpp     # Modular architecture
│   └── utils/                     # Utilities
│       ├── Random.h/cpp           # RNG
│       ├── PerlinNoise.h/cpp      # Noise generation
│       ├── SpatialGrid.h/cpp      # Spatial queries
│       ├── CommandProcessor.h/cpp # Console commands
│       └── Screenshot.h/cpp       # Image capture
├── shaders/                       # GLSL shaders
│   ├── vertex.glsl                # Terrain vertex
│   ├── fragment.glsl              # Terrain fragment
│   ├── creature_vertex.glsl       # Creature vertex
│   ├── creature_fragment.glsl     # Creature fragment
│   ├── tree_vertex.glsl           # Tree vertex
│   ├── tree_fragment.glsl         # Tree fragment
│   ├── nametag_vertex.glsl        # UI vertex
│   └── nametag_fragment.glsl      # UI fragment
├── docs/                          # Documentation
├── external/                      # Third-party code
├── CMakeLists.txt                 # Build configuration
└── README.md                      # Project overview
```

### 3.3 Core Components

| Component | File | Purpose |
|-----------|------|---------|
| **Simulation** | `core/Simulation.h/cpp` | Main update loop, population management |
| **Creature** | `entities/Creature.h/cpp` | Individual organism with behavior |
| **DiploidGenome** | `genetics/DiploidGenome.h/cpp` | Genetic inheritance system |
| **NeuralNetwork** | `ai/NeuralNetwork.h/cpp` | Brain implementation |
| **Terrain** | `environment/Terrain.h/cpp` | World geometry |
| **EcosystemManager** | `environment/EcosystemManager.h/cpp` | Ecosystem coordinator |
| **CreatureRenderer** | `rendering/CreatureRenderer.h/cpp` | Instanced drawing |

### 3.4 Data Flow

```
CREATURE UPDATE CYCLE:
┌─────────────────────────────────────────────────────────────┐
│ 1. SENSE                                                     │
│    SensorySystem → detect food, predators, prey              │
│    ↓                                                         │
│ 2. THINK                                                     │
│    NeuralNetwork → process inputs → decide actions           │
│    ↓                                                         │
│ 3. ACT                                                       │
│    SteeringBehaviors → calculate movement forces             │
│    ↓                                                         │
│ 4. PHYSICS                                                   │
│    Position update, collision, terrain interaction           │
│    ↓                                                         │
│ 5. ENERGY                                                    │
│    Consume energy, eat food, check death                     │
│    ↓                                                         │
│ 6. REPRODUCE (if conditions met)                             │
│    MateSelector → DiploidGenome → new Creature               │
└─────────────────────────────────────────────────────────────┘
```

---

## 4. Core Systems

### 4.1 Simulation Engine

The `Simulation` class (`src/core/Simulation.h/cpp`) is the central coordinator that manages:

- **Creature population**: Vector of `Creature*` objects
- **Food system**: Spawning and consumption
- **Spatial optimization**: `SpatialGrid` for O(1) neighbor queries
- **Environment**: Terrain and vegetation managers
- **Rendering**: Mesh cache and instanced renderer

**Key Methods:**

```cpp
class Simulation {
public:
    void init();                    // Initialize all systems
    void update(float deltaTime);   // Update simulation state
    void render(const Camera& camera); // Draw everything

    void togglePause();             // Pause/resume
    void increaseSpeed();           // Speed up time
    void decreaseSpeed();           // Slow down time

    // Population management
    void spawnCreature(CreatureType type, glm::vec3 position);
    void removeCreature(Creature* creature);

    // Statistics
    int getHerbivoreCount() const;
    int getCarnivoreCount() const;
    int getGeneration() const;
};
```

### 4.2 Entity System

**Creature** (`src/entities/Creature.h/cpp`) represents an individual organism:

```cpp
class Creature {
public:
    // Identity
    CreatureType type;
    std::string name;
    int generation;

    // Physical state
    glm::vec3 position;
    glm::vec3 velocity;
    float energy;
    bool isAlive;

    // Genetics
    Genome genome;                    // Simple genome
    DiploidGenome* diploidGenome;     // Advanced genome

    // AI
    NeuralNetwork* brain;
    SensorySystem sensory;
    SteeringBehaviors steering;

    // Methods
    void update(float deltaTime, ...);
    void render(Shader& shader);
    Creature* reproduce(Creature* mate);
    void takeDamage(float amount);
    void consumeFood(float energy);
};
```

**CreatureType** (`src/entities/CreatureType.h`) enumerates ecological roles:

| Type | Role | Diet |
|------|------|------|
| `HERBIVORE` | Generic plant eater | Plants |
| `CARNIVORE` | Generic predator | Herbivores |
| `GRAZER` | Grass specialist | Grass |
| `BROWSER` | Tree leaf eater | Leaves |
| `FRUGIVORE` | Fruit specialist | Fruit |
| `SMALL_PREDATOR` | Hunts herbivores | Small prey |
| `OMNIVORE` | Mixed diet | Plants + prey |
| `APEX_PREDATOR` | Top predator | Other predators |
| `SCAVENGER` | Carrion eater | Corpses |
| `PARASITE` | Host feeder | Host energy |
| `CLEANER` | Symbiotic cleaner | Parasites |

### 4.3 Update Loop

```cpp
void Simulation::update(float deltaTime) {
    if (paused) return;

    deltaTime *= timeScale;  // Apply speed multiplier

    // 1. Rebuild spatial grid
    spatialGrid->clear();
    for (Creature* c : creatures) {
        spatialGrid->insert(c);
    }

    // 2. Update ecosystem
    ecosystem->update(deltaTime);

    // 3. Update each creature
    for (Creature* c : creatures) {
        // Sense environment
        auto nearbyFood = spatialGrid->queryFood(c->position, c->genome.visionRange);
        auto nearbyCreatures = spatialGrid->queryCreatures(c->position, c->genome.visionRange);

        // Think and act
        c->update(deltaTime, nearbyFood, nearbyCreatures, terrain);

        // Check reproduction
        if (c->canReproduce()) {
            Creature* mate = findMate(c);
            if (mate) {
                Creature* offspring = c->reproduce(mate);
                newCreatures.push_back(offspring);
            }
        }

        // Check death
        if (c->energy <= 0) {
            deadCreatures.push_back(c);
        }
    }

    // 4. Add new creatures
    for (Creature* c : newCreatures) {
        creatures.push_back(c);
    }

    // 5. Remove dead creatures
    for (Creature* c : deadCreatures) {
        ecosystem->onCreatureDeath(c);
        removeCreature(c);
    }

    // 6. Spawn food
    foodSpawnTimer += deltaTime;
    if (foodSpawnTimer > foodSpawnInterval) {
        spawnFood();
        foodSpawnTimer = 0;
    }
}
```

---

## 5. Genetics System

### 5.1 Diploid Genome

The `DiploidGenome` class (`src/entities/genetics/DiploidGenome.h/cpp`) implements realistic genetic inheritance:

```cpp
class DiploidGenome {
public:
    // Structure
    std::vector<ChromosomePair> chromosomePairs;  // 4 pairs

    // Identity
    int speciesId;
    bool isHybrid;
    std::vector<int> ancestry;

    // Fitness tracking
    float geneticLoad;
    float inbreedingCoeff;
    float heterozygosity;

    // Methods
    DiploidGenome();                              // Random genome
    DiploidGenome(const DiploidGenome& parent1,  // Sexual reproduction
                  const DiploidGenome& parent2);

    void mutate();
    float getTrait(GeneType type) const;
    Gamete createGamete() const;                  // Meiosis

    float getGeneticDistance(const DiploidGenome& other) const;
    float calculateInbreedingCoeff() const;
};
```

### 5.2 Chromosomes and Genes

**Chromosome Organization:**

| Chromosome | Genes | Purpose |
|------------|-------|---------|
| **0** | 7 genes | Physical traits (size, speed, vision, efficiency, metabolism, fertility, maturation) |
| **1** | 5 genes | Color/Display (RGB, ornament intensity, display frequency) |
| **2** | 8 genes | Behavior/Mate preferences (aggression, sociality, curiosity, fear, mate preferences) |
| **3** | 5 genes | Niche/Tolerance (diet, habitat, activity time, heat/cold tolerance) |
| **All** | 24 genes | Neural network weights (distributed) |

**Chromosome Class:**

```cpp
class Chromosome {
public:
    std::vector<Gene> genes;

    // Recombination
    static std::pair<Chromosome, Chromosome> recombine(
        const Chromosome& maternal,
        const Chromosome& paternal
    );

    // Structural mutations
    void applyPointMutation(int geneIndex);
    void applyInsertion(int position, const Gene& gene);
    void applyDeletion(int position);
    void applyDuplication(int start, int end);
    void applyInversion(int start, int end);
};
```

### 5.3 Alleles and Dominance

**Allele Structure:**

```cpp
struct Allele {
    int id;                    // Unique identifier
    float value;               // Trait value (0.0 - 1.0)
    float dominance;           // 0 = recessive, 0.5 = additive, 1 = dominant
    float selectionCoef;       // Fitness effect
    MutationType mutationType; // How it arose
    bool isDeleterious;        // Harmful allele flag
    float expressionLevel;     // Gene expression modifier

    // Calculate phenotype from two alleles
    static float calculatePhenotype(const Allele& a1, const Allele& a2);
};
```

**Dominance Model:**

```
Phenotype = (1 - h) * dominant_value + h * recessive_value

Where h = dominance coefficient of recessive allele:
  h = 0.0: Complete dominance (dominant allele fully expressed)
  h = 0.5: Additive (both alleles contribute equally)
  h = 1.0: Complete recessiveness (recessive allele fully expressed)
```

### 5.4 Mutation Types

| Type | Probability | Effect |
|------|-------------|--------|
| **Point Silent** | 30% | No phenotypic effect |
| **Point Missense** | 50% | Small value change (±10-20%) |
| **Point Nonsense** | 5% | Severe function reduction |
| **Regulatory** | 10% | Expression level change |
| **Insertion** | 1% | Add new gene copy |
| **Deletion** | 0.5% | Remove gene |
| **Duplication** | 1% | Copy gene segment |
| **Inversion** | 0.5% | Reverse gene order |

### 5.5 Sexual Reproduction

```cpp
// Reproduction process
DiploidGenome offspring(parent1.diploidGenome, parent2.diploidGenome);

// Inside constructor:
// 1. Create gametes via meiosis
Gamete gamete1 = parent1.createGamete();  // Crossover + independent assortment
Gamete gamete2 = parent2.createGamete();

// 2. Combine gametes
for (int i = 0; i < NUM_CHROMOSOMES; i++) {
    chromosomePairs[i].maternal = gamete1.chromosomes[i];
    chromosomePairs[i].paternal = gamete2.chromosomes[i];
}

// 3. Apply mutations
if (random() < mutationRate) {
    mutate();
}

// 4. Inherit epigenetic marks (30% probability)
inheritEpigeneticMarks(parent1, parent2);
```

### 5.6 Speciation

**Species Detection:**

```cpp
class SpeciationTracker {
public:
    // Detection parameters
    float geneticDistanceThreshold = 0.15;  // Minimum genetic distance for new species
    int minimumPopulation = 10;              // Minimum population to form species

    // Methods
    void update(const std::vector<Creature*>& creatures);
    void checkForSpeciation();
    void checkForExtinction();
    void assignToSpecies(Creature* creature);

    // Phylogenetic tree
    PhylogeneticTree tree;
    std::string exportNewick() const;  // Export tree in standard format
};
```

**Species Formation Criteria:**
1. Genetic distance from existing species > threshold
2. Population size > minimum threshold
3. Reproductive isolation from other species

### 5.7 Mate Selection

**MateSelector Class:**

```cpp
class MateSelector {
public:
    struct MatePreferences {
        float sizePreference;       // -1 (smaller) to +1 (larger)
        float ornamentPreference;   // Importance of displays
        float similarityPreference; // -1 (different) to +1 (similar)
        float choosiness;           // Selectivity level (0-1)
        float minimumAcceptable;    // Threshold for rejection
    };

    // Find and evaluate mates
    std::vector<Creature*> findPotentialMates(Creature* seeker, float searchRadius);
    float evaluateMate(Creature* seeker, Creature* candidate);
    Creature* selectMate(Creature* seeker, const std::vector<Creature*>& candidates);

    // Compatibility check
    ReproductiveCompatibility checkCompatibility(Creature* a, Creature* b);
};
```

**Evaluation Criteria:**
- Size match (based on preference)
- Ornament quality
- Genetic compatibility (MHC-like dissimilarity)
- Physical compatibility (size ratio)
- Temporal compatibility (activity time overlap)
- Ecological compatibility (niche overlap)

### 5.8 Hybrid Zones

**HybridZone Types:**

| Type | Description | Hybrid Fitness |
|------|-------------|----------------|
| `TENSION` | Selection against hybrids | Reduced |
| `BOUNDED_HYBRID` | Hybrids favored in intermediate environment | Enhanced |
| `MOSAIC` | Patchy environment | Variable |
| `PARAPATRIC` | Along environmental gradient | Gradient |

```cpp
class HybridZone {
public:
    glm::vec3 center;
    float radius;
    HybridZoneType type;

    float calculateHybridFitness(const DiploidGenome& genome) const;
    float getInterbreedingProbability(const glm::vec3& position) const;
};
```

---

## 6. Neural Network & AI

### 6.1 NEAT Implementation

**NeuroEvolution of Augmenting Topologies (NEAT):**

```cpp
class NEATGenome {
public:
    std::vector<NodeGene> nodes;
    std::vector<ConnectionGene> connections;

    // Structural mutations
    void mutateAddNode(std::mt19937& rng);
    void mutateAddConnection(std::mt19937& rng);
    void mutateWeights(std::mt19937& rng);
    void mutatePlasticity(std::mt19937& rng);

    // Crossover
    static NEATGenome crossover(const NEATGenome& parent1,
                                 const NEATGenome& parent2,
                                 std::mt19937& rng);

    // Compatibility for speciation
    float getCompatibilityDistance(const NEATGenome& other) const;

    // Convert to runnable network
    NeuralNetwork* buildNetwork() const;
};
```

**Innovation Tracking:**
- Global innovation counter ensures gene alignment during crossover
- Historical marking allows structural comparison between genomes
- Enables meaningful crossover of networks with different topologies

### 6.2 Modular Brain Architecture

**Three Brain Modes:**

```cpp
enum class BrainType {
    LEGACY_STEERING,   // Original hardcoded behaviors
    MODULAR_BRAIN,     // Separate modules for different functions
    NEAT_EVOLVED       // NEAT-evolved topology
};
```

**Modular Brain Structure:**

```
┌─────────────────────────────────────────────────────────────┐
│                    MODULAR BRAIN                             │
│                                                              │
│  ┌────────────────┐         ┌────────────────┐             │
│  │    SENSORY     │         │   EMOTIONAL    │             │
│  │   PROCESSOR    │         │    MODULE      │             │
│  │                │         │                │             │
│  │ 15 in → 8 out  │         │ 10 in → 4 out  │             │
│  │ (6 hidden)     │         │ (4 hidden)     │             │
│  └───────┬────────┘         └───────┬────────┘             │
│          │                          │                       │
│          └────────────┬─────────────┘                       │
│                       ▼                                     │
│           ┌────────────────────┐                           │
│           │   DECISION MAKER   │                           │
│           │                    │                           │
│           │   28 in → 8 out    │                           │
│           │   (16 hidden)      │                           │
│           │   [recurrent]      │                           │
│           └─────────┬──────────┘                           │
│                     ▼                                       │
│           ┌────────────────────┐                           │
│           │  MOTOR CONTROLLER  │                           │
│           │                    │                           │
│           │   8 in → 4 out     │                           │
│           │   (4 hidden)       │                           │
│           └────────────────────┘                           │
│                     ▼                                       │
│           [turn, speed, attack, flee]                       │
└─────────────────────────────────────────────────────────────┘
```

### 6.3 Neuromodulation System

**Neuromodulator Types:**

| Modulator | Function | Effect |
|-----------|----------|--------|
| **Dopamine** | Reward/pleasure | Reinforces successful behaviors |
| **Norepinephrine** | Arousal/alertness | Increases learning rate |
| **Serotonin** | Mood/patience | Reduces impulsive actions |
| **Acetylcholine** | Attention/learning | Enhances memory formation |

```cpp
struct NeuromodulatorState {
    float dopamine;        // 0-1, reward signal
    float norepinephrine;  // 0-1, arousal level
    float serotonin;       // 0-1, mood state
    float acetylcholine;   // 0-1, attention level

    float getLearningRate() const {
        return 0.01f * (0.5f + acetylcholine) * (0.5f + 0.5f * norepinephrine);
    }

    float getRewardSignal() const {
        return dopamine;
    }
};
```

### 6.4 Sensory Processing

**Sensory Modalities:**

```cpp
struct SensoryGenome {
    // Vision
    float visionFOV;           // Field of view (π/2 to 2π)
    float visionRange;         // Detection distance
    float visionAcuity;        // Detail resolution
    float colorPerception;     // Color discrimination
    float motionDetection;     // Movement sensitivity

    // Hearing
    float hearingRange;        // Sound detection distance
    float hearingDirectionality; // Localization accuracy
    bool hasEcholocation;      // Active sonar

    // Smell
    float smellRange;          // Scent detection distance
    float smellSensitivity;    // Concentration threshold
    float pheromoneProduction; // Chemical signaling

    // Touch
    float touchRange;          // Contact detection
    float vibrationSensitivity; // Ground vibration sensing

    // Defense
    float camouflageLevel;     // Reduces detection by others

    // Memory
    float memoryCapacity;      // Spatial memory slots
    float memoryRetention;     // Memory decay rate

    float calculateEnergyCost() const;
};
```

**Sensory Input Vector (15 inputs):**
1. Distance to nearest food
2. Angle to nearest food
3. Distance to nearest predator
4. Angle to nearest predator
5. Distance to nearest prey
6. Angle to nearest prey
7. Current energy level
8. Speed (normalized)
9. Age (normalized)
10. Nearest creature distance
11. Population density
12. Time of day
13. Season indicator
14. Terrain height
15. Water proximity

### 6.5 Steering Behaviors

**Craig Reynolds' Behaviors:**

```cpp
class SteeringBehaviors {
public:
    glm::vec3 seek(const glm::vec3& target);      // Move toward target
    glm::vec3 flee(const glm::vec3& threat);      // Move away from threat
    glm::vec3 arrive(const glm::vec3& target);    // Approach with deceleration
    glm::vec3 wander();                           // Random exploration
    glm::vec3 pursuit(const Creature* target);    // Intercept moving target
    glm::vec3 evade(const Creature* threat);      // Escape from moving threat
    glm::vec3 separation(const std::vector<Creature*>& neighbors);
    glm::vec3 alignment(const std::vector<Creature*>& neighbors);
    glm::vec3 cohesion(const std::vector<Creature*>& neighbors);
    glm::vec3 avoidBoundary(const glm::vec3& worldSize);
};
```

### 6.6 Brain Evolution

**Evolution Process:**

```cpp
class BrainEvolutionManager {
public:
    void updateFitness(Creature* creature, float fitness);
    NEATGenome crossoverParents(Creature* parent1, Creature* parent2);
    void advanceGeneration();

    // Statistics
    int generation;
    int speciesCount;
    float averageFitness;
    float averageComplexity;
};
```

**Fitness Components:**
- Survival time (longevity)
- Food consumed (energy efficiency)
- Offspring produced (reproductive success)
- Distance traveled (exploration)
- Combat victories (for predators)

---

## 7. Ecosystem & Environment

### 7.1 Terrain Generation

**Perlin Noise Height Map:**

```cpp
class Terrain {
public:
    static const int GRID_SIZE = 300;    // 300x300 vertices
    static const float WORLD_SIZE = 300.0f;

    void generate();
    float getHeight(float x, float z) const;
    glm::vec3 getNormal(float x, float z) const;
    bool isWater(float x, float z) const;
    BiomeType getBiome(float x, float z) const;

    void render(Shader& shader);
};
```

**Biome Distribution:**

| Biome | Height Range | Color | Features |
|-------|--------------|-------|----------|
| Water | < 0.38 | Blue | Animated waves |
| Sand | 0.38 - 0.42 | Tan | Beach areas |
| Grass | 0.42 - 0.70 | Green | Main habitat |
| Rock | 0.70 - 0.85 | Gray | Mountain slopes |
| Snow | > 0.85 | White | Mountain peaks |

### 7.2 Vegetation System

**VegetationManager:**

```cpp
class VegetationManager {
public:
    void generate(const Terrain& terrain);
    void render(Shader& shader);

    std::vector<TreeInstance> trees;
    std::vector<BushInstance> bushes;
    std::vector<GrassInstance> grass;

private:
    bool isSuitableForTree(float height, BiomeType biome) const;
    TreeType selectTreeType(float height, bool nearWater) const;
};
```

**Placement Rules:**
- Trees: Every 25 units, 15% probability in suitable areas
- Bushes: Every 20 units, 35% probability
- Grass: Every 15 units, 40% probability

**Biome-Based Distribution:**
- Near water (< 0.5): 70% Willow, 30% Oak
- Mid-level (0.5-0.7): Mixed Oak/Willow/Pine
- Higher elevation (> 0.7): 75% Pine, 25% Oak

### 7.3 L-System Trees

**L-System Grammar:**

```
Tree Type   | Axiom | Rules                                    | Angle
------------|-------|------------------------------------------|-------
Oak         | FA    | A → [&FL!A]////[&FL!A]////[&FL!A]        | 28°
            |       | F → S[//&F], S → F                       |
Pine        | FFA   | A → [&&B]////[&&B]////[&&B]FA            | 25°
            |       | B → [+F][-F]                             |
Willow      | FFA   | A → [&&&W]////[&&&W]////[&&&W]           | 15°
            |       | W → F[&&W]                               |
Bush        | FFFA  | A → [+FB][-FB][&FB][^FB]//A              | 35°
            |       | B → [+F][-F]                             |
```

**Turtle Graphics Commands:**

| Command | Action |
|---------|--------|
| `F` | Move forward, draw branch |
| `+` / `-` | Rotate around up axis (yaw) |
| `&` / `^` | Rotate around left axis (pitch) |
| `\` / `/` | Roll around heading axis |
| `[` / `]` | Push/pop state for branching |
| `!` | Decrease branch diameter |

### 7.4 Seasonal Cycles

**SeasonManager:**

```cpp
class SeasonManager {
public:
    enum class Season { SPRING, SUMMER, FALL, WINTER };

    static const int DAYS_PER_SEASON = 90;
    static const int DAYS_PER_YEAR = 360;

    void update(float deltaTime);

    Season getCurrentSeason() const;
    float getGrowthMultiplier() const;
    float getTemperature() const;
    float getDayLength() const;

    // Specialized multipliers
    float getBerryMultiplier() const;    // Peaks in fall
    float getFruitMultiplier() const;    // Peaks in summer
    float getLeafMultiplier() const;     // Drops in fall
    float getHerbivoreReproduction() const;  // High in spring
    float getCarnivoreActivity() const;  // High in fall
    float getMetabolismRate() const;     // High in winter
};
```

**Seasonal Effects:**

| Season | Growth | Temperature | Day Length | Special Effects |
|--------|--------|-------------|------------|-----------------|
| Spring | 1.5× | 0.4 | 14h | +50% herbivore reproduction |
| Summer | 1.0× | 0.8 | 16h | +20% fruit availability |
| Fall | 0.5× | 0.5 | 12h | +50% berry availability, +20% predator activity |
| Winter | 0.1× | 0.2 | 8h | +50% metabolism cost |

### 7.5 Producer System

**ProducerSystem:**

```cpp
class ProducerSystem {
public:
    struct FoodSource {
        FoodType type;
        float energy;
        float regrowthRate;
        float currentAmount;
    };

    void update(float deltaTime, Season season);
    float consume(FoodType type, const glm::vec3& position, float amount);

    // Food types with regrowth rates
    // Grass:       0.5/s, 2 energy
    // Bush Berries: 0.2/s, 5 energy
    // Tree Fruit:   0.1/s, 8 energy
    // Tree Leaves:  0.15/s, 4 energy

private:
    SoilGrid soilGrid;  // Nutrient tracking
};
```

**Soil System:**
- 50×50 grid of soil tiles
- Tracks nitrogen, phosphorus, organic matter, moisture
- Growth multiplier: `0.5 + 0.5 * nitrogen * moisture`
- Higher terrain = lower fertility, lower moisture

### 7.6 Decomposer System

**DecomposerSystem:**

```cpp
class DecomposerSystem {
public:
    struct Corpse {
        glm::vec3 position;
        float biomass;           // 50% of creature's energy
        float decompositionProgress;
        float size;
    };

    void addCorpse(Creature* creature);
    void update(float deltaTime, Season season);
    float scavenge(const glm::vec3& position, float amount);

private:
    std::vector<Corpse> corpses;

    float calculateDecompositionRate(const Corpse& corpse, float moisture);
    void releaseNutrients(const Corpse& corpse, SoilGrid& soil);
};
```

**Decomposition Factors:**
- Base rate: 2.0 biomass/second
- Season multiplier: 1.5× in summer, 0.5× in winter
- Size factor: `1.0 / (0.5 + size * 0.5)`
- Moisture: 0.5-1.0 multiplier

**Nutrient Release:**
- 30% nitrogen
- 10% phosphorus
- 50% organic matter

### 7.7 Trophic Dynamics

**EcosystemManager:**

```cpp
class EcosystemManager {
public:
    void update(float deltaTime);
    void onCreatureDeath(Creature* creature);

    // Carrying capacities (base, adjusted by season)
    struct CarryingCapacity {
        int grazers = 40;        // 1.2× spring, 0.7× winter
        int browsers = 30;
        int frugivores = 25;
        int smallPredators = 12;
        int omnivores = 10;
        int apexPredators = 6;
    };

    float getEcosystemHealth() const;
    SpawnRecommendation getSpawnRecommendation() const;

private:
    ProducerSystem producers;
    DecomposerSystem decomposers;
    SeasonManager seasons;
    EcosystemMetrics metrics;
};
```

**Trophic Pyramid:**

```
                    ┌───────────┐
                    │   Apex    │ (6)
                    │ Predators │
                    └─────┬─────┘
                          │
              ┌───────────┴───────────┐
              │   Small Predators     │ (12)
              │   Omnivores           │
              └───────────┬───────────┘
                          │
    ┌─────────────────────┴─────────────────────┐
    │              Herbivores                    │ (40-50)
    │   Grazers, Browsers, Frugivores           │
    └─────────────────────┬─────────────────────┘
                          │
┌─────────────────────────┴─────────────────────────┐
│                    Producers                       │
│         Grass, Bushes, Trees (unlimited)          │
└───────────────────────────────────────────────────┘
```

---

## 8. Graphics & Rendering

### 8.1 Rendering Pipeline

```
┌─────────────────────────────────────────────────────────────┐
│                    RENDER FRAME                              │
│                                                              │
│  1. Clear framebuffer                                        │
│  2. Set up view/projection matrices from Camera              │
│                                                              │
│  3. TERRAIN PASS                                             │
│     ├─ Bind terrain shader                                   │
│     ├─ Set uniforms (view, projection, time, light)          │
│     └─ Draw terrain mesh (90,000 vertices)                   │
│                                                              │
│  4. VEGETATION PASS                                          │
│     ├─ Bind tree shader                                      │
│     └─ Draw trees (instanced)                                │
│                                                              │
│  5. CREATURE PASS                                            │
│     ├─ Group creatures by mesh key                           │
│     ├─ For each mesh type:                                   │
│     │   ├─ Bind creature shader                              │
│     │   ├─ Set per-instance data (transforms, colors)        │
│     │   └─ Draw instanced (single draw call)                 │
│     └─ Release resources                                     │
│                                                              │
│  6. UI PASS                                                  │
│     ├─ Bind nametag shader                                   │
│     └─ Draw nametags (billboarded quads)                     │
│                                                              │
│  7. Swap buffers                                             │
└─────────────────────────────────────────────────────────────┘
```

### 8.2 Procedural Generation

**No Texture Files Required:**
- All textures generated via GLSL shaders
- Perlin noise, FBM, Voronoi functions in shaders
- Per-frame evaluation (fast on modern GPUs)
- Scalable detail via noise octaves

**Noise Functions:**

```glsl
// 3D Perlin noise
float noise3D(vec3 p);

// Fractal Brownian Motion
float fbm(vec3 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise3D(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

// Voronoi cellular pattern
float voronoi(vec3 p);
```

### 8.3 Metaball System

**Implicit Surface Definition:**

```cpp
struct Metaball {
    glm::vec3 position;
    float radius;
    float strength;
};

class MetaballSystem {
public:
    void addMetaball(const glm::vec3& pos, float radius, float strength);
    void clear();

    // Evaluate potential field at point
    float evaluate(const glm::vec3& point) const;

    // Analytical gradient (6× faster than finite differences)
    glm::vec3 calculateGradient(const glm::vec3& point) const;

    // Get surface normal
    glm::vec3 calculateNormal(const glm::vec3& point) const;

private:
    std::vector<Metaball> metaballs;

    // Smooth falloff function: strength * (1 - (d/r)²)²
    float falloff(float distance, float radius, float strength) const;
};
```

**Analytical Gradient Formula:**
```
f(r) = strength * (1 - (r/R)²)²

f'(r) = -4 * strength * (1 - (r/R)²) * r / R²

This is 6× faster than finite difference approximation.
```

### 8.4 Marching Cubes

**Isosurface Extraction:**

```cpp
class MarchingCubes {
public:
    // Complete 256-entry edge table
    static const int edgeTable[256];

    // Complete triangle table
    static const int triTable[256][16];

    // Generate mesh from metaball field
    MeshData generateMesh(const MetaballSystem& metaballs,
                          float isovalue = 0.5f,
                          int resolution = 16);

private:
    // Interpolate vertex position on edge
    glm::vec3 interpolateVertex(const glm::vec3& p1, const glm::vec3& p2,
                                 float val1, float val2, float isovalue);
};
```

**Performance:**
- Resolution: 16-20 grid cells per axis
- Output: 1,000-3,000 vertices per creature
- Generation time: < 10ms per creature

### 8.5 Creature Building

**Body Plan Types:**

| Plan | Limbs | Description | Used By |
|------|-------|-------------|---------|
| QUADRUPED | 4 legs | Four-legged walker | 65% herbivores, 40% carnivores |
| BIPED | 2 legs | Two-legged runner | 25% carnivores |
| HEXAPOD | 6 legs | Insect-like | 20% herbivores, 15% carnivores |
| SERPENTINE | 0 legs | Snake-like | 10% herbivores, 12% carnivores |
| AVIAN | 2 wings | Bird-like | 5% herbivores, 8% carnivores |

**CreatureBuilder:**

```cpp
class CreatureBuilder {
public:
    MeshData buildCreature(const Genome& genome, CreatureType type);

private:
    MetaballSystem metaballs;

    void addBody(const Genome& genome, CreatureType type);
    void addHead(HeadShape shape, float size);
    void addLimbs(BodyPlan plan, float size, float length);
    void addTail(TailType type, float length);
    void addAppendages(const Genome& genome, CreatureType type);

    // Special features
    void addAntlers(float size, int branches);
    void addMandibles(float size);
    void addWings(float wingspan, float chord);
    void addSpikes(const std::vector<glm::vec3>& positions);
};
```

**Genome-Based Deterministic Random:**
```cpp
// Same genome always produces same appearance
float genomeRandom(const Genome& genome, int index) {
    float val = genome.neuralWeights[index % genome.neuralWeights.size()];
    val = fmod(val * 12.9898f + genome.size * 78.233f + genome.speed * 43.758f, 1.0f);
    return std::abs(val);
}
```

### 8.6 Instanced Rendering

**CreatureRenderer:**

```cpp
class CreatureRenderer {
public:
    void render(const std::vector<Creature*>& creatures,
                CreatureMeshCache& cache,
                Shader& shader,
                float time);

private:
    struct PerInstanceData {
        glm::mat4 modelMatrix;    // Locations 3-6 (4 vec4s)
        glm::vec3 color;          // Location 7
        float animationPhase;     // Location 8
    };

    std::unordered_map<MeshKey, RenderBatch> batches;
    GLuint instanceVBO;
};
```

**Batching Strategy:**
1. Group creatures by mesh key (type + size + speed categories)
2. Upload per-instance data to GPU
3. Single draw call per mesh type
4. Reduces CPU-GPU communication by 10-100×

### 8.7 Shader System

**Creature Vertex Shader (`creature_vertex.glsl`):**

```glsl
// Procedural animation
float bobAmount = sin(time * animSpeed) * 0.05;
float swayAmount = sin(time * animSpeed * 1.3) * 0.03;
float breathScale = 1.0 + sin(time * 0.8) * 0.01;

// Apply animations
animatedPos.y += bobAmount * max(aPos.y, 0.0) * 0.3;
animatedPos.x += swayAmount * max(aPos.y, 0.0) * 0.2;
animatedPos *= breathScale;
```

**Creature Fragment Shader (`creature_fragment.glsl`):**

```glsl
// Procedural skin pattern
float scales = voronoi(worldPos * 8.0);       // Scale pattern
float detail = fbm(worldPos * 20.0, 4);       // Fine detail
float pattern = 0.7 * scales + 0.3 * detail;

// Lighting
float wrapDiff = max(0.0, (diff + 0.3) / 1.3);  // Wrap-around lighting
float rim = pow(1.0 - max(dot(viewDir, norm), 0.0), 3.0);  // Rim lighting

// Fog
float fogFactor = exp(-0.008 * max(0.0, distance - 50.0));
```

**Terrain Fragment Shader (`fragment.glsl`):**

```glsl
// Biome blending
vec3 waterColor = vec3(0.2, 0.4, 0.6);
vec3 sandColor = vec3(0.9, 0.8, 0.6);
vec3 grassColor = vec3(0.3, 0.6, 0.2);
vec3 rockColor = vec3(0.5, 0.5, 0.5);
vec3 snowColor = vec3(0.95, 0.95, 1.0);

// Height-based selection with smooth blending
float blendRange = 0.05;
// ... interpolation logic
```

---

## 9. Physics & Morphology

### 9.1 Locomotion System

**Gait Types:**

| Gait | Duty Factor | Description | Used By |
|------|-------------|-------------|---------|
| WALK | 0.6 | Slow, efficient | All quadrupeds |
| TROT | 0.5 | Medium speed | Quadrupeds |
| GALLOP | 0.3 | Fast, high energy | Large quadrupeds |
| CRAWL | 0.8 | Very slow | Small creatures |
| SLITHER | N/A | Wave propagation | Serpentines |
| SWIM | N/A | Aquatic locomotion | Aquatic |
| FLY | N/A | Aerial locomotion | Avians |
| HOP | 0.2 | Jumping | Bipeds |
| TRIPOD | 0.5 | Insect gait | Hexapods |
| WAVE | 0.7 | Caterpillar-like | Serpentines |

**Central Pattern Generator (CPG):**

```cpp
class CPG {
public:
    void setGait(GaitType gait);
    void update(float deltaTime);
    float getLimbPhase(int limbIndex) const;
    bool isInStance(int limbIndex) const;

private:
    float frequency;          // Cycles per second
    float dutyFactor;         // Fraction of cycle in stance
    std::vector<float> phaseOffsets;  // Per-limb phase offsets
    float globalPhase;        // Current cycle phase (0-1)
};
```

**Inverse Kinematics:**
- FABRIK algorithm for multi-joint chains
- 2-bone analytical IK for simple limbs
- Joint constraints (angle limits)

### 9.2 Body Plans

**Morphology Genes (50+ parameters):**

```cpp
struct MorphologyGenes {
    // Body structure
    SymmetryType symmetry;     // Bilateral, radial, asymmetric
    int numSegments;           // Body segments
    float segmentSize[MAX_SEGMENTS];

    // Limbs
    int numLimbs;
    float limbLength;
    float limbThickness;
    JointType jointTypes[MAX_JOINTS];

    // Features
    bool hasWings;
    bool hasTail;
    bool hasShell;
    float hornSize;
    float clawSize;

    // Methods
    void randomize();
    void mutate(float rate, float strength);
    static MorphologyGenes crossover(const MorphologyGenes& p1,
                                      const MorphologyGenes& p2);
};
```

**Allometric Scaling (Kleiber's Law):**
```cpp
// Metabolic rate scales with mass^0.75
float metabolicRate = baseMR * pow(mass, 0.75f);

// Max speed scales with mass^0.17
float maxSpeed = baseSpeed * pow(mass, 0.17f);

// Limb frequency scales with mass^-0.17
float limbFreq = baseFreq * pow(mass, -0.17f);

// Muscle force scales with mass^0.67
float muscleForce = baseForce * pow(mass, 0.67f);
```

### 9.3 Metamorphosis

**Life Stages:**

```cpp
enum class LifeStage {
    EGG,
    LARVAL,
    JUVENILE,
    ADULT,
    ELDER
};

enum class MetamorphosisType {
    NONE,              // Direct development
    GRADUAL,           // Nymph → adult (like grasshoppers)
    COMPLETE,          // Larva → pupa → adult (like butterflies)
    AQUATIC_TO_LAND,   // Tadpole → frog
    LARVAL_DISPERSAL   // Specialized larval stage
};
```

**Morphology Interpolation:**

```cpp
class MetamorphosisController {
public:
    void update(float deltaTime);
    MorphologyGenes getInterpolatedMorphology() const;

private:
    MetamorphosisType type;
    float progress;        // 0.0 to 1.0
    MorphologyGenes larvalForm;
    MorphologyGenes adultForm;

    float easingFunction(float t) const;
};
```

**Staged Appearance:**
- Limbs appear after 30% progress
- Wings appear after 60% progress
- Fins disappear early for aquatic-to-land
- Color changes throughout

### 9.4 Fitness Landscape

**Fitness Components:**

| Category | Factors | Weight |
|----------|---------|--------|
| Movement | Speed, acceleration, maneuverability, stability | 25% |
| Energy | Efficiency, metabolic rate, stamina | 20% |
| Combat | Attack reach, power, speed, defense | 15% |
| Survival | Evasion, camouflage, sensory range | 15% |
| Resources | Food gathering, reach, carrying capacity | 15% |
| Reproduction | Mate finding, display quality, offspring care | 10% |

**Environment Modifiers:**

| Environment | Speed | Maneuverability | Efficiency | Combat | Sensory |
|-------------|-------|-----------------|------------|--------|---------|
| Plains | 1.3× | 0.8× | 1.0× | 1.0× | 1.2× |
| Forest | 0.7× | 1.3× | 1.0× | 0.9× | 0.8× |
| Mountain | 0.5× | 1.2× | 1.2× | 0.8× | 1.0× |
| Desert | 1.0× | 1.0× | 1.4× | 1.0× | 1.0× |
| Aquatic | 0.8× | 1.2× | 1.0× | 0.7× | 0.6× |
| Aerial | 1.2× | 1.4× | 1.1× | 0.5× | 1.4× |

### 9.5 Visual Indicators

**State-Based Appearance:**

| State | Color Effect | Posture | Animation |
|-------|--------------|---------|-----------|
| Healthy | Full saturation | Upright | Normal speed |
| Injured | Red tint | Slumped | Slower |
| Starving | Desaturated | Drooping | Very slow |
| Afraid | Pale/blue tint | Crouched | Trembling |
| Aggressive | Dark/red tint | Raised | Faster |
| Mating | Bright colors | Display posture | Exaggerated |

```cpp
struct VisualState {
    // Color modulation
    float saturation;    // 0-1
    float brightness;    // 0-1
    glm::vec3 tint;      // RGB modifier

    // Posture
    float slump;         // 0-1 (drooping)
    float crouch;        // 0-1 (lowered stance)
    float archBack;      // 0-1 (aggressive)

    // Animation
    float speedMultiplier;   // Animation speed
    float breathingRate;     // Breathing speed
    float tremblingAmount;   // Shake intensity
};
```

---

## 10. Configuration & Parameters

### 10.1 Simulation Parameters

```cpp
// Population
const int INITIAL_HERBIVORES = 60;
const int INITIAL_CARNIVORES = 15;
const int MAX_HERBIVORES = 150;
const int MAX_CARNIVORES = 50;

// Energy
const float STARTING_ENERGY = 100.0f;
const float MAX_ENERGY = 200.0f;
const float MOVEMENT_COST = 0.5f;      // per second while moving
const float BASE_METABOLISM = 0.2f;     // per second
const float FOOD_ENERGY = 30.0f;        // per food item
const float KILL_ENERGY = 120.0f;       // energy from killing

// Combat
const float ATTACK_RANGE = 2.5f;
const float ATTACK_DAMAGE = 15.0f;
const float ATTACK_COOLDOWN = 0.5f;     // seconds
```

### 10.2 Evolution Parameters

```cpp
// Mutation
const float MUTATION_RATE = 0.08f;       // 8% chance per reproduction
const float MUTATION_STRENGTH = 0.15f;   // ±15% change
const float STRUCTURAL_MUTATION_RATE = 0.05f;  // Node/connection additions

// Reproduction
const float HERBIVORE_REPRO_THRESHOLD = 180.0f;
const float CARNIVORE_REPRO_THRESHOLD = 170.0f;
const float REPRODUCTION_COST = 80.0f;

// Speciation
const float GENETIC_DISTANCE_THRESHOLD = 0.15f;
const int MINIMUM_SPECIES_POPULATION = 10;
```

### 10.3 Environment Parameters

```cpp
// World
const int TERRAIN_SIZE = 300;           // Grid units
const float WORLD_SIZE = 300.0f;        // World units

// Food
const float FOOD_SPAWN_INTERVAL = 0.1f; // seconds
const int MAX_FOOD = 200;

// Seasons
const int DAYS_PER_SEASON = 90;
const float REAL_SECONDS_PER_DAY = 60.0f;
```

### 10.4 Performance Tuning

```cpp
// Rendering
const int MARCHING_CUBES_RESOLUTION = 16;  // Grid cells
const int MESH_CACHE_SIZE = 48;            // Cached meshes

// Spatial optimization
const float SPATIAL_GRID_CELL_SIZE = 10.0f;

// AI
const int NEURAL_HIDDEN_NEURONS = 4;
const int SENSORY_UPDATE_INTERVAL = 2;     // Frames
```

---

## 11. API Reference

### 11.1 Core Classes

**Simulation** (`src/core/Simulation.h`)

```cpp
class Simulation {
public:
    // Lifecycle
    void init();
    void update(float deltaTime);
    void render(const Camera& camera);
    void cleanup();

    // Control
    void togglePause();
    void increaseSpeed();
    void decreaseSpeed();

    // Population
    void spawnCreature(CreatureType type, const glm::vec3& position);
    void removeCreature(Creature* creature);
    const std::vector<Creature*>& getCreatures() const;

    // Statistics
    int getHerbivoreCount() const;
    int getCarnivoreCount() const;
    int getGeneration() const;
    float getSimulationTime() const;
};
```

### 11.2 Entity Classes

**Creature** (`src/entities/Creature.h`)

```cpp
class Creature {
public:
    // State
    CreatureType type;
    glm::vec3 position;
    glm::vec3 velocity;
    float energy;
    bool isAlive;
    int generation;

    // Components
    Genome genome;
    DiploidGenome* diploidGenome;
    NeuralNetwork* brain;
    SensorySystem sensory;
    SteeringBehaviors steering;

    // Methods
    void update(float deltaTime, /* ... */);
    void render(Shader& shader);
    Creature* reproduce(Creature* mate);
    void takeDamage(float amount);
    void consumeFood(float energy);
    bool canReproduce() const;
};
```

**DiploidGenome** (`src/entities/genetics/DiploidGenome.h`)

```cpp
class DiploidGenome {
public:
    // Constructors
    DiploidGenome();  // Random
    DiploidGenome(const DiploidGenome& parent1, const DiploidGenome& parent2);

    // Genetics
    void mutate();
    float getTrait(GeneType type) const;
    Gamete createGamete() const;

    // Analysis
    float getGeneticDistance(const DiploidGenome& other) const;
    float calculateInbreedingCoeff() const;
    float getHeterozygosity() const;
    float getGeneticLoad() const;

    // Epigenetics
    void applyEnvironmentalStress(float intensity);
    void updateEpigeneticMarks();
};
```

### 11.3 Graphics Classes

**CreatureRenderer** (`src/graphics/rendering/CreatureRenderer.h`)

```cpp
class CreatureRenderer {
public:
    void render(const std::vector<Creature*>& creatures,
                CreatureMeshCache& cache,
                Shader& shader,
                float time);
};
```

**CreatureMeshCache** (`src/graphics/rendering/CreatureMeshCache.h`)

```cpp
class CreatureMeshCache {
public:
    MeshData* getMesh(const Genome& genome, CreatureType type);
    void preloadArchetypes();
    void printStats() const;
};
```

### 11.4 Utility Classes

**SpatialGrid** (`src/utils/SpatialGrid.h`)

```cpp
class SpatialGrid {
public:
    SpatialGrid(float cellSize, float worldSize);

    void clear();
    void insert(Creature* creature);

    std::vector<Creature*> queryCreatures(const glm::vec3& position,
                                           float radius) const;
    std::vector<Creature*> queryByType(const glm::vec3& position,
                                        float radius,
                                        CreatureType type) const;
    Creature* findNearest(const glm::vec3& position,
                          CreatureType type) const;
};
```

---

## 12. Appendices

### 12.1 Gene Type Reference

| ID | Gene Type | Range | Default | Description |
|----|-----------|-------|---------|-------------|
| 0 | SIZE | 0.5-2.0 | 1.0 | Body size multiplier |
| 1 | SPEED | 5.0-20.0 | 12.0 | Movement speed |
| 2 | VISION_RANGE | 10.0-50.0 | 30.0 | Detection distance |
| 3 | EFFICIENCY | 0.5-1.5 | 1.0 | Energy efficiency |
| 4 | METABOLIC_RATE | 0.5-1.5 | 1.0 | Base metabolism |
| 5 | FERTILITY | 0.5-1.5 | 1.0 | Reproduction rate |
| 6 | MATURATION_RATE | 0.5-2.0 | 1.0 | Growth speed |
| 7 | COLOR_RED | 0.0-1.0 | 0.5 | Red color component |
| 8 | COLOR_GREEN | 0.0-1.0 | 0.5 | Green color component |
| 9 | COLOR_BLUE | 0.0-1.0 | 0.5 | Blue color component |
| 10 | ORNAMENT_INTENSITY | 0.0-1.0 | 0.3 | Display strength |
| 11 | DISPLAY_FREQUENCY | 0.0-1.0 | 0.3 | Display rate |
| 12 | AGGRESSION | 0.0-1.0 | 0.5 | Attack tendency |
| 13 | SOCIALITY | 0.0-1.0 | 0.5 | Group behavior |
| 14 | CURIOSITY | 0.0-1.0 | 0.5 | Exploration tendency |
| 15 | FEAR_RESPONSE | 0.0-1.0 | 0.5 | Flight response |
| 16-19 | MATE_*_PREF | -1.0-1.0 | 0.0 | Mate preferences |
| 20 | CHOOSINESS | 0.0-1.0 | 0.5 | Mate selectivity |
| 21-25 | NICHE_* | 0.0-1.0 | 0.5 | Environmental preferences |

### 12.2 Creature Type Reference

| Type | Diet | Prey | Predators | Carrying Capacity |
|------|------|------|-----------|-------------------|
| HERBIVORE | Plants | N/A | Carnivores | 60 |
| CARNIVORE | Meat | Herbivores | Apex | 20 |
| GRAZER | Grass | N/A | Small Pred | 40 |
| BROWSER | Leaves | N/A | Small Pred | 30 |
| FRUGIVORE | Fruit | N/A | Small Pred | 25 |
| SMALL_PREDATOR | Small prey | Grazers, Browsers | Apex | 12 |
| OMNIVORE | Mixed | Small prey | Small Pred, Apex | 10 |
| APEX_PREDATOR | Large prey | All predators | None | 6 |
| SCAVENGER | Carrion | N/A | All predators | 8 |
| PARASITE | Host energy | Hosts | All | 5 |
| CLEANER | Parasites | Parasites | None | 5 |

### 12.3 Shader Reference

**Shader Uniforms:**

| Uniform | Type | Shader | Description |
|---------|------|--------|-------------|
| `model` | mat4 | All | Model matrix |
| `view` | mat4 | All | View matrix |
| `projection` | mat4 | All | Projection matrix |
| `time` | float | All | Simulation time |
| `lightDir` | vec3 | Fragment | Light direction |
| `viewPos` | vec3 | Fragment | Camera position |
| `creatureColor` | vec3 | Creature | Base color |
| `animationPhase` | float | Creature | Animation phase |

### 12.4 Troubleshooting

**Common Issues:**

| Problem | Cause | Solution |
|---------|-------|----------|
| Black screen | Shader compilation failed | Check shader error output |
| Low FPS | Too many creatures | Reduce MAX_HERBIVORES/CARNIVORES |
| Creatures clipping | Terrain collision issue | Verify terrain height queries |
| No evolution visible | Too low mutation rate | Increase MUTATION_RATE |
| Population crashes | Ecosystem imbalance | Adjust carrying capacities |
| Memory leak | Unreleased creatures | Check removeCreature() calls |

**Performance Tips:**
1. Keep creature count below 200 for 60 FPS
2. Use spatial grid for all neighbor queries
3. Cache mesh generation results
4. Use instanced rendering for creatures
5. Reduce marching cubes resolution for faster generation

---

## License

This project is provided for educational and research purposes.

---

## Credits

**Created by:** [Your Name]
**Documentation generated:** 2026-01-13
**Lines of code:** 17,600+
**Documentation pages:** 1,200+

---

*End of Documentation*
