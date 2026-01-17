# Phase 5: Complete 3D World Setup - Agent Implementation Guide

## Overview
Phase 5 restores and enhances all 3D visual elements from the pre-DX12 codebase. This document provides **exact specifications** for implementing terrain, water, grass, trees, and shadows using DirectX 12 via the ForgeEngine RHI abstraction layer.

**Current State:**
- DX12 rendering pipeline is working
- Creatures render as colored spheres (1100+ at 60fps)
- Ground is a flat green plane
- Camera controls work (WASD, mouse rotation, zoom)
- ImGui debug panels functional

**Target State:**
- Heightmap-based terrain with biome texturing
- Animated water planes with reflections
- Dense grass with wind animation
- L-System procedural trees
- Cascaded shadow maps

---

# TASK 1: HEIGHTMAP TERRAIN RENDERING

## 1.1 Files to Modify/Create

| File | Action | Purpose |
|------|--------|---------|
| `src/graphics/rendering/TerrainRenderer_DX12.h` | CREATE | Terrain renderer class header |
| `src/graphics/rendering/TerrainRenderer_DX12.cpp` | CREATE | Terrain renderer implementation |
| `shaders/hlsl/Terrain_VS.hlsl` | CREATE | Terrain vertex shader |
| `shaders/hlsl/Terrain_PS.hlsl` | CREATE | Terrain pixel shader |
| `src/main.cpp` | MODIFY | Integrate terrain renderer |

## 1.2 Existing Code to Use

The following files already exist and contain the logic you need:

### TerrainChunk System (`src/environment/TerrainChunk.h/cpp`)
```cpp
namespace TerrainConfig {
    constexpr int WORLD_SIZE = 2048;           // Total world units
    constexpr int CHUNK_SIZE = 64;             // Units per chunk
    constexpr int CHUNKS_PER_AXIS = 32;        // 32x32 grid = 1024 chunks
    constexpr int LOD_LEVELS = 4;              // 4 LOD levels
    constexpr float HEIGHT_SCALE = 30.0f;      // World height scale
    constexpr float WATER_LEVEL = 0.35f;       // Normalized water level

    // LOD resolutions (vertices per edge)
    constexpr int LOD_RESOLUTIONS[4] = { 65, 33, 17, 9 };

    // LOD switch distances
    constexpr float LOD_DISTANCES[4] = { 100.0f, 250.0f, 500.0f, 1000.0f };
}
```

### Terrain Height Generation (`src/environment/Terrain.cpp`)
```cpp
// Existing height generation uses Perlin noise
float height = noise.octaveNoise(nx * 4.0f, nz * 4.0f, 6, 0.5f);
float islandFactor = 1.0f - smoothstep(0.3f, 1.0f, distance);
height = height * islandFactor;
```

### Terrain Biome Colors (`src/environment/Terrain.cpp` - getTerrainColor())
```cpp
// Height-based coloring
if (height < waterLevel)      return glm::vec3(0.2f, 0.4f, 0.8f);  // Water
else if (height < 0.42f)      return glm::vec3(0.9f, 0.85f, 0.6f); // Beach
else if (height < 0.65f)      return glm::vec3(0.3f, 0.7f, 0.3f);  // Grass
else if (height < 0.8f)       return glm::vec3(0.2f, 0.5f, 0.2f);  // Forest
else                          return glm::vec3(0.6f, 0.6f, 0.6f);  // Mountain
```

## 1.3 TerrainRenderer_DX12.h - EXACT SPECIFICATION

```cpp
#pragma once

#include "../Camera.h"
#include "../Frustum.h"
#include "../../environment/TerrainChunk.h"
#include "../../environment/TerrainChunkManager.h"
#include "RHI/RHI.h"

#include <vector>
#include <memory>

using namespace Forge;
using namespace Forge::RHI;

// Terrain constant buffer (256-byte aligned for DX12)
struct alignas(256) TerrainConstants {
    float viewProj[16];       // 64 bytes - combined view*proj matrix
    float world[16];          // 64 bytes - chunk world transform
    float cameraPos[4];       // 16 bytes - camera position + padding
    float lightDir[4];        // 16 bytes - sun direction + padding
    float lightColor[4];      // 16 bytes - sun color + intensity in w
    float terrainScale[4];    // 16 bytes - x=heightScale, y=chunkSize, z=waterLevel, w=time
    float texCoordScale[4];   // 16 bytes - texture tiling parameters
    float padding[8];         // 32 bytes - pad to 256
};
static_assert(sizeof(TerrainConstants) == 256, "TerrainConstants must be 256 bytes");

// Terrain vertex format for DX12 (matches existing TerrainChunk vertex)
struct TerrainVertexDX12 {
    float position[3];   // World position XYZ
    float normal[3];     // Surface normal
    float color[3];      // Biome color RGB
    float texCoord[2];   // UV coordinates
};
static_assert(sizeof(TerrainVertexDX12) == 44, "TerrainVertexDX12 must be 44 bytes");

class TerrainRendererDX12 {
public:
    TerrainRendererDX12();
    ~TerrainRendererDX12();

    // Initialize with device and terrain reference
    bool init(IDevice* device, TerrainChunkManager* chunkManager);

    // Render visible terrain chunks
    void render(
        ICommandList* cmdList,
        IPipeline* pipeline,
        const Camera& camera,
        const glm::vec3& lightDir,
        const glm::vec3& lightColor,
        float time
    );

    // Render for shadow pass (depth only)
    void renderForShadow(
        ICommandList* cmdList,
        IPipeline* shadowPipeline,
        const glm::mat4& lightViewProj
    );

    // Statistics
    int getRenderedChunkCount() const { return m_renderedChunks; }
    int getCulledChunkCount() const { return m_culledChunks; }

private:
    IDevice* m_device = nullptr;
    TerrainChunkManager* m_chunkManager = nullptr;

    // Per-chunk mesh data converted to DX12 format
    struct ChunkMeshDX12 {
        UniquePtr<IBuffer> vertexBuffer;
        UniquePtr<IBuffer> indexBuffer;
        uint32_t indexCount = 0;
    };
    std::vector<ChunkMeshDX12> m_chunkMeshes;

    // Constant buffer (sized for all chunks)
    UniquePtr<IBuffer> m_constantBuffer;

    // Statistics
    int m_renderedChunks = 0;
    int m_culledChunks = 0;

    bool createChunkMesh(int chunkIndex, const TerrainChunk* chunk);
    void updateConstants(int chunkIndex, const TerrainChunk* chunk,
                        const Camera& camera, const glm::vec3& lightDir,
                        const glm::vec3& lightColor, float time);
};
```

