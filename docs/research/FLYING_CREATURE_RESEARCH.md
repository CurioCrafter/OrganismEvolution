# Flying Creature Research

## Overview

This document summarizes research into bird and insect flight mechanics for implementing realistic flying creature animation and behavior in the OrganismEvolution simulation.

---

## 1. Bird Flight Mechanics

### 1.1 Wing Bone Structure

Bird wings follow a modified forelimb structure with the following skeletal hierarchy:

```
Shoulder (Scapula/Coracoid)
└── Humerus (Upper Wing)
    └── Radius/Ulna (Forearm) [parallel bones]
        └── Carpals (Wrist - radiale/ulnare)
            └── Carpometacarpus (Fused palm bones)
                ├── Digit 1: Alula (2 phalanges) - "thumb"
                ├── Digit 2: Major digit (2 phalanges)
                └── Digit 3: Minor digit (1 phalanx)
```

#### Key Bone Functions for Animation

| Bone | Function | Animation Notes |
|------|----------|-----------------|
| **Humerus** | Primary rotation, connects to flight muscles | Short bone, rotates away from body during flight |
| **Radius** | Lightweight support bone | Thinner than ulna, slides forward when elbow flexes |
| **Ulna** | Secondary feather attachment | Directly transmits force to secondary feathers |
| **Carpometacarpus** | Primary feather support | Fused carpals + metacarpals; supports thrust feathers |
| **Alula** | "Bastard wing" - prevents stall | Moves independently; acts like airplane slats |

### 1.2 Joint Movement Constraints

Bird joints operate in a **coupled mechanism**:

```cpp
struct WingJointState {
    float shoulderAngle;    // Main rotation (-180 to +180 degrees)
    float elbowAngle;       // Hinged joint (0 to ~140 degrees)
    float wristAngle;       // Coupled to elbow via ligaments

    void updateCoupledJoints() {
        // Anatomical coupling: radius slides forward on ulna
        wristAngle = elbowAngle * WRIST_COUPLING_FACTOR; // ~0.7-0.9
    }
};
```

**Critical insight**: Most wing rotation comes from the **shoulder joint** (hypermobile), not the elbow or wrist.

### 1.3 Flap Cycle Phases

#### Phase 1: Downstroke (Power Stroke)
- **Duration**: 55-60% of cycle
- Wing fully extended
- Movement: Top-back to front-down
- Leading edge tilts downward
- Feathers compress (solid airfoil)
- Primary feather tips bend backward
- **Generates majority of thrust and lift**

```cpp
struct DownstrokeParams {
    float startAngle = 60.0f;       // Degrees above horizontal
    float endAngle = -30.0f;        // Degrees below horizontal
    float leadingEdgePitch = -15.0f; // Tilts down
    float featherSpread = 0.0f;     // Fully closed
    float duration = 0.55f;          // Fraction of cycle
};
```

#### Phase 2: Upstroke (Recovery Stroke)
- **Duration**: 40-45% of cycle
- Wing partially folds at elbow and wrist
- Leading edge tilts backward
- Feathers separate (allowing air passage)
- Minimizes drag and air resistance
- S-shaped path (vs straight in downstroke)

```cpp
struct UpstrokeParams {
    float startAngle = -30.0f;
    float endAngle = 60.0f;
    float leadingEdgePitch = 10.0f;   // Tilts backward
    float featherSpread = 0.8f;       // Spread apart
    float elbowFlex = 0.3f;           // Partial fold
    float wristFlex = 0.25f;          // Coupled fold
    float duration = 0.45f;
};
```

#### Phase 3: Glide
- Wings fully extended, no flapping
- Maintains lift through airfoil shape
- Slight downward tilt for thrust from descent
- Used between flap bursts or during soaring

### 1.4 Flap Cycle Timing by Bird Type

| Bird Type | Flaps/Second | Amplitude | Notes |
|-----------|--------------|-----------|-------|
| Hummingbird | 12-80 Hz | Figure-8 pattern | Unique hovering mechanics |
| Small songbird | 8-15 Hz | 60-90 degrees | Bounding flight pattern |
| Medium bird (crow) | 3-5 Hz | 45-70 degrees | Standard flapping |
| Large soaring bird | 1-3 Hz | 30-50 degrees | Mostly gliding |

---

## 2. Insect Flight Mechanics

### 2.1 Wing Beat Frequencies

