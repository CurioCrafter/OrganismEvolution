# Technology Decisions Document
## OrganismEvolution - Architecture Decision Records & Rationale

### Document Version
- **Created**: January 2026
- **Last Updated**: January 2026
- **Scope**: Technical decision documentation and future reference

---

## Overview

This document records significant technology and architecture decisions made for the OrganismEvolution project, following the Architecture Decision Record (ADR) format. Each decision includes context, alternatives considered, and rationale.

---

## ADR-001: Graphics API Selection (OpenGL)

### Status
**Accepted** - Implemented

### Context
The simulation requires real-time 3D rendering of hundreds to thousands of creatures, terrain, and vegetation. The choice of graphics API affects portability, performance ceiling, and development complexity.

### Decision
Use **OpenGL 4.3+** as the primary graphics API.

### Alternatives Considered

| Option | Pros | Cons |
|--------|------|------|
| **OpenGL 4.3+** | Cross-platform, mature tooling, familiar API | Single command queue, CPU overhead |
| **Vulkan** | Multi-threaded, low CPU overhead, modern | Complex API, longer dev time, overkill for current scale |
| **DirectX 12** | Excellent Windows performance, good tooling | Windows-only, complex like Vulkan |
| **DirectX 11** | Simpler than DX12, good Windows support | Windows-only, no cross-platform |
| **Metal** | Best macOS/iOS performance | Apple-only |
| **WebGPU** | Future web deployment | Immature, limited features |

### Rationale
1. **Cross-Platform Priority**: OpenGL runs on Windows, Linux, and macOS
2. **Sufficient Performance**: OpenGL 4.3 supports compute shaders, instanced rendering, and all features needed for current scale (1000s of creatures)
3. **Development Velocity**: Team familiarity with OpenGL, extensive documentation and examples
4. **GPU-Driven Rendering Not Yet Needed**: The primary benefit of Vulkan/DX12 (multi-threaded command recording) isn't a current bottleneck
5. **GLEW + GLFW Ecosystem**: Mature, stable libraries for context management

### Consequences
- Limited to ~10K creatures before GPU command submission becomes a bottleneck
- Cannot leverage explicit multi-queue GPU operations
- Future migration to Vulkan possible if needed (similar shader model)

### Future Reconsideration Triggers
- When GPU draw call submission exceeds 5ms per frame
- When compute shader dispatch overhead becomes significant
- When targeting platforms that don't support OpenGL (consoles)

---

## ADR-002: Window/Input Framework (GLFW)

### Status
**Accepted** - Implemented

### Context
Need a cross-platform windowing and input library that works with OpenGL.

### Decision
Use **GLFW 3.x** for window management and input handling.

### Alternatives Considered

| Option | Pros | Cons |
|--------|------|------|
| **GLFW** | Lightweight, OpenGL-focused, stable | No built-in audio |
| **SDL2** | Audio included, broader platform support | Heavier, more complex |
| **SFML** | High-level, easy to use | Less control, C++ only |
| **Raw Win32/X11** | Maximum control | Platform-specific code |

### Rationale
1. **Simplicity**: GLFW does one thing well - windows and input
2. **OpenGL Integration**: Seamless context creation
3. **Small Footprint**: Minimal dependencies
4. **Raw Mouse Motion**: Critical for camera controls (`glfwRawMouseMotionSupported()`)

### Consequences
- Need separate audio library (not yet implemented)
- No built-in GUI (addressed with ImGui)

---

## ADR-003: UI Framework (Dear ImGui)

### Status
**Accepted** - Implemented

### Context
Need debug UI and dashboard for monitoring simulation statistics, creature inspection, and parameter adjustment.

### Decision
Use **Dear ImGui** for immediate-mode GUI.

### Alternatives Considered

| Option | Pros | Cons |
|--------|------|------|
| **Dear ImGui** | Lightweight, no state management, easy integration | Not stylable, programmer-focused |
| **Qt** | Professional look, powerful | Heavy dependency, licensing |
| **Nuklear** | Similar to ImGui, C | Less features, smaller community |
| **Custom HTML/Electron overlay** | Modern UI | Heavy, complex integration |
| **In-engine retained mode GUI** | Full control | Large development effort |

### Rationale
1. **Development Speed**: ImGui requires minimal code for functional UI
2. **Debug-First Philosophy**: Simulation is data-heavy; ImGui excels at displaying data
3. **Performance**: Minimal overhead when not rendering
4. **No State Management**: Fits the simulation's update loop pattern

