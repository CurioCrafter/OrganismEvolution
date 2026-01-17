#include "SwimBehavior.h"
#include "../environment/Terrain.h"
#include "Creature.h"
#include "CreatureType.h"
#include <algorithm>
#include <cmath>

// Water level constant - MUST match terrain waterLevel (0.35f) * heightScale (30.0f) = 10.5f
// This is the Y coordinate where the water surface is located in world space
static constexpr float WATER_SURFACE_HEIGHT = 10.5f;

SwimBehavior::SwimBehavior() : m_config() {}

SwimBehavior::SwimBehavior(const SwimPhysicsConfig& config) : m_config(config) {}

bool SwimBehavior::isInWater(const glm::vec3& pos, const Terrain& terrain) {
    float waterSurface = getWaterSurfaceHeight(pos.x, pos.z, terrain);
    float seaFloor = getSeaFloorHeight(pos.x, pos.z, terrain);
    // Fish are in water if below the surface and above the sea floor
    // Water surface is at Y=10.5, terrain below that is underwater
    return pos.y < waterSurface && pos.y > seaFloor;
}

// Static helper to get the water surface height constant
float SwimBehavior::getWaterLevelConstant() {
    return WATER_SURFACE_HEIGHT;
}

float SwimBehavior::getWaterSurfaceHeight(float x, float z, const Terrain& terrain) {
    (void)x; (void)z; (void)terrain;
    // For now, use a constant water level
    // In a more advanced system, this could vary with waves
    return WATER_SURFACE_HEIGHT;
}

float SwimBehavior::getWaterDepth(const glm::vec3& pos, const Terrain& terrain) {
    float waterSurface = getWaterSurfaceHeight(pos.x, pos.z, terrain);
    return waterSurface - pos.y; // Positive when underwater
}

float SwimBehavior::getSeaFloorHeight(float x, float z, const Terrain& terrain) {
    float terrainHeight = terrain.getHeight(x, z);
    // Sea floor is terrain height, but capped at water level
    return std::min(terrainHeight, WATER_SURFACE_HEIGHT - 0.5f);
}

glm::vec3 SwimBehavior::calculateBuoyancy(
    const glm::vec3& position,
    const glm::vec3& velocity,
    float targetDepth,
    const Terrain& terrain
) const {
    float currentDepth = getWaterDepth(position, terrain);
    float depthError = targetDepth - currentDepth;

    // Buoyancy force proportional to depth error
    float buoyancyForce = depthError * m_config.buoyancyStrength;

    // Add damping to prevent oscillation
    float verticalDamping = -velocity.y * m_config.buoyancyDamping;

    return glm::vec3(0.0f, buoyancyForce + verticalDamping, 0.0f);
}

glm::vec3 SwimBehavior::calculateDrag(
    const glm::vec3& velocity,
    const glm::vec3& forward
) const {
    float speed = glm::length(velocity);
    if (speed < 0.001f) {
        return glm::vec3(0.0f);
    }

    glm::vec3 velocityDir = velocity / speed;

    // Calculate how aligned velocity is with forward direction
    float forwardAlignment = glm::dot(velocityDir, forward);

    // Decompose velocity into forward and lateral components
    glm::vec3 forwardVel = forward * glm::dot(velocity, forward);
    glm::vec3 lateralVel = velocity - forwardVel;
    lateralVel.y = 0.0f; // Handle vertical separately
    float verticalVel = velocity.y;

    // Apply different drag coefficients
    glm::vec3 dragForce(0.0f);

    // Forward drag (low - streamlined)
    float forwardSpeed = glm::length(forwardVel);
    if (forwardSpeed > 0.001f) {
        dragForce -= glm::normalize(forwardVel) * forwardSpeed * forwardSpeed * m_config.forwardDrag;
    }

    // Lateral drag (high - not streamlined)
    float lateralSpeed = glm::length(lateralVel);
    if (lateralSpeed > 0.001f) {
        dragForce -= glm::normalize(lateralVel) * lateralSpeed * lateralSpeed * m_config.lateralDrag;
    }

    // Vertical drag
    if (std::abs(verticalVel) > 0.001f) {
        float verticalDrag = verticalVel * std::abs(verticalVel) * m_config.verticalDrag;
        dragForce.y -= verticalDrag;
    }

    return dragForce;
}

glm::vec3 SwimBehavior::calculateThrust(
    const glm::vec3& forward,
    float swimPhase,
    float speed,
    bool isBursting
) const {
    // Thrust varies with swim phase (tail beat)
    float phaseFactor = 0.7f + 0.3f * std::sin(swimPhase);

    float thrustMagnitude = m_config.thrustPower * phaseFactor;

    if (isBursting) {
        thrustMagnitude *= m_config.burstMultiplier;
    }

    return forward * thrustMagnitude;
}