## 1.4 Terrain Vertex Shader - EXACT CODE

Create `shaders/hlsl/Terrain_VS.hlsl`:
```hlsl
// Terrain Vertex Shader
// Input: per-vertex position/normal/color from chunk mesh
// Output: world position, normal, color, texcoord for pixel shader

cbuffer TerrainConstants : register(b0) {
    float4x4 viewProj;
    float4x4 world;
    float4 cameraPos;
    float4 lightDir;
    float4 lightColor;
    float4 terrainScale;    // x=heightScale, y=chunkSize, z=waterLevel, w=time
    float4 texCoordScale;   // x=u scale, y=v scale
};

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 color : COLOR;
    float2 texCoord : TEXCOORD;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 color : TEXCOORD2;
    float2 texCoord : TEXCOORD3;
    float height : TEXCOORD4;
};

PSInput main(VSInput input) {
    PSInput output;

    // Transform to world space
    float4 worldPos = mul(world, float4(input.position, 1.0));
    output.position = mul(viewProj, worldPos);
    output.worldPos = worldPos.xyz;

    // Transform normal to world space
    output.normal = normalize(mul((float3x3)world, input.normal));

    // Pass through color and texcoord
    output.color = input.color;
    output.texCoord = input.texCoord * texCoordScale.xy;

    // Normalized height for biome blending
    output.height = input.position.y / terrainScale.x;

    return output;
}
```

## 1.5 Terrain Pixel Shader - EXACT CODE

Create `shaders/hlsl/Terrain_PS.hlsl`:
```hlsl
// Terrain Pixel Shader
// Implements biome coloring based on height/slope with basic lighting

cbuffer TerrainConstants : register(b0) {
    float4x4 viewProj;
    float4x4 world;
    float4 cameraPos;
    float4 lightDir;
    float4 lightColor;
    float4 terrainScale;    // x=heightScale, y=chunkSize, z=waterLevel, w=time
    float4 texCoordScale;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 color : TEXCOORD2;
    float2 texCoord : TEXCOORD3;
    float height : TEXCOORD4;
};

// Biome colors
static const float3 WATER_COLOR = float3(0.2, 0.4, 0.8);
static const float3 BEACH_COLOR = float3(0.9, 0.85, 0.6);
static const float3 GRASS_COLOR = float3(0.3, 0.7, 0.3);
static const float3 FOREST_COLOR = float3(0.2, 0.5, 0.2);
static const float3 MOUNTAIN_COLOR = float3(0.6, 0.6, 0.6);
static const float3 SNOW_COLOR = float3(0.95, 0.95, 1.0);

// Height thresholds (normalized)
static const float WATER_LEVEL = 0.35;
static const float BEACH_LEVEL = 0.42;
static const float GRASS_LEVEL = 0.65;
static const float FOREST_LEVEL = 0.80;
static const float SNOW_LEVEL = 0.92;

float3 GetBiomeColor(float height, float slope) {
    // Slope factor (steeper = more rock)
    float rockBlend = saturate(slope * 3.0);

    // Height-based color selection
    float3 baseColor;
    if (height < WATER_LEVEL) {
        baseColor = WATER_COLOR;
    }
    else if (height < BEACH_LEVEL) {
        float t = (height - WATER_LEVEL) / (BEACH_LEVEL - WATER_LEVEL);
        baseColor = lerp(WATER_COLOR, BEACH_COLOR, t);
    }
    else if (height < GRASS_LEVEL) {
        float t = (height - BEACH_LEVEL) / (GRASS_LEVEL - BEACH_LEVEL);
        baseColor = lerp(BEACH_COLOR, GRASS_COLOR, t);
    }
    else if (height < FOREST_LEVEL) {
        float t = (height - GRASS_LEVEL) / (FOREST_LEVEL - GRASS_LEVEL);
        baseColor = lerp(GRASS_COLOR, FOREST_COLOR, t);
    }
    else if (height < SNOW_LEVEL) {
        float t = (height - FOREST_LEVEL) / (SNOW_LEVEL - FOREST_LEVEL);
        baseColor = lerp(FOREST_COLOR, MOUNTAIN_COLOR, t);
    }
    else {
        float t = saturate((height - SNOW_LEVEL) / (1.0 - SNOW_LEVEL));
        baseColor = lerp(MOUNTAIN_COLOR, SNOW_COLOR, t);
    }

    // Blend with rock color on steep slopes
    return lerp(baseColor, MOUNTAIN_COLOR, rockBlend);
}

float4 main(PSInput input) : SV_TARGET {
    // Calculate slope from normal (1 = flat, 0 = vertical)
    float slope = 1.0 - input.normal.y;

    // Get biome color
    float3 albedo = GetBiomeColor(input.height, slope);

    // Use vertex color if available (override)
    if (length(input.color) > 0.1) {
        albedo = input.color;
    }

    // Lighting calculation
    float3 N = normalize(input.normal);
    float3 L = normalize(-lightDir.xyz);

    // Diffuse (Lambertian)
    float NdotL = max(dot(N, L), 0.0);
    float3 diffuse = albedo * lightColor.rgb * NdotL;

    // Ambient (simple hemisphere)
    float3 skyColor = float3(0.5, 0.7, 1.0);
    float3 groundColor = float3(0.3, 0.25, 0.2);
    float3 ambient = albedo * lerp(groundColor, skyColor, N.y * 0.5 + 0.5) * 0.3;

    // Final color
    float3 finalColor = ambient + diffuse;

    // Fog (optional - based on distance)
    float3 viewDir = cameraPos.xyz - input.worldPos;
    float distance = length(viewDir);
    float fogFactor = saturate((distance - 200.0) / 800.0);
    float3 fogColor = float3(0.7, 0.8, 0.9);
    finalColor = lerp(finalColor, fogColor, fogFactor);

    return float4(finalColor, 1.0);
}
```

## 1.6 Integration into main.cpp

