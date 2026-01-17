#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include "../animation/WingAnimator.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <functional>

// Forward declarations
class Creature;
class Terrain;

// ============================================================================
// Flight State Machine
// ============================================================================

enum class FlightState {
    GROUNDED,           // On terrain, can takeoff
    TAKING_OFF,         // Transitioning to flight
    FLYING,             // Normal powered flight
    GLIDING,            // Energy-saving soaring/gliding
    DIVING,             // Fast descent (for attacks or rapid descent)
    LANDING,            // Transitioning to ground
    HOVERING,           // Stationary flight (insects, hummingbirds)
    THERMAL_SOARING,    // Riding thermal updrafts
    DYNAMIC_SOARING,    // Using wind gradients (albatross-style)
    PERCHING,           // Resting on elevated surface (tree, cliff)
    STOOPING            // High-speed hunting dive (falcon-style)
};

// ============================================================================
// Wing Type for Physics Variations
// ============================================================================

enum class WingPhysicsType {
    BIRD_SOARING,       // High aspect ratio, low wing loading (eagles, vultures)
    BIRD_FAST,          // Pointed wings, high speed (falcons, swifts)
    BIRD_MANEUVERABLE,  // Rounded wings, agile (owls, sparrows)
    BIRD_HOVERING,      // Special muscles for hovering (hummingbirds)
    BAT_MEMBRANE,       // Flexible membrane, highly maneuverable
    INSECT_DIPTERA,     // Two-wing high frequency (flies)
    INSECT_ODONATA,     // Four independent wings (dragonflies)
    INSECT_LEPIDOPTERA, // Large low-frequency wings (butterflies)
    INSECT_HYMENOPTERA, // Coupled wings (bees)
    PTEROSAUR,          // Large membrane wings
    DRAGON              // Fantasy large membrane wings
};

// ============================================================================
// Wing Configuration
// ============================================================================

struct WingPhysicsConfig {
    // Basic wing parameters
    float wingSpan = 1.0f;              // Total wingspan (m)
    float wingArea = 0.5f;              // Wing surface area (m^2)
    float aspectRatio = 7.0f;           // wingspan^2 / area
    float mass = 1.0f;                  // Creature mass (kg)

    // Aerodynamic coefficients
    float liftCoefficient = 1.0f;       // Cl at optimal angle of attack
    float maxLiftCoefficient = 1.8f;    // Cl_max before stall
    float zeroDragCoefficient = 0.02f;  // Cd0 (parasitic drag)
    float oswaldEfficiency = 0.85f;     // Span efficiency factor

    // Wing shape parameters
    float camber = 0.08f;               // Wing curvature (affects lift)
    float taper = 0.4f;                 // Tip/root chord ratio
    float twist = 3.0f;                 // Washout angle (degrees)
    float dihedralAngle = 5.0f;         // V-angle for stability
    float sweepAngle = 0.0f;            // Sweep for high speed

    // Flight envelope
    float maxBankAngle = 60.0f;         // Maximum bank (degrees)
    float maxPitchAngle = 45.0f;        // Maximum pitch (degrees)
    float maxLoadFactor = 4.0f;         // Maximum G-force

    // Performance characteristics
    float flapPower = 50.0f;            // Thrust from flapping (N)
    float flapEfficiency = 0.7f;        // Power to thrust efficiency
    float supracoracoideus = 0.1f;      // Upstroke muscle (for hovering)

    // Type-specific behavior
    WingPhysicsType wingType = WingPhysicsType::BIRD_MANEUVERABLE;

    // Derived calculations
    float wingLoading() const { return mass * 9.8f / wingArea; }

    float stallSpeed() const {
        float rho = 1.225f;
        return std::sqrt(2.0f * mass * 9.8f / (rho * wingArea * maxLiftCoefficient));
    }

