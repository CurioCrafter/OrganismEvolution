#pragma once

#include "Skeleton.h"
#include "Pose.h"
#include "IKSolver.h"
#include "GaitGenerator.h"
#include "../physics/Morphology.h"
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include <memory>

namespace animation {

// Gait pattern types
enum class GaitType {
    Walk,           // Slow 4-beat gait
    Trot,           // 2-beat diagonal gait
    Canter,         // 3-beat gait
    Gallop,         // 4-beat fast gait
    Crawl,          // Serpentine movement
    Fly,            // Wing-based locomotion
    Swim,           // Aquatic movement
    Hover,          // Stationary flight
    Custom          // User-defined
};

// Configuration for a single foot/limb
struct FootConfig {
    uint32_t hipBoneIndex;      // Start of leg chain
    uint32_t kneeBoneIndex;     // Middle joint
    uint32_t ankleBoneIndex;    // Lower joint
    uint32_t footBoneIndex;     // End effector

    float liftHeight = 0.15f;   // How high to lift foot
    float stepLength = 0.3f;    // Forward distance per step
    float phaseOffset = 0.0f;   // Phase offset (0-1) in gait cycle

    glm::vec3 restOffset;       // Offset from body when at rest
};

// Configuration for a wing
struct WingConfig {
    uint32_t shoulderBoneIndex;
    uint32_t elbowBoneIndex;
    uint32_t wristBoneIndex;
    uint32_t tipBoneIndex;

    float flapAmplitude = 45.0f;  // Degrees
    float flapSpeed = 1.0f;
    float phaseOffset = 0.0f;
};

// Configuration for spine/tail segments
struct SpineConfig {
    std::vector<uint32_t> boneIndices;
    float waveMagnitude = 0.1f;
    float waveFrequency = 1.0f;
    float waveSpeed = 2.0f;
    float phaseOffset = 0.0f;
};

// Ground raycast callback for foot placement
using GroundCallback = std::function<bool(const glm::vec3& origin, const glm::vec3& direction,
                                          float maxDistance, glm::vec3& hitPoint, glm::vec3& hitNormal)>;

// Procedural foot placement result
struct FootPlacement {
    glm::vec3 targetPosition;
    glm::vec3 groundNormal;
    bool isGrounded;
    float blendWeight;      // Weight for IK blending
    float stepProgress;     // 0-1 progress through step
};

// Gait timing configuration
struct GaitTiming {
    float cycleTime = 1.0f;         // Time for one complete cycle
    float dutyFactor = 0.5f;        // Fraction of cycle foot is on ground
    std::vector<float> phaseOffsets; // Per-foot phase offsets
};

// Procedural locomotion controller
class ProceduralLocomotion {
public:
    ProceduralLocomotion() = default;

    // Initialize for a specific skeleton
    void initialize(const Skeleton& skeleton);

    // Configure feet (for bipeds/quadrupeds)
    void addFoot(const FootConfig& config);
    void clearFeet() { m_feet.clear(); }

    // Configure wings (for flying creatures)
    void addWing(const WingConfig& config);
    void clearWings() { m_wings.clear(); }

    // Configure spine (for serpentine motion)
    void setSpine(const SpineConfig& config);

    // Set gait parameters
    void setGaitType(GaitType type);
    void setGaitTiming(const GaitTiming& timing);

    // Set movement parameters
    void setVelocity(const glm::vec3& velocity) { m_velocity = velocity; }
    void setAngularVelocity(float omega) { m_angularVelocity = omega; }
    void setBodyPosition(const glm::vec3& pos) { m_bodyPosition = pos; }
    void setBodyRotation(const glm::quat& rot) { m_bodyRotation = rot; }

    // Ground raycast callback
    void setGroundCallback(GroundCallback callback) { m_groundCallback = std::move(callback); }

    // Update locomotion
    void update(float deltaTime);

    // Apply to skeleton pose
    void applyToPose(const Skeleton& skeleton, SkeletonPose& pose, IKSystem& ikSystem);

    // Get foot placements
    const std::vector<FootPlacement>& getFootPlacements() const { return m_footPlacements; }

    // Get current gait phase (0-1)
    float getGaitPhase() const { return m_gaitPhase; }

