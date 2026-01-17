# Grass Instanced Rendering System

This document describes the implementation of the grass rendering system in OrganismEvolution using GPU instancing for efficient rendering of thousands of grass blades.

## Overview

The grass rendering system uses GPU instancing to render 10,000+ grass blades efficiently with a single draw call. Key features include:

- **GPU Instancing**: All grass blades rendered in one `glDrawArraysInstanced` call
- **Wind Animation**: Multi-frequency wind simulation in vertex shader
- **Distance-Based LOD**: Grass fades out beyond 80 units
- **Alpha Cutoff**: Soft-edged grass blade shapes
- **Per-Blade Variation**: Unique rotation, scale, and position per instance

## Architecture

### Data Flow

```
VegetationManager::getGrassClusters()
        │
        ▼
┌─────────────────────┐
│ GrassCluster Data   │  position, density
└─────────────────────┘
        │
        ▼  initGrassInstancing()
┌─────────────────────┐
│ GrassInstanceData   │  position, scale, rotation per blade
└─────────────────────┘
        │
        ▼  Upload to GPU
┌─────────────────────┐
│ grassInstanceVBO    │  Instance buffer on GPU
└─────────────────────┘
        │
        ▼  renderGrassInstanced()
┌─────────────────────┐
│ glDrawArraysInstanced │  Single draw call for all grass
└─────────────────────┘
```

### Instance Data Structure

```cpp
struct GrassInstanceData {
    glm::vec3 position;   // World position of grass blade
    float scale;          // Scale factor (0.5 - 2.0 typical)
    float rotation;       // Y-axis rotation in radians
    float padding[3];     // Alignment padding (32 bytes total)
};
```

### Files

| File | Purpose |
|------|---------|
| `src/core/Simulation.cpp` | Grass instancing setup and render calls |
| `src/core/Simulation.h` | VAO/VBO handles, instance count |
| `src/environment/VegetationManager.cpp` | GrassCluster generation |
| `shaders/grass_vertex.glsl` | Instanced vertex shader with wind |
| `shaders/grass_fragment.glsl` | Grass coloring and alpha cutoff |

## Grass Blade Geometry

The grass blade is a simple quad (2 triangles, 6 vertices) oriented facing +Z:

```
     Top-left  ◄───► Top-right
        ▲ \         / ▲
        │   \     /   │
        │     \ /     │
        │      X      │  Height = 1.0 unit
        │     / \     │
        │   /     \   │
        ▼ /         \ ▼
  Bottom-left ◄───► Bottom-right

        Width = 0.12 units
```

Vertex layout:
```cpp
float grassVertices[] = {
    // Position              // Normal           // TexCoord (UV)
    -0.06f, 0.0f, 0.0f,     0.0f, 0.0f, 1.0f,   0.0f, 0.0f,  // Bottom-left
     0.06f, 0.0f, 0.0f,     0.0f, 0.0f, 1.0f,   1.0f, 0.0f,  // Bottom-right
     0.04f, 1.0f, 0.0f,     0.0f, 0.0f, 1.0f,   0.85f, 1.0f, // Top-right
    -0.06f, 0.0f, 0.0f,     0.0f, 0.0f, 1.0f,   0.0f, 0.0f,  // Bottom-left
     0.04f, 1.0f, 0.0f,     0.0f, 0.0f, 1.0f,   0.85f, 1.0f, // Top-right
    -0.04f, 1.0f, 0.0f,     0.0f, 0.0f, 1.0f,   0.15f, 1.0f, // Top-left
};
```

## Vertex Shader (grass_vertex.glsl)

### Inputs

| Location | Attribute | Type | Source |
|----------|-----------|------|--------|
| 0 | aPos | vec3 | Grass blade mesh |
| 1 | aNormal | vec3 | Grass blade mesh |
| 2 | aTexCoord | vec2 | Grass blade mesh |
| 3 | aInstancePos | vec3 | Instance buffer (per-instance) |
| 4 | aInstanceScale | float | Instance buffer (per-instance) |
| 5 | aInstanceRotation | float | Instance buffer (per-instance) |

### Wind Animation

The wind system uses multiple sine waves at different frequencies for natural-looking grass movement:

```glsl
// Multiple wind frequencies for more natural look
float windPhase1 = time * 2.0 + aInstancePos.x * 0.1 + aInstancePos.z * 0.15;
float windPhase2 = time * 3.5 + aInstancePos.x * 0.23 + aInstancePos.z * 0.17;
float windPhase3 = time * 1.2 + aInstancePos.x * 0.07 + aInstancePos.z * 0.11;

// Combine wind waves
float windOffset = sin(windPhase1) * 0.6 + sin(windPhase2) * 0.25 + sin(windPhase3) * 0.15;
windOffset *= heightFactor * windStrength * 0.4;
```

Key features:
- **Quadratic height factor**: Grass bends more at tips (`heightFactor = aPos.y * aPos.y`)
- **Position-based phase**: Each blade has unique wave phase based on world position
- **Vertical compression**: Blade compresses slightly when bent

### Distance-Based LOD

