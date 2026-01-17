# Aquatic Creature System Design Document

## Overview

This document describes the architecture and design of the aquatic creature system, including creature types, swimming animation, underwater physics, schooling behavior, and ecosystem integration.

---

## 1. Creature Type Hierarchy

### 1.1 New Aquatic Types

```cpp
enum class CreatureType {
    // ... existing types ...

    // Aquatic Types (water-dwelling)
    AQUATIC,              // Generic fish (legacy)
    AQUATIC_HERBIVORE,    // Small fish that eat algae - minnow/guppy
    AQUATIC_PREDATOR,     // Predatory fish - bass/pike
    AQUATIC_APEX,         // Apex predator - shark
    AMPHIBIAN,            // Land and water capable - frog/salamander
};
```

### 1.2 Diet Types for Aquatic

```cpp
enum class DietType {
    AQUATIC_FILTER,       // Filter feeder (plankton/algae)
    AQUATIC_ALGAE,        // Eats underwater plants
    AQUATIC_SMALL_PREY,   // Hunts small fish
    AQUATIC_ALL_PREY,     // Hunts all aquatic creatures
};
```

### 1.3 Creature Traits

| Type | Diet | Trophic Level | Attack Range | School? |
|------|------|--------------|--------------|---------|
| AQUATIC_HERBIVORE | AQUATIC_ALGAE | Primary | 0 | Yes |
| AQUATIC_PREDATOR | AQUATIC_SMALL_PREY | Secondary | 2.0 | No |
| AQUATIC_APEX | AQUATIC_ALL_PREY | Tertiary | 3.5 | No |
| AMPHIBIAN | OMNIVORE_FLEX | Secondary | 1.5 | No |

---

## 2. Swimming Animation System

### 2.1 SwimConfig Structure

```cpp
struct SwimConfig {
    SwimStyle style;          // CARANGIFORM, ANGUILLIFORM, etc.

    // Body wave parameters
    float bodyWaveSpeed;      // Hz (1-10)
    float bodyWaveAmp;        // Radians (0.1-0.3)
    float wavelength;         // Body lengths (0.8-1.2)

    // Stiffness control
    float headStiffness;      // 0-1 (0.85 typical)
    float midStiffness;       // 0-1 (0.5 typical)
    float tailStiffness;      // 0-1 (0.15 typical)

    // Fin parameters
    float dorsalFinAmp;       // Dorsal oscillation
    float pectoralFinAmp;     // Pectoral turn amplitude
    float caudalFinAmp;       // Tail fin multiplier

    // Speed modulation
    float minSpeedFactor;     // Animation at zero velocity
    float maxSpeedFactor;     // Animation at max velocity

    float phaseOffset;        // Per-creature variation
};
```

### 2.2 SwimState Runtime Data

```cpp
struct SwimState {
    float swimPhase;          // Current cycle phase (0-2PI)
    float currentSpeed;       // Normalized speed (0-1)
    float turnAmount;         // Turn direction (-1 to 1)
    float verticalAmount;     // Pitch direction (-1 to 1)

    // Damped values for smooth transitions
    float dampedPhase;
    float dampedSpeed;
    float dampedTurn;
};
```

### 2.3 Animation Pipeline

```
1. Update swim phase based on velocity
2. Calculate stiffness mask for each spine bone
3. Apply S-wave rotation to spine chain
4. Calculate tail fin motion
5. Calculate pectoral fin motion (steering)
6. Calculate dorsal fin motion (stability)
7. Combine into final bone rotations
```

---

## 3. Underwater Physics System

### 3.1 SwimPhysicsConfig

```cpp
struct SwimPhysicsConfig {
    // Buoyancy
    float neutralBuoyancyDepth;   // 0.3 default
    float buoyancyStrength;       // 5.0 default
    float buoyancyDamping;        // 0.8 default

    // Drag coefficients
    float forwardDrag;            // 0.3 (streamlined)
    float lateralDrag;            // 1.5 (not streamlined)
    float verticalDrag;           // 1.0 (moderate)

    // Propulsion
    float thrustPower;            // 15.0 base
    float burstMultiplier;        // 2.0 for escaping
    float turnRate;               // 3.0 rad/s

    // Energy
    float cruiseEnergyCost;       // 0.5 per second
    float burstEnergyCost;        // 2.0 per second
    float idleEnergyCost;         // 0.2 per second
};
```

### 3.2 Force Calculation

