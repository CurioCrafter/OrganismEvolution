#pragma once

#include "Skeleton.h"
#include "../physics/Morphology.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>
#include <memory>

namespace animation {

// =============================================================================
// RIG CONFIGURATION
// Settings that control rig generation from morphology
// =============================================================================

struct RigConfig {
    // Bone count limits
    int maxSpineBones = 12;
    int maxTailBones = 10;
    int maxLimbBones = 4;       // Per limb
    int maxWingBones = 6;       // Per wing
    int maxFinBones = 3;        // Per fin

    // Bone sizing
    float minBoneLength = 0.01f;
    float boneLengthScale = 1.0f;

    // Joint limits
    float defaultHingeLimit = 1.57f;    // 90 degrees
    float defaultBallLimit = 0.78f;     // 45 degrees each axis
    float spineFlexibility = 0.3f;      // Radians per spine bone

    // IK configuration
    bool enableIK = true;
    int ikIterations = 10;
    float ikTolerance = 0.001f;

    // LOD settings
    int minBonesLOD0 = 64;      // Full detail
    int minBonesLOD1 = 32;      // Medium detail
    int minBonesLOD2 = 16;      // Low detail
};

// =============================================================================
// LIMB DEFINITION
// Describes a limb chain in the rig
// =============================================================================

enum class LimbType {
    LEG_FRONT,
    LEG_REAR,
    LEG_MIDDLE,
    ARM,
    WING,
    FIN_PECTORAL,
    FIN_DORSAL,
    FIN_CAUDAL,
    TENTACLE,
    ANTENNA
};

enum class LimbSide {
    CENTER,
    LEFT,
    RIGHT
};

struct LimbDefinition {
    LimbType type;
    LimbSide side;
    int segmentCount;
    float totalLength;
    float baseThickness;
    float taperRatio;

    // Bone indices (filled after skeleton creation)
    std::vector<int32_t> boneIndices;

    // Attachment point on body (normalized 0-1)
    float attachmentPosition;   // Along body length
    glm::vec3 attachmentOffset; // Local offset from spine

    // Joint configuration
    JointType jointType;
    float jointFlexibility;

    // IK target bone index
    int32_t ikTargetBone = -1;

    // Rest pose
    glm::quat restRotation;
    float restSpread;           // Angle from body
};

// =============================================================================
// SPINE DEFINITION
// Describes the spine/backbone structure
// =============================================================================

struct SpineDefinition {
    int boneCount;
    float totalLength;
    std::vector<float> boneLengths;     // Per-bone lengths
    std::vector<float> boneWidths;      // For collision/mesh
    std::vector<int32_t> boneIndices;

    // Flexibility per region (head, neck, torso, pelvis)
    float neckFlexibility;
    float torsoFlexibility;
    float pelvisFlexibility;

    // Key bone indices
    int32_t headBone = -1;
    int32_t neckStartBone = -1;
    int32_t shoulderBone = -1;
    int32_t hipBone = -1;
    int32_t tailStartBone = -1;
};

// =============================================================================
// TAIL DEFINITION
// =============================================================================

struct TailDefinition {
    int boneCount;
    float totalLength;
    float baseTickness;
    float tipThickness;
    TailType tailType;

    std::vector<int32_t> boneIndices;

    // Motion parameters
    float wagAmplitude;
    float wagFrequency;
    float flexibility;
};

// =============================================================================
// HEAD DEFINITION
// =============================================================================

struct HeadDefinition {
    int32_t headBone = -1;
    int32_t jawBone = -1;

    float headSize;
    float jawRange;         // Max jaw opening angle

    // Eye bones (for eye tracking)
    int32_t leftEyeBone = -1;
    int32_t rightEyeBone = -1;
    std::vector<int32_t> extraEyeBones;

    // Feature bones
    int32_t crestBone = -1;
    int32_t frillBone = -1;
    std::vector<int32_t> hornBones;
    std::vector<int32_t> antennaeBones;
    std::vector<int32_t> barbelBones;
};

// =============================================================================
// RIG DEFINITION
// Complete rig specification
// =============================================================================

struct RigDefinition {
    // Source morphology
    MorphologyGenes sourceGenes;

    // Core structure
    SpineDefinition spine;
    TailDefinition tail;
    HeadDefinition head;

    // Limbs
    std::vector<LimbDefinition> limbs;

    // Total bone count
    int totalBones;

