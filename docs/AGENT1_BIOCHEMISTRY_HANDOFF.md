# Agent 1 Handoff: Biochemistry Runtime Integration

## Purpose
This document provides specific instructions for Agent 1 to integrate biochemistry penalties into the runtime creature update loop. The biochemistry system is fully implemented in Phase 10; this handoff connects it to the actual creature simulation.

---

## Quick Reference

### What You Need to Do
1. Pass `PlanetChemistry` to creature updates
2. Apply three penalty macros during creature update
3. Add optional logging for debugging

### Files to Modify
- `src/entities/Creature.h` - Add chemistry reference
- `src/entities/Creature.cpp` - Apply penalties in update()
- `src/main.cpp` or `src/managers/CreatureManager.cpp` - Pass chemistry to updates

---

## Step 1: Access PlanetChemistry in Creature Updates

### Option A: Pass as Parameter (Recommended)
Modify the creature update signature to include chemistry:

**In `Creature.h`:**
```cpp
void update(float dt, const PlanetChemistry& chemistry);
```

**In caller (main.cpp or CreatureManager.cpp):**
```cpp
// Access chemistry from world
const PlanetChemistry& chemistry = proceduralWorld.getCurrentWorld()->planetChemistry;

// Update all creatures
for (auto& creature : creatures) {
    creature.update(dt, chemistry);
}
```

### Option B: Store Reference in Creature Class
**In `Creature.h`:**
```cpp
private:
    const PlanetChemistry* m_worldChemistry = nullptr;

public:
    void setWorldChemistry(const PlanetChemistry* chemistry) {
        m_worldChemistry = chemistry;
    }
```

**In creature creation:**
```cpp
creature.setWorldChemistry(&world.planetChemistry);
```

---

## Step 2: Apply Energy Penalty

### Location
In `Creature::update()`, where energy consumption is calculated.

### Code to Add
```cpp
void Creature::update(float dt, const PlanetChemistry& chemistry) {
    // ... existing code ...

    // GET BIOCHEMISTRY ENERGY PENALTY
    float energyPenalty = BIOCHEM_ENERGY_PENALTY(m_genome, chemistry);

    // APPLY TO ENERGY CONSUMPTION
    float baseEnergyUse = calculateBaseEnergyUse();  // or however you calculate it
    m_energy -= baseEnergyUse * energyPenalty * dt;

    // ... rest of update ...
}
```

### Expected Behavior
- Creatures with good chemistry compatibility: `energyPenalty = 1.0` (no change)
- Creatures with poor chemistry: `energyPenalty = 1.2-2.0` (20-100% more energy use)
- Creatures with lethal chemistry: `energyPenalty > 2.0` (rapid energy depletion)

---

## Step 3: Apply Health Penalty

### Location
After energy update, before death check.

### Code to Add
```cpp
void Creature::update(float dt, const PlanetChemistry& chemistry) {
    // ... energy update ...

    // GET BIOCHEMISTRY HEALTH PENALTY
    float healthLoss = BIOCHEM_HEALTH_PENALTY(m_genome, chemistry);

    // APPLY ENVIRONMENTAL DAMAGE
    if (healthLoss > 0.0f) {
        m_health -= healthLoss * dt;

        // Optional: Visual feedback for environmental damage
        if (healthLoss > 1.0f) {
            // Creature is dying from environment (can add visual effect)
            m_environmentalDamageIntensity = std::min(1.0f, healthLoss / 2.0f);
        }
    }

    // ... death check ...
}
```

### Expected Behavior
- Good compatibility: `healthLoss = 0.0` (no environmental damage)
- Poor compatibility: `healthLoss = 0.0-0.5` (slow environmental damage)
- Very poor: `healthLoss = 0.5-2.0` (moderate to fast death)
- Lethal: `healthLoss > 2.0` (rapid death, usually <10 seconds)

---

## Step 4: Apply Reproduction Penalty

### Location
In reproduction check (wherever `canReproduce()` is called or reproduction chance is calculated).