    float maxGlideRatio() const {
        float Cd_min = zeroDragCoefficient;
        float Cl_opt = std::sqrt(3.14159f * aspectRatio * oswaldEfficiency * Cd_min);
        return Cl_opt / (2.0f * Cd_min);
    }

    float optimalGlideSpeed() const {
        float rho = 1.225f;
        float Cl_opt = std::sqrt(3.14159f * aspectRatio * oswaldEfficiency * zeroDragCoefficient);
        return std::sqrt(2.0f * mass * 9.8f / (rho * wingArea * Cl_opt));
    }

    float minSinkRate() const {
        return optimalGlideSpeed() / maxGlideRatio();
    }

    // Factory methods for different creature types
    static WingPhysicsConfig forBird(float size, float wingspan);
    static WingPhysicsConfig forInsect(float size, float wingspan);
    static WingPhysicsConfig forRaptor(float size, float wingspan);
    static WingPhysicsConfig forHummingbird(float size, float wingspan);
    static WingPhysicsConfig forOwl(float size, float wingspan);
    static WingPhysicsConfig forSeabird(float size, float wingspan);
    static WingPhysicsConfig forBat(float size, float wingspan);
    static WingPhysicsConfig forDragonfly(float size, float wingspan);
    static WingPhysicsConfig forButterfly(float size, float wingspan);
    static WingPhysicsConfig forBee(float size, float wingspan);
    static WingPhysicsConfig forDragon(float size, float wingspan);
};

// ============================================================================
// Atmospheric Conditions
// ============================================================================

struct AtmosphericConditions {
    float airDensity = 1.225f;          // kg/m^3 (sea level standard)
    float temperature = 288.15f;        // Kelvin (15C)
    float pressure = 101325.0f;         // Pascal
    float humidity = 0.5f;              // 0-1

    glm::vec3 windVelocity{0.0f};       // Wind vector
    float gustiness = 0.0f;             // Wind variability
    float turbulence = 0.0f;            // Random perturbations

    // Calculate density at altitude (simplified)
    float getDensityAtAltitude(float altitude) const {
        // Barometric formula approximation
        float scaleHeight = 8500.0f;
        return airDensity * std::exp(-altitude / scaleHeight);
    }

    // Get wind at position with gusts and turbulence
    glm::vec3 getWindAt(const glm::vec3& pos, float time) const;
};

// ============================================================================
// Thermal Column
// ============================================================================

struct ThermalColumn {
    glm::vec3 center;
    float radius = 50.0f;
    float strength = 3.0f;              // Core vertical velocity (m/s)
    float maxAltitude = 1500.0f;        // Cloud base altitude
    float age = 0.0f;                   // Thermal lifetime
    float maxAge = 1800.0f;             // Maximum lifetime (30 min)
    bool isActive = true;

    // Core model with shell
    float coreRadius = 0.3f;            // Core fraction of total radius
    float shellSink = -0.5f;            // Sink rate in shell

    bool contains(const glm::vec3& pos) const;
    float getStrengthAt(const glm::vec3& pos) const;
    glm::vec3 getDriftAt(const glm::vec3& pos, const glm::vec3& wind) const;
};

// ============================================================================
// Landing Target
// ============================================================================

struct LandingTarget {
    glm::vec3 position;
    glm::vec3 normal;                   // Surface normal
    float radius = 1.0f;                // Acceptable landing area
    bool isPerch = false;               // Tree branch vs ground
    bool isWater = false;               // Water landing (waterfowl)

    // Approach parameters
    glm::vec3 approachDirection;        // Preferred approach vector
    float glideSlopeAngle = 10.0f;      // Degrees
};

// ============================================================================
// Flight Physics State
// ============================================================================

struct FlightPhysicsState {
    // Forces
    float lift = 0.0f;
    float drag = 0.0f;
    float thrust = 0.0f;
    float weight = 0.0f;

    // Coefficients
    float liftCoefficient = 0.0f;
    float dragCoefficient = 0.0f;
    float loadFactor = 1.0f;            // G-force

