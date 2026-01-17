# Phase 4 Agent Prompts - Integration Fixes & Enhancements

These prompts address remaining issues from the Phase 3 Integration Audit plus new enhancements.

---

## Agent 1: Replay System Integration (3-4 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Wire the existing ReplaySystem into main.cpp so recording and playback actually work.

ISSUES TO FIX: S-05, S-06, S-07, S-08

============================================================================
PHASE 1: ANALYZE EXISTING SYSTEM (30+ minutes)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Analyze ReplaySystem:
- Read src/core/ReplaySystem.h thoroughly
- Document ReplayRecorder class API
- Document ReplayPlayer class API
- Identify what data is captured per frame
- Note: Neural weights NOT recorded (S-07) - design fix

SUB-AGENT 1B - Analyze main.cpp integration points:
- Read src/main.cpp
- Find the main update loop
- Find where creatures/food are updated
- Identify where recording should be called
- Find UI integration points for playback controls

CHECKPOINT 1: Document integration plan in docs/REPLAY_INTEGRATION_PLAN.md

============================================================================
PHASE 2: IMPLEMENT RECORDING (1+ hour)
============================================================================

Wire ReplayRecorder into main.cpp:

1. Add member variable:
```cpp
#include "ReplaySystem.h"
ReplayRecorder m_replayRecorder;
bool m_isRecording = true;  // Auto-start recording
```

2. In initialization:
```cpp
m_replayRecorder.startRecording();
m_replayRecorder.setRecordingInterval(1.0f);  // 1 frame per second
```

3. In main update loop (after creature updates):
```cpp
if (m_isRecording) {
    // Build frame snapshot
    ReplayFrame frame;
    frame.timestamp = m_simulationTime;

    for (const auto& creature : m_creatures) {
        CreatureSnapshot snap;
        snap.id = creature->id;
        snap.position = creature->position;
        snap.rotation = creature->rotation;
        snap.health = creature->health;
        snap.energy = creature->energy;
        snap.animationState = creature->currentAnimation;
        snap.neuralWeights = creature->brain->getWeights();  // FIX S-07
        frame.creatures.push_back(snap);
    }

    for (const auto& food : m_food) {
        FoodSnapshot fsnap;
        fsnap.position = food.position;
        fsnap.energy = food.energy;
        fsnap.isActive = food.isActive;
        frame.food.push_back(fsnap);
    }

    frame.camera = getCameraState();
    m_replayRecorder.recordFrame(frame);
}
```

4. Fix circular buffer inefficiency (S-08):
In ReplaySystem.h, change:
```cpp
// Before: m_frames.erase(m_frames.begin());  // O(n)
// After: Use ring buffer index
size_t m_writeIndex = 0;
void addFrame(const ReplayFrame& frame) {
    m_frames[m_writeIndex] = frame;
    m_writeIndex = (m_writeIndex + 1) % m_maxFrames;
}
```

CHECKPOINT 2: Recording works - verify with debug output

============================================================================
PHASE 3: IMPLEMENT PLAYBACK (1+ hour)
============================================================================

Wire ReplayPlayer into main.cpp:

1. Add member variables:
```cpp
ReplayPlayer m_replayPlayer;
bool m_isPlayingReplay = false;
```

2. Add playback mode toggle:
```cpp
void enterReplayMode() {
    m_isPlayingReplay = true;
    m_isRecording = false;
    m_replayPlayer.loadFromRecorder(m_replayRecorder);
    m_replayPlayer.play();
}

void exitReplayMode() {
    m_isPlayingReplay = false;
    m_isRecording = true;
    m_replayPlayer.stop();
}
```

3. In update loop, add replay branch:
```cpp
if (m_isPlayingReplay) {
    m_replayPlayer.update(deltaTime);
    const ReplayFrame& frame = m_replayPlayer.getCurrentFrame();

    // Update creature visuals from replay data
    for (const auto& snap : frame.creatures) {
        // Find creature or create ghost
        updateCreatureFromSnapshot(snap);
    }

    // Interpolate between frames for smooth playback
    float t = m_replayPlayer.getInterpolationFactor();
    interpolateCreaturePositions(t);
} else {
    // Normal simulation update
    updateSimulation(deltaTime);
}
```

CHECKPOINT 3: Playback works with interpolation

============================================================================
PHASE 4: UI INTEGRATION (45+ minutes)
============================================================================

Add replay controls to ImGui dashboard:

1. Recording indicator:
```cpp
if (m_isRecording) {
    ImGui::TextColored(ImVec4(1,0,0,1), "● REC");
    ImGui::SameLine();
    ImGui::Text("%.1f min", m_replayRecorder.getDuration() / 60.0f);
}
```

2. Replay controls panel:
```cpp
void renderReplayControls() {
    ImGui::Begin("Replay Controls");

    if (!m_isPlayingReplay) {
        if (ImGui::Button("Enter Replay Mode")) enterReplayMode();
    } else {
        // Timeline scrubber
        float progress = m_replayPlayer.getProgress();
        if (ImGui::SliderFloat("Timeline", &progress, 0.0f, 1.0f)) {
            m_replayPlayer.seek(progress);
        }

        // Playback controls
        if (ImGui::Button(m_replayPlayer.isPaused() ? "Play" : "Pause")) {
            m_replayPlayer.togglePause();
        }
        ImGui::SameLine();

        // Speed control
        float speed = m_replayPlayer.getSpeed();
        if (ImGui::SliderFloat("Speed", &speed, 0.25f, 4.0f)) {
            m_replayPlayer.setSpeed(speed);
        }

        // Frame stepping
        if (ImGui::Button("<<")) m_replayPlayer.stepBackward();
        ImGui::SameLine();
        if (ImGui::Button(">>")) m_replayPlayer.stepForward();

        if (ImGui::Button("Exit Replay")) exitReplayMode();
    }

    ImGui::End();
}
```

3. Keyboard shortcuts:
```cpp
if (keyPressed(VK_F10)) {
    if (m_isPlayingReplay) exitReplayMode();
    else enterReplayMode();
}
```

CHECKPOINT 4: Full UI controls working

============================================================================
PHASE 5: SAVE/LOAD REPLAYS (30+ minutes)
============================================================================

Add file persistence:

1. Save replay to file:
```cpp
void saveReplay(const std::string& filename) {
    m_replayRecorder.saveToFile(filename);
    SetStatusMessage("Replay saved: " + filename);
}
```

2. Load replay from file:
```cpp
void loadReplay(const std::string& filename) {
    if (m_replayPlayer.loadFromFile(filename)) {
        enterReplayMode();
        SetStatusMessage("Replay loaded: " + filename);
    }
}
```

3. File browser UI for replays

CHECKPOINT 5: Replays persist to disk

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/REPLAY_INTEGRATION_PLAN.md
- [ ] ReplayRecorder instantiated in main.cpp
- [ ] recordFrame() called each update
- [ ] Neural weights included in snapshots (S-07 fixed)
- [ ] Ring buffer optimization (S-08 fixed)
- [ ] ReplayPlayer instantiated
- [ ] Playback mode with interpolation
- [ ] ImGui replay controls panel
- [ ] Timeline scrubber
- [ ] Speed control
- [ ] F10 keyboard shortcut
- [ ] Save/load replays to file
- [ ] Recording indicator in UI

ESTIMATED TIME: 3-4 hours with sub-agents
```

---

## Agent 2: GPU Compute Activation (4-5 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Activate GPU compute for steering/AI to handle 10,000+ creatures at 60 FPS.

ISSUES TO FIX: S-09, S-10, S-11, S-12

============================================================================
PHASE 1: VERIFY SHADER EXISTS (30+ minutes)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Check shader files:
- Search for shaders/hlsl/SteeringCompute.hlsl
- If missing, create it based on GPUSteeringCompute.h expectations
- Verify shader compiles with dxc or fxc
- Document shader input/output structure

SUB-AGENT 1B - Analyze GPUSteeringCompute:
- Read src/ai/GPUSteeringCompute.h thoroughly
- Read src/ai/GPUSteeringCompute.cpp thoroughly
- Document initialization requirements
- Document dispatch() call signature
- Identify what data needs uploading

CHECKPOINT 1: SteeringCompute.hlsl exists and compiles

============================================================================
PHASE 2: CREATE/FIX COMPUTE SHADER (1+ hour)
============================================================================

If shader missing or incomplete, create shaders/hlsl/SteeringCompute.hlsl:

```hlsl
// SteeringCompute.hlsl - GPU-accelerated creature steering

struct CreatureInput {
    float3 position;
    float3 velocity;
    float3 forward;
    float energy;
    int creatureType;  // 0=herbivore, 1=carnivore, 2=flying, 3=aquatic
    float senseRange;
    float maxSpeed;
    float padding;
};

struct FoodPosition {
    float3 position;
    float energy;
};

struct SteeringOutput {
    float3 steeringForce;
    float targetSpeed;
};

cbuffer SimulationParams : register(b0) {
    uint creatureCount;
    uint foodCount;
    float deltaTime;
    float separationWeight;
    float alignmentWeight;
    float cohesionWeight;
    float foodAttractionWeight;
    float predatorAvoidanceWeight;
};

StructuredBuffer<CreatureInput> creatures : register(t0);
StructuredBuffer<FoodPosition> food : register(t1);
RWStructuredBuffer<SteeringOutput> outputs : register(u0);

float3 seek(float3 position, float3 target, float maxSpeed) {
    float3 desired = normalize(target - position) * maxSpeed;
    return desired;
}

float3 flee(float3 position, float3 threat, float maxSpeed) {
    float3 desired = normalize(position - threat) * maxSpeed;
    return desired;
}

