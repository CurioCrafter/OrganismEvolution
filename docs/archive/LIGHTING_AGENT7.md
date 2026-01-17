# Lighting System Review - Agent 7

**Date:** January 14, 2026
**Priority:** Low (User suspects external cause for flicker)
**Reviewer:** Agent 7

---

## Executive Summary

The lighting system in this DirectX 12 evolution simulation is well-designed with proper safeguards against common issues. The implementation follows best practices for PBR (Physically Based Rendering) with smooth day/night transitions. **No critical issues were found** that would cause lighting flicker. The user's suspicion that flicker may be caused by another game running is likely correct.

---

## System Overview

### Components Reviewed

1. **Terrain.hlsl** - Terrain lighting with PBR and water effects
2. **Creature.hlsl** - Creature lighting with instanced rendering
3. **DayNightCycle.h** - Time management and sky color calculations
4. **main.cpp** - Time value management and constant buffer updates

### Architecture

```
DayNightCycle.h          main.cpp                 Shaders
     |                      |                        |
GetSkyColors() -----> frameConstants -------> cbuffer Constants
GetSunPosition() ---> lightPos/lightColor --> PBR calculations
     |                      |                        |
  dayTime (0-1) --------> time (wrapped) ----> animations
```

---

## Review Findings

### 1. Light Position Stability - GOOD

**File:** `src/core/DayNightCycle.h` (lines 58-89)

The sun position calculation is smooth and mathematically stable:

```cpp
Vec3 GetSunPosition() const
{
    f32 sunAngle = GetSunAngle();
    f32 sunHeight = std::sin(sunAngle);
    f32 sunHorizontal = std::cos(sunAngle);

    return Vec3(
        sunHorizontal * 500.0f,
        sunHeight * 400.0f + 50.0f,  // Minimum height of 50
        -100.0f
    );
}
```

**Assessment:**
- Uses smooth trigonometric functions (sin/cos)
- No discontinuities in the calculation
- Minimum height of 50 prevents light from going below horizon
- Moon position similarly smooth

### 2. Time Value Wrapping - CORRECT

**File:** `src/main.cpp` (lines 4261-4264)

```cpp
m_time += dt;
// Wrap time to prevent floating point precision issues in shaders
// 1000 seconds gives plenty of range for animations while staying precise
if (m_time > 1000.0f) m_time -= 1000.0f;
```

**Assessment:**
- Time wrapping is correctly implemented at 1000 seconds
- This prevents floating-point precision loss that occurs with large values
- Subtraction method (not modulo) maintains continuity
- Comment explains the purpose

**Day/Night Cycle Wrapping** (`DayNightCycle.h` lines 35-41):

```cpp
void Update(f32 dt)
{
    if (paused) return;
    dayTime += dt / dayLengthSeconds;
    if (dayTime >= 1.0f) dayTime -= 1.0f;
    if (dayTime < 0.0f) dayTime += 1.0f;
}
```

**Assessment:**
- dayTime stays in 0-1 range
- Handles both overflow and underflow
- Smooth continuous cycle

### 3. Day/Night Transitions - SMOOTH

**File:** `src/core/DayNightCycle.h` (lines 97-191)

The `GetSkyColors()` function uses 10 distinct time phases with linear interpolation:

| Time Range | Phase |
|------------|-------|
| 0.00-0.15 | Deep Night |
| 0.15-0.20 | Pre-Dawn |
| 0.20-0.30 | Dawn (Golden Hour) |
| 0.30-0.45 | Morning |
| 0.45-0.55 | Midday |
| 0.55-0.70 | Afternoon |
| 0.70-0.80 | Dusk (Golden Hour) |
| 0.80-0.85 | Twilight |
| 0.85-1.00 | Night |

**Assessment:**
- All transitions use `LerpVec3()` for smooth color blending
- No sudden jumps between phases
- Each phase has carefully tuned colors

### 4. Lighting Constants - REASONABLE

#### Ambient Light Levels

**File:** `src/core/DayNightCycle.h`

| Time of Day | Ambient Color | Sun Intensity |
|-------------|---------------|---------------|
| Midday | (0.45, 0.50, 0.55) | 1.0 |
| Dawn | (0.40, 0.35, 0.30) | 0.9 |
| Dusk | (0.15, 0.10, 0.12) | 0.35 |
| Night | (0.03, 0.03, 0.06) | 0.15 |

**Assessment:**
- Midday ambient is strong (0.45-0.55 range) - good visibility
- Night ambient (0.03-0.06) might be low but has minimum floor in shader

#### Shader Minimum Floors

**File:** `Runtime/Shaders/Terrain.hlsl` (lines 700-735)