SchoolingForces SwimBehavior::calculateSchoolingForces(
    const glm::vec3& position,
    const glm::vec3& velocity,
    const std::vector<Creature*>& nearbyCreatures,
    float schoolRadius,
    float separationDistance
) const {
    SchoolingForces forces;
    forces.separation = glm::vec3(0.0f);
    forces.alignment = glm::vec3(0.0f);
    forces.cohesion = glm::vec3(0.0f);
    forces.predatorAvoidance = glm::vec3(0.0f);
    forces.neighborCount = 0;

    if (nearbyCreatures.empty()) {
        return forces;
    }

    glm::vec3 centerOfMass(0.0f);
    glm::vec3 averageVelocity(0.0f);
    int alignmentCount = 0;

    for (Creature* other : nearbyCreatures) {
        if (!other || !other->isAlive()) continue;

        glm::vec3 toOther = other->getPosition() - position;
        float dist = glm::length(toOther);

        if (dist < 0.01f || dist > schoolRadius) continue;

        // Check if this is a predator
        if (isPredator(other->getType()) && !isAquatic(other->getType())) {
            // This is a land/air predator near water - flee!
            forces.predatorAvoidance -= glm::normalize(toOther) * (schoolRadius / dist);
            continue;
        }

        // Only school with same type
        if (!isAquatic(other->getType())) continue;

        forces.neighborCount++;

        // Separation: avoid crowding
        if (dist < separationDistance) {
            forces.separation -= glm::normalize(toOther) / dist;
        }

        // Alignment: match velocity
        averageVelocity += other->getVelocity();
        alignmentCount++;

        // Cohesion: move toward center
        centerOfMass += other->getPosition();
    }

    // Average and normalize forces
    if (alignmentCount > 0) {
        averageVelocity /= static_cast<float>(alignmentCount);
        if (glm::length(averageVelocity) > 0.01f) {
            forces.alignment = glm::normalize(averageVelocity);
        }

        centerOfMass /= static_cast<float>(alignmentCount);
        glm::vec3 toCOM = centerOfMass - position;
        if (glm::length(toCOM) > 0.01f) {
            forces.cohesion = glm::normalize(toCOM);
        }
    }

    if (glm::length(forces.separation) > 0.01f) {
        forces.separation = glm::normalize(forces.separation);
    }

    if (glm::length(forces.predatorAvoidance) > 0.01f) {
        forces.predatorAvoidance = glm::normalize(forces.predatorAvoidance);
    }

    return forces;
}

float SwimBehavior::clampDepth(
    float targetDepth,
    const glm::vec3& position,
    const Terrain& terrain,
    float minSurfaceDistance,
    float minFloorDistance
) const {
    float waterSurface = getWaterSurfaceHeight(position.x, position.z, terrain);
    float seaFloor = getSeaFloorHeight(position.x, position.z, terrain);

    float maxDepth = waterSurface - seaFloor - minFloorDistance - minSurfaceDistance;

    // Clamp target depth to valid range
    targetDepth = std::max(minSurfaceDistance, targetDepth);
    targetDepth = std::min(maxDepth, targetDepth);

    return targetDepth;
}

void SwimBehavior::updatePhysics(
    glm::vec3& position,
    glm::vec3& velocity,
    const glm::vec3& steeringForce,
    float targetDepth,
    float maxSpeed,
    float deltaTime,
    const Terrain& terrain,
    bool isBursting
) {
    // Update swim phase
    float swimSpeed = glm::length(velocity) / maxSpeed;
    m_swimPhase += swimSpeed * 6.28318f * 2.0f * deltaTime;
    if (m_swimPhase > 628.318f) m_swimPhase -= 628.318f;

    // Calculate forward direction from velocity
    glm::vec3 forward(0.0f, 0.0f, 1.0f);
    float horizontalSpeed = glm::length(glm::vec2(velocity.x, velocity.z));
    if (horizontalSpeed > 0.1f) {
        forward = glm::normalize(glm::vec3(velocity.x, 0.0f, velocity.z));
    }

    // Calculate forces
    glm::vec3 totalForce = steeringForce;

    // Add buoyancy
    totalForce += calculateBuoyancy(position, velocity, targetDepth, terrain);

    // Add drag
    totalForce += calculateDrag(velocity, forward);

    // Add thrust (proportional to how hard we're trying to move)
    float steeringMagnitude = glm::length(steeringForce);
    if (steeringMagnitude > 0.1f) {
        totalForce += calculateThrust(forward, m_swimPhase, horizontalSpeed, isBursting);
    }

    // Apply forces to velocity
    velocity += totalForce * deltaTime;

    // Limit speed
    float speed = glm::length(velocity);
    float effectiveMaxSpeed = isBursting ? maxSpeed * 1.5f : maxSpeed;
    if (speed > effectiveMaxSpeed) {
        velocity = glm::normalize(velocity) * effectiveMaxSpeed;
    }

    // Update position
    position += velocity * deltaTime;

    // Ensure we stay in water
    float waterSurface = getWaterSurfaceHeight(position.x, position.z, terrain);
    float seaFloor = getSeaFloorHeight(position.x, position.z, terrain);

    // Clamp to water bounds
    if (position.y > waterSurface - 0.5f) {
        position.y = waterSurface - 0.5f;
        velocity.y = std::min(velocity.y, 0.0f);
    }
    if (position.y < seaFloor + 0.5f) {
        position.y = seaFloor + 0.5f;
        velocity.y = std::max(velocity.y, 0.0f);
    }
}

