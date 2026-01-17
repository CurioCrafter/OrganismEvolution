# Batching Strategies for Game Rendering

A comprehensive guide to draw call batching techniques for optimizing rendering performance in game engines.

## Table of Contents

1. [Understanding Draw Call Overhead](#understanding-draw-call-overhead)
2. [Types of Batching](#types-of-batching)
3. [Material Sorting Strategies](#material-sorting-strategies)
4. [State Grouping Optimization](#state-grouping-optimization)
5. [Texture Management](#texture-management)
6. [Implementation Patterns](#implementation-patterns)
7. [Our Current Batching System](#our-current-batching-system)
8. [When Batching Helps vs Hurts](#when-batching-helps-vs-hurts)
9. [References](#references)

---

## Understanding Draw Call Overhead

### What Happens in a Draw Call?

Each draw call triggers a sequence of operations:

```
CPU Side:
├── Validate current OpenGL state
├── Check buffer bindings
├── Verify shader program state
├── Pack command into command buffer
└── Signal driver for submission

Driver Side:
├── Translate to GPU commands
├── Handle memory management
├── Queue for GPU execution
└── Manage synchronization

GPU Side:
├── Fetch vertex data
├── Execute vertex shader
├── Rasterization
├── Execute fragment shader
└── Write to framebuffer
```

### The CPU Bottleneck

| Draw Calls/Frame | CPU Time @ 30μs each | FPS Impact |
|------------------|---------------------|------------|
| 500 | 15ms | ~66 FPS max |
| 1,000 | 30ms | ~33 FPS max |
| 2,000 | 60ms | ~16 FPS max |
| 5,000 | 150ms | ~6 FPS max |

**Key insight**: GPU can process millions of triangles, but CPU can only submit ~2,000-5,000 draw calls per frame efficiently.

---

## Types of Batching

### 1. Static Batching

Combine static (non-moving) objects at build time into single vertex buffers.

```
Before:
├── Rock A (100 triangles) → 1 draw call
├── Rock B (100 triangles) → 1 draw call
├── Rock C (100 triangles) → 1 draw call
└── Total: 3 draw calls, 300 triangles

After Static Batching:
└── Combined Rocks (300 triangles) → 1 draw call
```

**Implementation**:
```cpp
struct StaticBatch {
    GLuint VAO;
    GLuint VBO;
    std::vector<Vertex> combinedVertices;
    glm::mat4 worldTransform;  // Identity for world-space batches

    void addMesh(const MeshData& mesh, const glm::mat4& transform) {
        for (const auto& v : mesh.vertices) {
            Vertex worldV = v;
            worldV.position = glm::vec3(transform * glm::vec4(v.position, 1.0f));
            worldV.normal = glm::normalize(glm::mat3(transform) * v.normal);
            combinedVertices.push_back(worldV);
        }
    }

    void upload() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER,
                     combinedVertices.size() * sizeof(Vertex),
                     combinedVertices.data(),
                     GL_STATIC_DRAW);
        // Setup vertex attributes...
    }
};
```

**Pros**:
- Zero per-frame CPU cost
- Optimal GPU efficiency
- Single draw call for many objects

**Cons**:
- Increased memory (duplicated vertices in world space)
- Cannot move individual objects
- Must rebuild if objects change

### 2. Dynamic Batching

Transform and combine meshes every frame on CPU.

**When it works**:
- Small meshes (< 300 vertices recommended)
- Many objects moving together
- Objects share materials

**When to avoid**:
- Large meshes (CPU transform overhead too high)
- GPU is the bottleneck (batching won't help)
- Modern APIs (instancing is better)

**Modern recommendation**: Use GPU instancing instead of dynamic batching.

### 3. GPU Instancing (Recommended)

Render multiple instances of the same mesh with a single draw call.

See [INSTANCING_DEEP_DIVE.md](INSTANCING_DEEP_DIVE.md) for complete details.

---

## Material Sorting Strategies

### The State Change Problem

Different materials require different GPU states:
- Shader programs
- Textures
- Blend modes
- Depth settings

**State changes are expensive**! Minimize them by sorting draw calls.

### Sort Keys

Create a sort key that groups similar states together:

```cpp
struct SortKey {
    uint64_t key;

    // Bit layout (example):
    // [63-56] Render queue (opaque=0, transparent=128, overlay=255)
    // [55-48] Shader ID
    // [47-32] Material ID
    // [31-16] Texture ID
    // [15-0]  Depth (front-to-back for opaque, back-to-front for transparent)

    static SortKey create(const RenderItem& item) {
        SortKey k;
        k.key = 0;
        k.key |= (uint64_t)item.renderQueue << 56;
        k.key |= (uint64_t)item.shaderID << 48;
        k.key |= (uint64_t)item.materialID << 32;
        k.key |= (uint64_t)item.textureID << 16;

        if (item.isOpaque()) {
            k.key |= (uint16_t)(item.depth * 65535);  // Front-to-back
        } else {
            k.key |= (uint16_t)((1.0f - item.depth) * 65535);  // Back-to-front
        }
        return k;
    }
};

// Sort render items
std::sort(renderItems.begin(), renderItems.end(),
    [](const RenderItem& a, const RenderItem& b) {
        return a.sortKey.key < b.sortKey.key;
    });
```

### Optimal Sort Order

For **opaque objects** (depth test on, no blending):
1. Shader (most expensive state change)
2. Material/Texture
3. Front-to-back depth (early-Z optimization)

For **transparent objects** (blending enabled):
1. Back-to-front depth (required for correct blending)
2. Shader
3. Material/Texture

### State Caching

Track current state to avoid redundant changes:

```cpp
class RenderStateCache {
    GLuint currentShader = 0;
    GLuint currentVAO = 0;
    GLuint currentTextures[16] = {};

public:
    void useShader(GLuint shader) {
        if (shader != currentShader) {
            glUseProgram(shader);
            currentShader = shader;
        }
    }

    void bindVAO(GLuint vao) {
        if (vao != currentVAO) {
            glBindVertexArray(vao);
            currentVAO = vao;
        }
    }

    void bindTexture(int unit, GLuint texture) {
        if (texture != currentTextures[unit]) {
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(GL_TEXTURE_2D, texture);
            currentTextures[unit] = texture;
        }
    }
};
```

---

## State Grouping Optimization

### Material Variants

Instead of unique materials per object, use material **variants** with shared state:

```cpp
// BAD: Each object has unique material
struct UniqueMaterial {
    Shader* shader;
    Texture* diffuse;
    Texture* normal;
    glm::vec4 color;
    float roughness;
};

// GOOD: Shared base material with per-instance parameters
struct SharedMaterial {
    Shader* shader;          // Shared
    Texture* diffuseAtlas;   // Shared atlas
    Texture* normalAtlas;    // Shared atlas
};

struct MaterialInstance {
    int materialBaseID;      // Index into shared materials
    glm::vec4 color;         // Per-instance
    glm::vec4 uvOffsetScale; // Per-instance atlas coordinates
};
```

### Uber Shaders

Single shader handles multiple material types via uniforms/defines:

```glsl
// Single shader with material features toggled
#ifdef HAS_NORMAL_MAP
    vec3 normal = texture(normalMap, uv).rgb * 2.0 - 1.0;
#else
    vec3 normal = vertexNormal;
#endif

#ifdef HAS_SPECULAR_MAP
    float specular = texture(specularMap, uv).r;
#else
    float specular = defaultSpecular;
#endif
```

**Benefits**:
- Fewer shader switches
- Better batching opportunity
- Single shader maintains same GPU state

**Drawbacks**:
- Shader complexity increases
- May execute unused code paths

---

## Texture Management

### Texture Atlasing

Combine multiple textures into one large atlas:

```
Individual Textures:        Texture Atlas:
┌──────┐ ┌──────┐           ┌─────────────┐
│ Rock │ │Grass │           │ Rock │Grass │
└──────┘ └──────┘    →      ├──────┼──────┤
┌──────┐ ┌──────┐           │ Wood │Metal │
│ Wood │ │Metal │           └─────────────┘
└──────┘ └──────┘           Single 2048x2048
4 x 512x512 textures        texture
```

**Atlas Implementation**:
```cpp
struct TextureAtlas {
    GLuint textureID;
    int atlasSize;
    std::unordered_map<std::string, glm::vec4> uvRegions;  // x,y,w,h

    glm::vec4 getUVRegion(const std::string& name) const {
        return uvRegions.at(name);
    }
};

// In shader, transform UVs:
vec2 atlasUV = uvRegion.xy + fract(localUV) * uvRegion.zw;
```

### Texture Arrays

Use array textures for identical-size textures:

```cpp
// Create texture array
glGenTextures(1, &textureArray);
glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
             width, height, numLayers, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

// Upload each texture to a layer
for (int i = 0; i < numTextures; i++) {
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i,
                    width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, textureData[i]);
}
```

**Shader access**:
```glsl
uniform sampler2DArray textureArray;

// Sample with layer index
vec4 color = texture(textureArray, vec3(uv, layerIndex));
```

### Bindless Textures (Advanced)

OpenGL 4.4+ with ARB_bindless_texture:

```cpp
// Get GPU handle for texture
GLuint64 handle = glGetTextureHandleARB(textureID);
glMakeTextureHandleResidentARB(handle);

// Pass handle to shader via uniform or SSBO
glUniform1ui64ARB(handleLocation, handle);
```

**Shader**:
```glsl
// Access texture without binding
layout(bindless_sampler) uniform sampler2D dynamicTexture;
// Or via SSBO of handles for truly dynamic access
```

---

## Implementation Patterns

### Render Queue Pattern

```cpp
class RenderQueue {
    std::vector<RenderCommand> opaqueQueue;
    std::vector<RenderCommand> transparentQueue;
    std::vector<RenderCommand> overlayQueue;

public:
    void submit(const RenderCommand& cmd) {
        switch (cmd.queue) {
            case Queue::Opaque:
                opaqueQueue.push_back(cmd);
                break;
            case Queue::Transparent:
                transparentQueue.push_back(cmd);
                break;
            case Queue::Overlay:
                overlayQueue.push_back(cmd);
                break;
        }
    }

    void flush() {
        // Sort opaque front-to-back (for early-Z)
        sortByDepthFrontToBack(opaqueQueue);

        // Sort transparent back-to-front (for blending)
        sortByDepthBackToFront(transparentQueue);

        // Render in order
        renderQueue(opaqueQueue);
        renderQueue(transparentQueue);
        renderQueue(overlayQueue);

        // Clear for next frame
        opaqueQueue.clear();
        transparentQueue.clear();
        overlayQueue.clear();
    }
};
```

### Batch Builder Pattern

```cpp
class BatchBuilder {
    struct Batch {
        GLuint shader;
        GLuint material;
        GLuint mesh;
        std::vector<InstanceData> instances;
    };

    std::unordered_map<uint64_t, Batch> batches;

public:
    void addInstance(const RenderItem& item) {
        uint64_t key = makeBatchKey(item.shader, item.material, item.mesh);

        auto& batch = batches[key];
        if (batch.instances.empty()) {
            batch.shader = item.shader;
            batch.material = item.material;
            batch.mesh = item.mesh;
        }
        batch.instances.push_back(item.instanceData);
    }

    void render() {
        for (auto& [key, batch] : batches) {
            glUseProgram(batch.shader);
            bindMaterial(batch.material);
            bindMesh(batch.mesh);

            uploadInstanceData(batch.instances);
            glDrawElementsInstanced(GL_TRIANGLES, indexCount,
                                   GL_UNSIGNED_INT, 0, batch.instances.size());
        }
        batches.clear();
    }
};
```

---

## Our Current Batching System

### Architecture Overview

From [CreatureRenderer.h](../../src/graphics/rendering/CreatureRenderer.h):

```
Creature Rendering Pipeline:
    │
    ├─► Group by MeshKey (type, size, speed)
    │   ├─► Herbivore_Small_Fast → Batch 1
    │   ├─► Herbivore_Small_Slow → Batch 2
    │   ├─► Carnivore_Medium_Fast → Batch 3
    │   └─► ... (up to 48 combinations)
    │
    ├─► Collect instance data per batch
    │   └─► CreatureInstanceData { model, color, animPhase }
    │
    └─► Render each batch with instancing
        └─► glDrawElementsInstanced()
```

### Batch Grouping Logic

```cpp
struct RenderBatch {
    MeshKey key;
    MeshData* mesh;
    std::vector<CreatureInstanceData> instances;
};

void CreatureRenderer::render(...) {
    std::unordered_map<MeshKey, RenderBatch> batches;

    // Group creatures by mesh key
    for (const auto& creature : creatures) {
        MeshKey key = CreatureMeshCache::getMeshKey(creature->genome,
                                                    creature->type);
        auto& batch = batches[key];
        if (!batch.mesh) {
            batch.key = key;
            batch.mesh = meshCache->getMesh(creature->genome, creature->type);
        }
        batch.instances.push_back(makeInstanceData(creature));
    }

    // Render each batch
    for (auto& [key, batch] : batches) {
        renderBatch(batch, shader);
        lastDrawCalls++;
    }
}
```

### Statistics Tracking

The renderer tracks metrics for debugging:

```cpp
int getLastDrawCalls() const { return lastDrawCalls; }
int getLastInstancesRendered() const { return lastInstancesRendered; }
```

**Expected performance**:
- 1,000 creatures with 48 mesh types → max 48 draw calls
- 10,000 creatures → still max 48 draw calls!

---

## When Batching Helps vs Hurts

### Batching Helps When...

| Scenario | Benefit |
|----------|---------|
| Many similar objects | Major draw call reduction |
| CPU-bound rendering | Frees CPU for game logic |
| Simple materials | Easy to group by state |
| Static scenes | Static batching near-free |

### Batching Hurts When...

| Scenario | Problem |
|----------|---------|
| GPU-bound rendering | Batching overhead with no gain |
| Unique objects/materials | Nothing to batch |
| Excessive memory use | Static batches duplicate vertices |
| Complex sorting | CPU time for sorting > draw call savings |

### Decision Flowchart

```
START
  │
  ├─► Is rendering CPU-bound?
  │   ├─► YES: Batching will help
  │   └─► NO: Focus on GPU optimization instead
  │
  ├─► Do objects share materials/meshes?
  │   ├─► YES: Good batching candidate
  │   └─► NO: Limited batching potential
  │
  ├─► Are objects static?
  │   ├─► YES: Use static batching
  │   └─► NO: Use instancing or dynamic batching
  │
  └─► How many instances per batch?
      ├─► >100: Instancing is optimal
      ├─► 10-100: Instancing worth it
      └─► <10: Batching overhead may exceed benefit
```

### Profiling First!

Always measure before optimizing:

```cpp
// Simple draw call counter
class DrawCallProfiler {
    int drawCalls = 0;
    float cpuTime = 0;

public:
    void beginFrame() {
        drawCalls = 0;
        cpuTime = glfwGetTime();
    }

    void recordDrawCall() { drawCalls++; }

    void endFrame() {
        cpuTime = glfwGetTime() - cpuTime;
        printf("Draw calls: %d, CPU time: %.2fms\n",
               drawCalls, cpuTime * 1000);
    }
};
```

---

## References

### Unity Documentation
- [Introduction to Batching Draw Calls](https://docs.unity3d.com/Manual/DrawCallBatching.html)
- [Unity Draw Call Batching Guide](https://thegamedev.guru/unity-performance/draw-call-optimization/)

### Godot Documentation
- [Optimization using Batching](https://docs.godotengine.org/en/3.5/tutorials/performance/batching.html)

### Technical Articles
- [Comprehensive Batch Optimization Analysis](https://www.oreateai.com/blog/comprehensive-analysis-of-batch-optimization-techniques-in-unity-game-development/3d81605662833ce76783852b9ffe82a3)
- [Polycount - Draw Calls and Batching](https://polycount.com/discussion/116687/draw-calls-and-batching)

### GDC Talks
- [SIGGRAPH 2015 - GPU-Driven Rendering Pipelines](https://advances.realtimerendering.com/s2015/aaltonenhaar_siggraph2015_combined_final_footer_220dpi.pdf) (Assassin's Creed Unity)

### Books
- Real-Time Rendering, 4th Edition - Chapter 18: Pipeline Optimization
- GPU Pro 5 - Section on Batch Rendering

---

*Document version: 1.0 | Last updated: January 2026*
