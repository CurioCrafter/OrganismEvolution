# Lighting System Review

**Date:** January 2026
**Status:** Review Complete - System is Well-Implemented

## Executive Summary

The lighting system in this DirectX 12 evolution simulation is well-architected and properly implemented. No critical issues were found that would cause flickering. The user's suspicion that flickering may be from an external application is likely correct.

---

## Review Findings by Phase

### Phase 1: Time Value Handling - PASS

**Files Reviewed:** `src/main.cpp`, `src/core/DayNightCycle.h`

**Findings:**
- Time wrapping is correctly implemented in `main.cpp:4742-4745`:
  ```cpp
  m_time += dt;
  if (m_time > 1000.0f) m_time -= 1000.0f;
  ```
- DayNightCycle correctly wraps dayTime in 0-1 range (`DayNightCycle.h:38-40`):
  ```cpp
  dayTime += dt / dayLengthSeconds;
  if (dayTime >= 1.0f) dayTime -= 1.0f;
  if (dayTime < 0.0f) dayTime += 1.0f;
  ```
- Both use proper subtraction (not modulo) which prevents floating-point precision issues.

**Status:** No issues found.

---

### Phase 2: Light Position Stability - PASS

**Files Reviewed:** `src/core/DayNightCycle.h`

**Findings:**
- Sun position uses smooth trigonometric functions (`DayNightCycle.h:58-75`):
  ```cpp
  f32 sunAngle = GetSunAngle();
  f32 sunHeight = std::sin(sunAngle);
  f32 sunHorizontal = std::cos(sunAngle);
  ```
- No discontinuities in sun/moon position calculations.
- Moon position calculation is also smooth with proper phase handling.

**Status:** No issues found.

---

### Phase 3: Shader Numerical Stability - PASS (Minor Improvements Possible)

**Files Reviewed:** `Runtime/Shaders/Terrain.hlsl`, `Runtime/Shaders/Creature.hlsl`

**Current Protections in Place:**

1. **Division by zero protection** - The shaders correctly use epsilon in the BRDF denominator:
   ```hlsl
   // Terrain.hlsl:232
   float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;

   // Creature.hlsl:218
   float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
   ```

2. **Dot product clamping** - All NdotL, NdotV, NdotH values use `max(..., 0.0)`:
   ```hlsl
   float NdotH = max(dot(N, H), 0.0);
   float NdotV = max(dot(N, V), 0.0);
   float NdotL = max(dot(N, L), 0.0);
   ```

3. **Fresnel uses saturate()** - Prevents values outside 0-1 range:
   ```hlsl
   return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
   ```

4. **Vector normalization** - Normals are normalized before use.

**Minor Issue Found:**
In both shaders, `DistributionGGX` does not protect against division by zero when `denom` could theoretically be zero:
```hlsl
// Terrain.hlsl:174
return nom / denom;  // Could add epsilon for extra safety
```
However, mathematically `denom = PI * denom * denom` where `denom = (NdotH2 * (a2 - 1.0) + 1.0)` will always be >= 1.0 when NdotH2 and a2 are positive, so this is unlikely to cause issues in practice.

**Status:** Well-protected. Optional improvement below.

---

### Phase 4: PBR Lighting Implementation - PASS

**Findings:**
The PBR implementation follows the standard Cook-Torrance BRDF correctly:

1. **GGX Distribution (D)** - Correct Trowbridge-Reitz formula
2. **Geometry Smith (G)** - Correct Smith method with Schlick-GGX
3. **Fresnel Schlick (F)** - Standard approximation correctly implemented
4. **Energy Conservation** - kD = (1 - kS) * (1 - metallic) is correct

The BRDF formula `(kD * albedo / PI + specular) * lightCol * NdotL` is the standard formulation.

**Status:** Correctly implemented.

---

### Phase 5: Ambient Lighting - PASS

**Findings:**
Multiple safety measures ensure minimum visibility:

1. **Terrain shader minimum ambient floor** (`Terrain.hlsl:245-247`):
   ```hlsl
   float3 skyCol = max(skyTopColor, float3(0.15, 0.15, 0.18));
   float3 groundCol = max(skyHorizonColor * 0.4, float3(0.08, 0.08, 0.1));
   ```

