#include "CreatureBuilder.h"
#include "../../utils/Random.h"
#include <cmath>

// Use stable random based on genome for consistent creature appearance
static float genomeRandom(const Genome& genome, int index) {
    // Generate deterministic value from genome weights
    float val = genome.neuralWeights[index % genome.neuralWeights.size()];
    val = fmod(val * 12.9898f + genome.size * 78.233f + genome.speed * 43.758f, 1.0f);
    return std::abs(val);
}

BodyPlan CreatureBuilder::determineBodyPlan(const Genome& genome, CreatureType type) {
    float selector = genomeRandom(genome, 0) + genomeRandom(genome, 1) * 0.5f;
    selector = fmod(selector, 1.0f);

    if (type == CreatureType::HERBIVORE) {
        // Herbivores: wider variety to avoid uniform silhouettes
        if (selector < 0.45f) return BodyPlan::QUADRUPED;
        if (selector < 0.65f) return BodyPlan::HEXAPOD;
        if (selector < 0.80f) return BodyPlan::BIPED;
        if (selector < 0.90f) return BodyPlan::SERPENTINE;
        return BodyPlan::AVIAN;
    } else {
        // Predators: more variety
        if (selector < 0.30f) return BodyPlan::QUADRUPED;
        if (selector < 0.52f) return BodyPlan::BIPED;
        if (selector < 0.70f) return BodyPlan::HEXAPOD;
        if (selector < 0.85f) return BodyPlan::SERPENTINE;
        return BodyPlan::AVIAN;
    }
}

HeadShape CreatureBuilder::determineHeadShape(const Genome& genome, CreatureType type) {
    float selector = genomeRandom(genome, 2);

    if (type == CreatureType::HERBIVORE) {
        if (selector < 0.40f) return HeadShape::ROUND;
        if (selector < 0.60f) return HeadShape::FLAT;
        if (selector < 0.80f) return HeadShape::HORNED;
        return HeadShape::CRESTED;
    } else {
        if (selector < 0.45f) return HeadShape::ELONGATED;
        if (selector < 0.65f) return HeadShape::FLAT;
        if (selector < 0.85f) return HeadShape::HORNED;
        return HeadShape::CRESTED;
    }
}

TailType CreatureBuilder::determineTailType(const Genome& genome, CreatureType type) {
    float selector = genomeRandom(genome, 3);

    if (selector < 0.15f) return TailType::NONE;
    if (selector < 0.35f) return TailType::SHORT;
    if (selector < 0.55f) return TailType::LONG;
    if (selector < 0.70f) return TailType::BUSHY;
    if (selector < 0.85f) return TailType::SPIKED;
    return TailType::FINNED;
}

int CreatureBuilder::determineLimbCount(const Genome& genome, BodyPlan plan) {
    switch (plan) {
        case BodyPlan::QUADRUPED: return 4;
        case BodyPlan::BIPED: return 2;
        case BodyPlan::HEXAPOD: return 6;
        case BodyPlan::SERPENTINE: return 0;
        case BodyPlan::AVIAN: return 2;
        default: return 4;
    }
}

// =============================================================================
// ENHANCED VISUAL DIVERSITY METHODS (Task 3)
// =============================================================================

BodyShapeModifier CreatureBuilder::determineBodyShape(const Genome& genome, CreatureType type) {
    // Use genome traits to determine body shape for visual variety
    float selector = genomeRandom(genome, 20) + genomeRandom(genome, 21) * 0.5f;
    selector = fmod(selector, 1.0f);

    // Speed-based bias
    if (genome.speed > 15.0f) {
        // Fast creatures tend to be sleek
        if (selector < 0.4f) return BodyShapeModifier::SLEEK;
        if (selector < 0.6f) return BodyShapeModifier::ELONGATED;
    }

    // Size-based bias
    if (genome.size > 1.5f) {
        // Large creatures tend to be bulky
        if (selector < 0.35f) return BodyShapeModifier::BULKY;
        if (selector < 0.55f) return BodyShapeModifier::COMPACT;
    } else if (genome.size < 0.7f) {
        // Small creatures have more variety
        if (selector < 0.25f) return BodyShapeModifier::COMPACT;
        if (selector < 0.45f) return BodyShapeModifier::SPINY;
    }

    // Type-based biases
    if (type == CreatureType::AQUATIC || type == CreatureType::AQUATIC_PREDATOR) {
        if (selector < 0.5f) return BodyShapeModifier::SLEEK;
        if (selector < 0.75f) return BodyShapeModifier::ELONGATED;
    }

    // General distribution for other cases
    if (selector < 0.25f) return BodyShapeModifier::ELONGATED;
    if (selector < 0.40f) return BodyShapeModifier::COMPACT;
    if (selector < 0.55f) return BodyShapeModifier::SPINY;
    if (selector < 0.70f) return BodyShapeModifier::SLEEK;
    if (selector < 0.85f) return BodyShapeModifier::BULKY;
    return BodyShapeModifier::NORMAL;
}

float CreatureBuilder::getLimbLengthMultiplier(const Genome& genome) {
    // Generate limb length multiplier from 0.5x to 2.0x (dramatic variation)
    // Use genome traits for deterministic but varied output
    float baseMultiplier = 1.0f;

    // Speed influences leg length (faster = longer legs generally)
    float speedInfluence = (genome.speed - 10.0f) / 10.0f;  // -0.5 to +1.0
    baseMultiplier += speedInfluence * 0.4f;

    // Size affects proportions inversely (small creatures may have relatively longer legs)
    float sizeInfluence = (1.0f - genome.size) * 0.3f;  // Small size = longer relative legs
    baseMultiplier += sizeInfluence;

    // Add some random variation based on genome
    float randomVar = (genomeRandom(genome, 25) - 0.5f) * 0.6f;  // -0.3 to +0.3
    baseMultiplier += randomVar;

    // Clamp to 0.5 - 2.0 range for dramatic but reasonable variation
    return std::clamp(baseMultiplier, 0.5f, 2.0f);
}

float CreatureBuilder::getLimbThicknessMultiplier(const Genome& genome) {
    // Generate limb thickness multiplier from 0.5x to 2.0x
    float baseMultiplier = 1.0f;

    // Size influences thickness (larger = thicker)
    float sizeInfluence = (genome.size - 1.0f) * 0.5f;
    baseMultiplier += sizeInfluence;

    // Efficiency might indicate muscle mass
    float efficiencyInfluence = (genome.efficiency - 1.0f) * 0.3f;
    baseMultiplier += efficiencyInfluence;

    // Random variation
    float randomVar = (genomeRandom(genome, 26) - 0.5f) * 0.5f;
    baseMultiplier += randomVar;

    return std::clamp(baseMultiplier, 0.5f, 2.0f);
}

