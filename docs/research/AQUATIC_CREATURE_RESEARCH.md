# Aquatic Creature Research Documentation

## Overview

This document compiles research findings on fish swimming locomotion, animation techniques, and underwater ecosystem dynamics used to implement the aquatic creature system.

---

## 1. Fish Swimming Locomotion Styles

### 1.1 Body-Caudal Fin (BCF) Locomotion

Fish use body undulation combined with caudal (tail) fin motion for propulsion. The four main BCF styles are:

#### Anguilliform (Eel-like)
- **Body portion moving**: Entire body (100%)
- **Wave characteristics**: Full-body sinusoidal waves from head to tail
- **Examples**: Eels, lampreys, sea snakes
- **Best for**: Slow, highly maneuverable swimming; navigating tight spaces
- **Implementation**: Wave amplitude is constant along entire spine

#### Subcarangiform (Trout-like)
- **Body portion moving**: Posterior 2/3 of body
- **Wave characteristics**: Amplitude increases toward tail
- **Examples**: Trout, carp, goldfish
- **Best for**: General-purpose swimming
- **Implementation**: Wave begins at mid-body, amplitude ramps up

#### Carangiform (Mackerel-like)
- **Body portion moving**: Last 1/3 of body only
- **Wave characteristics**: Sharp amplitude increase at caudal fin
- **Examples**: Mackerel, tuna (at moderate speeds)
- **Best for**: Fast, efficient cruising
- **Implementation**: Most motion concentrated in tail region; exponential amplitude curve

#### Thunniform (Tuna-like)
- **Body portion moving**: Only the caudal fin
- **Wave characteristics**: Minimal body undulation, powerful tail strokes
- **Examples**: Tuna at high speed, lamnid sharks
- **Best for**: Maximum speed swimming
- **Implementation**: Near-rigid body, only tail oscillates

### 1.2 Median-Paired Fin (MPF) Locomotion

Used for slow-speed maneuvering:

#### Labriform (Fin-rowing)
- Uses pectoral fin oscillation (like rowing)
- Power stroke and recovery stroke
- **Examples**: Wrasses, parrotfish
- Excellent for precise maneuvering

#### Rajiform (Ray-like)
- Wave-like pectoral fin undulation
- **Examples**: Rays, skates
- Graceful, efficient for flat body plans

---

## 2. S-Wave Spine Motion Mathematics

### 2.1 Core Wave Function

The fundamental displacement equation for fish body undulation:

```
displacement(t, body_pos) = amplitude(body_pos) * sin(phase + body_pos * wavelength)
```

Where:
- `t` = time
- `body_pos` = normalized position along body (0 = head, 1 = tail)
- `phase` = ω * t (angular frequency * time)
- `wavelength` = number of wave cycles along the body
- `amplitude(body_pos)` = stiffness-modulated amplitude

### 2.2 Stiffness Mask Function

Different body regions have different flexibility:

```cpp
float calculateStiffnessMask(float bodyPosition) {
    // Head region: very stiff
    float headStiffness = 0.85f;

    // Mid-body: moderate flex
    float midStiffness = 0.5f;

    // Tail: very flexible
    float tailStiffness = 0.15f;

    // Smooth interpolation between regions
    float headRegion = 1.0 - smoothstep(0.0, 0.3, bodyPosition);
    float tailRegion = smoothstep(0.6, 1.0, bodyPosition);
    float midRegion = 1.0 - headRegion - tailRegion;

    float stiffness = headRegion * headStiffness +
                     midRegion * midStiffness +
                     tailRegion * tailStiffness;

    return 1.0 - stiffness;  // Return flexibility
}
```

### 2.3 Swimming Style Amplitude Curves

