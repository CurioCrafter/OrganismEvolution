#include "FlightBehavior.h"
#include "../environment/Terrain.h"
#include <algorithm>
#include <cmath>

// ============================================================================
// Main Update Loop
// ============================================================================

void FlightBehavior::update(float deltaTime, const Terrain& terrain) {
    // Use default atmospheric conditions
    AtmosphericConditions defaultAtmosphere;
    std::vector<ThermalColumn> emptyThermals;
    updateWithAtmosphere(deltaTime, terrain, defaultAtmosphere, emptyThermals);
}

void FlightBehavior::updateWithAtmosphere(float deltaTime, const Terrain& terrain,
                                           const AtmosphericConditions& atmosphere,
                                           const std::vector<ThermalColumn>& thermals) {
    m_stateTime += deltaTime;
    m_totalFlightTime += deltaTime;
    m_atmosphere = atmosphere;

    // Store thermals for use
    m_thermals.clear();
    for (const auto& t : thermals) {
        m_thermals.push_back(t);
    }

    // Get terrain info
    float terrainHeight = terrain.getHeight(m_position.x, m_position.z);
    m_altitude = m_position.y;
    m_groundClearance = m_position.y - terrainHeight;

    // Calculate air-relative velocity
    glm::vec3 wind = atmosphere.getWindAt(m_position, m_totalFlightTime);
    m_airVelocity = m_velocity - wind;
    m_airSpeed = glm::length(m_airVelocity);

    // Update based on current state
    switch (m_state) {
        case FlightState::GROUNDED:
            updateGrounded(deltaTime, terrain);
            break;
        case FlightState::TAKING_OFF:
            updateTakingOff(deltaTime, terrain);
            break;
        case FlightState::FLYING:
            updateFlying(deltaTime, terrain);
            break;
        case FlightState::GLIDING:
            updateGliding(deltaTime, terrain);
            break;
        case FlightState::DIVING:
            updateDiving(deltaTime, terrain);
            break;
        case FlightState::STOOPING:
            updateStooping(deltaTime, terrain);
            break;
        case FlightState::LANDING:
            updateLanding(deltaTime, terrain);
            break;
        case FlightState::HOVERING:
            updateHovering(deltaTime, terrain);
            break;
        case FlightState::THERMAL_SOARING:
            updateThermalSoaring(deltaTime, terrain);
            break;
        case FlightState::PERCHING:
            updatePerching(deltaTime, terrain);
            break;
        default:
            updateFlying(deltaTime, terrain);
            break;
    }

    // Universal updates for all flight states
    if (m_state != FlightState::GROUNDED && m_state != FlightState::PERCHING) {
        avoidTerrain(terrain);
        enforceFlightEnvelope(deltaTime, terrain);
        updateBankAndPitch(deltaTime);
    }

    // Update animation state
    updateAnimation(deltaTime);

    // Update energy
    updateEnergy(deltaTime);
}

// ============================================================================
// State-Specific Updates
// ============================================================================

void FlightBehavior::updateGrounded(float deltaTime, const Terrain& terrain) {
    float terrainHeight = terrain.getHeight(m_position.x, m_position.z);
    m_position.y = terrainHeight;
    m_velocity = glm::vec3(0.0f);
    m_bankAngle = 0.0f;
    m_pitchAngle = 0.0f;
    m_steeringForce = glm::vec3(0.0f);
    m_flapIntensity = 0.0f;
    m_wingFoldAmount = 1.0f;
}

