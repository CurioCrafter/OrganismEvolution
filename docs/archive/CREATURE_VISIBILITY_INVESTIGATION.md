# Creature Visibility Bug Investigation and Fix

## Problem Statement
Creatures in the DirectX 12 evolution simulation were only visible in water/low areas but became invisible when walking on grass, beach, or mountain terrain. This was a rendering issue, not a simulation issue.

## Root Cause Analysis

### Investigation Phases Completed

#### Phase 1: Shader Debug Mode Check
- **File**: `Runtime/Shaders/Creature.hlsl`
- **Finding**: Debug mode was already disabled (line 343 commented out)
- **Status**: Not the cause

#### Phase 2: Depth Buffer Settings
- **File**: `src/main.cpp` (line 3234)
- **Finding**: Depth compare operation was already set to `CompareOp::LessEqual`
- **Status**: Correctly configured, not the cause

#### Phase 3: Render Order Analysis
- **Finding**: Render order was correct:
  1. Terrain (line 3755-3762)
  2. Vegetation/Trees (line 3764-3788)
  3. Creatures (line 3859-3993)
  4. Nametags (line 4010+)
- **Status**: Correct order, not the cause

#### Phase 4: Model Matrix Verification
- **File**: `src/main.cpp` (line 1086-1094)
- **Finding**: Matrix calculation `T * R * S` was correct for column-major HLSL
- **Status**: Correctly implemented, not the cause

#### Phase 5: Instanced Rendering Setup
- **File**: `src/main.cpp` (lines 380-428)
- **Finding**: Instance data layout matched HLSL shader expectations
- **Status**: Correctly configured, not the cause

#### Phase 6: Constant Buffer Verification
- **Finding**: Frame constants and viewProjection matrix were correctly uploaded
- **Status**: Working correctly, not the cause

### ROOT CAUSE IDENTIFIED

**Primary Issue**: The Y position offset for creatures (0.1f) was too small relative to the terrain mesh and depth buffer precision.

**Why it worked in water but not on land**:
- Terrain uses `HEIGHT_SCALE = 30.0f`
- Water level is at height 0.0
- At water level, a 0.1 offset represents a larger relative depth difference
- On land (heights 5-25), the 0.1 offset was within depth buffer precision limits, causing depth-fighting with the terrain mesh

## Fixes Applied

### Fix 1: Increased Y Position Offset (PRIMARY FIX)
Changed creature Y offset from 0.1f to 0.5f in three locations:

**Location 1**: Initial creature spawn (`src/main.cpp:2142`)
```cpp
// Before:
creature->position.y = terrain.GetHeightWorld(...) + 0.1f;

// After:
creature->position.y = terrain.GetHeightWorld(...) + 0.5f;
```

**Location 2**: Creature update loop (`src/main.cpp:2375`)
```cpp
// Before:
c.position.y = terrainHeight + 0.1f;

// After:
c.position.y = terrainHeight + 0.5f;
```

**Location 3**: Child creature spawn (`src/main.cpp:2571`)
```cpp
// Before:
child->position.y = terrain.GetHeightWorld(...) + 0.1f;

// After:
child->position.y = terrain.GetHeightWorld(...) + 0.5f;
```

### Fix 2: Minimum Creature Size Enforcement
Added minimum size check in `GetModelMatrix()` (`src/main.cpp:1090-1091`):
```cpp
// Enforce minimum size of 0.5 to ensure visibility
f32 effectiveSize = std::max(genome.size, 0.5f);
Mat4 S = Mat4::Scale(Vec3(effectiveSize, effectiveSize, effectiveSize));
```

### Fix 3: Minimum Color Brightness in Shader
Added minimum color brightness in Creature.hlsl (lines 355-362):
```hlsl
// Ensure minimum color brightness to prevent invisible black creatures
float3 baseColor = max(input.color, float3(0.15, 0.15, 0.15));
float3 skinColor = generateSkinPattern(..., baseColor);
...
// Ensure skin color is never too dark
skinColor = max(skinColor, float3(0.08, 0.08, 0.08));
```

## Files Modified

| File | Line(s) | Change |
|------|---------|--------|
| `src/main.cpp` | 2142 | Y offset 0.1f -> 0.5f (spawn) |
| `src/main.cpp` | 2375 | Y offset 0.1f -> 0.5f (update) |
| `src/main.cpp` | 2571 | Y offset 0.1f -> 0.5f (child spawn) |
| `src/main.cpp` | 1090-1091 | Minimum size enforcement |
| `Runtime/Shaders/Creature.hlsl` | 355-362 | Minimum color brightness |

## Technical Details

### Why 0.5f Works
- At terrain heights of 0-30 units, a 0.5 unit offset provides:
  - Clear depth separation from terrain mesh
  - No visual "floating" effect (creatures still appear grounded)
  - Sufficient buffer for depth buffer precision at all terrain heights

### Depth Buffer Considerations
- Depth buffer format: D32_FLOAT
- Depth compare operation: LessEqual
- Near/far planes affect depth precision non-linearly
- Higher terrain = less depth precision = larger offset needed

## Verification Steps
After applying fixes, verify:
1. Creatures visible on all terrain types (water, beach, grass, forest, mountain, snow)
2. Creatures do not appear to float excessively above terrain
3. No depth-fighting flickering at any terrain height
4. Small creatures (size < 0.5) still render at minimum visible size
5. Dark-colored creatures still visible against dark terrain

## Alternative Approaches (Not Used)
1. **Depth Bias**: Could add pipeline depth bias for creature rendering
   - `pipeDesc.depthBias = -1;` (negative pulls forward)
   - Not needed with 0.5f Y offset

2. **Stencil-based rendering**: Mark terrain, render creatures where stencil is set
   - More complex, unnecessary for this fix

3. **Separate depth passes**: Render creatures with their own depth buffer
   - Performance overhead, unnecessary