[numthreads(256, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID) {
    if (id.x >= creatureCount) return;

    CreatureInput self = creatures[id.x];
    float3 steering = float3(0, 0, 0);

    // Find nearest food
    float nearestFoodDist = 999999.0f;
    float3 nearestFoodPos = self.position;
    for (uint f = 0; f < foodCount; f++) {
        float dist = distance(self.position, food[f].position);
        if (dist < nearestFoodDist && dist < self.senseRange) {
            nearestFoodDist = dist;
            nearestFoodPos = food[f].position;
        }
    }

    // Seek food
    if (nearestFoodDist < self.senseRange) {
        steering += seek(self.position, nearestFoodPos, self.maxSpeed) * foodAttractionWeight;
    }

    // Flocking behaviors
    float3 separation = float3(0, 0, 0);
    float3 alignment = float3(0, 0, 0);
    float3 cohesion = float3(0, 0, 0);
    int neighborCount = 0;

    for (uint c = 0; c < creatureCount; c++) {
        if (c == id.x) continue;

        float dist = distance(self.position, creatures[c].position);
        if (dist < self.senseRange * 0.5f) {
            // Same type - flock together
            if (creatures[c].creatureType == self.creatureType) {
                separation += (self.position - creatures[c].position) / max(dist, 0.1f);
                alignment += creatures[c].velocity;
                cohesion += creatures[c].position;
                neighborCount++;
            }
            // Predator detection (carnivores chase herbivores)
            else if (self.creatureType == 0 && creatures[c].creatureType == 1) {
                steering += flee(self.position, creatures[c].position, self.maxSpeed) * predatorAvoidanceWeight;
            }
        }
    }

    if (neighborCount > 0) {
        separation = normalize(separation) * self.maxSpeed * separationWeight;
        alignment = normalize(alignment / neighborCount) * self.maxSpeed * alignmentWeight;
        cohesion = seek(self.position, cohesion / neighborCount, self.maxSpeed) * cohesionWeight;
        steering += separation + alignment + cohesion;
    }

    // Clamp steering force
    float steerMag = length(steering);
    if (steerMag > self.maxSpeed) {
        steering = normalize(steering) * self.maxSpeed;
    }

    outputs[id.x].steeringForce = steering;
    outputs[id.x].targetSpeed = min(length(steering), self.maxSpeed);
}
```

CHECKPOINT 2: Shader compiles and logic is correct

============================================================================
PHASE 3: INTEGRATE INTO MAIN.CPP (1.5+ hours)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 3A - Initialize GPU compute:
```cpp
#include "GPUSteeringCompute.h"

// Member variable
std::unique_ptr<GPUSteeringCompute> m_gpuSteering;
bool m_useGPUSteering = true;
const int GPU_STEERING_THRESHOLD = 500;  // Use GPU when > 500 creatures

// In Initialize():
m_gpuSteering = std::make_unique<GPUSteeringCompute>();
if (!m_gpuSteering->initialize(m_device.Get(), L"shaders/hlsl/SteeringCompute.hlsl")) {
    OutputDebugStringA("GPU steering failed to initialize, falling back to CPU\n");
    m_useGPUSteering = false;
}
```

SUB-AGENT 3B - Wire dispatch into update loop:
```cpp
void updateCreatureSteering(float deltaTime) {
    if (m_useGPUSteering && m_creatures.size() > GPU_STEERING_THRESHOLD) {
        updateSteeringGPU(deltaTime);
    } else {
        updateSteeringCPU(deltaTime);
    }
}

void updateSteeringGPU(float deltaTime) {
    // Upload creature data
    std::vector<CreatureInputData> inputData;
    inputData.reserve(m_creatures.size());

    for (const auto& c : m_creatures) {
        CreatureInputData data;
        data.position = c->position;
        data.velocity = c->velocity;
        data.forward = c->forward;
        data.energy = c->energy;
        data.creatureType = static_cast<int>(c->type);
        data.senseRange = c->genome.senseRange;
        data.maxSpeed = c->genome.speed * c->maxSpeed;
        inputData.push_back(data);
    }

    // Upload food data
    std::vector<FoodPositionData> foodData;
    for (const auto& f : m_food) {
        if (f.isActive) {
            FoodPositionData fd;
            fd.position = f.position;
            fd.energy = f.energy;
            foodData.push_back(fd);
        }
    }

    // Dispatch compute shader
    m_gpuSteering->uploadCreatureData(inputData);
    m_gpuSteering->uploadFoodData(foodData);
    m_gpuSteering->dispatch(m_commandList.Get(), inputData.size());

    // Read back results (S-12 fix - actually use the results!)
    auto results = m_gpuSteering->readbackResults();

    for (size_t i = 0; i < m_creatures.size(); i++) {
        m_creatures[i]->applySteeringForce(results[i].steeringForce, deltaTime);
    }
}
```

CHECKPOINT 3: GPU steering dispatches and results are applied

============================================================================
PHASE 4: ASYNC COMPUTE (1+ hour)
============================================================================

Fix S-11 - Implement async compute to avoid GPU stalls:

1. Create async compute queue:
```cpp
// In GPUSteeringCompute.h
ComPtr<ID3D12CommandQueue> m_computeQueue;
ComPtr<ID3D12CommandAllocator> m_computeAllocator;
ComPtr<ID3D12GraphicsCommandList> m_computeCommandList;
ComPtr<ID3D12Fence> m_computeFence;
UINT64 m_computeFenceValue = 0;
HANDLE m_computeFenceEvent;

void initializeAsyncCompute(ID3D12Device* device);
void dispatchAsync();
bool isComputeComplete();
void waitForCompute();
```

2. Double-buffer the output:
```cpp
// Use results from LAST frame while computing THIS frame
std::vector<SteeringOutput> m_lastFrameResults;
std::vector<SteeringOutput> m_currentResults;

void swapBuffers() {
    std::swap(m_lastFrameResults, m_currentResults);
}
```

3. Pipeline the compute:
```cpp
// Frame N:
// 1. Apply steering from frame N-1
// 2. Start computing steering for frame N
// 3. Render frame N

void updateSteeringGPUAsync(float deltaTime) {
    // Apply last frame's results immediately (no stall)
    for (size_t i = 0; i < m_creatures.size(); i++) {
        if (i < m_lastFrameResults.size()) {
            m_creatures[i]->applySteeringForce(m_lastFrameResults[i].steeringForce, deltaTime);
        }
    }

    // Check if this frame's compute is done
    if (m_gpuSteering->isComputeComplete()) {
        m_gpuSteering->swapBuffers();
    }

    // Start next frame's compute (async)
    uploadDataAndDispatchAsync();
}
```

CHECKPOINT 4: Async compute pipeline working

============================================================================
PHASE 5: PERFORMANCE VERIFICATION (30+ minutes)
============================================================================

1. Add performance metrics:
```cpp
void renderPerformanceStats() {
    ImGui::Begin("Performance");

    ImGui::Text("Creatures: %d", m_creatures.size());
    ImGui::Text("Steering Mode: %s", m_useGPUSteering ? "GPU" : "CPU");

    if (m_useGPUSteering) {
        ImGui::Text("GPU Compute Time: %.2f ms", m_gpuSteering->getLastComputeTime());
        ImGui::Text("Async Latency: 1 frame");
    }

    ImGui::Text("Frame Time: %.2f ms (%.0f FPS)",
                m_frameTime * 1000.0f, 1.0f / m_frameTime);

    ImGui::End();
}
```

2. Test with increasing creature counts:
- 500 creatures: CPU should handle
- 1000 creatures: GPU should activate
- 5000 creatures: Should maintain 60 FPS
- 10000 creatures: Target 60 FPS

CHECKPOINT 5: Performance targets met

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] shaders/hlsl/SteeringCompute.hlsl exists and compiles
- [ ] GPUSteeringCompute instantiated in main.cpp
- [ ] Creature data uploaded to GPU each frame
- [ ] Food data uploaded to GPU
- [ ] Compute shader dispatched
- [ ] Results read back and applied to creatures (S-12 fixed)
- [ ] Async compute queue (S-11 fixed)
- [ ] Double-buffered results
- [ ] Automatic CPU/GPU switching based on creature count
- [ ] Performance metrics in UI
- [ ] 10,000 creatures at 60 FPS

ESTIMATED TIME: 4-5 hours with sub-agents
```

---

## Agent 3: Biome-Aware Vegetation (3-4 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Make vegetation system use biome data instead of just height.

ISSUES TO FIX: E-11, E-12, E-13, E-14

============================================================================
PHASE 1: ANALYZE CURRENT SYSTEMS (30+ minutes)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Analyze VegetationManager:
- Read src/environment/VegetationManager.cpp thoroughly
- Document current placement logic (height-based)
- Identify where biome queries should be added
- Note tree types currently supported

SUB-AGENT 1B - Analyze ClimateSystem:
- Read src/environment/ClimateSystem.cpp
- Document getClimateDataAt() return values
- Document BiomeType enum values
- Understand biome blend system

CHECKPOINT 1: Document integration plan

============================================================================
PHASE 2: EXPAND VEGETATION TYPES (1+ hour)
============================================================================

Add biome-specific vegetation in TreeGenerator.h:

```cpp
enum class TreeType {
    // Existing
    OAK,
    PINE,
    WILLOW,

    // Tropical
    PALM,
    MANGROVE,
    KAPOK,          // Rainforest emergent

    // Desert
    CACTUS_SAGUARO,
    CACTUS_BARREL,
    JOSHUA_TREE,

    // Boreal
    SPRUCE,
    FIR,
    BIRCH,

    // Savanna
    ACACIA,
    BAOBAB,

    // Swamp
    CYPRESS,

    // Mountain
    JUNIPER,
    ALPINE_FIR
};

struct VegetationProfile {
    std::vector<TreeType> trees;
    std::vector<float> treeWeights;  // Probability weights
    float treeDensity;               // 0-1
    float grassDensity;
    float bushDensity;
    glm::vec3 grassColor;
    glm::vec3 flowerColor;
    bool hasFlowers;
};

VegetationProfile getProfileForBiome(BiomeType biome);
```

Implement profiles:
```cpp
VegetationProfile getProfileForBiome(BiomeType biome) {
    VegetationProfile p;

    switch (biome) {
        case BiomeType::TROPICAL_RAINFOREST:
            p.trees = {PALM, KAPOK, MANGROVE};
            p.treeWeights = {0.4f, 0.4f, 0.2f};
            p.treeDensity = 0.9f;
            p.grassDensity = 0.3f;
            p.bushDensity = 0.8f;
            p.grassColor = {0.2f, 0.6f, 0.2f};
            p.hasFlowers = true;
            break;

        case BiomeType::DESERT_HOT:
            p.trees = {CACTUS_SAGUARO, CACTUS_BARREL, JOSHUA_TREE};
            p.treeWeights = {0.5f, 0.3f, 0.2f};
            p.treeDensity = 0.05f;  // Very sparse
            p.grassDensity = 0.02f;
            p.bushDensity = 0.1f;
            p.grassColor = {0.6f, 0.5f, 0.3f};  // Dead grass
            p.hasFlowers = false;
            break;

        case BiomeType::BOREAL_FOREST:
            p.trees = {SPRUCE, FIR, BIRCH, PINE};
            p.treeWeights = {0.4f, 0.3f, 0.2f, 0.1f};
            p.treeDensity = 0.7f;
            p.grassDensity = 0.4f;
            p.bushDensity = 0.3f;
            p.grassColor = {0.3f, 0.5f, 0.3f};
            p.hasFlowers = false;
            break;

        case BiomeType::SAVANNA:
            p.trees = {ACACIA, BAOBAB};
            p.treeWeights = {0.8f, 0.2f};
            p.treeDensity = 0.15f;  // Scattered
            p.grassDensity = 0.9f;  // Lots of grass
            p.bushDensity = 0.2f;
            p.grassColor = {0.7f, 0.6f, 0.3f};  // Golden
            p.hasFlowers = true;
            break;

        // ... add all 17 biome types
    }

    return p;
}
```

CHECKPOINT 2: All biome vegetation profiles defined

============================================================================
PHASE 3: INTEGRATE WITH CLIMATESYSTEM (1+ hour)
============================================================================

Modify VegetationManager to query ClimateSystem (E-14 fix):

```cpp
class VegetationManager {
private:
    ClimateSystem* m_climate;  // ADD THIS

public:
    void setClimateSystem(ClimateSystem* climate) {
        m_climate = climate;
    }

    void generateVegetation(const Terrain& terrain) {
        for (float x = 0; x < terrain.getWidth(); x += m_spacing) {
            for (float z = 0; z < terrain.getDepth(); z += m_spacing) {
                float height = terrain.getHeightAt(x, z);

                // NEW: Query climate system instead of using height
                ClimateData climate = m_climate->getClimateDataAt(x, z);
                BiomeType biome = climate.primaryBiome;
                float blendFactor = climate.biomeBlend.blendFactor;
                BiomeType secondaryBiome = climate.biomeBlend.secondary;

                // Get vegetation profile for this biome
                VegetationProfile profile = getProfileForBiome(biome);

                // Blend with secondary biome at transitions
                if (blendFactor > 0.1f) {
                    VegetationProfile secondary = getProfileForBiome(secondaryBiome);
                    profile = blendProfiles(profile, secondary, blendFactor);
                }

                // Place vegetation based on profile density
                placeVegetationAt(x, z, height, profile);
            }
        }
    }

    void placeVegetationAt(float x, float z, float height,
                           const VegetationProfile& profile) {
        // Trees
        if (randomFloat() < profile.treeDensity) {
            TreeType type = selectWeightedRandom(profile.trees, profile.treeWeights);
            placeTree(x, z, height, type);
        }

        // Grass patches
        if (randomFloat() < profile.grassDensity) {
            placeGrass(x, z, height, profile.grassColor);
        }

        // Bushes
        if (randomFloat() < profile.bushDensity) {
            placeBush(x, z, height);
        }

        // Flowers (biome-dependent)
        if (profile.hasFlowers && randomFloat() < 0.1f) {
            placeFlowers(x, z, height, profile.flowerColor);
        }
    }
};
```

CHECKPOINT 3: Vegetation queries ClimateSystem

============================================================================
PHASE 4: GENERATE NEW TREE MESHES (45+ minutes)
============================================================================

Add procedural generation for new tree types:

```cpp
void TreeGenerator::generateTree(TreeType type, MeshData& outMesh) {
    switch (type) {
        case TreeType::PALM:
            generatePalmTree(outMesh);
            break;
        case TreeType::CACTUS_SAGUARO:
            generateSaguaroCactus(outMesh);
            break;
        case TreeType::ACACIA:
            generateAcaciaTree(outMesh);
            break;
        // ... etc
    }
}

void TreeGenerator::generatePalmTree(MeshData& mesh) {
    // Curved trunk
    float height = randomRange(8.0f, 15.0f);
    float curve = randomRange(0.1f, 0.3f);
    generateCurvedCylinder(mesh, height, 0.3f, 0.2f, curve);

    // Fronds at top (6-10 large leaves)
    int frondCount = randomRange(6, 10);
    for (int i = 0; i < frondCount; i++) {
        float angle = (float)i / frondCount * 2.0f * PI;
        float droop = randomRange(0.2f, 0.5f);
        generatePalmFrond(mesh, height, angle, droop);
    }
}

void TreeGenerator::generateSaguaroCactus(MeshData& mesh) {
    // Main trunk
    float height = randomRange(5.0f, 12.0f);
    generateCylinder(mesh, height, 0.5f, 0.4f);

    // 0-3 arms
    int armCount = randomRange(0, 3);
    for (int i = 0; i < armCount; i++) {
        float armHeight = randomRange(height * 0.3f, height * 0.6f);
        float armAngle = randomRange(0, 2.0f * PI);
        generateCactusArm(mesh, armHeight, armAngle);
    }
}

void TreeGenerator::generateAcaciaTree(MeshData& mesh) {
    // Characteristic umbrella shape
    float height = randomRange(6.0f, 10.0f);

    // Trunk (often leaning)
    float lean = randomRange(-0.2f, 0.2f);
    generateLeaningTrunk(mesh, height * 0.6f, 0.3f, lean);

    // Flat canopy (wide and thin)
    generateFlatCanopy(mesh, height, height * 1.5f, height * 0.15f);
}
```

CHECKPOINT 4: New tree types render correctly

============================================================================
PHASE 5: UPDATE MAIN.CPP INTEGRATION (30+ minutes)
============================================================================

Wire the systems together:

```cpp
// In Initialize():
m_vegetationManager.setClimateSystem(&m_climateSystem);
m_vegetationManager.generateVegetation(m_terrain);

// When terrain regenerates:
void regenerateTerrain() {
    m_terrain.generate(m_seed);
    m_climateSystem.initialize(m_terrain);
    m_vegetationManager.clear();
    m_vegetationManager.generateVegetation(m_terrain);
}
```

CHECKPOINT 5: Full integration working

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] VegetationProfile struct with density and type data
- [ ] getProfileForBiome() for all 17 biome types
- [ ] VegetationManager queries ClimateSystem (E-14 fixed)
- [ ] Vegetation density varies by biome (E-12 fixed)
- [ ] New tree types: Palm, Cactus, Acacia, Spruce, Cypress, etc. (E-13 fixed)
- [ ] Biome blending for smooth vegetation transitions
- [ ] New tree mesh generators
- [ ] Integration wired in main.cpp
- [ ] Visual verification: rainforest looks dense, desert sparse