```glsl
const float GRASS_NEAR = 20.0;   // Full density within this distance
const float GRASS_FAR = 80.0;    // No grass beyond this distance

float distToCamera = length(aInstancePos - viewPos);
DistanceFade = 1.0 - smoothstep(GRASS_NEAR, GRASS_FAR, distToCamera);

// Early out for very distant grass
if (DistanceFade < 0.01) {
    gl_Position = vec4(0.0, 0.0, -1000.0, 1.0);
    return;
}
```

## Fragment Shader (grass_fragment.glsl)

### Alpha Cutoff for Blade Shape

Creates a pointed blade shape that narrows toward the tip:

```glsl
// Blade narrows toward top
float bladeWidth = 0.5 * (1.0 - Height * 0.7);

// Distance from blade center
float centerDist = abs(BladeUV.x - 0.5);

// Soft alpha edge
float alpha = 1.0 - smoothstep(bladeWidth * 0.8, bladeWidth, centerDist);
alpha *= DistanceFade;

if (alpha < 0.1) {
    discard;
}
```

### Color Gradient

Three-color gradient from base to tip:

```glsl
vec3 baseColor = vec3(0.12, 0.28, 0.06);   // Dark green at base
vec3 midColor = vec3(0.22, 0.48, 0.12);    // Medium green
vec3 tipColor = vec3(0.38, 0.62, 0.20);    // Lighter green at tips

// Smooth color transition
if (Height < 0.5) {
    grassColor = mix(baseColor, midColor, Height * 2.0);
} else {
    grassColor = mix(midColor, tipColor, (Height - 0.5) * 2.0);
}
```

### Lighting Effects

- **Half-Lambert diffuse**: `diff = diff * 0.5 + 0.5` for softer lighting
- **Subsurface scattering**: Backlit grass tips glow slightly
- **Ground shadow**: Grass is darker at the very base
- **Position variation**: Subtle color variation based on world position

## Rendering Pipeline

### Render Order

```cpp
void Simulation::render(Camera& camera) {
    // 1. Shadow pass
    renderShadowPass();

    // 2. Terrain
    terrain->render();

    // 3. Vegetation (trees, bushes)
    renderVegetation(camera);
        │
        └── renderGrassInstanced(camera);  // Called at end of vegetation

    // 4. Creatures
    creatureRenderer->render(...);

    // 5. UI
    renderNametags(camera);
}
```

### GL State for Grass

```cpp
void Simulation::renderGrassInstanced(const Camera& camera) {
    // Enable blending for alpha
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable culling (grass visible from both sides)
    glDisable(GL_CULL_FACE);

    // Draw all instances
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, grassInstanceCount);

    // Restore state
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}
```

## Performance Characteristics

### Benchmarks

| Metric | Before Instancing | After Instancing |
|--------|-------------------|------------------|
| Draw calls | ~10,000 (one per blade) | 1 |
| FPS (10K grass) | ~15-20 | 60+ |
| GPU memory | Dynamic | ~320KB (fixed) |

### Memory Usage

```
GrassInstanceData size: 32 bytes per instance
10,000 blades: 320,000 bytes (~312 KB)
20,000 blades: 640,000 bytes (~625 KB)
```

### Optimization Tips

1. **Reduce instance count**: Lower `bladesPerCluster` in `initGrassInstancing()`
2. **Reduce render distance**: Lower `GRASS_FAR` constant in vertex shader
3. **Simpler blade geometry**: Use single triangle instead of quad (3 vertices vs 6)
4. **Disable shadows**: Grass already skipped in shadow pass

## Uniform Variables

### Vertex Shader

| Uniform | Type | Description |
|---------|------|-------------|
| view | mat4 | Camera view matrix |
| projection | mat4 | Projection matrix |
| time | float | Animation time (seconds) |
| windDirection | vec3 | Normalized wind direction |
| windStrength | float | Wind intensity (0.0 - 1.0) |
| viewPos | vec3 | Camera position for LOD |

### Fragment Shader

| Uniform | Type | Description |
|---------|------|-------------|
| lightPos | vec3 | Main light position |
| viewPos | vec3 | Camera position |
| lightColor | vec3 | Light color (usually white) |

## Troubleshooting

### Grass Not Visible

1. Check shader compilation errors in console
2. Verify `grassInstanceCount > 0` after init
3. Check camera position is within 80 units of grass
4. Ensure `grassVAO` is bound correctly

### Grass Flickering

1. Check depth buffer precision
2. Increase polygon offset if z-fighting with terrain
3. Verify grass blade base is at ground level

### Poor Performance

1. Check `grassInstanceCount` isn't excessive
2. Verify early-out in vertex shader is working
3. Profile fragment shader discard efficiency

### Wind Animation Issues

1. Verify `time` uniform is being updated each frame
2. Check `windStrength` isn't zero
3. Verify `windDirection` is normalized

## Future Improvements

1. **Frustum culling**: Only upload visible grass instances
2. **GPU sorting**: Sort instances by distance for better alpha blending
3. **Texture-based grass**: Use grass texture atlas for variety
4. **Shadow receiving**: Add shadow map sampling to fragment shader
5. **Terrain-conforming**: Sample terrain height in vertex shader