void FlightBehavior::updateTakingOff(float deltaTime, const Terrain& terrain) {
    m_flapIntensity = 1.0f;
    m_wingFoldAmount = 0.0f;

    // Calculate forward direction
    glm::vec3 forward(std::cos(m_rotation), 0.0f, std::sin(m_rotation));

    // Accelerate forward and upward
    float accel = m_config.flapPower / m_config.mass;
    m_velocity += forward * accel * 0.7f * deltaTime;
    m_velocity.y += accel * 0.5f * deltaTime;

    // Apply drag during takeoff
    calculateAerodynamics(deltaTime);
    applyForces(deltaTime);

    // Update position
    m_position += m_velocity * deltaTime;

    // Check if airborne
    float terrainHeight = terrain.getHeight(m_position.x, m_position.z);
    if (m_position.y - terrainHeight > 3.0f && m_airSpeed > m_config.stallSpeed() * 1.2f) {
        transitionTo(FlightState::FLYING);
    }

    // Energy cost
    flightEnergy -= m_config.flapPower * 0.02f * deltaTime;
}

void FlightBehavior::updateFlying(float deltaTime, const Terrain& terrain) {
    m_steeringForce = glm::vec3(0.0f);
    m_wingFoldAmount = 0.0f;

    // Calculate aerodynamic forces
    calculateAerodynamics(deltaTime);

    // Apply thrust from flapping
    calculateThrust(deltaTime);

    // Add thermal lift
    glm::vec3 thermalForce = calculateThermalForce();
    m_velocity += thermalForce * deltaTime;

    // Check if in strong thermal - switch to soaring
    if (m_currentThermalStrength > 1.5f && glideFactor > 0.5f) {
        transitionTo(FlightState::THERMAL_SOARING);
    }

    // Altitude control
    maintainAltitude(deltaTime);

    // Target tracking
    if (m_hasTarget) {
        trackTarget(deltaTime);
    }

    // Apply all forces
    applyGravity(deltaTime);
    applyForces(deltaTime);

    // Update position
    m_position += m_velocity * deltaTime;

    // Energy cost from flapping
    flightEnergy -= m_flapIntensity * m_config.flapPower * 0.01f * deltaTime;
}

void FlightBehavior::updateGliding(float deltaTime, const Terrain& terrain) {
    m_steeringForce = glm::vec3(0.0f);
    m_flapIntensity = 0.1f;  // Occasional adjustment flaps
    m_wingFoldAmount = 0.0f;
    m_tailSpread = 0.3f;     // Tail spread for control

    // Calculate aerodynamics
    calculateAerodynamics(deltaTime);

    // Check for thermals
    glm::vec3 thermalForce = calculateThermalForce();
    if (m_currentThermalStrength > 1.0f) {
        transitionTo(FlightState::THERMAL_SOARING);
    }
    m_velocity += thermalForce * deltaTime;

    // Calculate glide descent
    float glideRatio = m_config.maxGlideRatio();
    float horizontalSpeed = glm::length(glm::vec2(m_airVelocity.x, m_airVelocity.z));
    float descentRate = horizontalSpeed / glideRatio;

    // Blend natural descent with current vertical velocity
    m_velocity.y = glm::mix(m_velocity.y, -descentRate + thermalForce.y, deltaTime * 2.0f);

    // Apply drag (reduced thrust)
    applyGravity(deltaTime);
    applyForces(deltaTime);

    // Maintain minimum speed
    if (m_airSpeed < minSpeed) {
        glm::vec3 forward(std::cos(m_rotation), 0.0f, std::sin(m_rotation));
        m_velocity += forward * 5.0f * deltaTime;
    }

    // Update position
    m_position += m_velocity * deltaTime;

    // Switch to powered flight if altitude too low
    if (m_groundClearance < preferredAltitude * 0.4f) {
        transitionTo(FlightState::FLYING);
    }

    // Minimal energy cost while gliding
    flightEnergy -= 0.5f * deltaTime;
}

