# Creature Variety Expansion System

## Executive Summary

This document outlines the expansion of the creature system from 2 basic types (herbivore/carnivore) to include flying, aquatic, and amphibious creatures with procedural skeletal animation.

**Current State:**
- 9 creature types defined (Grazer, Browser, Frugivore, Small Predator, Omnivore, Apex Predator, Scavenger, Parasite, Cleaner)
- All creatures are land-based with walking locomotion
- Procedural mesh generation via metaballs + marching cubes
- Gait system with CPG (Central Pattern Generator)
- IK solvers (2-bone and FABRIK) already implemented

**Expansion Goals:**
- Add 4 new habitat-based creature categories
- Implement organ systems (wings, fins, gills)
- Create skeletal animation system with procedural locomotion

---

## Phase 1: Creature Type Expansion

### 1.1 New Creature Types

Expand the `CreatureType` enum in [CreatureType.h](../src/entities/CreatureType.h):

```cpp
enum class CreatureType {
    // === Existing Land Types (Trophic Levels 2-4) ===
    GRAZER,         // Eats grass only
    BROWSER,        // Eats tree leaves and bushes
    FRUGIVORE,      // Eats fruits/berries
    SMALL_PREDATOR, // Hunts small herbivores
    OMNIVORE,       // Eats plants AND small creatures
    APEX_PREDATOR,  // Hunts all herbivores and small predators
    SCAVENGER,      // Eats corpses
    PARASITE,       // Attaches to hosts
    CLEANER,        // Removes parasites

    // === NEW: Aquatic Types ===
    AQUATIC_GRAZER,     // Fish eating algae/plants (koi, tilapia analog)
    AQUATIC_PREDATOR,   // Fish hunting smaller fish (pike, bass analog)
    AQUATIC_APEX,       // Large aquatic predator (shark analog)
    FILTER_FEEDER,      // Passive filter feeding (whale shark analog)

    // === NEW: Amphibian Types ===
    AMPHIBIAN_GRAZER,   // Eats plants on land and in water (turtle analog)
    AMPHIBIAN_PREDATOR, // Hunts in both environments (crocodile analog)
    AMPHIBIAN_OMNIVORE, // Opportunistic dual-environment feeder (frog analog)

    // === NEW: Flying Types ===
    AERIAL_GRAZER,      // Flying herbivore (fruit bat analog)
    AERIAL_PREDATOR,    // Flying carnivore hunting land creatures (eagle analog)
    AERIAL_INSECTIVORE, // Flying insect-eater (swallow analog)
    AERIAL_SCAVENGER,   // Flying corpse eater (vulture analog)

    // Legacy aliases
    HERBIVORE = GRAZER,
    CARNIVORE = APEX_PREDATOR
};
```

### 1.2 New Habitat System

Add habitat classification to complement trophic levels:

```cpp
// Primary habitat where creature spends most time
enum class HabitatType {
    TERRESTRIAL,    // Land only
    AQUATIC,        // Water only (cannot survive on land)
    AMPHIBIOUS,     // Both land and water
    AERIAL,         // Air primary (must land to rest/eat)
    ARBOREAL        // Tree-dwelling (special case of terrestrial)
};

// Terrain preferences within habitat
enum class TerrainPreference {
    OPEN_LAND,      // Grasslands, plains
    FOREST,         // Dense vegetation
    SHALLOW_WATER,  // Lakes, ponds, rivers
    DEEP_WATER,     // Ocean depths
    COASTAL,        // Beach/shoreline
    MOUNTAIN,       // High altitude
    CAVE            // Underground
};
```

### 1.3 Expanded CreatureTraits

Extend the `CreatureTraits` struct:

```cpp
struct CreatureTraits {
    // ... existing fields ...

    // === NEW: Habitat Parameters ===
    HabitatType primaryHabitat;
    HabitatType secondaryHabitat;     // NONE if single-habitat
    TerrainPreference terrainPref;

    // === NEW: Movement Capabilities ===
    bool canSwim;                      // Can traverse water
    bool canFly;                       // Can achieve flight
    bool canDive;                      // Can go underwater
    float waterSpeed;                  // Speed multiplier in water
    float airSpeed;                    // Speed multiplier in air
    float diveDepth;                   // Maximum dive depth (meters)
    float flightAltitude;              // Preferred flight altitude
    float flightEndurance;             // Time can stay airborne (seconds)

    // === NEW: Environmental Tolerance ===
    float oxygenEfficiency;            // 0-1, affects how long can hold breath
    float temperatureRange[2];         // Min/max comfortable temp
    float pressureTolerance;           // For deep diving

    // === NEW: Predation Modifiers ===
    float divingAttackBonus;           // Bonus damage from dive attacks
    float ambushFromWaterBonus;        // Crocodile-style attacks
    float aerialSpottingRange;         // How far can see prey from air
};
```

