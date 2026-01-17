# SimulationDemo Known Issues

Current bugs, planned fixes, and workarounds.

## Critical Issues

### Issue #1: Creatures Not Visible (Metaball/Marching Cubes)

**Severity:** Critical
**Status:** ✅ FIXED (2026-01-14)
**Affected:** Creature rendering

**Description:**
Creatures may not render due to marching cubes mesh generation failing silently.

**Root Cause:**
The isovalue threshold (1.0f) may not match actual metaball potential values. When multiple metaballs contribute, their influences stack and can exceed the threshold, causing all corner values to fail the isovalue test. This results in zero vertices generated.

**Evidence:**
- `MarchingCubes.cpp:396-403`: cubeIndex computed from 8 corners
- `MeshData.h:75`: `if (vertices.empty()) return;` (silent failure)
- `CreatureRenderer.cpp:100-106`: `if (!batch.mesh || batch.mesh->VAO == 0)` skips rendering

**Symptoms:**
- No creatures visible despite simulation running
- No errors in console
- Creature count shows non-zero population

**Workaround:**
Use simple geometric shapes (spheres, capsules) instead of metaball-generated meshes for debugging.

**Fixes Implemented:**

1. ✅ **Debug logging when vertex count is zero** (`MarchingCubes.cpp:455-460`)
   - Added error message with suggested isovalue fix

2. ✅ **Fallback sphere mesh generator** (`MarchingCubes.cpp:462-525`)
   - `MarchingCubes::generateFallbackSphere(radius, segments, rings)`
   - Generates UV sphere with proper normals and UVs
   - Always produces valid geometry

3. ✅ **Automatic fallback in CreatureMeshCache** (`CreatureMeshCache.cpp:82-95`)
   - Detects empty vertices, indices, or non-triangular index count
   - Automatically uses fallback sphere scaled by size category
   - Logs `[MESH FALLBACK]` when triggered

4. ✅ **Enhanced VAO validation in CreatureRenderer** (`CreatureRenderer.cpp:256-292`)
   - Detailed error messages for null mesh, VAO=0, and empty geometry
   - Logs VBO/EBO states for debugging

**Console Output (when fixed):**
```
[MESH] Herbivore mesh: 1536 vertices, 1536 indices
[MESH] Carnivore mesh: 2048 vertices, 2048 indices
```

**Console Output (if fallback triggered):**
```
[MESH ERROR] Herbivore mesh has 0 vertices after marching cubes!
[MESH FALLBACK] Using sphere mesh for Herbivore - creatures WILL be visible!
[FALLBACK] Generating sphere mesh: radius=1.1, segments=16, rings=12
[FALLBACK] Sphere generated: 221 vertices, 1152 indices (384 triangles)
[MESH] Herbivore mesh: 221 vertices, 1152 indices [FALLBACK SPHERE]
```

---

### Issue #2: Mouse Cursor Not Hiding (Win32)

**Severity:** High
**Status:** ✅ FIXED (2026-01-14)
**Affected:** Windows platform input

**Description:**
Mouse cursor may remain visible when it should be hidden during camera look mode.

**Root Cause:**
GLFW's `glfwSetInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED)` may not synchronize properly with Win32's ShowCursor counter system.

Win32's ShowCursor is a **counter**, not a boolean:
- Each `ShowCursor(TRUE)` increments the counter
- Each `ShowCursor(FALSE)` decrements the counter
- Cursor visible when counter > 0, hidden when counter <= 0
- Multiple desynchronized calls break cursor hiding

**Fixes Implemented:**

1. ✅ **Win32 header includes** (`main.cpp:17-23`)
   - Added `WIN32_LEAN_AND_MEAN` and `<windows.h>` for Win32 API access
   - Added `GLFW_EXPOSE_NATIVE_WIN32` and `<GLFW/glfw3native.h>` for `glfwGetWin32Window()`

2. ✅ **Manual cursor state tracking** (`main.cpp:62-63`)
   - Added `g_cursorHiddenWin32` static bool to track cursor visibility state
   - Prevents counter desync by tracking state independently

3. ✅ **HideCursorWin32() function** (`main.cpp:67-76`)
   - Checks `GetForegroundWindow()` before applying changes
   - Decrements ShowCursor counter in a loop until cursor is hidden
   - Only acts when cursor isn't already hidden (state check)

4. ✅ **ShowCursorWin32() function** (`main.cpp:80-90`)
   - Checks `GetForegroundWindow()` before applying changes
   - Increments ShowCursor counter in a loop until cursor is visible
   - Only acts when cursor isn't already visible (state check)

5. ✅ **ResetCursorStateWin32() function** (`main.cpp:93-97`)
   - Resets state tracking when window loses focus
   - Windows automatically shows cursor on focus loss, so we just update tracking

6. ✅ **Focus callback integration** (`main.cpp:182-196`)
   - On focus gain: Re-hides cursor if `mouseCaptured` was true before focus loss
   - On focus loss: Resets cursor state tracking

7. ✅ **TAB toggle integration** (`main.cpp:286-298`)
   - Calls `HideCursorWin32()` when enabling mouse capture
   - Calls `ShowCursorWin32()` when disabling mouse capture

**Behavior After Fix:**
- Cursor hides correctly when pressing TAB to enter camera look mode
- Cursor shows correctly when pressing TAB again to exit camera look mode
- Cursor behaves correctly on Alt+Tab (focus loss/gain)
- No counter desync issues on repeated toggles

---

## High Priority Issues

### Issue #3: Vertex Struct Not SIMD Aligned

