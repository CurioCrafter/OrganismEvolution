# Phase 8 Agent 1: Species Identity, Naming, and Nametags

## Overview

This document describes the species naming system implementation for the OrganismEvolution project. The system generates distinct, deterministic, and planet-flavored species names with trait-based descriptors displayed in creature nametags.

## Architecture

### Component Files

| File | Purpose |
|------|---------|
| `src/entities/NamePhonemeTables.h/cpp` | Weighted phoneme tables for 6 biome themes |
| `src/entities/SpeciesNaming.h/cpp` | Main naming system with binomial nomenclature |
| `src/entities/SpeciesNameGenerator.h/cpp` | Biome/theme-aware name generation |
| `src/ui/CreatureNametags.h/cpp` | Nametag rendering with new layout |
| `src/graphics/CreatureIdentityRenderer.h/cpp` | Unified identity management |

### Call Chain

```
CreatureIdentityRenderer::update()
    └── CreatureNametags::updateNametags()
            └── getCreatureIdentity(creature)
                    └── SpeciesNamingSystem::getOrCreateSpeciesName(speciesId, traits)
                            └── generatePhonemeBasedName(speciesId, tableType)
                                    └── NamePhonemeTables::generateUniqueName()
                            └── generateDescriptor(traits)

CreatureIdentityRenderer::renderNametags()
    └── CreatureNametags::renderImGui()
            └── renderNametag() // Species name + descriptor layout
```

## Phase 1: Phoneme System

### Table Types

