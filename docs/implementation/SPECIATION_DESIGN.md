# Speciation & Evolution Visualization System

## Overview

This document describes the creature speciation and evolution visualization system implemented in the OrganismEvolution simulator. The system enables observable evolution where creatures diverge into distinct species over time, with phylogenetic tree visualization and species-based visual differentiation.

## Core Components

### 1. Species Data Structure (`src/entities/genetics/Species.h`)

The `Species` class represents a distinct species within the simulation:

```cpp
class Species {
    SpeciesId id;                        // Unique species identifier
    std::string name;                    // Auto-generated Latin-like name
    uint64_t foundingLineage;            // Lineage ID of founder
    int foundingGeneration;              // Generation when species emerged
    PopulationStats stats;               // Population statistics
    EcologicalNiche niche;               // Ecological niche characteristics
    std::map<uint32_t, float> alleleFrequencies;  // Allele frequency tracking
    std::map<SpeciesId, float> reproductiveIsolation;  // Isolation from other species
    bool extinct;                        // Extinction status
    int extinctionGeneration;            // When species went extinct
};
```

Key features:
- Population tracking with historical statistics
- Allele frequency monitoring for genetic drift detection
- Reproductive isolation metrics between species pairs
- Ecological niche characterization for sympatric speciation

### 2. Speciation Tracker (`src/entities/genetics/Species.h/.cpp`)

The `SpeciationTracker` class monitors the population for speciation events:

```cpp
class SpeciationTracker {
    void update(std::vector<Creature*>& creatures, int currentGeneration);
    Species* getSpecies(SpeciesId id);
    std::vector<Species*> getActiveSpecies();
    PhylogeneticTree& getPhylogeneticTree();
};
```

**Speciation Detection Algorithm:**
1. Group creatures by current species
2. Build genetic distance matrix within each species
3. Apply single-linkage clustering with configurable threshold
4. Detect clusters that exceed population threshold
5. If cluster is genetically distant from parent species average, trigger speciation event

**Configuration Parameters:**
- `speciesThreshold` (default: 0.15): Genetic distance for species boundary
- `minPopulationForSpecies` (default: 10): Minimum population for new species
- `generationsForSpeciation` (default: 50): Generations of isolation for speciation

### 3. Phylogenetic Tree (`src/entities/genetics/Species.h/.cpp`)

The `PhylogeneticTree` class tracks evolutionary relationships:

```cpp
struct PhyloNode {
    uint64_t id;                    // Node identifier
    uint64_t parentId;              // Parent node (0 for root)
    SpeciesId speciesId;            // Associated species
    int generation;                 // Origin generation
    float branchLength;             // Evolutionary distance
    std::vector<uint64_t> childrenIds;  // Child species
    bool isExtant;                  // Currently alive
};

class PhylogeneticTree {
    uint64_t addRoot(SpeciesId speciesId, int generation);
    uint64_t addSpeciation(SpeciesId parentSpecies, SpeciesId childSpecies, int generation);
    void markExtinction(SpeciesId species, int generation);
    SpeciesId getMostRecentCommonAncestor(SpeciesId sp1, SpeciesId sp2) const;
    float getEvolutionaryDistance(SpeciesId sp1, SpeciesId sp2) const;
    std::string toNewick() const;  // Export standard phylogenetic format
};
```

### 4. Hybrid Zones (`src/entities/genetics/HybridZone.h/.cpp`)

The `HybridZone` system models areas where species can interbreed:

```cpp
enum class HybridZoneType {
    TENSION,           // Selection against hybrids + dispersal
    BOUNDED_HYBRID,    // Hybrids favored in intermediate environment
    MOSAIC,            // Patchy environment with parental types
    PARAPATRIC         // Along environmental gradient
};

class HybridZone {
    bool contains(const glm::vec3& position) const;
    float calculateHybridFitness(const glm::vec3& position) const;
    float getInterbreedingProbability(const glm::vec3& position) const;
};
```

### 5. Reproductive Isolation (`src/entities/Creature.h/.cpp`)

Creatures implement mate compatibility checking:

```cpp
bool Creature::canMateWith(const Creature& other) const;
bool Creature::willMateWith(const Creature& other) const;
float Creature::evaluateMateAttraction(const Creature& other) const;
```