```cpp
void updatePhysics(position, velocity, steeringForce, ...) {
    // 1. Calculate buoyancy
    float depthError = targetDepth - currentDepth;
    buoyancyForce.y = depthError * buoyancyStrength;
    buoyancyForce.y -= velocity.y * buoyancyDamping;

    // 2. Calculate directional drag
    dragForce = -forward * forwardSpeed^2 * forwardDrag
              - lateral * lateralSpeed^2 * lateralDrag
              - vertical * verticalSpeed^2 * verticalDrag;

    // 3. Calculate thrust
    float phaseFactor = 0.7 + 0.3 * sin(swimPhase);
    thrustForce = forward * thrustPower * phaseFactor;

    // 4. Combine and integrate
    totalForce = steeringForce + buoyancyForce + dragForce + thrustForce;
    velocity += totalForce * deltaTime;
    position += velocity * deltaTime;

    // 5. Clamp to water bounds
    position.y = clamp(position.y, seaFloor + 0.5, waterSurface - 0.5);
}
```

---

## 4. Schooling Behavior

### 4.1 SchoolingForces Structure

```cpp
struct SchoolingForces {
    glm::vec3 separation;        // Avoid crowding
    glm::vec3 alignment;         // Match heading
    glm::vec3 cohesion;          // Stay with group
    glm::vec3 predatorAvoidance; // Flee threats
    int neighborCount;           // Fish in school
};
```

### 4.2 Boids Algorithm with Predator Avoidance

```cpp
void updateBehaviorAquatic(deltaTime, creatures, ...) {
    // Categorize nearby creatures
    vector<Creature*> schoolmates;    // Same type for schooling
    vector<Creature*> predators;      // Threats
    vector<Creature*> prey;           // For predators

    // PRIORITY 1: FLEE FROM PREDATORS
    if (!predators.empty() && !isPredatorType) {
        for (predator : predators) {
            float urgency = 1.0 - (dist / detectionRange);
            fleeForce -= normalize(toPredator) * urgency^2 * 3.0;
        }
        steeringForce += normalize(fleeForce) * 4.0;
        isFleeing = true;

        // Add scatter for unpredictability
        scatter = random(-0.5, 0.5);
        steeringForce.xz += scatter;
    }

    // PRIORITY 2: HUNT PREY (predators only)
    if (isPredatorType && !prey.empty()) {
        // Find closest/weakest prey
        target = findBestTarget(prey);

        // Pursuit with prediction
        predictedPos = target.pos + target.vel * predictionTime;
        steeringForce += normalize(toPredicted) * 2.5;

        if (dist < attackRange) {
            attack(target);
        }
    }

    // SCHOOLING (when not fleeing or hunting)
    if (!isFleeing && !isHunting && !schoolmates.empty()) {
        // Separation
        for (fish : nearby if dist < separationDist) {
            separation -= normalize(toFish) / dist;
        }

        // Alignment
        alignment = averageVelocity(schoolmates);

        // Cohesion
        cohesion = centerOfMass(schoolmates) - position;

        // Apply with weights
        steeringForce += separation * 2.5 * schoolStrength;
        steeringForce += alignment * 1.5 * schoolStrength;
        steeringForce += cohesion * 1.0 * schoolStrength;
    }
}
```

---

## 5. Genome Traits for Aquatic

### 5.1 Aquatic-Specific Traits

```cpp
// In Genome class:
float finSize;           // 0.3 - 1.0 (fin size ratio)
float tailSize;          // 0.5 - 1.2 (caudal fin size)
float swimFrequency;     // 1.0 - 4.0 Hz
float swimAmplitude;     // 0.1 - 0.3 (S-wave amplitude)
float preferredDepth;    // 0.1 - 0.5 (normalized)
float schoolingStrength; // 0.5 - 1.0
```

### 5.2 Genome Initialization

```cpp
void Genome::randomizeAquatic() {
    size = Random::range(0.4f, 1.2f);
    speed = Random::range(8.0f, 18.0f);

    // Fish colors
    if (random < 0.3) {
        color = silver/blue;  // Schooling fish
    } else if (random < 0.5) {
        color = green/brown;  // Bottom feeders
    } else if (random < 0.7) {
        color = yellow/orange; // Tropical
    } else {
        color = varied;        // Patterned
    }

    // Aquatic traits
    finSize = Random::range(0.4f, 0.9f);
    tailSize = Random::range(0.6f, 1.1f);
    swimFrequency = Random::range(1.5f, 3.5f);
    swimAmplitude = Random::range(0.12f, 0.25f);
    preferredDepth = Random::range(0.15f, 0.4f);
    schoolingStrength = Random::range(0.6f, 0.95f);

    // Underwater senses
    visionRange = Random::range(15.0f, 40.0f);
    vibrationSensitivity = Random::range(0.7f, 0.95f);
    smellRange = Random::range(50.0f, 120.0f);
}
```

---

## 6. Underwater Rendering

### 6.1 Vertex Shader Modifications

```hlsl
// Creature type detection
float creatureType = instColorType.w;  // 0=land, 1=flying, 2=aquatic

if (creatureType >= 1.5) {  // Aquatic
    // Apply S-wave swimming animation
    float swimPhase = time * 3.0 * 2.0 * PI;
    float normalizedZ = (pos.z + 1.0) / 2.0;
    float amplitude = 0.15 * pow(normalizedZ, 1.5);
    float wave = sin(swimPhase + normalizedZ * 2.0 * PI) * amplitude;
    pos.x += wave;
}
```