Add to `main.cpp` after creature rendering setup:

```cpp
// === TERRAIN RENDERING ===
#include "graphics/rendering/TerrainRenderer_DX12.h"

// In AppState struct:
std::unique_ptr<TerrainRendererDX12> terrainRenderer;
UniquePtr<IShader> terrainVertexShader;
UniquePtr<IShader> terrainPixelShader;
UniquePtr<IPipeline> terrainPipeline;

// In initialization:
g_app.terrainRenderer = std::make_unique<TerrainRendererDX12>();
if (!g_app.terrainRenderer->init(g_app.device.get(), terrainChunkManager)) {
    std::cerr << "Failed to initialize terrain renderer" << std::endl;
}

// Load terrain shaders (compile from HLSL)
ShaderDesc terrainVSDesc;
terrainVSDesc.type = ShaderType::Vertex;
terrainVSDesc.entryPoint = "main";
terrainVSDesc.source = loadShaderFile("shaders/hlsl/Terrain_VS.hlsl");
g_app.terrainVertexShader = g_app.device->CreateShader(terrainVSDesc);

ShaderDesc terrainPSDesc;
terrainPSDesc.type = ShaderType::Pixel;
terrainPSDesc.entryPoint = "main";
terrainPSDesc.source = loadShaderFile("shaders/hlsl/Terrain_PS.hlsl");
g_app.terrainPixelShader = g_app.device->CreateShader(terrainPSDesc);

// Create terrain pipeline
PipelineDesc terrainPipeDesc;
terrainPipeDesc.vertexShader = g_app.terrainVertexShader.get();
terrainPipeDesc.pixelShader = g_app.terrainPixelShader.get();
terrainPipeDesc.vertexLayout = {
    { "POSITION", 0, Format::R32G32B32_FLOAT, 0, 0, InputClass::PerVertex, 0 },
    { "NORMAL", 0, Format::R32G32B32_FLOAT, 0, 12, InputClass::PerVertex, 0 },
    { "COLOR", 0, Format::R32G32B32_FLOAT, 0, 24, InputClass::PerVertex, 0 },
    { "TEXCOORD", 0, Format::R32G32_FLOAT, 0, 36, InputClass::PerVertex, 0 },
};
terrainPipeDesc.renderTargetFormats = { Format::R8G8B8A8_UNORM };
terrainPipeDesc.depthFormat = Format::D32_FLOAT;
terrainPipeDesc.cullMode = CullMode::Back;
terrainPipeDesc.depthTest = true;
terrainPipeDesc.depthWrite = true;
g_app.terrainPipeline = g_app.device->CreatePipeline(terrainPipeDesc);

// In render loop (BEFORE creatures):
g_app.terrainRenderer->render(
    g_app.commandList.get(),
    g_app.terrainPipeline.get(),
    camera,
    glm::vec3(0.5f, -0.8f, 0.3f),  // lightDir
    glm::vec3(1.0f, 0.95f, 0.9f),  // lightColor
    totalTime
);
```

---

# TASK 2: WATER PLANE RENDERING

## 2.1 Files to Modify/Create

| File | Action | Purpose |
|------|--------|---------|
| `src/graphics/WaterRenderer.cpp` | MODIFY | Complete the implementation |
| `shaders/hlsl/Water_VS.hlsl` | CREATE | Water vertex shader |
| `shaders/hlsl/Water_PS.hlsl` | CREATE | Water pixel shader |
| `src/main.cpp` | MODIFY | Integrate water renderer |

## 2.2 Existing WaterRenderer Header

The header already exists at `src/graphics/WaterRenderer.h`. It defines:

```cpp
// Water constant buffer (512 bytes for DX12 alignment)
struct alignas(256) WaterConstants {
    glm::mat4 view;              // Camera view matrix
    glm::mat4 projection;        // Camera projection matrix
    glm::mat4 viewProjection;    // Combined V*P
    glm::vec3 cameraPos;
    float time;
    glm::vec3 lightDir;
    float _pad1;
    glm::vec3 lightColor;
    float sunIntensity;
    glm::vec4 waterColor;        // Deep water color
    glm::vec4 shallowColor;      // Shallow water color
    float waveScale;             // Wave frequency (0.1)
    float waveSpeed;             // Animation speed (0.5)
    float waveHeight;            // Wave amplitude (0.5)
    float transparency;          // Alpha (0.7)
    glm::vec3 skyTopColor;
    float _pad2;
    glm::vec3 skyHorizonColor;
    float fresnelPower;          // Fresnel exponent (3.0)
    float foamIntensity;         // Foam strength (0.3)
    float foamScale;             // Foam pattern scale (10.0)
    float specularPower;         // Glossiness (64.0)
    float specularIntensity;     // Specular brightness (1.0)
};
```

## 2.3 Water Vertex Shader - EXACT CODE

Create `shaders/hlsl/Water_VS.hlsl`:
```hlsl
// Water Vertex Shader
// Applies wave displacement and passes data to pixel shader

cbuffer WaterConstants : register(b0) {
    float4x4 view;
    float4x4 projection;
    float4x4 viewProjection;
    float4 cameraPosTime;      // xyz = cameraPos, w = time
    float4 lightDirPad;        // xyz = lightDir
    float4 lightColorIntensity;// xyz = lightColor, w = sunIntensity
    float4 waterColor;         // Deep water RGBA
    float4 shallowColor;       // Shallow water RGBA
    float4 waveParams;         // x=scale, y=speed, z=height, w=transparency
    float4 skyTopPad;          // xyz = skyTopColor
    float4 skyHorizonFresnel;  // xyz = skyHorizonColor, w = fresnelPower
    float4 foamSpecular;       // x=foamIntensity, y=foamScale, z=specPower, w=specIntensity
};

struct VSInput {
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 texCoord : TEXCOORD1;
    float3 normal : TEXCOORD2;
};

PSInput main(VSInput input) {
    PSInput output;

    float time = cameraPosTime.w;
    float waveScale = waveParams.x;
    float waveSpeed = waveParams.y;
    float waveHeight = waveParams.z;

    // Calculate wave displacement
    float3 worldPos = input.position;

    // Gerstner-like wave (simplified)
    float wave1 = sin(worldPos.x * waveScale + time * waveSpeed) *
                  cos(worldPos.z * waveScale * 0.7 + time * waveSpeed * 0.8);
    float wave2 = sin(worldPos.x * waveScale * 0.5 - time * waveSpeed * 1.2) *
                  cos(worldPos.z * waveScale * 0.3 + time * waveSpeed * 0.6);

    float waveDisplacement = (wave1 * 0.6 + wave2 * 0.4) * waveHeight;
    worldPos.y += waveDisplacement;

    // Calculate wave normal (derivative)
    float dx = waveScale * cos(worldPos.x * waveScale + time * waveSpeed) *
               cos(worldPos.z * waveScale * 0.7 + time * waveSpeed * 0.8) * waveHeight;
    float dz = -waveScale * 0.7 * sin(worldPos.x * waveScale + time * waveSpeed) *
               sin(worldPos.z * waveScale * 0.7 + time * waveSpeed * 0.8) * waveHeight;

    output.normal = normalize(float3(-dx, 1.0, -dz));

    output.position = mul(viewProjection, float4(worldPos, 1.0));
    output.worldPos = worldPos;
    output.texCoord = input.texCoord;

    return output;
}
```

