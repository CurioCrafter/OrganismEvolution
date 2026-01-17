#pragma once

#include "MetaballSystem.h"
#include "../mesh/MeshData.h"
#include "../../entities/Genome.h"
#include "../../entities/aquatic/AquaticCreatures.h"
#include <glm/glm.hpp>
#include <vector>
#include <array>

// =============================================================================
// FISH MESH GENERATOR - Procedural mesh generation for aquatic creatures
// =============================================================================
// Generates detailed meshes for fish, sharks, jellyfish, cephalopods, and more
// Uses metaball blending for organic shapes with animation-ready topology

// Fish body shape types
enum class FishBodyShape {
    FUSIFORM,       // Torpedo shape (tuna, mackerel)
    LATERALLY_COMPRESSED,  // Flat sides (angelfish, discus)
    DEPRESSED,      // Flat top-bottom (ray, flounder)
    ELONGATED,      // Eel-like (eels, pipefish)
    GLOBIFORM,      // Round (pufferfish, sunfish)
    SERPENTINE,     // Snake-like with continuous fins (sea snake)
    TORPEDO         // Sharp-nosed predator (shark, barracuda)
};

// Fin configuration for procedural generation
struct FinConfiguration {
    // Dorsal fin
    float dorsalHeight = 0.3f;      // Height relative to body
    float dorsalLength = 0.4f;      // Length along back
    float dorsalPosition = 0.4f;    // Position along body (0=head, 1=tail)
    int dorsalCount = 1;            // Number of dorsal fins
    bool dorsalSpiked = false;      // Has spines

    // Pectoral fins (side fins)
    float pectoralSize = 0.2f;
    float pectoralAngle = 0.3f;     // Angle from body (radians)
    bool pectoralExtended = false;  // Wing-like (rays)

    // Pelvic fins
    float pelvicSize = 0.15f;
    float pelvicPosition = 0.5f;

    // Anal fin
    float analSize = 0.2f;
    float analPosition = 0.7f;

    // Caudal (tail) fin
    float caudalSize = 0.4f;
    float caudalForkDepth = 0.5f;   // 0=round, 1=deeply forked
    float caudalAsymmetry = 0.0f;   // 0=symmetric, +/-1=asymmetric (sharks)

    // Adipose fin (some fish have a small fin between dorsal and caudal)
    bool hasAdiposeFin = false;
    float adiposeSize = 0.1f;
};

// Jellyfish bell configuration
struct JellyfishConfig {
    float bellRadius = 1.0f;
    float bellHeight = 0.8f;
    float bellThickness = 0.15f;
    int tentacleCount = 16;
    float tentacleLength = 2.0f;
    float tentacleThickness = 0.05f;
    int oralArmCount = 4;
    float oralArmLength = 1.0f;
    float frillAmount = 0.3f;       // Edge frilling
    float translucency = 0.8f;
    glm::vec3 glowColor = glm::vec3(0.2f, 0.5f, 1.0f);
};

// Cephalopod configuration
struct CephalopodConfig {
    float mantleLength = 1.0f;
    float mantleWidth = 0.4f;
    int armCount = 8;               // 8 for octopus, 10 for squid
    float armLength = 1.5f;
    float armThickness = 0.1f;
    int suckerRows = 2;
    float suckerSize = 0.02f;
    bool hasTentacles = false;      // True for squid
    float tentacleLength = 2.0f;
    float finSize = 0.3f;           // 0 for octopus, >0 for squid
    float eyeSize = 0.15f;
    float sifonLength = 0.3f;
};

// Crustacean configuration
struct CrustaceanConfig {
    float carapaceLength = 0.8f;
    float carapaceWidth = 0.5f;
    float carapaceHeight = 0.3f;
    int legPairs = 5;               // Walking legs
    float legLength = 0.6f;
    float clawSize = 0.4f;          // 0 for no claws
    bool hasClaws = true;
    int antennaCount = 4;
    float antennaLength = 0.8f;
    float tailSegments = 6;         // Abdominal segments
    float tailLength = 0.5f;
    bool hasTailFan = true;         // Lobster/shrimp tail
};

// =============================================================================
// FISH MESH GENERATOR CLASS
// =============================================================================

class FishMeshGenerator {
public:
    FishMeshGenerator();

    // Generate mesh from genome
    MeshData generateFromGenome(const Genome& genome, AquaticSpecies species);

