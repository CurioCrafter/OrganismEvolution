# BUG ANALYSIS & RESEARCH FINDINGS

## CRITICAL FINDINGS

### BUG 1: Creatures Not Visible - Metaball/Marching Cubes Issue

**RESEARCH FINDINGS:**

1. **Isovalue Problem (CRITICAL)**
   - File: src/graphics/procedural/MetaballSystem.h, line 18
   - Default threshold: 1.0f (line 18)
   - MetaballSystem.evaluatePotential() returns SUM of influences
   - Problem: Threshold=1.0 may be TOO HIGH for marching cubes
   - For single metaball: max influence = 1.0 * (1 - 0)^2 = 1.0
   - For multiple metaballs: influences STACK, can exceed 1.0
   - ACTION: Check if isovalue threshold matches actual potential values

2. **Vertex Generation Validation**
   - File: src/graphics/rendering/CreatureMeshCache.cpp, line 79
   - Mesh is cached with: "Cached mesh with X vertices"
   - If X = 0, marching cubes returned empty geometry
   - MarchingCubes.cpp line 396-403: cubeIndex computed from 8 corner values
   - If NO corners pass isovalue test, no triangles generated
   - ACTION: Add validation: if vertices.size() == 0, DEBUG

3. **Mesh Upload Path**
   - File: src/graphics/mesh/MeshData.h, line 74-87
   - upload() function checks: if (vertices.empty()) return;
   - Silent return = VAO never created
   - Renderer tries to use VAO=0
   - File: src/graphics/rendering/CreatureRenderer.cpp, line 100-106
   - renderBatch checks: if (!batch.mesh || batch.mesh->VAO == 0)
   - ACTION: Log when VAO=0, check mesh vertex count

**ROOT CAUSE HYPOTHESIS:**
- Isovalue=1.0 vs actual potential values mismatch
- Creatures generate 0 vertices
- Silent failure in mesh upload
- Renderer skips rendering (VAO==0 check passes silently)

---

### BUG 2: Mouse Cursor Not Hiding - Win32 ShowCursor Counter

**RESEARCH FINDINGS:**

1. **Current Implementation (GLFW)**
   - File: src/main.cpp, line 192-199
   - Using GLFW API: glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED)
   - This works on GLFW level
   - Problem: GLFW may not reach Win32 ShowCursor on some versions

2. **Win32 ShowCursor Behavior (CRITICAL INSIGHT)**
   - ShowCursor() is a COUNTER, not a boolean!
   - Each call increments/decrements internal counter
   - Counter > 0 = cursor visible
   - Counter <= 0 = cursor hidden
   - Multiple calls can desynchronize the counter
   - ACTION: NEVER call ShowCursor() multiple times
   - ACTION: Always check current state first

3. **GetForegroundWindow Requirement**
   - Only works on FOREGROUND window
   - Need: HWND hwnd = GetForegroundWindow();
   - Then: ShowCursor(FALSE); // when counter < 0
   - Problem: glfwGetWin32Window() needed to get HWND from GLFW
   - ACTION: Use Win32 API directly if GLFW method fails

4. **Potential GLFW Bug**
   - glfwSetInputMode might not properly sync with Win32 ShowCursor
   - GLFW version might not call ShowCursor correctly
   - ACTION: Add Win32 fallback with proper ShowCursor usage

**ROOT CAUSE HYPOTHESIS:**
- GLFW cursor disable not reaching Win32 ShowCursor properly
- ShowCursor counter getting out of sync
- Need to implement Win32 API directly with counter awareness

---

### BUG 3: Black Triangle Artifact - DX12 Vertex Buffer Alignment

**RESEARCH FINDINGS:**

1. **Vertex Structure Analysis**
   - File: src/graphics/mesh/MeshData.h, line 8-18
   - struct Vertex {
   -     glm::vec3 position;  // 12 bytes, offset 0
   -     glm::vec3 normal;    // 12 bytes, offset 12
   -     glm::vec2 texCoord;  // 8 bytes, offset 24
   - }
   - Total size: 32 bytes (GOOD - 16 byte aligned)
   - BUT: glm::vec3 = 3 floats = 12 bytes (NOT 16 bytes!)
   
