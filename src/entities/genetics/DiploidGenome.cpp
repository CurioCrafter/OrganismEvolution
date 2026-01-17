#include "DiploidGenome.h"
#include "../../utils/Random.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace genetics {

uint64_t DiploidGenome::nextLineageId = 1;
GenomeConfig DiploidGenome::defaultConfig;

float EcologicalNiche::distanceTo(const EcologicalNiche& other) const {
    float dietDiff = std::abs(dietSpecialization - other.dietSpecialization);
    float habitatDiff = std::abs(habitatPreference - other.habitatPreference);
    float activityDiff = std::abs(activityTime - other.activityTime);

    return (dietDiff + habitatDiff + activityDiff) / 3.0f;
}

DiploidGenome::DiploidGenome()
    : speciesId(0), lineageId(nextLineageId++), hybrid(false) {
    randomize(defaultConfig);
}

DiploidGenome::DiploidGenome(const GenomeConfig& config)
    : speciesId(0), lineageId(nextLineageId++), hybrid(false) {
    randomize(config);
}

DiploidGenome::DiploidGenome(const DiploidGenome& parent1, const DiploidGenome& parent2,
                             bool isHybrid)
    : speciesId(parent1.speciesId), lineageId(nextLineageId++), hybrid(isHybrid) {

    // Create gametes from each parent (meiosis)
    auto gamete1 = parent1.createGamete();
    auto gamete2 = parent2.createGamete();

    // Combine gametes to form new diploid genome
    size_t numPairs = std::min(gamete1.size(), gamete2.size());
    chromosomePairs.reserve(numPairs);

    for (size_t i = 0; i < numPairs; i++) {
        chromosomePairs.emplace_back(gamete1[i], gamete2[i]);
    }

    // Inherit some epigenetic marks
    if (Random::chance(0.3f)) {
        inheritEpigeneticMarks(parent1);
    }
    if (Random::chance(0.3f)) {
        inheritEpigeneticMarks(parent2);
    }
}

