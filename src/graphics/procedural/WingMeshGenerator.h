#pragma once

#include "../mesh/MeshData.h"
#include "../../entities/Genome.h"
#include "../../entities/flying/FlyingCreatures.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <array>

// =============================================================================
// WING MESH GENERATOR - Procedural mesh generation for flying creatures
// =============================================================================
// Generates detailed wing meshes for birds, bats, insects, and fantasy creatures
// Produces animation-ready topology with bone weights and feather/membrane segments

// Wing types matching the animation system
enum class WingMeshType {
    FEATHERED_ELLIPTICAL,   // Sparrows, songbirds (good maneuverability)
    FEATHERED_HIGH_SPEED,   // Falcons, swifts (pointed, swept back)
    FEATHERED_HIGH_ASPECT,  // Albatross, gulls (long, narrow for gliding)
    FEATHERED_SLOTTED,      // Eagles, vultures (slots for soaring)
    MEMBRANE_BAT,           // Bat wings (finger membrane)
    MEMBRANE_DRAGON,        // Dragon/pterosaur wings (large membrane)
    INSECT_DIPTERA,         // Flies, mosquitoes (2 wings)
    INSECT_ODONATA,         // Dragonflies (4 independent wings)
    INSECT_LEPIDOPTERA,     // Butterflies (4 scaled wings)
    INSECT_HYMENOPTERA,     // Bees, wasps (4 coupled wings)
    INSECT_COLEOPTERA       // Beetles (hardened forewings)
};

// Feather configuration for bird wings
struct FeatherConfig {
    int primaryCount = 10;      // Primary flight feathers (outer wing)
    int secondaryCount = 12;    // Secondary feathers (inner wing)
    int tertialCount = 4;       // Tertial feathers (nearest body)
    int covertRows = 3;         // Rows of covert feathers
    float primaryLength = 1.0f;
    float secondaryLength = 0.8f;
    float featherWidth = 0.1f;
    float rachisThickness = 0.01f;  // Feather shaft thickness
    float barbDensity = 1.0f;   // Barb detail level
    float iridescence = 0.0f;   // 0-1 iridescent coloring
    glm::vec3 baseColor = glm::vec3(0.4f, 0.35f, 0.3f);
    glm::vec3 tipColor = glm::vec3(0.2f, 0.2f, 0.2f);
};

// Membrane configuration for bat/dragon wings
struct MembraneConfig {
    float thickness = 0.02f;        // Membrane thickness
    float elasticity = 0.3f;        // How much it can stretch
    int fingerCount = 4;            // Number of finger bones (3-5)
    float fingerLength = 1.0f;      // Length of longest finger
    float fingerSpread = 0.8f;      // Spread angle between fingers
    float thumbSize = 0.15f;        // Thumb/claw at wing wrist
    float webbing = 1.0f;           // 0-1 how much webbing between fingers
    float veinDensity = 0.5f;       // Visible vein pattern
    float translucency = 0.3f;      // Light transmission
    glm::vec3 membraneColor = glm::vec3(0.3f, 0.25f, 0.2f);
    glm::vec3 boneColor = glm::vec3(0.2f, 0.18f, 0.15f);
};

// Insect wing configuration
struct InsectWingConfig {
    float length = 0.5f;            // Wing length
    float width = 0.2f;             // Wing width at widest
    float thickness = 0.002f;       // Very thin membrane
    float veinComplexity = 0.5f;    // 0-1 vein pattern complexity
    bool hasScales = false;         // Butterfly wing scales
    float scaleIridescence = 0.0f;  // Scale iridescence
    bool hasHindwings = true;       // Diptera only have forewings
    float hindwingRatio = 0.8f;     // Hindwing size vs forewing
    float couplingStrength = 0.0f;  // Wing coupling (hymenoptera)
    bool isHardened = false;        // Elytra (beetle forewings)
    glm::vec3 color = glm::vec3(0.8f, 0.8f, 0.85f);
    glm::vec3 veinColor = glm::vec3(0.2f, 0.2f, 0.2f);
};

