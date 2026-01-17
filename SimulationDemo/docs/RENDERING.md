# SimulationDemo Rendering

DirectX 12 rendering pipeline documentation for the simulation.

## DX12 Pipeline Overview

### Architecture

SimulationDemo uses ModularGameEngine's RHI (Render Hardware Interface) module to abstract DirectX 12:

```
+------------------------------------------------------------------+
|                    APPLICATION RENDER LOOP                        |
+------------------------------------------------------------------+
                              |
                              v
+------------------------------------------------------------------+
|                      RHI ABSTRACTION                              |
+------------------------------------------------------------------+
| Device        | CommandQueue  | SwapChain    | DescriptorHeap    |
+------------------------------------------------------------------+
                              |
                              v
+------------------------------------------------------------------+
|                      DIRECTX 12 BACKEND                           |
+------------------------------------------------------------------+
| ID3D12Device  | ID3D12CommandQueue | IDXGISwapChain | Heaps      |
+------------------------------------------------------------------+
```

### Frame Lifecycle

```cpp
void RenderFrame() {
    // 1. Wait for previous frame
    WaitForPreviousFrame();

    // 2. Reset command allocator and list
    commandAllocator->Reset();
    commandList->Reset(commandAllocator, nullptr);

    // 3. Transition back buffer to render target
    ResourceBarrier(backBuffer, PRESENT -> RENDER_TARGET);

    // 4. Clear render target and depth buffer
    ClearRenderTargetView(rtvHandle, clearColor);
    ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f);

    // 5. Set render targets
    OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // 6. Render passes
    RenderTerrain();
    RenderVegetation();
    RenderCreatures();
    RenderUI();

    // 7. Transition back buffer to present
    ResourceBarrier(backBuffer, RENDER_TARGET -> PRESENT);

    // 8. Close and execute command list
    commandList->Close();
    commandQueue->ExecuteCommandLists(1, &commandList);

    // 9. Present
    swapChain->Present(1, 0);

    // 10. Signal fence
    SignalFence();
}
```

### Resource Management

#### Descriptor Heaps
| Heap Type | Size | Purpose |
|-----------|------|---------|
| CBV_SRV_UAV | 1024 | Constant buffers, textures, UAVs |
| RTV | 16 | Render target views |
| DSV | 4 | Depth stencil views |
| Sampler | 16 | Texture samplers |

#### Buffer Types
| Buffer | Usage | Update Frequency |
|--------|-------|------------------|
| Vertex Buffer | Mesh geometry | Static |
| Index Buffer | Triangle indices | Static |
| Instance Buffer | Per-instance data | Per-frame |
| Constant Buffer | Shader uniforms | Per-frame |

## Render Passes

### Pass 1: Terrain

```
Pipeline State:
├── Vertex Shader: terrain_vs.hlsl
├── Pixel Shader: terrain_ps.hlsl
├── Input Layout: Position, Normal, TexCoord
├── Primitive Topology: Triangle List
├── Rasterizer: Solid, Back-face cull
└── Depth: Enabled, Less-Equal
```

**Root Signature:**
```
[0] CBV - PerFrame (ViewProj, CameraPos, Time)
[1] CBV - PerObject (Model matrix)
[2] SRV - HeightMap (if used)
[3] Sampler - Linear wrap
```

### Pass 2: Vegetation

```
Pipeline State:
├── Vertex Shader: vegetation_vs.hlsl
├── Pixel Shader: vegetation_ps.hlsl
├── Input Layout: Position, Normal, TexCoord, InstanceData
├── Primitive Topology: Triangle List
├── Rasterizer: Solid, No cull (double-sided leaves)
└── Depth: Enabled, Less-Equal
```

### Pass 3: Creatures (Instanced)

```
Pipeline State:
├── Vertex Shader: creature_vs.hlsl
├── Pixel Shader: creature_ps.hlsl
├── Input Layout: Position, Normal + InstanceData
├── Primitive Topology: Triangle List
├── Rasterizer: Solid, Back-face cull
└── Depth: Enabled, Less-Equal
```

**Instancing Strategy:**
```cpp
struct InstanceData {
    float4x4 worldMatrix;    // 64 bytes
    float4   color;          // 16 bytes
    float    animationPhase; // 4 bytes
    float3   padding;        // 12 bytes
};                           // Total: 96 bytes per instance
```

### Pass 4: UI/Nametags

```
Pipeline State:
├── Vertex Shader: nametag_vs.hlsl
├── Pixel Shader: nametag_ps.hlsl
├── Primitive Topology: Triangle List
├── Rasterizer: Solid, No cull
├── Depth: Disabled (always on top)
└── Blend: Alpha blend enabled
```

