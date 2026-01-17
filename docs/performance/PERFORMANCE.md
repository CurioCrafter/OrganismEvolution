# Performance Analysis: OrganismEvolution Simulator

## Executive Summary

**Current State:** ~75-200 creatures at 40-60 FPS
**Target:** 1000+ creatures at 60 FPS stable
**Primary Bottlenecks:** Vegetation draw calls (3000+), nametag rendering (200+), uniform lookups

The creature rendering system is **well-optimized** with proper instancing (4-8 draw calls for all creatures). However, vegetation and UI elements use naive per-object rendering, creating massive draw call overhead.

### Quick Stats
- **Terrain:** 300x300 grid = 90,000 vertices, 1 draw call (optimal)
- **Creatures:** Instanced batching by mesh key, ~4-8 draw calls (optimal)
- **Vegetation:** ~400-800 trees + 400-800 bushes + 2000+ grass blades = **3000+ draw calls** (critical)
- **Nametags:** One draw call per creature = **~200 draw calls** (needs work)

---

## Current Performance Characteristics

### Draw Call Analysis (per frame)

| Component | Current Implementation | Draw Calls | Notes |
|-----------|----------------------|------------|-------|
| **Terrain** | Single indexed draw | 1 | Optimal |
| **Creatures** | Instanced batching | 4-8 | Well-optimized |
| **Trees** | Per-tree draw | N (trees) | NOT instanced |
| **Bushes** | Per-bush draw | N (bushes) | NOT instanced |
| **Grass** | Per-blade draw | N × blades | CRITICAL bottleneck |
| **Nametags** | Per-creature draw | N (creatures) | NOT instanced |

**Estimated Draw Calls (200 creatures, 100 trees, 50 bushes, 1000 grass):**
```
1 + 6 + 100 + 50 + 1000 + 200 = ~1,357 draw calls/frame
```

### Buffer Operations

| Operation | Frequency | Location | Impact |
|-----------|-----------|----------|--------|
| Instance buffer upload | Per batch/frame | `CreatureRenderer.cpp:123` | Medium |
| Mesh VBO bind | Per batch | `CreatureRenderer.cpp:110` | Low |
| Uniform string lookup | Per uniform set | `Shader.cpp:71-87` | Medium |

### Shader Switches

| Switch | Count/Frame | Cost |
|--------|-------------|------|
| Main → Creature | 1 | Low |
| Creature → Vegetation | 1 | Low |
| Vegetation → Nametag | 1 | Low |
| **Total** | 3-4 | Acceptable |

### Memory Bandwidth

| Data | Size/Frame | Pattern |
|------|------------|---------|
| Instance data (creatures) | ~80 bytes × N creatures | Dynamic upload |
| Uniform updates | ~200 bytes/batch | Per-uniform glUniform* |
| Vertex data | Static | No per-frame cost |

---

## Bottleneck Analysis

### 1. CRITICAL: Grass Rendering
**Location:** `src/core/Simulation.cpp:787-819`

```cpp
for (const auto& cluster : grassClusters) {
    int bladesPerCluster = 5 + (int)(cluster.density * 10);
    for (int i = 0; i < bladesPerCluster; i++) {
        vegetationShader->setMat4("model", model);  // Uniform per blade!
        glDrawArrays(GL_TRIANGLES, 0, 3);           // Draw call per blade!
    }
}
```

**Impact:** With 100 clusters × 10 blades = 1,000+ draw calls
**Solution:** Instanced rendering with grass blade instances

### 2. HIGH: Tree/Bush Rendering
**Location:** `src/core/Simulation.cpp:748-784`

```cpp
for (const auto& tree : trees) {
    vegetationShader->setMat4("model", model);
    glDrawElements(GL_TRIANGLES, ...);  // One draw call per tree
}
```

**Impact:** O(tree_count) draw calls
**Solution:** Group by tree type, use instanced rendering

### 3. HIGH: Nametag Rendering
**Location:** `src/core/Simulation.cpp:665-713`

```cpp
for (const auto& creature : creatures) {
    nametagShader->setMat4("model", model);
    nametagShader->setVec3("textColor", color);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);  // One draw call per nametag
}
```

**Impact:** O(creature_count) draw calls
**Solution:** Instanced rendering or texture atlas batching

### 4. MEDIUM: Uniform Location Lookups
**Location:** `src/graphics/Shader.cpp:70-88`

```cpp
void setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    // String lookup every single call!
}
```

**Impact:** ~15-20 string lookups per frame per shader
**Solution:** Cache uniform locations on shader load

### 5. MEDIUM: No Frustum Culling
**Location:** N/A (not implemented)

**Impact:** Renders 100% of objects even when 70%+ may be off-screen
**Solution:** AABB frustum culling before batching

