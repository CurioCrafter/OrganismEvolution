# OrganismEvolution Development Roadmap

**Version**: 2.0
**Created**: January 2026
**Last Updated**: January 2026

This document outlines the comprehensive development roadmap for the next phases of OrganismEvolution, building upon the established foundation of genetic algorithms, neural networks, and ecosystem simulation.

---

## Table of Contents

1. [Phase 3: Creature Variety](#phase-3-creature-variety)
2. [Phase 4: Skeletal Animation System](#phase-4-skeletal-animation-system)
3. [Phase 5: Advanced Ecosystem Simulation](#phase-5-advanced-ecosystem-simulation)
4. [Phase 6: Advanced Rendering](#phase-6-advanced-rendering)
5. [Phase 7: AI and Behavior Evolution](#phase-7-ai-and-behavior-evolution)
6. [Implementation Priority Matrix](#implementation-priority-matrix)
7. [Technical Dependencies](#technical-dependencies)

---

## Phase 3: Creature Variety

### Overview
Expand creature diversity beyond terrestrial quadrupeds to include flying, aquatic, and amphibious life forms with unique body plans, locomotion systems, and specialized organs.

---

### 3.1 Flying Creatures (Aerial System)

#### Description
Implement winged creatures capable of true 3D flight with realistic aerodynamics, thermal riding, and aerial hunting/evasion behaviors.

#### Required Code Changes

**New Classes:**
```
src/entities/locomotion/
├── FlightController.h/cpp       # Wing physics, lift/drag calculations
├── WingMorphology.h/cpp         # Wing shape, span, feather types
└── AerialBehaviors.h/cpp        # Soaring, diving, hovering patterns
```

**Modifications to Existing Files:**
| File | Changes Required |
|------|------------------|
| `Creature.h/cpp` | Add `locomotionMode` enum (TERRESTRIAL, AERIAL, AQUATIC, AMPHIBIOUS) |
| `Genome.h/cpp` | Add wing genes: `wingSpan`, `wingShape`, `featherDensity`, `flightMuscle` |
| `DiploidGenome.h/cpp` | Add Chromosome 5 for flight-related genes |
| `SteeringBehaviors.h/cpp` | Add 3D steering with altitude control |
| `CreatureBuilder.h/cpp` | Add wing metaball generation |
| `MarchingCubes.h/cpp` | Support thin membrane surfaces for wings |

**New Files Needed:**
| File | Purpose |
|------|---------|
| `src/entities/locomotion/FlightController.h` | Core flight physics engine |
| `src/entities/locomotion/FlightController.cpp` | Implementation with lift equation, stall mechanics |
| `src/entities/locomotion/WingMorphology.h` | Wing shape definitions and parameters |
| `src/entities/locomotion/WingMorphology.cpp` | Wing procedural generation |
| `src/entities/locomotion/AerialBehaviors.h` | Flight behavior patterns |
| `src/entities/locomotion/AerialBehaviors.cpp` | Thermal detection, formation flying |
| `src/physics/Aerodynamics.h` | Lift/drag coefficient calculations |
| `src/physics/Aerodynamics.cpp` | Reynolds number, wing loading physics |
| `shaders/wing_vertex.glsl` | Wing deformation shader |
| `shaders/wing_fragment.glsl` | Feather/membrane rendering |

**Engine Module Dependencies:**
- Physics system (new aerodynamics module)
- Terrain system (thermal generation based on terrain type)
- Weather system (wind affects flight)
- Neural network (3D navigation inputs/outputs)

**Key Algorithms:**
```cpp
// Simplified lift equation
float calculateLift(float velocity, float wingArea, float liftCoefficient, float airDensity) {
    return 0.5f * airDensity * velocity * velocity * wingArea * liftCoefficient;
}

// Stall detection
bool isStalling(float angleOfAttack, float criticalAngle) {
    return angleOfAttack > criticalAngle;
}
```

**Estimated Complexity:** HIGH
- Flight physics requires careful tuning for stability
- Wing animation needs procedural bone system
- AI must learn 3D navigation (altitude, thermals)

---

### 3.2 Aquatic Creatures (Marine System)

#### Description
Water-dwelling creatures with fins, gills, and buoyancy control. Support for different water depths and pressure adaptations.

#### Required Code Changes

**New Classes:**
```
src/entities/locomotion/
├── SwimController.h/cpp         # Buoyancy, fin propulsion
├── FinMorphology.h/cpp          # Fin types, tail shapes
└── AquaticBehaviors.h/cpp       # Schooling, depth seeking
```

**Modifications to Existing Files:**
| File | Changes Required |
|------|------------------|
| `Creature.h/cpp` | Add `currentMedium` (AIR, WATER, SURFACE), depth tracking |
| `Genome.h/cpp` | Add aquatic genes: `finSize`, `gillEfficiency`, `swimBladder`, `pressure resistance` |
| `Terrain.h/cpp` | Add water depth map, currents, temperature layers |
| `SensorySystem.h/cpp` | Add lateral line organ, electroreception, pressure sensing |
| `Food.h/cpp` | Add aquatic food sources (plankton, algae, fish) |

**New Files Needed:**
| File | Purpose |
|------|---------|
| `src/entities/locomotion/SwimController.h` | Swimming physics controller |
| `src/entities/locomotion/SwimController.cpp` | Buoyancy, drag, fin thrust |
| `src/entities/locomotion/FinMorphology.h` | Fin shape definitions |
| `src/entities/locomotion/FinMorphology.cpp` | Procedural fin generation |
| `src/entities/locomotion/AquaticBehaviors.h` | Water-specific behaviors |
| `src/entities/locomotion/AquaticBehaviors.cpp` | Schooling, depth regulation |
| `src/environment/WaterSystem.h` | Ocean/lake simulation |
| `src/environment/WaterSystem.cpp` | Currents, temperature, oxygen levels |
| `src/physics/Hydrodynamics.h` | Water physics calculations |
| `src/physics/Hydrodynamics.cpp` | Drag coefficients, buoyancy |
| `shaders/underwater_fragment.glsl` | Underwater rendering effects |
| `shaders/caustics_fragment.glsl` | Light caustics on underwater surfaces |

**Engine Module Dependencies:**
- Terrain system (water body definition)
- Rendering system (underwater camera, caustics)
- Food system (aquatic food spawning)
- Ecosystem (aquatic trophic levels)

**Estimated Complexity:** HIGH
- Fluid dynamics simulation
- Underwater rendering pipeline
- New food web for aquatic ecosystem

---

### 3.3 Amphibians (Transitional System)

#### Description
Creatures capable of transitioning between land and water with metamorphosis support (tadpole to frog lifecycle).

#### Required Code Changes

**New Classes:**
```
src/entities/lifecycle/
├── MetamorphosisController.h/cpp  # Life stage transitions
├── TransitionalBehaviors.h/cpp    # Land/water switching
└── RespiratorySystem.h/cpp        # Gills vs lungs switching
```

**Modifications to Existing Files:**
| File | Changes Required |
|------|------------------|
| `Creature.h/cpp` | Add `lifeStage` enum, `morphologyState`, transition progress |
| `Metamorphosis.h/cpp` | Expand to support full amphibian lifecycle |
| `Genome.h/cpp` | Add metamorphosis genes: `transitionAge`, `aquaticJuvenile`, `lungCapacity` |
| `CreatureBuilder.h/cpp` | Support morphology interpolation between stages |

**New Files Needed:**
| File | Purpose |
|------|---------|
| `src/entities/lifecycle/MetamorphosisController.h` | Stage transition controller |
| `src/entities/lifecycle/MetamorphosisController.cpp` | Gradual body transformation |
| `src/entities/lifecycle/TransitionalBehaviors.h` | Medium-switching AI |
| `src/entities/lifecycle/TransitionalBehaviors.cpp` | Shore-seeking, water entry logic |
| `src/entities/lifecycle/RespiratorySystem.h` | Oxygen acquisition system |
| `src/entities/lifecycle/RespiratorySystem.cpp` | Gill/lung efficiency |

**Engine Module Dependencies:**
- Locomotion system (must support both modes)
- Terrain system (shoreline detection)
- Morphology system (smooth transitions)
- Physics (both hydro and aerodynamics)

**Estimated Complexity:** VERY HIGH
- Most complex locomotion variant
- Requires interpolation between body plans
- Dual physics systems active simultaneously near water

---

### 3.4 New Body Plans and Organs

#### Description
Expand the body plan system beyond current options to include radial symmetry, colonial organisms, and specialized organs.

#### New Body Plans

| Body Plan | Description | Locomotion |
|-----------|-------------|------------|
| `RADIAL` | Jellyfish-like radial symmetry | Jet propulsion |
| `COLONIAL` | Multi-unit organisms (siphonophores) | Coordinated movement |
| `WORM` | Limbless elongated body | Peristaltic movement |
| `CEPHALOPOD` | Tentacled body plan | Tentacle crawling + jet |
| `INSECTOID` | Six-legged with exoskeleton | Hexapod walking |
| `ARACHNID` | Eight-legged | Octopod walking |

#### New Organ Systems

| Organ | Function | Gene Control |
|-------|----------|--------------|
| `Electric Organ` | Electroreception/shocking | `electricOrganSize`, `voltage` |
| `Bioluminescence` | Light production | `luminescenceIntensity`, `color` |
| `Venom Gland` | Toxin production | `venomPotency`, `deliveryMethod` |
| `Ink Sac` | Defensive ink cloud | `inkCapacity`, `rechargeRate` |
| `Camouflage Cells` | Active color change | `chromatophoreCount`, `changeSpeed` |
| `Echolocation` | Sound-based mapping | `echoFrequency`, `echoSensitivity` |
| `Infrared Pit` | Heat detection | `pitSensitivity`, `pitAccuracy` |
| `Magnetic Sense` | Magnetic field detection | `magnetiteConcentration` |

**New Files Needed:**
| File | Purpose |
|------|---------|
| `src/entities/organs/ElectricOrgan.h/cpp` | Electric discharge system |
| `src/entities/organs/BioluminescenceOrgan.h/cpp` | Light emission |
| `src/entities/organs/VenomSystem.h/cpp` | Toxin mechanics |
| `src/entities/organs/InkSac.h/cpp` | Ink cloud defense |
| `src/entities/organs/CamouflageSystem.h/cpp` | Active camouflage |
| `src/entities/organs/OrganManager.h/cpp` | Organ registry and updates |

**Estimated Complexity:** MEDIUM-HIGH
- Each organ requires dedicated simulation
- Visual effects for bioluminescence, ink, camouflage
- Combat system expansion for venom, electric

---

## Phase 4: Skeletal Animation System

### Overview
Replace static metaball geometry with a proper skeletal animation system supporting procedural locomotion, inverse kinematics, and GPU-accelerated skinning.

---

### 4.1 Bone Hierarchy Structure

#### Description
Implement a flexible bone system that can represent any creature body plan with proper parent-child relationships.

#### Core Architecture

```cpp
struct Bone {
    std::string name;
    int parentIndex;           // -1 for root
    glm::mat4 localBindPose;   // Local space transform
    glm::mat4 inverseBindPose; // For skinning
    glm::vec3 localPosition;
    glm::quat localRotation;
    glm::vec3 localScale;

    // Animation state
    glm::mat4 currentLocalTransform;
    glm::mat4 currentWorldTransform;
};

class Skeleton {
public:
    std::vector<Bone> bones;
    std::vector<int> boneHierarchy;  // Sorted parent-first

    void calculateWorldTransforms();
    void updateFromAnimation(const AnimationPose& pose);
    glm::mat4 getBoneMatrix(int boneIndex) const;

    // Procedural generation
    static Skeleton generateForBodyPlan(BodyPlan plan, const Genome& genome);
};
```

#### Standard Bone Templates

**Quadruped Skeleton (24 bones):**
```
Root
├── Spine0
│   ├── Spine1
│   │   ├── Spine2 (Chest)
│   │   │   ├── Neck
│   │   │   │   └── Head
│   │   │   │       ├── Jaw
│   │   │   │       ├── LeftEye
│   │   │   │       └── RightEye
│   │   │   ├── LeftShoulder
│   │   │   │   └── LeftFrontLeg
│   │   │   │       └── LeftFrontFoot
│   │   │   └── RightShoulder
│   │   │       └── RightFrontLeg
│   │   │           └── RightFrontFoot
│   │   ├── LeftHip
│   │   │   └── LeftBackLeg
│   │   │       └── LeftBackFoot
│   │   └── RightHip
│   │       └── RightBackLeg
│   │           └── RightBackFoot
│   └── Tail0
│       └── Tail1
│           └── Tail2
```

**New Files Needed:**
| File | Purpose |
|------|---------|
| `src/animation/Skeleton.h` | Bone hierarchy data structure |
| `src/animation/Skeleton.cpp` | Transform calculations |
| `src/animation/Bone.h` | Individual bone structure |
| `src/animation/SkeletonFactory.h` | Procedural skeleton generation |
| `src/animation/SkeletonFactory.cpp` | Body plan to skeleton mapping |
| `src/animation/AnimationPose.h` | Pose snapshot structure |

**Engine Module Dependencies:**
- Morphology system (skeleton scales with body size)
- Genome system (bone proportions from genes)
- Rendering system (bone matrices to GPU)

**Estimated Complexity:** MEDIUM
- Well-understood hierarchical structure
- Main challenge is procedural generation variety

---

### 4.2 Inverse Kinematics (IK) Solvers

#### Description
Implement IK solvers for foot placement, head tracking, and reaching behaviors.

#### IK Solver Types

**Two-Bone IK (Limbs):**
```cpp
class TwoBoneIKSolver {
public:
    void solve(
        const glm::vec3& rootPos,      // Shoulder/Hip
        const glm::vec3& midPos,       // Elbow/Knee
        const glm::vec3& endPos,       // Hand/Foot
        const glm::vec3& targetPos,    // Where to reach
        const glm::vec3& poleVector,   // Elbow/Knee direction hint
        float upperLength,
        float lowerLength,
        glm::quat& outUpperRot,
        glm::quat& outLowerRot
    );
};
```

**FABRIK (Multi-Joint Chains):**
```cpp
class FABRIKSolver {
public:
    struct Chain {
        std::vector<glm::vec3> jointPositions;
        std::vector<float> boneLengths;
    };

    void solve(
        Chain& chain,
        const glm::vec3& target,
        int maxIterations = 10,
        float tolerance = 0.001f
    );

private:
    void forwardReach(Chain& chain, const glm::vec3& target);
    void backwardReach(Chain& chain, const glm::vec3& base);
};
```

**CCD (Cyclic Coordinate Descent):**
```cpp
class CCDSolver {
public:
    void solve(
        std::vector<Bone*>& chain,
        const glm::vec3& target,
        int maxIterations = 20,
        float angleLimitPerIteration = 0.1f  // radians
    );
};
```

**New Files Needed:**
| File | Purpose |
|------|---------|
| `src/animation/ik/TwoBoneIK.h/cpp` | Simple limb IK |
| `src/animation/ik/FABRIKSolver.h/cpp` | Multi-joint IK |
| `src/animation/ik/CCDSolver.h/cpp` | Cyclic descent IK |
| `src/animation/ik/IKConstraints.h/cpp` | Joint angle limits |
| `src/animation/ik/IKRig.h/cpp` | Full-body IK setup |

**Estimated Complexity:** MEDIUM-HIGH
- Math-heavy but well-documented algorithms
- Constraint handling adds complexity
- Multiple solver types for different use cases

---

### 4.3 Procedural Locomotion

#### Description
Generate walking, running, swimming, and flying animations procedurally based on creature morphology and physics.

#### Locomotion Controllers

**Gait Controller:**
```cpp
class GaitController {
public:
    enum GaitType {
        WALK,       // 4-beat, always 3+ feet on ground
        TROT,       // 2-beat diagonal pairs
        CANTER,     // 3-beat asymmetric
        GALLOP,     // 4-beat suspended phase
        PACE,       // 2-beat lateral pairs
        BOUND,      // 2-beat front/back pairs
    };

    struct LegTiming {
        float phase;           // 0-1 in gait cycle
        float stanceRatio;     // Time on ground vs air
        float strideLength;
        float strideHeight;
    };

    void update(float deltaTime, float speed);
    glm::vec3 getFootTarget(int legIndex) const;

private:
    std::vector<LegTiming> legTimings;
    float cycleTime;
    float currentPhase;
};
```

**Foot Placement System:**
```cpp
class FootPlacementSystem {
public:
    void update(
        Skeleton& skeleton,
        const Terrain& terrain,
        const glm::vec3& velocity,
        float deltaTime
    );

private:
    struct FootState {
        bool planted;
        glm::vec3 plantedPosition;
        glm::vec3 targetPosition;
        float liftProgress;  // 0 = planted, 1 = at apex
    };

    std::vector<FootState> footStates;

    glm::vec3 predictFootTarget(int footIndex, const glm::vec3& velocity);
    glm::vec3 raycastGround(const glm::vec3& origin);
    void interpolateFoot(FootState& foot, float t);
};
```

**New Files Needed:**
| File | Purpose |
|------|---------|
| `src/animation/procedural/GaitController.h/cpp` | Gait pattern management |
| `src/animation/procedural/FootPlacement.h/cpp` | Ground adaptation |
| `src/animation/procedural/SpineController.h/cpp` | Spine undulation |
| `src/animation/procedural/HeadController.h/cpp` | Look-at behavior |
| `src/animation/procedural/TailController.h/cpp` | Tail physics |
| `src/animation/procedural/WingController.h/cpp` | Flight animation |
| `src/animation/procedural/SwimController.h/cpp` | Swimming animation |
| `src/animation/procedural/ProceduralAnimator.h/cpp` | Master coordinator |

**Engine Module Dependencies:**
- IK system (foot/head targeting)
- Physics system (velocity for gait selection)
- Terrain system (ground height queries)
- Skeleton system (bone manipulation)

**Estimated Complexity:** HIGH
- Requires good understanding of animal locomotion
- Many edge cases (slopes, obstacles, transitions)
- Different gaits need individual tuning

---

### 4.4 GPU Skinning

#### Description
Hardware-accelerated mesh deformation using bone matrices.

#### Skinning Pipeline

**Vertex Data Extension:**
```cpp
struct SkinnedVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::ivec4 boneIndices;  // Up to 4 bone influences
    glm::vec4 boneWeights;   // Normalized weights
};
```

**Skinning Shader:**
```glsl
// vertex shader
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in ivec4 aBoneIndices;
layout(location = 4) in vec4 aBoneWeights;

uniform mat4 uBoneMatrices[MAX_BONES];  // 64-128 typically
uniform mat4 uModelViewProjection;

out vec3 vNormal;
out vec2 vUV;

void main() {
    // Compute skinned position
    mat4 skinMatrix =
        uBoneMatrices[aBoneIndices.x] * aBoneWeights.x +
        uBoneMatrices[aBoneIndices.y] * aBoneWeights.y +
        uBoneMatrices[aBoneIndices.z] * aBoneWeights.z +
        uBoneMatrices[aBoneIndices.w] * aBoneWeights.w;

    vec4 skinnedPosition = skinMatrix * vec4(aPosition, 1.0);
    vec3 skinnedNormal = mat3(skinMatrix) * aNormal;

    gl_Position = uModelViewProjection * skinnedPosition;
    vNormal = normalize(skinnedNormal);
    vUV = aUV;
}
```

**Dual Quaternion Skinning (Advanced):**
```glsl
// Higher quality skinning without volume loss
uniform vec4 uBoneDualQuats[MAX_BONES * 2];  // Real + Dual parts

vec4 dualQuatBlend(ivec4 indices, vec4 weights) {
    // Blend dual quaternions with proper normalization
    // Handles twisting better than linear blend
}
```

**New Files Needed:**
| File | Purpose |
|------|---------|
| `src/graphics/skinning/SkinnedMesh.h/cpp` | Skinned mesh data |
| `src/graphics/skinning/SkinningSystem.h/cpp` | Bone matrix upload |
| `src/graphics/skinning/BoneWeightPainter.h/cpp` | Procedural weight generation |
| `shaders/skinned_vertex.glsl` | GPU skinning shader |
| `shaders/skinned_fragment.glsl` | Skinned mesh rendering |

**Performance Considerations:**
- Bone matrix upload per-frame
- Instancing with per-instance bone matrices (SSBO)
- LOD system for distant creatures (reduce bones)

**Estimated Complexity:** MEDIUM
- GPU skinning is well-established technique
- Main challenge is procedural weight generation
- Instancing support adds complexity

---

## Phase 5: Advanced Ecosystem Simulation

### Overview
Expand the ecosystem to include complex food webs, dynamic environmental conditions, and emergent population dynamics.

---

### 5.1 Food Web Dynamics

#### Description
Implement a multi-trophic level food web with energy flow tracking and population regulation.

#### Trophic Levels

```cpp
enum TrophicLevel {
    PRODUCER,           // Plants, algae
    PRIMARY_CONSUMER,   // Herbivores
    SECONDARY_CONSUMER, // Carnivores eating herbivores
    TERTIARY_CONSUMER,  // Apex predators
    DECOMPOSER,         // Bacteria, fungi
    DETRITIVORE         // Corpse/waste eaters
};
```

#### Energy Flow Model

```cpp
class FoodWeb {
public:
    struct TrophicLink {
        int predatorSpecies;
        int preySpecies;
        float preferenceWeight;     // How much predator prefers this prey
        float conversionEfficiency; // Energy transferred (typically 10%)
        float catchSuccessRate;
    };

    void update(float deltaTime);
    void trackPredation(int predator, int prey, float energyTransferred);

    // Analysis
    float calculateTrophicLevel(int speciesId);
    std::vector<int> getKeystone();  // Species whose removal causes cascade
    float getEcosystemStability();

private:
    std::vector<TrophicLink> links;
    std::map<int, float> speciesBiomass;

    void lotkaVolterraStep(float dt);  // Population dynamics
};
```

#### Carrying Capacity System

```cpp
class CarryingCapacity {
public:
    float calculateForSpecies(
        int speciesId,
        const Terrain& terrain,
        const FoodWeb& foodWeb,
        float currentPopulation
    );

private:
    // Limiting factors
    float calculateFoodAvailability(int speciesId);
    float calculateSpaceAvailability(int speciesId);
    float calculateWaterAvailability(int speciesId);
    float calculatePredationPressure(int speciesId);
};
```

**New Files Needed:**
| File | Purpose |
|------|---------|
| `src/environment/FoodWeb.h/cpp` | Food web structure |
| `src/environment/TrophicDynamics.h/cpp` | Energy flow calculations |
| `src/environment/CarryingCapacity.h/cpp` | Population limits |
| `src/environment/PopulationDynamics.h/cpp` | Lotka-Volterra integration |
| `src/environment/NutrientCycling.h/cpp` | Nitrogen/carbon cycles |

**Estimated Complexity:** HIGH
- Requires ecological modeling expertise
- Population explosions/crashes need tuning
- Balancing is an ongoing process

---

### 5.2 Seasonal Changes

#### Description
Expand the existing SeasonManager to affect all aspects of the simulation.

#### Season Effects

```cpp
struct SeasonalEffects {
    // Environmental
    float dayLength;           // Hours of daylight
    float temperature;         // Celsius
    float precipitation;       // mm/day
    float snowCover;           // 0-1

    // Biological
    float plantGrowthRate;     // Multiplier
    float metabolicRate;       // Multiplier for energy consumption
    float reproductionChance;  // Breeding season effect
    float migrationUrge;       // Triggers migration behavior

    // Visual
    glm::vec3 sunColor;
    float sunIntensity;
    glm::vec3 fogColor;
    float vegetationHue;       // Leaf color shift
};

class SeasonManager {
public:
    void update(float deltaTime);
    SeasonalEffects getCurrentEffects() const;

    // Trigger events
    void notifySpringArrival();   // Breeding, plant growth
    void notifySummerPeak();      // Maximum activity
    void notifyAutumnChange();    // Food storage, migration start
    void notifyWinterOnset();     // Hibernation, resource scarcity

private:
    float yearProgress;  // 0-1
    Season currentSeason;
    float transitionProgress;  // Between seasons
};
```

**Modifications to Existing Files:**
| File | Changes Required |
|------|------------------|
| `SeasonManager.h/cpp` | Expand effects, add transitions |
| `Creature.h/cpp` | Respond to seasonal cues |
| `VegetationManager.h/cpp` | Seasonal plant changes |
| `Terrain.h/cpp` | Snow/ice rendering |

**Estimated Complexity:** MEDIUM
- Builds on existing framework
- Main work is connecting all systems

---

### 5.3 Weather Effects

#### Description
Dynamic weather system affecting creature behavior, visibility, and survival.

#### Weather Types

```cpp
enum WeatherType {
    CLEAR,
    CLOUDY,
    OVERCAST,
    LIGHT_RAIN,
    HEAVY_RAIN,
    THUNDERSTORM,
    FOG,
    LIGHT_SNOW,
    HEAVY_SNOW,
    BLIZZARD,
    HEATWAVE,
    SANDSTORM
};

class WeatherSystem {
public:
    struct WeatherState {
        WeatherType type;
        float intensity;        // 0-1
        glm::vec3 windDirection;
        float windSpeed;
        float visibility;       // meters
        float wetness;          // Ground wetness level
        float temperature;
    };

    void update(float deltaTime);
    WeatherState getCurrentWeather() const;
    WeatherState getForecast(float hoursAhead) const;

private:
    void simulateAtmosphere();
    void generatePrecipitation();
    void updateWindPatterns();

    // Weather fronts and pressure systems
    std::vector<PressureSystem> pressureSystems;
};
```

**New Files Needed:**
| File | Purpose |
|------|---------|
| `src/environment/WeatherSystem.h/cpp` | Core weather simulation |
| `src/environment/AtmosphereModel.h/cpp` | Pressure/humidity simulation |
| `src/environment/PrecipitationSystem.h/cpp` | Rain/snow particles |
| `src/graphics/effects/RainRenderer.h/cpp` | Rain rendering |
| `src/graphics/effects/SnowRenderer.h/cpp` | Snow rendering |
| `src/graphics/effects/LightningEffect.h/cpp` | Lightning bolts |
| `shaders/rain_particle.glsl` | Rain particle shader |
| `shaders/snow_particle.glsl` | Snow particle shader |

**Engine Module Dependencies:**
- Particle system (precipitation)
- Lighting system (clouds, lightning)
- Audio system (thunder, rain sounds)
- Physics (wind affecting creatures)

**Estimated Complexity:** HIGH
- Weather simulation is complex
- Visual effects require particle systems
- Performance impact from particles

---

### 5.4 Day/Night Cycle

#### Description
Full 24-hour cycle affecting creature behavior, visibility, and ecosystem dynamics.

#### Day/Night System

```cpp
class DayNightCycle {
public:
    void update(float deltaTime);

    // Time queries
    float getTimeOfDay() const;        // 0-24 hours
    float getSunAltitude() const;      // Degrees above horizon
    float getSunAzimuth() const;       // Compass direction
    glm::vec3 getSunDirection() const;

    // Light calculations
    float getAmbientLight() const;     // 0-1
    glm::vec3 getSunColor() const;
    glm::vec3 getSkyColor() const;
    float getMoonPhase() const;        // 0-1
    float getMoonlight() const;

    // Event notifications
    bool isDawn() const;
    bool isDay() const;
    bool isDusk() const;
    bool isNight() const;

private:
    float dayProgress;      // 0-1
    float dayLengthSeconds; // Real seconds per game day
    int currentDay;
    float moonCycle;        // 29.5 day lunar cycle
};
```

**Creature Behavior Integration:**
```cpp
// In SensorySystem
float getEffectiveVision() const {
    float baseVision = visionRange;
    float lightLevel = dayNightCycle->getAmbientLight();
    float nightVisionBonus = genome.getNightVision();
    return baseVision * (lightLevel + nightVisionBonus * (1.0f - lightLevel));
}

// Behavioral adaptations
enum ActivityPattern {
    DIURNAL,      // Active during day
    NOCTURNAL,    // Active at night
    CREPUSCULAR,  // Active at dawn/dusk
    CATHEMERAL    // Active any time
};
```

**New Files Needed:**
| File | Purpose |
|------|---------|
| `src/environment/DayNightCycle.h/cpp` | Time and lighting |
| `src/environment/CelestialBody.h/cpp` | Sun/moon positions |
| `src/graphics/lighting/SkyRenderer.h/cpp` | Sky gradient rendering |
| `src/graphics/lighting/StarField.h/cpp` | Night sky stars |
| `shaders/sky_fragment.glsl` | Procedural sky |
| `shaders/atmosphere_fragment.glsl` | Atmospheric scattering |

**Estimated Complexity:** MEDIUM
- Core cycle is simple
- Sky rendering requires atmospheric scattering
- Behavior integration across all creatures

---

## Phase 6: Advanced Rendering

### Overview
Elevate visual quality with modern rendering techniques.

---

### 6.1 Shadow Mapping

#### Description
Real-time shadows from the sun and moon using cascaded shadow maps.

#### Implementation Architecture

**Cascaded Shadow Maps (CSM):**
```cpp
class ShadowMapper {
public:
    static constexpr int NUM_CASCADES = 4;

    struct Cascade {
        glm::mat4 viewProjection;
        float splitDistance;
        GLuint depthTexture;
        GLuint framebuffer;
        int resolution;  // 1024-4096
    };

    void setup(int baseResolution = 2048);
    void update(const Camera& camera, const glm::vec3& lightDir);
    void beginShadowPass(int cascadeIndex);
    void endShadowPass();

    // For main rendering pass
    void bindShadowMaps(Shader& shader);

private:
    std::array<Cascade, NUM_CASCADES> cascades;

    void calculateCascadeSplits(float nearPlane, float farPlane);
    glm::mat4 calculateLightMatrix(int cascadeIndex, const glm::vec3& lightDir);
};
```

**Shadow Shader:**
```glsl
// Fragment shader addition
uniform sampler2DArray uShadowMaps;
uniform mat4 uLightSpaceMatrices[4];
uniform float uCascadeSplits[4];

float calculateShadow(vec3 worldPos, vec3 normal) {
    // Determine cascade
    float depth = gl_FragCoord.z;
    int cascade = 3;
    for (int i = 0; i < 4; i++) {
        if (depth < uCascadeSplits[i]) {
            cascade = i;
            break;
        }
    }

    // Project to shadow map
    vec4 lightSpacePos = uLightSpaceMatrices[cascade] * vec4(worldPos, 1.0);
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    // PCF sampling
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(uShadowMaps, 0).xy;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float pcfDepth = texture(uShadowMaps,
                vec3(projCoords.xy + vec2(x,y) * texelSize, cascade)).r;
            shadow += projCoords.z - 0.005 > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}
```

**New Files Needed:**
| File | Purpose |
|------|---------|
| `src/graphics/shadows/ShadowMapper.h/cpp` | Shadow map management |
| `src/graphics/shadows/CascadeCalculator.h/cpp` | Cascade splitting |
| `shaders/shadow_depth_vertex.glsl` | Shadow pass vertex |
| `shaders/shadow_depth_fragment.glsl` | Shadow pass fragment |

**Estimated Complexity:** MEDIUM-HIGH
- Well-documented technique
- Cascade seams require careful handling
- Performance tuning needed

---

### 6.2 Screen-Space Reflections (SSR)

#### Description
Dynamic reflections on water and wet surfaces using screen-space ray marching.

**SSR Implementation:**
```glsl
// Screen-space ray march
vec3 calculateSSR(vec3 viewPos, vec3 normal, float roughness) {
    vec3 viewDir = normalize(viewPos);
    vec3 reflectDir = reflect(viewDir, normal);

    // Ray march in screen space
    vec3 rayStart = viewPos;
    vec3 rayEnd = viewPos + reflectDir * MAX_DISTANCE;

    // Project to screen
    vec2 startScreen = projectToScreen(rayStart);
    vec2 endScreen = projectToScreen(rayEnd);

    // March along ray
    vec2 rayDir = normalize(endScreen - startScreen);
    float stepSize = 1.0 / float(MAX_STEPS);

    for (int i = 0; i < MAX_STEPS; i++) {
        vec2 samplePos = startScreen + rayDir * float(i) * stepSize;
        float sampleDepth = texture(uDepthBuffer, samplePos).r;
        float rayDepth = linearizeDepth(/* current ray depth */);

        if (rayDepth > sampleDepth) {
            // Hit - return color at this position
            return texture(uColorBuffer, samplePos).rgb;
        }
    }
    return vec3(0.0);  // No hit
}
```

**New Files Needed:**
| File | Purpose |
|------|---------|
| `src/graphics/effects/SSREffect.h/cpp` | SSR post-process |
| `shaders/ssr_raymarch.glsl` | Ray marching shader |
| `shaders/ssr_resolve.glsl` | SSR blending |

**Estimated Complexity:** HIGH
- Ray marching is computationally expensive
- Requires deferred rendering pipeline
- Many edge cases (behind camera, off-screen)

---

### 6.3 Atmospheric Scattering

#### Description
Physically-based sky rendering with Rayleigh and Mie scattering.

**Scattering Model:**
```glsl
// Rayleigh scattering (blue sky)
vec3 rayleighCoefficient = vec3(5.8e-6, 13.5e-6, 33.1e-6);

// Mie scattering (haze, sun glow)
float mieCoefficient = 21e-6;

vec3 calculateAtmosphere(vec3 rayDir, vec3 sunDir) {
    // Ray-sphere intersection with atmosphere
    // Numerical integration along ray
    // Sum Rayleigh and Mie contributions

    vec3 totalRayleigh = vec3(0.0);
    vec3 totalMie = vec3(0.0);

    for (int i = 0; i < NUM_SAMPLES; i++) {
        // Sample point along ray
        vec3 samplePos = /* ... */;
        float height = length(samplePos) - EARTH_RADIUS;

        // Density at this height
        float rayleighDensity = exp(-height / RAYLEIGH_SCALE_HEIGHT);
        float mieDensity = exp(-height / MIE_SCALE_HEIGHT);

        // Optical depth to sun
        float opticalDepthSun = /* integrate to sun */;

        // Accumulate scattering
        totalRayleigh += /* ... */;
        totalMie += /* ... */;
    }

    return totalRayleigh + totalMie;
}
```

**New Files Needed:**
| File | Purpose |
|------|---------|
| `src/graphics/atmosphere/AtmosphericScattering.h/cpp` | Scattering calculation |
| `src/graphics/atmosphere/SkyDome.h/cpp` | Sky mesh rendering |
| `shaders/atmosphere_vertex.glsl` | Sky vertex shader |
| `shaders/atmosphere_fragment.glsl` | Scattering fragment shader |

**Estimated Complexity:** MEDIUM-HIGH
- Physics-based but well-documented
- Can precompute lookup tables
- Integration with day/night cycle

---

### 6.4 Volumetric Effects

#### Description
Volumetric lighting (god rays), fog, and clouds.

**Volumetric Light Scattering:**
```glsl
// Ray marching for volumetric lighting
vec3 calculateGodRays(vec3 worldPos, vec3 sunPos) {
    vec3 rayDir = normalize(worldPos - cameraPos);
    float rayLength = length(worldPos - cameraPos);

    vec3 accumLight = vec3(0.0);
    float stepSize = rayLength / float(NUM_STEPS);

    for (int i = 0; i < NUM_STEPS; i++) {
        vec3 samplePos = cameraPos + rayDir * stepSize * float(i);

        // Check if in shadow
        float shadow = sampleShadowMap(samplePos);

        // Accumulate light with phase function
        float phase = henyeyGreenstein(dot(rayDir, sunDir), 0.7);
        accumLight += sunColor * (1.0 - shadow) * phase * density;
    }

    return accumLight;
}
```

**Volumetric Clouds:**
```cpp
class VolumetricClouds {
public:
    void generate(const NoiseGenerator& noise);
    void render(const Camera& camera, const DayNightCycle& dayNight);

private:
    GLuint cloudNoiseTexture3D;
    GLuint weatherMapTexture;

    // Cloud layer parameters
    float cloudBaseHeight;
    float cloudTopHeight;
    float cloudCoverage;
    float cloudDensity;
};
```

**New Files Needed:**
| File | Purpose |
|------|---------|
| `src/graphics/volumetric/VolumetricLighting.h/cpp` | God rays |
| `src/graphics/volumetric/VolumetricClouds.h/cpp` | Cloud system |
| `src/graphics/volumetric/VolumetricFog.h/cpp` | Height fog |
| `shaders/volumetric_raymarch.glsl` | Volume ray marching |
| `shaders/cloud_fragment.glsl` | Cloud rendering |

**Estimated Complexity:** VERY HIGH
- Performance-intensive
- Requires optimization (temporal reprojection)
- Complex noise-based cloud modeling

---

## Phase 7: AI and Behavior Evolution

### Overview
Advance the neural network and behavior systems to produce emergent intelligence.

---

### 7.1 Neural Network Evolution

#### Description
Enhance the existing NEAT implementation with modern improvements.

**HyperNEAT Extension:**
```cpp
// Compositional Pattern Producing Networks
class CPPN {
public:
    // CPPN outputs weight between two nodes based on their positions
    float queryWeight(
        float x1, float y1,  // Source node position
        float x2, float y2,  // Target node position
        float distance
    );

    // Activation functions
    enum Activation {
        SINE, COSINE, GAUSSIAN, LINEAR, SIGMOID, STEP, SAWTOOTH
    };
};

class HyperNEAT {
public:
    // Generate a neural network from CPPN
    NeuralNetwork generateNetwork(
        const CPPN& cppn,
        const std::vector<glm::vec2>& inputPositions,
        const std::vector<glm::vec2>& hiddenPositions,
        const std::vector<glm::vec2>& outputPositions
    );
};
```

**Novelty Search:**
```cpp
class NoveltySearch {
public:
    struct BehaviorVector {
        std::vector<float> features;  // Behavior characterization
    };

    float calculateNovelty(
        const BehaviorVector& behavior,
        const std::vector<BehaviorVector>& archive,
        int k = 15  // k-nearest neighbors
    );

    void addToArchive(const BehaviorVector& behavior, float noveltyThreshold);

    // Behavior characterization from creature actions
    BehaviorVector characterizeBehavior(const Creature& creature, float timeWindow);
};
```

**New Files Needed:**
| File | Purpose |
|------|---------|
| `src/ai/CPPN.h/cpp` | Compositional pattern networks |
| `src/ai/HyperNEAT.h/cpp` | HyperNEAT implementation |
| `src/ai/NoveltySearch.h/cpp` | Novelty-based evolution |
| `src/ai/CoevolutionManager.h/cpp` | Predator-prey coevolution |
| `src/ai/SpeciesCompetition.h/cpp` | Species-level selection |

**Estimated Complexity:** HIGH
- Advanced neuroevolution techniques
- Behavior characterization is problem-specific
- Balancing novelty vs fitness

---

### 7.2 Steering Behaviors

#### Description
Expand the existing SteeringBehaviors with more sophisticated movement patterns.

**Extended Steering:**
```cpp
class SteeringBehaviors {
public:
    // Existing
    glm::vec3 seek(const glm::vec3& target);
    glm::vec3 flee(const glm::vec3& threat);
    glm::vec3 arrive(const glm::vec3& target, float slowRadius);
    glm::vec3 wander();

    // New behaviors
    glm::vec3 pursue(const Creature& target);      // Predictive chase
    glm::vec3 evade(const Creature& pursuer);      // Predictive escape
    glm::vec3 hide(const glm::vec3& threat, const std::vector<Obstacle>& obstacles);
    glm::vec3 interpose(const Creature& a, const Creature& b);

    // Group behaviors
    glm::vec3 separation(const std::vector<Creature*>& neighbors, float radius);
    glm::vec3 alignment(const std::vector<Creature*>& neighbors);
    glm::vec3 cohesion(const std::vector<Creature*>& neighbors);
    glm::vec3 flocking(const std::vector<Creature*>& neighbors);  // Combined

    // Path following
    glm::vec3 followPath(const Path& path, float waypointRadius);
    glm::vec3 offsetPursuit(const Creature& leader, const glm::vec3& offset);

    // Obstacle avoidance
    glm::vec3 obstacleAvoidance(const std::vector<Obstacle>& obstacles);
    glm::vec3 wallAvoidance(const std::vector<Wall>& walls);

    // Combined behaviors
    glm::vec3 calculatePrioritized(const std::vector<BehaviorWeight>& behaviors);
    glm::vec3 calculateBlended(const std::vector<BehaviorWeight>& behaviors);
};
```

**Modifications to Existing Files:**
| File | Changes Required |
|------|------------------|
| `SteeringBehaviors.h/cpp` | Add new behavior methods |
| `Creature.h/cpp` | Integrate new steering options |

**Estimated Complexity:** MEDIUM
- Well-documented algorithms (Craig Reynolds)
- Mostly straightforward vector math
- Tuning weights is the challenge

---

### 7.3 Sensory Systems

#### Description
Enhance the existing SensorySystem with more modalities and realism.

**Advanced Perception:**
```cpp
class AdvancedSensorySystem {
public:
    // Visual perception
    struct VisualPercept {
        glm::vec3 position;
        glm::vec3 velocity;
        float size;
        glm::vec3 color;
        CreatureType type;
        float confidence;  // Based on distance, lighting
        float lastSeen;
    };

    std::vector<VisualPercept> getVisualPercepts();

    // Memory and prediction
    struct MemoryEntry {
        VisualPercept percept;
        float timestamp;
        float reliability;  // Decays over time
    };

    glm::vec3 predictPosition(int creatureId, float futureTime);

    // Attention system
    void focusAttention(const glm::vec3& point);
    float getAttentionLevel(const glm::vec3& point);

    // Multi-modal integration
    struct IntegratedPercept {
        glm::vec3 estimatedPosition;
        float confidence;
        std::bitset<NUM_SENSES> contributingSenses;
    };

    std::vector<IntegratedPercept> getIntegratedPercepts();
};
```

**New Files Needed:**
| File | Purpose |
|------|---------|
| `src/entities/perception/PerceptMemory.h/cpp` | Spatial memory |
| `src/entities/perception/AttentionSystem.h/cpp` | Selective attention |
| `src/entities/perception/MultiSensoryIntegration.h/cpp` | Sense fusion |
| `src/entities/perception/PerceptPrediction.h/cpp` | Future state prediction |

**Estimated Complexity:** MEDIUM-HIGH
- Builds on existing sensory system
- Memory and prediction add complexity
- Attention system needs tuning

---

### 7.4 Social Behaviors

#### Description
Implement social structures, communication, and cooperative behaviors.

**Social System:**
```cpp
class SocialSystem {
public:
    // Social relationships
    struct Relationship {
        int otherCreatureId;
        float familiarity;     // 0-1
        float affiliativeness; // -1 (enemy) to 1 (ally)
        int interactions;
        float lastInteraction;
    };

    // Group dynamics
    struct SocialGroup {
        int leaderId;
        std::vector<int> memberIds;
        glm::vec3 territory;
        float territoryRadius;
        float cohesion;
    };

    // Communication
    enum Signal {
        ALARM_CALL,      // Predator warning
        FOOD_CALL,       // Food discovery
        MATING_CALL,     // Reproductive availability
        TERRITORIAL,     // Territory marking
        SUBMISSION,      // Social hierarchy
        AGGRESSION       // Threat display
    };

    void emitSignal(Signal type, float intensity);
    void receiveSignal(int senderId, Signal type, float intensity);

    // Social learning
    void observeBehavior(int otherCreatureId, const Action& action, float outcome);
    float getImitationProbability(const Action& observedAction);

    // Cooperation
    bool shouldCooperate(int otherCreatureId);
    void recordCooperationOutcome(int partnerId, float benefit);
};
```

**Emergent Social Structures:**
```cpp
// Dominance hierarchies
class DominanceHierarchy {
public:
    void recordContest(int winnerId, int loserId);
    float getDominanceRank(int creatureId);
    int getAlpha() const;
    bool canChallenge(int challengerId, int targetId);
};

// Kinship
class KinshipTracker {
public:
    float getRelatedness(int creature1, int creature2);
    std::vector<int> getKin(int creatureId, float minRelatedness);
    bool isOffspring(int parentId, int childId);
};

// Coalition formation
class CoalitionSystem {
public:
    struct Coalition {
        std::vector<int> members;
        int targetId;
        float commitmentLevel;
    };

    void formCoalition(const std::vector<int>& members, int target);
    void evaluateCoalitionSuccess(int coalitionId);
};
```

**New Files Needed:**
| File | Purpose |
|------|---------|
| `src/entities/social/SocialSystem.h/cpp` | Core social mechanics |
| `src/entities/social/DominanceHierarchy.h/cpp` | Social ranking |
| `src/entities/social/KinshipTracker.h/cpp` | Family relationships |
| `src/entities/social/Communication.h/cpp` | Signal system |
| `src/entities/social/CooperationModule.h/cpp` | Cooperative behaviors |
| `src/entities/social/SocialLearning.h/cpp` | Behavior imitation |

**Engine Module Dependencies:**
- Genetics (kinship calculation)
- Neural network (social decision inputs)
- Sensory system (signal detection)
- Steering behaviors (group movement)

**Estimated Complexity:** VERY HIGH
- Complex emergent dynamics
- Balancing cooperation vs competition
- Testing social behaviors is difficult

---

## Implementation Priority Matrix

| Feature | Impact | Complexity | Dependencies | Priority |
|---------|--------|------------|--------------|----------|
| Day/Night Cycle | High | Medium | Rendering | **P1** |
| Skeletal Animation | High | Medium-High | - | **P1** |
| Shadow Mapping | High | Medium | - | **P1** |
| Seasonal Changes | Medium | Medium | SeasonManager | **P2** |
| GPU Skinning | High | Medium | Skeleton | **P2** |
| Procedural Locomotion | High | High | IK, Skeleton | **P2** |
| Flying Creatures | High | High | Animation, Physics | **P3** |
| Steering Behaviors | Medium | Medium | - | **P3** |
| Food Web Dynamics | Medium | High | Ecosystem | **P3** |
| Aquatic Creatures | High | High | Water system | **P4** |
| Weather Effects | Medium | High | Particles | **P4** |
| Atmospheric Scattering | Medium | Medium-High | Day/Night | **P4** |
| Social Behaviors | High | Very High | Neural network | **P5** |
| Neural Network Evolution | High | High | NEAT | **P5** |
| Amphibians | Medium | Very High | Flying + Aquatic | **P5** |
| Volumetric Effects | Low | Very High | Rendering | **P6** |
| SSR | Low | High | Deferred rendering | **P6** |

### Recommended Implementation Order

1. **Foundation Phase** (P1)
   - Day/Night Cycle (affects all creatures)
   - Basic Skeletal Animation
   - Shadow Mapping

2. **Animation Phase** (P2)
   - GPU Skinning
   - IK Solvers
   - Procedural Locomotion
   - Seasonal Integration

3. **Creature Expansion Phase** (P3)
   - Flying Creatures
   - Extended Steering Behaviors
   - Food Web Dynamics

4. **Environment Phase** (P4)
   - Aquatic Creatures
   - Weather System
   - Atmospheric Rendering

5. **Intelligence Phase** (P5)
   - Social Behaviors
   - Advanced Neural Evolution
   - Amphibians

6. **Polish Phase** (P6)
   - Volumetric Effects
   - Screen-Space Reflections
   - Performance Optimization

---

## Technical Dependencies

### Dependency Graph

```
Skeletal Animation
    └── GPU Skinning
        └── Procedural Locomotion
            └── Flying Creatures
            └── Aquatic Creatures
                └── Amphibians

Day/Night Cycle
    └── Atmospheric Scattering
    └── Shadow Mapping
        └── Volumetric Lighting

Weather System
    └── Particle System
        └── Rain/Snow Rendering

NEAT (existing)
    └── HyperNEAT
    └── Novelty Search

Social Behaviors
    └── Communication System
    └── Kinship Tracker
        └── Coalition System
```

### External Library Considerations

| Feature | Potential Libraries | Notes |
|---------|---------------------|-------|
| Physics | Bullet Physics, PhysX | For flight/water physics |
| Animation | Ozz-Animation | High-performance skeletal |
| Particles | Custom or compute shaders | Performance critical |
| Audio | OpenAL, FMOD | For weather, communication |

---

## Conclusion

This roadmap provides a structured path from the current simulation to a fully-featured ecosystem simulation with diverse creature types, realistic animation, dynamic environments, and emergent artificial intelligence. Each phase builds upon previous work while maintaining system stability.

The modular architecture of OrganismEvolution makes these extensions feasible. Key success factors include:

1. **Incremental Integration**: Add features one at a time with thorough testing
2. **Performance Monitoring**: Profile regularly as complexity increases
3. **Behavior Tuning**: Expect extensive parameter adjustment for emergent systems
4. **Visual Feedback**: Implement debug visualizations for each new system

This document should be updated as features are implemented and new requirements emerge.