### Code to Add
```cpp
bool Creature::attemptReproduction(const PlanetChemistry& chemistry) {
    // ... existing reproduction checks ...

    // GET BIOCHEMISTRY REPRODUCTION PENALTY
    float reproPenalty = BIOCHEM_REPRO_PENALTY(m_genome, chemistry);

    // APPLY TO REPRODUCTION CHANCE
    float effectiveChance = m_baseReproductionChance * reproPenalty;

    if (Random::chance(effectiveChance)) {
        // Reproduce
        return true;
    }
    return false;
}
```

### Expected Behavior
- Good compatibility: `reproPenalty = 1.0` (normal reproduction)
- Moderate compatibility: `reproPenalty = 0.9-1.0` (slight reduction)
- Poor compatibility: `reproPenalty = 0.6-0.9` (10-40% reduction)
- Very poor: `reproPenalty = 0.2-0.6` (40-80% reduction)
- Lethal: `reproPenalty = 0.0` (cannot reproduce)

---

## Step 5: Performance Optimization (Optional but Recommended)

### For Large Populations
Instead of computing compatibility per creature every frame, use species-level caching:

**In SpeciesManager or CreatureManager:**
```cpp
void updateSpeciesCompatibility(Species& species, const PlanetChemistry& chemistry) {
    // Update once per species per frame (not per creature)
    auto& affinity = BiochemistrySystem::getInstance().getSpeciesAffinity(
        species.id,
        species.representativeGenome,
        chemistry
    );

    // Store cached values
    species.avgEnergyPenalty = affinity.compatibility.energyPenaltyMultiplier;
    species.avgHealthPenalty = affinity.compatibility.healthPenaltyRate;
    species.avgReproPenalty = affinity.compatibility.reproductionPenalty;
}
```

**In Creature::update():**
```cpp
// Use cached species values instead of per-creature computation
float energyPenalty = m_species->avgEnergyPenalty;
float healthLoss = m_species->avgHealthPenalty;
```

**Update species cache:**
```cpp
// In main loop, update species cache every N frames
if (frameCount % 60 == 0) {  // Once per second at 60 FPS
    for (auto& species : allSpecies) {
        updateSpeciesCompatibility(species, world.planetChemistry);
    }
}
```

---

## Step 6: Debugging and Logging (Optional)

### Add Periodic Logging
```cpp
// In main loop or update manager
if (enableChemistryLogging && frameCount % 300 == 0) {  // Every 5 seconds
    BiochemistrySystem::getInstance().logStatistics();
}
```

### Expected Log Output
```
=== Biochemistry Compatibility Statistics ===
  Species cached: 8
  Average compatibility: 0.654
  Minimum compatibility: 0.234
  Distribution:
    Lethal (<0.2):    0
    Poor (0.2-0.4):   2
    Moderate (0.4-0.6): 3
    Good (0.6-0.8):   2
    Excellent (>0.8): 1
============================================
```

### Per-Creature Debug
```cpp
// Optional: Log individual creature compatibility
if (debugCreature && creature.getID() == debugCreatureID) {
    auto compat = BiochemistrySystem::getInstance().computeCompatibility(
        creature.getGenome(),
        chemistry
    );
    std::cout << "Creature #" << creature.getID() << " compatibility: " << compat.overall
              << " (Energy: " << compat.energyPenaltyMultiplier
              << ", Health: " << compat.healthPenaltyRate
              << ", Repro: " << compat.reproductionPenalty << ")\n";
}
```

---

## Macro Reference

All macros are defined in `src/core/BiochemistrySystem.h`:

### BIOCHEM_ENERGY_PENALTY
```cpp
BIOCHEM_ENERGY_PENALTY(genome, chemistry)
```
**Returns**: `float` (1.0-2.0+)
- 1.0 = normal energy consumption
- 1.5 = 50% more energy use
- 2.0+ = double or more energy use

