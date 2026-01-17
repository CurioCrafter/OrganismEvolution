// Forge Engine - Steering Behavior Compute Shader
// GPU-accelerated steering behaviors for massive creature simulation
// Processes up to 65,536 creatures in parallel

// ============================================================================
// Thread Configuration
// ============================================================================

#define THREAD_GROUP_SIZE 64
#define MAX_NEIGHBORS 32
#define MAX_PREDATOR_SEARCH 512
#define MAX_PREY_SEARCH 512
#define MAX_FOOD_SEARCH 256
#define PI 3.14159265359f

// ============================================================================
// Data Structures (must match C++ GPUSteeringCompute.h)
// ============================================================================

struct CreatureInput
{
    float3 position;
    float energy;

    float3 velocity;
    float fear;

    uint type;          // CreatureType enum
    uint isAlive;       // bool as uint
    float waterLevel;
    float padding;
};

struct SteeringOutput
{
    float3 steeringForce;
    float priority;

    float3 targetPosition;
    uint behaviorFlags;
};

struct FoodPosition
{
    float3 position;
    float amount;
};

// ============================================================================
// Constant Buffer
// ============================================================================

cbuffer SteeringConstants : register(b0)
{
    // Steering forces
    float g_maxForce;
    float g_maxSpeed;
    float g_fleeDistance;
    float g_predatorAvoidanceMultiplier;

    // Flocking parameters
    float g_separationDistance;
    float g_alignmentDistance;
    float g_cohesionDistance;
    float g_separationWeight;

    float g_alignmentWeight;
    float g_cohesionWeight;
    float g_wanderRadius;
    float g_wanderDistance;

    float g_wanderJitter;
    float g_arriveSlowRadius;
    float g_pursuitPredictionTime;
    float g_waterAvoidanceDistance;

    // Simulation state
    uint g_creatureCount;
    uint g_foodCount;
    float g_deltaTime;
    float g_time;

    // Grid parameters
    float3 g_gridMin;
    float g_gridCellSize;

    float3 g_gridMax;
    uint g_gridCellsX;

    uint g_gridCellsY;
    uint g_gridCellsZ;
    uint2 g_padding;
};

// ============================================================================
// Resource Bindings
// ============================================================================

// Input buffers (SRV - read only)
StructuredBuffer<CreatureInput> g_creatureInputs : register(t0);
StructuredBuffer<FoodPosition> g_foodPositions : register(t1);

// Output buffer (UAV - read/write)
RWStructuredBuffer<SteeringOutput> g_steeringOutputs : register(u0);

// ============================================================================
// Creature Type Constants
// ============================================================================

#define TYPE_HERBIVORE 0
#define TYPE_CARNIVORE 1
#define TYPE_AQUATIC 2
#define TYPE_AQUATIC_PREDATOR 3
#define TYPE_AMPHIBIAN 4
#define TYPE_AMPHIBIAN_PREDATOR 5
#define TYPE_FLYING 6
#define TYPE_AERIAL_PREDATOR 7

// Behavior flags
#define FLAG_SEEK       (1u << 0)
#define FLAG_FLEE       (1u << 1)
#define FLAG_ARRIVE     (1u << 2)
#define FLAG_WANDER     (1u << 3)
#define FLAG_PURSUIT    (1u << 4)
#define FLAG_EVASION    (1u << 5)
#define FLAG_SEPARATE   (1u << 6)
#define FLAG_ALIGN      (1u << 7)
#define FLAG_COHESION   (1u << 8)
#define FLAG_AVOID_WATER (1u << 9)
#define FLAG_AVOID_LAND (1u << 10)
#define FLAG_ALTITUDE   (1u << 11)
#define FLAG_HUNTING    (1u << 12)
#define FLAG_FLEEING    (1u << 13)

// ============================================================================
// Helper Functions
// ============================================================================

// Check if type is a predator
bool IsPredator(uint type)
{
    return type == TYPE_CARNIVORE ||
           type == TYPE_AQUATIC_PREDATOR ||
           type == TYPE_AMPHIBIAN_PREDATOR ||
           type == TYPE_AERIAL_PREDATOR;
}

