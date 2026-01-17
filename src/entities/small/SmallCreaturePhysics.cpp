#include "SmallCreaturePhysics.h"
#include "../../environment/Terrain.h"
#include <cmath>
#include <algorithm>

namespace small {

// =============================================================================
// SmallCreaturePhysics Implementation
// =============================================================================

void SmallCreaturePhysics::update(SmallCreature& creature, float deltaTime,
                                   Terrain* terrain, MicroSpatialGrid& grid) {
    auto props = getProperties(creature.type);

    // Get movement state
    MovementState state = getMovementState(creature.position, props.minSize, terrain);

    // Different physics based on locomotion type
    XMFLOAT3 targetVel = { 0, 0, 0 };

    // Calculate direction to target
    float dx = creature.targetPosition.x - creature.position.x;
    float dy = creature.targetPosition.y - creature.position.y;
    float dz = creature.targetPosition.z - creature.position.z;
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);

    if (dist > 0.001f) {
        dx /= dist;
        dy /= dist;
        dz /= dist;
    }

    float maxSpeed = getMaxSpeed(creature, state.currentSurface);

    // Handle different locomotion types
    if (props.canFly && isFlyingInsect(creature.type)) {
        // Flying creatures
        targetVel = calculateFlyingVelocity(creature, creature.targetPosition, deltaTime);

        // Keep above minimum height
        float minHeight = state.groundHeight + 0.5f;
        if (creature.position.y < minHeight) {
            targetVel.y += (minHeight - creature.position.y) * 5.0f;
        }
    }
    else if (state.isSwimming && props.canSwim) {
        // Swimming
        targetVel = calculateSwimmingVelocity(creature, creature.targetPosition, state.waterLevel);
    }
    else if (state.isBurrowing) {
        // Burrowing underground
        targetVel = calculateBurrowVelocity(creature, creature.targetPosition);
    }
    else if (state.isClimbing && props.canClimb) {
        // Climbing
        targetVel = calculateClimbVelocity(creature, creature.targetPosition, state.surfaceNormal);
    }
    else if (props.canJump && (props.primaryLocomotion == LocomotionType::JUMPING ||
                               creature.type == SmallCreatureType::SPIDER_JUMPING)) {
        // Jumping locomotion
        if (state.isGrounded && dist > 0.2f) {
            // Check if should jump
            float jumpChance = creature.genome->speed * 0.1f;
            if (creature.velocity.y == 0.0f && jumpChance > 0.05f) {
                XMFLOAT3 jumpVel = calculateJump(creature, creature.targetPosition, 1.0f);
                creature.velocity = jumpVel;
            }
        }

        // Apply gravity if in air
        if (!state.isGrounded) {
            creature.velocity.y -= PhysicsConstants::GRAVITY * deltaTime * 0.1f;
        } else {
            // Ground movement between jumps
            targetVel.x = dx * maxSpeed * 0.3f;
            targetVel.z = dz * maxSpeed * 0.3f;
        }
    }
    else if (state.currentSurface == SurfaceType::WATER_SURFACE && canWalkOnWater(creature)) {
        // Walking on water surface (surface tension)
        targetVel.x = dx * maxSpeed * 0.5f;
        targetVel.z = dz * maxSpeed * 0.5f;
        creature.position.y = state.waterLevel;
    }
    else {
        // Standard ground movement
        if (state.isGrounded) {
            targetVel.x = dx * maxSpeed;
            targetVel.z = dz * maxSpeed;
        } else {
            // Falling
            creature.velocity.y -= PhysicsConstants::GRAVITY * deltaTime * 0.1f;
        }
    }

    // Apply steering if not jumping
    if (!state.isJumping && !(props.canJump && !state.isGrounded)) {
        creature.velocity = steerTowards(creature.velocity,
                                         XMFLOAT3{targetVel.x, targetVel.y, targetVel.z},
                                         maxSpeed, 5.0f * deltaTime);
    }

    // Apply drag
    creature.velocity = applyDrag(creature.velocity, props.minSize, state.currentSurface);

    // Update position
    creature.position.x += creature.velocity.x * deltaTime;
    creature.position.y += creature.velocity.y * deltaTime;
    creature.position.z += creature.velocity.z * deltaTime;

