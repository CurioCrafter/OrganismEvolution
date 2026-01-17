# OpenGL Files to Remove or Replace for DX12 Migration

This document lists all OpenGL-specific source files that need to be removed or replaced when migrating to DirectX 12.

## Summary

| Category | Files Count | Action Required |
|----------|-------------|-----------------|
| Graphics Core | 5 | Complete Rewrite |
| Core/Window | 2 | Complete Rewrite |
| Main Application | 1 | Significant Rewrite |

---

## Graphics Directory Files

### 1. Shader.h / Shader.cpp

**Path:**
- `C:\Users\andre\Desktop\OrganismEvolution\src\graphics\Shader.h`
- `C:\Users\andre\Desktop\OrganismEvolution\src\graphics\Shader.cpp`

**Why it's OpenGL-specific:**
- Includes `<GL/glew.h>`
- Uses `GLuint` for shader program ID
- Uses OpenGL shader functions: `glCreateShader`, `glShaderSource`, `glCompileShader`, `glCreateProgram`, `glAttachShader`, `glLinkProgram`, `glDeleteShader`, `glDeleteProgram`, `glUseProgram`
- Uses OpenGL uniform functions: `glUniform1i`, `glUniform1f`, `glUniform3fv`, `glUniformMatrix4fv`, `glGetUniformLocation`
- Uses OpenGL error checking: `glGetShaderiv`, `glGetShaderInfoLog`, `glGetProgramiv`, `glGetProgramInfoLog`

**DX12 Replacement Needed:**
- Create `D3D12Shader` class using D3D12 shader compilation (D3DCompile/DXC)
- Use `ID3D12PipelineState` for shader programs
- Use `ID3D12RootSignature` for uniform/constant buffer bindings
- Implement root constant buffers or descriptor heaps for uniform data

**Logic to Preserve:**
- File loading logic (reading shader source from files)
- Uniform setter interface (setBool, setInt, setFloat, setVec3, setMat4)
- Error checking and logging patterns

**Action:** API changes required. Logic structure can be preserved.

---

### 2. ShadowMap.h / ShadowMap.cpp

**Path:**
- `C:\Users\andre\Desktop\OrganismEvolution\src\graphics\ShadowMap.h`
- `C:\Users\andre\Desktop\OrganismEvolution\src\graphics\ShadowMap.cpp`

**Why it's OpenGL-specific:**
- Includes `<GL/glew.h>`
- Uses `GLuint` for FBO and texture handles
- Uses `GLenum` for texture units
- Framebuffer operations: `glGenFramebuffers`, `glBindFramebuffer`, `glFramebufferTexture2D`, `glCheckFramebufferStatus`, `glDeleteFramebuffers`
- Texture operations: `glGenTextures`, `glBindTexture`, `glTexImage2D`, `glTexParameteri`, `glTexParameterfv`, `glDeleteTextures`, `glActiveTexture`
- Render state: `glViewport`, `glClear`, `glDrawBuffer`, `glReadBuffer`
- OpenGL constants: `GL_FRAMEBUFFER`, `GL_TEXTURE_2D`, `GL_DEPTH_COMPONENT`, `GL_DEPTH_ATTACHMENT`, etc.

**DX12 Replacement Needed:**
- Create depth buffer using `ID3D12Resource` with `D3D12_RESOURCE_DIMENSION_TEXTURE2D`
- Use `ID3D12DescriptorHeap` for DSV (Depth Stencil View) and SRV (Shader Resource View)
- Implement resource barriers for depth buffer state transitions
- Use `ID3D12GraphicsCommandList::ClearDepthStencilView` for clearing

**Logic to Preserve:**
- Light space matrix calculation (`updateLightSpaceMatrix`)
- Shadow map dimensions and configuration
- Bind for reading/writing workflow concept

**Action:** Complete rewrite required. Mathematical logic (matrix calculations) preserved.

---

### 3. Camera.h / Camera.cpp

**Path:**
- `C:\Users\andre\Desktop\OrganismEvolution\src\graphics\Camera.h`
- `C:\Users\andre\Desktop\OrganismEvolution\src\graphics\Camera.cpp`

**Why it's OpenGL-specific:**
- Includes `<GL/glew.h>` (though minimally used)
- Uses `GLboolean` type in `processMouseMovement` function signature

