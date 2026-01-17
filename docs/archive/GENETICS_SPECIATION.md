# Genetics and Speciation Research Document

## Table of Contents
1. [Genetic Mechanisms](#1-genetic-mechanisms)
2. [Mutation Types](#2-mutation-types)
3. [Sexual Selection and Mate Choice](#3-sexual-selection-and-mate-choice)
4. [Speciation Mechanisms](#4-speciation-mechanisms)
5. [Population Genetics](#5-population-genetics)
6. [Horizontal Gene Transfer and Hybridization](#6-horizontal-gene-transfer-and-hybridization)
7. [Implementation Architecture](#7-implementation-architecture)
8. [References](#8-references)

---

## 1. Genetic Mechanisms

### 1.1 Chromosomes
Chromosomes are thread-like structures in the cell nucleus containing DNA wrapped around histone proteins. Each chromosome contains thousands of genes arranged in a linear sequence.

**Key properties:**
- **Homologous pairs**: Diploid organisms have two copies of each chromosome (one from each parent)
- **Centromere**: Region where sister chromatids are joined; important for cell division
- **Telomeres**: Protective caps at chromosome ends
- **Autosomes vs Sex chromosomes**: Most chromosomes are autosomes; sex chromosomes (X/Y) determine biological sex

**Implementation considerations:**
- Model chromosomes as ordered collections of genes
- Each chromosome has an ID and a set of loci (positions for genes)
- Support variable chromosome numbers per species

### 1.2 Genes
Genes are the fundamental units of heredity - sequences of DNA that encode functional products (usually proteins).

**Gene structure:**
- **Exons**: Coding regions that become part of the final mRNA
- **Introns**: Non-coding regions removed during RNA processing
- **Promoter**: Regulatory region controlling when/where gene is expressed
- **Enhancers/Silencers**: Distant regulatory elements affecting expression levels

**Implementation:**
```cpp
struct Gene {
    uint32_t locus;           // Position on chromosome
    GeneType type;            // What trait this gene affects
    float baseValue;          // Fundamental value before modifications
    float expressionLevel;    // 0.0 to 1.0 - how active the gene is
    bool isCoding;            // Whether this gene produces a functional product
    std::vector<float> regulatoryRegions;  // Promoter/enhancer values
};
```

### 1.3 Alleles
Alleles are different versions of the same gene at a particular locus. They arise through mutation and create genetic variation.

**Types of alleles:**
- **Wild-type**: The most common allele in a population
- **Mutant**: Any allele differing from wild-type
- **Null allele**: Non-functional (loss of function)
- **Gain-of-function**: Produces enhanced or new activity

**Allelic relationships:**
- **Codominance**: Both alleles fully expressed (e.g., AB blood type)
- **Incomplete dominance**: Intermediate phenotype (e.g., pink flowers)
- **Multiple allelism**: More than two alleles exist in population (e.g., coat color)

### 1.4 Dominance Relationships

**Complete dominance:**
- Dominant allele (A) fully masks recessive (a)
- Genotypes AA and Aa produce same phenotype
- Only aa shows recessive phenotype

**Incomplete dominance:**
- Heterozygote shows intermediate phenotype
- Example: Red (RR) x White (WW) = Pink (RW)

**Codominance:**
- Both alleles expressed simultaneously
- Neither masks the other
- Example: Roan cattle (red + white = mixed red/white hairs)

**Dominance coefficient (h):**
- h = 0: Complete dominance (recessive hidden)
- h = 0.5: Additive (no dominance)
- h = 1: Complete dominance (of the other allele)
- Values between represent partial dominance

**Implementation:**
```cpp
struct Allele {
    uint32_t id;              // Unique identifier
    float value;              // The allele's effect on the trait
    float dominanceCoeff;     // 0 = recessive, 0.5 = additive, 1 = dominant
    float fitnessEffect;      // Selection coefficient (-1 to 1)
    bool isDeleterious;       // Whether this is harmful
    MutationType origin;      // How this allele arose
};

float calculatePhenotype(const Allele& a1, const Allele& a2) {
    // General dominance formula
    // P = a1.value + h*(a2.value - a1.value) when a1 is dominant
    float h = (a1.dominanceCoeff + a2.dominanceCoeff) / 2.0f;
    return a1.value * (1-h) + a2.value * h + (a1.value * a2.value * (1-2*h));
}
```

---

## 2. Mutation Types

### 2.1 Point Mutations (Single Nucleotide Polymorphisms)
Changes to single base pairs in DNA sequence.

**Categories:**
- **Transitions**: Purine ↔ Purine (A↔G) or Pyrimidine ↔ Pyrimidine (C↔T)
- **Transversions**: Purine ↔ Pyrimidine (A↔C, A↔T, G↔C, G↔T)

**Effects:**
- **Silent/Synonymous**: No amino acid change (due to codon degeneracy)
- **Missense**: Different amino acid substituted
- **Nonsense**: Creates stop codon, truncates protein
- **Frameshift**: Disrupts reading frame (if indel not multiple of 3)

**Implementation:**
```cpp
enum class PointMutationType {
    SILENT,       // No phenotypic effect
    MISSENSE,     // Small value change
    NONSENSE,     // Large reduction in function
    REGULATORY    // Changes expression level
};

void applyPointMutation(Allele& allele, float mutationStrength) {
    float roll = Random::value();
    if (roll < 0.25f) {
        // Silent - no change
    } else if (roll < 0.70f) {
        // Missense - small change
        allele.value += Random::range(-mutationStrength, mutationStrength);
    } else if (roll < 0.85f) {
        // Nonsense - severe reduction
        allele.value *= Random::range(0.1f, 0.5f);
        allele.isDeleterious = true;
    } else {
        // Regulatory - expression change
        allele.expressionMod += Random::range(-0.3f, 0.3f);
    }
}
```

### 2.2 Insertions
Addition of nucleotides into the DNA sequence.

**Scale:**
- **Single nucleotide**: Often causes frameshift
- **Small insertions (2-50 bp)**: May disrupt gene function
- **Large insertions (transposons)**: Can add new regulatory elements

**Effects:**
- Frameshift if not multiple of 3 in coding region
- May create new binding sites
- Can duplicate regulatory elements

### 2.3 Deletions
Removal of nucleotides from the DNA sequence.

**Effects:**
- Loss of function if critical region deleted
- Frameshift if not multiple of 3
- May unmask recessive alleles (hemizygosity)

**Implementation for insertions/deletions:**
```cpp
void applyIndelMutation(Gene& gene, bool isInsertion) {
    float severity = Random::value();

    if (severity < 0.5f) {
        // Small indel - partial effect
        float modifier = isInsertion ? 1.1f : 0.9f;
        gene.baseValue *= modifier;
    } else if (severity < 0.8f) {
        // Medium indel - significant change
        float modifier = isInsertion ? 1.5f : 0.5f;
        gene.baseValue *= modifier;
        gene.expressionLevel *= 0.7f;
    } else {
        // Large indel - near loss of function
        gene.expressionLevel *= 0.2f;
        gene.isDeleterious = true;
    }
}
```

### 2.4 Duplications
Copying of DNA segments, creating additional copies.

**Types:**
- **Tandem duplication**: Copy adjacent to original
- **Dispersed duplication**: Copy elsewhere in genome
- **Whole gene duplication**: Entire gene copied
- **Segmental duplication**: Multiple genes copied

**Evolutionary importance:**
- Source of new genetic material
- Allows functional divergence
- One copy can mutate while other maintains function
- Examples: Opsins (color vision), Hemoglobin genes, HOX genes

**Implementation:**
```cpp
void applyGeneDuplication(Chromosome& chrom, size_t geneIndex) {
    Gene original = chrom.genes[geneIndex];
    Gene duplicate = original;

    // Duplicate has slightly reduced expression initially
    duplicate.expressionLevel *= 0.8f;

    // Insert after original (tandem) or at random position
    if (Random::chance(0.7f)) {
        chrom.genes.insert(chrom.genes.begin() + geneIndex + 1, duplicate);
    } else {
        size_t randomPos = Random::range(0, (int)chrom.genes.size());
        chrom.genes.insert(chrom.genes.begin() + randomPos, duplicate);
    }
}
```

### 2.5 Inversions
Reversal of a segment of DNA within a chromosome.

**Types:**
- **Paracentric**: Does not include centromere
- **Pericentric**: Includes centromere

**Effects:**
- Generally neutral if breakpoints not in genes
- Suppresses recombination in inverted region
- Important for maintaining gene complexes (supergenes)
- Can cause problems during meiosis (dicentric bridges)

**Evolutionary significance:**
- Creates "supergenes" - linked adaptive alleles
- Example: Butterfly wing patterns, Sparrow morphs

---

## 3. Sexual Selection and Mate Choice

### 3.1 Darwin's Theory of Sexual Selection
Selection arising from differential mating success, separate from natural selection for survival.

**Two mechanisms:**
1. **Intrasexual selection**: Competition within one sex (usually males)
   - Combat, display, sperm competition
   - Leads to weapons: antlers, horns, tusks

2. **Intersexual selection**: Mate choice by one sex (usually females)
   - Courtship displays, ornaments
   - Leads to: peacock tails, bird songs, bright colors

### 3.2 Models of Mate Choice

**Fisherian Runaway Selection:**
- Arbitrary female preference becomes genetically correlated with male trait
- Positive feedback loop drives both to extremes
- Eventually balanced by natural selection costs

**Good Genes (Handicap Principle):**
- Costly ornaments honestly signal genetic quality
- Only healthy males can afford expensive displays
- Females gain indirect benefits (good genes for offspring)

**Direct Benefits:**
- Female chooses based on resources male provides
- Territory quality, parental care, nuptial gifts
- Immediate survival/reproduction advantages

**Sensory Bias:**
- Pre-existing sensory preferences exploited by males
- Example: Preference for orange food items → orange spots on guppies

### 3.3 Fitness Signals

**Honest signals (hard to fake):**
- Body condition (health, nutrition)
- Symmetry (developmental stability)
- Immune system displays (bright colors)
- Complex behaviors (cognitive ability)

**Implementation:**
```cpp
struct FitnessSignal {
    SignalType type;
    float intensity;        // How strong the signal is
    float honestyCost;      // Energy cost to maintain
    float geneticComponent; // How heritable the signal is
};

float calculateMateAttractiveness(const Creature& male, const Creature& female) {
    float attraction = 0.0f;

    // Size-based attraction (often prefer larger)
    attraction += male.genome.getSize() * female.getMatePreference(PREF_SIZE);

    // Ornament display
    attraction += male.getOrnamentIntensity() * female.getMatePreference(PREF_ORNAMENT);

    // Health/condition
    float condition = male.getEnergy() / male.getMaxEnergy();
    attraction += condition * female.getMatePreference(PREF_CONDITION);

    // Genetic compatibility (MHC dissimilarity)
    float geneticDistance = calculateGeneticDistance(male.genome, female.genome);
    attraction += geneticDistance * female.getMatePreference(PREF_DISSIMILARITY);

    return attraction;
}
```

### 3.4 Mate Preferences Evolution

**Genetic basis of preferences:**
- Preferences are themselves heritable
- Can evolve via selection, drift, or linkage
- Often sex-limited in expression

**Implementation:**
```cpp
struct MatePreferences {
    float sizePreference;       // -1 to 1 (smaller to larger)
    float ornamentPreference;   // How much ornaments matter
    float similarityPreference; // Prefer similar (positive) or different (negative)
    float aggressivenessPreference;
    float choosiness;           // How selective (0 = accept all, 1 = very picky)

    // Thresholds
    float minimumAcceptable;    // Won't mate below this threshold
};
```

---

## 4. Speciation Mechanisms

### 4.1 Allopatric Speciation
Geographic separation prevents gene flow, allowing populations to diverge.

**Process:**
1. Geographic barrier divides population
2. Genetic drift and different selection pressures
3. Accumulation of incompatibilities
4. Reproductive isolation develops
5. Populations can no longer interbreed if reunited

**Examples:**
- Darwin's finches (island isolation)
- Grand Canyon squirrels
- Cichlids in different lakes

**Implementation:**
```cpp
// Track geographic regions
struct GeographicRegion {
    int id;
    glm::vec3 center;
    float radius;
    std::vector<Creature*> population;
};

// Barrier detection
bool isBarrierBetween(const glm::vec3& pos1, const glm::vec3& pos2, const Terrain& terrain) {
    // Check for water, mountains, etc.
    int samples = 10;
    for (int i = 1; i < samples; i++) {
        glm::vec3 point = glm::mix(pos1, pos2, i / (float)samples);
        if (terrain.isWater(point.x, point.z) ||
            terrain.getHeight(point.x, point.z) > MOUNTAIN_THRESHOLD) {
            return true;
        }
    }
    return false;
}
```

### 4.2 Sympatric Speciation
Speciation without geographic isolation - divergence within same area.

**Mechanisms:**
- **Ecological specialization**: Different niches (e.g., host plants)
- **Temporal isolation**: Different breeding seasons
- **Sexual selection**: Divergent mating preferences
- **Polyploidy**: Chromosome doubling (common in plants)

**Requirements:**
- Strong disruptive selection
- Assortative mating (like prefers like)
- Linkage between ecological and mate choice genes

**Examples:**
- Cichlids in single lakes
- Apple maggot flies (shifted from hawthorn to apple)
- Rhagoletis host races

**Implementation:**
```cpp
// Ecological niche tracking
struct EcologicalNiche {
    float dietSpecialization;   // 0 = generalist, 1 = specialist
    float preferredHabitat;     // Which terrain type
    float activityTime;         // 0 = nocturnal, 1 = diurnal
};

// Assortative mating function
float getAssortativeMatingBonus(const Creature& c1, const Creature& c2) {
    // Creatures prefer mates with similar niches
    float nicheDistance = calculateNicheDistance(c1.niche, c2.niche);
    float preference = 1.0f - nicheDistance;  // Higher similarity = higher preference
    return preference * ASSORTATIVE_MATING_STRENGTH;
}
```

### 4.3 Parapatric Speciation
Speciation along an environmental gradient with limited gene flow.

**Characteristics:**
- Adjacent populations with different environments
- Some hybridization at contact zone
- Selection against hybrids reinforces divergence

**Process:**
1. Population spans environmental gradient
2. Local adaptation at each end
3. Gene flow maintains some connection
4. Hybrid zone forms with reduced fitness
5. Reinforcement strengthens reproductive isolation

**Examples:**
- Grass species on mine tailings
- Ensatina salamanders ring species

**Implementation:**
```cpp
// Environmental gradient
float getEnvironmentValue(const glm::vec3& position) {
    // Could be temperature, moisture, elevation, etc.
    return terrain.getElevation(position.x, position.z) / MAX_ELEVATION;
}

// Selection coefficient varies with environment
float getLocalSelectionCoefficient(const Creature& c, const glm::vec3& position) {
    float localOptimum = getEnvironmentValue(position);
    float creatureTrait = c.genome.getAdaptationTrait();
    float mismatch = std::abs(localOptimum - creatureTrait);
    return 1.0f - (mismatch * SELECTION_STRENGTH);
}
```

### 4.4 Species Boundaries

**Biological species concept:**
- Species = reproductively isolated groups
- Can interbreed and produce viable, fertile offspring
- Problematic for asexual organisms, fossils

**Genetic distance thresholds:**
```cpp
// Measure genetic distance between individuals
float calculateGeneticDistance(const Genome& g1, const Genome& g2) {
    float distance = 0.0f;
    int comparisons = 0;

    for (size_t i = 0; i < g1.chromosomes.size(); i++) {
        for (size_t j = 0; j < g1.chromosomes[i].genes.size(); j++) {
            // Compare allele values
            float diff1 = std::abs(g1.getAllele1(i,j).value - g2.getAllele1(i,j).value);
            float diff2 = std::abs(g1.getAllele2(i,j).value - g2.getAllele2(i,j).value);
            distance += (diff1 + diff2) / 2.0f;
            comparisons++;
        }
    }

    return distance / comparisons;
}

// Species assignment
const float SPECIES_BOUNDARY_THRESHOLD = 0.15f;  // Calibrate based on observed speciation

bool areSameSpecies(const Genome& g1, const Genome& g2) {
    return calculateGeneticDistance(g1, g2) < SPECIES_BOUNDARY_THRESHOLD;
}
```

---

## 5. Population Genetics

### 5.1 Genetic Drift
Random changes in allele frequencies due to sampling effects in finite populations.

**Properties:**
- Stronger effect in small populations
- Random walk behavior
- Eventually leads to fixation or loss
- Reduces genetic variation

**Wright-Fisher model:**
- Non-overlapping generations
- Random sampling of alleles
- Variance in allele frequency: p(1-p)/(2N)

**Implementation:**
```cpp
// Simulate drift during reproduction
void applyGeneticDrift(Population& pop) {
    int N = pop.size();
    // Variance scales with 1/N
    float driftStrength = 1.0f / std::sqrt((float)N);

    // Random individuals don't reproduce (drift simulation)
    for (auto& creature : pop.creatures) {
        if (Random::value() < driftStrength * 0.1f) {
            creature.preventReproduction = true;
        }
    }
}
```

### 5.2 Founder Effect
Reduced genetic variation when small group establishes new population.

**Characteristics:**
- Subset of original variation
- Random alleles may be over/under-represented
- Can lead to rapid divergence
- Examples: Amish populations, island colonization

**Implementation:**
```cpp
void applyFounderEffect(const std::vector<Creature*>& founders, Species& newSpecies) {
    // Record which alleles made it through the bottleneck
    std::map<uint32_t, int> alleleCount;

    for (const Creature* founder : founders) {
        for (const auto& chrom : founder->genome.chromosomes) {
            for (const auto& gene : chrom.genes) {
                alleleCount[gene.allele1.id]++;
                alleleCount[gene.allele2.id]++;
            }
        }
    }

    // Calculate new allele frequencies
    int totalAlleles = founders.size() * 2;
    for (auto& [alleleId, count] : alleleCount) {
        float newFrequency = (float)count / totalAlleles;
        newSpecies.alleleFrequencies[alleleId] = newFrequency;
    }

    // Many alleles may be lost entirely
    newSpecies.geneticDiversity = calculateDiversity(newSpecies.alleleFrequencies);
}
```

### 5.3 Population Bottlenecks
Dramatic reduction in population size, reducing genetic variation.

**Effects:**
- Loss of rare alleles
- Increased homozygosity
- Increased inbreeding
- May expose deleterious recessives
- Can take many generations to recover variation

**Examples:**
- Cheetahs (very low genetic diversity)
- Northern elephant seals (reduced to ~20)
- European bison

**Implementation:**
```cpp
struct PopulationMetrics {
    int currentSize;
    int historicalMinimum;
    float geneticDiversity;    // Heterozygosity
    float inbreedingCoeff;     // F statistic
    int generationsSinceBottleneck;
};

void detectBottleneck(Population& pop) {
    if (pop.size() < pop.metrics.historicalMinimum) {
        pop.metrics.historicalMinimum = pop.size();
        pop.metrics.generationsSinceBottleneck = 0;

        // Calculate loss of heterozygosity
        float bottleneckSeverity = 1.0f - (float)pop.size() / NORMAL_POPULATION;
        pop.metrics.geneticDiversity *= (1.0f - bottleneckSeverity * 0.5f);

        std::cout << "Population bottleneck detected! Size: " << pop.size()
                  << ", Diversity: " << pop.metrics.geneticDiversity << std::endl;
    }
}
```

### 5.4 Hardy-Weinberg Equilibrium
Expected allele and genotype frequencies without evolution.

**Conditions:**
- No selection
- No mutation
- No migration
- Random mating
- Large population (no drift)

**Equations:**
- p + q = 1 (allele frequencies)
- p² + 2pq + q² = 1 (genotype frequencies)

**Deviations indicate:**
- Selection (if fitness differences)
- Non-random mating (if excess homozygotes)
- Population structure (Wahlund effect)

---

## 6. Horizontal Gene Transfer and Hybridization

### 6.1 Horizontal Gene Transfer (HGT)
Transfer of genetic material between organisms other than parent-offspring.

**Mechanisms:**
- **Transformation**: Uptake of free DNA
- **Transduction**: Transfer via viruses
- **Conjugation**: Direct transfer between cells

**Importance:**
- Major in bacteria (antibiotic resistance)
- Occurs in eukaryotes too (mitochondria, plastids)
- Creates reticulated phylogenies

**Simulation approach:**
```cpp
// Simulate occasional HGT events
void attemptHorizontalTransfer(Creature& recipient, const Creature& donor) {
    // Rare event
    if (!Random::chance(HGT_PROBABILITY)) return;

    // Transfer random gene
    size_t chromIndex = Random::range(0, (int)donor.genome.chromosomes.size());
    size_t geneIndex = Random::range(0, (int)donor.genome.chromosomes[chromIndex].genes.size());

    Gene transferredGene = donor.genome.chromosomes[chromIndex].genes[geneIndex];

    // Integration might fail or cause problems
    if (Random::chance(0.3f)) {
        // Successful integration
        recipient.genome.integrateHGT(transferredGene);
    } else if (Random::chance(0.5f)) {
        // Partial integration - reduced expression
        transferredGene.expressionLevel *= 0.5f;
        recipient.genome.integrateHGT(transferredGene);
    }
    // Otherwise: transfer fails
}
```

### 6.2 Hybridization
Interbreeding between distinct species or populations.

**Outcomes:**
- **Sterile hybrids**: Mules (horse x donkey)
- **Reduced fitness**: Hybrid breakdown
- **Hybrid vigor**: Heterosis (better than parents)
- **Introgression**: Gene flow between species
- **Hybrid speciation**: New species from hybrids

**Barriers to hybridization:**
- Pre-zygotic: Behavioral, mechanical, gametic
- Post-zygotic: Inviability, sterility, breakdown

**Implementation:**
```cpp
struct HybridCompatibility {
    float preMatingBarrier;    // 0 = no barrier, 1 = complete isolation
    float postMatingBarrier;   // Hybrid fitness reduction
    float hybridSterility;     // Probability hybrid is sterile
};

HybridCompatibility calculateHybridCompatibility(const Species& sp1, const Species& sp2) {
    HybridCompatibility compat;

    float geneticDistance = calculateSpeciesDistance(sp1, sp2);

    // Pre-mating barriers increase with genetic distance
    compat.preMatingBarrier = std::min(1.0f, geneticDistance * 2.0f);

    // Post-mating barriers (Dobzhansky-Muller incompatibilities)
    // Increases roughly quadratically with divergence
    compat.postMatingBarrier = std::min(1.0f, geneticDistance * geneticDistance * 5.0f);

    // Sterility (Haldane's rule - heterogametic sex more affected)
    compat.hybridSterility = std::min(1.0f, geneticDistance * 3.0f);

    return compat;
}

// Create hybrid offspring
std::unique_ptr<Creature> createHybrid(const Creature& parent1, const Creature& parent2) {
    auto compat = calculateHybridCompatibility(parent1.species, parent2.species);

    // Check if mating occurs
    if (Random::value() < compat.preMatingBarrier) {
        return nullptr;  // Mating rejected
    }

    // Create hybrid genome
    Genome hybridGenome(parent1.genome, parent2.genome, true);  // isHybrid = true

    auto hybrid = std::make_unique<Creature>(parent1.position, hybridGenome);

    // Apply hybrid fitness costs
    hybrid->fitnessModifier = 1.0f - compat.postMatingBarrier;
    hybrid->isSterile = Random::value() < compat.hybridSterility;
    hybrid->isHybrid = true;

    return hybrid;
}
```

### 6.3 Hybrid Zones
Geographic areas where hybridization occurs between divergent populations.

**Types:**
- **Tension zones**: Maintained by selection against hybrids + dispersal
- **Bounded hybrid superiority**: Hybrids favored in intermediate environment
- **Mosaic zones**: Patchy environment, parental types in different patches

**Dynamics:**
- Width depends on dispersal and selection
- Can be stable for long periods
- Source of gene flow (introgression)
- May lead to reinforcement or fusion

---

## 7. Implementation Architecture

### 7.1 Core Classes

```cpp
// Forward declarations
class Gene;
class Allele;
class Chromosome;
class DiploidGenome;
class Species;
class PhylogeneticTree;

// Allele - variant of a gene
class Allele {
public:
    uint32_t id;
    float value;
    float dominanceCoeff;     // 0-1, affects phenotype calculation
    float fitnessEffect;      // Selection coefficient
    bool isDeleterious;
    MutationType origin;

    static uint32_t nextId;

    Allele() : id(nextId++), value(0.0f), dominanceCoeff(0.5f),
               fitnessEffect(0.0f), isDeleterious(false), origin(MutationType::NONE) {}

    Allele mutate(float strength) const;
};

// Gene - a locus with two alleles (diploid)
class Gene {
public:
    uint32_t locus;
    GeneType type;
    Allele allele1, allele2;  // Maternal and paternal
    float expressionLevel;
    std::vector<EpigeneticMark> epigeneticMarks;

    float getPhenotype() const;
    bool isHomozygous() const;
    float getHeterozygosity() const;
};

// Chromosome - collection of genes
class Chromosome {
public:
    uint32_t id;
    std::vector<Gene> genes;
    float recombinationRate;  // cM/Mb (centimorgans per megabase)

    // Crossover during meiosis
    std::pair<Chromosome, Chromosome> recombine(const Chromosome& other) const;

    // Mutation operators
    void applyPointMutation(size_t geneIndex, float strength);
    void applyInsertion(size_t position, const Gene& newGene);
    void applyDeletion(size_t geneIndex);
    void applyDuplication(size_t geneIndex);
    void applyInversion(size_t start, size_t end);
};

// DiploidGenome - full genome with chromosome pairs
class DiploidGenome {
public:
    std::vector<std::pair<Chromosome, Chromosome>> chromosomePairs;
    SpeciesId speciesId;
    uint64_t lineageId;

    // Sexual reproduction
    static DiploidGenome createOffspring(const DiploidGenome& parent1,
                                          const DiploidGenome& parent2);

    // Mutation
    void mutate(float mutationRate, float mutationStrength);

    // Phenotype extraction
    float getTrait(GeneType type) const;

    // Genetic distance
    float distanceTo(const DiploidGenome& other) const;

    // Inbreeding coefficient
    float calculateInbreedingCoeff() const;
};
```

### 7.2 Species and Speciation Tracking

```cpp
class Species {
public:
    SpeciesId id;
    std::string name;
    uint64_t foundingLineage;
    int foundingGeneration;

    // Population genetics
    std::map<uint32_t, float> alleleFrequencies;
    float averageHeterozygosity;
    float effectivePopulationSize;

    // Reproductive isolation
    std::map<SpeciesId, float> reproductiveIsolation;

    // Niche
    EcologicalNiche niche;

    void updateStatistics(const std::vector<Creature*>& members);
    bool canInterbreedWith(const Species& other) const;
};

class SpeciationTracker {
public:
    std::vector<Species> activeSpecies;
    std::vector<Species> extinctSpecies;

    // Clustering algorithm to detect species boundaries
    void updateSpeciesAssignments(std::vector<Creature*>& creatures);

    // Detect speciation events
    void checkForSpeciation();

    // Detect extinction
    void checkForExtinction();

private:
    float speciesThreshold = 0.15f;  // Genetic distance threshold

    // UPGMA or similar clustering
    std::vector<std::vector<float>> buildDistanceMatrix(
        const std::vector<Creature*>& creatures);
    std::vector<int> clusterByDistance(
        const std::vector<std::vector<float>>& distances);
};
```

### 7.3 Phylogenetic Tree

```cpp
struct PhyloNode {
    uint64_t id;
    uint64_t parentId;
    SpeciesId speciesId;
    int generation;
    float branchLength;
    std::vector<uint64_t> childrenIds;
    bool isExtant;

    // Diagnostic info
    DiploidGenome representativeGenome;
    glm::vec3 color;  // For visualization
};

class PhylogeneticTree {
public:
    std::map<uint64_t, PhyloNode> nodes;
    uint64_t rootId;

    // Tree construction
    void addSpeciation(SpeciesId parentSpecies, SpeciesId childSpecies, int generation);
    void markExtinction(SpeciesId species, int generation);

    // Tree queries
    SpeciesId getMostRecentCommonAncestor(SpeciesId sp1, SpeciesId sp2) const;
    float getEvolutionaryDistance(SpeciesId sp1, SpeciesId sp2) const;
    std::vector<SpeciesId> getDescendants(SpeciesId ancestor) const;

    // Visualization
    void exportNewick(const std::string& filename) const;
    void render(/* rendering context */) const;
};
```

### 7.4 Mate Selection System

```cpp
class MateSelector {
public:
    // Find potential mates
    std::vector<Creature*> findPotentialMates(
        const Creature& seeker,
        const std::vector<Creature*>& candidates,
        float searchRadius);

    // Evaluate mate quality
    float evaluateMate(const Creature& chooser, const Creature& candidate);

    // Select best mate (or none if none acceptable)
    Creature* selectMate(
        const Creature& chooser,
        const std::vector<Creature*>& potentialMates);

private:
    // Preference-based evaluation
    float evaluateByPreferences(const Creature& chooser, const Creature& candidate);

    // Genetic compatibility
    float evaluateGeneticCompatibility(const DiploidGenome& g1, const DiploidGenome& g2);

    // Species compatibility
    bool areReproductivelyCompatible(const Creature& c1, const Creature& c2);
};
```

### 7.5 Epigenetics System

```cpp
enum class EpigeneticMarkType {
    METHYLATION,      // DNA methylation (usually silencing)
    ACETYLATION,      // Histone acetylation (usually activating)
    PHOSPHORYLATION,  // Signal response
    IMPRINTING        // Parent-of-origin effect
};

struct EpigeneticMark {
    EpigeneticMarkType type;
    float intensity;       // 0-1 strength of modification
    int generationsRemaining;  // How long mark persists
    bool isHeritable;
};

class EpigeneticsSystem {
public:
    // Environmental factors that can cause epigenetic changes
    void processEnvironmentalStress(Creature& creature, float stressLevel);
    void processNutritionLevel(Creature& creature, float nutrition);
    void processTemperature(Creature& creature, float temperature);

    // Inheritance of epigenetic marks
    void inheritMarks(const Creature& parent, Creature& offspring);

    // Effect on gene expression
    float calculateExpressionModifier(const Gene& gene);

private:
    float stressThreshold = 0.7f;
    float nutritionThreshold = 0.3f;
};
```

### 7.6 Genetic Load System

```cpp
class GeneticLoadTracker {
public:
    // Calculate genetic load for an individual
    float calculateGeneticLoad(const DiploidGenome& genome);

    // Population-level load
    float calculatePopulationLoad(const std::vector<Creature*>& population);

    // Mutation-selection balance
    void updateMutationLoad(Population& pop, float mutationRate);

    // Purging of deleterious mutations
    void simulatePurging(Population& pop, int generations);

private:
    // Count deleterious alleles
    int countDeleteriousAlleles(const DiploidGenome& genome);

    // Fitness reduction per deleterious allele
    float deleteriousFitnessEffect = 0.02f;  // 2% per allele

    // Partial dominance of deleterious alleles
    float deleteriousDominance = 0.2f;  // Mostly recessive
};
```

---

## 8. References

### Scientific Literature

1. **Futuyma, D.J. & Kirkpatrick, M. (2017)** - *Evolution* (4th ed.). Sinauer Associates.
   - Comprehensive evolutionary biology textbook

2. **Coyne, J.A. & Orr, H.A. (2004)** - *Speciation*. Sinauer Associates.
   - Definitive work on speciation mechanisms

3. **Lynch, M. & Walsh, B. (1998)** - *Genetics and Analysis of Quantitative Traits*. Sinauer.
   - Mathematical foundations of quantitative genetics

4. **Hartl, D.L. & Clark, A.G. (2007)** - *Principles of Population Genetics* (4th ed.). Sinauer.
   - Population genetics theory and applications

5. **Andersson, M. (1994)** - *Sexual Selection*. Princeton University Press.
   - Comprehensive review of sexual selection theory

### Key Papers

- **Dobzhansky, T. (1937)** - Genetics and the Origin of Species
- **Mayr, E. (1942)** - Systematics and the Origin of Species
- **Fisher, R.A. (1930)** - The Genetical Theory of Natural Selection
- **Wright, S. (1931)** - Evolution in Mendelian Populations
- **Kimura, M. (1968)** - Evolutionary rate at the molecular level

### Implementation Resources

- **NEAT (NeuroEvolution of Augmenting Topologies)** - Stanley & Miikkulainen, 2002
- **Avida Digital Evolution Platform** - Ofria & Wilke, 2004
- **EcoSim Ecosystem Simulation** - Various authors

---

## Appendix A: Genetic Distance Metrics

### Nei's Genetic Distance
```cpp
float neisDistance(const Population& pop1, const Population& pop2) {
    float Jx = 0, Jy = 0, Jxy = 0;

    for (const auto& [allele, freq1] : pop1.alleleFrequencies) {
        Jx += freq1 * freq1;
        if (pop2.alleleFrequencies.count(allele)) {
            Jxy += freq1 * pop2.alleleFrequencies.at(allele);
        }
    }
    for (const auto& [allele, freq2] : pop2.alleleFrequencies) {
        Jy += freq2 * freq2;
    }

    float I = Jxy / std::sqrt(Jx * Jy);
    return -std::log(I);
}
```

### FST (Fixation Index)
```cpp
float calculateFst(const std::vector<Population>& subpopulations) {
    // Hs = average heterozygosity within subpopulations
    // Ht = total heterozygosity (if all combined)
    // FST = (Ht - Hs) / Ht

    float Hs = 0;
    for (const auto& pop : subpopulations) {
        Hs += pop.heterozygosity / subpopulations.size();
    }

    float Ht = calculateTotalHeterozygosity(subpopulations);

    return (Ht - Hs) / Ht;
}
```

---

## Appendix B: Configuration Constants

```cpp
namespace GeneticConstants {
    // Mutation rates (per gene per generation)
    constexpr float POINT_MUTATION_RATE = 0.001f;
    constexpr float INSERTION_RATE = 0.0001f;
    constexpr float DELETION_RATE = 0.0001f;
    constexpr float DUPLICATION_RATE = 0.00005f;
    constexpr float INVERSION_RATE = 0.00001f;

    // Recombination
    constexpr float CROSSOVER_RATE = 0.01f;  // Per base pair
    constexpr int EXPECTED_CROSSOVERS_PER_CHROMOSOME = 2;

    // Speciation
    constexpr float SPECIES_THRESHOLD = 0.15f;
    constexpr int MIN_POPULATION_FOR_SPECIES = 10;
    constexpr int GENERATIONS_FOR_SPECIATION = 50;

    // Selection
    constexpr float SELECTION_STRENGTH = 0.1f;
    constexpr float SEXUAL_SELECTION_STRENGTH = 0.2f;

    // Epigenetics
    constexpr float EPIGENETIC_INHERITANCE_RATE = 0.3f;
    constexpr int MAX_EPIGENETIC_GENERATIONS = 3;

    // Hybridization
    constexpr float HYBRID_FITNESS_PENALTY = 0.2f;
    constexpr float HYBRID_STERILITY_THRESHOLD = 0.3f;
}
```

---

*Document version: 1.0*
*Last updated: Session date*
*Author: Claude AI Assistant*