| Insect Type | Frequency (Hz) | Notes |
|-------------|----------------|-------|
| **Butterflies** | 5-12 Hz | Slowest; easy to animate frame-by-frame |
| **Large Moths** | 5-40 Hz | Variable by species |
| **Dragonflies** | 20-40 Hz | Four independent wings |
| **Bumblebees** | 130-190 Hz | Size-dependent |
| **Honeybees** | 230-240 Hz | Short stroke amplitude (~90°) |
| **House Flies** | 200-330 Hz | Asynchronous muscles |
| **Small Flies** | 265+ Hz | Can exceed 1000 Hz for midges |

### 2.2 The Figure-8 Wing Motion Pattern

The characteristic figure-8 pattern is fundamental to insect flight:

```cpp
void updateWingPosition(WingKinematics& wing, float time, float freq) {
    float phase = time * freq * 2.0f * M_PI;

    // Primary stroke motion (forward/backward)
    wing.strokePosition = sin(phase) * strokeAmplitude;

    // Figure-8 deviation (creates the "8" shape)
    wing.deviationAngle = sin(2.0f * phase) * deviationAmplitude;

    // Wing rotation (pronation/supination at stroke reversals)
    wing.rotationAngle = baseAngleOfAttack + sin(phase) * rotationRange;
}
```

#### Reynolds Number Effects
- **High Reynolds Number** (larger/faster insects): Horizontal, elongated figure-8s
- **Low Reynolds Number** (smaller insects): Vertical, thick figure-8s

### 2.3 Two-Wing vs Four-Wing Insects

#### Two-Wing Insects (Diptera - Flies, Mosquitoes)
- Hindwings evolved into **halteres** (gyroscopic balance organs)
- Wings beat **exactly in-phase**
- Use **indirect flight muscles** attached to thorax
- Frequencies can exceed 100 Hz
- Exceptional agility and stability

#### Four-Wing Insects - Dragonflies (Odonata)
- Each wing controlled **independently** by direct muscles
- Variable phase relationships:
  - **Hovering**: 180° phase difference (anti-phase)
  - **Forward flight**: 54-100° phase difference
  - **Acceleration**: 0° (in-phase)
- 22% improved efficiency with optimal phase offset

#### Four-Wing Insects - Bees/Butterflies
- Fore and hind wings **hook together**
- Functionally behave as two-winged
- Very low frequency for butterflies (5-40 Hz)

### 2.4 Three Aerodynamic Mechanisms

1. **Delayed Stall (Leading Edge Vortex)**
   - Occurs during translation phases
   - Wings maintain high angle of attack (~45-50°)
   - Provides primary lift

2. **Rotational Circulation**
   - Occurs at stroke reversals
   - Rapid wing rotation (pronation/supination)
   - Provides force modulation for steering

3. **Wake Capture**
   - Wing interacts with vortices from previous stroke
   - Enhances lift at start of each half-stroke

---

## 3. Key Animation Parameters

### 3.1 Bird Flight Parameters

```cpp
struct BirdFlightParams {
    // Physical characteristics
    float mass;                 // kg
    float wingspan;             // meters
    float wingArea;             // m²
    float aspectRatio;          // wingspan² / area (6-16 typical)
    float wingLoading;          // mass / wingArea (N/m²)

    // Aerodynamic coefficients
    float liftCoefficient;      // Cl (0.5-3.0)
    float dragCoefficient;      // Cd (0.02-0.1)
    float maxLiftCoefficient;   // Cl_max before stall (1.5-3.0)

    // Flapping parameters
    float flapFrequency;        // Hz
    float flapAmplitude;        // degrees (15-90)
    float strokePlaneAngle;     // angle relative to horizontal

    // Derived
    float cruiseSpeed;          // m/s
    float stallSpeed;           // m/s (minimum for level flight)
};
```

### 3.2 Insect Flight Parameters

```cpp
struct InsectFlightParams {
    // Timing
    float wingbeatFrequency;      // Hz (see table)
    float strokeDuration;         // 1.0 / frequency

    // Stroke geometry
    float strokeAmplitude;        // Degrees (90-180)
    float strokePlaneAngle;       // Angle from horizontal
    float deviationAmplitude;     // Figure-8 thickness (5-30°)

    // Wing rotation
    float midstrokeAOA;           // Angle of attack (45-55°)
    float rotationTiming;         // When rotation starts
    float rotationDuration;       // How long rotation takes

    // Four-wing specific
    float foreHindPhaseOffset;    // Phase between fore/hind wings
    bool wingsIndependent;        // True for dragonflies
    bool wingsCoupled;            // True for bees, butterflies
};
```

---

## 4. Aerodynamic Physics

### 4.1 Basic Lift Equation

