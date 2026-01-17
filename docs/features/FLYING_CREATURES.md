# Flying Creatures - Implementation Documentation

## Overview

Flying creatures are a new creature type that adds aerial behavior to the OrganismEvolution simulation. They are bird-like omnivores that can hunt small herbivores from above and forage for plant-based food. They fly above the terrain at their preferred altitude, exhibit realistic wing flapping animation, and display unique aerial behaviors like circling, diving, and soaring.

### Key Features
- **Aerial locomotion**: Maintain altitude above terrain with smooth flight physics
- **Animated wings**: Vertex shader-based wing flapping with configurable frequency
- **Omnivore diet**: Can eat plant food and hunt small frugivores
- **Circling behavior**: Circle above prey before diving to attack
- **Soaring/gliding**: Energy-efficient flight when not actively hunting
- **Flocking**: Light flocking behavior with other flying creatures

## Genome Traits

Flying creatures have specialized genome traits that control their flight characteristics:

```cpp
// Flying-specific traits in Genome
float wingSpan;           // 0.5 - 2.0 (ratio to body size)
float flapFrequency;      // 2.0 - 8.0 Hz
float glideRatio;         // 0.3 - 0.8 (higher = more gliding vs flapping)
float preferredAltitude;  // 15.0 - 40.0 (height above terrain)
```

### Trait Descriptions

| Trait | Range | Description |
|-------|-------|-------------|
| `wingSpan` | 0.5-2.0 | Ratio of wing length to body size. Larger wings provide more lift but require more energy. |
| `flapFrequency` | 2.0-8.0 Hz | How fast the wings beat. Higher frequency = more energy consumption but faster acceleration. |
| `glideRatio` | 0.3-0.8 | Balance between flapping and gliding. Higher values = more energy efficient but slower. |
| `preferredAltitude` | 15.0-40.0 | Target height above terrain. Higher altitude = better visibility but more energy to maintain. |

### Random Initialization

Flying creatures are initialized with the `randomizeFlying()` method:

```cpp
void Genome::randomizeFlying() {
    // Base randomization
    randomize();

    // Flying-specific overrides
    size = Random::range(0.4f, 0.8f);           // Smaller than land creatures
    speed = Random::range(15.0f, 25.0f);        // Faster in air
    visionRange = Random::range(40.0f, 60.0f);  // Better vision from above

    // Distinctive colors (blues, grays, whites)
    color = glm::vec3(
        Random::range(0.3f, 0.6f),
        Random::range(0.4f, 0.7f),
        Random::range(0.6f, 0.9f)
    );

    // Flying traits
    wingSpan = Random::range(0.8f, 1.5f);
    flapFrequency = Random::range(3.0f, 6.0f);
    glideRatio = Random::range(0.4f, 0.7f);
    preferredAltitude = Random::range(20.0f, 35.0f);
}
```

## Mesh Generation

Flying creatures use a specialized metaball configuration to create a bird-like body shape.

### Body Structure

The `buildFlying()` method in `CreatureBuilder` creates the following metaball configuration:

```
Body Components:
├── Main body (streamlined ellipsoid)
├── Head (small, aerodynamic)
│   ├── Eyes (forward-facing for depth perception)
│   └── Beak (pointed)
├── Wings (extend in Z direction)
│   ├── Wing root (attached to body)
│   ├── Wing mid-section
│   ├── Wing outer section
│   └── Wing tip
│   └── Wing membrane (flatter metaballs)
├── Tail section
│   ├── Tail base
│   └── Tail feathers (fan shape)
└── Legs (short, tucked)
    └── Talons
```

### Metaball Positions

Key positions (normalized to body scale):

| Component | Position (X, Y, Z) | Radius | Strength |
|-----------|-------------------|--------|----------|
| Main body | (0, 0, 0) | 0.4 | 1.0 |
| Head | (0.3, 0.05, 0) | 0.25 | 0.8 |
| Beak | (0.45, 0, 0) | 0.08 | 0.4 |
| Right wing root | (0, 0.02, wingExtent*0.25) | 0.12 | 0.5 |
| Right wing tip | (-0.12, 0.05, wingExtent*0.95) | 0.04 | 0.3 |

