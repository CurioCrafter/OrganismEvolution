# Camera Auto-Movement/Drift Fix

## Problem Description
The camera was exhibiting unwanted auto-movement (drift) even when the user wasn't providing any input. This made the simulation difficult to observe as the view would slowly rotate or drift without user interaction.

## Root Causes Identified

### 1. Camera Smoothing with No Threshold (Primary Cause)
**Location:** `Camera::UpdateSmoothing()` in `src/main.cpp` (around line 2625)

**Issue:** The camera used exponential smoothing to interpolate between the actual camera values (`position`, `pitch`, `yaw`) and the smoothed values used for rendering (`smoothPosition`, `smoothPitch`, `smoothYaw`). The smoothing formula was:
```cpp
smoothPitch = smoothPitch + (pitch - smoothPitch) * smoothFactor;
```

Due to floating point precision, the difference `(pitch - smoothPitch)` never reaches exactly zero. This meant the camera was perpetually "settling" toward its target, causing slow drift.

### 2. Mouse Delta Accumulation with No Decay
**Location:** `HandleInput()` function, mouse look section (around line 4640)

**Issue:** The mouse look code used exponential smoothing for accumulated deltas:
```cpp
s_accumulatedDX = s_accumulatedDX * smoothFactor + dx * (1.0f - smoothFactor);
```

When there was no mouse movement (dx=0, dy=0), the accumulated values were NOT decayed. They retained their previous values, which could cause residual rotation if the user stopped moving the mouse mid-movement.

### 3. Small Dead Zone
**Issue:** The original dead zone of 0.5 pixels was too small to filter out mouse sensor noise and system-level cursor jitter.

## Fixes Applied

### Fix 1: Camera Smoothing with Threshold Check
**Before:**
```cpp
void UpdateSmoothing() const
{
    const f32 smoothFactor = 0.15f;
    smoothPitch = smoothPitch + (pitch - smoothPitch) * smoothFactor;
    smoothYaw = smoothYaw + (yaw - smoothYaw) * smoothFactor;
    smoothPosition = smoothPosition + (position - smoothPosition) * smoothFactor;
}
```

**After:**
```cpp
void UpdateSmoothing() const
{
    const f32 smoothFactor = 0.15f;
    const f32 threshold = 0.0001f;   // Stop smoothing when difference is negligible
    const f32 posThreshold = 0.001f; // Position threshold (slightly larger for world units)

    // Only smooth if difference exceeds threshold, otherwise snap to target
    f32 pitchDiff = pitch - smoothPitch;
    if (std::abs(pitchDiff) > threshold)
        smoothPitch = smoothPitch + pitchDiff * smoothFactor;
    else
        smoothPitch = pitch;

    f32 yawDiff = yaw - smoothYaw;
    if (std::abs(yawDiff) > threshold)
        smoothYaw = smoothYaw + yawDiff * smoothFactor;
    else
        smoothYaw = yaw;

    Vec3 posDiff = position - smoothPosition;
    f32 posDiffMag = posDiff.Length();
    if (posDiffMag > posThreshold)
        smoothPosition = smoothPosition + posDiff * smoothFactor;
    else
        smoothPosition = position;
}
```

**Added Helper Methods:**
```cpp
// Immediately sync smooth values to actual values (call when input stops)
void SyncSmoothing() const
{
    smoothPitch = pitch;
    smoothYaw = yaw;
    smoothPosition = position;
}

// Check if camera is currently being smoothed (has pending interpolation)
bool IsSmoothing() const
{
    const f32 threshold = 0.0001f;
    const f32 posThreshold = 0.001f;
    return std::abs(pitch - smoothPitch) > threshold ||
           std::abs(yaw - smoothYaw) > threshold ||
           (position - smoothPosition).Length() > posThreshold;
}
```

### Fix 2: Mouse Delta Decay with Zero-Frame Counter
**Before:**
```cpp
if (dx != 0.0f || dy != 0.0f)
{
    const f32 smoothFactor = 0.7f;
    s_accumulatedDX = s_accumulatedDX * smoothFactor + dx * (1.0f - smoothFactor);
    s_accumulatedDY = s_accumulatedDY * smoothFactor + dy * (1.0f - smoothFactor);
    // ... apply to camera
}
// No else clause - accumulated values just sit there
```