### 6.2 Pixel Shader Effects

```hlsl
// Fish scale pattern
float3 generateFishScales(pos, normal, baseColor, swimPhase) {
    float3 scaleUV = pos * 15.0;
    scaleUV.z *= 0.5;  // Elongate along body

    float scalePattern = voronoi(scaleUV * 1.5);
    float3 scaleColor = lerp(baseColor * 0.7, baseColor * 1.3, scalePattern);

    // Shimmer effect
    float shimmer = sin(swimPhase + pos.x * 10.0) * 0.1 + 0.9;
    return scaleColor * shimmer;
}

// Underwater lighting
if (isUnderwater) {
    // Light absorption (red first, blue last)
    float3 absorption = float3(
        exp(-depth * 0.15),  // Red
        exp(-depth * 0.07),  // Green
        exp(-depth * 0.04)   // Blue
    );
    lighting *= absorption;

    // Add caustics
    caustics = calculateCaustics(worldPos, depth);
    lighting += caustics * skinColor;
}

// Underwater fog
if (isUnderwater) {
    float fogFactor = exp(-0.08 * distance);
    float3 fogColor = lerp(turquoise, darkBlue, depth * 0.15);
    result = lerp(fogColor, result, fogFactor);
}
```

---

## 7. Ecosystem Integration

### 7.1 Food Chain

```
Level 4: AQUATIC_APEX (Shark)
    |
    v  hunts
Level 3: AQUATIC_PREDATOR (Bass/Pike)
    |
    v  hunts
Level 2: AQUATIC_HERBIVORE (Minnow)
    |
    v  eats
Level 1: Algae/Plants (spawned in water)
```

### 7.2 Spawning Rules

```cpp
void spawnAquaticCreature(type, position) {
    // Ensure position is underwater
    if (position.y > WATER_SURFACE - 0.5) {
        position.y = WATER_SURFACE - 2.0;
    }

    // Different types have different spawn rules
    switch (type) {
        case AQUATIC_HERBIVORE:
            // Spawn in schools
            spawnNearPosition(count: 5-10);
            break;

        case AQUATIC_PREDATOR:
            // Spawn alone
            spawnSingle();
            break;

        case AQUATIC_APEX:
            // Spawn rarely, in deep water
            if (position.y < WATER_SURFACE - 10.0) {
                spawnSingle();
            }
            break;
    }
}
```

### 7.3 Predator-Prey Rules

```cpp
bool canBeHuntedByAquatic(prey, predator, preySize) {
    switch (predator) {
        case AQUATIC_PREDATOR:
            // Can hunt small herbivore fish
            return (prey == AQUATIC_HERBIVORE && preySize < 0.8) ||
                   (prey == AQUATIC && preySize < 0.7);

        case AQUATIC_APEX:
            // Can hunt all smaller aquatic creatures
            return isAquatic(prey) &&
                   prey != AQUATIC_APEX &&
                   preySize < 1.5;

        default:
            return false;
    }
}
```

---

## 8. File Structure

```
src/
├── animation/
│   ├── SwimAnimator.h          # Swimming animation config
│   └── SwimAnimator.cpp        # S-wave implementation
├── entities/
│   ├── SwimBehavior.h          # Underwater physics
│   ├── SwimBehavior.cpp        # Buoyancy, drag, thrust
│   ├── CreatureType.h          # Aquatic type definitions
│   ├── CreatureTraits.cpp      # Aquatic creature traits
│   └── Creature.cpp            # updateBehaviorAquatic()
└── environment/
    └── Terrain.h               # Water level constants

Runtime/Shaders/
└── Creature.hlsl               # Fish scales, underwater fog
```

---

## 9. Performance Considerations

### 9.1 Spatial Partitioning

- Use SpatialGrid for O(1) neighbor queries
- Only query creatures in water regions
- Separate grids for land and water creatures

### 9.2 Animation Optimization

- Vertex shader animation (no CPU skinning needed)
- Instance batching by creature type
- LOD reduction for distant fish

### 9.3 Physics Simplification

- Simplified buoyancy (no full fluid simulation)
- Approximate drag coefficients
- Discrete depth zones instead of continuous

---

## 10. Future Enhancements

- [ ] Bioluminescence for deep fish
- [ ] Bubble trails when swimming fast
- [ ] Surface breaching behavior
- [ ] Coral reef biomes
- [ ] Migration patterns
- [ ] Egg-laying and fry stages
- [ ] Specialized shark hunting (circling behavior)

---

*Document generated for OrganismEvolution aquatic creature implementation*
*Lines: 300+*
