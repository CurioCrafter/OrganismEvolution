# Phase 8 Agent 5: Insect Scale and Micro-Fauna Fixes

## Summary

Fixed issues where insects were appearing too large and getting stuck on trees. Implemented proper size calibration, habitat-based spawn distribution, and visual readability improvements.

## Issues Addressed

### Problem 1: Insects Appearing Huge
- **Root Cause**: The scaling formula `props.minSize * creature.genome->size` used only `minSize` without proper interpolation. Point sprites had a `* 10.0f` visibility hack that made distant insects appear massive.
- **Solution**: Changed to proper size interpolation between `minSize` and `maxSize` based on genome, with category-based visibility bias for point sprites.

### Problem 2: Insects Stuck on Trees
- **Root Cause**: `canUseTree()` returned `true` for any creature with `canClimb` capability, causing ground insects (ants, beetles) to climb trees when they should be foraging on ground.
- **Solution**: Changed `canUseTree()` to be opt-in based on `primaryHabitat` (CANOPY, TREE_TRUNK) plus explicit arboreal species list.

### Problem 3: Poor Habitat Distribution
- **Root Cause**: Spawn system didn't account for habitat height preferences. All creatures spawned at ground level.
- **Solution**: Added `getSpawnHeightForHabitat()` function that adjusts spawn Y position based on creature's `primaryHabitat`.

---

## Size Ranges and Scaling Rules

### Size Categories (from SmallCreatureType.h)
| Category | Size Range | Examples |
|----------|------------|----------|
| MICROSCOPIC | < 1mm | Mites, small insects |
| TINY | 1mm - 1cm | Ants, beetles, spiders |
| SMALL | 1cm - 10cm | Mice, frogs, large insects |
| MEDIUM | 10cm - 30cm | Squirrels, rabbits, snakes |

### Actual Size Values (from properties table)
| Type | Min Size (m) | Max Size (m) |
|------|--------------|--------------|
| ANT_WORKER | 0.002 | 0.005 |
| BEETLE_GROUND | 0.005 | 0.03 |
| FLY | 0.003 | 0.01 |
| BEE_WORKER | 0.01 | 0.02 |
| BUTTERFLY | 0.02 | 0.1 |
| SPIDER_ORB_WEAVER | 0.005 | 0.03 |
| MOUSE | 0.05 | 0.1 |
| FROG | 0.03 | 0.12 |
| SQUIRREL_TREE | 0.15 | 0.3 |
| RABBIT | 0.2 | 0.4 |

### Scaling Formula (SmallCreatureRenderer.cpp)
```cpp
// For mesh instances:
float worldSize = props.minSize + (props.maxSize - props.minSize) * creature.genome->size;
float scale = worldSize * getMeshScaleFactor(creature.type);

// For point sprites:
float worldSize = props.minSize + (props.maxSize - props.minSize) * creature.genome->size;
float visibilityBias = getVisibilityBias(props.sizeCategory);
sprite.size = worldSize * visibilityBias;
```

### Visibility Bias by Category
| Category | Bias | Effective Sprite Size |
|----------|------|----------------------|
| MICROSCOPIC | 80.0x | 0.5mm -> ~4cm |
| TINY | 15.0x | 5mm -> ~7.5cm |
| SMALL | 4.0x | 5cm -> ~20cm |
| MEDIUM | 2.0x | 20cm -> ~40cm |

**Note**: These values are tuned for typical camera distances of 10-50m viewing. The bias creates a "perceived size" that scales appropriately:
- Very close: actual size dominates, creatures look correct
- Mid distance: tiny creatures remain visible as small dots
- Far distance: all creatures become similar-sized particles

---

## Spawn/Habitat Changes

### Tree Usage Rules (TreeDwellers.cpp)
Only these creature types can use trees:
1. **By Primary Habitat**: Creatures with `CANOPY` or `TREE_TRUNK` as primary habitat
2. **Explicit Arboreal Species**:
   - SQUIRREL_TREE
   - TREE_FROG
   - GECKO
   - CHAMELEON
   - SPIDER_ORB_WEAVER (for webs)
   - BAT_SMALL, BAT_LARGE (roosting)
   - BUTTERFLY, MOTH (pupation)