### Consequences
- Dashboard looks "programmer-ish" rather than polished
- Not suitable for end-user-facing final product UI
- Limited animation/styling capabilities

### Future Reconsideration Triggers
- If targeting general audience (would need styled UI)
- If complex multi-window layouts needed

---

## ADR-004: Math Library (GLM)

### Status
**Accepted** - Implemented

### Context
Need vector/matrix math operations for 3D graphics and physics.

### Decision
Use **GLM (OpenGL Mathematics)** for all vector and matrix operations.

### Alternatives Considered

| Option | Pros | Cons |
|--------|------|------|
| **GLM** | Header-only, mirrors GLSL, widespread | No SIMD by default |
| **Eigen** | Fast, SIMD-optimized | Heavy template usage, compile time |
| **DirectXMath** | Fast, SIMD | Windows-only API style |
| **Custom math** | Full control | Bugs, maintenance burden |

### Rationale
1. **GLSL Compatibility**: GLM types (`vec3`, `mat4`) match shader types exactly
2. **Header-Only**: No build complexity
3. **Industry Standard**: Extensive documentation, examples
4. **Sufficient Performance**: Current bottleneck is not math operations

### Consequences
- Not SIMD-optimized by default (can enable with `GLM_FORCE_SIMD_AVX2`)
- Template-heavy can slow compilation

### Future Consideration
- Enable GLM SIMD extensions when profiling shows math as bottleneck
- Consider Eigen for heavy compute (neural networks) if needed

---

## ADR-005: Creature Data Structure (OOP with Components)

### Status
**Accepted** - Implemented, under review for ECS migration

### Context
Creatures have many attributes: position, velocity, genome, neural network brain, sensory systems, reproduction state, etc. Need to organize this data.

### Decision
Use **Object-Oriented design** with a `Creature` class containing all components as members.

### Current Structure
```cpp
class Creature {
    glm::vec3 position;
    glm::vec3 velocity;
    Genome genome;
    genetics::DiploidGenome diploidGenome;
    std::unique_ptr<NeuralNetwork> brain;
    SensorySystem sensory;
    SteeringBehaviors steering;
    float energy, age, fitness;
    // ... 80+ members total
};
```

### Alternatives Considered

| Option | Pros | Cons |
|--------|------|------|
| **OOP (current)** | Intuitive, encapsulated | Cache-unfriendly, hard to parallelize |
| **Entity-Component-System** | Cache-friendly, parallel-ready | Verbose, learning curve |
| **Data-Oriented Design** | Maximum performance | Harder to reason about |
| **Hybrid (Components in Creature)** | Balance of both | Still scattered allocations |

### Rationale (Original Decision)
1. **Conceptual Clarity**: A creature "is" a thing with properties
2. **Encapsulation**: Creature behavior is self-contained
3. **Rapid Prototyping**: Easy to add/modify properties
4. **Small Scale**: At <500 creatures, OOP overhead is acceptable

### Consequences
- Cache misses when iterating all creatures (scattered heap allocations)
- Cannot easily parallelize creature updates (shared mutable state)
- Tightly coupled (hard to update only position without touching Creature)

### Migration Plan
See [ARCHITECTURE_ROADMAP.md](./ARCHITECTURE_ROADMAP.md) for ECS migration strategy:
1. Extract Position/Velocity to Structure-of-Arrays
2. Keep complex components (Genome, Brain) as objects
3. Systems iterate over arrays, reference objects when needed

---

## ADR-006: Neural Network Implementation (Custom CPU)

### Status
**Accepted** - Implemented

### Context
Each creature has a neural network "brain" that decides behavior based on sensory inputs.

### Decision
Use **custom CPU-based feedforward neural network** implementation.

### Current Implementation
```cpp
class NeuralNetwork {
    std::vector<float> weights;  // 120 weights
    // Simple feedforward: 20 inputs → 4 hidden → 10 hidden2 → 2 outputs
    glm::vec2 forward(const std::vector<float>& inputs);
};
```

### Alternatives Considered

| Option | Pros | Cons |
|--------|------|------|
| **Custom CPU** | Simple, no dependencies | Slow at scale |
| **PyTorch/TensorFlow** | Powerful, GPU support | Heavy dependency, C++ bindings complex |
| **ONNX Runtime** | Optimized inference | External dependency |
| **Custom GPU (compute shader)** | Fast, parallel | Development complexity |
| **Behavior Trees** | More interpretable | Less emergent behavior |

### Rationale
1. **Simplicity**: Neural network is small (120 weights), custom is faster than calling library
2. **No Dependencies**: No build complexity
3. **Evolution-Friendly**: Easy to mutate weight vectors
4. **Interpretability**: Can inspect exactly what's happening

