#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <array>
#include <cmath>

namespace animation {

// =============================================================================
// SECONDARY MOTION TYPES
// =============================================================================

enum class SecondaryMotionType {
    TAIL,           // Follow-through tail physics
    EAR,            // Ear flop/rotate
    ANTENNA,        // Antennae sway
    FEATHER,        // Feather ruffle/settle
    FUR,            // Fur ripple
    FAT,            // Soft body wobble
    MUSCLE,         // Muscle jiggle
    TENTACLE,       // Flexible appendage
    TONGUE,         // Tongue dynamics
    DEWLAP,         // Neck flap
    FRILL,          // Neck frill
    WHISKERS,       // Whisker twitch
    MANE,           // Mane flow
    FINS,           // Fin flutter
    MEMBRANE,       // Wing membrane ripple
    CHAIN           // Generic chain dynamics
};

// =============================================================================
// SPRING-DAMPER SYSTEM
// =============================================================================

struct SpringDamperParams {
    float stiffness = 100.0f;     // Spring constant (N/m)
    float damping = 10.0f;        // Damping coefficient
    float mass = 0.1f;            // Mass of element
    float restAngle = 0.0f;       // Rest angle (radians)
    float minAngle = -1.5f;       // Min constraint
    float maxAngle = 1.5f;        // Max constraint
    float drag = 0.1f;            // Air/fluid drag
};

// =============================================================================
// CHAIN ELEMENT (single segment)
// =============================================================================

struct ChainElement {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::quat rotation;
    glm::vec3 angularVelocity;

    float length = 0.1f;
    float mass = 0.1f;
    float stiffness = 50.0f;
    float damping = 5.0f;
    float drag = 0.1f;

    // Constraints
    float maxBendAngle = 0.5f;    // Max bend per segment
    float twistStiffness = 10.0f; // Resistance to twist

    ChainElement() : position(0), velocity(0), rotation(1, 0, 0, 0), angularVelocity(0) {}
};

// =============================================================================
// TAIL DYNAMICS
// =============================================================================

struct TailConfig {
    std::vector<uint32_t> boneIndices;  // Bones from base to tip

    // Physical properties (per segment or interpolated)
    float baseStiffness = 80.0f;   // Stiffness at base
    float tipStiffness = 20.0f;    // Stiffness at tip (softer)
    float baseDamping = 12.0f;
    float tipDamping = 3.0f;
    float baseMass = 0.3f;
    float tipMass = 0.05f;

    float gravityScale = 1.0f;     // How much gravity affects tail
    float inertiaScale = 1.0f;     // How much body motion affects tail

    float maxBendPerSegment = 0.4f;  // Radians
    float twistAmount = 0.1f;        // How much tail can twist

    // Animation overlay
    float waveFrequency = 0.0f;    // For swimming/expression (Hz)
    float waveAmplitude = 0.0f;    // Radians

    // Collision
    float collisionRadius = 0.05f;
    bool collidesWithBody = true;
};

class TailDynamics {
public:
    TailDynamics() = default;

    void initialize(const TailConfig& config);
    void setConfig(const TailConfig& config) { m_config = config; }

    // Update based on body motion
    void update(
        float deltaTime,
        const glm::vec3& rootPosition,
        const glm::quat& rootRotation,
        const glm::vec3& bodyVelocity,
        const glm::vec3& bodyAngularVelocity
    );

    // Get rotations to apply to bones
    const std::vector<glm::quat>& getSegmentRotations() const { return m_rotations; }

    // Get positions (for collision/debug)
    const std::vector<glm::vec3>& getSegmentPositions() const { return m_positions; }

    // Reset to rest pose
    void reset();

    // Expression controls
    void setWag(float amplitude, float frequency);  // Happy wag
    void setCurl(float amount);                      // Curl towards body
    void setRaise(float angle);                      // Raise/lower base

private:
    TailConfig m_config;
    std::vector<ChainElement> m_elements;
    std::vector<glm::quat> m_rotations;
    std::vector<glm::vec3> m_positions;

    // Expression state
    float m_wagAmplitude = 0.0f;
    float m_wagFrequency = 0.0f;
    float m_wagPhase = 0.0f;
    float m_curlAmount = 0.0f;
    float m_raiseAngle = 0.0f;

    void solveConstraints();
    float getInterpolatedStiffness(int index) const;
    float getInterpolatedDamping(int index) const;
    float getInterpolatedMass(int index) const;
};

// =============================================================================
// EAR DYNAMICS
// =============================================================================

struct EarConfig {
    uint32_t baseBoneIndex;
    uint32_t tipBoneIndex = UINT32_MAX;  // Optional

    // Physical properties
    float stiffness = 40.0f;
    float damping = 8.0f;
    float mass = 0.05f;
    float length = 0.1f;