    // Ground collision
    if (terrain && !props.canFly && !state.isBurrowing) {
        float groundY = terrain->getHeight(creature.position.x, creature.position.z);
        if (creature.position.y < groundY) {
            creature.position.y = groundY;
            creature.velocity.y = 0.0f;
        }
    }

    // Update rotation to face movement direction
    if (fabsf(creature.velocity.x) > 0.001f || fabsf(creature.velocity.z) > 0.001f) {
        creature.rotation = atan2f(creature.velocity.x, creature.velocity.z);
    }

    // Update animation
    float speedMag = sqrtf(creature.velocity.x * creature.velocity.x +
                          creature.velocity.z * creature.velocity.z);
    creature.animationSpeed = 1.0f + speedMag * 2.0f;
}

MovementState SmallCreaturePhysics::getMovementState(const XMFLOAT3& position, float size,
                                                      Terrain* terrain) {
    MovementState state = {};
    state.surfaceNormal = { 0, 1, 0 };
    state.waterLevel = 0.0f;

    if (terrain) {
        state.groundHeight = terrain->getHeight(position.x, position.z);

        // Water level detection - uses terrain water level when available
        // Default to y=0 if terrain doesn't provide water level
        float waterLevel = terrain->getWaterLevel();
        state.waterLevel = waterLevel;

        if (position.y < waterLevel - 0.1f) {
            state.currentSurface = SurfaceType::UNDERWATER;
            state.isSwimming = true;
        } else if (position.y < waterLevel + size * 0.5f && position.y > waterLevel - size * 0.5f) {
            state.currentSurface = SurfaceType::WATER_SURFACE;
        } else if (position.y < state.groundHeight - 0.01f) {
            state.isBurrowing = true;
            state.currentSurface = SurfaceType::GROUND;
        } else if (position.y < state.groundHeight + 0.1f) {
            state.isGrounded = true;
            state.currentSurface = SurfaceType::GROUND;
        } else if (position.y < state.groundHeight + 0.5f) {
            // In grass zone
            state.currentSurface = SurfaceType::GRASS;
            state.isGrounded = true;
        } else {
            state.currentSurface = SurfaceType::AIR;
        }

        // Get terrain normal for slope calculations
        // Simple finite difference
        float h1 = terrain->getHeight(position.x + 0.1f, position.z);
        float h2 = terrain->getHeight(position.x - 0.1f, position.z);
        float h3 = terrain->getHeight(position.x, position.z + 0.1f);
        float h4 = terrain->getHeight(position.x, position.z - 0.1f);

        XMFLOAT3 tangentX = { 0.2f, h1 - h2, 0.0f };
        XMFLOAT3 tangentZ = { 0.0f, h3 - h4, 0.2f };

        // Cross product for normal
        state.surfaceNormal.x = tangentX.y * tangentZ.z - tangentX.z * tangentZ.y;
        state.surfaceNormal.y = tangentX.z * tangentZ.x - tangentX.x * tangentZ.z;
        state.surfaceNormal.z = tangentX.x * tangentZ.y - tangentX.y * tangentZ.x;

        // Normalize
        float len = sqrtf(state.surfaceNormal.x * state.surfaceNormal.x +
                         state.surfaceNormal.y * state.surfaceNormal.y +
                         state.surfaceNormal.z * state.surfaceNormal.z);
        if (len > 0.001f) {
            state.surfaceNormal.x /= len;
            state.surfaceNormal.y /= len;
            state.surfaceNormal.z /= len;
        }

        // Calculate climb angle from normal
        state.climbAngle = acosf(state.surfaceNormal.y);
    } else {
        state.groundHeight = 0.0f;
        state.isGrounded = position.y < 0.1f;
        state.currentSurface = state.isGrounded ? SurfaceType::GROUND : SurfaceType::AIR;
    }

    return state;
}