ESTIMATED TIME: 3-4 hours with sub-agents
```

---

## Agent 4: Unified Genetics System (4-5 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Consolidate dual genome system and implement genotype-to-phenotype expression.

ISSUES TO FIX: C-13, C-14, C-15, C-16, C-17

============================================================================
PHASE 1: ANALYZE CURRENT GENOME SYSTEMS (45+ minutes)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Analyze Haploid Genome:
- Read src/entities/Genome.h thoroughly
- Document all traits stored
- Document mutation logic
- Document crossover logic

SUB-AGENT 1B - Analyze Diploid Genome:
- Read src/entities/genetics/DiploidGenome.h
- Document chromosome structure
- Document meiosis/gamete creation
- Document epigenetic marks
- Document MatePreferences

CHECKPOINT 1: Document both systems in docs/GENETICS_ANALYSIS.md

============================================================================
PHASE 2: DESIGN UNIFIED SYSTEM (1+ hour)
============================================================================

Choose: Keep DiploidGenome as primary (more realistic), adapt Genome traits into it.

New unified structure:
```cpp
// Unified Genome System
struct Gene {
    float value;           // Expression value
    float dominance;       // 0=recessive, 1=dominant
    bool isMutated;        // For tracking
};

struct Chromosome {
    // Physical traits
    Gene size;
    Gene speed;
    Gene senseRange;
    Gene efficiency;

    // Coloration
    Gene colorR, colorG, colorB;
    Gene patternType;

    // Sensory
    Gene visionRange;
    Gene hearingRange;
    Gene smellRange;

    // Flying traits
    Gene wingSpan;
    Gene flapFrequency;
    Gene glideRatio;

    // Aquatic traits
    Gene finSize;
    Gene swimFrequency;

    // Behavioral tendencies (feed into neural network)
    Gene aggression;
    Gene fearfulness;
    Gene sociability;
    Gene curiosity;

    // NEW: Creature type potential
    Gene terrestrialAptitude;
    Gene aquaticAptitude;
    Gene aerialAptitude;
};

class UnifiedGenome {
public:
    Chromosome maternal;
    Chromosome paternal;

    // Epigenetics
    std::vector<EpigeneticMark> marks;

    // Species tracking
    int speciesId;
    float geneticDistance;

    // Mate preferences (C-17 fix)
    MatePreferences preferences;

    // Express genotype to phenotype
    Phenotype express() const;

    // Determine creature type from genes (C-15 fix)
    CreatureType determineCreatureType() const;

    // Reproduction
    static UnifiedGenome crossover(const UnifiedGenome& parent1,
                                   const UnifiedGenome& parent2);
    void mutate(float rate);

    // Fitness (C-16 fix - enhanced)
    float calculateFitness(const CreatureStats& stats) const;
};
```

CHECKPOINT 2: Unified design documented

============================================================================
PHASE 3: IMPLEMENT UNIFIED GENOME (1.5+ hours)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 3A - Implement UnifiedGenome class:
```cpp
// src/entities/genetics/UnifiedGenome.cpp

Phenotype UnifiedGenome::express() const {
    Phenotype p;

    // Express each trait using dominance
    p.size = expressGene(maternal.size, paternal.size);
    p.speed = expressGene(maternal.speed, paternal.speed);
    p.senseRange = expressGene(maternal.senseRange, paternal.senseRange);

    // Color blending
    p.color.r = expressGene(maternal.colorR, paternal.colorR);
    p.color.g = expressGene(maternal.colorG, paternal.colorG);
    p.color.b = expressGene(maternal.colorB, paternal.colorB);

    // Flying traits (only expressed if aerial aptitude high)
    if (p.aerialAptitude > 0.6f) {
        p.wingSpan = expressGene(maternal.wingSpan, paternal.wingSpan);
        p.flapFrequency = expressGene(maternal.flapFrequency, paternal.flapFrequency);
    }

    // Apply epigenetic modifications
    for (const auto& mark : marks) {
        applyEpigeneticMark(p, mark);
    }

    return p;
}

float UnifiedGenome::expressGene(const Gene& m, const Gene& p) const {
    // Weighted average based on dominance
    float mWeight = m.dominance;
    float pWeight = p.dominance;
    float total = mWeight + pWeight;

    if (total < 0.01f) {
        return (m.value + p.value) / 2.0f;  // Codominant
    }

    return (m.value * mWeight + p.value * pWeight) / total;
}

CreatureType UnifiedGenome::determineCreatureType() const {
    Phenotype p = express();

    float terrestrial = p.terrestrialAptitude;
    float aquatic = p.aquaticAptitude;
    float aerial = p.aerialAptitude;

    // Highest aptitude wins
    if (aerial > terrestrial && aerial > aquatic && aerial > 0.6f) {
        // Flying creature
        if (p.aggression > 0.6f) return CreatureType::AERIAL_PREDATOR;
        if (p.size < 0.3f) return CreatureType::FLYING_INSECT;
        return CreatureType::FLYING_BIRD;
    }

    if (aquatic > terrestrial && aquatic > 0.6f) {
        // Aquatic creature
        if (p.aggression > 0.7f) return CreatureType::AQUATIC_APEX;
        if (p.aggression > 0.4f) return CreatureType::AQUATIC_PREDATOR;
        return CreatureType::AQUATIC_HERBIVORE;
    }

    // Terrestrial (default)
    if (p.aggression > 0.7f) return CreatureType::APEX_PREDATOR;
    if (p.aggression > 0.4f) return CreatureType::SMALL_PREDATOR;
    if (p.sociability > 0.6f) return CreatureType::GRAZER;
    return CreatureType::BROWSER;
}
```

SUB-AGENT 3B - Implement enhanced fitness (C-16 fix):
```cpp
float UnifiedGenome::calculateFitness(const CreatureStats& stats) const {
    float fitness = 0.0f;

    // Survival component (40%)
    fitness += stats.survivalTime * 0.1f;
    fitness += stats.currentEnergy * 0.05f;
    fitness += stats.currentHealth * 0.05f;

    // Reproduction success (30%) - MOST IMPORTANT
    fitness += stats.offspringCount * 50.0f;
    fitness += stats.offspringSurvived * 100.0f;  // Surviving offspring worth more

    // Resource acquisition (20%)
    fitness += stats.foodEaten * 5.0f;
    fitness += stats.energyEfficiency * 10.0f;

    // Predator-specific
    if (isPredator(stats.type)) {
        fitness += stats.killCount * 30.0f;
        fitness += stats.huntingSuccessRate * 50.0f;
    }

    // Social success (10%)
    fitness += stats.matingAttempts * 2.0f;
    fitness += stats.matingSuccessRate * 20.0f;

    return fitness;
}
```

CHECKPOINT 3: UnifiedGenome compiles and works

============================================================================
PHASE 4: IMPLEMENT MATE PREFERENCES (45+ minutes)
============================================================================

Fix C-17 - Actually use MatePreferences in mating:

```cpp
struct MatePreferences {
    float preferredSize;        // -1 to 1 (smaller to larger)
    float preferredColor[3];    // RGB target
    float colorTolerance;       // How strict about color
    float fitnessThreshold;     // Minimum fitness to consider
    float familiarityBonus;     // Prefer known individuals
};

bool Creature::willMateWith(const Creature& other) const {
    // Check basic compatibility
    if (!canMateWith(other)) return false;

    const MatePreferences& prefs = genome.preferences;
    float attraction = 0.0f;

    // Size preference
    float sizeDiff = other.phenotype.size - this->phenotype.size;
    float sizeMatch = 1.0f - abs(sizeDiff - prefs.preferredSize);
    attraction += sizeMatch * 0.3f;

    // Color preference
    float colorDiff = colorDistance(other.phenotype.color,
                                    glm::vec3(prefs.preferredColor[0],
                                              prefs.preferredColor[1],
                                              prefs.preferredColor[2]));
    if (colorDiff < prefs.colorTolerance) {
        attraction += (1.0f - colorDiff / prefs.colorTolerance) * 0.3f;
    }

    // Fitness preference
    if (other.fitness > prefs.fitnessThreshold) {
        attraction += 0.2f;
    }

    // Familiarity (have they met before?)
    if (hasMetBefore(other.id)) {
        attraction += prefs.familiarityBonus * 0.2f;
    }

    // Probabilistic acceptance
    return randomFloat() < attraction;
}
```

CHECKPOINT 4: Mate preferences affect reproduction

============================================================================
PHASE 5: MIGRATE CREATURE TO UNIFIED GENOME (45+ minutes)
============================================================================

Update Creature class:

```cpp
class Creature {
private:
    UnifiedGenome m_genome;      // UNIFIED
    Phenotype m_phenotype;       // Expressed traits
    CreatureType m_type;         // Determined from genome

public:
    Creature(const UnifiedGenome& genome)
        : m_genome(genome)
    {
        // Express genotype to phenotype
        m_phenotype = m_genome.express();

        // Determine type from genes (C-15 fix)
        m_type = m_genome.determineCreatureType();

        // Initialize based on phenotype
        m_maxSpeed = m_phenotype.speed * BASE_SPEED;
        m_senseRange = m_phenotype.senseRange * BASE_SENSE;
        m_size = m_phenotype.size;
        // ... etc
    }

    Creature reproduce(const Creature& mate) {
        UnifiedGenome childGenome = UnifiedGenome::crossover(m_genome, mate.m_genome);
        childGenome.mutate(m_mutationRate);
        return Creature(childGenome);
    }
};
```

CHECKPOINT 5: Creature uses unified genome

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/GENETICS_ANALYSIS.md
- [ ] UnifiedGenome class with diploid structure
- [ ] Gene expression with dominance
- [ ] CreatureType determined from genotype (C-15 fixed)
- [ ] Enhanced fitness function (C-16 fixed)
- [ ] MatePreferences used in mating decisions (C-17 fixed)
- [ ] Creature class migrated to UnifiedGenome
- [ ] Old Genome.h deprecated or removed
- [ ] Epigenetic marks inheritance
- [ ] Species tracking for speciation

ESTIMATED TIME: 4-5 hours with sub-agents
```

---

## Agent 5: NEAT Topology Evolution (4-5 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Implement full NEAT (NeuroEvolution of Augmenting Topologies) so neural network structure evolves, not just weights.

ISSUES TO FIX: C-12 (Neural topology not evolved)

============================================================================
PHASE 1: ANALYZE EXISTING NEAT CODE (45+ minutes)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Analyze NEATGenome:
- Read src/ai/NEATGenome.h thoroughly
- Read src/ai/NEATGenome.cpp if exists
- Document NodeGene and ConnectionGene structures
- Document mutation methods available
- Identify what's missing for full NEAT

SUB-AGENT 1B - Research NEAT algorithm:
- Web search "NEAT algorithm implementation"
- Web search "Stanley Miikkulainen NEAT paper"
- Document: innovation numbers, speciation, crossover
- Find reference implementations

CHECKPOINT 1: Document in docs/NEAT_ANALYSIS.md

============================================================================
PHASE 2: IMPLEMENT INNOVATION TRACKING (1+ hour)
============================================================================

Create global innovation tracker:

```cpp
// src/ai/neat/InnovationTracker.h

class InnovationTracker {
private:
    static InnovationTracker* s_instance;
    int m_nextInnovation = 0;
    int m_nextNodeId = 0;
    std::map<std::pair<int,int>, int> m_connectionInnovations;

public:
    static InnovationTracker& getInstance();

    int getConnectionInnovation(int inNode, int outNode) {
        auto key = std::make_pair(inNode, outNode);
        auto it = m_connectionInnovations.find(key);
        if (it != m_connectionInnovations.end()) {
            return it->second;
        }
        int innovation = m_nextInnovation++;
        m_connectionInnovations[key] = innovation;
        return innovation;
    }

    int getNewNodeId() { return m_nextNodeId++; }
    void resetGeneration() { m_connectionInnovations.clear(); }
};
```

CHECKPOINT 2: InnovationTracker working

============================================================================
PHASE 3: IMPLEMENT TOPOLOGY MUTATIONS (1.5+ hours)
============================================================================

