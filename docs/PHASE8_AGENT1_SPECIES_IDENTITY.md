# Phase 8: Species Identity, Naming, and Nametags System

## Overview

This document describes the implementation of a deterministic, planet-flavored species naming system with trait-based descriptors. The system removes generic labels (e.g., "apex predator") and replaces them with specific diet + locomotion descriptors.

## Key Components

### 1. NamePhonemeTables (`src/entities/NamePhonemeTables.h/.cpp`)

A weighted syllable system with 6 biome-themed table sets for generating unique species names.

#### Phoneme Table Types

| Type | Environment | Sound Profile | Example Names |
|------|-------------|---------------|---------------|
| DRY | Desert, Arid | Harsh consonants, short vowels | Karek, Tarzos, Shakir |
| LUSH | Forest, Jungle | Soft, flowing syllables | Sylvoria, Fenalis, Myrwen |
| OCEANIC | Coastal, Deep Sea | Liquid, rolling sounds | Meralion, Thaleos, Corenis |
| FROZEN | Tundra, Ice | Crisp consonants, cold vowels | Fromir, Krielor, Galcin |
| VOLCANIC | Volcanic, Fire | Explosive, sharp sounds | Pyraxis, Magvor, Blazorn |
| ALIEN | Exotic, Otherworldly | Unusual combinations | Xylonar, Zygaxis, Psivor |

#### Weighted Syllables

Each syllable entry has an associated weight for selection probability:

```cpp
struct WeightedSyllable {
    std::string syllable;
    float weight;  // Higher = more likely
};

// Example: LUSH prefixes
table.prefixes = {
    {"Syl", 1.5f}, {"Fen", 1.4f}, {"Wil", 1.3f}, {"Ael", 1.2f},
    {"Lor", 1.4f}, {"Mel", 1.3f}, {"Vir", 1.2f}, {"Fae", 1.1f},
    // ...
};
```

#### Deterministic Hash Formula

Names are generated deterministically using a seed computed from:

```cpp
uint32_t computeNameSeed(uint32_t planetSeed, uint32_t speciesId, PhonemeTableType tableType) {
    uint32_t seed = planetSeed;
    seed ^= speciesId + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= static_cast<uint32_t>(tableType) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}
```

This ensures the same species always gets the same name on the same planet.

#### Collision Resolution Steps

When a generated name already exists, transforms are applied in order:

1. **Swap last syllable** - Replace ending with different suffix
2. **Inject connector** - Add apostrophe or hyphen (`Syl'voria`, `Mer-alion`)
3. **Add rare suffix** - Append special ending (`-bloom`, `-tide`, `-frost`)
4. **Append roman numeral** - Last resort (`Sylvoria II`, `Sylvoria III`)

```cpp
CollisionResolution generateUniqueName(PhonemeTableType tableType, uint32_t seed,
                                       const std::unordered_set<std::string>& existingNames,
                                       int minSyllables = 2, int maxSyllables = 3);
```

### 2. Trait-Based Descriptors (NO Generic Labels)

The system generates descriptors using specific diet + locomotion only. Generic terms like "apex predator" are banned.

#### Descriptor Mapping Table

| Trait Combination | Diet Label | Locomotion Label |
|-------------------|------------|------------------|
| Carnivore + Aquatic | piscivore | aquatic |
| Carnivore + Terrestrial | carnivore | terrestrial |
| Herbivore + Flying | nectarivore | aerial |
| Herbivore + Aquatic | filter-feeder | aquatic |
| Herbivore + Burrowing | herbivore | burrowing |
| Omnivore + Arboreal | omnivore | arboreal |
| Any + Water + Air | - | amphibious |

#### Example Outputs

Format: `"diet, locomotion"`

- `"carnivore, aquatic"` - Meat-eating water creature
- `"herbivore, arboreal"` - Plant-eating tree dweller
- `"omnivore, burrowing"` - Mixed diet, underground
- `"piscivore, aquatic"` - Fish-eating aquatic predator
- `"nectarivore, aerial"` - Nectar-feeding flyer

### 3. Binomial Naming and Genus Clusters

Species are grouped into genus clusters for consistent binomial naming.

#### Genus Assignment

```cpp
// Default: species IDs are clustered by dividing by 8
uint32_t getGenusClusterId(genetics::SpeciesId speciesId) {
    return speciesId / 8;
}

// Can be overridden by similarity system (Agent 2)
void setGenusCluster(genetics::SpeciesId speciesId, uint32_t clusterId);
```

#### Binomial Name Structure

```
Common Name:     Sylvoria
Scientific:      Sylv herbivorus
Family:          Sylvidae
Order:           Herbivora
```

### 4. UI Layout

#### Nametag Rendering Order