// Check if predator can eat prey type
bool IsPredatorOf(uint predatorType, uint preyType)
{
    switch (predatorType)
    {
        case TYPE_CARNIVORE:
            return preyType == TYPE_HERBIVORE;

        case TYPE_AERIAL_PREDATOR:
            return preyType == TYPE_HERBIVORE ||
                   preyType == TYPE_FLYING ||
                   preyType == TYPE_AMPHIBIAN;

        case TYPE_AQUATIC_PREDATOR:
            return preyType == TYPE_AQUATIC ||
                   preyType == TYPE_AMPHIBIAN;

        case TYPE_AMPHIBIAN_PREDATOR:
            return preyType == TYPE_HERBIVORE ||
                   preyType == TYPE_AMPHIBIAN ||
                   preyType == TYPE_AQUATIC;

        default:
            return false;
    }
}

// Check if creature can be in water
bool CanSwim(uint type)
{
    return type == TYPE_AQUATIC ||
           type == TYPE_AQUATIC_PREDATOR ||
           type == TYPE_AMPHIBIAN ||
           type == TYPE_AMPHIBIAN_PREDATOR;
}

// Check if creature can be on land
bool CanWalk(uint type)
{
    return type == TYPE_HERBIVORE ||
           type == TYPE_CARNIVORE ||
           type == TYPE_AMPHIBIAN ||
           type == TYPE_AMPHIBIAN_PREDATOR;
}

// Check if creature can fly
bool CanFly(uint type)
{
    return type == TYPE_FLYING ||
           type == TYPE_AERIAL_PREDATOR;
}

// Limit vector magnitude
float3 LimitMagnitude(float3 v, float maxMag)
{
    float lenSq = dot(v, v);
    if (lenSq > maxMag * maxMag)
    {
        return normalize(v) * maxMag;
    }
    return v;
}