Add structural mutations to NEATGenome:

```cpp
void NEATGenome::mutateAddConnection(float probability) {
    if (randomFloat() > probability) return;

    int inNode = randomNode(NodeType::INPUT | NodeType::HIDDEN);
    int outNode = randomNode(NodeType::HIDDEN | NodeType::OUTPUT);

    if (hasConnection(inNode, outNode)) return;
    if (wouldCreateCycle(inNode, outNode)) return;

    ConnectionGene conn;
    conn.inNode = inNode;
    conn.outNode = outNode;
    conn.weight = randomFloat(-1.0f, 1.0f);
    conn.enabled = true;
    conn.innovation = InnovationTracker::getInstance()
                          .getConnectionInnovation(inNode, outNode);
    m_connections.push_back(conn);
}

void NEATGenome::mutateAddNode(float probability) {
    if (randomFloat() > probability) return;
    if (m_connections.empty()) return;

    // Pick random enabled connection to split
    int connIdx = randomEnabledConnection();
    ConnectionGene& oldConn = m_connections[connIdx];
    oldConn.enabled = false;

    // Create new node
    NodeGene newNode;
    newNode.id = InnovationTracker::getInstance().getNewNodeId();
    newNode.type = NodeType::HIDDEN;
    m_nodes.push_back(newNode);

    // Create two new connections (in->new, new->out)
    addConnection(oldConn.inNode, newNode.id, 1.0f);
    addConnection(newNode.id, oldConn.outNode, oldConn.weight);
}
```

CHECKPOINT 3: Topology mutations working

============================================================================
PHASE 4: IMPLEMENT SPECIATION (1+ hour)
============================================================================

```cpp
class SpeciesManager {
private:
    std::vector<Species> m_species;
    float m_c1 = 1.0f, m_c2 = 1.0f, m_c3 = 0.4f;
    float m_compatibilityThreshold = 3.0f;

public:
    float calculateCompatibility(const NEATGenome& g1, const NEATGenome& g2) {
        int matching = 0, disjoint = 0, excess = 0;
        float weightDiff = 0.0f;

        // Compare by innovation number
        // Count excess, disjoint, matching genes
        // Calculate average weight difference for matching

        int n = std::max(g1.getConnectionCount(), g2.getConnectionCount());
        if (n < 20) n = 1;

        return (m_c1 * excess / n) + (m_c2 * disjoint / n) + (m_c3 * weightDiff);
    }

    void speciate(std::vector<NEATGenome>& population) {
        for (auto& species : m_species) species.members.clear();

        for (auto& genome : population) {
            bool found = false;
            for (auto& species : m_species) {
                if (calculateCompatibility(genome, species.representative)
                    < m_compatibilityThreshold) {
                    species.members.push_back(&genome);
                    found = true;
                    break;
                }
            }
            if (!found) {
                createNewSpecies(genome);
            }
        }

        removeEmptySpecies();
        updateRepresentatives();
    }
};
```

CHECKPOINT 4: Speciation working

============================================================================
PHASE 5: IMPLEMENT NEAT CROSSOVER (45+ minutes)
============================================================================

```cpp
NEATGenome NEATGenome::crossover(const NEATGenome& parent1,
                                  const NEATGenome& parent2) {
    const NEATGenome& fitter = parent1.fitness >= parent2.fitness ? parent1 : parent2;
    const NEATGenome& other = &fitter == &parent1 ? parent2 : parent1;

    NEATGenome child;
    child.m_nodes = fitter.m_nodes;

    auto conns1 = fitter.getConnectionsByInnovation();
    auto conns2 = other.getConnectionsByInnovation();

    for (const auto& [innov, conn] : conns1) {
        if (conns2.count(innov)) {
            // Matching - randomly choose parent
            child.m_connections.push_back(
                randomFloat() < 0.5f ? conn : conns2.at(innov));
        } else {
            // Excess/disjoint from fitter - inherit
            child.m_connections.push_back(conn);
        }
    }

    return child;
}
```

CHECKPOINT 5: NEAT crossover working

============================================================================
PHASE 6: INTEGRATE WITH CREATURE SYSTEM (30+ minutes)
============================================================================

Update creature to use evolved topology:

```cpp
class Creature {
    NEATGenome m_brainGenome;
    NEATNetwork m_brain;  // Compiled for fast evaluation

    void initializeBrain() {
        m_brain = m_brainGenome.compile();
    }

    std::vector<float> think(const std::vector<float>& inputs) {
        return m_brain.forward(inputs);
    }
};
```

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/NEAT_ANALYSIS.md
- [ ] InnovationTracker singleton
- [ ] mutateAddConnection() with cycle detection
- [ ] mutateAddNode() with connection splitting
- [ ] Compatibility distance calculation
- [ ] SpeciesManager with speciation
- [ ] NEAT crossover with gene alignment
- [ ] Integration with Creature brain
- [ ] Observable topology growth over generations

ESTIMATED TIME: 4-5 hours with sub-agents
```

---

## Agent 6: Terrain Chunking & LOD Integration (3-4 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Complete terrain chunking system and integrate LOD with main terrain rendering.

ISSUES TO FIX: E-01, E-02 (Terrain chunking not integrated, ChunkManager incomplete)

============================================================================
PHASE 1: ANALYZE EXISTING CHUNK SYSTEM (30+ minutes)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Analyze TerrainChunk headers:
- Read src/environment/TerrainChunk.h
- Read src/environment/TerrainChunkManager.h
- Document chunk configuration (size, LOD levels)
- Identify what's implemented vs stub

SUB-AGENT 1B - Analyze main Terrain:
- Read src/environment/Terrain.cpp
- Document current rendering approach
- Identify integration points for chunking
- Note GPU resource management

CHECKPOINT 1: Document in docs/TERRAIN_CHUNKING_PLAN.md

============================================================================
PHASE 2: IMPLEMENT TERRAINCHUNK (1+ hour)
============================================================================

Complete TerrainChunk implementation:

```cpp
// src/environment/TerrainChunk.cpp

class TerrainChunk {
private:
    glm::ivec2 m_chunkCoord;
    glm::vec3 m_worldPos;
    float m_size;

    // Per-LOD mesh data
    struct LODMesh {
        ComPtr<ID3D12Resource> vertexBuffer;
        ComPtr<ID3D12Resource> indexBuffer;
        int indexCount;
        int resolution;  // Vertices per edge
    };
    std::array<LODMesh, 4> m_lodMeshes;

    int m_currentLOD = 0;
    bool m_isDirty = true;
    BoundingBox m_bounds;

public:
    void initialize(ID3D12Device* device, glm::ivec2 coord, float chunkSize) {
        m_chunkCoord = coord;
        m_size = chunkSize;
        m_worldPos = glm::vec3(coord.x * chunkSize, 0, coord.y * chunkSize);

        // Generate meshes for each LOD level
        // LOD 0: 64x64 vertices (full detail)
        // LOD 1: 32x32 vertices
        // LOD 2: 16x16 vertices
        // LOD 3: 8x8 vertices (distant)
        int resolutions[] = {64, 32, 16, 8};

        for (int lod = 0; lod < 4; lod++) {
            generateMesh(device, lod, resolutions[lod]);
        }

        calculateBounds();
    }

    void generateMesh(ID3D12Device* device, int lod, int resolution) {
        std::vector<TerrainVertex> vertices;
        std::vector<uint32_t> indices;

        float step = m_size / (resolution - 1);

        for (int z = 0; z < resolution; z++) {
            for (int x = 0; x < resolution; x++) {
                TerrainVertex v;
                v.position.x = m_worldPos.x + x * step;
                v.position.z = m_worldPos.z + z * step;
                v.position.y = sampleHeight(v.position.x, v.position.z);
                v.uv = glm::vec2(x / (float)(resolution-1), z / (float)(resolution-1));
                v.normal = calculateNormal(v.position.x, v.position.z);
                vertices.push_back(v);
            }
        }

        // Generate indices (triangle strip or indexed triangles)
        for (int z = 0; z < resolution - 1; z++) {
            for (int x = 0; x < resolution - 1; x++) {
                int tl = z * resolution + x;
                int tr = tl + 1;
                int bl = (z + 1) * resolution + x;
                int br = bl + 1;

                indices.push_back(tl);
                indices.push_back(bl);
                indices.push_back(tr);
                indices.push_back(tr);
                indices.push_back(bl);
                indices.push_back(br);
            }
        }

        // Create GPU buffers
        createVertexBuffer(device, vertices, m_lodMeshes[lod].vertexBuffer);
        createIndexBuffer(device, indices, m_lodMeshes[lod].indexBuffer);
        m_lodMeshes[lod].indexCount = indices.size();
        m_lodMeshes[lod].resolution = resolution;
    }

    void updateLOD(const glm::vec3& cameraPos) {
        float dist = glm::distance(cameraPos, getCenterPosition());

        if (dist < 50.0f) m_currentLOD = 0;
        else if (dist < 150.0f) m_currentLOD = 1;
        else if (dist < 400.0f) m_currentLOD = 2;
        else m_currentLOD = 3;
    }

    void render(ID3D12GraphicsCommandList* cmdList) {
        const LODMesh& mesh = m_lodMeshes[m_currentLOD];

        D3D12_VERTEX_BUFFER_VIEW vbv = {};
        vbv.BufferLocation = mesh.vertexBuffer->GetGPUVirtualAddress();
        vbv.SizeInBytes = /* vertex count * sizeof(TerrainVertex) */;
        vbv.StrideInBytes = sizeof(TerrainVertex);

        D3D12_INDEX_BUFFER_VIEW ibv = {};
        ibv.BufferLocation = mesh.indexBuffer->GetGPUVirtualAddress();
        ibv.SizeInBytes = mesh.indexCount * sizeof(uint32_t);
        ibv.Format = DXGI_FORMAT_R32_UINT;

        cmdList->IASetVertexBuffers(0, 1, &vbv);
        cmdList->IASetIndexBuffer(&ibv);
        cmdList->DrawIndexedInstanced(mesh.indexCount, 1, 0, 0, 0);
    }
};
```

CHECKPOINT 2: TerrainChunk generates and renders

============================================================================
PHASE 3: IMPLEMENT CHUNKMANAGER (1+ hour)
============================================================================

```cpp
// src/environment/TerrainChunkManager.cpp

class TerrainChunkManager {
private:
    std::unordered_map<glm::ivec2, std::unique_ptr<TerrainChunk>> m_chunks;
    float m_chunkSize = 64.0f;
    int m_loadRadius = 8;  // Chunks to keep loaded around camera
    glm::ivec2 m_lastCameraChunk = {INT_MAX, INT_MAX};

    ID3D12Device* m_device;
    HeightmapGenerator* m_heightmap;

public:
    void initialize(ID3D12Device* device, HeightmapGenerator* heightmap) {
        m_device = device;
        m_heightmap = heightmap;
    }

    void update(const glm::vec3& cameraPos) {
        glm::ivec2 cameraChunk = worldToChunk(cameraPos);

        // Only update if camera moved to new chunk
        if (cameraChunk != m_lastCameraChunk) {
            m_lastCameraChunk = cameraChunk;
            updateLoadedChunks(cameraChunk);
        }

        // Update LOD for all loaded chunks
        for (auto& [coord, chunk] : m_chunks) {
            chunk->updateLOD(cameraPos);
        }
    }

    void updateLoadedChunks(glm::ivec2 centerChunk) {
        // Mark chunks to keep
        std::set<glm::ivec2> chunksToKeep;

        for (int dz = -m_loadRadius; dz <= m_loadRadius; dz++) {
            for (int dx = -m_loadRadius; dx <= m_loadRadius; dx++) {
                glm::ivec2 coord = centerChunk + glm::ivec2(dx, dz);
                chunksToKeep.insert(coord);

                // Load if not present
                if (m_chunks.find(coord) == m_chunks.end()) {
                    loadChunk(coord);
                }
            }
        }

        // Unload distant chunks
        std::vector<glm::ivec2> toUnload;
        for (auto& [coord, chunk] : m_chunks) {
            if (chunksToKeep.find(coord) == chunksToKeep.end()) {
                toUnload.push_back(coord);
            }
        }

        for (auto& coord : toUnload) {
            unloadChunk(coord);
        }
    }

    void loadChunk(glm::ivec2 coord) {
        auto chunk = std::make_unique<TerrainChunk>();
        chunk->initialize(m_device, coord, m_chunkSize);
        chunk->setHeightmapGenerator(m_heightmap);
        m_chunks[coord] = std::move(chunk);
    }

    void unloadChunk(glm::ivec2 coord) {
        m_chunks.erase(coord);
    }

    void render(ID3D12GraphicsCommandList* cmdList, const Frustum& frustum) {
        for (auto& [coord, chunk] : m_chunks) {
            // Frustum culling
            if (frustum.intersects(chunk->getBounds())) {
                chunk->render(cmdList);
            }
        }
    }