---

## Phase 2: Organ System Design

### 2.1 Organ System Architecture

Create a modular organ system in `src/entities/organs/`:

```
src/entities/organs/
├── OrganSystem.h           // Base class and interfaces
├── RespiratorySystem.h     // Lungs, gills, tracheal
├── LocomotionOrgans.h      // Wings, fins, legs
├── SensoryOrgans.h         // Eyes, lateral line, echolocation
└── OrganDependencies.h     // Inter-organ relationships
```

#### OrganSystem Base Interface

```cpp
// src/entities/organs/OrganSystem.h
#pragma once

enum class OrganType {
    // Respiratory
    LUNGS,
    GILLS,
    TRACHEAL,           // Insect breathing
    SKIN_RESPIRATION,   // Amphibian supplementary

    // Locomotion
    WINGS_FEATHERED,
    WINGS_MEMBRANE,     // Bat-style
    WINGS_INSECT,
    FINS_PECTORAL,
    FINS_DORSAL,
    FINS_CAUDAL,
    FINS_PELVIC,
    LEGS_PLANTIGRADE,   // Flat-footed (bear, human)
    LEGS_DIGITIGRADE,   // Toe-walking (dog, cat)
    LEGS_UNGULIGRADE,   // Hoof-walking (horse, deer)
    SWIM_BLADDER,

    // Sensory
    LATERAL_LINE,       // Water pressure sensing
    ECHOLOCATION,
    ELECTRORECEPTION,   // Shark-style
    INFRARED_PIT,       // Snake heat sensing

    // Structural
    SHELL,
    EXOSKELETON,
    ENDOSKELETON
};

struct OrganStats {
    float efficiency;       // 0-1 how well organ performs
    float energyCost;       // Energy per second to maintain
    float mass;             // Contribution to body mass
    float development;      // 0-1 growth stage
    float health;           // 0-1 condition
};

class OrganSystem {
public:
    virtual ~OrganSystem() = default;

    virtual OrganType getType() const = 0;
    virtual OrganStats getStats() const = 0;
    virtual float getEnergyConsumption() const = 0;
    virtual void update(float deltaTime) = 0;

    // Genetic expression level affects performance
    virtual void setExpressionLevel(float level) = 0;
    virtual float getExpressionLevel() const = 0;

    // Damage and regeneration
    virtual void takeDamage(float amount) = 0;
    virtual void regenerate(float amount) = 0;

    // Environmental interaction
    virtual bool isActiveIn(HabitatType habitat) const = 0;
};
```

### 2.2 Wing System Design

```cpp
// src/entities/organs/WingSystem.h
#pragma once

#include "OrganSystem.h"
#include <glm/glm.hpp>

enum class WingType {
    FEATHERED,      // Bird-style (lift + thrust)
    MEMBRANE,       // Bat-style (membrane between fingers)
    INSECT,         // Rapid flapping, figure-8 motion
    GLIDING         // Fixed wing, no powered flight
};

struct WingBone {
    int boneIndex;
    glm::vec3 restPosition;
    glm::vec3 restRotation;
    float length;
    int parentBone;
};

struct WingParams {
    WingType type;
    float span;                 // Tip-to-tip distance
    float chord;                // Wing width (leading to trailing edge)
    float aspectRatio;          // span^2 / area (higher = more efficient glide)
    float camber;               // Curvature (0-0.2)
    float dihedral;             // V-angle of wings (stability)
    float sweepAngle;           // Backward angle (high-speed)
    int featherCount;           // For feathered wings
    float membraneThickness;    // For membrane wings

    // Computed aerodynamics
    float liftCoefficient;
    float dragCoefficient;
    float stallAngle;           // Angle of attack where lift drops
};

class WingSystem : public OrganSystem {
public:
    WingSystem(WingType type, float span, float chord);

    // OrganSystem interface
    OrganType getType() const override;
    OrganStats getStats() const override;
    float getEnergyConsumption() const override;
    void update(float deltaTime) override;
    void setExpressionLevel(float level) override;
    float getExpressionLevel() const override;
    void takeDamage(float amount) override;
    void regenerate(float amount) override;
    bool isActiveIn(HabitatType habitat) const override;

    // Wing-specific methods
    const WingParams& getParams() const { return params; }

    // Animation state
    float getFlapPhase() const { return flapPhase; }
    float getFlapFrequency() const { return flapFrequency; }
    void setFlapFrequency(float freq) { flapFrequency = freq; }

    // Aerodynamics
    glm::vec3 calculateLift(const glm::vec3& velocity, float airDensity) const;
    glm::vec3 calculateDrag(const glm::vec3& velocity, float airDensity) const;
    float getAngleOfAttack(const glm::vec3& velocity) const;
    bool isStalling(const glm::vec3& velocity) const;

    // Wing folding for ground movement
    void setFolded(bool folded) { isFolded = folded; }
    bool getFolded() const { return isFolded; }

    // Bone structure for animation
    const std::vector<WingBone>& getBones() const { return bones; }
    void updateBoneTransforms(float deltaTime);

private:
    WingParams params;
    std::vector<WingBone> bones;

    float flapPhase = 0.0f;         // 0-1 cycle position
    float flapFrequency = 3.0f;     // Flaps per second
    float flapAmplitude = 0.8f;     // Radians
    bool isFolded = false;

    float expressionLevel = 1.0f;
    OrganStats stats;

    void buildBoneStructure();
    void calculateAerodynamics();
};
```