    // Ear shape affects dynamics
    float flopFactor = 0.5f;      // 0 = stiff (cat), 1 = floppy (dog)
    float rotateRange = 0.5f;     // How much ear can rotate (radians)

    // Expression
    float alertAngle = 0.2f;      // Angle when alert
    float relaxedAngle = -0.1f;   // Angle when relaxed
    float backAngle = -0.5f;      // Angle when scared/angry
};

class EarDynamics {
public:
    void initialize(const EarConfig& leftConfig, const EarConfig& rightConfig);

    void update(
        float deltaTime,
        const glm::vec3& headVelocity,
        const glm::vec3& headAngularVelocity,
        float alertness  // 0-1 affects stiffness
    );

    // Get rotations
    glm::quat getLeftEarRotation() const { return m_leftRotation; }
    glm::quat getRightEarRotation() const { return m_rightRotation; }

    // Expression controls
    void setMood(float alertness, float happiness, float fear);
    void pointAt(const glm::vec3& direction);  // Both ears point toward sound

private:
    EarConfig m_leftConfig, m_rightConfig;
    glm::quat m_leftRotation, m_rightRotation;
    glm::vec3 m_leftVelocity, m_rightVelocity;

    // Expression state
    float m_alertness = 0.5f;
    float m_targetLeftAngle = 0.0f;
    float m_targetRightAngle = 0.0f;
};

// =============================================================================
// ANTENNA DYNAMICS
// =============================================================================

struct AntennaConfig {
    std::vector<uint32_t> boneIndices;  // Per-segment bones

    float stiffness = 20.0f;       // Very flexible
    float damping = 2.0f;
    float mass = 0.02f;
    float length = 0.3f;           // Total length

    float gravityScale = 0.3f;
    float airDragScale = 2.0f;     // High drag for thin antenna

    // Movement
    float searchAmplitude = 0.2f;  // Random searching motion
    float searchFrequency = 1.5f;
};

class AntennaDynamics {
public:
    void initialize(const AntennaConfig& leftConfig, const AntennaConfig& rightConfig);

    void update(
        float deltaTime,
        const glm::vec3& headVelocity,
        const glm::vec3& headAngularVelocity
    );

    const std::vector<glm::quat>& getLeftRotations() const { return m_leftRotations; }
    const std::vector<glm::quat>& getRightRotations() const { return m_rightRotations; }

    // Controls
    void setSearching(bool active);
    void pointAt(const glm::vec3& direction);

private:
    AntennaConfig m_leftConfig, m_rightConfig;
    std::vector<ChainElement> m_leftElements, m_rightElements;
    std::vector<glm::quat> m_leftRotations, m_rightRotations;

    bool m_searching = true;
    float m_searchPhase = 0.0f;
};

// =============================================================================
// SOFT BODY DYNAMICS (Fat/Muscle Jiggle)
// =============================================================================

struct SoftBodyConfig {
    // Region definition
    glm::vec3 center;             // Local space center
    float radius = 0.2f;          // Influence radius
    float mass = 0.5f;

    // Jiggle properties
    float stiffness = 200.0f;     // Return to rest
    float damping = 30.0f;        // Energy dissipation
    float maxDisplacement = 0.05f; // Maximum jiggle distance

    // Direction bias
    glm::vec3 jiggleAxis = glm::vec3(0, 1, 0);  // Primary jiggle direction
    float axialBias = 0.7f;       // How much motion is along axis vs omnidirectional
};

class SoftBodyDynamics {
public:
    void initialize(const std::vector<SoftBodyConfig>& regions);

    void update(
        float deltaTime,
        const glm::vec3& bodyVelocity,
        const glm::vec3& bodyAcceleration
    );

    // Get displacement for vertex deformation
    glm::vec3 getDisplacementAt(const glm::vec3& localPosition) const;

    // Get all region displacements
    const std::vector<glm::vec3>& getDisplacements() const { return m_displacements; }

private:
    std::vector<SoftBodyConfig> m_regions;
    std::vector<glm::vec3> m_displacements;
    std::vector<glm::vec3> m_velocities;
};

// =============================================================================
// TENTACLE DYNAMICS
// =============================================================================

struct TentacleConfig {
    std::vector<uint32_t> boneIndices;

    // Physical
    float baseStiffness = 30.0f;
    float tipStiffness = 5.0f;
    float baseDamping = 8.0f;
    float tipDamping = 2.0f;
    float massPerSegment = 0.05f;
    float totalLength = 1.0f;

    // Behavior
    float suction = 0.0f;          // 0-1 grip strength
    float coilStiffness = 20.0f;   // Resistance to coiling

