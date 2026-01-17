# Comprehensive Evolution Simulator TODO Documentation

## Current Codebase Summary (Research Completed)

This document outlines all planned features, improvements, and implementation strategies for creating a high-quality, YouTube-worthy 3D evolution simulation.

---

## CURRENT ARCHITECTURE OVERVIEW

### What Already Exists (Working Systems)

#### 1. Core Simulation Loop
- **Files**: `src/main.cpp`, `src/core/Simulation.cpp/h`
- OpenGL 3.3+ graphics with GLFW
- Main loop: `Simulation::update()` and `Simulation::render()`
- ~102 source files organized into logical subsystems

#### 2. Creature System (Sophisticated)
- **Files**: `src/entities/Creature.cpp/h`, `src/entities/Genome.cpp/h`
- Dual genome system: Legacy `Genome` + advanced `genetics::DiploidGenome`
- Herbivore vs Carnivore behaviors
- Predator-prey combat with damage, cooldowns, kill counts
- Fear/behavioral states
- 50+ evolvable genetic parameters

#### 3. Advanced Genetics System
- **Location**: `src/entities/genetics/` (16 files)
- Diploid genomes with paired chromosomes
- Sexual reproduction with meiosis
- Species tracking and speciation support
- Hybrid sterility and fitness effects
- Mate preferences and sexual selection
- Epigenetic marks (environmental stress response)

#### 4. Procedural Creature Generation Pipeline
```
MorphologyGenes → CreatureBuilder → MetaballSystem → MarchingCubes → Mesh
                                                          ↓
                                               CreatureMeshCache (LRU)
                                                          ↓
                                               CreatureRenderer (instanced)
```

- **MorphologyGenes** (`physics/Morphology.h`): 30+ body parameters
- **CreatureBuilder** (`graphics/procedural/CreatureBuilder.cpp`): 9 body archetypes
- **MetaballSystem**: Implicit surfaces with analytical gradients (6x faster!)
- **MarchingCubes**: Complete 256-entry lookup table
- **CreatureRenderer**: Instanced rendering for 100+ creatures

#### 5. Physics & Locomotion
- **Files**: `src/physics/Locomotion.cpp/h`
- Central Pattern Generator (CPG) for rhythmic gaits
- 9 gait types: walk, trot, gallop, crawl, slither, swim, fly, hop, tripod
- Inverse Kinematics (FABRIK algorithm)
- Joint constraints and energy expenditure

#### 6. Sensory System (Extremely Comprehensive)
- **Files**: `src/entities/SensorySystem.cpp/h` (~38KB)
- Vision, Hearing, Smell, Touch, Electroreception
- Probability-based detection with confidence
- Pheromone grid system
- Spatial memory with decay
- Sound propagation

#### 7. Neural Networks / AI
- **Location**: `src/ai/` (9 files)
- NEAT (NeuroEvolution of Augmenting Topologies)
- Modular brain architecture (SensoryProcessor, EmotionalModule)
- Neuromodulators (serotonin, norepinephrine, dopamine)

#### 8. Environment Systems
- **Terrain**: Perlin noise heightmap, 300x300 grid, 5 biomes
- **VegetationManager**: Trees, bushes, grass (exists but not rendering)
- **TreeGenerator**: L-system procedural trees
- **EcosystemManager**: Producer, Decomposer, Season systems
- **SeasonManager**: Day/night, seasonal changes, temperature

#### 9. Current Shaders
- `creature_vertex.glsl`: Triplanar UV mapping
- `creature_fragment.glsl`: 3D Perlin noise, Voronoi, procedural scales
- `fragment.glsl`: 5 biome-based terrain textures
- `vertex.glsl`: Perlin noise heightmap

---

## KNOWN ISSUES TO FIX

| Issue | Priority | Notes |
|-------|----------|-------|
| Camera scroll wheel offset bug | High | Known in STATUS.md |
| VegetationManager not rendering | High | Code exists, not integrated |
| Locomotion animation not visible | Medium | Physics built, not animated |
| NEAT may not be wired to creatures | Medium | Check integration |
| Metamorphosis system dormant | Low | Exists but unused |

---

## MAJOR FEATURE CATEGORIES