**Severity:** Medium
**Status:** Documented
**Affected:** Rendering performance, potential visual artifacts

**Description:**
The Vertex struct has suboptimal memory alignment that can cause GPU cache misses.

**Current Layout:**
```cpp
struct Vertex {
    glm::vec3 position;  // 12 bytes, offset 0
    glm::vec3 normal;    // 12 bytes, offset 12 (NOT 16-byte aligned!)
    glm::vec2 texCoord;  // 8 bytes, offset 24
};                       // Total: 32 bytes
```

**Problem:**
The normal at offset 12 is not 16-byte aligned. GPU SIMD operations reading from this offset may span cache lines, causing:
- Reduced performance
- Potential garbage data in normals
- Black or incorrect rendering

**Workaround:**
None currently. Issue may manifest as visual artifacts on some GPUs.

**Planned Fix:**
```cpp
struct Vertex {
    glm::vec3 position;  // 12 bytes, offset 0
    float padding1;      // 4 bytes, offset 12
    glm::vec3 normal;    // 12 bytes, offset 16 (now aligned!)
    float padding2;      // 4 bytes, offset 28
    glm::vec2 texCoord;  // 8 bytes, offset 32
};                       // Total: 40 bytes (or use vec4)
```

---

### Issue #4: Input Processed When Window Unfocused

**Severity:** Medium
**Status:** Partially Fixed
**Affected:** Input handling

**Description:**
Keyboard input may still be processed when the application window loses focus.

**Root Cause:**
`processInput()` function reads key states via `glfwGetKey()` which queries OS key state regardless of window focus. The `windowFocused` global variable exists but is not checked for keyboard input.

**Current State:**
- Mouse callback: Focus check added (Guard 2) ✅
- Keyboard input: No focus check ❌

**Workaround:**
Alt-tab away from the window to prevent unintended input.

**Planned Fix:**
```cpp
void processInput(GLFWwindow* window, float deltaTime) {
    if (!windowFocused) return;  // Add this check

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(FORWARD, deltaTime);
    // ... rest of input handling
}
```

---

## Medium Priority Issues

### Issue #5: Scroll Wheel Offset Accumulation

**Severity:** Medium
**Status:** Active
**Affected:** Camera zoom

**Description:**
Scroll wheel input may accumulate unexpected offset values, causing erratic zoom behavior.

**Workaround:**
Reset zoom to default by restarting the application.

**Planned Fix:**
Add bounds checking in `Camera::processMouseScroll()`.

---

### Issue #6: Vegetation Not Rendering

**Severity:** Medium
**Status:** Active
**Affected:** Visual completeness

**Description:**
Trees, bushes, and grass are spawned by VegetationManager but not rendered.

**Root Cause:**
VegetationManager is not integrated into the main render loop.

**Workaround:**
None. Vegetation visually absent.

**Planned Fix:**
1. Create VegetationRenderer class
2. Integrate into main render loop after terrain pass
3. Implement instanced rendering for grass clusters

---

### Issue #7: Delta Time Spikes on Focus Loss

**Severity:** Low
**Status:** Fixed
**Affected:** Simulation stability

**Description:**
When the window loses focus and regains it, the delta time can spike to very large values, causing creatures to teleport or simulation to become unstable.

**Fix Applied:**
```cpp
// Cap delta time to prevent huge jumps
if (deltaTime > 0.1f) {
    deltaTime = 0.1f;  // Max 100ms (10 FPS minimum)
}
```

---

## Low Priority Issues

### Issue #8: First Mouse Movement Jump

**Severity:** Low
**Status:** Fixed
**Affected:** Camera control

**Description:**
First mouse movement after capturing cursor causes camera to jump.

**Fix Applied:**
```cpp
if (firstMouse) {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
    return;  // Skip this frame
}
```

---

### Issue #9: DLL Dependencies on Windows

**Severity:** Low
**Status:** Fixed
**Affected:** Windows deployment

**Description:**
Application fails to start due to missing DLLs.

**Required DLLs:**
- glfw3.dll
- glew32.dll
- libgcc_s_seh-1.dll
- libstdc++-6.dll
- libwinpthread-1.dll

**Fix Applied:**
All required DLLs are now included in the build/ directory.

---

## Planned Improvements

### Short Term
- [ ] Add fallback mesh for creature rendering failures
- [ ] Implement vegetation rendering
- [ ] Add focus check to keyboard input
- [ ] Fix vertex struct alignment

### Medium Term
- [x] Implement proper Win32 cursor handling (DONE - 2026-01-14)
- [ ] Add GPU-side frustum culling
- [ ] Implement LOD for distant creatures
- [ ] Add shadow mapping

### Long Term
- [ ] Async compute for simulation updates
- [ ] Multi-threaded creature AI
- [ ] Deferred rendering pipeline
- [ ] HDR and tone mapping

---

## Debugging Tips

### Creature Visibility Issues
1. Check `Simulation::getCreatureCount()` - should be non-zero
2. Enable wireframe mode to see if meshes exist
3. Add debug output for mesh vertex counts
4. Verify camera is within world bounds

### Input Issues
1. Check `windowFocused` global variable
2. Check `mouseCaptured` state
3. Verify GLFW window has focus via callback
4. On Windows, check Win32 cursor counter

### Performance Issues
1. Profile with PIX or RenderDoc
2. Check instance buffer update time
3. Verify spatial grid is being used
4. Monitor draw call count

---

## Reporting New Issues

When reporting issues, include:
1. Steps to reproduce
2. Expected vs actual behavior
3. Platform (Windows version, GPU)
4. Relevant console output
5. Screenshot if visual issue