void FlightBehavior::updateDiving(float deltaTime, const Terrain& terrain) {
    m_steeringForce = glm::vec3(0.0f);
    m_flapIntensity = 0.0f;
    m_wingFoldAmount = 0.5f;  // Partially folded
    m_tailSpread = 0.0f;

    // Steep dive - gravity-assisted descent
    m_velocity.y -= 15.0f * deltaTime;

    // Forward acceleration
    glm::vec3 forward(std::cos(m_rotation), 0.0f, std::sin(m_rotation));
    m_velocity += forward * 5.0f * deltaTime;

    // High-speed drag
    float speed = glm::length(m_velocity);
    float diveCD = m_config.zeroDragCoefficient * 1.5f;
    float rho = m_atmosphere.getDensityAtAltitude(m_altitude);
    float dragForce = 0.5f * rho * speed * speed * m_config.wingArea * diveCD;

    if (speed > 0.1f) {
        m_velocity -= glm::normalize(m_velocity) * (dragForce / m_config.mass) * deltaTime;
    }

    // Update position
    m_position += m_velocity * deltaTime;

    // Pull out of dive near ground
    if (m_groundClearance < minAltitude * 2.0f) {
        m_velocity.y = std::max(m_velocity.y, 5.0f);
        transitionTo(FlightState::FLYING);
    }

    // Minimal energy cost
    flightEnergy -= 0.2f * deltaTime;
}

void FlightBehavior::updateStooping(float deltaTime, const Terrain& terrain) {
    // High-speed hunting dive (falcon-style)
    m_flapIntensity = 0.0f;
    m_wingFoldAmount = 0.9f;  // Wings tucked tight
    m_tailSpread = 0.0f;

    // Very steep dive toward target
    if (m_hasTarget) {
        glm::vec3 toTarget = m_targetPosition - m_position;
        if (glm::length(toTarget) > 0.1f) {
            glm::vec3 diveDir = glm::normalize(toTarget);
            // Steeper angle
            diveDir.y = std::min(diveDir.y, -0.7f);
            diveDir = glm::normalize(diveDir);

            // Accelerate in dive direction
            m_velocity += diveDir * 30.0f * deltaTime;
        }
    } else {
        // No target - just dive
        m_velocity.y -= 25.0f * deltaTime;
    }

    // Very high speed possible
    float maxStoopSpeed = maxSpeed * 3.0f;  // Falcons can exceed 300 km/h
    float speed = glm::length(m_velocity);
    if (speed > maxStoopSpeed) {
        m_velocity = glm::normalize(m_velocity) * maxStoopSpeed;
    }

    // Update position
    m_position += m_velocity * deltaTime;

    // Pull out conditions
    if (m_groundClearance < minAltitude * 3.0f) {
        m_velocity.y = std::max(m_velocity.y, 10.0f);
        transitionTo(FlightState::FLYING);
    }

    // Check if reached target
    if (m_hasTarget && glm::length(m_targetPosition - m_position) < 2.0f) {
        transitionTo(FlightState::FLYING);
    }
}

void FlightBehavior::updateLanding(float deltaTime, const Terrain& terrain) {
    m_flapIntensity = 0.5f;  // Gentle flapping for control
    m_wingFoldAmount = 0.0f;
    m_tailSpread = 1.0f;     // Full tail spread for braking

    glm::vec3 toLanding = m_landingTarget.position - m_position;
    float distToLanding = glm::length(toLanding);

    // Approach phase
    if (distToLanding > 5.0f) {
        // Fly toward landing spot
        if (distToLanding > 0.1f) {
            glm::vec3 approachDir = glm::normalize(toLanding);

            // Calculate glide path angle
            float glideAngle = glm::radians(m_landingTarget.glideSlopeAngle);
            approachDir.y = -std::sin(glideAngle);

            m_velocity += approachDir * 5.0f * deltaTime;
        }
    }

    // Slow down
    float targetSpeed = std::max(minSpeed, distToLanding * 0.5f);
    float currentSpeed = glm::length(m_velocity);
    if (currentSpeed > targetSpeed) {
        m_velocity *= (1.0f - 2.0f * deltaTime);
    }

    // Descend
    float descentRate = std::min(3.0f, distToLanding * 0.3f);
    m_velocity.y = glm::mix(m_velocity.y, -descentRate, deltaTime * 3.0f);

    // Update position
    m_position += m_velocity * deltaTime;

    // Touch down
    float terrainHeight = terrain.getHeight(m_position.x, m_position.z);
    if (m_position.y <= terrainHeight + 0.5f) {
        m_position.y = terrainHeight;
        m_velocity = glm::vec3(0.0f);

        if (m_landingTarget.isPerch) {
            transitionTo(FlightState::PERCHING);
        } else {
            transitionTo(FlightState::GROUNDED);
        }
    }

    // Energy cost
    flightEnergy -= m_flapIntensity * m_config.flapPower * 0.005f * deltaTime;
}