bool CreatureBuilder::hasCrests(const Genome& genome, CreatureType type) {
    // Flying creatures and some ground creatures can have crests
    float chance = genomeRandom(genome, 30);

    if (isFlying(type)) return chance > 0.5f;
    if (type == CreatureType::APEX_PREDATOR) return chance > 0.6f;
    return chance > 0.75f;
}

bool CreatureBuilder::hasFins(const Genome& genome, CreatureType type) {
    if (isAquatic(type)) return true;  // Aquatic always have fins

    // Some land creatures can have dorsal fins/sails
    float chance = genomeRandom(genome, 31);
    if (type == CreatureType::APEX_PREDATOR && genome.size > 1.3f) return chance > 0.7f;
    return chance > 0.9f;
}

bool CreatureBuilder::hasTailFin(const Genome& genome, CreatureType type) {
    if (isAquatic(type)) return true;

    float chance = genomeRandom(genome, 32);
    return chance > 0.8f;
}

bool CreatureBuilder::hasHorns(const Genome& genome, CreatureType type) {
    float chance = genomeRandom(genome, 33);

    // Herbivores more likely to have defensive horns
    if (::isHerbivore(type)) return chance > 0.4f;
    // Predators less likely but still possible
    if (::isPredator(type)) return chance > 0.7f;
    return chance > 0.6f;
}

bool CreatureBuilder::hasAntennae(const Genome& genome, CreatureType type) {
    float chance = genomeRandom(genome, 34);

    // Insects always have antennae
    if (type == CreatureType::FLYING_INSECT) return true;
    // Small creatures more likely
    if (genome.size < 0.7f) return chance > 0.5f;
    return chance > 0.85f;
}

void CreatureBuilder::buildCreatureMetaballs(
    MetaballSystem& metaballs,
    const Genome& genome,
    CreatureType type
) {
    metaballs.clear();

    if (type == CreatureType::HERBIVORE) {
        buildHerbivore(metaballs, genome);
    } else if (type == CreatureType::AQUATIC) {
        buildAquatic(metaballs, genome);
    } else if (type == CreatureType::FLYING) {
        buildFlying(metaballs, genome);
    } else {
        buildCarnivore(metaballs, genome);
    }
}