### Category 1: Procedural Animation System
### Category 2: Procedural Textures (Math-Based)
### Category 3: Creature Types (Flying, Swimming, etc.)
### Category 4: Nametag & UI System
### Category 5: Camera & Cinematic Systems
### Category 6: Environment Improvements
### Category 7: Player Interaction / God-Mode
### Category 8: YouTube-Ready Features
### Category 9: Performance & Polish

---

## CATEGORY 1: PROCEDURAL ANIMATION SYSTEM

### 1.1 Skeletal Animation Foundation
**Goal**: Make creatures visually animated with moving limbs

**Implementation Steps**:
1. Create `Skeleton` struct with bone hierarchy
2. Map metaball positions to bone positions
3. Apply bone transforms during mesh generation
4. Update bones each frame based on locomotion state

**Key Data Structures**:
```cpp
struct Bone {
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
    int parentIndex;
    std::string name;
};

struct Skeleton {
    std::vector<Bone> bones;
    std::vector<glm::mat4> boneMatrices;
    void updateBoneMatrices();
};

struct AnimationState {
    float phase;        // 0-1 cycle position
    float speed;        // animation speed multiplier
    GaitType gait;      // current gait
    float blend;        // blend between states
};
```

### 1.2 Leg/Limb Animation (Walking)
**Algorithm**: Use existing CPG (Central Pattern Generator) output

```cpp
// Pseudocode for leg animation
void animateLegs(Creature& c, float dt) {
    // Get gait pattern from CPG
    GaitPattern pattern = CPG::getPattern(c.currentGait);

    // For each leg
    for (int i = 0; i < c.numLegs; i++) {
        float phase = c.walkPhase + pattern.phaseOffsets[i];

        // Leg lifted during swing phase
        if (phase < pattern.dutyFactor) {
            // Stance phase - foot on ground
            c.legs[i].footTarget = groundPosition;
        } else {
            // Swing phase - foot lifting
            float swingPhase = (phase - pattern.dutyFactor) / (1.0 - pattern.dutyFactor);
            float height = sin(swingPhase * PI) * c.stepHeight;
            c.legs[i].footTarget = nextFootPosition + vec3(0, height, 0);
        }

        // Solve IK to position leg bones
        solveIK(c.legs[i]);
    }
}
```

### 1.3 Flying Creature Wing Animation
**Algorithm**: Sinusoidal wing flapping with figure-8 pattern

```cpp
void animateWings(Creature& c, float dt) {
    c.wingPhase += dt * c.flapFrequency;

    // Main flap angle
    float flapAngle = sin(c.wingPhase * 2 * PI) * c.flapAmplitude;

    // Forward/back sweep (figure-8)
    float sweepAngle = sin(c.wingPhase * 4 * PI) * c.sweepAmplitude;

    // Twist along wing span
    float twistAngle = cos(c.wingPhase * 2 * PI) * c.twistAmount;

    // Apply to wing bones
    for (int i = 0; i < c.wingBones.size(); i++) {
        float t = i / (float)c.wingBones.size(); // 0-1 along wing
        c.wingBones[i].rotation = quat(
            flapAngle * (1.0 - t * 0.3),  // Less flap at tip
            sweepAngle * t,                // More sweep at tip
            twistAngle * t                 // More twist at tip
        );
    }
}
```

### 1.4 Swimming/Fish Animation
**Algorithm**: Sinusoidal body wave propagation

```cpp
void animateSwimming(Creature& c, float dt) {
    c.swimPhase += dt * c.swimFrequency;

    // Wave travels from head to tail
    for (int i = 0; i < c.spineSegments.size(); i++) {
        float t = i / (float)c.spineSegments.size();

        // Phase offset increases along body
        float localPhase = c.swimPhase - t * c.wavelength;

        // Amplitude increases toward tail
        float amplitude = c.swimAmplitude * (0.2 + 0.8 * t);

        // Lateral displacement
        float displacement = sin(localPhase * 2 * PI) * amplitude;

        c.spineSegments[i].localPosition.x = displacement;
    }

    // Tail fin extra movement
    c.tailFin.rotation = sin(c.swimPhase * 2 * PI) * c.tailAmplitude;
}
```

### 1.5 Idle Animations
**Goal**: Creatures look alive even when stationary