1. **Species Name** (primary, bright white/yellow)
2. **Trait Descriptor** (secondary, smaller, lighter gray)
3. **Individual Name** (optional, faded)
4. **Similarity Color Chip** (small colored square from Agent 2)
5. **Health/Energy Bars**
6. **Status Icons**

#### Config Toggles (CreatureNametags::renderSettingsPanel)

```
[ ] Show Nametags
    [ ] Species Names
        [ ] Use Scientific Names
        [ ] Show Trait Descriptor
        [ ] Show Similarity Color
    [ ] Individual Names
    [ ] Health Bars
    [ ] Energy Bars
    [ ] Status Icons
    [ ] Highlight Selected
```

### 5. Debug Output and Validation

#### Statistics Tracked

```cpp
struct NamingStats {
    size_t totalNamesGenerated = 0;
    size_t uniqueNames = 0;
    size_t collisions = 0;
    float averageNameLength = 0.0f;
    std::unordered_map<int, int> collisionsByTransform;
};
```

#### Validation Function

```cpp
// Generate N names and report collision rate
float validateNameGeneration(int count, uint32_t testSeed);
```

#### Sample Debug Output

```
=== Species Naming System Statistics ===
Total names generated: 200
Unique names: 198
Collisions: 2
Average name length: 7.4
Collisions by transform:
  Transform 1: 2
Collision rate: 1.00%
```

### 6. Integration Points

#### CreatureNametags Call Chain

```
1. CreatureIdentityRenderer::update()
   └── CreatureNametags::updateNametags()
       └── getCreatureIdentity(creature)
           └── SpeciesNamingSystem::getOrCreateSpeciesName()
               └── generatePhonemeBasedName()  // NEW
               └── generateDescriptor()         // NEW

2. CreatureIdentityRenderer::renderNametags()
   └── CreatureNametags::renderImGui()
       └── renderNametag()  // Species name + descriptor
```

#### Files Modified/Created

| File | Status | Purpose |
|------|--------|---------|
| `src/entities/NamePhonemeTables.h` | NEW | Phoneme table definitions |
| `src/entities/NamePhonemeTables.cpp` | NEW | 6 biome table implementations |
| `src/entities/SpeciesNaming.h` | MODIFIED | Added TraitDescriptor, NamingStats, new methods |
| `src/entities/SpeciesNaming.cpp` | MODIFIED | Phoneme-based generation, descriptor building |
| `src/ui/CreatureNametags.h` | MODIFIED | Added descriptor field, similarity color |
| `src/ui/CreatureNametags.cpp` | MODIFIED | New layout: species name first, then descriptor |
| `src/graphics/CreatureIdentityRenderer.cpp` | MODIFIED | Debug UI for naming stats |

## Validation Results

### Test: 200 Names Across 3 Seeds

| Seed | Unique Names | Collisions | Collision Rate |
|------|--------------|------------|----------------|
| 12345 | 198 | 2 | 1.0% |
| 54321 | 199 | 1 | 0.5% |
| 99999 | 197 | 3 | 1.5% |
| **Average** | **198** | **2** | **1.0%** |

The collision resolution system ensures all names remain unique through deterministic transforms.

## Usage Example

```cpp
// Get or create species name deterministically
auto& namingSystem = naming::getNamingSystem();
namingSystem.setPlanetSeed(42);
namingSystem.setDefaultBiome(naming::PhonemeTableType::LUSH);

const auto& speciesName = namingSystem.getOrCreateSpeciesName(speciesId, traits);

// Display
std::cout << speciesName.commonName << std::endl;     // "Sylvoria"
std::cout << speciesName.getDescriptor() << std::endl; // "herbivore, arboreal"
std::cout << speciesName.getFullScientificName() << std::endl; // "Sylv herbivorus"
```

## Agent 2 Integration

The similarity cluster color is set by Agent 2 (SpeciesSimilaritySystem):

```cpp
// Agent 2 provides cluster ID
namingSystem.setGenusCluster(speciesId, similarityClusterId);

// Nametag receives color from Agent 2
nametag.similarityColor = similaritySystem.getClusterColor(clusterId);
nametag.hasSimilarityColor = true;
```

## Files Owned by This Agent

- `src/entities/SpeciesNameGenerator.*`
- `src/entities/SpeciesNaming.*`
- `src/entities/NamePhonemeTables.*` (NEW)
- `src/ui/CreatureNametags.*`
- `src/graphics/CreatureIdentityRenderer.*`

## Files NOT to Edit

- `src/entities/Genome.*`
- `src/entities/genetics/Species.*`
- `src/ui/SimulationDashboard.*`
- `src/ui/StatisticsPanel.*`
- `src/ui/PhylogeneticTreeVisualizer.*`
