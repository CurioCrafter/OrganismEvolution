# Phase 3 Agent Prompts - Extended Work Sessions

These prompts are designed for HOURS of work, not minutes. Each agent MUST spawn sub-agents for parallel work.

---

## Agent 1: Skeletal Animation System Implementation (4-6 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS on this task. Do NOT finish quickly.
You MUST spawn sub-agents for parallel research and implementation. Use the Task tool frequently.

MISSION: Implement a complete skeletal animation system for creatures with IK solvers.

============================================================================
PHASE 1: DEEP RESEARCH (1+ hour minimum)
============================================================================

SPAWN 3 SUB-AGENTS IN PARALLEL for research:

SUB-AGENT 1A - Research existing animation code:
- Read ALL files in src/entities/ (Creature.cpp, Genome.cpp, etc.)
- Read ALL files in deprecated/opengl/ for reference
- Document current creature structure and rendering pipeline
- Find where animation would integrate

SUB-AGENT 1B - Research IK algorithms:
- Web search for "Two-Bone IK implementation C++"
- Web search for "FABRIK IK algorithm tutorial"
- Web search for "CCD Inverse Kinematics"
- Document pros/cons of each approach
- Find reference implementations

SUB-AGENT 1C - Research the ModularGameEngine:
- Read C:\Users\andre\Desktop\ModularGameEngine\Engine\Animation\Public\Animation\*.h
- Document existing Skeleton, IKSystem, BlendTree classes
- Identify what can be reused vs what needs new implementation

CHECKPOINT 1: Create docs/ANIMATION_RESEARCH.md with findings from all 3 sub-agents.
DO NOT PROCEED until this document exists and has 500+ lines.

============================================================================
PHASE 2: ARCHITECTURE DESIGN (1+ hour minimum)
============================================================================

Based on research, design the animation system:

1. Create src/animation/ directory structure:
   - Skeleton.h/.cpp - Bone hierarchy, bind pose
   - Pose.h/.cpp - Runtime bone transforms
   - IKSolver.h/.cpp - Two-bone and FABRIK solvers
   - ProceduralLocomotion.h/.cpp - Foot placement, gaits
   - AnimationController.h/.cpp - State machine for animations

2. Design data structures:
```cpp
struct Bone {
    int parentIndex;  // -1 for root
    glm::mat4 localBindPose;
    glm::mat4 inverseBindPose;
    std::string name;
    // Joint limits for IK
    float minAngle, maxAngle;
};

struct Skeleton {
    std::vector<Bone> bones;
    std::unordered_map<std::string, int> boneNameToIndex;
    // Methods for bone lookup, hierarchy traversal
};

struct Pose {
    std::vector<glm::mat4> localTransforms;
    std::vector<glm::mat4> globalTransforms;
    void computeGlobalTransforms(const Skeleton& skeleton);
};
```

3. Design IK solver interface:
```cpp
class IKSolver {
public:
    virtual void solve(Pose& pose, int endEffectorBone,
                       const glm::vec3& target, int maxIterations) = 0;
};

class TwoBoneIKSolver : public IKSolver { ... };
class FABRIKSolver : public IKSolver { ... };
```

CHECKPOINT 2: Create docs/ANIMATION_ARCHITECTURE.md with complete design.
Include class diagrams (text-based), data flow, and integration points.

============================================================================
PHASE 3: IMPLEMENTATION (2+ hours minimum)
============================================================================

SPAWN 4 SUB-AGENTS IN PARALLEL for implementation:

SUB-AGENT 3A - Implement Skeleton and Pose:
- Create src/animation/Skeleton.h and Skeleton.cpp
- Create src/animation/Pose.h and Pose.cpp
- Implement all methods from design
- Add unit tests in tests/animation/

SUB-AGENT 3B - Implement Two-Bone IK:
- Create TwoBoneIKSolver in src/animation/IKSolver.cpp
- Use Law of Cosines for analytical solution
- Handle edge cases (target too far, too close)
- Add debug visualization

SUB-AGENT 3C - Implement FABRIK:
- Create FABRIKSolver in src/animation/IKSolver.cpp
- Iterative forward-backward reaching
- Support joint angle constraints
- Handle multiple end-effectors

SUB-AGENT 3D - Implement ProceduralLocomotion:
- Create src/animation/ProceduralLocomotion.cpp
- Foot placement using terrain raycasts
- Step triggers based on distance threshold
- Diagonal gait pattern (quadrupeds)
- Tripod gait pattern (hexapods)

CHECKPOINT 3: All 4 sub-agents complete. Run integration tests.

============================================================================
PHASE 4: SHADER INTEGRATION (1+ hour minimum)
============================================================================

Modify creature rendering for skeletal animation:

1. Update shaders/hlsl/CreatureSkinned.hlsl:
   - Already has bone matrix support (verify it works)
   - Add debug mode to visualize bone influences
   - Ensure proper skinning with 4 bones per vertex

2. Update src/main.cpp creature rendering:
   - Create bone matrix constant buffer
   - Upload skeleton pose each frame
   - Bind to b1 register as designed

3. Update creature mesh generation:
   - Add bone weights to vertices
   - Assign bones based on metaball positions
   - Generate proper bind pose

CHECKPOINT 4: Render a creature with animated skeleton visible in debug mode.

============================================================================
PHASE 5: CREATURE INTEGRATION (1+ hour minimum)
============================================================================

Connect animation system to creatures:

1. Modify src/entities/Creature.h:
   - Add Skeleton* skeleton
   - Add Pose currentPose
   - Add AnimationController* animator

2. Modify CreatureBuilder (src/graphics/procedural/):
   - Generate skeleton from body plan
   - Quadruped: spine + 4 legs + tail
   - Hexapod: spine + 6 legs
   - Avian: spine + 2 legs + 2 wings + tail

3. Update creature update loop:
   - Call animator->update(deltaTime)
   - Solve IK for each leg
   - Upload pose to GPU

CHECKPOINT 5: Creatures walk with procedural leg animation.

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/ANIMATION_RESEARCH.md (500+ lines)
- [ ] docs/ANIMATION_ARCHITECTURE.md (300+ lines)
- [ ] src/animation/Skeleton.h and .cpp
- [ ] src/animation/Pose.h and .cpp
- [ ] src/animation/IKSolver.h and .cpp (Two-bone + FABRIK)
- [ ] src/animation/ProceduralLocomotion.h and .cpp
- [ ] src/animation/AnimationController.h and .cpp
- [ ] Modified shaders/hlsl/CreatureSkinned.hlsl
- [ ] Modified src/entities/Creature.h and .cpp
- [ ] Modified CreatureBuilder for skeleton generation
- [ ] tests/animation/ test files
- [ ] Working creature leg animation in-game

ESTIMATED TIME: 4-6 hours with sub-agents
DO NOT MARK COMPLETE until all checkpoints pass and deliverables exist.
```

---

## Agent 2: Flying Creature Implementation (4-6 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS on this task. Do NOT finish quickly.
You MUST spawn sub-agents for parallel research and implementation.

MISSION: Add flying creatures (birds, insects) with wing animation and aerial behavior.

============================================================================
PHASE 1: RESEARCH AERIAL CREATURES (1+ hour)
============================================================================

SPAWN 3 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Research bird flight mechanics:
- Web search "bird wing anatomy for animation"
- Web search "procedural bird flight animation"
- Web search "flapping wing physics simulation"
- Document wing bone structure (humerus, radius/ulna, carpals, digits)
- Document flap cycle phases (upstroke, downstroke, glide)

SUB-AGENT 1B - Research insect flight:
- Web search "insect wing animation game development"
- Web search "four-wing insect flight patterns"
- Document wing beat frequencies (20-200 Hz)
- Document figure-8 wing motion pattern

SUB-AGENT 1C - Research existing creature code:
- Read src/entities/Creature.cpp thoroughly
- Read src/entities/CreatureType.h
- Read src/entities/Genome.h
- Read src/entities/SteeringBehaviors.cpp
- Document how to extend for flying

CHECKPOINT 1: Create docs/FLYING_CREATURE_RESEARCH.md (400+ lines)

============================================================================
PHASE 2: DESIGN FLYING CREATURE SYSTEM (1+ hour)
============================================================================

Design new creature types and behaviors:

1. Extend CreatureType enum:
```cpp
enum class CreatureType {
    HERBIVORE,      // Existing
    CARNIVORE,      // Existing
    FLYING_BIRD,    // New - feathered wings, 2 legs
    FLYING_INSECT,  // New - membrane wings, 6 legs
    AERIAL_PREDATOR // New - flying carnivore (hawk-like)
};
```

2. Design WingConfig:
```cpp
struct WingConfig {
    WingType type;         // FEATHERED, MEMBRANE, INSECT
    int boneCount;         // 3-5 per wing
    float span;            // Total wingspan
    float flapFrequency;   // Hz
    float flapAmplitude;   // Degrees
    float glideFactor;     // 0=always flap, 1=always glide
};
```

3. Design flight behavior:
```cpp
class FlightBehavior {
    float altitude;        // Current height above ground
    float targetAltitude;  // Desired height
    float climbRate;       // Vertical velocity
    float bankAngle;       // For turning