```cpp
void animateIdle(Creature& c, float dt) {
    // Breathing (subtle scale pulse)
    float breathPhase = c.time * c.breathRate;
    float breathScale = 1.0 + sin(breathPhase) * 0.02;
    c.torso.scale.y = breathScale;

    // Head look-around (occasional)
    if (c.idleTimer > c.nextLookTime) {
        c.targetLookDir = randomDirection();
        c.nextLookTime = c.idleTimer + random(2.0, 5.0);
    }
    c.headRotation = slerp(c.headRotation, c.targetLookDir, dt * 2.0);

    // Weight shifting (very subtle)
    float shiftPhase = c.time * 0.3;
    c.bodyLean = sin(shiftPhase) * 0.01;

    // Tail sway
    c.tailSway = sin(c.time * 1.5) * 0.1;
}
```

---

## CATEGORY 2: PROCEDURAL TEXTURES (MATH-BASED)

### 2.1 Noise Functions (Foundation)
**Already have**: Perlin noise in shaders

**Add These**:

```glsl
// Simplex Noise (faster than Perlin)
float simplex3D(vec3 p) {
    // Implementation...
}

// Worley/Cellular Noise (for scales, cells)
float worley(vec3 p, float cellSize) {
    vec3 cell = floor(p / cellSize);
    float minDist = 1000.0;

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            for (int z = -1; z <= 1; z++) {
                vec3 neighbor = cell + vec3(x, y, z);
                vec3 point = neighbor + hash33(neighbor);
                float dist = length(p / cellSize - point);
                minDist = min(minDist, dist);
            }
        }
    }
    return minDist;
}

// Fractal Brownian Motion (layered noise)
float fbm(vec3 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise(p * frequency);
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return value;
}
```

### 2.2 Pattern Generation Shaders

**Spots Pattern** (leopard, cheetah):
```glsl
float spots(vec3 p, float density, float size) {
    float n = worley(p * density, 1.0);
    return smoothstep(size, size * 0.5, n);
}
```

**Stripes Pattern** (tiger, zebra):
```glsl
float stripes(vec3 p, float frequency, float waviness) {
    float wave = sin(p.x * frequency + fbm(p * 2.0, 3) * waviness);
    return smoothstep(-0.2, 0.2, wave);
}
```

**Scales Pattern** (reptile, fish):
```glsl
float scales(vec3 p, float size) {
    vec2 uv = p.xy / size;
    vec2 cell = floor(uv);
    vec2 f = fract(uv);

    // Offset every other row
    if (mod(cell.y, 2.0) > 0.5) {
        f.x = fract(f.x + 0.5);
    }

    // Scale shape (pointed oval)
    float d = length((f - 0.5) * vec2(1.0, 1.5));
    return smoothstep(0.5, 0.4, d);
}
```

**Fur/Feather Pattern**:
```glsl
float fur(vec3 p, float density) {
    float n1 = fbm(p * density, 4);
    float n2 = fbm(p * density * 2.0 + 100.0, 4);
    return n1 * n2;
}
```

### 2.3 Creature Coloring System
**Based on genetics**:

```glsl
// In fragment shader
uniform vec3 baseColor;      // From genome
uniform vec3 secondaryColor; // From genome
uniform float patternType;   // 0=solid, 1=spots, 2=stripes, 3=scales
uniform float patternScale;  // From genome
uniform float patternIntensity;

vec3 getCreatureColor(vec3 worldPos, vec3 normal) {
    float pattern = 0.0;

    if (patternType < 0.5) {
        pattern = 0.0; // Solid color
    } else if (patternType < 1.5) {
        pattern = spots(worldPos, patternScale, 0.3);
    } else if (patternType < 2.5) {
        pattern = stripes(worldPos, patternScale, 2.0);
    } else {
        pattern = scales(worldPos, patternScale);
    }

    vec3 color = mix(baseColor, secondaryColor, pattern * patternIntensity);

    // Add subtle noise variation
    float variation = fbm(worldPos * 10.0, 3) * 0.1;
    color += variation;

    return color;
}
```

### 2.4 Biome-Appropriate Coloring