    // Angles
    float angleOfAttack = 0.0f;
    float sideslipAngle = 0.0f;

    // Stall state
    bool isStalling = false;
    float stallProgress = 0.0f;         // 0 = not stalling, 1 = full stall

    // Energy state
    float specificEnergy = 0.0f;        // Altitude + kinetic energy
    float energyRate = 0.0f;            // Rate of energy change
};

// ============================================================================
// Flight Behavior Controller
// ============================================================================

class FlightBehavior {
public:
    FlightBehavior() = default;

    // Initialize with configuration
    void initialize(const WingPhysicsConfig& config);

    // State setters
    void setPosition(const glm::vec3& pos) { m_position = pos; }
    void setVelocity(const glm::vec3& vel) { m_velocity = vel; }
    void setOrientation(const glm::quat& orient) { m_orientation = orient; }
    void setRotation(float rot) { m_rotation = rot; }
    void setTargetAltitude(float alt) { m_targetAltitude = alt; }
    void setTargetPosition(const glm::vec3& target) { m_targetPosition = target; m_hasTarget = true; }
    void setTargetVelocity(const glm::vec3& vel) { m_targetVelocity = vel; }
    void clearTarget() { m_hasTarget = false; }

    // Flight commands
    void commandTakeoff();
    void commandLand(const LandingTarget& target);
    void commandDive(const glm::vec3& target);
    void commandStoop(const glm::vec3& target);  // High-speed hunting dive
    void commandGlide();
    void commandFlap();
    void commandHover();
    void commandSoar();                          // Find and ride thermals
    void commandPerch(const glm::vec3& perchPoint);

    // Update physics
    void update(float deltaTime, const Terrain& terrain);
    void updateWithAtmosphere(float deltaTime, const Terrain& terrain,
                               const AtmosphericConditions& atmosphere,
                               const std::vector<ThermalColumn>& thermals);

    // State getters
    FlightState getState() const { return m_state; }
    const glm::vec3& getPosition() const { return m_position; }
    const glm::vec3& getVelocity() const { return m_velocity; }
    const glm::quat& getOrientation() const { return m_orientation; }
    float getRotation() const { return m_rotation; }
    float getBankAngle() const { return m_bankAngle; }
    float getPitchAngle() const { return m_pitchAngle; }
    float getAltitude() const { return m_altitude; }
    float getGroundClearance() const { return m_groundClearance; }
    float getAirSpeed() const { return m_airSpeed; }
    float getGroundSpeed() const { return glm::length(glm::vec2(m_velocity.x, m_velocity.z)); }
    float getVerticalSpeed() const { return m_velocity.y; }

    // Physics state access
    const FlightPhysicsState& getPhysicsState() const { return m_physics; }
    float getLift() const { return m_physics.lift; }
    float getDrag() const { return m_physics.drag; }
    float getThrust() const { return m_physics.thrust; }
    bool isStalling() const { return m_physics.isStalling; }
    float getLoadFactor() const { return m_physics.loadFactor; }

    // Animation interface
    animation::FlightAnimState getAnimState() const;
    float getFlapIntensity() const { return m_flapIntensity; }
    float getWingFold() const { return m_wingFoldAmount; }
    float getTailSpread() const { return m_tailSpread; }

    // Steering output (for use by Creature)
    glm::vec3 getSteeringForce() const { return m_steeringForce; }

    // Thermal interaction
    void addThermal(const ThermalColumn& thermal);
    void clearThermals() { m_thermals.clear(); }
    bool isInThermal() const { return m_isInThermal; }
    float getThermalStrength() const { return m_currentThermalStrength; }

    // Configuration access
    WingPhysicsConfig& getConfig() { return m_config; }
    const WingPhysicsConfig& getConfig() const { return m_config; }

