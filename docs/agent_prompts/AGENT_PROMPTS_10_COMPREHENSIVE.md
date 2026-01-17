# 10 Comprehensive Agent Prompts for OrganismEvolution Fixes

Each prompt below is designed to be copied into a **separate Claude chat session**. Each session can work independently for hours, spawn sub-agents, and create thorough documentation.

---

## AGENT 1: Water Depth, Wave Animation, and Ocean System Overhaul

```
I need you to comprehensively fix the water system in my DirectX 12 evolution simulation. The project is located at:

C:\Users\andre\Desktop\OrganismEvolution

CURRENT PROBLEMS:
1. Water covers too much of the terrain - the WATER_THRESH is set too high
2. Wave animation is too aggressive and unrealistic
3. Water-to-beach transitions are harsh
4. The water doesn't look realistic - it's just a flat color with basic ripples

KEY FILES TO MODIFY:

1. Runtime/Shaders/Terrain.hlsl - The main terrain shader containing all water rendering code
   - Contains biome thresholds around lines 661-665
   - Contains wave animation in VSMain around lines 607-621
   - Contains generateRealisticWaterColor() function
   - Contains PSMain with biome selection logic

2. src/main.cpp - Contains Terrain class with WATER_LEVEL constant

CURRENT PROBLEMATIC VALUES (in Terrain.hlsl):
```hlsl
static const float WATER_THRESH = 0.02;    // Still might be too high
static const float BEACH_THRESH = 0.06;
float waveSpeed = 0.3;
float waveHeight = 0.008;
```

YOUR COMPREHENSIVE TASKS:

PHASE 1: WATER LEVEL ANALYSIS (30 minutes)
- Read the entire Terrain.hlsl file thoroughly
- Read the Terrain class in main.cpp to understand height generation
- Document current water level settings and their effects
- Understand the relationship between WATER_LEVEL in C++, WATER_THRESH in HLSL, and heightNormalized

PHASE 2: WATER THRESHOLD TUNING (1 hour)
- Adjust WATER_THRESH to around 0.01-0.015 so even less terrain is underwater
- Adjust BEACH_THRESH proportionally (should be about 3x WATER_THRESH)
- Ensure the calculateProceduralNormal function thresholds match the PSMain thresholds
- Update any hardcoded threshold values throughout the shader

PHASE 3: WAVE ANIMATION IMPROVEMENT (1.5 hours)
- Reduce waveHeight to around 0.003-0.005 for subtle, realistic waves
- Reduce waveSpeed to around 0.2 for calmer water
- Add multiple wave frequencies for more natural water motion:
  - Large slow swells (freq 0.5, speed 0.1)
  - Medium waves (freq 2.0, speed 0.3)
  - Small ripples (freq 8.0, speed 0.5)
- Add wave direction variation (not just X and Z axis aligned)
- Consider wind direction affecting wave direction

PHASE 4: REALISTIC WATER RENDERING (2 hours)
Improve the generateRealisticWaterColor() function with:

1. Fresnel Effect (critical for realism):
```hlsl
float fresnel = pow(1.0 - saturate(dot(viewDir, waterNormal)), 4.0);
// More reflective at grazing angles, more transparent looking straight down
```

2. Depth-based coloring:
```hlsl
float waterDepth = (WATER_THRESH - heightNormalized) / WATER_THRESH;
float3 shallowColor = float3(0.1, 0.4, 0.5);   // Turquoise
float3 deepColor = float3(0.02, 0.1, 0.15);    // Dark blue-green
float3 waterBase = lerp(shallowColor, deepColor, saturate(waterDepth * 2.0));
```

3. Dynamic water normals for specular:
```hlsl
float3 calculateWaterNormal(float3 worldPos, float time) {
    float2 uv1 = worldPos.xz * 0.1 + time * 0.02;
    float2 uv2 = worldPos.xz * 0.25 - time * 0.03;
    float n1 = noise3D(float3(uv1, 0.0));
    float n2 = noise3D(float3(uv2, 0.0));
    return normalize(float3(n1 * 0.1, 1.0, n2 * 0.1));
}
```

4. Specular highlights (sun reflection):
```hlsl
float3 reflectDir = reflect(-lightDir, waterNormal);
float spec = pow(max(dot(viewDir, reflectDir), 0.0), 256.0);
float3 sunReflection = lightColor * spec * sunIntensity;
```

5. Shoreline foam:
```hlsl
float shoreDistance = (heightNormalized - WATER_THRESH) / 0.02;
float foam = saturate(1.0 - abs(shoreDistance) * 10.0);
foam *= noise3D(worldPos * 30.0 + time * 2.0) * 0.5 + 0.5;
waterColor = lerp(waterColor, float3(0.9, 0.95, 1.0), foam * 0.7);
```

PHASE 5: WATER-BEACH TRANSITION (30 minutes)
- Add a "wet sand" zone just above the waterline
- Darken the beach color near water
- Add subtle wave foam washing onto beach
- Smooth the color transition over a larger range

PHASE 6: TESTING AND DOCUMENTATION (30 minutes)
- Document all changes made
- Create before/after comparison
- List all modified values
- Create docs/WATER_SYSTEM_COMPREHENSIVE.md with full technical documentation

IMPORTANT IMPLEMENTATION NOTES:
- The shader uses column-major matrix multiplication (mul(matrix, vector))
- heightNormalized = input.height / 30.0 (terrain height range is 0-30)
- Water vertices are identified by input.position.y < 0.5
- The constant buffer must match exactly between C++ and HLSL

You may spawn sub-agents to research water shader techniques, parallelize shader modifications, or test different parameter values. Document everything thoroughly.

Expected deliverables:
1. Modified Runtime/Shaders/Terrain.hlsl with all water improvements
2. docs/WATER_SYSTEM_COMPREHENSIVE.md with full documentation
3. List of all changed values with explanations
```

---

## AGENT 2: Creature Visibility - Deep Investigation and Complete Fix

```
I need you to comprehensively investigate and fix why creatures are only visible in water areas but not on land in my DirectX 12 evolution simulation. This is a critical bug that requires deep investigation.

Project location: C:\Users\andre\Desktop\OrganismEvolution

THE PROBLEM:
Creatures appear when they're in water/low areas but become invisible when walking on grass, beach, or mountain terrain. This suggests a rendering issue, not a simulation issue.

KEY FILES TO INVESTIGATE:

1. src/main.cpp (~4500 lines) - Contains:
   - Creature spawning (around line 2113-2138)
   - Creature update and position setting (around line 2320-2378)
   - Creature rendering pipeline creation (search for "creaturePipeline")
   - Creature draw calls
   - Model matrix calculation
   - Constant buffer updates

2. Runtime/Shaders/Creature.hlsl (~393 lines) - Contains:
   - Instanced vertex shader (VSMain)
   - Pixel shader (PSMain)
   - PBR lighting calculations
   - Debug mode that might be enabled

3. Runtime/Shaders/Terrain.hlsl - For understanding terrain heights and comparison

COMPREHENSIVE INVESTIGATION TASKS:

PHASE 1: SHADER DEBUG MODE CHECK (15 minutes)
- Read Creature.hlsl PSMain function
- Look for any debug return statements like:
  ```hlsl
  return float4(1.0, 0.0, 1.0, 1.0); // Magenta debug color
  ```
- If debug mode is enabled, that's the problem - creatures render but are all magenta
- Check if the shader produces valid colors for all inputs

PHASE 2: DEPTH BUFFER INVESTIGATION (1 hour)
This is the most likely cause. Check:

1. Pipeline depth test settings in main.cpp:
   - Search for "DepthStencilState" or "depthCompareOp"
   - Current setting might be CompareOp::Less
   - Should be CompareOp::LessEqual to allow creatures at same depth as terrain

2. Depth bias settings:
   - Search for "DepthBias"
   - Might need to add depth bias to push creatures slightly forward

3. Creature Y position:
   - In UpdateCreature() around line 2363, creatures are set to:
     ```cpp
     c.position.y = terrainHeight + 0.1f;
     ```
   - The +0.1f offset might not be enough
   - Try increasing to +0.5f or +1.0f

PHASE 3: RENDER ORDER ANALYSIS (30 minutes)
Check the draw call order in the Render() function:
1. Is terrain rendered first?
2. Are creatures rendered after terrain?
3. Are there any resource state issues?

The correct order should be:
1. Clear depth/color buffers
2. Render terrain (fills depth buffer)
3. Render creatures (tested against depth buffer)
4. Render nametags (above everything)
5. Render ImGui

PHASE 4: MODEL MATRIX VERIFICATION (1 hour)
Check creature model matrix calculation:

1. Find GetModelMatrix() or equivalent function
2. Verify the matrix order is correct for column-major:
   ```cpp
   // Correct for column-major (DX12):
   Mat4 model = translationMatrix * rotationMatrix * scaleMatrix;
   ```

3. Verify scale values:
   - genome.size might be very small (< 0.1)
   - Creatures might be rendering but too tiny to see
   - Check creature scale range in Genome::Random()

4. Verify translation includes proper Y offset:
   - Position should be terrain height + offset
   - Check if Y is being set correctly in the matrix

PHASE 5: INSTANCED RENDERING CHECK (1 hour)
The creature shader uses instanced rendering:

1. Check instance buffer creation and update:
   - Search for "instanceBuffer" or "InstanceData"
   - Verify instance data is being filled for all creatures
   - Check if instance count matches actual creature count

2. Verify instance data format matches shader:
   ```hlsl
   // Shader expects per-instance:
   float4 instModelRow0 : INST_MODEL0;
   float4 instModelRow1 : INST_MODEL1;
   float4 instModelRow2 : INST_MODEL2;
   float4 instModelRow3 : INST_MODEL3;
   float4 instColorType : INST_COLOR;
   ```

3. Check input layout in pipeline creation matches this exactly

PHASE 6: CONSTANT BUFFER VERIFICATION (45 minutes)
1. Check that viewProjection matrix is correct
2. Verify viewPos (camera position) is being set
3. Check if any values are NaN or invalid
4. Ensure constant buffer is uploaded before creature draw calls

PHASE 7: POSITION CALCULATION DEBUG (30 minutes)
Add temporary debug output:
```cpp
// In UpdateCreature or rendering loop:
static bool debugOnce = true;
if (debugOnce && creatures.size() > 0) {
    printf("Creature 0 position: (%.2f, %.2f, %.2f)\n",
           creatures[0]->position.x,
           creatures[0]->position.y,
           creatures[0]->position.z);
    printf("Terrain height at creature: %.2f\n",
           terrain.GetHeightWorld(creatures[0]->position.x, creatures[0]->position.z));
    printf("Creature scale: %.2f\n", creatures[0]->genome.size);
    debugOnce = false;
}
```

PHASE 8: FIXES TO APPLY
Based on investigation, apply these fixes:

1. Depth test fix (in pipeline creation):
```cpp
depthStencilDesc.depthCompareOp = CompareOp::LessEqual;  // Was Less
```

2. Y position offset increase:
```cpp
c.position.y = terrainHeight + 0.5f;  // Was 0.1f
```

3. Minimum creature scale:
```cpp
genome.size = std::max(genome.size, 0.5f);  // Ensure visible size
```

4. Color visibility boost in Creature.hlsl:
```hlsl
// In PSMain, after calculating skinColor:
skinColor = max(skinColor, float3(0.1, 0.1, 0.1));  // Prevent pure black
```

PHASE 9: DOCUMENTATION (30 minutes)
Create comprehensive documentation:
- docs/CREATURE_VISIBILITY_INVESTIGATION.md
- Include all findings, root cause, and fixes
- Add diagrams if helpful
- List all code changes with line numbers

You should spawn sub-agents to:
1. Investigate shader compilation and debug output
2. Analyze the rendering pipeline state
3. Test different depth test configurations
4. Verify instance buffer data integrity

This is a complex bug that requires methodical investigation. Document your entire process.
```