## Wing Animation - Shader Math

The wing animation is implemented in the vertex shader (`creature_vertex.glsl`). The animation uses the Z-coordinate of each vertex to determine wing influence.

### Wing Detection

```glsl
// Detect wing vertices by Z position
float wingThreshold = 0.15;
float absZ = abs(animatedPos.z);

if (absZ > wingThreshold) {
    // This vertex is part of a wing
    float wingInfluence = clamp((absZ - wingThreshold) / 0.6, 0.0, 1.0);
    float wingSign = sign(animatedPos.z);  // -1 = left, +1 = right
}
```

### Flap Cycle Mathematics

The wing flapping follows a sinusoidal motion:

```glsl
// Flap frequency from genome (default 4.5 Hz)
float flapHz = max(flapFrequency, 4.5);

// Convert to radians per second
float flapPhase = time * flapHz * 6.28318;

// Maximum flap angle (~45 degrees)
float maxFlapAngle = 0.8;  // radians

// Current flap angle (sinusoidal)
float flapAngle = sin(flapPhase) * maxFlapAngle * wingInfluence;
```

### Wing Rotation

Wings rotate around the X-axis (body axis):

```glsl
// Apply rotation
float cosA = cos(flapAngle);
float sinA = sin(flapAngle) * wingSign;

// Rotate Y component (vertical motion)
float newY = animatedPos.y * cosA - abs(animatedPos.z) * sinA * 0.3;

// Blend based on wing influence
animatedPos.y = mix(animatedPos.y, newY, wingInfluence * 0.8);
```

### Additional Animation Effects

1. **Forward sweep**: Wings move slightly forward/backward during flap
   ```glsl
   float sweepAmount = sin(flapPhase) * 0.05 * wingInfluence;
   animatedPos.x += sweepAmount;
   ```

2. **Membrane flex**: Trailing edge lags behind leading edge
   ```glsl
   if (animatedPos.x < -0.05) {
       float membraneDelay = (animatedPos.x + 0.05) * 2.0;
       float delayedFlap = sin(flapPhase + membraneDelay) * 0.1;
       animatedPos.y += delayedFlap * wingInfluence;
   }
   ```

3. **Body bob**: Small vertical motion from wing beats
   ```glsl
   float bodyBob = sin(time * flapHz * 6.28318) * 0.02;
   animatedPos.y += bodyBob * (1.0 - clamp(abs(animatedPos.z) / 0.3, 0.0, 1.0));
   ```

4. **Gliding bank**: Subtle body tilt during soaring
   ```glsl
   float bankAngle = sin(time * 0.5) * 0.03;
   ```

## Flight Physics

Flying creatures use specialized physics that differ from land creatures.

### Altitude Maintenance

```cpp
void Creature::updateBehaviorFlying(...) {
    // Get terrain height at current position
    float terrainHeight = terrain.getHeight(position.x, position.z);
    float targetAltitude = terrainHeight + genome.preferredAltitude;

    // Smooth altitude adjustment
    float altitudeError = targetAltitude - position.y;
    steeringForce.y += altitudeError * 2.0f;
}
```

### Ground Avoidance (Critical!)

```cpp
// Emergency pull-up when too close to ground
if (position.y < terrainHeight + 10.0f) {
    float emergencyFactor = 1.0f - (position.y - terrainHeight) / 10.0f;
    emergencyFactor = clamp(emergencyFactor, 0.0f, 1.0f);
    steeringForce.y += 20.0f * emergencyFactor;
}

// Extra strong pull-up over water
if (terrain.isWater(position.x, position.z) && position.y < 15.0f) {
    steeringForce.y += 30.0f;
}
```

### Physics Update

