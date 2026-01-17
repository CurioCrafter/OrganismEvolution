#pragma once

#include <glm/glm.hpp>
#include <vector>

class Creature;

// Craig Reynolds' Steering Behaviors Implementation
// Based on "Steering Behaviors for Autonomous Characters" and Nature of Code
class SteeringBehaviors {
public:
    // Configuration
    struct Config {
        float maxForce = 1.0f;          // Maximum steering force
        float maxSpeed = 10.0f;         // Maximum velocity magnitude
        float slowingRadius = 10.0f;    // For arrive behavior
        float wanderRadius = 2.0f;      // Wander circle radius
        float wanderDistance = 4.0f;    // Distance to wander circle
        float wanderJitter = 0.3f;      // Random displacement per frame
        float separationDistance = 5.0f; // Minimum distance between creatures
        float alignmentDistance = 15.0f; // Range for velocity alignment
        float cohesionDistance = 20.0f;  // Range for cohesion grouping
        float fleeDistance = 35.0f;      // Panic flee distance
        float pursuitPredictionTime = 0.5f; // Time ahead for pursuit prediction
    };

    SteeringBehaviors();
    explicit SteeringBehaviors(const Config& config);

    // Core steering behaviors (return steering force vectors)

    // Seek: Move directly toward target at max speed
    glm::vec3 seek(const glm::vec3& position, const glm::vec3& velocity,
                   const glm::vec3& target) const;

    // Flee: Move directly away from target at max speed
    glm::vec3 flee(const glm::vec3& position, const glm::vec3& velocity,
                   const glm::vec3& target) const;

    // Arrive: Seek but slow down as approaching target
    glm::vec3 arrive(const glm::vec3& position, const glm::vec3& velocity,
                     const glm::vec3& target) const;

    // Wander: Random meandering movement
    glm::vec3 wander(const glm::vec3& position, const glm::vec3& velocity,
                     glm::vec3& wanderTarget) const;

    // Pursuit: Predict target's future position and intercept
    glm::vec3 pursuit(const glm::vec3& position, const glm::vec3& velocity,
                      const glm::vec3& targetPos, const glm::vec3& targetVel) const;

    // Evasion: Predict pursuer's future position and flee from it
    glm::vec3 evasion(const glm::vec3& position, const glm::vec3& velocity,
                      const glm::vec3& pursuerPos, const glm::vec3& pursuerVel) const;

    // Flocking behaviors (Reynolds' Boids)

    // Separation: Steer away from crowded neighbors
    glm::vec3 separate(const glm::vec3& position, const glm::vec3& velocity,
                       const std::vector<Creature*>& neighbors) const;

    // Alignment: Match velocity with nearby neighbors
    glm::vec3 align(const glm::vec3& position, const glm::vec3& velocity,
                    const std::vector<Creature*>& neighbors) const;

    // Cohesion: Steer toward center of mass of neighbors
    glm::vec3 cohesion(const glm::vec3& position, const glm::vec3& velocity,
                       const std::vector<Creature*>& neighbors) const;

    // Combined flocking behavior
    glm::vec3 flock(const glm::vec3& position, const glm::vec3& velocity,
                    const std::vector<Creature*>& neighbors,
                    float separationWeight = 1.5f,
                    float alignmentWeight = 1.0f,
                    float cohesionWeight = 1.0f) const;

    // Obstacle/boundary avoidance
    glm::vec3 avoidBoundary(const glm::vec3& position, const glm::vec3& velocity,
                            float boundaryWidth, float boundaryDepth) const;

    // Utility functions
    void setConfig(const Config& config) { this->config = config; }
    const Config& getConfig() const { return config; }

    // Apply steering force to velocity (respecting maxSpeed)
    glm::vec3 applyForce(const glm::vec3& velocity, const glm::vec3& force, float deltaTime) const;

private:
    Config config;

    // Helper: Limit vector magnitude
    glm::vec3 limit(const glm::vec3& v, float maxMagnitude) const;

    // Helper: Set vector magnitude
    glm::vec3 setMagnitude(const glm::vec3& v, float magnitude) const;
};