void DiploidGenome::randomize(const GenomeConfig& config) {
    chromosomePairs.clear();
    chromosomePairs.reserve(config.numChromosomes);

    // Define gene layout for each chromosome
    // Chromosome 0: Physical traits
    std::vector<GeneType> chrom0 = {
        GeneType::SIZE, GeneType::SPEED, GeneType::VISION_RANGE, GeneType::EFFICIENCY,
        GeneType::METABOLIC_RATE, GeneType::FERTILITY, GeneType::MATURATION_RATE
    };

    // Chromosome 1: Color and display traits
    std::vector<GeneType> chrom1 = {
        GeneType::COLOR_RED, GeneType::COLOR_GREEN, GeneType::COLOR_BLUE,
        GeneType::PATTERN_TYPE, GeneType::ORNAMENT_INTENSITY, GeneType::DISPLAY_FREQUENCY
    };

    // Chromosome 2: Behavioral traits and mate preferences
    std::vector<GeneType> chrom2 = {
        GeneType::AGGRESSION, GeneType::SOCIALITY, GeneType::CURIOSITY, GeneType::FEAR_RESPONSE,
        GeneType::MATE_SIZE_PREF, GeneType::MATE_ORNAMENT_PREF, GeneType::MATE_SIMILARITY_PREF,
        GeneType::CHOOSINESS
    };

    // Chromosome 3: Niche and tolerance traits
    std::vector<GeneType> chrom3 = {
        GeneType::DIET_SPECIALIZATION, GeneType::HABITAT_PREFERENCE, GeneType::ACTIVITY_TIME,
        GeneType::HEAT_TOLERANCE, GeneType::COLD_TOLERANCE
    };

    // Chromosome 4: Creature type aptitudes (NEW - C-15 fix)
    std::vector<GeneType> chrom4 = {
        GeneType::TERRESTRIAL_APTITUDE, GeneType::AQUATIC_APTITUDE, GeneType::AERIAL_APTITUDE
    };

    // Chromosome 5: Flying traits (NEW)
    std::vector<GeneType> chrom5 = {
        GeneType::WING_SPAN, GeneType::FLAP_FREQUENCY, GeneType::GLIDE_RATIO, GeneType::PREFERRED_ALTITUDE
    };

    // Chromosome 6: Aquatic traits (NEW)
    std::vector<GeneType> chrom6 = {
        GeneType::FIN_SIZE, GeneType::TAIL_SIZE, GeneType::SWIM_FREQUENCY,
        GeneType::SWIM_AMPLITUDE, GeneType::PREFERRED_DEPTH, GeneType::SCHOOLING_STRENGTH
    };

    // Chromosome 7: Sensory traits (NEW)
    std::vector<GeneType> chrom7 = {
        GeneType::VISION_FOV, GeneType::VISION_ACUITY, GeneType::COLOR_PERCEPTION, GeneType::MOTION_DETECTION,
        GeneType::HEARING_RANGE, GeneType::HEARING_DIRECTIONALITY, GeneType::ECHOLOCATION_ABILITY,
        GeneType::SMELL_RANGE, GeneType::SMELL_SENSITIVITY, GeneType::PHEROMONE_PRODUCTION,
        GeneType::TOUCH_RANGE, GeneType::VIBRATION_SENSITIVITY
    };

    // Chromosome 8: Defense, communication, memory (NEW)
    std::vector<GeneType> chrom8 = {
        GeneType::CAMOUFLAGE_LEVEL, GeneType::ALARM_CALL_VOLUME,
        GeneType::MEMORY_CAPACITY, GeneType::MEMORY_RETENTION
    };

    std::vector<std::vector<GeneType>> chromosomeLayouts = {
        chrom0, chrom1, chrom2, chrom3, chrom4, chrom5, chrom6, chrom7, chrom8
    };

    // Add neural weight genes to appropriate chromosomes
    int neuralIdx = 0;
    int genesPerChrom = config.neuralWeightCount / config.numChromosomes;

    for (int c = 0; c < config.numChromosomes; c++) {
        Chromosome maternal(c * 2, config.genesPerChromosome);
        Chromosome paternal(c * 2 + 1, config.genesPerChromosome);

        // Initialize with layout genes
        if (c < static_cast<int>(chromosomeLayouts.size())) {
            maternal.initializeGenes(chromosomeLayouts[c]);

            // Copy structure to paternal with different allele values
            for (size_t i = 0; i < maternal.getGeneCount(); i++) {
                Gene gene(maternal.getGene(i).getLocus(), maternal.getGene(i).getType());
                paternal.addGene(gene);
            }
        }

        // Add neural weight genes
        for (int n = 0; n < genesPerChrom && neuralIdx < config.neuralWeightCount; n++, neuralIdx++) {
            Gene neuralGene(maternal.getGeneCount(), GeneType::NEURAL_WEIGHT);
            neuralGene.setNeuralIndex(neuralIdx);
            maternal.addGene(neuralGene);

            Gene neuralGene2(paternal.getGeneCount(), GeneType::NEURAL_WEIGHT);
            neuralGene2.setNeuralIndex(neuralIdx);
            paternal.addGene(neuralGene2);
        }

        chromosomePairs.emplace_back(maternal, paternal);
    }
}

std::vector<Chromosome> DiploidGenome::createGamete() const {
    std::vector<Chromosome> gamete;
    gamete.reserve(chromosomePairs.size());

    for (const auto& pair : chromosomePairs) {
        // Perform recombination
        auto [recomb1, recomb2] = pair.first.recombine(pair.second);

        // Randomly select one recombinant for the gamete
        gamete.push_back(Random::chance(0.5f) ? recomb1 : recomb2);
    }

    return gamete;
}