## 2.4 Water Pixel Shader - EXACT CODE

Create `shaders/hlsl/Water_PS.hlsl`:
```hlsl
// Water Pixel Shader
// Implements fresnel reflections, specular, transparency, and foam

cbuffer WaterConstants : register(b0) {
    float4x4 view;
    float4x4 projection;
    float4x4 viewProjection;
    float4 cameraPosTime;
    float4 lightDirPad;
    float4 lightColorIntensity;
    float4 waterColor;
    float4 shallowColor;
    float4 waveParams;
    float4 skyTopPad;
    float4 skyHorizonFresnel;
    float4 foamSpecular;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float2 texCoord : TEXCOORD1;
    float3 normal : TEXCOORD2;
};

float4 main(PSInput input) : SV_TARGET {
    float3 cameraPos = cameraPosTime.xyz;
    float time = cameraPosTime.w;
    float3 lightDir = normalize(lightDirPad.xyz);
    float3 lightColor = lightColorIntensity.xyz;
    float sunIntensity = lightColorIntensity.w;
    float transparency = waveParams.w;
    float3 skyTop = skyTopPad.xyz;
    float3 skyHorizon = skyHorizonFresnel.xyz;
    float fresnelPower = skyHorizonFresnel.w;
    float foamIntensity = foamSpecular.x;
    float foamScale = foamSpecular.y;
    float specPower = foamSpecular.z;
    float specIntensity = foamSpecular.w;

    // View direction
    float3 V = normalize(cameraPos - input.worldPos);
    float3 N = normalize(input.normal);

    // Fresnel effect (more reflection at grazing angles)
    float fresnel = pow(1.0 - saturate(dot(V, N)), fresnelPower);

    // Sky reflection (simple gradient)
    float3 R = reflect(-V, N);
    float skyBlend = R.y * 0.5 + 0.5;
    float3 skyReflection = lerp(skyHorizon, skyTop, skyBlend);

    // Specular (sun reflection)
    float3 H = normalize(-lightDir + V);
    float NdotH = max(dot(N, H), 0.0);
    float specular = pow(NdotH, specPower) * specIntensity * sunIntensity;

    // Foam (wave crests)
    float foam = 0.0;
    if (foamIntensity > 0.0) {
        float foamPattern = sin(input.worldPos.x * foamScale + time * 2.0) *
                           sin(input.worldPos.z * foamScale * 0.8 + time * 1.5);
        float waveHeight = (input.worldPos.y - (-0.35 * 30.0)) / 2.0; // Normalize
        foam = saturate(foamPattern * waveHeight * foamIntensity);
    }

    // Combine water color with reflection
    float3 waterBase = lerp(waterColor.rgb, shallowColor.rgb, 0.3);
    float3 finalColor = lerp(waterBase, skyReflection, fresnel * 0.6);

    // Add specular highlight
    finalColor += lightColor * specular;

    // Add foam
    finalColor = lerp(finalColor, float3(1.0, 1.0, 1.0), foam);

    // Output with transparency
    return float4(finalColor, transparency);
}
```

## 2.5 Integration into main.cpp

```cpp
// === WATER RENDERING ===
#include "graphics/WaterRenderer.h"

// In AppState:
std::unique_ptr<WaterRenderer> waterRenderer;

// In initialization:
g_app.waterRenderer = std::make_unique<WaterRenderer>();
g_app.waterRenderer->Initialize(g_app.device.get(), Format::R8G8B8A8_UNORM, Format::D32_FLOAT);

// Generate water mesh at water level height
float waterHeight = TerrainConfig::WATER_LEVEL * TerrainConfig::HEIGHT_SCALE;
g_app.waterRenderer->GenerateMesh(64, TerrainConfig::WORLD_SIZE, waterHeight);

// Set water parameters
g_app.waterRenderer->SetWaterColor(
    glm::vec4(0.0f, 0.2f, 0.4f, 1.0f),   // Deep
    glm::vec4(0.0f, 0.5f, 0.6f, 1.0f)    // Shallow
);
g_app.waterRenderer->SetWaveParams(0.1f, 0.5f, 0.5f);
g_app.waterRenderer->SetTransparency(0.7f);

// In render loop (AFTER terrain, BEFORE creatures):
g_app.waterRenderer->Render(
    g_app.commandList.get(),
    camera.getViewMatrix(),
    camera.getProjectionMatrix(),
    camera.getPosition(),
    glm::vec3(0.5f, -0.8f, 0.3f),  // lightDir
    glm::vec3(1.0f, 0.95f, 0.9f),  // lightColor
    1.0f,                          // sunIntensity
    totalTime
);
```

---

# TASK 3: GRASS SYSTEM

## 3.1 Files to Modify/Create

| File | Action | Purpose |
|------|--------|---------|
| `src/graphics/rendering/GrassRenderer_DX12.h` | CREATE | Grass renderer header |
| `src/graphics/rendering/GrassRenderer_DX12.cpp` | CREATE | Grass renderer implementation |
| `shaders/hlsl/Grass_VS.hlsl` | CREATE | Grass vertex shader with instancing |
| `shaders/hlsl/Grass_PS.hlsl` | CREATE | Grass pixel shader |
| `src/main.cpp` | MODIFY | Integrate grass renderer |

