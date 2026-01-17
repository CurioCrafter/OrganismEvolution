#pragma once

#include "MetaballSystem.h"
#include "../../physics/Morphology.h"
#include "../../physics/VisualIndicators.h"
#include "../../entities/Genome.h"
#include "../../entities/CreatureType.h"
#include <glm/glm.hpp>
#include <string>

// =============================================================================
// FAMILY ARCHETYPE SYSTEM (Phase 8)
// Defines 8 distinct creature body archetypes for maximum visual diversity
// =============================================================================

enum class FamilyArchetype : uint8_t {
    SEGMENTED = 0,   // Centipede/worm-like: high segment count, many small legs
    PLATED,          // Armadillo/turtle-like: heavy armor plates, compact body
    FINNED,          // Fish/ray-like: prominent fins, streamlined body
    LONG_LIMBED,     // Spider/crane-like: elongated thin limbs, small body
    RADIAL,          // Starfish/jellyfish-like: radial symmetry, tentacles
    BURROWING,       // Mole/wombat-like: compact, powerful claws, small eyes
    GLIDING,         // Flying squirrel/sugar glider: membrane flaps, lightweight
    SPINED,          // Porcupine/hedgehog-like: defensive spines, compact body

    Count
};

// Archetype-specific gene ranges and constraints
struct ArchetypeConstraints {
    // Body proportions
    float minBodyAspect, maxBodyAspect;
    float minBodyWidth, maxBodyWidth;
    float minBodyHeight, maxBodyHeight;

    // Segmentation
    int minSegments, maxSegments;
    float minSegmentTaper, maxSegmentTaper;

    // Limbs
    int minLegPairs, maxLegPairs;
    int minLegSegments, maxLegSegments;
    float minLegLength, maxLegLength;
    float minLegThickness, maxLegThickness;

    // Fins (probability and size)
    float finProbability;
    float minFinSize, maxFinSize;
    int minDorsalFins, maxDorsalFins;

    // Armor/spines
    float armorProbability;
    float minArmorCoverage, maxArmorCoverage;
    float spineProbability;
    int minSpikeRows, maxSpikeRows;
    float minSpikeLength, maxSpikeLength;

    // Special features
    float crestProbability;
    float hornProbability;
    float antennaeProbability;
    float tentacleProbability;

    // Preferred pattern types (indices into PatternType enum)
    uint8_t preferredPatterns[4];
    int numPreferredPatterns;
};

// Get string name for archetype (for debug output)
const char* getArchetypeName(FamilyArchetype archetype);

// =============================================================================
// MORPHOLOGY BUILDER
// Converts the modular BodyPlan system to metaballs for rendering
// =============================================================================

class MorphologyBuilder {
public:
    // Build metaballs from MorphologyGenes (main entry point)
    static void buildFromMorphology(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        CreatureType type,
        const VisualState* visualState = nullptr
    );

    // Build metaballs from BodyPlan (more detailed)
    static void buildFromBodyPlan(
        MetaballSystem& metaballs,
        const BodyPlan& bodyPlan,
        const MorphologyGenes& genes,
        const VisualState* visualState = nullptr
    );

    // Build metaballs for creature at specific life stage
    static void buildForLifeStage(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        CreatureType type,
        LifeStage stage,
        float stageProgress = 0.0f
    );

    // Convert old Genome to new MorphologyGenes
    static MorphologyGenes genomeToMorphology(const Genome& genome, CreatureType type);

    // ==========================================
    // Family Archetype System (Phase 8)
    // ==========================================

    // Determine archetype from species ID and planet seed (deterministic)
    static FamilyArchetype determineArchetype(uint32_t speciesId, uint32_t planetSeed);

    // Get constraints for an archetype
    static const ArchetypeConstraints& getArchetypeConstraints(FamilyArchetype archetype);

    // Apply archetype-specific modifications to morphology genes
    static void applyArchetypeToMorphology(
        MorphologyGenes& genes,
        FamilyArchetype archetype,
        uint32_t speciesId
    );

    // Build archetype-specific features
    static void buildArchetypeFeatures(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        FamilyArchetype archetype,
        const glm::vec3& bodyCenter
    );

    // Get preferred pattern type for archetype
    static uint8_t getArchetypePreferredPattern(FamilyArchetype archetype, uint32_t speciesId);

private:
    // Build individual body parts
    static void buildTorso(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& offset
    );

    static void buildHead(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& neckEnd,
        const VisualState* visualState
    );

    static void buildLimb(
        MetaballSystem& metaballs,
        const glm::vec3& attachPoint,
        const glm::vec3& direction,
        float length,
        float baseThickness,
        int segments,
        AppendageType limbType,
        const MorphologyGenes& genes
    );

    static void buildTail(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& basePosition
    );

    static void buildWings(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& attachPoint
    );

    static void buildFins(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& bodyCenter
    );

    static void buildFeature(
        MetaballSystem& metaballs,
        FeatureType feature,
        const glm::vec3& position,
        float size,
        const glm::vec3& direction
    );