### Consequences
- Limited to simple architectures (no RNN, attention)
- CPU-bound at large creature counts
- No GPU acceleration

### Future Migration Path
When creature count exceeds 5000 and neural network forward pass becomes bottleneck:
1. Batch all creature inputs into single buffer
2. Use compute shader for parallel forward pass
3. Read results back for decision making

---

## ADR-007: Spatial Partitioning (Fixed Grid)

### Status
**Accepted** - Implemented, planned for upgrade

### Context
Creatures need to find nearby food, predators, and mates. Naive O(n²) search is too slow.

### Decision
Use **fixed-size 2D spatial grid** (20x20 cells).

### Current Implementation
```cpp
class SpatialGrid {
    float cellWidth, cellDepth;  // ~15 units
    std::vector<std::vector<std::vector<Creature*>>> cells;  // 20x20

    void clear();  // O(400)
    void insert(Creature* c);  // O(1)
    std::vector<Creature*> query(vec3 pos, float radius);  // O(cells in radius)
};
```

### Alternatives Considered

| Option | Pros | Cons |
|--------|------|------|
| **Fixed Grid (current)** | Simple, O(1) insert | Inefficient for large worlds, full rebuild |
| **Spatial Hash** | Dynamic size, O(1) operations | Hash collisions, memory unpredictable |
| **Quadtree** | Adaptive, efficient queries | Complex insert/remove, balancing |
| **BVH** | Great for static geometry | Costly updates for moving objects |
| **Loose Octree** | Good for 3D, stable | More complex, memory overhead |

### Rationale
1. **Fixed World Size**: Terrain is 300x300 units, known at init time
2. **Simplicity**: Easy to implement correctly
3. **Sufficient Performance**: With 300 creatures, ~0.75 per cell average

### Consequences
- Full `clear()` every frame (could be incremental)
- 2D only (flying creatures need 3D queries)
- Poor for non-uniform distribution

### Upgrade Plan
1. Implement incremental updates (dirty flag per creature)
2. Add 3D support via layered grids or octree for flying/aquatic
3. Consider spatial hash for infinite world support

---

## ADR-008: Rendering Strategy (Instanced Forward)

### Status
**Accepted** - Implemented

### Context
Need to render hundreds of creatures efficiently, each with unique position, color, and animation state.

### Decision
Use **instanced forward rendering** with batching by mesh type.

### Current Implementation
```cpp
class CreatureRenderer {
    void render(const std::vector<Creature>& creatures) {
        // Group by mesh key (body type)
        // Upload instance data (model matrix, color, animation phase)
        // Single draw call per mesh type
        glDrawElementsInstanced(...);
    }
};
```

### Alternatives Considered

| Option | Pros | Cons |
|--------|------|------|
| **Instanced Forward (current)** | Simple, sufficient for scale | Fill-rate limited, one light type |
| **Deferred Rendering** | Many lights, decoupled | Memory bandwidth, transparency |
| **Forward+** | Many lights, less memory | Compute shader complexity |
| **GPU-Driven Indirect** | Maximum throughput | Vulkan/DX12 preferred |
| **Individual Draw Calls** | Simple code | Terrible performance |

### Rationale
1. **Single Light Source**: Sun-only lighting doesn't benefit from deferred
2. **Transparency Needs**: Vegetation, water need alpha blending (hard in deferred)
3. **Sufficient Throughput**: 1000 creatures = 10-50 draw calls with batching
4. **Simpler Shadows**: Single cascade shadow map works with forward

### Consequences
- Adding many point lights would be expensive
- Fill-rate bound with many overlapping creatures
- No screen-space effects (AO, reflections) without extra passes

### Future Migration Triggers
- If night-time with many light sources needed → Forward+ or Deferred
- If 10K+ creatures needed → GPU-driven indirect draws

---

## ADR-009: Build System (CMake)

### Status
**Accepted** - Implemented

### Context
Need cross-platform build system that handles C++ compilation, library linking, and shader copying.

### Decision
Use **CMake 3.16+** as the build system.

### Rationale
1. **Industry Standard**: Most C++ projects use CMake
2. **IDE Support**: Generates Visual Studio, Xcode, Ninja, Makefile
3. **Dependency Management**: Works with vcpkg, Conan, FetchContent
4. **Cross-Platform**: Same CMakeLists.txt works on Windows, Linux, macOS

### Current CMakeLists.txt Features
- Finds OpenGL, GLEW, GLFW
- Compiles all source files
- Copies shaders to build directory
- Sets C++17 standard