### 2.3 Fin System Design

```cpp
// src/entities/organs/FinSystem.h
#pragma once

#include "OrganSystem.h"

enum class FinType {
    PECTORAL,       // Side fins (steering, lift)
    DORSAL,         // Back fin (stability)
    CAUDAL,         // Tail fin (propulsion)
    PELVIC,         // Bottom fins (pitch control)
    ANAL,           // Rear bottom fin (stability)
    ADIPOSE         // Small fatty fin (sensing?)
};

enum class CaudalShape {
    LUNATE,         // Crescent moon (fast swimmers - tuna)
    FORKED,         // V-shaped (fast, maneuverable)
    TRUNCATE,       // Squared off (slow, precise)
    ROUNDED,        // Rounded (slow, high thrust)
    HETEROCERCAL,   // Asymmetric (sharks)
    DIPHYCERCAL     // Symmetric, pointed (primitive fish)
};

struct FinParams {
    FinType type;
    float area;                 // Surface area
    float aspectRatio;          // Span^2 / area
    float flexibility;          // How much fin can bend (0-1)
    CaudalShape caudalShape;    // Only for CAUDAL type

    // Position on body
    glm::vec3 attachPoint;      // Local position
    glm::vec3 orientation;      // Normal direction

    // Computed hydrodynamics
    float thrustCoefficient;    // For caudal fins
    float liftCoefficient;      // For pectoral fins
    float dragCoefficient;
};

class FinSystem : public OrganSystem {
public:
    FinSystem(FinType type, float area, float aspectRatio);

    // OrganSystem interface
    OrganType getType() const override;
    OrganStats getStats() const override;
    float getEnergyConsumption() const override;
    void update(float deltaTime) override;
    void setExpressionLevel(float level) override;
    float getExpressionLevel() const override;
    void takeDamage(float amount) override;
    void regenerate(float amount) override;
    bool isActiveIn(HabitatType habitat) const override;

    // Fin-specific methods
    const FinParams& getParams() const { return params; }

    // Animation
    float getOscillationPhase() const { return oscillationPhase; }
    float getOscillationFrequency() const { return oscillationFreq; }
    float getDeflectionAngle() const { return deflectionAngle; }

    // Hydrodynamics
    glm::vec3 calculateThrust(float oscillationSpeed, float waterDensity) const;
    glm::vec3 calculateLift(const glm::vec3& velocity, float waterDensity) const;
    glm::vec3 calculateDrag(const glm::vec3& velocity, float waterDensity) const;

    // Control surfaces
    void setDeflection(float angle) { deflectionAngle = angle; }

private:
    FinParams params;

    float oscillationPhase = 0.0f;
    float oscillationFreq = 2.0f;       // Oscillations per second
    float oscillationAmplitude = 0.5f;  // Radians
    float deflectionAngle = 0.0f;       // Control surface angle

    float expressionLevel = 1.0f;
    OrganStats stats;

    void calculateHydrodynamics();
};
```

### 2.4 Respiratory System (Gills)

