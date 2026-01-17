#include "MorphologyBuilder.h"
#include "MarchingCubes.h"
#include "../../physics/Metamorphosis.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <sstream>
#include <iomanip>

// =============================================================================
// FAMILY ARCHETYPE SYSTEM IMPLEMENTATION (Phase 8)
// =============================================================================

const char* getArchetypeName(FamilyArchetype archetype) {
    switch (archetype) {
        case FamilyArchetype::SEGMENTED:   return "Segmented";
        case FamilyArchetype::PLATED:      return "Plated";
        case FamilyArchetype::FINNED:      return "Finned";
        case FamilyArchetype::LONG_LIMBED: return "Long-Limbed";
        case FamilyArchetype::RADIAL:      return "Radial";
        case FamilyArchetype::BURROWING:   return "Burrowing";
        case FamilyArchetype::GLIDING:     return "Gliding";
        case FamilyArchetype::SPINED:      return "Spined";
        default: return "Unknown";
    }
}

// Archetype constraint definitions - each archetype has distinct visual characteristics
static const ArchetypeConstraints s_archetypeConstraints[] = {
    // SEGMENTED: Centipede/worm-like
    {
        .minBodyAspect = 2.0f, .maxBodyAspect = 4.0f,     // Long body
        .minBodyWidth = 0.2f, .maxBodyWidth = 0.4f,       // Narrow
        .minBodyHeight = 0.2f, .maxBodyHeight = 0.35f,    // Low profile
        .minSegments = 5, .maxSegments = 12,              // Many segments
        .minSegmentTaper = 0.9f, .maxSegmentTaper = 1.0f, // Consistent size
        .minLegPairs = 4, .maxLegPairs = 8,               // Many legs
        .minLegSegments = 2, .maxLegSegments = 3,         // Simple legs
        .minLegLength = 0.3f, .maxLegLength = 0.5f,       // Short legs
        .minLegThickness = 0.05f, .maxLegThickness = 0.1f,
        .finProbability = 0.1f,                           // Rarely has fins
        .minFinSize = 0.1f, .maxFinSize = 0.2f,
        .minDorsalFins = 0, .maxDorsalFins = 1,
        .armorProbability = 0.3f,
        .minArmorCoverage = 0.2f, .maxArmorCoverage = 0.5f,
        .spineProbability = 0.2f,
        .minSpikeRows = 1, .maxSpikeRows = 2,
        .minSpikeLength = 0.02f, .maxSpikeLength = 0.08f,
        .crestProbability = 0.1f,
        .hornProbability = 0.1f,
        .antennaeProbability = 0.7f,                      // Often has antennae
        .tentacleProbability = 0.1f,
        .preferredPatterns = {2, 5, 10, 9},               // Spots, Scales, Speckled, Bands
        .numPreferredPatterns = 4
    },
    // PLATED: Armadillo/turtle-like
    {
        .minBodyAspect = 0.8f, .maxBodyAspect = 1.5f,     // Compact
        .minBodyWidth = 0.5f, .maxBodyWidth = 0.8f,       // Wide
        .minBodyHeight = 0.4f, .maxBodyHeight = 0.7f,     // Domed
        .minSegments = 2, .maxSegments = 4,               // Few segments
        .minSegmentTaper = 0.85f, .maxSegmentTaper = 0.95f,
        .minLegPairs = 2, .maxLegPairs = 3,
        .minLegSegments = 2, .maxLegSegments = 3,
        .minLegLength = 0.4f, .maxLegLength = 0.6f,
        .minLegThickness = 0.12f, .maxLegThickness = 0.2f, // Thick legs
        .finProbability = 0.05f,
        .minFinSize = 0.1f, .maxFinSize = 0.15f,
        .minDorsalFins = 0, .maxDorsalFins = 0,
        .armorProbability = 0.95f,                         // Almost always has armor
        .minArmorCoverage = 0.6f, .maxArmorCoverage = 1.0f,// Heavy coverage
        .spineProbability = 0.3f,
        .minSpikeRows = 0, .maxSpikeRows = 2,
        .minSpikeLength = 0.03f, .maxSpikeLength = 0.1f,
        .crestProbability = 0.2f,
        .hornProbability = 0.4f,
        .antennaeProbability = 0.1f,
        .tentacleProbability = 0.0f,
        .preferredPatterns = {5, 3, 11, 7},               // Scales, Patches, Marbled, Camouflage
        .numPreferredPatterns = 4
    },
    // FINNED: Fish/ray-like
    {
        .minBodyAspect = 1.5f, .maxBodyAspect = 3.0f,     // Streamlined
        .minBodyWidth = 0.3f, .maxBodyWidth = 0.6f,
        .minBodyHeight = 0.3f, .maxBodyHeight = 0.5f,
        .minSegments = 2, .maxSegments = 4,
        .minSegmentTaper = 0.85f, .maxSegmentTaper = 0.95f,
        .minLegPairs = 0, .maxLegPairs = 2,               // Often no legs
        .minLegSegments = 2, .maxLegSegments = 2,
        .minLegLength = 0.2f, .maxLegLength = 0.4f,
        .minLegThickness = 0.05f, .maxLegThickness = 0.1f,
        .finProbability = 1.0f,                            // Always has fins
        .minFinSize = 0.3f, .maxFinSize = 0.6f,           // Large fins
        .minDorsalFins = 1, .maxDorsalFins = 3,           // Multiple dorsals
        .armorProbability = 0.2f,
        .minArmorCoverage = 0.1f, .maxArmorCoverage = 0.3f,
        .spineProbability = 0.3f,
        .minSpikeRows = 0, .maxSpikeRows = 1,
        .minSpikeLength = 0.02f, .maxSpikeLength = 0.05f,
        .crestProbability = 0.4f,                          // Sail fins
        .hornProbability = 0.1f,
        .antennaeProbability = 0.2f,
        .tentacleProbability = 0.1f,
        .preferredPatterns = {5, 15, 1, 9},               // Scales, Countershading, Stripes, Bands
        .numPreferredPatterns = 4
    },
    // LONG_LIMBED: Spider/crane-like
    {
        .minBodyAspect = 0.8f, .maxBodyAspect = 1.2f,     // Small body
        .minBodyWidth = 0.2f, .maxBodyWidth = 0.35f,      // Compact core
        .minBodyHeight = 0.2f, .maxBodyHeight = 0.35f,
        .minSegments = 1, .maxSegments = 3,
        .minSegmentTaper = 0.9f, .maxSegmentTaper = 1.0f,
        .minLegPairs = 3, .maxLegPairs = 6,               // Many legs
        .minLegSegments = 3, .maxLegSegments = 5,         // Multi-jointed
        .minLegLength = 1.2f, .maxLegLength = 2.0f,       // Very long legs
        .minLegThickness = 0.03f, .maxLegThickness = 0.06f, // Thin legs
        .finProbability = 0.1f,
        .minFinSize = 0.1f, .maxFinSize = 0.15f,
        .minDorsalFins = 0, .maxDorsalFins = 0,
        .armorProbability = 0.15f,
        .minArmorCoverage = 0.1f, .maxArmorCoverage = 0.3f,
        .spineProbability = 0.2f,
        .minSpikeRows = 0, .maxSpikeRows = 1,
        .minSpikeLength = 0.02f, .maxSpikeLength = 0.05f,
        .crestProbability = 0.1f,
        .hornProbability = 0.15f,
        .antennaeProbability = 0.5f,
        .tentacleProbability = 0.2f,
        .preferredPatterns = {0, 2, 10, 17},              // Solid, Spots, Speckled, Tribal
        .numPreferredPatterns = 4
    },
    // RADIAL: Starfish/jellyfish-like
    {
        .minBodyAspect = 0.8f, .maxBodyAspect = 1.2f,     // Roughly circular
        .minBodyWidth = 0.4f, .maxBodyWidth = 0.7f,
        .minBodyHeight = 0.3f, .maxBodyHeight = 0.5f,
        .minSegments = 1, .maxSegments = 2,               // Minimal segments
        .minSegmentTaper = 0.9f, .maxSegmentTaper = 1.0f,
        .minLegPairs = 0, .maxLegPairs = 0,               // No legs, uses arms
        .minLegSegments = 0, .maxLegSegments = 0,
        .minLegLength = 0.0f, .maxLegLength = 0.0f,
        .minLegThickness = 0.0f, .maxLegThickness = 0.0f,
        .finProbability = 0.3f,
        .minFinSize = 0.2f, .maxFinSize = 0.4f,
        .minDorsalFins = 0, .maxDorsalFins = 1,
        .armorProbability = 0.2f,
        .minArmorCoverage = 0.1f, .maxArmorCoverage = 0.4f,
        .spineProbability = 0.4f,
        .minSpikeRows = 1, .maxSpikeRows = 3,
        .minSpikeLength = 0.03f, .maxSpikeLength = 0.1f,
        .crestProbability = 0.3f,
        .hornProbability = 0.1f,
        .antennaeProbability = 0.3f,
        .tentacleProbability = 0.9f,                       // Almost always has tendrils
        .preferredPatterns = {4, 8, 16, 13},              // Gradient, Rings, Eyespots, Rosettes
        .numPreferredPatterns = 4
    },
    // BURROWING: Mole/wombat-like
    {
        .minBodyAspect = 1.2f, .maxBodyAspect = 1.8f,     // Torpedo-like
        .minBodyWidth = 0.4f, .maxBodyWidth = 0.6f,
        .minBodyHeight = 0.35f, .maxBodyHeight = 0.5f,
        .minSegments = 2, .maxSegments = 4,
        .minSegmentTaper = 0.9f, .maxSegmentTaper = 1.0f,
        .minLegPairs = 2, .maxLegPairs = 3,
        .minLegSegments = 2, .maxLegSegments = 3,
        .minLegLength = 0.3f, .maxLegLength = 0.5f,       // Short, powerful
        .minLegThickness = 0.15f, .maxLegThickness = 0.25f, // Very thick
        .finProbability = 0.05f,
        .minFinSize = 0.1f, .maxFinSize = 0.15f,
        .minDorsalFins = 0, .maxDorsalFins = 0,
        .armorProbability = 0.4f,
        .minArmorCoverage = 0.2f, .maxArmorCoverage = 0.5f,
        .spineProbability = 0.2f,
        .minSpikeRows = 0, .maxSpikeRows = 1,
        .minSpikeLength = 0.02f, .maxSpikeLength = 0.05f,
        .crestProbability = 0.1f,
        .hornProbability = 0.3f,
        .antennaeProbability = 0.4f,
        .tentacleProbability = 0.1f,
        .preferredPatterns = {0, 7, 12, 18},              // Solid, Camouflage, Mottled, Brindle
        .numPreferredPatterns = 4
    },
    // GLIDING: Flying squirrel/sugar glider
    {
        .minBodyAspect = 1.0f, .maxBodyAspect = 1.6f,
        .minBodyWidth = 0.3f, .maxBodyWidth = 0.5f,
        .minBodyHeight = 0.2f, .maxBodyHeight = 0.35f,    // Flat profile
        .minSegments = 2, .maxSegments = 3,
        .minSegmentTaper = 0.9f, .maxSegmentTaper = 1.0f,
        .minLegPairs = 2, .maxLegPairs = 3,
        .minLegSegments = 3, .maxLegSegments = 4,
        .minLegLength = 0.7f, .maxLegLength = 1.0f,       // Long for membrane attachment
        .minLegThickness = 0.04f, .maxLegThickness = 0.08f, // Light
        .finProbability = 0.3f,
        .minFinSize = 0.15f, .maxFinSize = 0.3f,
        .minDorsalFins = 0, .maxDorsalFins = 1,
        .armorProbability = 0.05f,                         // Almost never armored
        .minArmorCoverage = 0.05f, .maxArmorCoverage = 0.15f,
        .spineProbability = 0.1f,
        .minSpikeRows = 0, .maxSpikeRows = 0,
        .minSpikeLength = 0.01f, .maxSpikeLength = 0.03f,
        .crestProbability = 0.3f,
        .hornProbability = 0.1f,
        .antennaeProbability = 0.2f,
        .tentacleProbability = 0.1f,
        .preferredPatterns = {1, 4, 15, 6},               // Stripes, Gradient, Countershading, Feathers
        .numPreferredPatterns = 4
    },
    // SPINED: Porcupine/hedgehog-like
    {
        .minBodyAspect = 0.9f, .maxBodyAspect = 1.5f,
        .minBodyWidth = 0.4f, .maxBodyWidth = 0.6f,
        .minBodyHeight = 0.4f, .maxBodyHeight = 0.6f,     // Round/domed
        .minSegments = 2, .maxSegments = 4,
        .minSegmentTaper = 0.9f, .maxSegmentTaper = 1.0f,
        .minLegPairs = 2, .maxLegPairs = 3,
        .minLegSegments = 2, .maxLegSegments = 3,
        .minLegLength = 0.4f, .maxLegLength = 0.6f,
        .minLegThickness = 0.08f, .maxLegThickness = 0.15f,
        .finProbability = 0.1f,
        .minFinSize = 0.1f, .maxFinSize = 0.2f,
        .minDorsalFins = 0, .maxDorsalFins = 0,
        .armorProbability = 0.3f,
        .minArmorCoverage = 0.15f, .maxArmorCoverage = 0.4f,
        .spineProbability = 1.0f,                          // Always has spines
        .minSpikeRows = 3, .maxSpikeRows = 6,             // Many spine rows
        .minSpikeLength = 0.1f, .maxSpikeLength = 0.25f,  // Long spines
        .crestProbability = 0.4f,
        .hornProbability = 0.2f,
        .antennaeProbability = 0.1f,
        .tentacleProbability = 0.0f,
        .preferredPatterns = {0, 10, 18, 9},              // Solid, Speckled, Brindle, Bands
        .numPreferredPatterns = 4
    }
};