---

## AGENT 3: Vegetation System - Complete Grass and Tree Implementation

```
I need you to implement a comprehensive vegetation system for my DirectX 12 evolution simulation. Currently there's no grass or trees visible on the terrain.

Project location: C:\Users\andre\Desktop\OrganismEvolution

CURRENT STATE:
- Terrain renders with procedural biome colors (grass is just a flat green color)
- There may be tree data structures but no visible trees
- No grass blades or vegetation detail

KEY FILES:

1. Runtime/Shaders/Terrain.hlsl - Has generateGrassColor() and generateForestColor() functions
2. Runtime/Shaders/Vegetation.hlsl - May exist for tree rendering
3. src/main.cpp - Contains:
   - Tree struct and vector (trees)
   - Tree placement algorithm
   - Vegetation pipeline (m_vegetationPipeline)
   - Tree buffers (m_treeVB, m_treeIB)

COMPREHENSIVE IMPLEMENTATION TASKS:

PHASE 1: ANALYZE EXISTING VEGETATION CODE (30 minutes)
- Search main.cpp for all "tree", "vegetation", "grass" references
- Check if tree buffers are created and populated
- Check if vegetation pipeline exists and is being used
- Check if trees are being rendered (look for draw calls)
- Read Vegetation.hlsl if it exists

PHASE 2: FIX EXISTING TREE RENDERING (1 hour)
If trees exist but aren't rendering:

1. Verify tree mesh generation:
```cpp
// Should create trunk + canopy mesh
void CreateTreeMesh(std::vector<TreeVertex>& vertices,
                    std::vector<u32>& indices) {
    // Trunk: tapered cylinder
    for (int i = 0; i < 8; i++) {
        float angle = i * 3.14159f * 2.0f / 8.0f;
        float bottomRadius = 0.3f;
        float topRadius = 0.1f;
        // Add trunk vertices...
    }
    // Canopy: cone or sphere shape
    // Add canopy vertices...
}
```

2. Verify tree instance data is being created for each tree
3. Verify vegetation pipeline is bound before tree draw calls
4. Check if trees are positioned correctly on terrain

PHASE 3: ENHANCE GRASS IN TERRAIN SHADER (2 hours)
Improve generateGrassColor() in Terrain.hlsl:

1. Add grass blade patterns:
```hlsl
float3 generateGrassColor(float3 pos, float3 normal) {
    // Base grass colors
    float3 lightGrass = float3(0.3, 0.55, 0.2);
    float3 darkGrass = float3(0.15, 0.35, 0.1);
    float3 yellowGrass = float3(0.5, 0.5, 0.15);  // Dried patches

    // Large-scale color variation (grass patches)
    float patchNoise = fbm(pos * 0.5, 2);
    float3 baseGrass = lerp(darkGrass, lightGrass, patchNoise);

    // Dried grass patches
    float driedPatch = smoothstep(0.6, 0.8, fbm(pos * 0.3, 2));
    baseGrass = lerp(baseGrass, yellowGrass, driedPatch * 0.4);

    // Individual grass blade patterns (high frequency)
    float bladePattern = noise3D(pos * 80.0);
    float bladeHighlight = smoothstep(0.6, 0.9, bladePattern);
    baseGrass += float3(0.05, 0.1, 0.02) * bladeHighlight;

    // Grass blade direction variation
    float windEffect = sin(pos.x * 0.5 + time * 0.5) * 0.5 + 0.5;
    float bladeDirection = noise3D(pos * 40.0 + float3(time * 0.2, 0, 0));

    // Darken grass in valleys (ambient occlusion approximation)
    float ao = saturate(normal.y);
    baseGrass *= 0.7 + ao * 0.3;

    return baseGrass;
}
```

2. Add grass clumps:
```hlsl
// Voronoi-based grass clump pattern
float grassClump = voronoi(pos.xz * 3.0);
float clumpEdge = smoothstep(0.2, 0.3, grassClump);
// Edges of clumps are slightly darker
```

PHASE 4: IMPLEMENT BILLBOARD GRASS (2 hours)
For high-quality grass, add geometry amplification or billboards:

Option A: Vertex-displaced grass in terrain shader:
```hlsl
// In vertex shader, for grass biome vertices:
if (isGrassBiome && input.position.y > GRASS_MIN_HEIGHT) {
    // Add vertex displacement for grass blade effect
    float grassHeight = noise3D(input.position * 20.0) * 0.3;
    float windSway = sin(input.position.x * 2.0 + time * 3.0) * 0.1;
    output.position.y += grassHeight;
    output.position.x += windSway * grassHeight;
}
```

Option B: Create separate grass billboard system:
```cpp
struct GrassBlade {
    float3 position;
    float rotation;
    float height;
    float3 color;
};

