# Genetics System Implementation Summary

This document summarizes the sophisticated genetics system implemented in the OrganismEvolution simulation.

## Architecture Overview

The genetics system follows a modular architecture with the following components:

```
src/entities/genetics/
├── Allele.h/cpp          - Individual allele variants
├── Gene.h/cpp            - Diploid genes with two alleles
├── Chromosome.h/cpp      - Ordered collections of genes
├── DiploidGenome.h/cpp   - Full genome with chromosome pairs
├── MateSelector.h/cpp    - Sexual selection and mate choice
├── Species.h/cpp         - Species definitions and tracking
├── HybridZone.h/cpp      - Hybrid zone dynamics
└── GeneticsManager.h/cpp - Integration and management
```

## Core Components

### 1. Allele System (`Allele.h/cpp`)
- **Unique identification**: Each allele has a unique ID
- **Value-based traits**: Alleles store numerical values affecting phenotype
- **Dominance coefficients**: 0 = recessive, 0.5 = additive, 1 = dominant
- **Fitness effects**: Selection coefficients for natural selection
- **Deleterious tracking**: Flags for harmful mutations
- **Mutation types**: Silent, missense, nonsense, regulatory mutations

### 2. Gene System (`Gene.h/cpp`)
- **Diploid structure**: Each gene contains two alleles (maternal/paternal)
- **Gene types**: 25+ different gene types affecting various traits
- **Phenotype calculation**: Combines alleles using dominance model
- **Epigenetic marks**: Methylation, acetylation, phosphorylation, imprinting
- **Expression levels**: Modifiable gene expression

### 3. Chromosome System (`Chromosome.h/cpp`)
- **Ordered gene collections**: Genes arranged in linear order
- **Crossover recombination**: Single and double crossover events
- **Structural mutations**: Insertions, deletions, duplications, inversions
- **Genetic distance calculation**: Compare chromosomes between individuals

### 4. Diploid Genome (`DiploidGenome.h/cpp`)
- **Chromosome pairs**: Multiple pairs of homologous chromosomes
- **Sexual reproduction**: Meiosis with crossover and independent assortment
- **Trait extraction**: Convert genotype to phenotype values
- **Genetic load**: Track cumulative deleterious allele effects
- **Inbreeding coefficient**: Measure of homozygosity
- **Heterozygosity**: Genetic diversity measure

### 5. Mate Selection (`MateSelector.h/cpp`)
- **Preference-based selection**: Size, ornament, similarity preferences
- **Genetic compatibility**: MHC-like dissimilarity preference
- **Reproductive isolation**: Behavioral, temporal, mechanical, gametic
- **Isolation type detection**: Automatic identification of isolation mechanisms

### 6. Species Tracking (`Species.h/cpp`)
- **Species definition**: Population-based with genetic distance thresholds
- **Allele frequency tracking**: Monitor allele distribution in population
- **Population statistics**: Size, heterozygosity, fitness, genetic load
- **Phylogenetic tree**: Track evolutionary history and relationships
- **Newick export**: Standard format for phylogenetic visualization

### 7. Hybrid Zones (`HybridZone.h/cpp`)
- **Zone types**: Tension, bounded hybrid superiority, mosaic, parapatric
- **Hybrid tracking**: F1 hybrids vs backcrosses
- **Fitness modification**: Zone-dependent hybrid fitness
- **Introgression**: Gene flow between species
- **Zone dynamics**: Creation, expansion, collapse detection

### 8. Genetics Manager (`GeneticsManager.h/cpp`)
- **System integration**: Coordinates all genetics subsystems
- **Reproduction handling**: Sexual and asexual reproduction
- **Epigenetic updates**: Environmental effects on gene expression
- **Drift detection**: Small population effects
- **Purging selection**: Remove high genetic load individuals
- **FST calculation**: Population differentiation measure

## Key Features Implemented

### Realistic Genetic Mechanisms
- ✅ Chromosomes with multiple genes per trait
- ✅ Diploid genetics with dominant/recessive alleles
- ✅ Codominance and incomplete dominance
- ✅ Sexual reproduction with crossover and recombination

### Mutation Types
- ✅ Point mutations (silent, missense, nonsense)
- ✅ Insertions and deletions
- ✅ Gene duplications
- ✅ Chromosomal inversions
- ✅ Regulatory mutations

### Sexual Selection
- ✅ Mate choice based on fitness signals
- ✅ Ornament preferences
- ✅ Size preferences
- ✅ Genetic compatibility (MHC-like)
- ✅ Choosiness levels

### Speciation Mechanisms
- ✅ Genetic distance-based species boundaries
- ✅ Automatic speciation detection
- ✅ Extinction tracking
- ✅ Phylogenetic tree construction

### Reproductive Isolation
- ✅ Behavioral isolation
- ✅ Temporal isolation
- ✅ Mechanical isolation
- ✅ Gametic isolation
- ✅ Ecological isolation

### Population Genetics
- ✅ Genetic drift effects
- ✅ Founder effects (through speciation)
- ✅ Bottleneck detection
- ✅ FST calculation
- ✅ Hardy-Weinberg deviation