// Random number generator (PCG-based)
uint PCGHash(uint input)
{
    uint state = input * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float RandomFloat(uint seed)
{
    return float(PCGHash(seed)) / 4294967295.0f;
}

float3 RandomInUnitSphere(uint seed)
{
    float u = RandomFloat(seed);
    float v = RandomFloat(seed + 1u);
    float theta = u * 2.0f * PI;
    float phi = acos(2.0f * v - 1.0f);

    float sinPhi = sin(phi);
    return float3(sinPhi * cos(theta), sinPhi * sin(theta), cos(phi));
}

uint SampleStart(uint seed, uint count)
{
    return (count > 0) ? (PCGHash(seed) % count) : 0;
}

// ============================================================================
// Steering Behaviors
// ============================================================================

// Seek: Move toward target
float3 Seek(float3 position, float3 velocity, float3 target)
{
    float3 desired = normalize(target - position) * g_maxSpeed;
    return desired - velocity;
}

// Flee: Move away from target
float3 Flee(float3 position, float3 velocity, float3 threat)
{
    float3 desired = normalize(position - threat) * g_maxSpeed;
    return desired - velocity;
}

// Arrive: Slow down when approaching target
float3 Arrive(float3 position, float3 velocity, float3 target)
{
    float3 toTarget = target - position;
    float distance = length(toTarget);

    if (distance < 0.001f)
        return float3(0, 0, 0);

    float speed = g_maxSpeed;
    if (distance < g_arriveSlowRadius)
    {
        speed = g_maxSpeed * (distance / g_arriveSlowRadius);
    }

    float3 desired = (toTarget / distance) * speed;
    return desired - velocity;
}

// Wander: Random wandering behavior
float3 Wander(float3 position, float3 velocity, uint seed)
{
    float3 forward = length(velocity) > 0.001f ? normalize(velocity) : float3(1, 0, 0);

    // Wander target on a circle in front of creature
    float3 circleCenter = position + forward * g_wanderDistance;

    // Add jitter to wander angle using time-based seed
    float angle = (RandomFloat(seed) * 2.0f - 1.0f) * PI;
    float3 displacement = float3(cos(angle), 0, sin(angle)) * g_wanderRadius;

    // For flying creatures, add vertical wander
    if (velocity.y != 0)
    {
        displacement.y = (RandomFloat(seed + 2u) * 2.0f - 1.0f) * g_wanderRadius * 0.5f;
    }

    float3 target = circleCenter + displacement;
    return Seek(position, velocity, target);
}

// Pursuit: Predict and intercept target
float3 Pursuit(float3 position, float3 velocity, float3 targetPos, float3 targetVel)
{
    float3 toTarget = targetPos - position;
    float distance = length(toTarget);

    // Predict future position
    float predictionTime = min(distance / g_maxSpeed, g_pursuitPredictionTime);
    float3 futurePos = targetPos + targetVel * predictionTime;

    return Seek(position, velocity, futurePos);
}

// Evasion: Predict and evade pursuer
float3 Evasion(float3 position, float3 velocity, float3 pursuerPos, float3 pursuerVel)
{
    float3 toPursuer = pursuerPos - position;
    float distance = length(toPursuer);

    // Predict future position
    float predictionTime = min(distance / g_maxSpeed, g_pursuitPredictionTime);
    float3 futurePos = pursuerPos + pursuerVel * predictionTime;

    return Flee(position, velocity, futurePos);
}

// ============================================================================
// Flocking Behaviors
// ============================================================================

// Find nearby creatures of same type for flocking
void FindFlockmates(uint myIndex, CreatureInput me, out float3 separation, out float3 alignment,
                    out float3 cohesion, out uint separateCount, out uint alignCount, out uint cohesionCount)
{
    separation = float3(0, 0, 0);
    alignment = float3(0, 0, 0);
    cohesion = float3(0, 0, 0);
    separateCount = 0;
    alignCount = 0;
    cohesionCount = 0;

    // Check nearby creatures (brute force for now - spatial hashing in future)
    for (uint i = 0; i < g_creatureCount && i < MAX_NEIGHBORS * 10; i++)
    {
        if (i == myIndex)
            continue;

        CreatureInput other = g_creatureInputs[i];

        if (!other.isAlive)
            continue;

        // Only flock with same type (or compatible types)
        if (other.type != me.type)
            continue;

        float3 toOther = other.position - me.position;
        float distance = length(toOther);

        // Separation
        if (distance < g_separationDistance && distance > 0.001f)
        {
            separation -= toOther / distance;
            separateCount++;
        }

        // Alignment
        if (distance < g_alignmentDistance)
        {
            alignment += other.velocity;
            alignCount++;
        }

        // Cohesion
        if (distance < g_cohesionDistance)
        {
            cohesion += other.position;
            cohesionCount++;
        }
    }
}

// Combine flocking forces
float3 Flock(uint myIndex, CreatureInput me)
{
    float3 separation, alignment, cohesion;
    uint sepCount, alignCount, cohCount;

    FindFlockmates(myIndex, me, separation, alignment, cohesion, sepCount, alignCount, cohCount);

    float3 result = float3(0, 0, 0);

    if (sepCount > 0)
    {
        separation = normalize(separation) * g_maxSpeed;
        result += (separation - me.velocity) * g_separationWeight;
    }

    if (alignCount > 0)
    {
        alignment = normalize(alignment / float(alignCount)) * g_maxSpeed;
        result += (alignment - me.velocity) * g_alignmentWeight;
    }

    if (cohCount > 0)
    {
        cohesion = cohesion / float(cohCount);
        result += Seek(me.position, me.velocity, cohesion) * g_cohesionWeight;
    }

    return result;
}

// ============================================================================
// Predator-Prey Behaviors
// ============================================================================

// Find nearest food
float3 SeekFood(float3 position, float3 velocity, uint seed, out bool foundFood)
{
    if (g_foodCount == 0)
    {
        foundFood = false;
        return float3(0, 0, 0);
    }

    foundFood = false;
    float nearestDistSq = 1e10f;
    float3 nearestFood = position;

    uint searchCount = min(g_foodCount, MAX_FOOD_SEARCH);
    uint start = SampleStart(seed, g_foodCount);
    for (uint s = 0; s < searchCount; ++s)
    {
        uint i = (start + s) % g_foodCount;
        float3 toFood = g_foodPositions[i].position - position;
        float distSq = dot(toFood, toFood);

        if (distSq < nearestDistSq)
        {
            nearestDistSq = distSq;
            nearestFood = g_foodPositions[i].position;
            foundFood = true;
        }
    }

    if (foundFood)
    {
        return Arrive(position, velocity, nearestFood);
    }
    return float3(0, 0, 0);
}

// Find predators to flee from
float3 FleeFromPredators(uint myIndex, CreatureInput me, uint seed, out bool fleeing)
{
    fleeing = false;
    float3 fleeForce = float3(0, 0, 0);
    uint predatorCount = 0;

    uint searchCount = min(g_creatureCount, MAX_PREDATOR_SEARCH);
    uint start = SampleStart(seed, g_creatureCount);
    for (uint s = 0; s < searchCount && predatorCount < MAX_NEIGHBORS; ++s)
    {
        uint i = (start + s) % g_creatureCount;
        if (i == myIndex)
            continue;

        CreatureInput other = g_creatureInputs[i];

        if (!other.isAlive)
            continue;

        if (!IsPredatorOf(other.type, me.type))
            continue;

        float3 toOther = other.position - me.position;
        float distance = length(toOther);

        if (distance < g_fleeDistance)
        {
            // Scale flee force by distance (closer = more urgent)
            float urgency = 1.0f - (distance / g_fleeDistance);
            fleeForce += Evasion(me.position, me.velocity, other.position, other.velocity) * urgency;
            predatorCount++;
            fleeing = true;
        }
    }

    if (predatorCount > 0)
    {
        return LimitMagnitude(fleeForce * g_predatorAvoidanceMultiplier, g_maxForce);
    }
    return float3(0, 0, 0);
}

// Find prey to hunt
float3 HuntPrey(uint myIndex, CreatureInput me, uint seed, out bool hunting)
{
    hunting = false;
    float nearestDistSq = 1e10f;
    float3 nearestPreyPos = me.position;
    float3 nearestPreyVel = float3(0, 0, 0);

    uint searchCount = min(g_creatureCount, MAX_PREY_SEARCH);
    uint start = SampleStart(seed, g_creatureCount);
    for (uint s = 0; s < searchCount; ++s)
    {
        uint i = (start + s) % g_creatureCount;
        if (i == myIndex)
            continue;

        CreatureInput other = g_creatureInputs[i];

        if (!other.isAlive)
            continue;

        if (!IsPredatorOf(me.type, other.type))
            continue;

        float3 toOther = other.position - me.position;
        float distSq = dot(toOther, toOther);

        if (distSq < nearestDistSq)
        {
            nearestDistSq = distSq;
            nearestPreyPos = other.position;
            nearestPreyVel = other.velocity;
            hunting = true;
        }
    }

    if (hunting)
    {
        return Pursuit(me.position, me.velocity, nearestPreyPos, nearestPreyVel);
    }
    return float3(0, 0, 0);
}

// ============================================================================
// Environment Avoidance
// ============================================================================

// Avoid water (for land creatures)
float3 AvoidWater(float3 position, float3 velocity, float waterLevel)
{
    float distToWater = position.y - waterLevel;

    if (distToWater < g_waterAvoidanceDistance && distToWater > 0)
    {
        float urgency = 1.0f - (distToWater / g_waterAvoidanceDistance);
        return float3(0, urgency * g_maxForce, 0);
    }
    return float3(0, 0, 0);
}

// Avoid land (for aquatic creatures)
float3 AvoidLand(float3 position, float3 velocity, float waterLevel)
{
    float distAboveWater = position.y - waterLevel;

    if (distAboveWater > -g_waterAvoidanceDistance && distAboveWater < 0)
    {
        float urgency = 1.0f + (distAboveWater / g_waterAvoidanceDistance);
        return float3(0, -urgency * g_maxForce, 0);
    }
    return float3(0, 0, 0);
}

// Maintain altitude (for flying creatures)
float3 MaintainAltitude(float3 position, float targetAltitude)
{
    float altDiff = targetAltitude - position.y;
    if (abs(altDiff) > 1.0f)
    {
        return float3(0, sign(altDiff) * g_maxForce * 0.5f, 0);
    }
    return float3(0, 0, 0);
}

// ============================================================================
// Main Compute Shader
// ============================================================================

[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint index = dispatchThreadId.x;

    // Bounds check
    if (index >= g_creatureCount)
        return;

    CreatureInput me = g_creatureInputs[index];
    SteeringOutput output;
    output.steeringForce = float3(0, 0, 0);
    output.priority = 0.0f;
    output.targetPosition = me.position;
    output.behaviorFlags = 0;

    // Skip dead creatures
    if (!me.isAlive)
    {
        g_steeringOutputs[index] = output;
        return;
    }

    float3 totalForce = float3(0, 0, 0);
    uint seed = index * 12345u + uint(g_time * 1000.0f);

    // Type-specific behavior
    if (IsPredator(me.type))
    {
        // Predators: Hunt prey
        bool hunting;
        float3 huntForce = HuntPrey(index, me, seed, hunting);
        if (hunting)
        {
            totalForce += huntForce * 2.0f;
            output.behaviorFlags |= FLAG_HUNTING | FLAG_PURSUIT;
        }
        else
        {
            // Wander when no prey nearby
            totalForce += Wander(me.position, me.velocity, seed) * 0.5f;
            output.behaviorFlags |= FLAG_WANDER;
        }
    }
    else
    {
        // Prey: Flee from predators first
        bool fleeing;
        float3 fleeForce = FleeFromPredators(index, me, seed, fleeing);
        if (fleeing)
        {
            totalForce += fleeForce * 3.0f;  // High priority
            output.priority = 1.0f;
            output.behaviorFlags |= FLAG_FLEEING | FLAG_EVASION;
        }
        else
        {
            // Seek food when safe
            bool foundFood;
            float3 foodForce = SeekFood(me.position, me.velocity, seed, foundFood);
            if (foundFood)
            {
                totalForce += foodForce;
                output.behaviorFlags |= FLAG_SEEK | FLAG_ARRIVE;
            }

            // Flocking behavior
            float3 flockForce = Flock(index, me);
            totalForce += flockForce * 0.8f;
            output.behaviorFlags |= FLAG_SEPARATE | FLAG_ALIGN | FLAG_COHESION;
        }
    }

    // Add wandering for exploration
    if (length(totalForce) < 0.1f)
    {
        totalForce += Wander(me.position, me.velocity, seed);
        output.behaviorFlags |= FLAG_WANDER;
    }

    // Environment constraints based on creature type
    if (CanWalk(me.type) && !CanSwim(me.type))
    {
        // Land creatures avoid water
        totalForce += AvoidWater(me.position, me.velocity, me.waterLevel);
        output.behaviorFlags |= FLAG_AVOID_WATER;
    }

    if (CanSwim(me.type) && !CanWalk(me.type))
    {
        // Pure aquatic creatures avoid land
        totalForce += AvoidLand(me.position, me.velocity, me.waterLevel);
        output.behaviorFlags |= FLAG_AVOID_LAND;
    }

    if (CanFly(me.type))
    {
        // Flying creatures maintain altitude
        float targetAlt = me.waterLevel + 20.0f;  // Default flying altitude
        totalForce += MaintainAltitude(me.position, targetAlt);
        output.behaviorFlags |= FLAG_ALTITUDE;
    }

    // Limit total force
    output.steeringForce = LimitMagnitude(totalForce, g_maxForce);

    // Calculate target position from steering
    float3 acceleration = output.steeringForce;
    float3 newVelocity = LimitMagnitude(me.velocity + acceleration * g_deltaTime, g_maxSpeed);
    output.targetPosition = me.position + newVelocity * g_deltaTime;

    // Write output
    g_steeringOutputs[index] = output;
}
