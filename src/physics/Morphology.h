#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>
#include <cmath>

// =============================================================================
// MORPHOLOGY GENE SYSTEM
// Encodes the full body plan of a creature as evolvable parameters
// =============================================================================

// Symmetry types for body organization
enum class SymmetryType {
    BILATERAL,      // Left-right mirror (most animals)
    RADIAL,         // Rotational symmetry (jellyfish, starfish)
    ASYMMETRIC      // No symmetry (rare, specialized)
};

// Joint types with different degrees of freedom
enum class JointType {
    FIXED,          // No movement (fused)
    HINGE,          // 1 DOF - single axis rotation (knee, elbow)
    BALL_SOCKET,    // 3 DOF - rotation in any direction (hip, shoulder)
    SADDLE,         // 2 DOF - two perpendicular axes (thumb base)
    PIVOT           // 1 DOF - rotation around bone axis (forearm)
};

// Appendage specializations
enum class AppendageType {
    LEG,            // Locomotion
    ARM,            // Manipulation
    WING,           // Flight/gliding
    FIN,            // Swimming
    TAIL,           // Balance/propulsion
    TENTACLE,       // Flexible manipulation
    ANTENNA         // Sensing
};

// Special feature types
enum class FeatureType {
    NONE,
    CLAWS,          // Attack/grip
    HORNS,          // Defense/attack
    ANTLERS,        // Display/combat
    PROBOSCIS,      // Feeding
    MANDIBLES,      // Cutting/crushing
    SPIKES,         // Defense
    SHELL,          // Armor
    CREST,          // Display
    BIOLUMINESCENCE,// Communication/luring

    // Extended morphology features (Phase 7)
    DORSAL_RIDGE,   // Ridge along back
    SAIL_FIN,       // Large sail-like dorsal fin
    FRILL,          // Expandable neck/head frill
    BARBELS,        // Whisker-like sensory organs
    EYE_STALKS,     // Protruding eye stalks
    TAIL_CLUB,      // Clubbed tail end
    TAIL_FAN,       // Fan-shaped tail
    TAIL_WHIP,      // Whip-like tail
    BODY_SPINES,    // Rows of spines along body
    SEGMENTED_ARMOR,// Articulated armor plates
    DISPLAY_PLUMES, // Decorative feathers/fins
    EYE_SPOTS,      // False eye patterns
    SPIRAL_HORNS,   // Spiral-shaped horns
    BRANCHED_HORNS, // Multi-branched antler-like horns
    ANTENNAE        // Sensory antennae
};

// Tail variant types (Phase 7)
enum class TailType : uint8_t {
    STANDARD = 0,   // Normal tapered tail
    CLUBBED,        // Club/mace at end
    FAN,            // Fan-shaped
    WHIP,           // Long and thin
    FORKED,         // Split end
    PREHENSILE,     // Gripping capable
    SPIKED          // Spines at end
};

// Crest types (Phase 7)
enum class CrestType : uint8_t {
    NONE = 0,
    RIDGE,          // Low ridge
    SAIL,           // Tall sail fin
    FRILL,          // Expandable frill
    SPINY           // Spiny crest
};

// Horn types (Phase 7)
enum class HornType : uint8_t {
    STRAIGHT = 0,
    CURVED,
    SPIRAL,
    BRANCHED
};

// Jaw types (Phase 7)
enum class JawShape : uint8_t {
    STANDARD = 0,
    UNDERSLUNG,     // Recessed jaw
    PROTRUDING,     // Extended jaw
    BEAK,           // Bird-like beak
    FILTER          // Filter feeder
};

// =============================================================================
// MORPHOLOGY GENES
// =============================================================================

struct MorphologyGenes {
    // === Body Organization ===
    SymmetryType symmetry = SymmetryType::BILATERAL;
    int segmentCount = 3;           // Number of body segments (1-8)
    float segmentTaper = 0.9f;      // How segments change size (0.5-1.5)
    float bodyLength = 1.0f;        // Total body length multiplier
    float bodyWidth = 0.5f;         // Width relative to length
    float bodyHeight = 0.5f;        // Height relative to length