```cpp
// In CreatureBuilder or Genome
vec3 getBiomeAppropriateColor(Biome biome, float randomSeed) {
    switch(biome) {
        case FOREST:
            return mix(vec3(0.2, 0.4, 0.1), vec3(0.4, 0.3, 0.2), randomSeed);
        case DESERT:
            return mix(vec3(0.8, 0.7, 0.5), vec3(0.6, 0.5, 0.3), randomSeed);
        case OCEAN:
            return mix(vec3(0.1, 0.3, 0.6), vec3(0.2, 0.5, 0.7), randomSeed);
        case SNOW:
            return mix(vec3(0.9, 0.9, 0.95), vec3(0.7, 0.7, 0.8), randomSeed);
        case SAVANNA:
            return mix(vec3(0.7, 0.6, 0.3), vec3(0.5, 0.4, 0.2), randomSeed);
    }
}
```

---

## CATEGORY 3: CREATURE TYPES

### 3.1 Flying Creatures
**Requirements**:
- Wings (already in MorphologyGenes)
- Flight physics
- Altitude management
- Landing/takeoff states

**Implementation**:
```cpp
enum class FlightState {
    GROUNDED,
    TAKEOFF,
    FLYING,
    GLIDING,
    LANDING,
    HOVERING
};

class FlyingCreature : public Creature {
    FlightState flightState;
    float altitude;
    float wingSpan;
    float liftCoefficient;

    void updateFlight(float dt) {
        // Calculate lift
        float airSpeed = glm::length(velocity);
        float lift = 0.5 * airDensity * airSpeed * airSpeed * wingArea * liftCoefficient;

        // Apply forces
        vec3 liftForce = vec3(0, lift, 0);
        vec3 drag = -velocity * dragCoefficient;
        vec3 thrust = forward * flapPower;

        acceleration = (liftForce + drag + thrust + gravity) / mass;
    }
};
```

### 3.2 Aquatic Creatures (Fish)
**Requirements**:
- Swimming locomotion
- Buoyancy
- Underwater environment
- Schooling behavior

**Implementation**:
```cpp
class AquaticCreature : public Creature {
    float buoyancy;
    float swimSpeed;
    float depth;

    void updateSwimming(float dt) {
        // Buoyancy force
        float buoyancyForce = (waterDensity - bodyDensity) * volume * gravity;

        // Drag in water (higher than air)
        vec3 drag = -velocity * waterDragCoefficient;

        // Thrust from tail
        vec3 thrust = forward * tailPower;

        acceleration = (vec3(0, buoyancyForce, 0) + drag + thrust) / mass;
    }

    // Fish schooling (Boids algorithm)
    vec3 calculateSchoolingForce(std::vector<AquaticCreature*>& neighbors) {
        vec3 separation = calculateSeparation(neighbors);
        vec3 alignment = calculateAlignment(neighbors);
        vec3 cohesion = calculateCohesion(neighbors);

        return separation * 1.5 + alignment * 1.0 + cohesion * 1.0;
    }
};
```

### 3.3 Insects/Hexapods
**Requirements**:
- 6 legs with tripod gait
- Optional wings
- Small size
- Swarm behavior

### 3.4 Serpentine Creatures
**Requirements**:
- No limbs
- Slithering locomotion
- Body wave physics

---

## CATEGORY 4: NAMETAG & UI SYSTEM

### 4.1 3D Nametags (Billboard Sprites)
**Goal**: Floating names above creatures that always face camera

**Implementation**:
```cpp
class Nametag {
    std::string name;
    vec3 worldPosition;
    vec3 color;
    float opacity;
    float scale;

    // Health bar
    float health;
    float maxHealth;

    // Species icon
    int speciesIcon;
};

// Vertex Shader (nametag_vertex.glsl)
// Already exists in shaders folder!

// Fragment Shader (nametag_fragment.glsl)
// Already exists in shaders folder!
```

**Nametag Features to Add**:
- Health bar below name
- Species icon
- Generation number (Gen 1, Gen 2, etc.)
- Trait icons (carnivore skull, herbivore leaf)
- Selected creature highlight
- Distance-based fade

### 4.2 Creature Info Panel
**When creature is selected, show**:
- Name and species
- Age / Generation
- Energy level
- Current state (hunting, fleeing, eating, mating)
- Key traits (speed, size, vision)
- Family tree preview
- Kill count (carnivores)
- Offspring count

### 4.3 Population Statistics HUD
- Total population by species
- Birth/death rates
- Average traits over time
- Food availability
- Current season/time

### 4.4 Minimap
- Top-down terrain view
- Creature dots (colored by type)
- Selected creature highlight
- Camera frustum indicator