bool SmallCreaturePhysics::canWalkOnWater(const SmallCreature& creature) {
    auto props = getProperties(creature.type);

    // Surface tension supports small, light creatures
    // Approximation: creatures under ~1cm with exoskeletons can walk on water

    if (props.minSize > 0.01f) return false;
    if (!props.hasExoskeleton) return false;

    // Water striders, some spiders, small beetles
    if (creature.type == SmallCreatureType::SPIDER_WOLF ||
        creature.type == SmallCreatureType::SPIDER_JUMPING) {
        return props.minSize < 0.005f;
    }

    return isInsect(creature.type) && props.minSize < 0.005f;
}

XMFLOAT3 SmallCreaturePhysics::calculateJump(const SmallCreature& creature,
                                              const XMFLOAT3& targetPos,
                                              float jumpStrength) {
    auto props = getProperties(creature.type);

    float dx = targetPos.x - creature.position.x;
    float dy = targetPos.y - creature.position.y;
    float dz = targetPos.z - creature.position.z;
    float horizDist = sqrtf(dx * dx + dz * dz);

    // Normalize horizontal direction
    if (horizDist > 0.001f) {
        dx /= horizDist;
        dz /= horizDist;
    }

    // Calculate jump multiplier based on creature type
    float jumpMult = 10.0f; // Default
    if (isAmphibian(creature.type) && (creature.type == SmallCreatureType::FROG ||
                                        creature.type == SmallCreatureType::TREE_FROG)) {
        jumpMult = PhysicsConstants::FROG_JUMP_MULT;
    } else if (creature.type == SmallCreatureType::GRASSHOPPER) {
        jumpMult = PhysicsConstants::GRASSHOPPER_JUMP_MULT;
    } else if (creature.type == SmallCreatureType::SPIDER_JUMPING) {
        jumpMult = PhysicsConstants::SPIDER_JUMP_MULT;
    } else if (creature.type == SmallCreatureType::CRICKET) {
        jumpMult = 15.0f;
    }

    // Base jump velocity from body size
    float baseJumpV = sqrtf(2.0f * PhysicsConstants::GRAVITY * props.minSize * jumpMult);
    baseJumpV *= jumpStrength * creature.genome->speed;

    // Calculate optimal angle for distance
    // For max range, angle = 45 degrees, but adjust based on height difference
    float angle = 0.785f; // 45 degrees
    if (dy > 0) {
        // Need to jump higher
        angle = 0.785f + atanf(dy / std::max(horizDist, 0.1f)) * 0.5f;
    } else if (dy < -0.5f) {
        // Target is below, lower angle
        angle = 0.5f;
    }

    XMFLOAT3 jumpVel;
    jumpVel.x = dx * baseJumpV * cosf(angle);
    jumpVel.y = baseJumpV * sinf(angle);
    jumpVel.z = dz * baseJumpV * cosf(angle);

    // Clamp to reasonable values
    float maxJumpSpeed = props.baseSpeed * 10.0f;
    float jumpMag = sqrtf(jumpVel.x * jumpVel.x + jumpVel.y * jumpVel.y + jumpVel.z * jumpVel.z);
    if (jumpMag > maxJumpSpeed) {
        float scale = maxJumpSpeed / jumpMag;
        jumpVel.x *= scale;
        jumpVel.y *= scale;
        jumpVel.z *= scale;
    }

    return jumpVel;
}

bool SmallCreaturePhysics::canClimbSurface(const SmallCreature& creature,
                                            float surfaceAngle, SurfaceType surface) {
    auto props = getProperties(creature.type);

    if (!props.canClimb) return false;

    // Insects with tarsal pads can climb almost anything
    if (props.hasExoskeleton && isInsect(creature.type)) {
        return surfaceAngle < 3.0f; // Nearly vertical
    }

    // Geckos have incredible climbing
    if (creature.type == SmallCreatureType::GECKO) {
        return surfaceAngle < 3.14f; // Can climb upside down
    }

    // Squirrels can climb trees well
    if (creature.type == SmallCreatureType::SQUIRREL_TREE) {
        if (surface == SurfaceType::TREE_BARK) {
            return surfaceAngle < 1.57f; // 90 degrees
        }
        return surfaceAngle < 1.0f;
    }

    // Spiders can climb most surfaces
    if (isSpider(creature.type)) {
        return surfaceAngle < 2.5f;
    }

    // Default climbing limit
    return surfaceAngle < PhysicsConstants::MIN_CLIMB_ANGLE;
}