    // === Limbs (Legs) ===
    int legPairs = 2;               // 0-4 pairs (0-8 total legs)
    int legSegments = 3;            // Segments per leg (1-4)
    float legLength = 0.8f;         // Length relative to body
    float legThickness = 0.15f;     // Thickness relative to length
    float legSpread = 0.7f;         // How far apart legs spread (0-1)
    float legAttachPoint = 0.5f;    // Where on body legs attach (0=front, 1=back)

    // === Arms (Manipulation appendages) ===
    int armPairs = 0;               // 0-2 pairs
    int armSegments = 3;            // Segments per arm
    float armLength = 0.6f;         // Length relative to body
    float armThickness = 0.1f;      // Thickness relative to length
    bool hasHands = false;          // Terminal manipulators

    // === Wings ===
    int wingPairs = 0;              // 0-1 pairs (0-2 wings)
    float wingSpan = 2.0f;          // Span relative to body length
    float wingChord = 0.4f;         // Width of wing
    float wingMembraneThickness = 0.02f;
    bool canFly = false;            // True flight vs gliding

    // === Tail ===
    bool hasTail = true;
    int tailSegments = 5;           // Segments in tail
    float tailLength = 0.8f;        // Length relative to body
    float tailThickness = 0.2f;     // Base thickness
    float tailTaper = 0.5f;         // How much it tapers (0-1)
    bool tailPrehensile = false;    // Can grip
    bool hasPrehensibleTail = false; // Alias for tailPrehensile for swim animator
    float tailStrength = 0.5f;      // Strength of tail propulsion (0-1)

    // === Body Shape (Swimming) ===
    int bodySegments = 3;           // Number of body segments for undulation
    float bodyStreamline = 0.5f;    // How streamlined the body is (0-1)

    // === Tentacles (Cephalopods) ===
    int tentacleCount = 0;          // Number of tentacles (0-8)
    float tentacleLength = 1.0f;    // Length relative to body

    // === Legs (General) ===
    int legCount = 0;               // Total leg count (derived from legPairs * 2)

    // === Exoskeleton ===
    bool hasExoskeleton = false;    // Crustacean/insect body plan

    // === Fins (Swimming) ===
    int finCount = 0;               // Total number of fins (0-6)
    bool hasDorsalFin = false;
    bool hasPectoralFins = false;
    bool hasCaudalFin = false;      // Tail fin
    float finSize = 0.3f;           // Relative to body

    // === Head ===
    float headSize = 0.3f;          // Relative to body
    float neckLength = 0.2f;        // Neck length
    float neckFlexibility = 0.8f;   // Range of motion (0-1)
    int eyeCount = 2;               // Number of eyes (0-8)
    float eyeSize = 0.1f;           // Eye size relative to head
    bool eyesSideFacing = false;    // Prey vs predator orientation

    // === Joints ===
    JointType primaryJointType = JointType::HINGE;
    float jointFlexibility = 0.7f;  // Overall flexibility (0-1)
    float jointStrength = 0.5f;     // Torque capacity (0-1)
    float jointDamping = 0.3f;      // Energy dissipation

    // === Special Features ===
    FeatureType primaryFeature = FeatureType::NONE;
    FeatureType secondaryFeature = FeatureType::NONE;
    float featureSize = 0.3f;       // Size of special features
    float armorCoverage = 0.0f;     // 0=none, 1=full armor

    // === Extended Morphology Features (Phase 7) ===

    // Body aspect and tapering
    float bodyAspect = 1.0f;        // Length/width ratio (0.3-3.0)
    float bodyTaper = 0.9f;         // Front-to-back taper

    // Dorsal features (crests, sails, ridges)
    CrestType crestType = CrestType::NONE;
    float crestHeight = 0.0f;       // 0-0.8 relative to body height
    float crestExtent = 0.0f;       // 0-1 (portion of body covered)

    // Horns and antennae
    int hornCount = 0;              // 0-6
    float hornLength = 0.0f;        // 0-1.5 relative to head
    float hornCurvature = 0.0f;     // -1 to 1 (forward/back curve)
    HornType hornType = HornType::STRAIGHT;
    int antennaeCount = 0;          // 0-4
    float antennaeLength = 0.0f;    // 0-2 relative to body