## 3.2 Existing GrassSystem Header

`src/environment/GrassSystem.h` already defines:

```cpp
struct GrassBladeInstance {
    glm::vec3 position;     // Base position
    float rotation;         // Y-axis rotation
    float height;           // Blade height
    float width;            // Blade width at base
    float bendFactor;       // How much it bends (0-1)
    float colorVariation;   // Color randomization (0-1)
};

struct GrassConfig {
    float density = 1.0f;           // Blades per square unit
    float minHeight = 0.2f;
    float maxHeight = 0.5f;
    float minWidth = 0.02f;
    float maxWidth = 0.05f;
    glm::vec3 baseColor = {0.3f, 0.5f, 0.2f};
    glm::vec3 tipColor = {0.4f, 0.6f, 0.25f};
    bool hasFlowers = false;
    float flowerDensity = 0.0f;
};
```

## 3.3 GrassRenderer_DX12.h - EXACT SPECIFICATION

```cpp
#pragma once

#include "../../environment/GrassSystem.h"
#include "../Camera.h"
#include "RHI/RHI.h"
#include <vector>

using namespace Forge;
using namespace Forge::RHI;

// GPU instance data (must match HLSL layout)
#pragma pack(push, 4)
struct GrassInstanceGPU {
    float position[3];        // 12 bytes - World position
    float rotation;           // 4 bytes  - Y rotation (radians)
    float height;             // 4 bytes  - Blade height
    float width;              // 4 bytes  - Blade width
    float bendFactor;         // 4 bytes  - Bend amount (0-1)
    float colorVariation;     // 4 bytes  - Color variation (0-1)
};
#pragma pack(pop)
static_assert(sizeof(GrassInstanceGPU) == 32, "GrassInstanceGPU must be 32 bytes");

// Grass constant buffer
struct alignas(256) GrassConstants {
    float viewProj[16];       // 64 bytes
    float cameraPos[4];       // 16 bytes - xyz = pos, w = unused
    float windDir[4];         // 16 bytes - xy = direction, z = strength, w = time
    float grassColors[8];     // 32 bytes - base color (rgb + pad) + tip color (rgb + pad)
    float lightDir[4];        // 16 bytes
    float lightColor[4];      // 16 bytes
    float lodParams[4];       // 16 bytes - x=lodDist1, y=lodDist2, z=maxDist, w=unused
    float padding[16];        // 64 bytes - pad to 256
};
static_assert(sizeof(GrassConstants) == 256, "GrassConstants must be 256 bytes");

class GrassRendererDX12 {
public:
    GrassRendererDX12();
    ~GrassRendererDX12();

    bool init(IDevice* device, GrassSystem* grassSystem);

    void updateInstances(const glm::vec3& cameraPos);

    void render(
        ICommandList* cmdList,
        IPipeline* pipeline,
        const Camera& camera,
        const glm::vec3& lightDir,
        const glm::vec3& lightColor,
        float time
    );

    void setWindParams(const glm::vec2& direction, float strength);

    int getVisibleBladeCount() const { return m_visibleCount; }

private:
    IDevice* m_device = nullptr;
    GrassSystem* m_grassSystem = nullptr;

    UniquePtr<IBuffer> m_instanceBuffer;
    UniquePtr<IBuffer> m_bladeVertexBuffer;  // Billboard quad vertices
    UniquePtr<IBuffer> m_bladeIndexBuffer;
    UniquePtr<IBuffer> m_constantBuffer;

    std::vector<GrassInstanceGPU> m_visibleInstances;
    int m_visibleCount = 0;

    // Wind parameters
    glm::vec2 m_windDirection = {1.0f, 0.0f};
    float m_windStrength = 0.3f;

    // LOD parameters
    float m_lodDistance1 = 30.0f;
    float m_lodDistance2 = 60.0f;
    float m_maxRenderDistance = 100.0f;

    bool createBladeGeometry();
    void updateInstanceBuffer();
};
```

## 3.4 Grass Vertex Shader - EXACT CODE

Create `shaders/hlsl/Grass_VS.hlsl`:
```hlsl
// Grass Vertex Shader with GPU Instancing
// Renders grass blades as billboards with wind animation

cbuffer GrassConstants : register(b0) {
    float4x4 viewProj;
    float4 cameraPosUnused;    // xyz = camera position
    float4 windDirTime;        // xy = wind direction, z = strength, w = time
    float4 baseColor;          // RGB + padding
    float4 tipColor;           // RGB + padding
    float4 lightDir;
    float4 lightColor;
    float4 lodParams;          // x=lodDist1, y=lodDist2, z=maxDist
};

// Per-vertex data (billboard quad)
struct VSInput {
    float3 position : POSITION;    // Local quad position (-0.5 to 0.5)
    float2 texCoord : TEXCOORD;
};

// Per-instance data
struct InstanceData {
    float3 worldPos : INSTANCE_POSITION;
    float rotation : INSTANCE_ROTATION;
    float height : INSTANCE_HEIGHT;
    float width : INSTANCE_WIDTH;
    float bendFactor : INSTANCE_BEND;
    float colorVar : INSTANCE_COLOR;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float3 color : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
};

PSInput main(VSInput vertex, InstanceData instance) {
    PSInput output;

    float3 cameraPos = cameraPosUnused.xyz;
    float2 windDir = windDirTime.xy;
    float windStrength = windDirTime.z;
    float time = windDirTime.w;

    // Billboard orientation (face camera)
    float3 toCam = normalize(cameraPos - instance.worldPos);
    float3 right = normalize(cross(float3(0, 1, 0), toCam));
    float3 up = float3(0, 1, 0);

    // Apply Y-axis rotation
    float c = cos(instance.rotation);
    float s = sin(instance.rotation);
    right = right * c + cross(up, right) * s;

    // Wind animation (stronger at top of blade)
    float windPhase = time * 2.0 + instance.worldPos.x * 0.5 + instance.worldPos.z * 0.3;
    float windOffset = sin(windPhase) * windStrength * vertex.texCoord.y * vertex.texCoord.y;

    // Add bend from instance data
    float totalBend = windOffset + instance.bendFactor * 0.3;

    // Build world position
    float3 localPos = right * vertex.position.x * instance.width +
                      up * vertex.position.y * instance.height;

    // Apply wind/bend offset (horizontal displacement)
    localPos.x += totalBend * vertex.texCoord.y * windDir.x;
    localPos.z += totalBend * vertex.texCoord.y * windDir.y;

    float3 worldPos = instance.worldPos + localPos;

    output.position = mul(viewProj, float4(worldPos, 1.0));
    output.texCoord = vertex.texCoord;
    output.worldPos = worldPos;

    // Interpolate color from base to tip with variation
    float3 variedBase = baseColor.rgb * (1.0 - instance.colorVar * 0.2);
    float3 variedTip = tipColor.rgb * (1.0 + instance.colorVar * 0.1);
    output.color = lerp(variedBase, variedTip, vertex.texCoord.y);

    return output;
}
```