float SwimBehavior::calculateEnergyCost(float speed, float maxSpeed, float deltaTime) const {
    float normalizedSpeed = maxSpeed > 0.0f ? speed / maxSpeed : 0.0f;

    float cost = m_config.idleEnergyCost;

    if (normalizedSpeed > 0.1f) {
        // Cruising cost scales with speed squared (drag)
        cost = m_config.cruiseEnergyCost * normalizedSpeed * normalizedSpeed;
    }

    if (m_mode == SwimMode::FLEEING || m_mode == SwimMode::HUNTING) {
        // Burst swimming is expensive
        cost = m_config.burstEnergyCost;
    }

    return cost * deltaTime;
}

// Namespace implementations
namespace AquaticBehavior {

Creature* findNearestPredator(
    const glm::vec3& position,
    const std::vector<Creature*>& creatures,
    float detectionRange
) {
    Creature* nearest = nullptr;
    float nearestDist = detectionRange;

    for (Creature* c : creatures) {
        if (!c || !c->isAlive()) continue;
        if (!isPredator(c->getType())) continue;

        float dist = glm::length(c->getPosition() - position);
        if (dist < nearestDist) {
            nearestDist = dist;
            nearest = c;
        }
    }

    return nearest;
}

Creature* findNearestPrey(
    const glm::vec3& position,
    const std::vector<Creature*>& creatures,
    float huntingRange,
    float minPreySize,
    float maxPreySize
) {
    Creature* nearest = nullptr;
    float nearestDist = huntingRange;

    for (Creature* c : creatures) {
        if (!c || !c->isAlive()) continue;

        // Skip non-aquatic creatures (can't catch land animals from water)
        if (!isAquatic(c->getType())) continue;

        float preySize = c->getGenome().size;
        if (preySize < minPreySize || preySize > maxPreySize) continue;

        float dist = glm::length(c->getPosition() - position);
        if (dist < nearestDist) {
            nearestDist = dist;
            nearest = c;
        }
    }

    return nearest;
}

glm::vec3 calculateFleeDirection(
    const glm::vec3& position,
    const std::vector<Creature*>& predators,
    const PredatorAvoidanceConfig& config
) {
    glm::vec3 fleeDir(0.0f);

    for (Creature* predator : predators) {
        if (!predator || !predator->isAlive()) continue;

        glm::vec3 toPredator = predator->getPosition() - position;
        float dist = glm::length(toPredator);

        if (dist > config.detectionRange || dist < 0.01f) continue;

        // Flee strength increases as predator gets closer
        float urgency = 1.0f - (dist / config.detectionRange);
        urgency *= urgency; // Quadratic increase

        fleeDir -= glm::normalize(toPredator) * urgency * config.fleeStrength;
    }

    // Add scatter angle for unpredictability
    if (glm::length(fleeDir) > 0.01f) {
        float scatter = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * config.scatterAngle;
        float s = std::sin(scatter);
        float c = std::cos(scatter);
        float newX = fleeDir.x * c - fleeDir.z * s;
        float newZ = fleeDir.x * s + fleeDir.z * c;
        fleeDir.x = newX;
        fleeDir.z = newZ;
    }

    return fleeDir;
}

bool shouldScatter(
    const glm::vec3& position,
    const std::vector<Creature*>& predators,
    float panicRange
) {
    for (Creature* predator : predators) {
        if (!predator || !predator->isAlive()) continue;

        float dist = glm::length(predator->getPosition() - position);
        if (dist < panicRange) {
            return true;
        }
    }
    return false;
}

float calculateTargetDepth(
    float preferredDepth,
    float predatorDepth,
    float foodDepth,
    bool hasPredatorNearby,
    bool hasFoodNearby
) {
    float targetDepth = preferredDepth;

    if (hasPredatorNearby) {
        // Try to escape by going to different depth
        if (predatorDepth < preferredDepth) {
            targetDepth = preferredDepth + 5.0f; // Go deeper
        } else {
            targetDepth = preferredDepth - 3.0f; // Go shallower
        }
    } else if (hasFoodNearby) {
        // Move toward food depth
        targetDepth = glm::mix(preferredDepth, foodDepth, 0.5f);
    }

    // Clamp to reasonable bounds
    targetDepth = std::max(0.5f, targetDepth);
    targetDepth = std::min(20.0f, targetDepth);

    return targetDepth;
}

} // namespace AquaticBehavior

// ============================================================================
// NEW DEPTH/PRESSURE METHODS
// ============================================================================

float SwimBehavior::calculateWaterPressure(float depth) {
    // Pressure increases by ~1 atm per 10 meters
    // At surface (depth=0): 1 atm
    // At 10m: 2 atm, at 100m: 11 atm
    return 1.0f + (std::max(0.0f, depth) / 10.0f);
}