    void updateFlight(float dt, const Terrain& terrain);
    void flapWings(Pose& pose, float dt);
    void glide(Pose& pose);
    void land(Pose& pose);
    void takeoff(Pose& pose);
};
```

CHECKPOINT 2: Create docs/FLYING_CREATURE_DESIGN.md (300+ lines)

============================================================================
PHASE 3: IMPLEMENT WING ANIMATION (1.5+ hours)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 3A - Implement WingAnimator:
Create src/animation/WingAnimator.h and .cpp:
```cpp
class WingAnimator {
    float flapPhase;       // 0-2π cycle
    float glideFactor;     // 0=flapping, 1=gliding
    WingConfig config;

public:
    void update(float dt, float velocity, bool isLanding);
    void applyToSkeleton(Pose& pose, int wingRootBone);

private:
    void calculateFlapAngles(float phase, float& shoulder, float& elbow, float& wrist);
    void applyDampedTransform(Pose& pose, int boneIndex, const glm::quat& targetRot);
};
```

Implement flap cycle:
- Downstroke: wings sweep down and forward (power)
- Upstroke: wings fold and sweep up (recovery)
- Add secondary motion for feathers/membrane

SUB-AGENT 3B - Implement wing metaball generation:
Modify src/graphics/procedural/OrganBuilder.cpp (or create if not exists):
```cpp
void addWings(MetaballSystem& system, const WingConfig& config, BodyPlan plan) {
    // Bird wings: elongated ellipsoids along bone chain
    // Insect wings: flat discs with transparency

    // Add wing bones to skeleton
    // Position metaballs along wing bones
    // Set up bone weights for vertices
}
```

CHECKPOINT 3: Wings render and animate (even if not flying yet)

============================================================================
PHASE 4: IMPLEMENT FLIGHT PHYSICS (1+ hour)
============================================================================

Create src/entities/FlightBehavior.h and .cpp:

1. Flight state machine:
   - GROUNDED - on terrain, can takeoff
   - TAKING_OFF - transitioning to flight
   - FLYING - normal flight
   - GLIDING - energy-saving descent
   - LANDING - transitioning to ground
   - DIVING - fast descent (predators)

2. Flight physics:
```cpp
void FlightBehavior::updateFlight(float dt) {
    // Lift based on velocity and wing area
    float lift = wingArea * velocity * velocity * liftCoefficient;

    // Drag
    float drag = dragCoefficient * velocity * velocity;

    // Gravity
    float gravity = mass * 9.8f;

    // Net vertical force
    float verticalForce = lift - gravity - (velocity.y > 0 ? drag : -drag);

    // Update velocity and position
    velocity.y += verticalForce / mass * dt;
    position += velocity * dt;

    // Banking for turns
    if (turning) {
        bankAngle = lerp(bankAngle, targetBank, dt * 3.0f);
        // Apply bank to rotation
    }
}
```

3. Terrain avoidance:
```cpp
void FlightBehavior::avoidTerrain(const Terrain& terrain) {
    float groundHeight = terrain.getHeightAt(position.x, position.z);
    float clearance = position.y - groundHeight;

    if (clearance < minAltitude) {
        // Emergency climb
        targetAltitude = groundHeight + minAltitude * 2;
    }
}
```

CHECKPOINT 4: Flying creatures maintain altitude and avoid terrain

============================================================================
PHASE 5: INTEGRATE WITH ECOSYSTEM (1+ hour)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 5A - Aerial predator behavior:
- Implement hunting from above (diving)
- Target selection (ground creatures)
- Dive attack mechanics
- Energy cost of flight vs walking

SUB-AGENT 5B - Flying herbivore behavior:
- Implement seed/fruit foraging
- Perching on trees (when vegetation system exists)
- Flocking behavior (boids algorithm)
- Migration patterns

Update src/core/Simulation.cpp (or main.cpp):
- Spawn flying creatures
- Update flight behavior each frame
- Handle air-to-ground predation
- Handle flying creature reproduction

CHECKPOINT 5: Flying creatures hunt ground creatures and reproduce

============================================================================
PHASE 6: SHADER AND RENDERING (30+ minutes)
============================================================================

Update shaders for flying creatures:

1. Modify Creature.hlsl or create CreatureFlying.hlsl:
   - Wing transparency for insect wings
   - Feather detail for bird wings
   - Motion blur for fast-flapping wings

2. Add wing shadow casting:
   - Wings cast shadows on ground
   - Self-shadowing on creature body

3. Add flight trail effect (optional):
   - Subtle particle trail when diving
   - Wind disturbance visualization

CHECKPOINT 6: Flying creatures look visually distinct and polished

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/FLYING_CREATURE_RESEARCH.md (400+ lines)
- [ ] docs/FLYING_CREATURE_DESIGN.md (300+ lines)
- [ ] Extended CreatureType enum
- [ ] src/animation/WingAnimator.h and .cpp
- [ ] Wing metaball generation in OrganBuilder
- [ ] src/entities/FlightBehavior.h and .cpp
- [ ] Flight physics with lift/drag/gravity
- [ ] Terrain avoidance for flyers
- [ ] Aerial predator hunting behavior
- [ ] Flying herbivore foraging behavior
- [ ] Updated shaders for wings
- [ ] Flying creatures spawning and surviving in simulation

ESTIMATED TIME: 4-6 hours with sub-agents
```
 to add swimming mode
- Identify sha
---

## Agent 3: Aquatic Creature Implementation (4-6 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Add aquatic creatures (fish, sharks) with swimming animation and underwater behavior.

============================================================================
PHASE 1: RESEARCH AQUATIC LOCOMOTION (1+ hour)
============================================================================

SPAWN 3 SUB-AGENTS:

SUB-AGENT 1A - Research fish swimming:
- Web search "fish swimming animation procedural"
- Web search "carangiform swimming simulation"
- Web search "anguilliform locomotion game"
- Document S-wave spine motion
- Document fin functions (propulsion, steering, stability)

SUB-AGENT 1B - Research existing water system:
- Read Runtime/Shaders/Terrain.hlsl water sections thoroughly
- Document current water level (WATER_THRESH = 0.012)
- Understand water depth calculation
- Find where underwater rendering could integrate

SUB-AGENT 1C - Research creature extension:
- Read src/entities/Creature.cpp
- Read src/entities/SteeringBehaviors.cpp
- Document howred vs separate behavior code

CHECKPOINT 1: Create docs/AQUATIC_CREATURE_RESEARCH.md (400+ lines)

============================================================================
PHASE 2: DESIGN AQUATIC SYSTEM (1+ hour)
============================================================================

1. New creature types:
```cpp
enum class CreatureType {
    // ... existing ...
    AQUATIC_HERBIVORE,  // Fish that eat algae/plants
    AQUATIC_PREDATOR,   // Sharks, predatory fish
    AMPHIBIAN           // Can survive land and water
};
```

2. Swimming configuration:
```cpp
struct SwimConfig {
    SwimStyle style;      // CARANGIFORM (tail), ANGUILLIFORM (eel), LABRIFORM (fins)
    float bodyWaveSpeed;  // How fast the S-wave travels
    float bodyWaveAmp;    // Amplitude of body undulation
    int spineSegments;    // Number of spine bones
    bool hasDorsalFin;
    bool hasPectoralFins;
    bool hasCaudalFin;    // Tail fin
};
```

3. Swimming behavior:
```cpp
class SwimBehavior {
    float depth;          // Current depth below water surface
    float targetDepth;    // Desired depth
    float swimPhase;      // Phase of swimming cycle

    void updateSwimming(float dt);
    void applySpineWave(Pose& pose, float dt);
    void applyFinMotion(Pose& pose, float dt);
    void surfaceForAir(/* if needed */);
};
```

CHECKPOINT 2: Create docs/AQUATIC_CREATURE_DESIGN.md (300+ lines)

============================================================================
PHASE 3: IMPLEMENT SWIMMING ANIMATION (1.5+ hours)
============================================================================

SPAWN 2 SUB-AGENTS:

SUB-AGENT 3A - Implement SwimAnimator:
Create src/animation/SwimAnimator.h and .cpp:
```cpp
class SwimAnimator {
    float swimPhase;
    float dampingFactor = 0.85f;
    SwimConfig config;

public:
    void update(float dt, float velocity);
    void applySpineWave(Pose& pose, const std::vector<int>& spineBones);
    void applyFinTrailing(Pose& pose, int finBone);

private:
    // Damped transform for trailing motion
    void applyDampedRotation(Pose& pose, int boneIndex,
                             const glm::quat& parentRot, float damping);
};
```

Implement S-wave motion:
```cpp
void SwimAnimator::applySpineWave(Pose& pose, const std::vector<int>& spineBones) {
    for (int i = 0; i < spineBones.size(); i++) {
        float phaseOffset = float(i) / spineBones.size() * 2.0f * PI;
        float angle = sin(swimPhase + phaseOffset) * config.bodyWaveAmp;

        // Apply rotation around vertical axis (yaw)
        glm::quat rotation = glm::angleAxis(angle, glm::vec3(0, 1, 0));
        pose.localTransforms[spineBones[i]] =
            glm::mat4_cast(rotation) * pose.localTransforms[spineBones[i]];
    }
}
```

SUB-AGENT 3B - Implement fish metaball generation:
Create or modify OrganBuilder for fish bodies:
- Streamlined body shape (ellipsoids)
- Dorsal fin (flat triangular metaballs)
- Pectoral fins (side steering fins)
- Caudal fin (tail fin for propulsion)
- Gills (visual detail)

CHECKPOINT 3: Fish render with animated swimming motion

============================================================================
PHASE 4: IMPLEMENT UNDERWATER PHYSICS (1+ hour)
============================================================================

Create src/entities/SwimBehavior.h and .cpp:

1. Water detection:
```cpp
bool SwimBehavior::isInWater(const glm::vec3& pos, const Terrain& terrain) {
    float terrainHeight = terrain.getHeightAt(pos.x, pos.z);
    float waterSurfaceHeight = terrain.getWaterLevel();
    return pos.y < waterSurfaceHeight && pos.y > terrainHeight;
}
```

2. Buoyancy and drag:
```cpp
void SwimBehavior::updateSwimming(float dt) {
    // Buoyancy (fish are neutrally buoyant)
    float buoyancy = (targetDepth - depth) * buoyancyStrength;

    // Water drag (much higher than air)
    float drag = velocity.length() * velocity.length() * waterDragCoeff;
    glm::vec3 dragForce = -glm::normalize(velocity) * drag;

    // Thrust from tail
    float thrust = swimPower * (1.0f + abs(sin(swimPhase)));
    glm::vec3 thrustForce = forward * thrust;

    // Update velocity
    velocity += (thrustForce + dragForce + glm::vec3(0, buoyancy, 0)) * dt;
    position += velocity * dt;
}
```

3. Depth management:
```cpp
void SwimBehavior::manageDepth(const Terrain& terrain) {
    float waterSurface = terrain.getWaterLevel();
    float seaFloor = terrain.getHeightAt(position.x, position.z);

    // Stay within water bounds
    float minDepth = waterSurface - 0.5f;  // Don't breach surface
    float maxDepth = seaFloor + 0.5f;       // Don't hit floor

    targetDepth = glm::clamp(targetDepth, maxDepth, minDepth);
}
```

CHECKPOINT 4: Fish swim realistically within water boundaries

============================================================================
PHASE 5: UNDERWATER ECOSYSTEM (1+ hour)
============================================================================

SPAWN 2 SUB-AGENTS:

SUB-AGENT 5A - Implement underwater food chain:
- Algae/plant food sources (spawn in water)
- Small fish eat algae
- Large fish eat small fish
- Sharks hunt all fish

SUB-AGENT 5B - Implement school behavior:
- Boids algorithm for fish schools
- Separation, alignment, cohesion
- Predator avoidance (scatter when shark near)
- Return to school after danger passes

Update main simulation:
- Spawn aquatic creatures in water areas
- Separate update loop for underwater vs land
- Handle land-water boundary (prevent fish on land)

