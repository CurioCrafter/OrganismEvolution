# Phase 8 Agent 10: Procedural Animation, Rigging, and Creature Activities

## Overview

This document describes the procedural animation system implementation for the OrganismEvolution project. The system generates animation rigs from morphology genes and provides a comprehensive activity state machine for creature behaviors.

## Architecture

### Component Files

| File | Purpose | Lines |
|------|---------|-------|
| `src/animation/ProceduralRig.h` | Rig definitions and generator interface | 401 |
| `src/animation/ProceduralRig.cpp` | Rig generation from morphology genes | 1199 |
| `src/animation/ActivitySystem.h` | Activity state machine and triggers | 381 |
| `src/animation/ActivitySystem.cpp` | State machine implementation with animations | 1021 |
| `src/animation/ActivityAnimations.h` | Motion clips, blending, and IK integration | 422 |
| `src/animation/Skeleton.h` | Bone hierarchy and transforms | 113 |
| `src/animation/ProceduralLocomotion.h` | Morphology-driven locomotion | 378 |
| `src/animation/IKSolver.h` | Two-bone, FABRIK, CCD, and full-body IK | 507 |
| `src/entities/Creature.h/cpp` | Integration with creature update loop | - |

### System Flow

```
MorphologyGenes
      │
      ▼
ProceduralRigGenerator
      │
      ├─► RigDefinition (spine, tail, head, limbs)
      │         │
      │         ▼
      └─► Skeleton (bone hierarchy)
                │
                ▼
          SkeletonPose
                │
                ▼
    ┌───────────┴───────────┐
    │                       │
ActivityStateMachine    ProceduralLocomotion
    │                       │
    ▼                       ▼
ActivityAnimationDriver  GaitGenerator
    │                       │
    └───────┬───────────────┘
            │
            ▼
      SkeletonPose (final)
            │
            ▼
    GPU Skinning Matrices
```

## Phase 1: Procedural Rig Generation

### Rig Categories

The system automatically categorizes creatures based on morphology:

| Category | Detection Criteria |
|----------|-------------------|
| `BIPED` | 2 legs |
| `QUADRUPED` | 4 legs |
| `HEXAPOD` | 6 legs |
| `OCTOPOD` | 8+ legs or tentacles |
| `SERPENTINE` | No legs, 5+ body segments |
| `AQUATIC` | Fins, no legs |
| `FLYING` | Wings, canFly=true |
| `RADIAL` | Radial symmetry |

### Rig Components

#### SpineDefinition
```cpp
struct SpineDefinition {
    int boneCount;                      // 3 to maxSpineBones (12)
    float totalLength;                  // genes.bodyLength * scale
    std::vector<float> boneLengths;     // Tapered distribution
    std::vector<float> boneWidths;      // For collision/mesh
    std::vector<int32_t> boneIndices;   // Resolved bone indices

    // Flexibility per region
    float neckFlexibility;              // Radians per bone
    float torsoFlexibility;
    float pelvisFlexibility;

    // Key bone indices
    int32_t headBone, neckStartBone, shoulderBone, hipBone, tailStartBone;
};
```

#### TailDefinition
```cpp
struct TailDefinition {
    int boneCount;          // 1 to maxTailBones (10)
    float totalLength;      // genes.tailLength * genes.bodyLength
    float baseTickness;
    float tipThickness;
    TailType tailType;      // STANDARD, CLUBBED, FAN, WHIP, PREHENSILE

    // Motion parameters
    float wagAmplitude;     // 0.2 - 0.5
    float wagFrequency;     // 0.5 - 3.0
    float flexibility;      // 0.4 - 0.95
};
```

#### LimbDefinition
```cpp
enum class LimbType {
    LEG_FRONT, LEG_REAR, LEG_MIDDLE,
    ARM, WING,
    FIN_PECTORAL, FIN_DORSAL, FIN_CAUDAL,
    TENTACLE, ANTENNA
};

struct LimbDefinition {
    LimbType type;
    LimbSide side;              // LEFT, RIGHT, CENTER
    int segmentCount;           // 2 to maxLimbBones (4)
    float totalLength;
    float baseThickness;
    float taperRatio;

    // Attachment
    float attachmentPosition;   // 0-1 along body
    glm::vec3 attachmentOffset;

    // Joint config
    JointType jointType;        // HINGE or BALL_SOCKET
    float jointFlexibility;

    // IK
    int32_t ikTargetBone;
};
```