void PlaceGrassBlades(std::vector<GrassBlade>& grass) {
    for (float x = -100; x < 100; x += 0.5f) {
        for (float z = -100; z < 100; z += 0.5f) {
            float height = terrain.GetHeightWorld(x, z);
            if (height > BEACH_HEIGHT && height < FOREST_HEIGHT) {
                GrassBlade blade;
                blade.position = Vec3(x + RandomFloat(-0.2f, 0.2f),
                                      height,
                                      z + RandomFloat(-0.2f, 0.2f));
                blade.rotation = RandomFloat(0, 6.28f);
                blade.height = RandomFloat(0.2f, 0.5f);
                grass.push_back(blade);
            }
        }
    }
}
```

PHASE 5: IMPROVE TREE VARIETY (1.5 hours)
Create multiple tree types:

1. Oak tree (round canopy):
```cpp
void CreateOakMesh(std::vector<Vertex>& v, std::vector<u32>& i) {
    // Trunk: brown cylinder
    CreateCylinder(v, i, 0.0f, 0.0f, 0.3f, 2.0f, brown);
    // Canopy: green sphere
    CreateSphere(v, i, 0.0f, 3.0f, 0.0f, 1.5f, green);
}
```

2. Pine tree (conical):
```cpp
void CreatePineMesh(std::vector<Vertex>& v, std::vector<u32>& i) {
    // Trunk: brown cylinder
    CreateCylinder(v, i, 0.0f, 0.0f, 0.2f, 2.5f, brown);
    // Canopy: stacked cones
    CreateCone(v, i, 0.0f, 2.0f, 0.0f, 1.2f, 2.0f, darkGreen);
    CreateCone(v, i, 0.0f, 3.5f, 0.0f, 0.8f, 1.5f, darkGreen);
}
```

3. Willow tree (drooping):
```cpp
void CreateWillowMesh(std::vector<Vertex>& v, std::vector<u32>& i) {
    // Trunk
    CreateCylinder(v, i, 0.0f, 0.0f, 0.4f, 3.0f, brown);
    // Drooping branches (multiple small cones pointing down)
    for (int i = 0; i < 12; i++) {
        float angle = i * 3.14159f * 2.0f / 12.0f;
        float x = cos(angle) * 0.8f;
        float z = sin(angle) * 0.8f;
        CreateDroopingBranch(v, i, x, 3.0f, z, lightGreen);
    }
}
```

PHASE 6: TREE PLACEMENT ALGORITHM (1 hour)
Improve tree placement for realistic forests:

```cpp
void PlaceTrees(std::vector<Tree>& trees) {
    // Use Poisson disk sampling for natural distribution
    std::vector<Vec2> points = PoissonDiskSampling(200.0f, 200.0f, 8.0f);

    for (const auto& p : points) {
        float x = p.x - 100.0f;
        float z = p.y - 100.0f;
        float height = terrain.GetHeightWorld(x, z);
        float heightNorm = height / 30.0f;

        // Only place trees in forest biome (0.35 - 0.67 normalized)
        if (heightNorm > 0.35f && heightNorm < 0.67f) {
            Tree tree;
            tree.position = Vec3(x, height, z);

            // Tree type based on height within forest
            if (heightNorm < 0.45f) {
                tree.type = TreeType::OAK;  // Lower forest
            } else if (heightNorm < 0.55f) {
                tree.type = TreeType::WILLOW;  // Mid forest
            } else {
                tree.type = TreeType::PINE;  // Upper forest, transition to mountain
            }

            tree.scale = RandomFloat(0.8f, 1.5f);
            tree.rotation = RandomFloat(0, 6.28f);
            trees.push_back(tree);
        }
    }
}
```

PHASE 7: VEGETATION SHADER IMPROVEMENTS (1 hour)
Create or improve Vegetation.hlsl:

```hlsl
// Vegetation.hlsl

cbuffer Constants : register(b0) {
    float4x4 viewProjection;
    float3 viewPos;
    float time;
    float3 lightPos;
    float3 lightColor;
    float3 windDirection;
    float windStrength;
};

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 color : COLOR;
    // Per-instance data
    float4 instanceTransform0 : INST0;
    float4 instanceTransform1 : INST1;
    float4 instanceTransform2 : INST2;
    float4 instanceTransform3 : INST3;
};

PSInput VSMain(VSInput input) {
    PSInput output;

    // Reconstruct instance matrix
    float4x4 instanceMatrix = float4x4(
        input.instanceTransform0,
        input.instanceTransform1,
        input.instanceTransform2,
        input.instanceTransform3
    );

    // Apply wind animation to upper parts of tree
    float3 pos = input.position;
    float windFactor = saturate(input.position.y / 4.0);  // More sway at top
    pos.x += sin(time * 2.0 + input.position.x * 0.5) * windStrength * windFactor;
    pos.z += cos(time * 1.7 + input.position.z * 0.5) * windStrength * windFactor * 0.7;

    float4 worldPos = mul(instanceMatrix, float4(pos, 1.0));
    output.fragPos = worldPos.xyz;
    output.position = mul(viewProjection, worldPos);
    output.normal = mul((float3x3)instanceMatrix, input.normal);
    output.color = input.color;

    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    float3 norm = normalize(input.normal);
    float3 lightDir = normalize(lightPos - input.fragPos);

    // Simple lighting for vegetation
    float diff = max(dot(norm, lightDir), 0.0);
    float3 ambient = input.color * 0.4;
    float3 diffuse = input.color * diff * 0.6;

    // Subsurface scattering approximation for leaves
    float3 viewDir = normalize(viewPos - input.fragPos);
    float backScatter = max(dot(-viewDir, lightDir), 0.0);
    float3 sss = input.color * backScatter * 0.2;

    return float4(ambient + diffuse + sss, 1.0);
}
```

PHASE 8: DOCUMENTATION (30 minutes)
Create docs/VEGETATION_SYSTEM_COMPLETE.md with:
- All vegetation types implemented
- Shader code explanations
- Placement algorithm details
- Performance considerations
- Future improvements (LOD, instancing optimizations)

Spawn sub-agents to:
1. Research vegetation rendering techniques
2. Implement different tree types in parallel
3. Optimize grass rendering for performance
4. Create vegetation LOD system
```

---

## AGENT 4: Camera System - Complete Fix for Drift, Controls, and Smoothing

```
I need you to comprehensively fix the camera system in my DirectX 12 evolution simulation. The camera has multiple issues that need to be addressed together.

Project location: C:\Users\andre\Desktop\OrganismEvolution

CURRENT PROBLEMS:
1. Camera keeps drifting/moving when user isn't providing input
2. Camera smoothing might be causing issues
3. Camera might be auto-following something
4. First-frame mouse capture causes a jump

KEY FILE:
src/main.cpp - Contains all camera code including:
- Camera struct (around line 2450-2700)
- HandleInput() function (around line 4500-4700)
- Mouse capture/release logic
- Camera movement calculations

COMPREHENSIVE TASKS:

PHASE 1: CAMERA STRUCT ANALYSIS (30 minutes)
Find and analyze the Camera struct:
```cpp
struct Camera {
    Vec3 position;
    Vec3 smoothPosition;  // For interpolation
    f32 yaw, pitch;
    f32 smoothYaw, smoothPitch;
    f32 zoom;  // FOV
    f32 speed;
    f32 sensitivity;
    // ... etc
};
```

Document:
- All camera state variables
- Smoothing mechanism
- How target vs smooth values work

PHASE 2: FIX CAMERA DRIFT (1.5 hours)
The drift likely comes from smoothing that never converges. Fix:

1. Add convergence threshold to UpdateSmoothing():
```cpp
void UpdateSmoothing() {
    const f32 ANGLE_THRESHOLD = 0.0001f;
    const f32 POSITION_THRESHOLD = 0.001f;
    const f32 smoothFactor = 0.15f;

    // Yaw smoothing with threshold
    f32 yawDiff = yaw - smoothYaw;
    if (std::abs(yawDiff) > ANGLE_THRESHOLD) {
        smoothYaw += yawDiff * smoothFactor;
    } else {
        smoothYaw = yaw;  // Snap when close enough
    }

    // Pitch smoothing with threshold
    f32 pitchDiff = pitch - smoothPitch;
    if (std::abs(pitchDiff) > ANGLE_THRESHOLD) {
        smoothPitch += pitchDiff * smoothFactor;
    } else {
        smoothPitch = pitch;
    }

    // Position smoothing with threshold
    Vec3 posDiff = position - smoothPosition;
    if (posDiff.Length() > POSITION_THRESHOLD) {
        smoothPosition = smoothPosition + posDiff * smoothFactor;
    } else {
        smoothPosition = position;
    }
}
```

2. Add immediate sync method:
```cpp
void SyncSmoothing() {
    smoothYaw = yaw;
    smoothPitch = pitch;
    smoothPosition = position;
}
```

PHASE 3: FIX INPUT HANDLING (1.5 hours)
The HandleInput() function needs comprehensive fixes:

1. Reset velocity each frame:
```cpp
void HandleInput(f32 dt) {
    // Check if ImGui wants input first
    if (ImGui::GetCurrentContext() != nullptr) {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse || io.WantCaptureKeyboard) {
            return;  // Don't process game input when ImGui is active
        }
    }

    // Only process input when window is focused
    if (GetForegroundWindow() != m_windowHandle) {
        if (s_mouseCaptured) {
            ReleaseMouse();  // Release if we lose focus
        }
        return;
    }

    // Start with zero velocity - no residual movement
    Vec3 moveDirection(0, 0, 0);

    // ... rest of input handling
}
```

2. Fix mouse capture/release:
```cpp
// Track mouse capture state properly
static bool s_mouseCaptured = false;
static bool s_firstFrameAfterCapture = false;
static POINT s_lastMousePos = {0, 0};