// Wing bone/joint structure for animation
struct WingBone {
    std::string name;
    glm::vec3 position;
    glm::quat rotation;
    float length;
    int parentIndex;  // -1 for root
};

struct WingSkeleton {
    std::vector<WingBone> bones;
    std::vector<glm::mat4> bindPose;
    std::vector<glm::mat4> inverseBindPose;

    // Common bone indices
    int shoulderIdx = -1;
    int elbowIdx = -1;
    int wristIdx = -1;
    int primaryIdx = -1;  // First primary feather bone

    // Per-vertex bone weights (4 bones per vertex max)
    std::vector<glm::ivec4> boneIndices;
    std::vector<glm::vec4> boneWeights;
};

// Individual feather mesh data for detailed rendering
struct FeatherMesh {
    MeshData mesh;
    glm::vec3 attachPoint;      // Where it attaches to wing
    glm::vec3 restDirection;    // Natural direction
    float length;
    float stiffness;            // How much it bends
    int boneIndex;              // Which bone it's attached to
};

// =============================================================================
// WING MESH GENERATOR CLASS
// =============================================================================

class WingMeshGenerator {
public:
    WingMeshGenerator();

    // Generate wing mesh from genome
    MeshData generateFromGenome(const Genome& genome, WingMeshType type);

    // Generate specific wing types
    MeshData generateFeatheredWing(const Genome& genome, const FeatherConfig& config);
    MeshData generateMembraneWing(const Genome& genome, const MembraneConfig& config);
    MeshData generateInsectWing(const Genome& genome, const InsectWingConfig& config);
    MeshData generateDragonWing(const Genome& genome, const MembraneConfig& config);

    // Generate wing pair (both left and right)
    std::pair<MeshData, MeshData> generateWingPair(const Genome& genome, WingMeshType type);

    // Generate skeleton for animation
    WingSkeleton generateFeatheredSkeleton(const Genome& genome, int segmentCount = 4);
    WingSkeleton generateMembraneSkeleton(const Genome& genome, int fingerCount = 4);
    WingSkeleton generateInsectSkeleton();

    // Generate individual feathers (for detailed close-up rendering)
    std::vector<FeatherMesh> generatePrimaryFeathers(const FeatherConfig& config);
    std::vector<FeatherMesh> generateSecondaryFeathers(const FeatherConfig& config);
    std::vector<FeatherMesh> generateCovertFeathers(const FeatherConfig& config);
    FeatherMesh generateSingleFeather(float length, float width, int barbCount);

    // Generate wing for specific creature types
    MeshData generateSongbirdWing(const Genome& genome);
    MeshData generateRaptorWing(const Genome& genome);
    MeshData generateHummingbirdWing(const Genome& genome);
    MeshData generateOwlWing(const Genome& genome);
    MeshData generateSeabirdWing(const Genome& genome);
    MeshData generateBatWing(const Genome& genome);
    MeshData generateDragonflyWing(const Genome& genome);
    MeshData generateButterflyWing(const Genome& genome);
    MeshData generateBeeWing(const Genome& genome);

    // LOD generation
    MeshData generateLOD(const MeshData& highDetail, int targetTriangles);

    // Configure generation parameters
    void setResolution(int resolution) { m_resolution = resolution; }
    void setFeatherDetail(int detail) { m_featherDetail = detail; }
    void setMembraneSubdivisions(int subdivs) { m_membraneSubdivisions = subdivs; }

private:
    int m_resolution = 16;
    int m_featherDetail = 8;
    int m_membraneSubdivisions = 12;

    // Internal generation helpers

    // Feathered wing generation
    void generateWingBone(MeshData& mesh, const glm::vec3& start, const glm::vec3& end,
                          float radius, int segments);
    void generateFeatherRow(MeshData& mesh, const std::vector<glm::vec3>& attachPoints,
                            const glm::vec3& direction, const FeatherConfig& config,
                            float lengthMultiplier);
    void generateFeatherGeometry(MeshData& mesh, const glm::vec3& base,
                                 const glm::vec3& direction, float length, float width,
                                 int barbCount);
    void generateAlulaFeathers(MeshData& mesh, const glm::vec3& wristPos,
                               const FeatherConfig& config);
    void generateWinglet(MeshData& mesh, const glm::vec3& tipPos,
                         const FeatherConfig& config);

