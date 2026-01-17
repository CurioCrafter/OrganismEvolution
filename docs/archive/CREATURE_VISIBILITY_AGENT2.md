# Creature Visibility Fix - Agent 2

## Problem Description

Creatures were only visible when positioned in water areas, but completely invisible when on land terrain. This was a critical rendering bug affecting the core simulation experience.

## Root Cause Analysis

After thorough investigation of the rendering pipeline, two primary issues were identified:

### Issue 1: Depth Test Comparison Operator (Primary Cause)

**Location:** `src/main.cpp` line 2905 (pipeline creation)

**Original Code:**
```cpp
pipeDesc.depthCompareOp = CompareOp::Less;
```

**Problem:**
The depth comparison was set to `Less`, meaning pixels only pass the depth test if their depth value is strictly less than the current depth buffer value. When creatures are positioned at exactly the terrain surface height, their depth values equal the terrain's depth values, causing them to fail the depth test.

**Why Water Worked:**
In water areas, the terrain surface is lower (WATER_LEVEL = 0.0), and the water rendering uses different visual effects. The creature's position relative to the water surface created a slight depth difference that allowed them to pass the depth test.

### Issue 2: Creature Y Position (Contributing Factor)

**Locations:**
- `src/main.cpp` line 2166 (UpdateCreature)
- `src/main.cpp` line 1936 (SpawnCreatures)
- `src/main.cpp` line 2363 (child spawning during reproduction)

**Original Code:**
```cpp
creature->position.y = terrain.GetHeightWorld(creature->position.x, creature->position.z);
```

**Problem:**
Creatures were positioned at exactly the terrain height with no vertical offset. This caused:
1. Z-fighting artifacts (flickering between terrain and creature)
2. Depth test failures when combined with `CompareOp::Less`

## Fixes Applied

### Fix 1: Change Depth Comparison to LESS_EQUAL

**File:** `src/main.cpp`

```cpp
// Before:
pipeDesc.depthCompareOp = CompareOp::Less;

// After:
pipeDesc.depthCompareOp = CompareOp::LessEqual;
```

This allows pixels with depth values equal to or less than the current depth buffer to pass, ensuring creatures at exactly terrain height are rendered.

### Fix 2: Add Y Position Offset

**File:** `src/main.cpp`

Three locations were updated to add a 0.1 unit Y offset:

1. **UpdateCreature() - Line 2166:**
```cpp
// Before:
c.position.y = terrainHeight;

// After:
c.position.y = terrainHeight + 0.1f;
```

2. **SpawnCreatures() - Line 1936:**
```cpp
// Before:
creature->position.y = terrain.GetHeightWorld(...);

// After:
creature->position.y = terrain.GetHeightWorld(...) + 0.1f;
```

3. **Reproduction/child spawning - Line 2363:**
```cpp
// Before:
child->position.y = terrain.GetHeightWorld(...);

// After:
child->position.y = terrain.GetHeightWorld(...) + 0.1f;
```

## Technical Details

### Terrain Height System
- Terrain heights range from 0 to 30 (HEIGHT_SCALE = 30.0f)
- Water level is at 0.0 (WATER_LEVEL = 0.0f)
- Biome boundaries:
  - Water: height < 0.0
  - Beach: 0.0 to 2.0
  - Grass: 2.0 to 10.0
  - Forest: 10.0 to 20.0
  - Mountain: 20.0 to 25.0
  - Snow: > 25.0

### Render Order (Correct)
1. Clear depth buffer to 1.0
2. Render terrain (writes to depth buffer)
3. Render creatures (tests against depth buffer)
4. Render nametags (billboards)
5. Render ImGui overlay

### Why the Fix Works

The combination of `LessEqual` depth test and the 0.1 unit Y offset ensures:

1. **Depth Test Pass:** Even in edge cases where creature depth equals terrain depth, the `LessEqual` comparison allows the pixel to pass.

2. **Visual Correctness:** The 0.1 unit offset ensures creatures visually appear ON the terrain rather than clipping through it.

3. **No Z-Fighting:** The offset eliminates depth buffer precision issues that cause flickering.

## Verification Steps

After applying these fixes, verify:

1. [ ] Creatures are visible on grass terrain
2. [ ] Creatures are visible on beach terrain
3. [ ] Creatures are visible on mountain terrain
4. [ ] Creatures remain visible in/near water (regression test)
5. [ ] No flickering or z-fighting artifacts
6. [ ] Newly spawned creatures are visible immediately
7. [ ] Child creatures from reproduction are visible

## Files Modified

| File | Line(s) | Change |
|------|---------|--------|
| `src/main.cpp` | 2903-2905 | Changed `CompareOp::Less` to `CompareOp::LessEqual` |
| `src/main.cpp` | 2166-2167 | Added +0.1f Y offset in UpdateCreature |
| `src/main.cpp` | 1936-1937 | Added +0.1f Y offset in SpawnCreatures |
| `src/main.cpp` | 2363-2364 | Added +0.1f Y offset in child spawning |

## Related Components

- **Creature.hlsl:** Shader properly handles position transformation - no changes needed
- **Terrain.hlsl:** Reference for height calculations - no changes needed
- **Camera system:** View/projection matrices are correct - no changes needed
- **Metaball system:** Creature mesh generation is correct - no changes needed

## Author

Agent 2 - Creature Visibility Investigation
Date: 2026-01-14