### 6. LOW: Instance Buffer Strategy
**Location:** `CreatureRenderer.cpp:122-126`

```cpp
glBufferData(GL_ARRAY_BUFFER,
             batch.instances.size() * sizeof(CreatureInstanceData),
             batch.instances.data(),
             GL_DYNAMIC_DRAW);
```

**Impact:** Full buffer re-allocation each frame
**Solution:** Use `glBufferSubData` with pre-allocated buffer or persistent mapping

---

## Optimization Recommendations

### Priority 1: Easy Wins (High Impact, Low Effort)

#### 1.1 Cache Uniform Locations
**Estimated Impact:** 5-10% frame time reduction
**Effort:** 1-2 hours

```cpp
// Before (current)
void setMat4(const std::string& name, const glm::mat4& mat) {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), ...);
}

// After (optimized)
class Shader {
    std::unordered_map<std::string, GLint> uniformCache;

    GLint getLocation(const std::string& name) {
        auto it = uniformCache.find(name);
        if (it != uniformCache.end()) return it->second;
        GLint loc = glGetUniformLocation(ID, name.c_str());
        uniformCache[name] = loc;
        return loc;
    }
};
```

#### 1.2 Disable Nametags by Default
**Estimated Impact:** Remove N draw calls (N = creature count)
**Effort:** 30 minutes

Add toggle: `bool renderNametags = false;` - only enable when debugging.

#### 1.3 Reduce Grass Density
**Estimated Impact:** Linear reduction in draw calls
**Effort:** 15 minutes

Temporary fix until instancing is implemented.

---

### Priority 2: Medium Effort, High Impact

#### 2.1 Instance Vegetation (Trees, Bushes)
**Estimated Impact:** 80%+ reduction in vegetation draw calls
**Effort:** 4-6 hours

```cpp
// Group trees by type
std::unordered_map<TreeType, std::vector<glm::mat4>> treeBatches;

for (const auto& tree : trees) {
    treeBatches[tree.type].push_back(tree.modelMatrix);
}

// Render each type with one instanced call
for (auto& [type, instances] : treeBatches) {
    uploadInstanceBuffer(instances);
    glDrawElementsInstanced(GL_TRIANGLES, mesh->indexCount,
                           GL_UNSIGNED_INT, 0, instances.size());
}
```

#### 2.2 Instance Grass Blades
**Estimated Impact:** 90%+ reduction in grass draw calls
**Effort:** 4-6 hours

Use same pattern as creatures - single draw call for all grass in a cluster type.

#### 2.3 Frustum Culling
**Estimated Impact:** 30-70% reduction depending on camera view
**Effort:** 3-4 hours

```cpp
bool isInFrustum(const glm::vec3& position, float radius, const Frustum& frustum) {
    for (int i = 0; i < 6; i++) {
        if (glm::dot(frustum.planes[i], glm::vec4(position, 1.0f)) < -radius) {
            return false;
        }
    }
    return true;
}

// In render loop
for (const auto& creature : creatures) {
    if (!isInFrustum(creature->getPosition(), creature->getBoundingRadius(), frustum)) {
        continue;  // Skip off-screen creatures
    }
    // Add to batch...
}
```

---

### Priority 3: Architecture Improvements

#### 3.1 Uniform Buffer Objects (UBOs)
**Estimated Impact:** Reduced CPU-GPU transfer overhead
**Effort:** 6-8 hours

```cpp
// Shared across all shaders
struct GlobalUniforms {
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec3 lightPos;
    float time;
    glm::vec3 viewPos;
    float padding;
};

GLuint globalUBO;
glGenBuffers(1, &globalUBO);
glBindBuffer(GL_UNIFORM_BUFFER, globalUBO);
glBufferData(GL_UNIFORM_BUFFER, sizeof(GlobalUniforms), nullptr, GL_DYNAMIC_DRAW);
glBindBufferBase(GL_UNIFORM_BUFFER, 0, globalUBO);

// Update once per frame
GlobalUniforms globals = {...};
glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GlobalUniforms), &globals);
```

#### 3.2 LOD System for Creatures
**Estimated Impact:** 30-50% reduction in vertex processing
**Effort:** 8-12 hours

```cpp
enum LODLevel { LOD_HIGH, LOD_MEDIUM, LOD_LOW, LOD_BILLBOARD };

LODLevel getLOD(float distance) {
    if (distance < 20.0f) return LOD_HIGH;      // ~500 triangles
    if (distance < 50.0f) return LOD_MEDIUM;    // ~200 triangles
    if (distance < 100.0f) return LOD_LOW;      // ~50 triangles
    return LOD_BILLBOARD;                        // 2 triangles
}
```