// =============================================================================
// MORPHOLOGY BUILDER IMPLEMENTATION
// =============================================================================

void MorphologyBuilder::buildFromMorphology(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    CreatureType type,
    const VisualState* visualState)
{
    metaballs.clear();

    // Center position
    glm::vec3 center(0.0f);

    // Build torso
    buildTorso(metaballs, genes, center);

    // Calculate where head and tail attach
    float bodyHalfLength = genes.bodyLength * 0.5f;
    glm::vec3 frontEnd = center + glm::vec3(0, genes.bodyHeight * 0.4f, bodyHalfLength);
    glm::vec3 backEnd = center + glm::vec3(0, genes.bodyHeight * 0.4f, -bodyHalfLength);

    // Build head with neck
    glm::vec3 neckEnd = frontEnd + glm::vec3(0, genes.neckLength * 0.3f, genes.neckLength);
    buildHead(metaballs, genes, neckEnd, visualState);

    // Build tail
    if (genes.hasTail) {
        buildTail(metaballs, genes, backEnd);
    }

    // Build legs
    if (genes.legPairs > 0) {
        float legSpacing = genes.bodyLength / (genes.legPairs + 1);

        for (int pair = 0; pair < genes.legPairs; pair++) {
            float zPos = -bodyHalfLength + legSpacing * (pair + 1);

            for (int side = 0; side < 2; side++) {
                float xDir = (side == 0) ? -1.0f : 1.0f;
                glm::vec3 attachPoint = center + glm::vec3(
                    genes.bodyWidth * 0.45f * xDir,
                    0,
                    zPos
                );

                glm::vec3 direction = glm::normalize(glm::vec3(xDir * genes.legSpread, -1.0f, 0.0f));

                buildLimb(metaballs, attachPoint, direction,
                         genes.legLength * genes.bodyLength,
                         genes.legThickness * genes.bodyWidth,
                         genes.legSegments,
                         AppendageType::LEG,
                         genes);
            }
        }
    }

    // Build arms
    if (genes.armPairs > 0) {
        glm::vec3 shoulderPos = frontEnd + glm::vec3(0, genes.bodyHeight * 0.2f, -genes.bodyLength * 0.1f);

        for (int pair = 0; pair < genes.armPairs; pair++) {
            for (int side = 0; side < 2; side++) {
                float xDir = (side == 0) ? -1.0f : 1.0f;
                glm::vec3 attachPoint = shoulderPos + glm::vec3(
                    genes.bodyWidth * 0.4f * xDir,
                    0,
                    -pair * genes.bodyLength * 0.1f
                );

                glm::vec3 direction = glm::normalize(glm::vec3(xDir * 0.8f, -0.3f, 0.4f));

                buildLimb(metaballs, attachPoint, direction,
                         genes.armLength * genes.bodyLength,
                         genes.armThickness * genes.bodyWidth,
                         genes.armSegments,
                         AppendageType::ARM,
                         genes);
            }
        }
    }

    // Build wings
    if (genes.wingPairs > 0) {
        glm::vec3 wingAttach = center + glm::vec3(0, genes.bodyHeight * 0.5f, 0);
        buildWings(metaballs, genes, wingAttach);
    }

    // Build fins
    if (genes.finCount > 0 || genes.hasDorsalFin || genes.hasPectoralFins || genes.hasCaudalFin) {
        buildFins(metaballs, genes, center);
    }

    // Build special features
    if (genes.primaryFeature != FeatureType::NONE) {
        glm::vec3 headPos = neckEnd + glm::vec3(0, 0, genes.headSize * genes.bodyWidth);
        buildFeature(metaballs, genes.primaryFeature, headPos, genes.featureSize, glm::vec3(0, 0, 1));
    }

    // ==========================================
    // Extended morphology features (Phase 7)
    // ==========================================

    glm::vec3 headPos = neckEnd + glm::vec3(0, 0, genes.headSize * genes.bodyWidth);
    float headRadius = genes.headSize * genes.bodyWidth;

    // Build dorsal crest/sail/ridge
    if (genes.crestType != CrestType::NONE && genes.crestHeight > 0.05f) {
        buildDorsalCrest(metaballs, genes, center);
    }

    // Build horns (if not already built via primaryFeature)
    if (genes.hornCount > 0 && genes.hornLength > 0.05f &&
        genes.primaryFeature != FeatureType::HORNS &&
        genes.primaryFeature != FeatureType::SPIRAL_HORNS &&
        genes.primaryFeature != FeatureType::BRANCHED_HORNS) {
        buildHorns(metaballs, genes, headPos, headRadius);
    }

    // Build antennae
    if (genes.antennaeCount > 0 && genes.antennaeLength > 0.1f) {
        buildAntennae(metaballs, genes, headPos, headRadius);
    }

    // Build neck frill
    if (genes.hasNeckFrill && genes.frillSize > 0.1f) {
        buildNeckFrill(metaballs, genes, neckEnd);
    }

    // Build body spines
    if (genes.spikeRows > 0 && genes.spikeLength > 0.02f) {
        buildBodySpines(metaballs, genes, center);
    }

    // Build shell/armor
    if (genes.armorCoverage > 0.1f) {
        buildShellArmor(metaballs, genes, center);
    }

    // Build eye spots (subtle geometry markers)
    if (genes.hasEyeSpots && genes.eyeSpotCount > 0) {
        buildEyeSpots(metaballs, genes, center);
    }

    // Build barbels
    if (genes.barbelLength > 0.1f) {
        glm::vec3 jawPos = headPos + glm::vec3(0, -headRadius * 0.3f, headRadius * 0.5f);
        buildBarbels(metaballs, genes, jawPos);
    }

    // Build tail variant features
    if (genes.tailType != TailType::STANDARD && genes.hasTail) {
        glm::vec3 tailEnd = backEnd + glm::vec3(0, 0, -genes.tailLength * genes.bodyLength);
        buildTailVariant(metaballs, genes, tailEnd);
    }

    // Build multiple fins (if more than default count)
    if (genes.dorsalFinCount > 1 || genes.pectoralFinPairs > 1 || genes.ventralFinCount > 0) {
        buildMultipleFins(metaballs, genes, center);
    }

    // Build secondary feature
    if (genes.secondaryFeature != FeatureType::NONE) {
        glm::vec3 secondaryPos = center + glm::vec3(0, genes.bodyHeight * 0.3f, 0);
        buildFeature(metaballs, genes.secondaryFeature, secondaryPos, genes.featureSize * 0.7f, glm::vec3(0, 1, 0));
    }
}

void MorphologyBuilder::buildFromBodyPlan(
    MetaballSystem& metaballs,
    const BodyPlan& bodyPlan,
    const MorphologyGenes& genes,
    const VisualState* visualState)
{
    metaballs.clear();

    const auto& segments = bodyPlan.getSegments();

    for (const auto& seg : segments) {
        // Add metaballs for each segment based on size
        float radius = (seg.size.x + seg.size.y + seg.size.z) / 3.0f;

        // Main segment ball
        metaballs.addMetaball(seg.localPosition, radius, 1.0f);

        // Add extra balls for long segments
        if (seg.size.z > radius * 1.5f) {
            int extraBalls = static_cast<int>(seg.size.z / radius);
            for (int i = 1; i < extraBalls; i++) {
                float t = static_cast<float>(i) / extraBalls;
                glm::vec3 pos = seg.localPosition + glm::vec3(0, 0, (t - 0.5f) * seg.size.z * 2.0f);
                metaballs.addMetaball(pos, radius * 0.9f, 1.0f);
            }
        }

        // Add feature metaballs
        if (seg.feature != FeatureType::NONE) {
            glm::vec3 featurePos = seg.localPosition + glm::vec3(0, seg.size.y, seg.size.z * 0.5f);
            buildFeature(metaballs, seg.feature, featurePos, genes.featureSize, glm::vec3(0, 0, 1));
        }
    }

    // Apply posture modifications if visual state provided
    if (visualState && visualState->postureSlump > 0.01f) {
        // Modify metaball positions based on posture
        // This would require accessing and modifying the metaballs directly
    }
}

void MorphologyBuilder::buildForLifeStage(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    CreatureType type,
    LifeStage stage,
    float stageProgress)
{
    // Get appropriate morphology for life stage
    MorphologyGenes stageMorphology = genes;

    switch (stage) {
        case LifeStage::EGG:
            // Simple sphere
            metaballs.clear();
            metaballs.addMetaball(glm::vec3(0), genes.baseMass * 0.3f, 1.0f);
            return;

        case LifeStage::LARVAL:
            if (genes.hasMetamorphosis) {
                stageMorphology = LarvalMorphology::generateCompleteLarval(genes);
            } else {
                stageMorphology.baseMass *= 0.5f;
                stageMorphology.bodyLength *= 0.7f;
            }
            break;

        case LifeStage::JUVENILE:
            stageMorphology.baseMass *= 0.7f;
            stageMorphology.bodyLength *= 0.85f;
            stageMorphology.featureSize *= 0.6f;
            break;

        case LifeStage::ELDER:
            // Slightly shrunken
            stageMorphology.bodyHeight *= 0.95f;
            break;

        default:
            break;
    }

    buildFromMorphology(metaballs, stageMorphology, type, nullptr);
}