void CreatureBuilder::buildHerbivore(
    MetaballSystem& metaballs,
    const Genome& genome
) {
    float size = genome.size;
    BodyPlan bodyPlan = determineBodyPlan(genome, CreatureType::HERBIVORE);
    HeadShape headShape = determineHeadShape(genome, CreatureType::HERBIVORE);
    TailType tailType = determineTailType(genome, CreatureType::HERBIVORE);
    BodyShapeModifier bodyShape = determineBodyShape(genome, CreatureType::HERBIVORE);

    // Get dramatic limb proportion multipliers (0.5x to 2.0x)
    float limbLengthMult = getLimbLengthMultiplier(genome);
    float limbThickMult = getLimbThicknessMultiplier(genome);

    // Body aspect ratio based on body plan, genome, AND body shape modifier
    float bodyAspect = 1.0f;
    int bodySegments = 1;

    // Apply body shape modifier effects
    float bodyWidthMod = 1.0f;
    float bodyHeightMod = 1.0f;
    switch (bodyShape) {
        case BodyShapeModifier::ELONGATED:
            bodyAspect *= 1.5f;
            bodyWidthMod = 0.8f;
            break;
        case BodyShapeModifier::COMPACT:
            bodyAspect *= 0.7f;
            bodyWidthMod = 1.3f;
            bodyHeightMod = 0.85f;
            break;
        case BodyShapeModifier::SLEEK:
            bodyAspect *= 1.2f;
            bodyWidthMod = 0.85f;
            bodyHeightMod = 0.9f;
            break;
        case BodyShapeModifier::BULKY:
            bodyWidthMod = 1.4f;
            bodyHeightMod = 1.2f;
            break;
        case BodyShapeModifier::SPINY:
            // Spiny creatures handled later with spike additions
            break;
        default:
            break;
    }

    switch (bodyPlan) {
        case BodyPlan::QUADRUPED:
            bodyAspect = 1.0f + genome.efficiency * 0.2f;
            bodySegments = 2;
            break;
        case BodyPlan::BIPED:
            bodyAspect = 0.95f + genome.speed * 0.01f;
            bodySegments = 2;
            break;
        case BodyPlan::HEXAPOD:
            bodyAspect = 1.5f;
            bodySegments = 3;
            break;
        case BodyPlan::SERPENTINE:
            bodyAspect = 2.5f;
            bodySegments = 6 + (int)(genomeRandom(genome, 5) * 4);
            break;
        case BodyPlan::AVIAN:
            bodyAspect = 1.2f;
            bodySegments = 2;
            break;
        default:
            break;
    }

    // Main torso
    if (bodyPlan == BodyPlan::SERPENTINE) {
        addSpine(metaballs, size, bodySegments, genomeRandom(genome, 4) * 0.5f);
    } else {
        addTorso(metaballs, glm::vec3(0.0f), size, bodyAspect, bodySegments);
    }

    // Head
    float headSize = size * (0.5f + genomeRandom(genome, 6) * 0.3f);
    glm::vec3 headPos = glm::vec3(size * bodyAspect * 0.5f, size * 0.2f, 0.0f);
    addHead(metaballs, headPos, headSize, headShape);

    // Eyes based on vision range (more vision = bigger/more eyes)
    float eyeSize = size * 0.1f + genome.visionRange * 0.003f;
    int eyeCount = (genomeRandom(genome, 7) > 0.8f && bodyPlan == BodyPlan::HEXAPOD) ? 4 : 2;
    bool sideFacing = (bodyPlan != BodyPlan::AVIAN);
    addEyes(metaballs, headPos + glm::vec3(headSize * 0.3f, headSize * 0.2f, headSize * 0.4f),
            eyeSize, sideFacing, eyeCount);

    // Head features based on head shape
    if (headShape == HeadShape::HORNED) {
        int hornCount = (genomeRandom(genome, 8) > 0.5f) ? 2 : 4;
        addHorns(metaballs, headPos + glm::vec3(0.0f, headSize * 0.4f, 0.0f), size * 0.15f, hornCount);
    } else if (headShape == HeadShape::CRESTED) {
        addCrest(metaballs, headPos + glm::vec3(-headSize * 0.2f, headSize * 0.5f, 0.0f), size * 0.25f);
    }

    // Ears for mammal-like creatures
    if (bodyPlan == BodyPlan::QUADRUPED && genomeRandom(genome, 9) > 0.4f) {
        bool pointed = genomeRandom(genome, 10) > 0.5f;
        addEars(metaballs, headPos + glm::vec3(-headSize * 0.1f, headSize * 0.4f, headSize * 0.35f),
                size * 0.15f, pointed);
    }

    // Antennae for hexapods
    if (bodyPlan == BodyPlan::HEXAPOD && genomeRandom(genome, 11) > 0.3f) {
        addAntennae(metaballs, headPos + glm::vec3(headSize * 0.3f, headSize * 0.3f, 0.0f),
                    size * 0.05f, size * (0.4f + genomeRandom(genome, 12) * 0.4f));
    }

    // Tail
    if (tailType != TailType::NONE) {
        float tailLength = size * (0.5f + genomeRandom(genome, 13) * 0.8f);
        glm::vec3 tailBase = glm::vec3(-size * bodyAspect * 0.5f, 0.0f, 0.0f);
        addTail(metaballs, tailBase, size * 0.3f, tailLength, tailType);
    }

    // Limbs based on body plan - NOW WITH DRAMATIC VARIATION (0.5x to 2.0x)
    int limbCount = determineLimbCount(genome, bodyPlan);
    float legLength = size * (0.6f + genome.speed / 30.0f) * limbLengthMult;
    float legThickness = size * 0.2f * limbThickMult;

    if (bodyPlan == BodyPlan::QUADRUPED) {
        // 4 legs spread out
        float frontX = size * 0.25f;
        float backX = -size * 0.3f;
        float legSpread = size * 0.3f;

        addLimb(metaballs, glm::vec3(frontX, -size * 0.35f, legSpread),
                glm::vec3(0.1f, -1.0f, 0.2f), legThickness, legLength, 2);
        addLimb(metaballs, glm::vec3(frontX, -size * 0.35f, -legSpread),
                glm::vec3(0.1f, -1.0f, -0.2f), legThickness, legLength, 2);
        addLimb(metaballs, glm::vec3(backX, -size * 0.35f, legSpread),
                glm::vec3(-0.1f, -1.0f, 0.2f), legThickness * 1.1f, legLength * 1.05f, 2);
        addLimb(metaballs, glm::vec3(backX, -size * 0.35f, -legSpread),
                glm::vec3(-0.1f, -1.0f, -0.2f), legThickness * 1.1f, legLength * 1.05f, 2);

    } else if (bodyPlan == BodyPlan::BIPED) {
        // 2 legs with optional small forelimbs
        addLimb(metaballs, glm::vec3(-size * 0.1f, -size * 0.35f, size * 0.2f),
                glm::vec3(0.0f, -1.0f, 0.15f), legThickness * 1.1f, legLength * 1.2f, 3);
        addLimb(metaballs, glm::vec3(-size * 0.1f, -size * 0.35f, -size * 0.2f),
                glm::vec3(0.0f, -1.0f, -0.15f), legThickness * 1.1f, legLength * 1.2f, 3);

        if (genomeRandom(genome, 17) > 0.6f) {
            addLimb(metaballs, glm::vec3(size * 0.2f, -size * 0.05f, size * 0.25f),
                    glm::vec3(0.2f, -0.4f, 0.4f), legThickness * 0.6f, legLength * 0.5f, 2);
            addLimb(metaballs, glm::vec3(size * 0.2f, -size * 0.05f, -size * 0.25f),
                    glm::vec3(0.2f, -0.4f, -0.4f), legThickness * 0.6f, legLength * 0.5f, 2);
        }

    } else if (bodyPlan == BodyPlan::HEXAPOD) {
        // 6 legs (3 pairs)
        float legSpread = size * 0.35f;
        float spacing = size * 0.35f;

        for (int i = 0; i < 3; i++) {
            float xPos = size * 0.3f - i * spacing;
            float thickness = legThickness * (1.0f - i * 0.1f);

            addLimb(metaballs, glm::vec3(xPos, -size * 0.3f, legSpread),
                    glm::vec3(0.0f, -1.0f, 0.3f), thickness, legLength * 0.9f, 3);
            addLimb(metaballs, glm::vec3(xPos, -size * 0.3f, -legSpread),
                    glm::vec3(0.0f, -1.0f, -0.3f), thickness, legLength * 0.9f, 3);
        }

    } else if (bodyPlan == BodyPlan::AVIAN) {
        // 2 legs + wings
        addLimb(metaballs, glm::vec3(-size * 0.1f, -size * 0.4f, size * 0.2f),
                glm::vec3(0.0f, -1.0f, 0.15f), legThickness, legLength * 1.3f, 3);
        addLimb(metaballs, glm::vec3(-size * 0.1f, -size * 0.4f, -size * 0.2f),
                glm::vec3(0.0f, -1.0f, -0.15f), legThickness, legLength * 1.3f, 3);

        float wingSpan = size * (1.2f + genomeRandom(genome, 14) * 0.8f);
        addWings(metaballs, glm::vec3(0.0f, size * 0.1f, size * 0.35f), size * 0.15f, wingSpan);
    }

    // Defensive spikes for larger herbivores OR spiny body shape
    if ((size > 1.4f && genomeRandom(genome, 15) > 0.5f) || bodyShape == BodyShapeModifier::SPINY) {
        int spikeCount = 3 + (int)(genomeRandom(genome, 16) * 5);
        addSpikes(metaballs, glm::vec3(-size * 0.3f, size * 0.35f, 0.0f), size * 0.15f, spikeCount);

        // Spiny creatures get additional spikes along their body
        if (bodyShape == BodyShapeModifier::SPINY) {
            addSpikes(metaballs, glm::vec3(0.0f, size * 0.4f, 0.0f), size * 0.12f, spikeCount - 1);
            addSpikes(metaballs, glm::vec3(size * 0.2f, size * 0.35f, 0.0f), size * 0.1f, spikeCount - 2);
        }
    }

    // Add unique features based on genome-determined traits
    if (hasCrests(genome, CreatureType::HERBIVORE) && headShape != HeadShape::CRESTED) {
        // Add a small crest even if head shape didn't specify one
        float headSize = size * (0.5f + genomeRandom(genome, 6) * 0.3f);
        glm::vec3 headPos = glm::vec3(size * bodyAspect * 0.5f, size * 0.2f, 0.0f);
        addCrest(metaballs, headPos + glm::vec3(-headSize * 0.2f, headSize * 0.4f, 0.0f), size * 0.15f);
    }

    if (hasFins(genome, CreatureType::HERBIVORE)) {
        // Add a dorsal fin/sail
        addFins(metaballs, glm::vec3(0.0f, size * 0.3f, 0.0f), size * 0.3f, true);
    }
}