XMFLOAT3 SmallCreaturePhysics::calculateClimbVelocity(const SmallCreature& creature,
                                                       const XMFLOAT3& targetPos,
                                                       const XMFLOAT3& surfaceNormal) {
    auto props = getProperties(creature.type);

    // Direction to target
    float dx = targetPos.x - creature.position.x;
    float dy = targetPos.y - creature.position.y;
    float dz = targetPos.z - creature.position.z;
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);

    if (dist < 0.001f) return { 0, 0, 0 };

    // Normalize direction
    dx /= dist;
    dy /= dist;
    dz /= dist;

    // Project onto surface plane
    float dot = dx * surfaceNormal.x + dy * surfaceNormal.y + dz * surfaceNormal.z;

    XMFLOAT3 climbDir;
    climbDir.x = dx - dot * surfaceNormal.x;
    climbDir.y = dy - dot * surfaceNormal.y;
    climbDir.z = dz - dot * surfaceNormal.z;

    // Normalize
    float climbMag = sqrtf(climbDir.x * climbDir.x + climbDir.y * climbDir.y + climbDir.z * climbDir.z);
    if (climbMag > 0.001f) {
        climbDir.x /= climbMag;
        climbDir.y /= climbMag;
        climbDir.z /= climbMag;
    }

    // Apply climbing speed
    float speed = props.baseSpeed * PhysicsConstants::MAX_CLIMB_SPEED_MULT * creature.genome->speed;

    climbDir.x *= speed;
    climbDir.y *= speed;
    climbDir.z *= speed;

    return climbDir;
}

bool SmallCreaturePhysics::canBurrow(const SmallCreature& creature, Terrain* terrain) {
    auto props = getProperties(creature.type);
    return props.canBurrow;
}

XMFLOAT3 SmallCreaturePhysics::calculateBurrowVelocity(const SmallCreature& creature,
                                                        const XMFLOAT3& targetPos) {
    auto props = getProperties(creature.type);

    float dx = targetPos.x - creature.position.x;
    float dy = targetPos.y - creature.position.y;
    float dz = targetPos.z - creature.position.z;
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);

    if (dist < 0.001f) return { 0, 0, 0 };

    // Normalize and apply burrow speed
    float speed = props.baseSpeed * PhysicsConstants::BURROW_SPEED_MULT * creature.genome->speed;

    XMFLOAT3 vel;
    vel.x = (dx / dist) * speed;
    vel.y = (dy / dist) * speed;
    vel.z = (dz / dist) * speed;

    return vel;
}

XMFLOAT3 SmallCreaturePhysics::calculateFlyingVelocity(const SmallCreature& creature,
                                                        const XMFLOAT3& targetPos,
                                                        float deltaTime) {
    auto props = getProperties(creature.type);

    float dx = targetPos.x - creature.position.x;
    float dy = targetPos.y - creature.position.y;
    float dz = targetPos.z - creature.position.z;
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);

    if (dist < 0.001f) return creature.velocity;

    // Normalize direction
    dx /= dist;
    dy /= dist;
    dz /= dist;

    float maxSpeed = props.baseSpeed * creature.genome->speed;

    // Different flight styles
    XMFLOAT3 targetVel;

    if (creature.type == SmallCreatureType::DRAGONFLY ||
        creature.type == SmallCreatureType::DAMSELFLY) {
        // Fast, darting flight
        targetVel.x = dx * maxSpeed * 2.0f;
        targetVel.y = dy * maxSpeed * 1.5f;
        targetVel.z = dz * maxSpeed * 2.0f;
    }
    else if (creature.type == SmallCreatureType::BUTTERFLY ||
             creature.type == SmallCreatureType::MOTH) {
        // Fluttering flight - add sinusoidal wobble
        float flutter = sinf(creature.animationTime * 5.0f) * 0.3f;
        targetVel.x = dx * maxSpeed * 0.5f + flutter;
        targetVel.y = dy * maxSpeed * 0.3f + cosf(creature.animationTime * 3.0f) * 0.2f;
        targetVel.z = dz * maxSpeed * 0.5f;
    }
    else if (creature.type == SmallCreatureType::FLY ||
             creature.type == SmallCreatureType::GNAT ||
             creature.type == SmallCreatureType::MOSQUITO) {
        // Erratic flight
        static thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> noise(-1.0f, 1.0f);

        targetVel.x = dx * maxSpeed + noise(rng) * maxSpeed * 0.5f;
        targetVel.y = dy * maxSpeed * 0.5f + noise(rng) * 0.3f;
        targetVel.z = dz * maxSpeed + noise(rng) * maxSpeed * 0.5f;
    }
    else if (isBee(creature.type) || creature.type == SmallCreatureType::WASP) {
        // Direct, purposeful flight
        targetVel.x = dx * maxSpeed;
        targetVel.y = dy * maxSpeed;
        targetVel.z = dz * maxSpeed;
    }
    else {
        // Default flying behavior
        targetVel.x = dx * maxSpeed;
        targetVel.y = dy * maxSpeed * 0.7f;
        targetVel.z = dz * maxSpeed;
    }

    return targetVel;
}