    glm::ivec2 worldToChunk(const glm::vec3& worldPos) {
        return glm::ivec2(
            (int)floor(worldPos.x / m_chunkSize),
            (int)floor(worldPos.z / m_chunkSize)
        );
    }
};
```

CHECKPOINT 3: ChunkManager loads/unloads chunks dynamically

============================================================================
PHASE 4: LOD STITCHING (45+ minutes)
============================================================================

Handle seams between chunks at different LOD levels:

```cpp
void TerrainChunk::generateStitchingIndices(
    int myLOD, int neighborLODs[4]) {

    // neighborLODs: [North, East, South, West]
    // If neighbor has lower resolution, we need to skip vertices on that edge

    // For each edge, check if neighbor has different LOD
    for (int edge = 0; edge < 4; edge++) {
        if (neighborLODs[edge] > myLOD) {
            // Neighbor is lower detail - create degenerate triangles
            generateEdgeStitch(edge, myLOD, neighborLODs[edge]);
        }
    }
}

void TerrainChunk::generateEdgeStitch(int edge, int myLOD, int neighborLOD) {
    int myRes = getResolutionForLOD(myLOD);
    int neighborRes = getResolutionForLOD(neighborLOD);
    int ratio = myRes / neighborRes;

    // Skip every 'ratio' vertices on this edge to match neighbor
    // Generate triangles that connect properly
}
```

CHECKPOINT 4: No visible seams between LOD levels

============================================================================
PHASE 5: INTEGRATE WITH MAIN.CPP (30+ minutes)
============================================================================

Replace or augment existing terrain rendering:

```cpp
// In main.cpp

TerrainChunkManager m_terrainChunks;
bool m_useChunkedTerrain = true;

void Initialize() {
    // ...existing terrain init...

    m_terrainChunks.initialize(m_device.Get(), &m_heightmapGenerator);
}

void Update(float dt) {
    // Update chunk loading based on camera
    m_terrainChunks.update(m_camera.getPosition());
}

void RenderTerrain(ID3D12GraphicsCommandList* cmdList) {
    if (m_useChunkedTerrain) {
        m_terrainChunks.render(cmdList, m_camera.getFrustum());
    } else {
        m_terrain.render(cmdList);  // Fallback to old system
    }
}
```

CHECKPOINT 5: Chunked terrain rendering in game

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/TERRAIN_CHUNKING_PLAN.md
- [ ] TerrainChunk.cpp fully implemented
- [ ] 4 LOD levels per chunk (64/32/16/8 resolution)
- [ ] TerrainChunkManager.cpp implemented
- [ ] Dynamic chunk loading/unloading
- [ ] Frustum culling per chunk
- [ ] LOD selection based on distance
- [ ] LOD stitching (no seams)
- [ ] Integration with main.cpp
- [ ] Toggle between chunked/legacy terrain

ESTIMATED TIME: 3-4 hours with sub-agents
```

---

## Agent 7: Dynamic Climate Evolution (3-4 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Implement dynamic climate that evolves over time, affecting biomes and creature behavior.

ISSUES TO FIX: E-03 (ClimateSystem.update() empty), E-06 (Biome blend unused)

============================================================================
PHASE 1: ANALYZE CLIMATE SYSTEM (30+ minutes)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Analyze ClimateSystem:
- Read src/environment/ClimateSystem.cpp thoroughly
- Document current biome classification
- Identify empty update() function
- Note what data is available

SUB-AGENT 1B - Research climate dynamics:
- Web search "procedural climate simulation games"
- Web search "Milankovitch cycles simulation"
- Document long-term climate patterns
- Note seasonal temperature variations

CHECKPOINT 1: Document in docs/CLIMATE_DYNAMICS_PLAN.md

============================================================================
PHASE 2: IMPLEMENT CLIMATE UPDATE (1+ hour)
============================================================================

Fill in the empty update() function:

```cpp
// src/environment/ClimateSystem.cpp

void ClimateSystem::update(float deltaTime) {
    m_simulationTime += deltaTime;

    // Update global temperature based on:
    // 1. Long-term cycles (ice ages)
    // 2. Seasonal variation
    // 3. Random weather events

    updateGlobalTemperature();
    updateMoisturePatterns();
    propagateClimateChanges();
    updateBiomeTransitions();
}

void ClimateSystem::updateGlobalTemperature() {
    // Long-term cycle (simplified Milankovitch)
    // Period: ~1000 game-years, amplitude: ±5°C
    float longCycle = sin(m_simulationTime * 0.0001f) * 5.0f;

    // Medium cycle (centuries)
    float mediumCycle = sin(m_simulationTime * 0.001f) * 2.0f;

    // Seasonal variation based on SeasonManager
    float seasonalOffset = m_seasonManager->getTemperatureModifier();

    m_globalTemperatureOffset = longCycle + mediumCycle + seasonalOffset;

    // Random climate events (volcanic winter, etc.)
    if (m_activeClimateEvent != ClimateEvent::NONE) {
        applyClimateEvent();
    }
}

void ClimateSystem::updateMoisturePatterns() {
    // Simulate wind-driven moisture transport
    // Prevailing winds push moisture from oceans inland

    for (int x = 0; x < m_gridWidth; x++) {
        for (int z = 0; z < m_gridHeight; z++) {
            float currentMoisture = m_moistureGrid[x][z];

            // Evaporation from water bodies
            if (isWater(x, z)) {
                m_moistureGrid[x][z] = 1.0f;
                continue;
            }

            // Wind transport (prevailing westerlies)
            float upwindMoisture = 0.0f;
            if (x > 0) {
                upwindMoisture = m_moistureGrid[x-1][z];
            }

            // Rain shadow effect (mountains block moisture)
            float elevation = getElevation(x, z);
            float rainShadow = 1.0f - glm::clamp(elevation * 2.0f, 0.0f, 0.8f);

            // Moisture transport with dissipation
            float newMoisture = upwindMoisture * 0.9f * rainShadow;

            // Blend with existing (smoothing)
            m_moistureGrid[x][z] = glm::mix(currentMoisture, newMoisture, 0.1f);
        }
    }
}

void ClimateSystem::updateBiomeTransitions() {
    // Biomes shift based on climate changes
    for (int x = 0; x < m_gridWidth; x++) {
        for (int z = 0; z < m_gridHeight; z++) {
            ClimateData& data = m_climateGrid[x][z];

            // Apply global temperature offset
            data.temperature = m_baseTemperature[x][z] + m_globalTemperatureOffset;

            // Recalculate biome
            BiomeType newBiome = classifyBiome(data.temperature, data.moisture);

            if (newBiome != data.primaryBiome) {
                // Track biome transition
                data.biomeBlend.secondary = data.primaryBiome;
                data.biomeBlend.blendFactor = 0.0f;
                data.primaryBiome = newBiome;
                data.isTransitioning = true;
            }

            // Gradual transition
            if (data.isTransitioning) {
                data.biomeBlend.blendFactor += 0.01f;  // Slow transition
                if (data.biomeBlend.blendFactor >= 1.0f) {
                    data.isTransitioning = false;
                    data.biomeBlend.blendFactor = 0.0f;
                }
            }
        }
    }
}
```

CHECKPOINT 2: Climate updates each frame

============================================================================
PHASE 3: IMPLEMENT CLIMATE EVENTS (45+ minutes)
============================================================================

Add random climate events:

```cpp
enum class ClimateEvent {
    NONE,
    VOLCANIC_WINTER,    // -3°C for 50 game-days
    SOLAR_MAXIMUM,      // +2°C for 100 game-days
    DROUGHT,            // -50% moisture for 30 game-days
    MONSOON,            // +100% moisture for 20 game-days
    ICE_AGE_START,      // Long-term cooling begins
    ICE_AGE_END         // Long-term warming begins
};

void ClimateSystem::triggerRandomEvent() {
    if (randomFloat() < 0.0001f) {  // ~1% chance per game-day
        int eventRoll = randomInt(0, 100);

        if (eventRoll < 20) {
            startEvent(ClimateEvent::DROUGHT, 30.0f);
        } else if (eventRoll < 40) {
            startEvent(ClimateEvent::MONSOON, 20.0f);
        } else if (eventRoll < 55) {
            startEvent(ClimateEvent::VOLCANIC_WINTER, 50.0f);
        } else if (eventRoll < 65) {
            startEvent(ClimateEvent::SOLAR_MAXIMUM, 100.0f);
        }
        // Ice ages are rarer, triggered by long-term cycles
    }
}

void ClimateSystem::applyClimateEvent() {
    switch (m_activeClimateEvent) {
        case ClimateEvent::VOLCANIC_WINTER:
            m_globalTemperatureOffset -= 3.0f;
            break;
        case ClimateEvent::SOLAR_MAXIMUM:
            m_globalTemperatureOffset += 2.0f;
            break;
        case ClimateEvent::DROUGHT:
            for (auto& row : m_moistureGrid) {
                for (auto& m : row) m *= 0.5f;
            }
            break;
        case ClimateEvent::MONSOON:
            for (auto& row : m_moistureGrid) {
                for (auto& m : row) m = glm::min(m * 2.0f, 1.0f);
            }
            break;
    }

    m_eventDuration -= m_deltaTime;
    if (m_eventDuration <= 0) {
        endEvent();
    }
}
```

CHECKPOINT 3: Climate events trigger and affect world

============================================================================
PHASE 4: CREATURE CLIMATE ADAPTATION (45+ minutes)
============================================================================

Make creatures respond to climate:

```cpp
// In Creature.cpp

void Creature::updateClimateResponse(const ClimateData& climate) {
    float optimalTemp = getOptimalTemperature();  // From genome
    float currentTemp = climate.temperature;
    float tempDiff = abs(currentTemp - optimalTemp);

    // Temperature stress
    if (tempDiff > 10.0f) {
        // Severe stress - energy drain
        m_energy -= tempDiff * 0.1f * m_deltaTime;
        m_climateStress = glm::clamp(tempDiff / 20.0f, 0.0f, 1.0f);
    }

    // Behavioral adaptations
    if (currentTemp > optimalTemp + 5.0f) {
        // Too hot - seek water, shade
        m_seekingCooling = true;
        m_preferredBiomes.insert(BiomeType::WATER_EDGE);
    } else if (currentTemp < optimalTemp - 5.0f) {
        // Too cold - seek shelter, group together
        m_seekingWarmth = true;
        m_socialDistance *= 0.5f;  // Huddle
    }

    // Migration pressure
    if (m_climateStress > 0.5f && m_migrationCooldown <= 0) {
        considerMigration(climate);
    }
}

void Creature::considerMigration(const ClimateData& currentClimate) {
    // Look for better climate in sensing range
    glm::vec3 bestDirection = glm::vec3(0);
    float bestSuitability = calculateClimateSuitability(currentClimate);

    for (int angle = 0; angle < 360; angle += 45) {
        glm::vec3 dir = glm::vec3(cos(glm::radians((float)angle)), 0,
                                  sin(glm::radians((float)angle)));
        glm::vec3 samplePos = m_position + dir * m_senseRange;

        ClimateData sampleClimate = m_climateSystem->getClimateDataAt(
            samplePos.x, samplePos.z);
        float suitability = calculateClimateSuitability(sampleClimate);

        if (suitability > bestSuitability) {
            bestSuitability = suitability;
            bestDirection = dir;
        }
    }

    if (bestSuitability > calculateClimateSuitability(currentClimate) + 0.2f) {
        m_migrationDirection = bestDirection;
        m_isMigrating = true;
    }
}
```

CHECKPOINT 4: Creatures respond to climate

============================================================================
PHASE 5: UI CLIMATE DISPLAY (30+ minutes)
============================================================================

Add climate info to dashboard:

```cpp
void renderClimatePanel() {
    ImGui::Begin("Climate");

    // Global state
    ImGui::Text("Global Temperature: %.1f°C",
                m_climateSystem.getGlobalTemperature());
    ImGui::Text("Season: %s", m_seasonManager.getSeasonName());

    // Climate event
    if (m_climateSystem.hasActiveEvent()) {
        ImGui::TextColored(ImVec4(1,0.5f,0,1), "Event: %s (%.0f days left)",
                           m_climateSystem.getEventName(),
                           m_climateSystem.getEventTimeRemaining());
    }

    // Local climate at camera
    ClimateData local = m_climateSystem.getClimateDataAt(
        m_camera.getPosition().x, m_camera.getPosition().z);
    ImGui::Separator();
    ImGui::Text("Local Climate:");
    ImGui::Text("  Temperature: %.1f°C", local.temperature * 70 - 30);
    ImGui::Text("  Moisture: %.0f%%", local.moisture * 100);
    ImGui::Text("  Biome: %s", getBiomeName(local.primaryBiome));

    // Climate history graph
    if (ImPlot::BeginPlot("Temperature History")) {
        ImPlot::PlotLine("Global Temp", m_tempHistory.data(),
                         m_tempHistory.size());
        ImPlot::EndPlot();
    }

    ImGui::End();
}
```