void DiploidGenome::mutate(float mutationRate, float mutationStrength) {
    for (auto& pair : chromosomePairs) {
        // Point mutations
        for (size_t i = 0; i < pair.first.getGeneCount(); i++) {
            if (Random::chance(mutationRate * defaultConfig.pointMutationProb)) {
                pair.first.applyPointMutation(i, mutationStrength);
            }
        }
        for (size_t i = 0; i < pair.second.getGeneCount(); i++) {
            if (Random::chance(mutationRate * defaultConfig.pointMutationProb)) {
                pair.second.applyPointMutation(i, mutationStrength);
            }
        }

        // Structural mutations (rarer)
        if (Random::chance(mutationRate * defaultConfig.duplicationProb * 0.1f)) {
            size_t idx = Random::range(0, static_cast<int>(pair.first.getGeneCount()));
            if (Random::chance(0.5f)) {
                pair.first.applyDuplication(idx);
            } else {
                pair.second.applyDuplication(idx);
            }
        }

        if (Random::chance(mutationRate * defaultConfig.deletionProb * 0.05f)) {
            // Don't delete if too few genes
            if (pair.first.getGeneCount() > 5) {
                size_t idx = Random::range(0, static_cast<int>(pair.first.getGeneCount()));
                if (Random::chance(0.5f)) {
                    pair.first.applyDeletion(idx);
                } else {
                    pair.second.applyDeletion(idx);
                }
            }
        }

        if (Random::chance(mutationRate * defaultConfig.inversionProb * 0.05f)) {
            size_t start = Random::range(0, static_cast<int>(pair.first.getGeneCount()) - 2);
            size_t end = Random::range(static_cast<int>(start) + 1,
                                       static_cast<int>(pair.first.getGeneCount()));
            if (Random::chance(0.5f)) {
                pair.first.applyInversion(start, end);
            } else {
                pair.second.applyInversion(start, end);
            }
        }
    }
}

float DiploidGenome::getTrait(GeneType type) const {
    const Gene* gene = findGene(type);
    if (gene) {
        return gene->getPhenotype();
    }

    // Return default value if gene not found
    auto range = getGeneValueRange(type);
    return range.defaultVal;
}

glm::vec3 DiploidGenome::getColor() const {
    return glm::vec3(
        getTrait(GeneType::COLOR_RED),
        getTrait(GeneType::COLOR_GREEN),
        getTrait(GeneType::COLOR_BLUE)
    );
}

std::vector<float> DiploidGenome::getNeuralWeights() const {
    std::vector<float> weights(defaultConfig.neuralWeightCount, 0.0f);

    for (const auto& pair : chromosomePairs) {
        for (const auto& gene : pair.first.getGenes()) {
            if (gene.getType() == GeneType::NEURAL_WEIGHT && gene.getNeuralIndex() >= 0) {
                int idx = gene.getNeuralIndex();
                if (idx < static_cast<int>(weights.size())) {
                    weights[idx] = gene.getPhenotype();
                }
            }
        }
    }

    return weights;
}

MatePreferences DiploidGenome::getMatePreferences() const {
    MatePreferences prefs;
    prefs.sizePreference = getTrait(GeneType::MATE_SIZE_PREF);
    prefs.ornamentPreference = getTrait(GeneType::MATE_ORNAMENT_PREF);
    prefs.similarityPreference = getTrait(GeneType::MATE_SIMILARITY_PREF);
    prefs.choosiness = getTrait(GeneType::CHOOSINESS);
    prefs.minimumAcceptable = prefs.choosiness * 0.5f;  // Scale with choosiness
    return prefs;
}

EcologicalNiche DiploidGenome::getEcologicalNiche() const {
    EcologicalNiche niche;
    niche.dietSpecialization = getTrait(GeneType::DIET_SPECIALIZATION);
    niche.habitatPreference = getTrait(GeneType::HABITAT_PREFERENCE);
    niche.activityTime = getTrait(GeneType::ACTIVITY_TIME);
    return niche;
}

float DiploidGenome::distanceTo(const DiploidGenome& other) const {
    if (chromosomePairs.empty() || other.chromosomePairs.empty()) {
        return 1.0f;
    }

    float totalDistance = 0.0f;
    int comparisons = 0;

    size_t numPairs = std::min(chromosomePairs.size(), other.chromosomePairs.size());

    for (size_t i = 0; i < numPairs; i++) {
        // Compare both chromosomes in each pair
        totalDistance += chromosomePairs[i].first.distanceTo(other.chromosomePairs[i].first);
        totalDistance += chromosomePairs[i].second.distanceTo(other.chromosomePairs[i].second);
        comparisons += 2;
    }

    if (comparisons == 0) return 1.0f;
    return totalDistance / comparisons;
}