#### 3.3 Spatial Partitioning for Culling
**Estimated Impact:** O(log N) culling instead of O(N)
**Effort:** 6-8 hours

Use octree or BVH for spatial queries.

---

### Priority 4: Advanced Optimizations

#### 4.1 Persistent Mapped Buffers
**Estimated Impact:** Eliminate buffer stalls
**Effort:** 4-6 hours

```cpp
glBufferStorage(GL_ARRAY_BUFFER, maxInstances * sizeof(InstanceData),
                nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

instancePtr = glMapBufferRange(GL_ARRAY_BUFFER, 0, maxInstances * sizeof(InstanceData),
                               GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
```

#### 4.2 Indirect Drawing
**Estimated Impact:** GPU-driven rendering
**Effort:** 12-16 hours

Move culling to compute shader, use `glMultiDrawElementsIndirect`.

#### 4.3 Mesh Shaders (if supported)
**Estimated Impact:** Eliminate CPU bottleneck entirely
**Effort:** 20+ hours (requires modern GPU)

---

## Implementation Roadmap

### Phase 1: Quick Wins (Day 1)
- [ ] Cache uniform locations in Shader class
- [ ] Add nametag toggle (off by default)
- [ ] Reduce grass blade count temporarily

### Phase 2: Vegetation Instancing (Days 2-3)
- [ ] Create VegetationRenderer class with instancing
- [ ] Instance trees by type
- [ ] Instance bushes by type
- [ ] Instance grass clusters

### Phase 3: Culling (Day 4)
- [ ] Implement frustum extraction from view-projection matrix
- [ ] Add AABB frustum test
- [ ] Cull creatures before batching
- [ ] Cull vegetation before batching

### Phase 4: Polish (Days 5-6)
- [ ] Implement UBOs for global uniforms
- [ ] Add LOD levels to creature meshes
- [ ] Profile and tune batch sizes

---

## Expected Results

| Metric | Current | After Phase 1 | After Phase 2 | After Phase 4 |
|--------|---------|---------------|---------------|---------------|
| Draw calls | ~1,400 | ~1,200 | ~20-30 | ~15-25 |
| Creatures | 75 | 100 | 500 | 1,000+ |
| Target FPS | Variable | 45+ | 55+ | 60 stable |

---

## Profiling Checklist

Before and after each optimization:

1. **GPU profiling** (RenderDoc, NSight):
   - Draw call count
   - GPU time per pass
   - Buffer transfer time

2. **CPU profiling**:
   - Time in render loop
   - Time in batch preparation
   - Time in uniform updates

3. **Memory**:
   - VRAM usage
   - Buffer allocation patterns

---

## Key Files Reference

| File | Component | Lines |
|------|-----------|-------|
| `src/main.cpp` | Main render loop | 467-541 |
| `src/core/Simulation.cpp` | Simulation render | 130-180, 638-820 |
| `src/graphics/rendering/CreatureRenderer.cpp` | Instanced creatures | 43-168 |
| `src/graphics/Shader.cpp` | Uniform handling | 70-88 |
| `shaders/creature_vertex.glsl` | Creature animation | 25-41 |

---

## Conclusion

The creature rendering is already well-optimized with instancing. The primary bottleneck is **vegetation and UI rendering**, which uses naive per-object draw calls. Implementing instancing for vegetation (Priority 2) will provide the largest performance improvement, potentially reducing draw calls by 95%+ and enabling the target of 1000+ creatures at 60 FPS.

The recommended approach is to follow the phased roadmap, starting with quick wins for immediate improvement, then systematically adding instancing and culling.

---

## Appendix A: Detailed Code Analysis

### Creature Instancing Implementation (Reference)

The existing creature renderer at [CreatureRenderer.cpp](src/graphics/rendering/CreatureRenderer.cpp) demonstrates proper instancing:

```cpp
// Per-instance data structure (CreatureRenderer.h:12-17)
struct CreatureInstanceData {
    glm::mat4 modelMatrix;    // Transform
    glm::vec3 color;          // Creature color
    float animationPhase;     // Walk cycle
    glm::vec3 padding;        // GPU alignment
};

// Batching by mesh archetype (CreatureRenderer.cpp:61-104)
std::unordered_map<MeshKey, RenderBatch> batches;
for (const auto& creature : creatures) {
    MeshKey key = CreatureMeshCache::getMeshKey(genome, type);
    batches[key].instances.push_back(instanceData);
}

// Single instanced draw per batch (CreatureRenderer.cpp:182-188)
glDrawElementsInstanced(
    GL_TRIANGLES,
    batch.mesh->indices.size(),
    GL_UNSIGNED_INT,
    0,
    batch.instances.size()
);
```

### Vegetation Render Loop Analysis

