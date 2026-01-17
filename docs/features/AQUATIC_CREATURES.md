# Aquatic Creatures (Fish) Documentation

## Overview

Aquatic creatures are a new creature type added to the evolution simulator. They represent fish-like organisms that live exclusively in water areas, exhibiting realistic swimming behavior with S-wave motion and strong schooling tendencies.

### Key Features
- **Water-Only Habitat**: Fish spawn and live only in water areas (terrain below water level)
- **S-Wave Swimming**: Realistic undulating body motion in vertex shader
- **Schooling Behavior**: Boids algorithm implementation for natural flocking
- **Iridescent Shimmer**: View-dependent color shifting simulating fish scales
- **Depth Control**: Fish maintain preferred depths and avoid the surface

---

## Genome Traits

Aquatic creatures have specialized genome traits defined in `Genome.h`:

| Trait | Range | Description |
|-------|-------|-------------|
| `finSize` | 0.3 - 1.0 | Size of dorsal and pectoral fins |
| `tailSize` | 0.5 - 1.2 | Caudal fin size (affects propulsion) |
| `swimFrequency` | 1.0 - 4.0 Hz | Body wave frequency during swimming |
| `swimAmplitude` | 0.1 - 0.3 | S-wave amplitude (how much the body bends) |
| `preferredDepth` | 0.1 - 0.5 | Normalized depth below water surface |
| `schoolingStrength` | 0.5 - 1.0 | How strongly the fish schools with others |

### Aquatic Genome Initialization

Fish genomes are randomized with these characteristic values:
```cpp
genome.speed = Random::range(12.0f, 22.0f);      // Fast swimmers
genome.visionRange = Random::range(20.0f, 40.0f);
genome.size = Random::range(0.4f, 0.9f);         // Smaller than land creatures
```

Color variations include:
- Silver/white (30%) - `(0.7-0.9, 0.7-0.9, 0.8-1.0)`
- Blue (20%) - `(0.1-0.3, 0.3-0.6, 0.6-0.9)`
- Orange/gold (20%) - `(0.8-1.0, 0.4-0.7, 0.1-0.3)`
- Green/teal (30%) - `(0.2-0.4, 0.5-0.8, 0.4-0.6)`

---

## Mesh Generation

Fish bodies are generated using the metaball system in `CreatureBuilder.cpp`. The `buildAquatic()` function creates a streamlined body with:

### Body Structure
- **Main Body**: 3 overlapping metaballs forming a torpedo shape
- **Head**: Pointed, streamlined with snout
- **Tail Section**: Narrows toward the caudal fin

### Fins
| Fin Type | Description |
|----------|-------------|
| Caudal (Tail) | Forked tail fin for propulsion |
| Dorsal | Top fin for stability |
| Pectoral | Side fins for steering |
| Pelvic | Lower side fins |
| Anal | Bottom rear fin |

### Additional Features
- **Eyes**: Side-facing (typical prey position)
- **Gill Covers**: Slight operculum bulge

---

## Swimming Animation (Vertex Shader)

The S-wave swimming animation is implemented in `creature_vertex.glsl`:

### Algorithm
1. **Wave Influence**: Increases from head (0) to tail (1)
2. **Phase Propagation**: Wave travels backward from head to tail
3. **Lateral Displacement**: Side-to-side movement on Z-axis
4. **Vertical Undulation**: Subtle Y-axis movement

### Key Parameters
```glsl
float waveInfluence = clamp(-bodyPos * 2.5 + 0.2, 0.0, 1.0);
float swimPhase = time * animSpeed * 2.5;
float waveAngle = swimPhase - bodyPos * 5.0;
float lateralWave = sin(waveAngle) * 0.12 * waveInfluence;
```

### Additional Animations
- **Tail Flutter**: Enhanced movement at tail tip
- **Pectoral Fin Flapping**: Side fins move with swimming
- **Dorsal Fin Ripple**: Top fin undulates with motion

---

## Depth Control

Fish maintain their position in the water column:

### Water Level
```cpp
const float WATER_LEVEL = -5.0f;  // Terrain height below this is water
```

### Depth Maintenance
```cpp
float targetY = WATER_LEVEL - genome.preferredDepth * 15.0f;
float depthError = targetY - position.y;
steeringForce.y += depthError * 5.0f;
```