## Shader Descriptions

### Terrain Shaders

#### Vertex Shader (terrain_vs.hlsl)
```hlsl
cbuffer PerFrame : register(b0) {
    float4x4 viewProj;
    float3 cameraPos;
    float time;
};

cbuffer PerObject : register(b1) {
    float4x4 model;
};

struct VSInput {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD0;
};

struct VSOutput {
    float4 position  : SV_POSITION;
    float3 worldPos  : TEXCOORD0;
    float3 normal    : TEXCOORD1;
    float2 texCoord  : TEXCOORD2;
    float  height    : TEXCOORD3;
};

VSOutput main(VSInput input) {
    VSOutput output;

    float4 worldPos = mul(model, float4(input.position, 1.0));

    // Water animation
    if (worldPos.y < WATER_LEVEL) {
        worldPos.y += sin(worldPos.x * 0.5 + time) * 0.3;
        worldPos.y += cos(worldPos.z * 0.5 + time) * 0.2;
    }

    output.position = mul(viewProj, worldPos);
    output.worldPos = worldPos.xyz;
    output.normal = mul((float3x3)model, input.normal);
    output.texCoord = input.texCoord;
    output.height = input.position.y;

    return output;
}
```

#### Pixel Shader (terrain_ps.hlsl)
```hlsl
// Procedural noise functions
float hash(float3 p);
float noise(float3 p);
float fbm(float3 p, int octaves);
float voronoi(float2 p);

// Biome determination by height
float3 getBiomeColor(float height, float3 worldPos) {
    // Water: 0-35%
    if (height < 0.35) {
        float depth = 0.35 - height;
        float3 shallow = float3(0.2, 0.5, 0.7);
        float3 deep = float3(0.1, 0.2, 0.4);
        return lerp(shallow, deep, depth * 3.0);
    }
    // Sand: 35-42%
    if (height < 0.42) {
        return float3(0.76, 0.7, 0.5) + noise(worldPos) * 0.1;
    }
    // Grass: 42-65%
    if (height < 0.65) {
        float n = fbm(worldPos * 2.0, 4);
        return float3(0.2, 0.5, 0.2) + float3(0.1, 0.2, 0.0) * n;
    }
    // Rock: 65-85%
    if (height < 0.85) {
        float v = voronoi(worldPos.xz * 0.5);
        return float3(0.4, 0.4, 0.45) + float3(0.1) * v;
    }
    // Snow: 85%+
    return float3(0.95, 0.95, 1.0);
}

float4 main(VSOutput input) : SV_TARGET {
    float3 biomeColor = getBiomeColor(input.height, input.worldPos);

    // Lighting
    float3 lightDir = normalize(float3(1.0, 1.0, 0.5));
    float3 normal = normalize(input.normal);

    float ambient = 0.4;
    float diffuse = max(dot(normal, lightDir), 0.0);
    float3 lighting = ambient + diffuse * 0.6;

    // Fog
    float dist = length(input.worldPos - cameraPos);
    float fog = 1.0 - exp(-dist * 0.002);
    float3 fogColor = float3(0.7, 0.8, 0.9);

    float3 finalColor = biomeColor * lighting;
    finalColor = lerp(finalColor, fogColor, fog);

    return float4(finalColor, 1.0);
}
```

### Creature Shaders

#### Vertex Shader (creature_vs.hlsl)
```hlsl
cbuffer PerFrame : register(b0) {
    float4x4 viewProj;
    float time;
};

struct VSInput {
    // Per-vertex
    float3 position : POSITION;
    float3 normal   : NORMAL;
    // Per-instance
    float4x4 world  : INSTANCE_WORLD;
    float4 color    : INSTANCE_COLOR;
    float animPhase : INSTANCE_ANIM;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal   : TEXCOORD1;
    float4 color    : TEXCOORD2;
};

VSOutput main(VSInput input) {
    VSOutput output;

    // Apply animation (simple bob)
    float3 animatedPos = input.position;
    animatedPos.y += sin(input.animPhase + time * 5.0) * 0.1;

    float4 worldPos = mul(input.world, float4(animatedPos, 1.0));

    output.position = mul(viewProj, worldPos);
    output.worldPos = worldPos.xyz;
    output.normal = mul((float3x3)input.world, input.normal);
    output.color = input.color;

    return output;
}
```