XMFLOAT3 SmallCreaturePhysics::calculateSwimmingVelocity(const SmallCreature& creature,
                                                          const XMFLOAT3& targetPos,
                                                          float waterLevel) {
    auto props = getProperties(creature.type);

    float dx = targetPos.x - creature.position.x;
    float dy = targetPos.y - creature.position.y;
    float dz = targetPos.z - creature.position.z;
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);

    if (dist < 0.001f) return { 0, 0, 0 };

    float speed = props.baseSpeed * 0.5f * creature.genome->speed;

    // Amphibians swim well
    if (isAmphibian(creature.type)) {
        speed = props.baseSpeed * creature.genome->speed;
    }

    XMFLOAT3 vel;
    vel.x = (dx / dist) * speed;
    vel.y = (dy / dist) * speed * 0.5f; // Slower vertical movement
    vel.z = (dz / dist) * speed;

    return vel;
}

XMFLOAT3 SmallCreaturePhysics::applyDrag(const XMFLOAT3& velocity, float size,
                                          SurfaceType surface) {
    float dragCoeff = 0.1f;

    switch (surface) {
        case SurfaceType::AIR:
            dragCoeff = 0.05f; // Low air resistance for small creatures
            break;
        case SurfaceType::WATER_SURFACE:
            dragCoeff = 0.3f;
            break;
        case SurfaceType::UNDERWATER:
            dragCoeff = 0.5f;
            break;
        case SurfaceType::GRASS:
            dragCoeff = 0.2f;
            break;
        default:
            dragCoeff = 0.1f;
    }

    // Smaller creatures experience relatively more drag
    dragCoeff *= (1.0f + 1.0f / (size * 10.0f + 0.1f));

    float retention = 1.0f - dragCoeff;
    retention = std::clamp(retention, 0.8f, 0.99f);

    return { velocity.x * retention, velocity.y * retention, velocity.z * retention };
}

float SmallCreaturePhysics::getMaxSpeed(const SmallCreature& creature, SurfaceType surface) {
    auto props = getProperties(creature.type);
    float baseSpeed = props.baseSpeed * creature.genome->speed;

    switch (surface) {
        case SurfaceType::AIR:
            return baseSpeed * 2.0f; // Flying is fast
        case SurfaceType::UNDERWATER:
            return baseSpeed * 0.5f; // Swimming is slower
        case SurfaceType::GRASS:
            return baseSpeed * 0.8f; // Grass slows ground movement
        case SurfaceType::TREE_BARK:
            return baseSpeed * PhysicsConstants::MAX_CLIMB_SPEED_MULT;
        default:
            return baseSpeed;
    }
}