MorphologyGenes MorphologyBuilder::genomeToMorphology(const Genome& genome, CreatureType type) {
    MorphologyGenes morph;

    // Map old genome values to new morphology
    morph.baseMass = genome.size;
    morph.bodyLength = 0.5f + genome.size * 0.5f;
    morph.bodyWidth = 0.3f + genome.size * 0.2f;
    morph.bodyHeight = 0.3f + genome.size * 0.2f;

    // Determine body plan from type
    if (type == CreatureType::HERBIVORE) {
        morph.legPairs = 2; // Quadruped
        morph.eyesSideFacing = true;
        morph.primaryFeature = FeatureType::NONE;
    } else {
        morph.legPairs = 2; // Can be biped or quadruped
        morph.eyesSideFacing = false;
        morph.primaryFeature = FeatureType::CLAWS;
    }

    // Speed affects leg configuration
    if (genome.speed > 15.0f) {
        morph.legLength = 0.9f;
    } else {
        morph.legLength = 0.7f;
    }

    // Vision affects eyes
    morph.eyeSize = 0.05f + (genome.visionRange / 50.0f) * 0.15f;

    // Efficiency affects metabolism
    morph.metabolicMultiplier = 1.0f / genome.efficiency;

    // ==========================================
    // Map new morphology diversity genes (Phase 7)
    // ==========================================

    // Body structure from genome
    morph.segmentCount = genome.segmentCount;
    morph.bodyAspect = genome.bodyAspect;
    morph.segmentTaper = genome.bodyTaper;

    // Fin configuration
    morph.dorsalFinCount = genome.dorsalFinCount;
    morph.pectoralFinPairs = genome.pectoralFinCount / 2;  // Convert count to pairs
    morph.ventralFinCount = genome.ventralFinCount;
    morph.finAspect = genome.finAspect;
    morph.finRayDensity = genome.finRayCount / 12.0f;  // Normalize to 0-1

    // Crest/sail/ridge
    morph.crestType = static_cast<CrestType>(genome.crestType);
    morph.crestHeight = genome.crestHeight;
    morph.crestExtent = genome.crestExtent;

    // Horns and antennae
    morph.hornCount = genome.hornCount;
    morph.hornLength = genome.hornLength;
    morph.hornCurvature = genome.hornCurvature;
    morph.hornType = static_cast<HornType>(genome.hornType);
    morph.antennaeCount = genome.antennaeCount;
    morph.antennaeLength = genome.antennaeLength;

    // Tail variant
    morph.tailType = static_cast<TailType>(genome.tailVariant);
    morph.tailFinHeight = genome.tailFinHeight;
    morph.tailBulbSize = genome.tailBulbSize;

    // Jaw configuration
    morph.jawShape = static_cast<JawShape>(genome.jawType);
    morph.jawProtrusion = genome.jawProtrusion;
    morph.barbelLength = genome.barbels;

    // Limb structure
    morph.limbSegmentCount = genome.limbSegments;
    morph.limbTaper = genome.limbTaper;
    morph.footSpread = genome.footSpread;
    morph.hasClaws = genome.hasClaws;
    morph.clawSize = genome.clawLength;

    // Spines
    morph.spikeRows = genome.spikeRows;
    morph.spikeLength = genome.spikeLength;
    morph.spikeDensity = genome.spikeDensity;

    // Shell/armor
    morph.armorCoverage = genome.shellCoverage;
    morph.shellSegmentation = genome.shellSegmentation;
    morph.shellTextureType = genome.shellTexture;

    // Frills and displays
    morph.hasNeckFrill = genome.hasNeckFrill;
    morph.frillSize = genome.frillSize;
    morph.hasBodyFrills = genome.hasBodyFrills;
    morph.displayFeatherSize = genome.displayFeatherSize;

    // Eye diversity
    morph.eyeArrangement = genome.eyeArrangement;
    morph.eyeProtrusion = genome.eyeProtrusion;
    morph.hasEyeSpots = genome.hasEyeSpots;
    morph.eyeSpotCount = genome.eyeSpotCount;

    // Update primary/secondary features based on genome traits
    if (genome.hornCount > 0) {
        if (genome.hornType == 2) {
            morph.primaryFeature = FeatureType::SPIRAL_HORNS;
        } else if (genome.hornType == 3) {
            morph.primaryFeature = FeatureType::BRANCHED_HORNS;
        } else {
            morph.primaryFeature = FeatureType::HORNS;
        }
    }

    if (genome.crestHeight > 0.1f) {
        if (morph.primaryFeature == FeatureType::NONE) {
            if (genome.crestType == 2) {
                morph.primaryFeature = FeatureType::SAIL_FIN;
            } else if (genome.crestType == 3) {
                morph.primaryFeature = FeatureType::FRILL;
            } else {
                morph.primaryFeature = FeatureType::CREST;
            }
        } else {
            morph.secondaryFeature = FeatureType::CREST;
        }
    }

    if (genome.spikeRows > 0) {
        if (morph.secondaryFeature == FeatureType::NONE) {
            morph.secondaryFeature = FeatureType::BODY_SPINES;
        }
    }

    if (genome.hasEyeSpots && genome.eyeSpotCount > 0) {
        if (morph.secondaryFeature == FeatureType::NONE) {
            morph.secondaryFeature = FeatureType::EYE_SPOTS;
        }
    }

    // Use some neural weights for body variation (legacy)
    if (genome.neuralWeights.size() >= 12) {
        morph.hasTail = genome.neuralWeights[0] > 0.0f;
        morph.tailLength = 0.5f + std::abs(genome.neuralWeights[1]) * 0.5f;
        morph.headSize = 0.2f + std::abs(genome.neuralWeights[2]) * 0.2f;
        morph.neckLength = 0.1f + std::abs(genome.neuralWeights[3]) * 0.2f;
        morph.jointFlexibility = 0.5f + genome.neuralWeights[4] * 0.3f;
        morph.jointStrength = 0.5f + genome.neuralWeights[5] * 0.3f;

        if (std::abs(genome.neuralWeights[7]) > 0.8f) {
            morph.wingPairs = 1;
            morph.canFly = genome.neuralWeights[8] > 0.5f;
        }
        if (std::abs(genome.neuralWeights[9]) > 0.85f) {
            morph.hasMetamorphosis = true;
            morph.metamorphosisAge = 20.0f + std::abs(genome.neuralWeights[10]) * 30.0f;
        }
    }

    return morph;
}

void MorphologyBuilder::buildTorso(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& offset)
{
    float segmentLength = genes.bodyLength / genes.segmentCount;
    float startZ = -genes.bodyLength * 0.5f;
    float currentScale = 1.0f;

    for (int i = 0; i < genes.segmentCount; i++) {
        float z = startZ + segmentLength * (i + 0.5f);

        // Taper affects width/height
        float width = genes.bodyWidth * currentScale;
        float height = genes.bodyHeight * currentScale;

        // Main segment ball
        glm::vec3 pos = offset + glm::vec3(0, height * 0.5f, z);
        float radius = (width + height) * 0.25f;
        metaballs.addMetaball(pos, radius, 1.0f);

        // Add side bulges for wider segments
        if (width > height * 1.2f) {
            float bulgeRadius = radius * 0.6f;
            metaballs.addMetaball(pos + glm::vec3(width * 0.3f, 0, 0), bulgeRadius, 0.8f);
            metaballs.addMetaball(pos + glm::vec3(-width * 0.3f, 0, 0), bulgeRadius, 0.8f);
        }

        // Top/bottom for taller segments
        if (height > width * 1.2f) {
            float bulgeRadius = radius * 0.5f;
            metaballs.addMetaball(pos + glm::vec3(0, height * 0.2f, 0), bulgeRadius, 0.7f);
        }

        currentScale *= genes.segmentTaper;
    }
}

void MorphologyBuilder::buildHead(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& neckEnd,
    const VisualState* visualState)
{
    float headRadius = genes.headSize * genes.bodyWidth;

    // Apply head droop from visual state
    glm::vec3 headPos = neckEnd + glm::vec3(0, 0, headRadius * 0.8f);
    if (visualState && visualState->headDroop > 0.01f) {
        headPos.y -= visualState->headDroop * headRadius;
    }

    // Main head
    metaballs.addMetaball(headPos, headRadius, 1.0f);

    // Snout/muzzle
    glm::vec3 snoutPos = headPos + glm::vec3(0, -headRadius * 0.2f, headRadius * 0.6f);
    metaballs.addMetaball(snoutPos, headRadius * 0.5f, 0.9f);

    // Build eyes
    buildEyes(metaballs, genes, headPos, headRadius);

    // Neck connection
    glm::vec3 neckPos = (neckEnd + headPos) * 0.5f;
    metaballs.addMetaball(neckPos, headRadius * 0.6f, 0.8f);
}

void MorphologyBuilder::buildLimb(
    MetaballSystem& metaballs,
    const glm::vec3& attachPoint,
    const glm::vec3& direction,
    float length,
    float baseThickness,
    int segments,
    AppendageType limbType,
    const MorphologyGenes& genes)
{
    float segmentLength = length / segments;
    glm::vec3 currentPos = attachPoint;
    glm::vec3 currentDir = direction;

    for (int i = 0; i < segments; i++) {
        float thickness = getThicknessAtSegment(baseThickness, i, segments);

        // Move to segment center
        glm::vec3 segmentCenter = currentPos + currentDir * (segmentLength * 0.5f);
        metaballs.addMetaball(segmentCenter, thickness, 1.0f);

        // Joint bulge
        if (i < segments - 1) {
            glm::vec3 jointPos = currentPos + currentDir * segmentLength;
            metaballs.addMetaball(jointPos, thickness * 1.1f, 0.7f);
        }

        // Move to next segment
        currentPos = currentPos + currentDir * segmentLength;

        // Bend legs downward for natural pose
        if (limbType == AppendageType::LEG && i < segments - 1) {
            currentDir = glm::normalize(currentDir + glm::vec3(0, -0.3f, 0));
        }
    }

    // Foot/hand at end
    if (limbType == AppendageType::LEG) {
        float footRadius = baseThickness * 0.6f;
        metaballs.addMetaball(currentPos + glm::vec3(0, -footRadius * 0.3f, footRadius * 0.3f),
                             footRadius, 0.9f);
    } else if (limbType == AppendageType::ARM && genes.hasHands) {
        float handRadius = baseThickness * 0.5f;
        metaballs.addMetaball(currentPos, handRadius, 0.9f);
        // Fingers
        for (int f = 0; f < 3; f++) {
            float angle = (f - 1) * 0.4f;
            glm::vec3 fingerDir = glm::normalize(currentDir + glm::vec3(std::sin(angle), 0, std::cos(angle)) * 0.3f);
            metaballs.addMetaball(currentPos + fingerDir * handRadius * 1.5f, handRadius * 0.3f, 0.7f);
        }
    }
}

void MorphologyBuilder::buildTail(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& basePosition)
{
    float segmentLength = genes.tailLength * genes.bodyLength / genes.tailSegments;
    float currentThickness = genes.tailThickness * genes.bodyWidth;
    glm::vec3 currentPos = basePosition;
    glm::vec3 direction(0, 0, -1);

    for (int i = 0; i < genes.tailSegments; i++) {
        glm::vec3 segmentCenter = currentPos + direction * (segmentLength * 0.5f);
        metaballs.addMetaball(segmentCenter, currentThickness, 1.0f);

        currentPos = currentPos + direction * segmentLength;
        currentThickness *= genes.tailTaper;

        // Slight droop
        direction = glm::normalize(direction + glm::vec3(0, -0.1f, 0));
    }

    // Tail tip features
    if (genes.hasCaudalFin) {
        // Fin at tail tip
        float finHeight = genes.finSize * genes.bodyHeight;
        metaballs.addMetaball(currentPos + glm::vec3(0, finHeight * 0.3f, 0), finHeight * 0.3f, 0.6f);
        metaballs.addMetaball(currentPos + glm::vec3(0, -finHeight * 0.3f, 0), finHeight * 0.3f, 0.6f);
    }
}

void MorphologyBuilder::buildWings(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& attachPoint)
{
    float halfSpan = genes.wingSpan * genes.bodyLength * 0.5f;
    float chord = genes.wingChord * genes.bodyLength;

    for (int side = 0; side < 2; side++) {
        float xDir = (side == 0) ? -1.0f : 1.0f;

        // Wing bone
        glm::vec3 wingRoot = attachPoint + glm::vec3(xDir * genes.bodyWidth * 0.4f, 0, 0);
        glm::vec3 wingTip = wingRoot + glm::vec3(xDir * halfSpan, 0.1f, 0);

        // Wing segments along leading edge
        int wingSegments = 4;
        for (int i = 0; i <= wingSegments; i++) {
            float t = static_cast<float>(i) / wingSegments;
            glm::vec3 pos = glm::mix(wingRoot, wingTip, t);
            float radius = 0.05f * (1.0f - t * 0.5f); // Thinner towards tip
            metaballs.addMetaball(pos, radius, 0.8f);

            // Wing membrane (trailing edge)
            if (i > 0 && i < wingSegments) {
                glm::vec3 membranePos = pos + glm::vec3(0, 0, -chord * (1.0f - t * 0.5f));
                metaballs.addMetaball(membranePos, radius * 0.5f, 0.5f);
            }
        }
    }
}

void MorphologyBuilder::buildFins(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& bodyCenter)
{
    float finRadius = genes.finSize * genes.bodyHeight * 0.4f;

    // Dorsal fin
    if (genes.hasDorsalFin) {
        glm::vec3 dorsalPos = bodyCenter + glm::vec3(0, genes.bodyHeight * 0.6f, 0);
        metaballs.addMetaball(dorsalPos, finRadius, 0.7f);
        metaballs.addMetaball(dorsalPos + glm::vec3(0, finRadius * 0.8f, 0), finRadius * 0.5f, 0.5f);
    }

    // Pectoral fins
    if (genes.hasPectoralFins) {
        for (int side = 0; side < 2; side++) {
            float xDir = (side == 0) ? -1.0f : 1.0f;
            glm::vec3 pectoralPos = bodyCenter + glm::vec3(
                xDir * (genes.bodyWidth * 0.5f + finRadius * 0.5f),
                0,
                genes.bodyLength * 0.2f
            );
            metaballs.addMetaball(pectoralPos, finRadius, 0.7f);
        }
    }
}