CHECKPOINT 5: Functioning underwater ecosystem with predator-prey

============================================================================
PHASE 6: UNDERWATER RENDERING (30+ minutes)
============================================================================

1. Create underwater camera effects:
   - Blue-green color tint when camera underwater
   - Reduced visibility/fog underwater
   - Caustic light patterns on fish

2. Update Creature.hlsl for underwater:
   - Different lighting model underwater
   - Subsurface scattering for fish scales
   - Iridescent scale effect (optional)

3. Fish-specific visual effects:
   - Bubble trail when swimming fast
   - Scales catching light
   - Bioluminescence for deep fish (optional)

CHECKPOINT 6: Underwater looks visually distinct and polished

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/AQUATIC_CREATURE_RESEARCH.md (400+ lines)
- [ ] docs/AQUATIC_CREATURE_DESIGN.md (300+ lines)
- [ ] Extended CreatureType enum for aquatic
- [ ] src/animation/SwimAnimator.h and .cpp
- [ ] Fish metaball generation (fins, streamlined body)
- [ ] src/entities/SwimBehavior.h and .cpp
- [ ] Buoyancy and water drag physics
- [ ] Underwater food chain
- [ ] Fish schooling behavior
- [ ] Underwater rendering effects
- [ ] Aquatic creatures spawning and surviving

ESTIMATED TIME: 4-6 hours with sub-agents
```

---

## Agent 4: Neural Network Evolution System (4-6 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Implement NEAT (NeuroEvolution of Augmenting Topologies) for creature brains.

============================================================================
PHASE 1: RESEARCH NEAT AND EXISTING CODE (1+ hour)
============================================================================

SPAWN 3 SUB-AGENTS:

SUB-AGENT 1A - Research NEAT algorithm:
- Web search "NEAT algorithm implementation C++"
- Web search "NEAT neural network evolution tutorial"
- Web search "Stanley Miikkulainen NEAT paper"
- Document: speciation, innovation numbers, crossover, mutation
- Find open-source NEAT implementations for reference

SUB-AGENT 1B - Research existing neural network code:
- Read src/entities/NeuralNetwork.cpp (if exists)
- Read src/ai/NeuralNetwork.cpp (if exists)
- Read src/ai/NEATGenome.cpp (if exists)
- Read src/ai/BrainModules.cpp (if exists)
- Document current implementation state

SUB-AGENT 1C - Research creature behavior integration:
- Read src/entities/SteeringBehaviors.cpp
- Read src/entities/SensorySystem.cpp
- Document inputs available (vision, smell, etc.)
- Document outputs needed (movement, attack, eat, etc.)

CHECKPOINT 1: Create docs/NEAT_RESEARCH.md (500+ lines)

============================================================================
PHASE 2: DESIGN NEAT SYSTEM (1+ hour)
============================================================================

Design complete NEAT implementation:

1. Gene structures:
```cpp
struct NodeGene {
    int id;
    NodeType type;  // INPUT, HIDDEN, OUTPUT
    float bias;
    ActivationFunc activation;  // SIGMOID, TANH, RELU
};

struct ConnectionGene {
    int inNode, outNode;
    float weight;
    bool enabled;
    int innovation;  // Global innovation number
};

struct NEATGenome {
    std::vector<NodeGene> nodes;
    std::vector<ConnectionGene> connections;
    float fitness;
    int speciesId;

    // Mutation methods
    void mutateWeights(float rate, float strength);
    void mutateAddConnection(InnovationTracker& tracker);
    void mutateAddNode(InnovationTracker& tracker);
    void mutateToggleConnection();

    // Crossover
    static NEATGenome crossover(const NEATGenome& parent1, const NEATGenome& parent2);

    // Evaluation
    std::vector<float> evaluate(const std::vector<float>& inputs);
};
```

2. Species management:
```cpp
struct Species {
    int id;
    NEATGenome representative;
    std::vector<NEATGenome*> members;
    float averageFitness;
    int stagnantGenerations;

    bool isCompatible(const NEATGenome& genome, float threshold);
    void adjustFitness();  // Fitness sharing
};

class SpeciesManager {
    std::vector<Species> species;
    float compatibilityThreshold = 3.0f;

    void speciate(std::vector<NEATGenome>& population);
    void reproduce(std::vector<NEATGenome>& nextGen);
    void cullStagnant(int maxStagnation);
};
```

3. Innovation tracking:
```cpp
class InnovationTracker {
    int nextInnovation = 0;
    int nextNodeId = 0;
    std::map<std::pair<int,int>, int> connectionInnovations;

    int getConnectionInnovation(int inNode, int outNode);
    int getNewNodeId();
};
```

CHECKPOINT 2: Create docs/NEAT_ARCHITECTURE.md (400+ lines)

============================================================================
PHASE 3: IMPLEMENT NEAT CORE (1.5+ hours)
============================================================================

SPAWN 3 SUB-AGENTS:

SUB-AGENT 3A - Implement NEATGenome:
Create src/ai/neat/NEATGenome.h and .cpp:
- All mutation methods
- Network evaluation (forward pass)
- Genome distance calculation (for speciation)
- Serialization (save/load genomes)

SUB-AGENT 3B - Implement Species and SpeciesManager:
Create src/ai/neat/Species.h and .cpp:
- Compatibility checking
- Fitness sharing
- Representative selection
- Reproduction with crossover

SUB-AGENT 3C - Implement InnovationTracker:
Create src/ai/neat/InnovationTracker.h and .cpp:
- Thread-safe innovation assignment
- Connection innovation lookup
- Node ID generation
- Save/load for resuming evolution

CHECKPOINT 3: NEAT core compiles and passes unit tests

============================================================================
PHASE 4: CREATURE BRAIN INTEGRATION (1+ hour)
============================================================================

Connect NEAT to creature behavior:

1. Define brain inputs (from SensorySystem):
```cpp
std::vector<float> CreatureBrain::getInputs(Creature& creature, World& world) {
    std::vector<float> inputs;

    // Vision (8 directions, distance to nearest object)
    for (int i = 0; i < 8; i++) {
        auto [distance, type] = creature.sensory.raycast(i * 45.0f);
        inputs.push_back(distance / maxVisionRange);
        inputs.push_back(type == FOOD ? 1.0f : 0.0f);
        inputs.push_back(type == PREDATOR ? 1.0f : 0.0f);
        inputs.push_back(type == PREY ? 1.0f : 0.0f);
    }

    // Internal state
    inputs.push_back(creature.energy / maxEnergy);
    inputs.push_back(creature.health / maxHealth);
    inputs.push_back(creature.age / maxAge);

    // Memory (last action, last food direction, etc.)
    // ... add more inputs ...

    return inputs;
}
```

2. Define brain outputs:
```cpp
void CreatureBrain::applyOutputs(Creature& creature, const std::vector<float>& outputs) {
    // Movement (continuous)
    creature.desiredDirection.x = outputs[0] * 2.0f - 1.0f;  // -1 to 1
    creature.desiredDirection.z = outputs[1] * 2.0f - 1.0f;
    creature.desiredSpeed = outputs[2];

    // Actions (thresholded)
    if (outputs[3] > 0.5f) creature.tryEat();
    if (outputs[4] > 0.5f) creature.tryAttack();
    if (outputs[5] > 0.5f) creature.tryReproduce();

    // Optional: express emotions/state
    creature.alertLevel = outputs[6];
}
```

3. Update creature update loop:
```cpp
void Creature::update(float dt, World& world) {
    // Get brain inputs
    auto inputs = brain.getInputs(*this, world);

    // Evaluate neural network
    auto outputs = genome.brain.evaluate(inputs);

    // Apply brain outputs
    brain.applyOutputs(*this, outputs);

    // Physics and movement
    // ... existing code ...
}
```

CHECKPOINT 4: Creatures make decisions using neural networks

============================================================================
PHASE 5: EVOLUTION AND FITNESS (1+ hour)
============================================================================

SPAWN 2 SUB-AGENTS:

SUB-AGENT 5A - Implement fitness evaluation:
```cpp
float calculateFitness(const Creature& creature) {
    float fitness = 0.0f;

    // Survival time
    fitness += creature.age * 0.1f;

    // Food eaten
    fitness += creature.foodEaten * 10.0f;

    // Offspring produced
    fitness += creature.offspring.size() * 50.0f;

    // Distance traveled (exploration)
    fitness += creature.distanceTraveled * 0.01f;

    // Energy efficiency
    fitness += creature.energyEfficiency * 5.0f;

    // Predator: kills made
    if (creature.type == CARNIVORE) {
        fitness += creature.kills * 30.0f;
    }

    return fitness;
}
```

SUB-AGENT 5B - Implement generation cycling:
```cpp
class EvolutionManager {
    SpeciesManager speciesManager;
    InnovationTracker innovationTracker;
    int generation = 0;

    void onCreatureDeath(Creature& creature) {
        creature.genome.brain.fitness = calculateFitness(creature);
        // Add to species for next generation
    }

    void advanceGeneration() {
        speciesManager.speciate(population);
        speciesManager.adjustFitness();
        auto nextGen = speciesManager.reproduce();
        speciesManager.cullStagnant(15);

        // Spawn new creatures with evolved brains
        spawnNewGeneration(nextGen);
        generation++;

        // Log statistics
        logGenerationStats();
    }
};
```

CHECKPOINT 5: Evolution progresses across generations with improving fitness

============================================================================
PHASE 6: VISUALIZATION AND DEBUGGING (30+ minutes)
============================================================================

1. Neural network visualizer:
   - ImGui window showing network topology
   - Node activations in real-time
   - Connection weights color-coded

2. Evolution statistics:
   - Fitness graph over generations
   - Species count over time
   - Innovation rate
   - Best genome topology display

3. Save/load system:
   - Save best genomes each generation
   - Load and replay specific creatures
   - Export evolution history

CHECKPOINT 6: Can visualize and debug neural network evolution

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/NEAT_RESEARCH.md (500+ lines)
- [ ] docs/NEAT_ARCHITECTURE.md (400+ lines)
- [ ] src/ai/neat/NEATGenome.h and .cpp
- [ ] src/ai/neat/Species.h and .cpp
- [ ] src/ai/neat/SpeciesManager.h and .cpp
- [ ] src/ai/neat/InnovationTracker.h and .cpp
- [ ] Brain input/output integration
- [ ] Fitness calculation
- [ ] Generation cycling
- [ ] Neural network visualizer (ImGui)
- [ ] Evolution statistics display
- [ ] Save/load for genomes
- [ ] Observable evolution across generations

ESTIMATED TIME: 4-6 hours with sub-agents
```