float DiploidGenome::calculateInbreedingCoeff() const {
    // Based on proportion of homozygous loci
    int totalGenes = 0;
    int homozygousGenes = 0;

    for (const auto& pair : chromosomePairs) {
        size_t minGenes = std::min(pair.first.getGeneCount(), pair.second.getGeneCount());
        for (size_t i = 0; i < minGenes; i++) {
            const Gene& g1 = pair.first.getGene(i);
            const Gene& g2 = pair.second.getGene(i);

            if (g1.getType() == g2.getType()) {
                totalGenes++;
                // Check if alleles are similar (homozygous)
                float diff = std::abs(g1.getAllele1().getValue() - g2.getAllele1().getValue());
                if (diff < 0.05f) {
                    homozygousGenes++;
                }
            }
        }
    }

    if (totalGenes == 0) return 0.0f;
    return static_cast<float>(homozygousGenes) / totalGenes;
}

float DiploidGenome::getGeneticLoad() const {
    float load = 0.0f;

    for (const auto& pair : chromosomePairs) {
        for (const auto& gene : pair.first.getGenes()) {
            load -= gene.getFitnessEffect();
        }
        for (const auto& gene : pair.second.getGenes()) {
            load -= gene.getFitnessEffect();
        }
    }

    return std::max(0.0f, load);
}

int DiploidGenome::countDeleteriousAlleles() const {
    int count = 0;

    for (const auto& pair : chromosomePairs) {
        for (const auto& gene : pair.first.getGenes()) {
            if (gene.getAllele1().isDeleterious()) count++;
            if (gene.getAllele2().isDeleterious()) count++;
        }
        for (const auto& gene : pair.second.getGenes()) {
            if (gene.getAllele1().isDeleterious()) count++;
            if (gene.getAllele2().isDeleterious()) count++;
        }
    }

    return count;
}

float DiploidGenome::getHeterozygosity() const {
    int totalGenes = 0;
    float totalHet = 0.0f;

    for (const auto& pair : chromosomePairs) {
        for (const auto& gene : pair.first.getGenes()) {
            totalHet += gene.getHeterozygosity();
            totalGenes++;
        }
    }

    if (totalGenes == 0) return 0.0f;
    return totalHet / totalGenes;
}

void DiploidGenome::applyEnvironmentalStress(float stressLevel) {
    if (stressLevel < 0.5f) return;

    // High stress can cause epigenetic changes
    for (auto& pair : chromosomePairs) {
        for (auto& gene : pair.first.getGenes()) {
            if (Random::chance(stressLevel * 0.1f)) {
                EpigeneticMark mark(
                    EpigeneticMarkType::METHYLATION,
                    stressLevel * 0.5f,
                    2,  // Lasts 2 generations
                    true
                );
                gene.addEpigeneticMark(mark);
            }
        }
    }
}

void DiploidGenome::applyNutritionEffect(float nutritionLevel) {
    // Poor nutrition can affect gene expression
    if (nutritionLevel > 0.5f) return;

    float intensity = (0.5f - nutritionLevel) * 2.0f;

    for (auto& pair : chromosomePairs) {
        for (auto& gene : pair.first.getGenes()) {
            // Metabolic and growth genes affected most
            if (gene.getType() == GeneType::SIZE ||
                gene.getType() == GeneType::METABOLIC_RATE ||
                gene.getType() == GeneType::MATURATION_RATE) {
                if (Random::chance(intensity * 0.2f)) {
                    EpigeneticMark mark(
                        EpigeneticMarkType::METHYLATION,
                        intensity * 0.3f,
                        3,
                        true
                    );
                    gene.addEpigeneticMark(mark);
                }
            }
        }
    }
}

void DiploidGenome::updateEpigeneticMarks() {
    for (auto& pair : chromosomePairs) {
        for (auto& gene : pair.first.getGenes()) {
            gene.updateEpigeneticMarks();
        }
        for (auto& gene : pair.second.getGenes()) {
            gene.updateEpigeneticMarks();
        }
    }
}

