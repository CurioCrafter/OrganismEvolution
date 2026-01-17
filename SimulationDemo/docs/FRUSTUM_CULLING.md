# Frustum Culling Implementation

## Overview

Frustum culling is a rendering optimization technique that skips drawing objects outside the camera's view. This implementation extracts the six frustum planes from the view-projection matrix and tests creature bounding volumes against them before submitting render batches to the GPU.

## Architecture

### Files Created

- `src/graphics/Frustum.h` - Frustum class declaration
- `src/graphics/Frustum.cpp` - Frustum plane extraction and intersection tests

### Files Modified

- `src/graphics/Camera.h` - Added frustum member and update methods
- `src/graphics/Camera.cpp` - Implemented frustum update and view-projection helpers
- `src/graphics/rendering/CreatureRenderer.h` - Added culled count statistic
- `src/graphics/rendering/CreatureRenderer.cpp` - Integrated frustum culling in render loop
- `src/core/Simulation.cpp` - Added frustum update in render method
- `CMakeLists.txt` - Added Frustum.cpp to build

## Technical Details

### Frustum Plane Extraction

The frustum planes are extracted from the combined view-projection matrix using the Gribb/Hartmann method:

```cpp
// For a view-projection matrix M:
// Left plane:   row3 + row0
// Right plane:  row3 - row0
// Bottom plane: row3 + row1
// Top plane:    row3 - row1
// Near plane:   row3 + row2
// Far plane:    row3 - row2
```

All planes are normalized so the normal vector has unit length, making distance calculations trivial.

### Intersection Tests

#### Sphere Test (Used for Creatures)
```cpp
bool isSphereVisible(const glm::vec3& center, float radius) const;
```

For each frustum plane, compute the signed distance from the sphere center. If the distance is less than `-radius` for any plane, the sphere is completely outside.

**Bounding radius calculation:**
- Creature bounding radius = `genome.size * 2.5`
- This accounts for legs, tail, and animation bounds

#### AABB Test (Available for Future Use)
```cpp
bool isBoxVisible(const glm::vec3& min, const glm::vec3& max) const;
```

Uses the optimized p-vertex method: for each plane, find the AABB corner most in the direction of the plane normal. If this corner is outside any plane, the box is completely outside.

### Integration Flow

1. **Simulation::render()** - Updates frustum at frame start
2. **Camera::updateFrustum()** - Extracts planes from current view-projection matrix
3. **CreatureRenderer::render()** - Tests each creature against frustum before batching

```
render(camera)
  |
  v
camera.updateFrustum(aspectRatio)
  |
  v
frustum.update(viewProjection)
  |
  v
for each creature:
    if frustum.isSphereVisible(position, boundingRadius):
        add to batch
    else:
        culledCount++
```

## API Reference

### Frustum Class

```cpp
class Frustum {
public:
    enum Plane { LEFT, RIGHT, BOTTOM, TOP, NEAR, FAR };

    // Update frustum from view-projection matrix
    void update(const glm::mat4& viewProjection);

    // Visibility tests
    bool isBoxVisible(const glm::vec3& min, const glm::vec3& max) const;
    bool isSphereVisible(const glm::vec3& center, float radius) const;
    bool isPointVisible(const glm::vec3& point) const;

    // Debug helpers
    float getDistanceToPlane(int planeIndex, const glm::vec3& point) const;
    glm::vec3 getPlaneNormal(int planeIndex) const;
};
```

### Camera Extensions

```cpp
class Camera {
public:
    // ... existing members ...

    // Update frustum from current view state
    void updateFrustum(float aspectRatio, float nearPlane = 0.1f, float farPlane = 1000.0f);

    // Get frustum for external culling
    const Frustum& getFrustum() const;

    // Matrix helpers
    glm::mat4 getProjectionMatrix(float aspectRatio, float nearPlane, float farPlane) const;
    glm::mat4 getViewProjectionMatrix(float aspectRatio, float nearPlane, float farPlane);
};
```

### CreatureRenderer Statistics

```cpp
class CreatureRenderer {
public:
    int getLastDrawCalls() const;         // Batches rendered
    int getLastInstancesRendered() const; // Creatures drawn
    int getLastCulledCount() const;       // Creatures skipped (NEW)
};
```

## Performance Considerations

### Expected Improvements

- **Best case**: Camera zoomed in or facing edge of world - ~80-90% creatures culled
- **Typical case**: Normal gameplay view - ~40-60% creatures culled
- **Worst case**: Top-down view of entire world - ~0-10% creatures culled

### Overhead

- Frustum update: 6 plane extractions + 6 normalizations per frame (~negligible)
- Per-creature test: 6 dot products + comparisons (~30 FLOPs per creature)
- Memory: 6 × vec4 = 96 bytes per frustum

### When Culling Helps Most

1. Large creature populations (100+)
2. Tight camera angles (first-person or chase views)
3. High-resolution rendering where draw calls are expensive

## Debug Visualization (Optional)

The frustum debug visualization can be enabled by uncommenting the debug code in Simulation.cpp. It draws:

- Frustum plane normals as colored lines
- Near/far plane outlines
- Culled creature count overlay

## Testing

### Verification Checklist

- [ ] No visual popping at frustum edges (bounding radius is conservative)
- [ ] Creatures smoothly appear/disappear when panning camera
- [ ] FPS improvement visible in debug overlay when zoomed in
- [ ] All creature types (herbivore, carnivore, aquatic, flying) culled correctly

### Performance Measurement

Compare frame times with many creatures visible vs. few visible:
- Zoom out (all visible): Note FPS
- Zoom in tight (few visible): Note FPS improvement
- Expected improvement: 20-50% depending on view angle

## Future Enhancements

1. **Hierarchical culling**: Spatial grid + frustum for O(1) average case
2. **Occlusion culling**: Skip creatures behind terrain/vegetation
3. **LOD integration**: Use frustum distance for mesh LOD selection
4. **Vegetation culling**: Apply same technique to trees/bushes
