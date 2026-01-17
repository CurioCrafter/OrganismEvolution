#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <functional>

// Forward declarations
class Terrain;
class Creature;
class SpatialGrid;

// =============================================================================
// DEPTH BANDS - Explicit ocean depth zones for aquatic spawning and behavior
// =============================================================================

enum class DepthBand {
    SURFACE,      // 0-2m   - Air-breathing creatures, surface feeders
    SHALLOW,      // 2-5m   - Small fish, reef dwellers, kelp forests
    MID_WATER,    // 5-25m  - Schooling fish, most common zone
    DEEP,         // 25-50m - Larger predators, pressure-adapted species
    ABYSS,        // 50m+   - Deep sea creatures, bioluminescent species
    COUNT
};

// Get depth band for a given depth value (positive = below water surface)
inline DepthBand getDepthBand(float depth) {
    if (depth < 2.0f)  return DepthBand::SURFACE;
    if (depth < 5.0f)  return DepthBand::SHALLOW;
    if (depth < 25.0f) return DepthBand::MID_WATER;
    if (depth < 50.0f) return DepthBand::DEEP;
    return DepthBand::ABYSS;
}

// Get depth band name for debugging/UI
inline const char* getDepthBandName(DepthBand band) {
    switch (band) {
        case DepthBand::SURFACE:   return "Surface (0-2m)";
        case DepthBand::SHALLOW:   return "Shallow (2-5m)";
        case DepthBand::MID_WATER: return "Mid-Water (5-25m)";
        case DepthBand::DEEP:      return "Deep (25-50m)";
        case DepthBand::ABYSS:     return "Abyss (50m+)";
        default: return "Unknown";
    }
}

// Get min/max depth range for a depth band
inline void getDepthBandRange(DepthBand band, float& minDepth, float& maxDepth) {
    switch (band) {
        case DepthBand::SURFACE:   minDepth = 0.5f;  maxDepth = 2.0f;  break;
        case DepthBand::SHALLOW:   minDepth = 2.0f;  maxDepth = 5.0f;  break;
        case DepthBand::MID_WATER: minDepth = 5.0f;  maxDepth = 25.0f; break;
        case DepthBand::DEEP:      minDepth = 25.0f; maxDepth = 50.0f; break;
        case DepthBand::ABYSS:     minDepth = 50.0f; maxDepth = 100.0f; break;
        default:                   minDepth = 5.0f;  maxDepth = 25.0f; break;
    }
}

// Get random depth within a depth band
inline float getRandomDepthInBand(DepthBand band, float randomValue01) {
    float minDepth, maxDepth;
    getDepthBandRange(band, minDepth, maxDepth);
    return minDepth + randomValue01 * (maxDepth - minDepth);
}

// Swimming behavior modes
enum class SwimMode {
    CRUISING,      // Normal swimming
    SCHOOLING,     // Following school
    FLEEING,       // Escaping predator
    HUNTING,       // Chasing prey
    SURFACING,     // Coming up for air (amphibians)
    DIVING,        // Going deeper
    FORAGING,      // Searching for food
    BREACHING,     // Jumping out of water
    RESTING,       // Stationary/minimal movement
    MIGRATING      // Long-distance travel
};

// Water current types
enum class CurrentType {
    NONE,
    LAMINAR,       // Smooth flow
    TURBULENT,     // Chaotic flow
    TIDAL,         // Periodic reversal
    UPWELLING,     // Vertical current
    GYRE           // Circular current
};

// Configuration for swimming physics
struct SwimPhysicsConfig {
    // Buoyancy parameters
    float neutralBuoyancyDepth = 0.3f;  // Depth at which fish is neutrally buoyant
    float buoyancyStrength = 5.0f;      // Force applied to maintain depth
    float buoyancyDamping = 0.8f;       // Damping on vertical motion
    float swimbladderEfficiency = 0.8f; // How well fish controls buoyancy (0=shark, 1=bony fish)

    // Drag coefficients
    float forwardDrag = 0.3f;           // Drag when moving forward (streamlined)
    float lateralDrag = 1.5f;           // Drag when moving sideways
    float verticalDrag = 1.0f;          // Drag when moving up/down
    float streamlining = 0.8f;          // Body streamlining factor (0-1)

    // Propulsion
    float thrustPower = 15.0f;          // Base thrust from tail
    float burstMultiplier = 2.0f;       // Multiplier for burst swimming
    float turnRate = 3.0f;              // How fast fish can turn (rad/s)
    float acceleration = 8.0f;          // Max acceleration