#### Pixel Shader (creature_ps.hlsl)
```hlsl
cbuffer PerFrame : register(b0) {
    float3 lightDir;
    float3 cameraPos;
};

float4 main(VSOutput input) : SV_TARGET {
    float3 normal = normalize(input.normal);
    float3 viewDir = normalize(cameraPos - input.worldPos);

    // Diffuse
    float diffuse = max(dot(normal, lightDir), 0.0);

    // Specular (Blinn-Phong)
    float3 halfVec = normalize(lightDir + viewDir);
    float specular = pow(max(dot(normal, halfVec), 0.0), 32.0);

    // Combine
    float3 ambient = 0.3 * input.color.rgb;
    float3 diff = 0.6 * diffuse * input.color.rgb;
    float3 spec = 0.2 * specular * float3(1.0);

    return float4(ambient + diff + spec, input.color.a);
}
```

## Constant Buffer Layout

### PerFrame Constants (b0)
```hlsl
cbuffer PerFrame : register(b0) {
    float4x4 view;           // 64 bytes, offset 0
    float4x4 projection;     // 64 bytes, offset 64
    float4x4 viewProj;       // 64 bytes, offset 128
    float3   cameraPos;      // 12 bytes, offset 192
    float    time;           // 4 bytes,  offset 204
    float3   lightDir;       // 12 bytes, offset 208
    float    padding;        // 4 bytes,  offset 220
};                           // Total: 224 bytes (aligned to 256)
```

### PerObject Constants (b1)
```hlsl
cbuffer PerObject : register(b1) {
    float4x4 model;          // 64 bytes, offset 0
    float4x4 normalMatrix;   // 64 bytes, offset 64
};                           // Total: 128 bytes (aligned to 256)
```

### Material Constants (b2)
```hlsl
cbuffer Material : register(b2) {
    float3 albedo;           // 12 bytes, offset 0
    float  roughness;        // 4 bytes,  offset 12
    float  metallic;         // 4 bytes,  offset 16
    float3 emissive;         // 12 bytes, offset 20
};                           // Total: 32 bytes (aligned to 256)
```

## Vertex Format

### Standard Vertex (32 bytes)
```cpp
struct Vertex {
    float3 position;   // 12 bytes, offset 0
    float3 normal;     // 12 bytes, offset 12
    float2 texCoord;   // 8 bytes,  offset 24
};                     // Total: 32 bytes
```

**Input Layout:**
```cpp
D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
};
```

### Instance Data (96 bytes)
```cpp
struct InstanceData {
    float4x4 worldMatrix;    // 64 bytes, offset 0
    float4   color;          // 16 bytes, offset 64
    float    animationPhase; // 4 bytes,  offset 80
    float3   padding;        // 12 bytes, offset 84
};                           // Total: 96 bytes
```

**Input Layout (additional):**
```cpp
D3D12_INPUT_ELEMENT_DESC instanceLayout[] = {
    // ... vertex elements ...
    {"INSTANCE_WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,
        D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
    {"INSTANCE_WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16,
        D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
    {"INSTANCE_WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32,
        D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
    {"INSTANCE_WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48,
        D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
    {"INSTANCE_COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64,
        D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
    {"INSTANCE_ANIM", 0, DXGI_FORMAT_R32_FLOAT, 1, 80,
        D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
};
```

## Render Settings

### Default Configuration
| Setting | Value |
|---------|-------|
| Resolution | 1280 x 720 |
| MSAA | 4x |
| VSync | Enabled (1) |
| Back Buffers | 2 (double buffering) |
| Depth Format | D32_FLOAT |
| Render Target Format | R8G8B8A8_UNORM |

### Projection Settings
| Parameter | Value |
|-----------|-------|
| FOV | 45 degrees |
| Near Plane | 0.1 |
| Far Plane | 1000.0 |
| Aspect Ratio | 16:9 (1.777) |

### Lighting Parameters
| Parameter | Value |
|-----------|-------|
| Light Direction | normalize(1, 1, 0.5) |
| Ambient Strength | 0.4 |
| Diffuse Strength | 0.6 |
| Specular Strength | 0.2 |
| Shininess | 32 |

### Fog Settings
| Parameter | Value |
|-----------|-------|
| Fog Density | 0.002 |
| Fog Color | (0.7, 0.8, 0.9) |
| Type | Exponential |

## Performance Considerations

### Batching Strategy
- Creatures grouped by mesh type
- Single draw call per mesh variant
- Instance buffer updated per frame

### Memory Layout
- Vertex buffers: DEFAULT heap (GPU-only)
- Instance buffers: UPLOAD heap (CPU-writable)
- Constant buffers: UPLOAD heap, 256-byte aligned

### Synchronization
- Fence per frame for CPU-GPU sync
- Triple buffering for constant buffers
- Resource barriers for state transitions