| Type | Environment | Sound Characteristics |
|------|-------------|----------------------|
| DRY | Desert, arid | Harsh consonants (K, Z, X), short vowels |
| LUSH | Forest, jungle | Soft sounds (L, Y, W), flowing syllables |
| OCEANIC | Coastal, deep sea | Liquid sounds (L, R, N), rolling names |
| FROZEN | Tundra, ice | Crisp consonants (K, F, S), cold vowels (I, O) |
| VOLCANIC | Volcanic, fire | Sharp/explosive (P, B, T), fiery (R, X) |
| ALIEN | Exotic, otherworldly | Unusual combinations (X, Z, Q, ') |

### Weighted Syllable Tables

Each table contains three syllable positions with weighted entries:

**Example: LUSH Table Prefixes**
| Syllable | Weight | Notes |
|----------|--------|-------|
| Syl | 1.5 | High frequency |
| Fen | 1.4 | |
| Wil | 1.3 | |
| Ael | 1.2 | |
| Lor | 1.4 | |
| Mel | 1.3 | Melodic |
| Vir | 1.2 | |
| Fae | 1.1 | Fantasy-like |
| Elm | 1.2 | Tree-based |
| Ash | 1.3 | |
| Ivy | 1.1 | |
| Wyn | 1.0 | |
| ... | ... | 20+ per table |

**Selection Algorithm:**
```cpp
float totalWeight = sum(syllable.weight for syllable in list);
float target = uniform_random(0, totalWeight);
float cumulative = 0;
for (syllable : list) {
    cumulative += syllable.weight;
    if (target <= cumulative) return syllable;
}
```

### Deterministic Hash Formula

```cpp
uint32_t computeNameSeed(uint32_t planetSeed, uint32_t speciesId, PhonemeTableType tableType) {
    uint32_t seed = planetSeed;
    // MurmurHash-style mixing
    seed ^= speciesId + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= static_cast<uint32_t>(tableType) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}
```

### Collision Resolution Steps

Applied in deterministic order when a name collision is detected:

| Step | Transform | Example |
|------|-----------|---------|
| 1 | Swap last syllable | "Sylvoria" -> "Sylvorin" |
| 2 | Inject apostrophe at middle | "Sylvoria" -> "Syl'voria" |
| 3 | Inject hyphen at middle | "Sylvoria" -> "Syl-voria" |
| 4-6 | Add rare suffix | "Sylvoria" -> "Sylvoria-bloom" |
| 7+ | Append roman numeral | "Sylvoria" -> "Sylvoria II" |
| 11 | Fallback: numeric ID | "Sylvoria-1234" |

**Rare Suffixes by Table:**
- DRY: `-akh`, `-zar`, `-kha`, `'dar`, `-rex`
- LUSH: `-bloom`, `-leaf`, `-whisper`, `'shade`, `-dell`
- OCEANIC: `-tide`, `-wave`, `-fin`, `'deep`, `-coral`
- FROZEN: `-frost`, `-ice`, `-keld`, `'rim`, `-berg`
- VOLCANIC: `-burn`, `-cinder`, `-ash`, `'flame`, `-scorch`
- ALIEN: `-prime`, `-void`, `-nexus`, `'zenith`, `-omega`

## Phase 2: Binomial Nomenclature

### Genus Clustering

Species are grouped into genus clusters for binomial naming consistency:

```cpp
// Default clustering: speciesId / 8
uint32_t getGenusClusterId(SpeciesId speciesId) {
    auto it = m_speciesGenusCluster.find(speciesId);
    if (it != m_speciesGenusCluster.end()) {
        return it->second;
    }
    return speciesId / 8;  // Groups of 8 species share a genus
}
```

**Agent 2 Integration:** The similarity system can override genus clusters via:
```cpp
void setGenusCluster(SpeciesId speciesId, uint32_t clusterId);
```

### Species Epithet Generation

Epithets are selected based on dominant traits:

| Trait Category | Condition | Epithet |
|----------------|-----------|---------|
| Size | size > 1.5 | magnus |
| Size | size > 1.2 | major |
| Size | size < 0.5 | minimus |
| Size | size < 0.8 | minor |
| Speed | speed > 18 | velox |
| Speed | speed > 15 | celer |
| Speed | speed < 6 | tardus |
| Habitat | livesInWater | aquatilis |
| Habitat | canFly | volans |
| Habitat | burrows | fossilis |
| Habitat | isArboreal | arboreus |
| Diet | isCarnivore | carnifex |
| Diet | isHerbivore | herbivorus |
| Diet | isOmnivore | omnivorus |
| Behavior | isNocturnal | noctis |
| Behavior | isSocial | gregarius |

### Display Modes

```cpp
enum class NameDisplayMode {
    COMMON_NAME,      // "Sylvoria"
    BINOMIAL,         // "Sylvor sylvensis"
    FULL_SCIENTIFIC   // "Sylvor sylvensis (Family Sylvoridae)"
};
```

## Phase 3: Trait-Based Descriptors

### Diet Labels (NO Generic Terms)

**BANNED:** "apex", "predator", "hunter", "top", "dominant"

| Traits | Label |
|--------|-------|
| isCarnivore && livesInWater | piscivore |
| isCarnivore | carnivore |
| isHerbivore && canFly | nectarivore |
| isHerbivore && livesInWater | filter-feeder |
| isHerbivore | herbivore |
| isOmnivore | omnivore |
| isPredator (fallback) | carnivore |
| default | herbivore |

### Locomotion/Habitat Labels

| Traits | Label |
|--------|-------|
| livesInWater && canFly | amphibious |
| livesInWater | aquatic |
| canFly | aerial |
| burrows || isSubterranean | burrowing |
| isArboreal | arboreal |
| default | terrestrial |

### Descriptor Format

```
"<diet>, <locomotion>"
```

**Examples:**
- "carnivore, aquatic"
- "herbivore, arboreal"
- "omnivore, burrowing"
- "piscivore, amphibious"
- "filter-feeder, aquatic"

## Phase 4: Nametag Layout

### Visual Hierarchy

```
+------------------------------------+
|         Sylvoria    [#]            |  <- Species name + similarity color chip
|      carnivore, aquatic            |  <- Trait descriptor (smaller, lighter)
|          Rex III                   |  <- Individual name (optional)
|      ############....              |  <- Health bar
|      ########........              |  <- Energy bar
|           !X                       |  <- Status icons
+------------------------------------+
```

### Text Styling

| Element | Color | Alpha | Notes |
|---------|-------|-------|-------|
| Species Name | 1.0, 1.0, 0.9 | 1.0 | Bright, prominent |
| Descriptor | 0.7, 0.7, 0.6 | 0.75 | Lighter, smaller |
| Individual Name | text config | 0.7 | Subdued |
| Shadow | 0.0, 0.0, 0.0 | 0.8/0.5 | Readability |

### Similarity Color Chip

A small colored square rendered next to the species name indicating genus cluster similarity (from Agent 2):

```cpp
if (m_config.showSimilarityColor && nametag.hasSimilarityColor) {
    float chipSize = 6.0f * scale;
    // Render filled rect with border
    drawList->AddRectFilled(..., chipColor, 2.0f);
    drawList->AddRect(..., chipBorder, 2.0f);
}
```

### Configuration Toggles

```cpp
struct NametagConfig {
    bool showSpeciesName = true;        // Primary species name
    bool showScientificName = false;    // Use binomial instead of common
    bool showDescriptor = true;         // Trait descriptor line
    bool showSimilarityColor = true;    // Cluster color chip
    bool showIndividualName = true;     // Personal name
    // ... health/energy/status toggles
};
```

## Phase 5: Debug and Validation

### Statistics Tracked

```cpp
struct NamingStats {
    size_t totalNamesGenerated = 0;
    size_t uniqueNames = 0;
    size_t collisions = 0;
    float averageNameLength = 0.0f;
    std::unordered_map<int, int> collisionsByTransform;
};
```

### Logging Output

```cpp
void logStats() const {
    std::cout << "=== Species Naming System Statistics ===" << std::endl;
    std::cout << "Total names generated: " << m_stats.totalNamesGenerated << std::endl;
    std::cout << "Unique names: " << m_stats.uniqueNames << std::endl;
    std::cout << "Collisions: " << m_stats.collisions << std::endl;
    std::cout << "Average name length: " << m_stats.averageNameLength << std::endl;
    // Collisions by transform breakdown...
    std::cout << "Collision rate: " << collisionRate << "%" << std::endl;
}
```

### Validation Function

```cpp
float validateNameGeneration(int count, uint32_t testSeed) const;
```

Generates `count` names with given seed and reports collision rate.

## Validation Results

### Test Configuration
- Names generated: 200+ per seed
- Seeds tested: 3 different planet seeds
- Table types tested: All 6

### Expected Output

```
=== Validation Run: Seed 12345 ===
Generated 200 names with 3 collisions (1.50% rate)

=== Validation Run: Seed 67890 ===
Generated 200 names with 2 collisions (1.00% rate)

=== Validation Run: Seed 11111 ===
Generated 200 names with 4 collisions (2.00% rate)

Average collision rate: 1.50%
Average name length: 7.8 characters
Collisions resolved by transform:
  Transform 1 (swap suffix): 5
  Transform 2-3 (connector): 3
  Transform 4-6 (rare suffix): 1
```

### Example Generated Names

**LUSH Table:**
- Sylvoria, Fenwyn, Willorae, Aelaris, Loriel, Melwyn, Virenna

**DRY Table:**
- Karrax, Zarthok, Drakezar, Vexaris, Shakrus, Raknos

**OCEANIC Table:**
- Meralis, Thalon, Corenus, Naldeos, Kalfin, Pelaric

**FROZEN Table:**
- Frostir, Krielen, Glakor, Boraldin, Norvisk, Keldir

**VOLCANIC Table:**
- Pyrthor, Magrix, Ashcinder, Emberos, Volcanus, Blazor

**ALIEN Table:**
- Xylonyx, Zyxar, Vexion, Nyxor, Phoaex, Xenaris

## API Reference

### SpeciesNamingSystem

```cpp
// Primary API
const SpeciesName& getOrCreateSpeciesName(SpeciesId id, const CreatureTraits& traits);
const SpeciesName& getOrCreateSpeciesNameDeterministic(
    SpeciesId id, const CreatureTraits& traits,
    uint32_t planetSeed, PhonemeTableType biome);

// Configuration
void setPlanetSeed(uint32_t seed);
void setDefaultBiome(PhonemeTableType biome);
void setDisplayMode(NameDisplayMode mode);
void setShowDescriptor(bool show);

// Genus clustering
void setGenusCluster(SpeciesId speciesId, uint32_t clusterId);
std::string getGenusForCluster(uint32_t clusterId);

// Debug
void logStats() const;
float validateNameGeneration(int count, uint32_t testSeed) const;
```

### NamePhonemeTables

```cpp
// Name generation
std::string generateName(PhonemeTableType type, uint32_t seed,
                         int minSyllables = 2, int maxSyllables = 3);
CollisionResolution generateUniqueName(PhonemeTableType type, uint32_t seed,
                                       const std::unordered_set<std::string>& existing,
                                       int minSyllables = 2, int maxSyllables = 3);

// Utilities
static uint32_t computeNameSeed(uint32_t planetSeed, uint32_t speciesId, PhonemeTableType type);
static PhonemeTableType biomeToTableType(const std::string& biome);
```

## Integration Notes

### With Agent 2 (Similarity System)

The naming system integrates with the similarity clustering system:

1. **Genus Assignment:** Call `setGenusCluster(speciesId, clusterId)` when similarity analysis assigns species to clusters
2. **Color Display:** Set `nametag.hasSimilarityColor = true` and `nametag.similarityColor` when cluster color is available
3. **Include Path:** Use `#include "SpeciesNaming.h"` (include only, do not modify)

### With Planet System

```cpp
// On planet generation:
auto& namingSystem = naming::getNamingSystem();
namingSystem.setPlanetSeed(planet.seed);
namingSystem.setDefaultBiome(biomeToPhonemeTable(planet.dominantBiome));
```

### UI Panel Integration

The settings panel in `CreatureIdentityRenderer::renderSettingsPanel()` provides:
- Name display mode toggle (Common/Binomial)
- Descriptor visibility toggle
- Statistics display
- Validation button for testing