void MorphologyBuilder::buildFeature(
    MetaballSystem& metaballs,
    FeatureType feature,
    const glm::vec3& position,
    float size,
    const glm::vec3& direction)
{
    switch (feature) {
        case FeatureType::HORNS: {
            for (int side = 0; side < 2; side++) {
                float xDir = (side == 0) ? -0.3f : 0.3f;
                glm::vec3 hornBase = position + glm::vec3(xDir * size, size * 0.5f, 0);
                glm::vec3 hornTip = hornBase + glm::vec3(xDir, 1.5f, 0.3f) * size;

                int segments = 3;
                for (int i = 0; i <= segments; i++) {
                    float t = static_cast<float>(i) / segments;
                    glm::vec3 pos = glm::mix(hornBase, hornTip, t);
                    float radius = size * 0.15f * (1.0f - t * 0.7f);
                    metaballs.addMetaball(pos, radius, 0.9f);
                }
            }
            break;
        }

        case FeatureType::ANTLERS: {
            for (int side = 0; side < 2; side++) {
                float xDir = (side == 0) ? -1.0f : 1.0f;
                glm::vec3 base = position + glm::vec3(xDir * size * 0.3f, size * 0.4f, 0);

                // Main beam
                metaballs.addMetaball(base, size * 0.08f, 0.9f);
                metaballs.addMetaball(base + glm::vec3(xDir * 0.5f, 1.0f, 0) * size, size * 0.06f, 0.8f);
                metaballs.addMetaball(base + glm::vec3(xDir * 0.8f, 1.5f, 0.2f) * size, size * 0.05f, 0.7f);

                // Tines
                metaballs.addMetaball(base + glm::vec3(xDir * 0.3f, 0.8f, 0.4f) * size, size * 0.04f, 0.6f);
                metaballs.addMetaball(base + glm::vec3(xDir * 0.6f, 1.2f, -0.2f) * size, size * 0.04f, 0.6f);
            }
            break;
        }

        case FeatureType::CLAWS: {
            float clawRadius = size * 0.08f;
            for (int i = 0; i < 3; i++) {
                float offset = (i - 1) * size * 0.2f;
                glm::vec3 clawBase = position + glm::vec3(offset, -size * 0.2f, size * 0.3f);
                glm::vec3 clawTip = clawBase + direction * size * 0.4f + glm::vec3(0, -size * 0.1f, 0);
                metaballs.addMetaball(clawBase, clawRadius, 0.9f);
                metaballs.addMetaball(clawTip, clawRadius * 0.5f, 0.8f);
            }
            break;
        }

        case FeatureType::PROBOSCIS: {
            int segments = 5;
            float length = size * 2.0f;
            for (int i = 0; i <= segments; i++) {
                float t = static_cast<float>(i) / segments;
                glm::vec3 pos = position + direction * (length * t);
                float radius = size * 0.1f * (1.0f - t * 0.5f);
                metaballs.addMetaball(pos, radius, 0.8f);
            }
            break;
        }

        case FeatureType::MANDIBLES: {
            for (int side = 0; side < 2; side++) {
                float xDir = (side == 0) ? -1.0f : 1.0f;
                glm::vec3 mandibleBase = position + glm::vec3(xDir * size * 0.3f, -size * 0.2f, 0);
                glm::vec3 mandibleTip = mandibleBase + glm::vec3(xDir * 0.2f, 0, 0.6f) * size;

                metaballs.addMetaball(mandibleBase, size * 0.1f, 0.9f);
                metaballs.addMetaball(mandibleTip, size * 0.06f, 0.8f);
            }
            break;
        }

        case FeatureType::SPIKES: {
            int numSpikes = 5;
            for (int i = 0; i < numSpikes; i++) {
                float angle = (static_cast<float>(i) / numSpikes) * 3.14159f * 2.0f;
                glm::vec3 spikeDir(std::cos(angle) * 0.5f, 1.0f, std::sin(angle) * 0.5f);
                glm::vec3 spikeBase = position + spikeDir * size * 0.3f;
                glm::vec3 spikeTip = spikeBase + spikeDir * size * 0.5f;

                metaballs.addMetaball(spikeBase, size * 0.06f, 0.8f);
                metaballs.addMetaball(spikeTip, size * 0.03f, 0.7f);
            }
            break;
        }

        case FeatureType::CREST: {
            int crestSegments = 4;
            for (int i = 0; i < crestSegments; i++) {
                float t = static_cast<float>(i) / crestSegments;
                glm::vec3 pos = position + glm::vec3(0, size * (0.3f + t * 0.4f), -t * size * 0.3f);
                float radius = size * 0.1f * (1.0f - t * 0.3f);
                metaballs.addMetaball(pos, radius, 0.7f);
            }
            break;
        }

        default:
            break;
    }
}

void MorphologyBuilder::buildEyes(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& headCenter,
    float headRadius)
{
    float eyeRadius = genes.eyeSize * headRadius;
    int eyePairs = genes.eyeCount / 2;

    for (int pair = 0; pair < eyePairs; pair++) {
        float verticalOffset = (pair - (eyePairs - 1) * 0.5f) * eyeRadius * 1.5f;

        for (int side = 0; side < 2; side++) {
            float xDir = (side == 0) ? -1.0f : 1.0f;

            float xOffset, zOffset;
            if (genes.eyesSideFacing) {
                // Prey eyes - on sides
                xOffset = headRadius * 0.8f * xDir;
                zOffset = headRadius * 0.2f;
            } else {
                // Predator eyes - forward facing
                xOffset = headRadius * 0.4f * xDir;
                zOffset = headRadius * 0.7f;
            }

            glm::vec3 eyePos = headCenter + glm::vec3(xOffset, verticalOffset + headRadius * 0.2f, zOffset);
            metaballs.addMetaball(eyePos, eyeRadius, 0.9f);
        }
    }
}

glm::vec3 MorphologyBuilder::calculateLimbDirection(int limbIndex, int totalLimbs, bool isLeftSide, float spread) {
    float xDir = isLeftSide ? -1.0f : 1.0f;
    return glm::normalize(glm::vec3(xDir * spread, -1.0f, 0.0f));
}

float MorphologyBuilder::getThicknessAtSegment(float baseThickness, int segment, int totalSegments) {
    float taper = 1.0f - (static_cast<float>(segment) / totalSegments) * 0.5f;
    return baseThickness * taper;
}

// =============================================================================
// EXTENDED MORPHOLOGY BUILDERS (Phase 7)
// =============================================================================

void MorphologyBuilder::buildDorsalCrest(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& bodyCenter)
{
    if (genes.crestType == CrestType::NONE || genes.crestHeight < 0.05f) return;

    float crestStart = -genes.bodyLength * 0.5f * genes.crestExtent;
    float crestEnd = genes.bodyLength * 0.5f * genes.crestExtent;
    float crestLength = crestEnd - crestStart;

    int segments = static_cast<int>(genes.crestExtent * 8) + 2;

    for (int i = 0; i <= segments; i++) {
        float t = static_cast<float>(i) / segments;
        float z = crestStart + t * crestLength;

        // Height varies along crest (higher in middle)
        float heightMod = 1.0f - 4.0f * (t - 0.5f) * (t - 0.5f);
        float height = genes.crestHeight * genes.bodyHeight * heightMod;

        glm::vec3 crestPos = bodyCenter + glm::vec3(0, genes.bodyHeight * 0.5f + height * 0.5f, z);

        float radius;
        switch (genes.crestType) {
            case CrestType::RIDGE:
                radius = height * 0.15f;
                metaballs.addMetaball(crestPos, radius, 0.7f);
                break;

            case CrestType::SAIL:
                // Tall, thin sail fin
                radius = height * 0.08f;
                metaballs.addMetaball(crestPos, radius, 0.6f);
                // Add height extension
                metaballs.addMetaball(crestPos + glm::vec3(0, height * 0.3f, 0), radius * 0.7f, 0.5f);
                break;

            case CrestType::FRILL:
                // Wide, fan-like frill
                radius = height * 0.2f;
                metaballs.addMetaball(crestPos, radius, 0.6f);
                // Side extensions
                metaballs.addMetaball(crestPos + glm::vec3(genes.bodyWidth * 0.2f, 0, 0), radius * 0.5f, 0.4f);
                metaballs.addMetaball(crestPos + glm::vec3(-genes.bodyWidth * 0.2f, 0, 0), radius * 0.5f, 0.4f);
                break;

            case CrestType::SPINY:
                // Individual spines
                if (i % 2 == 0) {
                    radius = height * 0.1f;
                    metaballs.addMetaball(crestPos, radius, 0.8f);
                    metaballs.addMetaball(crestPos + glm::vec3(0, height * 0.4f, 0), radius * 0.4f, 0.6f);
                }
                break;

            default:
                break;
        }
    }
}

void MorphologyBuilder::buildHorns(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& headPosition,
    float headRadius)
{
    if (genes.hornCount == 0 || genes.hornLength < 0.05f) return;

    float hornLength = genes.hornLength * headRadius * 2.0f;
    int hornsPerSide = (genes.hornCount + 1) / 2;

    for (int h = 0; h < hornsPerSide; h++) {
        float verticalOffset = (h - (hornsPerSide - 1) * 0.5f) * headRadius * 0.3f;

        for (int side = 0; side < 2; side++) {
            if (side == 1 && genes.hornCount % 2 == 1 && h == hornsPerSide - 1) continue; // Odd count

            float xDir = (side == 0) ? -1.0f : 1.0f;
            glm::vec3 hornBase = headPosition + glm::vec3(xDir * headRadius * 0.4f, headRadius * 0.5f + verticalOffset, 0);

            // Build horn based on type
            int segments = 4;
            glm::vec3 currentPos = hornBase;

            for (int i = 0; i <= segments; i++) {
                float t = static_cast<float>(i) / segments;
                float radius = headRadius * 0.12f * (1.0f - t * 0.7f);

                glm::vec3 offset;
                switch (genes.hornType) {
                    case HornType::STRAIGHT:
                        offset = glm::vec3(xDir * 0.3f, 1.0f, genes.hornCurvature * 0.3f) * hornLength * t;
                        break;

                    case HornType::CURVED:
                        offset = glm::vec3(
                            xDir * (0.3f + t * 0.4f * genes.hornCurvature),
                            1.0f - t * 0.3f * std::abs(genes.hornCurvature),
                            genes.hornCurvature * t * 0.5f
                        ) * hornLength * t;
                        break;

                    case HornType::SPIRAL:
                        {
                            float angle = t * 3.14159f * 1.5f;
                            float spiralRadius = t * 0.3f * headRadius;
                            offset = glm::vec3(
                                xDir * (0.2f + std::cos(angle) * spiralRadius / hornLength),
                                1.0f,
                                std::sin(angle) * spiralRadius / hornLength + genes.hornCurvature * 0.2f
                            ) * hornLength * t;
                        }
                        break;

                    case HornType::BRANCHED:
                        offset = glm::vec3(xDir * 0.4f, 1.0f, genes.hornCurvature * 0.2f) * hornLength * t;
                        // Add branches
                        if (i == segments / 2 || i == segments) {
                            glm::vec3 branchPos = hornBase + offset;
                            glm::vec3 branchDir = glm::normalize(glm::vec3(xDir * 0.5f, 0.5f, 0.3f));
                            metaballs.addMetaball(branchPos + branchDir * hornLength * 0.2f, radius * 0.6f, 0.7f);
                            metaballs.addMetaball(branchPos + branchDir * hornLength * 0.35f, radius * 0.3f, 0.6f);
                        }
                        break;
                }

                glm::vec3 pos = hornBase + offset;
                metaballs.addMetaball(pos, radius, 0.85f);
            }
        }
    }
}

void MorphologyBuilder::buildAntennae(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& headPosition,
    float headRadius)
{
    if (genes.antennaeCount == 0 || genes.antennaeLength < 0.1f) return;

    float antennaLength = genes.antennaeLength * genes.bodyLength;
    int antennaePerSide = (genes.antennaeCount + 1) / 2;

    for (int a = 0; a < antennaePerSide; a++) {
        float forwardOffset = a * headRadius * 0.3f;

        for (int side = 0; side < 2; side++) {
            if (side == 1 && genes.antennaeCount % 2 == 1 && a == antennaePerSide - 1) continue;

            float xDir = (side == 0) ? -1.0f : 1.0f;
            glm::vec3 antennaBase = headPosition + glm::vec3(xDir * headRadius * 0.3f, headRadius * 0.4f, headRadius * 0.5f + forwardOffset);

            int segments = 6;
            for (int i = 0; i <= segments; i++) {
                float t = static_cast<float>(i) / segments;
                float radius = headRadius * 0.04f * (1.0f - t * 0.6f);

                // Antennae curve outward and slightly forward
                glm::vec3 offset(
                    xDir * t * antennaLength * 0.3f,
                    t * antennaLength * 0.8f * (1.0f - t * 0.3f),  // Droop slightly at end
                    t * antennaLength * 0.2f
                );

                metaballs.addMetaball(antennaBase + offset, radius, 0.7f);
            }
        }
    }
}

void MorphologyBuilder::buildNeckFrill(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& neckPosition)
{
    if (!genes.hasNeckFrill || genes.frillSize < 0.1f) return;

    float frillRadius = genes.frillSize * genes.bodyWidth;
    int frillSegments = 12;

    for (int i = 0; i < frillSegments; i++) {
        float angle = (static_cast<float>(i) / frillSegments) * 3.14159f * 2.0f;

        // Frill extends outward in a disc, with more coverage on sides and top
        float coverage = std::max(0.0f, std::cos(angle * 0.5f));
        if (coverage < 0.3f) continue;

        float x = std::cos(angle) * frillRadius * coverage;
        float y = std::sin(angle) * frillRadius * 0.7f * coverage;

        glm::vec3 frillPos = neckPosition + glm::vec3(x, y + frillRadius * 0.3f, -genes.bodyWidth * 0.1f);
        float radius = frillRadius * 0.15f * coverage;

        metaballs.addMetaball(frillPos, radius, 0.5f);

        // Add membrane between points
        if (i > 0 && i < frillSegments - 1) {
            glm::vec3 innerPos = neckPosition + glm::vec3(x * 0.4f, y * 0.4f + frillRadius * 0.2f, -genes.bodyWidth * 0.05f);
            metaballs.addMetaball(innerPos, radius * 0.6f, 0.4f);
        }
    }
}

