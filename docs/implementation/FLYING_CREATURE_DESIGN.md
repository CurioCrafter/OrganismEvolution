# Flying Creature System Design

## Overview

This document describes the design and architecture of the enhanced flying creature system for the OrganismEvolution simulation. The system supports three distinct flying creature types with specialized behaviors, animations, and physics.

---

## 1. Creature Type Hierarchy

### 1.1 Flying Creature Types

```cpp
enum class CreatureType {
    // Aerial Types
    FLYING,         // Generic flying creature (legacy - omnivore behavior)
    FLYING_BIRD,    // Bird - feathered wings, 2 legs, high glide ratio
    FLYING_INSECT,  // Insect - membrane wings, 4-6 legs, fast wing beats
    AERIAL_PREDATOR // Aerial apex predator - hawk/eagle, dives from above
};
```

### 1.2 Creature Type Characteristics

| Type | Size | Speed | Wings | Glide Ratio | Diet |
|------|------|-------|-------|-------------|------|
| FLYING (legacy) | 0.4-0.8 | 15-25 | Feathered | 0.4-0.7 | Omnivore |
| FLYING_BIRD | 0.3-0.9 | 12-22 | Feathered | 0.5-0.8 | Seeds, insects |
| FLYING_INSECT | 0.1-0.4 | 8-18 | Membrane | 0.1-0.4 | Nectar, plant matter |
| AERIAL_PREDATOR | 0.7-1.3 | 18-30 | Feathered | 0.7-0.9 | Small creatures |

---

## 2. Genome Extensions

### 2.1 Flying-Specific Genome Traits

```cpp
// In Genome.h
// Flying Creature Traits
float wingSpan;           // 0.5 - 2.0 (ratio to body size)
float flapFrequency;      // 2.0 - 8.0 Hz (birds), 20-100 Hz (insects)
float glideRatio;         // 0.3 - 0.8 (higher = more gliding)
float preferredAltitude;  // 15.0 - 40.0 (height above terrain)
```

### 2.2 Randomization Methods

```cpp
void Genome::randomizeFlying();       // Generic flying creature
void Genome::randomizeBird();         // Bird-specific traits
void Genome::randomizeInsect();       // Insect-specific traits
void Genome::randomizeAerialPredator(); // Raptor-specific traits
```

#### Bird Genome Characteristics
- Larger wingspan (1.0-1.8)
- Lower flap frequency (3-8 Hz)
- Higher glide ratio (0.5-0.8)
- Enhanced vision (50-80 range, 0.7-0.95 acuity)
- Good color perception (0.7-0.9)

#### Insect Genome Characteristics
- Smaller size (0.1-0.4)
- Very high flap frequency (20-100 Hz)
- Low glide ratio (0.1-0.4)
- Near 360° field of view (4.5-6.0 radians)
- Excellent motion detection (0.8-0.95)

#### Aerial Predator Genome Characteristics
- Larger size (0.7-1.3)
- Low flap frequency, powerful strokes (2-4 Hz)
- Excellent glide ratio (0.7-0.9)
- Exceptional vision (80-120 range, 0.9-0.99 acuity)
- High altitude preference (40-80)

---

## 3. Animation System

### 3.1 Wing Animator Architecture

```cpp
// In animation/WingAnimator.h

enum class WingType {
    FEATHERED,      // Bird wings
    MEMBRANE,       // Bat-like wings
    INSECT_SINGLE,  // Two-wing insects (flies)
    INSECT_DOUBLE,  // Four-wing insects (dragonflies)
    INSECT_COUPLED  // Four-wing coupled (bees, butterflies)
};

enum class FlightAnimState {
    GROUNDED,       // Wings folded
    TAKING_OFF,     // Transition to flight
    FLAPPING,       // Active powered flight
    GLIDING,        // Wings extended, minimal flapping
    DIVING,         // Fast descent, wings tucked
    HOVERING,       // Stationary flight
    LANDING         // Transition to ground
};

class WingAnimator {
    void initialize(const WingAnimConfig& config);
    void setFlightState(FlightAnimState state);
    void setVelocity(float velocity);
    void setBankAngle(float angle);
    void update(float deltaTime);

    const WingPose& getLeftWingPose() const;
    const WingPose& getRightWingPose() const;
};
```

### 3.2 Wing Pose Structure

```cpp
struct WingPose {
    glm::quat shoulderRotation;
    glm::quat elbowRotation;
    glm::quat wristRotation;
    glm::quat tipRotation;
    float featherSpread;    // 0-1
    float wingTipBend;      // Backward bend
};
```

### 3.3 Animation Phases

#### Downstroke (55% of cycle)
- Wing fully extended
- Shoulder rotates from +60° to -30°
- Elbow/wrist extended
- Feathers closed for maximum lift
- Wing tip bends backward from air pressure

#### Upstroke (45% of cycle)
- Wing partially folds
- Shoulder rotates from -30° to +60°
- Elbow flexes up to 30%
- Wrist coupled to elbow
- Feathers spread for low drag

