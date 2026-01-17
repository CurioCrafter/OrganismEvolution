# Phase 11 Agent 4 Handoff Notes

## Integration Required

The creature variety generation system has been implemented in the genome layer, but **integration with the spawning system is required** to activate it.

### Files That Need Integration

The following files spawn creatures and would benefit from using `initializeForBiome()`:

1. **[src/main.cpp](../src/main.cpp)** - Lines 165, 235, 243, 256
   - `InitializeGenomeForType()` currently calls type-specific randomizers
   - Should query biome at spawn position and call `genome.initializeForBiome(biome, chemistry, varietySeed)`

2. **[src/core/CreatureManager.cpp](../src/core/CreatureManager.cpp)** (if exists)
   - Creature spawning logic may exist here
   - Needs biome awareness for spawn variety

3. **[src/ui/*](../src/ui/)** - Any UI that spawns creatures
   - Debug spawn tools
   - Population controls

### Integration Steps

To integrate the variety system:

```cpp
// OLD CODE (in InitializeGenomeForType or spawn functions):
void InitializeGenomeForType(Genome& genome, CreatureType type) {
    if (baseType == CreatureType::AQUATIC) {
        genome.randomizeAquatic();
    } else if (baseType == CreatureType::FLYING) {
        genome.randomizeBird();
    }
    // etc...
}

// NEW CODE (recommended):
void InitializeGenomeForType(Genome& genome, CreatureType type,
                            const glm::vec3& spawnPosition,
                            const BiomeSystem* biomeSystem,
                            const PlanetChemistry& chemistry) {
    // Query biome at spawn position
    BiomeType biome = biomeSystem->getBiomeAt(spawnPosition);

    // Use variety seed based on position for deterministic variety
    float varietySeed = glm::fract(spawnPosition.x * 12.9898f + spawnPosition.z * 78.233f);

    // Initialize with biome-appropriate variety
    genome.initializeForBiome(biome, chemistry, varietySeed);

    // Note: Type is now inferred from biome, not passed in
    // The biome determines whether creatures are aquatic, flying, or land-based
}
```

### Required Changes

1. **Add BiomeSystem parameter** to spawn functions
2. **Add PlanetChemistry parameter** to spawn functions (or use global)
3. **Query biome** at spawn position before genome initialization
4. **Call `initializeForBiome()`** instead of type-specific randomizers

### Diversity Logging (Optional but Recommended)

To validate variety is working:

```cpp
// After spawning initial population
std::vector<Genome> genomes;
for (auto* creature : world.activeCreatures) {
    genomes.push_back(creature->genome);
}

auto metrics = Genome::calculatePopulationDiversity(genomes);
LOG_INFO("Population diversity: %.3f (size: %.3f, color: %.3f, morph: %.3f)",
         metrics.overallDiversity, metrics.sizeVariance,
         metrics.colorVariance, metrics.morphologyVariance);

// Expected: diversity > 0.3 for good variety
if (metrics.overallDiversity < 0.3f) {
    LOG_WARN("Low population diversity detected!");
}
```

### Integration Dependencies

The following systems must be accessible from spawn code:

- **BiomeSystem** - For `getBiomeAt(position)` queries
- **PlanetChemistry** - Current planet's chemistry (may already be global)
- **Random** - Already available (used internally)

### Backward Compatibility

The current `initializeForPreset()` and `initializeForRegion()` functions remain unchanged and continue to work. The new `initializeForBiome()` is an additional option.

### Testing Recommendations

1. Spawn 50-100 creatures in one biome
2. Calculate diversity metrics
3. Verify diversity score > 0.3
4. Visual inspection: at least 3 distinct body archetypes
5. Run 5-10 minutes to ensure no immediate die-offs

## API Summary for Integration

### New Public Functions

```cpp
// In Genome.h/.cpp
void Genome::initializeForBiome(BiomeType biome,
                               const PlanetChemistry& chemistry,
                               float varietySeed = 0.0f);

static Genome::DiversityMetrics
Genome::calculatePopulationDiversity(const std::vector<Genome>& population);
```

### Usage Pattern

```cpp
// At spawn location
BiomeType biome = biomeSystem->getBiomeAt(spawnPosition);
float varietySeed = Random::value();  // Or position-based

Genome genome;
genome.initializeForBiome(biome, planetChemistry, varietySeed);

SimCreature* creature = new SimCreature(spawnPosition, genome);
```

## Questions for Integration Team

1. **Where is BiomeSystem instantiated?** Need reference to query biome at position
2. **Is PlanetChemistry global or per-island?** Affects how to pass to genome init
3. **Should variety be enabled for all spawns or just initial population?** Both work, but different implications
4. **Do we want to log diversity metrics?** Helpful for validation but adds logging overhead

## Known Limitations

1. **No climate integration yet** - Variety is biome-based only, not temperature/moisture aware
2. **No niche tracking** - System doesn't avoid spawning duplicate niches
3. **Type vs Biome conflict** - Current system uses `CreatureType`, new system infers from biome
   - May need mapping: BiomeType + Bias → CreatureType

## Contact

For questions about this implementation, see:
- [PHASE11_AGENT4_CREATURE_VARIETY.md](PHASE11_AGENT4_CREATURE_VARIETY.md) - Full documentation
- [src/entities/Genome.h](../src/entities/Genome.h) - API reference
- [src/entities/Genome.cpp](../src/entities/Genome.cpp) - Implementation

---

**Handoff Date**: 2026-01-17
**From**: Phase 11 Agent 4
**Status**: Awaiting integration