### BIOCHEM_HEALTH_PENALTY
```cpp
BIOCHEM_HEALTH_PENALTY(genome, chemistry)
```
**Returns**: `float` (0.0-2.0+)
- 0.0 = no environmental damage
- 0.5 = slow death over ~100 seconds
- 1.0 = moderate death over ~50 seconds
- 2.0+ = rapid death in <25 seconds

### BIOCHEM_REPRO_PENALTY
```cpp
BIOCHEM_REPRO_PENALTY(genome, chemistry)
```
**Returns**: `float` (0.0-1.0)
- 1.0 = normal reproduction rate
- 0.5 = half reproduction rate
- 0.0 = cannot reproduce

---

## Common Issues and Solutions

### Issue: Creatures die instantly
**Cause**: Lethal compatibility (<0.2)
**Solution**: Check if genomes are being adapted to chemistry at spawn.
**Fix**: Ensure `genome.adaptToChemistry(chemistry)` is called when spawning.

### Issue: No penalties observed
**Cause**: Penalties not applied in update loop
**Solution**: Verify the macro calls are actually executing.
**Fix**: Add temporary logging to confirm macros are called.

### Issue: Frame rate drops
**Cause**: Computing compatibility every frame for every creature
**Solution**: Use species-level caching (see Step 5).
**Fix**: Cache compatibility per species, not per creature.

### Issue: All creatures identical
**Cause**: No variation in chemistry adaptation
**Solution**: Ensure randomness in `adaptToChemistry()`.
**Fix**: Check that random ranges are being applied (±0.1 to ±0.15).

---

## Testing Checklist

After implementing, verify:

- [ ] Creatures with poor chemistry compatibility die faster
- [ ] Well-adapted creatures have longer lifespans
- [ ] Energy drain is visible (poorly-adapted creatures run out of energy faster)
- [ ] Reproduction rates differ between well and poorly adapted species
- [ ] No significant frame rate drop (if using species caching)
- [ ] Log output shows compatibility distribution (some poor, some good)
- [ ] Chemistry log appears at world generation

---

## Expected Behavior Summary

### Earth-Like World (Water, 21% O2, pH 7, 15°C)
- Most creatures: 0.6-0.9 compatibility
- Few deaths from environment
- Reproduction mostly normal

### Toxic World (Sulfuric Acid, 5% O2, pH 2, 80°C)
- Most creatures: 0.2-0.5 compatibility
- Many deaths from environment
- Low reproduction rates
- Evolution pressure to adapt

### Frozen World (Ammonia, 2% O2, pH 8, -80°C)
- Temperature-adapted: 0.6-0.9 compatibility
- Non-adapted: 0.1-0.4 compatibility
- Visible selection for cold adaptation

---

## Code Location Reference

### Required Includes
```cpp
#include "../core/BiochemistrySystem.h"
#include "../environment/PlanetChemistry.h"
```

### Key Methods
- `BiochemistrySystem::getInstance().computeCompatibility()` - Manual computation
- `BiochemistrySystem::getInstance().getSpeciesAffinity()` - Cached species computation
- `BiochemistrySystem::getInstance().clearAllCaches()` - Reset on world change
- `BiochemistrySystem::getInstance().logStatistics()` - Debug logging

---

## Final Notes

1. **Start Simple**: Implement macros first, optimize with caching later if needed
2. **Test on Different Worlds**: Try Earth-like, toxic, and frozen worlds
3. **Watch Evolution**: Over many generations, compatibility should improve
4. **Logging is Your Friend**: Use `logStatistics()` to understand what's happening

Once integrated, the biochemistry system will create meaningful ecological pressure, driving evolution toward better-adapted species and making each planet's chemistry matter for survival.

---

## Questions for Agent 1

If you encounter any issues or need clarification:

1. **Can't find where energy is consumed?**
   - Look for `m_energy -= ...` or similar in update loop

2. **Reproduction system is complex?**
   - Apply penalty to any reproduction probability/chance calculation

3. **Performance issues?**
   - Implement species caching as shown in Step 5

4. **Want visual feedback?**
   - Add color tints, particle effects, or status icons for environmental damage

Good luck with the integration! The biochemistry system is complete and ready for runtime connection.