---

## CATEGORY 5: CAMERA & CINEMATIC SYSTEMS

### 5.1 Camera Modes

**Free Camera** (current):
- WASD movement
- Mouse look
- Scroll zoom

**Follow Camera**:
```cpp
void followCreature(Creature* target, float dt) {
    vec3 desiredPos = target->position
                    - target->forward * followDistance
                    + vec3(0, followHeight, 0);

    position = lerp(position, desiredPos, dt * followSmoothing);
    lookAt(target->position + vec3(0, target->height * 0.5, 0));
}
```

**Orbit Camera**:
```cpp
void orbitCreature(Creature* target, float dt) {
    orbitAngle += orbitSpeed * dt;

    float x = cos(orbitAngle) * orbitRadius;
    float z = sin(orbitAngle) * orbitRadius;

    position = target->position + vec3(x, orbitHeight, z);
    lookAt(target->position);
}
```

**Cinematic Camera**:
- Smooth dolly shots
- Tracking shots
- Focus pulls
- Dramatic angles for predator-prey chases

### 5.2 Picture-in-Picture
- Small window showing different view
- Useful for following multiple creatures

### 5.3 Time Controls
- Pause (Space)
- Slow motion (0.25x, 0.5x)
- Normal speed (1x)
- Fast forward (2x, 4x, 8x)
- Ultra fast (skip frames, 100x)

---

## CATEGORY 6: ENVIRONMENT IMPROVEMENTS

### 6.1 Fix Vegetation Rendering (HIGH PRIORITY)
**Current state**: VegetationManager exists but doesn't render

**Fix**:
```cpp
// In Simulation::render()
vegetationManager.render(camera, terrainShader);
```

### 6.2 Water System
**Features**:
- Reflective water surface
- Underwater fog
- Waves animation
- Rivers and lakes
- Ocean with tides

```glsl
// Water shader
vec3 waterColor(vec3 worldPos, float depth) {
    vec3 shallowColor = vec3(0.1, 0.5, 0.7);
    vec3 deepColor = vec3(0.0, 0.1, 0.3);

    float depthFactor = clamp(depth / maxDepth, 0.0, 1.0);
    vec3 color = mix(shallowColor, deepColor, depthFactor);

    // Waves
    float wave = sin(worldPos.x * 0.5 + time * 2.0) * 0.1;
    wave += sin(worldPos.z * 0.3 + time * 1.5) * 0.05;

    return color;
}
```

### 6.3 Weather System
- Rain (particle system)
- Snow
- Fog (distance-based)
- Wind (affects creatures and plants)
- Lightning storms

### 6.4 Day/Night Cycle
- Sun position changes
- Sky color gradient
- Star sky at night
- Moon
- Affects creature behavior (nocturnal predators)

### 6.5 Multiple Biomes
**Current**: 5 biome textures exist
**Expand**:
- Tropical rainforest
- Coral reef (underwater)
- Volcanic/lava
- Crystal caves
- Alien/fantasy biomes (for YouTube variety)

---

## CATEGORY 7: PLAYER INTERACTION / GOD-MODE

### 7.1 Creature Spawning
- Click to place creatures
- Choose species/type
- Set initial traits
- Spawn mutations

### 7.2 Environmental Manipulation
- Place food sources
- Create barriers/walls
- Modify terrain height
- Add water bodies
- Place shelter structures

### 7.3 Evolution Controls
- Force mutation events
- Selective breeding tool
- Kill specific creatures
- Protect species from extinction
- Introduce invasive species

### 7.4 World Settings (Generation)
```cpp
struct WorldSettings {
    int seed;
    float worldSize;

    // Terrain
    float mountainHeight;
    float waterLevel;
    float terrainRoughness;

    // Biomes
    float forestDensity;
    float desertSize;
    bool hasOcean;

    // Starting creatures
    int initialHerbivores;
    int initialCarnivores;
    float initialDiversity;

    // Simulation
    float mutationRate;
    float foodRegrowthRate;
    float dayLength;
};
```

### 7.5 Presets
- "Peaceful Garden" - herbivores only, lots of food
- "Survival" - harsh environment, limited resources
- "Predator Arena" - lots of carnivores
- "Ocean World" - mostly water
- "Extinction Event" - start after mass extinction

---