---

## Agent 5: Advanced Terrain and Biome System (4-6 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Enhance terrain with realistic biomes, vegetation, and dynamic weather.

============================================================================
PHASE 1: RESEARCH TERRAIN SYSTEMS (1+ hour)
============================================================================

SPAWN 3 SUB-AGENTS:

SUB-AGENT 1A - Research procedural terrain:
- Web search "procedural terrain generation heightmap"
- Web search "hydraulic erosion terrain algorithm"
- Web search "biome distribution climate simulation"
- Document advanced noise techniques (ridged, domain warping)

SUB-AGENT 1B - Research existing terrain code:
- Read Runtime/Shaders/Terrain.hlsl thoroughly (all 700+ lines)
- Read src/environment/Terrain.cpp (if exists)
- Document current biome system
- Document water system implementation

SUB-AGENT 1C - Research vegetation systems:
- Read src/environment/VegetationManager.cpp (if exists)
- Read src/environment/TreeGenerator.cpp (if exists)
- Read Runtime/Shaders/Vegetation.hlsl
- Web search "procedural vegetation placement games"

CHECKPOINT 1: Create docs/TERRAIN_RESEARCH.md (400+ lines)

============================================================================
PHASE 2: DESIGN ENHANCED BIOME SYSTEM (1+ hour)
============================================================================

Design multi-factor biome selection:

1. Climate simulation:
```cpp
struct ClimateData {
    float temperature;   // Based on latitude + elevation
    float moisture;      // Based on distance to water + wind
    float elevation;     // From heightmap
    float slope;         // Steepness

    BiomeType getBiome() const;
};

enum class BiomeType {
    DEEP_OCEAN, SHALLOW_WATER, BEACH,
    TROPICAL_RAINFOREST, TROPICAL_SEASONAL,
    TEMPERATE_FOREST, TEMPERATE_GRASSLAND,
    BOREAL_FOREST, TUNDRA, ICE,
    DESERT_HOT, DESERT_COLD,
    SAVANNA, SWAMP, MOUNTAIN_ROCK
};
```

2. Biome transition system:
```cpp
struct BiomeBlend {
    BiomeType primary;
    BiomeType secondary;
    float blendFactor;   // 0 = pure primary, 1 = pure secondary
    float noiseOffset;   // For natural-looking boundaries
};

BiomeBlend calculateBiomeBlend(const ClimateData& climate) {
    // Use Whittaker diagram logic
    // Temperature vs Precipitation determines biome
    // Add noise for natural boundaries
}
```

3. Vegetation density:
```cpp
struct VegetationDensity {
    float treeDensity;       // 0-1
    float grassDensity;      // 0-1
    float flowerDensity;     // 0-1
    float shrubDensity;      // 0-1

    static VegetationDensity forBiome(BiomeType biome);
};
```

CHECKPOINT 2: Create docs/BIOME_DESIGN.md (300+ lines)

============================================================================
PHASE 3: IMPLEMENT TERRAIN IMPROVEMENTS (1.5+ hours)
============================================================================

SPAWN 3 SUB-AGENTS:

SUB-AGENT 3A - Implement erosion simulation:
Create src/environment/TerrainErosion.cpp:
```cpp
class TerrainErosion {
public:
    void simulateHydraulicErosion(Heightmap& heightmap, int iterations);
    void simulateThermalErosion(Heightmap& heightmap, float talusAngle);

private:
    void dropletErosion(Heightmap& heightmap, glm::vec2 startPos);
    void depositSediment(Heightmap& heightmap, glm::vec2 pos, float amount);
};
```

SUB-AGENT 3B - Implement climate simulation:
Create src/environment/ClimateSystem.cpp:
```cpp
class ClimateSystem {
    float worldLatitude = 45.0f;  // Affects temperature
    glm::vec2 prevailingWind = {1, 0};

public:
    ClimateData getClimateAt(const glm::vec3& worldPos, const Terrain& terrain);
    void simulateMoisture(Terrain& terrain);  // Rain shadow, etc.
    void updateSeasons(float dayOfYear);
};
```

SUB-AGENT 3C - Enhance Terrain.hlsl biomes:
Add to Runtime/Shaders/Terrain.hlsl:
- New biome color functions (tropical, boreal, desert, etc.)
- Climate-based biome selection
- Improved transitions using noise
- Seasonal color variations

CHECKPOINT 3: Terrain shows varied biomes based on climate

============================================================================
PHASE 4: IMPLEMENT VEGETATION SYSTEM (1+ hour)
============================================================================

Create comprehensive vegetation:

1. Tree generation:
```cpp
struct TreeType {
    string name;           // "Oak", "Pine", "Palm", etc.
    float minHeight, maxHeight;
    float trunkRadius;
    int branchLevels;
    LeafType leafType;     // BROADLEAF, NEEDLE, PALM
    std::vector<BiomeType> validBiomes;
};

class TreeGenerator {
    void generateTree(TreeType type, glm::vec3 position, MeshData& outMesh);
    void generateLSystem(const string& rules, int iterations, MeshData& outMesh);
};
```

2. Grass and ground cover:
```cpp
class GrassSystem {
    // GPU instanced grass blades
    void generateGrassField(const Terrain& terrain, BiomeType biome);
    void updateWind(float time);  // Animate grass
};
```

3. Vegetation placement:
```cpp
class VegetationPlacer {
    void placeVegetation(const Terrain& terrain, const ClimateSystem& climate) {
        for (each terrain cell) {
            BiomeType biome = climate.getClimateAt(pos).getBiome();
            VegetationDensity density = VegetationDensity::forBiome(biome);

            // Poisson disk sampling for tree placement
            if (random() < density.treeDensity) {
                TreeType type = selectTreeForBiome(biome);
                placeTree(type, pos);
            }

            // Similar for grass, flowers, shrubs
        }
    }
};
```

CHECKPOINT 4: Varied vegetation across biomes

============================================================================
PHASE 5: IMPLEMENT WEATHER SYSTEM (1+ hour)
============================================================================

SPAWN 2 SUB-AGENTS:

SUB-AGENT 5A - Weather simulation:
Create src/environment/WeatherSystem.cpp:
```cpp
enum class WeatherType {
    CLEAR, PARTLY_CLOUDY, OVERCAST,
    RAIN_LIGHT, RAIN_HEAVY, THUNDERSTORM,
    SNOW_LIGHT, SNOW_HEAVY, BLIZZARD,
    FOG, SANDSTORM
};

class WeatherSystem {
    WeatherType currentWeather;
    float cloudCoverage;
    float precipitation;
    float windStrength;
    glm::vec2 windDirection;

    void update(float dt);
    void transitionTo(WeatherType target, float duration);
    WeatherType getWeatherForClimate(const ClimateData& climate);
};
```

SUB-AGENT 5B - Weather rendering:
Add to shaders:
- Rain particle system
- Snow particle system
- Dynamic cloud shadows
- Fog density based on weather
- Wet surface reflections after rain

CHECKPOINT 5: Weather affects visuals and potentially creature behavior

============================================================================
PHASE 6: DAY/NIGHT AND SEASONS (30+ minutes)
============================================================================

Enhance existing day/night cycle:

1. Seasonal changes:
```cpp
struct Season {
    string name;
    float dayLength;      // Hours of daylight
    float temperatureMod; // Add to base temperature
    float3 sunTint;       // Warmer in summer, cooler in winter
    float vegetationState; // 0=winter, 1=full summer
};

class SeasonManager {
    float yearProgress;   // 0-1, where 0.25 = spring, 0.5 = summer, etc.
    Season getCurrentSeason();
    void update(float dt);
};
```

2. Vegetation seasonal changes:
- Spring: flowers bloom, leaves grow
- Summer: full foliage
- Autumn: leaves change color, fall
- Winter: bare trees, snow coverage

3. Update shaders for seasons:
- Grass color changes with season
- Tree foliage density
- Snow accumulation

CHECKPOINT 6: Visible seasonal changes over time

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/TERRAIN_RESEARCH.md (400+ lines)
- [ ] docs/BIOME_DESIGN.md (300+ lines)
- [ ] src/environment/TerrainErosion.cpp
- [ ] src/environment/ClimateSystem.cpp
- [ ] Enhanced Terrain.hlsl with new biomes
- [ ] src/environment/TreeGenerator.cpp (enhanced)
- [ ] src/environment/GrassSystem.cpp
- [ ] src/environment/VegetationPlacer.cpp
- [ ] src/environment/WeatherSystem.cpp
- [ ] Weather rendering (rain, snow, fog)
- [ ] Seasonal changes
- [ ] Visually varied terrain with multiple biomes

ESTIMATED TIME: 4-6 hours with sub-agents
```

---

## Agent 6: Performance Optimization and Scaling (4-6 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Optimize for 10,000+ creatures at 60 FPS using GPU compute and instancing.

============================================================================
PHASE 1: PERFORMANCE PROFILING (1+ hour)
============================================================================

SPAWN 3 SUB-AGENTS:

SUB-AGENT 1A - Profile current performance:
- Add frame timing to main.cpp
- Measure: CPU time, GPU time, draw calls, memory
- Identify bottlenecks (CPU-bound vs GPU-bound)
- Count current creatures and measure FPS

SUB-AGENT 1B - Analyze rendering pipeline:
- Read src/main.cpp rendering code
- Count draw calls per frame
- Identify redundant state changes
- Document constant buffer update patterns

SUB-AGENT 1C - Analyze simulation code:
- Read creature update loop
- Measure time per creature update
- Identify O(n²) operations (collision, AI)
- Document memory access patterns

CHECKPOINT 1: Create docs/PERFORMANCE_ANALYSIS.md (300+ lines)

============================================================================
PHASE 2: DESIGN OPTIMIZATION STRATEGY (1+ hour)
============================================================================

Plan optimizations:

1. GPU instanced rendering:
```cpp
struct CreatureInstanceData {
    Mat4 modelMatrix;      // 64 bytes
    Vec4 color;            // 16 bytes
    Vec4 parameters;       // 16 bytes (size, speed, energy, etc.)
};  // Total: 96 bytes per instance

// Single draw call for all creatures of same mesh type
void renderCreaturesInstanced(const std::vector<Creature*>& creatures) {
    // Group by mesh type
    // Upload instance buffer
    // Single DrawIndexedInstanced call
}
```

2. GPU compute for AI:
```cpp
// Compute shader processes all creatures in parallel
// SteeringCompute.hlsl already exists - use it