2. **SIMD Alignment Issue (CRITICAL)**
   - GLM by default: vec3 = 3 floats = 12 bytes
   - Direct3D expects: vec3 = 16 bytes (SIMD aligned)
   - This is OpenGL code, not DX12, but SAME alignment rules apply
   - Problem: Position offset = 0 (correct)
   - Problem: Normal offset = 12 (NOT 16-byte aligned!)
   - In SIMD: reading from offset 12 crosses cache line boundary
   - This causes incorrect data reads from GPU
   
3. **Vertex Attribute Pointer (MeshData.h, line 95-106)**
   - Line 95: glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
   - stride = sizeof(Vertex) = 32 bytes ✓
   - offset = 0 ✓
   - Line 99-100: Normal attribute pointer
   - stride = sizeof(Vertex) = 32 bytes ✓
   - offset = offsetof(Vertex, normal) = 12 bytes ✗ MISALIGNED
   
4. **Why Black Triangle?**
   - Misaligned read causes GPU to read garbage for normal
   - Normal = garbage vector
   - Lighting calculation = wrong
   - Result = pitch black triangle
   
5. **Vec3 Padding Solution**
   - Change struct to use 16-byte alignment:
   ```cpp
   struct Vertex {
       glm::vec3 position;    // 12 bytes
       float padding1;        // 4 bytes
       glm::vec3 normal;      // 12 bytes
       float padding2;        // 4 bytes
       glm::vec2 texCoord;    // 8 bytes
       // Total: 40 bytes (safe)
   }
   ```
   - OR use vec4 for positions/normals

**ROOT CAUSE HYPOTHESIS:**
- Vertex structure NOT SIMD aligned
- Normal attribute reads from offset 12 (cache-miss)
- Corrupted normal data = black rendering

---

### BUG 4: Input When Unfocused - Window Focus Detection

**RESEARCH FINDINGS:**

1. **Current Implementation (Good but incomplete)**
   - File: src/main.cpp, line 33 & 96-110
   - windowFocused global variable tracks focus state
   - window_focus_callback sets windowFocused = (focused == GLFW_TRUE)
   - File: src/main.cpp, line 117
   - mouse_callback has Guard 2: if (!windowFocused) return;
   
2. **Problem: processInput() has NO focus check!**
   - File: src/main.cpp, line 140-207
   - processInput(window) called in main loop (line 490)
   - Uses glfwGetKey() which reads OS key state
   - NO CHECK for windowFocused!
   - Result: WASD/controls work even when window unfocused
   
3. **GetAsyncKeyState Behavior (Win32)**
   - GetAsyncKeyState(key) returns key state from OS
   - High bit set = key pressed NOW
   - Works regardless of window focus!
   - This is exactly the problem
   
4. **Solution: Add Focus Check in processInput**
   - File: src/main.cpp, line 140
   - Insert at start: if (!windowFocused) return;
   - OR: Check each glfwGetKey with: && windowFocused
   
5. **Alternative for GetAsyncKeyState Usage**
   - If using GetAsyncKeyState: need to check GetForegroundWindow
   - HWND currentWindow = GetForegroundWindow();
   - if (currentWindow != appWindow) return; // Don't process input

**ROOT CAUSE HYPOTHESIS:**
- processInput() missing windowFocused check
- Input processed even when window unfocused
- Need guard at start of processInput() function

---

## SUMMARY TABLE

| Bug | Root Cause | File | Line | Fix Type |
|-----|-----------|------|------|----------|
| 1 | Isovalue mismatch or 0 vertices | MarchingCubes.cpp MetaballSystem.h | 18, 396-403 | Validation + threshold tuning |
| 2 | GLFW cursor not syncing with Win32 | main.cpp | 192-199 | Win32 API fallback + ShowCursor counter |
| 3 | Vertex struct NOT SIMD aligned | MeshData.h | 8-18 | Add padding to vec3 fields |
| 4 | processInput() missing focus check | main.cpp | 140 | Add windowFocused guard |

---