    // Tail variants
    TailType tailType = TailType::STANDARD;
    float tailFinHeight = 0.0f;     // For fan tails
    float tailBulbSize = 0.0f;      // For club tails

    // Jaw and mouth
    JawShape jawShape = JawShape::STANDARD;
    float jawProtrusion = 0.0f;     // -0.3 to 0.5
    float barbelLength = 0.0f;      // Whisker-like appendages (0-1)

    // Limb variation
    int limbSegmentCount = 3;       // 2-5 segments per limb
    float limbTaper = 0.7f;         // 0.3-1.0
    float footSpread = 1.0f;        // 0.3-2.0 (webbing)
    bool hasClaws = false;
    float clawSize = 0.0f;          // 0-0.5

    // Spines and protrusions
    int spikeRows = 0;              // 0-4 rows
    float spikeLength = 0.0f;       // 0-0.6
    float spikeDensity = 0.0f;      // 0-1

    // Shell/armor details
    float shellSegmentation = 0.0f; // 0-1 (articulation)
    int shellTextureType = 0;       // 0=smooth, 1=ridged, 2=bumpy, 3=plated

    // Frills and displays
    bool hasNeckFrill = false;
    float frillSize = 0.0f;         // 0-1.5
    bool hasBodyFrills = false;
    float displayFeatherSize = 0.0f;// 0-1

    // Eye diversity
    int eyeArrangement = 0;         // 0=paired, 1=forward, 2=stalked, 3=compound, 4=wide
    float eyeProtrusion = 0.0f;     // 0-0.5
    bool hasEyeSpots = false;       // False eye patterns
    int eyeSpotCount = 0;           // 0-8

    // Multiple fin support
    int dorsalFinCount = 1;         // 0-3
    int pectoralFinPairs = 1;       // 0-2 pairs
    int ventralFinCount = 0;        // 0-2
    float finAspect = 1.0f;         // 0.3-3.0 (rounded vs swept)
    float finRayDensity = 0.5f;     // For ray-finned appearance

    // === Allometry (Size-dependent traits) ===
    float baseMass = 1.0f;          // Base mass before size scaling
    float densityMultiplier = 1.0f; // Affects actual density
    float metabolicMultiplier = 1.0f; // Affects energy consumption

    // === Metamorphosis ===
    bool hasMetamorphosis = false;
    float metamorphosisAge = 0.0f;  // Age when transformation occurs
    float larvalSpeedBonus = 1.2f;  // Larvae often faster/smaller
    float adultSizeMultiplier = 1.5f; // Adults often larger

    // === Methods ===
    void randomize();
    void mutate(float rate, float strength);
    static MorphologyGenes crossover(const MorphologyGenes& parent1, const MorphologyGenes& parent2);

    // Derived calculations
    int getTotalLimbs() const { return legPairs * 2 + armPairs * 2 + wingPairs * 2; }
    float getExpectedMass() const;
    float getMetabolicRate() const;
    float getMaxSpeed() const;
    float getLimbFrequency() const;
};

// =============================================================================
// JOINT DEFINITION
// =============================================================================

struct JointDefinition {
    JointType type = JointType::HINGE;
    glm::vec3 position = glm::vec3(0.0f);   // Local position
    glm::vec3 axis = glm::vec3(1, 0, 0);    // Primary rotation axis
    float minAngle = -1.57f;                 // Minimum angle (radians)
    float maxAngle = 1.57f;                  // Maximum angle (radians)
    float stiffness = 100.0f;                // Spring constant
    float damping = 10.0f;                   // Damping coefficient
    float maxTorque = 50.0f;                 // Maximum applicable torque

    // For ball-socket joints
    glm::vec3 secondaryAxis = glm::vec3(0, 1, 0);
    float minAngle2 = -0.5f;
    float maxAngle2 = 0.5f;
};

// =============================================================================
// BODY SEGMENT
// =============================================================================

struct BodySegment {
    std::string name;