void MorphologyBuilder::buildBodySpines(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& bodyCenter)
{
    if (genes.spikeRows == 0 || genes.spikeLength < 0.02f) return;

    float spineLength = genes.spikeLength * genes.bodyWidth;
    int spinesPerRow = static_cast<int>(genes.spikeDensity * 10) + 3;

    for (int row = 0; row < genes.spikeRows; row++) {
        // Position rows along body height
        float rowAngle = (static_cast<float>(row) / genes.spikeRows - 0.5f) * 3.14159f * 0.8f;
        float xOffset = std::sin(rowAngle) * genes.bodyWidth * 0.5f;
        float yOffset = std::cos(rowAngle) * genes.bodyHeight * 0.5f;

        for (int s = 0; s < spinesPerRow; s++) {
            float t = (static_cast<float>(s) / (spinesPerRow - 1)) * genes.bodyLength - genes.bodyLength * 0.5f;

            glm::vec3 spineBase = bodyCenter + glm::vec3(xOffset, yOffset, t);

            // Spine points outward from body surface
            glm::vec3 outward = glm::normalize(glm::vec3(xOffset, yOffset, 0));
            glm::vec3 spineTip = spineBase + outward * spineLength;

            // Base and tip metaballs
            float baseRadius = spineLength * 0.2f;
            metaballs.addMetaball(spineBase, baseRadius, 0.8f);
            metaballs.addMetaball((spineBase + spineTip) * 0.5f, baseRadius * 0.5f, 0.7f);
            metaballs.addMetaball(spineTip, baseRadius * 0.2f, 0.6f);
        }
    }
}

void MorphologyBuilder::buildShellArmor(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& bodyCenter)
{
    if (genes.armorCoverage < 0.1f) return;

    float shellThickness = genes.bodyHeight * 0.15f;
    int plateCount = static_cast<int>(genes.shellSegmentation * 8) + 2;

    float coverage = genes.armorCoverage;
    float startZ = -genes.bodyLength * 0.5f * coverage;
    float endZ = genes.bodyLength * 0.5f * coverage;

    for (int p = 0; p < plateCount; p++) {
        float t = static_cast<float>(p) / (plateCount - 1);
        float z = startZ + t * (endZ - startZ);

        // Shell plates cover top and sides
        float plateWidth = genes.bodyWidth * (0.6f + coverage * 0.4f);
        float plateHeight = genes.bodyHeight * 0.3f;

        glm::vec3 plateCenter = bodyCenter + glm::vec3(0, genes.bodyHeight * 0.4f, z);

        // Main plate
        float plateRadius = (plateWidth + plateHeight) * 0.25f;
        metaballs.addMetaball(plateCenter, plateRadius, 0.75f);

        // Side extensions based on coverage
        if (coverage > 0.5f) {
            metaballs.addMetaball(plateCenter + glm::vec3(plateWidth * 0.4f, 0, 0), plateRadius * 0.6f, 0.6f);
            metaballs.addMetaball(plateCenter + glm::vec3(-plateWidth * 0.4f, 0, 0), plateRadius * 0.6f, 0.6f);
        }

        // Texture bumps based on shell texture type
        if (genes.shellTextureType == 2 || genes.shellTextureType == 3) {  // Bumpy or plated
            float bumpRadius = shellThickness * 0.3f;
            metaballs.addMetaball(plateCenter + glm::vec3(0, shellThickness * 0.3f, 0), bumpRadius, 0.5f);
        }
    }
}

void MorphologyBuilder::buildEyeSpots(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& bodyCenter)
{
    if (!genes.hasEyeSpots || genes.eyeSpotCount == 0) return;

    // Eye spots are visual patterns - we add small bumps where they'd appear
    // The actual color/pattern is handled by texture generation
    float spotRadius = genes.bodyWidth * 0.08f;
    int spotsPerSide = (genes.eyeSpotCount + 1) / 2;

    for (int s = 0; s < spotsPerSide; s++) {
        float zPos = (static_cast<float>(s) / spotsPerSide - 0.5f) * genes.bodyLength * 0.6f;

        for (int side = 0; side < 2; side++) {
            if (side == 1 && genes.eyeSpotCount % 2 == 1 && s == spotsPerSide - 1) continue;

            float xDir = (side == 0) ? -1.0f : 1.0f;

            // Position on side of body
            glm::vec3 spotPos = bodyCenter + glm::vec3(
                xDir * genes.bodyWidth * 0.5f,
                genes.bodyHeight * 0.1f,
                zPos
            );

            // Subtle bump for eye spot
            metaballs.addMetaball(spotPos, spotRadius, 0.3f);
        }
    }
}

void MorphologyBuilder::buildBarbels(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& jawPosition)
{
    if (genes.barbelLength < 0.1f) return;

    float barbelLength = genes.barbelLength * genes.bodyLength * 0.3f;
    int barbelPairs = static_cast<int>(genes.barbelLength * 3) + 1;

    for (int b = 0; b < barbelPairs; b++) {
        float zOffset = b * genes.bodyWidth * 0.15f;

        for (int side = 0; side < 2; side++) {
            float xDir = (side == 0) ? -1.0f : 1.0f;

            glm::vec3 barbelBase = jawPosition + glm::vec3(xDir * genes.bodyWidth * 0.15f, -genes.bodyHeight * 0.1f, zOffset);

            int segments = 4;
            for (int i = 0; i <= segments; i++) {
                float t = static_cast<float>(i) / segments;
                float radius = genes.bodyWidth * 0.02f * (1.0f - t * 0.7f);

                // Barbels hang down and slightly outward
                glm::vec3 offset(
                    xDir * t * barbelLength * 0.3f,
                    -t * barbelLength * 0.8f,
                    t * barbelLength * 0.2f
                );

                metaballs.addMetaball(barbelBase + offset, radius, 0.7f);
            }
        }
    }
}

void MorphologyBuilder::buildTailVariant(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& tailEnd)
{
    switch (genes.tailType) {
        case TailType::CLUBBED:
            if (genes.tailBulbSize > 0.05f) {
                float clubRadius = genes.tailBulbSize * genes.bodyWidth;
                metaballs.addMetaball(tailEnd, clubRadius, 0.9f);
                // Spikes on club
                for (int i = 0; i < 4; i++) {
                    float angle = i * 3.14159f * 0.5f;
                    glm::vec3 spikeDir(std::cos(angle), std::sin(angle), 0);
                    metaballs.addMetaball(tailEnd + spikeDir * clubRadius * 0.8f, clubRadius * 0.3f, 0.7f);
                }
            }
            break;

        case TailType::FAN:
            if (genes.tailFinHeight > 0.05f) {
                float fanHeight = genes.tailFinHeight * genes.bodyHeight;
                float fanWidth = fanHeight * 1.5f;
                int fanSegments = 7;
                for (int i = 0; i < fanSegments; i++) {
                    float angle = (static_cast<float>(i) / (fanSegments - 1) - 0.5f) * 2.5f;
                    float radius = fanHeight * 0.15f;
                    glm::vec3 fanPos = tailEnd + glm::vec3(
                        std::sin(angle) * fanWidth * 0.5f,
                        std::cos(angle) * fanHeight * 0.5f,
                        -std::abs(std::sin(angle)) * genes.bodyWidth * 0.1f
                    );
                    metaballs.addMetaball(fanPos, radius, 0.6f);
                }
            }
            break;

        case TailType::FORKED:
            {
                float forkLength = genes.tailLength * genes.bodyLength * 0.3f;
                for (int side = 0; side < 2; side++) {
                    float xDir = (side == 0) ? -0.5f : 0.5f;
                    glm::vec3 forkEnd = tailEnd + glm::vec3(xDir * forkLength * 0.5f, 0, -forkLength);
                    float radius = genes.tailThickness * genes.bodyWidth * 0.3f;
                    metaballs.addMetaball((tailEnd + forkEnd) * 0.5f, radius, 0.7f);
                    metaballs.addMetaball(forkEnd, radius * 0.5f, 0.6f);
                }
            }
            break;

        case TailType::SPIKED:
            {
                int spikeCount = 3;
                float spikeLength = genes.spikeLength * genes.bodyWidth;
                for (int i = 0; i < spikeCount; i++) {
                    float zOffset = i * spikeLength * 0.8f;
                    glm::vec3 spikeBase = tailEnd + glm::vec3(0, 0, -zOffset);
                    glm::vec3 spikeDir(0, 1, -0.3f);
                    spikeDir = glm::normalize(spikeDir);
                    metaballs.addMetaball(spikeBase + spikeDir * spikeLength * 0.5f, spikeLength * 0.15f, 0.7f);
                    metaballs.addMetaball(spikeBase + spikeDir * spikeLength, spikeLength * 0.05f, 0.6f);
                }
            }
            break;

        default:
            // STANDARD, WHIP, PREHENSILE handled by base tail builder
            break;
    }
}

void MorphologyBuilder::buildMultipleFins(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& bodyCenter)
{
    float baseFinSize = genes.finSize * genes.bodyHeight * 0.4f;

    // Multiple dorsal fins
    if (genes.dorsalFinCount > 1) {
        float spacing = genes.bodyLength / (genes.dorsalFinCount + 1);
        for (int d = 0; d < genes.dorsalFinCount; d++) {
            float zPos = -genes.bodyLength * 0.5f + spacing * (d + 1);
            float finHeight = baseFinSize * (1.0f - d * 0.15f);  // Slightly smaller each time

            glm::vec3 finPos = bodyCenter + glm::vec3(0, genes.bodyHeight * 0.5f + finHeight * 0.3f, zPos);
            metaballs.addMetaball(finPos, finHeight * 0.3f, 0.6f);

            // Fin shape based on aspect ratio
            if (genes.finAspect > 1.5f) {
                // Swept fin
                metaballs.addMetaball(finPos + glm::vec3(0, finHeight * 0.4f, -finHeight * 0.2f), finHeight * 0.2f, 0.5f);
            } else if (genes.finAspect < 0.7f) {
                // Rounded fin
                metaballs.addMetaball(finPos + glm::vec3(finHeight * 0.15f, finHeight * 0.2f, 0), finHeight * 0.2f, 0.5f);
                metaballs.addMetaball(finPos + glm::vec3(-finHeight * 0.15f, finHeight * 0.2f, 0), finHeight * 0.2f, 0.5f);
            } else {
                metaballs.addMetaball(finPos + glm::vec3(0, finHeight * 0.5f, 0), finHeight * 0.15f, 0.5f);
            }
        }
    }

    // Multiple pectoral fin pairs
    if (genes.pectoralFinPairs > 1) {
        float spacing = genes.bodyLength * 0.3f / genes.pectoralFinPairs;
        for (int p = 0; p < genes.pectoralFinPairs; p++) {
            float zPos = genes.bodyLength * 0.2f - p * spacing;

            for (int side = 0; side < 2; side++) {
                float xDir = (side == 0) ? -1.0f : 1.0f;
                glm::vec3 finPos = bodyCenter + glm::vec3(
                    xDir * (genes.bodyWidth * 0.5f + baseFinSize * 0.3f),
                    0,
                    zPos
                );

                float finSize = baseFinSize * (1.0f - p * 0.2f);
                metaballs.addMetaball(finPos, finSize * 0.4f, 0.6f);

                // Fin ray detail
                if (genes.finRayDensity > 0.5f) {
                    int rays = static_cast<int>(genes.finRayDensity * 4) + 1;
                    for (int r = 0; r < rays; r++) {
                        float rayAngle = (static_cast<float>(r) / rays - 0.5f) * 1.2f;
                        glm::vec3 rayEnd = finPos + glm::vec3(xDir * finSize * std::cos(rayAngle), finSize * std::sin(rayAngle), 0);
                        metaballs.addMetaball(rayEnd, finSize * 0.1f, 0.4f);
                    }
                }
            }
        }
    }

    // Ventral fins
    for (int v = 0; v < genes.ventralFinCount; v++) {
        float zPos = -genes.bodyLength * 0.1f - v * genes.bodyWidth * 0.3f;
        glm::vec3 finPos = bodyCenter + glm::vec3(0, -genes.bodyHeight * 0.4f, zPos);
        float finSize = baseFinSize * 0.6f;
        metaballs.addMetaball(finPos, finSize * 0.3f, 0.6f);
        metaballs.addMetaball(finPos + glm::vec3(0, -finSize * 0.3f, 0), finSize * 0.15f, 0.5f);
    }
}

float MorphologyBuilder::heavyTailedValue(float baseValue, float min, float max, float extremeChance) {
    // Returns a value with occasional extreme outliers
    if (rand() / static_cast<float>(RAND_MAX) < extremeChance) {
        // Extreme value - push toward min or max
        return (rand() % 2 == 0) ? min + (max - min) * 0.1f : max - (max - min) * 0.1f;
    }
    return baseValue;
}

// =============================================================================
// LOD AND PERFORMANCE (Phase 7)
// =============================================================================

