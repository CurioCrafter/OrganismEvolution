#include "Gene.h"
#include "../../utils/Random.h"
#include <algorithm>
#include <cmath>

namespace genetics {

Gene::Gene()
    : locus(0), type(GeneType::SIZE), expressionLevel(1.0f), neuralIndex(-1) {
    auto range = getGeneValueRange(type);
    allele1 = Allele(range.defaultVal, 0.5f);
    allele2 = Allele(range.defaultVal, 0.5f);
}

Gene::Gene(uint32_t locus, GeneType type)
    : locus(locus), type(type), expressionLevel(1.0f), neuralIndex(-1) {
    auto range = getGeneValueRange(type);
    float val1 = Random::range(range.min, range.max);
    float val2 = Random::range(range.min, range.max);
    allele1 = Allele(val1, Random::range(0.3f, 0.7f));
    allele2 = Allele(val2, Random::range(0.3f, 0.7f));
}

Gene::Gene(uint32_t locus, GeneType type, const Allele& a1, const Allele& a2)
    : locus(locus), type(type), allele1(a1), allele2(a2),
      expressionLevel(1.0f), neuralIndex(-1) {
}

float Gene::getPhenotype() const {
    float basePhenotype = Allele::calculatePhenotype(allele1, allele2);

    // Apply expression level
    float expr = expressionLevel * getEpigeneticModifier();

    // Clamp to valid range
    auto range = getGeneValueRange(type);
    float phenotype = basePhenotype * expr;
    return std::clamp(phenotype, range.min, range.max);
}

bool Gene::isHomozygous() const {
    // Consider homozygous if allele values are very similar
    return std::abs(allele1.getValue() - allele2.getValue()) < 0.01f;
}

float Gene::getHeterozygosity() const {
    // Measure of how different the two alleles are
    auto range = getGeneValueRange(type);
    float rangeSize = range.max - range.min;
    if (rangeSize <= 0) return 0.0f;

    float diff = std::abs(allele1.getValue() - allele2.getValue());
    return diff / rangeSize;
}

void Gene::addEpigeneticMark(const EpigeneticMark& mark) {
    // Check if we already have this type
    for (auto& existing : epigeneticMarks) {
        if (existing.type == mark.type) {
            // Combine marks
            existing.intensity = (existing.intensity + mark.intensity) / 2.0f;
            existing.generationsRemaining = std::max(existing.generationsRemaining,
                                                      mark.generationsRemaining);
            return;
        }
    }
    epigeneticMarks.push_back(mark);
}

void Gene::updateEpigeneticMarks() {
    // Decay marks over generations
    for (auto it = epigeneticMarks.begin(); it != epigeneticMarks.end();) {
        it->generationsRemaining--;
        if (it->generationsRemaining <= 0) {
            it = epigeneticMarks.erase(it);
        } else {
            // Intensity fades over time
            it->intensity *= 0.8f;
            ++it;
        }
    }
}

float Gene::getEpigeneticModifier() const {
    float modifier = 1.0f;

    for (const auto& mark : epigeneticMarks) {
        switch (mark.type) {
            case EpigeneticMarkType::METHYLATION:
                // Methylation typically reduces expression
                modifier *= (1.0f - mark.intensity * 0.5f);
                break;

            case EpigeneticMarkType::ACETYLATION:
                // Acetylation typically increases expression
                modifier *= (1.0f + mark.intensity * 0.3f);
                break;

            case EpigeneticMarkType::PHOSPHORYLATION:
                // Signal-dependent, slight increase
                modifier *= (1.0f + mark.intensity * 0.1f);
                break;

            case EpigeneticMarkType::IMPRINTING:
                // Can silence one allele effectively
                modifier *= (1.0f - mark.intensity * 0.4f);
                break;
        }
    }

    return std::clamp(modifier, 0.1f, 2.0f);
}

Allele Gene::getRandomAllele() const {
    return Random::chance(0.5f) ? allele1 : allele2;
}

void Gene::mutate(float strength) {
    // Mutate one or both alleles
    if (Random::chance(0.5f)) {
        allele1 = allele1.mutate(strength);
    }
    if (Random::chance(0.5f)) {
        allele2 = allele2.mutate(strength);
    }

    // Small chance of expression level change
    if (Random::chance(0.1f)) {
        expressionLevel += Random::range(-0.1f, 0.1f);
        expressionLevel = std::clamp(expressionLevel, 0.1f, 1.5f);
    }
}

float Gene::getFitnessEffect() const {
    float effect = (allele1.getFitnessEffect() + allele2.getFitnessEffect()) / 2.0f;

    // Homozygous deleterious alleles have stronger effect
    if (allele1.isDeleterious() && allele2.isDeleterious()) {
        effect *= 1.5f;
    }
    // Heterozygote advantage in some cases
    else if ((allele1.isDeleterious() != allele2.isDeleterious()) &&
             getHeterozygosity() > 0.3f) {
        effect *= 0.5f;  // Reduced negative effect
    }

    return effect;
}

GeneValueRange getGeneValueRange(GeneType type) {
    switch (type) {
        case GeneType::SIZE:
            return {0.5f, 2.0f, 1.0f};
        case GeneType::SPEED:
            return {5.0f, 20.0f, 12.0f};
        case GeneType::VISION_RANGE:
            return {10.0f, 50.0f, 30.0f};
        case GeneType::EFFICIENCY:
            return {0.5f, 1.5f, 1.0f};

        case GeneType::COLOR_RED:
        case GeneType::COLOR_GREEN:
        case GeneType::COLOR_BLUE:
            return {0.0f, 1.0f, 0.5f};
        case GeneType::PATTERN_TYPE:
            return {0.0f, 1.0f, 0.0f};  // 0 = solid, higher = more complex patterns

        case GeneType::AGGRESSION:
        case GeneType::SOCIALITY:
        case GeneType::CURIOSITY:
        case GeneType::FEAR_RESPONSE:
            return {0.0f, 1.0f, 0.5f};

        case GeneType::MATE_SIZE_PREF:
        case GeneType::MATE_ORNAMENT_PREF:
        case GeneType::MATE_SIMILARITY_PREF:
            return {-1.0f, 1.0f, 0.0f};

        case GeneType::CHOOSINESS:
            return {0.0f, 1.0f, 0.5f};

        case GeneType::ORNAMENT_INTENSITY:
        case GeneType::DISPLAY_FREQUENCY:
            return {0.0f, 1.0f, 0.3f};

        case GeneType::NEURAL_WEIGHT:
            return {-1.0f, 1.0f, 0.0f};

        case GeneType::FERTILITY:
            return {0.5f, 1.5f, 1.0f};
        case GeneType::MATURATION_RATE:
            return {0.5f, 2.0f, 1.0f};

        case GeneType::METABOLIC_RATE:
            return {0.5f, 1.5f, 1.0f};
        case GeneType::HEAT_TOLERANCE:
        case GeneType::COLD_TOLERANCE:
            return {0.0f, 1.0f, 0.5f};

        case GeneType::DIET_SPECIALIZATION:
            return {0.0f, 1.0f, 0.3f};  // 0 = generalist
        case GeneType::HABITAT_PREFERENCE:
            return {0.0f, 1.0f, 0.5f};
        case GeneType::ACTIVITY_TIME:
            return {0.0f, 1.0f, 0.5f};  // 0 = nocturnal, 1 = diurnal

        // ========================================
        // Creature Type Aptitudes (C-15 fix)
        // ========================================
        case GeneType::TERRESTRIAL_APTITUDE:
            return {0.0f, 1.0f, 0.8f};  // Default high for land creatures
        case GeneType::AQUATIC_APTITUDE:
            return {0.0f, 1.0f, 0.2f};  // Default low
        case GeneType::AERIAL_APTITUDE:
            return {0.0f, 1.0f, 0.1f};  // Default very low (flying is rare)

        // ========================================
        // Flying Traits
        // ========================================
        case GeneType::WING_SPAN:
            return {0.5f, 2.5f, 1.0f};
        case GeneType::FLAP_FREQUENCY:
            return {2.0f, 100.0f, 5.0f};  // Birds: 2-8Hz, Insects: 20-100Hz
        case GeneType::GLIDE_RATIO:
            return {0.1f, 0.9f, 0.5f};
        case GeneType::PREFERRED_ALTITUDE:
            return {5.0f, 80.0f, 25.0f};

        // ========================================
        // Aquatic Traits
        // ========================================
        case GeneType::FIN_SIZE:
            return {0.3f, 1.0f, 0.6f};
        case GeneType::TAIL_SIZE:
            return {0.5f, 1.2f, 0.8f};
        case GeneType::SWIM_FREQUENCY:
            return {1.0f, 4.0f, 2.0f};
        case GeneType::SWIM_AMPLITUDE:
            return {0.1f, 0.3f, 0.2f};
        case GeneType::PREFERRED_DEPTH:
            return {0.1f, 0.5f, 0.3f};
        case GeneType::SCHOOLING_STRENGTH:
            return {0.0f, 1.0f, 0.7f};  // Most fish school

        // ========================================
        // Advanced Sensory Traits
        // ========================================
        case GeneType::VISION_FOV:
            return {1.0f, 6.0f, 3.0f};  // ~180 degrees default
        case GeneType::VISION_ACUITY:
            return {0.0f, 1.0f, 0.5f};
        case GeneType::COLOR_PERCEPTION:
            return {0.0f, 1.0f, 0.5f};
        case GeneType::MOTION_DETECTION:
            return {0.0f, 1.0f, 0.6f};

        case GeneType::HEARING_RANGE:
            return {10.0f, 100.0f, 40.0f};
        case GeneType::HEARING_DIRECTIONALITY:
            return {0.0f, 1.0f, 0.5f};
        case GeneType::ECHOLOCATION_ABILITY:
            return {0.0f, 1.0f, 0.0f};  // Very rare trait

        case GeneType::SMELL_RANGE:
            return {10.0f, 150.0f, 50.0f};
        case GeneType::SMELL_SENSITIVITY:
            return {0.0f, 1.0f, 0.5f};
        case GeneType::PHEROMONE_PRODUCTION:
            return {0.0f, 1.0f, 0.3f};

        case GeneType::TOUCH_RANGE:
            return {0.5f, 8.0f, 2.0f};
        case GeneType::VIBRATION_SENSITIVITY:
            return {0.0f, 1.0f, 0.4f};

        // ========================================
        // Defense & Communication
        // ========================================
        case GeneType::CAMOUFLAGE_LEVEL:
            return {0.0f, 1.0f, 0.3f};
        case GeneType::ALARM_CALL_VOLUME:
            return {0.0f, 1.0f, 0.5f};

        // ========================================
        // Memory Traits
        // ========================================
        case GeneType::MEMORY_CAPACITY:
            return {0.0f, 1.0f, 0.5f};
        case GeneType::MEMORY_RETENTION:
            return {0.0f, 1.0f, 0.5f};

        default:
            return {0.0f, 1.0f, 0.5f};
    }
}

const char* geneTypeToString(GeneType type) {
    switch (type) {
        case GeneType::SIZE: return "Size";
        case GeneType::SPEED: return "Speed";
        case GeneType::VISION_RANGE: return "Vision Range";
        case GeneType::EFFICIENCY: return "Efficiency";
        case GeneType::COLOR_RED: return "Color (Red)";
        case GeneType::COLOR_GREEN: return "Color (Green)";
        case GeneType::COLOR_BLUE: return "Color (Blue)";
        case GeneType::PATTERN_TYPE: return "Pattern Type";
        case GeneType::AGGRESSION: return "Aggression";
        case GeneType::SOCIALITY: return "Sociality";
        case GeneType::CURIOSITY: return "Curiosity";
        case GeneType::FEAR_RESPONSE: return "Fear Response";
        case GeneType::MATE_SIZE_PREF: return "Mate Size Preference";
        case GeneType::MATE_ORNAMENT_PREF: return "Mate Ornament Preference";
        case GeneType::MATE_SIMILARITY_PREF: return "Mate Similarity Preference";
        case GeneType::CHOOSINESS: return "Choosiness";
        case GeneType::ORNAMENT_INTENSITY: return "Ornament Intensity";
        case GeneType::DISPLAY_FREQUENCY: return "Display Frequency";
        case GeneType::NEURAL_WEIGHT: return "Neural Weight";
        case GeneType::FERTILITY: return "Fertility";
        case GeneType::MATURATION_RATE: return "Maturation Rate";
        case GeneType::METABOLIC_RATE: return "Metabolic Rate";
        case GeneType::HEAT_TOLERANCE: return "Heat Tolerance";
        case GeneType::COLD_TOLERANCE: return "Cold Tolerance";
        case GeneType::DIET_SPECIALIZATION: return "Diet Specialization";
        case GeneType::HABITAT_PREFERENCE: return "Habitat Preference";
        case GeneType::ACTIVITY_TIME: return "Activity Time";
        // Creature Type Aptitudes
        case GeneType::TERRESTRIAL_APTITUDE: return "Terrestrial Aptitude";
        case GeneType::AQUATIC_APTITUDE: return "Aquatic Aptitude";
        case GeneType::AERIAL_APTITUDE: return "Aerial Aptitude";
        // Flying Traits
        case GeneType::WING_SPAN: return "Wing Span";
        case GeneType::FLAP_FREQUENCY: return "Flap Frequency";
        case GeneType::GLIDE_RATIO: return "Glide Ratio";
        case GeneType::PREFERRED_ALTITUDE: return "Preferred Altitude";
        // Aquatic Traits
        case GeneType::FIN_SIZE: return "Fin Size";
        case GeneType::TAIL_SIZE: return "Tail Size";
        case GeneType::SWIM_FREQUENCY: return "Swim Frequency";
        case GeneType::SWIM_AMPLITUDE: return "Swim Amplitude";
        case GeneType::PREFERRED_DEPTH: return "Preferred Depth";
        case GeneType::SCHOOLING_STRENGTH: return "Schooling Strength";
        // Sensory Traits
        case GeneType::VISION_FOV: return "Vision FOV";
        case GeneType::VISION_ACUITY: return "Vision Acuity";
        case GeneType::COLOR_PERCEPTION: return "Color Perception";
        case GeneType::MOTION_DETECTION: return "Motion Detection";
        case GeneType::HEARING_RANGE: return "Hearing Range";
        case GeneType::HEARING_DIRECTIONALITY: return "Hearing Directionality";
        case GeneType::ECHOLOCATION_ABILITY: return "Echolocation Ability";
        case GeneType::SMELL_RANGE: return "Smell Range";
        case GeneType::SMELL_SENSITIVITY: return "Smell Sensitivity";
        case GeneType::PHEROMONE_PRODUCTION: return "Pheromone Production";
        case GeneType::TOUCH_RANGE: return "Touch Range";
        case GeneType::VIBRATION_SENSITIVITY: return "Vibration Sensitivity";
        // Defense & Communication
        case GeneType::CAMOUFLAGE_LEVEL: return "Camouflage Level";
        case GeneType::ALARM_CALL_VOLUME: return "Alarm Call Volume";
        // Memory
        case GeneType::MEMORY_CAPACITY: return "Memory Capacity";
        case GeneType::MEMORY_RETENTION: return "Memory Retention";
        default: return "Unknown";
    }
}

} // namespace genetics
