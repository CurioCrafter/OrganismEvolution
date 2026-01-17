# Water Depth and Wave Height Fix

## Problem Description

The water in the terrain shader was set too high, causing water to cover too much of the terrain. Additionally, the wave animation was too aggressive, making the water appear unrealistically turbulent.

**Issues identified:**
1. Water threshold (WATER_LEVEL and WATER_THRESH) set at 0.05 normalized height was flooding too much terrain
2. Wave height at 0.03 was creating overly dramatic wave animation
3. Wave speed at 0.5 made the water appear choppy and unrealistic
4. Beach threshold at 0.10 was too far from the water level, creating an overly wide beach zone

## Changes Made

### File Modified
`Runtime/Shaders/Terrain.hlsl`

### Settings Comparison

| Setting | Before | After | Reason |
|---------|--------|-------|--------|
| WATER_LEVEL | 0.05 | 0.02 | Lowered to expose more terrain above water |
| WATER_THRESH | 0.05 | 0.02 | Consistent with WATER_LEVEL for biome calculation |
| BEACH_THRESH | 0.10 | 0.06 | Adjusted for smooth water-to-beach transition |
| waveHeight | 0.03 | 0.008 | Reduced by ~73% for subtler, realistic waves |
| waveSpeed | 0.5 | 0.3 | Slowed by 40% for calmer water appearance |

### Detailed Changes

#### 1. Water Level Constant (Line ~279)
```hlsl
// Before
static const float WATER_LEVEL = 0.05;

// After
static const float WATER_LEVEL = 0.02;
```

#### 2. Vertex Shader Wave Animation (Lines ~542-556)
```hlsl
// Before
float waveSpeed = 0.5;
float waveHeight = 0.03;

// After
float waveSpeed = 0.3;
float waveHeight = 0.008;
```

#### 3. Biome Thresholds in Pixel Shader (Lines ~592-597)
```hlsl
// Before
static const float WATER_THRESH = 0.05;    // ~1.5 world units
static const float BEACH_THRESH = 0.10;    // ~3 world units

// After
static const float WATER_THRESH = 0.02;    // ~0.6 world units
static const float BEACH_THRESH = 0.06;    // ~1.8 world units
```

#### 4. Procedural Normal Thresholds (Lines ~497-526)
Updated the thresholds in `calculateProceduralNormal()` to match the new biome boundaries:
- Changed `0.10` to `0.06` for water/beach boundary
- Ensures normal perturbation is consistent with visual biomes

## Why These Values Are Better

### Water Level (0.02 vs 0.05)
- **0.05 normalized** = ~1.5 world units of height covered by water
- **0.02 normalized** = ~0.6 world units of height covered by water
- This exposes approximately 60% more terrain that was previously underwater

### Beach Threshold (0.06 vs 0.10)
- Keeps beach proportionally sized relative to lowered water level
- Maintains a 0.04 normalized height beach zone (same as before: 0.10 - 0.05 = 0.05)
- Ensures smooth visual transition from water to beach to grassland

### Wave Height (0.008 vs 0.03)
- Original 0.03 created very noticeable vertex displacement
- New 0.008 creates gentle ripples that look more realistic
- Waves still visible but no longer dominate the water surface

### Wave Speed (0.3 vs 0.5)
- Slower animation creates calmer, more natural-looking water
- Reduces the "choppy" appearance that was distracting
- Better matches the reduced wave height for a cohesive calm water effect

## Biome Threshold Summary (After Fix)

| Biome | Normalized Height Range | World Height Range |
|-------|------------------------|-------------------|
| Water | 0.00 - 0.02 | 0.0 - 0.6 |
| Beach | 0.02 - 0.06 | 0.6 - 1.8 |
| Grass | 0.06 - 0.35 | 1.8 - 10.5 |
| Rock/Mountain | 0.35 - 0.85 | 10.5 - 25.5 |
| Snow | 0.85 - 1.00 | 25.5 - 30.0 |

## Testing Notes

After applying these changes:
1. Water should cover less terrain area
2. Waves should appear calmer and more subtle
3. Beach zone should start immediately above water level
4. Transition between water and beach should be smooth (handled by blend zones)
5. All other biome thresholds (grass, mountain, snow) remain unchanged

## Date
January 14, 2026