## 3.5 Grass Pixel Shader - EXACT CODE

Create `shaders/hlsl/Grass_PS.hlsl`:
```hlsl
// Grass Pixel Shader
// Simple lit grass blade with alpha cutout

cbuffer GrassConstants : register(b0) {
    float4x4 viewProj;
    float4 cameraPosUnused;
    float4 windDirTime;
    float4 baseColor;
    float4 tipColor;
    float4 lightDir;
    float4 lightColor;
    float4 lodParams;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float3 color : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
};

float4 main(PSInput input) : SV_TARGET {
    // Alpha mask for grass blade shape (tapered at top)
    float alpha = 1.0 - abs(input.texCoord.x * 2.0 - 1.0);  // Taper at edges
    alpha *= 1.0 - input.texCoord.y * 0.3;  // Slight taper at top

    // Alpha cutout
    if (alpha < 0.5) discard;

    // Simple lighting
    float3 L = normalize(-lightDir.xyz);
    float NdotL = max(dot(float3(0, 1, 0), L), 0.0);  // Assume up-facing normals

    float3 ambient = input.color * 0.4;
    float3 diffuse = input.color * lightColor.rgb * NdotL * 0.6;

    float3 finalColor = ambient + diffuse;

    return float4(finalColor, 1.0);
}
```

## 3.6 Integration into main.cpp

```cpp
// === GRASS RENDERING ===
#include "graphics/rendering/GrassRenderer_DX12.h"

// In AppState:
std::unique_ptr<GrassRendererDX12> grassRenderer;
UniquePtr<IPipeline> grassPipeline;

// In initialization:
g_app.grassRenderer = std::make_unique<GrassRendererDX12>();
g_app.grassRenderer->init(g_app.device.get(), grassSystem);

// Create grass pipeline (with alpha blend)
PipelineDesc grassPipeDesc;
grassPipeDesc.vertexShader = grassVS.get();
grassPipeDesc.pixelShader = grassPS.get();
grassPipeDesc.vertexLayout = {
    // Per-vertex (slot 0)
    { "POSITION", 0, Format::R32G32B32_FLOAT, 0, 0, InputClass::PerVertex, 0 },
    { "TEXCOORD", 0, Format::R32G32_FLOAT, 0, 12, InputClass::PerVertex, 0 },
    // Per-instance (slot 1)
    { "INSTANCE_POSITION", 0, Format::R32G32B32_FLOAT, 1, 0, InputClass::PerInstance, 1 },
    { "INSTANCE_ROTATION", 0, Format::R32_FLOAT, 1, 12, InputClass::PerInstance, 1 },
    { "INSTANCE_HEIGHT", 0, Format::R32_FLOAT, 1, 16, InputClass::PerInstance, 1 },
    { "INSTANCE_WIDTH", 0, Format::R32_FLOAT, 1, 20, InputClass::PerInstance, 1 },
    { "INSTANCE_BEND", 0, Format::R32_FLOAT, 1, 24, InputClass::PerInstance, 1 },
    { "INSTANCE_COLOR", 0, Format::R32_FLOAT, 1, 28, InputClass::PerInstance, 1 },
};
grassPipeDesc.cullMode = CullMode::None;  // Double-sided grass
grassPipeDesc.blendEnable = false;  // Using alpha cutout, not blend
grassPipeDesc.depthTest = true;
grassPipeDesc.depthWrite = true;
g_app.grassPipeline = g_app.device->CreatePipeline(grassPipeDesc);

// In render loop (AFTER terrain, AFTER water, BEFORE creatures):
g_app.grassRenderer->updateInstances(camera.getPosition());
g_app.grassRenderer->render(
    g_app.commandList.get(),
    g_app.grassPipeline.get(),
    camera,
    glm::vec3(0.5f, -0.8f, 0.3f),
    glm::vec3(1.0f, 0.95f, 0.9f),
    totalTime
);
```

---

# TASK 4: TREE RENDERING

## 4.1 Files to Modify/Create

| File | Action | Purpose |
|------|--------|---------|
| `src/graphics/rendering/TreeRenderer_DX12.h` | CREATE | Tree renderer header |
| `src/graphics/rendering/TreeRenderer_DX12.cpp` | CREATE | Tree renderer implementation |
| `shaders/hlsl/Tree_VS.hlsl` | CREATE | Tree vertex shader |
| `shaders/hlsl/Tree_PS.hlsl` | CREATE | Tree pixel shader |
| `src/main.cpp` | MODIFY | Integrate tree renderer |

## 4.2 Existing Tree Generation System

### TreeType Enum (`src/environment/TreeGenerator.h`)
```cpp
enum class TreeType {
    OAK, PINE, WILLOW, BIRCH,           // Temperate
    PALM, MANGROVE, KAPOK,               // Tropical
    CACTUS_SAGUARO, CACTUS_BARREL, JOSHUA_TREE,  // Desert
    SPRUCE, FIR,                         // Boreal
    ACACIA, BAOBAB,                      // Savanna
    CYPRESS,                             // Swamp
    JUNIPER, ALPINE_FIR,                 // Mountain
    BUSH,                                // Generic
    COUNT  // 18 types
};
```

### L-System Grammar (`src/environment/LSystem.h`)
```cpp
// Commands:
// F = forward (draw branch)
// + = yaw right, - = yaw left
// & = pitch down, ^ = pitch up
// / = roll left, \ = roll right
// [ = push state, ] = pop state
// ! = decrease radius
// L = leaf placeholder
```