    // Get speed factor (0 = standing, 1 = full speed)
    float getSpeedFactor() const;

    // Is creature currently in motion?
    bool isMoving() const { return glm::length(m_velocity) > 0.01f; }

    // Body motion (procedural bob/sway)
    glm::vec3 getBodyOffset() const { return m_bodyOffset; }
    glm::quat getBodyTilt() const { return m_bodyTilt; }

private:
    // Foot data
    std::vector<FootConfig> m_feet;
    std::vector<FootPlacement> m_footPlacements;
    std::vector<glm::vec3> m_footTargets;         // Current IK targets
    std::vector<glm::vec3> m_footPreviousTargets; // Previous targets for blending

    // Wing data
    std::vector<WingConfig> m_wings;

    // Spine data
    SpineConfig m_spine;
    bool m_hasSpine = false;

    // Movement state
    glm::vec3 m_velocity{0.0f};
    float m_angularVelocity = 0.0f;
    glm::vec3 m_bodyPosition{0.0f};
    glm::quat m_bodyRotation{1.0f, 0.0f, 0.0f, 0.0f};

    // Gait state
    GaitType m_gaitType = GaitType::Walk;
    GaitTiming m_gaitTiming;
    float m_gaitPhase = 0.0f;
    float m_time = 0.0f;

    // Body motion
    glm::vec3 m_bodyOffset{0.0f};
    glm::quat m_bodyTilt{1.0f, 0.0f, 0.0f, 0.0f};

    // Ground callback
    GroundCallback m_groundCallback;

    // Internal methods
    void updateGaitPhase(float deltaTime);
    void updateFootPlacements();
    void updateBodyMotion();
    void updateWings(float deltaTime, SkeletonPose& pose);
    void updateSpine(float deltaTime, SkeletonPose& pose);

    // Calculate foot position in step
    glm::vec3 calculateFootPosition(const FootConfig& foot, float phase, const FootPlacement& placement);

    // Raycast to ground
    bool raycastGround(const glm::vec3& origin, glm::vec3& hitPoint, glm::vec3& hitNormal);

    // Get gait phase for specific foot
    float getFootPhase(size_t footIndex) const;
};

// Preset gait configurations
namespace GaitPresets {
    // Biped walk (alternating legs)
    GaitTiming bipedWalk();

    // Biped run
    GaitTiming bipedRun();

    // Quadruped walk (lateral sequence)
    GaitTiming quadrupedWalk();

    // Quadruped trot (diagonal pairs)
    GaitTiming quadrupedTrot();

    // Quadruped gallop
    GaitTiming quadrupedGallop();

    // Six-legged (insect) walk
    GaitTiming hexapodWalk();

    // Eight-legged (spider) walk
    GaitTiming octopodWalk();
}

// Helper to set up locomotion for common creature types
namespace LocomotionSetup {
    // Configure biped locomotion
    void setupBiped(ProceduralLocomotion& loco, const Skeleton& skeleton);

    // Configure quadruped locomotion
    void setupQuadruped(ProceduralLocomotion& loco, const Skeleton& skeleton);

    // Configure flying creature
    void setupFlying(ProceduralLocomotion& loco, const Skeleton& skeleton);

    // Configure aquatic creature
    void setupAquatic(ProceduralLocomotion& loco, const Skeleton& skeleton);

    // Configure serpentine creature
    void setupSerpentine(ProceduralLocomotion& loco, const Skeleton& skeleton);
}

// =============================================================================
// MORPHOLOGY-DRIVEN LOCOMOTION CONTROLLER
// Generates animation automatically from creature body plan
// =============================================================================

class MorphologyLocomotion {
public:
    MorphologyLocomotion();
    ~MorphologyLocomotion() = default;

    // Initialize from morphology genes
    void initializeFromMorphology(const MorphologyGenes& genes, const Skeleton& skeleton);

    // Set terrain callback
    void setGroundCallback(GroundCallback callback);

    // Set movement state
    void setVelocity(const glm::vec3& velocity);
    void setAngularVelocity(float omega);
    void setBodyTransform(const glm::vec3& position, const glm::quat& rotation);

    // Set terrain info
    void setTerrainSlope(float slopeAngle);
    void setTerrainRoughness(float roughness);
    void setIsSwimming(bool swimming);
    void setIsFlying(bool flying);