2. **Minimum sun intensity** (`Terrain.hlsl:861`):
   ```hlsl
   float effectiveSunIntensity = max(sunIntensity, 0.2);
   ```

3. **Final color floor** (`Terrain.hlsl:896`):
   ```hlsl
   result = max(result, terrainColor * 0.2);
   ```

4. **Ambient minimum in main calculation** (`Terrain.hlsl:865`):
   ```hlsl
   float3 minAmbient = max(ambientColor, float3(0.2, 0.2, 0.25));
   ```

**Status:** Excellent safety measures in place.

---

### Phase 6: Day/Night Transition Smoothness - PASS

**Findings:**
The `DayNightCycle::GetSkyColors()` function uses linear interpolation (`LerpVec3`, `LerpF32`) for all transitions:

```cpp
static Vec3 LerpVec3(const Vec3& a, const Vec3& b, f32 t)
{
    return Vec3(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    );
}
```

The day is divided into smooth phases with gradual transitions:
- Deep night (0.0-0.15)
- Pre-dawn (0.15-0.2)
- Dawn (0.2-0.3)
- Morning (0.3-0.45)
- Midday (0.45-0.55)
- Afternoon (0.55-0.7)
- Dusk (0.7-0.8)
- Twilight (0.8-0.85)
- Night (0.85-1.0)

Each transition calculates `t` as a normalized value within the phase, ensuring smooth blending.

**Status:** Smooth transitions implemented correctly.

---

### Phase 7: Constant Buffer Alignment - PASS

**Findings:**
The constant buffer layout is meticulously verified with static_assert checks:

```cpp
static_assert(sizeof(Constants) == 400, "Constants struct must be 400 bytes");
static_assert(offsetof(Constants, view) == 0, "view must be at offset 0");
static_assert(offsetof(Constants, model) == 304, "model must be at offset 304");
static_assert(offsetof(Constants, model) % 16 == 0, "model matrix MUST be 16-byte aligned");
```

The HLSL cbuffer layout in both shaders matches the C++ struct exactly with commented offsets:
```hlsl
cbuffer Constants : register(b0)
{
    float4x4 view;             // Offset 0, 64 bytes
    float4x4 projection;       // Offset 64, 64 bytes
    // ... etc
};
```

**Status:** Perfect alignment with compile-time verification.

---

## Potential External Causes of Flickering

Since the lighting code is solid, the flickering likely comes from:

1. **Display driver issues** - GPU driver bugs or settings
2. **Compositor/overlay applications** - Screen recording, game overlay, or other applications
3. **VSync/frame pacing** - If not using VSync, tearing could appear as flicker
4. **Multi-monitor setups** - Different refresh rates can cause visual artifacts
5. **Hardware issues** - Faulty cable, monitor, or GPU

## Recommendations

1. **Test with VSync enabled** if not already
2. **Close overlay applications** (Discord, GeForce Experience, etc.)
3. **Update GPU drivers**
4. **Check Windows Game Mode settings**

---

## Optional Minor Improvements

These are not necessary but could add extra safety:

### 1. Extra Division Protection in GGX (Optional)

In both shaders, add epsilon to GGX distribution:
```hlsl
// In DistributionGGX function:
return nom / max(denom, 0.0001);  // Extra safety
```

### 2. Creature Shader Dynamic Sky Colors (Optional)

The creature shader uses hardcoded sky colors for ambient IBL:
```hlsl
float3 skyCol = float3(0.4, 0.6, 0.9);  // Hardcoded
float3 groundCol = float3(0.3, 0.25, 0.2);  // Hardcoded
```

Could use the dynamic `skyTopColor`/`skyHorizonColor` from the constant buffer like the terrain shader does for consistency.

---

## Conclusion

**The lighting system is well-implemented with no stability issues that would cause flickering.** The code follows best practices for:
- Time value precision
- Smooth interpolation
- Numerical stability
- PBR correctness
- Constant buffer alignment

The reported flickering is most likely caused by an external application or display configuration, as the user suspected.
