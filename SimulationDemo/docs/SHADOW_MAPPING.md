# Shadow Mapping Implementation

This document describes the shadow mapping system used in the OrganismEvolution simulation to render dynamic shadows from the sun/moon directional light.

## Overview

The shadow mapping system implements **directional light shadow mapping** with **PCF (Percentage Closer Filtering)** for soft shadow edges. Shadows are cast by creatures and vegetation onto the terrain, creating a more realistic and immersive environment.

### Key Features

- **2048x2048** shadow map resolution for high-quality shadows
- **Orthographic projection** for directional light (sun)
- **PCF 5x5 kernel** for smooth, soft shadow edges
- **Shadow bias** to prevent shadow acne artifacts
- **Front-face culling** during shadow pass to reduce peter-panning
- **Dynamic sun position** integrated with day/night cycle

## Architecture

### Core Components

```
src/graphics/ShadowMap.h      - Shadow map class declaration
src/graphics/ShadowMap.cpp    - Shadow map implementation
shaders/shadow_vertex.glsl    - Depth-only vertex shader (terrain/vegetation)
shaders/shadow_creature_vertex.glsl - Instanced creature shadow vertex shader
shaders/shadow_fragment.glsl  - Empty fragment shader (depth-only)
shaders/fragment.glsl         - Terrain fragment shader with shadow sampling
shaders/creature_fragment.glsl - Creature fragment shader with shadow sampling
```

### Data Flow

```
1. Update sun direction from day/night cycle
2. Calculate light space matrix (orthographic projection * view)
3. Shadow Pass:
   - Bind shadow FBO
   - Render terrain depth
   - Render vegetation depth
   - Render creatures depth
   - Unbind shadow FBO
4. Main Render Pass:
   - Bind shadow map as texture
   - Sample shadow map in fragment shaders
   - Apply PCF filtering
   - Combine with lighting
```

## Implementation Details

### ShadowMap Class

```cpp
class ShadowMap {
public:
    bool init(unsigned int width = 2048, unsigned int height = 2048);
    void bindForWriting();      // Start shadow pass
    void bindForReading(GLenum textureUnit);  // Sample in main pass
    void updateLightSpaceMatrix(const glm::vec3& lightDir,
                                 const glm::vec3& sceneCenter,
                                 float sceneRadius);
    const glm::mat4& getLightSpaceMatrix() const;

private:
    GLuint m_fbo;           // Framebuffer object
    GLuint m_depthTexture;  // Depth texture (GL_DEPTH_COMPONENT32F)
    glm::mat4 m_lightSpaceMatrix;
};
```

### Shadow Map Initialization

The shadow map uses a depth-only framebuffer with specific texture parameters:

```cpp
// Depth texture format: 32-bit float for precision
glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F,
             width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

// LINEAR filtering enables hardware PCF
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

// Clamp to border with white (1.0) for areas outside shadow map
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

// Enable comparison mode for sampler2DShadow
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
```

### Light Space Matrix Calculation

The light space matrix transforms world coordinates to the light's view space:

```cpp
void ShadowMap::updateLightSpaceMatrix(const glm::vec3& lightDir,
                                        const glm::vec3& sceneCenter,
                                        float sceneRadius) {
    // Normalize light direction
    glm::vec3 lightDirection = glm::normalize(lightDir);

    // Position light far from scene along light direction
    glm::vec3 lightPos = sceneCenter - lightDirection * sceneRadius * 2.0f;

    // Create view matrix (handle case when light is directly overhead)
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(lightDirection, up)) > 0.99f) {
        up = glm::vec3(0.0f, 0.0f, 1.0f);
    }
    m_lightView = glm::lookAt(lightPos, sceneCenter, up);

    // Orthographic projection covering scene with padding
    float orthoSize = sceneRadius * 1.5f;
    m_lightProjection = glm::ortho(-orthoSize, orthoSize,
                                    -orthoSize, orthoSize,
                                    0.1f, sceneRadius * 4.0f);

    m_lightSpaceMatrix = m_lightProjection * m_lightView;
}
```

### Sun Position Integration

The sun direction is updated based on the day/night cycle:

