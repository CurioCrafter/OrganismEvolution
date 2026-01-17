# 10 Comprehensive Claude Agent Prompts

Each prompt is designed for significant, multi-hour autonomous work. Copy the entire prompt block into a new Claude session.

---

## Agent 1: Complete OpenGL to DirectX 12 Migration (MASSIVE)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution

MISSION: Complete the migration from OpenGL to DirectX 12. The DX12 entry point is `src/main.cpp` (Forge RHI). Legacy OpenGL sources are kept under `deprecated/opengl` and `deprecated/opengl/legacy_src`. Your job is to keep the DX12 build fully working and remove remaining OpenGL dependencies from active codepaths.

## PHASE 1: Understand Current State (1+ hour)

Read and understand these files thoroughly:
- src/main.cpp (DX12 main implementation using Forge RHI - study this deeply)
- src/graphics/DX12Device.h and .cpp
- src/graphics/Shader_DX12.h
- src/graphics/rendering/CreatureRenderer_DX12.h and .cpp
- src/graphics/ShadowMap_DX12.h and .cpp
- shaders/hlsl/*.hlsl (all 5 HLSL shaders)

Document what's implemented vs what's missing by comparing to:
- deprecated/opengl/main.cpp (legacy OpenGL reference)
- deprecated/opengl/legacy_src/core/Simulation.cpp (legacy OpenGL simulation logic)
- src/entities/Creature.cpp (creature behavior)

## PHASE 2: Create CMakeLists_DX12.txt (30 minutes)

Create a complete CMakeLists.txt for DirectX 12:
- Target Windows only (no cross-platform needed)
- Link: d3d12.lib, dxgi.lib, d3dcompiler.lib, dxguid.lib
- Link: user32.lib, gdi32.lib, shell32.lib
- Compile HLSL shaders to CSO during build
- Include proper ImGui DX12/Win32 backends

Remove all references to:
- OpenGL, GLFW, GLEW, GLM
- Any cross-platform conditionals

## PHASE 3: Port Missing Systems (3+ hours)

Port these OpenGL systems to DX12:

1. **Terrain Rendering** (src/environment/Terrain.cpp)
   - Generate terrain mesh with DX12 buffers
   - Upload to GPU with proper barriers
   - Create PSO for terrain shader

2. **Vegetation System** (src/environment/VegetationManager.cpp)
   - Instance buffer for trees/plants
   - Billboard rendering for distant vegetation
   - Use Vegetation.hlsl shader

3. **Dashboard UI** (deprecated/opengl/legacy_src/ui/DashboardUI.cpp)
   - Port to ImGui DX12 backend
   - Descriptor heap for ImGui textures
   - Proper frame timing display

4. **Screenshot System** (deprecated/opengl/legacy_src/utils/Screenshot.cpp)
   - DX12 render target readback
   - Proper staging buffer usage
   - PNG encoding (or BMP for simplicity)

## PHASE 4: Integration Testing (1+ hour)

Create a minimal test that:
1. Opens a Win32 window (800x600)
2. Initializes DX12 device, swapchain, command queue
3. Renders terrain with proper biome colors
4. Spawns 20 creatures (10 herbivore, 10 carnivore)
5. Camera controls: WASD + mouse look
6. Shows FPS counter

Debug checklist:
- [ ] Terrain renders with visible biomes
- [ ] Creatures render as colored shapes on terrain
- [ ] Camera moves smoothly
- [ ] No validation errors in debug layer
- [ ] Stable 60 FPS

## PHASE 5: Cleanup (30 minutes)

1. Delete or move to `deprecated/` folder:
   - deprecated/opengl/legacy_src/platform/Win32Window.* (legacy Win32 + WGL)
   - deprecated/opengl/main.cpp (legacy OpenGL reference)
   - deprecated/opengl/main_enhanced.cpp
   - deprecated/opengl/Shader.cpp/h (OpenGL)
   - deprecated/opengl/ShadowMap.cpp/h (OpenGL)
   - deprecated/opengl/legacy_src/graphics/rendering/CreatureRenderer.cpp/h (OpenGL)
      - deprecated/opengl/GLFWContext.h

2. Update all #includes that reference deleted files
3. Confirm src/main.cpp is the DX12 entry point
4. Update CMakeLists.txt to be the DX12 version

## DELIVERABLES

- [ ] Working CMakeLists_DX12.txt (or updated CMakeLists.txt)
- [ ] All systems ported to DX12
- [ ] OpenGL files removed/deprecated
- [ ] Clean build with no errors
- [ ] Application runs and shows creatures on terrain

## REFERENCE FILES

DX12 implementation reference:
- C:\Users\andre\Desktop\ModularGameEngine\SimulationDemo\main.cpp (153KB reference)
- C:\Users\andre\Desktop\ModularGameEngine\Runtime\Shaders\*.hlsl

You have FULL AUTHORITY to:
- Create new files
- Delete obsolete files
- Refactor as needed
- Make architectural decisions

This is a COMPLETE REWRITE task. Don't try to preserve OpenGL compatibility.
```

---

## Agent 2: Implement Full Genetics System Integration

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution

MISSION: The project has a sophisticated genetics system in src/entities/genetics/ that is partially integrated. Your job is to FULLY integrate it with creature behavior, reproduction, and evolution.

## CURRENT STATE ANALYSIS (1 hour)

Read and understand:
- src/entities/genetics/Allele.h/cpp - Individual gene variants
- src/entities/genetics/Gene.h/cpp - Gene with dominant/recessive alleles
- src/entities/genetics/Chromosome.h/cpp - Collection of genes
- src/entities/genetics/DiploidGenome.h/cpp - Full diploid genome (maternal + paternal)
- src/entities/genetics/MateSelector.h/cpp - Partner selection logic
- src/entities/genetics/Species.h/cpp - Species definitions
- src/entities/genetics/HybridZone.h/cpp - Hybridization between species
- src/entities/genetics/GeneticsManager.h/cpp - Orchestration

Compare to current creature genetics:
- src/entities/Genome.h/cpp - Simple genome (what creatures currently use)
- src/entities/Creature.h/cpp - How creatures reproduce

## INTEGRATION TASKS (3+ hours)

### Task 1: Replace Simple Genome with DiploidGenome

Current Genome.h has flat traits:
```cpp
float speed, size, senseRadius, metabolism, aggression;
```

Replace with DiploidGenome that provides:
- Mendelian inheritance (dominant/recessive)
- Crossover during meiosis
- Mutation at specific gene loci
- Heterozygote advantage calculations

Update Creature class to:
- Store DiploidGenome instead of Genome
- Calculate phenotype from genotype (gene expression)
- Pass genetics to offspring properly

### Task 2: Implement Sexual Selection

Use MateSelector to:
- Score potential mates by genetic compatibility
- Implement assortative mating (similar creatures prefer each other)
- Add courtship behavior (display, chase, acceptance)
- Calculate hybrid fitness reduction

Add mating rituals:
- Pre-copulatory displays (visual/behavioral)
- Female choice based on male traits
- Male-male competition

### Task 3: Speciation Mechanics

Use Species and HybridZone to:
- Track genetic distance between populations
- Detect when populations diverge into new species
- Create hybrid zones where species meet
- Calculate reproductive isolation

Implement:
- Allopatric speciation (geographic isolation)
- Sympatric speciation (ecological niche differentiation)
- Ring species formation

### Task 4: Genetic Diseases and Fitness

Add:
- Deleterious recessive alleles that reduce fitness when homozygous
- Genetic load tracking for populations
- Inbreeding depression calculations
- Heterozygote advantage (overdominance) for some traits

### Task 5: Visualization

Add debug visualization for:
- Gene frequency charts per population
- Hardy-Weinberg equilibrium tracking
- Phylogenetic tree of species
- Genetic diversity metrics

## TESTING

Create tests that verify:
1. Two identical parents produce similar offspring
2. Recessive traits appear in 1/4 of offspring from heterozygotes
3. Mutation rate produces expected genetic diversity
4. Populations diverge over 100+ generations
5. Hybrid fitness penalty works correctly

## DELIVERABLES

- [ ] Creature uses DiploidGenome instead of Genome
- [ ] MateSelector integrated into reproduction
- [ ] Species tracking implemented
- [ ] HybridZone mechanics working
- [ ] Genetic visualization in debug UI
- [ ] 5+ unit tests passing
- [ ] Documentation of genetics system

## FILES TO MODIFY

Primary:
- src/entities/Creature.h/cpp
- src/core/Simulation.cpp

Integrate:
- All files in src/entities/genetics/

Create:
- src/ui/GeneticsPanel.h/cpp (visualization)
- tests/genetics_tests.cpp
```

---

## Agent 3: GPU Compute Shader AI System

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution

MISSION: Implement a GPU compute shader system that runs creature AI (neural networks + steering behaviors) entirely on the GPU, enabling 10,000+ creatures at 60 FPS.

## CURRENT STATE

- src/ai/GPUSteeringCompute.h/cpp - Partial implementation exists
- shaders/hlsl/SteeringCompute.hlsl - Compute shader exists (674 lines)
- src/entities/NeuralNetwork.cpp - CPU neural network
- src/entities/SteeringBehaviors.cpp - CPU steering

## ARCHITECTURE DESIGN (1 hour)

Design a GPU-native simulation where:
1. All creature state lives in GPU buffers
2. Compute shader runs neural network forward pass
3. Compute shader calculates steering forces
4. Compute shader integrates position/velocity
5. CPU only handles:
   - Spawning (upload new creature)
   - Death (mark inactive)
   - Reproduction (spawn offspring)
   - User interaction

## DATA LAYOUT (Structure of Arrays)

```cpp
// GPU Buffers (all creatures, contiguous)
struct CreatureGPUBuffers {
    // Transform data (read/write by compute, read by vertex)
    Buffer<float3> positions;      // World position
    Buffer<float3> velocities;     // Movement velocity
    Buffer<float4> rotations;      // Quaternion orientation

    // Neural network weights (read-only after spawn)
    Buffer<float> weightsIH;       // INPUT_SIZE * HIDDEN_SIZE per creature
    Buffer<float> weightsHO;       // HIDDEN_SIZE * OUTPUT_SIZE per creature
    Buffer<float> biases;          // HIDDEN_SIZE + OUTPUT_SIZE per creature

    // State data (read/write)
    Buffer<float> energy;          // Current energy level
    Buffer<float> age;             // Creature age
    Buffer<uint> flags;            // isAlive, type, etc. (bitfield)

    // Sensory inputs (computed each frame)
    Buffer<float> sensorInputs;    // INPUT_SIZE per creature

    // Debug outputs
    Buffer<float> debugOutputs;    // For visualization
};
```

## IMPLEMENTATION TASKS (4+ hours)

### Task 1: Create GPU Buffer Manager

```cpp
class GPUCreatureSystem {
    static constexpr int MAX_CREATURES = 65536;  // 2^16

    // DX12 resources
    ComPtr<ID3D12Resource> positionBuffer;
    ComPtr<ID3D12Resource> velocityBuffer;
    // ... etc

    void Initialize(ID3D12Device* device);
    void SpawnCreature(const CreatureData& data, int slot);
    void KillCreature(int slot);
    void DispatchCompute(ID3D12GraphicsCommandList* cmdList);
    void ReadbackDebug();
};
```

### Task 2: Update SteeringCompute.hlsl

The existing shader has good structure. Enhance it:
- Full neural network forward pass (currently has inputs defined)
- All steering behaviors (seek, flee, wander, arrive, pursuit, evasion, separation, alignment, cohesion)
- Terrain collision (sample terrain height, clamp Y position)
- Predator-prey interaction (carnivores hunt herbivores)
- Energy consumption/regeneration

### Task 3: Double-Buffered State

For consistent reads while computing:
```
Frame N:   Read buffer A → Compute → Write buffer B
Frame N+1: Read buffer B → Compute → Write buffer A
```

Implement ping-pong buffers with proper barriers.

### Task 4: Spatial Hashing on GPU

For neighbor queries (separation/alignment/cohesion), implement:
- 3D grid hash function
- Atomic increment for cell counts
- Prefix sum for compacted neighbor list
- OR: Use fixed-radius neighbor search with grid

### Task 5: CPU-GPU Synchronization

Handle these edge cases:
- Creature spawns mid-frame
- Creature dies mid-frame
- User selects creature (need data readback)
- Reproduction (upload offspring weights)

Use staging buffers and fences properly.

## PERFORMANCE TARGETS

- 10,000 creatures: 60 FPS
- 50,000 creatures: 30 FPS
- 100,000 creatures: 15 FPS (acceptable for screenshots)

## PROFILING

Add PIX markers for:
- Neural network compute time
- Steering behavior compute time
- Position integration time
- Buffer transitions

## DELIVERABLES

- [ ] GPUCreatureSystem class fully implemented
- [ ] SteeringCompute.hlsl enhanced with full behaviors
- [ ] Double-buffered state management
- [ ] Spawn/kill working without stalls
- [ ] 10,000 creatures at 60 FPS demonstrated
- [ ] PIX capture showing GPU timeline
- [ ] Documentation of data layout

## REFERENCE

- shaders/hlsl/SteeringCompute.hlsl (existing, 674 lines)
- src/ai/GPUSteeringCompute.h/cpp (existing, partial)
- Microsoft DirectX 12 Compute Shader samples
```

---

## Agent 4: Procedural Terrain System Rewrite

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution

MISSION: Rewrite the terrain system to be GPU-accelerated with proper LOD, terrain features, and visual quality matching AAA games.

## CURRENT STATE

- src/environment/Terrain.cpp - CPU-generated heightmap with Perlin noise
- shaders/hlsl/Terrain.hlsl - Good shader with biomes (714 lines)
- Terrain is flat-shaded, no LOD, single mesh

## NEW ARCHITECTURE

### Chunked Terrain with LOD

```
World: 2048x2048 units
Chunk: 64x64 units (32x32 = 1024 chunks total)
LOD levels: 4 (full detail, half, quarter, eighth)

LOD selection:
- LOD 0: < 64 units from camera (64x64 vertices)
- LOD 1: 64-128 units (32x32 vertices)
- LOD 2: 128-256 units (16x16 vertices)
- LOD 3: > 256 units (8x8 vertices)
```

### GPU Terrain Generation

Move terrain generation to compute shader:
```hlsl
// TerrainGenerate.hlsl
[numthreads(8, 8, 1)]
void GenerateHeightmap(uint3 id : SV_DispatchThreadID)
{
    float2 worldPos = ChunkOrigin + id.xy * CellSize;

    // Multi-octave Perlin noise
    float height = 0.0f;
    float amplitude = 1.0f;
    float frequency = 0.005f;

    for (int i = 0; i < 6; i++)
    {
        height += amplitude * perlinNoise(worldPos * frequency);
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }

    // Terrain features
    height = applyRidges(height, worldPos);
    height = applyValleys(height, worldPos);
    height = applyErosion(height, worldPos);

    HeightmapOut[id.xy] = height;
}
```

### Terrain Features

Implement:
1. **Hydraulic erosion** - Rivers carve valleys
2. **Thermal erosion** - Cliff face weathering
3. **Ridge noise** - Mountain ridges using absolute value Perlin
4. **Voronoi-based features** - Plateaus, craters
5. **Domain warping** - Natural-looking distortion

### Water System

Upgrade from simple threshold to:
- Water depth buffer
- Shoreline foam calculation
- Underwater caustics
- Wave normal mapping
- Reflection/refraction (screen-space or planar)

### Vegetation Placement

Generate vegetation positions in compute shader:
- Use Poisson disk sampling for natural distribution
- Density varies by biome
- Trees on slopes respect gravity
- No vegetation in water or on cliffs

## IMPLEMENTATION TASKS (4+ hours)

### Task 1: Chunk System

```cpp
class TerrainChunkManager {
    struct Chunk {
        Vec2 worldPos;
        int lodLevel;
        ComPtr<ID3D12Resource> vertexBuffer;
        ComPtr<ID3D12Resource> indexBuffer;
        bool dirty;
    };

    std::unordered_map<uint64_t, Chunk> chunks;

    void UpdateVisibleChunks(const Camera& camera);
    void GenerateChunk(int chunkX, int chunkZ, int lod);
    void RenderChunks(ID3D12GraphicsCommandList* cmd);
};
```

### Task 2: LOD Stitching

Handle seams between LOD levels:
- Skirt geometry method (drop edges below terrain)
- OR: T-junction elimination (merge vertices at boundary)
- OR: Geomorphing (smooth transition between LODs)

### Task 3: Normal Calculation

Compute normals in shader:
```hlsl
float3 CalculateNormal(float2 uv, Texture2D heightmap)
{
    float left = heightmap.Sample(sampler, uv + float2(-texelSize, 0)).r;
    float right = heightmap.Sample(sampler, uv + float2(texelSize, 0)).r;
    float up = heightmap.Sample(sampler, uv + float2(0, -texelSize)).r;
    float down = heightmap.Sample(sampler, uv + float2(0, texelSize)).r;

    return normalize(float3(left - right, 2.0f * texelSize, up - down));
}
```

### Task 4: Biome Blending

Enhance Terrain.hlsl:
- Triplanar texture projection (no stretching on cliffs)
- Detail textures (high frequency noise)
- Macro variation (large-scale color changes)
- Slope-based material selection

### Task 5: Performance Optimization

- Frustum cull chunks on CPU
- Occlusion culling for chunks behind mountains
- Async chunk generation (background thread)
- Texture streaming for detailed chunks

## DELIVERABLES

- [ ] Chunked terrain with LOD working
- [ ] GPU heightmap generation
- [ ] Erosion and terrain features
- [ ] Enhanced water rendering
- [ ] Vegetation placement system
- [ ] No visible LOD popping or seams
- [ ] Performance: 60 FPS with full terrain visible
- [ ] Documentation of terrain system

## VISUAL TARGETS

The terrain should look like:
- Horizon Zero Dawn (biome diversity)
- The Witcher 3 (vegetation density)
- Red Dead Redemption 2 (terrain detail)

Not acceptable:
- Minecraft (blocky, uniform)
- Basic demo terrain (single texture, flat)
```

---

## Agent 5: Neural Network Evolution System

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution

MISSION: Implement a sophisticated neural network evolution system using NEAT (NeuroEvolution of Augmenting Topologies) that allows creatures to develop complex behaviors over many generations.

## CURRENT STATE

- src/entities/NeuralNetwork.h/cpp - Simple fixed-topology feedforward network
- src/ai/NeuralNetwork.h/cpp - Duplicate (cleanup needed)
- src/ai/NEATGenome.h/cpp - NEAT implementation exists (partial)
- src/ai/BrainModules.h/cpp - Modular brain components

## NEAT IMPLEMENTATION (4+ hours)

### Core NEAT Concepts

1. **Genes represent connections**, not weights
2. **Innovation numbers** track structural changes
3. **Speciation** protects innovation
4. **Crossover** aligns by innovation number
5. **Mutation** adds nodes/connections incrementally

### NEATGenome Structure

```cpp
struct ConnectionGene {
    int innovationNumber;   // Global innovation tracker
    int inputNode;
    int outputNode;
    float weight;
    bool enabled;
};

struct NodeGene {
    int nodeId;
    NodeType type;  // INPUT, HIDDEN, OUTPUT
    float bias;
    ActivationFunction activation;  // SIGMOID, TANH, RELU, STEP
};

class NEATGenome {
    std::vector<NodeGene> nodes;
    std::vector<ConnectionGene> connections;
    float fitness;
    int speciesId;

    static int globalInnovation;

    void MutateWeight(float rate = 0.8f);
    void MutateAddConnection();
    void MutateAddNode();
    void MutateToggleConnection();
    void MutateActivation();

    NEATGenome Crossover(const NEATGenome& other) const;
    float CompatibilityDistance(const NEATGenome& other) const;
    std::vector<float> Forward(const std::vector<float>& inputs);
};
```

### Task 1: Implement Full NEAT Algorithm

Following Kenneth Stanley's original paper:
1. Initialize population with minimal structure (inputs directly connected to outputs)
2. Evaluate fitness (survival time, offspring count, energy efficiency)
3. Speciate population by compatibility distance
4. Reproduction within species (best produce offspring)
5. Crossover between species champions
6. Mutation (structural and weight)
7. Replace worst performers

### Task 2: Innovation Number System

```cpp
class InnovationTracker {
    struct Innovation {
        int inputNode;
        int outputNode;
        int innovationNumber;
    };

    std::vector<Innovation> currentGeneration;
    int nextInnovation = 1;

    int GetInnovation(int input, int output);
    void NextGeneration();
};
```

This ensures same structural mutations get same innovation number.

### Task 3: Speciation

```cpp
class Species {
    int id;
    NEATGenome representative;
    std::vector<NEATGenome*> members;
    float avgFitness;
    int staleness;  // Generations without improvement

    bool Compatible(const NEATGenome& genome, float threshold = 3.0f);
    void AdjustFitness();  // Explicit fitness sharing
    NEATGenome Reproduce();
};
```

### Task 4: Phenotype Decoding

Convert NEAT genome to runnable network:
```cpp
class NEATNetwork {
    std::vector<std::vector<int>> layers;  // Topologically sorted

    void BuildFromGenome(const NEATGenome& genome);
    std::vector<float> Forward(const std::vector<float>& inputs);
};
```

Handle:
- Recurrent connections (optional)
- Disabled genes (skip)
- Multiple paths to same output

### Task 5: Fitness Functions

Implement multiple fitness metrics:
- **Survival time** - How long creature lives
- **Energy efficiency** - Energy gained / energy spent
- **Offspring count** - Reproductive success
- **Territory control** - Area dominated
- **Prey caught** (carnivores)
- **Grazing efficiency** (herbivores)

### Task 6: Visualization

- Network topology viewer (nodes, connections)
- Species population chart
- Innovation history
- Champion replay

## INTEGRATION

Replace current NeuralNetwork with NEAT:
```cpp
class Creature {
    // OLD: NeuralNetwork brain;
    NEATGenome genome;
    NEATNetwork brain;  // Decoded from genome

    void Think() {
        auto inputs = GatherSensorInputs();
        auto outputs = brain.Forward(inputs);
        ApplyBehavior(outputs);
    }

    Creature Reproduce(const Creature& mate) {
        NEATGenome childGenome = genome.Crossover(mate.genome);
        childGenome.Mutate();
        return Creature(childGenome);
    }
};
```

## DELIVERABLES

- [ ] Full NEAT implementation in src/ai/
- [ ] Innovation number tracking
- [ ] Speciation with fitness sharing
- [ ] Network topology visualization
- [ ] Fitness metrics dashboard
- [ ] 100+ generations without stagnation
- [ ] Observable behavior evolution (creatures get smarter)
- [ ] Documentation of NEAT parameters

## REFERENCES

- Original NEAT paper: http://nn.cs.utexas.edu/downloads/papers/stanley.ec02.pdf
- NEAT C++ implementation: https://github.com/FernandoTorres/NEAT
- HyperNEAT extension (optional): https://eplex.cs.ucf.edu/hyperNEATpage/
```

---

## Agent 6: Advanced Rendering Features

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution

MISSION: Implement advanced rendering features to make the simulation visually stunning - shadows, reflections, ambient occlusion, volumetric effects.

## CURRENT STATE

- shaders/hlsl/Terrain.hlsl - Good biome system, basic lighting
- shaders/hlsl/Creature.hlsl - Instanced rendering, subsurface scattering
- src/graphics/ShadowMap_DX12.h/cpp - Shadow mapping (partial)

## RENDERING FEATURES (5+ hours)

### Feature 1: Cascaded Shadow Maps (CSM)

For large terrain, single shadow map isn't enough:
```cpp
struct CascadedShadowMap {
    static constexpr int CASCADE_COUNT = 4;

    float cascadeSplits[CASCADE_COUNT] = {10.0f, 30.0f, 100.0f, 500.0f};
    Mat4 lightViewProj[CASCADE_COUNT];
    ComPtr<ID3D12Resource> shadowMaps[CASCADE_COUNT];  // 2048x2048 each

    void UpdateCascades(const Camera& camera, const Vec3& lightDir);
    void RenderCascade(int cascade, ID3D12GraphicsCommandList* cmd);
};
```

Shader:
```hlsl
float SampleShadowCSM(float3 worldPos, float viewDepth)
{
    int cascade = 0;
    for (int i = 0; i < 4; i++) {
        if (viewDepth < cascadeSplits[i]) {
            cascade = i;
            break;
        }
    }

    float4 shadowCoord = mul(float4(worldPos, 1.0f), lightViewProj[cascade]);
    shadowCoord.xyz /= shadowCoord.w;
    shadowCoord.xy = shadowCoord.xy * 0.5f + 0.5f;
    shadowCoord.y = 1.0f - shadowCoord.y;

    return PCFFilter(shadowMaps[cascade], shadowCoord.xyz);
}
```

### Feature 2: Screen-Space Ambient Occlusion (SSAO)

```hlsl
// SSAO.hlsl
float ComputeSSAO(float2 uv, float depth, float3 normal)
{
    float occlusion = 0.0f;

    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        float3 sampleOffset = hemisphereKernel[i];
        float3 samplePos = viewPos + sampleOffset * RADIUS;

        float4 projPos = mul(float4(samplePos, 1.0f), projection);
        float2 sampleUV = projPos.xy / projPos.w * 0.5f + 0.5f;

        float sampleDepth = depthTexture.Sample(sampler, sampleUV).r;
        float rangeCheck = smoothstep(0.0f, 1.0f, RADIUS / abs(depth - sampleDepth));

        occlusion += (sampleDepth >= samplePos.z + 0.025f ? 1.0f : 0.0f) * rangeCheck;
    }

    return 1.0f - (occlusion / SAMPLE_COUNT);
}
```

### Feature 3: Screen-Space Reflections (SSR)

For water and wet surfaces:
```hlsl
float3 ScreenSpaceReflection(float2 uv, float3 viewPos, float3 normal)
{
    float3 viewDir = normalize(viewPos);
    float3 reflectDir = reflect(viewDir, normal);

    // Ray march in screen space
    float4 startPos = mul(float4(viewPos, 1.0f), projection);
    float3 endPos = viewPos + reflectDir * MAX_DISTANCE;
    float4 endPosProj = mul(float4(endPos, 1.0f), projection);

    // Binary search refinement
    for (int i = 0; i < MARCH_STEPS; i++)
    {
        // ...
    }

    return hitColor;
}
```

### Feature 4: Volumetric Fog/God Rays

```hlsl
float3 VolumetricScattering(float3 rayStart, float3 rayEnd, float3 lightPos)
{
    float3 accumFog = 0.0f;
    float stepSize = length(rayEnd - rayStart) / STEPS;

    for (int i = 0; i < STEPS; i++)
    {
        float3 samplePos = lerp(rayStart, rayEnd, (float)i / STEPS);

        // Shadow test
        float inShadow = SampleShadow(samplePos);

        // Mie scattering
        float cosAngle = dot(normalize(samplePos - camPos), normalize(lightPos - samplePos));
        float mie = MiePhase(cosAngle, 0.76f);

        accumFog += inShadow * mie * FOG_DENSITY * stepSize;
    }

    return accumFog * lightColor;
}
```

### Feature 5: Bloom

```hlsl
// Bloom extraction
float4 BloomExtract(float2 uv)
{
    float4 color = sceneTexture.Sample(sampler, uv);
    float brightness = dot(color.rgb, float3(0.2126f, 0.7152f, 0.0722f));
    return brightness > BLOOM_THRESHOLD ? color : 0.0f;
}

// Gaussian blur passes
// Additive composite with scene
```

### Feature 6: Tone Mapping and Color Grading

```hlsl
float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float3 ColorGrade(float3 color)
{
    // Lift, gamma, gain
    color = pow(color, 1.0f / gamma);
    color = color * gain + lift;

    // Saturation
    float luminance = dot(color, float3(0.299f, 0.587f, 0.114f));
    color = lerp(luminance.xxx, color, saturation);

    return color;
}
```

## IMPLEMENTATION ORDER

1. CSM shadows (most impactful)
2. SSAO (adds depth)
3. Bloom (makes lighting pop)
4. Tone mapping (HDR look)
5. SSR (water quality)
6. Volumetric fog (atmosphere)

## DELIVERABLES

- [ ] Cascaded shadow maps (4 cascades)
- [ ] SSAO with bilateral blur
- [ ] Bloom with threshold control
- [ ] ACES tone mapping
- [ ] SSR for water
- [ ] Volumetric god rays
- [ ] All effects toggleable via ImGui
- [ ] Performance: 60 FPS with all effects

## PERFORMANCE BUDGET

At 1080p, 60 FPS (16.67ms):
- Shadow pass: 2ms
- Main pass: 4ms
- SSAO: 1.5ms
- SSR: 2ms
- Bloom: 0.5ms
- Fog: 1ms
- Post-process: 0.5ms
- UI: 0.5ms
- Headroom: 4ms
```

---

## Agent 7: Complete UI/Dashboard System

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution

MISSION: Create a comprehensive debug and gameplay UI using ImGui that provides full control over the simulation and visualization of all systems.

## CURRENT STATE

- src/ui/DashboardMetrics.h/cpp - Metrics collection
- src/ui/DashboardUI.h/cpp - Basic UI (OpenGL-based)
- src/ui/ImGuiManager.h/cpp - DX12 ImGui integration

## UI REQUIREMENTS (4+ hours)

### Panel 1: Simulation Control

```cpp
void RenderSimulationPanel()
{
    ImGui::Begin("Simulation Control");

    // Playback controls
    if (ImGui::Button(isPaused ? "Play" : "Pause")) TogglePause();
    ImGui::SameLine();
    if (ImGui::Button("Step")) SingleStep();
    ImGui::SameLine();
    ImGui::SliderFloat("Speed", &timeScale, 0.1f, 10.0f);

    // Population controls
    ImGui::Separator();
    ImGui::Text("Population");
    ImGui::SliderInt("Max Creatures", &maxCreatures, 10, 10000);
    if (ImGui::Button("Spawn Herbivore")) SpawnHerbivore();
    ImGui::SameLine();
    if (ImGui::Button("Spawn Carnivore")) SpawnCarnivore();
    if (ImGui::Button("Mass Extinction")) KillAllCreatures();

    // Environment
    ImGui::Separator();
    ImGui::Text("Environment");
    ImGui::SliderFloat("Food Spawn Rate", &foodSpawnRate, 0.0f, 20.0f);
    ImGui::SliderFloat("Day/Night Speed", &dayNightSpeed, 0.0f, 10.0f);

    ImGui::End();
}
```

### Panel 2: Statistics Dashboard

```cpp
void RenderStatisticsPanel()
{
    ImGui::Begin("Statistics");

    // Real-time graphs
    ImGui::PlotLines("Population", populationHistory.data(),
                     populationHistory.size(), 0, nullptr, 0, maxPop,
                     ImVec2(300, 100));

    ImGui::PlotLines("Avg Fitness", fitnessHistory.data(),
                     fitnessHistory.size(), 0, nullptr, 0, 1.0f,
                     ImVec2(300, 50));

    // Current stats table
    ImGui::Columns(2);
    ImGui::Text("Herbivores"); ImGui::NextColumn();
    ImGui::Text("%d", herbivoreCount); ImGui::NextColumn();
    ImGui::Text("Carnivores"); ImGui::NextColumn();
    ImGui::Text("%d", carnivoreCount); ImGui::NextColumn();
    ImGui::Text("Generation"); ImGui::NextColumn();
    ImGui::Text("%d", generation); ImGui::NextColumn();
    ImGui::Text("Total Deaths"); ImGui::NextColumn();
    ImGui::Text("%d", totalDeaths); ImGui::NextColumn();
    ImGui::Columns(1);

    // Species list
    if (ImGui::CollapsingHeader("Species"))
    {
        for (auto& species : speciesList)
        {
            ImGui::Text("Species %d: %d members, %.2f avg fitness",
                        species.id, species.memberCount, species.avgFitness);
        }
    }

    ImGui::End();
}
```

### Panel 3: Selected Creature Inspector

```cpp
void RenderCreatureInspector()
{
    if (!selectedCreature) return;

    ImGui::Begin("Creature Inspector");

    // Basic info
    ImGui::Text("ID: %d", selectedCreature->id);
    ImGui::Text("Type: %s", selectedCreature->type == HERBIVORE ? "Herbivore" : "Carnivore");
    ImGui::Text("Age: %.1f", selectedCreature->age);
    ImGui::ProgressBar(selectedCreature->energy / 200.0f, ImVec2(-1, 0), "Energy");

    // Genome traits
    if (ImGui::CollapsingHeader("Genome", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Speed: %.2f", selectedCreature->genome.speed);
        ImGui::Text("Size: %.2f", selectedCreature->genome.size);
        ImGui::Text("Sense Radius: %.1f", selectedCreature->genome.senseRadius);
        ImGui::Text("Metabolism: %.2f", selectedCreature->genome.metabolism);
    }

    // Neural network visualization
    if (ImGui::CollapsingHeader("Brain"))
    {
        RenderNeuralNetworkGraph(selectedCreature->brain);
    }

    // Lineage
    if (ImGui::CollapsingHeader("Lineage"))
    {
        ImGui::Text("Generation: %d", selectedCreature->generation);
        ImGui::Text("Offspring: %d", selectedCreature->offspringCount);
        ImGui::Text("Ancestors: %d", selectedCreature->ancestorCount);
    }

    // Actions
    ImGui::Separator();
    if (ImGui::Button("Follow")) SetCameraFollow(selectedCreature);
    ImGui::SameLine();
    if (ImGui::Button("Kill")) KillCreature(selectedCreature);
    ImGui::SameLine();
    if (ImGui::Button("Clone")) CloneCreature(selectedCreature);

    ImGui::End();
}
```

### Panel 4: Neural Network Visualizer

Draw actual network topology:
```cpp
void RenderNeuralNetworkGraph(const NeuralNetwork& brain)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize(400, 200);

    // Draw nodes
    for (int layer = 0; layer < 3; layer++)
    {
        int nodeCount = (layer == 0) ? INPUT_SIZE :
                        (layer == 1) ? HIDDEN_SIZE : OUTPUT_SIZE;
        for (int n = 0; n < nodeCount; n++)
        {
            float x = canvasPos.x + (layer + 1) * canvasSize.x / 4;
            float y = canvasPos.y + (n + 1) * canvasSize.y / (nodeCount + 1);
            float activation = GetNodeActivation(brain, layer, n);
            ImU32 color = IM_COL32(activation * 255, 100, 100, 255);
            drawList->AddCircleFilled(ImVec2(x, y), 8, color);
        }
    }

    // Draw connections (colored by weight)
    // ...
}
```

### Panel 5: Rendering Settings

```cpp
void RenderGraphicsPanel()
{
    ImGui::Begin("Graphics");

    // Quality presets
    if (ImGui::Button("Low")) SetQualityPreset(LOW);
    ImGui::SameLine();
    if (ImGui::Button("Medium")) SetQualityPreset(MEDIUM);
    ImGui::SameLine();
    if (ImGui::Button("High")) SetQualityPreset(HIGH);
    ImGui::SameLine();
    if (ImGui::Button("Ultra")) SetQualityPreset(ULTRA);

    // Individual toggles
    ImGui::Checkbox("Shadows", &enableShadows);
    ImGui::Checkbox("SSAO", &enableSSAO);
    ImGui::Checkbox("Bloom", &enableBloom);
    ImGui::Checkbox("SSR", &enableSSR);
    ImGui::Checkbox("Volumetric Fog", &enableVolumetrics);

    // Debug visualization
    ImGui::Separator();
    ImGui::Text("Debug");
    ImGui::Checkbox("Wireframe", &wireframeMode);
    ImGui::Checkbox("Show Normals", &showNormals);
    ImGui::Checkbox("Show Collision", &showCollision);
    ImGui::Checkbox("Show AI Targets", &showAITargets);

    // Performance
    ImGui::Separator();
    ImGui::Text("FPS: %.1f", 1000.0f / deltaTime);
    ImGui::Text("Frame Time: %.2f ms", deltaTime);
    ImGui::Text("Draw Calls: %d", drawCalls);
    ImGui::Text("Triangles: %d", triangleCount);

    ImGui::End();
}
```

### Panel 6: Genetics Visualization

```cpp
void RenderGeneticsPanel()
{
    ImGui::Begin("Genetics");

    // Gene frequency chart
    ImGui::Text("Gene Frequencies");
    for (auto& gene : geneList)
    {
        float freq = CalculateAlleleFrequency(gene);
        ImGui::ProgressBar(freq, ImVec2(-1, 0), gene.name.c_str());
    }

    // Hardy-Weinberg
    ImGui::Separator();
    ImGui::Text("Hardy-Weinberg Equilibrium");
    ImGui::Text("Expected: p² + 2pq + q² = 1");
    ImGui::Text("Observed: %.3f + %.3f + %.3f = %.3f",
                observedPP, observed2PQ, observedQQ,
                observedPP + observed2PQ + observedQQ);

    // Species phylogeny (simple tree)
    if (ImGui::CollapsingHeader("Phylogeny"))
    {
        RenderPhylogeneticTree();
    }

    ImGui::End();
}
```

## DELIVERABLES

- [ ] Full simulation control panel
- [ ] Real-time statistics with graphs
- [ ] Creature inspector with all details
- [ ] Neural network topology visualizer
- [ ] Graphics settings panel
- [ ] Genetics visualization
- [ ] Keyboard shortcuts for all actions
- [ ] Dockable/saveable layout
- [ ] All panels work at 60 FPS
```

---

## Agent 8: Ecosystem and Food Web System

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution

MISSION: Implement a complete ecosystem simulation with producers, consumers, decomposers, nutrient cycling, and emergent food web dynamics.

## CURRENT STATE

- src/environment/ProducerSystem.h/cpp - Plants/algae
- src/environment/DecomposerSystem.h/cpp - Decomposition
- src/environment/EcosystemMetrics.h/cpp - Tracking
- src/environment/EcosystemManager.h/cpp - Orchestration
- src/environment/SeasonManager.h/cpp - Seasons

## ECOSYSTEM ARCHITECTURE (4+ hours)

### Trophic Levels

```cpp
enum class TrophicLevel {
    PRODUCER,           // Grass, algae, trees (energy from sun)
    PRIMARY_CONSUMER,   // Herbivores (eat producers)
    SECONDARY_CONSUMER, // Carnivores (eat herbivores)
    TERTIARY_CONSUMER,  // Apex predators (eat carnivores)
    DECOMPOSER          // Bacteria, fungi (break down dead matter)
};
```

### Nutrient Cycling

```
[Sun] → [Producers] → [Herbivores] → [Carnivores]
           ↑              ↓              ↓
           └──── [Decomposers] ←── [Dead Matter]
                      ↓
                 [Soil Nutrients]
                      ↓
                 [Producers]
```

### Task 1: Producer System Enhancement

```cpp
class ProducerSystem {
    struct GrassPatch {
        Vec3 position;
        float biomass;          // 0-100
        float growthRate;       // Based on nutrients, water, sunlight
        float nutrientLevel;    // Soil nutrients at this location
    };

    struct Tree {
        Vec3 position;
        float age;
        float height;           // Grows with age
        float fruitCount;       // Food for herbivores
        float seedRadius;       // Reproduction range
    };

    std::vector<GrassPatch> grass;
    std::vector<Tree> trees;

    void Update(float deltaTime, const SeasonManager& season);
    void Photosynthesize(float sunlight);
    void ConsumeNutrients(float amount);
    void SpreadSeeds();
};
```

### Task 2: Decomposer System

```cpp
class DecomposerSystem {
    struct Corpse {
        Vec3 position;
        float biomass;          // Amount of organic matter
        float decompositionRate;// Based on temperature, moisture
        float nutrients;        // Released to soil as it decomposes
    };

    std::vector<Corpse> corpses;
    std::vector<float> soilNutrients;  // Grid-based

    void AddCorpse(const Creature& dead);
    void Update(float deltaTime, float temperature, float moisture);
    void ReleaseNutrients();
};
```

### Task 3: Carrying Capacity

Dynamic carrying capacity based on resources:
```cpp
int CalculateCarryingCapacity(TrophicLevel level) {
    switch (level) {
        case PRODUCER:
            return soilNutrients * GRASS_PER_NUTRIENT;
        case PRIMARY_CONSUMER:
            return totalGrassBiomass / GRASS_PER_HERBIVORE;
        case SECONDARY_CONSUMER:
            return herbivoreCount / HERBIVORES_PER_CARNIVORE;
        // Ecological rule: ~10% energy transfer between levels
    }
}
```

### Task 4: Predator-Prey Dynamics

Implement Lotka-Volterra interactions:
```cpp
// Population change rates
float dPrey_dt = preyGrowthRate * preyCount -
                 predationRate * preyCount * predatorCount;

float dPredator_dt = conversionEfficiency * predationRate * preyCount * predatorCount -
                     predatorDeathRate * predatorCount;
```

But with individual-based modeling (not just equations):
- Each creature makes decisions
- Emergent population dynamics
- Can exhibit chaos, oscillations, extinction

### Task 5: Seasons

```cpp
class SeasonManager {
    enum Season { SPRING, SUMMER, AUTUMN, WINTER };

    Season current;
    float dayLength;        // Hours of sunlight
    float temperature;      // Affects metabolism
    float precipitation;    // Affects plant growth

    // Effects
    float GetPlantGrowthModifier();
    float GetMetabolismModifier();
    float GetReproductionModifier();
    bool IsMigrationSeason();
};
```

### Task 6: Food Web Visualization

```cpp
void RenderFoodWeb()
{
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Draw nodes (species)
    for (auto& species : allSpecies)
    {
        ImVec2 pos = GetFoodWebPosition(species.trophicLevel, species.id);
        float radius = sqrt(species.population) * SCALE;
        dl->AddCircleFilled(pos, radius, GetSpeciesColor(species));
    }

    // Draw edges (predation relationships)
    for (auto& predation : predationMatrix)
    {
        float strength = predation.preyConsumed / totalPreyPopulation;
        ImU32 color = IM_COL32(255, 100, 100, strength * 255);
        dl->AddLine(predator.pos, prey.pos, color, strength * 3);
    }
}
```

### Task 7: Emergent Behaviors

Enable these to emerge naturally:
- **Boom-bust cycles** - Prey overpopulate, predators follow, prey crash
- **Keystone species** - One species whose removal cascades
- **Invasive species** - New creature dominates ecosystem
- **Extinction cascades** - One extinction causes others
- **Niche partitioning** - Species specialize to coexist

## METRICS TO TRACK

- Shannon diversity index
- Species richness
- Evenness
- Trophic efficiency
- Nutrient cycling rate
- Ecosystem stability (variance in population)

## DELIVERABLES

- [ ] Full producer system (grass, trees, algae)
- [ ] Decomposer system with nutrient cycling
- [ ] Dynamic carrying capacity
- [ ] Seasonal effects on all systems
- [ ] Food web visualization
- [ ] Ecosystem metrics dashboard
- [ ] Stable long-term simulation (1000+ generations)
- [ ] Documentation of ecosystem dynamics
```

---

## Agent 9: Multiplayer/Network Foundation

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution

MISSION: Design and implement the foundation for a networked multiplayer simulation where multiple users can observe and interact with the same ecosystem.

## ARCHITECTURE DESIGN (5+ hours)

### Network Model: Authoritative Server

```
[Server]
    - Runs simulation
    - Owns all creature state
    - Processes player commands
    - Broadcasts state updates

[Client 1] ←→ [Server] ←→ [Client 2]
    - Renders simulation
    - Sends commands
    - Interpolates state
```

### Task 1: Protocol Design

```cpp
// Message types
enum class NetMessage : uint8_t {
    // Client → Server
    CONNECT_REQUEST,
    DISCONNECT,
    SPAWN_CREATURE,
    KILL_CREATURE,
    CAMERA_UPDATE,
    SELECT_CREATURE,

    // Server → Client
    CONNECT_ACCEPT,
    CONNECT_REJECT,
    FULL_STATE_SYNC,
    DELTA_UPDATE,
    CREATURE_SPAWN,
    CREATURE_DEATH,
    EVENT_NOTIFICATION
};

// Delta update (sent 20-60 times/second)
struct CreatureDelta {
    uint32_t creatureId;
    uint16_t flags;  // Which fields changed
    // Optional fields based on flags:
    float position[3];
    float rotation[4];
    float energy;
    uint8_t state;
};

struct DeltaPacket {
    uint32_t serverTick;
    uint16_t creatureCount;
    CreatureDelta creatures[];  // Variable length
};
```

### Task 2: Server Implementation

```cpp
class SimulationServer {
    // Network
    SOCKET listenSocket;
    std::vector<ClientConnection> clients;

    // Simulation
    SimulationWorld world;
    uint32_t currentTick;

    // State
    std::unordered_map<uint32_t, Creature> creatures;
    RingBuffer<DeltaPacket> deltaHistory;  // For late-join catchup

    void Run();
    void AcceptConnections();
    void ProcessClientMessages();
    void UpdateSimulation(float dt);
    void BroadcastDeltas();
    void HandleClientCommand(ClientConnection& client, const Command& cmd);
};
```

### Task 3: Client Implementation

```cpp
class SimulationClient {
    // Network
    SOCKET serverSocket;

    // Local state (interpolated)
    std::unordered_map<uint32_t, Creature> creatures;
    std::unordered_map<uint32_t, Creature> previousState;
    float interpolationT;

    // Rendering
    DX12Renderer renderer;
    Camera camera;

    void Connect(const char* serverAddress, uint16_t port);
    void Disconnect();
    void Update(float dt);
    void ProcessServerMessages();
    void InterpolateState();
    void Render();
    void SendCommand(const Command& cmd);
};
```

### Task 4: State Interpolation

```cpp
Creature InterpolateCreature(const Creature& prev, const Creature& current, float t)
{
    Creature result;

    // Linear interpolation for position
    result.position = Lerp(prev.position, current.position, t);

    // Spherical interpolation for rotation
    result.rotation = Slerp(prev.rotation, current.rotation, t);

    // Linear for scalar values
    result.energy = Lerp(prev.energy, current.energy, t);

    // Snap for discrete states
    result.state = current.state;

    return result;
}
```

### Task 5: Delta Compression

Only send what changed:
```cpp
uint16_t CalculateDeltaFlags(const Creature& prev, const Creature& current)
{
    uint16_t flags = 0;

    if (Distance(prev.position, current.position) > POSITION_THRESHOLD)
        flags |= DELTA_POSITION;

    if (Dot(prev.rotation, current.rotation) < ROTATION_THRESHOLD)
        flags |= DELTA_ROTATION;

    if (abs(prev.energy - current.energy) > ENERGY_THRESHOLD)
        flags |= DELTA_ENERGY;

    if (prev.state != current.state)
        flags |= DELTA_STATE;

    return flags;
}
```

### Task 6: Player Interactions

```cpp
// Commands players can send
struct SpawnCommand {
    CreatureType type;
    Vec3 position;
    // Server validates position is valid, enforces spawn limits
};

struct SelectCommand {
    uint32_t creatureId;
    // Server tracks which player owns which selection
};

struct FollowCommand {
    uint32_t creatureId;
    // Server sends priority updates for followed creature
};

struct ModifyEnvironmentCommand {
    EnvironmentAction action;  // PLACE_FOOD, REMOVE_OBSTACLE, etc.
    Vec3 position;
};
```

### Task 7: Latency Compensation

```cpp
// Client-side prediction for responsive feel
void PredictCreaturePosition(Creature& creature, float latency)
{
    // Extrapolate based on velocity
    creature.position += creature.velocity * latency;

    // Will be corrected when server update arrives
}

// Server-side lag compensation for commands
void ProcessCommandWithLagCompensation(Command& cmd, float clientLatency)
{
    // Rewind simulation state by clientLatency
    // Apply command at that historical state
    // Fast-forward back to present
}
```

### Task 8: Bandwidth Optimization

- Area of Interest (AOI): Only send creatures near player camera
- LOD for network: Full detail nearby, position-only for distant
- Quantization: Compress floats to 16-bit fixed point
- Batch messages: Combine multiple updates in one packet

## DELIVERABLES

- [ ] Protocol specification document
- [ ] SimulationServer class (standalone executable)
- [ ] SimulationClient class (integrated into main app)
- [ ] State interpolation working smoothly
- [ ] Delta compression reducing bandwidth
- [ ] Player commands working
- [ ] 2+ clients connected to same server
- [ ] Latency < 100ms feels responsive
- [ ] Documentation of network architecture
```

---

## Agent 10: Performance Profiling and Optimization

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution

MISSION: Perform comprehensive performance profiling and implement optimizations to achieve 10,000+ creatures at 60 FPS stable.

## PROFILING PHASE (2 hours)

### Task 1: Instrument Code

Add timing to all major systems:
```cpp
class Profiler {
    struct Sample {
        const char* name;
        double startTime;
        double duration;
    };

    std::vector<Sample> frameSamples;

    void BeginSample(const char* name);
    void EndSample();
    void PrintFrameReport();
    void SaveToCSV(const char* filename);
};

// Usage
PROFILE_SCOPE("CreatureUpdate");
UpdateAllCreatures(dt);
PROFILE_END();
```

### Task 2: GPU Profiling

Use DX12 timestamp queries:
```cpp
class GPUProfiler {
    ComPtr<ID3D12QueryHeap> queryHeap;
    ComPtr<ID3D12Resource> readbackBuffer;

    void BeginGPUTimestamp(ID3D12GraphicsCommandList* cmd, const char* name);
    void EndGPUTimestamp(ID3D12GraphicsCommandList* cmd);
    void ResolveTimestamps();
    void PrintGPUReport();
};
```

### Task 3: Memory Profiling

Track allocations:
```cpp
void* operator new(size_t size) {
    void* ptr = malloc(size);
    g_memoryTracker.RecordAllocation(ptr, size, __FILE__, __LINE__);
    return ptr;
}

void operator delete(void* ptr) {
    g_memoryTracker.RecordDeallocation(ptr);
    free(ptr);
}
```

## OPTIMIZATION PHASE (4+ hours)

### Known Bottlenecks to Fix

#### 1. O(n²) Creature Interactions

**Problem:** Every creature checks every other creature
**Solution:** Spatial hashing

```cpp
class SpatialHash {
    static constexpr int CELL_SIZE = 10;

    std::unordered_map<uint64_t, std::vector<Creature*>> cells;

    uint64_t HashPosition(const Vec3& pos) {
        int x = (int)(pos.x / CELL_SIZE);
        int z = (int)(pos.z / CELL_SIZE);
        return ((uint64_t)x << 32) | (uint64_t)z;
    }

    void Clear();
    void Insert(Creature* creature);
    std::vector<Creature*> Query(const Vec3& center, float radius);
};
```

**Expected improvement:** O(n²) → O(n * k) where k << n

#### 2. Per-Frame Allocations

**Problem:** Creating vectors every frame
```cpp
// BAD
void Update() {
    std::vector<Creature*> nearby;  // Allocates!
    nearby = FindNearby(position);
}
```

**Solution:** Pre-allocate and reuse
```cpp
// GOOD
class Creature {
    std::vector<Creature*> nearbyCache;  // Reused each frame

    void Update() {
        nearbyCache.clear();  // Clear, don't deallocate
        FindNearby(position, nearbyCache);
    }
};
```

#### 3. Cache-Unfriendly Data Layout

**Problem:** Array of Structures (AoS)
```cpp
struct Creature {
    Vec3 position;        // Used every frame
    std::string name;     // Rarely used
    NeuralNetwork brain;  // Large, cold data
    // ... 500 bytes total
};
std::vector<Creature> creatures;  // Cache thrashing
```

**Solution:** Structure of Arrays (SoA)
```cpp
struct CreaturePositions {
    std::vector<Vec3> positions;
    std::vector<Vec3> velocities;
    // Hot data together
};

struct CreatureMetadata {
    std::vector<std::string> names;
    std::vector<NeuralNetwork> brains;
    // Cold data separate
};
```

#### 4. Branch Misprediction

**Problem:** Type-based branching in hot loop
```cpp
for (auto& creature : creatures) {
    if (creature.type == HERBIVORE) {      // Mispredicts!
        UpdateHerbivore(creature);
    } else {
        UpdateCarnivore(creature);
    }
}
```

**Solution:** Sort by type
```cpp
std::sort(creatures.begin(), creatures.end(),
          [](auto& a, auto& b) { return a.type < b.type; });

// Now branch predictor is happy
for (auto& creature : creatures) {
    // All herbivores first, then all carnivores
}
```

#### 5. Draw Call Overhead

**Problem:** One draw call per creature
**Solution:** GPU instancing (already have this, verify it's working)

#### 6. Constant Buffer Updates

**Problem:** Map/Unmap for every creature
**Solution:** Single large buffer, batch updates

### SIMD Optimization

```cpp
// Before
for (int i = 0; i < count; i++) {
    positions[i].x += velocities[i].x * dt;
    positions[i].y += velocities[i].y * dt;
    positions[i].z += velocities[i].z * dt;
}

// After (SSE)
for (int i = 0; i < count; i += 4) {
    __m128 px = _mm_load_ps(&positions[i].x);
    __m128 vx = _mm_load_ps(&velocities[i].x);
    __m128 dt4 = _mm_set1_ps(dt);
    px = _mm_add_ps(px, _mm_mul_ps(vx, dt4));
    _mm_store_ps(&positions[i].x, px);
}
```

### Multithreading

```cpp
// Parallel creature update
void UpdateCreaturesParallel()
{
    const int threadCount = std::thread::hardware_concurrency();
    const int batchSize = creatures.size() / threadCount;

    std::vector<std::thread> threads;
    for (int t = 0; t < threadCount; t++)
    {
        int start = t * batchSize;
        int end = (t == threadCount - 1) ? creatures.size() : start + batchSize;

        threads.emplace_back([this, start, end]() {
            for (int i = start; i < end; i++) {
                UpdateCreature(creatures[i]);
            }
        });
    }

    for (auto& t : threads) t.join();
}
```

## DELIVERABLES

- [ ] Profiler class with CPU timing
- [ ] GPU profiler with timestamp queries
- [ ] Memory tracking
- [ ] Spatial hash implemented
- [ ] SoA data layout for hot data
- [ ] SIMD where beneficial
- [ ] Multithreaded creature update
- [ ] Performance comparison: before/after
- [ ] Documentation: optimization techniques used
- [ ] Target achieved: 10,000 creatures at 60 FPS
```

---

## USAGE INSTRUCTIONS

1. **Copy entire prompt block** (from ``` to ```)
2. **Paste into new Claude session**
3. **Let agent work autonomously** - each is designed for 2-5 hours of work
4. **Run multiple agents in parallel** if tasks are independent

## RECOMMENDED ORDER

For maximum progress with minimal conflicts:
1. Agent 1 (OpenGL→DX12) - MUST DO FIRST
2. Agent 4 (Terrain) - Independent
3. Agent 6 (Rendering) - Independent
4. Agent 7 (UI) - Independent
5. Agent 3 (GPU Compute) - After Agent 1
6. Agent 10 (Performance) - After base works
7. Agent 5 (NEAT) - After performance is acceptable
8. Agent 2 (Genetics) - After NEAT
9. Agent 8 (Ecosystem) - After genetics
10. Agent 9 (Network) - Future feature