```cpp
// src/entities/organs/RespiratorySystem.h
#pragma once

#include "OrganSystem.h"

enum class RespiratoryType {
    LUNGS_MAMMAL,       // Air sacs, diaphragm breathing
    LUNGS_AVIAN,        // Flow-through with air sacs (most efficient)
    GILLS_FISH,         // Counter-current water flow
    GILLS_EXTERNAL,     // Exposed gills (salamander larvae)
    TRACHEAL,           // Insect-style tubes
    SKIN_DIFFUSION,     // Cutaneous respiration (amphibians)
    BOOK_LUNGS          // Arachnid breathing
};

struct GillParams {
    int archCount;              // Number of gill arches (3-7)
    float surfaceArea;          // Total gas exchange area
    float filamentDensity;      // Filaments per arch
    float bloodFlowRate;        // ml/min
    float waterFlowRate;        // ml/min (counter-current)
    float extractionEfficiency; // 0-1, how much O2 extracted
};

class RespiratorySystem : public OrganSystem {
public:
    RespiratorySystem(RespiratoryType type);

    // OrganSystem interface
    OrganType getType() const override;
    OrganStats getStats() const override;
    float getEnergyConsumption() const override;
    void update(float deltaTime) override;
    void setExpressionLevel(float level) override;
    float getExpressionLevel() const override;
    void takeDamage(float amount) override;
    void regenerate(float amount) override;
    bool isActiveIn(HabitatType habitat) const override;

    // Respiration-specific
    float getOxygenLevel() const { return oxygenLevel; }
    float getCO2Level() const { return co2Level; }
    float getBreathingRate() const { return breathingRate; }

    // Environment interaction
    void breathe(float environmentO2, float deltaTime);
    bool canBreatheIn(HabitatType habitat) const;
    float getBreathHoldDuration() const;  // For diving

    // Visual indicators
    bool isGillsVisible() const;
    float getGillMovementPhase() const { return gillMovementPhase; }

private:
    RespiratoryType type;
    GillParams gillParams;      // Only used for gill types

    float oxygenLevel = 1.0f;   // 0-1 blood oxygen
    float co2Level = 0.0f;      // 0-1 blood CO2
    float breathingRate = 12.0f; // Breaths per minute
    float breathHoldMax = 30.0f; // Seconds
    float breathHoldCurrent = 0.0f;

    float gillMovementPhase = 0.0f;
    float expressionLevel = 1.0f;
    OrganStats stats;

    void setupGillParams();
    float calculateExtractionRate(float environmentO2) const;
};
```

---

## Phase 3: Skeletal Animation System

### 3.1 Architecture Overview

```
src/graphics/animation/
├── Skeleton.h              // Bone hierarchy definition
├── Bone.h                  // Individual bone data
├── AnimationClip.h         // Keyframe animation storage
├── AnimationBlender.h      // Blending between clips
├── ProceduralAnimation.h   // Runtime procedural motion
├── IKChain.h               // IK solver wrapper
└── LocomotionAnimator.h    // Movement animation controller
```

### 3.2 Bone Hierarchy System

```cpp
// src/graphics/animation/Bone.h
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

struct BoneTransform {
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    glm::mat4 toMatrix() const;
    static BoneTransform lerp(const BoneTransform& a, const BoneTransform& b, float t);
    static BoneTransform slerp(const BoneTransform& a, const BoneTransform& b, float t);
};

class Bone {
public:
    std::string name;
    int index;
    int parentIndex;            // -1 for root
    std::vector<int> children;

    // Rest pose (bind pose)
    BoneTransform bindPose;
    glm::mat4 inverseBindMatrix;

    // Current animated pose
    BoneTransform localTransform;
    glm::mat4 worldTransform;

    // Constraints
    glm::vec3 minAngles;        // Euler angle limits
    glm::vec3 maxAngles;

    // Physical properties
    float length;
    float mass;

    void updateWorldTransform(const glm::mat4& parentWorld);
    void applyConstraints();
};
```

### 3.3 Skeleton Definition