PressureEffect SwimBehavior::calculatePressureEffect(float depth) const {
    PressureEffect effect;

    float minSafe = m_config.minDepth;
    float maxSafe = m_config.maxDepth;
    float preferred = m_config.preferredDepth;
    float resistance = m_config.pressureResistance;

    // Check if in safe zone
    effect.isInSafeZone = (depth >= minSafe && depth <= maxSafe);

    if (effect.isInSafeZone) {
        // Within safe range - minor effects based on distance from preferred
        float distFromPreferred = std::abs(depth - preferred);
        float optimalRange = (maxSafe - minSafe) * 0.3f;

        if (distFromPreferred > optimalRange) {
            // Some performance penalty outside optimal range
            float penalty = (distFromPreferred - optimalRange) / (maxSafe - minSafe);
            effect.speedModifier = 1.0f - penalty * 0.2f * (1.0f / resistance);
            effect.energyModifier = 1.0f + penalty * 0.3f * (1.0f / resistance);
        }
    } else {
        // Outside safe depth - take damage and severe penalties
        float outsideRange = 0.0f;

        if (depth < minSafe) {
            // Too shallow - mainly discomfort, not pressure damage
            outsideRange = minSafe - depth;
            effect.damagePerSecond = m_config.pressureDamageRate * outsideRange * 0.5f / resistance;
        } else {
            // Too deep - pressure damage
            outsideRange = depth - maxSafe;
            float pressureMultiplier = 1.0f + outsideRange * 0.1f;
            effect.damagePerSecond = m_config.pressureDamageRate * outsideRange * pressureMultiplier / resistance;
        }

        // Severe performance penalties
        float severityFactor = std::min(outsideRange / 20.0f, 1.0f);
        effect.speedModifier = 1.0f - severityFactor * 0.5f;
        effect.energyModifier = 1.0f + severityFactor * 1.0f;
    }

    // Clamp modifiers
    effect.speedModifier = std::max(0.3f, effect.speedModifier);
    effect.energyModifier = std::min(3.0f, effect.energyModifier);

    return effect;
}

bool SwimBehavior::canSurviveAtDepth(float depth) const {
    return depth >= m_config.minDepth && depth <= m_config.maxDepth;
}

float SwimBehavior::getDepthAdjustmentDirection(float currentDepth) const {
    if (currentDepth < m_config.minDepth) {
        return 1.0f;  // Need to go deeper (positive = down)
    } else if (currentDepth > m_config.maxDepth) {
        return -1.0f; // Need to go shallower (negative = up)
    }

    // Within safe range - move toward preferred
    float diff = m_config.preferredDepth - currentDepth;
    return diff * 0.1f; // Gentle adjustment
}

// ============================================================================
// NEW CURRENT METHODS
// ============================================================================

float SwimBehavior::noise3D(float x, float y, float z) const {
    // Simple 3D noise based on sine waves (for turbulence)
    float n = std::sin(x * 1.3f + y * 0.7f) * std::cos(z * 1.1f + x * 0.9f);
    n += std::sin(y * 1.7f + z * 0.8f) * std::cos(x * 1.2f + y * 0.6f) * 0.5f;
    n += std::sin(z * 1.5f + x * 1.0f) * std::cos(y * 1.4f + z * 0.5f) * 0.25f;
    return n * 0.5f; // Normalize roughly to [-0.5, 0.5]
}

glm::vec3 SwimBehavior::curlNoise(const glm::vec3& pos) const {
    // Curl noise for divergence-free flow (realistic fluid motion)
    const float eps = 0.01f;

    float dx_y = noise3D(pos.x + eps, pos.y, pos.z) - noise3D(pos.x - eps, pos.y, pos.z);
    float dx_z = noise3D(pos.x, pos.y + eps, pos.z) - noise3D(pos.x, pos.y - eps, pos.z);

    float dy_x = noise3D(pos.x, pos.y + eps, pos.z) - noise3D(pos.x, pos.y - eps, pos.z);
    float dy_z = noise3D(pos.x, pos.y, pos.z + eps) - noise3D(pos.x, pos.y, pos.z - eps);

    float dz_x = noise3D(pos.x, pos.y, pos.z + eps) - noise3D(pos.x, pos.y, pos.z - eps);
    float dz_y = noise3D(pos.x + eps, pos.y, pos.z) - noise3D(pos.x - eps, pos.y, pos.z);

    // Curl = (dFz/dy - dFy/dz, dFx/dz - dFz/dx, dFy/dx - dFx/dy)
    return glm::vec3(
        dz_y - dy_z,
        dx_z - dz_x,
        dy_x - dx_y
    ) / (2.0f * eps);
}