    // Membrane wing generation
    void generateFingerBone(MeshData& mesh, const glm::vec3& start, const glm::vec3& end,
                            float startRadius, float endRadius, int segments);
    void generateMembraneBetween(MeshData& mesh, const std::vector<glm::vec3>& edge1,
                                 const std::vector<glm::vec3>& edge2, float thickness);
    void generateMembraneVeins(MeshData& mesh, const std::vector<glm::vec3>& veinPaths,
                               float thickness);
    void generateThumbClaw(MeshData& mesh, const glm::vec3& position, float size);

    // Insect wing generation
    void generateInsectVeins(MeshData& mesh, const std::vector<glm::vec3>& veinPath,
                             float thickness);
    void generateInsectMembrane(MeshData& mesh, const std::vector<glm::vec3>& outline,
                                float thickness);
    void generateButterflyScales(MeshData& mesh, const std::vector<glm::vec3>& surface,
                                 float scaleSize, const glm::vec3& color);
    void generateHaltere(MeshData& mesh, const glm::vec3& position, float size);

    // Shape functions for wing profiles
    glm::vec3 getEllipticalWingProfile(float t, float span, float chord);
    glm::vec3 getPointedWingProfile(float t, float span, float chord);
    glm::vec3 getHighAspectWingProfile(float t, float span, float chord);
    glm::vec3 getSlottedWingTip(float t, int slotCount, float slotDepth);
    glm::vec3 getInsectWingProfile(float t, float length, float width);
    glm::vec3 getButterflyWingProfile(float t, float length, float width);

    // Airfoil cross-section
    std::vector<glm::vec3> generateAirfoilProfile(float chord, float camber,
                                                   float thickness, int points);

    // Utility functions
    void calculateNormals(MeshData& mesh);
    void generateUVCoordinates(MeshData& mesh, float uScale, float vScale);
    void smoothMesh(MeshData& mesh, int iterations);
    void weldVertices(MeshData& mesh, float threshold);

    // Bone weight calculation
    void calculateBoneWeights(MeshData& mesh, const WingSkeleton& skeleton);

    // Config generation from genome
    FeatherConfig configFromGenome(const Genome& genome);
    MembraneConfig membraneConfigFromGenome(const Genome& genome);
    InsectWingConfig insectConfigFromGenome(const Genome& genome);
};

// =============================================================================
// WING POSE FOR MESH DEFORMATION
// =============================================================================

struct WingMeshPose {
    glm::quat shoulderRotation;
    glm::quat elbowRotation;
    glm::quat wristRotation;
    glm::quat primaryRotation;

    // For membrane wings - finger positions
    std::vector<glm::quat> fingerRotations;

    // Feather spread (0-1)
    float featherSpread = 0.0f;

    // Wing tip bend (degrees)
    float wingTipBend = 0.0f;

    // Apply pose to skeleton
    void applyToSkeleton(WingSkeleton& skeleton) const;
};

// =============================================================================
// WING ANIMATION HELPERS
// =============================================================================

namespace WingMeshAnimation {
    // Deform mesh based on skeleton pose
    MeshData deformMesh(const MeshData& restPose, const WingSkeleton& skeleton,
                        const std::vector<glm::mat4>& boneTransforms);

    // Calculate bone transforms from pose
    std::vector<glm::mat4> calculateBoneTransforms(const WingSkeleton& skeleton,
                                                    const WingMeshPose& pose);

    // Interpolate between poses
    WingMeshPose interpolatePoses(const WingMeshPose& a, const WingMeshPose& b, float t);

    // Generate flapping animation keyframes
    std::vector<WingMeshPose> generateFlapCycle(float amplitude, float frequency,
                                                 int keyframeCount);
}