#### Glide
- Wings fully extended
- Slight dihedral (5° upward V)
- Feathers partially spread
- No active movement

#### Insect Figure-8
- Stroke angle: sin(phase) * amplitude
- Deviation: sin(2 * phase) * deviation_amplitude
- Rotation: sin(phase) * rotation_range

---

## 4. Flight Physics System

### 4.1 Flight Behavior Class

```cpp
// In entities/FlightBehavior.h

enum class FlightState {
    GROUNDED,       // On terrain, can takeoff
    TAKING_OFF,     // Transitioning to flight
    FLYING,         // Normal powered flight
    GLIDING,        // Energy-saving soaring
    DIVING,         // Fast descent
    LANDING,        // Transitioning to ground
    HOVERING,       // Stationary flight
    THERMAL_SOARING // Riding thermal updrafts
};

class FlightBehavior {
    void initialize(const WingPhysicsConfig& config);
    void update(float deltaTime, const Terrain& terrain);

    // Flight commands
    void commandTakeoff();
    void commandLand();
    void commandDive();
    void commandGlide();
    void commandHover();

    // State access
    FlightState getState() const;
    float getBankAngle() const;
    float getPitchAngle() const;
    bool isStalling() const;
};
```

### 4.2 Physics Configuration

```cpp
struct WingPhysicsConfig {
    float wingSpan = 1.0f;
    float wingArea = 0.5f;
    float aspectRatio = 7.0f;
    float liftCoefficient = 1.0f;
    float maxLiftCoefficient = 1.8f;
    float dragCoefficient = 0.05f;
    float mass = 1.0f;

    // Factory methods
    static WingPhysicsConfig forBird(float size, float wingspan);
    static WingPhysicsConfig forInsect(float size, float wingspan);
    static WingPhysicsConfig forRaptor(float size, float wingspan);
};
```

### 4.3 Aerodynamic Calculations

#### Lift
```
L = 0.5 * rho * V² * S * Cl
```
- rho: Air density (1.225 kg/m³ at sea level)
- V: Velocity (m/s)
- S: Wing area (m²)
- Cl: Lift coefficient

#### Drag
```
D = 0.5 * rho * V² * S * Cd_total
Cd_total = Cd_parasitic + Cd_induced
Cd_induced = Cl² / (π * AR * e)
```
- AR: Aspect ratio
- e: Oswald efficiency (~0.85)

#### Stall Speed
```
V_stall = sqrt(2 * m * g / (rho * S * Cl_max))
```

### 4.4 Thermal Soaring

```cpp
struct ThermalColumn {
    glm::vec3 center;
    float radius;
    float strength;     // Vertical velocity (m/s)
    float maxAltitude;

    float getStrengthAt(const glm::vec3& pos) const;
};
```

Thermals provide vertical lift without energy expenditure:
- Strength decreases toward edges (quadratic falloff)
- Maximum altitude limits thermal effect
- Creatures can detect and exploit thermals

---

## 5. Behavior System

### 5.1 Behavior Dispatch

```cpp
// In Creature.cpp
void Creature::update(...) {
    // ...
    if (isFlying(type)) {
        if (type == CreatureType::FLYING_BIRD) {
            updateBehaviorBird(deltaTime, ...);
        } else if (type == CreatureType::FLYING_INSECT) {
            updateBehaviorInsect(deltaTime, ...);
        } else if (type == CreatureType::AERIAL_PREDATOR) {
            updateBehaviorAerialPredator(deltaTime, ...);
        } else {
            updateBehaviorFlying(deltaTime, ...);
        }
        updateFlyingPhysics(deltaTime, terrain);
    }
}
```

### 5.2 Bird Behavior

- **Foraging**: Circle above food, descend to eat
- **Flocking**: Light flocking with other birds (boids algorithm)
- **Soaring**: Use thermals when available
- **Altitude maintenance**: Proportional controller

### 5.3 Insect Behavior

- **Nectar foraging**: Visit flowers/plants
- **Agile flight**: Quick direction changes
- **Hovering**: Can maintain position
- **Low altitude**: Generally fly closer to ground

### 5.4 Aerial Predator Behavior

- **Hunting from above**: Circle at high altitude
- **Diving attack**: Steep dive toward prey
- **Target selection**: Prioritize small, vulnerable creatures
- **Energy conservation**: Extensive gliding between hunts

---

## 6. Rendering System

### 6.1 Flying Creature Shader

```hlsl
// CreatureFlying.hlsl

cbuffer WingConstants : register(b1) {
    float wingPhase;
    float flapFrequency;
    float glideRatio;
    float bankAngle;
    float pitchAngle;
    float wingSpan;
    float isInsect;
    float motionBlurAmount;
    float4 wingBoneAngles;
};
```

### 6.2 Visual Features

#### Bird Wings
- Feather pattern generation
- Barb detail based on wing position
- Iridescence for some species
- Proper shadow casting

#### Insect Wings
- Membrane transparency
- Vein pattern
- Motion blur for fast-beating wings
- Optional wing disc rendering

