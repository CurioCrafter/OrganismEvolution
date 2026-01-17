# Draw Call Optimization Guide

A comprehensive guide to measuring, reducing, and optimizing draw calls for high-performance game rendering.

## Table of Contents

1. [Measuring Draw Call Cost](#measuring-draw-call-cost)
2. [Draw Call Reduction Techniques](#draw-call-reduction-techniques)
3. [Indirect Drawing Guide](#indirect-drawing-guide)
4. [GPU-Driven Rendering Overview](#gpu-driven-rendering-overview)
5. [Level of Detail (LOD) Systems](#level-of-detail-lod-systems)
6. [Culling Strategies](#culling-strategies)
7. [Nanite: Next-Generation Geometry](#nanite-next-generation-geometry)
8. [Practical Implementation](#practical-implementation)
9. [References](#references)

---

## Measuring Draw Call Cost

### Understanding the Bottleneck

Before optimizing, identify whether you're **CPU-bound** or **GPU-bound**:

| Symptom | Bottleneck | Solution |
|---------|------------|----------|
| High CPU usage, GPU idle | CPU-bound | Reduce draw calls |
| Low CPU usage, GPU at 100% | GPU-bound | Optimize shaders, reduce geometry |
| Both high | Mixed | Address both |

### Profiling Tools

#### OpenGL Timer Queries

```cpp
class GPUTimer {
    GLuint queries[2];
    bool resultAvailable = false;

public:
    void begin() {
        glGenQueries(2, queries);
        glQueryCounter(queries[0], GL_TIMESTAMP);
    }

    void end() {
        glQueryCounter(queries[1], GL_TIMESTAMP);
    }

    double getElapsedMs() {
        GLuint64 startTime, endTime;
        glGetQueryObjectui64v(queries[0], GL_QUERY_RESULT, &startTime);
        glGetQueryObjectui64v(queries[1], GL_QUERY_RESULT, &endTime);
        return (endTime - startTime) / 1000000.0;  // ns to ms
    }
};

// Usage
GPUTimer timer;
timer.begin();
renderScene();
timer.end();
printf("GPU time: %.2f ms\n", timer.getElapsedMs());
```

#### Frame Time Breakdown

```cpp
struct FrameStats {
    double cpuPrepareTime;   // Scene setup, culling
    double cpuSubmitTime;    // Draw call submission
    double gpuRenderTime;    // Actual GPU work
    int drawCalls;
    int triangles;
    int instances;
};

void profileFrame(FrameStats& stats) {
    auto cpuStart = std::chrono::high_resolution_clock::now();

    // Prepare phase
    prepareScene();
    auto afterPrepare = std::chrono::high_resolution_clock::now();

    // Submit phase (with GPU timer)
    GPUTimer gpuTimer;
    gpuTimer.begin();
    submitDrawCalls();
    gpuTimer.end();
    glFinish();  // Wait for GPU
    auto afterSubmit = std::chrono::high_resolution_clock::now();

    // Calculate times
    stats.cpuPrepareTime = duration(cpuStart, afterPrepare);
    stats.cpuSubmitTime = duration(afterPrepare, afterSubmit);
    stats.gpuRenderTime = gpuTimer.getElapsedMs();
}
```

#### External Profilers

| Tool | Platform | Features |
|------|----------|----------|
| **RenderDoc** | Cross-platform | Frame capture, GPU debugging, draw call analysis |
| **NVIDIA NSight** | NVIDIA GPUs | Deep GPU profiling, shader debugging |
| **AMD Radeon GPU Profiler** | AMD GPUs | Wavefront analysis, occupancy |
| **Intel GPA** | Intel GPUs | Frame analysis, optimization suggestions |
| **apitrace** | Cross-platform | API call tracing and replay |

### Key Metrics to Track

```cpp
struct RenderMetrics {
    // Draw call metrics
    int totalDrawCalls;
    int instancedDrawCalls;
    int singleInstanceCalls;

    // Geometry metrics
    int totalTriangles;
    int trianglesPerDrawCall;
    int instancesRendered;

    // State change metrics
    int shaderSwitches;
    int textureSwitches;
    int vaoSwitches;

    void print() {
        printf("=== Render Metrics ===\n");
        printf("Draw calls: %d (instanced: %d, single: %d)\n",
               totalDrawCalls, instancedDrawCalls, singleInstanceCalls);
        printf("Triangles: %d (%.1f per call)\n",
               totalTriangles, (float)totalTriangles / totalDrawCalls);
        printf("Instances: %d\n", instancesRendered);
        printf("State changes - Shader: %d, Texture: %d, VAO: %d\n",
               shaderSwitches, textureSwitches, vaoSwitches);
    }
};
```

---

## Draw Call Reduction Techniques

### Technique Comparison

| Technique | Draw Call Reduction | Complexity | Best For |
|-----------|--------------------| -----------|----------|
| **State Sorting** | 20-40% | Low | Any scene |
| **Static Batching** | 50-90% | Medium | Static geometry |
| **GPU Instancing** | 90-99% | Medium | Many identical objects |
| **Multi-Draw Indirect** | 95-99% | High | GPU-driven pipelines |
| **Mesh Merging** | 50-80% | Low | Permanent combinations |

### 1. State Sorting (Always Do This)

Sort draw calls to minimize state changes:

```cpp
// Sort key prioritizes: Shader > Material > Texture > Depth
uint64_t makeSortKey(const DrawCall& call) {
    return ((uint64_t)call.shaderID << 48) |
           ((uint64_t)call.materialID << 32) |
           ((uint64_t)call.textureID << 16) |
           (uint16_t)(call.depth * 65535);
}

std::sort(drawCalls.begin(), drawCalls.end(),
    [](const DrawCall& a, const DrawCall& b) {
        return makeSortKey(a) < makeSortKey(b);
    });
```

### 2. Frustum Culling (Essential)

Don't draw what the camera can't see:

```cpp
struct Frustum {
    glm::vec4 planes[6];  // left, right, bottom, top, near, far

    bool isVisible(const BoundingSphere& sphere) const {
        for (int i = 0; i < 6; i++) {
            float dist = glm::dot(glm::vec3(planes[i]), sphere.center) + planes[i].w;
            if (dist < -sphere.radius) return false;  // Completely outside
        }
        return true;
    }
};

void cullAndRender(const std::vector<Object>& objects, const Frustum& frustum) {
    for (const auto& obj : objects) {
        if (frustum.isVisible(obj.bounds)) {
            submitDrawCall(obj);
        }
    }
}
```

### 3. Occlusion Culling

Don't draw what's hidden behind other objects:

```cpp
// Hardware occlusion queries
class OcclusionQuery {
    GLuint query;
    bool resultReady = false;
    int visibleSamples = 0;

public:
    void begin() {
        glGenQueries(1, &query);
        glBeginQuery(GL_SAMPLES_PASSED, query);
    }

    void end() {
        glEndQuery(GL_SAMPLES_PASSED);
    }

    bool isVisible() {
        GLint available;
        glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &available);
        if (available) {
            GLuint samples;
            glGetQueryObjectuiv(query, GL_QUERY_RESULT, &samples);
            return samples > 0;
        }
        return true;  // Assume visible if result not ready
    }
};
```

### 4. Distance Culling

Simple but effective for large worlds:

```cpp
void distanceCull(std::vector<Object>& objects, const glm::vec3& cameraPos) {
    const float MAX_RENDER_DISTANCE = 500.0f;
    const float MAX_RENDER_DISTANCE_SQ = MAX_RENDER_DISTANCE * MAX_RENDER_DISTANCE;

    objects.erase(std::remove_if(objects.begin(), objects.end(),
        [&](const Object& obj) {
            float distSq = glm::length2(obj.position - cameraPos);
            return distSq > MAX_RENDER_DISTANCE_SQ;
        }), objects.end());
}
```

---

## Indirect Drawing Guide

### What is Indirect Drawing?

Instead of CPU issuing individual draw commands, parameters are stored in a GPU buffer:

```
Traditional:                    Indirect:
CPU → glDrawElements(...)       CPU → Write params to buffer
CPU → glDrawElements(...)       GPU → Read params from buffer
CPU → glDrawElements(...)       GPU → Execute draws
...1000 calls...               1 call → 1000 draws
```

### Draw Indirect Command Structure

```cpp
// For glDrawElementsIndirect
struct DrawElementsIndirectCommand {
    GLuint count;         // Number of indices
    GLuint instanceCount; // Number of instances
    GLuint firstIndex;    // Starting index in index buffer
    GLint  baseVertex;    // Offset added to indices
    GLuint baseInstance;  // First instance ID (for gl_BaseInstance)
};

// For glDrawArraysIndirect
struct DrawArraysIndirectCommand {
    GLuint count;         // Number of vertices
    GLuint instanceCount; // Number of instances
    GLuint first;         // Starting vertex
    GLuint baseInstance;  // First instance ID
};
```

### Single Indirect Draw

```cpp
// Create and fill command buffer
GLuint indirectBuffer;
glGenBuffers(1, &indirectBuffer);
glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);

DrawElementsIndirectCommand cmd = {
    .count = mesh.indexCount,
    .instanceCount = instanceCount,
    .firstIndex = 0,
    .baseVertex = 0,
    .baseInstance = 0
};
glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(cmd), &cmd, GL_DYNAMIC_DRAW);

// Draw
glBindVertexArray(vao);
glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);
glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr);
```

### Multi-Draw Indirect (MDI)

Draw multiple different meshes in a single call:

```cpp
// Combine all meshes into mega-buffer
struct MegaBuffer {
    GLuint VAO, VBO, EBO;
    std::vector<MeshRange> meshRanges;  // firstIndex, count per mesh
};

void setupMegaBuffer(MegaBuffer& mb, const std::vector<MeshData>& meshes) {
    std::vector<Vertex> allVertices;
    std::vector<uint32_t> allIndices;

    for (const auto& mesh : meshes) {
        MeshRange range;
        range.firstIndex = allIndices.size();
        range.baseVertex = allVertices.size();
        range.indexCount = mesh.indices.size();

        allVertices.insert(allVertices.end(),
                          mesh.vertices.begin(), mesh.vertices.end());

        for (uint32_t idx : mesh.indices) {
            allIndices.push_back(idx);  // Indices relative to mesh base
        }

        mb.meshRanges.push_back(range);
    }

    // Upload combined buffers
    glGenVertexArrays(1, &mb.VAO);
    glGenBuffers(1, &mb.VBO);
    glGenBuffers(1, &mb.EBO);
    // ... upload data ...
}

// Build indirect commands
std::vector<DrawElementsIndirectCommand> commands;
uint32_t baseInstance = 0;

for (int i = 0; i < meshRanges.size(); i++) {
    int instancesForMesh = getInstanceCount(i);

    commands.push_back({
        .count = meshRanges[i].indexCount,
        .instanceCount = instancesForMesh,
        .firstIndex = meshRanges[i].firstIndex,
        .baseVertex = meshRanges[i].baseVertex,
        .baseInstance = baseInstance
    });

    baseInstance += instancesForMesh;
}

// Upload commands to GPU
glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandBuffer);
glBufferData(GL_DRAW_INDIRECT_BUFFER,
             commands.size() * sizeof(DrawElementsIndirectCommand),
             commands.data(), GL_DYNAMIC_DRAW);

// SINGLE CALL renders all meshes with all instances!
glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT,
                           nullptr, commands.size(), 0);
```

### Accessing Draw ID in Shader

With `ARB_shader_draw_parameters` (OpenGL 4.6 or extension):

```glsl
#version 460 core

// Built-in variables
// gl_DrawID     - Index of current draw command (0 to drawCount-1)
// gl_BaseInstance - baseInstance from command struct

// Per-draw data in SSBO
layout(std430, binding = 0) buffer DrawData {
    mat4 modelMatrices[];
};

void main() {
    mat4 model = modelMatrices[gl_DrawID];
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
```

---

## GPU-Driven Rendering Overview

### The Philosophy

Traditional: **CPU tells GPU what to draw**
GPU-Driven: **GPU decides what to draw**

```
Traditional Pipeline:
CPU: Cull → Sort → Build commands → Submit
GPU: Execute draw calls

GPU-Driven Pipeline:
CPU: Upload scene data → Submit compute + indirect draw
GPU: Cull → Build commands → Execute draws
```

### Key Techniques

#### 1. GPU Frustum Culling

```glsl
// Compute shader for frustum culling
layout(local_size_x = 64) in;

layout(std430, binding = 0) buffer InputInstances {
    InstanceData instances[];
};

layout(std430, binding = 1) buffer VisibleInstances {
    InstanceData visibleInstances[];
};

layout(std430, binding = 2) buffer Counter {
    uint visibleCount;
};

uniform vec4 frustumPlanes[6];

bool isInFrustum(vec3 center, float radius) {
    for (int i = 0; i < 6; i++) {
        if (dot(vec4(center, 1.0), frustumPlanes[i]) < -radius)
            return false;
    }
    return true;
}

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= instanceCount) return;

    InstanceData inst = instances[idx];
    vec3 center = inst.boundingSphere.xyz;
    float radius = inst.boundingSphere.w;

    if (isInFrustum(center, radius)) {
        uint outIdx = atomicAdd(visibleCount, 1);
        visibleInstances[outIdx] = inst;
    }
}
```

#### 2. GPU Command Generation

```glsl
// Generate draw commands on GPU
layout(local_size_x = 1) in;

layout(std430, binding = 0) buffer DrawCommands {
    DrawElementsIndirectCommand commands[];
};

layout(std430, binding = 1) buffer InstanceCounts {
    uint counts[];  // Per-mesh instance counts
};

void main() {
    uint meshIdx = gl_GlobalInvocationID.x;

    commands[meshIdx].count = meshIndexCounts[meshIdx];
    commands[meshIdx].instanceCount = counts[meshIdx];
    commands[meshIdx].firstIndex = meshFirstIndices[meshIdx];
    commands[meshIdx].baseVertex = meshBaseVertices[meshIdx];
    commands[meshIdx].baseInstance = calculateBaseInstance(meshIdx);
}
```

### Two-Phase Occlusion Culling

Advanced technique used in AAA games:

```
Phase 1: Render depth with last frame's visible set
Phase 2: Test all objects against depth buffer (HiZ)
Phase 3: Render newly visible objects
Phase 4: Store visible set for next frame
```

### Performance Gains

From Assassin's Creed Unity (SIGGRAPH 2015):
- **10x more objects** than previous games
- **1-2 orders of magnitude fewer draw calls**
- **20-40% triangles culled** via GPU culling
- **30-80% shadow triangles culled**

---

## Level of Detail (LOD) Systems

### LOD Types

| Type | Description | Best For |
|------|-------------|----------|
| **Discrete LOD** | Pre-made mesh variants | Most objects |
| **Continuous LOD** | Runtime mesh simplification | Terrain |
| **Impostors** | Billboard/sprite replacement | Distant trees |
| **Hierarchical LOD (HLOD)** | Merge distant objects | Cities, forests |

### Distance-Based LOD

```cpp
struct LODMesh {
    MeshData* meshes[4];  // LOD0 (highest) to LOD3 (lowest)
    float distances[3];   // Transition distances

    MeshData* selectLOD(float distance) const {
        if (distance < distances[0]) return meshes[0];
        if (distance < distances[1]) return meshes[1];
        if (distance < distances[2]) return meshes[2];
        return meshes[3];
    }
};

// Typical distance thresholds
// LOD0: 0-20m (full detail)
// LOD1: 20-50m (50% triangles)
// LOD2: 50-100m (25% triangles)
// LOD3: 100m+ (10% triangles or impostor)
```

### Screen-Size LOD

More accurate than distance - accounts for FOV and object size:

```cpp
float calculateScreenSize(const BoundingSphere& bounds,
                         const glm::mat4& viewProj,
                         float screenHeight) {
    // Project bounding sphere to screen space
    glm::vec4 center = viewProj * glm::vec4(bounds.center, 1.0);
    glm::vec4 edge = viewProj * glm::vec4(bounds.center + glm::vec3(bounds.radius, 0, 0), 1.0);

    float centerW = center.w;
    float edgeW = edge.w;

    if (centerW <= 0 || edgeW <= 0) return 0;  // Behind camera

    float screenRadius = abs((edge.x / edgeW) - (center.x / centerW)) * 0.5f * screenHeight;
    return screenRadius * 2;  // Diameter in pixels
}

int selectLODByScreenSize(float screenSize) {
    if (screenSize > 200) return 0;  // LOD0: > 200 pixels
    if (screenSize > 100) return 1;  // LOD1: 100-200 pixels
    if (screenSize > 50) return 2;   // LOD2: 50-100 pixels
    if (screenSize > 10) return 3;   // LOD3: 10-50 pixels
    return -1;                        // Don't render: < 10 pixels
}
```

### LOD Transitions (Avoiding Pop)

#### Dithered Fade

```glsl
// Dither pattern for smooth LOD transitions
uniform float lodBlend;  // 0.0 to 1.0 during transition

void main() {
    // ... calculate fragment color ...

    // Dither-based LOD transition
    vec2 screenPos = gl_FragCoord.xy;
    float dither = fract(dot(screenPos, vec2(0.5, 0.25)));

    if (dither > lodBlend) discard;

    FragColor = color;
}
```

#### Alpha Fade

```glsl
uniform float lodAlpha;  // 1.0 (fully visible) to 0.0 (fading out)

void main() {
    // ... calculate fragment color ...
    FragColor = vec4(color.rgb, color.a * lodAlpha);
}
```

### Impostor System

Replace distant 3D meshes with textured billboards:

```cpp
struct Impostor {
    GLuint textureArray;  // Multiple angles
    int angleCount;       // e.g., 8 or 16 angles

    void render(const glm::vec3& position, const glm::vec3& cameraPos) {
        // Calculate view angle
        glm::vec3 toCamera = normalize(cameraPos - position);
        float angle = atan2(toCamera.x, toCamera.z);

        // Select appropriate impostor angle
        int angleIndex = int((angle / (2 * PI) + 0.5f) * angleCount) % angleCount;

        // Render billboard facing camera
        renderBillboard(position, textureArray, angleIndex);
    }
};
```

---

## Nanite: Next-Generation Geometry

### Overview

Epic's Nanite (Unreal Engine 5) represents the state-of-the-art in geometry rendering:

- **Virtualized geometry**: Stream only needed triangles
- **Cluster-based LOD**: 128-triangle clusters with hierarchical DAG
- **GPU-driven**: Culling and LOD selection on GPU
- **Software rasterization**: Custom rasterizer for small triangles

### Key Innovations

#### Cluster Hierarchy

```
Mesh divided into clusters:
         ┌───────────────────┐
         │   Root (LOD N)    │  Simplified mesh
         └────────┬──────────┘
              ┌───┴───┐
         ┌────┴───┐ ┌─┴────┐
         │ Child  │ │Child │  More detail
         └───┬────┘ └──┬───┘
           ┌─┴─┐     ┌─┴─┐
          │...│    │...│      Most detail (128 tri each)
          └───┘    └───┘
```

Each cluster:
- Contains ~128 triangles
- Has bounding box for culling
- Parent simplifies children
- GPU selects cut through hierarchy

#### Visibility Buffer

Instead of traditional deferred rendering:
1. Rasterize visibility IDs (cluster + triangle)
2. Deferred material evaluation
3. Single geometry pass

### Applicability

| Scenario | Nanite-like Approach? |
|----------|----------------------|
| Film-quality assets | Yes - handles millions of triangles |
| Procedural geometry | Challenging - requires preprocessing |
| Animated meshes | Limited support currently |
| Real-time generation | No - requires offline cluster building |

For our organism simulation with procedural creatures, traditional instancing is more appropriate than Nanite-style virtualization.

---

## Practical Implementation

### Our Optimization Checklist

Based on OrganismEvolution codebase:

- [x] **Frustum culling** - Don't render off-screen creatures
- [x] **Mesh caching** - Reuse meshes via `CreatureMeshCache`
- [x] **GPU instancing** - `CreatureRenderer` batches by mesh type
- [x] **State sorting** - Batches grouped by mesh key
- [ ] **Multi-draw indirect** - Could reduce 48 calls to 1
- [ ] **GPU culling** - Move frustum cull to compute shader
- [ ] **LOD system** - Lower detail for distant creatures
- [ ] **Impostors** - Billboards for very distant creatures

### Recommended Next Steps

1. **Add distance-based LOD** for creatures
2. **Implement frustum culling** (CPU is fine for <10K objects)
3. **Consider MDI** if draw calls become bottleneck

### Performance Targets

| Creature Count | Target Draw Calls | Target Frame Time |
|----------------|-------------------|-------------------|
| 1,000 | <50 | <2ms GPU |
| 10,000 | <50 | <8ms GPU |
| 100,000 | <100 (with MDI) | <16ms GPU |

---

## References

### Indirect Drawing
- [J Stephano - Multi-Draw Indirect](https://ktstephano.github.io/rendering/opengl/mdi) - Excellent MDI tutorial
- [CPP Rendering - Indirect Rendering](https://cpp-rendering.io/indirect-rendering/) - "A way to a million draw calls"
- [NVIDIA MDI Sample](https://docs.nvidia.com/gameworks/content/gameworkslibrary/graphicssamples/opengl_samples/multidrawindirectsample.htm)
- [Khronos glMultiDrawElementsIndirect](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glMultiDrawElementsIndirect.xhtml)

### GPU-Driven Rendering
- [SIGGRAPH 2015 - GPU-Driven Rendering Pipelines](https://advances.realtimerendering.com/s2015/aaltonenhaar_siggraph2015_combined_final_footer_220dpi.pdf) - Assassin's Creed Unity
- [Vulkan Guide - GPU Driven Engines](https://vkguide.dev/docs/gpudriven/gpu_driven_engines/)
- [GitHub - Unity GPU-Driven Pipeline](https://github.com/miccall/Unity-GPU-Driven-Pipeline)

### LOD Systems
- [Wikipedia - Level of Detail](https://en.wikipedia.org/wiki/Level_of_detail_(computer_graphics))
- [Sloyd - Mastering LOD](https://www.sloyd.ai/blog/mastering-level-of-detail-lod-balancing-graphics-and-performance-in-game-development)
- [IndieGamesDevel - How LOD Systems Work](https://indiegamesdevel.com/how-lod-systems-work-in-video-games-and-why-they-are-used/)

### Nanite
- [Epic - Nanite Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-virtualized-geometry-in-unreal-engine)
- [Unreal Engine Blog - Understanding Nanite](https://www.unrealengine.com/en-US/blog/understanding-nanite---unreal-engine-5-s-new-virtualized-geometry-system)
- [80.lv - Nanite Analysis](https://80.lv/articles/discussing-the-possibilities-and-drawbacks-of-unreal-engine-5-s-nanite)

### Profiling Tools
- [RenderDoc](https://renderdoc.org/) - Free, cross-platform GPU debugger
- [NVIDIA NSight Graphics](https://developer.nvidia.com/nsight-graphics)
- [AMD Radeon GPU Profiler](https://gpuopen.com/rgp/)

---

*Document version: 1.0 | Last updated: January 2026*