void CaptureMouse() {
    if (!s_mouseCaptured) {
        s_mouseCaptured = true;
        s_firstFrameAfterCapture = true;
        ShowCursor(FALSE);

        // Store current position
        GetCursorPos(&s_lastMousePos);

        // Center cursor
        RECT rect;
        GetClientRect(m_windowHandle, &rect);
        POINT center = {(rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2};
        ClientToScreen(m_windowHandle, &center);
        SetCursorPos(center.x, center.y);
    }
}

void ReleaseMouse() {
    if (s_mouseCaptured) {
        s_mouseCaptured = false;
        ShowCursor(TRUE);
    }
}
```

3. Skip first frame after capture:
```cpp
void ProcessMouseLook(f32 sensitivity) {
    if (!s_mouseCaptured) return;

    // Skip first frame to avoid jump
    if (s_firstFrameAfterCapture) {
        s_firstFrameAfterCapture = false;
        // Re-center cursor
        RECT rect;
        GetClientRect(m_windowHandle, &rect);
        POINT center = {(rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2};
        ClientToScreen(m_windowHandle, &center);
        SetCursorPos(center.x, center.y);
        return;
    }

    // Get current cursor position
    POINT currentPos;
    GetCursorPos(&currentPos);

    // Calculate center
    RECT rect;
    GetClientRect(m_windowHandle, &rect);
    POINT center = {(rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2};
    ClientToScreen(m_windowHandle, &center);

    // Calculate delta
    f32 dx = (f32)(currentPos.x - center.x);
    f32 dy = (f32)(currentPos.y - center.y);

    // Dead zone filter
    if (std::abs(dx) < 2.0f) dx = 0.0f;
    if (std::abs(dy) < 2.0f) dy = 0.0f;

    // Clamp to prevent huge jumps
    dx = std::clamp(dx, -50.0f, 50.0f);
    dy = std::clamp(dy, -50.0f, 50.0f);

    // Apply to camera
    m_camera.yaw += dx * sensitivity;
    m_camera.pitch += dy * sensitivity;  // Note: sign depends on coordinate system

    // Clamp pitch
    m_camera.pitch = std::clamp(m_camera.pitch, -1.5f, 1.5f);

    // Re-center cursor
    SetCursorPos(center.x, center.y);
}
```

PHASE 4: CHECK FOR AUTO-MOVEMENT (30 minutes)
Search for any code that might be automatically moving the camera:
- "follow" mode
- "orbit" mode
- "auto" camera movement
- Any velocity that persists between frames

If found, ensure it's disabled or controlled by a flag.

PHASE 5: FIX KEYBOARD MOVEMENT (1 hour)
Ensure WASD only moves when keys are pressed:

```cpp
void ProcessKeyboardMovement(f32 dt) {
    Vec3 moveDir(0, 0, 0);

    // Calculate forward and right vectors from camera orientation
    Vec3 forward(
        std::sin(m_camera.yaw) * std::cos(m_camera.pitch),
        0,  // Keep movement horizontal
        std::cos(m_camera.yaw) * std::cos(m_camera.pitch)
    );
    forward = forward.Normalized();

    Vec3 right = Vec3(0, 1, 0).Cross(forward).Normalized();

    // Check movement keys
    if (GetAsyncKeyState('W') & 0x8000) moveDir = moveDir + forward;
    if (GetAsyncKeyState('S') & 0x8000) moveDir = moveDir - forward;
    if (GetAsyncKeyState('A') & 0x8000) moveDir = moveDir - right;
    if (GetAsyncKeyState('D') & 0x8000) moveDir = moveDir + right;
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) moveDir.y += 1.0f;
    if (GetAsyncKeyState('C') & 0x8000 || GetAsyncKeyState(VK_CONTROL) & 0x8000) moveDir.y -= 1.0f;

    // Only move if there's input
    if (moveDir.Length() > 0.01f) {
        moveDir = moveDir.Normalized();

        f32 speed = m_camera.speed;
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000) speed *= 2.0f;  // Sprint

        m_camera.position = m_camera.position + moveDir * speed * dt;
    }
    // If no input, position stays exactly where it is
}
```

PHASE 6: ADD CAMERA BOUNDS (30 minutes)
Prevent camera from going through terrain or too far:

```cpp
void ClampCameraPosition() {
    // Don't go below terrain
    f32 terrainHeight = m_world.terrain.GetHeightWorld(
        m_camera.position.x, m_camera.position.z);
    m_camera.position.y = std::max(m_camera.position.y, terrainHeight + 2.0f);

    // Stay within world bounds
    f32 worldSize = 150.0f;
    m_camera.position.x = std::clamp(m_camera.position.x, -worldSize, worldSize);
    m_camera.position.z = std::clamp(m_camera.position.z, -worldSize, worldSize);
    m_camera.position.y = std::clamp(m_camera.position.y, 0.0f, 200.0f);
}
```

PHASE 7: TESTING AND DOCUMENTATION (30 minutes)
Test all camera behaviors:
1. Camera stays still when no input
2. WASD moves relative to camera facing
3. Mouse look works smoothly
4. No jump on first capture
5. Shift sprint works
6. Q/E or Space/C vertical movement works

Create docs/CAMERA_SYSTEM_COMPLETE.md with:
- All fixes applied
- Control scheme documentation
- Technical details of smoothing
- Known limitations
```

---

## AGENT 5: Mouse Look Inversion and Input System Polish

```
I need you to fix the inverted mouse look and polish the entire input system in my DirectX 12 evolution simulation.

Project location: C:\Users\andre\Desktop\OrganismEvolution

CURRENT PROBLEMS:
1. Mouse look is inverted - moving mouse up looks down (and vice versa)
2. Possibly horizontal is also inverted
3. Input system may have other issues

KEY FILE:
src/main.cpp - Contains HandleInput() function and all mouse processing

COMPREHENSIVE TASKS:

PHASE 1: LOCATE MOUSE LOOK CODE (15 minutes)
Find the mouse look implementation in HandleInput():
- Search for "pitch", "yaw", "mouse", "sensitivity"
- Document the current code structure

PHASE 2: UNDERSTAND COORDINATE SYSTEMS (30 minutes)
Windows screen coordinates:
- Y increases downward (top of screen = 0)
- X increases rightward (left of screen = 0)

Camera conventions (most common):
- Positive pitch = looking UP
- Positive yaw = turning RIGHT

Therefore:
- Mouse moving UP (negative dy) should INCREASE pitch (look up)
- Mouse moving DOWN (positive dy) should DECREASE pitch (look down)

The fix depends on the current code:
```cpp
// If currently: pitch += dy * sensitivity (WRONG for standard FPS)
// Change to:   pitch -= dy * sensitivity (CORRECT)

// OR negate dy before using:
// f32 dy = center.y - currentPos.y;  // Note reversed subtraction
```

PHASE 3: FIX VERTICAL (PITCH) INVERSION (30 minutes)
The most common fix:
```cpp
// Current (inverted):
m_camera.pitch += dy * sensitivity;

// Fixed (standard FPS controls):
m_camera.pitch -= dy * sensitivity;
```

Alternative approach using negated delta:
```cpp
f32 dy = -(currentPos.y - center.y);  // Negate at source
m_camera.pitch += dy * sensitivity;
```

PHASE 4: VERIFY HORIZONTAL (YAW) (30 minutes)
Check if horizontal is correct:
- Mouse RIGHT should turn camera RIGHT (positive yaw usually)
- Mouse LEFT should turn camera LEFT (negative yaw usually)

```cpp
// Standard (usually correct):
m_camera.yaw += dx * sensitivity;

// If inverted:
m_camera.yaw -= dx * sensitivity;
```

PHASE 5: ADD INVERT OPTIONS (45 minutes)
For user preference, add invert toggles:
```cpp
// In Camera struct or config:
bool invertY = false;
bool invertX = false;

// In mouse processing:
f32 yawMultiplier = invertX ? -1.0f : 1.0f;
f32 pitchMultiplier = invertY ? -1.0f : 1.0f;

m_camera.yaw += dx * sensitivity * yawMultiplier;
m_camera.pitch -= dy * sensitivity * pitchMultiplier;
```

Add to ImGui debug panel:
```cpp
if (ImGui::CollapsingHeader("Controls")) {
    ImGui::Checkbox("Invert Y Axis", &m_camera.invertY);
    ImGui::Checkbox("Invert X Axis", &m_camera.invertX);
    ImGui::SliderFloat("Sensitivity", &m_camera.sensitivity, 0.001f, 0.01f);
}
```

PHASE 6: SENSITIVITY TUNING (30 minutes)
Current sensitivity might be too high or low:
```cpp
// Typical values:
f32 sensitivity = 0.003f;  // Low sensitivity
f32 sensitivity = 0.005f;  // Medium (recommended)
f32 sensitivity = 0.008f;  // High sensitivity

// Add adjustable range in ImGui:
ImGui::SliderFloat("Mouse Sensitivity", &m_camera.sensitivity, 0.001f, 0.015f, "%.4f");
```

PHASE 7: COMPREHENSIVE INPUT POLISH (1 hour)
Fix all input-related issues:

1. Key state tracking (for key-down vs key-held):
```cpp
static std::unordered_map<int, bool> s_keyStates;

bool IsKeyPressed(int key) {
    bool currentState = (GetAsyncKeyState(key) & 0x8000) != 0;
    bool wasPressed = s_keyStates[key];
    s_keyStates[key] = currentState;
    return currentState && !wasPressed;  // Just pressed this frame
}

bool IsKeyHeld(int key) {
    return (GetAsyncKeyState(key) & 0x8000) != 0;
}
```

2. Better mouse capture toggle:
```cpp
// Left click to capture (not hold)
static bool wasLeftPressed = false;
bool leftPressed = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
if (leftPressed && !wasLeftPressed && !s_mouseCaptured) {
    CaptureMouse();
}
wasLeftPressed = leftPressed;

// Escape to release
if (IsKeyPressed(VK_ESCAPE)) {
    if (s_mouseCaptured) {
        ReleaseMouse();
    } else {
        // Could quit application
    }
}
```

3. Focus handling:
```cpp
// Release mouse when window loses focus
if (GetForegroundWindow() != m_windowHandle) {
    if (s_mouseCaptured) {
        ReleaseMouse();
    }
    return;  // Don't process input when unfocused
}
```

4. Function key handling (F1-F12):
```cpp
// F3 toggle debug panel
if (IsKeyPressed(VK_F3)) {
    m_showDebugPanel = !m_showDebugPanel;
}

// P toggle pause
if (IsKeyPressed('P') && !ImGui::GetIO().WantCaptureKeyboard) {
    m_simulationPaused = !m_simulationPaused;
}
```

PHASE 8: DOCUMENTATION (30 minutes)
Create docs/INPUT_SYSTEM_COMPLETE.md with:
- All control bindings
- Mouse look explanation
- Sensitivity settings
- Invert options
- Code changes made
```

---

## AGENT 6: Nametag System - Complete Fix for All Creatures

```
I need you to fix the nametag system so all creatures show their nametags, not just one.

Project location: C:\Users\andre\Desktop\OrganismEvolution

CURRENT PROBLEM:
Only 1 creature shows a nametag. There should be nametags visible for nearby creatures (or all creatures within a distance).

KEY FILES:
1. src/main.cpp - Contains nametag rendering code
2. Runtime/Shaders/Nametag.hlsl - Nametag billboard shader

COMPREHENSIVE TASKS:

PHASE 1: ANALYZE CURRENT NAMETAG SYSTEM (30 minutes)
Search main.cpp for:
- "nametag" - find all references
- "billboard" - nametags are usually billboards
- "NametagVertex" or similar struct
- Nametag draw calls

Document:
- How nametags are created
- How nametag data is uploaded
- How draw calls are made

PHASE 2: IDENTIFY THE BUG (1 hour)
Common causes for only 1 nametag:

1. Loop only runs once:
```cpp
// WRONG:
DrawNametag(creatures[0]);

// RIGHT:
for (auto& creature : creatures) {
    DrawNametag(*creature);
}
```

2. Instance count is 1:
```cpp
// WRONG:
cmdList->DrawInstanced(6, 1, 0, 0);  // Only 1 instance

// RIGHT:
cmdList->DrawInstanced(6, numNametags, 0, 0);
```

3. Constant buffer overwrite:
```cpp
// WRONG - all nametags get last creature's data:
for (auto& c : creatures) {
    UpdateConstantBuffer(c->data);  // Overwrites each time
}
// Then later:
DrawAllNametags();  // All use last data

// RIGHT - update before each draw:
for (auto& c : creatures) {
    UpdateConstantBuffer(c->data);
    DrawNametag();  // Draw immediately after update
}
```

4. Buffer too small:
```cpp
// WRONG:
m_nametagBuffer = CreateBuffer(sizeof(NametagData) * 1);

// RIGHT:
m_nametagBuffer = CreateBuffer(sizeof(NametagData) * MAX_NAMETAGS);
```

PHASE 3: IMPLEMENT PROPER NAMETAG SYSTEM (2 hours)
Create a robust nametag system:

1. Define constants:
```cpp
static constexpr u32 MAX_NAMETAGS = 200;
static constexpr f32 NAMETAG_MAX_DISTANCE = 50.0f;
```

2. Create nametag constant structure:
```cpp
// Must be 256-byte aligned for DX12 CBV
struct alignas(256) NametagConstants {
    Mat4 model;           // 64 bytes
    Vec4 color;           // 16 bytes
    Vec4 textData;        // 16 bytes (character codes, etc.)
    f32 padding[40];      // Pad to 256 bytes
};
```

3. Create per-frame nametag buffers:
```cpp
// In initialization:
for (u32 i = 0; i < NUM_FRAMES_IN_FLIGHT; i++) {
    m_nametagCB[i] = m_device->CreateBuffer(
        sizeof(NametagConstants) * MAX_NAMETAGS,
        BufferUsage::Constant,
        MemoryType::Upload
    );
}
```

4. Render nametags properly:
```cpp
void RenderNametags(ICommandList* cmdList, SimulationWorld& world, Camera& camera) {
    // Collect visible nametags
    std::vector<NametagConstants> nametags;
    nametags.reserve(MAX_NAMETAGS);

    for (auto& creature : world.creatures) {
        if (!creature->alive) continue;

        // Distance check
        Vec3 toCreature = creature->position - camera.position;
        f32 distance = toCreature.Length();
        if (distance > NAMETAG_MAX_DISTANCE) continue;

        // Create nametag data
        NametagConstants nt;

        // Billboard matrix (always faces camera)
        Vec3 pos = creature->position + Vec3(0, creature->genome.size * 2.0f, 0);
        nt.model = CreateBillboardMatrix(pos, camera);

        // Color based on creature type
        if (creature->type == CreatureType::HERBIVORE) {
            nt.color = Vec4(0.3f, 0.8f, 0.3f, 1.0f);  // Green
        } else {
            nt.color = Vec4(0.9f, 0.3f, 0.3f, 1.0f);  // Red
        }

        // ID for text display
        nt.textData = Vec4((f32)creature->id, (f32)creature->generation, 0, 0);

        nametags.push_back(nt);
        if (nametags.size() >= MAX_NAMETAGS) break;
    }

    if (nametags.empty()) return;

    // Upload all nametag data at once
    void* mapped = m_nametagCB[m_frameIndex]->Map();
    memcpy(mapped, nametags.data(), sizeof(NametagConstants) * nametags.size());
    m_nametagCB[m_frameIndex]->Unmap();

    // Bind pipeline
    cmdList->BindPipeline(m_nametagPipeline.get());
    cmdList->BindVertexBuffer(m_nametagVB.get());
    cmdList->BindIndexBuffer(m_nametagIB.get());

    // Draw each nametag with offset into constant buffer
    for (u32 i = 0; i < nametags.size(); i++) {
        cmdList->BindConstantBuffer(0, m_nametagCB[m_frameIndex].get(),
                                   i * sizeof(NametagConstants));
        cmdList->DrawIndexed(6, 1, 0, 0, 0);
    }
}
```

5. Billboard matrix helper:
```cpp
Mat4 CreateBillboardMatrix(const Vec3& position, const Camera& camera) {
    // Get camera right and up vectors
    Vec3 forward = (camera.position - position).Normalized();
    Vec3 right = Vec3(0, 1, 0).Cross(forward).Normalized();
    Vec3 up = forward.Cross(right).Normalized();

    // Build billboard rotation matrix
    Mat4 rotation = Mat4::Identity();
    rotation.m[0][0] = right.x;  rotation.m[0][1] = up.x;  rotation.m[0][2] = forward.x;
    rotation.m[1][0] = right.y;  rotation.m[1][1] = up.y;  rotation.m[1][2] = forward.y;
    rotation.m[2][0] = right.z;  rotation.m[2][1] = up.z;  rotation.m[2][2] = forward.z;

    // Apply position
    return Mat4::Translation(position) * rotation * Mat4::Scale(Vec3(0.5f));
}
```

PHASE 4: UPDATE NAMETAG SHADER (1 hour)
Ensure Nametag.hlsl handles the data correctly:

```hlsl
// Nametag.hlsl
cbuffer NametagConstants : register(b0) {
    float4x4 model;
    float4 color;
    float4 textData;  // x = creature ID
};

struct VSInput {
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR;
    float creatureID : TEXCOORD1;
};

PSInput VSMain(VSInput input) {
    PSInput output;

    float4 worldPos = mul(model, float4(input.position, 1.0));
    output.position = mul(viewProjection, worldPos);
    output.texCoord = input.texCoord;
    output.color = color;
    output.creatureID = textData.x;

    return output;
}

float4 PSMain(PSInput input) : SV_TARGET {
    // Simple colored quad with slight transparency at edges
    float alpha = 1.0 - length(input.texCoord - 0.5) * 1.5;
    alpha = saturate(alpha);

    return float4(input.color.rgb, alpha);
}
```

PHASE 5: ADD RHI SUPPORT FOR OFFSET BINDING (30 minutes)
If the RHI doesn't support binding at offset, add it:

```cpp
// In RHI.h or similar:
virtual void BindConstantBuffer(u32 slot, IBuffer* buffer, u32 offset = 0) = 0;

// In DX12 implementation:
void DX12CommandList::BindConstantBuffer(u32 slot, IBuffer* buffer, u32 offset) {
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = buffer->GetGPUVirtualAddress() + offset;
    m_cmdList->SetGraphicsRootConstantBufferView(slot, gpuAddress);
}
```

PHASE 6: TESTING AND DOCUMENTATION (30 minutes)
Test:
1. Multiple nametags visible
2. Nametags face camera correctly
3. Colors distinguish herbivores/carnivores
4. Distance culling works
5. Performance with many nametags

Create docs/NAMETAG_SYSTEM_COMPLETE.md
```

---

## AGENT 7: Lighting System Review and Enhancement

```
I need you to review and potentially enhance the lighting system in my DirectX 12 evolution simulation. The user mentioned some flickering, though it may be external.

Project location: C:\Users\andre\Desktop\OrganismEvolution

NOTE: User suspects lighting flicker might be from another application. Focus on reviewing the code for correctness and making minor improvements.

KEY FILES:
1. Runtime/Shaders/Terrain.hlsl - Terrain PBR lighting
2. Runtime/Shaders/Creature.hlsl - Creature lighting
3. src/core/DayNightCycle.h - Day/night cycle management
4. src/main.cpp - Time management, constant buffer updates

COMPREHENSIVE REVIEW TASKS:

PHASE 1: TIME VALUE REVIEW (30 minutes)
Check for floating-point precision issues:

1. In main.cpp, verify time wrapping:
```cpp
// Should wrap to prevent precision loss
m_time += dt;
if (m_time > 1000.0f) {
    m_time -= 1000.0f;
}
```

2. In DayNightCycle.h, verify dayTime stays in 0-1:
```cpp
dayTime += dt / dayLengthSeconds;
if (dayTime > 1.0f) {
    dayTime -= 1.0f;
}
```

PHASE 2: LIGHT POSITION STABILITY (30 minutes)
Verify sun/moon position calculation is smooth:
```cpp
// In DayNightCycle, sun position should use smooth functions
Vec3 GetSunDirection() const {
    float angle = dayTime * 2.0f * PI;  // 0 to 2π over day
    return Vec3(
        cos(angle),
        sin(angle),  // Smooth sine wave for height
        0.0f
    ).Normalized();
}
```

No discontinuities or jumps should occur.

PHASE 3: SHADER NUMERICAL STABILITY (1 hour)
Check both shaders for potential NaN/Inf:

1. Division by zero protection:
```hlsl
// All divisions should have epsilon
float denom = someValue + 0.0001;
result = numerator / denom;
```

2. Square root of negative protection:
```hlsl
float sqrtValue = sqrt(max(0.0, someValue));
```

3. Normalize only non-zero vectors:
```hlsl
float len = length(vec);
if (len > 0.001) {
    vec = vec / len;
}
```

4. Clamp inputs to valid ranges:
```hlsl
float fresnel = pow(saturate(1.0 - NdotV), 5.0);
```

PHASE 4: PBR LIGHTING REVIEW (1 hour)
Verify PBR implementation is correct:

1. GGX Distribution:
```hlsl
float DistributionGGX(float3 N, float3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / max(denom, 0.0001);  // Protect division
}
```

2. Geometry Smith:
```hlsl
float GeometrySmith(float3 N, float3 V, float3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}
```

3. Fresnel Schlick:
```hlsl
float3 fresnelSchlick(float cosTheta, float3 F0) {
    return F0 + (1.0 - F0) * pow(saturate(1.0 - cosTheta), 5.0);
}
```

PHASE 5: AMBIENT LIGHTING REVIEW (30 minutes)
Ensure minimum visibility:

```hlsl
// In both shaders, ensure minimum ambient
float3 ambient = ambientColor * 0.3;  // Base ambient
ambient = max(ambient, float3(0.15, 0.15, 0.15));  // Floor

// Final color should never be pure black
result = max(result, float3(0.05, 0.05, 0.05));
```

PHASE 6: DAY/NIGHT TRANSITION SMOOTHNESS (30 minutes)
Review DayNightCycle transitions:

```cpp
// Transitions should use smooth interpolation
Vec3 LerpVec3(const Vec3& a, const Vec3& b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return Vec3(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    );
}

// Use smoothstep for phase transitions
float smoothPhaseBlend = t * t * (3.0f - 2.0f * t);
```

PHASE 7: CONSTANT BUFFER ALIGNMENT (30 minutes)
Verify constant buffer layout matches between C++ and HLSL:

```cpp
// C++ struct (must match HLSL exactly):
struct Constants {
    Mat4 view;             // 64 bytes
    Mat4 projection;       // 64 bytes
    Mat4 viewProjection;   // 64 bytes
    Vec3 viewPos;          // 12 bytes
    f32 time;              // 4 bytes (packs with viewPos)
    Vec3 lightPos;         // 12 bytes
    f32 padding1;          // 4 bytes
    Vec3 lightColor;       // 12 bytes
    f32 padding2;          // 4 bytes
    // Day/night data...
};
// Total must be multiple of 16 bytes
```

PHASE 8: MINOR IMPROVEMENTS (30 minutes)
Add any helpful improvements found:

1. Subsurface scattering for creatures:
```hlsl
float3 calculateSSS(float3 N, float3 L, float3 V, float3 albedo) {
    float3 H = normalize(L + N * 0.5);
    float VdotH = saturate(dot(V, -H));
    return albedo * pow(VdotH, 3.0) * 0.15;
}
```

2. Better ambient occlusion approximation:
```hlsl
float ao = saturate(normal.y * 0.5 + 0.5);
ambient *= 0.5 + ao * 0.5;
```

PHASE 9: DOCUMENTATION (30 minutes)
Create docs/LIGHTING_SYSTEM_REVIEW.md with:
- Review findings
- Any issues found and fixed
- Stability improvements
- Note about external flicker possibility
```

---

## AGENT 8: ImGui Panel Click-Through Fix and UI Enhancement

```
I need you to fix the ImGui debug panel so clicks on it don't pass through to the game, and enhance the UI.

Project location: C:\Users\andre\Desktop\OrganismEvolution

CURRENT PROBLEM:
Clicking on the ImGui debug panel (F3) causes the clicks to pass through to the game, triggering mouse capture instead of interacting with the UI controls.

KEY FILE:
src/main.cpp - Contains ImGui initialization and input handling

COMPREHENSIVE TASKS:

PHASE 1: UNDERSTAND THE PROBLEM (30 minutes)
The issue is that HandleInput() uses GetAsyncKeyState() which polls hardware directly, bypassing ImGui's message system.

ImGui tracks whether it wants to capture input via:
```cpp
ImGuiIO& io = ImGui::GetIO();
io.WantCaptureMouse;    // True when mouse is over ImGui window
io.WantCaptureKeyboard; // True when ImGui has keyboard focus
```

PHASE 2: FIX INPUT HANDLING (1 hour)
Add ImGui capture checks at the start of HandleInput():

```cpp
void HandleInput(f32 dt) {
    // CRITICAL: Check if ImGui wants input FIRST
    bool imguiWantsMouse = false;
    bool imguiWantsKeyboard = false;

    if (ImGui::GetCurrentContext() != nullptr) {
        ImGuiIO& io = ImGui::GetIO();
        imguiWantsMouse = io.WantCaptureMouse;
        imguiWantsKeyboard = io.WantCaptureKeyboard;
    }

    // Mouse handling - only if ImGui doesn't want it
    if (!imguiWantsMouse) {
        ProcessMouseInput();
    }

    // Keyboard handling - only if ImGui doesn't want it
    if (!imguiWantsKeyboard) {
        ProcessKeyboardInput(dt);
    }

    // Function keys can work even with ImGui focus (F3 toggle)
    ProcessFunctionKeys();
}
```

PHASE 3: SEPARATE INPUT FUNCTIONS (1 hour)
Refactor input handling into clear functions:

```cpp
void ProcessMouseInput() {
    // Mouse capture/release
    static bool wasLeftPressed = false;
    bool leftPressed = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

    if (leftPressed && !wasLeftPressed && !s_mouseCaptured) {
        CaptureMouse();
    }
    wasLeftPressed = leftPressed;

    // Mouse look (only when captured)
    if (s_mouseCaptured) {
        ProcessMouseLook();
    }
}

void ProcessKeyboardInput(f32 dt) {
    // Movement keys
    Vec3 moveDir(0, 0, 0);

    if (GetAsyncKeyState('W') & 0x8000) moveDir.z += 1.0f;
    if (GetAsyncKeyState('S') & 0x8000) moveDir.z -= 1.0f;
    if (GetAsyncKeyState('A') & 0x8000) moveDir.x -= 1.0f;
    if (GetAsyncKeyState('D') & 0x8000) moveDir.x += 1.0f;
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) moveDir.y += 1.0f;
    if (GetAsyncKeyState('C') & 0x8000) moveDir.y -= 1.0f;

    // Apply movement...

    // Pause toggle
    static bool wasPPressed = false;
    bool pPressed = (GetAsyncKeyState('P') & 0x8000) != 0;
    if (pPressed && !wasPPressed) {
        m_renderer.m_simulationPaused = !m_renderer.m_simulationPaused;
    }
    wasPPressed = pPressed;
}

