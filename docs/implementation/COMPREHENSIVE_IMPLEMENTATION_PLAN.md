# Comprehensive Implementation Plan

Based on research from:
- [NVIDIA GPU Gems - SpeedTree Rendering](https://developer.nvidia.com/gpugems/gpugems3/part-i-geometry/chapter-4-next-generation-speedtree-rendering)
- [NVIDIA GPU Gems - Grass Rendering](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-7-rendering-countless-blades-waving-grass)
- [Craig Reynolds - Boids Algorithm](https://www.red3d.com/cwr/boids/)
- [Craig Reynolds - Steering Behaviors](https://www.red3d.com/cwr/steer/)
- [Nature of Code - Autonomous Agents](https://natureofcode.com/autonomous-agents/)
- [The Algorithmic Beauty of Plants](https://archive.org/details/the-algorithmic-beauty-of-plants)
- [L-System Tree Generation](https://gpfault.net/posts/generating-trees.txt.html)

---

## Part 1: Vegetation System

### 1.1 L-System Tree Generation (Improved)

**Current Issues:**
- Trees all look similar
- No parametric variation
- Limited branch types

**Proper Implementation:**

```cpp
// Parametric L-System with stochastic rules
struct ParametricRule {
    char predecessor;
    std::string successor;
    float probability;  // For stochastic selection
    std::function<bool(float)> condition;  // For context-sensitive rules
};

// Tree parameters that vary per tree
struct TreeParameters {
    float branchAngle;        // 15-45 degrees
    float branchAngleVariance; // ±5-15 degrees randomness
    float segmentLength;      // Base segment length
    float lengthDecay;        // How much shorter each generation (0.7-0.9)
    float radiusDecay;        // Branch thinning factor (0.6-0.8)
    int iterations;           // 3-6 for different tree sizes
    float leafDensity;        // 0.3-0.8
    glm::vec3 trunkColor;     // Brown variations
    glm::vec3 leafColor;      // Green variations
    float bushiness;          // More branches vs taller growth
};

// Different tree archetypes
TreeParameters OAK_PARAMS = {
    .branchAngle = 25.0f,
    .branchAngleVariance = 10.0f,
    .segmentLength = 0.6f,
    .lengthDecay = 0.85f,
    .radiusDecay = 0.7f,
    .iterations = 4,
    .leafDensity = 0.7f,
    .trunkColor = {0.4f, 0.25f, 0.1f},
    .leafColor = {0.2f, 0.5f, 0.15f},
    .bushiness = 0.8f
};

TreeParameters PINE_PARAMS = {
    .branchAngle = 35.0f,
    .branchAngleVariance = 5.0f,
    .segmentLength = 0.8f,
    .lengthDecay = 0.75f,
    .radiusDecay = 0.65f,
    .iterations = 5,
    .leafDensity = 0.9f,
    .trunkColor = {0.3f, 0.2f, 0.1f},
    .leafColor = {0.1f, 0.35f, 0.1f},
    .bushiness = 0.3f  // Tall and narrow
};

TreeParameters WILLOW_PARAMS = {
    .branchAngle = 15.0f,
    .branchAngleVariance = 8.0f,
    .segmentLength = 0.5f,
    .lengthDecay = 0.9f,
    .radiusDecay = 0.8f,
    .iterations = 5,
    .leafDensity = 0.6f,
    .trunkColor = {0.35f, 0.25f, 0.15f},
    .leafColor = {0.25f, 0.55f, 0.2f},
    .bushiness = 0.6f
};
```

**L-System Production Rules:**

```
// Oak: Bushy, spreading canopy
Axiom: A
Rules:
  A -> F[+A][-A]FA  (probability 0.6)
  A -> F[++A][--A]A (probability 0.4)
  F -> FF

// Pine: Conical, regular branching
Axiom: A
Rules:
  A -> F[+A]F[-A]A
  F -> FF

// Willow: Drooping branches
Axiom: A
Rules:
  A -> F[+A][--A]FA
  F -> F
```

### 1.2 Grass Rendering (GPU Instanced)

Based on [GPU Gems grass chapter](https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-7-rendering-countless-blades-waving-grass):

**Grass Blade Geometry:**
- Use 3 intersecting quads per grass clump (star pattern)
- Disable back-face culling
- Alpha-tested texture with multiple blade silhouettes

**Vertex Shader Wind Animation:**
```glsl
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in mat4 aInstanceModel;

uniform float time;
uniform vec2 windDirection;
uniform float windStrength;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main() {
    vec3 pos = aPos;

    // Only animate upper vertices (texCoord.y near 0)
    if (aTexCoord.y < 0.5) {
        // World position for wave calculation
        vec3 worldPos = vec3(aInstanceModel * vec4(aPos, 1.0));

        // Wind wave based on position and time
        float wave = sin(time * 2.0 + worldPos.x * 0.5 + worldPos.z * 0.3);
        wave += sin(time * 1.5 + worldPos.x * 0.3 - worldPos.z * 0.4) * 0.5;

        // Apply wind displacement
        pos.x += windDirection.x * windStrength * wave * (1.0 - aTexCoord.y);
        pos.z += windDirection.y * windStrength * wave * (1.0 - aTexCoord.y);
    }

    vec4 worldPos = aInstanceModel * vec4(pos, 1.0);
    FragPos = worldPos.xyz;
    Normal = mat3(transpose(inverse(aInstanceModel))) * aNormal;
    TexCoord = aTexCoord;

    gl_Position = projection * view * worldPos;
}
```

**Instancing Setup:**
```cpp
// Generate instance data for grass clusters
std::vector<glm::mat4> grassInstances;
for (const auto& cluster : grassClusters) {
    // Multiple blades per cluster with slight variation
    for (int i = 0; i < bladesPerCluster; i++) {
        glm::mat4 model = glm::mat4(1.0f);

        // Random offset within cluster
        float offsetX = (Random::value() - 0.5f) * clusterRadius;
        float offsetZ = (Random::value() - 0.5f) * clusterRadius;

        model = glm::translate(model, cluster.position + glm::vec3(offsetX, 0, offsetZ));
        model = glm::rotate(model, Random::value() * 6.28f, glm::vec3(0, 1, 0));
        model = glm::scale(model, glm::vec3(0.8f + Random::value() * 0.4f));

        grassInstances.push_back(model);
    }
}

// Upload to GPU
glBindBuffer(GL_ARRAY_BUFFER, grassInstanceVBO);
glBufferData(GL_ARRAY_BUFFER, grassInstances.size() * sizeof(glm::mat4),
             grassInstances.data(), GL_STATIC_DRAW);

// Single draw call for ALL grass
glDrawArraysInstanced(GL_TRIANGLES, 0, verticesPerBlade, grassInstances.size());
```

---

## Part 2: Steering Behaviors

Based on [Craig Reynolds' steering behaviors](https://www.red3d.com/cwr/steer/) and [Nature of Code](https://natureofcode.com/autonomous-agents/):

### 2.1 Core Steering Formula

```cpp
// Reynolds' fundamental equation
glm::vec3 steer = desired_velocity - current_velocity;
steer = glm::clamp(steer, -maxForce, maxForce);
```

### 2.2 Behavior Implementations

```cpp
class SteeringBehaviors {
public:
    // SEEK: Move toward target at max speed
    glm::vec3 seek(const glm::vec3& target) {
        glm::vec3 desired = glm::normalize(target - position) * maxSpeed;
        glm::vec3 steer = desired - velocity;
        return glm::clamp(steer, glm::vec3(-maxForce), glm::vec3(maxForce));
    }

    // FLEE: Move away from target at max speed
    glm::vec3 flee(const glm::vec3& target) {
        glm::vec3 desired = glm::normalize(position - target) * maxSpeed;
        glm::vec3 steer = desired - velocity;
        return glm::clamp(steer, glm::vec3(-maxForce), glm::vec3(maxForce));
    }

    // ARRIVE: Slow down as approaching target
    glm::vec3 arrive(const glm::vec3& target, float slowRadius = 10.0f) {
        glm::vec3 toTarget = target - position;
        float distance = glm::length(toTarget);

        if (distance < 0.1f) return glm::vec3(0);

        float speed = maxSpeed;
        if (distance < slowRadius) {
            speed = maxSpeed * (distance / slowRadius);
        }

        glm::vec3 desired = glm::normalize(toTarget) * speed;
        return glm::clamp(desired - velocity, glm::vec3(-maxForce), glm::vec3(maxForce));
    }

    // WANDER: Random exploration
    glm::vec3 wander() {
        // Project circle ahead of agent
        glm::vec3 circleCenter = position + glm::normalize(velocity) * wanderDistance;

        // Random point on circle
        wanderAngle += (Random::value() - 0.5f) * wanderJitter;
        glm::vec3 displacement = glm::vec3(
            cos(wanderAngle) * wanderRadius,
            0,
            sin(wanderAngle) * wanderRadius
        );

        return seek(circleCenter + displacement);
    }

    // PURSUIT: Predict and intercept moving target
    glm::vec3 pursuit(const Creature& target) {
        glm::vec3 toTarget = target.position - position;
        float distance = glm::length(toTarget);

        // Predict future position
        float lookAheadTime = distance / maxSpeed;
        glm::vec3 futurePos = target.position + target.velocity * lookAheadTime;

        return seek(futurePos);
    }

    // EVASION: Flee from predicted position
    glm::vec3 evade(const Creature& pursuer) {
        glm::vec3 toPursuer = pursuer.position - position;
        float distance = glm::length(toPursuer);

        float lookAheadTime = distance / maxSpeed;
        glm::vec3 futurePos = pursuer.position + pursuer.velocity * lookAheadTime;

        return flee(futurePos);
    }

    // SEPARATION: Avoid crowding neighbors
    glm::vec3 separate(const std::vector<Creature*>& neighbors, float desiredSeparation) {
        glm::vec3 sum(0);
        int count = 0;

        for (auto* other : neighbors) {
            float d = glm::distance(position, other->position);
            if (d > 0 && d < desiredSeparation) {
                glm::vec3 diff = glm::normalize(position - other->position);
                diff /= d;  // Weight by distance (closer = stronger)
                sum += diff;
                count++;
            }
        }

        if (count > 0) {
            sum /= (float)count;
            sum = glm::normalize(sum) * maxSpeed;
            return glm::clamp(sum - velocity, glm::vec3(-maxForce), glm::vec3(maxForce));
        }
        return glm::vec3(0);
    }

    // ALIGNMENT: Match velocity with neighbors
    glm::vec3 align(const std::vector<Creature*>& neighbors, float neighborDist) {
        glm::vec3 sum(0);
        int count = 0;

        for (auto* other : neighbors) {
            float d = glm::distance(position, other->position);
            if (d > 0 && d < neighborDist) {
                sum += other->velocity;
                count++;
            }
        }

        if (count > 0) {
            sum /= (float)count;
            sum = glm::normalize(sum) * maxSpeed;
            return glm::clamp(sum - velocity, glm::vec3(-maxForce), glm::vec3(maxForce));
        }
        return glm::vec3(0);
    }

    // COHESION: Steer toward center of neighbors
    glm::vec3 cohesion(const std::vector<Creature*>& neighbors, float neighborDist) {
        glm::vec3 sum(0);
        int count = 0;

        for (auto* other : neighbors) {
            float d = glm::distance(position, other->position);
            if (d > 0 && d < neighborDist) {
                sum += other->position;
                count++;
            }
        }

        if (count > 0) {
            sum /= (float)count;
            return seek(sum);
        }
        return glm::vec3(0);
    }
};
```

### 2.3 Force Combination (Weighted Blending)

```cpp
void Creature::applyBehaviors(const std::vector<Creature*>& allCreatures,
                              const std::vector<glm::vec3>& foodPositions,
                              const std::vector<Creature*>& predators) {
    glm::vec3 force(0);

    // Get nearby creatures
    std::vector<Creature*> neighbors = getNeighbors(allCreatures, perceptionRadius);

    if (type == CreatureType::HERBIVORE) {
        // Herbivore behaviors
        glm::vec3 seekFood = seekNearestFood(foodPositions) * 1.5f;
        glm::vec3 separate = steering.separate(neighbors, separationDist) * 2.0f;
        glm::vec3 align = steering.align(neighbors, alignmentDist) * 1.0f;
        glm::vec3 cohere = steering.cohesion(neighbors, cohesionDist) * 1.0f;
        glm::vec3 evadePredators = evadeAllPredators(predators) * 3.0f;  // High priority!

        force = seekFood + separate + align + cohere + evadePredators;

    } else if (type == CreatureType::CARNIVORE) {
        // Carnivore behaviors
        glm::vec3 huntPrey = pursueNearestPrey() * 2.0f;
        glm::vec3 separate = steering.separate(neighbors, separationDist) * 1.5f;
        glm::vec3 wander = steering.wander() * 0.5f;

        force = huntPrey + separate + wander;
    }

    // Apply accumulated force
    acceleration += force;
}
```

---

## Part 3: Predator-Prey Ecosystem

Based on [predator-flocks simulation](https://github.com/decentralion/predator-flocks):

### 3.1 Creature Types

```cpp
enum class CreatureType {
    HERBIVORE,
    CARNIVORE,
    OMNIVORE
};

struct CreatureTraits {
    CreatureType type;
    float maxSpeed;
    float maxForce;
    float visionRange;
    float attackRange;
    float fleeDistance;
    glm::vec3 baseColor;
    float metabolism;  // Energy consumption rate
};

CreatureTraits HERBIVORE_TRAITS = {
    .type = CreatureType::HERBIVORE,
    .maxSpeed = 8.0f,
    .maxForce = 0.3f,
    .visionRange = 30.0f,
    .attackRange = 0.0f,
    .fleeDistance = 25.0f,
    .baseColor = {0.3f, 0.7f, 0.3f},  // Green
    .metabolism = 0.5f
};

CreatureTraits CARNIVORE_TRAITS = {
    .type = CreatureType::CARNIVORE,
    .maxSpeed = 12.0f,  // Faster than prey
    .maxForce = 0.5f,   // More maneuverable
    .visionRange = 50.0f,
    .attackRange = 3.0f,
    .fleeDistance = 0.0f,
    .baseColor = {0.8f, 0.2f, 0.2f},  // Red
    .metabolism = 1.0f  // Higher energy needs
};
```

### 3.2 Ecosystem Dynamics

```cpp
class Ecosystem {
    void update(float deltaTime) {
        // Separate creatures by type for efficient lookups
        std::vector<Creature*> herbivores, carnivores;
        for (auto& c : creatures) {
            if (c->getType() == CreatureType::HERBIVORE) herbivores.push_back(c.get());
            else carnivores.push_back(c.get());
        }

        // Update herbivores
        for (auto* h : herbivores) {
            // Apply flocking with same species
            h->applyFlocking(herbivores);

            // Seek food (trees, grass)
            h->seekFood(foodSources);

            // FLEE from predators (highest priority)
            h->evadePredators(carnivores);

            h->update(deltaTime);
        }

        // Update carnivores
        for (auto* c : carnivores) {
            // Pursue prey
            Creature* target = findNearestPrey(c, herbivores);
            if (target) {
                c->pursue(target);

                // Check for kill
                if (glm::distance(c->position, target->position) < c->attackRange) {
                    c->consumeFood(target->getEnergyValue());
                    target->die();
                }
            } else {
                // No prey visible - wander
                c->wander();
            }

            c->update(deltaTime);
        }

        // Handle reproduction
        handleReproduction();

        // Remove dead creatures
        removeDeadCreatures();
    }
};
```

### 3.3 Visual Differentiation

```cpp
// Herbivores: Round, green, shorter legs
// Carnivores: Elongated, red/orange, longer legs, visible teeth/spikes

void CreatureBuilder::buildHerbivore(const Genome& genome, MeshData& mesh) {
    float size = genome.size;

    // Rounder body (sphere-like)
    addMetaball(mesh, glm::vec3(0, size * 0.6f, 0), size * 1.0f, 1.0f);

    // Shorter, stockier legs (4 legs)
    float legLength = size * 0.5f;
    addLeg(mesh, glm::vec3(-size*0.5f, 0, -size*0.3f), legLength);
    addLeg(mesh, glm::vec3(size*0.5f, 0, -size*0.3f), legLength);
    addLeg(mesh, glm::vec3(-size*0.5f, 0, size*0.3f), legLength);
    addLeg(mesh, glm::vec3(size*0.5f, 0, size*0.3f), legLength);

    // Small round head
    addMetaball(mesh, glm::vec3(0, size * 0.8f, size * 0.6f), size * 0.4f, 0.8f);
}

void CreatureBuilder::buildCarnivore(const Genome& genome, MeshData& mesh) {
    float size = genome.size;

    // Elongated body
    addMetaball(mesh, glm::vec3(0, size * 0.5f, -size*0.3f), size * 0.7f, 1.0f);
    addMetaball(mesh, glm::vec3(0, size * 0.5f, size*0.3f), size * 0.6f, 0.9f);

    // Longer legs (2 or 4 depending on speed)
    float legLength = size * 0.8f;
    if (genome.speed > 15.0f) {
        // Bipedal fast predator
        addLeg(mesh, glm::vec3(-size*0.3f, 0, 0), legLength);
        addLeg(mesh, glm::vec3(size*0.3f, 0, 0), legLength);
    } else {
        // Quadruped predator
        addLeg(mesh, glm::vec3(-size*0.4f, 0, -size*0.4f), legLength);
        addLeg(mesh, glm::vec3(size*0.4f, 0, -size*0.4f), legLength);
        addLeg(mesh, glm::vec3(-size*0.4f, 0, size*0.4f), legLength);
        addLeg(mesh, glm::vec3(size*0.4f, 0, size*0.4f), legLength);
    }

    // Elongated head with "teeth" (spikes)
    addMetaball(mesh, glm::vec3(0, size * 0.6f, size * 0.8f), size * 0.35f, 0.8f);
    addSpike(mesh, glm::vec3(0, size * 0.5f, size * 1.1f), size * 0.15f);  // Snout
}
```

---

## Implementation Priority

1. **Steering Behaviors** - Foundation for all movement
2. **Creature Types** - Herbivore/Carnivore differentiation
3. **Predator-Prey Dynamics** - Hunt, flee, eat mechanics
4. **Improved Tree Generation** - Parametric L-systems
5. **Instanced Grass** - GPU-efficient rendering
6. **Wind Animation** - Shader-based vegetation movement

---

## Performance Considerations

1. **Spatial Partitioning** - Use grid/quadtree for O(n) neighbor queries instead of O(n²)
2. **Instanced Rendering** - Single draw call for all grass/similar objects
3. **LOD System** - Simpler geometry at distance
4. **Culling** - Don't process/render off-screen entities