---

## ADR-010: Future - Why Not DX12/Vulkan Now?

### Status
**Deferred** - Reconsidering when scaling needs arise

### Context
DirectX 12 and Vulkan offer lower CPU overhead and better multi-threading than OpenGL. Why not use them?

### Decision
**Remain on OpenGL** for now, plan migration path for future.

### Detailed Analysis

#### Current Bottleneck Analysis
| Bottleneck | Current Impact | DX12/Vulkan Benefit |
|------------|---------------|---------------------|
| Creature AI (CPU) | **High** | None |
| Physics (CPU) | Medium | None |
| Draw Call Submission | Low | **High** |
| Shader Compilation | Once at startup | Similar |
| Buffer Updates | Low | Medium |
| Shadow Rendering | Medium | Medium (multi-queue) |

**Conclusion**: Current bottleneck is CPU simulation, not GPU command submission. DX12/Vulkan would not address the primary limitation.

#### Development Cost Analysis
| Task | OpenGL (hours) | Vulkan (hours) |
|------|---------------|----------------|
| Basic triangle | 2 | 20 |
| Textured mesh | 4 | 30 |
| Shadow mapping | 8 | 40 |
| Instanced rendering | 4 | 20 |
| Compute shader | 6 | 30 |
| Debug tooling setup | 2 | 10 |

**Total for current features**: ~26 hours (OpenGL) vs ~150 hours (Vulkan)

#### When to Migrate

Vulkan/DX12 becomes worthwhile when:
1. GPU draw call submission exceeds 3ms/frame
2. Multi-GPU support needed
3. Ray tracing required
4. Async compute critical (simulation on GPU)
5. Console ports planned

### Migration Strategy (When Needed)
1. Abstract rendering backend behind interface
2. Implement Vulkan backend alongside OpenGL
3. Use libraries: VMA (memory), SPIRV-Cross (shaders)
4. Gradual port: start with compute, then rendering

---

## ADR-011: Shader Language (GLSL)

### Status
**Accepted** - Implemented

### Context
Need shader language for vertex, fragment, and potentially compute shaders.

### Decision
Use **GLSL 4.30** (OpenGL Shading Language).

### Rationale
1. **OpenGL Match**: GLSL is native to OpenGL
2. **GLM Compatibility**: Same types (`vec3`, `mat4`)
3. **Compute Shader Support**: GLSL 4.30 added compute shaders
4. **Hot Reload Potential**: Text-based shaders can be reloaded at runtime

### Current Shaders
- `vertex.glsl` / `fragment.glsl` - Terrain
- `creature_vertex.glsl` / `creature_fragment.glsl` - Instanced creatures
- `shadow_*.glsl` - Shadow map generation
- `tree_*.glsl` - Vegetation

### Future Consideration
If migrating to Vulkan, use SPIRV-Cross to translate GLSL → SPIR-V, or write in HLSL and cross-compile.

---

## Decision Log Summary

| ID | Decision | Status | Key Trade-off |
|----|----------|--------|---------------|
| ADR-001 | OpenGL 4.3+ | Active | Portability vs raw performance |
| ADR-002 | GLFW | Active | Simplicity vs features |
| ADR-003 | Dear ImGui | Active | Dev speed vs polish |
| ADR-004 | GLM | Active | Ease vs SIMD optimization |
| ADR-005 | OOP Creatures | Under Review | Clarity vs cache efficiency |
| ADR-006 | Custom CPU Neural Net | Active | Simplicity vs scale |
| ADR-007 | Fixed Spatial Grid | Planned Upgrade | Simplicity vs flexibility |
| ADR-008 | Instanced Forward | Active | Simplicity vs many lights |
| ADR-009 | CMake | Active | Standard vs alternatives |
| ADR-010 | Defer DX12/Vulkan | Deferred | Dev time vs future perf |
| ADR-011 | GLSL | Active | OpenGL native |

---

## References

### Decision-Making Resources
- [Architectural Decision Records](https://adr.github.io/)
- [Technology Radar (ThoughtWorks)](https://www.thoughtworks.com/radar)

### API Comparison
- [OpenGL vs Vulkan Performance](https://www.khronos.org/assets/uploads/developers/presentations/)
- [Modern Graphics API Comparison (GDC)](https://www.gdcvault.com/)

### Similar Project Decisions
- [Godot Engine - Why OpenGL?](https://godotengine.org/article/vulkan-progress-report-1)
- [Unity - Rendering Pipelines](https://docs.unity3d.com/Manual/render-pipelines.html)