    // Category
    enum class RigCategory {
        BIPED,
        QUADRUPED,
        HEXAPOD,
        OCTOPOD,
        SERPENTINE,
        AQUATIC,
        FLYING,
        RADIAL,         // Jellyfish/starfish
        CUSTOM
    } category;

    // Validation
    bool isValid() const;
    std::string getDebugInfo() const;
};

// =============================================================================
// PROCEDURAL RIG GENERATOR
// Creates rigs from morphology genes
// =============================================================================

class ProceduralRigGenerator {
public:
    ProceduralRigGenerator();
    ~ProceduralRigGenerator() = default;

    // Configure generation
    void setConfig(const RigConfig& config) { m_config = config; }
    const RigConfig& getConfig() const { return m_config; }

    // Generate rig definition from morphology
    RigDefinition generateRigDefinition(const MorphologyGenes& genes) const;

    // Build skeleton from rig definition
    Skeleton buildSkeleton(const RigDefinition& rig) const;

    // Convenience: generate skeleton directly from morphology
    Skeleton generateSkeleton(const MorphologyGenes& genes) const;

    // Generate LOD variants
    Skeleton generateSkeletonLOD(const MorphologyGenes& genes, int lodLevel) const;

    // Get rig category for morphology
    static RigDefinition::RigCategory categorizeFromMorphology(const MorphologyGenes& genes);

private:
    RigConfig m_config;

    // Rig generation helpers
    void generateSpine(RigDefinition& rig, const MorphologyGenes& genes) const;
    void generateTail(RigDefinition& rig, const MorphologyGenes& genes) const;
    void generateHead(RigDefinition& rig, const MorphologyGenes& genes) const;
    void generateLegs(RigDefinition& rig, const MorphologyGenes& genes) const;
    void generateArms(RigDefinition& rig, const MorphologyGenes& genes) const;
    void generateWings(RigDefinition& rig, const MorphologyGenes& genes) const;
    void generateFins(RigDefinition& rig, const MorphologyGenes& genes) const;
    void generateTentacles(RigDefinition& rig, const MorphologyGenes& genes) const;
    void generateFeatures(RigDefinition& rig, const MorphologyGenes& genes) const;

    // Skeleton building helpers
    int32_t addSpineBones(Skeleton& skeleton, const SpineDefinition& spine) const;
    int32_t addTailBones(Skeleton& skeleton, const TailDefinition& tail, int32_t parentBone) const;
    int32_t addHeadBones(Skeleton& skeleton, const HeadDefinition& head, int32_t parentBone) const;
    void addLimbBones(Skeleton& skeleton, LimbDefinition& limb, int32_t parentBone) const;

    // LOD helpers
    RigDefinition reduceLOD(const RigDefinition& rig, int lodLevel) const;
    int calculateLODBoneCount(int originalCount, int lodLevel) const;

    // Specialized rig builders
    void buildBipedRig(RigDefinition& rig, const MorphologyGenes& genes) const;
    void buildQuadrupedRig(RigDefinition& rig, const MorphologyGenes& genes) const;
    void buildHexapodRig(RigDefinition& rig, const MorphologyGenes& genes) const;
    void buildSerpentineRig(RigDefinition& rig, const MorphologyGenes& genes) const;
    void buildAquaticRig(RigDefinition& rig, const MorphologyGenes& genes) const;
    void buildFlyingRig(RigDefinition& rig, const MorphologyGenes& genes) const;
    void buildRadialRig(RigDefinition& rig, const MorphologyGenes& genes) const;
};

// =============================================================================
// RIG BONE NAMING CONVENTIONS
// =============================================================================

namespace RigBoneNames {
    // Spine bones
    constexpr const char* ROOT = "root";
    constexpr const char* PELVIS = "pelvis";
    constexpr const char* SPINE_PREFIX = "spine_";
    constexpr const char* CHEST = "chest";
    constexpr const char* NECK_PREFIX = "neck_";
    constexpr const char* HEAD = "head";
    constexpr const char* JAW = "jaw";

    // Tail bones
    constexpr const char* TAIL_PREFIX = "tail_";

    // Leg bones
    constexpr const char* LEG_PREFIX = "leg_";
    constexpr const char* THIGH = "_thigh";
    constexpr const char* SHIN = "_shin";
    constexpr const char* ANKLE = "_ankle";
    constexpr const char* FOOT = "_foot";
    constexpr const char* TOE = "_toe";