void CreatureBuilder::buildCarnivore(
    MetaballSystem& metaballs,
    const Genome& genome
) {
    float size = genome.size;
    BodyPlan bodyPlan = determineBodyPlan(genome, CreatureType::CARNIVORE);
    HeadShape headShape = determineHeadShape(genome, CreatureType::CARNIVORE);
    TailType tailType = determineTailType(genome, CreatureType::CARNIVORE);
    BodyShapeModifier bodyShape = determineBodyShape(genome, CreatureType::CARNIVORE);

    // Get dramatic limb proportion multipliers (0.5x to 2.0x)
    float limbLengthMult = getLimbLengthMultiplier(genome);
    float limbThickMult = getLimbThicknessMultiplier(genome);

    // Predators are more streamlined
    float bodyAspect = 1.3f;
    int bodySegments = 2;

    // Apply body shape modifier effects for carnivores
    switch (bodyShape) {
        case BodyShapeModifier::ELONGATED:
            bodyAspect *= 1.4f;
            break;
        case BodyShapeModifier::COMPACT:
            bodyAspect *= 0.75f;
            break;
        case BodyShapeModifier::SLEEK:
            bodyAspect *= 1.25f;
            break;
        case BodyShapeModifier::BULKY:
            bodyAspect *= 0.9f;
            break;
        default:
            break;
    }

    switch (bodyPlan) {
        case BodyPlan::QUADRUPED:
            bodyAspect *= 1.0f + genome.speed * 0.015f;
            bodySegments = 2;
            break;
        case BodyPlan::BIPED:
            bodyAspect *= 0.77f;
            bodySegments = 2;
            break;
        case BodyPlan::HEXAPOD:
            bodyAspect *= 1.23f;
            bodySegments = 3;
            break;
        case BodyPlan::SERPENTINE:
            bodyAspect = 3.0f;  // Serpentine is always long
            bodySegments = 8 + (int)(genomeRandom(genome, 5) * 5);
            break;
        case BodyPlan::AVIAN:
            bodyAspect *= 0.85f;
            bodySegments = 2;
            break;
        default:
            break;
    }

    // Torso
    if (bodyPlan == BodyPlan::SERPENTINE) {
        addSpine(metaballs, size, bodySegments, genomeRandom(genome, 4) * 0.6f);
    } else {
        addTorso(metaballs, glm::vec3(0.0f), size, bodyAspect, bodySegments);
    }

    // Head (predators have larger, more prominent heads)
    float headSize = size * (0.55f + genomeRandom(genome, 6) * 0.25f);
    glm::vec3 headPos = glm::vec3(size * bodyAspect * 0.55f, size * 0.15f, 0.0f);
    addHead(metaballs, headPos, headSize, headShape);

    // Forward-facing eyes for depth perception
    float eyeSize = size * 0.08f + genome.visionRange * 0.002f;
    int eyeCount = (genomeRandom(genome, 7) > 0.85f) ? 4 : 2;
    addEyes(metaballs, headPos + glm::vec3(headSize * 0.35f, headSize * 0.15f, headSize * 0.3f),
            eyeSize, false, eyeCount);

    // Head features
    if (headShape == HeadShape::HORNED) {
        addHorns(metaballs, headPos + glm::vec3(0.0f, headSize * 0.35f, 0.0f), size * 0.2f, 2);
    } else if (headShape == HeadShape::CRESTED) {
        addCrest(metaballs, headPos + glm::vec3(-headSize * 0.3f, headSize * 0.45f, 0.0f), size * 0.3f);
    }

    // Mandibles for hexapod predators
    if (bodyPlan == BodyPlan::HEXAPOD && genomeRandom(genome, 8) > 0.4f) {
        addMandibles(metaballs, headPos + glm::vec3(headSize * 0.5f, -headSize * 0.2f, 0.0f), size * 0.12f);
    }

    // Pointed ears for mammal-like predators
    if ((bodyPlan == BodyPlan::QUADRUPED || bodyPlan == BodyPlan::BIPED) && genomeRandom(genome, 9) > 0.5f) {
        addEars(metaballs, headPos + glm::vec3(-headSize * 0.15f, headSize * 0.45f, headSize * 0.3f),
                size * 0.18f, true);
    }

    // Tail (predators often have long tails for balance)
    if (tailType == TailType::NONE && genome.speed > 12.0f) {
        tailType = TailType::LONG;  // Fast predators need balance
    }

    if (tailType != TailType::NONE) {
        float tailLength = size * (0.7f + genomeRandom(genome, 13) * 0.6f);
        glm::vec3 tailBase = glm::vec3(-size * bodyAspect * 0.5f, 0.0f, 0.0f);
        addTail(metaballs, tailBase, size * 0.25f, tailLength, tailType);
    }

    // Limbs - NOW WITH DRAMATIC VARIATION (0.5x to 2.0x)
    float legLength = size * (0.8f + genome.speed / 25.0f) * limbLengthMult;
    float legThickness = size * 0.18f * limbThickMult;

    if (bodyPlan == BodyPlan::QUADRUPED) {
        // 4 powerful legs
        float frontX = size * 0.3f;
        float backX = -size * 0.35f;
        float legSpread = size * 0.25f;

        addLimb(metaballs, glm::vec3(frontX, -size * 0.3f, legSpread),
                glm::vec3(0.15f, -1.0f, 0.15f), legThickness, legLength, 2);
        addLimb(metaballs, glm::vec3(frontX, -size * 0.3f, -legSpread),
                glm::vec3(0.15f, -1.0f, -0.15f), legThickness, legLength, 2);
        // Back legs are more powerful
        addLimb(metaballs, glm::vec3(backX, -size * 0.3f, legSpread),
                glm::vec3(-0.1f, -1.0f, 0.2f), legThickness * 1.3f, legLength * 1.15f, 2);
        addLimb(metaballs, glm::vec3(backX, -size * 0.3f, -legSpread),
                glm::vec3(-0.1f, -1.0f, -0.2f), legThickness * 1.3f, legLength * 1.15f, 2);

        // Claws on front legs
        if (genomeRandom(genome, 17) > 0.5f) {
            addClaws(metaballs, glm::vec3(frontX + legLength * 0.1f, -legLength * 0.9f, legSpread * 1.2f),
                     size * 0.1f);
            addClaws(metaballs, glm::vec3(frontX + legLength * 0.1f, -legLength * 0.9f, -legSpread * 1.2f),
                     size * 0.1f);
        }

    } else if (bodyPlan == BodyPlan::BIPED) {
        // 2 powerful back legs (T-Rex style)
        addLimb(metaballs, glm::vec3(-size * 0.15f, -size * 0.35f, size * 0.25f),
                glm::vec3(0.0f, -1.0f, 0.2f), legThickness * 1.5f, legLength * 1.4f, 3);
        addLimb(metaballs, glm::vec3(-size * 0.15f, -size * 0.35f, -size * 0.25f),
                glm::vec3(0.0f, -1.0f, -0.2f), legThickness * 1.5f, legLength * 1.4f, 3);

        // Small arms
        addLimb(metaballs, glm::vec3(size * 0.2f, 0.0f, size * 0.3f),
                glm::vec3(0.3f, -0.3f, 0.5f), legThickness * 0.6f, legLength * 0.4f, 2);
        addLimb(metaballs, glm::vec3(size * 0.2f, 0.0f, -size * 0.3f),
                glm::vec3(0.3f, -0.3f, -0.5f), legThickness * 0.6f, legLength * 0.4f, 2);

    } else if (bodyPlan == BodyPlan::HEXAPOD) {
        // 6 legs with front pair being larger (pincers)
        float legSpread = size * 0.4f;
        float spacing = size * 0.4f;

        // Front "pincers"
        addLimb(metaballs, glm::vec3(size * 0.4f, -size * 0.2f, legSpread),
                glm::vec3(0.4f, -0.5f, 0.5f), legThickness * 1.3f, legLength * 0.8f, 2);
        addLimb(metaballs, glm::vec3(size * 0.4f, -size * 0.2f, -legSpread),
                glm::vec3(0.4f, -0.5f, -0.5f), legThickness * 1.3f, legLength * 0.8f, 2);

        // Walking legs
        for (int i = 1; i < 3; i++) {
            float xPos = size * 0.3f - i * spacing;
            addLimb(metaballs, glm::vec3(xPos, -size * 0.3f, legSpread),
                    glm::vec3(0.0f, -1.0f, 0.35f), legThickness, legLength * 0.85f, 3);
            addLimb(metaballs, glm::vec3(xPos, -size * 0.3f, -legSpread),
                    glm::vec3(0.0f, -1.0f, -0.35f), legThickness, legLength * 0.85f, 3);
        }

    } else if (bodyPlan == BodyPlan::AVIAN) {
        // Bird-like predator with powerful talons
        addLimb(metaballs, glm::vec3(-size * 0.1f, -size * 0.4f, size * 0.2f),
                glm::vec3(0.0f, -1.0f, 0.1f), legThickness * 1.2f, legLength * 1.2f, 3);
        addLimb(metaballs, glm::vec3(-size * 0.1f, -size * 0.4f, -size * 0.2f),
                glm::vec3(0.0f, -1.0f, -0.1f), legThickness * 1.2f, legLength * 1.2f, 3);

        // Wings
        float wingSpan = size * (1.5f + genomeRandom(genome, 14) * 1.0f);
        addWings(metaballs, glm::vec3(0.0f, size * 0.15f, size * 0.35f), size * 0.12f, wingSpan);

        // Talons
        addClaws(metaballs, glm::vec3(-size * 0.1f, -legLength * 1.1f, size * 0.25f), size * 0.12f);
        addClaws(metaballs, glm::vec3(-size * 0.1f, -legLength * 1.1f, -size * 0.25f), size * 0.12f);
    }
}