**After (Updated January 2026):**
```cpp
// Track consecutive zero-delta frames for drift prevention
static i32 s_zeroFrameCount = 0;

if (dx != 0.0f || dy != 0.0f)
{
    // Movement detected - reset zero frame counter
    s_zeroFrameCount = 0;

    // Light smoothing to reduce jitter while maintaining responsiveness
    const f32 smoothFactor = 0.3f;
    s_accumulatedDX = s_accumulatedDX * smoothFactor + dx * (1.0f - smoothFactor);
    s_accumulatedDY = s_accumulatedDY * smoothFactor + dy * (1.0f - smoothFactor);

    // Apply to camera...
}
else
{
    // No mouse movement this frame
    s_zeroFrameCount++;

    // After 3 consecutive zero-delta frames, stop all camera motion
    // This prevents drift from residual accumulated values
    if (s_zeroFrameCount >= 3)
    {
        s_accumulatedDX = 0.0f;
        s_accumulatedDY = 0.0f;
        // Sync camera smoothing to stop any rendering drift
        m_camera.SyncSmoothing();
    }
    else
    {
        // Rapid decay during transition frames
        s_accumulatedDX *= 0.1f;
        s_accumulatedDY *= 0.1f;
    }
}
```

**Key Improvement:** Instead of relying solely on decay factors, we now count consecutive frames with no mouse delta. After 3 zero frames, accumulated values are immediately zeroed and camera smoothing is synced. This provides a more definitive stop to camera motion.

### Fix 3: Increased Dead Zone
**Before:**
```cpp
const f32 deadZone = 0.5f;
```

**After:**
```cpp
const f32 deadZone = 2.0f;
```

### Fix 4: Camera Position Bounds (Added January 2026)

Added camera bounds to prevent the camera from going through terrain or outside the simulation world.

```cpp
// Clamp camera position to world bounds
f32 worldSize = Terrain::WIDTH * Terrain::SCALE * 0.5f;
m_camera.position.x = std::clamp(m_camera.position.x, -worldSize, worldSize);
m_camera.position.z = std::clamp(m_camera.position.z, -worldSize, worldSize);

// Prevent camera from going below terrain (with minimum clearance)
f32 terrainHeight = m_world.terrain.GetHeightWorld(m_camera.position.x, m_camera.position.z);
const f32 minClearance = 2.0f;
m_camera.position.y = std::max(m_camera.position.y, terrainHeight + minClearance);

// Maximum height limit
m_camera.position.y = std::min(m_camera.position.y, 300.0f);
```

**Bounds:**
- XZ: Constrained to ±(Terrain::WIDTH * Terrain::SCALE * 0.5f) = world extents
- Y minimum: 2 units above terrain surface at current XZ position
- Y maximum: 300 units

## Behavior Changes

### Before Fix
- Camera would slowly drift/rotate even when user wasn't touching controls
- After stopping mouse movement, camera might continue rotating briefly
- Small mouse sensor noise could cause micro-jitter
- Camera could clip through terrain or fly infinitely far from the world

### After Fix
- Camera remains perfectly still when no input is provided
- Smoothing still works for smooth visual transitions during movement
- Camera snaps to final position when input stops (no lingering drift)
- Increased tolerance for mouse sensor noise
- Camera cannot go below terrain surface or outside world bounds

## Files Modified
- `C:\Users\andre\Desktop\OrganismEvolution\src\main.cpp`
  - `Camera::UpdateSmoothing()` - Added threshold checks and snap-to-target behavior
  - `Camera::SyncSmoothing()` - New method to immediately sync smooth values
  - `Camera::IsSmoothing()` - New method to check if smoothing is in progress
  - `HandleInput()` - Added mouse delta decay and increased dead zone

## Testing Recommendations
1. Run the simulation and leave mouse/keyboard untouched
2. Verify camera remains stationary
3. Move camera with WASD and mouse, then stop - verify it settles quickly
4. Rapidly move and stop the mouse - verify no residual rotation
