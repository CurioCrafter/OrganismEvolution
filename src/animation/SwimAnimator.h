#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <cmath>
#include <functional>
#include <memory>

// Forward declarations
struct MorphologyGenes;

namespace animation {

// Swimming style determines how the body undulates
enum class SwimStyle {
    CARANGIFORM,    // Most fish - tail-dominated motion (tuna, mackerel)
    ANGUILLIFORM,   // Full-body waves (eels, lamprey)
    SUBCARANGIFORM, // Posterior 2/3 body motion (trout, carp)
    THUNNIFORM,     // Only caudal fin moves (tuna at high speed)
    LABRIFORM,      // Pectoral fin propulsion (slow maneuvering)
    RAJIFORM,       // Ray-like undulating wings
    JELLYFISH,      // Radial pulsing propulsion
    OCTOPOD,        // Tentacle jet propulsion
    CRUSTACEAN,     // Crab sideways swimming
    SEAHORSE,       // Dorsal fin wave with vertical body
    FLATFISH        // Asymmetric body undulation
};

// Configuration for swimming animation
struct SwimConfig {
    SwimStyle style = SwimStyle::CARANGIFORM;

    // Body wave parameters
    float bodyWaveSpeed = 3.0f;      // How fast the S-wave travels down the body (Hz)
    float bodyWaveAmp = 0.15f;       // Maximum amplitude of body undulation (radians)
    float wavelength = 1.0f;         // Number of wavelengths along the body

    // Stiffness control (0 = flexible, 1 = rigid)
    float headStiffness = 0.85f;     // Head resists motion
    float midStiffness = 0.5f;       // Mid-body moderate flex
    float tailStiffness = 0.15f;     // Tail very flexible

    // Fin parameters
    float dorsalFinAmp = 0.1f;       // Dorsal fin oscillation amplitude
    float pectoralFinAmp = 0.2f;     // Pectoral fin amplitude during turns
    float caudalFinAmp = 0.25f;      // Tail fin amplitude multiplier

    // Speed-dependent modulation
    float minSpeedFactor = 0.3f;     // Animation at zero velocity
    float maxSpeedFactor = 1.5f;     // Animation at max velocity

    // Phase offsets for natural variation
    float phaseOffset = 0.0f;        // Individual creature phase offset

    // Jellyfish-specific parameters
    float pulseFrequency = 1.0f;     // Bell contraction frequency (Hz)
    float pulseAmplitude = 0.4f;     // Bell contraction amount (0-1)
    float recoveryTime = 0.6f;       // Time for bell to re-expand (fraction of cycle)

    // Tentacle/appendage parameters
    int tentacleCount = 8;           // Number of tentacles (jellyfish/octopus)
    float tentacleLength = 1.0f;     // Relative tentacle length
    float tentacleDrag = 0.8f;       // How much tentacles trail (0-1)

    // Crustacean parameters
    float legPaddleFreq = 4.0f;      // Leg paddling frequency
    float sidewaysAngle = 1.57f;     // Angle for sideways movement (radians)

    // Buoyancy parameters
    float buoyancyOffset = 0.0f;     // Vertical oscillation offset
    float buoyancyAmplitude = 0.02f; // Gentle vertical bob amplitude
};

// Runtime state for swimming animation
struct SwimState {
    float swimPhase = 0.0f;          // Current phase of swimming cycle (0-2PI)
    float currentSpeed = 0.0f;        // Normalized speed (0-1)
    float turnAmount = 0.0f;          // How much the fish is turning (-1 to 1)
    float verticalAmount = 0.0f;      // Vertical swimming component (-1 to 1)

    // Damping for smooth transitions
    float dampedPhase = 0.0f;
    float dampedSpeed = 0.0f;
    float dampedTurn = 0.0f;

    // Jellyfish state
    float pulsePhase = 0.0f;         // Current phase in pulse cycle
    bool isContracting = true;       // Whether bell is contracting or recovering

    // Tentacle state (physics-based trailing)
    std::vector<glm::vec3> tentaclePositions;
    std::vector<glm::vec3> tentacleVelocities;

    // Buoyancy state
    float buoyancyPhase = 0.0f;

    // Depth adaptation
    float currentDepth = 0.0f;       // Current water depth
    float pressureCompensation = 1.0f; // Body compression at depth
};

// Tentacle segment for physics simulation
struct TentacleSegment {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::quat rotation;
    float length;
};

// Configuration for a single tentacle
struct TentacleConfig {
    int segmentCount = 6;
    float segmentLength = 0.1f;
    float mass = 0.1f;
    float stiffness = 0.5f;
    float damping = 0.3f;
    float drag = 0.8f;
    glm::vec3 attachPoint;           // Where tentacle attaches to body
};

// SwimAnimator - Applies procedural swimming animation to creature poses
class SwimAnimator {
public:
    SwimAnimator();
    explicit SwimAnimator(const SwimConfig& config);

    // Set configuration
    void setConfig(const SwimConfig& config) { m_config = config; }
    const SwimConfig& getConfig() const { return m_config; }