void FlightBehavior::updateHovering(float deltaTime, const Terrain& terrain) {
    m_steeringForce = glm::vec3(0.0f);
    m_flapIntensity = 1.0f;
    m_wingFoldAmount = 0.0f;

    // Counter gravity with rapid wing beats
    float hoverThrust = m_config.mass * 9.8f * 1.1f;  // Slightly more than weight
    m_velocity.y += (hoverThrust / m_config.mass - 9.8f) * deltaTime;

    // Dampen horizontal drift
    m_velocity.x *= (1.0f - 3.0f * deltaTime);
    m_velocity.z *= (1.0f - 3.0f * deltaTime);

    // Altitude hold
    float altError = m_targetAltitude - m_altitude;
    m_velocity.y += altError * 3.0f * deltaTime;

    // Velocity damping
    m_velocity.y *= (1.0f - deltaTime);

    // Update position
    m_position += m_velocity * deltaTime;

    // High energy cost for hovering
    float hoverCost = m_config.flapPower * 0.05f;  // 5x normal flight
    flightEnergy -= hoverCost * deltaTime;

    // Can't hover forever - check energy
    if (flightEnergy < maxFlightEnergy * 0.2f) {
        transitionTo(FlightState::FLYING);
    }
}

void FlightBehavior::updateThermalSoaring(float deltaTime, const Terrain& terrain) {
    m_flapIntensity = 0.05f;  // Minimal flapping
    m_wingFoldAmount = 0.0f;
    m_tailSpread = 0.4f;

    // Find and circle thermal
    glm::vec3 thermalForce = calculateThermalForce();

    if (m_currentThermalStrength > 0.5f) {
        // In thermal - circle to stay in it
        circleThermal(deltaTime);
        m_velocity.y += m_currentThermalStrength * deltaTime;
        m_isInThermal = true;
    } else {
        // Lost thermal - search or transition
        findBestThermal();
        if (!m_isInThermal) {
            transitionTo(FlightState::GLIDING);
        }
    }

    // Apply basic aerodynamics
    calculateAerodynamics(deltaTime);
    applyGravity(deltaTime);
    applyForces(deltaTime);

    // Update position
    m_position += m_velocity * deltaTime;

    // Very low energy cost while soaring
    flightEnergy -= 0.2f * deltaTime;

    // Gained too much altitude - leave thermal
    if (m_altitude > maxAltitude * 0.9f) {
        transitionTo(FlightState::GLIDING);
    }
}

void FlightBehavior::updatePerching(float deltaTime, const Terrain& terrain) {
    // Stay at perch position
    m_velocity = glm::vec3(0.0f);
    m_flapIntensity = 0.0f;
    m_wingFoldAmount = 1.0f;
    m_tailSpread = 0.0f;

    // Regenerate energy while perching
    flightEnergy = std::min(maxFlightEnergy, flightEnergy + energyRegenRate * deltaTime);
}

// ============================================================================
// Physics Calculations
// ============================================================================