**DX12 Replacement Needed:**
- Remove `<GL/glew.h>` include
- Replace `GLboolean` with standard `bool` type

**Logic to Preserve:**
- ALL camera logic (view matrix, projection matrix, frustum culling)
- Movement processing
- Mouse input handling
- The entire Frustum class is API-agnostic and should be kept as-is

**Action:** Minimal changes required. Replace GL types with standard C++ types.

---

### 4. MeshData.h

**Path:**
- `C:\Users\andre\Desktop\OrganismEvolution\src\graphics\mesh\MeshData.h`

**Why it's OpenGL-specific:**
- Includes `<GL/glew.h>`
- Uses `GLuint` for VAO, VBO, EBO handles
- Uses OpenGL buffer functions: `glGenVertexArrays`, `glGenBuffers`, `glBindVertexArray`, `glBindBuffer`, `glBufferData`
- Uses vertex attribute functions: `glVertexAttribPointer`, `glEnableVertexAttribArray`
- Uses cleanup functions: `glDeleteVertexArrays`, `glDeleteBuffers`
- Uses OpenGL constants: `GL_ARRAY_BUFFER`, `GL_ELEMENT_ARRAY_BUFFER`, `GL_STATIC_DRAW`, `GL_FLOAT`, `GL_FALSE`

**DX12 Replacement Needed:**
- Use `ID3D12Resource` for vertex and index buffers (committed or placed resources)
- Create `D3D12_VERTEX_BUFFER_VIEW` and `D3D12_INDEX_BUFFER_VIEW` structures
- Use `ID3D12Device::CreateCommittedResource` for buffer creation
- Use upload heaps and copy commands for GPU data transfer

**Logic to Preserve:**
- `Vertex` struct definition (position, normal, texCoord)
- Bounds calculation logic
- Move semantics pattern
- Vertex/index data storage

**Action:** Complete rewrite of GPU resource management. Data structures preserved.

---

### 5. CreatureRenderer.h / CreatureRenderer.cpp

**Path:**
- `C:\Users\andre\Desktop\OrganismEvolution\src\graphics\rendering\CreatureRenderer.h`
- `C:\Users\andre\Desktop\OrganismEvolution\src\graphics\rendering\CreatureRenderer.cpp`

**Why it's OpenGL-specific:**
- Uses `GLuint` for instance VBO
- Buffer operations: `glGenBuffers`, `glDeleteBuffers`, `glBindBuffer`, `glBufferData`
- Vertex attribute setup: `glBindVertexArray`, `glVertexAttribPointer`, `glEnableVertexAttribArray`, `glDisableVertexAttribArray`, `glVertexAttribDivisor`
- Draw calls: `glDrawElementsInstanced`
- OpenGL constants: `GL_ARRAY_BUFFER`, `GL_FLOAT`, `GL_FALSE`, `GL_DYNAMIC_DRAW`, `GL_TRIANGLES`, `GL_UNSIGNED_INT`

**DX12 Replacement Needed:**
- Create structured buffer for instance data using `ID3D12Resource`
- Use `ID3D12GraphicsCommandList::DrawIndexedInstanced` for instanced rendering
- Implement input layout descriptions in pipeline state
- Handle per-instance vertex buffer views

**Logic to Preserve:**
- `CreatureInstanceData` struct layout
- `RenderBatch` grouping logic
- Frustum culling integration
- Statistics tracking

**Action:** Complete rewrite of rendering. Batching and culling logic preserved.

---

## Core Directory Files

### 6. GLFWContext.h

**Path:**
- `C:\Users\andre\Desktop\OrganismEvolution\src\core\GLFWContext.h`

**Why it's OpenGL-specific:**
- Includes `<GLFW/glfw3.h>`
- RAII wrapper for GLFW initialization (`glfwInit`, `glfwTerminate`)
- Window creation with OpenGL context hints:
  - `GLFW_CONTEXT_VERSION_MAJOR/MINOR`
  - `GLFW_OPENGL_PROFILE`
  - `GLFW_OPENGL_DEBUG_CONTEXT`
- Window management: `glfwCreateWindow`, `glfwDestroyWindow`, `glfwMakeContextCurrent`, `glfwSwapBuffers`, `glfwWindowShouldClose`