```cpp
void Creature::updateFlyingPhysics(float deltaTime, const Terrain& terrain) {
    // Apply velocity damping (air resistance)
    velocity.y *= 0.98f;

    // Enforce altitude constraints
    float minAltitude = terrainHeight + 8.0f;
    if (position.y < minAltitude) {
        position.y = minAltitude;
        velocity.y = std::max(velocity.y, 2.0f);  // Bounce up
    }

    // Maximum altitude
    float maxAltitude = terrainHeight + genome.preferredAltitude * 2.0f;
    if (position.y > maxAltitude) {
        position.y = maxAltitude;
        velocity.y = std::min(velocity.y, 0.0f);
    }
}
```

## Behavior AI

Flying creatures exhibit complex aerial behaviors.

### Hunting Behavior

1. **Detection**: Scan for small frugivores within vision range
2. **Circling**: Circle above prey at ~12 unit radius
3. **Diving**: When close enough, dive down to attack
4. **Attack**: Deal damage when within attack range

```cpp
// Circle above prey
if (preyDist < 20.0f) {
    float circleAngle = atan2(position.z - preyPos.z, position.x - preyPos.x);
    circleAngle += 0.8f * deltaTime;  // Rotate around prey

    glm::vec3 circleTarget;
    circleTarget.x = preyPos.x + cos(circleAngle) * 12.0f;
    circleTarget.z = preyPos.z + sin(circleAngle) * 12.0f;
    circleTarget.y = preyPos.y + preferredAltitude * 0.5f;
}

// Dive to attack
if (preyDist < 5.0f && position.y > preyPos.y - 2.0f) {
    steeringForce += normalize(diveTarget - position) * 3.0f;
    steeringForce.y -= 15.0f;  // Strong downward force
}
```

### Foraging Behavior

Flying creatures can also eat plant-based food:

```cpp
// Circle above food before landing
if (foodDist < 15.0f) {
    float circleAngle = atan2(position.z - foodPos.z, position.x - foodPos.x);
    circleAngle += 0.5f * deltaTime;
    // ... similar to hunting circle
}

// Descend to eat
if (foodDist < 3.0f && position.y < foodPos.y + 5.0f) {
    steeringForce += arrive(position, velocity, foodPos) * 2.0f;
    steeringForce.y -= 5.0f;
}
```

### Soaring Behavior

When no immediate goals, flying creatures enter energy-efficient soaring mode:

```cpp
// Soaring influenced by glideRatio
float soarInfluence = genome.glideRatio;

// Wander with reduced activity
glm::vec3 wanderForce = steering.wander(...);
steeringForce += wanderForce * (1.0f - soarInfluence * 0.5f);

// Lazy circles
float soarPhase = currentTime * 0.3f;
steeringForce.x += sin(soarPhase) * soarInfluence * 3.0f;
steeringForce.z += cos(soarPhase) * soarInfluence * 3.0f;
```

### Flocking

Flying creatures exhibit light flocking behavior:

```cpp
std::vector<Creature*> flyingNeighbors = getNeighborsOfType(
    otherCreatures, CreatureType::FLYING, genome.visionRange * 0.5f);

if (!flyingNeighbors.empty()) {
    glm::vec3 flockForce = steering.flock(position, velocity, flyingNeighbors,
        2.0f,   // Separation (avoid collision)
        0.5f,   // Alignment (match direction)
        0.3f    // Cohesion (stay together)
    );
    steeringForce += flockForce * 0.4f;  // Light influence
}
```

## Integration

### Spawning

Flying creatures are spawned in `Simulation::spawnFlyingCreatures()`:

```cpp
void Simulation::spawnFlyingCreatures() {
    for (int i = 0; i < initialFlying; i++) {
        Genome genome;
        genome.randomizeFlying();

        // Find land position to spawn above
        float terrainHeight = terrain->getHeight(x, z);
        while (terrainHeight < 0.0f) { /* retry */ }

        // Spawn at preferred altitude
        position.y = terrainHeight + genome.preferredAltitude;

        // Initial velocity in random direction
        float angle = Random::range(0.0f, 6.28318f);
        glm::vec3 initialVelocity(cos(angle) * speed * 0.5f, 0, sin(angle) * speed * 0.5f);

        creatures.push_back(make_unique<Creature>(position, genome, CreatureType::FLYING));
    }
}
```