CHECKPOINT 5: Climate UI working

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/CLIMATE_DYNAMICS_PLAN.md
- [ ] ClimateSystem.update() implemented (E-03 fixed)
- [ ] Global temperature cycles
- [ ] Moisture transport simulation
- [ ] Biome transitions over time (E-06 fixed)
- [ ] Climate events (drought, volcanic winter, etc.)
- [ ] Creature climate stress system
- [ ] Creature migration behavior
- [ ] Climate UI panel
- [ ] Temperature history graph

ESTIMATED TIME: 3-4 hours with sub-agents
```

---

## Agent 8: Post-Processing Pipeline (3-4 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Wire existing post-processing shaders into the main render loop for visual polish.

ISSUES TO FIX: R-01 (Shadow pass optional), R-02 (LOD not active), R-03 (Post-processing not wired)

============================================================================
PHASE 1: ANALYZE EXISTING SHADERS (30+ minutes)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Analyze post-process shaders:
- Read shaders/hlsl/SSAO.hlsl
- Read shaders/hlsl/SSR.hlsl
- Read shaders/hlsl/Bloom.hlsl
- Read shaders/hlsl/ToneMapping.hlsl
- Read shaders/hlsl/VolumetricFog.hlsl
- Document inputs/outputs for each

SUB-AGENT 1B - Analyze current render loop:
- Read src/main.cpp render code
- Document render target setup
- Identify where post-processing should insert
- Note existing PostProcess_DX12.h if present

CHECKPOINT 1: Document in docs/POSTPROCESS_PIPELINE_PLAN.md

============================================================================
PHASE 2: CREATE RENDER TARGET CHAIN (1+ hour)
============================================================================

Set up multi-pass rendering:

```cpp
// src/graphics/PostProcessPipeline.h

class PostProcessPipeline {
private:
    // Render targets
    ComPtr<ID3D12Resource> m_hdrTarget;          // Scene rendered here
    ComPtr<ID3D12Resource> m_depthBuffer;        // For SSAO, volumetrics
    ComPtr<ID3D12Resource> m_normalBuffer;       // G-buffer normal
    ComPtr<ID3D12Resource> m_ssaoTarget;         // SSAO result
    ComPtr<ID3D12Resource> m_bloomTarget[2];     // Ping-pong for blur
    ComPtr<ID3D12Resource> m_finalTarget;        // After tone mapping

    // Descriptor heaps
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_srvHeap;

    // PSOs for each pass
    ComPtr<ID3D12PipelineState> m_ssaoPSO;
    ComPtr<ID3D12PipelineState> m_bloomExtractPSO;
    ComPtr<ID3D12PipelineState> m_bloomBlurPSO;
    ComPtr<ID3D12PipelineState> m_toneMappingPSO;
    ComPtr<ID3D12PipelineState> m_fogPSO;

    // Settings
    bool m_ssaoEnabled = true;
    bool m_bloomEnabled = true;
    bool m_fogEnabled = true;
    float m_bloomThreshold = 1.0f;
    float m_bloomIntensity = 0.5f;
    float m_exposure = 1.0f;

public:
    void initialize(ID3D12Device* device, int width, int height);
    void resize(int width, int height);

    ID3D12Resource* getHDRTarget() { return m_hdrTarget.Get(); }
    ID3D12Resource* getDepthBuffer() { return m_depthBuffer.Get(); }

    void execute(ID3D12GraphicsCommandList* cmdList,
                 ID3D12Resource* sceneColor,
                 ID3D12Resource* sceneDepth,
                 ID3D12Resource* backBuffer);
};

void PostProcessPipeline::initialize(ID3D12Device* device, int width, int height) {
    // Create HDR render target (RGBA16F)
    D3D12_RESOURCE_DESC hdrDesc = {};
    hdrDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    hdrDesc.Width = width;
    hdrDesc.Height = height;
    hdrDesc.DepthOrArraySize = 1;
    hdrDesc.MipLevels = 1;
    hdrDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    hdrDesc.SampleDesc.Count = 1;
    hdrDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
        &hdrDesc, D3D12_RESOURCE_STATE_RENDER_TARGET,
        nullptr, IID_PPV_ARGS(&m_hdrTarget));

    // Create other targets...
    createSSAOResources(device, width, height);
    createBloomResources(device, width, height);

    // Create PSOs
    createShaderPSOs(device);
}
```

CHECKPOINT 2: Render targets created

============================================================================
PHASE 3: IMPLEMENT SSAO PASS (45+ minutes)
============================================================================

```cpp
void PostProcessPipeline::executeSSAO(ID3D12GraphicsCommandList* cmdList,
                                       ID3D12Resource* depth,
                                       ID3D12Resource* normal) {
    if (!m_ssaoEnabled) return;

    // Transition resources
    transitionResource(cmdList, m_ssaoTarget.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET);

    // Set render target
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_ssaoRTV;
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Clear to white (no occlusion)
    float clearColor[] = {1, 1, 1, 1};
    cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // Set PSO and root signature
    cmdList->SetPipelineState(m_ssaoPSO.Get());
    cmdList->SetGraphicsRootSignature(m_postProcessRootSig.Get());

    // Bind depth and normal textures
    cmdList->SetGraphicsRootDescriptorTable(0, m_depthSRV);
    cmdList->SetGraphicsRootDescriptorTable(1, m_normalSRV);

    // SSAO parameters
    struct SSAOParams {
        glm::mat4 projection;
        glm::mat4 invProjection;
        glm::vec4 samples[16];  // Hemisphere samples
        float radius;
        float bias;
        float intensity;
        float padding;
    };

    SSAOParams params;
    params.projection = m_camera->getProjection();
    params.invProjection = glm::inverse(params.projection);
    params.radius = 0.5f;
    params.bias = 0.025f;
    params.intensity = 1.5f;
    generateHemisphereSamples(params.samples, 16);

    cmdList->SetGraphicsRoot32BitConstants(2, sizeof(params)/4, &params, 0);

    // Draw fullscreen quad
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);  // Fullscreen triangle

    // Transition back
    transitionResource(cmdList, m_ssaoTarget.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}