### MeshData Output (`src/graphics/mesh/MeshData.h`)
```cpp
struct MeshData {
    std::vector<Vertex> vertices;     // Position, Normal, TexCoord, Color
    std::vector<uint32_t> indices;    // Triangle indices
    glm::vec3 boundsMin;
    glm::vec3 boundsMax;
};
```

## 4.3 TreeRenderer_DX12.h - EXACT SPECIFICATION

```cpp
#pragma once

#include "../../environment/TreeGenerator.h"
#include "../../environment/VegetationManager.h"
#include "../Camera.h"
#include "RHI/RHI.h"
#include <unordered_map>

using namespace Forge;
using namespace Forge::RHI;

// Tree instance data (per-tree transform)
struct TreeInstanceGPU {
    float position[3];    // World position
    float rotation;       // Y-axis rotation (radians)
    float scale[3];       // Non-uniform scale
    int treeType;         // TreeType enum value
};
static_assert(sizeof(TreeInstanceGPU) == 32, "TreeInstanceGPU must be 32 bytes");

// Tree constant buffer
struct alignas(256) TreeConstants {
    float viewProj[16];       // 64 bytes
    float model[16];          // 64 bytes - per-tree model matrix
    float cameraPos[4];       // 16 bytes
    float lightDir[4];        // 16 bytes
    float lightColor[4];      // 16 bytes
    float windParams[4];      // 16 bytes - xy=dir, z=strength, w=time
    float padding[16];        // 64 bytes
};
static_assert(sizeof(TreeConstants) == 256, "TreeConstants must be 256 bytes");

class TreeRendererDX12 {
public:
    TreeRendererDX12();
    ~TreeRendererDX12();

    bool init(IDevice* device, VegetationManager* vegManager);

    // Pre-generate meshes for all tree types
    void generateTreeMeshes();

    void render(
        ICommandList* cmdList,
        IPipeline* pipeline,
        const Camera& camera,
        const glm::vec3& lightDir,
        const glm::vec3& lightColor,
        float time
    );

    void renderForShadow(
        ICommandList* cmdList,
        IPipeline* shadowPipeline,
        const glm::mat4& lightViewProj
    );

    int getRenderedTreeCount() const { return m_renderedCount; }
    int getTotalTreeCount() const { return m_totalCount; }

private:
    IDevice* m_device = nullptr;
    VegetationManager* m_vegManager = nullptr;

    // Pre-generated mesh per tree type
    struct TreeMeshDX12 {
        UniquePtr<IBuffer> vertexBuffer;
        UniquePtr<IBuffer> indexBuffer;
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        glm::vec3 boundsMin;
        glm::vec3 boundsMax;
    };
    std::unordered_map<TreeType, TreeMeshDX12> m_treeMeshes;

    // Instance buffer for each tree type
    std::unordered_map<TreeType, UniquePtr<IBuffer>> m_instanceBuffers;
    std::unordered_map<TreeType, std::vector<TreeInstanceGPU>> m_instances;

    UniquePtr<IBuffer> m_constantBuffer;

    int m_renderedCount = 0;
    int m_totalCount = 0;

    // Wind parameters
    glm::vec2 m_windDirection = {1.0f, 0.0f};
    float m_windStrength = 0.2f;

    bool createMeshForType(TreeType type);
    void collectVisibleInstances(const Camera& camera);
};
```

## 4.4 Tree Shaders (Simpler than grass - per-vertex color from mesh)

### Tree_VS.hlsl
```hlsl
cbuffer TreeConstants : register(b0) {
    float4x4 viewProj;
    float4x4 model;
    float4 cameraPos;
    float4 lightDir;
    float4 lightColor;
    float4 windParams;  // xy=dir, z=strength, w=time
};

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 color : COLOR;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 color : TEXCOORD2;
};

PSInput main(VSInput input) {
    PSInput output;

    // Slight wind sway for leaves (based on height)
    float3 pos = input.position;
    float height = pos.y;
    float windOffset = sin(windParams.w * 2.0 + pos.x * 0.5) * windParams.z * height * 0.01;
    pos.x += windOffset * windParams.x;
    pos.z += windOffset * windParams.y;

    float4 worldPos = mul(model, float4(pos, 1.0));
    output.position = mul(viewProj, worldPos);
    output.worldPos = worldPos.xyz;
    output.normal = normalize(mul((float3x3)model, input.normal));
    output.color = input.color;

    return output;
}
```

### Tree_PS.hlsl
```hlsl
cbuffer TreeConstants : register(b0) {
    float4x4 viewProj;
    float4x4 model;
    float4 cameraPos;
    float4 lightDir;
    float4 lightColor;
    float4 windParams;
};

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 color : TEXCOORD2;
};

float4 main(PSInput input) : SV_TARGET {
    float3 N = normalize(input.normal);
    float3 L = normalize(-lightDir.xyz);

    float NdotL = max(dot(N, L), 0.0);

    float3 ambient = input.color * 0.35;
    float3 diffuse = input.color * lightColor.rgb * NdotL * 0.65;

    return float4(ambient + diffuse, 1.0);
}
```

---

# TASK 5: SHADOW MAPPING

## 5.1 Files Already Exist

| File | Status | Purpose |
|------|--------|---------|
| `src/graphics/ShadowMap_DX12.h` | EXISTS | Single shadow map + CSM classes |
| `src/graphics/ShadowMap_DX12.cpp` | EXISTS | Shadow map implementation |

## 5.2 Shadow System Overview

Two options are available:

### Option A: Single Shadow Map (Simpler)
```cpp
ShadowMapDX12 shadowMap;
shadowMap.init(device, dsvHeap, srvHeap, dsvIndex, srvIndex, 2048, 2048);

// Update light matrix each frame
shadowMap.updateLightSpaceMatrix(lightDir, sceneCenter, sceneRadius);

// Shadow pass
shadowMap.beginShadowPass(cmdList);
terrainRenderer->renderForShadow(cmdList, shadowPipeline, shadowMap.getLightSpaceMatrix());
creatureRenderer->renderForShadow(creatures, cmdList, shadowPipeline, time);
treeRenderer->renderForShadow(cmdList, shadowPipeline, shadowMap.getLightSpaceMatrixFloat4x4());
shadowMap.endShadowPass(cmdList);

// Main pass - bind shadow map as SRV
cmdList->SetGraphicsRootDescriptorTable(shadowTextureSlot, shadowMap.getSRVGpuHandle());
```