    // Flight parameters (adjustable)
    float flapPower = 1.0f;             // 0-1 flapping intensity
    float glideFactor = 0.5f;           // Preference for gliding vs flapping
    float preferredAltitude = 25.0f;    // Target cruise altitude
    float minAltitude = 5.0f;           // Minimum safe altitude
    float maxAltitude = 100.0f;         // Maximum altitude
    float maxSpeed = 20.0f;             // Speed limit
    float minSpeed = 3.0f;              // Stall speed

    // Energy management
    float flightEnergy = 100.0f;        // Current energy
    float maxFlightEnergy = 100.0f;     // Maximum energy
    float energyRegenRate = 5.0f;       // Energy recovery when resting

private:
    WingPhysicsConfig m_config;

    // Current state
    FlightState m_state = FlightState::GROUNDED;
    glm::vec3 m_position{0.0f};
    glm::vec3 m_velocity{0.0f};
    glm::quat m_orientation{1.0f, 0.0f, 0.0f, 0.0f};
    float m_rotation = 0.0f;
    float m_bankAngle = 0.0f;
    float m_pitchAngle = 0.0f;
    float m_altitude = 0.0f;
    float m_groundClearance = 0.0f;
    float m_airSpeed = 0.0f;

    // Air-relative velocity
    glm::vec3 m_airVelocity{0.0f};

    // Physics state
    FlightPhysicsState m_physics;

    // Target tracking
    glm::vec3 m_targetPosition{0.0f};
    glm::vec3 m_targetVelocity{0.0f};
    float m_targetAltitude = 25.0f;
    bool m_hasTarget = false;

    // Landing
    LandingTarget m_landingTarget;
    bool m_isLanding = false;

    // Steering output
    glm::vec3 m_steeringForce{0.0f};

    // Animation state
    float m_flapIntensity = 0.0f;
    float m_wingFoldAmount = 0.0f;
    float m_tailSpread = 0.0f;

    // Thermals
    std::vector<ThermalColumn> m_thermals;
    bool m_isInThermal = false;
    float m_currentThermalStrength = 0.0f;
    glm::vec3 m_thermalCenter{0.0f};

    // Atmospheric state
    AtmosphericConditions m_atmosphere;

    // State timers
    float m_stateTime = 0.0f;
    float m_totalFlightTime = 0.0f;

    // Internal state update methods
    void updateGrounded(float deltaTime, const Terrain& terrain);
    void updateTakingOff(float deltaTime, const Terrain& terrain);
    void updateFlying(float deltaTime, const Terrain& terrain);
    void updateGliding(float deltaTime, const Terrain& terrain);
    void updateDiving(float deltaTime, const Terrain& terrain);
    void updateStooping(float deltaTime, const Terrain& terrain);
    void updateLanding(float deltaTime, const Terrain& terrain);
    void updateHovering(float deltaTime, const Terrain& terrain);
    void updateThermalSoaring(float deltaTime, const Terrain& terrain);
    void updatePerching(float deltaTime, const Terrain& terrain);

    // Physics calculations
    void calculateAerodynamics(float deltaTime);
    void calculateLift();
    void calculateDrag();
    void calculateThrust(float deltaTime);
    void applyForces(float deltaTime);
    void applyGravity(float deltaTime);
    void updateEnergy(float deltaTime);

    // Navigation and control
    void avoidTerrain(const Terrain& terrain);
    void maintainAltitude(float deltaTime);
    void trackTarget(float deltaTime);
    void updateBankAndPitch(float deltaTime);
    void enforceFlightEnvelope(float deltaTime, const Terrain& terrain);

    // Thermal handling
    glm::vec3 calculateThermalForce();
    void findBestThermal();
    void circleThermal(float deltaTime);

    // Animation updates
    void updateAnimation(float deltaTime);

    // State transitions
    void transitionTo(FlightState newState);
    bool canTransitionTo(FlightState newState) const;
};

// ============================================================================
// Inline Implementations - Factory Methods
// ============================================================================