glm::vec3 SwimBehavior::calculateCurrentAtPosition(
    const glm::vec3& position,
    float time,
    const Terrain& terrain
) const {
    float depth = getWaterDepth(position, terrain);
    if (depth <= 0.0f) return glm::vec3(0.0f); // Above water

    glm::vec3 current = m_currentConfig.baseDirection * m_currentConfig.baseStrength;

    // Apply depth falloff (currents weaker at depth)
    float depthFactor = std::exp(-depth * m_currentConfig.depthFalloff);
    current *= depthFactor;

    // Surface multiplier (currents stronger near surface)
    if (depth < 5.0f) {
        float surfaceFactor = 1.0f + (5.0f - depth) / 5.0f * (m_currentConfig.surfaceMultiplier - 1.0f);
        current *= surfaceFactor;
    }

    // Apply current type modifiers
    switch (m_currentConfig.type) {
        case CurrentType::TIDAL: {
            // Sinusoidal reversal
            float tidalPhase = std::sin(time * 6.28318f / m_currentConfig.tidalPeriod);
            current *= tidalPhase;
            break;
        }

        case CurrentType::TURBULENT: {
            // Add curl noise
            glm::vec3 turbulence = curlNoise(position * m_currentConfig.turbulenceScale + time * 0.1f);
            current += turbulence * m_currentConfig.baseStrength * 0.5f;
            break;
        }

        case CurrentType::UPWELLING: {
            // Vertical component based on position
            float upwellStrength = std::sin(position.x * 0.05f) * std::cos(position.z * 0.05f);
            current.y += upwellStrength * m_currentConfig.baseStrength * 0.3f;
            break;
        }

        case CurrentType::GYRE: {
            // Circular flow around center
            glm::vec3 toCenter = -position;
            toCenter.y = 0.0f;
            float dist = glm::length(toCenter);
            if (dist > 0.1f) {
                glm::vec3 perpendicular(-toCenter.z, 0.0f, toCenter.x);
                perpendicular = glm::normalize(perpendicular);
                float gyreStrength = m_currentConfig.baseStrength * (1.0f - std::min(dist / 200.0f, 1.0f));
                current = perpendicular * gyreStrength;
            }
            break;
        }

        default:
            break;
    }

    return current;
}

glm::vec3 SwimBehavior::calculateTurbulence(const glm::vec3& position, float time) const {
    return curlNoise(position * m_currentConfig.turbulenceScale + time * 0.2f) *
           m_currentConfig.baseStrength * 0.3f;
}

glm::vec3 SwimBehavior::calculateCurrentEffect(
    const glm::vec3& position,
    const glm::vec3& velocity,
    float time,
    const Terrain& terrain
) const {
    glm::vec3 current = calculateCurrentAtPosition(position, time, terrain);

    // Current pushes creature - effect is difference between current and creature velocity
    // scaled by how well the creature can resist the current (based on size/strength)
    glm::vec3 relativeVelocity = current - velocity;

    // Fish naturally resist currents somewhat
    float resistance = 0.3f;
    return relativeVelocity * resistance;
}

// ============================================================================
// NEW BREACHING METHODS
// ============================================================================

bool SwimBehavior::canBreach(const glm::vec3& velocity, float currentDepth) const {
    // Need sufficient upward velocity and be near surface
    float upwardSpeed = -velocity.y; // Negative Y is up in water
    float totalSpeed = glm::length(velocity);

    return totalSpeed >= m_config.breachMinSpeed &&
           upwardSpeed > totalSpeed * 0.5f && // Mostly upward
           currentDepth < 3.0f; // Near surface
}

bool SwimBehavior::initiateBreaching(
    glm::vec3& velocity,
    const glm::vec3& position,
    const Terrain& terrain
) {
    if (!canBreach(velocity, getWaterDepth(position, terrain))) {
        return false;
    }

    // Adjust velocity for breach angle
    float speed = glm::length(velocity);
    glm::vec3 horizontalDir = glm::normalize(glm::vec3(velocity.x, 0.0f, velocity.z));

    // Set breach trajectory
    velocity = glm::vec3(
        horizontalDir.x * speed * std::sin(m_config.breachAngle),
        speed * std::cos(m_config.breachAngle), // Upward
        horizontalDir.z * speed * std::sin(m_config.breachAngle)
    );

    // Record breach state
    m_breachState.isAirborne = true;
    m_breachState.airborneTime = 0.0f;
    m_breachState.exitVelocity = velocity;
    m_breachState.exitPosition = position;
    m_breachState.maxHeight = 0.0f;
    m_breachState.rotationAngle = 0.0f;

    m_mode = SwimMode::BREACHING;
    return true;
}

void SwimBehavior::updateBreachingPhysics(
    glm::vec3& position,
    glm::vec3& velocity,
    float deltaTime,
    const Terrain& terrain
) {
    if (!m_breachState.isAirborne) return;

    // Apply gravity (in air, Y is up)
    velocity.y -= m_config.airborneGravity * deltaTime;

    // Air drag (minimal)
    velocity *= (1.0f - 0.01f * deltaTime);

    // Update position
    position += velocity * deltaTime;

    // Track statistics
    m_breachState.airborneTime += deltaTime;
    float heightAboveWater = position.y - getWaterSurfaceHeight(position.x, position.z, terrain);
    m_breachState.maxHeight = std::max(m_breachState.maxHeight, heightAboveWater);

    // Add rotation for visual effect
    m_breachState.rotationAngle += deltaTime * 3.14159f; // Half rotation per second

    // Check for water reentry
    if (position.y < getWaterSurfaceHeight(position.x, position.z, terrain)) {
        handleWaterReentry(position, velocity, terrain);
    }
}

void SwimBehavior::handleWaterReentry(
    glm::vec3& position,
    glm::vec3& velocity,
    const Terrain& terrain
) {
    float waterSurface = getWaterSurfaceHeight(position.x, position.z, terrain);

    // Clamp position to just below surface
    position.y = waterSurface - 0.5f;

    // Apply reentry drag (splash effect)
    float reentrySpeed = glm::length(velocity);
    velocity *= (1.0f / (1.0f + m_config.reentryDrag));

    // Redirect velocity downward if hitting at steep angle
    if (velocity.y > 0.0f) {
        velocity.y = -std::abs(velocity.y) * 0.5f;
    }

    // Reset breach state
    m_breachState.isAirborne = false;
    m_mode = SwimMode::CRUISING;
}

// ============================================================================
// NEW AIR-BREATHING METHODS
// ============================================================================

void SwimBehavior::updateOxygenLevel(
    float& oxygenLevel,
    float currentDepth,
    float deltaTime,
    const Terrain& terrain
) {
    if (!m_config.canBreathAir) {
        oxygenLevel = 1.0f; // Gill breathers always have oxygen
        return;
    }

    float waterSurface = WATER_SURFACE_HEIGHT;

    if (currentDepth <= 0.5f) {
        // At surface - replenish oxygen
        float replenishRate = 1.0f / m_config.surfaceBreathTime;
        oxygenLevel = std::min(1.0f, oxygenLevel + replenishRate * deltaTime);
    } else {
        // Underwater - consume oxygen
        float consumeRate = 1.0f / m_config.breathHoldDuration;
        oxygenLevel = std::max(0.0f, oxygenLevel - consumeRate * deltaTime);
    }
}

bool SwimBehavior::needsToSurface(float oxygenLevel) const {
    if (!m_config.canBreathAir) return false;
    return oxygenLevel < 0.3f;
}

float SwimBehavior::getSurfacingUrgency(float oxygenLevel) const {
    if (!m_config.canBreathAir) return 0.0f;

    if (oxygenLevel > 0.5f) return 0.0f;
    if (oxygenLevel > 0.3f) return (0.5f - oxygenLevel) / 0.2f; // 0 to 1
    return 1.0f; // Critical
}

// ============================================================================
// ADVANCED PHYSICS UPDATE
// ============================================================================

void SwimBehavior::updatePhysicsAdvanced(
    glm::vec3& position,
    glm::vec3& velocity,
    const glm::vec3& steeringForce,
    float targetDepth,
    float maxSpeed,
    float deltaTime,
    float time,
    const Terrain& terrain,
    float& healthDamage,
    bool isBursting
) {
    // Handle breaching separately
    if (m_breachState.isAirborne) {
        updateBreachingPhysics(position, velocity, deltaTime, terrain);
        healthDamage = 0.0f;
        return;
    }

    float currentDepth = getWaterDepth(position, terrain);

    // Calculate pressure effects
    PressureEffect pressure = calculatePressureEffect(currentDepth);
    healthDamage = pressure.damagePerSecond * deltaTime;

    // Apply pressure speed modifier
    float effectiveMaxSpeed = maxSpeed * pressure.speedModifier;

    // Calculate current effects
    glm::vec3 currentForce = calculateCurrentEffect(position, velocity, time, terrain);

    // Add turbulence for realism
    if (m_currentConfig.type == CurrentType::TURBULENT) {
        currentForce += calculateTurbulence(position, time);
    }

    // Update swim phase
    float swimSpeed = glm::length(velocity) / effectiveMaxSpeed;
    m_swimPhase += swimSpeed * 6.28318f * 2.0f * deltaTime;
    if (m_swimPhase > 628.318f) m_swimPhase -= 628.318f;

    // Calculate forward direction from velocity
    glm::vec3 forward(0.0f, 0.0f, 1.0f);
    float horizontalSpeed = glm::length(glm::vec2(velocity.x, velocity.z));
    if (horizontalSpeed > 0.1f) {
        forward = glm::normalize(glm::vec3(velocity.x, 0.0f, velocity.z));
    }

    // Calculate total steering (including depth adjustment if needed)
    glm::vec3 totalSteering = steeringForce;

    // Add depth correction if outside safe zone
    if (!pressure.isInSafeZone) {
        float adjustment = getDepthAdjustmentDirection(currentDepth);
        totalSteering.y += adjustment * 5.0f; // Strong correction when unsafe
    }

    // Calculate forces
    glm::vec3 totalForce = totalSteering;

    // Add buoyancy (with swimbladder efficiency)
    glm::vec3 buoyancy = calculateBuoyancy(position, velocity, targetDepth, terrain);
    buoyancy *= m_config.swimbladderEfficiency;
    totalForce += buoyancy;

    // Add drag (modified by streamlining)
    glm::vec3 drag = calculateDrag(velocity, forward);
    drag *= (1.0f - m_config.streamlining * 0.3f);
    totalForce += drag;

    // Add current effect
    totalForce += currentForce;

    // Add thrust
    float steeringMagnitude = glm::length(totalSteering);
    if (steeringMagnitude > 0.1f) {
        totalForce += calculateThrust(forward, m_swimPhase, horizontalSpeed, isBursting);
    }

    // Apply forces to velocity (with acceleration limit)
    glm::vec3 acceleration = totalForce;
    float accelMag = glm::length(acceleration);
    if (accelMag > m_config.acceleration) {
        acceleration = acceleration / accelMag * m_config.acceleration;
    }

    velocity += acceleration * deltaTime;

    // Limit speed (with pressure modifier)
    float speed = glm::length(velocity);
    float effectiveMax = isBursting ? effectiveMaxSpeed * m_config.burstMultiplier : effectiveMaxSpeed;
    if (speed > effectiveMax) {
        velocity = glm::normalize(velocity) * effectiveMax;
    }

    // Update position
    position += velocity * deltaTime;

    // Check for breaching opportunity
    if (canBreach(velocity, currentDepth) && m_mode == SwimMode::BREACHING) {
        initiateBreaching(velocity, position, terrain);
        return;
    }

    // Ensure we stay in water
    float waterSurface = getWaterSurfaceHeight(position.x, position.z, terrain);
    float seaFloor = getSeaFloorHeight(position.x, position.z, terrain);

    // Clamp to water bounds
    if (position.y > waterSurface - 0.5f) {
        position.y = waterSurface - 0.5f;
        velocity.y = std::min(velocity.y, 0.0f);
    }
    if (position.y < seaFloor + 0.5f) {
        position.y = seaFloor + 0.5f;
        velocity.y = std::max(velocity.y, 0.0f);
    }
}

float SwimBehavior::calculateAdvancedEnergyCost(
    float speed,
    float maxSpeed,
    float depth,
    const glm::vec3& currentVelocity,
    const glm::vec3& moveDirection,
    float deltaTime
) const {
    float baseCost = calculateEnergyCost(speed, maxSpeed, deltaTime);

    // Depth pressure modifier
    PressureEffect pressure = calculatePressureEffect(depth);
    baseCost *= pressure.energyModifier;

    // Swimming against current costs more
    if (glm::length(currentVelocity) > 0.1f && glm::length(moveDirection) > 0.1f) {
        float alignment = glm::dot(
            glm::normalize(currentVelocity),
            glm::normalize(moveDirection)
        );
        // alignment: 1 = with current (easy), -1 = against (hard)
        float currentModifier = 1.0f - alignment * 0.3f;
        baseCost *= currentModifier;
    }

    // Depth change costs extra
    if (std::abs(moveDirection.y) > 0.1f) {
        baseCost += m_config.depthChangeCost * std::abs(moveDirection.y) * deltaTime;
    }

    return baseCost;
}

// ============================================================================
// AMPHIBIOUS LOCOMOTION IMPLEMENTATION (PHASE 7 - Agent 3)
// ============================================================================

float AmphibiousLocomotion::smoothBlend(float a, float b, float t) {
    // Smooth hermite interpolation
    t = std::max(0.0f, std::min(1.0f, t));
    float smoothT = t * t * (3.0f - 2.0f * t);
    return a + (b - a) * smoothT;
}

bool AmphibiousLocomotion::isInShoreZone(float waterDepth) {
    // Shore zone is defined as shallow water (-1 to 2 meters depth)
    return waterDepth > -1.0f && waterDepth < 2.0f;
}

float AmphibiousLocomotion::calculateAnimationBlend(float locomotionBlend, float waterDepth) {
    // Animation blend considers both the creature's adaptation level and current environment
    float envBlend = 0.0f;

    if (waterDepth <= 0.0f) {
        // Above water - use walk animation
        envBlend = 1.0f;
    } else if (waterDepth < 1.0f) {
        // Very shallow - blend based on depth
        envBlend = 1.0f - waterDepth;
    } else if (waterDepth < 3.0f) {
        // Shallow water - partial swim
        envBlend = (3.0f - waterDepth) / 3.0f * 0.5f;
    } else {
        // Deep water - full swim
        envBlend = 0.0f;
    }

    // Combine creature adaptation with environmental factor
    // A fully aquatic creature (locomotionBlend=0) will always swim
    // A land-adapted creature (locomotionBlend=1) will walk even in shallow water
    return smoothBlend(envBlend * 0.5f, envBlend, locomotionBlend);
}

float AmphibiousLocomotion::getMaxSpeed(float locomotionBlend, float waterDepth) const {
    if (isInShoreZone(waterDepth)) {
        // Shore zone - slower movement
        float shoreBlend = std::min(1.0f, std::abs(waterDepth) / 2.0f);
        float baseSpeed = smoothBlend(m_config.swimMaxSpeed, m_config.walkMaxSpeed, locomotionBlend);
        return smoothBlend(m_config.shoreMaxSpeed, baseSpeed, shoreBlend);
    }

    if (waterDepth > 0.0f) {
        // Underwater - blend between swim and reduced walk speed
        float underwaterWalkSpeed = m_config.walkMaxSpeed * 0.3f;  // Walking underwater is slow
        return smoothBlend(m_config.swimMaxSpeed, underwaterWalkSpeed, locomotionBlend);
    }

    // On land - blend between reduced swim speed and walk speed
    float landSwimSpeed = m_config.swimMaxSpeed * 0.1f;  // "Swimming" on land is flopping
    return smoothBlend(landSwimSpeed, m_config.walkMaxSpeed, locomotionBlend);
}

float AmphibiousLocomotion::getAcceleration(float locomotionBlend, float waterDepth) const {
    if (isInShoreZone(waterDepth)) {
        // Shore zone - poor traction
        return smoothBlend(m_config.swimAcceleration, m_config.walkAcceleration, locomotionBlend) * 0.6f;
    }

    if (waterDepth > 0.0f) {
        // Underwater - swim acceleration dominates
        return smoothBlend(m_config.swimAcceleration, m_config.walkAcceleration * 0.5f, locomotionBlend);
    }

    // On land - walk acceleration dominates
    return smoothBlend(m_config.swimAcceleration * 0.2f, m_config.walkAcceleration, locomotionBlend);
}

float AmphibiousLocomotion::getTurnRate(float locomotionBlend, float waterDepth) const {
    if (isInShoreZone(waterDepth)) {
        // Shore zone - awkward turning
        return smoothBlend(m_config.swimTurnRate, m_config.walkTurnRate, locomotionBlend) * 0.7f;
    }

    if (waterDepth > 0.0f) {
        // Underwater
        return smoothBlend(m_config.swimTurnRate, m_config.walkTurnRate * 0.5f, locomotionBlend);
    }

    // On land
    return smoothBlend(m_config.swimTurnRate * 0.3f, m_config.walkTurnRate, locomotionBlend);
}

AmphibiousVelocityResult AmphibiousLocomotion::calculateBlendedVelocity(
    const glm::vec3& currentVelocity,
    const glm::vec3& desiredDirection,
    float locomotionBlend,
    float waterDepth,
    float deltaTime
) const {
    AmphibiousVelocityResult result;

    float maxSpeed = getMaxSpeed(locomotionBlend, waterDepth);
    float acceleration = getAcceleration(locomotionBlend, waterDepth);
    float turnRate = getTurnRate(locomotionBlend, waterDepth);

    // Calculate target velocity
    glm::vec3 targetVelocity = desiredDirection * maxSpeed;

    // For land movement, zero out vertical component
    if (waterDepth <= 0.0f && locomotionBlend > 0.5f) {
        targetVelocity.y = 0.0f;
    }

    // Smoothly interpolate toward target velocity
    glm::vec3 velocityDiff = targetVelocity - currentVelocity;
    float diffLength = glm::length(velocityDiff);

    if (diffLength > 0.01f) {
        float maxChange = acceleration * deltaTime;
        if (diffLength <= maxChange) {
            result.velocity = targetVelocity;
        } else {
            result.velocity = currentVelocity + (velocityDiff / diffLength) * maxChange;
        }
    } else {
        result.velocity = currentVelocity;
    }

    // Apply shore drag if in transition zone
    if (isInShoreZone(waterDepth)) {
        float dragFactor = 1.0f - m_config.shoreDrag * deltaTime;
        dragFactor = std::max(0.5f, dragFactor);
        result.velocity *= dragFactor;
    }

    // Clamp to max speed
    float speed = glm::length(result.velocity);
    if (speed > maxSpeed) {
        result.velocity = (result.velocity / speed) * maxSpeed;
        speed = maxSpeed;
    }

    // Calculate energy cost
    float speedRatio = maxSpeed > 0.0f ? speed / maxSpeed : 0.0f;
    if (isInShoreZone(waterDepth)) {
        result.energyCost = m_config.shoreEnergyCost * speedRatio * deltaTime;
    } else if (waterDepth > 0.0f) {
        result.energyCost = smoothBlend(
            m_config.swimEnergyCost,
            m_config.walkEnergyCost * 2.0f,  // Walking underwater is exhausting
            locomotionBlend
        ) * speedRatio * deltaTime;
    } else {
        result.energyCost = smoothBlend(
            m_config.swimEnergyCost * 3.0f,  // "Swimming" on land is exhausting
            m_config.walkEnergyCost,
            locomotionBlend
        ) * speedRatio * deltaTime;
    }

    // Calculate animation blend
    result.animationBlend = calculateAnimationBlend(locomotionBlend, waterDepth);

    return result;
}