    // Energy costs
    float cruiseEnergyCost = 0.5f;      // Energy per second while cruising
    float burstEnergyCost = 2.0f;       // Energy per second during burst
    float idleEnergyCost = 0.2f;        // Energy per second while stationary
    float depthChangeCost = 0.3f;       // Extra cost for vertical movement

    // Depth/Pressure parameters
    float minDepth = 0.5f;              // Minimum operating depth
    float maxDepth = 100.0f;            // Maximum operating depth (pressure limit)
    float preferredDepth = 10.0f;       // Optimal depth for this species
    float pressureResistance = 1.0f;    // Resistance to pressure effects (1.0 = normal)
    float pressureDamageRate = 0.1f;    // Damage per second when outside depth range

    // Air breathing (for mammals)
    bool canBreathAir = false;          // Can surface for air
    float breathHoldDuration = 0.0f;    // Seconds can stay underwater
    float surfaceBreathTime = 2.0f;     // Time needed at surface to refill

    // Breaching
    float breachMinSpeed = 8.0f;        // Minimum speed to breach
    float breachAngle = 0.7f;           // Radians from vertical for breach
    float airborneGravity = 9.81f;      // Gravity when airborne
    float reentryDrag = 2.0f;           // Extra drag on water reentry
};

// Water current configuration
struct WaterCurrentConfig {
    CurrentType type = CurrentType::LAMINAR;
    glm::vec3 baseDirection = glm::vec3(1.0f, 0.0f, 0.0f);
    float baseStrength = 1.0f;
    float turbulenceScale = 0.1f;       // For turbulent currents
    float tidalPeriod = 120.0f;         // Seconds for tidal reversal
    float depthFalloff = 0.02f;         // How current decreases with depth
    float surfaceMultiplier = 1.5f;     // Current stronger at surface
};

// Pressure effect result
struct PressureEffect {
    float damagePerSecond = 0.0f;       // Damage from pressure
    float speedModifier = 1.0f;         // Speed reduction from pressure
    float energyModifier = 1.0f;        // Energy cost modifier
    bool isInSafeZone = true;           // Within safe depth range
};

// Breaching state
struct BreachState {
    bool isAirborne = false;
    float airborneTime = 0.0f;
    glm::vec3 exitVelocity = glm::vec3(0.0f);
    glm::vec3 exitPosition = glm::vec3(0.0f);
    float maxHeight = 0.0f;
    float rotationAngle = 0.0f;         // Spin during breach
};

// Result of boids calculation
struct SchoolingForces {
    glm::vec3 separation;   // Avoid crowding
    glm::vec3 alignment;    // Match heading
    glm::vec3 cohesion;     // Stay with group
    glm::vec3 predatorAvoidance;  // Flee from predators
    int neighborCount;      // Number of fish in school
};

// SwimBehavior - Manages underwater physics and behavior for aquatic creatures
class SwimBehavior {
public:
    SwimBehavior();
    explicit SwimBehavior(const SwimPhysicsConfig& config);

    // Set configuration
    void setConfig(const SwimPhysicsConfig& config) { m_config = config; }
    const SwimPhysicsConfig& getConfig() const { return m_config; }

    // Set current configuration
    void setCurrentConfig(const WaterCurrentConfig& config) { m_currentConfig = config; }
    const WaterCurrentConfig& getCurrentConfig() const { return m_currentConfig; }

    // Check if a position is underwater
    static bool isInWater(const glm::vec3& pos, const Terrain& terrain);

    // Get the water surface level constant (Y coordinate)
    static float getWaterLevelConstant();

    // Get water surface height at position
    static float getWaterSurfaceHeight(float x, float z, const Terrain& terrain);

    // Get water depth at position (negative = above water)
    static float getWaterDepth(const glm::vec3& pos, const Terrain& terrain);

    // Get sea floor height at position
    static float getSeaFloorHeight(float x, float z, const Terrain& terrain);

    // Calculate buoyancy force
    glm::vec3 calculateBuoyancy(
        const glm::vec3& position,
        const glm::vec3& velocity,
        float targetDepth,
        const Terrain& terrain
    ) const;

    // Calculate water drag force
    glm::vec3 calculateDrag(
        const glm::vec3& velocity,
        const glm::vec3& forward
    ) const;

    // Calculate thrust force from swimming
    glm::vec3 calculateThrust(
        const glm::vec3& forward,
        float swimPhase,
        float speed,
        bool isBursting
    ) const;