inline WingPhysicsConfig WingPhysicsConfig::forBird(float size, float wingspan) {
    WingPhysicsConfig config;
    config.wingSpan = wingspan;
    config.wingArea = wingspan * wingspan / 7.0f;
    config.aspectRatio = 7.0f;
    config.mass = size * 0.5f;
    config.liftCoefficient = 1.2f;
    config.maxLiftCoefficient = 1.8f;
    config.zeroDragCoefficient = 0.025f;
    config.oswaldEfficiency = 0.85f;
    config.flapPower = size * 30.0f;
    config.wingType = WingPhysicsType::BIRD_MANEUVERABLE;
    return config;
}

inline WingPhysicsConfig WingPhysicsConfig::forInsect(float size, float wingspan) {
    WingPhysicsConfig config;
    config.wingSpan = wingspan;
    config.wingArea = wingspan * wingspan / 4.0f;
    config.aspectRatio = 4.0f;
    config.mass = size * 0.001f;  // Insects are very light
    config.liftCoefficient = 1.5f;
    config.maxLiftCoefficient = 3.0f;  // Unsteady aerodynamics
    config.zeroDragCoefficient = 0.1f;
    config.oswaldEfficiency = 0.7f;
    config.flapPower = size * 5.0f;
    config.supracoracoideus = 0.15f;
    config.wingType = WingPhysicsType::INSECT_HYMENOPTERA;
    return config;
}

inline WingPhysicsConfig WingPhysicsConfig::forRaptor(float size, float wingspan) {
    WingPhysicsConfig config;
    config.wingSpan = wingspan;
    config.wingArea = wingspan * wingspan / 8.0f;
    config.aspectRatio = 8.0f;
    config.mass = size * 0.8f;
    config.liftCoefficient = 1.3f;
    config.maxLiftCoefficient = 2.0f;
    config.zeroDragCoefficient = 0.02f;
    config.oswaldEfficiency = 0.9f;
    config.flapPower = size * 50.0f;
    config.maxLoadFactor = 6.0f;  // Raptors can pull high G
    config.wingType = WingPhysicsType::BIRD_SOARING;
    return config;
}

inline WingPhysicsConfig WingPhysicsConfig::forHummingbird(float size, float wingspan) {
    WingPhysicsConfig config;
    config.wingSpan = wingspan;
    config.wingArea = wingspan * wingspan / 5.0f;
    config.aspectRatio = 5.0f;
    config.mass = size * 0.003f;  // Very light (3-5 grams)
    config.liftCoefficient = 1.4f;
    config.maxLiftCoefficient = 2.5f;
    config.zeroDragCoefficient = 0.04f;
    config.oswaldEfficiency = 0.8f;
    config.flapPower = size * 2.0f;
    config.supracoracoideus = 0.18f;  // Strong upstroke for hovering
    config.wingType = WingPhysicsType::BIRD_HOVERING;
    return config;
}

inline WingPhysicsConfig WingPhysicsConfig::forOwl(float size, float wingspan) {
    WingPhysicsConfig config;
    config.wingSpan = wingspan;
    config.wingArea = wingspan * wingspan / 5.5f;
    config.aspectRatio = 5.5f;
    config.mass = size * 0.6f;
    config.liftCoefficient = 1.4f;
    config.maxLiftCoefficient = 2.2f;
    config.zeroDragCoefficient = 0.03f;  // Quiet flight = more drag
    config.oswaldEfficiency = 0.82f;
    config.flapPower = size * 35.0f;
    config.wingType = WingPhysicsType::BIRD_MANEUVERABLE;
    return config;
}