Current implementation at [Simulation.cpp:749-820](src/core/Simulation.cpp#L749-L820):

```
renderVegetation(camera)
├── for each tree in trees (400-800 iterations)
│   ├── glm::translate/rotate/scale
│   ├── setMat4("model", model)
│   ├── glBindVertexArray(mesh->getVAO())
│   └── glDrawElements(...)           <- Individual draw call
├── for each bush in bushes (400-800 iterations)
│   └── Same pattern as trees         <- Individual draw call
└── for each grass cluster (300-500 clusters)
    └── for each blade (5-15 per cluster)
        ├── setMat4("model", model)
        └── glDrawArrays(...)         <- Individual draw call per blade!
```

### Memory Layout Analysis

| Component | Size per Unit | Count | Total Memory |
|-----------|--------------|-------|--------------|
| Creature Instance | 80 bytes | 200 | 16 KB/frame |
| Tree Instance (proposed) | 64 bytes | 800 | 50 KB (static) |
| Grass Instance (proposed) | 32 bytes | 5000 | 160 KB (static) |
| Terrain Vertex | 36 bytes | 90,000 | 3.2 MB (static) |

---

## Appendix B: Frustum Culling Implementation Guide

### Frustum Plane Extraction

```cpp
void extractFrustumPlanes(const glm::mat4& viewProj, glm::vec4 planes[6]) {
    // Left plane
    planes[0] = glm::vec4(
        viewProj[0][3] + viewProj[0][0],
        viewProj[1][3] + viewProj[1][0],
        viewProj[2][3] + viewProj[2][0],
        viewProj[3][3] + viewProj[3][0]
    );
    // Right plane
    planes[1] = glm::vec4(
        viewProj[0][3] - viewProj[0][0],
        viewProj[1][3] - viewProj[1][0],
        viewProj[2][3] - viewProj[2][0],
        viewProj[3][3] - viewProj[3][0]
    );
    // Bottom plane
    planes[2] = glm::vec4(
        viewProj[0][3] + viewProj[0][1],
        viewProj[1][3] + viewProj[1][1],
        viewProj[2][3] + viewProj[2][1],
        viewProj[3][3] + viewProj[3][1]
    );
    // Top plane
    planes[3] = glm::vec4(
        viewProj[0][3] - viewProj[0][1],
        viewProj[1][3] - viewProj[1][1],
        viewProj[2][3] - viewProj[2][1],
        viewProj[3][3] - viewProj[3][1]
    );
    // Near plane
    planes[4] = glm::vec4(
        viewProj[0][3] + viewProj[0][2],
        viewProj[1][3] + viewProj[1][2],
        viewProj[2][3] + viewProj[2][2],
        viewProj[3][3] + viewProj[3][2]
    );
    // Far plane
    planes[5] = glm::vec4(
        viewProj[0][3] - viewProj[0][2],
        viewProj[1][3] - viewProj[1][2],
        viewProj[2][3] - viewProj[2][2],
        viewProj[3][3] - viewProj[3][2]
    );

    // Normalize planes
    for (int i = 0; i < 6; i++) {
        float len = glm::length(glm::vec3(planes[i]));
        planes[i] /= len;
    }
}
```

### Sphere-Frustum Test

```cpp
bool isSphereVisible(const glm::vec3& center, float radius, const glm::vec4 planes[6]) {
    for (int i = 0; i < 6; i++) {
        float distance = glm::dot(glm::vec3(planes[i]), center) + planes[i].w;
        if (distance < -radius) {
            return false;  // Completely outside this plane
        }
    }
    return true;  // Inside or intersecting frustum
}
```

---

## Appendix C: LOD System Design

### Distance Thresholds

| LOD Level | Distance Range | Triangle Target | Use Case |
|-----------|---------------|-----------------|----------|
| LOD0 (Full) | 0-50 units | 200-500 | Close-up detail |
| LOD1 (Medium) | 50-100 units | 50-150 | Mid-range |
| LOD2 (Low) | 100-200 units | 20-50 | Background |
| LOD3 (Billboard) | 200+ units | 2 | Distant impostor |

### Billboard Implementation

For LOD3, replace 3D mesh with camera-facing quad:

```cpp
glm::mat4 createBillboardMatrix(const glm::vec3& position, const Camera& camera) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);

    // Extract camera rotation and apply inverse
    glm::mat4 view = camera.getViewMatrix();
    model[0] = glm::vec4(view[0][0], view[1][0], view[2][0], 0.0f);
    model[1] = glm::vec4(view[0][1], view[1][1], view[2][1], 0.0f);
    model[2] = glm::vec4(view[0][2], view[1][2], view[2][2], 0.0f);

    return model;
}
```

---

*Last updated: Performance analysis by Agent 9*