void ProcessFunctionKeys() {
    // F3 toggle debug panel (works even when ImGui has focus)
    static bool wasF3Pressed = false;
    bool f3Pressed = (GetAsyncKeyState(VK_F3) & 0x8000) != 0;
    if (f3Pressed && !wasF3Pressed) {
        m_renderer.m_showDebugPanel = !m_renderer.m_showDebugPanel;
    }
    wasF3Pressed = f3Pressed;

    // Escape to release mouse or close
    static bool wasEscPressed = false;
    bool escPressed = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
    if (escPressed && !wasEscPressed) {
        if (s_mouseCaptured) {
            ReleaseMouse();
        }
    }
    wasEscPressed = escPressed;
}
```

PHASE 4: VERIFY IMGUI WNDPROC HANDLER (30 minutes)
Ensure ImGui message handler is called in WndProc:

```cpp
// In WindowsWindow.cpp or equivalent:
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // ImGui MUST handle messages FIRST
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return true;  // ImGui consumed the message
    }

    // Handle other messages...
    switch (msg) {
        case WM_CLOSE:
            // ...
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}
```

PHASE 5: ENHANCE DEBUG PANEL (1 hour)
Add more useful controls to the ImGui panel:

```cpp
void RenderDebugPanel(SimulationWorld& world, Camera& camera, f32 time) {
    if (!m_showDebugPanel) return;

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);

    ImGui::Begin("Simulation Debug (F3)", &m_showDebugPanel);

    // Performance
    if (ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
        ImGui::Text("Creatures: %zu", world.creatures.size());
        ImGui::Text("Food: %zu", world.food.size());
    }

    // Camera Controls
    if (ImGui::CollapsingHeader("Camera")) {
        ImGui::SliderFloat("Move Speed", &camera.speed, 10.0f, 200.0f);
        ImGui::SliderFloat("Sensitivity", &camera.sensitivity, 0.001f, 0.01f, "%.4f");
        ImGui::Checkbox("Invert Y", &camera.invertY);

        if (ImGui::Button("Reset Camera")) {
            camera.position = Vec3(0, 50, 100);
            camera.yaw = 0;
            camera.pitch = -0.3f;
        }
    }

    // Simulation Controls
    if (ImGui::CollapsingHeader("Simulation", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Paused", &m_simulationPaused);
        ImGui::SliderFloat("Speed", &m_simulationSpeed, 0.1f, 10.0f, "%.1fx");

        ImGui::Separator();

        if (ImGui::Button("Spawn 10 Herbivores")) {
            world.SpawnCreatures(CreatureType::HERBIVORE, 10);
        }
        ImGui::SameLine();
        if (ImGui::Button("Spawn 5 Carnivores")) {
            world.SpawnCreatures(CreatureType::CARNIVORE, 5);
        }
    }

    // Day/Night
    if (ImGui::CollapsingHeader("Day/Night Cycle")) {
        ImGui::SliderFloat("Time", &world.dayNight.dayTime, 0.0f, 1.0f);
        ImGui::SliderFloat("Day Length", &world.dayNight.dayLengthSeconds, 30.0f, 600.0f);
        ImGui::Checkbox("Pause Cycle", &world.dayNight.paused);

        if (ImGui::Button("Dawn")) world.dayNight.dayTime = 0.25f;
        ImGui::SameLine();
        if (ImGui::Button("Noon")) world.dayNight.dayTime = 0.5f;
        ImGui::SameLine();
        if (ImGui::Button("Dusk")) world.dayNight.dayTime = 0.75f;
        ImGui::SameLine();
        if (ImGui::Button("Night")) world.dayNight.dayTime = 0.0f;
    }

    // Graphics
    if (ImGui::CollapsingHeader("Graphics")) {
        ImGui::Checkbox("Show Nametags", &m_showNametags);
        ImGui::Checkbox("Show Trees", &m_showTrees);
        ImGui::SliderFloat("Nametag Distance", &m_nametagMaxDistance, 10.0f, 100.0f);
    }

    ImGui::End();
}
```

PHASE 6: DOCUMENTATION (30 minutes)
Create docs/IMGUI_INTEGRATION.md with:
- How ImGui input capture works
- Fixes applied
- All debug panel controls
- Keyboard shortcuts
```