    // Arm bones
    constexpr const char* ARM_PREFIX = "arm_";
    constexpr const char* SHOULDER = "_shoulder";
    constexpr const char* UPPER_ARM = "_upper";
    constexpr const char* FOREARM = "_forearm";
    constexpr const char* HAND = "_hand";
    constexpr const char* FINGER = "_finger";

    // Wing bones
    constexpr const char* WING_PREFIX = "wing_";
    constexpr const char* WING_ROOT = "_root";
    constexpr const char* WING_MID = "_mid";
    constexpr const char* WING_TIP = "_tip";

    // Fin bones
    constexpr const char* FIN_DORSAL = "fin_dorsal";
    constexpr const char* FIN_PECTORAL_PREFIX = "fin_pectoral_";
    constexpr const char* FIN_CAUDAL = "fin_caudal";
    constexpr const char* FIN_ANAL = "fin_anal";
    constexpr const char* FIN_PELVIC_PREFIX = "fin_pelvic_";

    // Feature bones
    constexpr const char* CREST = "crest";
    constexpr const char* FRILL = "frill";
    constexpr const char* HORN_PREFIX = "horn_";
    constexpr const char* ANTENNA_PREFIX = "antenna_";
    constexpr const char* BARBEL_PREFIX = "barbel_";
    constexpr const char* EYE_PREFIX = "eye_";

    // Tentacles
    constexpr const char* TENTACLE_PREFIX = "tentacle_";

    // Side suffixes
    constexpr const char* LEFT = "_L";
    constexpr const char* RIGHT = "_R";
    constexpr const char* CENTER = "_C";

    // Helper functions
    std::string makeSpineBone(int index);
    std::string makeNeckBone(int index);
    std::string makeTailBone(int index);
    std::string makeLegBone(int pair, LimbSide side, const char* segment);
    std::string makeArmBone(int pair, LimbSide side, const char* segment);
    std::string makeWingBone(LimbSide side, const char* segment);
    std::string makeFinBone(const char* finType, LimbSide side, int segment = -1);
    std::string makeTentacleBone(int index, int segment);
    std::string makeHornBone(int index);
    std::string makeAntennaBone(int index, int segment);
}

// =============================================================================
// RIG PRESET FACTORIES
// Quick rig creation for common creature types
// =============================================================================

namespace RigPresets {
    // Biped (humanoid)
    RigDefinition createBipedRig(float height = 1.8f, bool hasArms = true, bool hasTail = false);

    // Quadruped (4-legged mammal)
    RigDefinition createQuadrupedRig(float length = 1.0f, float height = 0.6f, bool hasTail = true);

    // Hexapod (6-legged insect)
    RigDefinition createHexapodRig(float length = 0.3f, bool hasWings = false, bool hasAntennae = true);

    // Serpentine (snake/worm)
    RigDefinition createSerpentineRig(float length = 2.0f, int segments = 20);

    // Aquatic (fish)
    RigDefinition createAquaticRig(float length = 0.5f, bool hasLateralFins = true);

    // Flying (bird/bat)
    RigDefinition createFlyingRig(float wingspan = 1.5f, bool hasTail = true, bool hasLegs = true);

    // Radial (jellyfish/starfish)
    RigDefinition createRadialRig(float radius = 0.3f, int armCount = 5);

    // Cephalopod (octopus/squid)
    RigDefinition createCephalopodRig(float bodySize = 0.4f, int tentacleCount = 8);
}

// =============================================================================
// RIG VALIDATION AND DEBUGGING
// =============================================================================

namespace RigValidation {
    // Check rig integrity
    bool validateRig(const RigDefinition& rig, std::string& errorMessage);

    // Check skeleton matches rig definition
    bool validateSkeleton(const Skeleton& skeleton, const RigDefinition& rig, std::string& errorMessage);

    // Get bone statistics
    struct BoneStats {
        int totalBones;
        int spineBones;
        int limbBones;
        int featureBones;
        int maxChainLength;
        float totalBoneLength;
    };
    BoneStats calculateBoneStats(const RigDefinition& rig);
    BoneStats calculateBoneStats(const Skeleton& skeleton);

    // Debug visualization
    std::string rigToString(const RigDefinition& rig);
    std::string skeletonToString(const Skeleton& skeleton);
}

} // namespace animation