```cpp
// src/graphics/animation/Skeleton.h
#pragma once

#include "Bone.h"
#include <vector>
#include <unordered_map>

// Standard bone names for different body plans
namespace BoneNames {
    // Core
    constexpr const char* ROOT = "root";
    constexpr const char* SPINE_BASE = "spine_base";
    constexpr const char* SPINE_MID = "spine_mid";
    constexpr const char* SPINE_TOP = "spine_top";
    constexpr const char* NECK = "neck";
    constexpr const char* HEAD = "head";

    // Legs (with L/R prefix)
    constexpr const char* HIP = "hip";
    constexpr const char* THIGH = "thigh";
    constexpr const char* SHIN = "shin";
    constexpr const char* FOOT = "foot";
    constexpr const char* TOE = "toe";

    // Arms/Wings (with L/R prefix)
    constexpr const char* SHOULDER = "shoulder";
    constexpr const char* UPPER_ARM = "upper_arm";
    constexpr const char* FOREARM = "forearm";
    constexpr const char* HAND = "hand";

    // Wing specific
    constexpr const char* WING_HUMERUS = "wing_humerus";
    constexpr const char* WING_RADIUS = "wing_radius";
    constexpr const char* WING_METACARPAL = "wing_metacarpal";
    constexpr const char* WING_FINGER = "wing_finger";

    // Tail
    constexpr const char* TAIL_BASE = "tail_base";
    constexpr const char* TAIL_MID = "tail_mid";
    constexpr const char* TAIL_TIP = "tail_tip";

    // Fins
    constexpr const char* FIN_DORSAL = "fin_dorsal";
    constexpr const char* FIN_PECTORAL = "fin_pectoral";
    constexpr const char* FIN_CAUDAL = "fin_caudal";
}

class Skeleton {
public:
    Skeleton() = default;

    // Build from morphology
    void buildFromMorphology(const MorphologyGenes& genes);

    // Build for specific creature types
    void buildQuadruped(float scale);
    void buildBiped(float scale);
    void buildFish(float scale);
    void buildBird(float scale);
    void buildSerpentine(float scale);

    // Bone access
    int getBoneCount() const { return static_cast<int>(bones.size()); }
    Bone& getBone(int index) { return bones[index]; }
    const Bone& getBone(int index) const { return bones[index]; }
    Bone* getBoneByName(const std::string& name);
    int getBoneIndex(const std::string& name) const;

    // Hierarchy
    const Bone& getRootBone() const { return bones[0]; }
    std::vector<int> getChildren(int boneIndex) const;

    // Transform updates
    void updateHierarchy();             // Recalculate all world transforms
    void resetToBindPose();

    // IK chain helpers
    std::vector<int> getLegChain(bool isLeft, bool isFront) const;
    std::vector<int> getArmChain(bool isLeft) const;
    std::vector<int> getWingChain(bool isLeft) const;
    std::vector<int> getSpineChain() const;
    std::vector<int> getTailChain() const;

    // For GPU skinning
    std::vector<glm::mat4> getBoneMatrices() const;

private:
    std::vector<Bone> bones;
    std::unordered_map<std::string, int> boneNameIndex;

    int addBone(const std::string& name, int parent, const BoneTransform& bindPose);
    void buildSpine(int segmentCount, float totalLength);
    void buildLeg(int parentBone, bool isLeft, bool isFront, float length);
    void buildArm(int parentBone, bool isLeft, float length);
    void buildWing(int parentBone, bool isLeft, float span);
    void buildTail(int parentBone, int segmentCount, float length);
    void buildFins(bool dorsal, bool pectoral, bool caudal, float size);
};
```

### 3.4 IK Solver Integration

```cpp
// src/graphics/animation/IKChain.h
#pragma once

#include "Skeleton.h"
#include <vector>

enum class IKSolverType {
    TWO_BONE,       // Analytical 2-segment (legs, arms)
    FABRIK,         // Forward And Backward Reaching IK (spines, tails)
    CCD,            // Cyclic Coordinate Descent (general purpose)
    JACOBIAN        // Jacobian transpose (smooth but slower)
};

struct IKTarget {
    glm::vec3 position;
    glm::quat rotation;         // Optional end effector orientation
    bool useRotation = false;
    float weight = 1.0f;        // For blending
};

class IKChain {
public:
    IKChain(Skeleton* skeleton, const std::vector<int>& boneIndices, IKSolverType solver);

    // Set target
    void setTarget(const IKTarget& target);
    void setPoleVector(const glm::vec3& pole);  // For 2-bone to control elbow/knee direction

    // Solve
    bool solve(int maxIterations = 10, float tolerance = 0.001f);

    // Get results
    bool isSolved() const { return solved; }
    float getError() const { return currentError; }

    // Chain configuration
    void setStiffness(int boneIndex, float stiffness);  // Per-bone resistance to bending
    void enableConstraints(bool enable) { useConstraints = enable; }

private:
    Skeleton* skeleton;
    std::vector<int> boneIndices;
    IKSolverType solverType;

    IKTarget target;
    glm::vec3 poleVector;

    bool solved = false;
    float currentError = 0.0f;
    bool useConstraints = true;

    // Solver implementations
    bool solveTwoBone();
    bool solveFABRIK(int maxIter, float tolerance);
    bool solveCCD(int maxIter, float tolerance);
    bool solveJacobian(int maxIter, float tolerance);

    void applyBoneConstraints(int boneIndex);
    glm::vec3 getEndEffectorPosition() const;
    float computeError() const;
};
```

### 3.5 Procedural Locomotion System