### Spawn Height by Habitat (SmallCreatureEcosystem.cpp)
| Habitat | Spawn Height |
|---------|--------------|
| UNDERGROUND | Ground - 0.1m |
| GROUND_SURFACE | Ground level |
| GRASS | Ground + 0.05-0.2m |
| BUSH | Ground + 0.3-1.5m |
| TREE_TRUNK | Ground + 1-5m |
| CANOPY | Ground + 4-10m |
| WATER_SURFACE | Ground + 0.01m |
| UNDERWATER | Ground - 0.5m |
| AERIAL | Ground + 1-5m |

---

## Physics Tuning Changes

No direct physics changes were made in this phase. The physics system (`SmallCreaturePhysics.h`) already has proper constants:

- **Jump Multipliers**: Frogs 20x body length, Grasshoppers 30x, Spiders 50x
- **Friction**: Ground 0.6, Grass 0.4, Bark 0.7, Leaf 0.3
- **Climbing**: Min angle ~45°, speed multiplier 0.5x
- **Burrowing**: Speed multiplier 0.2x

Size changes affect physics implicitly through:
- Proper world-scale positions
- Correct collision radii
- Appropriate movement speeds

---

## Debug Features Added

### Habitat Statistics (SmallCreatureRenderer)
New `HabitatStats` structure tracks creature distribution:
```cpp
struct HabitatStats {
    size_t groundCount;       // Ground surface creatures
    size_t grassCount;        // Grass layer creatures
    size_t bushCount;         // Bush/shrub layer creatures
    size_t aerialCount;       // Flying creatures
    size_t canopyCount;       // Tree-dwelling creatures
    size_t undergroundCount;  // Burrowing creatures
    size_t aquaticCount;      // Water creatures

    // Size category distribution
    size_t microscopicCount;  // < 1mm
    size_t tinyCount;         // 1mm - 1cm
    size_t smallCount;        // 1cm - 10cm
    size_t mediumCount;       // 10cm - 30cm
};

// Access via:
renderer.getHabitatStats();
renderer.setShowHabitatDebug(true);

// Get formatted debug string:
std::string debugInfo = renderer.getHabitatDebugString();
```

### Debug Overlay Output Format
```
=== Small Creature Distribution ===
HABITAT:
  Ground:      1234
  Grass:        567
  Bush:         234
  Canopy/Tree:  456
  Aerial:       789
  Underground:  123
  Aquatic:       45
SIZE CATEGORY:
  Microscopic (<1mm):   234
  Tiny (1-10mm):       2345
  Small (1-10cm):       567
  Medium (10-30cm):      89
RENDERING:
  Detailed:     123
  Simplified:   456
  Point:       1890
  Particle:     234
  Total:       2703
```

---

## Files Modified

1. **src/graphics/SmallCreatureRenderer.cpp**
   - Fixed size scaling formula (lines 77, 96)
   - Added `getMeshScaleFactor()` function
   - Added `getVisibilityBias()` function
   - Added habitat statistics tracking

2. **src/graphics/SmallCreatureRenderer.h**
   - Added `getMeshScaleFactor()` declaration
   - Added `getVisibilityBias()` declaration
   - Added `HabitatStats` structure
   - Added debug settings

3. **src/entities/small/TreeDwellers.cpp**
   - Rewrote `canUseTree()` to be habitat-based (lines 575-607)

4. **src/entities/small/SmallCreatureEcosystem.cpp**
   - Added `getSpawnHeightForHabitat()` function
   - Updated spawn logic to use habitat-based heights

5. **src/entities/small/SmallCreatureEcosystem.h**
   - Added `getSpawnHeightForHabitat()` declaration

---

## Validation Notes

### Expected Behavior After Changes
1. **Ants**: Should appear ~3-5mm, stay on ground foraging, not climb trees
2. **Butterflies**: Should appear ~2-10cm, fly at 1-5m height, occasionally visit trees
3. **Squirrels**: Should appear ~15-30cm, primarily in trees at 4-10m height
4. **Flying insects**: Should be distributed in air at varying heights
5. **Ground beetles**: Should stay at ground level, not climb trees

### Testing Checklist
- [ ] Insects appear appropriately small (ants barely visible up close)
- [ ] No massive point sprites at medium distance
- [ ] Ground insects (ants, beetles) stay on ground
- [ ] Tree squirrels spawn and move in canopy
- [ ] Flying insects (flies, bees, dragonflies) spread across heights
- [ ] Habitat stats show reasonable distribution
- [ ] No physics instability from scale changes

### Known Limitations
- Visibility bias may still need tuning for specific viewing distances
- Underground creatures only approximate (spawn slightly below ground)
- Aquatic spawning doesn't validate actual water presence