void FlightBehavior::calculateAerodynamics(float deltaTime) {
    float rho = m_atmosphere.getDensityAtAltitude(m_altitude);

    // Check for stall
    float stallSpeed = m_config.stallSpeed();
    m_physics.isStalling = m_airSpeed < stallSpeed * 0.9f;

    if (m_physics.isStalling) {
        m_physics.stallProgress = std::min(1.0f, m_physics.stallProgress + deltaTime * 2.0f);
    } else {
        m_physics.stallProgress = std::max(0.0f, m_physics.stallProgress - deltaTime * 3.0f);
    }

    // Calculate angle of attack (simplified)
    if (m_airSpeed > 0.1f) {
        glm::vec3 airDir = glm::normalize(m_airVelocity);
        glm::vec3 forward(std::cos(m_rotation), std::sin(m_pitchAngle), std::sin(m_rotation));
        m_physics.angleOfAttack = std::asin(glm::dot(glm::vec3(0, 1, 0), airDir) - forward.y);
    }

    // Calculate lift
    calculateLift();

    // Calculate drag
    calculateDrag();

    // Update load factor
    m_physics.weight = m_config.mass * 9.8f;
    if (m_physics.weight > 0.0f) {
        m_physics.loadFactor = m_physics.lift / m_physics.weight;
    }

    // Calculate specific energy
    m_physics.specificEnergy = m_altitude + 0.5f * m_airSpeed * m_airSpeed / 9.8f;
}

void FlightBehavior::calculateLift() {
    float rho = m_atmosphere.getDensityAtAltitude(m_altitude);

    float Cl;
    if (m_physics.isStalling) {
        // Reduced lift during stall
        Cl = m_config.liftCoefficient * (1.0f - m_physics.stallProgress * 0.7f);
    } else {
        // Normal lift coefficient
        Cl = m_config.liftCoefficient;

        // Adjust for angle of attack
        float aoaDeg = glm::degrees(m_physics.angleOfAttack);
        if (aoaDeg > 15.0f) {
            // Approaching stall
            float excess = (aoaDeg - 15.0f) / 5.0f;
            Cl *= (1.0f - excess * 0.5f);
        }
    }

    // Lift = 0.5 * rho * V^2 * S * Cl
    m_physics.liftCoefficient = Cl;
    m_physics.lift = 0.5f * rho * m_airSpeed * m_airSpeed * m_config.wingArea * Cl;

    // Reduce lift when banking (lift tilts with bank)
    m_physics.lift *= std::cos(m_bankAngle);
}

void FlightBehavior::calculateDrag() {
    float rho = m_atmosphere.getDensityAtAltitude(m_altitude);

    // Parasitic drag (form + skin friction)
    float Cd0 = m_config.zeroDragCoefficient;

    // Induced drag (from lift)
    float Cl = m_physics.liftCoefficient;
    float k = 1.0f / (3.14159f * m_config.aspectRatio * m_config.oswaldEfficiency);
    float Cd_induced = k * Cl * Cl;

    // Total drag coefficient
    float Cd_total = Cd0 + Cd_induced;

    // Extra drag during stall
    if (m_physics.isStalling) {
        Cd_total *= (1.0f + m_physics.stallProgress);
    }

    // Drag = 0.5 * rho * V^2 * S * Cd
    m_physics.dragCoefficient = Cd_total;
    m_physics.drag = 0.5f * rho * m_airSpeed * m_airSpeed * m_config.wingArea * Cd_total;
}

void FlightBehavior::calculateThrust(float deltaTime) {
    // Thrust from flapping
    float thrustPower = flapPower * m_config.flapPower * m_config.flapEfficiency;
    m_physics.thrust = thrustPower;

    // Apply thrust in forward direction
    glm::vec3 forward(std::cos(m_rotation), std::sin(m_pitchAngle * 0.5f), std::sin(m_rotation));
    forward = glm::normalize(forward);

    float thrustAccel = thrustPower / m_config.mass;
    m_velocity += forward * thrustAccel * deltaTime;

    // Set flap intensity based on power
    m_flapIntensity = flapPower;
}