### 6.3 Animation in Shader

```hlsl
float3 animateWing(float3 pos, bool isLeftWing) {
    float wingProgress = saturate(abs(pos.x) / (wingSpan * 0.5));

    // Calculate flap angle based on phase
    bool isDownstroke = wingPhase < 0.55;
    float flapAngle = calculateFlapAngle(wingPhase, isDownstroke);

    // Apply progressive rotation toward wing tip
    float3x3 rotation = rotationMatrix(float3(0, 0, side), flapAngle * wingProgress);

    return mul(rotation, pos);
}
```

---

## 7. Energy Model

### 7.1 Flight Energy Costs

```cpp
// Flying creatures have higher metabolism
if (type == CreatureType::FLYING || isFlying(type)) {
    // Wing flapping cost (based on flap frequency)
    float flapCost = genome.flapFrequency * 0.05f * deltaTime;

    // Gliding reduces energy cost
    flapCost *= (1.0f - genome.glideRatio * 0.5f);

    energy -= flapCost;
}
```

### 7.2 Reproduction Requirements

| Creature Type | Energy Threshold | Kill Requirement |
|--------------|------------------|------------------|
| FLYING (legacy) | 160 | 1 |
| FLYING_BIRD | 150 | 0 (herbivore) |
| FLYING_INSECT | 140 | 0 (herbivore) |
| AERIAL_PREDATOR | 180 | 2 |

---

## 8. Food Chain Integration

### 8.1 Predation Rules

```cpp
// FLYING and FLYING_BIRD can hunt small frugivores
case CreatureType::FLYING:
case CreatureType::FLYING_BIRD:
    return prey == CreatureType::FRUGIVORE && preySize < 0.8f;

// FLYING_INSECT generally not predators
case CreatureType::FLYING_INSECT:
    return false;

// AERIAL_PREDATOR can hunt many creature types
case CreatureType::AERIAL_PREDATOR:
    return (prey == CreatureType::FRUGIVORE && preySize < 1.2f) ||
           (prey == CreatureType::SMALL_PREDATOR && preySize < 0.8f) ||
           (prey == CreatureType::FLYING_BIRD && preySize < 0.9f) ||
           (prey == CreatureType::FLYING_INSECT) ||
           (prey == CreatureType::FLYING && preySize < 0.9f);
```

### 8.2 Prey-Predator Dynamics

- **Ground predators** generally cannot catch flying creatures
- **Aerial predators** hunt from above using diving attacks
- **Flying birds** can catch insects but rarely hunt ground creatures
- **Insects** are prey for many predator types

---

## 9. Integration Checklist

### 9.1 Files Added
- [x] `src/animation/WingAnimator.h` - Wing animation system
- [x] `src/entities/FlightBehavior.h` - Flight physics and state machine
- [x] `src/entities/FlightBehavior.cpp` - Implementation stub
- [x] `shaders/hlsl/CreatureFlying.hlsl` - Flying creature shader

### 9.2 Files Modified
- [x] `src/entities/CreatureType.h` - New flying types and helpers
- [x] `src/entities/Genome.h` - Randomization method declarations
- [x] `src/entities/Genome.cpp` - Randomization implementations

### 9.3 Files To Integrate (Future Work)
- [ ] `src/entities/Creature.cpp` - Behavior dispatch for new types
- [ ] `src/main.cpp` - Spawn new flying creature types
- [ ] Graphics pipeline - Use CreatureFlying.hlsl for flying creatures
- [ ] Creature builder - Generate appropriate body plans

---

## 10. Usage Example

```cpp
// Spawning a flying bird
Genome birdGenome;
birdGenome.randomizeBird();
auto bird = std::make_unique<Creature>(
    glm::vec3(0, 30, 0),  // Start in the air
    birdGenome,
    CreatureType::FLYING_BIRD
);

// Spawning an aerial predator (hawk)
Genome raptorGenome;
raptorGenome.randomizeAerialPredator();
auto hawk = std::make_unique<Creature>(
    glm::vec3(50, 60, 50),  // High altitude
    raptorGenome,
    CreatureType::AERIAL_PREDATOR
);

// Spawning a flying insect
Genome insectGenome;
insectGenome.randomizeInsect();
auto butterfly = std::make_unique<Creature>(
    glm::vec3(-20, 5, 30),  // Low altitude
    insectGenome,
    CreatureType::FLYING_INSECT
);
```

---

## 11. Future Enhancements

### 11.1 Behavior Enhancements
- Migration patterns
- Seasonal behavior changes
- Territory defense
- Nesting sites
- Formation flying (V-formation)

### 11.2 Physics Enhancements
- Wind effects
- Weather response (rain, storms)
- Thermal detection AI
- Realistic stall and recovery

### 11.3 Visual Enhancements
- Feather detail LOD
- Wing membrane physics
- Particle trail effects
- Dynamic shadow from wings

### 11.4 Ecosystem Integration
- Pollination mechanics (insects)
- Seed dispersal (birds)
- Scavenging behavior
- Disease vector mechanics