```cpp
// src/graphics/animation/ProceduralAnimation.h
#pragma once

#include "Skeleton.h"
#include "IKChain.h"
#include "../../physics/Locomotion.h"

// Foot placement configuration
struct FootPlacement {
    int legIndex;
    IKChain* ikChain;

    glm::vec3 currentPosition;
    glm::vec3 targetPosition;
    glm::vec3 liftPosition;         // Peak of step arc

    float phase;                    // 0-1 in gait cycle
    float stepHeight;
    float stepLength;
    bool isPlanted;                 // Foot on ground

    // Terrain adaptation
    float groundHeight;
    glm::vec3 groundNormal;
};

// Wing animation state
struct WingAnimation {
    IKChain* leftWingIK;
    IKChain* rightWingIK;

    float flapPhase;
    float flapFrequency;
    float flapAmplitude;

    // Wing morphing between states
    float glideFactor;              // 0 = flapping, 1 = gliding
    float foldFactor;               // 0 = extended, 1 = folded

    // Per-bone angles during flap cycle
    struct WingPose {
        float shoulderAngle;
        float elbowAngle;
        float wristAngle;
        float fingerSpread;
    };
    WingPose currentPose;
};

// Fin animation state
struct FinAnimation {
    float oscillationPhase;
    float oscillationFreq;
    float tailSweepAmplitude;

    // Body undulation for fish
    std::vector<float> spineAngles;
    float undulationWavelength;
    float undulationAmplitude;
};

// Spine/tail procedural animation
struct SpineAnimation {
    std::vector<int> spineBones;
    IKChain* spineIK;

    float curvature;                // Current bend amount
    glm::vec3 lookTarget;           // For head tracking
    float headTrackWeight;          // How much head follows target

    // Secondary motion
    float sway;
    float bob;
    float twist;
};

class ProceduralAnimator {
public:
    ProceduralAnimator(Skeleton* skeleton, const MorphologyGenes& genes);

    // Main update
    void update(float deltaTime, const glm::vec3& velocity, const glm::vec3& facing);

    // Locomotion modes
    void setLocomotionMode(GaitType gait);
    void transitionToFlight();
    void transitionToSwimming();
    void transitionToWalking();

    // Ground interaction
    void setGroundHeight(float height);
    void setTerrainNormalAt(const glm::vec3& pos, const glm::vec3& normal);

    // External input
    void setLookTarget(const glm::vec3& target);
    void setFlapSpeed(float speed);
    void setSwimSpeed(float speed);

    // Get state for physics
    const std::vector<FootPlacement>& getFootPlacements() const { return feet; }
    bool isAirborne() const { return airborne; }
    bool isSubmerged() const { return submerged; }

private:
    Skeleton* skeleton;
    MorphologyGenes genes;

    // Locomotion state
    GaitType currentGait = GaitType::WALK;
    CentralPatternGenerator cpg;
    float locomotionSpeed = 0.0f;
    bool airborne = false;
    bool submerged = false;

    // Animation subsystems
    std::vector<FootPlacement> feet;
    WingAnimation wings;
    FinAnimation fins;
    SpineAnimation spine;

    // IK chains
    std::vector<std::unique_ptr<IKChain>> ikChains;

    // Internal methods
    void updateCPG(float deltaTime);
    void updateFootPlacements(float deltaTime, const glm::vec3& velocity);
    void updateWingAnimation(float deltaTime, const glm::vec3& velocity);
    void updateFinAnimation(float deltaTime, const glm::vec3& velocity);
    void updateSpineAnimation(float deltaTime, const glm::vec3& velocity);
    void updateSecondaryMotion(float deltaTime);

    void solveAllIK();

    // Procedural motion helpers
    glm::vec3 calculateFootTarget(int footIndex, float phase) const;
    float calculateStepHeight(float phase) const;
    WingAnimation::WingPose calculateWingPose(float phase, float glideFactor) const;
    std::vector<float> calculateFishUndulation(float phase) const;
};
```

### 3.6 Body Plan Specific Skeletons

#### Quadruped (Land Creatures)
```
                        head
                         |
                        neck
                         |
    [L_front_leg]---spine_top---[R_front_leg]
                         |
                     spine_mid
                         |
    [L_rear_leg]---spine_base---[R_rear_leg]
                         |
                      tail_1
                         |
                      tail_2
                         |
                      tail_3
```

#### Avian (Flying Creatures)
```
                        head
                         |
                        neck
                         |
    [L_wing]--------spine_top--------[R_wing]
                         |
                     spine_mid
                         |
    [L_leg]---------spine_base---------[R_leg]
                         |
                      tail_fan
```

#### Fish (Aquatic Creatures)
```
    [dorsal_fin]
         |
    head--spine_1--spine_2--spine_3--spine_4--caudal_fin
         |         |         |         |
    [L_pectoral]   |    [pelvic]       |
    [R_pectoral]                  [anal_fin]
```

#### Serpentine (Snakes, Eels)
```
    head--spine_1--spine_2--spine_3--spine_4--...--spine_N--tail
                    \__________undulation wave____________/
```

---

## Phase 4: Implementation Roadmap

### Phase 4.1: Foundation (Core Systems)

**Goal:** Establish the base infrastructure for multi-habitat creatures.

**Tasks:**
1. Extend `CreatureType` enum with new types
2. Add `HabitatType` and `TerrainPreference` enums
3. Extend `CreatureTraits` with habitat parameters
4. Create base `OrganSystem` interface