### Population Parameters

```cpp
static constexpr int initialFlying = 15;
static constexpr int minFlying = 8;
static constexpr int maxFlying = 40;
```

### Rendering

Flying creatures are rendered through the standard `CreatureRenderer` pipeline:

1. Meshes are cached by `CreatureMeshCache` with key `(FLYING, sizeCategory, speedCategory)`
2. Instance batches are grouped by mesh key
3. Before each batch, `creatureType` uniform is set to `3` (FLYING)
4. Vertex shader applies wing animation when `creatureType == 3`

### Reproduction

Flying creatures reproduce as omnivores:

```cpp
// Reproduction thresholds
static constexpr float flyingReproductionThreshold = 160.0f;
static constexpr float flyingReproductionCost = 70.0f;
static constexpr int minKillsToReproduceFlying = 1;

bool Creature::canReproduce() const {
    if (type == CreatureType::FLYING) {
        return energy > flyingReproductionThreshold &&
               killCount >= minKillsToReproduceFlying;
    }
}
```

## Tuning Guide

### Making Flight More Energetic

Increase `flapFrequency` range in `Genome::randomizeFlying()`:
```cpp
flapFrequency = Random::range(4.0f, 7.0f);  // Faster flapping
```

### Adjusting Altitude Range

Modify `preferredAltitude` range:
```cpp
preferredAltitude = Random::range(25.0f, 50.0f);  // Higher flight
```

### More Aggressive Hunting

In `updateBehaviorFlying()`, increase hunting priority:
```cpp
bool shouldHunt = (energy < 100.0f && nearestPrey != nullptr);  // Hunt more often
```

### Slower, Gliding Birds

Increase `glideRatio` range:
```cpp
glideRatio = Random::range(0.6f, 0.9f);  // More gliding
```

### Wing Animation Speed

Adjust flap phase multiplier in shader:
```glsl
float flapPhase = time * flapHz * 8.0;  // 8.0 instead of 6.28 for faster animation
```

## Future Ideas

1. **Predator Birds**: Large flying carnivores that hunt other flying creatures
2. **Migration**: Seasonal movement patterns across the terrain
3. **Nesting**: Flying creatures create nests and protect eggs
4. **Thermal Soaring**: Use rising air columns for energy-efficient flight
5. **Diving Attacks**: Increase attack damage when diving from height
6. **Night/Day Behavior**: Different flight patterns based on time of day
7. **Weather Effects**: Wind affects flight speed and energy consumption
8. **Territorial Behavior**: Defend airspace around food sources
9. **Mating Displays**: Aerial acrobatics to attract mates
10. **Fledgling Stage**: Young birds that can't fly initially

## Files Modified

| File | Changes |
|------|---------|
| `CreatureType.h` | Added `FLYING` to enum, updated helper functions |
| `Genome.h/cpp` | Added flying traits, `randomizeFlying()` method |
| `CreatureBuilder.h/cpp` | Added `buildFlying()` method for bird-like mesh |
| `creature_vertex.glsl` | Added wing animation for `creatureType == 3` |
| `Creature.h/cpp` | Added `updateBehaviorFlying()`, `updateFlyingPhysics()`, reproduction settings |
| `Simulation.h/cpp` | Added spawning, population management for flying creatures |
| `CreatureRenderer.cpp` | Added creature type uniform setting for shader |
| `CreatureMeshCache.cpp` | Added preloading of flying archetypes |

## Success Criteria Checklist

- [x] 15 flying creatures visible in sky at simulation start
- [x] Wings visibly flap with configurable frequency
- [x] Creatures maintain altitude above terrain
- [x] Creatures circle and dive to eat/hunt
- [x] Full documentation with shader math explained