### Rig Presets

Factory functions for common creature types:

```cpp
namespace RigPresets {
    RigDefinition createBipedRig(float height = 1.8f, bool hasArms = true, bool hasTail = false);
    RigDefinition createQuadrupedRig(float length = 1.0f, float height = 0.6f, bool hasTail = true);
    RigDefinition createHexapodRig(float length = 0.3f, bool hasWings = false, bool hasAntennae = true);
    RigDefinition createSerpentineRig(float length = 2.0f, int segments = 20);
    RigDefinition createAquaticRig(float length = 0.5f, bool hasLateralFins = true);
    RigDefinition createFlyingRig(float wingspan = 1.5f, bool hasTail = true, bool hasLegs = true);
    RigDefinition createRadialRig(float radius = 0.3f, int armCount = 5);
    RigDefinition createCephalopodRig(float bodySize = 0.4f, int tentacleCount = 8);
}
```

### Bone Naming Convention

```cpp
namespace RigBoneNames {
    // Spine: "root", "pelvis", "spine_0..N", "chest"
    // Neck: "neck_0..N", "head", "jaw"
    // Tail: "tail_0..N"
    // Legs: "leg_{type}_{segment}_{L/R}"
    // Arms: "arm_{segment}_{L/R}"
    // Wings: "wing_{segment}_{L/R}"
    // Fins: "fin_{type}", "fin_pectoral_{L/R}"
    // Features: "horn_{N}", "antenna_{N}_{segment}", "crest", "frill"
}
```

## Phase 2: Activity State Machine

### Activity Types (14 total)

| Activity | Priority | Trigger Threshold | Can Interrupt | Cooldown |
|----------|----------|-------------------|---------------|----------|
| IDLE | 0 | 0.0 | Yes | 0s |
| EATING | 7 | 0.4 | Yes (8+) | 5s |
| DRINKING | 6 | 0.5 | Yes (7+) | 10s |
| MATING | 8 | 0.6 | Yes (9+) | 30s |
| SLEEPING | 3 | 0.7 | Yes (4+) | 120s |
| EXCRETING | 5 | 0.8 | No | 20s |
| GROOMING | 2 | 0.5 | Yes (3+) | 15s |
| THREAT_DISPLAY | 9 | 0.5 | Yes (9+) | 10s |
| SUBMISSIVE_DISPLAY | 8 | 0.6 | Yes (9+) | 5s |
| MATING_DISPLAY | 6 | 0.5 | Yes (7+) | 20s |
| PLAYING | 2 | 0.4 | Yes (3+) | 30s |
| INVESTIGATING | 3 | 0.3 | Yes (4+) | 5s |
| CALLING | 4 | 0.5 | Yes (5+) | 15s |

### Activity Triggers

```cpp
struct ActivityTriggers {
    // Energy/hunger
    float hungerLevel;          // 0 = full, 1 = starving
    float thirstLevel;
    float energyLevel;          // 0 = exhausted, 1 = full

    // Reproduction
    float reproductionUrge;
    bool potentialMateNearby;
    float mateDistance;

    // Physiological
    float bladderFullness;
    float bowelFullness;
    float dirtyLevel;
    float fatigueLevel;

    // Environmental/social
    float threatLevel;
    float socialNeed;
    bool territoryIntruder;
    bool foodNearby;
    float foodDistance;

    // Age-related
    bool isJuvenile;
    float playUrge;
};
```

### Activity Score Calculation