### Advanced Features
- ✅ Horizontal gene transfer potential
- ✅ Hybridization and hybrid zones
- ✅ Genetic load tracking
- ✅ Purging of deleterious mutations
- ✅ Epigenetics system

## Gene Types

The system tracks 25+ gene types organized by function:

### Physical Traits
- SIZE, SPEED, VISION_RANGE, EFFICIENCY
- METABOLIC_RATE, FERTILITY, MATURATION_RATE

### Color/Display Traits
- COLOR_RED, COLOR_GREEN, COLOR_BLUE
- ORNAMENT_INTENSITY, DISPLAY_FREQUENCY

### Behavioral Traits
- AGGRESSION, SOCIALITY, CURIOSITY, FEAR_RESPONSE

### Mate Preferences
- MATE_SIZE_PREF, MATE_ORNAMENT_PREF, MATE_SIMILARITY_PREF, CHOOSINESS

### Niche Traits
- DIET_SPECIALIZATION, HABITAT_PREFERENCE, ACTIVITY_TIME
- HEAT_TOLERANCE, COLD_TOLERANCE

### Neural Network
- NEURAL_WEIGHT (24 weights for creature brain)

## Usage

### Basic Usage in Simulation

```cpp
#include "entities/genetics/GeneticsManager.h"

// Create genetics manager
genetics::GeneticsManager geneticsManager;

// Initialize with creature population
geneticsManager.initialize(creatures, currentGeneration);

// Update each frame
geneticsManager.update(creatures, currentGeneration, deltaTime);

// Handle reproduction with mate selection
auto offspring = geneticsManager.handleReproduction(
    parent, potentialMates, terrain, generation);

// Get statistics
auto stats = geneticsManager.getStats();
std::cout << "Active species: " << stats.activeSpeciesCount << std::endl;
```

### Creating Creatures with New Genetics

```cpp
#include "entities/genetics/DiploidGenome.h"

// Create new genome
genetics::DiploidGenome genome;

// Create creature with diploid genome
Creature creature(position, genome, CreatureType::HERBIVORE);

// Sexual reproduction
Creature offspring(position, parent1.getDiploidGenome(),
                   parent2.getDiploidGenome(), type);
```

### Accessing Genetic Information

```cpp
// Get traits from diploid genome
float size = creature.getDiploidGenome().getTrait(genetics::GeneType::SIZE);
float speed = creature.getDiploidGenome().getTrait(genetics::GeneType::SPEED);
glm::vec3 color = creature.getDiploidGenome().getColor();

// Get genetic statistics
float heterozygosity = creature.getDiploidGenome().getHeterozygosity();
float geneticLoad = creature.getDiploidGenome().getGeneticLoad();
float inbreeding = creature.getDiploidGenome().calculateInbreedingCoeff();
```

### Exporting Phylogenetic Tree

```cpp
// Export tree in Newick format
geneticsManager.exportPhylogeneticTree("evolution_tree.nwk");

// Get tree as string
std::string newick = geneticsManager.getSpeciationTracker()
                                     .getPhylogeneticTree()
                                     .toNewick();
```

## Configuration

The genetics system is highly configurable:

```cpp
genetics::GeneticsConfig config;
config.baseMutationRate = 0.05f;           // 5% base mutation rate
config.mutationStrength = 0.15f;           // Mutation magnitude
config.speciesDistanceThreshold = 0.15f;   // Genetic distance for speciation
config.minPopulationForSpecies = 10;       // Minimum population size
config.mateSearchRadius = 30.0f;           // How far to search for mates
config.useSexualSelection = true;          // Enable mate choice
config.trackHybridZones = true;            // Track hybrid zones
config.enableEpigenetics = true;           // Environmental effects on genes

geneticsManager.setConfig(config);
```

## Expected Behaviors

### Speciation Events
- Populations that become geographically separated will diverge
- Genetic distance accumulates over generations
- When threshold exceeded, new species is declared
- Phylogenetic tree records all speciation events

### Hybrid Zones
- Where different species overlap, hybrid zones form
- F1 hybrids may have reduced fitness or sterility
- Introgression allows gene flow between species
- Zones can be stable or collapse over time

### Natural Selection
- High genetic load individuals have reduced fitness
- Deleterious alleles are purged over generations
- Beneficial mutations spread through population
- Sexual selection drives ornament evolution

### Genetic Drift
- Small populations experience random allele frequency changes
- Rare alleles may be lost or fixed by chance
- Reduces genetic diversity over time

## Files Modified

- `src/entities/Creature.h` - Added diploid genome member and accessors
- `src/entities/Creature.cpp` - Added new constructors for diploid genomes
- `CMakeLists.txt` - Added new source files

## Future Enhancements

Potential areas for expansion:
- Horizontal gene transfer between species
- Gene regulatory networks
- Quantitative trait loci (QTL) mapping
- Sexual dimorphism
- Genomic imprinting
- Copy number variations
- Transposable elements

## References

See `docs/GENETICS_SPECIATION.md` for the full research document with scientific background and implementation details.