Reproductive isolation mechanisms:
- Same species: Always compatible
- Parent-child species: 30% fertility (hybrid zone)
- Sibling species: 10% fertility
- Distant species: Incompatible

## UI Visualization

### Phylogenetic Tree Visualizer (`src/ui/PhylogeneticTreeVisualizer.h/.cpp`)

Interactive tree visualization supporting:
- **Vertical Layout**: Root at top, descendants below
- **Horizontal Layout**: Root at left, descendants right
- **Radial Layout**: Root at center, descendants outward

Features:
- Zoom and pan controls
- Node sizing by population
- Color coding by species color
- Extinction indicators (gray nodes)
- Hover tooltips with species details
- Click selection for species focus

### Species Evolution Panel (`src/ui/PhylogeneticTreeVisualizer.h/.cpp`)

Dashboard panel showing:
- Active/total species count
- Speciation/extinction event counts
- Active species list with expandable details
- Extinct species list
- Interactive phylogenetic tree canvas

## Visual Differentiation

### Species-Tinted Colors (`src/entities/Creature.cpp`)

```cpp
glm::vec3 Creature::getSpeciesTintedColor() const {
    glm::vec3 baseColor = genome.color;

    // Generate species color using golden ratio for even distribution
    float hue = fmod(speciesId * 0.618033988749895f, 1.0f);
    glm::vec3 speciesColor = hsvToRgb(hue, 0.7f, 0.9f);

    // Mix base color with species color (30% species tint)
    return glm::mix(baseColor, speciesColor, 0.3f);
}
```

### Pattern Types (`src/entities/Creature.cpp`)

```cpp
int Creature::getSpeciesPatternType() const {
    // 0=solid, 1=stripes, 2=spots, 3=gradient
    return static_cast<int>(speciesId) % 4;
}
```

### Shader Integration

The creature renderer uses species-tinted colors:
```cpp
// CreatureRenderer_DX12.cpp
glm::vec3 renderColor = creature->getSpeciesTintedColor();
instanceData.SetFromCreature(model, renderColor, animationPhase);
```

## Genetic Distance Calculation

Species differentiation is based on genetic distance between `DiploidGenome` instances:

```cpp
float DiploidGenome::distanceTo(const DiploidGenome& other) const {
    // Compares allele frequencies across all chromosomes
    // Returns normalized distance [0, 1]
}
```

The distance incorporates:
- Allele differences across all gene loci
- Heterozygosity differences
- Ecological niche divergence

## Species Name Generation

Auto-generated Latin-style names:

```cpp
std::string SpeciationTracker::generateSpeciesName(int index) {
    static const char* prefixes[] = {
        "Mega", "Micro", "Proto", "Neo", "Pseudo", "Para", "Epi",
        "Hyper", "Ultra", "Super", "Trans", "Meta"
    };
    static const char* roots[] = {
        "saurus", "therium", "morpha", "phyla", "genus", "cephalus",
        "dactyl", "pteryx", "raptor", "mimus", "venator", "cursor"
    };
    // Combines prefix + root + optional number
}
```

## Integration Points

### Simulation Loop Integration

The speciation tracker should be updated each generation:
```cpp
void SimulationLoop::update() {
    // ... creature updates ...

    if (generationComplete) {
        std::vector<Creature*> creaturePtrs = getCreaturePtrs();
        speciationTracker.update(creaturePtrs, currentGeneration);
    }
}
```

### Dashboard Integration

Connect the speciation tracker to the dashboard:
```cpp
dashboard.setSpeciationTracker(&speciationTracker);
```

The Species tab will then display:
- Population statistics
- Active/extinct species lists
- Interactive phylogenetic tree

## Observable Evolution Events

The system logs key evolutionary events:
```
[SPECIATION] Megasaurus emerged from Species_1 (generation 127, 15 individuals)
[EXTINCTION] Protomorpha went extinct (generation 203)
```

## Performance Considerations

- Distance matrix computation is O(n²) - computed only when checking for speciation
- Tree traversal is O(log n) for ancestor queries
- Species assignment is O(k) where k = number of species
- Visual rendering uses species color caching

## Future Enhancements

Potential improvements:
1. Ring species simulation (circular geographic distribution)
2. Reinforcement learning for mate preference evolution
3. Adaptive speciation threshold based on population dynamics
4. Parapatric speciation along environmental gradients
5. Sexual selection driving speciation
6. Gene flow visualization between hybrid zones