## CATEGORY 8: YOUTUBE-READY FEATURES

### 8.1 Recording Features
- Screenshot hotkey (F12)
- Video recording integration
- Replay system (save simulation state)

### 8.2 Visual Polish
- Anti-aliasing
- Ambient occlusion
- Bloom on bright objects
- Motion blur (optional)
- Depth of field for focus

### 8.3 Interesting Events Detection
- "A new species has emerged!"
- "Extinction event: X species died out"
- "Record holder: Fastest creature ever"
- "Ancient creature: 100 generations old"
- "Epic battle: Large predator vs prey herd"

### 8.4 Statistics Dashboard
- Population graphs over time
- Trait evolution charts
- Phylogenetic tree visualization
- Heat maps of creature activity
- Top creatures leaderboard

### 8.5 Narration Support
- Event log with timestamps
- Key moments highlighted
- Creature biographies
- Species history

---

## CATEGORY 9: PERFORMANCE & POLISH

### 9.1 Optimization
- LOD (Level of Detail) for distant creatures
- Frustum culling
- Spatial partitioning (already have SpatialGrid)
- Multithreading for physics/AI
- GPU compute shaders for large simulations

### 9.2 Quality Settings
```cpp
enum class QualityLevel {
    LOW,      // 100 creatures, simple shaders
    MEDIUM,   // 500 creatures, basic effects
    HIGH,     // 1000 creatures, all effects
    ULTRA     // 2000+ creatures, max quality
};
```

### 9.3 Bug Fixes Needed
- [ ] Camera scroll wheel bug
- [ ] Vegetation not rendering
- [ ] Verify NEAT integration
- [ ] Test all gait types

---

## IMPLEMENTATION PRIORITY ORDER

### Phase 1: Fix & Polish (Week 1-2)
1. Fix camera scroll bug
2. Enable vegetation rendering
3. Verify all existing systems work
4. Add basic nametags
5. Add creature selection

### Phase 2: Animation (Week 3-4)
1. Implement skeletal animation system
2. Walking leg animation
3. Idle animations
4. Creature states visible (eating, fleeing)

### Phase 3: Visual Quality (Week 5-6)
1. Enhanced procedural textures
2. Pattern variety (spots, stripes, scales)
3. Genetic color variation
4. Better lighting

### Phase 4: Creature Variety (Week 7-8)
1. Flying creatures
2. Swimming creatures
3. Insect creatures
4. Body type variety

### Phase 5: Environment (Week 9-10)
1. Water improvements
2. Day/night cycle
3. Weather effects
4. More biomes

### Phase 6: Player Tools (Week 11-12)
1. Creature spawning
2. World settings
3. Presets
4. Save/load

### Phase 7: YouTube Features (Week 13-14)
1. Statistics dashboard
2. Event detection
3. Recording features
4. Visual effects (bloom, DOF)

---

## FILE REFERENCE QUICK GUIDE

| System | Key Files |
|--------|-----------|
| Main Loop | `src/main.cpp`, `src/core/Simulation.cpp` |
| Creatures | `src/entities/Creature.cpp`, `Genome.cpp` |
| Genetics | `src/entities/genetics/*.cpp` |
| Rendering | `src/graphics/rendering/CreatureRenderer.cpp` |
| Procedural | `src/graphics/procedural/MarchingCubes.cpp` |
| Physics | `src/physics/Locomotion.cpp`, `Morphology.cpp` |
| AI | `src/ai/NEATGenome.cpp`, `BrainModules.cpp` |
| Sensory | `src/entities/SensorySystem.cpp` |
| Environment | `src/environment/*.cpp` |
| Shaders | `shaders/*.glsl` |

---

## RESEARCH TOPICS FOR FUTURE AGENTS

When resuming work, research these topics:
1. Procedural animation - inverse kinematics, CPG gaits
2. Procedural textures - noise functions, GLSL patterns
3. Ecosystem AI - predator-prey dynamics, flocking
4. Genetic evolution - NEAT, speciation mechanics
5. Environment generation - biomes, weather, water
6. UI/Camera systems - nametags, cinematic cameras
7. Player interaction - god-game mechanics
8. Performance - LOD, culling, multithreading

---

*Document created: January 2026*
*Project: OrganismEvolution*
*Goal: YouTube-worthy 3D evolution simulation*