**Integration Points:**
- [CreatureType.h:15-36](../src/entities/CreatureType.h#L15-L36) - Add new enum values
- [CreatureType.h:62-107](../src/entities/CreatureType.h#L62-L107) - Extend CreatureTraits

**Testing:**
- Unit tests for trait lookup
- Verify backward compatibility with existing creatures

---

### Phase 4.2: Organ Systems

**Goal:** Implement wing, fin, and gill systems.

**Tasks:**
1. Create `src/entities/organs/` directory structure
2. Implement `WingSystem` with aerodynamics
3. Implement `FinSystem` with hydrodynamics
4. Implement `RespiratorySystem` with gas exchange
5. Create `OrganManager` to coordinate multiple organs per creature

**Integration Points:**
- [Morphology.h:84-104](../src/physics/Morphology.h#L84-L104) - Existing wing/fin gene parameters
- [CreatureBuilder.h:104-116](../src/graphics/procedural/CreatureBuilder.h#L104-L116) - Existing addWings/addFins methods

**Dependencies:**
- Phase 4.1 must be complete

---

### Phase 4.3: Skeletal Animation

**Goal:** Replace static meshes with animated skeletons.

**Tasks:**
1. Create `Skeleton` and `Bone` classes
2. Implement skeleton builders for each body plan
3. Integrate existing IK solvers ([Locomotion.h:94-125](../src/physics/Locomotion.h#L94-L125))
4. Create `ProceduralAnimator` with foot placement
5. Implement GPU skinning shader

**Integration Points:**
- [Locomotion.h:52-87](../src/physics/Locomotion.h#L52-L87) - Existing CPG system
- [Locomotion.h:94-125](../src/physics/Locomotion.h#L94-L125) - Existing IKSolver
- [MarchingCubes](../src/graphics/procedural/) - Mesh generation

**Key Decisions:**
- CPU vs GPU skinning (recommend GPU for performance)
- Blending between procedural and baked animations

---

### Phase 4.4: Locomotion Controllers

**Goal:** Implement movement for each habitat type.

**Tasks:**
1. Extend `LocomotionController` for swimming
2. Implement flight controller with lift/drag physics
3. Create amphibian controller with mode switching
4. Update `GaitType` enum for new locomotion styles

**Integration Points:**
- [Locomotion.h:132-204](../src/physics/Locomotion.h#L132-L204) - Existing LocomotionController
- [Locomotion.h:273-317](../src/physics/Locomotion.h#L273-L317) - Gait patterns

**New Gait Patterns Needed:**
```cpp
GaitType::SWIM_CRUISE,      // Steady swimming
GaitType::SWIM_BURST,       // Escape/attack dash
GaitType::FLY_FLAP,         // Powered flight
GaitType::FLY_GLIDE,        // Soaring
GaitType::FLY_HOVER,        // Stationary flight
GaitType::AMPHIBIAN_WALK,   // Walking on land with fins
GaitType::AMPHIBIAN_SWIM    // Swimming with legs
```

---

### Phase 4.5: Environment Integration

**Goal:** Make creatures interact properly with water, air, and terrain.

**Tasks:**
1. Add water volume detection in terrain system
2. Implement buoyancy physics
3. Add air current system for flight
4. Create habitat transition zones (beach, surface)
5. Update rendering for underwater/aerial views

**Integration Points:**
- [Terrain.cpp](../src/environment/Terrain.cpp) - Height and water level
- [Simulation.cpp](../src/core/Simulation.cpp) - Main update loop

---

### Phase 4.6: AI and Behavior

**Goal:** Make creatures behave appropriately for their habitat.

**Tasks:**
1. Extend `SteeringBehaviors` for 3D movement
2. Add schooling behavior for fish
3. Add flocking behavior for birds
4. Implement hunting behaviors for aerial/aquatic predators
5. Update `SensorySystem` for underwater sensing

**Integration Points:**
- [SteeringBehaviors.h](../src/entities/SteeringBehaviors.h)
- [SensorySystem.h](../src/entities/SensorySystem.h)
- [EcosystemBehaviors.h](../src/entities/EcosystemBehaviors.h)

---

### Phase 4.7: Genetics Integration

**Goal:** Make new creature types evolvable.

**Tasks:**
1. Add organ genes to `DiploidGenome`
2. Create mutation operators for wing/fin/gill genes
3. Implement selection pressure for habitat adaptation
4. Add speciation triggers for habitat isolation

**Integration Points:**
- [genetics/DiploidGenome.h](../src/entities/genetics/DiploidGenome.h)
- [Genome.h](../src/entities/Genome.h) - Legacy genome

**New Gene Types:**
```cpp
GeneType::WING_SPAN,
GeneType::WING_ASPECT_RATIO,
GeneType::FIN_COUNT,
GeneType::FIN_SIZE,
GeneType::GILL_EFFICIENCY,
GeneType::LUNG_CAPACITY,
GeneType::SWIM_SPEED,
GeneType::FLIGHT_ENDURANCE,
GeneType::HABITAT_PREFERENCE,
GeneType::DEPTH_TOLERANCE
```

---

## Appendix A: Creature Type Specifications

### A.1 Aquatic Creatures

| Type | Diet | Speed | Depth Range | Special |
|------|------|-------|-------------|---------|
| AQUATIC_GRAZER | Algae, plants | Slow | 0-20m | Schooling |
| AQUATIC_PREDATOR | Small fish | Medium | 0-50m | Ambush |
| AQUATIC_APEX | All fish | Fast | 0-100m | Lone hunter |
| FILTER_FEEDER | Plankton | Very slow | 0-30m | Passive |

### A.2 Amphibian Creatures

| Type | Diet | Land Speed | Water Speed | Transition Time |
|------|------|------------|-------------|-----------------|
| AMPHIBIAN_GRAZER | Plants | Slow | Medium | 5s |
| AMPHIBIAN_PREDATOR | Prey | Medium | Fast | 3s |
| AMPHIBIAN_OMNIVORE | All | Medium | Medium | 4s |

### A.3 Flying Creatures

| Type | Diet | Flight Speed | Endurance | Attack Style |
|------|------|--------------|-----------|--------------|
| AERIAL_GRAZER | Fruit | Slow | High | None |
| AERIAL_PREDATOR | Land prey | Fast | Medium | Dive |
| AERIAL_INSECTIVORE | Insects | Very fast | Low | Aerial catch |
| AERIAL_SCAVENGER | Carrion | Medium | Very high | Circling |

---

## Appendix B: Organ Energy Costs

| Organ | Base Cost (energy/s) | Scaling Factor |
|-------|---------------------|----------------|
| Lungs (mammal) | 0.5 | × bodyMass^0.75 |
| Lungs (avian) | 0.4 | × bodyMass^0.75 |
| Gills | 0.3 | × waterFlowRate |
| Wings (passive) | 0.1 | × wingArea |
| Wings (flapping) | 2.0 | × flapFrequency × wingArea |
| Fins (passive) | 0.05 | × finArea |
| Fins (active) | 0.5 | × oscillationFreq × finArea |

---

## Appendix C: Animation Blending Weights

| Transition | Source | Target | Blend Time |
|------------|--------|--------|------------|
| Walk → Trot | Walk gait | Trot gait | 0.3s |
| Walk → Swim | Walk gait | Swim cycle | 1.0s |
| Swim → Walk | Swim cycle | Walk gait | 1.5s |
| Walk → Fly | Walk gait | Takeoff | 0.5s |
| Fly (flap) → Fly (glide) | Flap cycle | Glide pose | 0.8s |
| Fly → Land | Flight pose | Walk gait | 1.2s |

---

## Appendix D: File Structure After Implementation

```
src/
├── entities/
│   ├── CreatureType.h          # Extended with new types
│   ├── Creature.h/.cpp         # Updated with organ systems
│   ├── Genome.h/.cpp           # Extended with new genes
│   └── organs/                 # NEW DIRECTORY
│       ├── OrganSystem.h
│       ├── WingSystem.h/.cpp
│       ├── FinSystem.h/.cpp
│       ├── RespiratorySystem.h/.cpp
│       └── OrganManager.h/.cpp
│
├── graphics/
│   ├── animation/              # NEW DIRECTORY
│   │   ├── Skeleton.h/.cpp
│   │   ├── Bone.h/.cpp
│   │   ├── IKChain.h/.cpp
│   │   └── ProceduralAnimator.h/.cpp
│   ├── procedural/
│   │   └── CreatureBuilder.h/.cpp  # Extended for new body plans
│   └── rendering/
│       └── SkeletalRenderer.h/.cpp # NEW: GPU skinning
│
├── physics/
│   ├── Locomotion.h/.cpp       # Extended for swim/fly
│   ├── Morphology.h/.cpp       # Extended with organ params
│   ├── FlightPhysics.h/.cpp    # NEW
│   └── SwimPhysics.h/.cpp      # NEW
│
└── shaders/
    └── skeletal_vertex.glsl    # NEW: GPU skinning shader
```

---

## Success Criteria Checklist

- [ ] All 4 new creature type categories defined with traits
- [ ] Organ systems (wings, fins, gills) fully specified
- [ ] Skeletal animation architecture documented
- [ ] IK solver selection per limb type defined
- [ ] Procedural locomotion for walk/swim/fly specified
- [ ] Integration points with existing code identified
- [ ] Implementation phases clearly ordered with dependencies
- [ ] Energy costs balanced across organ types
- [ ] Backward compatibility with existing creatures maintained