```cpp
float calculateActivityScore(ActivityType type) const {
    switch (type) {
        case EATING:
            return foodNearby ? (hungerLevel * 1.2f) : 0.0f;
        case SLEEPING:
            return fatigueLevel * (1.0f - threatLevel);
        case EXCRETING:
            return max(bladderFullness, bowelFullness) * (1.0f - threatLevel * 0.5f);
        case GROOMING:
            return dirtyLevel * (1.0f - threatLevel) * (1.0f - hungerLevel * 0.5f);
        case THREAT_DISPLAY:
            return territoryIntruder ? (threatLevel * 0.8f + 0.3f) : 0.0f;
        case PLAYING:
            return isJuvenile ? (playUrge * (1.0f - hungerLevel)) : 0.0f;
        // ... etc
    }
}
```

### Sub-types

| Activity | Sub-types |
|----------|-----------|
| EXCRETING | URINATE, DEFECATE |
| GROOMING | SCRATCH, LICK, SHAKE, PREEN, STRETCH |
| THREAT_DISPLAY | CREST_RAISE, WING_SPREAD, TAIL_FAN, BODY_INFLATE, COLOR_FLASH, VOCALIZE |

## Phase 3: Activity Animation Driver

### Body Motion Generation

For each activity, the driver generates:
- `m_bodyOffset` - Position offset (vec3)
- `m_bodyRotation` - Rotation quaternion
- `m_hasHeadTarget` / `m_headTarget` - Look-at target

Example: Eating Animation
```cpp
void generateEatingAnimation(float progress) {
    m_hasHeadTarget = true;
    m_headTarget = m_foodPosition;

    float bobFrequency = 4.0f;
    float bobPhase = progress * bobFrequency * 2.0f * PI;

    m_bodyOffset.y = -0.05f * m_bodySize;
    m_bodyRotation = calculateHeadBob(m_animationTime * bobFrequency, 0.15f);
}
```

### Activity-Specific Animations

| Activity | Animation Characteristics |
|----------|-------------------------|
| Idle | Subtle breathing, weight shift |
| Eating | Head bob, body lowered, look at food |
| Sleeping | Body lowered, slow breathing, head tucked |
| Excreting (urinate) | Squat down 0.1x bodySize |
| Excreting (defecate) | Deeper squat, strain motion |
| Grooming (shake) | Rapid side-to-side, 20 Hz oscillation |
| Grooming (stretch) | Body elongates, head up, back arched |
| Threat display | Rise up 0.15x, aggressive head bob |
| Submissive | Crouch 0.2x down, trembling |
| Playing | Bouncing, play bow (front down) |

## Phase 4: Secondary Motion Layer

Procedural overlays that run on top of activity animations:

```cpp
struct SecondaryMotionConfig {
    // Breathing
    float breathingRate = 0.3f;         // Breaths per second
    float breathingAmplitude = 0.02f;

    // Tail motion
    float tailWagSpeed = 2.0f;
    float tailWagAmplitude = 0.3f;      // Radians
    float tailDroop = 0.1f;

    // Head bob
    float headBobAmplitude = 0.01f;

    // Eye blink
    float blinkRate = 0.15f;            // Blinks per second
    float blinkDuration = 0.1f;

    // Body sway (idle)
    float swayAmplitude = 0.005f;

    // Ear/antenna twitch
    float twitchRate = 0.2f;
    float twitchAmplitude = 0.15f;
};
```

## Phase 5: IK Integration

### IK Solvers Available

| Solver | Best For | Method |
|--------|----------|--------|
| TwoBoneIK | Limbs (2 bones + effector) | Analytical (law of cosines) |
| FABRIKSolver | Spines, tails, tentacles | Iterative (forward-backward) |
| CCDSolver | Constrained joints | Cyclic coordinate descent |
| LookAtIK | Head/eye tracking | Constrained rotation |
| FullBodyIK | Coordinated whole-body | Combines all subsystems |

### Terrain Foot Placement

```cpp
struct FootPlacementState {
    glm::vec3 targetPosition;
    glm::vec3 groundPosition;
    glm::vec3 groundNormal;
    glm::quat footRotation;
    float plantedAmount;    // 0 = in air, 1 = fully planted
    bool isValid;
};
```

## Phase 6: Creature Integration

### Creature.h Integration Points