    // Generate specific creature types
    MeshData generateFish(const Genome& genome, FishBodyShape bodyShape);
    MeshData generateShark(const Genome& genome);
    MeshData generateJellyfish(const Genome& genome, const JellyfishConfig& config);
    MeshData generateOctopus(const Genome& genome, const CephalopodConfig& config);
    MeshData generateSquid(const Genome& genome, const CephalopodConfig& config);
    MeshData generateCrab(const Genome& genome, const CrustaceanConfig& config);
    MeshData generateLobster(const Genome& genome, const CrustaceanConfig& config);
    MeshData generateEel(const Genome& genome);
    MeshData generateRay(const Genome& genome);
    MeshData generateWhale(const Genome& genome);
    MeshData generateDolphin(const Genome& genome);
    MeshData generateSeaHorse(const Genome& genome);

    // Animation helpers - generate bone/joint positions
    struct SkeletonData {
        std::vector<glm::vec3> jointPositions;
        std::vector<int> parentIndices;
        std::vector<glm::mat4> bindPose;
        std::vector<float> jointWeights;  // Per-vertex weights
    };

    SkeletonData generateFishSkeleton(const Genome& genome, int spineSegments = 8);
    SkeletonData generateJellyfishSkeleton(int tentacleCount);
    SkeletonData generateCephalopodSkeleton(int armCount);

    // LOD generation
    MeshData generateLOD(const MeshData& highDetail, int targetTriangles);

    // Configure generation parameters
    void setResolution(int resolution) { m_resolution = resolution; }
    void setSmoothing(float smoothing) { m_smoothing = smoothing; }

private:
    int m_resolution = 32;          // Marching cubes resolution
    float m_smoothing = 0.5f;       // Surface smoothing factor

    // Internal mesh building methods
    void buildFishBody(
        MetaballSystem& metaballs,
        FishBodyShape shape,
        float length,
        float width,
        float height
    );

    void buildFishHead(
        MetaballSystem& metaballs,
        const glm::vec3& position,
        float size,
        bool predatory
    );

    void buildFishTail(
        MetaballSystem& metaballs,
        const glm::vec3& position,
        float size,
        float forkDepth
    );

    void addDorsalFin(
        MeshData& mesh,
        const glm::vec3& baseStart,
        const glm::vec3& baseEnd,
        float height,
        bool spiked
    );

    void addPectoralFin(
        MeshData& mesh,
        const glm::vec3& attachPoint,
        float size,
        float angle,
        bool mirrored
    );

    void addCaudalFin(
        MeshData& mesh,
        const glm::vec3& attachPoint,
        float size,
        float forkDepth,
        float asymmetry
    );

    void addPelvicFin(
        MeshData& mesh,
        const glm::vec3& attachPoint,
        float size,
        bool mirrored
    );

    void addAnalFin(
        MeshData& mesh,
        const glm::vec3& position,
        float size
    );

    // Jellyfish-specific building
    void buildJellyfishBell(
        MeshData& mesh,
        float radius,
        float height,
        float thickness,
        float frilling
    );

    void addJellyfishTentacles(
        MeshData& mesh,
        int count,
        float length,
        float thickness,
        float bellRadius
    );

    void addOralArms(
        MeshData& mesh,
        int count,
        float length,
        float bellRadius
    );

    // Cephalopod-specific building
    void buildCephalopodMantle(
        MetaballSystem& metaballs,
        float length,
        float width,
        bool hasFinns
    );

    void addCephalopodArm(
        MeshData& mesh,
        const glm::vec3& attachPoint,
        const glm::vec3& direction,
        float length,
        float thickness,
        int suckerRows
    );

    void addCephalopodEye(
        MetaballSystem& metaballs,
        const glm::vec3& position,
        float size
    );

    // Crustacean-specific building
    void buildCrustaceanCarapace(
        MetaballSystem& metaballs,
        float length,
        float width,
        float height
    );

    void addCrustaceanLeg(
        MeshData& mesh,
        const glm::vec3& attachPoint,
        float length,
        int segments,
        bool hasClaw
    );

    void addCrustaceanClaw(
        MetaballSystem& metaballs,
        const glm::vec3& position,
        float size,
        bool mirrored
    );

    void addCrustaceanAntenna(
        MeshData& mesh,
        const glm::vec3& position,
        float length,
        bool thick
    );

