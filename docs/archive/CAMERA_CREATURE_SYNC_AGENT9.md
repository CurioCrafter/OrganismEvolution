# Camera-Creature Synchronization Fix

## Agent 9 Investigation Report

**Date:** 2026-01-14
**Issue:** Camera jerk affecting creature positions
**Status:** FIXED

---

## Problem Description

When the camera moved suddenly (jerk), creature positions appeared to be affected - creatures visually shifted or "jumped" relative to their expected positions. This was a visual artifact, not an actual position change.

---

## Root Cause Analysis

### Primary Issue: viewPos/ViewMatrix Mismatch

The root cause was a **mismatch between the camera position used in the view matrix and the camera position sent to shaders as `viewPos`**.

**Before Fix:**
```cpp
// Line 3341: View matrix uses SMOOTHED position
Mat4 viewMat = camera.GetViewMatrix();  // Uses smoothPosition internally

// Line 3349: viewPos uses RAW position (BUG!)
frameConstants.viewPos = camera.position;  // Raw, unsmoothed position
```

**The Problem:**
1. `GetViewMatrix()` uses `smoothPosition` (the interpolated camera position)
2. `viewPos` was set to `camera.position` (the raw, unsmoothed position)
3. These two values differ during camera movement due to smoothing interpolation

**Why This Causes Visual Artifacts:**
- The view matrix transforms object positions using smoothed camera position
- The `viewPos` uniform is used for lighting calculations (specular highlights, view-dependent effects)
- When camera jerks, the mismatch causes:
  - Lighting to "pop" or shift unnaturally
  - Creatures to appear to move because lighting changes abruptly
  - Billboard objects (if any) to face the wrong direction

### How Camera Smoothing Works

The Camera class implements exponential smoothing to reduce jitter:

```cpp
void UpdateSmoothing() const
{
    const f32 smoothFactor = 0.15f;  // 15% movement toward target per frame

    // Smooth pitch, yaw, and position
    smoothPitch = smoothPitch + (pitch - smoothPitch) * smoothFactor;
    smoothYaw = smoothYaw + (yaw - smoothYaw) * smoothFactor;
    smoothPosition = smoothPosition + (position - smoothPosition) * smoothFactor;
}
```

This means:
- `position` = raw input position (immediate response to WASD)
- `smoothPosition` = interpolated position (15% closer to target each frame)
- During sudden movements, these values can differ significantly

---

## Main Loop Order Analysis

The main loop order is **CORRECT**:

```cpp
void Run()
{
    while (!m_window->ShouldClose())
    {
        // 1. Poll events
        m_window->PollEvents();

        // 2. Handle input (updates camera.position/pitch/yaw)
        HandleInput(deltaTime);

        // 3. Update simulation (creature positions)
        Update(deltaTime);

        // 4. Render (uses camera data)
        Render();
    }
}

void Render()
{
    // Called BEFORE rendering to update smoothed values
    m_camera.UpdateSmoothing();  // Updates smoothPosition from position

    m_renderer.BeginFrame();
    m_renderer.Render(m_world, m_camera, m_time);
    m_renderer.EndFrame();
}
```

The order is correct:
1. Input updates `camera.position` (raw)
2. Simulation updates creature positions
3. `UpdateSmoothing()` interpolates smoothPosition toward position
4. `GetViewMatrix()` uses smoothPosition
5. **BUG WAS HERE:** viewPos used raw position instead of smoothPosition

---

## The Fix

### 1. Added GetSmoothedPosition() Method (Camera class)

```cpp
// Get smoothed position for consistent rendering
// IMPORTANT: viewPos in constant buffer MUST use this to match the view matrix
Vec3 GetSmoothedPosition() const
{
    return smoothPosition;
}
```

### 2. Updated viewPos Assignment (Render function)

**Before:**
```cpp
frameConstants.viewPos = camera.position;  // RAW position - WRONG!
```

**After:**
```cpp
// CRITICAL FIX: Use smoothed position to match the view matrix
// Previously used camera.position (raw) which caused visual desync with creatures
// when camera jerked - the view matrix and viewPos were using different positions
frameConstants.viewPos = camera.GetSmoothedPosition();  // Smoothed position - CORRECT!
```

---

## Files Modified

1. **src/main.cpp**
   - Added `GetSmoothedPosition()` method to Camera class (around line 2478)
   - Changed `frameConstants.viewPos` to use `camera.GetSmoothedPosition()` (around line 3366)

---

## Technical Details

### Constant Buffer Structure (for reference)
```cpp
struct Constants
{
    ShaderMat4 view;           // Uses smoothPosition via GetViewMatrix()
    ShaderMat4 projection;
    ShaderMat4 viewProjection; // Uses smoothPosition via GetViewMatrix()
    ShaderFloat3 viewPos;      // NOW uses smoothPosition via GetSmoothedPosition()
    f32 time;
    // ... lighting and other data
};
```

### View Matrix Calculation
```cpp
Mat4 GetViewMatrix() const
{
    // Uses smoothed values for rendering
    Vec3 front;
    front.x = std::cos(smoothPitch) * std::sin(smoothYaw);
    front.y = std::sin(smoothPitch);
    front.z = std::cos(smoothPitch) * std::cos(smoothYaw);
    front = front.Normalized();

    Vec3 right = front.Cross(Vec3(0, 1, 0)).Normalized();
    Vec3 up = right.Cross(front).Normalized();

    return Mat4::LookAt(smoothPosition, smoothPosition + front, up);
}
```

---

## Verification

After the fix:
1. All camera-related shader uniforms use consistent smoothed values
2. View matrix position matches viewPos position
3. No visual desync between creature transforms and lighting calculations

---

## Other Observations

### Camera Smoothing Factor
The smoothing factor of 0.15 (15% per frame) may feel slightly sluggish:
- At 60 FPS: ~7 frames to reach 90% of target
- Consider increasing to 0.25-0.35 for snappier feel if desired

### Threshold-Based Smoothing
The camera already has threshold-based snapping to prevent infinite drift:
```cpp
const f32 threshold = 0.0001f;   // For angles
const f32 posThreshold = 0.001f; // For position

// Snap to target when difference is negligible
if (posDiffMag > posThreshold)
    smoothPosition = smoothPosition + posDiff * smoothFactor;
else
    smoothPosition = position;  // Snap directly
```

This prevents floating-point precision issues from accumulating.

---

## Summary

The "camera jerk affecting creature positions" was caused by using mismatched camera positions:
- View matrix used smoothed position
- viewPos shader uniform used raw position

The fix ensures all rendering uses the same smoothed camera position, eliminating the visual desync during camera movement.