```cpp
// In Simulation::renderShadowPass()
float sunAngle = dayNightCycle * 3.14159f;  // 0 to PI over the day
sunDirection = glm::normalize(glm::vec3(
    std::cos(sunAngle) * 0.8f,              // X: east to west
    std::max(0.3f, std::sin(sunAngle)),     // Y: always some height
    0.3f                                     // Z: slight offset
));
```

## Shadow Pass Rendering

### Terrain and Vegetation

```glsl
// shadow_vertex.glsl
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;    // Unused
layout (location = 2) in vec3 aColor;     // Unused

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main() {
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}
```

### Instanced Creatures

```glsl
// shadow_creature_vertex.glsl
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;    // Unused
layout (location = 2) in vec2 aTexCoord;  // Unused

// Per-instance attributes
layout (location = 3) in mat4 aInstanceModel;   // locations 3,4,5,6
layout (location = 7) in vec3 aInstanceColor;   // Unused
layout (location = 8) in float aAnimationPhase; // Unused

uniform mat4 lightSpaceMatrix;

void main() {
    vec4 worldPos = aInstanceModel * vec4(aPos, 1.0);
    gl_Position = lightSpaceMatrix * worldPos;
}
```

### Shadow Pass State

```cpp
void Simulation::renderShadowPass() {
    shadowMap->bindForWriting();

    // Enable front-face culling to reduce shadow acne
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    // Polygon offset for additional acne prevention
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);

    // Render terrain
    shadowShader->use();
    shadowShader->setMat4("lightSpaceMatrix", shadowMap->getLightSpaceMatrix());
    shadowShader->setMat4("model", glm::mat4(1.0f));
    terrain->renderForShadow();

    // Render vegetation
    renderVegetationForShadow();

    // Render creatures
    shadowCreatureShader->use();
    shadowCreatureShader->setMat4("lightSpaceMatrix", shadowMap->getLightSpaceMatrix());
    creatureRenderer->renderForShadow(creatures, *shadowCreatureShader, shadowTime);

    // Reset state
    glDisable(GL_POLYGON_OFFSET_FILL);
    glCullFace(GL_BACK);
    shadowMap->unbind();
}
```

## Shadow Sampling (Main Pass)

### PCF Soft Shadows

Both terrain and creature fragment shaders use a 5x5 PCF kernel for soft shadows:

```glsl
float calculateShadow(vec4 fragPosLightSpace) {
    if (!shadowsEnabled) return 1.0;  // No shadows if disabled

    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // Outside shadow map = fully lit
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z > 1.0) {
        return 1.0;
    }

    // PCF 5x5 kernel for soft shadows
    float shadow = 0.0;
    float texelSize = 1.0 / 2048.0;  // Shadow map size

    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            // Hardware comparison via sampler2DShadow
            shadow += texture(shadowMap, vec3(projCoords.xy + offset,
                                               projCoords.z - 0.002));
        }
    }

    return shadow / 25.0;  // Average of 5x5 = 25 samples
}
```

### Shadow Application

```glsl
// Calculate shadow factor
float shadow = calculateShadow(FragPosLightSpace);

// Apply to lighting (ambient unaffected)
float shadowFactor = 0.3 + 0.7 * shadow;  // Retain 30% in shadow
diffuse *= shadowFactor;
specular *= shadow;  // Full shadow on specular

vec3 result = (ambient + diffuse + specular) * surfaceColor;
```

## Anti-Aliasing Techniques

### 1. Shadow Bias

A small depth bias (0.002) prevents shadow acne:

```glsl
shadow += texture(shadowMap, vec3(projCoords.xy + offset, projCoords.z - 0.002));
```

### 2. Front-Face Culling

During shadow pass, front faces are culled to move shadow surfaces:

```cpp
glCullFace(GL_FRONT);  // Shadow pass
glCullFace(GL_BACK);   // Main pass (default)
```

### 3. Polygon Offset

Additional depth offset prevents z-fighting:

```cpp
glEnable(GL_POLYGON_OFFSET_FILL);
glPolygonOffset(2.0f, 4.0f);  // slope factor, constant factor
```

### 4. PCF Filtering