    // Calculate schooling forces (boids algorithm)
    SchoolingForces calculateSchoolingForces(
        const glm::vec3& position,
        const glm::vec3& velocity,
        const std::vector<Creature*>& nearbyCreatures,
        float schoolRadius,
        float separationDistance
    ) const;

    // Manage depth - ensures fish stays within valid water bounds
    float clampDepth(
        float targetDepth,
        const glm::vec3& position,
        const Terrain& terrain,
        float minSurfaceDistance = 0.5f,
        float minFloorDistance = 0.5f
    ) const;

    // ==================== NEW DEPTH/PRESSURE METHODS ====================

    // Calculate pressure effect at given depth
    PressureEffect calculatePressureEffect(float depth) const;

    // Calculate water pressure at depth (atmospheres)
    static float calculateWaterPressure(float depth);

    // Check if creature can safely be at this depth
    bool canSurviveAtDepth(float depth) const;

    // Get optimal depth adjustment direction (up/down)
    float getDepthAdjustmentDirection(float currentDepth) const;

    // ==================== NEW CURRENT METHODS ====================

    // Calculate water current at position
    glm::vec3 calculateCurrentAtPosition(
        const glm::vec3& position,
        float time,
        const Terrain& terrain
    ) const;

    // Calculate current effect on movement
    glm::vec3 calculateCurrentEffect(
        const glm::vec3& position,
        const glm::vec3& velocity,
        float time,
        const Terrain& terrain
    ) const;

    // Get local turbulence at position
    glm::vec3 calculateTurbulence(
        const glm::vec3& position,
        float time
    ) const;

    // ==================== NEW BREACHING METHODS ====================

    // Check if conditions allow breaching
    bool canBreach(
        const glm::vec3& velocity,
        float currentDepth
    ) const;

    // Initiate a breach attempt
    bool initiateBreaching(
        glm::vec3& velocity,
        const glm::vec3& position,
        const Terrain& terrain
    );

    // Update airborne physics during breach
    void updateBreachingPhysics(
        glm::vec3& position,
        glm::vec3& velocity,
        float deltaTime,
        const Terrain& terrain
    );

    // Handle water reentry after breach
    void handleWaterReentry(
        glm::vec3& position,
        glm::vec3& velocity,
        const Terrain& terrain
    );

    // Get breach state
    const BreachState& getBreachState() const { return m_breachState; }
    bool isBreaching() const { return m_breachState.isAirborne; }

    // ==================== NEW AIR-BREATHING METHODS ====================

    // Update oxygen level for air breathers
    void updateOxygenLevel(
        float& oxygenLevel,
        float currentDepth,
        float deltaTime,
        const Terrain& terrain
    );

    // Check if creature needs to surface for air
    bool needsToSurface(float oxygenLevel) const;

    // Calculate urgency to surface (0-1)
    float getSurfacingUrgency(float oxygenLevel) const;

    // Full physics update for an aquatic creature
    void updatePhysics(
        glm::vec3& position,
        glm::vec3& velocity,
        const glm::vec3& steeringForce,
        float targetDepth,
        float maxSpeed,
        float deltaTime,
        const Terrain& terrain,
        bool isBursting = false
    );

    // Extended update with pressure and current effects
    void updatePhysicsAdvanced(
        glm::vec3& position,
        glm::vec3& velocity,
        const glm::vec3& steeringForce,
        float targetDepth,
        float maxSpeed,
        float deltaTime,
        float time,
        const Terrain& terrain,
        float& healthDamage,
        bool isBursting = false
    );

    // Get current swim mode
    SwimMode getMode() const { return m_mode; }
    void setMode(SwimMode mode) { m_mode = mode; }

    // Energy cost calculation
    float calculateEnergyCost(float speed, float maxSpeed, float deltaTime) const;

    // Advanced energy cost with depth/current considerations
    float calculateAdvancedEnergyCost(
        float speed,
        float maxSpeed,
        float depth,
        const glm::vec3& currentVelocity,
        const glm::vec3& moveDirection,
        float deltaTime
    ) const;

    // Get swim phase (for animation)
    float getSwimPhase() const { return m_swimPhase; }

private:
    SwimPhysicsConfig m_config;
    WaterCurrentConfig m_currentConfig;
    SwimMode m_mode = SwimMode::CRUISING;
    float m_swimPhase = 0.0f;

    // Breaching state
    BreachState m_breachState;