inline WingPhysicsConfig WingPhysicsConfig::forSeabird(float size, float wingspan) {
    WingPhysicsConfig config;
    config.wingSpan = wingspan;
    config.wingArea = wingspan * wingspan / 12.0f;
    config.aspectRatio = 12.0f;  // Very high aspect ratio
    config.mass = size * 0.7f;
    config.liftCoefficient = 1.1f;
    config.maxLiftCoefficient = 1.6f;
    config.zeroDragCoefficient = 0.015f;  // Very efficient
    config.oswaldEfficiency = 0.92f;
    config.flapPower = size * 25.0f;
    config.wingType = WingPhysicsType::BIRD_SOARING;
    return config;
}

inline WingPhysicsConfig WingPhysicsConfig::forBat(float size, float wingspan) {
    WingPhysicsConfig config;
    config.wingSpan = wingspan;
    config.wingArea = wingspan * wingspan / 5.0f;
    config.aspectRatio = 5.0f;
    config.mass = size * 0.015f;
    config.liftCoefficient = 1.3f;
    config.maxLiftCoefficient = 2.2f;
    config.zeroDragCoefficient = 0.035f;
    config.oswaldEfficiency = 0.78f;
    config.flapPower = size * 8.0f;
    config.supracoracoideus = 0.12f;
    config.wingType = WingPhysicsType::BAT_MEMBRANE;
    return config;
}

inline WingPhysicsConfig WingPhysicsConfig::forDragonfly(float size, float wingspan) {
    WingPhysicsConfig config;
    config.wingSpan = wingspan;
    config.wingArea = wingspan * wingspan / 6.0f;
    config.aspectRatio = 6.0f;
    config.mass = size * 0.001f;
    config.liftCoefficient = 1.6f;
    config.maxLiftCoefficient = 3.5f;
    config.zeroDragCoefficient = 0.08f;
    config.oswaldEfficiency = 0.75f;
    config.flapPower = size * 3.0f;
    config.supracoracoideus = 0.15f;
    config.maxLoadFactor = 8.0f;  // Very agile
    config.wingType = WingPhysicsType::INSECT_ODONATA;
    return config;
}

inline WingPhysicsConfig WingPhysicsConfig::forButterfly(float size, float wingspan) {
    WingPhysicsConfig config;
    config.wingSpan = wingspan;
    config.wingArea = wingspan * wingspan / 2.5f;  // Very large wing area
    config.aspectRatio = 2.5f;
    config.mass = size * 0.0005f;
    config.liftCoefficient = 1.8f;
    config.maxLiftCoefficient = 2.5f;
    config.zeroDragCoefficient = 0.15f;
    config.oswaldEfficiency = 0.65f;
    config.flapPower = size * 1.0f;
    config.wingType = WingPhysicsType::INSECT_LEPIDOPTERA;
    return config;
}

inline WingPhysicsConfig WingPhysicsConfig::forBee(float size, float wingspan) {
    WingPhysicsConfig config;
    config.wingSpan = wingspan;
    config.wingArea = wingspan * wingspan / 3.5f;
    config.aspectRatio = 3.5f;
    config.mass = size * 0.0001f;
    config.liftCoefficient = 1.7f;
    config.maxLiftCoefficient = 3.2f;
    config.zeroDragCoefficient = 0.12f;
    config.oswaldEfficiency = 0.7f;
    config.flapPower = size * 2.5f;
    config.supracoracoideus = 0.14f;
    config.wingType = WingPhysicsType::INSECT_HYMENOPTERA;
    return config;
}

inline WingPhysicsConfig WingPhysicsConfig::forDragon(float size, float wingspan) {
    WingPhysicsConfig config;
    config.wingSpan = wingspan;
    config.wingArea = wingspan * wingspan / 5.0f;
    config.aspectRatio = 5.0f;
    config.mass = size * 500.0f;  // Heavy fantasy creature
    config.liftCoefficient = 1.5f;
    config.maxLiftCoefficient = 2.5f;
    config.zeroDragCoefficient = 0.04f;
    config.oswaldEfficiency = 0.8f;
    config.flapPower = size * 5000.0f;  // Powerful
    config.maxLoadFactor = 5.0f;
    config.wingType = WingPhysicsType::DRAGON;
    return config;
}