```cpp
// Carangiform: exponential increase toward tail
amplitude *= pow(bodyPos, 1.5);

// Anguilliform: constant amplitude
// amplitude unchanged

// Subcarangiform: starts at 1/3 body
if (bodyPos > 0.33) {
    float adjustedPos = (bodyPos - 0.33) / 0.67;
    amplitude *= adjustedPos;
} else {
    amplitude *= 0.1;
}

// Thunniform: only tail moves
if (bodyPos > 0.7) {
    float adjustedPos = (bodyPos - 0.7) / 0.3;
    amplitude *= adjustedPos * 2.0;
} else {
    amplitude *= 0.05;
}
```

---

## 3. Fin Functions

### 3.1 Caudal Fin (Tail)
- **Primary function**: Propulsion (thrust generation)
- **Animation**: Primary oscillation, amplitude correlates with swimming speed
- **Types**: Forked (fast swimmers), rounded (maneuverable), asymmetric (sharks)

### 3.2 Dorsal Fin (Back)
- **Primary function**: Stability (prevents rolling and yawing)
- **Animation**: Subtle trailing motion following body wave
- **Behavior**: May fold flat at high speeds for streamlining

### 3.3 Pectoral Fins (Sides)
- **Primary function**: Steering, braking, lift/dive control
- **Animation**: Active during turns, fold during straight swimming
- **Behavior**: Flare out for braking, angle for lift adjustment

### 3.4 Pelvic Fins (Belly)
- **Primary function**: Pitch control, stability
- **Animation**: Subtle adjustments, often move with pectorals

### 3.5 Anal Fin (Rear belly)
- **Primary function**: Stability (mirrors dorsal fin)
- **Animation**: Follows body wave with phase delay

---

## 4. Typical Swimming Parameters

### 4.1 Real Fish Data

| Parameter | Symbol | Typical Range | Description |
|-----------|--------|--------------|-------------|
| Tail Beat Frequency (TBF) | f | 1-10 Hz | Oscillations per second |
| Tail Beat Amplitude (TBA) | A | 0.1-0.2 L | Max lateral displacement |
| Wavelength | λ | 0.8-1.2 L | Wave cycles along body |
| Wave Speed | C | 1.0-1.4 U | Propagation velocity |
| Strouhal Number | St | 0.2-0.4 | Efficiency metric |

(L = body length, U = swimming velocity)

### 4.2 Recommended Animation Parameters

```cpp
struct SwimConfig {
    float bodyWaveSpeed = 3.0f;      // 1-10 Hz
    float bodyWaveAmp = 0.15f;       // 0.1-0.3 radians
    float wavelength = 1.0f;         // 0.8-1.2
    float headStiffness = 0.85f;     // 0-1
    float midStiffness = 0.5f;       // 0-1
    float tailStiffness = 0.15f;     // 0-1
    float caudalFinAmp = 0.25f;      // Tail fin multiplier
    float pectoralFinAmp = 0.2f;     // Side fins
    float dorsalFinAmp = 0.1f;       // Back fin
};
```

---

## 5. Underwater Physics

### 5.1 Buoyancy

Fish maintain neutral buoyancy using swim bladders:

```cpp
float buoyancyForce = (targetDepth - currentDepth) * buoyancyStrength;
velocity.y += buoyancyForce * deltaTime;
```

### 5.2 Water Drag

Water drag is significantly higher than air:

```cpp
// Drag is directional
float forwardDrag = 0.3f;   // Low - streamlined
float lateralDrag = 1.5f;   // High - not streamlined
float verticalDrag = 1.0f;  // Moderate

// Drag force = -0.5 * rho * Cd * A * v^2
```

### 5.3 Thrust from Swimming

Tail beat generates thrust proportional to phase:

```cpp
float phaseFactor = 0.7f + 0.3f * sin(swimPhase);
float thrust = thrustPower * phaseFactor;
```

---

## 6. Schooling Behavior (Boids Algorithm)

### 6.1 Core Rules

1. **Separation**: Steer away from nearby flockmates to avoid crowding
2. **Alignment**: Match heading with local group average
3. **Cohesion**: Steer toward center of mass of neighbors