struct CreatureComputeData {
    Vec3 position;
    Vec3 velocity;
    Vec3 targetDirection;
    float energy;
    // ... other AI-relevant data
};

// Upload all creature data
// Dispatch compute shader
// Read back updated velocities/targets
```

3. Spatial partitioning:
```cpp
class SpatialGrid {
    float cellSize;
    std::vector<std::vector<Creature*>> cells;

    void insert(Creature* creature);
    void update();  // Rebuild each frame
    std::vector<Creature*> queryNearby(Vec3 pos, float radius);
};
```

4. LOD system:
```cpp
enum class LODLevel {
    FULL,       // < 50 units - full mesh, full animation
    REDUCED,    // 50-150 units - simplified mesh
    BILLBOARD,  // > 150 units - 2D sprite
    CULLED      // Outside frustum - don't render
};

LODLevel calculateLOD(const Creature& creature, const Camera& camera);
```

CHECKPOINT 2: Create docs/OPTIMIZATION_PLAN.md (400+ lines)

============================================================================
PHASE 3: IMPLEMENT INSTANCED RENDERING (1.5+ hours)
============================================================================

SPAWN 2 SUB-AGENTS:

SUB-AGENT 3A - Implement instance buffer system:
Create src/graphics/InstanceBuffer.h and .cpp:
```cpp
class InstanceBuffer {
    ID3D12Resource* buffer;
    void* mappedData;
    size_t instanceSize;
    size_t maxInstances;
    size_t currentCount;

public:
    void create(ID3D12Device* device, size_t instanceSize, size_t maxInstances);
    void* beginUpdate();
    void endUpdate(size_t count);
    void bind(ID3D12GraphicsCommandList* cmdList, UINT slot);
};
```

SUB-AGENT 3B - Modify creature rendering:
Update src/main.cpp:
- Group creatures by mesh type
- Fill instance buffer each frame
- Replace individual draw calls with DrawIndexedInstanced

Update Creature.hlsl:
- Read instance data from StructuredBuffer
- Use SV_InstanceID to index

CHECKPOINT 3: Creatures render with single draw call per mesh type

============================================================================
PHASE 4: IMPLEMENT GPU COMPUTE AI (1+ hour)
============================================================================

Enhance GPU-based steering:

1. Update shaders/hlsl/SteeringCompute.hlsl:
```hlsl
struct CreatureData {
    float3 position;
    float3 velocity;
    float3 forward;
    float energy;
    int creatureType;
    int targetId;
    float padding[2];
};

RWStructuredBuffer<CreatureData> creatures : register(u0);
StructuredBuffer<float4> foodPositions : register(t0);