void FlightBehavior::applyForces(float deltaTime) {
    if (m_airSpeed < 0.1f) return;

    // Lift acts perpendicular to velocity, mostly upward
    // In coordinated flight, lift counters weight and provides centripetal force
    float liftAccel = m_physics.lift / m_config.mass;

    // Vertical component of lift (reduced by bank angle)
    float verticalLift = liftAccel * std::cos(m_bankAngle);
    m_velocity.y += verticalLift * deltaTime;

    // Drag acts opposite to velocity
    glm::vec3 dragDir = -glm::normalize(m_airVelocity);
    float dragAccel = m_physics.drag / m_config.mass;
    m_velocity += dragDir * dragAccel * deltaTime;
}

void FlightBehavior::applyGravity(float deltaTime) {
    m_velocity.y -= 9.8f * deltaTime;
}

void FlightBehavior::updateEnergy(float deltaTime) {
    // Clamp energy
    flightEnergy = std::clamp(flightEnergy, 0.0f, maxFlightEnergy);

    // Regenerate energy when grounded or perching
    if (m_state == FlightState::GROUNDED || m_state == FlightState::PERCHING) {
        flightEnergy = std::min(maxFlightEnergy, flightEnergy + energyRegenRate * deltaTime);
    }
}

// ============================================================================
// Navigation and Control
// ============================================================================

void FlightBehavior::avoidTerrain(const Terrain& terrain) {
    if (m_groundClearance < minAltitude) {
        float urgency = 1.0f - m_groundClearance / minAltitude;
        urgency = std::clamp(urgency, 0.0f, 1.0f);

        m_steeringForce.y += 50.0f * urgency;
        m_velocity.y = std::max(m_velocity.y, 5.0f * urgency);

        // Force powered flight if too close
        if (m_groundClearance < minAltitude * 0.5f) {
            if (m_state == FlightState::GLIDING || m_state == FlightState::DIVING) {
                transitionTo(FlightState::FLYING);
            }
        }
    }
}

void FlightBehavior::maintainAltitude(float deltaTime) {
    float altError = m_targetAltitude - m_altitude;

    // Proportional-derivative control
    float altForce = altError * 3.0f - m_velocity.y * 1.0f;
    altForce = std::clamp(altForce, -15.0f, 15.0f);

    m_steeringForce.y += altForce;

    // Adjust pitch for altitude control
    if (altError > 5.0f) {
        flapPower = std::min(1.0f, flapPower + deltaTime);
    } else if (altError < -5.0f) {
        flapPower = std::max(0.3f, flapPower - deltaTime);
    }
}

void FlightBehavior::trackTarget(float deltaTime) {
    if (!m_hasTarget) return;

    glm::vec3 toTarget = m_targetPosition - m_position;
    float dist = glm::length(toTarget);

    if (dist < 1.0f) {
        m_hasTarget = false;
        return;
    }

    toTarget = glm::normalize(toTarget);

    // Calculate turn direction
    glm::vec3 forward(std::cos(m_rotation), 0.0f, std::sin(m_rotation));
    float cross = forward.x * toTarget.z - forward.z * toTarget.x;

    // Apply bank for turning
    float targetBank = std::clamp(cross * 2.0f, -1.0f, 1.0f) * glm::radians(m_config.maxBankAngle);
    m_bankAngle = glm::mix(m_bankAngle, targetBank, deltaTime * 3.0f);

    // Add steering force toward target
    m_steeringForce += toTarget * 10.0f;
    m_velocity += m_steeringForce * deltaTime;
}

