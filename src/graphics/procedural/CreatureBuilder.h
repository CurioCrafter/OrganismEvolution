#pragma once

#include "../../entities/Genome.h"
#include "../../entities/CreatureType.h"
#include "MetaballSystem.h"
#include <glm/glm.hpp>

// Body plan archetypes for more variety
enum class BodyPlan {
    QUADRUPED,      // 4 legs (default herbivore)
    BIPED,          // 2 legs (fast predator)
    HEXAPOD,        // 6 legs (insect-like)
    SERPENTINE,     // No legs, long body
    AVIAN,          // 2 legs, wings (flying type)
    FISH            // Streamlined aquatic body
};

// Body shape modifiers for enhanced visual diversity (Task 3)
enum class BodyShapeModifier {
    NORMAL,         // Standard proportions
    ELONGATED,      // Long, stretched body (eel-like)
    COMPACT,        // Short, stocky body (toad-like)
    SPINY,          // Covered in spines/quills
    SLEEK,          // Streamlined, aerodynamic
    BULKY           // Heavy, thick-set
};

// Head shape variants
enum class HeadShape {
    ROUND,          // Herbivore default
    ELONGATED,      // Predator default
    FLAT,           // Fish/reptile like
    HORNED,         // With horns/antlers
    CRESTED         // With head crest/frill
};

// Tail variants
enum class TailType {
    NONE,
    SHORT,
    LONG,
    BUSHY,          // Squirrel-like
    SPIKED,         // Club tail
    FINNED          // Fish/swimming tail
};

class CreatureBuilder {
public:
    // Generate metaball configuration from genome
    static void buildCreatureMetaballs(
        MetaballSystem& metaballs,
        const Genome& genome,
        CreatureType type
    );

    // Determine body plan from genome
    static BodyPlan determineBodyPlan(const Genome& genome, CreatureType type);
    static HeadShape determineHeadShape(const Genome& genome, CreatureType type);
    static TailType determineTailType(const Genome& genome, CreatureType type);
    static int determineLimbCount(const Genome& genome, BodyPlan plan);
    static BodyShapeModifier determineBodyShape(const Genome& genome, CreatureType type);

    // Get limb proportion multiplier based on genome (0.5x to 2.0x)
    static float getLimbLengthMultiplier(const Genome& genome);
    static float getLimbThicknessMultiplier(const Genome& genome);

    // Get unique feature flags based on genome
    static bool hasCrests(const Genome& genome, CreatureType type);
    static bool hasFins(const Genome& genome, CreatureType type);
    static bool hasTailFin(const Genome& genome, CreatureType type);
    static bool hasHorns(const Genome& genome, CreatureType type);
    static bool hasAntennae(const Genome& genome, CreatureType type);

private:
    // Build herbivore body
    static void buildHerbivore(
        MetaballSystem& metaballs,
        const Genome& genome
    );

    // Build carnivore body
    static void buildCarnivore(
        MetaballSystem& metaballs,
        const Genome& genome
    );

    // Build aquatic body (fish)
    static void buildAquatic(
        MetaballSystem& metaballs,
        const Genome& genome
    );

    // Build flying creature body (bird-like)
    static void buildFlying(
        MetaballSystem& metaballs,
        const Genome& genome
    );

    // Helper functions for body parts
    static void addHead(
        MetaballSystem& metaballs,
        const glm::vec3& position,
        float size,
        HeadShape shape
    );

    static void addTorso(
        MetaballSystem& metaballs,
        const glm::vec3& position,
        float size,
        float aspectRatio,
        int segments = 0
    );

    static void addSpine(
        MetaballSystem& metaballs,
        float size,
        int segments,
        float curvature
    );

    static void addTail(
        MetaballSystem& metaballs,
        const glm::vec3& basePosition,
        float size,
        float length,
        TailType type
    );

    static void addLimb(
        MetaballSystem& metaballs,
        const glm::vec3& attachPoint,
        const glm::vec3& direction,
        float thickness,
        float length,
        int joints = 2
    );

    static void addWings(
        MetaballSystem& metaballs,
        const glm::vec3& attachPoint,
        float size,
        float span
    );

    static void addFins(
        MetaballSystem& metaballs,
        const glm::vec3& attachPoint,
        float size,
        bool dorsal
    );

    static void addEyes(
        MetaballSystem& metaballs,
        const glm::vec3& position,
        float size,
        bool sideFacing,
        int eyeCount = 2
    );

    static void addHorns(
        MetaballSystem& metaballs,
        const glm::vec3& position,
        float size,
        int count = 2
    );

    static void addAntlers(
        MetaballSystem& metaballs,
        const glm::vec3& position,
        float size,
        int branches
    );

    static void addCrest(
        MetaballSystem& metaballs,
        const glm::vec3& position,
        float size
    );

    static void addSpikes(
        MetaballSystem& metaballs,
        const glm::vec3& basePosition,
        float size,
        int count
    );

    static void addEars(
        MetaballSystem& metaballs,
        const glm::vec3& position,
        float size,
        bool pointed
    );

    static void addMandibles(
        MetaballSystem& metaballs,
        const glm::vec3& position,
        float size
    );

    static void addAntennae(
        MetaballSystem& metaballs,
        const glm::vec3& position,
        float size,
        float length
    );

    static void addClaws(
        MetaballSystem& metaballs,
        const glm::vec3& position,
        float size
    );
};