// ============================================================================
// Inline Implementations - Atmospheric Conditions
// ============================================================================

inline glm::vec3 AtmosphericConditions::getWindAt(const glm::vec3& pos, float time) const {
    glm::vec3 wind = windVelocity;

    // Add height variation (wind increases with altitude)
    float altitudeFactor = 1.0f + pos.y * 0.005f;
    wind *= altitudeFactor;

    // Add gusts
    if (gustiness > 0.0f) {
        float gustPhase = time * 0.3f + pos.x * 0.01f + pos.z * 0.01f;
        float gust = std::sin(gustPhase) * std::sin(gustPhase * 2.3f) * gustiness;
        wind *= (1.0f + gust);
    }

    // Add turbulence
    if (turbulence > 0.0f) {
        float tx = std::sin(time * 2.1f + pos.y * 0.15f) * turbulence;
        float ty = std::cos(time * 1.7f + pos.x * 0.12f) * turbulence * 0.5f;
        float tz = std::sin(time * 1.9f + pos.z * 0.13f) * turbulence;
        wind += glm::vec3(tx, ty, tz);
    }

    return wind;
}

// ============================================================================
// Inline Implementations - Thermal Column
// ============================================================================

inline bool ThermalColumn::contains(const glm::vec3& pos) const {
    if (!isActive) return false;
    if (pos.y > maxAltitude) return false;

    glm::vec2 horizontal(pos.x - center.x, pos.z - center.z);
    return glm::length(horizontal) < radius;
}

inline float ThermalColumn::getStrengthAt(const glm::vec3& pos) const {
    if (!isActive) return 0.0f;
    if (pos.y > maxAltitude) return 0.0f;

    glm::vec2 horizontal(pos.x - center.x, pos.z - center.z);
    float dist = glm::length(horizontal);

    if (dist > radius) return 0.0f;

    float normalizedDist = dist / radius;

    // Core-shell model
    if (normalizedDist < coreRadius) {
        // In core - full strength with slight variation
        float coreStrength = strength * (1.0f - normalizedDist / coreRadius * 0.1f);
        return coreStrength;
    } else {
        // In shell - transitions from positive to negative
        float shellProgress = (normalizedDist - coreRadius) / (1.0f - coreRadius);
        float shellStrength = glm::mix(strength * 0.5f, shellSink, shellProgress * shellProgress);
        return shellStrength;
    }
}

inline glm::vec3 ThermalColumn::getDriftAt(const glm::vec3& pos, const glm::vec3& wind) const {
    // Thermals drift with the wind
    float driftFactor = 0.8f;  // Thermal moves slower than wind
    return wind * driftFactor;
}

// ============================================================================
// Inline Implementations - FlightBehavior Core
// ============================================================================

inline void FlightBehavior::initialize(const WingPhysicsConfig& config) {
    m_config = config;
    m_state = FlightState::GROUNDED;
    maxFlightEnergy = config.mass * 200.0f;
    flightEnergy = maxFlightEnergy;
    minSpeed = config.stallSpeed() * 1.1f;
    maxSpeed = minSpeed * 4.0f;
}

inline animation::FlightAnimState FlightBehavior::getAnimState() const {
    switch (m_state) {
        case FlightState::GROUNDED:
        case FlightState::PERCHING:
            return animation::FlightAnimState::GROUNDED;
        case FlightState::TAKING_OFF:
            return animation::FlightAnimState::TAKING_OFF;
        case FlightState::FLYING:
            return animation::FlightAnimState::FLAPPING;
        case FlightState::GLIDING:
        case FlightState::THERMAL_SOARING:
        case FlightState::DYNAMIC_SOARING:
            return animation::FlightAnimState::GLIDING;
        case FlightState::DIVING:
        case FlightState::STOOPING:
            return animation::FlightAnimState::DIVING;
        case FlightState::LANDING:
            return animation::FlightAnimState::LANDING;
        case FlightState::HOVERING:
            return animation::FlightAnimState::HOVERING;
        default:
            return animation::FlightAnimState::FLAPPING;
    }
}