XMFLOAT3 SmallCreaturePhysics::steerTowards(const XMFLOAT3& currentVel,
                                             const XMFLOAT3& targetVel,
                                             float maxSpeed, float steerStrength) {
    XMFLOAT3 steer;
    steer.x = targetVel.x - currentVel.x;
    steer.y = targetVel.y - currentVel.y;
    steer.z = targetVel.z - currentVel.z;

    // Limit steering force
    float steerMag = sqrtf(steer.x * steer.x + steer.y * steer.y + steer.z * steer.z);
    float maxSteer = maxSpeed * steerStrength;
    if (steerMag > maxSteer) {
        steer.x = (steer.x / steerMag) * maxSteer;
        steer.y = (steer.y / steerMag) * maxSteer;
        steer.z = (steer.z / steerMag) * maxSteer;
    }

    XMFLOAT3 newVel;
    newVel.x = currentVel.x + steer.x;
    newVel.y = currentVel.y + steer.y;
    newVel.z = currentVel.z + steer.z;

    // Clamp to max speed
    float velMag = sqrtf(newVel.x * newVel.x + newVel.y * newVel.y + newVel.z * newVel.z);
    if (velMag > maxSpeed) {
        newVel.x = (newVel.x / velMag) * maxSpeed;
        newVel.y = (newVel.y / velMag) * maxSpeed;
        newVel.z = (newVel.z / velMag) * maxSpeed;
    }

    return newVel;
}

// =============================================================================
// InsectFlight Implementation
// =============================================================================

XMFLOAT3 InsectFlight::calculateHover(const SmallCreature& creature,
                                       float targetHeight, float deltaTime) {
    XMFLOAT3 vel = creature.velocity;

    // PD controller for height
    float heightError = targetHeight - creature.position.y;
    float verticalVel = creature.velocity.y;

    float kP = 5.0f;
    float kD = 2.0f;

    vel.y = kP * heightError - kD * verticalVel;

    // Add slight horizontal drift
    vel.x += sinf(creature.animationTime * 2.0f) * 0.05f;
    vel.z += cosf(creature.animationTime * 1.7f) * 0.05f;

    return vel;
}

XMFLOAT3 InsectFlight::calculateDart(const SmallCreature& creature,
                                      const XMFLOAT3& targetPos) {
    float dx = targetPos.x - creature.position.x;
    float dy = targetPos.y - creature.position.y;
    float dz = targetPos.z - creature.position.z;
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);

    if (dist < 0.01f) return { 0, 0, 0 };

    // Dragonflies dart quickly, then hover
    auto props = getProperties(creature.type);
    float speed = props.baseSpeed * 3.0f; // Very fast

    return {
        (dx / dist) * speed,
        (dy / dist) * speed,
        (dz / dist) * speed
    };
}

XMFLOAT3 InsectFlight::calculateErratic(const SmallCreature& creature,
                                         float deltaTime, std::mt19937& rng) {
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    auto props = getProperties(creature.type);
    float speed = props.baseSpeed;

    // Random direction changes
    XMFLOAT3 vel;
    vel.x = creature.velocity.x + dist(rng) * speed * 0.5f;
    vel.y = creature.velocity.y + dist(rng) * speed * 0.3f;
    vel.z = creature.velocity.z + dist(rng) * speed * 0.5f;

    // Clamp speed
    float velMag = sqrtf(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);
    if (velMag > speed * 1.5f) {
        vel.x = (vel.x / velMag) * speed * 1.5f;
        vel.y = (vel.y / velMag) * speed * 1.5f;
        vel.z = (vel.z / velMag) * speed * 1.5f;
    }

    return vel;
}

XMFLOAT3 InsectFlight::calculateGlide(const SmallCreature& creature,
                                       const XMFLOAT3& windDirection) {
    auto props = getProperties(creature.type);

    // Butterflies glide on thermals
    XMFLOAT3 vel;
    vel.x = creature.velocity.x * 0.95f + windDirection.x * 0.1f;
    vel.y = creature.velocity.y * 0.95f - 0.05f; // Slow descent
    vel.z = creature.velocity.z * 0.95f + windDirection.z * 0.1f;

    // Occasional wing flap to gain altitude
    if (creature.position.y < 2.0f) {
        vel.y += 0.3f;
    }

    return vel;
}