**DX12 Replacement Needed:**
- Option A: Use pure Win32 API for window creation (`CreateWindowEx`, `RegisterClassEx`)
- Option B: Keep GLFW but remove OpenGL context creation, use for windowing only
- Remove OpenGL-specific window hints
- Create DXGI swap chain instead of using `glfwSwapBuffers`

**Logic to Preserve:**
- RAII pattern for window lifecycle
- Window dimension and title management
- Non-copyable semantics

**Action:** Complete replacement with Win32 window class or stripped-down GLFW wrapper.

---

### 7. Simulation.h / Simulation.cpp

**Path:**
- `C:\Users\andre\Desktop\OrganismEvolution\src\core\Simulation.h`
- `C:\Users\andre\Desktop\OrganismEvolution\src\core\Simulation.cpp`

**Why it's OpenGL-specific:**
- Uses `GLuint` for VAO/VBO handles (foodVAO, foodVBO, nametagVAO, nametagVBO, grassVAO, grassVBO, grassInstanceVBO, debugVAO, debugVBO)
- Uses `GLint` for cached viewport
- Uses `GLenum` constants (GL_TEXTURE0)
- Extensive OpenGL calls throughout:
  - Buffer management: `glGenVertexArrays`, `glGenBuffers`, `glDeleteVertexArrays`, `glDeleteBuffers`, `glBindVertexArray`, `glBindBuffer`, `glBufferData`, `glBufferSubData`
  - Vertex attributes: `glVertexAttribPointer`, `glEnableVertexAttribArray`, `glVertexAttribDivisor`
  - Render state: `glEnable`, `glDisable`, `glDepthFunc`, `glCullFace`, `glBlendFunc`, `glPolygonOffset`, `glViewport`, `glClear`, `glLineWidth`
  - Draw calls: `glDrawArrays`, `glDrawElements`, `glDrawArraysInstanced`, `glDrawElementsInstanced`
  - State queries: `glGetIntegerv`

**DX12 Replacement Needed:**
- Replace all VAO/VBO management with `ID3D12Resource` buffers
- Replace render state with pipeline state objects
- Replace draw calls with command list methods
- Replace viewport management with `D3D12_VIEWPORT` and `RSSetViewports`
- Implement proper resource barrier transitions

**Logic to Preserve:**
- ALL simulation logic (creature spawning, updates, evolution, reproduction)
- Population balance algorithms
- Food spawning system
- Vegetation rendering organization
- Shadow pass workflow
- Debug visualization patterns

**Action:** Significant rewrite of rendering code. All simulation logic preserved.

---

## Main Application File

### 8. main.cpp

**Path:**
- `C:\Users\andre\Desktop\OrganismEvolution\src\main.cpp`

**Why it's OpenGL-specific:**
- Includes `<GL/glew.h>` and `<GLFW/glfw3.h>`
- GLEW initialization: `glewExperimental`, `glewInit`
- Debug callback setup: `glDebugMessageCallback`, `glDebugMessageControl`
- OpenGL state: `glViewport`, `glEnable`, `glClearColor`, `glClear`
- OpenGL queries: `glGetString`, `glGetIntegerv`, `glGetBooleanv`
- GLFW input: `glfwGetKey`, `glfwSetInputMode`, `glfwRawMouseMotionSupported`
- GLFW callbacks: framebuffer_size_callback, window_focus_callback, mouse_callback, scroll_callback
- Test rendering with VAO/VBO
- Window management via `GLFWContext` and `GLFWWindow`

**DX12 Replacement Needed:**
- Replace GLEW with D3D12 device creation (`D3D12CreateDevice`)
- Replace GLFW window with Win32 window or GLFW without OpenGL context
- Create DXGI factory, adapter enumeration, swap chain
- Replace OpenGL debug callback with D3D12 debug layer (`ID3D12Debug`)
- Replace `glClear` with `ClearRenderTargetView` and `ClearDepthStencilView`
- Replace input handling (keep GLFW input or use Win32 messages)
- Create command queue, command allocator, command list
- Implement frame synchronization with fence

**Logic to Preserve:**
- Input processing patterns (keyboard, mouse)
- Camera control logic
- Simulation speed control
- Debug mode toggling
- Screenshot functionality
- HUD rendering workflow
- Main loop structure