### Surface Avoidance
Fish are pushed down when too close to the surface:
```cpp
if (position.y > WATER_LEVEL - 1.0f) {
    steeringForce.y -= 15.0f;
}
```

---

## Schooling Behavior

Fish use the Boids algorithm for realistic flocking:

### Three Rules
1. **Separation**: Avoid crowding neighbors
   - Weight: 2.5 * schoolingStrength
2. **Alignment**: Steer toward average heading
   - Weight: 1.5 * schoolingStrength
3. **Cohesion**: Move toward center of mass
   - Weight: 1.0 * schoolingStrength

### Implementation
```cpp
// Separation
if (dist < separationDist) {
    separation -= glm::normalize(toFish) / dist;
}

// Alignment
alignment += fish->getVelocity();

// Cohesion
centerOfMass += fish->getPosition();
cohesion = centerOfMass - position;
```

---

## Visual Effects (Fragment Shader)

### Iridescent Shimmer

Fish have view-dependent color shifting in `creature_fragment.glsl`:

```glsl
// Detect aquatic creatures by blue-ish coloring
float aquaticFactor = max(0.0, skinColor.b - max(skinColor.r, skinColor.g) * 0.5);

// Time-based shimmer
float shimmer = sin(shimmerPhase + time * 3.0) * 0.5 + 0.5;

// View-angle iridescence
float viewAngle = 1.0 - max(dot(viewDir, norm), 0.0);
float iridescence = pow(viewAngle, 2.0);

// Blue-green color shift
vec3 shimmerColor = vec3(0.1, 0.25, 0.35) * shimmer * iridescence;
```

### Scale-Like Specular
```glsl
float scalePattern = voronoi(TriplanarUV * 15.0);
float scaleSpec = pow(max(dot(reflect(-lightDir, norm), viewDir), 0.0), 48.0);
scaleSpec *= (1.0 - scalePattern) * 0.8;
```

---

## Spawning System

### Population Parameters
```cpp
static constexpr int initialAquatic = 25;
static constexpr int minAquatic = 15;
static constexpr int maxAquatic = 60;
```

### Water Area Detection
Fish only spawn in valid water areas:
```cpp
float terrainHeight = terrain->getHeight(position.x, position.z);
if (terrainHeight < WATER_LEVEL - 3.0f) {
    foundWater = true;
    position.y = WATER_LEVEL - genome.preferredDepth * 12.0f;
}
```

### Population Balance
When aquatic population drops below minimum:
```cpp
if (aquaticCount < minAquatic) {
    spawnAquaticCreatures();
}
```

---

## Food System

Aquatic creatures use the `AQUATIC_FILTER` diet type, representing filter feeders that consume plankton and algae in the water.

---

## Predator Interactions

### Being Hunted
Aquatic creatures can be hunted by:
- **Apex Predators**: When fish are near the water surface

```cpp
case CreatureType::APEX_PREDATOR:
    return isHerbivore(prey) || prey == CreatureType::SMALL_PREDATOR || prey == CreatureType::AQUATIC;
```

### Defense Mechanisms
- Staying at depth (away from surface predators)
- Schooling (safety in numbers)
- Fast swimming speed

---

## Files Modified

| File | Changes |
|------|---------|
| `src/entities/CreatureType.h` | Added AQUATIC type, AQUATIC_FILTER diet, isAquatic() helper |
| `src/entities/Genome.h` | Added aquatic traits (6 new parameters) |
| `src/entities/Genome.cpp` | Aquatic trait initialization, mutation, crossover |
| `src/entities/Creature.h` | Added updateBehaviorAquatic() declaration |
| `src/entities/Creature.cpp` | Implemented aquatic behavior with schooling |
| `src/graphics/procedural/CreatureBuilder.h` | Added FISH body plan, buildAquatic() declaration |
| `src/graphics/procedural/CreatureBuilder.cpp` | Implemented fish mesh generation |
| `src/core/Simulation.h` | Added aquatic population constants and methods |
| `src/core/Simulation.cpp` | Implemented spawning and population management |
| `shaders/creature_vertex.glsl` | Added S-wave swimming animation |
| `shaders/creature_fragment.glsl` | Added iridescent shimmer effect |

---

## Success Criteria Checklist

- [x] 25 fish visible swimming in water
- [x] S-wave swimming animation visible
- [x] Fish school together (Boids algorithm)
- [x] Fish avoid beaches and surface
- [x] Iridescent shimmer effect
- [x] Full documentation (this file)