```cpp
float calculateLift(float velocity, float airDensity, float wingArea,
                    float liftCoeff, float angleOfAttack) {
    // L = 0.5 * rho * V² * S * Cl
    return 0.5f * airDensity * velocity * velocity * wingArea * liftCoeff;
}
```

### 4.2 Drag Equation (Induced + Parasitic)

```cpp
float calculateDrag(float velocity, float airDensity, float wingArea,
                    float dragCoeff, float aspectRatio, float liftCoeff) {
    // Induced drag: proportional to Cl² and inversely to AR
    float inducedDrag = (liftCoeff * liftCoeff) /
                        (PI * aspectRatio * SPAN_EFFICIENCY);

    // Total drag
    float totalCd = dragCoeff + inducedDrag;
    return 0.5f * airDensity * velocity * velocity * wingArea * totalCd;
}
```

### 4.3 Thrust from Flapping

```cpp
float calculateFlapThrust(float flapFrequency, float flapAmplitude,
                          float wingArea, float velocity) {
    float strokeVelocity = flapFrequency * flapAmplitude * (PI / 180.0f);
    return THRUST_COEFFICIENT * wingArea * strokeVelocity * velocity;
}
```

### 4.4 Aspect Ratio Effects

| Aspect Ratio | Flight Characteristics |
|--------------|----------------------|
| Low (3-5) | High maneuverability, less efficient cruise |
| Medium (6-8) | Balanced performance |
| High (9-16) | Efficient cruise, less maneuverable |

---

## 5. Reference Parameters by Species

### 5.1 Birds

| Parameter | Sparrow | Crow | Eagle |
|-----------|---------|------|-------|
| Mass (kg) | 0.02-0.04 | 0.3-0.6 | 3-6 |
| Wingspan (m) | 0.2-0.25 | 0.8-1.0 | 1.8-2.3 |
| Aspect Ratio | 5-7 | 6-8 | 7-10 |
| Flap Freq (Hz) | 10-15 | 3-5 | 1-2 |
| Amplitude (°) | 70-90 | 45-70 | 30-50 |
| Cruise Speed (m/s) | 8-15 | 10-15 | 12-20 |

### 5.2 Insects

| Parameter | Butterfly | Dragonfly | Bee | Fly |
|-----------|-----------|-----------|-----|-----|
| Frequency (Hz) | 10 | 30 | 200 | 250 |
| Stroke Amplitude (°) | 120 | 70 | 90 | 150 |
| Angle of Attack (°) | 30 | 35 | 50 | 45 |
| Wing Count | 4 (coupled) | 4 (independent) | 4 (coupled) | 2 |
| Phase Offset | N/A | 0-180° | N/A | N/A |

---

## 6. Implementation Recommendations

### 6.1 Real-Time Animation Strategy

- **Low-frequency (< 30 Hz)**: Full skeletal animation with keyframes
- **Medium-frequency (30-100 Hz)**: Procedural animation with sin/cos
- **High-frequency (> 100 Hz)**: Motion blur or translucent wing disc

### 6.2 Visual Simplification

```cpp
void renderWings(Creature& creature, float deltaTime) {
    if (creature.wingbeatFrequency < 30.0f) {
        renderAnimatedWings(creature);
    } else if (creature.wingbeatFrequency < 100.0f) {
        renderMotionBlurredWings(creature);
    } else {
        renderWingDisc(creature, calculateBlurOpacity(creature.wingbeatFrequency));
    }
}
```

### 6.3 Stroke Amplitude Adjustments

- **Hovering**: Normal amplitude (~90-120°)
- **Forward flight**: Reduced amplitude, tilted stroke plane
- **Ascending**: Increase amplitude by 30-45%
- **Maneuvering**: Asymmetric amplitude left/right

---

## 7. Sources

### Wing Anatomy
- Bird Wing Anatomy - AnatomyLearner
- Bird Wing Anatomy - Birdfact
- Bird anatomy - Wikipedia

### Flight Animation
- Bird Flight Analysis for Animation - Animator Notebook
- How to Animate Birds in Flight - Animation Mentor
- Wing Rig Breakdown - LightBulbBox

### Physics and Aerodynamics
- Flappy Hummingbird Simulation - arXiv
- Bird flight - Wikipedia
- Aerodynamics of bird flight - EPJ Conferences

### Insect Flight
- Insect Flight - Wikipedia
- Wing mechanics of 23 flying insects in slow motion
- Formulae for Insect Wingbeat Frequency - PMC
- Phasing of dragonfly wings - Royal Society Interface

### Game Development
- Unity Free Flight - GitHub
- Procedural Birds - itch.io
- Advanced Insects System for Unreal Engine - Fab