    // Utility functions
    MeshData marchingCubes(const MetaballSystem& metaballs);
    void smoothMesh(MeshData& mesh, int iterations);
    void calculateNormals(MeshData& mesh);
    void generateUVs(MeshData& mesh, const glm::vec3& axis = glm::vec3(0, 1, 0));
    void mergeMeshes(MeshData& target, const MeshData& source);

    // Generate procedural fin mesh (flat triangulated surface)
    MeshData generateFinMesh(
        const std::vector<glm::vec3>& outline,
        float thickness,
        bool doubleSided = true
    );

    // Generate tube/tentacle mesh
    MeshData generateTubeMesh(
        const std::vector<glm::vec3>& centerline,
        const std::vector<float>& radii,
        int radialSegments = 8
    );

    // Generate shell/carapace surface
    MeshData generateShellSurface(
        const std::vector<glm::vec3>& controlPoints,
        int uSegments,
        int vSegments
    );

    // Noise functions for organic detail
    float perlinNoise(float x, float y, float z) const;
    float fbmNoise(const glm::vec3& pos, int octaves = 4) const;
};

// =============================================================================
// FISH ANIMATION DATA
// =============================================================================

// Animation state for fish swimming
struct FishAnimationState {
    float swimPhase = 0.0f;         // 0-2PI, swimming cycle
    float swimAmplitude = 0.2f;     // Body wave amplitude
    float swimFrequency = 2.0f;     // Cycles per second
    float turnBend = 0.0f;          // -1 to 1, body bend for turning

    // Fin animation
    float pectoralAngle = 0.0f;     // Current pectoral fin angle
    float dorsalErect = 1.0f;       // 0=folded, 1=erect
    float caudalAngle = 0.0f;       // Tail fin angle

    // Mouth animation
    float mouthOpen = 0.0f;         // 0=closed, 1=open
    float gillFlare = 0.0f;         // 0=normal, 1=flared
};

// Animation state for jellyfish
struct JellyfishAnimationState {
    float pulsePhase = 0.0f;        // Bell contraction phase
    float pulseAmplitude = 0.3f;    // How much bell contracts
    float pulseFrequency = 0.5f;    // Pulses per second

    // Per-tentacle wave offsets
    std::vector<float> tentaclePhases;
    std::vector<float> tentacleAmplitudes;
};

// Animation state for cephalopods
struct CephalopodAnimationState {
    float mantlePulse = 0.0f;       // Breathing/jet propulsion
    float colorChangePhase = 0.0f; // For camouflage animation

    // Per-arm curl state
    std::vector<float> armCurls;    // -1 to 1 curl amount
    std::vector<float> armWaves;    // Wave phase per arm

    // Ink cloud state
    bool inkReleased = false;
    float inkAge = 0.0f;
};

// =============================================================================
// PROCEDURAL PATTERN GENERATOR
// =============================================================================

// Generate procedural textures/colors for aquatic creatures
namespace AquaticPatterns {
    // Fish scale patterns
    glm::vec4 generateScalePattern(
        const glm::vec2& uv,
        float scaleSize,
        const glm::vec3& baseColor,
        const glm::vec3& highlightColor
    );

    // Counter-shading (dark on top, light on bottom)
    glm::vec3 applyCounterShading(
        const glm::vec3& normal,
        const glm::vec3& baseColor,
        float intensity = 0.5f
    );

    // Stripe pattern
    glm::vec3 generateStripes(
        const glm::vec2& uv,
        const glm::vec3& color1,
        const glm::vec3& color2,
        float frequency,
        float angle = 0.0f
    );

    // Spot pattern
    glm::vec3 generateSpots(
        const glm::vec2& uv,
        const glm::vec3& baseColor,
        const glm::vec3& spotColor,
        float spotSize,
        float density
    );

    // Bioluminescence glow
    glm::vec3 generateBiolumGlow(
        const glm::vec3& position,
        const glm::vec3& glowCenter,
        const glm::vec3& glowColor,
        float intensity,
        float radius
    );

    // Iridescent/metallic sheen
    glm::vec3 calculateIridescence(
        const glm::vec3& normal,
        const glm::vec3& viewDir,
        float intensity
    );

    // Translucent jellyfish coloring
    glm::vec4 generateJellyfishColor(
        const glm::vec3& position,
        float bellRadius,
        const glm::vec3& baseColor,
        float translucency
    );
}