[numthreads(256, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID) {
    if (id.x >= creatureCount) return;

    CreatureData self = creatures[id.x];

    // Find nearest food
    float3 foodDir = findNearestFood(self.position);

    // Find nearest predator/prey
    float3 threatDir = findNearestThreat(self.position, self.creatureType);

    // Calculate steering
    float3 steering = calculateSteering(self, foodDir, threatDir);

    // Update velocity
    creatures[id.x].velocity = normalize(self.velocity + steering) * maxSpeed;
}
```

2. Implement dispatch in main.cpp:
```cpp
void updateCreaturesGPU() {
    // Upload creature data to GPU buffer
    uploadCreatureData();

    // Set compute root signature
    cmdList->SetComputeRootSignature(computeRootSig);

    // Bind buffers
    cmdList->SetComputeRootUnorderedAccessView(0, creatureBuffer);
    cmdList->SetComputeRootShaderResourceView(1, foodBuffer);

    // Dispatch
    UINT groupCount = (creatureCount + 255) / 256;
    cmdList->Dispatch(groupCount, 1, 1);

    // Barrier
    cmdList->ResourceBarrier(...);

    // Read back (or keep on GPU for rendering)
}
```

CHECKPOINT 4: AI runs on GPU, CPU load reduced

============================================================================
PHASE 5: IMPLEMENT SPATIAL PARTITIONING (1+ hour)
============================================================================

SPAWN 2 SUB-AGENTS:

SUB-AGENT 5A - Implement SpatialGrid:
Create src/utils/SpatialGrid.h and .cpp (or enhance existing):
```cpp
template<typename T>
class SpatialGrid {
    float cellSize;
    int gridWidth, gridHeight;
    std::vector<std::vector<T*>> cells;

public:
    void clear();
    void insert(T* item, const Vec3& position);
    void queryRadius(const Vec3& center, float radius,
                     std::vector<T*>& results);
    void queryFrustum(const Frustum& frustum,
                      std::vector<T*>& results);
};
```

SUB-AGENT 5B - Integrate spatial grid:
Update collision detection:
- Use grid for creature-creature collision
- Use grid for creature-food collision
- Use grid for frustum culling

Measure improvement:
- Before: O(n²) collision checks
- After: O(n * k) where k = avg creatures per cell

CHECKPOINT 5: Collision detection scales linearly

============================================================================
PHASE 6: IMPLEMENT LOD AND CULLING (30+ minutes)
============================================================================

1. Frustum culling:
```cpp
bool isInFrustum(const Vec3& position, float radius, const Frustum& frustum) {
    for (int i = 0; i < 6; i++) {
        if (frustum.planes[i].distanceToPoint(position) < -radius) {
            return false;
        }
    }
    return true;
}
```

2. LOD selection:
```cpp
void updateCreatureLODs(const Camera& camera) {
    for (Creature* c : creatures) {
        float dist = distance(c->position, camera.position);

        if (!isInFrustum(c->position, c->radius, camera.frustum)) {
            c->lod = LOD_CULLED;
        } else if (dist < 50.0f) {
            c->lod = LOD_FULL;
        } else if (dist < 150.0f) {
            c->lod = LOD_REDUCED;
        } else {
            c->lod = LOD_BILLBOARD;
        }
    }
}
```

3. Group rendering by LOD:
- Full LOD: Complete mesh
- Reduced: Lower poly mesh
- Billboard: Single quad facing camera

CHECKPOINT 6: Performance scales to 10,000+ creatures

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/PERFORMANCE_ANALYSIS.md (300+ lines)
- [ ] docs/OPTIMIZATION_PLAN.md (400+ lines)
- [ ] src/graphics/InstanceBuffer.h and .cpp
- [ ] Instanced creature rendering
- [ ] Enhanced SteeringCompute.hlsl
- [ ] GPU compute AI dispatch
- [ ] src/utils/SpatialGrid.h and .cpp (enhanced)
- [ ] Frustum culling
- [ ] LOD system
- [ ] Benchmark: 10,000+ creatures at 60 FPS
- [ ] Performance metrics display in ImGui

ESTIMATED TIME: 4-6 hours with sub-agents
```

---

## Agent 7: Save/Load and Replay System (3-4 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Implement comprehensive save/load system with replay functionality.

============================================================================
PHASE 1: RESEARCH SERIALIZATION (45+ minutes)
============================================================================

SPAWN 2 SUB-AGENTS:

SUB-AGENT 1A - Research serialization approaches:
- Web search "C++ game state serialization"
- Web search "binary vs JSON game save format"
- Web search "protobuf game development"
- Compare: JSON, binary, MessagePack, protobuf

SUB-AGENT 1B - Analyze current data structures:
- Read src/entities/Creature.h - what needs saving
- Read src/entities/Genome.h - genetic data
- Read src/ai/neat/NEATGenome.h (if exists)
- Read src/environment/Terrain.h
- Document all state that needs persistence

CHECKPOINT 1: Create docs/SERIALIZATION_RESEARCH.md (200+ lines)

============================================================================
PHASE 2: DESIGN SAVE FILE FORMAT (45+ minutes)
============================================================================

Design save system:

1. Save file structure:
```cpp
struct SaveFileHeader {
    char magic[4] = {'E','V','O','S'};  // "Evolution Save"
    uint32_t version;
    uint64_t timestamp;
    uint32_t creatureCount;
    uint32_t foodCount;
    uint32_t generation;
    float simulationTime;
};

struct CreatureSaveData {
    uint32_t id;
    CreatureType type;
    Vec3 position;
    Vec3 velocity;
    float energy;
    float health;
    float age;
    GenomeSaveData genome;
    NEATGenomeSaveData brain;  // If using NEAT
};

struct WorldSaveData {
    uint64_t randomSeed;
    float dayTime;
    SeasonType season;
    WeatherType weather;
    // Terrain is regenerated from seed
};
```

2. Replay system:
```cpp
struct ReplayFrame {
    float timestamp;
    std::vector<CreatureSnapshot> creatures;
    std::vector<FoodSnapshot> food;
    CameraState camera;
};

struct ReplayData {
    SaveFileHeader header;
    WorldSaveData world;
    std::vector<ReplayFrame> frames;  // Every N seconds
};
```

CHECKPOINT 2: Create docs/SAVE_FORMAT.md (200+ lines)

============================================================================
PHASE 3: IMPLEMENT SAVE/LOAD (1.5+ hours)
============================================================================

SPAWN 3 SUB-AGENTS:

SUB-AGENT 3A - Implement binary serialization:
Create src/utils/Serializer.h and .cpp:
```cpp
class BinaryWriter {
    std::ofstream file;
public:
    void write(const void* data, size_t size);
    void writeString(const std::string& str);
    template<typename T> void write(const T& value);
    template<typename T> void writeVector(const std::vector<T>& vec);
};

class BinaryReader {
    std::ifstream file;
public:
    void read(void* data, size_t size);
    std::string readString();
    template<typename T> T read();
    template<typename T> std::vector<T> readVector();
};
```

SUB-AGENT 3B - Implement creature serialization:
Add to Creature class:
```cpp
void Creature::serialize(BinaryWriter& writer) const {
    writer.write(id);
    writer.write(static_cast<int>(type));
    writer.write(position);
    writer.write(velocity);
    writer.write(energy);
    writer.write(health);
    writer.write(age);
    genome.serialize(writer);
    if (brain) brain->serialize(writer);
}

void Creature::deserialize(BinaryReader& reader) {
    id = reader.read<uint32_t>();
    type = static_cast<CreatureType>(reader.read<int>());
    position = reader.read<Vec3>();
    // ... etc
}
```

SUB-AGENT 3C - Implement world save/load:
Create src/core/SaveManager.h and .cpp:
```cpp
class SaveManager {
public:
    bool saveGame(const std::string& filename, const World& world);
    bool loadGame(const std::string& filename, World& world);

    // Auto-save
    void enableAutoSave(float intervalSeconds);
    void update(float dt);

    // Save slots
    std::vector<SaveSlotInfo> listSaveSlots();
    bool deleteSave(const std::string& filename);
};
```

CHECKPOINT 3: Can save and load full game state

============================================================================
PHASE 4: IMPLEMENT REPLAY SYSTEM (1+ hour)
============================================================================

Create replay functionality:

1. Recording:
```cpp
class ReplayRecorder {
    std::vector<ReplayFrame> frames;
    float recordInterval = 1.0f;  // Record every second
    float timeSinceLastRecord = 0.0f;

public:
    void update(float dt, const World& world) {
        timeSinceLastRecord += dt;
        if (timeSinceLastRecord >= recordInterval) {
            recordFrame(world);
            timeSinceLastRecord = 0.0f;
        }
    }

    void recordFrame(const World& world) {
        ReplayFrame frame;
        frame.timestamp = world.simulationTime;

        for (const Creature* c : world.creatures) {
            frame.creatures.push_back(c->getSnapshot());
        }

        frames.push_back(frame);
    }

    void saveReplay(const std::string& filename);
};
```

2. Playback:
```cpp
class ReplayPlayer {
    ReplayData replay;
    size_t currentFrame = 0;
    float playbackTime = 0.0f;
    float playbackSpeed = 1.0f;
    bool paused = false;

public:
    void loadReplay(const std::string& filename);
    void update(float dt);
    void seek(float time);
    void setSpeed(float speed);
    void togglePause();

    const ReplayFrame& getCurrentFrame() const;
    float getProgress() const;  // 0-1
};
```

3. Replay rendering:
- Interpolate creature positions between frames
- Show timeline scrubber in ImGui
- Allow speed control (0.5x, 1x, 2x, 4x)

CHECKPOINT 4: Can record and replay simulations

============================================================================
PHASE 5: UI INTEGRATION (30+ minutes)
============================================================================

Create save/load UI:

1. Main menu:
```cpp
void renderMainMenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Simulation")) newSimulation();
            if (ImGui::MenuItem("Save", "Ctrl+S")) saveGame();
            if (ImGui::MenuItem("Load", "Ctrl+L")) showLoadDialog();
            if (ImGui::MenuItem("Quick Save", "F5")) quickSave();
            if (ImGui::MenuItem("Quick Load", "F9")) quickLoad();
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) requestExit();
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
```

2. Save/Load dialog:
- List available saves with timestamp, creature count, generation
- Preview thumbnail (optional)
- Confirm overwrite for save
- Confirm load (loses current)

3. Replay controls:
- Timeline bar
- Play/pause button
- Speed slider
- Frame step buttons

CHECKPOINT 5: Functional save/load/replay UI

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/SERIALIZATION_RESEARCH.md (200+ lines)
- [ ] docs/SAVE_FORMAT.md (200+ lines)
- [ ] src/utils/Serializer.h and .cpp
- [ ] Creature::serialize/deserialize
- [ ] Genome::serialize/deserialize
- [ ] src/core/SaveManager.h and .cpp
- [ ] Auto-save functionality
- [ ] src/core/ReplayRecorder.h and .cpp
- [ ] src/core/ReplayPlayer.h and .cpp
- [ ] Save/Load UI
- [ ] Replay timeline UI
- [ ] Keyboard shortcuts (F5/F9, Ctrl+S/L)
- [ ] Working save/load/replay system

ESTIMATED TIME: 3-4 hours with sub-agents
```

---

## Agent 8: Shadow Mapping and Advanced Lighting (3-4 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Implement cascaded shadow maps and enhance lighting for visual quality.

============================================================================
PHASE 1: RESEARCH SHADOW TECHNIQUES (45+ minutes)
============================================================================

SPAWN 2 SUB-AGENTS:

SUB-AGENT 1A - Research shadow mapping:
- Web search "cascaded shadow maps DirectX 12"
- Web search "PCF shadow filtering"
- Web search "variance shadow maps"
- Document CSM split schemes (logarithmic, PSSM)

SUB-AGENT 1B - Analyze existing lighting:
- Read Runtime/Shaders/Terrain.hlsl lighting code
- Read Runtime/Shaders/Creature.hlsl lighting
- Read src/graphics/ShadowMap_DX12.h (if exists)
- Document current PBR implementation

CHECKPOINT 1: Create docs/SHADOW_RESEARCH.md (200+ lines)

============================================================================
PHASE 2: DESIGN SHADOW SYSTEM (45+ minutes)
============================================================================

Design cascaded shadow maps:

1. Shadow map structure:
```cpp
struct ShadowCascade {
    Mat4 viewProjection;
    float splitDistance;
    ID3D12Resource* depthTexture;
    D3D12_CPU_DESCRIPTOR_HANDLE dsv;
    D3D12_GPU_DESCRIPTOR_HANDLE srv;
};

class CascadedShadowMap {
    static const int CASCADE_COUNT = 4;
    ShadowCascade cascades[CASCADE_COUNT];
    int shadowMapSize = 2048;

    void calculateSplits(const Camera& camera, float nearPlane, float farPlane);
    void calculateLightMatrices(const Vec3& lightDir, const Camera& camera);
    void render(ID3D12GraphicsCommandList* cmdList, const Scene& scene);
};
```

2. Shadow filtering:
```cpp
// PCF (Percentage Closer Filtering)
float sampleShadowPCF(Texture2D shadowMap, SamplerComparisonState sampler,
                      float3 shadowCoord, float bias);

// PCSS (Percentage Closer Soft Shadows) - optional
float sampleShadowPCSS(Texture2D shadowMap, float3 shadowCoord,
                       float lightSize, float receiverDepth);
```

CHECKPOINT 2: Create docs/SHADOW_DESIGN.md (200+ lines)

============================================================================
PHASE 3: IMPLEMENT SHADOW MAPS (1.5+ hours)
============================================================================

SPAWN 2 SUB-AGENTS:

SUB-AGENT 3A - Implement C++ shadow system:
Create or enhance src/graphics/ShadowMap_DX12.cpp:
```cpp
class CascadedShadowMap {
public:
    void initialize(ID3D12Device* device);
    void updateCascades(const Camera& camera, const Vec3& sunDirection);

    void beginShadowPass(ID3D12GraphicsCommandList* cmdList, int cascade);
    void endShadowPass(ID3D12GraphicsCommandList* cmdList, int cascade);

    void bindForSampling(ID3D12GraphicsCommandList* cmdList, int rootSlot);

    const Mat4& getCascadeViewProj(int cascade) const;
    float getCascadeSplit(int cascade) const;

private:
    void createDepthTextures(ID3D12Device* device);
    void calculateCascadeSplits(float nearPlane, float farPlane);
    Mat4 calculateLightViewProj(const Frustum& cascadeFrustum, const Vec3& lightDir);
};
```

SUB-AGENT 3B - Create shadow shaders:
Create shaders/hlsl/Shadow.hlsl:
```hlsl
// Vertex shader - simple depth only
float4 VSMain(float3 position : POSITION) : SV_POSITION {
    return mul(lightViewProj, mul(model, float4(position, 1.0)));
}

// No pixel shader needed for depth-only pass
// (Or use empty PS if required)
```

Create shadow sampling functions for Terrain.hlsl and Creature.hlsl:
```hlsl
Texture2DArray shadowMaps : register(t1);
SamplerComparisonState shadowSampler : register(s1);

float4 cascadeSplits;
float4x4 cascadeViewProjs[4];

float calculateShadow(float3 worldPos, float3 normal, float3 lightDir) {
    // Determine cascade
    float viewDepth = length(worldPos - viewPos);
    int cascade = 3;
    for (int i = 0; i < 4; i++) {
        if (viewDepth < cascadeSplits[i]) {
            cascade = i;
            break;
        }
    }

    // Transform to shadow space
    float4 shadowCoord = mul(cascadeViewProjs[cascade], float4(worldPos, 1.0));
    shadowCoord.xyz /= shadowCoord.w;
    shadowCoord.xy = shadowCoord.xy * 0.5 + 0.5;
    shadowCoord.y = 1.0 - shadowCoord.y;

    // Apply bias
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);

    // Sample with PCF
    float shadow = 0.0;
    float2 texelSize = 1.0 / float2(2048, 2048);
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float2 offset = float2(x, y) * texelSize;
            shadow += shadowMaps.SampleCmpLevelZero(
                shadowSampler,
                float3(shadowCoord.xy + offset, cascade),
                shadowCoord.z - bias
            );
        }
    }
    return shadow / 9.0;
}
```

CHECKPOINT 3: Shadow maps render and sample correctly

============================================================================
PHASE 4: INTEGRATE SHADOWS INTO LIGHTING (45+ minutes)
============================================================================

Update all shaders to use shadows:

1. Terrain.hlsl:
```hlsl
// In PSMain, before final lighting:
float shadow = calculateShadow(input.worldPos, norm, lightDir);

// Apply shadow to direct lighting only
float3 directLight = calculatePBR(...) * shadow;
float3 ambient = calculateAmbientIBL(...);  // No shadow on ambient
result = directLight + ambient;
```

2. Creature.hlsl:
```hlsl
// Same pattern
float shadow = calculateShadow(input.fragPos, norm, lightDir);
float3 lit = (diffuse + specular) * shadow + ambient;
```

3. Vegetation.hlsl:
```hlsl
// Shadows on vegetation too
float shadow = calculateShadow(worldPos, normal, lightDir);
```

CHECKPOINT 4: All objects receive and cast shadows

============================================================================
PHASE 5: ENHANCE LIGHTING QUALITY (45+ minutes)
============================================================================

Add advanced lighting features:

1. Ambient occlusion (screen-space or baked):
```hlsl
// Simple SSAO approximation using depth buffer
float calculateSSAO(float2 uv, float depth, float3 normal) {
    float occlusion = 0.0;
    float radius = 0.5;

    for (int i = 0; i < 16; i++) {
        float3 sampleDir = hemisphereKernel[i];
        // ... SSAO calculation
    }

    return 1.0 - occlusion;
}
```

2. Bloom for bright areas:
```cpp
class BloomEffect {
    void extractBright(ID3D12Resource* hdrTarget, ID3D12Resource* brightTarget);
    void blur(ID3D12Resource* target, int passes);
    void composite(ID3D12Resource* scene, ID3D12Resource* bloom);
};
```

3. Color grading/tone mapping:
```hlsl
// ACES filmic tone mapping
float3 ACESFilm(float3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x*(a*x+b))/(x*(c*x+d)+e));
}
```

CHECKPOINT 5: Visually polished lighting with shadows, AO, bloom

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/SHADOW_RESEARCH.md (200+ lines)
- [ ] docs/SHADOW_DESIGN.md (200+ lines)
- [ ] src/graphics/ShadowMap_DX12.cpp (enhanced)
- [ ] shaders/hlsl/Shadow.hlsl
- [ ] Shadow sampling in Terrain.hlsl
- [ ] Shadow sampling in Creature.hlsl
- [ ] Shadow sampling in Vegetation.hlsl
- [ ] Cascaded shadow map working
- [ ] PCF shadow filtering
- [ ] Optional: SSAO
- [ ] Optional: Bloom
- [ ] Tone mapping
- [ ] Visually improved scene with shadows

ESTIMATED TIME: 3-4 hours with sub-agents
```