void CreatureBuilder::buildAquatic(
    MetaballSystem& metaballs,
    const Genome& genome
) {
    float size = genome.size;
    // Derive fin and tail scales from genome properties
    float finScale = 0.8f + genome.efficiency * 0.4f;  // Based on efficiency
    float tailScale = 0.9f + genome.speed * 0.02f;     // Based on speed

    // Streamlined fish body - elongated ellipsoid
    // Main body (3 overlapping metaballs for torpedo shape)
    metaballs.addMetaball(glm::vec3(0.0f, 0.0f, 0.0f), size * 0.4f, 1.0f);           // Center body
    metaballs.addMetaball(glm::vec3(size * 0.25f, 0.0f, 0.0f), size * 0.35f, 0.95f); // Front body
    metaballs.addMetaball(glm::vec3(-size * 0.2f, 0.0f, 0.0f), size * 0.32f, 0.85f); // Rear body

    // Head (pointed, streamlined)
    metaballs.addMetaball(glm::vec3(size * 0.45f, 0.0f, 0.0f), size * 0.22f, 0.75f); // Head
    metaballs.addMetaball(glm::vec3(size * 0.58f, 0.0f, 0.0f), size * 0.12f, 0.5f);  // Snout

    // Tail section (narrows toward caudal fin)
    metaballs.addMetaball(glm::vec3(-size * 0.4f, 0.0f, 0.0f), size * 0.2f, 0.6f);   // Tail base
    metaballs.addMetaball(glm::vec3(-size * 0.55f, 0.0f, 0.0f), size * 0.12f, 0.45f); // Tail narrow

    // Caudal fin (tail fin - forked)
    float tailFin = tailScale * size;
    metaballs.addMetaball(glm::vec3(-size * 0.7f, tailFin * 0.15f, 0.0f), tailFin * 0.12f, 0.35f);  // Top fork
    metaballs.addMetaball(glm::vec3(-size * 0.7f, -tailFin * 0.15f, 0.0f), tailFin * 0.12f, 0.35f); // Bottom fork
    metaballs.addMetaball(glm::vec3(-size * 0.65f, 0.0f, 0.0f), tailFin * 0.08f, 0.3f); // Center connector

    // Dorsal fin (top)
    float dorsalFin = finScale * size;
    metaballs.addMetaball(glm::vec3(0.0f, size * 0.25f + dorsalFin * 0.1f, 0.0f), dorsalFin * 0.15f, 0.45f);
    metaballs.addMetaball(glm::vec3(-size * 0.1f, size * 0.2f + dorsalFin * 0.15f, 0.0f), dorsalFin * 0.12f, 0.4f);

    // Pectoral fins (side fins)
    float pectoralFin = finScale * size * 0.8f;
    metaballs.addMetaball(glm::vec3(size * 0.15f, -size * 0.08f, size * 0.18f + pectoralFin * 0.1f), pectoralFin * 0.1f, 0.35f);
    metaballs.addMetaball(glm::vec3(size * 0.15f, -size * 0.08f, -size * 0.18f - pectoralFin * 0.1f), pectoralFin * 0.1f, 0.35f);

    // Pelvic fins (lower side fins, smaller)
    float pelvicFin = finScale * size * 0.5f;
    metaballs.addMetaball(glm::vec3(-size * 0.05f, -size * 0.15f, size * 0.1f), pelvicFin * 0.08f, 0.3f);
    metaballs.addMetaball(glm::vec3(-size * 0.05f, -size * 0.15f, -size * 0.1f), pelvicFin * 0.08f, 0.3f);

    // Anal fin (bottom rear)
    metaballs.addMetaball(glm::vec3(-size * 0.25f, -size * 0.12f, 0.0f), finScale * size * 0.08f, 0.3f);

    // Eyes (slightly protruding, side-facing for fish)
    float eyeSize = size * 0.06f;
    metaballs.addMetaball(glm::vec3(size * 0.4f, size * 0.05f, size * 0.12f), eyeSize, 0.6f);
    metaballs.addMetaball(glm::vec3(size * 0.4f, size * 0.05f, -size * 0.12f), eyeSize, 0.6f);

    // Gill covers (operculum) - slight bulge
    metaballs.addMetaball(glm::vec3(size * 0.25f, 0.0f, size * 0.15f), size * 0.1f, 0.4f);
    metaballs.addMetaball(glm::vec3(size * 0.25f, 0.0f, -size * 0.15f), size * 0.1f, 0.4f);
}