    // Physical properties
    glm::vec3 localPosition = glm::vec3(0.0f);
    glm::vec3 size = glm::vec3(1.0f);       // Half-extents
    float mass = 1.0f;
    glm::mat3 inertia = glm::mat3(1.0f);    // Inertia tensor

    // Connections
    int parentIndex = -1;                    // -1 for root
    JointDefinition jointToParent;
    std::vector<int> childIndices;

    // Appendage type (if this is a limb segment)
    AppendageType appendageType = AppendageType::LEG;
    int segmentIndexInLimb = 0;              // 0=proximal, higher=distal
    bool isTerminal = false;                 // Last segment of limb

    // Visual
    glm::vec3 color = glm::vec3(0.6f);
    FeatureType feature = FeatureType::NONE;
};

// =============================================================================
// BODY PLAN - Complete description of creature morphology
// =============================================================================

class BodyPlan {
public:
    BodyPlan() = default;

    // Build body plan from genes
    void buildFromGenes(const MorphologyGenes& genes);

    // Access segments
    const std::vector<BodySegment>& getSegments() const { return segments; }
    std::vector<BodySegment>& getSegments() { return segments; }
    int getSegmentCount() const { return static_cast<int>(segments.size()); }

    // Find specific segments
    int findRootSegment() const;
    std::vector<int> findLimbRoots() const;
    std::vector<int> findTerminalSegments() const;

    // Calculate properties
    float getTotalMass() const;
    glm::vec3 getCenterOfMass() const;
    glm::mat3 getTotalInertia() const;

    // Get bounds for mesh generation
    void getBounds(glm::vec3& minBound, glm::vec3& maxBound) const;

private:
    std::vector<BodySegment> segments;
    MorphologyGenes sourceGenes;

    // Building helpers
    void addTorsoSegments(const MorphologyGenes& genes);
    void addHead(const MorphologyGenes& genes);
    void addTail(const MorphologyGenes& genes);
    void addLegs(const MorphologyGenes& genes);
    void addArms(const MorphologyGenes& genes);
    void addWings(const MorphologyGenes& genes);
    void addFins(const MorphologyGenes& genes);
    void addSpecialFeatures(const MorphologyGenes& genes);

    int addSegment(const BodySegment& segment);
    void calculateInertia(BodySegment& segment);
};

// =============================================================================
// ALLOMETRIC SCALING - Size-dependent properties
// =============================================================================

namespace Allometry {
    // Kleiber's law: metabolic rate scales with M^0.75
    inline float metabolicRate(float mass) {
        return std::pow(mass, 0.75f);
    }

    // Maximum sustainable speed scales with M^0.17
    inline float maxSpeed(float mass) {
        return 10.0f * std::pow(mass, 0.17f);
    }

    // Limb frequency scales with M^-0.17
    inline float limbFrequency(float mass) {
        return 2.0f * std::pow(mass, -0.17f);
    }

    // Muscle force scales with M^0.67 (cross-sectional area)
    inline float muscleForce(float mass) {
        return 100.0f * std::pow(mass, 0.67f);
    }

    // Jump height is roughly constant across sizes
    inline float jumpHeight(float /*mass*/) {
        return 0.5f;
    }

    // Bone strength scales with M^0.67
    inline float boneStrength(float mass) {
        return 1000.0f * std::pow(mass, 0.67f);
    }

    // Wing loading for flight
    inline float wingLoading(float mass, float wingArea) {
        return (mass * 9.81f) / wingArea;
    }

    // Can the creature fly? (rough heuristic)
    inline bool canFly(float mass, float wingArea) {
        return wingLoading(mass, wingArea) < 200.0f; // N/m^2
    }
}

// =============================================================================
// LIFE STAGE for metamorphosis
// =============================================================================

enum class LifeStage {
    EGG,
    LARVAL,
    JUVENILE,
    ADULT,
    ELDER
};

struct LifeStageInfo {
    LifeStage stage = LifeStage::ADULT;
    float ageInStage = 0.0f;
    float sizeMultiplier = 1.0f;
    float speedMultiplier = 1.0f;
    float strengthMultiplier = 1.0f;
    bool canReproduce = true;
};