void FlightBehavior::updateBankAndPitch(float deltaTime) {
    // Calculate target rotation from velocity
    glm::vec2 horizVel(m_velocity.x, m_velocity.z);
    float horizSpeed = glm::length(horizVel);

    if (horizSpeed > 0.5f) {
        float targetRotation = std::atan2(m_velocity.z, m_velocity.x);

        // Smooth rotation
        float rotDiff = targetRotation - m_rotation;
        while (rotDiff > 3.14159f) rotDiff -= 6.28318f;
        while (rotDiff < -3.14159f) rotDiff += 6.28318f;

        m_rotation += rotDiff * deltaTime * 2.0f;

        // Bank into turns
        float turnRate = rotDiff / deltaTime;
        float targetBank = std::clamp(turnRate * 0.3f, -glm::radians(m_config.maxBankAngle),
                                       glm::radians(m_config.maxBankAngle));
        m_bankAngle = glm::mix(m_bankAngle, targetBank, deltaTime * 4.0f);
    }

    // Calculate pitch from climb/descent rate
    float targetPitch = std::atan2(m_velocity.y, horizSpeed);
    targetPitch = std::clamp(targetPitch, -glm::radians(m_config.maxPitchAngle),
                             glm::radians(m_config.maxPitchAngle));
    m_pitchAngle = glm::mix(m_pitchAngle, targetPitch, deltaTime * 3.0f);

    // Update orientation quaternion
    glm::quat yawQuat = glm::angleAxis(m_rotation, glm::vec3(0, 1, 0));
    glm::quat pitchQuat = glm::angleAxis(m_pitchAngle, glm::vec3(0, 0, 1));
    glm::quat bankQuat = glm::angleAxis(m_bankAngle, glm::vec3(1, 0, 0));
    m_orientation = yawQuat * pitchQuat * bankQuat;
}

void FlightBehavior::enforceFlightEnvelope(float deltaTime, const Terrain& terrain) {
    // Speed limits
    if (m_airSpeed > maxSpeed) {
        m_velocity = glm::normalize(m_velocity) * maxSpeed;
    }

    // Altitude limits
    float terrainHeight = terrain.getHeight(m_position.x, m_position.z);

    if (m_position.y < terrainHeight + minAltitude) {
        m_position.y = terrainHeight + minAltitude;
        m_velocity.y = std::max(m_velocity.y, 2.0f);
    }

    if (m_position.y > terrainHeight + maxAltitude) {
        m_position.y = terrainHeight + maxAltitude;
        m_velocity.y = std::min(m_velocity.y, 0.0f);
    }

    // Load factor limits
    if (m_physics.loadFactor > m_config.maxLoadFactor) {
        // Reduce lift to avoid structural damage
        m_velocity.y -= (m_physics.loadFactor - m_config.maxLoadFactor) * 5.0f * deltaTime;
    }
}

// ============================================================================
// Thermal Handling
// ============================================================================

glm::vec3 FlightBehavior::calculateThermalForce() {
    glm::vec3 totalForce(0.0f);
    m_currentThermalStrength = 0.0f;
    m_isInThermal = false;

    for (const auto& thermal : m_thermals) {
        float strength = thermal.getStrengthAt(m_position);
        if (strength > 0.0f) {
            totalForce.y += strength;
            if (strength > m_currentThermalStrength) {
                m_currentThermalStrength = strength;
                m_thermalCenter = thermal.center;
                m_isInThermal = true;
            }
        }
    }

    return totalForce;
}

void FlightBehavior::findBestThermal() {
    float bestStrength = 0.0f;
    glm::vec3 bestCenter(0.0f);

    for (const auto& thermal : m_thermals) {
        if (!thermal.isActive) continue;

        float dist = glm::length(glm::vec2(thermal.center.x - m_position.x,
                                            thermal.center.z - m_position.z));
        if (dist < 200.0f) {  // Within detection range
            float potential = thermal.strength / (1.0f + dist * 0.01f);
            if (potential > bestStrength) {
                bestStrength = potential;
                bestCenter = thermal.center;
            }
        }
    }

    if (bestStrength > 0.0f) {
        m_thermalCenter = bestCenter;
        m_hasTarget = true;
        m_targetPosition = bestCenter;
        m_targetPosition.y = m_position.y + 10.0f;  // Aim above
    }
}

