# Animation System Architecture

## OrganismEvolution Skeletal Animation System

**Document Version:** 1.0
**Date:** January 2026
**Status:** Implementation Complete

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Core Components](#core-components)
3. [Class Diagrams](#class-diagrams)
4. [Data Flow](#data-flow)
5. [Integration Points](#integration-points)
6. [GPU Skinning Pipeline](#gpu-skinning-pipeline)
7. [Usage Examples](#usage-examples)

---

## Architecture Overview

The OrganismEvolution animation system is a modular, production-quality skeletal animation framework designed for procedural creature animation. It combines:

- **Skeletal Animation**: Bone hierarchy with bind poses and runtime poses
- **IK Solvers**: Three algorithms (Two-Bone, FABRIK, CCD) for procedural limb positioning
- **Procedural Locomotion**: Gait-based foot placement with ground adaptation
- **GPU Skinning**: Dual quaternion skinning for smooth joint deformation

### System Stack

```
┌─────────────────────────────────────────────────────────────┐
│                    Creature Entity                          │
│  (Position, Velocity, Rotation, State, Genome)              │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                  CreatureAnimator                           │
│  - Manages skeleton, pose, locomotion, IK                   │
│  - update(deltaTime) drives all animation                   │
└─────────────────────────────────────────────────────────────┘
                            │
            ┌───────────────┼───────────────┐
            ▼               ▼               ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│    Skeleton     │ │ SkeletonPose    │ │ ProceduralLoco  │
│ - Bone hierarchy│ │ - Local xforms  │ │ - Gait timing   │
│ - Bind poses    │ │ - Global xforms │ │ - Foot targets  │
│ - Bone lengths  │ │ - Skin matrices │ │ - Body motion   │
└─────────────────┘ └─────────────────┘ └─────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                     IKSystem                                │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐            │
│  │ TwoBoneIK  │  │ FABRIKSolver│ │ CCDSolver  │            │
│  │ (limbs)    │  │ (spine/tail)│ │ (flexible) │            │
│  └────────────┘  └────────────┘  └────────────┘            │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│              GPU Skinning (CreatureSkinned.hlsl)            │
│  - Bone matrix constant buffer (b1)                         │
│  - Dual quaternion skinning                                 │
│  - PBR lighting                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## Core Components

### 1. Skeleton (`src/animation/Skeleton.h/.cpp`)

The immutable bone hierarchy definition.

```cpp
namespace animation {

constexpr uint32_t MAX_BONES = 64;
constexpr uint32_t MAX_BONES_PER_VERTEX = 4;

struct BoneTransform {
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;

    glm::mat4 toMatrix() const;
    static BoneTransform lerp(const BoneTransform& a, const BoneTransform& b, float t);
};

struct Bone {
    std::string name;
    int32_t parentIndex;        // -1 for root
    BoneTransform bindPose;
    glm::mat4 inverseBindMatrix;
    float length;
    glm::vec3 minAngles, maxAngles;  // Joint constraints
};

class Skeleton {
    int32_t addBone(const std::string& name, int32_t parentIndex, const BoneTransform& bindPose);
    int32_t findBoneIndex(const std::string& name) const;
    const Bone& getBone(uint32_t index) const;
    std::vector<int32_t> getRootBones() const;
    std::vector<int32_t> getChildBones(int32_t parentIndex) const;
    glm::mat4 calculateBoneWorldTransform(uint32_t boneIndex) const;
};

// Factory functions
namespace SkeletonFactory {
    Skeleton createBiped(float height);
    Skeleton createQuadruped(float length, float height);
    Skeleton createSerpentine(float length, int segments);
    Skeleton createFlying(float wingspan);
    Skeleton createAquatic(float length, int segments);
}

}
```

### 2. Pose (`src/animation/Pose.h/.cpp`)

Runtime bone transforms and skinning data.

```cpp
namespace animation {

struct SkinWeight {
    std::array<uint32_t, 4> boneIndices;
    std::array<float, 4> weights;
    void normalize();
    void addInfluence(uint32_t boneIndex, float weight);
};

class SkeletonPose {
    // Per-bone transforms
    std::vector<BoneTransform> m_localTransforms;
    std::vector<glm::mat4> m_globalTransforms;
    std::vector<glm::mat4> m_skinningMatrices;

public:
    BoneTransform& getLocalTransform(uint32_t boneIndex);
    const glm::mat4& getGlobalTransform(uint32_t boneIndex) const;

    void calculateGlobalTransforms(const Skeleton& skeleton);
    void calculateSkinningMatrices(const Skeleton& skeleton, std::vector<glm::mat4>& out);
    void updateMatrices(const Skeleton& skeleton);  // Both global and skinning

    void setToBindPose(const Skeleton& skeleton);
    static SkeletonPose lerp(const SkeletonPose& a, const SkeletonPose& b, float t);
    void blendMasked(const SkeletonPose& other, float weight, const std::vector<bool>& mask);
};

// Skinned vertex for GPU upload
struct SkinnedVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::uvec4 boneIndices;
    glm::vec4 boneWeights;
};

}
```

### 3. IK Solvers (`src/animation/IKSolver.h/.cpp`)

Three inverse kinematics algorithms with unified interface.

```cpp
namespace animation {

struct IKConfig {
    uint32_t maxIterations = 10;
    float tolerance = 0.001f;
    float damping = 1.0f;
    float softLimit = 0.99f;
    float minBendAngle = 0.01f;
};

struct IKTarget {
    glm::vec3 position;
    std::optional<glm::quat> rotation;
    float weight = 1.0f;
};

struct PoleVector {
    glm::vec3 position;
    float weight = 1.0f;
    bool enabled = false;
};

// Two-Bone IK - Analytical solution for limbs
class TwoBoneIK {
    bool solve(const Skeleton& skeleton, SkeletonPose& pose,
               uint32_t upperBone, uint32_t lowerBone, uint32_t effectorBone,
               const IKTarget& target) const;

    bool solveWithPole(/* same args */, const PoleVector& pole) const;
};

// FABRIK - Iterative for long chains
class FABRIKSolver {
    bool solve(const Skeleton& skeleton, SkeletonPose& pose,
               const std::vector<uint32_t>& chainBones,
               const IKTarget& target) const;

    bool solveConstrained(/* same args */,
                          const std::vector<glm::vec2>& angleConstraints) const;
};

// CCD - Iterative with damping
class CCDSolver {
    bool solve(const Skeleton& skeleton, SkeletonPose& pose,
               const std::vector<uint32_t>& chainBones,
               const IKTarget& target) const;
};

// IK System Manager
class IKSystem {
    enum class SolverType { TwoBone, FABRIK, CCD };

    ChainHandle addChain(const IKChain& chain, SolverType type, uint32_t priority);
    void setTarget(ChainHandle handle, const IKTarget& target);
    void setPoleVector(ChainHandle handle, const PoleVector& pole);
    void solve(const Skeleton& skeleton, SkeletonPose& pose);
};

}
```

### 4. Procedural Locomotion (`src/animation/ProceduralLocomotion.h/.cpp`)

Gait-based procedural animation with ground adaptation.

```cpp
namespace animation {

enum class GaitType {
    Walk, Trot, Canter, Gallop, Crawl, Fly, Swim, Hover, Custom
};

struct FootConfig {
    uint32_t hipBoneIndex, kneeBoneIndex, ankleBoneIndex, footBoneIndex;
    float liftHeight = 0.15f;
    float stepLength = 0.3f;
    float phaseOffset = 0.0f;
    glm::vec3 restOffset;
};

struct WingConfig {
    uint32_t shoulderBoneIndex, elbowBoneIndex, wristBoneIndex, tipBoneIndex;
    float flapAmplitude = 45.0f;
    float flapSpeed = 1.0f;
};

struct SpineConfig {
    std::vector<uint32_t> boneIndices;
    float waveMagnitude, waveFrequency, waveSpeed;
};

struct FootPlacement {
    glm::vec3 targetPosition;
    glm::vec3 groundNormal;
    bool isGrounded;
    float blendWeight;
    float stepProgress;
};

using GroundCallback = std::function<bool(const glm::vec3& origin,
    const glm::vec3& direction, float maxDistance,
    glm::vec3& hitPoint, glm::vec3& hitNormal)>;

class ProceduralLocomotion {
    void initialize(const Skeleton& skeleton);

    void addFoot(const FootConfig& config);
    void addWing(const WingConfig& config);
    void setSpine(const SpineConfig& config);

    void setGaitType(GaitType type);
    void setVelocity(const glm::vec3& velocity);
    void setBodyPosition(const glm::vec3& pos);
    void setBodyRotation(const glm::quat& rot);
    void setGroundCallback(GroundCallback callback);

    void update(float deltaTime);
    void applyToPose(const Skeleton& skeleton, SkeletonPose& pose, IKSystem& ikSystem);

    float getGaitPhase() const;
    bool isMoving() const;
    glm::vec3 getBodyOffset() const;
    glm::quat getBodyTilt() const;
};

// Preset gait configurations
namespace GaitPresets {
    GaitTiming bipedWalk();
    GaitTiming bipedRun();
    GaitTiming quadrupedWalk();
    GaitTiming quadrupedTrot();
    GaitTiming quadrupedGallop();
    GaitTiming hexapodWalk();
    GaitTiming octopodWalk();
}

// Setup helpers
namespace LocomotionSetup {
    void setupBiped(ProceduralLocomotion& loco, const Skeleton& skeleton);
    void setupQuadruped(ProceduralLocomotion& loco, const Skeleton& skeleton);
    void setupFlying(ProceduralLocomotion& loco, const Skeleton& skeleton);
    void setupAquatic(ProceduralLocomotion& loco, const Skeleton& skeleton);
    void setupSerpentine(ProceduralLocomotion& loco, const Skeleton& skeleton);
}

}
```

### 5. CreatureAnimator (`src/animation/Animation.h`)

High-level animation controller combining all systems.

```cpp
namespace animation {

class CreatureAnimator {
    Skeleton m_skeleton;
    SkeletonPose m_pose;
    ProceduralLocomotion m_locomotion;
    IKSystem m_ikSystem;

public:
    // Initialize for creature type
    void initializeBiped(float height = 1.0f);
    void initializeQuadruped(float length = 1.0f, float height = 0.5f);
    void initializeFlying(float wingspan = 1.5f);
    void initializeAquatic(float length = 1.0f);
    void initializeSerpentine(float length = 2.0f);

    // Access components
    const Skeleton& getSkeleton() const;
    SkeletonPose& getPose();
    ProceduralLocomotion& getLocomotion();
    IKSystem& getIKSystem();

    // Update each frame
    void update(float deltaTime);

    // Set movement
    void setVelocity(const glm::vec3& velocity);
    void setAngularVelocity(float omega);
    void setPosition(const glm::vec3& position);
    void setRotation(const glm::quat& rotation);

    // GPU data
    const std::vector<glm::mat4>& getSkinningMatrices() const;
};

// GPU upload structure
struct GPUSkinningData {
    static constexpr uint32_t MAX_BONES = 64;
    glm::mat4 boneMatrices[MAX_BONES];
    uint32_t activeBoneCount;

    void uploadFromPose(const SkeletonPose& pose);
};

}
```

---

## Class Diagrams

### Skeletal Animation Hierarchy

```
Skeleton (immutable definition)
    │
    ├── Bone[] (indexed array)
    │       ├── name: string
    │       ├── parentIndex: int32
    │       ├── bindPose: BoneTransform
    │       ├── inverseBindMatrix: mat4
    │       └── length: float
    │
    └── Methods
            ├── addBone()
            ├── findBoneIndex()
            ├── getRootBones()
            └── getChildBones()

SkeletonPose (mutable runtime state)
    │
    ├── localTransforms: BoneTransform[]
    ├── globalTransforms: mat4[]
    ├── skinningMatrices: mat4[]
    │
    └── Methods
            ├── calculateGlobalTransforms()
            ├── calculateSkinningMatrices()
            ├── lerp()
            └── blendMasked()
```

### IK System Hierarchy

```
IKSystem (manager)
    │
    ├── TwoBoneIK
    │       └── solve() - Analytical (Law of Cosines)
    │
    ├── FABRIKSolver
    │       └── solve() - Iterative forward/backward
    │
    └── CCDSolver
            └── solve() - Cyclic coordinate descent

Common interfaces:
    ├── IKConfig (maxIterations, tolerance, damping)
    ├── IKTarget (position, rotation, weight)
    ├── PoleVector (position, weight, enabled)
    └── IKChain (startBone, endBone, length)
```

### Locomotion System

```
ProceduralLocomotion
    │
    ├── State
    │       ├── velocity: vec3
    │       ├── bodyPosition: vec3
    │       ├── bodyRotation: quat
    │       └── gaitPhase: float
    │
    ├── Configuration
    │       ├── feet: FootConfig[]
    │       ├── wings: WingConfig[]
    │       └── spine: SpineConfig
    │
    ├── Output
    │       ├── footPlacements: FootPlacement[]
    │       ├── bodyOffset: vec3
    │       └── bodyTilt: quat
    │
    └── Methods
            ├── update(deltaTime)
            └── applyToPose(skeleton, pose, ikSystem)
```

---

## Data Flow

### Per-Frame Animation Update

```
┌──────────────────────────────────────────────────────────────┐
│                    Creature::update()                        │
│  1. Update position, velocity, rotation from physics         │
│  2. Update creature state (fear, energy, etc.)               │
└──────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌──────────────────────────────────────────────────────────────┐
│               CreatureAnimator::update(deltaTime)            │
│  1. Pass body state to locomotion:                           │
│     m_locomotion.setBodyPosition(m_position)                 │
│     m_locomotion.setVelocity(m_velocity)                     │
│                                                              │
│  2. Update locomotion timing:                                │
│     m_locomotion.update(deltaTime)                           │
│     - Updates gait phase                                     │
│     - Calculates foot targets                                │
│     - Computes body bob/sway                                 │
│                                                              │
│  3. Apply to pose with IK:                                   │
│     m_locomotion.applyToPose(m_skeleton, m_pose, m_ikSystem) │
│     - Solves IK for each leg                                 │
│     - Applies wing/spine procedural animation                │
│                                                              │
│  4. Finalize matrices:                                       │
│     m_pose.updateMatrices(m_skeleton)                        │
│     - Calculates global transforms                           │
│     - Calculates skinning matrices                           │
└──────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌──────────────────────────────────────────────────────────────┐
│                  Renderer::uploadBoneData()                  │
│  GPUSkinningData data;                                       │
│  data.uploadFromPose(animator.getPose());                    │
│  cmdList->SetGraphicsRoot32BitConstants(boneBufferSlot,      │
│                                          data, 0);           │
└──────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌──────────────────────────────────────────────────────────────┐
│              GPU: CreatureSkinned.hlsl                       │
│  VSMain:                                                     │
│  1. Read bone matrices from cbuffer (b1)                     │
│  2. Apply dual quaternion skinning:                          │
│     skinnedPos = skinPositionDQ(pos, indices, weights)       │
│     skinnedNormal = skinNormal(normal, indices, weights)     │
│  3. Apply instance transform                                 │
│  4. Output to rasterizer                                     │
│                                                              │
│  PSMain:                                                     │
│  1. Generate procedural skin pattern                         │
│  2. Apply PBR lighting                                       │
│  3. Add subsurface scattering                                │
│  4. Apply fog                                                │
└──────────────────────────────────────────────────────────────┘
```

### IK Solving Flow

```
Input: Target foot position from gait
           │
           ▼
┌──────────────────────────────────┐
│  IKSystem::solve(skeleton, pose) │
│                                  │
│  For each enabled chain:         │
│   ├─ TwoBone: Hip → Knee → Ankle │
│   │    Uses Law of Cosines       │
│   │    O(1) complexity           │
│   │                              │
│   ├─ FABRIK: Spine chain         │
│   │    Forward-backward reaching │
│   │    O(n) per iteration        │
│   │                              │
│   └─ CCD: Tail/tentacle          │
│        Per-joint rotation        │
│        O(n²) per iteration       │
└──────────────────────────────────┘
           │
           ▼
Output: Modified local transforms in pose
```

---

## Integration Points

### 1. Creature Entity Integration

```cpp
// In Creature.h - Add animation member
class Creature {
private:
    animation::CreatureAnimator m_animator;
    bool m_animationEnabled = true;

public:
    void initializeAnimation() {
        switch (type) {
            case CreatureType::HERBIVORE:
            case CreatureType::CARNIVORE:
                m_animator.initializeQuadruped(genome.size, genome.size * 0.5f);
                break;
            case CreatureType::FLYING:
                m_animator.initializeFlying(genome.wingSpan);
                break;
            case CreatureType::AQUATIC:
                m_animator.initializeAquatic(genome.size);
                break;
        }

        // Set up ground raycast callback
        m_animator.getLocomotion().setGroundCallback(
            [this](const glm::vec3& origin, const glm::vec3& dir,
                   float maxDist, glm::vec3& hit, glm::vec3& normal) {
                return terrain->raycast(origin, dir, maxDist, hit, normal);
            }
        );
    }

    void update(float deltaTime) {
        // ... existing update logic ...

        // Update animation
        if (m_animationEnabled) {
            m_animator.setPosition(position);
            m_animator.setRotation(glm::quat(glm::vec3(0, rotation, 0)));
            m_animator.setVelocity(velocity);
            m_animator.update(deltaTime);
        }
    }

    const animation::CreatureAnimator& getAnimator() const { return m_animator; }
};
```

### 2. Renderer Integration

```cpp
// In CreatureRenderer_DX12.cpp
void CreatureRendererDX12::render(
    const std::vector<std::unique_ptr<Creature>>& creatures,
    ICommandList* cmdList, float time)
{
    // Bind skinned creature pipeline
    cmdList->SetPipelineState(m_skinnedPipeline);

    for (const auto& creature : creatures) {
        const auto& animator = creature->getAnimator();

        // Upload bone matrices
        GPUSkinningData boneData;
        boneData.uploadFromPose(animator.getPose());

        cmdList->SetGraphicsRoot32BitConstants(
            BONE_BUFFER_SLOT,
            sizeof(boneData) / 4,
            &boneData, 0);

        // Set per-creature constants and draw
        // ...
    }
}
```

### 3. Mesh Generation Integration

```cpp
// In CreatureBuilder.cpp
SkinnedMeshData CreatureBuilder::buildSkinnedMesh(
    const Genome& genome,
    const animation::Skeleton& skeleton)
{
    SkinnedMeshData mesh;

    // Generate metaball mesh
    MetaballSystem metaballs;
    buildCreatureMetaballs(genome, metaballs);

    // Convert to skinned vertices
    auto vertices = metaballs.generateMesh();

    // Auto-assign bone weights
    animation::SkinningUtils::autoSkinWeights(
        vertices.positions,
        skeleton,
        mesh.skinWeights,
        0.5f  // falloff radius
    );

    // Build final skinned vertices
    for (size_t i = 0; i < vertices.size(); ++i) {
        SkinnedVertex v;
        v.position = vertices.positions[i];
        v.normal = vertices.normals[i];
        v.texCoord = vertices.texCoords[i];
        v.boneIndices = glm::uvec4(
            mesh.skinWeights[i].boneIndices[0],
            mesh.skinWeights[i].boneIndices[1],
            mesh.skinWeights[i].boneIndices[2],
            mesh.skinWeights[i].boneIndices[3]
        );
        v.boneWeights = glm::vec4(
            mesh.skinWeights[i].weights[0],
            mesh.skinWeights[i].weights[1],
            mesh.skinWeights[i].weights[2],
            mesh.skinWeights[i].weights[3]
        );
        mesh.vertices.push_back(v);
    }

    mesh.skeleton = &skeleton;
    return mesh;
}
```

---

## GPU Skinning Pipeline

### Shader Constant Buffer Layout (b1)

```hlsl
cbuffer BoneMatrices : register(b1)
{
    float4x4 boneMatrices[64];  // MAX_BONES
    uint activeBoneCount;
    float3 bonePadding;
};
```

### Vertex Input Layout

```hlsl
struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
    uint4 boneIndices : BLENDINDICES;
    float4 boneWeights : BLENDWEIGHT;

    // Instanced data
    float4 instModelRow0 : INST_MODEL0;
    float4 instModelRow1 : INST_MODEL1;
    float4 instModelRow2 : INST_MODEL2;
    float4 instModelRow3 : INST_MODEL3;
    float4 instColorType : INST_COLOR;
};
```

### Skinning Implementation

```hlsl
// Linear blend skinning
float3 skinPosition(float3 pos, uint4 indices, float4 weights)
{
    float3 result = float3(0, 0, 0);
    [unroll]
    for (int i = 0; i < 4; i++) {
        if (weights[i] > 0.0001f) {
            result += mul(boneMatrices[indices[i]], float4(pos, 1.0)).xyz * weights[i];
        }
    }
    return result;
}

// Dual quaternion skinning (better joint deformation)
float3 skinPositionDQ(float3 pos, uint4 indices, float4 weights)
{
    DualQuat blended = blendDualQuats(indices, weights);
    return transformPointDQ(blended, pos);
}
```

---

## Usage Examples

### Example 1: Basic Quadruped Animation

```cpp
#include "animation/Animation.h"

void setupQuadrupedCreature(Creature& creature) {
    // Initialize animator
    creature.getAnimator().initializeQuadruped(1.0f, 0.5f);

    // Configure gait
    auto& loco = creature.getAnimator().getLocomotion();
    loco.setGaitType(animation::GaitType::Walk);

    // Set ground callback for foot IK
    loco.setGroundCallback([&](const glm::vec3& origin, const glm::vec3& dir,
                                float maxDist, glm::vec3& hit, glm::vec3& normal) {
        return terrain->raycast(origin, dir, maxDist, hit, normal);
    });
}

void updateCreature(Creature& creature, float deltaTime) {
    // Update creature physics/AI
    creature.updatePhysics(deltaTime);

    // Sync animation with movement
    creature.getAnimator().setPosition(creature.getPosition());
    creature.getAnimator().setVelocity(creature.getVelocity());
    creature.getAnimator().setRotation(creature.getRotation());

    // Update animation
    creature.getAnimator().update(deltaTime);
}

void renderCreature(const Creature& creature, ICommandList* cmd) {
    // Get skinning matrices
    const auto& matrices = creature.getAnimator().getSkinningMatrices();

    // Upload to GPU
    GPUSkinningData boneData;
    boneData.uploadFromPose(creature.getAnimator().getPose());
    cmd->SetGraphicsRoot32BitConstants(BONE_SLOT, sizeof(boneData)/4, &boneData, 0);

    // Draw skinned mesh
    cmd->DrawIndexedInstanced(creature.getMesh().indexCount, 1, 0, 0, 0);
}
```

### Example 2: Flying Creature with Wing Animation

```cpp
void setupFlyingCreature(Creature& creature, float wingspan) {
    creature.getAnimator().initializeFlying(wingspan);

    auto& loco = creature.getAnimator().getLocomotion();
    loco.setGaitType(animation::GaitType::Fly);

    // Configure wing flapping from genome
    animation::WingConfig leftWing, rightWing;
    leftWing.flapAmplitude = 45.0f;
    leftWing.flapSpeed = creature.getGenome().flapFrequency;
    rightWing = leftWing;
    rightWing.phaseOffset = 0.5f;  // Opposite phase

    loco.addWing(leftWing);
    loco.addWing(rightWing);
}
```

### Example 3: Custom IK for Reaching

```cpp
void makeCreatureReach(Creature& creature, const glm::vec3& targetPos) {
    auto& ikSystem = creature.getAnimator().getIKSystem();
    const auto& skeleton = creature.getAnimator().getSkeleton();

    // Find arm bones
    int32_t shoulder = skeleton.findBoneIndex("LeftShoulder");
    int32_t elbow = skeleton.findBoneIndex("LeftElbow");
    int32_t hand = skeleton.findBoneIndex("LeftHand");

    // Add two-bone IK chain
    animation::IKChain armChain{
        static_cast<uint32_t>(shoulder),
        static_cast<uint32_t>(hand),
        2  // chain length
    };

    auto handle = ikSystem.addChain(armChain, animation::IKSystem::SolverType::TwoBone, 10);

    // Set target
    animation::IKTarget target;
    target.position = targetPos;
    target.weight = 1.0f;
    ikSystem.setTarget(handle, target);
}
```

---

## Performance Considerations

### CPU Performance

| Operation | Complexity | Typical Time |
|-----------|------------|--------------|
| Gait phase update | O(1) | < 0.01ms |
| Foot placement | O(feet) | < 0.05ms |
| Two-Bone IK | O(1) | < 0.01ms |
| FABRIK (10 bones) | O(n * iter) | < 0.1ms |
| Global transform calc | O(bones) | < 0.05ms |
| Skinning matrix calc | O(bones) | < 0.05ms |

### GPU Performance

| Operation | Cost |
|-----------|------|
| Bone matrix upload | ~4KB per creature |
| Dual quat skinning | ~50 ALU ops/vertex |
| PBR lighting | ~100 ALU ops/pixel |

### Optimization Tips

1. **Bone LOD**: Reduce active bones at distance
2. **Gait caching**: Cache common gait patterns
3. **Batch IK**: Solve all IK chains together
4. **GPU instancing**: Share bone buffers for same skeleton
5. **Pose caching**: Cache bind pose matrices

---

## File Reference

| File | Purpose | Lines |
|------|---------|-------|
| `src/animation/Skeleton.h` | Bone hierarchy definition | 113 |
| `src/animation/Skeleton.cpp` | Skeleton implementation | ~500 |
| `src/animation/Pose.h` | Runtime pose and skinning | 135 |
| `src/animation/Pose.cpp` | Pose implementation | ~300 |
| `src/animation/IKSolver.h` | IK solver interfaces | 217 |
| `src/animation/IKSolver.cpp` | IK implementations | ~700 |
| `src/animation/ProceduralLocomotion.h` | Gait controller | 227 |
| `src/animation/ProceduralLocomotion.cpp` | Locomotion implementation | ~650 |
| `src/animation/Animation.h` | Unified controller | 198 |
| `src/animation/SwimAnimator.h/.cpp` | Aquatic animation | ~600 |
| `src/animation/WingAnimator.h` | Flying animation | ~500 |
| `shaders/hlsl/CreatureSkinned.hlsl` | GPU skinning shader | 553 |

---

## Summary

The OrganismEvolution animation system provides:

1. **Complete Skeletal Animation**: Bone hierarchy, bind poses, runtime poses, and GPU skinning
2. **Multiple IK Solvers**: Two-Bone (analytical), FABRIK (iterative), CCD (flexible)
3. **Procedural Locomotion**: Gait patterns for all creature types with ground adaptation
4. **Specialized Animators**: Wing and swim animation systems
5. **GPU Integration**: Dual quaternion skinning shader with PBR lighting

The architecture is modular, performant, and ready for integration with the existing creature system.

---

*Document generated from animation system analysis. Implementation is complete and ready for creature integration.*
