#pragma once

#include "Allele.h"
#include <vector>
#include <cstdint>

namespace genetics {

// Types of traits that genes can affect
enum class GeneType {
    // Physical traits
    SIZE,
    SPEED,
    VISION_RANGE,
    EFFICIENCY,

    // Color components
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    PATTERN_TYPE,  // NEW: Pattern type (spots, stripes, etc.)

    // Behavioral traits
    AGGRESSION,
    SOCIALITY,
    CURIOSITY,
    FEAR_RESPONSE,

    // Mate preferences
    MATE_SIZE_PREF,
    MATE_ORNAMENT_PREF,
    MATE_SIMILARITY_PREF,
    CHOOSINESS,

    // Sexual selection traits
    ORNAMENT_INTENSITY,
    DISPLAY_FREQUENCY,

    // Neural network weights (indexed)
    NEURAL_WEIGHT,

    // Reproductive traits
    FERTILITY,
    MATURATION_RATE,

    // Metabolic traits
    METABOLIC_RATE,
    HEAT_TOLERANCE,
    COLD_TOLERANCE,

    // Niche specialization
    DIET_SPECIALIZATION,
    HABITAT_PREFERENCE,
    ACTIVITY_TIME,  // 0 = nocturnal, 1 = diurnal

    // ========================================
    // NEW: Creature Type Aptitudes (C-15 fix)
    // ========================================
    TERRESTRIAL_APTITUDE,  // Land creature potential
    AQUATIC_APTITUDE,      // Water creature potential
    AERIAL_APTITUDE,       // Flying creature potential

    // ========================================
    // NEW: Flying Traits
    // ========================================
    WING_SPAN,             // Wing size ratio to body (0.5-2.0)
    FLAP_FREQUENCY,        // Wing beat rate Hz (2.0-100.0, insects higher)
    GLIDE_RATIO,           // Gliding vs flapping efficiency (0.1-0.9)
    PREFERRED_ALTITUDE,    // Preferred flight height (5.0-80.0)

    // ========================================
    // NEW: Aquatic Traits
    // ========================================
    FIN_SIZE,              // Dorsal/pectoral fin size (0.3-1.0)
    TAIL_SIZE,             // Caudal fin size (0.5-1.2)
    SWIM_FREQUENCY,        // Body wave frequency Hz (1.0-4.0)
    SWIM_AMPLITUDE,        // S-wave body movement (0.1-0.3)
    PREFERRED_DEPTH,       // Depth preference (0.1-0.5 normalized)
    SCHOOLING_STRENGTH,    // Group cohesion tendency (0.0-1.0)

    // ========================================
    // NEW: Advanced Sensory Traits
    // ========================================
    VISION_FOV,            // Field of view radians (1.0-6.0)
    VISION_ACUITY,         // Detail perception (0.0-1.0)
    COLOR_PERCEPTION,      // Color sensitivity (0.0-1.0)
    MOTION_DETECTION,      // Motion sensitivity bonus (0.0-1.0)

    HEARING_RANGE,         // Auditory detection distance (10.0-100.0)
    HEARING_DIRECTIONALITY, // Sound localization accuracy (0.0-1.0)
    ECHOLOCATION_ABILITY,  // Echolocation capability (0.0-1.0)

    SMELL_RANGE,           // Odor detection distance (10.0-150.0)
    SMELL_SENSITIVITY,     // Detection threshold (0.0-1.0)
    PHEROMONE_PRODUCTION,  // Pheromone emission rate (0.0-1.0)

    TOUCH_RANGE,           // Short-range detection (0.5-8.0)
    VIBRATION_SENSITIVITY, // Ground/water vibration (0.0-1.0)

    // ========================================
    // NEW: Defense & Communication
    // ========================================
    CAMOUFLAGE_LEVEL,      // Visual detection reduction (0.0-1.0)
    ALARM_CALL_VOLUME,     // Warning signal intensity (0.0-1.0)

    // ========================================
    // NEW: Memory Traits
    // ========================================
    MEMORY_CAPACITY,       // Spatial memory size (0.0-1.0)
    MEMORY_RETENTION,      // Memory persistence (0.0-1.0)

    COUNT  // Number of gene types
};

// Epigenetic marks that modify gene expression
enum class EpigeneticMarkType {
    METHYLATION,      // Usually silencing
    ACETYLATION,      // Usually activating
    PHOSPHORYLATION,  // Signal response
    IMPRINTING        // Parent-of-origin effect
};

struct EpigeneticMark {
    EpigeneticMarkType type;
    float intensity;           // 0-1 strength
    int generationsRemaining;  // How long mark persists
    bool isHeritable;

    EpigeneticMark(EpigeneticMarkType t = EpigeneticMarkType::METHYLATION,
                   float i = 0.5f, int gen = 2, bool h = true)
        : type(t), intensity(i), generationsRemaining(gen), isHeritable(h) {}
};

// A gene is a locus with two alleles (diploid)
class Gene {
public:
    Gene();
    Gene(uint32_t locus, GeneType type);
    Gene(uint32_t locus, GeneType type, const Allele& a1, const Allele& a2);

    // Getters
    uint32_t getLocus() const { return locus; }
    GeneType getType() const { return type; }
    const Allele& getAllele1() const { return allele1; }
    const Allele& getAllele2() const { return allele2; }
    Allele& getAllele1() { return allele1; }
    Allele& getAllele2() { return allele2; }
    float getExpressionLevel() const { return expressionLevel; }
    int getNeuralIndex() const { return neuralIndex; }

    // Setters
    void setAllele1(const Allele& a) { allele1 = a; }
    void setAllele2(const Allele& a) { allele2 = a; }
    void setExpressionLevel(float e) { expressionLevel = e; }
    void setNeuralIndex(int idx) { neuralIndex = idx; }

    // Calculate the expressed phenotype for this gene
    float getPhenotype() const;

    // Check genetic state
    bool isHomozygous() const;
    float getHeterozygosity() const;

    // Epigenetics
    void addEpigeneticMark(const EpigeneticMark& mark);
    void updateEpigeneticMarks();
    float getEpigeneticModifier() const;
    const std::vector<EpigeneticMark>& getEpigeneticMarks() const { return epigeneticMarks; }

    // Inheritance
    Allele getRandomAllele() const;

    // Mutation
    void mutate(float strength);

    // Get the total fitness effect of this gene
    float getFitnessEffect() const;

private:
    uint32_t locus;
    GeneType type;
    Allele allele1, allele2;  // Maternal and paternal
    float expressionLevel;    // Base expression (0-1)
    int neuralIndex;          // For NEURAL_WEIGHT type, which weight index
    std::vector<EpigeneticMark> epigeneticMarks;
};

// Helper to get default value range for a gene type
struct GeneValueRange {
    float min;
    float max;
    float defaultVal;
};

GeneValueRange getGeneValueRange(GeneType type);

// Convert gene type to string
const char* geneTypeToString(GeneType type);

} // namespace genetics