---

## AGENT 9: Camera-Creature Position Sync Fix

```
I need you to fix the issue where camera jerks seem to affect creature positions visually.

Project location: C:\Users\andre\Desktop\OrganismEvolution

CURRENT PROBLEM:
When the camera makes sudden movements, creature positions appear to shift or jump along with it. This is a visual artifact, not actual position change.

KEY FILE:
src/main.cpp - Contains camera and rendering code

COMPREHENSIVE INVESTIGATION:

PHASE 1: UNDERSTAND THE VISUAL ARTIFACT (30 minutes)
This happens when:
1. View matrix uses different camera position than lighting calculations
2. Frame timing causes desync between camera update and creature render
3. Smoothing interpolation uses different values at different points

PHASE 2: CHECK VIEW MATRIX CONSISTENCY (1 hour)
Find all places where camera position is used:

1. View matrix calculation:
```cpp
Mat4 viewMatrix = camera.GetViewMatrix();  // Uses smoothPosition
```

2. viewPos in constant buffer (for lighting):
```cpp
frameConstants.viewPos = camera.position;  // WRONG - uses raw position
```

The fix:
```cpp
// Use the SAME position for both view matrix AND viewPos uniform
Vec3 cameraPos = camera.GetSmoothedPosition();
frameConstants.view = Mat4::LookAt(cameraPos, cameraPos + front, up);
frameConstants.viewPos = cameraPos;  // Must match view matrix source
```

PHASE 3: VERIFY UPDATE ORDER (30 minutes)
The main loop should be:
```cpp
while (running) {
    // 1. Poll events (Windows messages)
    m_window->PollEvents();

    // 2. Handle input (updates camera.position/yaw/pitch)
    HandleInput(dt);

    // 3. Update simulation (creature positions)
    Update(dt);

    // 4. Update camera smoothing BEFORE rendering
    m_camera.UpdateSmoothing();

    // 5. Render using smoothed camera values
    Render();
}
```

PHASE 4: FIX CAMERA SMOOTHING (1 hour)
Add GetSmoothedPosition() if not present:

```cpp
class Camera {
public:
    Vec3 position;       // Target position (set by input)
    Vec3 smoothPosition; // Interpolated position (used for rendering)
    f32 yaw, pitch;
    f32 smoothYaw, smoothPitch;