### Option B: Cascaded Shadow Maps (Better quality)
```cpp
CascadedShadowMapDX12 csmShadowMap;
csmShadowMap.init(device, dsvHeap, srvHeap, dsvStartIndex, srvIndex, 2048);
csmShadowMap.setCascadeSplits({15.0f, 50.0f, 150.0f, 500.0f});

// Update cascades each frame
csmShadowMap.updateCascades(cameraView, cameraProj, lightDir, nearPlane, farPlane);

// Render each cascade
for (uint32_t i = 0; i < CSM_CASCADE_COUNT; i++) {
    csmShadowMap.beginCascade(i, cmdList);
    glm::mat4 cascadeVP = glmFromXM(csmShadowMap.getCascadeViewProj(i));
    terrainRenderer->renderForShadow(cmdList, shadowPipeline, cascadeVP);
    creatureRenderer->renderForShadow(creatures, cmdList, shadowPipeline, time);
    csmShadowMap.endCascade(cmdList);
}
csmShadowMap.endShadowPass(cmdList);
```

## 5.3 Shadow Sampling in Shaders

Add to terrain/creature pixel shaders:

```hlsl
// Additional constant buffer for shadow
cbuffer ShadowConstants : register(b1) {
    float4x4 lightViewProj;
    float4 cascadeSplits;     // View-space depth splits
    float shadowBias;
    float3 padding;
};

Texture2D shadowMap : register(t4);
SamplerComparisonState shadowSampler : register(s1);

float CalculateShadow(float3 worldPos) {
    float4 lightSpacePos = mul(lightViewProj, float4(worldPos, 1.0));
    lightSpacePos.xyz /= lightSpacePos.w;

    float2 shadowUV = lightSpacePos.xy * 0.5 + 0.5;
    shadowUV.y = 1.0 - shadowUV.y;  // Flip Y for D3D

    float currentDepth = lightSpacePos.z - shadowBias;

    // PCF 3x3 filtering
    float shadow = 0.0;
    float2 texelSize = 1.0 / float2(2048.0, 2048.0);

    [unroll]
    for (int x = -1; x <= 1; x++) {
        [unroll]
        for (int y = -1; y <= 1; y++) {
            shadow += shadowMap.SampleCmpLevelZero(
                shadowSampler,
                shadowUV + float2(x, y) * texelSize,
                currentDepth
            );
        }
    }

    return shadow / 9.0;  // Average of 9 samples
}

// In main():
float shadow = CalculateShadow(input.worldPos);
float3 finalColor = ambient + diffuse * shadow;
```

---

# RENDER ORDER

The correct render order in `main.cpp` render loop:

```cpp
// 1. Shadow Pass (if enabled)
if (shadowsEnabled) {
    shadowMap.beginShadowPass(cmdList);
    terrainRenderer->renderForShadow(...);
    treeRenderer->renderForShadow(...);
    creatureRenderer->renderForShadow(...);
    shadowMap.endShadowPass(cmdList);
}

// 2. Main Pass - Opaque Geometry
cmdList->SetRenderTarget(backBuffer, depthBuffer);
cmdList->ClearRenderTarget(backBuffer, {0.5f, 0.7f, 1.0f, 1.0f});  // Sky blue
cmdList->ClearDepthStencil(depthBuffer, 1.0f, 0);

// 2a. Terrain (furthest, solid)
terrainRenderer->render(cmdList, terrainPipeline, camera, lightDir, lightColor, time);

// 2b. Trees (solid)
treeRenderer->render(cmdList, treePipeline, camera, lightDir, lightColor, time);

// 2c. Creatures (solid)
creatureRenderer->render(creatures, camera, cmdList, creaturePipeline, time);

// 2d. Grass (alpha cutout, but solid pass)
grassRenderer->render(cmdList, grassPipeline, camera, lightDir, lightColor, time);

// 3. Transparent Pass
// 3a. Water (transparent - must be after all opaque)
waterRenderer->Render(cmdList, view, proj, cameraPos, lightDir, lightColor, sunIntensity, time);

// 4. UI Pass
ImGui::Render();
```

---

# PERFORMANCE TARGETS

| Element | Count | Target FPS |
|---------|-------|------------|
| Terrain chunks | 1024 (32x32) | 60+ fps |
| Grass blades | 100,000+ | 60+ fps |
| Trees | 5,000+ | 60+ fps |
| Creatures | 1,100+ | 60+ fps (achieved) |
| Water | 1 plane (64x64 grid) | 60+ fps |
| Shadow map | 2048x2048 | 60+ fps |

**Optimization Requirements:**
1. **Frustum Culling** - All renderers must cull off-screen objects
2. **LOD System** - Terrain and grass must use distance-based LOD
3. **GPU Instancing** - Grass and creatures use instanced rendering
4. **Batch by Material** - Trees batch by type, creatures batch by mesh key

---

# SUCCESS CRITERIA

- [ ] Terrain renders with height variation (not flat)
- [ ] Biome colors visible (green grass, tan beach, gray mountains)
- [ ] Water plane renders at sea level with transparency
- [ ] Water has animated waves and reflections
- [ ] Grass covers terrain surface with wind animation
- [ ] Trees placed procedurally based on biome
- [ ] Shadows visible on terrain and creatures
- [ ] All 1100+ creatures visible and moving
- [ ] Maintain 30+ FPS minimum, 60+ FPS target on RTX 3080 Ti
- [ ] Camera controls work smoothly
- [ ] No visual artifacts or Z-fighting

---

# DEBUGGING TIPS

1. **Nothing renders:** Check pipeline creation, root signature CBV/SRV counts, vertex buffer binding
2. **Black screen:** Check shader compilation errors in debug output
3. **Z-fighting:** Ensure depth format is D32_FLOAT, check near/far plane ratio
4. **Wrong colors:** Check constant buffer alignment (256 bytes), verify HLSL cbuffer layout matches C++
5. **Creatures not on terrain:** Query terrain height at creature position, add to Y coordinate
6. **Performance issues:** Enable GPU profiling, check draw call count, verify instancing is working