void DiploidGenome::inheritEpigeneticMarks(const DiploidGenome& parent) {
    // Some epigenetic marks can be inherited
    for (size_t p = 0; p < chromosomePairs.size() && p < parent.chromosomePairs.size(); p++) {
        for (size_t g = 0; g < chromosomePairs[p].first.getGeneCount() &&
                          g < parent.chromosomePairs[p].first.getGeneCount(); g++) {

            const auto& parentMarks = parent.chromosomePairs[p].first.getGene(g).getEpigeneticMarks();
            for (const auto& mark : parentMarks) {
                if (mark.isHeritable && Random::chance(0.3f)) {
                    EpigeneticMark inherited = mark;
                    inherited.intensity *= 0.7f;  // Reduced intensity
                    inherited.generationsRemaining = std::max(1, mark.generationsRemaining - 1);
                    chromosomePairs[p].first.getGene(g).addEpigeneticMark(inherited);
                }
            }
        }
    }
}

const Gene* DiploidGenome::getGene(GeneType type) const {
    return findGene(type);
}

Gene* DiploidGenome::getGene(GeneType type) {
    return findGene(type);
}

const Gene* DiploidGenome::findGene(GeneType type) const {
    for (const auto& pair : chromosomePairs) {
        const Gene* gene = pair.first.getGeneByType(type);
        if (gene) return gene;

        gene = pair.second.getGeneByType(type);
        if (gene) return gene;
    }
    return nullptr;
}

Gene* DiploidGenome::findGene(GeneType type) {
    for (auto& pair : chromosomePairs) {
        Gene* gene = pair.first.getGeneByType(type);
        if (gene) return gene;

        gene = pair.second.getGeneByType(type);
        if (gene) return gene;
    }
    return nullptr;
}

// ============================================
// Phenotype sensory energy cost calculation
// ============================================
float Phenotype::calculateSensoryEnergyCost() const {
    float cost = 0.0f;

    // Vision: highest cost
    cost += (visionFOV / 6.28f) * 0.08f;           // FOV normalization
    cost += (visionRange / 60.0f) * 0.15f;         // Range normalized
    cost += visionAcuity * 0.25f;                  // Acuity is most expensive
    cost += colorPerception * 0.15f;
    cost += motionDetection * 0.12f;

    // Hearing
    cost += (hearingRange / 100.0f) * 0.08f;
    cost += hearingDirectionality * 0.08f;
    cost += echolocationAbility * 0.35f;           // Very expensive!

    // Smell
    cost += (smellRange / 150.0f) * 0.04f;
    cost += smellSensitivity * 0.04f;
    cost += pheromoneProduction * 0.08f;

    // Touch
    cost += (touchRange / 8.0f) * 0.02f;
    cost += vibrationSensitivity * 0.02f;

    // Camouflage
    cost += camouflageLevel * 0.12f;

    // Communication
    cost += alarmCallVolume * 0.05f;

    // Memory
    cost += memoryCapacity * 0.1f;
    cost += memoryRetention * 0.05f;

    return cost;
}