XMFLOAT3 InsectFlight::calculateSwarmForce(const SmallCreature& creature,
                                            const std::vector<SmallCreature*>& neighbors) {
    if (neighbors.empty()) return { 0, 0, 0 };

    XMFLOAT3 separation = { 0, 0, 0 };
    XMFLOAT3 alignment = { 0, 0, 0 };
    XMFLOAT3 cohesion = { 0, 0, 0 };

    int count = 0;
    for (auto* other : neighbors) {
        if (other->id == creature.id) continue;

        float dx = other->position.x - creature.position.x;
        float dy = other->position.y - creature.position.y;
        float dz = other->position.z - creature.position.z;
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);

        if (dist < 0.01f) continue;

        // Separation - avoid crowding
        if (dist < 0.2f) {
            separation.x -= dx / dist;
            separation.y -= dy / dist;
            separation.z -= dz / dist;
        }

        // Alignment - match velocity
        alignment.x += other->velocity.x;
        alignment.y += other->velocity.y;
        alignment.z += other->velocity.z;

        // Cohesion - move towards center
        cohesion.x += dx;
        cohesion.y += dy;
        cohesion.z += dz;

        ++count;
    }

    if (count > 0) {
        alignment.x /= count;
        alignment.y /= count;
        alignment.z /= count;

        cohesion.x /= count;
        cohesion.y /= count;
        cohesion.z /= count;
    }

    // Combine forces
    float sepWeight = 1.5f;
    float alignWeight = 1.0f;
    float cohWeight = 0.8f;

    XMFLOAT3 force;
    force.x = separation.x * sepWeight + alignment.x * alignWeight + cohesion.x * cohWeight;
    force.y = separation.y * sepWeight + alignment.y * alignWeight + cohesion.y * cohWeight;
    force.z = separation.z * sepWeight + alignment.z * alignWeight + cohesion.z * cohWeight;

    return force;
}

// =============================================================================
// ArachnidMovement Implementation
// =============================================================================

XMFLOAT3 ArachnidMovement::calculateWebMovement(const SmallCreature& creature,
                                                 const XMFLOAT3& webCenter,
                                                 float webRadius) {
    // Spider moves towards center of web
    float dx = webCenter.x - creature.position.x;
    float dy = webCenter.y - creature.position.y;
    float dz = webCenter.z - creature.position.z;
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);

    if (dist < 0.1f) return { 0, 0, 0 };

    auto props = getProperties(creature.type);
    float speed = props.baseSpeed * 0.3f; // Slow on web

    // Constrain to web surface (simplified as disc)
    return {
        (dx / dist) * speed,
        (dy / dist) * speed * 0.5f,
        (dz / dist) * speed
    };
}

XMFLOAT3 ArachnidMovement::calculateRappel(const SmallCreature& creature,
                                            float targetHeight) {
    float dy = targetHeight - creature.position.y;
    float speed = 0.1f; // Slow controlled descent/ascent

    return { 0, (dy > 0 ? 1.0f : -1.0f) * speed, 0 };
}

XMFLOAT3 ArachnidMovement::calculatePounce(const SmallCreature& creature,
                                            const XMFLOAT3& targetPos) {
    return SmallCreaturePhysics::calculateJump(creature, targetPos, 1.5f);
}

// =============================================================================
// BurrowingMovement Implementation
// =============================================================================

XMFLOAT3 BurrowingMovement::calculateTunneling(const SmallCreature& creature,
                                                const XMFLOAT3& targetPos,
                                                float soilHardness) {
    float dx = targetPos.x - creature.position.x;
    float dy = targetPos.y - creature.position.y;
    float dz = targetPos.z - creature.position.z;
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);

    if (dist < 0.01f) return { 0, 0, 0 };

    auto props = getProperties(creature.type);

    // Harder soil = slower movement
    float speedMult = 1.0f / (1.0f + soilHardness * 0.1f);
    float speed = props.baseSpeed * PhysicsConstants::BURROW_SPEED_MULT * speedMult;

    return {
        (dx / dist) * speed,
        (dy / dist) * speed,
        (dz / dist) * speed
    };
}

XMFLOAT3 BurrowingMovement::calculateEmergence(const SmallCreature& creature,
                                                float groundLevel) {
    float dy = groundLevel - creature.position.y;

    if (dy < 0) return { 0, 0, 0 }; // Already above ground

    auto props = getProperties(creature.type);
    float speed = props.baseSpeed * 0.5f;

    return { 0, speed, 0 };
}

} // namespace small