    static void buildEyes(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& headCenter,
        float headRadius
    );

    // ==========================================
    // Extended morphology builders (Phase 7)
    // ==========================================

    static void buildDorsalCrest(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& bodyCenter
    );

    static void buildHorns(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& headPosition,
        float headRadius
    );

    static void buildAntennae(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& headPosition,
        float headRadius
    );

    static void buildNeckFrill(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& neckPosition
    );

    static void buildBodySpines(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& bodyCenter
    );

    static void buildShellArmor(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& bodyCenter
    );

    static void buildEyeSpots(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& bodyCenter
    );

    static void buildBarbels(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& jawPosition
    );

    static void buildTailVariant(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& tailEnd
    );

    static void buildMultipleFins(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& bodyCenter
    );

    // ==========================================
    // Phase 8 Geometry Modules (Archetype-specific)
    // ==========================================

    // Tendrils for radial archetype
    static void buildTendrils(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& bodyCenter,
        int tendrilCount,
        float tendrilLength
    );

    // Membrane flaps for gliding archetype
    static void buildMembraneFlaps(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& bodyCenter
    );

    // Radial arms for radial archetype
    static void buildRadialArms(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& bodyCenter,
        int armCount
    );

    // Heavy digging claws for burrowing archetype
    static void buildDiggingClaws(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& limbEnd,
        float clawSize
    );

    // Articulated armor plates for plated archetype
    static void buildArticulatedPlates(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& bodyCenter
    );

    // Dense spine coverage for spined archetype
    static void buildDenseSpines(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        const glm::vec3& bodyCenter
    );

    // Helper functions
    static glm::vec3 calculateLimbDirection(int limbIndex, int totalLimbs, bool isLeftSide, float spread);
    static float getThicknessAtSegment(float baseThickness, int segment, int totalSegments);

    // Heavy-tailed distribution helper for extreme morphology
    static float heavyTailedValue(float baseValue, float min, float max, float extremeChance = 0.02f);

    // ==========================================
    // LOD and Performance (Phase 7)
    // ==========================================

    // LOD levels for morphology detail
    enum class LODLevel {
        FULL,           // All features (< 10m distance)
        REDUCED,        // Skip small features (10-30m)
        SIMPLIFIED,     // Basic silhouette only (30-100m)
        MINIMAL         // Box/sphere approximation (> 100m)
    };

    // Build with LOD control
    static void buildFromMorphologyWithLOD(
        MetaballSystem& metaballs,
        const MorphologyGenes& genes,
        CreatureType type,
        LODLevel lod,
        const VisualState* visualState = nullptr
    );

    // Get recommended LOD for distance
    static LODLevel getLODForDistance(float distance);

    // Estimate vertex count for generated mesh
    static int estimateVertexCount(const MorphologyGenes& genes, LODLevel lod);

    // Validation: generate test creatures and report stats
    struct MorphologyStats {
        int vertexCount;
        int metaballCount;
        float boundingRadius;
        int featureCount;
        FamilyArchetype archetype;
        bool withinVertexBudget;  // True if within LOD-appropriate budget
    };

    // Vertex budget constants (Phase 8)
    static constexpr int VERTEX_BUDGET_FULL = 18000;      // Full detail
    static constexpr int VERTEX_BUDGET_REDUCED = 8000;    // Reduced detail
    static constexpr int VERTEX_BUDGET_SIMPLIFIED = 2000; // Simplified
    static constexpr int VERTEX_BUDGET_MINIMAL = 200;     // Minimal (billboard-ready)

    static MorphologyStats validateMorphology(
        const MorphologyGenes& genes,
        CreatureType type,
        LODLevel lod = LODLevel::FULL
    );

    // Batch validation for performance testing
    static void validateRandomCreatures(
        int count,
        std::vector<MorphologyStats>& outStats
    );

    // Debug report: logs archetype + vertex counts for N random creatures
    static void generateDebugReport(
        int creatureCount,
        uint32_t planetSeed,
        std::string& outReport
    );

    // Validate archetype distribution (spawn N creatures and report diversity)
    static void validateArchetypeDistribution(
        int creatureCount,
        uint32_t planetSeed,
        std::string& outReport
    );
};

// =============================================================================
// CREATURE MESH GENERATOR
// High-level integration with existing creature rendering
// =============================================================================

class CreatureMeshGenerator {
public:
    // Generate mesh data from morphology
    struct GeneratedMesh {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;
        glm::vec3 center;
        float boundingRadius;
    };

    static GeneratedMesh generate(
        const MorphologyGenes& genes,
        CreatureType type,
        int resolution = 24
    );

    // Generate with visual state modifications
    static GeneratedMesh generateWithState(
        const MorphologyGenes& genes,
        CreatureType type,
        const VisualState& state,
        int resolution = 24
    );

private:
    static void applyPostureToMetaballs(
        MetaballSystem& metaballs,
        const VisualState& state
    );
};