// ============================================
// Express genotype to phenotype (C-15 fix)
// ============================================
Phenotype DiploidGenome::express() const {
    Phenotype p;

    // Physical traits
    p.size = getTrait(GeneType::SIZE);
    p.speed = getTrait(GeneType::SPEED);
    p.visionRange = getTrait(GeneType::VISION_RANGE);
    p.efficiency = getTrait(GeneType::EFFICIENCY);
    p.metabolicRate = getTrait(GeneType::METABOLIC_RATE);
    p.fertility = getTrait(GeneType::FERTILITY);
    p.maturationRate = getTrait(GeneType::MATURATION_RATE);

    // Color
    p.color = getColor();
    p.patternType = getTrait(GeneType::PATTERN_TYPE);
    p.ornamentIntensity = getTrait(GeneType::ORNAMENT_INTENSITY);
    p.displayFrequency = getTrait(GeneType::DISPLAY_FREQUENCY);

    // Behavioral
    p.aggression = getTrait(GeneType::AGGRESSION);
    p.sociality = getTrait(GeneType::SOCIALITY);
    p.curiosity = getTrait(GeneType::CURIOSITY);
    p.fearResponse = getTrait(GeneType::FEAR_RESPONSE);

    // Aptitudes
    p.terrestrialAptitude = getTrait(GeneType::TERRESTRIAL_APTITUDE);
    p.aquaticAptitude = getTrait(GeneType::AQUATIC_APTITUDE);
    p.aerialAptitude = getTrait(GeneType::AERIAL_APTITUDE);

    // Flying traits (only fully expressed if aerial aptitude high)
    p.wingSpan = getTrait(GeneType::WING_SPAN);
    p.flapFrequency = getTrait(GeneType::FLAP_FREQUENCY);
    p.glideRatio = getTrait(GeneType::GLIDE_RATIO);
    p.preferredAltitude = getTrait(GeneType::PREFERRED_ALTITUDE);

    // Aquatic traits
    p.finSize = getTrait(GeneType::FIN_SIZE);
    p.tailSize = getTrait(GeneType::TAIL_SIZE);
    p.swimFrequency = getTrait(GeneType::SWIM_FREQUENCY);
    p.swimAmplitude = getTrait(GeneType::SWIM_AMPLITUDE);
    p.preferredDepth = getTrait(GeneType::PREFERRED_DEPTH);
    p.schoolingStrength = getTrait(GeneType::SCHOOLING_STRENGTH);

    // Sensory - Vision
    p.visionFOV = getTrait(GeneType::VISION_FOV);
    p.visionAcuity = getTrait(GeneType::VISION_ACUITY);
    p.colorPerception = getTrait(GeneType::COLOR_PERCEPTION);
    p.motionDetection = getTrait(GeneType::MOTION_DETECTION);

    // Sensory - Hearing
    p.hearingRange = getTrait(GeneType::HEARING_RANGE);
    p.hearingDirectionality = getTrait(GeneType::HEARING_DIRECTIONALITY);
    p.echolocationAbility = getTrait(GeneType::ECHOLOCATION_ABILITY);

    // Sensory - Smell
    p.smellRange = getTrait(GeneType::SMELL_RANGE);
    p.smellSensitivity = getTrait(GeneType::SMELL_SENSITIVITY);
    p.pheromoneProduction = getTrait(GeneType::PHEROMONE_PRODUCTION);

    // Sensory - Touch
    p.touchRange = getTrait(GeneType::TOUCH_RANGE);
    p.vibrationSensitivity = getTrait(GeneType::VIBRATION_SENSITIVITY);

    // Defense & Communication
    p.camouflageLevel = getTrait(GeneType::CAMOUFLAGE_LEVEL);
    p.alarmCallVolume = getTrait(GeneType::ALARM_CALL_VOLUME);

    // Memory
    p.memoryCapacity = getTrait(GeneType::MEMORY_CAPACITY);
    p.memoryRetention = getTrait(GeneType::MEMORY_RETENTION);

    // Tolerance
    p.heatTolerance = getTrait(GeneType::HEAT_TOLERANCE);
    p.coldTolerance = getTrait(GeneType::COLD_TOLERANCE);

    // Niche
    p.dietSpecialization = getTrait(GeneType::DIET_SPECIALIZATION);
    p.habitatPreference = getTrait(GeneType::HABITAT_PREFERENCE);
    p.activityTime = getTrait(GeneType::ACTIVITY_TIME);

    return p;
}