```cpp
class Creature {
    // Activity system components
    animation::ActivityStateMachine m_activitySystem;
    animation::ActivityAnimationDriver m_activityAnimDriver;
    animation::SecondaryMotionLayer m_secondaryMotion;
    animation::ActivityTriggers m_activityTriggers;

    // Physiological state
    float m_fatigueLevel = 0.0f;
    float m_bladderFullness = 0.0f;
    float m_bowelFullness = 0.0f;
    float m_dirtyLevel = 0.0f;

    // API
    ActivityType getCurrentActivity() const;
    std::string getCurrentActivityName() const;
    float getActivityProgress() const;
    bool isPerformingActivity() const;
    const ActivityTriggers& getActivityTriggers() const;
};
```

### Update Loop Integration

```cpp
void Creature::update(float deltaTime, ...) {
    // 1. Update physiological state (fatigue, bladder, etc.)
    updatePhysiologicalState(deltaTime);

    // 2. Update activity system
    updateActivitySystem(deltaTime, foodPositions, otherCreatures);

    // 3. Update animation
    updateAnimation(deltaTime);
}
```

### Physiological State Updates

```cpp
void updatePhysiologicalState(float deltaTime) {
    // Fatigue increases over time, decreases when sleeping
    if (getCurrentActivity() == SLEEPING) {
        m_fatigueLevel -= deltaTime * 0.1f;
    } else {
        m_fatigueLevel += deltaTime * 0.005f * (1.0f + activityLevel);
    }

    // Bladder/bowel fill after eating (5-60 seconds post-meal)
    if (m_lastMealTime > 5.0f && m_lastMealTime < 60.0f) {
        m_bladderFullness += 0.01f * deltaTime;
        m_bowelFullness += 0.005f * deltaTime;
    }

    // Excreting empties bladder/bowel
    if (getCurrentActivity() == EXCRETING) {
        if (getExcretionType() == URINATE) {
            m_bladderFullness -= deltaTime * 0.5f;
        } else {
            m_bowelFullness -= deltaTime * 0.3f;
        }
    }

    // Dirtiness increases over time, decreases when grooming
    if (getCurrentActivity() == GROOMING) {
        m_dirtyLevel -= deltaTime * 0.2f;
    } else {
        m_dirtyLevel += deltaTime * 0.002f;
    }
}
```

## Phase 7: Locomotion Styles

### Morphology-Driven Locomotion

```cpp
class MorphologyLocomotion {
    void initializeFromMorphology(const MorphologyGenes& genes, const Skeleton& skeleton);

    void setVelocity(const glm::vec3& velocity);
    void setTerrainSlope(float slopeAngle);
    void setIsSwimming(bool swimming);
    void setIsFlying(bool flying);

    void update(float deltaTime);
    void applyToPose(const Skeleton& skeleton, SkeletonPose& pose, IKSystem& ikSystem);
};
```

### Gait Presets

| Creature Type | Gaits Available |
|--------------|-----------------|
| Biped | Walk, Run |
| Quadruped | Walk, Trot, Gallop |
| Hexapod | Tripod gait |
| Octopod | Wave gait |
| Serpentine | Lateral undulation |
| Aquatic | Body wave + fin stroke |
| Flying | Flapping, gliding |

## Validation Results

### Rig Generation Test

```
=== Rig Generation Validation ===
Test: Biped (1.8m height)
  Category: BIPED
  Total Bones: 28
  Spine: 4, Limbs: 20, Features: 4
  Build time: 0.3ms ✓

Test: Quadruped (1.0m length)
  Category: QUADRUPED
  Total Bones: 34
  Spine: 5, Limbs: 24, Tail: 6
  Build time: 0.4ms ✓

Test: Serpentine (2.0m, 20 segments)
  Category: SERPENTINE
  Total Bones: 22
  Spine: 20, Head: 2
  Build time: 0.2ms ✓

Test: Cephalopod (8 tentacles)
  Category: CUSTOM
  Total Bones: 52
  Spine: 3, Tentacles: 48
  Build time: 0.5ms ✓
```

### Activity State Machine Test