void FlightBehavior::circleThermal(float deltaTime) {
    // Calculate position relative to thermal center
    glm::vec2 toCenter(m_thermalCenter.x - m_position.x, m_thermalCenter.z - m_position.z);
    float distToCenter = glm::length(toCenter);

    // Target radius is slightly inside the core
    float targetRadius = 30.0f;  // Adjust based on thermal size

    // Calculate tangent direction for circling
    glm::vec2 tangent(-toCenter.y, toCenter.x);
    tangent = glm::normalize(tangent);

    // Add radial correction to stay at target radius
    glm::vec2 radialCorrection(0.0f);
    if (distToCenter > 0.1f) {
        float radiusError = distToCenter - targetRadius;
        radialCorrection = -glm::normalize(toCenter) * radiusError * 0.3f;
    }

    // Combine for steering
    glm::vec2 targetDir = tangent + radialCorrection * 0.5f;
    targetDir = glm::normalize(targetDir);

    // Apply steering
    float targetRotation = std::atan2(targetDir.y, targetDir.x);
    float rotDiff = targetRotation - m_rotation;
    while (rotDiff > 3.14159f) rotDiff -= 6.28318f;
    while (rotDiff < -3.14159f) rotDiff += 6.28318f;

    m_rotation += rotDiff * deltaTime * 2.0f;

    // Bank into the turn
    m_bankAngle = glm::radians(30.0f);  // Consistent bank for thermalling

    // Maintain speed for efficient circling
    glm::vec3 forward(std::cos(m_rotation), 0.0f, std::sin(m_rotation));
    float targetSpeed = m_config.optimalGlideSpeed();
    float speedError = targetSpeed - glm::length(glm::vec2(m_velocity.x, m_velocity.z));

    m_velocity.x += forward.x * speedError * deltaTime;
    m_velocity.z += forward.z * speedError * deltaTime;
}

// ============================================================================
// Animation Updates
// ============================================================================

void FlightBehavior::updateAnimation(float deltaTime) {
    // Update tail spread based on maneuvering
    float targetTailSpread = std::abs(m_bankAngle) / glm::radians(60.0f);
    targetTailSpread += std::abs(m_pitchAngle) / glm::radians(30.0f) * 0.5f;
    targetTailSpread = std::clamp(targetTailSpread, 0.0f, 1.0f);
    m_tailSpread = glm::mix(m_tailSpread, targetTailSpread, deltaTime * 5.0f);

    // Wing fold based on state
    float targetFold = 0.0f;
    switch (m_state) {
        case FlightState::GROUNDED:
        case FlightState::PERCHING:
            targetFold = 1.0f;
            break;
        case FlightState::DIVING:
            targetFold = 0.5f;
            break;
        case FlightState::STOOPING:
            targetFold = 0.9f;
            break;
        default:
            targetFold = 0.0f;
            break;
    }
    m_wingFoldAmount = glm::mix(m_wingFoldAmount, targetFold, deltaTime * 4.0f);

    // Flap intensity based on state and energy needs
    float targetFlap = 0.0f;
    switch (m_state) {
        case FlightState::TAKING_OFF:
            targetFlap = 1.0f;
            break;
        case FlightState::FLYING:
            targetFlap = flapPower;
            break;
        case FlightState::HOVERING:
            targetFlap = 1.0f;
            break;
        case FlightState::GLIDING:
        case FlightState::THERMAL_SOARING:
            targetFlap = 0.1f;
            break;
        case FlightState::LANDING:
            targetFlap = 0.5f;
            break;
        default:
            targetFlap = 0.0f;
            break;
    }
    m_flapIntensity = glm::mix(m_flapIntensity, targetFlap, deltaTime * 3.0f);
}