---

## Agent 9: Comprehensive UI and Dashboard (3-4 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Build comprehensive ImGui dashboard with statistics, graphs, and controls.

============================================================================
PHASE 1: RESEARCH UI REQUIREMENTS (45+ minutes)
============================================================================

SPAWN 2 SUB-AGENTS:

SUB-AGENT 1A - Research ImGui features:
- Web search "ImGui plotting graphs"
- Web search "implot library charts"
- Web search "ImGui DX12 integration"
- Document ImGui widgets available

SUB-AGENT 1B - Analyze current UI:
- Read src/main.cpp ImGui code
- Read src/ui/ImGuiManager.cpp (if exists)
- Document current UI state
- List missing features

CHECKPOINT 1: Create docs/UI_RESEARCH.md (150+ lines)

============================================================================
PHASE 2: DESIGN DASHBOARD LAYOUT (45+ minutes)
============================================================================

Design comprehensive dashboard:

1. Main panels:
```
+------------------+------------------------+------------------+
|  Simulation      |                        |  Selected        |
|  Controls        |     Main Viewport      |  Creature        |
|  - Speed         |                        |  Details         |
|  - Pause/Play    |                        |  - Stats         |
|  - Time of Day   |                        |  - Genome        |
|  - Weather       |                        |  - Neural Net    |
+------------------+                        +------------------+
|  Population      |                        |  Evolution       |
|  Stats           |                        |  Graphs          |
|  - Herbivores    |                        |  - Fitness       |
|  - Carnivores    |                        |  - Population    |
|  - Deaths        |                        |  - Generations   |
+------------------+------------------------+------------------+
|                      Status Bar                              |
|  FPS: 60 | Creatures: 1000 | Generation: 50 | Time: 12:34   |
+-------------------------------------------------------------+
```

2. Dockable windows:
- Simulation Controls
- Population Statistics
- Creature Inspector
- Evolution Graphs
- Neural Network Viewer
- Performance Monitor
- Console/Log

CHECKPOINT 2: Create docs/UI_DESIGN.md (200+ lines)

============================================================================
PHASE 3: IMPLEMENT CORE UI PANELS (1.5+ hours)
============================================================================

SPAWN 3 SUB-AGENTS:

SUB-AGENT 3A - Implement Simulation Controls:
```cpp
void renderSimulationControls() {
    ImGui::Begin("Simulation Controls");

    // Time control
    ImGui::Text("Time Scale");
    ImGui::SliderFloat("##timescale", &timeScale, 0.0f, 10.0f);
    if (ImGui::Button(isPaused ? "Play" : "Pause")) {
        isPaused = !isPaused;
    }
    ImGui::SameLine();
    if (ImGui::Button("Step")) {
        stepOneFrame = true;
    }

    ImGui::Separator();

    // Day/Night
    ImGui::Text("Time of Day");
    ImGui::SliderFloat("##daytime", &dayTime, 0.0f, 24.0f, "%.1f hours");

    // Weather
    ImGui::Text("Weather");
    const char* weathers[] = {"Clear", "Cloudy", "Rain", "Storm", "Snow"};
    ImGui::Combo("##weather", &currentWeather, weathers, 5);

    ImGui::Separator();

    // Spawn controls
    if (ImGui::Button("Spawn Herbivore")) spawnCreature(HERBIVORE);
    if (ImGui::Button("Spawn Carnivore")) spawnCreature(CARNIVORE);
    if (ImGui::Button("Spawn Food (x10)")) spawnFood(10);

    ImGui::Separator();

    // Chaos buttons
    if (ImGui::Button("Kill All Carnivores")) killAllOfType(CARNIVORE);
    if (ImGui::Button("Mass Extinction (50%)")) massExtinction(0.5f);
    if (ImGui::Button("Food Boom")) spawnFood(100);

    ImGui::End();
}
```

SUB-AGENT 3B - Implement Population Statistics:
```cpp
void renderPopulationStats() {
    ImGui::Begin("Population Statistics");

    // Current counts
    ImGui::Text("Herbivores: %d", herbivoreCount);
    ImGui::Text("Carnivores: %d", carnivoreCount);
    ImGui::Text("Food: %d", foodCount);
    ImGui::Text("Total Creatures: %d", totalCreatures);

    ImGui::Separator();

    // Birth/Death rates
    ImGui::Text("Births this minute: %d", birthsPerMinute);
    ImGui::Text("Deaths this minute: %d", deathsPerMinute);
    ImGui::Text("Growth Rate: %.2f%%", growthRate * 100);

    ImGui::Separator();

    // Species diversity (if NEAT)
    ImGui::Text("Species Count: %d", speciesCount);
    ImGui::Text("Average Fitness: %.2f", averageFitness);
    ImGui::Text("Best Fitness: %.2f", bestFitness);

    ImGui::Separator();

    // Average stats
    ImGui::Text("Avg Herbivore Energy: %.1f", avgHerbivoreEnergy);
    ImGui::Text("Avg Carnivore Energy: %.1f", avgCarnivoreEnergy);
    ImGui::Text("Avg Creature Age: %.1f", avgAge);

    ImGui::End();
}
```

SUB-AGENT 3C - Implement Creature Inspector:
```cpp
void renderCreatureInspector() {
    ImGui::Begin("Creature Inspector");

    if (selectedCreature == nullptr) {
        ImGui::Text("Click a creature to select");
        ImGui::End();
        return;
    }

    Creature* c = selectedCreature;

    // Basic info
    ImGui::Text("ID: %d", c->id);
    ImGui::Text("Type: %s", creatureTypeNames[c->type]);
    ImGui::Text("Generation: %d", c->generation);

    ImGui::Separator();

    // Stats
    ImGui::ProgressBar(c->energy / c->maxEnergy, ImVec2(-1, 0), "Energy");
    ImGui::ProgressBar(c->health / c->maxHealth, ImVec2(-1, 0), "Health");
    ImGui::Text("Age: %.1f seconds", c->age);
    ImGui::Text("Position: (%.1f, %.1f, %.1f)", c->position.x, c->position.y, c->position.z);
    ImGui::Text("Speed: %.2f", length(c->velocity));

    ImGui::Separator();

    // Genome
    if (ImGui::CollapsingHeader("Genome")) {
        ImGui::Text("Size: %.2f", c->genome.size);
        ImGui::Text("Speed Gene: %.2f", c->genome.speed);
        ImGui::Text("Sense Range: %.2f", c->genome.senseRange);
        ImGui::ColorEdit3("Color", &c->genome.color.r);
        // ... more genome traits
    }

    // Actions
    ImGui::Separator();
    if (ImGui::Button("Follow Camera")) followCreature(c);
    if (ImGui::Button("Kill")) c->energy = 0;
    if (ImGui::Button("Clone")) cloneCreature(c);

    ImGui::End();
}
```

CHECKPOINT 3: Core UI panels functional

============================================================================
PHASE 4: IMPLEMENT GRAPHS AND CHARTS (1+ hour)
============================================================================

Use ImPlot or custom rendering for graphs:

1. Population over time:
```cpp
void renderPopulationGraph() {
    ImGui::Begin("Evolution Graphs");

    if (ImPlot::BeginPlot("Population Over Time")) {
        ImPlot::SetupAxes("Time (generations)", "Count");
        ImPlot::PlotLine("Herbivores", generationTimes.data(),
                         herbivoreHistory.data(), historySize);
        ImPlot::PlotLine("Carnivores", generationTimes.data(),
                         carnivoreHistory.data(), historySize);
        ImPlot::EndPlot();
    }

    if (ImPlot::BeginPlot("Average Fitness")) {
        ImPlot::SetupAxes("Generation", "Fitness");
        ImPlot::PlotLine("Average", generationNumbers.data(),
                         avgFitnessHistory.data(), historySize);
        ImPlot::PlotLine("Best", generationNumbers.data(),
                         bestFitnessHistory.data(), historySize);
        ImPlot::EndPlot();
    }

    ImGui::End();
}
```

2. Trait distribution histograms:
```cpp
void renderTraitDistribution() {
    // Collect size distribution
    std::vector<float> sizes;
    for (Creature* c : creatures) {
        sizes.push_back(c->genome.size);
    }

    if (ImPlot::BeginPlot("Size Distribution")) {
        ImPlot::SetupAxes("Size", "Count");
        ImPlot::PlotHistogram("Creatures", sizes.data(), sizes.size(), 20);
        ImPlot::EndPlot();
    }
}
```

3. Neural network visualization:
```cpp
void renderNeuralNetwork(NEATGenome& genome) {
    ImGui::Begin("Neural Network");

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 offset = ImGui::GetCursorScreenPos();

    // Draw nodes
    for (const NodeGene& node : genome.nodes) {
        float x = nodePositions[node.id].x * 200 + offset.x;
        float y = nodePositions[node.id].y * 300 + offset.y;
        ImU32 color = getNodeColor(node.type, node.activation);
        drawList->AddCircleFilled(ImVec2(x, y), 10, color);
    }

    // Draw connections
    for (const ConnectionGene& conn : genome.connections) {
        if (!conn.enabled) continue;
        ImVec2 start = getNodeScreenPos(conn.inNode);
        ImVec2 end = getNodeScreenPos(conn.outNode);
        ImU32 color = conn.weight > 0 ? IM_COL32(0,255,0,128) : IM_COL32(255,0,0,128);
        float thickness = abs(conn.weight) * 3.0f;
        drawList->AddLine(start, end, color, thickness);
    }

    ImGui::End();
}
```

CHECKPOINT 4: Graphs and visualizations working

============================================================================
PHASE 5: POLISH AND FEATURES (30+ minutes)
============================================================================

Add finishing touches:

1. Docking and layout saving:
```cpp
void setupDocking() {
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Load saved layout
    ImGui::LoadIniSettingsFromDisk("imgui_layout.ini");
}

void saveDocking() {
    ImGui::SaveIniSettingsToDisk("imgui_layout.ini");
}
```