void MorphologyBuilder::buildFromMorphologyWithLOD(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    CreatureType type,
    LODLevel lod,
    const VisualState* visualState)
{
    metaballs.clear();

    if (lod == LODLevel::MINIMAL) {
        // Just a simple sphere approximation
        float avgSize = (genes.bodyLength + genes.bodyWidth + genes.bodyHeight) / 3.0f;
        metaballs.addMetaball(glm::vec3(0.0f), avgSize * 0.5f, 1.0f);
        return;
    }

    glm::vec3 center(0.0f);
    float bodyHalfLength = genes.bodyLength * 0.5f;

    // Build torso (always included)
    buildTorso(metaballs, genes, center);

    glm::vec3 frontEnd = center + glm::vec3(0, genes.bodyHeight * 0.4f, bodyHalfLength);
    glm::vec3 backEnd = center + glm::vec3(0, genes.bodyHeight * 0.4f, -bodyHalfLength);
    glm::vec3 neckEnd = frontEnd + glm::vec3(0, genes.neckLength * 0.3f, genes.neckLength);

    // Build head (always included)
    buildHead(metaballs, genes, neckEnd, visualState);

    // Build tail (skip at SIMPLIFIED)
    if (genes.hasTail && lod != LODLevel::SIMPLIFIED) {
        buildTail(metaballs, genes, backEnd);
    }

    // Build legs (reduce at lower LODs)
    if (genes.legPairs > 0) {
        int legsToRender = genes.legPairs;
        if (lod == LODLevel::SIMPLIFIED) {
            legsToRender = std::min(legsToRender, 2);
        }

        float legSpacing = genes.bodyLength / (genes.legPairs + 1);
        for (int pair = 0; pair < legsToRender; pair++) {
            float zPos = -bodyHalfLength + legSpacing * (pair + 1);
            for (int side = 0; side < 2; side++) {
                float xDir = (side == 0) ? -1.0f : 1.0f;
                glm::vec3 attachPoint = center + glm::vec3(genes.bodyWidth * 0.45f * xDir, 0, zPos);
                glm::vec3 direction = glm::normalize(glm::vec3(xDir * genes.legSpread, -1.0f, 0.0f));

                int segmentCount = genes.legSegments;
                if (lod == LODLevel::REDUCED) segmentCount = std::max(2, segmentCount - 1);
                if (lod == LODLevel::SIMPLIFIED) segmentCount = 2;

                buildLimb(metaballs, attachPoint, direction,
                         genes.legLength * genes.bodyLength,
                         genes.legThickness * genes.bodyWidth,
                         segmentCount,
                         AppendageType::LEG,
                         genes);
            }
        }
    }

    // Wings (skip at SIMPLIFIED)
    if (genes.wingPairs > 0 && lod != LODLevel::SIMPLIFIED) {
        glm::vec3 wingAttach = center + glm::vec3(0, genes.bodyHeight * 0.5f, 0);
        buildWings(metaballs, genes, wingAttach);
    }

    // Fins (skip at SIMPLIFIED, reduce at REDUCED)
    if ((genes.finCount > 0 || genes.hasDorsalFin || genes.hasPectoralFins || genes.hasCaudalFin) &&
        lod != LODLevel::SIMPLIFIED) {
        buildFins(metaballs, genes, center);
    }

    // Primary features (skip at SIMPLIFIED)
    if (genes.primaryFeature != FeatureType::NONE && lod != LODLevel::SIMPLIFIED) {
        glm::vec3 headPos = neckEnd + glm::vec3(0, 0, genes.headSize * genes.bodyWidth);
        buildFeature(metaballs, genes.primaryFeature, headPos, genes.featureSize, glm::vec3(0, 0, 1));
    }

    // Extended morphology features - only at FULL LOD
    if (lod == LODLevel::FULL) {
        glm::vec3 headPos = neckEnd + glm::vec3(0, 0, genes.headSize * genes.bodyWidth);
        float headRadius = genes.headSize * genes.bodyWidth;

        if (genes.crestType != CrestType::NONE && genes.crestHeight > 0.05f) {
            buildDorsalCrest(metaballs, genes, center);
        }

        if (genes.hornCount > 0 && genes.hornLength > 0.05f &&
            genes.primaryFeature != FeatureType::HORNS) {
            buildHorns(metaballs, genes, headPos, headRadius);
        }

        if (genes.antennaeCount > 0 && genes.antennaeLength > 0.1f) {
            buildAntennae(metaballs, genes, headPos, headRadius);
        }

        if (genes.hasNeckFrill && genes.frillSize > 0.1f) {
            buildNeckFrill(metaballs, genes, neckEnd);
        }

        if (genes.spikeRows > 0 && genes.spikeLength > 0.02f) {
            buildBodySpines(metaballs, genes, center);
        }

        if (genes.armorCoverage > 0.1f) {
            buildShellArmor(metaballs, genes, center);
        }

        if (genes.hasEyeSpots && genes.eyeSpotCount > 0) {
            buildEyeSpots(metaballs, genes, center);
        }

        if (genes.barbelLength > 0.1f) {
            glm::vec3 jawPos = headPos + glm::vec3(0, -headRadius * 0.3f, headRadius * 0.5f);
            buildBarbels(metaballs, genes, jawPos);
        }

        if (genes.tailType != TailType::STANDARD && genes.hasTail) {
            glm::vec3 tailEnd = backEnd + glm::vec3(0, 0, -genes.tailLength * genes.bodyLength);
            buildTailVariant(metaballs, genes, tailEnd);
        }

        if (genes.dorsalFinCount > 1 || genes.pectoralFinPairs > 1 || genes.ventralFinCount > 0) {
            buildMultipleFins(metaballs, genes, center);
        }

        if (genes.secondaryFeature != FeatureType::NONE) {
            glm::vec3 secondaryPos = center + glm::vec3(0, genes.bodyHeight * 0.3f, 0);
            buildFeature(metaballs, genes.secondaryFeature, secondaryPos, genes.featureSize * 0.7f, glm::vec3(0, 1, 0));
        }
    }
}

MorphologyBuilder::LODLevel MorphologyBuilder::getLODForDistance(float distance) {
    if (distance < 10.0f) return LODLevel::FULL;
    if (distance < 30.0f) return LODLevel::REDUCED;
    if (distance < 100.0f) return LODLevel::SIMPLIFIED;
    return LODLevel::MINIMAL;
}

int MorphologyBuilder::estimateVertexCount(const MorphologyGenes& genes, LODLevel lod) {
    // Base estimate per metaball (after marching cubes at default resolution)
    const int verticesPerMetaball = 120;  // Approximate based on resolution 24

    int metaballEstimate = 0;

    // Base body
    metaballEstimate += genes.segmentCount * 2;  // Torso segments

    // Head
    metaballEstimate += 4;

    // Tail
    if (genes.hasTail && lod != LODLevel::SIMPLIFIED) {
        metaballEstimate += genes.tailSegments;
    }

    // Legs
    if (lod != LODLevel::SIMPLIFIED) {
        int legSegments = genes.legSegments;
        if (lod == LODLevel::REDUCED) legSegments = std::max(2, legSegments - 1);
        metaballEstimate += genes.legPairs * 2 * (legSegments + 2);  // Segments + joints + feet
    } else {
        metaballEstimate += genes.legPairs * 2 * 3;  // Simplified legs
    }

    // Extended features (only FULL LOD)
    if (lod == LODLevel::FULL) {
        if (genes.crestHeight > 0.05f) metaballEstimate += 10;
        if (genes.hornCount > 0) metaballEstimate += genes.hornCount * 5;
        if (genes.antennaeCount > 0) metaballEstimate += genes.antennaeCount * 6;
        if (genes.hasNeckFrill) metaballEstimate += 15;
        if (genes.spikeRows > 0) metaballEstimate += genes.spikeRows * 10;
        if (genes.armorCoverage > 0.1f) metaballEstimate += 20;
    }

    // Apply LOD reduction factors
    float lodFactor = 1.0f;
    switch (lod) {
        case LODLevel::REDUCED: lodFactor = 0.7f; break;
        case LODLevel::SIMPLIFIED: lodFactor = 0.4f; break;
        case LODLevel::MINIMAL: return 100;  // Just a sphere
        default: break;
    }

    return static_cast<int>(metaballEstimate * verticesPerMetaball * lodFactor);
}

MorphologyBuilder::MorphologyStats MorphologyBuilder::validateMorphology(
    const MorphologyGenes& genes,
    CreatureType type,
    LODLevel lod)
{
    MorphologyStats stats = {};

    MetaballSystem metaballs;
    buildFromMorphologyWithLOD(metaballs, genes, type, lod, nullptr);

    stats.metaballCount = static_cast<int>(metaballs.getMetaballCount());

    // Generate mesh to get actual vertex count
    MeshData meshData = MarchingCubes::generateMesh(metaballs, 24);
    stats.vertexCount = static_cast<int>(meshData.vertices.size());

    // Calculate bounding radius
    meshData.calculateBounds();
    glm::vec3 center = (meshData.boundsMin + meshData.boundsMax) * 0.5f;
    stats.boundingRadius = glm::length(meshData.boundsMax - center);

    // Count features
    stats.featureCount = 0;
    if (genes.crestHeight > 0.05f) stats.featureCount++;
    if (genes.hornCount > 0) stats.featureCount++;
    if (genes.antennaeCount > 0) stats.featureCount++;
    if (genes.hasNeckFrill) stats.featureCount++;
    if (genes.spikeRows > 0) stats.featureCount++;
    if (genes.armorCoverage > 0.1f) stats.featureCount++;
    if (genes.primaryFeature != FeatureType::NONE) stats.featureCount++;
    if (genes.secondaryFeature != FeatureType::NONE) stats.featureCount++;

    return stats;
}

void MorphologyBuilder::validateRandomCreatures(
    int count,
    std::vector<MorphologyStats>& outStats)
{
    outStats.clear();
    outStats.reserve(count);

    for (int i = 0; i < count; i++) {
        MorphologyGenes genes;
        genes.randomize();

        // Randomly select creature type
        CreatureType types[] = {
            CreatureType::HERBIVORE,
            CreatureType::SMALL_PREDATOR,
            CreatureType::GRAZER,
            CreatureType::APEX_PREDATOR,
            CreatureType::AQUATIC,
            CreatureType::AMPHIBIAN
        };
        CreatureType type = types[rand() % 6];

        MorphologyStats stats = validateMorphology(genes, type, LODLevel::FULL);
        outStats.push_back(stats);
    }
}

// =============================================================================
// CREATURE MESH GENERATOR IMPLEMENTATION
// =============================================================================

CreatureMeshGenerator::GeneratedMesh CreatureMeshGenerator::generate(
    const MorphologyGenes& genes,
    CreatureType type,
    int resolution)
{
    MetaballSystem metaballs;
    MorphologyBuilder::buildFromMorphology(metaballs, genes, type);

    // Generate mesh using marching cubes (static method)
    MeshData meshData = MarchingCubes::generateMesh(metaballs, resolution);

    GeneratedMesh result;

    // Convert Vertex structs to flat float array (8 floats per vertex)
    result.vertices.reserve(meshData.vertices.size() * 8);
    for (const auto& v : meshData.vertices) {
        result.vertices.push_back(v.position.x);
        result.vertices.push_back(v.position.y);
        result.vertices.push_back(v.position.z);
        result.vertices.push_back(v.normal.x);
        result.vertices.push_back(v.normal.y);
        result.vertices.push_back(v.normal.z);
        result.vertices.push_back(v.texCoord.x);
        result.vertices.push_back(v.texCoord.y);
    }
    result.indices = std::move(meshData.indices);

    // Use precalculated bounds from MeshData
    meshData.calculateBounds();
    result.center = (meshData.boundsMin + meshData.boundsMax) * 0.5f;
    result.boundingRadius = glm::length(meshData.boundsMax - result.center);

    return result;
}

CreatureMeshGenerator::GeneratedMesh CreatureMeshGenerator::generateWithState(
    const MorphologyGenes& genes,
    CreatureType type,
    const VisualState& state,
    int resolution)
{
    MetaballSystem metaballs;
    MorphologyBuilder::buildFromMorphology(metaballs, genes, type, &state);

    // Apply posture modifications
    applyPostureToMetaballs(metaballs, state);

    // Generate mesh using marching cubes (static method)
    MeshData meshData = MarchingCubes::generateMesh(metaballs, resolution);

    GeneratedMesh result;

    // Convert Vertex structs to flat float array (8 floats per vertex)
    result.vertices.reserve(meshData.vertices.size() * 8);
    for (const auto& v : meshData.vertices) {
        result.vertices.push_back(v.position.x);
        result.vertices.push_back(v.position.y);
        result.vertices.push_back(v.position.z);
        result.vertices.push_back(v.normal.x);
        result.vertices.push_back(v.normal.y);
        result.vertices.push_back(v.normal.z);
        result.vertices.push_back(v.texCoord.x);
        result.vertices.push_back(v.texCoord.y);
    }
    result.indices = std::move(meshData.indices);

    // Use precalculated bounds from MeshData
    meshData.calculateBounds();
    result.center = (meshData.boundsMin + meshData.boundsMax) * 0.5f;
    result.boundingRadius = glm::length(meshData.boundsMax - result.center);

    return result;
}