void CreatureBuilder::addHead(
    MetaballSystem& metaballs,
    const glm::vec3& position,
    float size,
    HeadShape shape
) {
    switch (shape) {
        case HeadShape::ROUND:
            metaballs.addMetaball(position, size, 1.0f);
            metaballs.addMetaball(position + glm::vec3(size * 0.35f, -size * 0.15f, 0.0f), size * 0.5f, 0.8f);
            break;

        case HeadShape::ELONGATED:
            metaballs.addMetaball(position, size * 0.9f, 1.0f);
            metaballs.addMetaball(position + glm::vec3(size * 0.5f, -size * 0.1f, 0.0f), size * 0.55f, 0.9f);
            metaballs.addMetaball(position + glm::vec3(size * 0.85f, -size * 0.15f, 0.0f), size * 0.35f, 0.7f);
            break;

        case HeadShape::FLAT:
            metaballs.addMetaball(position, size * 0.7f, 1.0f);
            metaballs.addMetaball(position + glm::vec3(size * 0.3f, -size * 0.2f, size * 0.2f), size * 0.4f, 0.7f);
            metaballs.addMetaball(position + glm::vec3(size * 0.3f, -size * 0.2f, -size * 0.2f), size * 0.4f, 0.7f);
            break;

        case HeadShape::HORNED:
        case HeadShape::CRESTED:
            // Base head, features added separately
            metaballs.addMetaball(position, size, 1.0f);
            metaballs.addMetaball(position + glm::vec3(size * 0.4f, -size * 0.1f, 0.0f), size * 0.5f, 0.85f);
            break;
    }
}

void CreatureBuilder::addTorso(
    MetaballSystem& metaballs,
    const glm::vec3& position,
    float size,
    float aspectRatio,
    int segments
) {
    // Main body ball
    metaballs.addMetaball(position, size, 1.0f);

    if (segments > 1) {
        float segmentSpacing = size * 0.55f;
        for (int i = 1; i < segments; i++) {
            float offset = -segmentSpacing * i;
            float segmentSize = size * (1.0f - i * 0.12f);
            metaballs.addMetaball(position + glm::vec3(offset, 0.0f, 0.0f), segmentSize, 0.9f);
        }
    }

    // Add some belly/chest bulk
    metaballs.addMetaball(position + glm::vec3(0.0f, -size * 0.15f, 0.0f), size * 0.75f, 0.6f);
}

void CreatureBuilder::addSpine(
    MetaballSystem& metaballs,
    float size,
    int segments,
    float curvature
) {
    // Serpentine body made of connected segments
    float totalLength = size * segments * 0.4f;
    float segmentLength = totalLength / segments;

    for (int i = 0; i < segments; i++) {
        float t = (float)i / (segments - 1);
        float segmentSize = size * (0.8f - t * 0.5f);  // Taper toward tail

        // Add slight S-curve
        float xPos = -i * segmentLength;
        float yPos = sin(t * 3.14159f * curvature) * size * 0.3f;
        float zPos = sin(t * 3.14159f * 2.0f * curvature) * size * 0.2f;

        metaballs.addMetaball(glm::vec3(xPos, yPos, zPos), segmentSize, 0.85f);
    }
}

void CreatureBuilder::addTail(
    MetaballSystem& metaballs,
    const glm::vec3& basePosition,
    float baseSize,
    float length,
    TailType type
) {
    int segments = (int)(length / baseSize) + 2;
    segments = glm::clamp(segments, 2, 6);
    float segmentLength = length / segments;

    for (int i = 0; i < segments; i++) {
        float t = (float)i / segments;
        float segmentSize;

        switch (type) {
            case TailType::SHORT:
            case TailType::LONG:
                segmentSize = baseSize * (1.0f - t * 0.65f);
                break;
            case TailType::BUSHY:
                segmentSize = baseSize * (1.0f - t * 0.3f) * (1.0f + sin(t * 3.14159f) * 0.3f);
                break;
            case TailType::SPIKED:
                segmentSize = baseSize * (1.0f - t * 0.5f);
                break;
            case TailType::FINNED:
                segmentSize = baseSize * (0.8f - t * 0.4f);
                break;
            default:
                segmentSize = baseSize * (1.0f - t * 0.6f);
        }

        glm::vec3 offset(-segmentLength * i, -t * baseSize * 0.2f, 0.0f);
        metaballs.addMetaball(basePosition + offset, segmentSize, 0.7f - t * 0.2f);
    }

    // Add special tail features
    glm::vec3 tipPos = basePosition + glm::vec3(-length, -baseSize * 0.2f, 0.0f);

    if (type == TailType::SPIKED) {
        // Club at the end
        metaballs.addMetaball(tipPos, baseSize * 0.8f, 0.8f);
        addSpikes(metaballs, tipPos, baseSize * 0.3f, 3);
    } else if (type == TailType::FINNED) {
        // Fin at the end
        addFins(metaballs, tipPos, baseSize * 0.5f, false);
    } else if (type == TailType::BUSHY) {
        // Fluffy end
        for (int i = 0; i < 3; i++) {
            float angle = i * 2.0f * 3.14159f / 3.0f;
            glm::vec3 offset(0.0f, cos(angle) * baseSize * 0.3f, sin(angle) * baseSize * 0.3f);
            metaballs.addMetaball(tipPos + offset, baseSize * 0.4f, 0.5f);
        }
    }
}

void CreatureBuilder::addLimb(
    MetaballSystem& metaballs,
    const glm::vec3& attachPoint,
    const glm::vec3& direction,
    float thickness,
    float length,
    int joints
) {
    glm::vec3 dir = glm::normalize(direction);
    float segmentLength = length / joints;

    for (int i = 0; i < joints; i++) {
        float t = (float)i / joints;
        float segmentSize = thickness * (1.0f - t * 0.35f);

        // Add slight bend to limbs
        glm::vec3 bendOffset(0.0f);
        if (i == 1 && joints >= 2) {
            bendOffset = glm::vec3(0.0f, -segmentLength * 0.15f, 0.0f);  // Knee/elbow bend
        }

        glm::vec3 offset = dir * segmentLength * (i + 0.5f) + bendOffset;
        metaballs.addMetaball(attachPoint + offset, segmentSize, 0.6f);
    }

    // Foot/pad at the end
    glm::vec3 footPos = attachPoint + dir * length;
    metaballs.addMetaball(footPos, thickness * 0.5f, 0.5f);
}

void CreatureBuilder::addWings(
    MetaballSystem& metaballs,
    const glm::vec3& attachPoint,
    float size,
    float span
) {
    // Wing bone structure
    int segments = 3;
    float segmentLength = span / segments;

    for (int side = -1; side <= 1; side += 2) {
        glm::vec3 basePos = attachPoint;
        basePos.z *= side;

        for (int i = 0; i < segments; i++) {
            float t = (float)i / segments;
            float segmentSize = size * (1.0f - t * 0.5f);

            glm::vec3 offset(
                -segmentLength * 0.2f * i,
                segmentLength * 0.1f * i,
                side * segmentLength * i
            );

            metaballs.addMetaball(basePos + offset, segmentSize, 0.5f);

            // Wing membrane (flatter metaballs along the wing)
            if (i > 0) {
                metaballs.addMetaball(
                    basePos + offset + glm::vec3(-size * 0.5f, 0.0f, 0.0f),
                    size * 0.6f,
                    0.3f
                );
            }
        }
    }
}