inline void FlightBehavior::addThermal(const ThermalColumn& thermal) {
    m_thermals.push_back(thermal);
}

inline void FlightBehavior::commandTakeoff() {
    if (m_state == FlightState::GROUNDED || m_state == FlightState::PERCHING) {
        transitionTo(FlightState::TAKING_OFF);
    }
}

inline void FlightBehavior::commandLand(const LandingTarget& target) {
    m_landingTarget = target;
    m_isLanding = true;
    transitionTo(FlightState::LANDING);
}

inline void FlightBehavior::commandDive(const glm::vec3& target) {
    m_targetPosition = target;
    m_hasTarget = true;
    transitionTo(FlightState::DIVING);
}

inline void FlightBehavior::commandStoop(const glm::vec3& target) {
    m_targetPosition = target;
    m_hasTarget = true;
    transitionTo(FlightState::STOOPING);
}

inline void FlightBehavior::commandGlide() {
    if (m_state == FlightState::FLYING || m_state == FlightState::THERMAL_SOARING) {
        transitionTo(FlightState::GLIDING);
    }
}

inline void FlightBehavior::commandFlap() {
    if (m_state == FlightState::GLIDING || m_state == FlightState::THERMAL_SOARING) {
        transitionTo(FlightState::FLYING);
    }
}

inline void FlightBehavior::commandHover() {
    if (m_config.supracoracoideus >= 0.12f) {  // Need strong upstroke muscles
        transitionTo(FlightState::HOVERING);
    }
}

inline void FlightBehavior::commandSoar() {
    transitionTo(FlightState::THERMAL_SOARING);
}

inline void FlightBehavior::commandPerch(const glm::vec3& perchPoint) {
    m_landingTarget.position = perchPoint;
    m_landingTarget.isPerch = true;
    transitionTo(FlightState::LANDING);
}

inline void FlightBehavior::transitionTo(FlightState newState) {
    if (canTransitionTo(newState)) {
        m_state = newState;
        m_stateTime = 0.0f;
    }
}

inline bool FlightBehavior::canTransitionTo(FlightState newState) const {
    // Define valid state transitions
    switch (m_state) {
        case FlightState::GROUNDED:
            return newState == FlightState::TAKING_OFF;
        case FlightState::TAKING_OFF:
            return newState == FlightState::FLYING ||
                   newState == FlightState::HOVERING;
        case FlightState::FLYING:
            return newState == FlightState::GLIDING ||
                   newState == FlightState::DIVING ||
                   newState == FlightState::STOOPING ||
                   newState == FlightState::LANDING ||
                   newState == FlightState::HOVERING ||
                   newState == FlightState::THERMAL_SOARING;
        case FlightState::GLIDING:
            return newState == FlightState::FLYING ||
                   newState == FlightState::DIVING ||
                   newState == FlightState::LANDING ||
                   newState == FlightState::THERMAL_SOARING;
        case FlightState::THERMAL_SOARING:
            return newState == FlightState::FLYING ||
                   newState == FlightState::GLIDING ||
                   newState == FlightState::LANDING;
        case FlightState::DIVING:
        case FlightState::STOOPING:
            return newState == FlightState::FLYING ||
                   newState == FlightState::GLIDING ||
                   newState == FlightState::LANDING;
        case FlightState::LANDING:
            return newState == FlightState::GROUNDED ||
                   newState == FlightState::PERCHING ||
                   newState == FlightState::FLYING;  // Abort landing
        case FlightState::HOVERING:
            return newState == FlightState::FLYING ||
                   newState == FlightState::LANDING;
        case FlightState::PERCHING:
            return newState == FlightState::TAKING_OFF;
        default:
            return false;
    }
}