```

CHECKPOINT 3: SSAO pass working

============================================================================
PHASE 4: IMPLEMENT BLOOM PASS (45+ minutes)
============================================================================

```cpp
void PostProcessPipeline::executeBloom(ID3D12GraphicsCommandList* cmdList,
                                        ID3D12Resource* hdrScene) {
    if (!m_bloomEnabled) return;

    // Pass 1: Extract bright pixels
    {
        transitionResource(cmdList, m_bloomTarget[0].Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET);

        cmdList->OMSetRenderTargets(1, &m_bloomRTV[0], FALSE, nullptr);
        cmdList->SetPipelineState(m_bloomExtractPSO.Get());
        cmdList->SetGraphicsRootDescriptorTable(0, m_hdrSceneSRV);

        struct BloomExtractParams {
            float threshold;
            float softKnee;
        } params = {m_bloomThreshold, 0.5f};
        cmdList->SetGraphicsRoot32BitConstants(1, 2, &params, 0);

        cmdList->DrawInstanced(3, 1, 0, 0);

        transitionResource(cmdList, m_bloomTarget[0].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    // Pass 2-5: Gaussian blur (ping-pong)
    for (int i = 0; i < 4; i++) {
        int srcIdx = i % 2;
        int dstIdx = 1 - srcIdx;

        transitionResource(cmdList, m_bloomTarget[dstIdx].Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET);

        cmdList->OMSetRenderTargets(1, &m_bloomRTV[dstIdx], FALSE, nullptr);
        cmdList->SetPipelineState(m_bloomBlurPSO.Get());
        cmdList->SetGraphicsRootDescriptorTable(0, m_bloomSRV[srcIdx]);

        // Alternate horizontal/vertical blur
        struct BlurParams { int horizontal; } params = {i % 2};
        cmdList->SetGraphicsRoot32BitConstants(1, 1, &params, 0);

        cmdList->DrawInstanced(3, 1, 0, 0);

        transitionResource(cmdList, m_bloomTarget[dstIdx].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
}
```

CHECKPOINT 4: Bloom pass working

============================================================================
PHASE 5: IMPLEMENT TONE MAPPING & COMPOSITE (45+ minutes)
============================================================================

```cpp
void PostProcessPipeline::executeToneMapping(ID3D12GraphicsCommandList* cmdList,
                                              ID3D12Resource* backBuffer) {
    // Final pass: combine everything and tone map

    transitionResource(cmdList, backBuffer,
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);

    cmdList->OMSetRenderTargets(1, &m_backBufferRTV, FALSE, nullptr);
    cmdList->SetPipelineState(m_toneMappingPSO.Get());

    // Bind all inputs
    cmdList->SetGraphicsRootDescriptorTable(0, m_hdrSceneSRV);  // Scene
    cmdList->SetGraphicsRootDescriptorTable(1, m_ssaoSRV);       // SSAO
    cmdList->SetGraphicsRootDescriptorTable(2, m_bloomSRV[0]);   // Bloom

    struct ToneMappingParams {
        float exposure;
        float gamma;
        float bloomIntensity;
        int tonemapOperator;  // 0=Reinhard, 1=ACES, 2=Uncharted2
    } params = {m_exposure, 2.2f, m_bloomIntensity, 1};

    cmdList->SetGraphicsRoot32BitConstants(3, sizeof(params)/4, &params, 0);

    cmdList->DrawInstanced(3, 1, 0, 0);

    transitionResource(cmdList, backBuffer,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);
}

void PostProcessPipeline::execute(ID3D12GraphicsCommandList* cmdList,
                                   ID3D12Resource* sceneColor,
                                   ID3D12Resource* sceneDepth,
                                   ID3D12Resource* backBuffer) {
    executeSSAO(cmdList, sceneDepth, m_normalBuffer.Get());
    executeBloom(cmdList, sceneColor);
    if (m_fogEnabled) {
        executeFog(cmdList, sceneColor, sceneDepth);
    }
    executeToneMapping(cmdList, backBuffer);
}
```

CHECKPOINT 5: Full pipeline composites correctly

============================================================================
PHASE 6: INTEGRATE WITH MAIN.CPP (30+ minutes)
============================================================================

```cpp
// In main.cpp

PostProcessPipeline m_postProcess;

void Initialize() {
    // ... existing init ...
    m_postProcess.initialize(m_device.Get(), m_width, m_height);
}

void Render() {
    // Render scene to HDR target
    ID3D12Resource* hdrTarget = m_postProcess.getHDRTarget();
    ID3D12Resource* depthBuffer = m_postProcess.getDepthBuffer();

    // Set HDR render target
    cmdList->OMSetRenderTargets(1, &hdrRTV, TRUE, &dsvHandle);

    // Render terrain, creatures, vegetation, etc.
    RenderScene(cmdList);

    // Apply post-processing
    m_postProcess.execute(cmdList, hdrTarget, depthBuffer, m_backBuffer);

    // UI renders directly to back buffer (after post-process)
    m_imguiManager.render(cmdList);
}

void renderPostProcessUI() {
    ImGui::Begin("Post-Processing");

    ImGui::Checkbox("SSAO", &m_postProcess.ssaoEnabled);
    ImGui::Checkbox("Bloom", &m_postProcess.bloomEnabled);
    ImGui::Checkbox("Volumetric Fog", &m_postProcess.fogEnabled);

    ImGui::SliderFloat("Exposure", &m_postProcess.exposure, 0.1f, 5.0f);
    ImGui::SliderFloat("Bloom Threshold", &m_postProcess.bloomThreshold, 0.5f, 3.0f);
    ImGui::SliderFloat("Bloom Intensity", &m_postProcess.bloomIntensity, 0.0f, 2.0f);

    ImGui::End();
}
```

CHECKPOINT 6: Post-processing visible in game

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/POSTPROCESS_PIPELINE_PLAN.md
- [ ] PostProcessPipeline class
- [ ] HDR render target setup
- [ ] SSAO pass implementation
- [ ] Bloom extract + blur passes
- [ ] Tone mapping (ACES filmic)
- [ ] Volumetric fog pass
- [ ] Composite all effects
- [ ] Integration with main.cpp
- [ ] UI controls for enabling/disabling effects
- [ ] Exposure and bloom settings

ESTIMATED TIME: 3-4 hours with sub-agents
```

---

## Agent 9: Creature Speciation & Evolution Visualization (4-5 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Implement visible speciation where creatures diverge into distinct species over time, with phylogenetic tree visualization.

NEW FEATURE: Observable evolution with species divergence and visual lineage tracking.

============================================================================
PHASE 1: DESIGN SPECIATION SYSTEM (45+ minutes)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Research speciation:
- Web search "allopatric speciation simulation"
- Web search "genetic divergence species formation"
- Web search "phylogenetic tree visualization"
- Document reproductive isolation mechanisms

SUB-AGENT 1B - Analyze current genetics:
- Read UnifiedGenome if exists, or Genome.h
- Read NEATGenome for speciation concepts
- Document species ID tracking
- Identify integration points

CHECKPOINT 1: Document in docs/SPECIATION_DESIGN.md

============================================================================
PHASE 2: IMPLEMENT SPECIES TRACKING (1+ hour)
============================================================================

```cpp
// src/entities/SpeciesManager.h

struct Species {
    int id;
    std::string name;                    // Auto-generated or derived
    glm::vec3 displayColor;              // For visualization
    int foundingGeneration;
    int currentPopulation;
    int peakPopulation;

    // Representative genome (average traits)
    Phenotype averagePhenotype;

    // Phylogenetic data
    int parentSpeciesId;                 // -1 for original species
    std::vector<int> childSpeciesIds;

    // Extinction tracking
    bool isExtinct = false;
    int extinctionGeneration = -1;
};

class SpeciesTracker {
private:
    std::vector<Species> m_allSpecies;   // Including extinct
    std::vector<Species*> m_activeSpecies;
    int m_nextSpeciesId = 0;

    float m_speciationThreshold = 0.3f;  // Genetic distance for new species

public:
    void initialize();

    // Called when creature is born
    int assignSpecies(Creature& creature, const Creature* parent1,
                      const Creature* parent2);

    // Check if creature should found new species
    bool shouldSpeciate(const Creature& creature, const Species& currentSpecies);

    // Create new species from divergent creature
    Species& createNewSpecies(const Creature& founder, int parentSpeciesId);

    // Update statistics each generation
    void updateGenerationStats();

    // Mark species as extinct when population hits 0
    void checkExtinctions();

    // Getters
    const Species& getSpecies(int id) const;
    const std::vector<Species*>& getActiveSpecies() const;
    int getSpeciesCount() const;
};

void SpeciesTracker::initialize() {
    // Create initial species for each creature type
    createInitialSpecies("Herbivore Prime", CreatureType::GRAZER, glm::vec3(0,1,0));
    createInitialSpecies("Carnivore Prime", CreatureType::SMALL_PREDATOR, glm::vec3(1,0,0));
    // etc.
}

int SpeciesTracker::assignSpecies(Creature& creature,
                                   const Creature* parent1,
                                   const Creature* parent2) {
    if (!parent1) {
        // Spawned creature - assign to default species
        return getDefaultSpeciesForType(creature.getType());
    }

    // Inherit parent species (use parent1 by default)
    int parentSpeciesId = parent1->getSpeciesId();
    Species& parentSpecies = getSpecies(parentSpeciesId);

    // Check if creature has diverged enough for speciation
    float geneticDistance = calculateGeneticDistance(
        creature.getPhenotype(), parentSpecies.averagePhenotype);

    if (geneticDistance > m_speciationThreshold) {
        // New species!
        Species& newSpecies = createNewSpecies(creature, parentSpeciesId);
        creature.setSpeciesId(newSpecies.id);
        return newSpecies.id;
    }

    creature.setSpeciesId(parentSpeciesId);
    return parentSpeciesId;
}

Species& SpeciesTracker::createNewSpecies(const Creature& founder,
                                           int parentSpeciesId) {
    Species newSpecies;
    newSpecies.id = m_nextSpeciesId++;
    newSpecies.name = generateSpeciesName(founder);
    newSpecies.displayColor = generateDistinctColor(m_allSpecies.size());
    newSpecies.foundingGeneration = m_currentGeneration;
    newSpecies.parentSpeciesId = parentSpeciesId;
    newSpecies.averagePhenotype = founder.getPhenotype();
    newSpecies.currentPopulation = 1;

    // Link to parent
    if (parentSpeciesId >= 0) {
        getSpecies(parentSpeciesId).childSpeciesIds.push_back(newSpecies.id);
    }

    m_allSpecies.push_back(newSpecies);
    m_activeSpecies.push_back(&m_allSpecies.back());

    // Notify UI
    onSpeciesCreated(newSpecies);

    return m_allSpecies.back();
}
```

CHECKPOINT 2: Species tracking working

============================================================================
PHASE 3: IMPLEMENT REPRODUCTIVE ISOLATION (45+ minutes)
============================================================================

```cpp
// Creatures can only mate within same or closely related species

bool Creature::canMateWith(const Creature& other) const {
    // Same species - always OK
    if (m_speciesId == other.m_speciesId) {
        return true;
    }

    // Check if species are closely related (recent divergence)
    const Species& mySpecies = g_speciesTracker.getSpecies(m_speciesId);
    const Species& otherSpecies = g_speciesTracker.getSpecies(other.m_speciesId);

    // Parent-child species can still interbreed (hybrid zone)
    if (mySpecies.parentSpeciesId == other.m_speciesId ||
        otherSpecies.parentSpeciesId == m_speciesId) {

        // But with reduced fertility
        float hybridChance = 0.3f;  // 30% chance of successful mating
        return randomFloat() < hybridChance;
    }

    // Sibling species (same parent) - very low chance
    if (mySpecies.parentSpeciesId == otherSpecies.parentSpeciesId &&
        mySpecies.parentSpeciesId >= 0) {

        float siblingChance = 0.1f;
        return randomFloat() < siblingChance;
    }

    // Completely different species - no mating
    return false;
}

// Hybrid offspring have mixed traits and potentially reduced fitness
Creature Creature::reproduceHybrid(const Creature& mate) {
    UnifiedGenome childGenome = UnifiedGenome::crossover(m_genome, mate.m_genome);
    childGenome.mutate(m_mutationRate * 1.5f);  // Higher mutation in hybrids

    Creature child(childGenome);

    // Hybrid vigor OR outbreeding depression (random)
    if (randomFloat() < 0.3f) {
        // Hybrid vigor - slight boost
        child.m_maxEnergy *= 1.1f;
    } else if (randomFloat() < 0.5f) {
        // Outbreeding depression - penalty
        child.m_maxEnergy *= 0.85f;
        child.m_fertility *= 0.7f;
    }

    return child;
}
```

CHECKPOINT 3: Reproductive isolation working

============================================================================
PHASE 4: IMPLEMENT PHYLOGENETIC TREE (1+ hour)
============================================================================

Visualize species relationships:

```cpp
// src/ui/PhylogeneticTree.h

class PhylogeneticTree {
private:
    struct TreeNode {
        int speciesId;
        glm::vec2 position;
        float branchLength;
        std::vector<TreeNode*> children;
    };

    TreeNode* m_root = nullptr;
    float m_treeWidth = 800.0f;
    float m_treeHeight = 600.0f;

public:
    void buildFromSpeciesTracker(const SpeciesTracker& tracker);
    void render(ImDrawList* drawList, glm::vec2 offset);

private:
    void layoutTree(TreeNode* node, float xMin, float xMax, float y);
    void renderNode(ImDrawList* drawList, TreeNode* node, glm::vec2 offset);
    void renderBranch(ImDrawList* drawList, TreeNode* parent, TreeNode* child,
                      glm::vec2 offset);
};

void PhylogeneticTree::render(ImDrawList* drawList, glm::vec2 offset) {
    if (!m_root) return;

    renderSubtree(drawList, m_root, offset);
}

void PhylogeneticTree::renderSubtree(ImDrawList* drawList, TreeNode* node,
                                      glm::vec2 offset) {
    const Species& species = g_speciesTracker.getSpecies(node->speciesId);

    glm::vec2 pos = node->position + offset;

    // Draw node (circle)
    ImU32 color = species.isExtinct ?
        IM_COL32(128, 128, 128, 200) :  // Gray for extinct
        ImGui::ColorConvertFloat4ToU32(ImVec4(
            species.displayColor.r,
            species.displayColor.g,
            species.displayColor.b, 1.0f));

    float radius = species.isExtinct ? 5.0f :
        glm::clamp(species.currentPopulation / 50.0f, 5.0f, 20.0f);

    drawList->AddCircleFilled(ImVec2(pos.x, pos.y), radius, color);

    // Draw species name on hover
    if (ImGui::IsMouseHoveringRect(
            ImVec2(pos.x - radius, pos.y - radius),
            ImVec2(pos.x + radius, pos.y + radius))) {
        ImGui::BeginTooltip();
        ImGui::Text("%s", species.name.c_str());
        ImGui::Text("Population: %d", species.currentPopulation);
        ImGui::Text("Founded: Gen %d", species.foundingGeneration);
        if (species.isExtinct) {
            ImGui::TextColored(ImVec4(1,0,0,1), "EXTINCT (Gen %d)",
                               species.extinctionGeneration);
        }
        ImGui::EndTooltip();
    }

    // Draw branches to children
    for (TreeNode* child : node->children) {
        glm::vec2 childPos = child->position + offset;

        // Elbow branch
        drawList->AddLine(ImVec2(pos.x, pos.y),
                          ImVec2(pos.x, childPos.y), color, 2.0f);
        drawList->AddLine(ImVec2(pos.x, childPos.y),
                          ImVec2(childPos.x, childPos.y), color, 2.0f);

        renderSubtree(drawList, child, offset);
    }
}
```

CHECKPOINT 4: Phylogenetic tree visualizes

============================================================================
PHASE 5: SPECIES STATISTICS PANEL (45+ minutes)
============================================================================

```cpp
void renderSpeciesPanel() {
    ImGui::Begin("Species & Evolution");

    // Overview
    ImGui::Text("Active Species: %d", g_speciesTracker.getActiveSpecies().size());
    ImGui::Text("Total Species (inc. extinct): %d", g_speciesTracker.getAllSpecies().size());
    ImGui::Text("Speciation Events: %d", g_speciesTracker.getSpeciationCount());
    ImGui::Text("Extinctions: %d", g_speciesTracker.getExtinctionCount());

    ImGui::Separator();

    // Species list
    if (ImGui::CollapsingHeader("Active Species")) {
        for (const Species* sp : g_speciesTracker.getActiveSpecies()) {
            ImGui::PushStyleColor(ImGuiCol_Text,
                ImVec4(sp->displayColor.r, sp->displayColor.g, sp->displayColor.b, 1));

            if (ImGui::TreeNode(sp->name.c_str())) {
                ImGui::PopStyleColor();

                ImGui::Text("Population: %d (peak: %d)",
                            sp->currentPopulation, sp->peakPopulation);
                ImGui::Text("Founded: Generation %d", sp->foundingGeneration);
                ImGui::Text("Avg Size: %.2f", sp->averagePhenotype.size);
                ImGui::Text("Avg Speed: %.2f", sp->averagePhenotype.speed);

                if (sp->parentSpeciesId >= 0) {
                    const Species& parent = g_speciesTracker.getSpecies(sp->parentSpeciesId);
                    ImGui::Text("Parent: %s", parent.name.c_str());
                }

                if (ImGui::Button("Focus Camera")) {
                    focusCameraOnSpecies(sp->id);
                }

                ImGui::TreePop();
            } else {
                ImGui::PopStyleColor();
            }
        }
    }

    // Phylogenetic tree tab
    if (ImGui::CollapsingHeader("Phylogenetic Tree")) {
        ImVec2 treeSize(600, 400);
        ImGui::BeginChild("TreeCanvas", treeSize, true);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();

        m_phyloTree.render(drawList, glm::vec2(canvasPos.x, canvasPos.y));

        ImGui::EndChild();
    }

    ImGui::End();
}
```

CHECKPOINT 5: Species UI complete

============================================================================
PHASE 6: CREATURE VISUAL DIFFERENTIATION (30+ minutes)
============================================================================

Make species visually distinct:

```cpp
void Creature::updateSpeciesAppearance() {
    const Species& mySpecies = g_speciesTracker.getSpecies(m_speciesId);

    // Tint creature color slightly toward species color
    glm::vec3 baseColor = m_phenotype.color;
    glm::vec3 speciesColor = mySpecies.displayColor;

    m_renderColor = glm::mix(baseColor, speciesColor, 0.3f);

    // Add species-specific pattern/marking
    // This could be stripes, spots, etc. based on species ID
    m_patternType = mySpecies.id % 4;  // 0=solid, 1=stripes, 2=spots, 3=gradient
}

// In creature shader
float getPattern(float2 uv, int patternType, int speciesId) {
    float pattern = 1.0;

    if (patternType == 1) {  // Stripes
        pattern = frac(uv.y * 5.0 + speciesId * 0.3) > 0.5 ? 1.0 : 0.8;
    } else if (patternType == 2) {  // Spots
        float2 spotUV = frac(uv * 3.0 + speciesId * 0.2);
        float spot = length(spotUV - 0.5);
        pattern = spot > 0.3 ? 1.0 : 0.7;
    }

    return pattern;
}
```

CHECKPOINT 6: Species visually distinguishable

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/SPECIATION_DESIGN.md
- [ ] Species struct with phylogenetic data
- [ ] SpeciesTracker class
- [ ] Genetic distance calculation
- [ ] Speciation threshold detection
- [ ] Reproductive isolation mechanics
- [ ] Hybrid offspring handling
- [ ] PhylogeneticTree visualization
- [ ] Species statistics panel
- [ ] Species list with details
- [ ] Visual species differentiation
- [ ] Observable speciation events

ESTIMATED TIME: 4-5 hours with sub-agents
```

---

## Agent 10: Final Polish & Performance Optimization (4-6 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Final integration pass, performance optimization, bug fixing, and polish.

============================================================================
PHASE 1: INTEGRATION VERIFICATION (1+ hour)
============================================================================

SPAWN 4 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Verify creature systems:
- Test all creature types spawn correctly
- Verify neural networks influence behavior
- Test reproduction and genetics
- Check species assignment
- Document any regressions

SUB-AGENT 1B - Verify environment systems:
- Test terrain chunking
- Verify biome vegetation
- Test weather/climate
- Check day/night cycle
- Document issues

SUB-AGENT 1C - Verify simulation systems:
- Test save/load
- Test replay recording/playback
- Verify GPU compute activation
- Check performance with 5000+ creatures
- Document issues

SUB-AGENT 1D - Verify rendering:
- Test post-processing effects
- Verify shadows working
- Check LOD transitions
- Test UI panels
- Document visual issues

CHECKPOINT 1: Create docs/PHASE4_INTEGRATION_REPORT.md

============================================================================
PHASE 2: PERFORMANCE PROFILING (1+ hour)
============================================================================

Profile and optimize:

```cpp
// Add frame timing breakdown
struct FrameTimings {
    float creatureUpdate;
    float physicsUpdate;
    float spatialGridRebuild;
    float neuralNetworkEval;
    float rendering;
    float postProcessing;
    float uiRendering;
    float total;
};

FrameTimings m_timings;

void profiledUpdate() {
    auto t0 = std::chrono::high_resolution_clock::now();

    updateCreatures();
    auto t1 = std::chrono::high_resolution_clock::now();
    m_timings.creatureUpdate = duration(t1 - t0);

    updatePhysics();
    auto t2 = std::chrono::high_resolution_clock::now();
    m_timings.physicsUpdate = duration(t2 - t1);

    m_spatialGrid.rebuild();
    auto t3 = std::chrono::high_resolution_clock::now();
    m_timings.spatialGridRebuild = duration(t3 - t2);

    // ... etc
}

void renderPerformanceOverlay() {
    ImGui::Begin("Performance");

    ImGui::Text("Frame Time: %.2f ms (%.0f FPS)",
                m_timings.total * 1000, 1.0f / m_timings.total);

    ImGui::Separator();
    ImGui::Text("Breakdown:");
    ImGui::Text("  Creatures: %.2f ms", m_timings.creatureUpdate * 1000);
    ImGui::Text("  Physics: %.2f ms", m_timings.physicsUpdate * 1000);
    ImGui::Text("  Spatial Grid: %.2f ms", m_timings.spatialGridRebuild * 1000);
    ImGui::Text("  Neural Net: %.2f ms", m_timings.neuralNetworkEval * 1000);
    ImGui::Text("  Rendering: %.2f ms", m_timings.rendering * 1000);
    ImGui::Text("  Post-Process: %.2f ms", m_timings.postProcessing * 1000);
    ImGui::Text("  UI: %.2f ms", m_timings.uiRendering * 1000);

    // Pie chart
    float values[] = {m_timings.creatureUpdate, m_timings.physicsUpdate,
                      m_timings.rendering, m_timings.postProcessing};
    const char* labels[] = {"Creatures", "Physics", "Render", "PostProc"};
    ImPlot::PieChart(labels, values, 4);

    ImGui::End();
}
```

CHECKPOINT 2: Performance bottlenecks identified

============================================================================
PHASE 3: CRITICAL OPTIMIZATIONS (1+ hour)
============================================================================

SPAWN 3 SUB-AGENTS based on profiling results:

SUB-AGENT 3A - Optimize creature update:
```cpp
// Batch neural network evaluations
void batchUpdateNeuralNetworks() {
    // Gather all inputs
    std::vector<std::vector<float>> allInputs;
    for (auto& creature : m_creatures) {
        allInputs.push_back(creature->gatherNeuralInputs());
    }

    // If GPU available, batch evaluate
    if (m_gpuNeuralEnabled) {
        auto allOutputs = m_gpuNeuralEvaluator.batchEvaluate(allInputs);
        for (size_t i = 0; i < m_creatures.size(); i++) {
            m_creatures[i]->applyNeuralOutputs(allOutputs[i]);
        }
    } else {
        // Parallel CPU evaluation
        #pragma omp parallel for
        for (size_t i = 0; i < m_creatures.size(); i++) {
            auto outputs = m_creatures[i]->brain->forward(allInputs[i]);
            m_creatures[i]->applyNeuralOutputs(outputs);
        }
    }
}
```

SUB-AGENT 3B - Optimize rendering:
```cpp
// Aggressive frustum culling
void cullAndSort() {
    m_visibleCreatures.clear();

    for (auto& creature : m_creatures) {
        if (!m_frustum.contains(creature->getBounds())) continue;

        float dist = glm::distance(creature->position, m_camera.position);
        m_visibleCreatures.push_back({creature.get(), dist});
    }

    // Sort front-to-back for early-Z
    std::sort(m_visibleCreatures.begin(), m_visibleCreatures.end(),
              [](const auto& a, const auto& b) { return a.dist < b.dist; });
}

// Reduce overdraw
void renderOpaqueFrontToBack() {
    for (auto& [creature, dist] : m_visibleCreatures) {
        creature->render(m_cmdList.Get());
    }
}
```

SUB-AGENT 3C - Optimize memory:
```cpp
// Object pooling for creatures
class CreaturePool {
    std::vector<std::unique_ptr<Creature>> m_pool;
    std::vector<Creature*> m_free;

public:
    Creature* acquire() {
        if (m_free.empty()) {
            m_pool.push_back(std::make_unique<Creature>());
            return m_pool.back().get();
        }
        Creature* c = m_free.back();
        m_free.pop_back();
        return c;
    }

    void release(Creature* c) {
        c->reset();
        m_free.push_back(c);
    }
};
```

CHECKPOINT 3: Performance improved

============================================================================
PHASE 4: BUG FIXING SPRINT (1+ hour)
============================================================================

Fix issues from integration report:

```cpp
// Common bugs to check and fix:

// 1. Null pointer checks
void Creature::update(float dt) {
    if (!brain) {
        initializeBrain();  // Lazy init if missing
    }
    // ...
}

// 2. Division by zero
float calculateFitness() {
    float survival = survivalTime / std::max(maxAge, 0.001f);
    // ...
}

// 3. Array bounds
void SpatialGrid::query(glm::vec3 pos, float radius, std::vector<Creature*>& out) {
    int minX = std::max(0, worldToCell(pos.x - radius));
    int maxX = std::min(m_gridWidth - 1, worldToCell(pos.x + radius));
    // ... similar for Y, Z
}

// 4. Race conditions (if multithreaded)
std::mutex m_creaturesMutex;
void addCreature(std::unique_ptr<Creature> c) {
    std::lock_guard<std::mutex> lock(m_creaturesMutex);
    m_creatures.push_back(std::move(c));
}

// 5. Memory leaks
void cleanupDeadCreatures() {
    m_creatures.erase(
        std::remove_if(m_creatures.begin(), m_creatures.end(),
            [](const auto& c) { return c->isDead(); }),
        m_creatures.end());
}
```

CHECKPOINT 4: Known bugs fixed

============================================================================
PHASE 5: POLISH & UX IMPROVEMENTS (45+ minutes)
============================================================================

Final polish:

```cpp
// 1. Loading screen
void renderLoadingScreen(float progress, const std::string& status) {
    ImGui::SetNextWindowPos(ImVec2(m_width/2 - 200, m_height/2 - 50));
    ImGui::SetNextWindowSize(ImVec2(400, 100));
    ImGui::Begin("Loading", nullptr, ImGuiWindowFlags_NoDecoration);

    ImGui::Text("%s", status.c_str());
    ImGui::ProgressBar(progress);

    ImGui::End();
}

// 2. Smooth camera transitions
void Camera::focusOn(const glm::vec3& target, float duration) {
    m_transitionStart = m_position;
    m_transitionTarget = target - m_forward * 20.0f;  // Offset behind target
    m_transitionDuration = duration;
    m_transitionProgress = 0.0f;
    m_isTransitioning = true;
}

void Camera::updateTransition(float dt) {
    if (!m_isTransitioning) return;

    m_transitionProgress += dt / m_transitionDuration;
    if (m_transitionProgress >= 1.0f) {
        m_position = m_transitionTarget;
        m_isTransitioning = false;
    } else {
        float t = smoothstep(0.0f, 1.0f, m_transitionProgress);
        m_position = glm::mix(m_transitionStart, m_transitionTarget, t);
    }
}

// 3. Notification system
class NotificationManager {
    struct Notification {
        std::string text;
        float duration;
        float elapsed;
        NotificationType type;
    };

    std::vector<Notification> m_notifications;

public:
    void show(const std::string& text, NotificationType type = INFO) {
        m_notifications.push_back({text, 3.0f, 0.0f, type});
    }

    void render() {
        float y = 50.0f;
        for (auto it = m_notifications.begin(); it != m_notifications.end(); ) {
            float alpha = 1.0f - (it->elapsed / it->duration);
            ImGui::SetNextWindowPos(ImVec2(m_width - 310, y));
            ImGui::SetNextWindowBgAlpha(alpha * 0.8f);

            ImGui::Begin(("##notif" + std::to_string(y)).c_str(), nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);

            ImVec4 color = getColorForType(it->type);
            color.w = alpha;
            ImGui::TextColored(color, "%s", it->text.c_str());

            ImGui::End();

            it->elapsed += m_dt;
            if (it->elapsed >= it->duration) {
                it = m_notifications.erase(it);
            } else {
                ++it;
                y += 40.0f;
            }
        }
    }
};

// 4. Keyboard shortcut help
void renderHelpOverlay() {
    if (!m_showHelp) return;

    ImGui::Begin("Keyboard Shortcuts", &m_showHelp);

    ImGui::Text("Camera:");
    ImGui::BulletText("WASD - Move");
    ImGui::BulletText("Mouse - Look");
    ImGui::BulletText("Scroll - Zoom");

    ImGui::Separator();
    ImGui::Text("Simulation:");
    ImGui::BulletText("Space - Pause/Resume");
    ImGui::BulletText("F5 - Quick Save");
    ImGui::BulletText("F9 - Quick Load");
    ImGui::BulletText("F10 - Toggle Replay Mode");

    ImGui::Separator();
    ImGui::Text("UI:");
    ImGui::BulletText("F1 - Toggle Help");
    ImGui::BulletText("F3 - Toggle Debug");
    ImGui::BulletText("Tab - Toggle Dashboard");

    ImGui::End();
}
```

CHECKPOINT 5: Polish complete

============================================================================
PHASE 6: FINAL VERIFICATION (30+ minutes)
============================================================================

Final checklist:

1. Fresh build test:
   - Clean build from scratch
   - Verify no warnings
   - Run in Release mode

2. Performance targets:
   - [ ] 1000 creatures at 60 FPS
   - [ ] 5000 creatures at 30 FPS
   - [ ] 10000 creatures at 15 FPS (with GPU compute)

3. Feature verification:
   - [ ] All creature types functional
   - [ ] Evolution observable over generations
   - [ ] Speciation occurs
   - [ ] Save/load works
   - [ ] Replay works
   - [ ] All UI panels functional

4. Visual quality:
   - [ ] Post-processing effects look good
   - [ ] No visual glitches
   - [ ] Smooth camera
   - [ ] Readable UI

5. Stability:
   - [ ] No crashes in 30-minute session
   - [ ] Memory stable (no leaks)
   - [ ] Graceful handling of edge cases

CHECKPOINT 6: Project ready for release

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/PHASE4_INTEGRATION_REPORT.md
- [ ] Performance profiling implemented
- [ ] Creature update optimization
- [ ] Rendering optimization (culling, sorting)
- [ ] Memory optimization (pooling)
- [ ] All bugs from integration fixed
- [ ] Loading screen
- [ ] Smooth camera transitions
- [ ] Notification system
- [ ] Help overlay
- [ ] Performance targets met
- [ ] 30-minute stability test passed

ESTIMATED TIME: 4-6 hours with sub-agents
```

---

# Phase 4 Summary

| Agent | Mission | Est. Time | Issues Fixed |
|-------|---------|-----------|--------------|
| 1 | Replay System Integration | 3-4 hrs | S-05, S-06, S-07, S-08 |
| 2 | GPU Compute Activation | 4-5 hrs | S-09, S-10, S-11, S-12 |
| 3 | Biome-Aware Vegetation | 3-4 hrs | E-11, E-12, E-13, E-14 |
| 4 | Unified Genetics System | 4-5 hrs | C-13, C-14, C-15, C-16, C-17 |
| 5 | NEAT Topology Evolution | 4-5 hrs | C-12 |
| 6 | Terrain Chunking & LOD | 3-4 hrs | E-01, E-02 |
| 7 | Dynamic Climate Evolution | 3-4 hrs | E-03, E-06 |
| 8 | Post-Processing Pipeline | 3-4 hrs | R-01, R-02, R-03 |
| 9 | Creature Speciation | 4-5 hrs | NEW FEATURE |
| 10 | Final Polish & Optimization | 4-6 hrs | All remaining |

**Total Estimated Time: 37-46 hours**

Copy-paste each agent prompt into a separate Claude session.