void CreatureBuilder::addFins(
    MetaballSystem& metaballs,
    const glm::vec3& attachPoint,
    float size,
    bool dorsal
) {
    if (dorsal) {
        // Dorsal fin (on top)
        metaballs.addMetaball(attachPoint + glm::vec3(0.0f, size, 0.0f), size * 0.6f, 0.5f);
        metaballs.addMetaball(attachPoint + glm::vec3(-size * 0.3f, size * 0.7f, 0.0f), size * 0.4f, 0.4f);
    } else {
        // Tail fin (horizontal)
        metaballs.addMetaball(attachPoint + glm::vec3(0.0f, size * 0.3f, 0.0f), size * 0.5f, 0.5f);
        metaballs.addMetaball(attachPoint + glm::vec3(0.0f, -size * 0.3f, 0.0f), size * 0.5f, 0.5f);
    }
}

void CreatureBuilder::addEyes(
    MetaballSystem& metaballs,
    const glm::vec3& position,
    float size,
    bool sideFacing,
    int eyeCount
) {
    if (eyeCount == 2) {
        // Standard pair
        metaballs.addMetaball(position, size, 0.8f);
        metaballs.addMetaball(glm::vec3(position.x, position.y, -position.z), size, 0.8f);
    } else if (eyeCount == 4) {
        // 4 eyes (spider-like)
        float spacing = size * 1.2f;
        metaballs.addMetaball(position, size, 0.75f);
        metaballs.addMetaball(glm::vec3(position.x, position.y, -position.z), size, 0.75f);
        metaballs.addMetaball(position + glm::vec3(-spacing * 0.5f, spacing * 0.3f, spacing * 0.3f), size * 0.7f, 0.7f);
        metaballs.addMetaball(position + glm::vec3(-spacing * 0.5f, spacing * 0.3f, -spacing * 0.3f), size * 0.7f, 0.7f);
    }
}

void CreatureBuilder::addHorns(
    MetaballSystem& metaballs,
    const glm::vec3& position,
    float size,
    int count
) {
    if (count == 2) {
        // Two horns curving outward
        for (int side = -1; side <= 1; side += 2) {
            glm::vec3 hornBase = position + glm::vec3(0.0f, 0.0f, side * size * 0.5f);
            metaballs.addMetaball(hornBase, size * 0.6f, 0.6f);
            metaballs.addMetaball(hornBase + glm::vec3(-size * 0.3f, size * 0.8f, side * size * 0.4f), size * 0.35f, 0.5f);
        }
    } else if (count == 4) {
        // Four horns
        for (int i = 0; i < 4; i++) {
            float angle = i * 3.14159f / 2.0f + 3.14159f / 4.0f;
            glm::vec3 hornBase = position + glm::vec3(cos(angle) * size * 0.3f, 0.0f, sin(angle) * size * 0.5f);
            metaballs.addMetaball(hornBase, size * 0.5f, 0.55f);
            metaballs.addMetaball(hornBase + glm::vec3(0.0f, size * 0.7f, 0.0f), size * 0.3f, 0.45f);
        }
    }
}

void CreatureBuilder::addAntlers(
    MetaballSystem& metaballs,
    const glm::vec3& position,
    float size,
    int branches
) {
    for (int side = -1; side <= 1; side += 2) {
        glm::vec3 antlerBase = position + glm::vec3(0.0f, 0.0f, side * size * 0.4f);

        // Main beam
        metaballs.addMetaball(antlerBase, size * 0.4f, 0.5f);
        metaballs.addMetaball(antlerBase + glm::vec3(-size * 0.2f, size * 0.6f, side * size * 0.2f), size * 0.3f, 0.45f);

        // Branches
        for (int b = 0; b < branches; b++) {
            float t = (float)(b + 1) / (branches + 1);
            glm::vec3 branchPos = antlerBase + glm::vec3(-size * 0.1f * t, size * 0.3f * t, side * size * 0.1f * t);
            metaballs.addMetaball(branchPos + glm::vec3(size * 0.3f, size * 0.2f, 0.0f), size * 0.2f, 0.4f);
        }
    }
}

void CreatureBuilder::addCrest(
    MetaballSystem& metaballs,
    const glm::vec3& position,
    float size
) {
    // Fan-like crest
    int spines = 4;
    for (int i = 0; i < spines; i++) {
        float t = (float)i / (spines - 1) - 0.5f;
        float height = size * (1.0f - std::abs(t) * 0.4f);
        glm::vec3 spinePos = position + glm::vec3(-size * 0.3f * i / spines, height, t * size * 0.3f);
        metaballs.addMetaball(spinePos, size * 0.25f, 0.5f);
    }
}

void CreatureBuilder::addSpikes(
    MetaballSystem& metaballs,
    const glm::vec3& basePosition,
    float size,
    int count
) {
    float spacing = size * 1.3f;
    for (int i = 0; i < count; i++) {
        float offset = (i - count / 2.0f) * spacing;
        glm::vec3 spikeBase = basePosition + glm::vec3(offset, 0.0f, 0.0f);

        metaballs.addMetaball(spikeBase, size * 0.7f, 0.5f);
        metaballs.addMetaball(spikeBase + glm::vec3(0.0f, size * 1.1f, 0.0f), size * 0.35f, 0.4f);
    }
}

void CreatureBuilder::addEars(
    MetaballSystem& metaballs,
    const glm::vec3& position,
    float size,
    bool pointed
) {
    for (int side = -1; side <= 1; side += 2) {
        glm::vec3 earPos = position;
        earPos.z *= side;

        metaballs.addMetaball(earPos, size, 0.6f);

        if (pointed) {
            metaballs.addMetaball(earPos + glm::vec3(0.0f, size * 1.0f, side * size * 0.2f), size * 0.5f, 0.5f);
        } else {
            // Floppy/round ears
            metaballs.addMetaball(earPos + glm::vec3(0.0f, size * 0.5f, side * size * 0.3f), size * 0.7f, 0.55f);
        }
    }
}

void CreatureBuilder::addMandibles(
    MetaballSystem& metaballs,
    const glm::vec3& position,
    float size
) {
    for (int side = -1; side <= 1; side += 2) {
        glm::vec3 mandibleBase = position + glm::vec3(0.0f, 0.0f, side * size * 0.8f);
        metaballs.addMetaball(mandibleBase, size * 0.8f, 0.6f);
        metaballs.addMetaball(mandibleBase + glm::vec3(size * 0.6f, 0.0f, side * size * 0.3f), size * 0.5f, 0.5f);
    }
}

