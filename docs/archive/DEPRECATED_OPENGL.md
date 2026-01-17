# DEPRECATED: OpenGL Implementation

> **NOTE**: This OpenGL-based rendering system has been superseded by the DirectX 12 implementation in `ModularGameEngine/SimulationDemo`.

## Migration Status

The OrganismEvolution project has been **fully ported** to the ModularGameEngine using DirectX 12. The OpenGL code in this repository is kept for reference only and should not be used for new development.

**New Project Location**: `C:\Users\andre\Desktop\ModularGameEngine\SimulationDemo`

---

## OpenGL Files (Reference Only)

### Graphics Core

| File | Purpose | Status |
|------|---------|--------|
| `src/graphics/Shader.h/.cpp` | GLSL shader compilation | Replaced by HLSL + DXC |
| `src/graphics/Camera.h/.cpp` | OpenGL camera system | Ported to SimulationDemo |
| `src/graphics/mesh/MeshData.h` | VAO/VBO/EBO management | Replaced by DX12 buffers |

### Rendering System

| File | Purpose | Status |
|------|---------|--------|
| `src/graphics/rendering/CreatureRenderer.h/.cpp` | Instanced creature rendering | Ported to DX12 |
| `src/graphics/rendering/CreatureMeshCache.h` | Mesh pooling | Simplified in port |

### Procedural Generation (Ported)

| File | Purpose | Status |
|------|---------|--------|
| `src/graphics/procedural/MarchingCubes.h` | Metaball mesh extraction | **Ported** |
| `src/graphics/procedural/MetaballSystem.h` | Organic shapes | **Ported** |
| `src/graphics/procedural/CreatureBuilder.h` | Genome to metaballs | **Ported** |

### Shaders (OpenGL/GLSL)

| File | Purpose | Replacement |
|------|---------|-------------|
| `shaders/vertex.glsl` | Terrain vertex shader | `Runtime/Shaders/Terrain.hlsl` |
| `shaders/fragment.glsl` | Terrain fragment shader | `Runtime/Shaders/Terrain.hlsl` |
| `shaders/creature_vertex.glsl` | Creature vertex shader | `Runtime/Shaders/Creature.hlsl` |
| `shaders/creature_fragment.glsl` | Creature fragment shader | `Runtime/Shaders/Creature.hlsl` |
| `shaders/tree_vertex.glsl` | Tree vertex shader | Not ported (trees removed) |
| `shaders/tree_fragment.glsl` | Tree fragment shader | Not ported |
| `shaders/nametag_vertex.glsl` | Nametag vertex shader | `Runtime/Shaders/Nametag.hlsl` |
| `shaders/nametag_fragment.glsl` | Nametag fragment shader | `Runtime/Shaders/Nametag.hlsl` |

---

## OpenGL API Usage (Deprecated)

The following OpenGL functions were used and are now replaced:

### Buffer Management
- `glGenVertexArrays()` / `glDeleteVertexArrays()` -> `IDevice::CreateBuffer()`
- `glGenBuffers()` / `glDeleteBuffers()` -> `IDevice::CreateBuffer()`
- `glBindVertexArray()` / `glBindBuffer()` -> `ICommandList::BindVertexBuffer()`
- `glBufferData()` -> `IBuffer::Map()` / `Unmap()`

### Rendering
- `glDrawElements()` -> `ICommandList::DrawIndexed()`
- `glDrawElementsInstanced()` -> `ICommandList::DrawIndexedInstanced()`
- `glVertexAttribPointer()` -> Input layout in pipeline state
- `glVertexAttribDivisor()` -> Instance input layout

### Shaders
- `glCreateShader()` / `glCompileShader()` -> `ShaderCompiler::Compile()`
- `glCreateProgram()` / `glLinkProgram()` -> Pipeline State Object
- `glUniform*()` -> Constant buffer updates

### State Management
- `glEnable(GL_DEPTH_TEST)` -> Pipeline State depth-stencil state
- `glViewport()` -> `ICommandList::SetViewport()`
- `glClearColor()` / `glClear()` -> `ICommandList::ClearRenderTarget()`

---

## Porting Notes

### What Changed in the Port

1. **Shader Language**: GLSL -> HLSL
   - `vec3` -> `float3`
   - `mat4` -> `float4x4`
   - `texture()` -> `Sample()`
   - Column-major matrices (same convention kept)

2. **Buffer Binding**: OpenGL's VAO system -> DX12 root signature + descriptor heaps

3. **Draw Calls**: OpenGL immediate state -> DX12 command lists

4. **Synchronization**: OpenGL implicit sync -> DX12 explicit fences

### What Was Preserved

1. **Procedural Generation**: Perlin noise, metaballs, marching cubes
2. **Biome System**: Height-based terrain coloring
3. **Creature Meshes**: Metaball-based organic shapes
4. **Animation**: Per-instance animation phase

---

## Do Not Use

The OpenGL code should NOT be:
- Extended with new features
- Used as a basis for new projects
- Maintained or bug-fixed

All development should happen in `ModularGameEngine/SimulationDemo`.