// ============================================
// Determine creature type from genotype (C-15 fix)
// ============================================
CreatureType DiploidGenome::determineCreatureType() const {
    Phenotype p = express();

    float terrestrial = p.terrestrialAptitude;
    float aquatic = p.aquaticAptitude;
    float aerial = p.aerialAptitude;

    // Highest aptitude wins, with threshold requirements
    // Aerial creatures (flying)
    if (aerial > terrestrial && aerial > aquatic && aerial > 0.6f) {
        // Flying creature - determine subtype
        if (p.aggression > 0.6f && p.size > 0.7f) {
            return CreatureType::AERIAL_PREDATOR;  // Hawk/Eagle analog
        }
        if (p.size < 0.3f || p.flapFrequency > 20.0f) {
            return CreatureType::FLYING_INSECT;    // Insect - small or fast wing beats
        }
        return CreatureType::FLYING_BIRD;          // Generic bird
    }

    // Aquatic creatures (fish, etc.)
    if (aquatic > terrestrial && aquatic > 0.6f) {
        // Aquatic creature - determine subtype
        if (p.aggression > 0.7f && p.size > 1.2f) {
            return CreatureType::AQUATIC_APEX;     // Shark analog
        }
        if (p.aggression > 0.4f) {
            return CreatureType::AQUATIC_PREDATOR; // Predatory fish
        }
        return CreatureType::AQUATIC_HERBIVORE;    // Small fish
    }

    // Amphibian check (moderate in both)
    if (aquatic > 0.4f && terrestrial > 0.4f) {
        return CreatureType::AMPHIBIAN;
    }

    // Terrestrial creatures (land-based)
    // Apex predators - high aggression, larger size
    if (p.aggression > 0.7f && p.size > 1.0f) {
        return CreatureType::APEX_PREDATOR;
    }

    // Small predators - moderate aggression
    if (p.aggression > 0.4f && p.aggression <= 0.7f) {
        return CreatureType::SMALL_PREDATOR;
    }

    // Omnivores - moderate aggression, high sociality
    if (p.aggression > 0.3f && p.sociality > 0.5f) {
        return CreatureType::OMNIVORE;
    }

    // Scavengers - low aggression, high smell sensitivity
    if (p.aggression < 0.3f && p.smellSensitivity > 0.7f) {
        return CreatureType::SCAVENGER;
    }

    // Herbivore subtypes based on diet and behavior
    if (p.sociality > 0.6f) {
        return CreatureType::GRAZER;  // Social herbivore - cow/deer analog
    }

    if (p.size > 1.0f) {
        return CreatureType::BROWSER; // Large solitary herbivore - giraffe analog
    }

    return CreatureType::FRUGIVORE;   // Small herbivore - default
}

// ============================================
// Enhanced fitness calculation (C-16 fix)
// ============================================
float DiploidGenome::calculateFitness(const CreatureStats& stats) const {
    float fitness = 0.0f;

    // ====================================
    // Survival component (40% weight)
    // ====================================
    // Longer survival = higher fitness
    fitness += stats.survivalTime * 0.1f;

    // Current resource levels
    fitness += (stats.currentEnergy / 200.0f) * 10.0f;  // Normalized to max energy
    fitness += (stats.currentHealth / 100.0f) * 10.0f;  // Normalized to max health

    // ====================================
    // Reproduction success (30% weight) - MOST IMPORTANT
    // ====================================
    // Having offspring is the ultimate fitness measure
    fitness += stats.offspringCount * 50.0f;

    // Surviving offspring worth more (indicates good genes AND good environment choice)
    fitness += stats.offspringSurvived * 100.0f;

    // ====================================
    // Resource acquisition (20% weight)
    // ====================================
    fitness += stats.foodEaten * 5.0f;
    fitness += stats.energyEfficiency * 10.0f;

    // ====================================
    // Type-specific bonuses
    // ====================================
    if (isPredator(stats.type)) {
        // Predators get bonus for successful hunts
        fitness += stats.killCount * 30.0f;
        fitness += stats.huntingSuccessRate * 50.0f;
    }

    if (isHerbivore(stats.type)) {
        // Herbivores get bonus for efficient grazing (avoiding predators while eating)
        fitness += stats.foodEaten * 2.0f;  // Extra food bonus
    }

    if (isFlying(stats.type)) {
        // Flying creatures get bonus for mobility (less energy per distance)
        fitness += stats.energyEfficiency * 15.0f;
    }

    if (isAquatic(stats.type)) {
        // Aquatic creatures bonus for schooling behavior survival
        fitness += stats.survivalTime * 0.05f;  // Extra survival bonus
    }

    // ====================================
    // Social success (10% weight)
    // ====================================
    fitness += stats.matingAttempts * 2.0f;
    fitness += stats.matingSuccessRate * 20.0f;

    // ====================================
    // Genetic quality modifiers
    // ====================================
    // Penalize for genetic load (deleterious alleles)
    float geneticLoad = getGeneticLoad();
    fitness *= (1.0f - geneticLoad * 0.2f);

    // Bonus for heterozygosity (genetic diversity = adaptability)
    float heterozygosity = getHeterozygosity();
    fitness *= (1.0f + heterozygosity * 0.1f);

    // Penalty for inbreeding
    float inbreeding = calculateInbreedingCoeff();
    fitness *= (1.0f - inbreeding * 0.3f);

    return std::max(0.0f, fitness);
}

} // namespace genetics