    // Internal state for smooth motion
    float m_currentThrust = 0.0f;
    glm::vec3 m_smoothedVelocity = glm::vec3(0.0f);

    // Noise functions for turbulence
    float noise3D(float x, float y, float z) const;
    glm::vec3 curlNoise(const glm::vec3& pos) const;
};

// Predator avoidance configuration
struct PredatorAvoidanceConfig {
    float detectionRange = 25.0f;       // Distance at which predators are detected
    float panicRange = 10.0f;           // Distance at which fish panics
    float fleeStrength = 3.0f;          // Multiplier for flee force
    float scatterAngle = 1.0f;          // Radians of random scatter when fleeing
};

// =============================================================================
// AMPHIBIOUS LOCOMOTION BLENDING (PHASE 7 - Agent 3)
// Blends between swim and walk velocity profiles based on transition progress
// =============================================================================

// Configuration for amphibious locomotion
struct AmphibiousLocomotionConfig {
    // Swim velocity profile
    float swimMaxSpeed = 8.0f;
    float swimAcceleration = 4.0f;
    float swimTurnRate = 2.5f;

    // Walk velocity profile
    float walkMaxSpeed = 4.0f;
    float walkAcceleration = 6.0f;
    float walkTurnRate = 4.0f;

    // Shore/transition zone
    float shoreMaxSpeed = 3.0f;         // Slower at shore (awkward movement)
    float shoreDrag = 2.0f;             // Extra drag in shallow water

    // Energy costs per second
    float swimEnergyCost = 0.5f;
    float walkEnergyCost = 0.8f;
    float shoreEnergyCost = 1.2f;       // Inefficient at shore
};

// Result of amphibious velocity calculation
struct AmphibiousVelocityResult {
    glm::vec3 velocity;
    float energyCost;
    float animationBlend;               // 0 = swim animation, 1 = walk animation
};

// Amphibious locomotion helper class
class AmphibiousLocomotion {
public:
    AmphibiousLocomotion() = default;
    explicit AmphibiousLocomotion(const AmphibiousLocomotionConfig& config)
        : m_config(config) {}

    void setConfig(const AmphibiousLocomotionConfig& config) { m_config = config; }
    const AmphibiousLocomotionConfig& getConfig() const { return m_config; }

    // Calculate blended velocity based on locomotion blend factor
    // locomotionBlend: 0 = pure swim, 1 = pure walk
    AmphibiousVelocityResult calculateBlendedVelocity(
        const glm::vec3& currentVelocity,
        const glm::vec3& desiredDirection,
        float locomotionBlend,
        float waterDepth,               // Positive = underwater, negative = above water
        float deltaTime
    ) const;

    // Get max speed for current blend state
    float getMaxSpeed(float locomotionBlend, float waterDepth) const;

    // Get acceleration for current blend state
    float getAcceleration(float locomotionBlend, float waterDepth) const;

    // Get turn rate for current blend state
    float getTurnRate(float locomotionBlend, float waterDepth) const;

    // Get animation blend factor for rendering
    // Returns 0 = full swim animation, 1 = full walk animation
    static float calculateAnimationBlend(float locomotionBlend, float waterDepth);

    // Check if creature should use shore movement mode
    static bool isInShoreZone(float waterDepth);

private:
    AmphibiousLocomotionConfig m_config;

    // Smooth blending helper
    static float smoothBlend(float a, float b, float t);
};

// Helper functions for aquatic ecosystem
namespace AquaticBehavior {
    // Find nearest predator threat
    Creature* findNearestPredator(
        const glm::vec3& position,
        const std::vector<Creature*>& creatures,
        float detectionRange
    );

    // Find nearest prey
    Creature* findNearestPrey(
        const glm::vec3& position,
        const std::vector<Creature*>& creatures,
        float huntingRange,
        float minPreySize,
        float maxPreySize
    );

    // Calculate flee direction from multiple threats
    glm::vec3 calculateFleeDirection(
        const glm::vec3& position,
        const std::vector<Creature*>& predators,
        const PredatorAvoidanceConfig& config
    );

    // Check if creature should scatter (break from school)
    bool shouldScatter(
        const glm::vec3& position,
        const std::vector<Creature*>& predators,
        float panicRange
    );

    // Calculate optimal depth for current situation
    float calculateTargetDepth(
        float preferredDepth,
        float predatorDepth,
        float foodDepth,
        bool hasPredatorNearby,
        bool hasFoodNearby
    );
}