    // Update animation
    void update(float deltaTime);

    // Apply to skeleton
    void applyToPose(const Skeleton& skeleton, SkeletonPose& pose, IKSystem& ikSystem);

    // Getters
    GaitPattern getCurrentGait() const;
    float getGaitPhase() const;
    glm::vec3 getBodyOffset() const;
    glm::quat getBodyTilt() const;
    bool isInMotion() const;
    bool isInTransition() const;

    // Access internal systems
    GaitGenerator& getGaitGenerator() { return m_gaitGenerator; }
    const GaitGenerator& getGaitGenerator() const { return m_gaitGenerator; }

private:
    GaitGenerator m_gaitGenerator;
    ProceduralLocomotion m_legacyLoco;  // For wing/spine animation

    // State
    glm::vec3 m_velocity;
    glm::vec3 m_bodyPosition;
    glm::quat m_bodyRotation;
    float m_angularVelocity = 0.0f;

    // Terrain
    float m_terrainSlope = 0.0f;
    float m_terrainRoughness = 0.0f;
    bool m_isSwimming = false;
    bool m_isFlying = false;

    // Configuration from morphology
    int m_legCount = 0;
    bool m_hasWings = false;
    bool m_hasTail = false;
    bool m_hasSpine = false;
};

// =============================================================================
// LOCOMOTION STYLE PRESETS
// Pre-configured locomotion styles for different creature archetypes
// =============================================================================

namespace LocomotionStyles {

    // Standard bipedal humanoid walk/run
    struct BipedStyle {
        float walkSpeed = 1.5f;
        float runSpeed = 4.0f;
        float armSwingAmount = 0.3f;
        float hipSwayAmount = 0.02f;
        float shoulderRollAmount = 0.01f;
    };

    // Four-legged mammal style
    struct QuadrupedStyle {
        float walkSpeed = 1.0f;
        float trotSpeed = 3.0f;
        float gallopSpeed = 6.0f;
        float spineFlexAmount = 0.1f;
        float shoulderDropAmount = 0.02f;
    };

    // Six-legged insect style
    struct HexapodStyle {
        float walkSpeed = 0.5f;
        float runSpeed = 2.0f;
        float legLiftHeight = 0.05f;
        float bodyPitchAmount = 0.02f;
    };

    // Spider-like eight-legged style
    struct OctopodStyle {
        float walkSpeed = 0.3f;
        float runSpeed = 1.5f;
        float legSpread = 0.8f;
        float bodyLowerAmount = 0.01f;
    };

    // Snake/eel serpentine style
    struct SerpentineStyle {
        float crawlSpeed = 0.5f;
        float waveAmplitude = 0.2f;
        float waveFrequency = 1.5f;
        float wavelengthMultiplier = 1.0f;
    };

    // Bird/flying creature style
    struct AvianStyle {
        float walkSpeed = 0.5f;
        float runSpeed = 1.5f;
        float hopHeight = 0.1f;
        float headBobAmount = 0.03f;
        float wingFoldAngle = 1.2f;
    };

    // Fish swimming style
    struct AquaticStyle {
        float cruiseSpeed = 2.0f;
        float sprintSpeed = 5.0f;
        float bodyWaveAmplitude = 0.15f;
        float tailAmplitude = 0.3f;
        float finSteeringAmount = 0.2f;
    };

    // Apply style to locomotion system
    void applyBipedStyle(MorphologyLocomotion& loco, const BipedStyle& style);
    void applyQuadrupedStyle(MorphologyLocomotion& loco, const QuadrupedStyle& style);
    void applyHexapodStyle(MorphologyLocomotion& loco, const HexapodStyle& style);
    void applyOctopodStyle(MorphologyLocomotion& loco, const OctopodStyle& style);
    void applySerpentineStyle(MorphologyLocomotion& loco, const SerpentineStyle& style);
    void applyAvianStyle(MorphologyLocomotion& loco, const AvianStyle& style);
    void applyAquaticStyle(MorphologyLocomotion& loco, const AquaticStyle& style);

    // Auto-detect best style from morphology
    void autoApplyStyle(MorphologyLocomotion& loco, const MorphologyGenes& genes);
}

} // namespace animation