    void UpdateSmoothing() {
        const f32 factor = 0.15f;

        // Smooth position
        Vec3 posDiff = position - smoothPosition;
        if (posDiff.Length() > 0.001f) {
            smoothPosition = smoothPosition + posDiff * factor;
        } else {
            smoothPosition = position;
        }

        // Smooth angles
        f32 yawDiff = yaw - smoothYaw;
        if (std::abs(yawDiff) > 0.0001f) {
            smoothYaw += yawDiff * factor;
        } else {
            smoothYaw = yaw;
        }

        // Same for pitch...
    }

    Vec3 GetSmoothedPosition() const { return smoothPosition; }

    Mat4 GetViewMatrix() const {
        Vec3 front = GetForwardVector(smoothYaw, smoothPitch);
        return Mat4::LookAt(smoothPosition, smoothPosition + front, Vec3(0, 1, 0));
    }
};
```

PHASE 5: FIX CONSTANT BUFFER UPDATE (1 hour)
Ensure all rendering uses consistent camera position:

```cpp
void Render() {
    // Update camera smoothing FIRST
    m_camera.UpdateSmoothing();

    // Get smoothed position for ALL rendering
    Vec3 cameraPos = m_camera.GetSmoothedPosition();

    // Build matrices
    Mat4 view = m_camera.GetViewMatrix();  // Uses smoothed internally
    Mat4 proj = Mat4::Perspective(m_camera.zoom, aspectRatio, 0.1f, 500.0f);
    Mat4 viewProj = proj * view;

    // Update constant buffer
    Constants constants;
    constants.view = view;
    constants.projection = proj;
    constants.viewProjection = viewProj;
    constants.viewPos = cameraPos;  // CRITICAL: Same as view matrix source
    constants.time = m_time;

    // Light position relative to camera
    constants.lightPos = cameraPos + Vec3(50, 100, 50);  // Or from day/night

    // Upload and render...
}
```

PHASE 6: CHECK CREATURE RENDERING (30 minutes)
Verify creatures don't reference camera position inconsistently:

```cpp
void RenderCreatures(ICommandList* cmdList, const Mat4& viewProj, const Vec3& camPos) {
    for (auto& creature : creatures) {
        // Creature position is independent of camera
        Mat4 model = creature->GetModelMatrix();

        // Upload to constant buffer
        constants.model = model;
        constants.viewProjection = viewProj;
        constants.viewPos = camPos;  // Consistent camera position

        // Draw...
    }
}
```

PHASE 7: ADD DEBUG VISUALIZATION (30 minutes)
Optional: Add debug output to verify sync:

```cpp
// In ImGui panel or console:
ImGui::Text("Camera Raw: (%.1f, %.1f, %.1f)",
            camera.position.x, camera.position.y, camera.position.z);
ImGui::Text("Camera Smooth: (%.1f, %.1f, %.1f)",
            camera.smoothPosition.x, camera.smoothPosition.y, camera.smoothPosition.z);