### 6.2 Fish-Specific Extensions

- **Predator avoidance**: Scatter when threat detected
- **Polarization**: Tendency to align more strongly than typical boids
- **Depth coordination**: School maintains similar depth
- **Speed matching**: Fish adjust TBF to match school

### 6.3 Implementation

```cpp
SchoolingForces calculateSchoolingForces(...) {
    glm::vec3 separation(0), alignment(0), cohesion(0);

    for (each neighbor) {
        // Separation: repel from close neighbors
        if (dist < separationDist) {
            separation -= normalize(toNeighbor) / dist;
        }

        // Alignment: average velocities
        alignment += neighbor.velocity;

        // Cohesion: center of mass
        centerOfMass += neighbor.position;
    }

    // Apply weights
    steeringForce = separation * 2.5 +
                    alignment * 1.5 +
                    cohesion * 1.0;
}
```

---

## 7. Existing Water System Integration

### 7.1 Terrain Shader Water Constants

From `Runtime/Shaders/Terrain.hlsl`:

```hlsl
static const float WATER_LEVEL = 0.012;          // Normalized height
static const float HEIGHT_SCALE = 30.0;          // World units
// Actual water surface ~0.36 world units

// Water depth thresholds
WATER_THRESH = 0.012   // Underwater
WET_SAND_THRESH = 0.025
BEACH_THRESH = 0.05
```

### 7.2 Water Depth Calculation

```hlsl
float calculateWaterDepth(float heightNormalized) {
    float waterSurface = WATER_LEVEL;
    float maxDepth = 0.15;

    float depthBelow = waterSurface - heightNormalized;
    float linearDepth = saturate(depthBelow / maxDepth);

    return 1.0 - exp(-linearDepth * 3.0);  // Exponential curve
}
```

### 7.3 Caustics System

Multi-layer Voronoi noise creates animated light patterns:

```hlsl
float caustic1 = voronoi(causticUV + time * 0.04);
float caustic2 = voronoi(causticUV * 1.7 - time * 0.055);
float caustic3 = voronoi(causticUV * 0.8 + time * 0.03);

float causticIntensity = (1.0 - depth) * 0.35;
```

---

## 8. Sensory System Integration

From `SensorySystem.h`:

```cpp
struct EnvironmentConditions {
    float visibility;      // Reduced underwater
    bool isUnderwater;     // Behavior switch flag
};

// Aquatic-enhanced senses
float vibrationSensitivity;  // Lateral line detection
float smellRange;            // Excellent underwater smell
```

---

## 9. References

### Academic/Technical
- Reynolds, C. (1987). Flocks, Herds, and Schools: A Distributed Behavioral Model
- Webb, P.W. (1984). Body Form, Locomotion and Foraging in Aquatic Vertebrates
- Sfakiotakis, M. et al. (1999). Review of Fish Swimming Modes for Aquatic Locomotion

### Game Development
- GDC Vault: "Creating the Art of ABZU" (Giant Squid Studios)
- Godot Engine: "Animating Thousands of Fish" tutorial
- Harry Alisavakis: "Fish Shader" blog post

### Online Resources
- Wikipedia: Fish Locomotion, Fish Fin
- COMSOL: "Studying the Swimming Patterns of Fish with Simulation"
- Nature Communications: "Tail Beat Frequency Scaling in Fish"

---

## 10. Implementation Files

| File | Description |
|------|-------------|
| `src/animation/SwimAnimator.h/cpp` | S-wave spine animation |
| `src/entities/SwimBehavior.h/cpp` | Underwater physics |
| `src/entities/CreatureType.h` | Aquatic type definitions |
| `src/entities/Creature.cpp` | Aquatic behavior update |
| `Runtime/Shaders/Creature.hlsl` | Underwater rendering |

---

*Document generated for OrganismEvolution aquatic creature implementation*
*Lines: 400+*