5x5 kernel smooths shadow edges:

- 25 samples per fragment
- Hardware comparison mode (`sampler2DShadow`)
- Soft penumbra effect

## Performance Considerations

### Shadow Map Resolution

| Resolution | Quality | Performance Impact |
|------------|---------|-------------------|
| 1024x1024  | Low     | Minimal           |
| 2048x2048  | Good    | Moderate (current)|
| 4096x4096  | High    | Significant       |

### PCF Kernel Size

| Kernel | Samples | Softness | Performance |
|--------|---------|----------|-------------|
| 3x3    | 9       | Sharp    | Fast        |
| 5x5    | 25      | Soft     | Good (current)|
| 7x7    | 49      | Very Soft| Slow        |

### Optimization Opportunities

1. **Cascaded Shadow Maps (CSM)**: Multiple shadow maps at different resolutions for near/far objects
2. **Shadow Map Caching**: Skip shadow pass when light hasn't moved
3. **LOD for Shadow Casters**: Simplified meshes in shadow pass
4. **Variable PCF**: Smaller kernel for distant shadows

## Vertex Shader Setup

Vertex shaders must pass fragment position in light space:

```glsl
// In main pass vertex shader
out vec4 FragPosLightSpace;
uniform mat4 lightSpaceMatrix;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;

    // Transform to light space for shadow sampling
    FragPosLightSpace = lightSpaceMatrix * worldPos;

    gl_Position = projection * view * worldPos;
}
```

## Debugging Tips

### Visualize Shadow Map

```cpp
// In debug mode, render shadow map to screen
glBindTexture(GL_TEXTURE_2D, shadowMap->getDepthTexture());
// Render fullscreen quad with depth-to-grayscale shader
```

### Check Light Space Matrix

```cpp
std::cout << "Light Space Matrix:" << std::endl;
for (int i = 0; i < 4; i++) {
    std::cout << shadowMap->getLightSpaceMatrix()[i].x << ", "
              << shadowMap->getLightSpaceMatrix()[i].y << ", "
              << shadowMap->getLightSpaceMatrix()[i].z << ", "
              << shadowMap->getLightSpaceMatrix()[i].w << std::endl;
}
```

### Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| Shadow acne | Insufficient bias | Increase bias or use slope-scale bias |
| Peter-panning | Bias too large | Use front-face culling instead |
| Blocky shadows | Low resolution | Increase shadow map size |
| No shadows | Incorrect matrices | Verify lightSpaceMatrix uniform |
| Shadows flicker | Z-fighting | Use polygon offset |
| Missing shadows on edges | Sampling outside map | Use CLAMP_TO_BORDER |

## Integration Example

### Simulation.h

```cpp
// Shadow mapping members
std::unique_ptr<ShadowMap> shadowMap;
std::unique_ptr<Shader> shadowShader;
std::unique_ptr<Shader> shadowCreatureShader;
glm::vec3 sunDirection;

// Public access
ShadowMap* getShadowMap() const { return shadowMap.get(); }
bool hasShadowMapping() const { return shadowMap != nullptr; }
```

### Main Render Loop (main.cpp)

```cpp
// Set shadow uniforms for terrain shader
if (simulation->hasShadowMapping()) {
    ShadowMap* shadowMap = simulation->getShadowMap();
    shader.setMat4("lightSpaceMatrix", shadowMap->getLightSpaceMatrix());
    shader.setBool("shadowsEnabled", true);
    shadowMap->bindForReading(GL_TEXTURE0);
    shader.setInt("shadowMap", 0);
} else {
    shader.setBool("shadowsEnabled", false);
}
```

## Future Improvements

1. **Cascaded Shadow Maps**: Better quality for large terrains
2. **Variance Shadow Maps**: Softer shadows with fewer samples
3. **Contact Hardening**: Realistic soft/hard shadow transitions
4. **Temporal Filtering**: Reduce flickering in animated shadows
5. **Shadow Map Caching**: Performance optimization for static sun

## References

- LearnOpenGL Shadow Mapping: https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
- OpenGL Shadow Mapping Tutorial: https://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/
- Percentage-Closer Filtering: https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing
