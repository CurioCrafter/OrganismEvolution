#include "SteeringBehaviors.h"
#include "Creature.h"
#include "../utils/Random.h"
#include <algorithm>
#include <cmath>

SteeringBehaviors::SteeringBehaviors() : config() {}

SteeringBehaviors::SteeringBehaviors(const Config& config) : config(config) {}

glm::vec3 SteeringBehaviors::limit(const glm::vec3& v, float maxMagnitude) const {
    float mag = glm::length(v);
    if (mag > maxMagnitude && mag > 0.0001f) {
        return (v / mag) * maxMagnitude;
    }
    return v;
}

glm::vec3 SteeringBehaviors::setMagnitude(const glm::vec3& v, float magnitude) const {
    float mag = glm::length(v);
    if (mag > 0.0001f) {
        return (v / mag) * magnitude;
    }
    return v;
}

// SEEK: steer toward target
// steering = desired - velocity
// desired = normalize(target - position) * maxSpeed
glm::vec3 SteeringBehaviors::seek(const glm::vec3& position, const glm::vec3& velocity,
                                   const glm::vec3& target) const {
    glm::vec3 toTarget = target - position;
    float distance = glm::length(toTarget);

    if (distance < 0.0001f) {
        return glm::vec3(0.0f);
    }

    glm::vec3 desired = (toTarget / distance) * config.maxSpeed;
    glm::vec3 steering = desired - velocity;

    return limit(steering, config.maxForce);
}

// FLEE: steer away from target
glm::vec3 SteeringBehaviors::flee(const glm::vec3& position, const glm::vec3& velocity,
                                   const glm::vec3& target) const {
    glm::vec3 toTarget = target - position;
    float distance = glm::length(toTarget);

    // Only flee if within flee distance
    if (distance > config.fleeDistance || distance < 0.0001f) {
        return glm::vec3(0.0f);
    }

    // Stronger flee force when closer
    float fleeStrength = 1.0f - (distance / config.fleeDistance);

    glm::vec3 desired = -(toTarget / distance) * config.maxSpeed * fleeStrength;
    glm::vec3 steering = desired - velocity;

    return limit(steering, config.maxForce * (1.0f + fleeStrength));
}

// ARRIVE: seek but slow down as approaching target
glm::vec3 SteeringBehaviors::arrive(const glm::vec3& position, const glm::vec3& velocity,
                                     const glm::vec3& target) const {
    glm::vec3 toTarget = target - position;
    float distance = glm::length(toTarget);

    if (distance < 0.0001f) {
        return glm::vec3(0.0f);
    }

    // Calculate desired speed based on distance
    float desiredSpeed;
    if (distance < config.slowingRadius) {
        // Slow down proportionally
        desiredSpeed = config.maxSpeed * (distance / config.slowingRadius);
    } else {
        desiredSpeed = config.maxSpeed;
    }

    glm::vec3 desired = (toTarget / distance) * desiredSpeed;
    glm::vec3 steering = desired - velocity;

    return limit(steering, config.maxForce);
}

// WANDER: random meandering - project a circle in front, pick random point on it
glm::vec3 SteeringBehaviors::wander(const glm::vec3& position, const glm::vec3& velocity,
                                     glm::vec3& wanderTarget) const {
    // Add random jitter to wander target
    wanderTarget.x += Random::range(-1.0f, 1.0f) * config.wanderJitter;
    wanderTarget.z += Random::range(-1.0f, 1.0f) * config.wanderJitter;
    wanderTarget.y = 0.0f;

    // Project wander target onto circle
    float mag = glm::length(wanderTarget);
    if (mag > 0.0001f) {
        wanderTarget = (wanderTarget / mag) * config.wanderRadius;
    } else {
        wanderTarget = glm::vec3(config.wanderRadius, 0.0f, 0.0f);
    }

    // Calculate wander force
    glm::vec3 forward = velocity;
    float velMag = glm::length(forward);
    if (velMag < 0.0001f) {
        forward = glm::vec3(1.0f, 0.0f, 0.0f);
    } else {
        forward = forward / velMag;
    }

    // Circle center is ahead of creature
    glm::vec3 circleCenter = position + forward * config.wanderDistance;

    // Target is on the wander circle
    glm::vec3 target = circleCenter + wanderTarget;

    return seek(position, velocity, target);
}

// PURSUIT: predict where target will be and intercept
glm::vec3 SteeringBehaviors::pursuit(const glm::vec3& position, const glm::vec3& velocity,
                                      const glm::vec3& targetPos, const glm::vec3& targetVel) const {
    glm::vec3 toTarget = targetPos - position;
    float distance = glm::length(toTarget);

    // Prediction time based on distance
    float predictionTime = config.pursuitPredictionTime;
    float mySpeed = glm::length(velocity);
    if (mySpeed > 0.0001f) {
        predictionTime = distance / mySpeed;
        predictionTime = std::min(predictionTime, config.pursuitPredictionTime * 2.0f);
    }

    // Predict future position
    glm::vec3 futurePos = targetPos + targetVel * predictionTime;

    return seek(position, velocity, futurePos);
}

// EVASION: predict pursuer's intercept point and flee from it
glm::vec3 SteeringBehaviors::evasion(const glm::vec3& position, const glm::vec3& velocity,
                                      const glm::vec3& pursuerPos, const glm::vec3& pursuerVel) const {
    glm::vec3 toPursuer = pursuerPos - position;
    float distance = glm::length(toPursuer);

    // Don't evade if too far
    if (distance > config.fleeDistance * 1.5f) {
        return glm::vec3(0.0f);
    }

    // Prediction time based on distance
    float predictionTime = config.pursuitPredictionTime;
    float pursuerSpeed = glm::length(pursuerVel);
    if (pursuerSpeed > 0.0001f) {
        predictionTime = distance / pursuerSpeed;
        predictionTime = std::min(predictionTime, config.pursuitPredictionTime * 2.0f);
    }

    // Predict future position of pursuer
    glm::vec3 futurePos = pursuerPos + pursuerVel * predictionTime;

    // Flee from predicted position with extra urgency when close
    float urgency = 1.0f + (1.0f - distance / config.fleeDistance);
    glm::vec3 fleeForce = flee(position, velocity, futurePos);

    return fleeForce * urgency;
}

// SEPARATION: steer away from nearby neighbors (avoid crowding)
glm::vec3 SteeringBehaviors::separate(const glm::vec3& position, const glm::vec3& velocity,
                                       const std::vector<Creature*>& neighbors) const {
    glm::vec3 steering(0.0f);
    int count = 0;

    for (const Creature* other : neighbors) {
        if (other == nullptr) continue;

        glm::vec3 toOther = position - other->getPosition();
        float distance = glm::length(toOther);

        if (distance > 0.001f && distance < config.separationDistance) {
            // Weight by inverse distance (closer = stronger repulsion)
            glm::vec3 repulsion = glm::normalize(toOther) / distance;
            steering += repulsion;
            count++;
        }
    }

    if (count > 0) {
        steering /= (float)count;
        steering = setMagnitude(steering, config.maxSpeed);
        steering = steering - velocity;
        steering = limit(steering, config.maxForce);
    }

    return steering;
}

// ALIGNMENT: match velocity with nearby neighbors
glm::vec3 SteeringBehaviors::align(const glm::vec3& position, const glm::vec3& velocity,
                                    const std::vector<Creature*>& neighbors) const {
    glm::vec3 avgVelocity(0.0f);
    int count = 0;

    for (const Creature* other : neighbors) {
        if (other == nullptr) continue;

        float distance = glm::length(position - other->getPosition());

        if (distance > 0.001f && distance < config.alignmentDistance) {
            avgVelocity += other->getVelocity();
            count++;
        }
    }

    if (count > 0) {
        avgVelocity /= (float)count;
        avgVelocity = setMagnitude(avgVelocity, config.maxSpeed);
        glm::vec3 steering = avgVelocity - velocity;
        return limit(steering, config.maxForce);
    }

    return glm::vec3(0.0f);
}

// COHESION: steer toward center of mass of neighbors
glm::vec3 SteeringBehaviors::cohesion(const glm::vec3& position, const glm::vec3& velocity,
                                       const std::vector<Creature*>& neighbors) const {
    glm::vec3 centerOfMass(0.0f);
    int count = 0;

    for (const Creature* other : neighbors) {
        if (other == nullptr) continue;

        float distance = glm::length(position - other->getPosition());

        if (distance > 0.001f && distance < config.cohesionDistance) {
            centerOfMass += other->getPosition();
            count++;
        }
    }

    if (count > 0) {
        centerOfMass /= (float)count;
        return seek(position, velocity, centerOfMass);
    }

    return glm::vec3(0.0f);
}

// FLOCK: combined boids behavior
glm::vec3 SteeringBehaviors::flock(const glm::vec3& position, const glm::vec3& velocity,
                                    const std::vector<Creature*>& neighbors,
                                    float separationWeight,
                                    float alignmentWeight,
                                    float cohesionWeight) const {
    glm::vec3 sep = separate(position, velocity, neighbors) * separationWeight;
    glm::vec3 ali = align(position, velocity, neighbors) * alignmentWeight;
    glm::vec3 coh = cohesion(position, velocity, neighbors) * cohesionWeight;

    return sep + ali + coh;
}

// BOUNDARY AVOIDANCE: steer away from world boundaries
glm::vec3 SteeringBehaviors::avoidBoundary(const glm::vec3& position, const glm::vec3& velocity,
                                            float boundaryWidth, float boundaryDepth) const {
    glm::vec3 steering(0.0f);
    float margin = 20.0f;  // Distance from edge to start turning

    float halfWidth = boundaryWidth * 0.5f;
    float halfDepth = boundaryDepth * 0.5f;

    // Check X boundaries
    if (position.x < -halfWidth + margin) {
        float urgency = 1.0f - (position.x + halfWidth) / margin;
        steering.x += config.maxForce * urgency;
    } else if (position.x > halfWidth - margin) {
        float urgency = 1.0f - (halfWidth - position.x) / margin;
        steering.x -= config.maxForce * urgency;
    }

    // Check Z boundaries
    if (position.z < -halfDepth + margin) {
        float urgency = 1.0f - (position.z + halfDepth) / margin;
        steering.z += config.maxForce * urgency;
    } else if (position.z > halfDepth - margin) {
        float urgency = 1.0f - (halfDepth - position.z) / margin;
        steering.z -= config.maxForce * urgency;
    }

    return steering;
}

// Apply force to velocity respecting maxSpeed
glm::vec3 SteeringBehaviors::applyForce(const glm::vec3& velocity, const glm::vec3& force,
                                         float deltaTime) const {
    glm::vec3 newVelocity = velocity + force * deltaTime;
    return limit(newVelocity, config.maxSpeed);
}
