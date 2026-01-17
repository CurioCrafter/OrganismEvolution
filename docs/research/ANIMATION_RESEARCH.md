# Animation System Research Document

## OrganismEvolution Skeletal Animation Implementation

**Document Version:** 1.0
**Date:** January 2026
**Purpose:** Comprehensive research findings for implementing skeletal animation with IK solvers

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Existing Codebase Analysis](#existing-codebase-analysis)
3. [IK Algorithm Research](#ik-algorithm-research)
4. [ModularGameEngine Reference](#modulargameengine-reference)
5. [Recommended Architecture](#recommended-architecture)
6. [Integration Strategy](#integration-strategy)

---

## Executive Summary

The OrganismEvolution codebase has **exceptional foundations for animation integration**:

1. **Skeletal structure** is fully defined (`BodySegment` hierarchy in Morphology.h)
2. **Animation parameters** are genome-encoded (`flapFrequency`, `swimFrequency`)
3. **Visual state system** is complete (`VisualStateCalculator` in VisualIndicators.h)
4. **Locomotion system** exists (`CentralPatternGenerator`, gait patterns in Locomotion.h)
5. **Rendering infrastructure** supports animation (`animPhase` field already allocated)
6. **No core simulation changes needed** - animation hooks into render/visual layer only

### Key Finding

Animation can be **seamlessly integrated at the render pass level** without modifying core simulation code, by using existing `VisualState` and morphological data structures.

---

## Existing Codebase Analysis

### 1. Creature Data Structure (Creature.h/cpp)

**Location:** `src/entities/Creature.h` (148 lines) | `src/entities/Creature.cpp` (1050 lines)

#### Movement-Related Fields

```cpp
// Position and movement
glm::vec3 position;
glm::vec3 velocity;
float rotation;  // Yaw rotation for 2D movement

// Behavior timing
glm::vec3 wanderTarget;    // Persistent wander state
float m_wanderAngle;       // Per-instance wander angle (thread-safe)
float currentTime;         // Accumulated time for sensory system

// State tracking
float fear;                // 0-1 fear level (affects behavior)
float huntingCooldown;     // Time between attacks
int killCount;             // Predator kill tracking
bool beingHunted;          // Prey being targeted
```

#### Movement Update Cycle

The `Creature::update()` method follows this sequence:

1. **Age/Time tracking** - increments `age` and `currentTime`
2. **Energy metabolism** - applies movement, sensory, and metabolism costs
3. **Sensory updates** - calls `sensory.sense()` to gather environmental data
4. **Behavior selection** - dispatches to type-specific behavior
5. **Physics update** - applies steering forces and terrain collision
6. **Fitness calculation** - updates creature fitness score

#### Behavior-Specific Movement

The codebase handles **4 distinct creature types** with different movement patterns:

| Type | Key Behaviors | Animation Implications |
|------|--------------|----------------------|
| **Herbivore** | Flee (1.4x speed boost), flocking, arrive at food | Fear-based speed changes, group coordination |
| **Carnivore** | Pursuit, attack at 2.5 units, separation | Aggressive stance, attack animation |
| **Aquatic** | Schooling (boids), depth control, wander angle | Swimming undulation, schooling sync |
| **Flying** | Altitude maintenance, circling, gliding | Wing flapping, banking turns |

### 2. Genome/Genetic Data (Genome.h/cpp)

**Location:** `src/entities/Genome.h` (87 lines) | `src/entities/Genome.cpp` (404 lines)

#### Animation-Relevant Traits

```cpp
// Physical morphology
float size;               // 0.5-2.0 (scales all dimensions)
float speed;              // 5.0-20.0 (base movement speed)

// Flying creature traits (directly affect limb animation parameters)
float wingSpan;           // 0.5-2.0
float flapFrequency;      // 2.0-8.0 Hz (for wing beat animation!)
float glideRatio;         // 0.3-0.8 (higher = more gliding)
float preferredAltitude;  // 15.0-40.0 (height above terrain)

// Aquatic traits (for swimming animation)
float swimFrequency;      // 1.0-4.0 Hz (body wave frequency!)
float swimAmplitude;      // 0.1-0.3 (S-wave amplitude!)
float tailSize;           // 0.5-1.2 (tail propulsion size)
float preferredDepth;     // 0.1-0.5 (normalized)

// Sensory/display traits (for idle animation)
float displayIntensity;   // 0.0-1.0 (mating display strength)
float alarmCallVolume;    // 0.0-1.0 (affects breathing when calling)
```

**Critical observation:** The genome already encodes **animation parameters**:
- `flapFrequency` directly maps to wing beat cycles
- `swimFrequency` and `swimAmplitude` define swimming oscillations
- `displayIntensity` could drive mating display animations
- `size` scales all morphological features

### 3. DX12 Rendering Architecture (CreatureRenderer_DX12.h)

**Location:** `src/graphics/rendering/CreatureRenderer_DX12.h` (155 lines)

#### Instance Data Structure

```cpp
struct CreatureInstanceDataDX12 {
    float modelRow0[4];        // Model matrix row 0
    float modelRow1[4];        // Model matrix row 1
    float modelRow2[4];        // Model matrix row 2
    float modelRow3[4];        // Model matrix row 3
    float colorData[4];        // RGB color + W = animPhase
};
```

**KEY FINDING:** The `colorData[3]` field is already allocated for `animPhase`! This is the **primary hook point** for per-creature animation state.

### 4. Procedural Creature Mesh Generation (CreatureBuilder.h/cpp)

**Location:** `src/graphics/procedural/CreatureBuilder.h` (192 lines) | `CreatureBuilder.cpp` (902 lines)

#### Body Plan Archetypes

```cpp
enum class BodyPlan {
    QUADRUPED,      // 4 legs (gait animation)
    BIPED,          // 2 legs (walking/running)
    HEXAPOD,        // 6 legs (tripod/wave gaits)
    SERPENTINE,     // No legs (slithering)
    AVIAN,          // Wings (flapping)
    FISH            // Streamlined (tail undulation)
};
```

#### Limb Helper Function

```cpp
static void addLimb(
    MetaballSystem& metaballs,
    const glm::vec3& attachPoint,
    const glm::vec3& direction,
    float thickness,
    float length,
    int joints = 2                            // <- JOINTS!
);
```

Limbs have **explicit joint structure** - perfect for animation!

### 5. Morphology System (Morphology.h)

**Location:** `src/physics/Morphology.h` (310 lines)

This is the **most important file for animation** as it defines the complete skeletal structure.

#### Morphology Gene System

```cpp
struct MorphologyGenes {
    // Limb definitions
    int legPairs = 2;           // Number of leg pairs
    int legSegments = 3;        // Segments per leg (animation bones)
    float legLength = 0.8f;
    float legThickness = 0.15f;
    float legSpread = 0.7f;

    // Wing definition
    int wingPairs = 0;
    float wingSpan = 2.0f;
    float wingChord = 0.4f;

    // Tail definition
    bool hasTail = true;
    int tailSegments = 5;       // <- Multiple bones for tail IK!
    float tailLength = 0.8f;
    float tailTaper = 0.5f;

    // Joint properties
    JointType primaryJointType = JointType::HINGE;
    float jointFlexibility = 0.7f;
    float jointStrength = 0.5f;
    float jointDamping = 0.3f;
};
```

#### Joint Definition Structure

```cpp
struct JointDefinition {
    JointType type = JointType::HINGE;      // FIXED, HINGE, BALL_SOCKET, SADDLE, PIVOT
    glm::vec3 position;                      // Local position
    glm::vec3 axis = glm::vec3(1, 0, 0);    // Rotation axis
    float minAngle = -1.57f;                 // Min/max range (radians)
    float maxAngle = 1.57f;
    float stiffness = 100.0f;                // Spring constant
    float damping = 10.0f;                   // Damping
    float maxTorque = 50.0f;                 // Max torque
};
```

#### Body Segment Structure

```cpp
struct BodySegment {
    std::string name;
    glm::vec3 localPosition;
    glm::vec3 size;                         // Half-extents
    float mass;
    glm::mat3 inertia;                      // Inertia tensor

    int parentIndex = -1;                   // Skeletal hierarchy!
    JointDefinition jointToParent;          // Joint to parent bone
    std::vector<int> childIndices;          // Children

    AppendageType appendageType;            // LEG, ARM, WING, FIN, TAIL
    int segmentIndexInLimb;                 // 0=proximal, higher=distal
    bool isTerminal;                        // End of chain
};
```

**This is a complete skeletal rig structure!**

### 6. Visual State System (VisualIndicators.h)

**Location:** `src/physics/VisualIndicators.h` (283 lines)

#### Visual State Flags (Game State -> Animation)

```cpp
enum class VisualStateFlag : unsigned int {
    NONE,
    INJURED,            // -> limp/wounded animation
    STARVING,           // -> weak/trembling animation
    EXHAUSTED,          // -> panting/tired animation
    AFRAID,             // -> crouch/cower animation
    AGGRESSIVE,         // -> attack stance animation
    ALERT,              // -> heightened posture
    RELAXED,            // -> idle/resting animation
    MATING_DISPLAY,     // -> courtship animation!
    CARRYING_FOOD,      // -> modified gait
    METAMORPHOSING,     // -> transformation animation
    DOMINANT,           // -> power posture
    SUBMISSIVE          // -> subordinate posture
};
```

#### Animation Parameters

```cpp
struct VisualState {
    // === Animation ===
    float animationSpeed = 1.0f;    // Movement speed multiplier
    float breathingRate = 1.0f;     // Breathing cycles/sec
    float breathingDepth = 1.0f;    // Breathing amplitude
    float trembleIntensity = 0.0f;  // Shaking/trembling
    float swayAmount = 0.0f;        // Idle swaying

    // === Posture ===
    float postureSlump = 0.0f;      // 0=upright, 1=slumped
    float headDroop = 0.0f;         // Head down
    float tailDroop = 0.0f;         // Tail position
    float legSpread = 1.0f;         // Stance width
    float crouchFactor = 0.0f;      // 0=standing, 1=crouched
    float archBack = 0.0f;          // Defensive arch
};
```

#### Animation Parameter Generators

```cpp
namespace AnimationParams {
    struct BreathingParams {
        float rate;     // Cycles per second
        float depth;    // Amplitude
        float pattern;  // 0=regular, 1=irregular
    };

    BreathingParams getBreathingParams(VisualStateFlag flags, float exertion);
    float getTrembleIntensity(VisualStateFlag flags, float fear, float cold);
    PostureParams getPostureParams(VisualStateFlag flags, float energy, float fear);
};
```

**This is production-ready animation parameter generation!**

### 7. Locomotion System (Locomotion.h)

**Location:** `src/physics/Locomotion.h` (318 lines)

#### Central Pattern Generator (CPG)

```cpp
class CentralPatternGenerator {
    GaitType currentGait = GaitType::WALK;
    float baseFrequency = 1.0f;        // Cycles per second
    float globalPhase = 0.0f;          // Master phase (0-1)

    std::vector<float> limbPhaseOffsets;    // Phase offset per limb
    std::vector<float> dutyFactors;         // Stance duration (0-1)

public:
    float getLimbPhase(int limbIndex) const;  // Get limb animation phase
    bool isLimbInStance(int limbIndex) const; // Foot on ground?
    void setFrequency(float freq);
};
```

#### Gait Types

```cpp
enum class GaitType {
    WALK,       // Slow, stable (3+ feet on ground)
    TROT,       // Medium speed (diagonal pairs)
    GALLOP,     // Fast (suspension phases)
    CRAWL,      // Very slow, close to ground
    SLITHER,    // Serpentine/no legs
    SWIM,       // Aquatic propulsion
    FLY,        // Aerial locomotion
    HOP,        // Bipedal hopping
    TRIPOD,     // Hexapod tripod gait
    WAVE        // Multi-leg wave gait
};
```

#### Pre-defined Gait Patterns

```cpp
namespace GaitPatterns {
    std::vector<float> quadrupedWalk()   { return {0.0f, 0.5f, 0.25f, 0.75f}; }
    std::vector<float> quadrupedTrot()   { return {0.0f, 0.5f, 0.5f, 0.0f}; }
    std::vector<float> quadrupedGallop() { return {0.0f, 0.1f, 0.5f, 0.6f}; }
    std::vector<float> hexapodTripod()   { return {0.0f, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f}; }

    float getDutyFactor(GaitType gait);  // Stance phase ratio
};
```

#### IK Solver Interface (Already Defined!)

```cpp
class IKSolver {
    static bool solveLimb(
        const std::vector<BodySegment>& segments,
        const std::vector<int>& limbSegmentIndices,
        const glm::vec3& targetPosition,
        std::vector<float>& outJointAngles,
        int maxIterations = 10
    );

    static bool solveFABRIK(...);  // Foot-to-limb IK
    static bool solve2Bone(...);   // Analytical 2-segment IK
};
```

---

## IK Algorithm Research

### Algorithm Comparison Summary

| Aspect | Two-Bone IK | FABRIK | CCD |
|--------|-------------|--------|-----|
| **Type** | Analytical | Iterative (position-based) | Iterative (angle-based) |
| **Complexity** | O(1) | O(n) per iteration | O(n^2) per iteration |
| **Chain Length** | Exactly 2 bones | Any length | Any length |
| **Implementation** | Moderate | Easy | Very Easy |
| **Convergence** | Instant | Fast (2-10 iterations) | Slower (10-50 iterations) |
| **Constraints** | Limited | Moderate | Good |
| **Multiple Targets** | No | Yes | Difficult |
| **Pose Quality** | Excellent | Very Good | Can be unnatural |
| **Use Case** | Arms, Legs | Character rigs | Tentacles, spines |

### 1. Two-Bone IK (Analytical Solution)

Two-Bone IK provides an **analytical (closed-form) solution** for positioning a three-joint chain using the Law of Cosines.

#### Mathematical Foundation

```
c² = a² + b² - 2ab*cos(C)

Solving for angle:
cos(C) = (a² + b² - c²) / (2ab)
C = acos((a² + b² - c²) / (2ab))
```

For a two-bone chain:
- **A** = upper bone length (shoulder to elbow)
- **B** = lower bone length (elbow to wrist)
- **D** = distance from root to target

```cpp
float cosElbowAngle = (A*A + B*B - D*D) / (2*A*B);
float elbowAngle = acos(clamp(cosElbowAngle, -1.0f, 1.0f));

float cosShoulderAngle = (A*A + D*D - B*B) / (2*A*D);
float shoulderAngle = acos(clamp(cosShoulderAngle, -1.0f, 1.0f));
```

#### C++ Implementation

```cpp
void two_joint_ik(
    vec3 a, vec3 b, vec3 c, vec3 t, float eps,
    quat a_gr, quat b_gr,
    quat& a_lr, quat& b_lr)
{
    // Bone lengths
    float lab = length(b - a);  // Upper bone
    float lcb = length(b - c);  // Lower bone

    // Clamp target distance to valid range
    float lat = clamp(length(t - a), eps, lab + lcb - eps);

    // Current angles
    float ac_ab_0 = acos(clamp(dot(normalize(c - a), normalize(b - a)), -1.0f, 1.0f));
    float ba_bc_0 = acos(clamp(dot(normalize(a - b), normalize(c - b)), -1.0f, 1.0f));
    float ac_at_0 = acos(clamp(dot(normalize(c - a), normalize(t - a)), -1.0f, 1.0f));

    // Target angles (Law of Cosines)
    float ac_ab_1 = acos(clamp((lcb*lcb - lab*lab - lat*lat) / (-2.0f*lab*lat), -1.0f, 1.0f));
    float ba_bc_1 = acos(clamp((lat*lat - lab*lab - lcb*lcb) / (-2.0f*lab*lcb), -1.0f, 1.0f));

    // Rotation axes
    vec3 axis0 = normalize(cross(c - a, b - a));  // Bend axis
    vec3 axis1 = normalize(cross(c - a, t - a));  // Swing axis

    // Build rotation quaternions
    quat r0 = quat_angle_axis(ac_ab_1 - ac_ab_0, quat_mul(quat_inv(a_gr), axis0));
    quat r1 = quat_angle_axis(ba_bc_1 - ba_bc_0, quat_mul(quat_inv(b_gr), axis0));
    quat r2 = quat_angle_axis(ac_at_0, quat_mul(quat_inv(a_gr), axis1));

    // Compose final rotations
    a_lr = quat_mul(a_lr, quat_mul(r0, r2));
    b_lr = quat_mul(b_lr, r1);
}
```

#### Edge Case Handling

| Edge Case | Problem | Solution |
|-----------|---------|----------|
| **Target too far** | `cos(angle) > 1` causes NaN | Clamp target distance to `maxReach - epsilon` |
| **Target too close** | `cos(angle) < -1` causes NaN | Clamp target distance to `minReach + epsilon` |
| **Singularity** | Cross product undefined | Use alternative axis from pole vector |
| **Numerical instability** | Floating point errors | Clamp all `acos` inputs to `[-1, 1]` |

#### Pole Vector Control

```cpp
// Pole vector determines elbow/knee direction
vec3 toEnd = normalize(endPos - rootPos);
vec3 toMid = normalize(midPos - rootPos);

// Project pole direction onto perpendicular plane
vec3 poleDir = normalize(poleVector - rootPos);
poleDir = normalize(poleDir - dot(poleDir, toEnd) * toEnd);

// Current mid-joint direction in the plane
vec3 currentMidDir = normalize(toMid - dot(toMid, toEnd) * toEnd);

// Calculate twist rotation to align mid-joint with pole
float twistAngle = signedAngle(currentMidDir, poleDir, toEnd);
quat twistRotation = quaternionFromAxisAngle(toEnd, twistAngle);
```

### 2. FABRIK (Forward And Backward Reaching Inverse Kinematics)

FABRIK is a heuristic iterative method that works in **position space** rather than angle space.

#### Algorithm Steps

**Forward Reaching Phase** (End to Root):
1. Move end-effector to target position
2. For each joint from end toward root:
   - Calculate direction from current joint to previously adjusted joint
   - Place current joint at `bone_length` distance along that direction

**Backward Reaching Phase** (Root to End):
1. Reset root to its original fixed position
2. For each joint from root toward end:
   - Calculate direction from current joint to next joint
   - Place next joint at `bone_length` distance along that direction

#### C++ Implementation

```cpp
struct FABRIKSolver {
    std::vector<glm::vec3> joints;
    std::vector<float> boneLengths;
    float tolerance = 0.001f;
    int maxIterations = 10;

    void solve(const glm::vec3& target) {
        int n = joints.size();
        glm::vec3 rootPos = joints[0];

        // Calculate total reach
        float totalLength = 0.0f;
        for (float len : boneLengths) totalLength += len;

        float targetDist = glm::distance(rootPos, target);

        // Unreachable target - extend toward it
        if (targetDist > totalLength) {
            for (int i = 0; i < n - 1; i++) {
                glm::vec3 dir = glm::normalize(target - joints[i]);
                joints[i + 1] = joints[i] + dir * boneLengths[i];
            }
            return;
        }

        int iteration = 0;
        while (glm::distance(joints[n-1], target) > tolerance
               && iteration < maxIterations) {

            // Forward reaching
            joints[n - 1] = target;
            for (int i = n - 2; i >= 0; i--) {
                glm::vec3 dir = glm::normalize(joints[i] - joints[i + 1]);
                joints[i] = joints[i + 1] + dir * boneLengths[i];
            }

            // Backward reaching
            joints[0] = rootPos;
            for (int i = 0; i < n - 1; i++) {
                glm::vec3 dir = glm::normalize(joints[i + 1] - joints[i]);
                joints[i + 1] = joints[i] + dir * boneLengths[i];
            }

            iteration++;
        }
    }
};
```

#### Joint Angle Constraints

```cpp
void applyBallConstraint(glm::vec3& jointPos, const glm::vec3& parentPos,
                         const glm::vec3& parentForward, float maxAngle) {
    glm::vec3 dir = glm::normalize(jointPos - parentPos);
    float angle = acos(glm::clamp(glm::dot(dir, parentForward), -1.0f, 1.0f));

    if (angle > maxAngle) {
        glm::vec3 axis = glm::normalize(glm::cross(parentForward, dir));
        glm::quat rotation = glm::angleAxis(angle - maxAngle, axis);
        dir = rotation * dir;
        jointPos = parentPos + dir * boneLength;
    }
}
```

### 3. CCD (Cyclic Coordinate Descent)

CCD is an iterative numerical method that adjusts one joint at a time, rotating each to minimize end-effector distance to target.

#### C++ Implementation

```cpp
struct CCDSolver {
    struct Joint {
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 localAxis;
        float minAngle, maxAngle;
    };

    std::vector<Joint> joints;
    int maxIterations = 50;
    float tolerance = 0.001f;
    float damping = 1.0f;

    void solve(const glm::vec3& target) {
        for (int iter = 0; iter < maxIterations; iter++) {
            if (glm::distance(getEndEffector(), target) < tolerance) {
                return;
            }

            // Iterate from end toward root
            for (int i = joints.size() - 2; i >= 0; i--) {
                glm::vec3 endPos = getEndEffector();
                glm::vec3 jointPos = joints[i].position;

                glm::vec3 toEnd = glm::normalize(endPos - jointPos);
                glm::vec3 toTarget = glm::normalize(target - jointPos);

                float dotProduct = glm::clamp(glm::dot(toEnd, toTarget), -1.0f, 1.0f);
                float angle = acos(dotProduct) * damping;

                if (angle > 0.0001f) {
                    glm::vec3 axis = glm::cross(toEnd, toTarget);
                    if (glm::length(axis) > 0.0001f) {
                        axis = glm::normalize(axis);
                        glm::quat rotation = glm::angleAxis(angle, axis);
                        joints[i].rotation = rotation * joints[i].rotation;

                        updateForwardKinematics(i);
                    }
                }
            }
        }
    }
};
```

### When to Use Each Algorithm

| Scenario | Recommended | Reason |
|----------|------------|--------|
| Human/Animal limbs | **Two-Bone IK** | Exact solution, fast, natural |
| Full body rig | **FABRIK** | Multiple end-effectors, fast |
| Tentacles/Tails | **CCD** or **FABRIK** | Long chains, organic motion |
| Spiders/Insects | **Two-Bone + FABRIK** | Combine for legs + body |
| Robotic arms | **CCD** | Good constraint handling |
| Real-time games | **Two-Bone IK** | Fastest, deterministic |
| Complex creatures | **FABRIK** | Best balance of speed/quality |
| Procedural animation | **Two-Bone IK** | Legs: fast, predictable |

---

## Procedural Locomotion Research

### Gait Patterns

#### Quadruped Gaits

| Gait | Pattern | Legs Moving | Speed | Stability |
|------|---------|-------------|-------|-----------|
| **Walk (Creep)** | Sequential | 1 at a time | Slow | Very High |
| **Trot (Diagonal)** | Diagonal pairs | 2 diagonal | Medium | Good |
| **Pace** | Lateral pairs | 2 same side | Medium | Lower |
| **Canter** | Asymmetric | 2-1-1 | Fast | Medium |
| **Gallop** | Bounding | 2-2 | Fastest | Lowest |

#### Implementation

```cpp
struct QuadrupedGait {
    enum Leg { LF, RF, LH, RH };

    // Diagonal pairs move together
    // Phase 0: LF + RH stance, RF + LH swing
    // Phase 1: RF + LH stance, LF + RH swing

    float gaitPhase = 0.0f;
    float cycleTime = 0.5f;

    bool isInSwingPhase(Leg leg) {
        bool firstGroup = (leg == LF || leg == RH);
        bool firstGroupSwinging = gaitPhase < 0.5f;
        return firstGroup ? firstGroupSwinging : !firstGroupSwinging;
    }

    void update(float deltaTime, float speed) {
        float normalizedSpeed = speed / maxSpeed;
        gaitPhase += (deltaTime / cycleTime) * normalizedSpeed;
        gaitPhase = fmod(gaitPhase, 1.0f);
    }
};
```

#### Hexapod Tripod Gait

```cpp
struct HexapodGait {
    enum Leg { L1, R1, L2, R2, L3, R3 };

    // Tripod groups: {L1, R2, L3} and {R1, L2, R3}
    // Alternating tripods provide stable 3-point contact

    float gaitPhase = 0.0f;

    bool isInSwingPhase(Leg leg) {
        bool tripodA = (leg == L1 || leg == R2 || leg == L3);
        bool tripodASwinging = gaitPhase < 0.5f;
        return tripodA ? tripodASwinging : !tripodASwinging;
    }
};
```

### Foot Placement with Terrain Raycasting

```cpp
struct FootPlacementSystem {
    struct Leg {
        glm::vec3 currentFootPos;
        glm::vec3 targetFootPos;
        glm::vec3 restPosition;
        glm::vec3 groundNormal;
        float stepProgress;
        bool isMoving;
    };

    std::vector<Leg> legs;
    float stepHeight = 0.3f;
    float stepDuration = 0.2f;
    float maxFootOffset = 0.5f;

    glm::vec3 raycastGround(const glm::vec3& origin) {
        glm::vec3 rayDir(0, -1, 0);
        float rayLength = 2.0f;

        RaycastHit hit;
        if (physics->raycast(origin, rayDir, rayLength, hit)) {
            return hit.point;
        }
        return origin + rayDir * rayLength;
    }

    glm::vec3 interpolateStep(const glm::vec3& start,
                               const glm::vec3& end,
                               float t) {
        // Horizontal linear interpolation
        glm::vec3 pos = glm::mix(start, end, t);

        // Vertical arc using sine curve
        float arc = sin(t * 3.14159f) * stepHeight;
        pos.y += arc;

        return pos;
    }
};
```

### Step Triggers

```cpp
struct StepTrigger {
    float distanceThreshold = 0.5f;
    float angleThreshold = 30.0f;
    float timeThreshold = 1.0f;

    bool shouldTriggerStep(const Leg& leg, const glm::vec3& targetPos,
                           float timeSinceLastStep) {
        // Distance trigger (most common)
        float distance = glm::distance(
            glm::vec2(leg.currentFootPos.x, leg.currentFootPos.z),
            glm::vec2(targetPos.x, targetPos.z)
        );
        if (distance > distanceThreshold) return true;

        // Angle trigger (for rotation in place)
        float angleDiff = calculateAngleDifference(leg, targetPos);
        if (angleDiff > angleThreshold) return true;

        // Time trigger (prevents feet from staying planted too long)
        if (timeSinceLastStep > timeThreshold) return true;

        return false;
    }
};
```

---

## ModularGameEngine Reference

### Architecture Overview

```
AnimationSystem (Manager)
├── Skeleton (bone hierarchy)
├── AnimationClip (keyframe data)
├── AnimationPlayer (simple playback)
├── AnimationStateMachine (state-based control)
├── BlendTree (node-based blending)
├── IKSystem (inverse kinematics)
│   ├── TwoBoneIK
│   ├── FABRIKSolver
│   └── CCDSolver
└── Skinning (GPU/CPU deformation)
```

### Core Data Structures

#### Bone Transform

```cpp
struct BoneTransform {
    Math::Vec3 translation;
    Math::Quat rotation;
    Math::Vec3 scale;

    Math::Mat4 ToMatrix() const {
        return Math::Mat4::Translation(translation) *
               rotation.ToMatrix() *
               Math::Mat4::Scale(scale);
    }

    static BoneTransform Lerp(const BoneTransform& a,
                               const BoneTransform& b, f32 t);
};
```

#### Bone Definition

```cpp
struct Bone {
    std::string name;
    i32 parentIndex = -1;
    BoneTransform bindPose;
    Math::Mat4 inverseBindMatrix;
};
```

#### Skeleton Pose

```cpp
struct SkeletonPose {
    Vector<BoneTransform> localTransforms;
    Vector<Math::Mat4> globalTransforms;

    static SkeletonPose Lerp(const SkeletonPose& a,
                              const SkeletonPose& b, f32 t);
};
```

### IK System Architecture

```cpp
class TwoBoneIK {
    bool Solve(const Skeleton& skeleton,
               SkeletonPose& pose,
               const IKSolverChain& chain,
               const IKSolverTarget& target);

    bool SolveWithPole(const Skeleton& skeleton,
                       SkeletonPose& pose,
                       const IKSolverChain& chain,
                       const IKSolverTarget& target,
                       const PoleVector& pole);
};

class FABRIKSolver {
    bool Solve(const Skeleton& skeleton,
               SkeletonPose& pose,
               const IKSolverChain& chain,
               const IKSolverTarget& target);

    u32 m_maxIterations = 10;
    f32 m_tolerance = 0.001f;
};

class CCDSolver {
    bool Solve(const Skeleton& skeleton,
               SkeletonPose& pose,
               const IKSolverChain& chain,
               const IKSolverTarget& target);

    f32 m_damping = 1.0f;
};
```

### Skinning System

```cpp
struct SkinWeight {
    u32 boneIndices[4];
    f32 weights[4];

    void Normalize();
    u32 GetInfluenceCount() const;
};

enum class SkinningMethod : u8 {
    CPU = 0,
    GPU_Compute,
    GPU_VertexShader,
};

class Skinning {
    void ApplySkinning(
        const SkeletonPose& pose,
        const SkinnedMeshData& meshData,
        Span<const Math::Vec3> srcPositions,
        Span<const Math::Vec3> srcNormals,
        Span<Math::Vec3> dstPositions,
        Span<Math::Vec3> dstNormals);

    void UploadBoneMatrices(const SkeletonPose& pose,
                            const Skeleton& skeleton);
};
```

### What to Reuse vs. Build Fresh

#### REUSABLE (minimal adaptation)

| Component | Why Reusable |
|-----------|-------------|
| **Bone Transform & Skeleton** | Core data structure, generic |
| **Keyframe sampling** | Standard animation technique |
| **IK System** | Modular, solver-independent |
| **Skinning** | Standard vertex deformation |
| **Animation Blending** | Generic lerp/slerp |

#### BUILD FRESH (too specialized)

| Component | Why Fresh | Better Approach |
|-----------|----------|-----------------|
| **State Machine** | Overkill for procedural creatures | Simple animation queue |
| **Blend Trees** | UI/designer-focused | Direct parameter blending |
| **AnimationComponents** | Tied to full ECS | Lightweight creature component |

---

## Integration Strategy

### Primary Integration Points (No Core Code Changes)

#### Point 1: Instance Buffer Animation Phase

**Location:** `src/graphics/rendering/CreatureRenderer_DX12.cpp`

```cpp
void SetFromCreature(const glm::mat4& model, const glm::vec3& color, float animPhase) {
    // ... matrix setup ...
    colorData[3] = animPhase;  // <- ANIMATION DATA ALREADY HERE
}
```

**Strategy:**
1. Calculate per-creature animation phase in renderer
2. Pass to GPU via `animPhase` (already allocated!)
3. Use in HLSL shaders for skeletal deformation

#### Point 2: VisualState in Mesh Building

**Location:** `src/graphics/procedural/MorphologyBuilder.h`

```cpp
static void buildForLifeStage(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    CreatureType type,
    LifeStage stage,
    float stageProgress = 0.0f        // <- ANIMATION PARAMETER!
);
```

**Strategy:**
1. Use `VisualState` to modify metaball positions per frame
2. Apply posture deformations (crouch, slump, arch)
3. Scale breathingDepth for chest expansion

#### Point 3: Creature Update Provides State

The creature state is fully available for animation:
- `position` and `rotation` for transform
- `velocity` for foot IK target calculation
- `currentTime` for phase-based animation
- `fear`, `energy` for state-dependent animation
- Behavior type for gait selection

### Recommended Phased Implementation

#### Phase 1: Non-Visual (No Breaking Changes)
- Add `VisualState` member to Creature class
- Implement `VisualStateCalculator::update()`
- Hook into Creature update loop
- **Result:** State tracking ready, no visual changes

#### Phase 2: Shader-Based Animation (Quick Wins)
- Pass `animPhase` from renderer to GPU
- Add vertex shader time modulation
- Implement simple breathing wave (sine modulation)
- Add posture-based vertex scaling
- **Result:** Breathing, idle sway, subtle movement

#### Phase 3: Gait-Driven Animation
- Implement simple CPG phase calculation
- Map phase to preset limb positions
- Use 2-bone IK for foot placement
- Blend between walk/trot/gallop based on speed
- **Result:** Walking animation matches movement

#### Phase 4: State-Driven Posture
- Implement afraid crouch, starving slump
- Hook visual state flags to posture parameters
- Add fear-based trembling
- Implement mating display animation
- **Result:** Animations convey creature state

#### Phase 5: Advanced Features
- Wing flapping (flying creatures)
- Swimming undulation (aquatic creatures)
- Impact-based animation (jumping, landing)
- Damage/limping animation
- **Result:** Specialized animations per type

---

## Code Snippets for Integration

### Calculating Animation Phase in Renderer

```cpp
// In CreatureRendererDX12::render()
for (const auto& creature : creatures) {
    float animPhase = 0.0f;

    // Calculate gait phase based on speed and movement
    float speed = glm::length(creature->getVelocity());
    float normalizedSpeed = speed / creature->getGenome().speed;

    // Add time-based phase
    float gaitPhase = fmod(time * creature->getGenome().speed * 2.0f, 1.0f);

    // Factor in creature time for consistency
    gaitPhase = fmod(creature->getCurrentTime() * 5.0f, 1.0f);

    // Apply state modulation
    const auto& visualState = creature->getVisualState();
    if (hasFlag(visualState.stateFlags, VisualStateFlag::EXHAUSTED)) {
        gaitPhase *= 0.3f;
    }

    animPhase = gaitPhase;
    instanceData.SetFromCreature(modelMatrix, color, animPhase);
}
```

### Skeletal Deformation in HLSL

```hlsl
cbuffer BoneBuffer : register(b0) {
    float4x4 bones[64];
};

float4 DeformedPosition = float4(0, 0, 0, 0);
for (int i = 0; i < input.boneWeights.length; i++) {
    int boneIdx = input.boneIndices[i];
    float weight = input.boneWeights[i];

    float4 bonePos = mul(float4(input.position, 1.0f), bones[boneIdx]);
    DeformedPosition += bonePos * weight;
}

// Add breathing deformation
float breathingPhase = frac(time * breathingRate);
float breathingWave = sin(breathingPhase * PI) * breathingDepth;
DeformedPosition.y += breathingWave;

// Apply posture
DeformedPosition.y -= postureSlump * 0.5f;
```

---

## Sources and References

### Two-Bone IK
- [The Orange Duck - Simple Two Joint IK](https://theorangeduck.com/page/simple-two-joint)
- [Little Polygon - Procedural Animation: Inverse Kinematics](https://blog.littlepolygon.com/posts/twobone/)
- [ozz-animation Two Bone IK Sample](https://guillaumeblanc.github.io/ozz-animation/samples/two_bone_ik/)
- [Ryan Juckett - Analytic Two-Bone IK in 2D](https://www.ryanjuckett.com/analytic-two-bone-ik-in-2d/)

### FABRIK
- [FABRIK Official Page - Andreas Aristidou](https://andreasaristidou.com/FABRIK)
- [FABRIK Research Paper](https://www.andreasaristidou.com/publications/papers/FABRIK.pdf)
- [GitHub - chFleschutz/inverse-kinematics-algorithms](https://github.com/chFleschutz/inverse-kinematics-algorithms)

### CCD
- [Rodolphe Vaillant - CCD IK Tutorial](http://rodolphe-vaillant.fr/?e=114)
- [GitHub - Well-Jing/Inverse-Kinematics-CCD](https://github.com/Well-Jing/Inverse-Kinematics-CCD)

### Procedural Locomotion
- [Little Polygon - Procedural Animation: Locomotion](https://blog.littlepolygon.com/posts/loco1/)
- [Trifox - Exploring Procedural Animation](https://www.trifox-game.com/exploring-procedural-animation-in-trifox/)
- [ozz-animation - Foot IK Sample](https://guillaumeblanc.github.io/ozz-animation/samples/foot_ik/)
- [Oscar Liang - Quadruped Robot Gait Study](https://oscarliang.com/quadruped-robot-gait-study/)

---

## Conclusion

The OrganismEvolution codebase has **production-ready foundations for animation**:

1. **Skeletal structure** fully defined (`BodySegment` hierarchy)
2. **Animation parameters** genome-encoded (`flapFrequency`, `swimFrequency`)
3. **Visual state system** complete (`VisualStateCalculator`)
4. **Locomotion system** exists (`CentralPatternGenerator`, gait patterns)
5. **Rendering infrastructure** supports animation (`animPhase` field allocated)

### Most Promising Integration Path

**GPU Skinning with Procedural Animation**
- Compute bone matrices from CPG/IK per frame
- Use existing DX12 instance buffer for bone transforms
- Implement vertex skinning in HLSL
- Generate animation parameters from creature state/time

**Result:** Performant, scalable, non-invasive animation system

The architecture is **ready for implementation**. This is primarily a rendering/shader engineering task, not a fundamental redesign.

---

*Document generated from comprehensive research phase. Ready for Phase 2: Architecture Design.*