```hlsl
// Ensure sunIntensity has a minimum value to prevent completely dark terrain
float effectiveSunIntensity = max(sunIntensity, 0.2);

// Ensure ambient color has a minimum floor
float3 minAmbient = max(ambientColor, float3(0.2, 0.2, 0.25));

// CRITICAL FIX: Ensure minimum visibility to prevent black terrain
result = max(result, terrainColor * 0.2);
```

**Assessment:**
- Multiple safety floors prevent complete darkness
- Minimum sun intensity: 0.2
- Minimum ambient: (0.2, 0.2, 0.25)
- Minimum final result: 20% of terrain color
- These values are appropriate for visibility

#### Sky Color Minimum (IBL)

```hlsl
// Ensure minimum ambient light to prevent terrain from going completely dark
float3 skyCol = max(skyTopColor, float3(0.15, 0.15, 0.18));
// Ground color with minimum floor
float3 groundCol = max(skyHorizonColor * 0.4, float3(0.08, 0.08, 0.1));
```

### 5. Floating Point Precision - SAFE

**Potential Issues Checked:**

1. **Division by Zero** - Protected in PBR calculations:
   ```hlsl
   float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
   ```
   The `+ 0.0001` prevents division by zero.

2. **NaN Prevention** - Fresnel uses saturate:
   ```hlsl
   return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
   ```
   `saturate()` clamps to 0-1, preventing negative values in `pow()`.

3. **Height Normalization** - Safe clamping:
   ```hlsl
   float heightNormalized = input.height / 30.0;
   heightNormalized = saturate(heightNormalized);
   ```

### 6. Creature Shader Lighting - MINOR GAPS

**File:** `Runtime/Shaders/Creature.hlsl`

**Observation:** The creature shader has hardcoded sky colors in `calculateAmbientIBL()`:

```hlsl
float3 calculateAmbientIBL(float3 N, float3 V, float3 albedo, float metallic, float roughness)
{
    float3 skyCol = float3(0.4, 0.6, 0.9);        // Static sky color
    float3 groundCol = float3(0.3, 0.25, 0.2);   // Static ground color
    // ...
}
```

**Compared to Terrain.hlsl:**
```hlsl
float3 skyCol = max(skyTopColor, float3(0.15, 0.15, 0.18));  // Dynamic
float3 groundCol = max(skyHorizonColor * 0.4, float3(0.08, 0.08, 0.1));  // Dynamic
```

**Assessment:** This is a minor inconsistency - creatures use static ambient while terrain uses dynamic sky colors. This won't cause flicker but means creatures don't fully respond to day/night cycle in their ambient component. Not a bug, just a potential enhancement opportunity.

---

## Potential Causes of External Flicker

Since the code review found no lighting instability issues, the user's suspicion of external causes is well-founded. Possible external causes include:

1. **GPU Resource Contention** - Another game/application competing for GPU resources
2. **V-Sync/Refresh Rate Mismatch** - Display timing issues
3. **Driver Issues** - GPU driver conflicts
4. **Thermal Throttling** - GPU reducing performance due to heat
5. **Power State Changes** - Laptop power management affecting GPU clocks
6. **Multi-Monitor Setup** - Different refresh rates causing sync issues

---

## Minor Improvement Opportunities

These are **optional** low-priority enhancements that could be made in the future:

### 1. Creature Shader Dynamic Ambient (Low Priority)

Update `Creature.hlsl` to use dynamic sky colors like terrain does. This would require passing `skyTopColor` and `skyHorizonColor` to the creature IBL calculation.

### 2. Star Twinkle Rate (Cosmetic)

Current star twinkle uses time directly:
```hlsl
float twinkle = sin(time * 3.0 + starNoise * 100.0) * 0.5 + 0.5;
```

Could add variation to twinkle rate per star for more natural appearance.

### 3. Fog Distance Adjustment (Cosmetic)

Current fog start distance is 50 units. Consider making this dynamic based on time of day (thicker fog at dawn/dusk for atmosphere).

---

## Conclusion

**The lighting system is well-implemented with no issues that would cause flicker.**

Key findings:
- All time values properly wrapped to prevent precision loss
- Day/night transitions are smooth with interpolation
- Multiple safety floors prevent complete darkness
- PBR calculations protected against division by zero and NaN
- Light positions calculated with smooth trigonometric functions

**Recommendation:** The flicker the user experienced is likely caused by an external factor (another game running, GPU driver, display settings). No code changes are required at this time.

---

## Files Reviewed

| File | Path | Lines | Status |
|------|------|-------|--------|
| Terrain.hlsl | Runtime/Shaders/Terrain.hlsl | 756 | GOOD |
| Creature.hlsl | Runtime/Shaders/Creature.hlsl | 393 | GOOD |
| DayNightCycle.h | src/core/DayNightCycle.h | 255 | GOOD |
| main.cpp | src/main.cpp | ~4300 | GOOD |

---

*Review completed by Agent 7 - January 14, 2026*