    // Underwater dynamics
    float buoyancy = 0.5f;
    float waterDrag = 3.0f;
};

class TentacleDynamics {
public:
    void initialize(const TentacleConfig& config);

    void update(
        float deltaTime,
        const glm::vec3& basePosition,
        const glm::quat& baseRotation,
        const glm::vec3& bodyVelocity,
        bool isUnderwater
    );

    const std::vector<glm::quat>& getSegmentRotations() const { return m_rotations; }
    const std::vector<glm::vec3>& getSegmentPositions() const { return m_positions; }

    // Controls
    void reachToward(const glm::vec3& target, float strength);
    void coil(float amount);
    void relax();

private:
    TentacleConfig m_config;
    std::vector<ChainElement> m_elements;
    std::vector<glm::quat> m_rotations;
    std::vector<glm::vec3> m_positions;

    glm::vec3 m_reachTarget;
    float m_reachStrength = 0.0f;
    float m_coilAmount = 0.0f;
};

// =============================================================================
// FEATHER/FUR DYNAMICS (Simplified for GPU)
// =============================================================================

struct FeatherParams {
    float ruffleStiffness = 50.0f;    // How quickly feathers settle
    float ruffleDecay = 5.0f;         // How quickly ruffle dissipates
    float windSensitivity = 1.0f;     // Response to wind
    float spreadAmount = 0.3f;        // How much feathers can spread
};

class FeatherDynamics {
public:
    void initialize(const FeatherParams& params);

    void update(
        float deltaTime,
        const glm::vec3& bodyVelocity,
        const glm::vec3& windDirection,
        float windStrength
    );

    // Get ruffle amount for shader (0-1)
    float getRuffleAmount() const { return m_ruffleAmount; }

    // Get spread amount (feathers spreading apart)
    float getSpreadAmount() const { return m_spreadAmount; }

    // Get wind direction (local space)
    const glm::vec3& getWindDirection() const { return m_windDir; }

    // Trigger ruffle (from sudden motion, impact, etc.)
    void triggerRuffle(float intensity);

    // Set spread (display behavior)
    void setSpread(float amount);

private:
    FeatherParams m_params;
    float m_ruffleAmount = 0.0f;
    float m_spreadAmount = 0.0f;
    glm::vec3 m_windDir = glm::vec3(0);
    float m_targetSpread = 0.0f;
};

// =============================================================================
// SECONDARY MOTION SYSTEM - Manages all secondary motion for a creature
// =============================================================================

class SecondaryMotionSystem {
public:
    SecondaryMotionSystem() = default;

    // Initialize subsystems
    void initializeTail(const TailConfig& config);
    void initializeEars(const EarConfig& left, const EarConfig& right);
    void initializeAntennae(const AntennaConfig& left, const AntennaConfig& right);
    void initializeSoftBody(const std::vector<SoftBodyConfig>& regions);
    void initializeTentacles(const std::vector<TentacleConfig>& configs);
    void initializeFeathers(const FeatherParams& params);

    // Update all systems
    void update(
        float deltaTime,
        const glm::vec3& bodyPosition,
        const glm::quat& bodyRotation,
        const glm::vec3& bodyVelocity,
        const glm::vec3& bodyAngularVelocity,
        const glm::vec3& headPosition,
        const glm::quat& headRotation,
        const glm::vec3& windDirection = glm::vec3(0),
        float windStrength = 0.0f,
        bool isUnderwater = false
    );

    // Access subsystems
    TailDynamics* getTail() { return m_tail.get(); }
    EarDynamics* getEars() { return m_ears.get(); }
    AntennaDynamics* getAntennae() { return m_antennae.get(); }
    SoftBodyDynamics* getSoftBody() { return m_softBody.get(); }
    const std::vector<std::unique_ptr<TentacleDynamics>>& getTentacles() const { return m_tentacles; }
    FeatherDynamics* getFeathers() { return m_feathers.get(); }

    // Expression shortcuts
    void setMood(float alertness, float happiness, float fear, float aggression);
    void lookAt(const glm::vec3& worldTarget);

    // Physics shortcuts
    void applyImpact(const glm::vec3& direction, float intensity);

private:
    std::unique_ptr<TailDynamics> m_tail;
    std::unique_ptr<EarDynamics> m_ears;
    std::unique_ptr<AntennaDynamics> m_antennae;
    std::unique_ptr<SoftBodyDynamics> m_softBody;
    std::vector<std::unique_ptr<TentacleDynamics>> m_tentacles;
    std::unique_ptr<FeatherDynamics> m_feathers;

    // Previous frame data for velocity calculation
    glm::vec3 m_prevPosition;
    glm::quat m_prevRotation;
    glm::vec3 m_prevVelocity;
    bool m_initialized = false;
};

} // namespace animation