ImGui::Text("ViewPos Uniform: (%.1f, %.1f, %.1f)",
            lastConstants.viewPos.x, lastConstants.viewPos.y, lastConstants.viewPos.z);
```

PHASE 8: DOCUMENTATION (30 minutes)
Create docs/CAMERA_CREATURE_SYNC.md with:
- Root cause explanation
- All fixes applied
- Update order diagram
- Testing verification
```

---

## AGENT 10: Final Integration, Testing, and Comprehensive Documentation

```
I need you to perform final integration testing and create comprehensive documentation for all the fixes applied to my DirectX 12 evolution simulation.

Project location: C:\Users\andre\Desktop\OrganismEvolution

YOUR COMPREHENSIVE TASKS:

PHASE 1: READ ALL AGENT DOCUMENTATION (1 hour)
Read these documentation files if they exist:
- docs/WATER_SYSTEM_COMPREHENSIVE.md (Agent 1)
- docs/CREATURE_VISIBILITY_INVESTIGATION.md (Agent 2)
- docs/VEGETATION_SYSTEM_COMPLETE.md (Agent 3)
- docs/CAMERA_SYSTEM_COMPLETE.md (Agent 4)
- docs/INPUT_SYSTEM_COMPLETE.md (Agent 5)
- docs/NAMETAG_SYSTEM_COMPLETE.md (Agent 6)
- docs/LIGHTING_SYSTEM_REVIEW.md (Agent 7)
- docs/IMGUI_INTEGRATION.md (Agent 8)
- docs/CAMERA_CREATURE_SYNC.md (Agent 9)

Document which files exist and summarize their contents.

PHASE 2: CODE REVIEW (1 hour)
Review the key files for obvious issues:

1. src/main.cpp - Check for:
   - Compilation errors
   - Obvious logic bugs
   - Missing function implementations
   - Inconsistent naming

2. Runtime/Shaders/Terrain.hlsl - Check for:
   - HLSL syntax errors
   - Matching constant buffer layout
   - Proper function signatures

3. Runtime/Shaders/Creature.hlsl - Same checks

4. Runtime/Shaders/Nametag.hlsl - Same checks

5. Runtime/Shaders/Vegetation.hlsl - Same checks

PHASE 3: CREATE TESTING CHECKLIST (1 hour)
Create docs/TESTING_CHECKLIST.md with step-by-step tests:

```markdown
# Testing Checklist

## Water System
- [ ] Water covers appropriate amount of terrain (not too much)
- [ ] Waves are subtle and realistic
- [ ] Water-to-beach transition is smooth
- [ ] Water has specular highlights from sun
- [ ] Water is darker at depth

## Creature Visibility
- [ ] Creatures visible on grass terrain
- [ ] Creatures visible on beach
- [ ] Creatures visible on mountain/rock
- [ ] Creatures visible in water areas
- [ ] Creatures have proper colors (green herbivores, red carnivores)
- [ ] Creatures are not all magenta (debug mode disabled)

## Vegetation
- [ ] Grass has visible detail patterns
- [ ] Trees are visible in forest areas
- [ ] Trees have different types (oak, pine, willow)
- [ ] Vegetation doesn't flicker

## Camera Controls
- [ ] WASD moves camera in correct directions
- [ ] W moves forward (in look direction)
- [ ] S moves backward
- [ ] A moves left
- [ ] D moves right
- [ ] Space moves up
- [ ] C/Ctrl moves down
- [ ] Shift enables sprint (2x speed)
- [ ] Camera stays still when no input

## Mouse Look
- [ ] Mouse up looks UP (not inverted)
- [ ] Mouse right turns RIGHT
- [ ] Left click captures mouse
- [ ] Escape releases mouse
- [ ] No jump on first capture
- [ ] Smooth mouse movement

## Nametags
- [ ] Multiple creatures show nametags
- [ ] Nametags face camera (billboard)
- [ ] Herbivore nametags are green
- [ ] Carnivore nametags are red
- [ ] Distant nametags are culled

## ImGui Panel
- [ ] F3 toggles panel visibility
- [ ] Clicking panel doesn't capture mouse
- [ ] Sliders work correctly
- [ ] Buttons spawn creatures
- [ ] Pause/unpause works

## Lighting
- [ ] Day/night cycle works
- [ ] No flickering (except external causes)
- [ ] Terrain properly lit
- [ ] Creatures properly lit
- [ ] Minimum visibility maintained

## General
- [ ] Application starts without crash
- [ ] Stable 60 FPS
- [ ] No visual artifacts
- [ ] Creatures move and evolve
- [ ] Population grows over time
```

PHASE 4: CREATE CONTROLS DOCUMENTATION (30 minutes)
Create docs/CONTROLS.md:

```markdown
# Evolution Simulation Controls

## Camera Movement
| Key | Action |
|-----|--------|
| W | Move forward |
| S | Move backward |
| A | Move left |
| D | Move right |
| Space | Move up |
| C / Ctrl | Move down |
| Shift | Sprint (2x speed) |

## Mouse
| Action | Effect |
|--------|--------|
| Left Click | Capture mouse for look |
| Mouse Move | Look around (when captured) |
| Escape | Release mouse |

## Function Keys
| Key | Action |
|-----|--------|
| F3 | Toggle debug panel |
| P | Pause/unpause simulation |

## Debug Panel Controls
- Simulation speed slider
- Day/night time slider
- Spawn buttons for creatures
- Camera settings
```

PHASE 5: CREATE KNOWN ISSUES DOCUMENT (30 minutes)
Create docs/KNOWN_ISSUES.md:

```markdown
# Known Issues

## Active Issues
1. **First-frame mouse jump** - Sometimes mouse capture causes a small jump. Mostly mitigated.
2. **External lighting flicker** - If running other graphics-intensive applications, minor flicker may occur.

## Resolved Issues
1. Water too high - Fixed by adjusting WATER_THRESH
2. Creatures invisible on land - Fixed depth test and Y offset
3. Camera drift - Fixed smoothing convergence
4. Mouse inversion - Fixed pitch direction
5. Only 1 nametag - Fixed constant buffer race condition
6. ImGui click-through - Fixed input capture checks

## Workarounds
- If creatures appear magenta, check Creature.hlsl for debug mode
- If camera drifts, try pressing Escape and recapturing
```

PHASE 6: CREATE MASTER SUMMARY (1 hour)
Create docs/PHASE2_SUMMARY.md:

```markdown
# Phase 2 Fixes Summary

## Overview
This document summarizes all fixes applied to the DirectX 12 Evolution Simulation.

## Agents and Their Work

### Agent 1: Water System
- Lowered WATER_THRESH from 0.05 to 0.02
- Reduced wave height from 0.03 to 0.008
- Added realistic water rendering with fresnel, depth coloring, foam

### Agent 2: Creature Visibility
- Changed depth test from Less to LessEqual
- Added Y position offset (+0.5) above terrain
- Verified instance buffer data integrity

### Agent 3: Vegetation System
- Enhanced grass color generation
- Implemented tree rendering with multiple types
- Added forest biome transitions

### Agent 4: Camera System
- Fixed smoothing convergence thresholds
- Added proper dead zone filtering
- Implemented sprint and vertical movement

### Agent 5: Mouse Look
- Fixed inverted Y axis (pitch)
- Added sensitivity controls
- Added invert options

### Agent 6: Nametag System
- Fixed constant buffer race condition
- Implemented per-creature nametag rendering
- Added distance culling

### Agent 7: Lighting Review
- Verified numerical stability
- Confirmed time wrapping
- Minor ambient improvements

### Agent 8: ImGui Integration
- Fixed input capture checking
- Enhanced debug panel
- Added more controls

### Agent 9: Camera-Creature Sync
- Fixed viewPos/view matrix mismatch
- Ensured consistent smoothed position usage

### Agent 10: Documentation
- Created testing checklist
- Created controls documentation
- Compiled all agent work

## Files Modified
- src/main.cpp (~4500 lines)
- Runtime/Shaders/Terrain.hlsl
- Runtime/Shaders/Creature.hlsl
- Runtime/Shaders/Nametag.hlsl
- Runtime/Shaders/Vegetation.hlsl
- ForgeEngine/RHI/Public/RHI/RHI.h
- ForgeEngine/RHI/DX12/DX12Device.cpp

## Build Instructions
1. Open OrganismEvolution.sln in Visual Studio 2022
2. Build in Debug or Release configuration
3. Run the application
4. Press F3 to toggle debug panel
```

PHASE 7: FINAL VERIFICATION (30 minutes)
- List any remaining TODO comments in code
- Check for any obvious compilation issues
- Verify all documentation files are created
- Create a final status report
```

---

# How to Use These Prompts

1. **Copy one prompt** (everything between the triple backticks)
2. **Open a new Claude chat session**
3. **Paste the prompt** as your first message
4. **Let Claude work** - it will read files, make changes, spawn sub-agents, and create documentation
5. **Repeat** for each of the 10 agents

Each agent is designed to work independently for extended periods and create comprehensive documentation of their work.