**Action:** Major rewrite of initialization and rendering. Input and control logic preserved.

---

## Files That Are API-Agnostic (No Changes Needed)

These files have no OpenGL dependencies and can be used as-is:

### Graphics:
- `Frustum.h / Frustum.cpp` - Pure math, uses only GLM
- `CreatureMeshCache.h / CreatureMeshCache.cpp` - Uses MeshData but no direct GL calls
- `procedural/MarchingCubes.h / MarchingCubes.cpp` - Mesh generation algorithms
- `procedural/MorphologyBuilder.h / MorphologyBuilder.cpp` - Creature shape generation
- `procedural/CreatureBuilder.h / CreatureBuilder.cpp` - Creature mesh building
- `procedural/MetaballSystem.h` - Metaball algorithms

### Note:
CreatureMeshCache will need updates only to work with the new MeshData DX12 implementation.

---

## Migration Priority Order

1. **Phase 1 - Window & Device Setup:**
   - `GLFWContext.h` -> Win32Window or DXGIWindow
   - `main.cpp` initialization section

2. **Phase 2 - Resource Management:**
   - `MeshData.h` -> D3D12MeshData
   - `Shader.h/cpp` -> D3D12Shader/PipelineState

3. **Phase 3 - Rendering Infrastructure:**
   - `ShadowMap.h/cpp` -> D3D12ShadowMap
   - `CreatureRenderer.h/cpp` -> D3D12CreatureRenderer

4. **Phase 4 - Integration:**
   - `Simulation.h/cpp` rendering methods
   - `main.cpp` render loop
   - `Camera.h` (minor GL type removal)

---

## OpenGL Functions Reference

For reference, here are all OpenGL functions used that need DX12 equivalents:

### Buffer Management
| OpenGL | DX12 Equivalent |
|--------|-----------------|
| `glGenVertexArrays` | Input layout in PSO |
| `glGenBuffers` | `CreateCommittedResource` |
| `glBindVertexArray` | `IASetVertexBuffers` + `IASetIndexBuffer` |
| `glBindBuffer` | Resource binding in descriptor heap |
| `glBufferData` | `Map`/`Unmap` or copy queue |
| `glBufferSubData` | `Map`/`Unmap` for updates |
| `glDeleteVertexArrays` | Release `ID3D12Resource` |
| `glDeleteBuffers` | Release `ID3D12Resource` |

### Shader Management
| OpenGL | DX12 Equivalent |
|--------|-----------------|
| `glCreateShader` | `D3DCompile` or DXC |
| `glShaderSource` | Compile from source string |
| `glCompileShader` | `D3DCompile` |
| `glCreateProgram` | `CreateGraphicsPipelineState` |
| `glLinkProgram` | Part of PSO creation |
| `glUseProgram` | `SetPipelineState` |
| `glUniform*` | Root constants or CBV |

### Draw Calls
| OpenGL | DX12 Equivalent |
|--------|-----------------|
| `glDrawArrays` | `DrawInstanced` |
| `glDrawElements` | `DrawIndexedInstanced` |
| `glDrawElementsInstanced` | `DrawIndexedInstanced` |
| `glDrawArraysInstanced` | `DrawInstanced` |

### State Management
| OpenGL | DX12 Equivalent |
|--------|-----------------|
| `glEnable(GL_DEPTH_TEST)` | Depth stencil state in PSO |
| `glEnable(GL_BLEND)` | Blend state in PSO |
| `glEnable(GL_CULL_FACE)` | Rasterizer state in PSO |
| `glViewport` | `RSSetViewports` |
| `glClear` | `ClearRenderTargetView` / `ClearDepthStencilView` |

### Texture/Framebuffer
| OpenGL | DX12 Equivalent |
|--------|-----------------|
| `glGenTextures` | `CreateCommittedResource` (texture) |
| `glBindTexture` | SRV in descriptor heap |
| `glTexImage2D` | Upload via copy queue |
| `glGenFramebuffers` | Create RTV/DSV resources |
| `glBindFramebuffer` | `OMSetRenderTargets` |
| `glFramebufferTexture2D` | Create RTV/DSV descriptors |