void CreatureBuilder::addAntennae(
    MetaballSystem& metaballs,
    const glm::vec3& position,
    float size,
    float length
) {
    for (int side = -1; side <= 1; side += 2) {
        glm::vec3 base = position + glm::vec3(0.0f, 0.0f, side * size * 2.0f);

        int segments = 3;
        for (int i = 0; i < segments; i++) {
            float t = (float)i / segments;
            glm::vec3 segPos = base + glm::vec3(length * 0.3f * t, length * t, side * length * 0.2f * t);
            metaballs.addMetaball(segPos, size * (1.0f - t * 0.5f), 0.4f);
        }
    }
}

void CreatureBuilder::addClaws(
    MetaballSystem& metaballs,
    const glm::vec3& position,
    float size
) {
    // Three claws/talons
    for (int i = -1; i <= 1; i++) {
        glm::vec3 clawPos = position + glm::vec3(size * 0.3f, -size * 0.2f, i * size * 0.4f);
        metaballs.addMetaball(clawPos, size * 0.4f, 0.5f);
        metaballs.addMetaball(clawPos + glm::vec3(size * 0.4f, -size * 0.3f, 0.0f), size * 0.2f, 0.4f);
    }
}

void CreatureBuilder::buildFlying(
    MetaballSystem& metaballs,
    const Genome& genome
) {
    // Flying creature - bird-like body with prominent wings
    float bodyScale = genome.size;
    float wingExtent = genome.wingSpan * bodyScale;

    // Streamlined body (longer, thinner than land creatures)
    // Main body - aerodynamic shape
    metaballs.addMetaball(glm::vec3(0.0f, 0.0f, 0.0f), 0.4f * bodyScale, 1.0f);

    // Head - smaller, forward-facing for aerodynamics
    metaballs.addMetaball(glm::vec3(0.3f * bodyScale, 0.05f * bodyScale, 0.0f), 0.25f * bodyScale, 0.8f);

    // Tail base - for balance and steering
    metaballs.addMetaball(glm::vec3(-0.35f * bodyScale, 0.0f, 0.0f), 0.2f * bodyScale, 0.6f);

    // Tail tip - fan-like for maneuvering
    metaballs.addMetaball(glm::vec3(-0.5f * bodyScale, 0.0f, 0.0f), 0.1f * bodyScale, 0.4f);

    // Beak - sharp and pointed
    metaballs.addMetaball(glm::vec3(0.45f * bodyScale, 0.0f, 0.0f), 0.08f * bodyScale, 0.4f);
    metaballs.addMetaball(glm::vec3(0.55f * bodyScale, -0.02f * bodyScale, 0.0f), 0.04f * bodyScale, 0.3f);

    // Eyes - forward-facing for depth perception (hunting)
    float eyeSize = bodyScale * 0.06f + genome.visionRange * 0.002f;
    metaballs.addMetaball(glm::vec3(0.32f * bodyScale, 0.08f * bodyScale, 0.08f * bodyScale), eyeSize, 0.7f);
    metaballs.addMetaball(glm::vec3(0.32f * bodyScale, 0.08f * bodyScale, -0.08f * bodyScale), eyeSize, 0.7f);

    // Wings - the defining feature
    // Wing structure extends primarily in Z direction (sideways)
    // These will be animated in the shader based on Z position

    // Right wing
    metaballs.addMetaball(glm::vec3(0.0f, 0.02f * bodyScale, wingExtent * 0.25f), 0.12f * bodyScale, 0.5f);   // Wing root
    metaballs.addMetaball(glm::vec3(-0.05f * bodyScale, 0.03f * bodyScale, wingExtent * 0.5f), 0.09f * bodyScale, 0.4f);   // Wing mid
    metaballs.addMetaball(glm::vec3(-0.1f * bodyScale, 0.04f * bodyScale, wingExtent * 0.75f), 0.06f * bodyScale, 0.35f);  // Wing outer
    metaballs.addMetaball(glm::vec3(-0.12f * bodyScale, 0.05f * bodyScale, wingExtent * 0.95f), 0.04f * bodyScale, 0.3f);  // Wing tip

    // Wing membrane (flatter, wider metaballs for visible wing surface)
    metaballs.addMetaball(glm::vec3(-0.1f * bodyScale, 0.0f, wingExtent * 0.4f), 0.15f * bodyScale, 0.25f);
    metaballs.addMetaball(glm::vec3(-0.15f * bodyScale, 0.01f, wingExtent * 0.65f), 0.12f * bodyScale, 0.2f);

    // Left wing (mirrored)
    metaballs.addMetaball(glm::vec3(0.0f, 0.02f * bodyScale, -wingExtent * 0.25f), 0.12f * bodyScale, 0.5f);
    metaballs.addMetaball(glm::vec3(-0.05f * bodyScale, 0.03f * bodyScale, -wingExtent * 0.5f), 0.09f * bodyScale, 0.4f);
    metaballs.addMetaball(glm::vec3(-0.1f * bodyScale, 0.04f * bodyScale, -wingExtent * 0.75f), 0.06f * bodyScale, 0.35f);
    metaballs.addMetaball(glm::vec3(-0.12f * bodyScale, 0.05f * bodyScale, -wingExtent * 0.95f), 0.04f * bodyScale, 0.3f);

    // Left wing membrane
    metaballs.addMetaball(glm::vec3(-0.1f * bodyScale, 0.0f, -wingExtent * 0.4f), 0.15f * bodyScale, 0.25f);
    metaballs.addMetaball(glm::vec3(-0.15f * bodyScale, 0.01f, -wingExtent * 0.65f), 0.12f * bodyScale, 0.2f);

    // Legs - tucked under body, shorter for flight
    float legLength = bodyScale * 0.4f;
    float legThickness = bodyScale * 0.08f;

    // Right leg
    addLimb(metaballs, glm::vec3(-0.1f * bodyScale, -0.2f * bodyScale, 0.1f * bodyScale),
            glm::vec3(0.0f, -1.0f, 0.15f), legThickness, legLength, 2);

    // Left leg
    addLimb(metaballs, glm::vec3(-0.1f * bodyScale, -0.2f * bodyScale, -0.1f * bodyScale),
            glm::vec3(0.0f, -1.0f, -0.15f), legThickness, legLength, 2);

    // Talons for grabbing prey
    addClaws(metaballs, glm::vec3(-0.1f * bodyScale, -legLength * 0.9f, 0.12f * bodyScale), bodyScale * 0.06f);
    addClaws(metaballs, glm::vec3(-0.1f * bodyScale, -legLength * 0.9f, -0.12f * bodyScale), bodyScale * 0.06f);

    // Tail feathers (fan shape for control)
    float tailFanSize = bodyScale * 0.15f;
    for (int i = -2; i <= 2; i++) {
        float angle = i * 0.3f;  // Spread angle
        float zOffset = sin(angle) * tailFanSize;
        float yOffset = cos(angle) * tailFanSize * 0.3f - tailFanSize * 0.2f;
        metaballs.addMetaball(glm::vec3(-0.6f * bodyScale, yOffset, zOffset), 0.05f * bodyScale, 0.35f);
    }
}