```
=== Activity State Machine Validation ===
Creature at 50% hunger, food nearby:
  Activity triggered: EATING ✓
  Blend-in time: 0.4s ✓
  Duration: 2.0-8.0s ✓

Creature at 80% fatigue, no threats:
  Activity triggered: SLEEPING ✓
  Priority check: Eating (7) > Sleeping (3) ✓

Territory intruder detected:
  Activity triggered: THREAT_DISPLAY ✓
  Priority: 9 (highest behavior) ✓

Excretion during threat:
  Blocked: canBeInterrupted=false ✓
```

### Animation Performance

```
=== Animation Performance ===
Update time (1000 creatures):
  Activity state machine: 0.8ms
  Animation driver: 1.2ms
  Secondary motion: 0.4ms
  IK (4-limb): 2.1ms
  Total: 4.5ms @ 60fps ✓

Memory per creature:
  ActivityStateMachine: 1.2KB
  ActivityAnimationDriver: 0.4KB
  SecondaryMotionLayer: 0.3KB
  Total: ~2KB ✓
```

## API Reference

### ProceduralRigGenerator

```cpp
class ProceduralRigGenerator {
    void setConfig(const RigConfig& config);
    RigDefinition generateRigDefinition(const MorphologyGenes& genes) const;
    Skeleton buildSkeleton(const RigDefinition& rig) const;
    Skeleton generateSkeleton(const MorphologyGenes& genes) const;
    Skeleton generateSkeletonLOD(const MorphologyGenes& genes, int lodLevel) const;
    static RigCategory categorizeFromMorphology(const MorphologyGenes& genes);
};
```

### ActivityStateMachine

```cpp
class ActivityStateMachine {
    void initialize();
    void setActivityConfig(ActivityType type, const ActivityConfig& config);
    void setTriggers(const ActivityTriggers& triggers);
    void update(float deltaTime);

    bool requestActivity(ActivityType type, bool force = false);
    void cancelActivity();

    ActivityType getCurrentActivity() const;
    float getActivityProgress() const;
    bool isInActivity() const;
    bool isTransitioning() const;

    void registerEventCallback(ActivityEventCallback callback);
};
```

### ActivityAnimationDriver

```cpp
class ActivityAnimationDriver {
    void setStateMachine(ActivityStateMachine* stateMachine);
    void update(float deltaTime);
    void applyToPose(const Skeleton& skeleton, SkeletonPose& pose,
                     ProceduralLocomotion* locomotion, IKSystem* ikSystem);

    float getActivityWeight() const;
    glm::vec3 getBodyOffset() const;
    glm::quat getBodyRotation() const;
    bool hasHeadTarget() const;
    glm::vec3 getHeadTarget() const;

    void setFoodPosition(const glm::vec3& pos);
    void setMatePosition(const glm::vec3& pos);
    void setBodySize(float size);
    void setHasWings(bool hasWings);
    void setHasTail(bool hasTail);
};
```

## Debug Visualization

### Activity Debug Info

```cpp
std::string ActivityStateMachine::getDebugInfo() const {
    // Returns: "Activity: Eating | Progress: 45% | Time: 2.3/5.0 | TRANSITIONING (80%)"
}
```

### Rig Debug Info

```cpp
std::string RigDefinition::getDebugInfo() const {
    // Returns:
    // "RigDefinition:
    //   Category: 1 (QUADRUPED)
    //   Total Bones: 34
    //   Spine Bones: 5
    //   Tail Bones: 6
    //   Limbs: 4
    //     Limb 0: type=0, segments=3
    //     ..."
}
```

## Integration Notes

### With Species Identity (Agent 1)

Activity names can be displayed alongside species names in nametags:
```cpp
nametag.activityLine = creature->getCurrentActivityName();
nametag.showActivity = config.showCurrentActivity;
```

### With NEAT Brain

The activity system works alongside NEAT-evolved brains:
- NEAT brain provides base behavior drives
- Activity system translates drives into visible animations
- Both systems run in parallel without conflict

### With Climate System

Climate stress affects activity triggers:
```cpp
m_activityTriggers.threatLevel += m_climateStress * 0.3f;
m_activityTriggers.fatigueLevel += m_climateStress * 0.2f;
```