    // Update animation state based on velocity and delta time
    void update(float deltaTime, const glm::vec3& velocity, float maxSpeed);

    // Apply S-wave motion to a chain of spine bones
    // spineBones: indices of bones from head to tail
    // Returns rotation angles for each bone
    std::vector<glm::quat> calculateSpineWave(const std::vector<int>& spineBones) const;

    // Apply tail fin motion
    // Returns rotation for the caudal fin bone
    glm::quat calculateTailFinMotion() const;

    // Apply pectoral fin motion (for steering)
    // Returns left and right fin rotations
    std::pair<glm::quat, glm::quat> calculatePectoralFinMotion() const;

    // Apply dorsal fin motion
    glm::quat calculateDorsalFinMotion() const;

    // Get current swim phase (for external sync)
    float getSwimPhase() const { return m_state.swimPhase; }

    // Get current animation speed factor
    float getSpeedFactor() const;

    // Reset animation state
    void reset();

    // Apply all animations to bone transforms
    // Modifies the provided bone rotations in place
    void applyToSkeleton(
        std::vector<glm::quat>& boneRotations,
        const std::vector<int>& spineBoneIndices,
        int tailFinIndex,
        int leftPectoralIndex,
        int rightPectoralIndex,
        int dorsalIndex
    ) const;

    // === NEW: Jellyfish Animation ===
    // Calculate bell contraction for jellyfish
    float calculateBellContraction() const;

    // Calculate tentacle rotations (physics-based trailing)
    std::vector<std::vector<glm::quat>> calculateTentacleMotion(
        const std::vector<TentacleConfig>& tentacles,
        const glm::vec3& bodyVelocity
    ) const;

    // Update jellyfish pulse animation
    void updateJellyfishPulse(float deltaTime);

    // === NEW: Eel/Serpentine Animation ===
    // Full-body serpentine wave for eel-like creatures
    std::vector<glm::quat> calculateSerpentineWave(
        const std::vector<int>& spineBones,
        float waveAmplitude,
        float waveFrequency
    ) const;

    // === NEW: Crab/Crustacean Animation ===
    // Calculate leg paddle motions for sideways swimming
    std::vector<glm::quat> calculateCrabLegMotion(int legCount) const;

    // Calculate sideways body orientation
    glm::quat calculateCrabBodyOrientation() const;

    // === NEW: Ray/Manta Animation ===
    // Calculate wing undulation for ray-like creatures
    std::pair<std::vector<glm::quat>, std::vector<glm::quat>> calculateRayWingMotion(
        int leftWingBones,
        int rightWingBones
    ) const;

    // === NEW: Seahorse Animation ===
    // Dorsal fin rapid wave for seahorse propulsion
    std::vector<glm::quat> calculateSeahorseDorsalWave(int finBoneCount) const;

    // Seahorse tail curling
    std::vector<glm::quat> calculateSeahorseTailCurl(
        const std::vector<int>& tailBones,
        float curlTarget
    ) const;

    // === NEW: Octopus Animation ===
    // Jet propulsion body contraction
    float calculateOctopusMantle() const;

    // Individual arm movement with personality
    std::vector<std::vector<glm::quat>> calculateOctopusArms(
        const std::vector<TentacleConfig>& arms,
        const glm::vec3& targetDirection
    ) const;

    // === Depth/Environment Effects ===
    void setWaterDepth(float depth);
    void setWaterCurrent(const glm::vec3& current);
    glm::vec3 getBuoyancyOffset() const;

    // === State Access ===
    const SwimState& getState() const { return m_state; }
    float getPulsePhase() const { return m_state.pulsePhase; }
    bool isContracting() const { return m_state.isContracting; }

private:
    SwimConfig m_config;
    SwimState m_state;
    glm::vec3 m_waterCurrent{0.0f};

    // Calculate stiffness factor for a bone position along the body
    float calculateStiffnessMask(float bodyPosition) const;

    // Smooth step function for blending
    static float smoothstep(float edge0, float edge1, float x);

    // Damping factor for smooth transitions
    static constexpr float DAMPING_FACTOR = 0.85f;

    // Internal jellyfish helpers
    float calculatePulseWave(float phase) const;
    glm::vec3 calculateTentacleDrag(const glm::vec3& velocity, float dragCoeff) const;

    // Internal eel helpers
    float calculateEelWave(float bodyPos, float phase) const;

    // Internal crab helpers
    float calculateLegPhase(int legIndex, int totalLegs) const;
};

// Helper function to create swim config from genome traits
SwimConfig createSwimConfigFromGenome(
    float swimFrequency,    // 1.0 - 4.0 Hz
    float swimAmplitude,    // 0.1 - 0.3
    float finSize,          // 0.3 - 1.0
    float tailSize,         // 0.5 - 1.2
    float creatureSize      // Overall creature size
);

// =============================================================================
// MORPHOLOGY-DRIVEN SWIM CONTROLLER
// Automatically generates swim animation from creature body plan
// =============================================================================

class MorphologySwimController {
public:
    MorphologySwimController();
    ~MorphologySwimController() = default;