void CreatureMeshGenerator::applyPostureToMetaballs(
    MetaballSystem& metaballs,
    const VisualState& state)
{
    // This would modify metaball positions based on posture
    // For now, the posture is handled during the building phase
    // A more advanced implementation would store metaball references
    // and modify them here

    // Apply crouch by scaling Y
    if (state.crouchFactor > 0.01f) {
        // Would need access to metaball positions to modify
    }

    // Apply slump by rotating front sections down
    if (state.postureSlump > 0.01f) {
        // Would need segmented access to metaballs
    }
}

// =============================================================================
// FAMILY ARCHETYPE SYSTEM (Phase 8)
// =============================================================================

FamilyArchetype MorphologyBuilder::determineArchetype(uint32_t speciesId, uint32_t planetSeed) {
    // Deterministic archetype selection based on species ID and planet seed
    // Uses a hash to ensure consistent results across saves
    uint32_t combined = speciesId ^ (planetSeed * 0x9E3779B9);  // Golden ratio hash

    // Weight distribution for archetypes (some more common than others)
    // SEGMENTED: 12%, PLATED: 12%, FINNED: 15%, LONG_LIMBED: 12%
    // RADIAL: 10%, BURROWING: 13%, GLIDING: 11%, SPINED: 15%
    static const int weights[] = {12, 12, 15, 12, 10, 13, 11, 15};  // Must sum to 100

    int roll = static_cast<int>(combined % 100);
    int cumulative = 0;

    for (int i = 0; i < static_cast<int>(FamilyArchetype::Count); i++) {
        cumulative += weights[i];
        if (roll < cumulative) {
            return static_cast<FamilyArchetype>(i);
        }
    }

    return FamilyArchetype::SEGMENTED;  // Fallback
}

const ArchetypeConstraints& MorphologyBuilder::getArchetypeConstraints(FamilyArchetype archetype) {
    int index = static_cast<int>(archetype);
    if (index < 0 || index >= static_cast<int>(FamilyArchetype::Count)) {
        index = 0;
    }
    return s_archetypeConstraints[index];
}

void MorphologyBuilder::applyArchetypeToMorphology(
    MorphologyGenes& genes,
    FamilyArchetype archetype,
    uint32_t speciesId)
{
    const ArchetypeConstraints& c = getArchetypeConstraints(archetype);

    // Use species ID to generate deterministic variation within constraints
    auto rangeValue = [speciesId](float min, float max, int offset) -> float {
        float t = fmod((speciesId * 0.618033988749895f + offset * 0.1f), 1.0f);
        return min + t * (max - min);
    };

    auto rangeInt = [speciesId](int min, int max, int offset) -> int {
        if (min >= max) return min;
        int range = max - min + 1;
        return min + static_cast<int>((speciesId + offset * 137) % range);
    };

    auto probability = [speciesId](float prob, int offset) -> bool {
        float roll = fmod((speciesId * 0.618033988749895f + offset * 0.31f), 1.0f);
        return roll < prob;
    };

    // Apply body proportions
    genes.bodyAspect = rangeValue(c.minBodyAspect, c.maxBodyAspect, 0);
    genes.bodyWidth = rangeValue(c.minBodyWidth, c.maxBodyWidth, 1);
    genes.bodyHeight = rangeValue(c.minBodyHeight, c.maxBodyHeight, 2);
    genes.bodyLength = genes.bodyWidth * genes.bodyAspect;

    // Apply segmentation
    genes.segmentCount = rangeInt(c.minSegments, c.maxSegments, 3);
    genes.segmentTaper = rangeValue(c.minSegmentTaper, c.maxSegmentTaper, 4);

    // Apply limbs
    genes.legPairs = rangeInt(c.minLegPairs, c.maxLegPairs, 5);
    genes.legSegments = rangeInt(c.minLegSegments, c.maxLegSegments, 6);
    genes.legLength = rangeValue(c.minLegLength, c.maxLegLength, 7);
    genes.legThickness = rangeValue(c.minLegThickness, c.maxLegThickness, 8);

    // Apply fins (probability-based)
    if (probability(c.finProbability, 9)) {
        genes.hasDorsalFin = true;
        genes.finSize = rangeValue(c.minFinSize, c.maxFinSize, 10);
        genes.dorsalFinCount = rangeInt(c.minDorsalFins, c.maxDorsalFins, 11);

        // Finned archetype gets additional fin types
        if (archetype == FamilyArchetype::FINNED) {
            genes.hasPectoralFins = true;
            genes.hasCaudalFin = true;
            genes.pectoralFinPairs = rangeInt(1, 2, 12);
        }
    }

    // Apply armor (probability-based)
    if (probability(c.armorProbability, 13)) {
        genes.armorCoverage = rangeValue(c.minArmorCoverage, c.maxArmorCoverage, 14);
        genes.shellSegmentation = rangeValue(0.3f, 0.8f, 15);
        genes.shellTextureType = rangeInt(0, 3, 16);
    }

    // Apply spines (probability-based)
    if (probability(c.spineProbability, 17)) {
        genes.spikeRows = rangeInt(c.minSpikeRows, c.maxSpikeRows, 18);
        genes.spikeLength = rangeValue(c.minSpikeLength, c.maxSpikeLength, 19);
        genes.spikeDensity = rangeValue(0.3f, 0.8f, 20);
    }

    // Apply crest (probability-based)
    if (probability(c.crestProbability, 21)) {
        genes.crestType = static_cast<CrestType>(rangeInt(1, 4, 22));
        genes.crestHeight = rangeValue(0.1f, 0.4f, 23);
        genes.crestExtent = rangeValue(0.4f, 0.9f, 24);
    }

    // Apply horns (probability-based)
    if (probability(c.hornProbability, 25)) {
        genes.hornCount = rangeInt(1, 4, 26);
        genes.hornLength = rangeValue(0.1f, 0.5f, 27);
        genes.hornCurvature = rangeValue(-0.5f, 0.5f, 28);
        genes.hornType = static_cast<HornType>(rangeInt(0, 3, 29));
    }

    // Apply antennae (probability-based)
    if (probability(c.antennaeProbability, 30)) {
        genes.antennaeCount = rangeInt(1, 4, 31);
        genes.antennaeLength = rangeValue(0.2f, 0.6f, 32);
    }

    // Archetype-specific special adjustments
    switch (archetype) {
        case FamilyArchetype::RADIAL:
            // Radial creatures use symmetry differently
            genes.symmetry = SymmetryType::RADIAL;
            genes.legPairs = 0;  // No legs, uses radial arms
            break;

        case FamilyArchetype::GLIDING:
            // Gliders always have wing-like structures
            genes.wingPairs = 1;
            genes.wingSpan = rangeValue(1.5f, 2.5f, 33);
            genes.wingChord = rangeValue(0.3f, 0.6f, 34);
            genes.canFly = false;  // Gliding, not flying
            break;

        case FamilyArchetype::BURROWING:
            // Burrowers have small eyes and big claws
            genes.eyeSize = rangeValue(0.02f, 0.05f, 35);  // Small eyes
            genes.hasClaws = true;
            genes.clawSize = rangeValue(0.15f, 0.3f, 36);  // Big claws
            break;

        case FamilyArchetype::FINNED:
            // Aquatic-optimized
            genes.hasTail = true;
            genes.tailType = TailType::FAN;
            genes.tailLength = rangeValue(0.4f, 0.7f, 37);
            break;

        default:
            break;
    }
}

void MorphologyBuilder::buildArchetypeFeatures(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    FamilyArchetype archetype,
    const glm::vec3& bodyCenter)
{
    switch (archetype) {
        case FamilyArchetype::RADIAL:
            // Build radial arms and tendrils
            {
                int armCount = 5 + (genes.segmentCount % 4);  // 5-8 arms
                buildRadialArms(metaballs, genes, bodyCenter, armCount);

                // Add tendrils if marked for it
                if (genes.antennaeCount > 0) {
                    float tendrilLength = genes.antennaeLength * genes.bodyLength;
                    buildTendrils(metaballs, genes, bodyCenter, genes.antennaeCount * 2, tendrilLength);
                }
            }
            break;

        case FamilyArchetype::GLIDING:
            // Build membrane flaps
            buildMembraneFlaps(metaballs, genes, bodyCenter);
            break;

        case FamilyArchetype::BURROWING:
            // Add heavy digging claws to front limbs
            if (genes.legPairs > 0) {
                float clawSize = genes.clawSize * genes.bodyWidth;
                glm::vec3 frontLegEnd = bodyCenter + glm::vec3(genes.bodyWidth * 0.6f, -genes.legLength * genes.bodyLength, genes.bodyLength * 0.3f);
                buildDiggingClaws(metaballs, genes, frontLegEnd, clawSize);
                frontLegEnd.x = -frontLegEnd.x;  // Other side
                buildDiggingClaws(metaballs, genes, frontLegEnd, clawSize);
            }
            break;

        case FamilyArchetype::PLATED:
            // Build articulated armor plates
            if (genes.armorCoverage > 0.3f) {
                buildArticulatedPlates(metaballs, genes, bodyCenter);
            }
            break;

        case FamilyArchetype::SPINED:
            // Build dense spine coverage
            if (genes.spikeRows > 0) {
                buildDenseSpines(metaballs, genes, bodyCenter);
            }
            break;

        default:
            // Other archetypes use standard morphology features
            break;
    }
}

uint8_t MorphologyBuilder::getArchetypePreferredPattern(FamilyArchetype archetype, uint32_t speciesId) {
    const ArchetypeConstraints& c = getArchetypeConstraints(archetype);
    if (c.numPreferredPatterns == 0) return 0;

    int index = static_cast<int>(speciesId % c.numPreferredPatterns);
    return c.preferredPatterns[index];
}

// =============================================================================
// PHASE 8 GEOMETRY MODULES
// =============================================================================

void MorphologyBuilder::buildTendrils(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& bodyCenter,
    int tendrilCount,
    float tendrilLength)
{
    if (tendrilCount == 0 || tendrilLength < 0.1f) return;

    // Tendrils hang from underside of body
    glm::vec3 tendrilBase = bodyCenter + glm::vec3(0, -genes.bodyHeight * 0.4f, 0);

    for (int t = 0; t < tendrilCount; t++) {
        // Distribute around center
        float angle = (static_cast<float>(t) / tendrilCount) * 6.28318f;
        float xOffset = std::cos(angle) * genes.bodyWidth * 0.3f;
        float zOffset = std::sin(angle) * genes.bodyLength * 0.3f;

        glm::vec3 startPos = tendrilBase + glm::vec3(xOffset, 0, zOffset);

        // Build tendril segments
        int segments = 6 + (t % 3);  // 6-8 segments
        float segmentLength = tendrilLength / segments;
        float thickness = genes.bodyWidth * 0.05f;

        glm::vec3 currentPos = startPos;
        glm::vec3 direction(0, -1, 0);

        for (int s = 0; s < segments; s++) {
            float t_ratio = static_cast<float>(s) / segments;
            float segRadius = thickness * (1.0f - t_ratio * 0.7f);  // Taper

            glm::vec3 segCenter = currentPos + direction * (segmentLength * 0.5f);
            metaballs.addMetaball(segCenter, segRadius, 0.6f);

            currentPos = currentPos + direction * segmentLength;

            // Tendrils curl slightly outward and wave
            float waveAmount = 0.2f + t_ratio * 0.3f;
            direction = glm::normalize(direction + glm::vec3(
                std::cos(angle + s * 0.5f) * waveAmount,
                -0.8f,
                std::sin(angle + s * 0.5f) * waveAmount
            ));
        }
    }
}

void MorphologyBuilder::buildMembraneFlaps(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& bodyCenter)
{
    // Membrane flaps stretch between front and back legs
    float flapWidth = genes.wingSpan * genes.bodyLength * 0.3f;
    float flapLength = genes.bodyLength * 0.8f;

    for (int side = 0; side < 2; side++) {
        float xDir = (side == 0) ? -1.0f : 1.0f;

        // Front attachment point (shoulder area)
        glm::vec3 frontAttach = bodyCenter + glm::vec3(
            xDir * genes.bodyWidth * 0.45f,
            genes.bodyHeight * 0.2f,
            genes.bodyLength * 0.3f
        );

        // Back attachment point (hip area)
        glm::vec3 backAttach = bodyCenter + glm::vec3(
            xDir * genes.bodyWidth * 0.45f,
            genes.bodyHeight * 0.1f,
            -genes.bodyLength * 0.35f
        );

        // Outer edge (when extended)
        glm::vec3 outerFront = frontAttach + glm::vec3(xDir * flapWidth, 0, 0);
        glm::vec3 outerBack = backAttach + glm::vec3(xDir * flapWidth * 0.7f, -flapWidth * 0.1f, 0);

        // Build membrane with thin metaballs
        int rows = 5;
        int cols = 8;
        float membraneThickness = 0.02f;

        for (int r = 0; r < rows; r++) {
            float rowT = static_cast<float>(r) / (rows - 1);
            glm::vec3 rowStart = glm::mix(frontAttach, backAttach, rowT);
            glm::vec3 rowEnd = glm::mix(outerFront, outerBack, rowT);

            for (int c = 0; c < cols; c++) {
                float colT = static_cast<float>(c) / (cols - 1);
                glm::vec3 pos = glm::mix(rowStart, rowEnd, colT);

                // Membrane curves down slightly at edges
                float edgeDroop = colT * colT * 0.1f * flapWidth;
                pos.y -= edgeDroop;

                float radius = membraneThickness * (1.0f - colT * 0.3f);
                metaballs.addMetaball(pos, radius, 0.3f);  // Low weight for thin membrane
            }
        }
    }
}