2. Theme customization:
```cpp
void applyDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.12f, 0.94f);
    colors[ImGuiCol_Header] = ImVec4(0.2f, 0.4f, 0.6f, 0.8f);
    // ... more colors
}
```

3. Keyboard shortcuts:
```cpp
void handleKeyboardShortcuts() {
    if (ImGui::IsKeyPressed(ImGuiKey_Space)) togglePause();
    if (ImGui::IsKeyPressed(ImGuiKey_F1)) showHelp = !showHelp;
    if (ImGui::IsKeyPressed(ImGuiKey_F3)) showDebug = !showDebug;
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) deselectCreature();
}
```

4. Status bar:
```cpp
void renderStatusBar() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - 25));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 25));

    ImGui::Begin("StatusBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
    ImGui::Text("FPS: %.0f | Creatures: %d | Gen: %d | Time: %.1f | %s",
                fps, creatureCount, generation, simulationTime,
                isPaused ? "PAUSED" : "RUNNING");
    ImGui::End();
}
```

CHECKPOINT 5: Polished, professional-looking UI

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/UI_RESEARCH.md (150+ lines)
- [ ] docs/UI_DESIGN.md (200+ lines)
- [ ] Simulation Controls panel
- [ ] Population Statistics panel
- [ ] Creature Inspector panel
- [ ] Evolution graphs (population, fitness)
- [ ] Trait distribution histograms
- [ ] Neural network visualizer
- [ ] Docking support
- [ ] Layout save/load
- [ ] Custom theme
- [ ] Keyboard shortcuts
- [ ] Status bar
- [ ] Help window (F1)

ESTIMATED TIME: 3-4 hours with sub-agents
```

---

## Agent 10: Final Integration, Testing, and Documentation (4-6 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Final integration of all systems, comprehensive testing, and documentation.

============================================================================
PHASE 1: INTEGRATION AUDIT (1+ hour)
============================================================================

SPAWN 4 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Audit creature systems:
- Verify all creature types work (herbivore, carnivore, flying, aquatic)
- Check skeletal animation integration
- Check neural network brain integration
- Test reproduction and genetics
- Document any integration issues

SUB-AGENT 1B - Audit rendering systems:
- Verify instanced rendering
- Check shadow mapping
- Test all shaders compile and work
- Verify LOD system
- Check UI rendering
- Document any rendering issues

SUB-AGENT 1C - Audit environment systems:
- Test terrain generation
- Verify biome distribution
- Check weather system
- Test day/night cycle
- Verify vegetation placement
- Document any environment issues

SUB-AGENT 1D - Audit simulation systems:
- Test save/load
- Test replay system
- Verify performance with 1000+ creatures
- Check spatial partitioning
- Test GPU compute
- Document any simulation issues

CHECKPOINT 1: Create docs/INTEGRATION_AUDIT.md (500+ lines)

============================================================================
PHASE 2: BUG FIXING SPRINT (1.5+ hours)
============================================================================

Address issues found in audit:

SPAWN SUB-AGENTS for each major bug category:

SUB-AGENT 2A - Fix creature bugs:
- Read INTEGRATION_AUDIT.md creature section
- Fix each issue listed
- Verify fix works
- Document resolution

SUB-AGENT 2B - Fix rendering bugs:
- Read INTEGRATION_AUDIT.md rendering section
- Fix each issue listed
- Verify fix works
- Document resolution

SUB-AGENT 2C - Fix environment bugs:
- Read INTEGRATION_AUDIT.md environment section
- Fix each issue listed
- Verify fix works
- Document resolution

SUB-AGENT 2D - Fix simulation bugs:
- Read INTEGRATION_AUDIT.md simulation section
- Fix each issue listed
- Verify fix works
- Document resolution

CHECKPOINT 2: All bugs from audit fixed, documented in BUG_FIXES.md

============================================================================
PHASE 3: COMPREHENSIVE TESTING (1+ hour)
============================================================================

Create and run test suite:

1. Unit tests:
```cpp
// tests/test_genome.cpp
TEST(Genome, Mutation) {
    Genome g;
    g.randomize();
    Genome mutated = g;
    mutated.mutate(0.1f);
    EXPECT_NE(g.size, mutated.size);
}

// tests/test_neat.cpp
TEST(NEAT, Crossover) {
    NEATGenome parent1, parent2;
    // ... setup
    NEATGenome child = NEATGenome::crossover(parent1, parent2);
    EXPECT_GT(child.connections.size(), 0);
}

// tests/test_spatial_grid.cpp
TEST(SpatialGrid, Query) {
    SpatialGrid<int> grid(10.0f);
    grid.insert(&testData, Vec3(5, 0, 5));
    auto results = grid.queryRadius(Vec3(5, 0, 5), 2.0f);
    EXPECT_EQ(results.size(), 1);
}
```

2. Integration tests:
```cpp
TEST(Integration, CreatureLifecycle) {
    World world;
    world.spawnCreature(HERBIVORE);
    EXPECT_EQ(world.creatures.size(), 1);

    // Simulate for 100 frames
    for (int i = 0; i < 100; i++) {
        world.update(0.016f);
    }

    // Creature should have moved, consumed energy
    EXPECT_NE(world.creatures[0]->position, startPos);
    EXPECT_LT(world.creatures[0]->energy, startEnergy);
}

TEST(Integration, SaveLoad) {
    World world;
    world.spawnCreature(HERBIVORE);
    world.spawnCreature(CARNIVORE);

    SaveManager::save("test_save.evo", world);

    World loaded;
    SaveManager::load("test_save.evo", loaded);

    EXPECT_EQ(loaded.creatures.size(), 2);
}
```

3. Performance tests:
```cpp
TEST(Performance, TenThousandCreatures) {
    World world;
    for (int i = 0; i < 10000; i++) {
        world.spawnCreature(i % 2 == 0 ? HERBIVORE : CARNIVORE);
    }

    auto start = std::chrono::high_resolution_clock::now();
    world.update(0.016f);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 16);  // Must complete in one frame
}
```

CHECKPOINT 3: All tests pass, results in TEST_RESULTS.md

============================================================================
PHASE 4: DOCUMENTATION COMPILATION (1+ hour)
============================================================================

SPAWN 3 SUB-AGENTS:

SUB-AGENT 4A - Create User Manual:
Create docs/USER_MANUAL.md:
- Installation instructions
- Controls reference
- UI guide
- Gameplay tips
- Troubleshooting

SUB-AGENT 4B - Create Developer Guide:
Create docs/DEVELOPER_GUIDE.md:
- Architecture overview
- Code organization
- Adding new creature types
- Adding new behaviors
- Shader modification guide
- Build instructions

SUB-AGENT 4C - Create API Reference:
Create docs/API_REFERENCE.md:
- All public classes and methods
- Usage examples
- Parameter descriptions
- Return values

CHECKPOINT 4: Complete documentation suite

============================================================================
PHASE 5: README AND FINAL POLISH (30+ minutes)
============================================================================

Update main README.md:

```markdown
# OrganismEvolution

A real-time evolution simulator with neural network creature brains,
procedural animation, and ecosystem simulation.

## Features
- **Genetic Evolution**: Creatures evolve through natural selection
- **Neural Network Brains**: NEAT algorithm for evolving creature AI
- **Procedural Animation**: IK-based skeletal animation for all creatures
- **Diverse Ecosystems**: Multiple biomes, weather, day/night cycle
- **Multiple Creature Types**: Land, flying, and aquatic creatures
- **High Performance**: GPU compute for 10,000+ creatures at 60 FPS

## Screenshots
[Include screenshots]

## Quick Start
```bash
git clone https://github.com/user/OrganismEvolution
cd OrganismEvolution
mkdir build && cd build
cmake ..
cmake --build . --config Release
./OrganismEvolution.exe
```

## Controls
- WASD: Move camera
- Mouse: Look around
- Space: Toggle pause
- F5: Quick save
- F9: Quick load
- Click: Select creature

## Documentation
- [User Manual](docs/USER_MANUAL.md)
- [Developer Guide](docs/DEVELOPER_GUIDE.md)
- [API Reference](docs/API_REFERENCE.md)

## Requirements
- Windows 10/11
- DirectX 12 compatible GPU
- 8GB RAM recommended

## License
MIT License
```

CHECKPOINT 5: Professional README and project presentation

============================================================================
PHASE 6: FINAL VERIFICATION (30+ minutes)
============================================================================

Final checklist:

1. Fresh build test:
   - Clone to new directory
   - Build from scratch
   - Verify it runs

2. Feature verification:
   - All creature types spawn and behave correctly
   - Evolution progresses across generations
   - Save/load works
   - UI is functional
   - Performance meets targets

3. Documentation verification:
   - All docs are accurate
   - Links work
   - Examples compile

CHECKPOINT 6: Project ready for release

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/INTEGRATION_AUDIT.md (500+ lines)
- [ ] docs/BUG_FIXES.md
- [ ] tests/ directory with unit tests
- [ ] tests/ directory with integration tests
- [ ] docs/TEST_RESULTS.md
- [ ] docs/USER_MANUAL.md
- [ ] docs/DEVELOPER_GUIDE.md
- [ ] docs/API_REFERENCE.md
- [ ] Updated README.md
- [ ] All features working
- [ ] All tests passing
- [ ] Fresh build verification
- [ ] Project ready for release

ESTIMATED TIME: 4-6 hours with sub-agents
```

---

# Summary

These 10 agents are designed for **4-6 hours each** of intensive work:

| Agent | Mission | Est. Time |
|-------|---------|-----------|
| 1 | Skeletal Animation System | 4-6 hours |
| 2 | Flying Creatures | 4-6 hours |
| 3 | Aquatic Creatures | 4-6 hours |
| 4 | NEAT Neural Network Evolution | 4-6 hours |
| 5 | Advanced Terrain & Biomes | 4-6 hours |
| 6 | Performance Optimization | 4-6 hours |
| 7 | Save/Load & Replay | 3-4 hours |
| 8 | Shadow Mapping & Lighting | 3-4 hours |
| 9 | Comprehensive UI Dashboard | 3-4 hours |
| 10 | Integration, Testing, Docs | 4-6 hours |

**Key differences from previous agents:**

1. **Multiple phases** with explicit checkpoints
2. **Mandatory sub-agent spawning** for parallel work
3. **Research phases** requiring web searches and code analysis
4. **Documentation requirements** (400+ line documents)
5. **Explicit "DO NOT finish quickly" instructions**
6. **Verification checklists** at each phase

Copy-paste each agent prompt into a separate Claude chat session.