    // Initialize from morphology genes
    void initializeFromMorphology(const MorphologyGenes& genes);

    // Set movement state
    void setVelocity(const glm::vec3& velocity);
    void setTargetDirection(const glm::vec3& direction);
    void setMaxSpeed(float maxSpeed);

    // Environment state
    void setWaterDepth(float depth);
    void setWaterCurrent(const glm::vec3& current);
    void setWaterTemperature(float temp);      // Affects metabolism/speed
    void setWaterViscosity(float viscosity);   // Affects drag

    // Update animation
    void update(float deltaTime);

    // Apply to bone transforms
    void applyToBoneRotations(
        std::vector<glm::quat>& rotations,
        const std::vector<int>& spineBones,
        const std::vector<int>& finBones,
        const std::vector<std::vector<int>>& tentacleBones
    );

    // Get current state
    SwimStyle getActiveStyle() const { return m_activeStyle; }
    float getSwimEfficiency() const;  // 0-1, how efficiently creature is swimming
    glm::vec3 getBodyOffset() const;
    glm::quat getBodyTilt() const;

    // Access internal animator
    SwimAnimator& getAnimator() { return m_animator; }
    const SwimAnimator& getAnimator() const { return m_animator; }

private:
    SwimAnimator m_animator;
    SwimStyle m_activeStyle = SwimStyle::CARANGIFORM;

    // Morphology data
    bool m_hasFins = false;
    bool m_hasTail = true;
    bool m_hasTentacles = false;
    int m_finCount = 0;
    int m_tentacleCount = 0;
    float m_bodyLength = 1.0f;

    // Movement state
    glm::vec3 m_velocity{0.0f};
    glm::vec3 m_targetDirection{0.0f, 0.0f, 1.0f};
    float m_maxSpeed = 5.0f;

    // Environment
    float m_waterDepth = 0.0f;
    glm::vec3 m_waterCurrent{0.0f};
    float m_waterTemp = 20.0f;
    float m_waterViscosity = 1.0f;

    // Body motion
    glm::vec3 m_bodyOffset{0.0f};
    glm::quat m_bodyTilt{1.0f, 0.0f, 0.0f, 0.0f};

    // Determine best swim style from morphology
    SwimStyle determineSwimStyle(const MorphologyGenes& genes) const;
};

// =============================================================================
// SWIM STYLE PRESETS
// Pre-configured swim parameters for different aquatic creature types
// =============================================================================

namespace SwimPresets {

    // Standard fish (salmon, trout)
    SwimConfig standardFish();

    // Fast pelagic fish (tuna, marlin)
    SwimConfig pelagicFish();

    // Eel-like (moray, lamprey)
    SwimConfig eelLike();

    // Ray/manta
    SwimConfig rayLike();

    // Jellyfish
    SwimConfig jellyfish();

    // Octopus
    SwimConfig octopus();

    // Crab/lobster
    SwimConfig crustacean();

    // Seahorse
    SwimConfig seahorse();

    // Flatfish (flounder, sole)
    SwimConfig flatfish();

    // Whale/dolphin (large marine mammal)
    SwimConfig cetacean();

    // Sea turtle
    SwimConfig seaTurtle();

    // Sea snake
    SwimConfig seaSnake();

}  // namespace SwimPresets

// =============================================================================
// ADVANCED SWIM BEHAVIORS
// Higher-level swimming behaviors built on top of base animation
// =============================================================================

class SwimBehaviors {
public:
    // Schooling behavior - coordinate with nearby creatures
    static glm::vec3 calculateSchoolingVelocity(
        const glm::vec3& myPosition,
        const glm::vec3& myVelocity,
        const std::vector<glm::vec3>& neighborPositions,
        const std::vector<glm::vec3>& neighborVelocities,
        float separationDist,
        float alignmentDist,
        float cohesionDist
    );

    // Predator avoidance
    static glm::vec3 calculateFleeDirection(
        const glm::vec3& myPosition,
        const glm::vec3& predatorPosition,
        float fleeDistance
    );

    // Prey pursuit with prediction
    static glm::vec3 calculatePursuitDirection(
        const glm::vec3& myPosition,
        const glm::vec3& myVelocity,
        const glm::vec3& preyPosition,
        const glm::vec3& preyVelocity,
        float maxPredictionTime
    );

    // Depth regulation (maintain target depth)
    static float calculateDepthAdjustment(
        float currentDepth,
        float targetDepth,
        float maxVerticalSpeed
    );

    // Surface breathing (for air-breathing aquatics)
    static bool shouldSurface(float currentOxygen, float oxygenThreshold);

    // Bottom feeding behavior
    static glm::vec3 calculateBottomSearchPattern(
        const glm::vec3& position,
        float searchRadius,
        float time
    );

    // Territorial patrol
    static glm::vec3 calculatePatrolPath(
        const glm::vec3& territoryCenter,
        float territoryRadius,
        float time,
        int waypointCount
    );
};

} // namespace animation