void MorphologyBuilder::buildRadialArms(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& bodyCenter,
    int armCount)
{
    if (armCount < 3) armCount = 5;  // Default to 5-armed

    float armLength = genes.bodyWidth * 1.5f;
    float baseThickness = genes.bodyWidth * 0.15f;

    for (int a = 0; a < armCount; a++) {
        float angle = (static_cast<float>(a) / armCount) * 6.28318f;

        glm::vec3 armDir(std::cos(angle), 0, std::sin(angle));
        glm::vec3 armBase = bodyCenter + armDir * genes.bodyWidth * 0.4f;

        // Build arm segments
        int segments = 5;
        float segmentLength = armLength / segments;
        glm::vec3 currentPos = armBase;

        for (int s = 0; s < segments; s++) {
            float t = static_cast<float>(s) / segments;
            float thickness = baseThickness * (1.0f - t * 0.6f);  // Taper

            glm::vec3 segCenter = currentPos + armDir * (segmentLength * 0.5f);
            metaballs.addMetaball(segCenter, thickness, 0.85f);

            // Joint bulge
            if (s < segments - 1) {
                glm::vec3 jointPos = currentPos + armDir * segmentLength;
                metaballs.addMetaball(jointPos, thickness * 0.8f, 0.6f);
            }

            currentPos = currentPos + armDir * segmentLength;

            // Arms can curve slightly upward at tips
            armDir = glm::normalize(armDir + glm::vec3(0, 0.1f, 0));
        }

        // Add small sucker-like bumps along underside
        int suckerCount = 3;
        for (int sc = 0; sc < suckerCount; sc++) {
            float suckerT = 0.3f + (static_cast<float>(sc) / suckerCount) * 0.5f;
            glm::vec3 suckerPos = armBase + armDir * (armLength * suckerT);
            suckerPos.y -= baseThickness * (1.0f - suckerT * 0.5f);
            metaballs.addMetaball(suckerPos, baseThickness * 0.2f, 0.4f);
        }
    }
}

void MorphologyBuilder::buildDiggingClaws(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& limbEnd,
    float clawSize)
{
    // Large, curved digging claws
    int clawCount = 3;

    for (int c = 0; c < clawCount; c++) {
        float spreadAngle = (c - 1) * 0.4f;  // -0.4, 0, 0.4 radians

        glm::vec3 clawDir = glm::normalize(glm::vec3(
            std::sin(spreadAngle) * 0.3f,
            -0.5f,  // Angled downward
            0.8f    // Forward-facing for digging
        ));

        // Build curved claw
        int segments = 4;
        glm::vec3 currentPos = limbEnd;
        float segmentLength = clawSize / segments;
        float thickness = clawSize * 0.2f;

        for (int s = 0; s < segments; s++) {
            float t = static_cast<float>(s) / segments;
            float segThickness = thickness * (1.0f - t * 0.7f);  // Sharp taper

            glm::vec3 segCenter = currentPos + clawDir * (segmentLength * 0.5f);
            metaballs.addMetaball(segCenter, segThickness, 0.9f);

            currentPos = currentPos + clawDir * segmentLength;

            // Curve downward
            clawDir = glm::normalize(clawDir + glm::vec3(0, -0.2f, 0.1f));
        }
    }
}

void MorphologyBuilder::buildArticulatedPlates(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& bodyCenter)
{
    // Overlapping armor plates like an armadillo
    int plateRows = static_cast<int>(genes.shellSegmentation * 8) + 4;
    float plateLength = genes.bodyLength / plateRows;
    float plateThickness = genes.bodyHeight * 0.1f;

    for (int row = 0; row < plateRows; row++) {
        float rowT = static_cast<float>(row) / (plateRows - 1);
        float zPos = -genes.bodyLength * 0.5f + rowT * genes.bodyLength;

        // Plate width varies - wider in middle
        float widthMod = 1.0f - std::abs(rowT - 0.5f) * 0.4f;
        float plateWidth = genes.bodyWidth * widthMod;

        // Main plate
        glm::vec3 plateCenter = bodyCenter + glm::vec3(0, genes.bodyHeight * 0.4f, zPos);
        metaballs.addMetaball(plateCenter, plateWidth * 0.4f, 0.8f);

        // Side extensions
        metaballs.addMetaball(plateCenter + glm::vec3(plateWidth * 0.35f, -plateThickness * 0.5f, 0),
                             plateWidth * 0.25f, 0.6f);
        metaballs.addMetaball(plateCenter + glm::vec3(-plateWidth * 0.35f, -plateThickness * 0.5f, 0),
                             plateWidth * 0.25f, 0.6f);

        // Raised ridge on each plate
        if (row % 2 == 0) {
            metaballs.addMetaball(plateCenter + glm::vec3(0, plateThickness * 0.5f, 0),
                                 plateWidth * 0.1f, 0.5f);
        }
    }
}

void MorphologyBuilder::buildDenseSpines(
    MetaballSystem& metaballs,
    const MorphologyGenes& genes,
    const glm::vec3& bodyCenter)
{
    // Dense spine coverage for spined archetype
    int totalSpines = genes.spikeRows * static_cast<int>(genes.spikeDensity * 15);
    float spineLength = genes.spikeLength * genes.bodyWidth;
    float spineThickness = spineLength * 0.15f;

    for (int s = 0; s < totalSpines; s++) {
        // Distribute spines over dorsal surface
        float phi = (static_cast<float>(s) / totalSpines) * 6.28318f;  // Around body
        float theta = (static_cast<float>(s % genes.spikeRows) / genes.spikeRows) * 3.14159f * 0.5f;  // Up surface

        // Position on body surface (top and sides, not belly)
        float xPos = std::sin(theta) * std::cos(phi) * genes.bodyWidth * 0.5f;
        float yPos = std::cos(theta) * genes.bodyHeight * 0.4f;
        float zPos = (static_cast<float>(s % 10) / 10.0f - 0.5f) * genes.bodyLength * 0.8f;

        // Skip belly area
        if (yPos < -genes.bodyHeight * 0.1f) continue;

        glm::vec3 spineBase = bodyCenter + glm::vec3(xPos, yPos, zPos);

        // Spine points outward from surface
        glm::vec3 outward = glm::normalize(glm::vec3(xPos, yPos + genes.bodyHeight * 0.2f, 0));
        glm::vec3 spineTip = spineBase + outward * spineLength;

        // Build spine
        metaballs.addMetaball(spineBase, spineThickness, 0.8f);
        metaballs.addMetaball((spineBase + spineTip) * 0.5f, spineThickness * 0.5f, 0.7f);
        metaballs.addMetaball(spineTip, spineThickness * 0.15f, 0.6f);
    }
}

// =============================================================================
// DEBUG REPORTING AND VALIDATION (Phase 8)
// =============================================================================

void MorphologyBuilder::generateDebugReport(
    int creatureCount,
    uint32_t planetSeed,
    std::string& outReport)
{
    std::ostringstream ss;
    ss << "=== MORPHOLOGY DIVERSITY DEBUG REPORT ===\n";
    ss << "Planet Seed: " << planetSeed << "\n";
    ss << "Creatures Generated: " << creatureCount << "\n\n";

    ss << std::left << std::setw(6) << "ID"
       << std::setw(14) << "Archetype"
       << std::setw(10) << "Vertices"
       << std::setw(12) << "Metaballs"
       << std::setw(10) << "Features"
       << std::setw(12) << "Budget OK"
       << "\n";
    ss << std::string(64, '-') << "\n";

    int archetypeCounts[static_cast<int>(FamilyArchetype::Count)] = {0};
    int overBudgetCount = 0;
    int totalVertices = 0;

    for (int i = 0; i < creatureCount; i++) {
        uint32_t speciesId = static_cast<uint32_t>(i * 137 + 42);  // Deterministic variation

        FamilyArchetype archetype = determineArchetype(speciesId, planetSeed);
        archetypeCounts[static_cast<int>(archetype)]++;

        // Generate morphology with archetype
        MorphologyGenes genes;
        genes.randomize();
        applyArchetypeToMorphology(genes, archetype, speciesId);

        // Validate
        MorphologyStats stats = validateMorphology(genes, CreatureType::HERBIVORE, LODLevel::FULL);
        stats.archetype = archetype;
        stats.withinVertexBudget = (stats.vertexCount <= VERTEX_BUDGET_FULL);

        totalVertices += stats.vertexCount;
        if (!stats.withinVertexBudget) overBudgetCount++;

        ss << std::left << std::setw(6) << speciesId
           << std::setw(14) << getArchetypeName(archetype)
           << std::setw(10) << stats.vertexCount
           << std::setw(12) << stats.metaballCount
           << std::setw(10) << stats.featureCount
           << std::setw(12) << (stats.withinVertexBudget ? "YES" : "NO")
           << "\n";
    }

    ss << "\n=== ARCHETYPE DISTRIBUTION ===\n";
    for (int i = 0; i < static_cast<int>(FamilyArchetype::Count); i++) {
        FamilyArchetype arch = static_cast<FamilyArchetype>(i);
        float percentage = (static_cast<float>(archetypeCounts[i]) / creatureCount) * 100.0f;
        ss << std::left << std::setw(14) << getArchetypeName(arch)
           << ": " << archetypeCounts[i] << " (" << std::fixed << std::setprecision(1) << percentage << "%)\n";
    }

    ss << "\n=== PERFORMANCE SUMMARY ===\n";
    ss << "Average Vertices: " << (totalVertices / creatureCount) << "\n";
    ss << "Over Budget Count: " << overBudgetCount << " ("
       << std::fixed << std::setprecision(1) << (static_cast<float>(overBudgetCount) / creatureCount * 100.0f) << "%)\n";
    ss << "Vertex Budget (FULL): " << VERTEX_BUDGET_FULL << "\n";

    outReport = ss.str();
}

void MorphologyBuilder::validateArchetypeDistribution(
    int creatureCount,
    uint32_t planetSeed,
    std::string& outReport)
{
    std::ostringstream ss;
    ss << "=== ARCHETYPE DISTRIBUTION VALIDATION ===\n\n";

    int archetypeCounts[static_cast<int>(FamilyArchetype::Count)] = {0};

    for (int i = 0; i < creatureCount; i++) {
        uint32_t speciesId = static_cast<uint32_t>(i);
        FamilyArchetype archetype = determineArchetype(speciesId, planetSeed);
        archetypeCounts[static_cast<int>(archetype)]++;
    }

    ss << "Sample Size: " << creatureCount << "\n";
    ss << "Planet Seed: " << planetSeed << "\n\n";

    ss << std::left << std::setw(14) << "Archetype"
       << std::setw(8) << "Count"
       << std::setw(12) << "Percentage"
       << "Distribution\n";
    ss << std::string(60, '-') << "\n";

    for (int i = 0; i < static_cast<int>(FamilyArchetype::Count); i++) {
        FamilyArchetype arch = static_cast<FamilyArchetype>(i);
        float percentage = (static_cast<float>(archetypeCounts[i]) / creatureCount) * 100.0f;
        int barLength = static_cast<int>(percentage / 2);  // 2% per character

        ss << std::left << std::setw(14) << getArchetypeName(arch)
           << std::setw(8) << archetypeCounts[i]
           << std::fixed << std::setprecision(1) << std::setw(11) << percentage << "%"
           << " " << std::string(barLength, '#') << "\n";
    }

    // Check for good distribution (no archetype < 5% or > 25%)
    ss << "\n=== DISTRIBUTION QUALITY ===\n";
    bool goodDistribution = true;
    for (int i = 0; i < static_cast<int>(FamilyArchetype::Count); i++) {
        float percentage = (static_cast<float>(archetypeCounts[i]) / creatureCount) * 100.0f;
        if (percentage < 5.0f) {
            ss << "WARNING: " << getArchetypeName(static_cast<FamilyArchetype>(i))
               << " is underrepresented (" << percentage << "%)\n";
            goodDistribution = false;
        }
        if (percentage > 25.0f) {
            ss << "WARNING: " << getArchetypeName(static_cast<FamilyArchetype>(i))
               << " is overrepresented (" << percentage << "%)\n";
            goodDistribution = false;
        }
    }

    if (goodDistribution) {
        ss << "Distribution is well-balanced across all archetypes.\n";
    }

    outReport = ss.str();
}
