#include "MutationSystem.h"
#include "../../utils/Random.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <numeric>

namespace genetics {

// =============================================================================
// STATIC MEMBER INITIALIZATION
// =============================================================================

uint64_t Mutation::nextId = 1;

// =============================================================================
// MUTATION IMPLEMENTATION
// =============================================================================

Mutation::Mutation()
    : id(nextId++),
      type(MutationCategory::POINT_MUTATION),
      magnitude(0.0f),
      effect(MutationEffect::NEUTRAL),
      fitnessEffect(0.0f),
      affectedGeneType(GeneType::SIZE),
      originalValue(0.0f),
      newValue(0.0f),
      generationOccurred(0),
      isDominant(false),
      isFixed(false),
      sourceLineageId(0) {}

Mutation::Mutation(MutationCategory t, const MutationLocation& loc, float mag,
                   MutationEffect eff, float fitEffect)
    : id(nextId++),
      type(t),
      location(loc),
      magnitude(mag),
      effect(eff),
      fitnessEffect(fitEffect),
      affectedGeneType(GeneType::SIZE),
      originalValue(0.0f),
      newValue(0.0f),
      generationOccurred(0),
      isDominant(false),
      isFixed(false),
      sourceLineageId(0) {}

// =============================================================================
// MUTATION TRACKER IMPLEMENTATION
// =============================================================================

MutationTracker::MutationTracker()
    : maxHistorySize(100000),
      currentGeneration(0) {}

void MutationTracker::recordMutation(const Mutation& mutation) {
    allMutations.push_back(mutation);

    // Index by gene type
    mutationsByGeneType[mutation.affectedGeneType].push_back(mutation.id);

    // Index by generation
    mutationsByGeneration[mutation.generationOccurred].push_back(mutation.id);

    // Initialize fate tracking if enabled
    MutationFate fate;
    fate.mutationId = mutation.id;
    fate.generationAppeared = mutation.generationOccurred;
    fate.generationLastSeen = mutation.generationOccurred;
    fate.frequencyHistory.push_back(0.0f);  // Initial frequency
    mutationFates[mutation.id] = fate;

    // Prune if necessary
    if (allMutations.size() > maxHistorySize) {
        pruneHistory();
    }
}

void MutationTracker::recordMutations(const std::vector<Mutation>& mutations) {
    for (const auto& mutation : mutations) {
        recordMutation(mutation);
    }
}

std::vector<Mutation> MutationTracker::getBeneficialMutations(int sinceGeneration) const {
    std::vector<Mutation> result;
    for (const auto& mutation : allMutations) {
        if (mutation.effect == MutationEffect::BENEFICIAL &&
            mutation.generationOccurred >= sinceGeneration) {
            result.push_back(mutation);
        }
    }
    return result;
}

std::vector<Mutation> MutationTracker::getDeleteriousMutations(int sinceGeneration) const {
    std::vector<Mutation> result;
    for (const auto& mutation : allMutations) {
        if ((mutation.effect == MutationEffect::DELETERIOUS ||
             mutation.effect == MutationEffect::LETHAL) &&
            mutation.generationOccurred >= sinceGeneration) {
            result.push_back(mutation);
        }
    }
    return result;
}

std::vector<Mutation> MutationTracker::getNeutralMutations(int sinceGeneration) const {
    std::vector<Mutation> result;
    for (const auto& mutation : allMutations) {
        if (mutation.effect == MutationEffect::NEUTRAL &&
            mutation.generationOccurred >= sinceGeneration) {
            result.push_back(mutation);
        }
    }
    return result;
}

std::vector<Mutation> MutationTracker::getMutationsByType(MutationCategory type,
                                                           int sinceGeneration) const {
    std::vector<Mutation> result;
    for (const auto& mutation : allMutations) {
        if (mutation.type == type &&
            mutation.generationOccurred >= sinceGeneration) {
            result.push_back(mutation);
        }
    }
    return result;
}

std::vector<Mutation> MutationTracker::getMutationsForGeneType(GeneType geneType) const {
    std::vector<Mutation> result;
    auto it = mutationsByGeneType.find(geneType);
    if (it != mutationsByGeneType.end()) {
        for (uint64_t id : it->second) {
            for (const auto& mutation : allMutations) {
                if (mutation.id == id) {
                    result.push_back(mutation);
                    break;
                }
            }
        }
    }
    return result;
}

float MutationTracker::getMutationRate(GeneType geneType, int windowGenerations) const {
    if (currentGeneration == 0) return 0.0f;

    int startGen = std::max(0, currentGeneration - windowGenerations);
    int count = 0;

    auto it = mutationsByGeneType.find(geneType);
    if (it != mutationsByGeneType.end()) {
        for (const auto& mutation : allMutations) {
            if (mutation.affectedGeneType == geneType &&
                mutation.generationOccurred >= startGen) {
                count++;
            }
        }
    }

    int generations = currentGeneration - startGen;
    if (generations == 0) return 0.0f;

    return static_cast<float>(count) / generations;
}

float MutationTracker::getOverallMutationRate(int windowGenerations) const {
    if (currentGeneration == 0) return 0.0f;

    int startGen = std::max(0, currentGeneration - windowGenerations);
    int count = 0;

    for (const auto& mutation : allMutations) {
        if (mutation.generationOccurred >= startGen) {
            count++;
        }
    }

    int generations = currentGeneration - startGen;
    if (generations == 0) return 0.0f;

    return static_cast<float>(count) / generations;
}

MutationSpectrum MutationTracker::getMutationSpectrum(int sinceGeneration) const {
    MutationSpectrum spectrum;

    for (const auto& mutation : allMutations) {
        if (mutation.generationOccurred >= sinceGeneration) {
            spectrum.addMutation(mutation);
        }
    }

    return spectrum;
}

MutationSpectrum MutationTracker::getLineageSpectrum(uint64_t lineageId) const {
    MutationSpectrum spectrum;

    for (const auto& mutation : allMutations) {
        if (mutation.sourceLineageId == lineageId) {
            spectrum.addMutation(mutation);
        }
    }

    return spectrum;
}

MutationFate MutationTracker::trackMutationFate(uint64_t mutationId, int generations) const {
    auto it = mutationFates.find(mutationId);
    if (it != mutationFates.end()) {
        return it->second;
    }
    return MutationFate();
}

void MutationTracker::updateMutationFrequencies(int generation,
                                                  const std::map<uint64_t, float>& mutationFrequencies) {
    currentGeneration = generation;

    for (auto& [id, fate] : mutationFates) {
        auto freqIt = mutationFrequencies.find(id);
        if (freqIt != mutationFrequencies.end()) {
            fate.update(generation, freqIt->second);
        } else {
            // Mutation not found in population - may be lost
            fate.update(generation, 0.0f);
        }
    }
}

std::vector<Mutation> MutationTracker::getFixedMutations() const {
    std::vector<Mutation> result;

    for (const auto& [id, fate] : mutationFates) {
        if (fate.isFixed) {
            for (const auto& mutation : allMutations) {
                if (mutation.id == id) {
                    result.push_back(mutation);
                    break;
                }
            }
        }
    }

    return result;
}

std::vector<Mutation> MutationTracker::getLostMutations() const {
    std::vector<Mutation> result;

    for (const auto& [id, fate] : mutationFates) {
        if (fate.isLost) {
            for (const auto& mutation : allMutations) {
                if (mutation.id == id) {
                    result.push_back(mutation);
                    break;
                }
            }
        }
    }

    return result;
}

int MutationTracker::getMutationsInGeneration(int generation) const {
    auto it = mutationsByGeneration.find(generation);
    if (it != mutationsByGeneration.end()) {
        return static_cast<int>(it->second.size());
    }
    return 0;
}

float MutationTracker::getAverageFitnessEffect() const {
    if (allMutations.empty()) return 0.0f;

    float sum = 0.0f;
    for (const auto& mutation : allMutations) {
        sum += mutation.fitnessEffect;
    }
    return sum / static_cast<float>(allMutations.size());
}

float MutationTracker::getFitnessEffectVariance() const {
    if (allMutations.size() < 2) return 0.0f;

    float mean = getAverageFitnessEffect();
    float sumSquares = 0.0f;

    for (const auto& mutation : allMutations) {
        float diff = mutation.fitnessEffect - mean;
        sumSquares += diff * diff;
    }

    return sumSquares / static_cast<float>(allMutations.size() - 1);
}

void MutationTracker::clear() {
    allMutations.clear();
    mutationFates.clear();
    mutationsByGeneType.clear();
    mutationsByGeneration.clear();
}

void MutationTracker::setMaxHistorySize(size_t maxMutations) {
    maxHistorySize = maxMutations;
    if (allMutations.size() > maxHistorySize) {
        pruneHistory();
    }
}

void MutationTracker::exportToCSV(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) return;

    // Header
    file << "ID,Type,Generation,GeneType,Magnitude,Effect,FitnessEffect,"
         << "OriginalValue,NewValue,ChromosomeIndex,GeneIndex,Description\n";

    for (const auto& mutation : allMutations) {
        file << mutation.id << ","
             << MutationSystem::mutationCategoryToString(mutation.type) << ","
             << mutation.generationOccurred << ","
             << geneTypeToString(mutation.affectedGeneType) << ","
             << mutation.magnitude << ","
             << MutationSystem::mutationEffectToString(mutation.effect) << ","
             << mutation.fitnessEffect << ","
             << mutation.originalValue << ","
             << mutation.newValue << ","
             << mutation.location.chromosomeIndex << ","
             << mutation.location.geneIndex << ","
             << "\"" << mutation.description << "\"\n";
    }
}

void MutationTracker::pruneHistory() {
    if (allMutations.size() <= maxHistorySize) return;

    // Remove oldest mutations (keep most recent)
    size_t toRemove = allMutations.size() - maxHistorySize;
    allMutations.erase(allMutations.begin(), allMutations.begin() + toRemove);

    // Rebuild indices
    mutationsByGeneType.clear();
    mutationsByGeneration.clear();

    for (const auto& mutation : allMutations) {
        mutationsByGeneType[mutation.affectedGeneType].push_back(mutation.id);
        mutationsByGeneration[mutation.generationOccurred].push_back(mutation.id);
    }
}

// =============================================================================
// MUTATION EFFECT CALCULATOR IMPLEMENTATION
// =============================================================================

MutationEffectCalculator::MutationEffectCalculator()
    : beneficialProb(0.01f),
      neutralProb(0.30f),
      deleteriousProb(0.69f) {
    initializeTraitFunctions();
}

void MutationEffectCalculator::initializeTraitFunctions() {
    // Define optimal value functions for each trait based on environment

    // Size: optimal depends on predation and resource availability
    traitOptimalFunctions[GeneType::SIZE] = [](float value, const EnvironmentContext& env) {
        // High predation favors smaller size (easier to hide)
        // Low resources favor smaller size (lower metabolic needs)
        float optimalSize = 1.0f - env.predationPressure * 0.3f - (1.0f - env.resourceAvailability) * 0.2f;
        float deviation = std::abs(value - optimalSize);
        return -deviation * 0.5f;  // Penalty for deviation from optimal
    };

    // Speed: optimal depends on predation pressure
    traitOptimalFunctions[GeneType::SPEED] = [](float value, const EnvironmentContext& env) {
        // High predation favors higher speed
        float optimalSpeed = 10.0f + env.predationPressure * 10.0f;
        float normalizedValue = value / 20.0f;  // Normalize to 0-1 range
        float normalizedOptimal = optimalSpeed / 20.0f;
        float deviation = std::abs(normalizedValue - normalizedOptimal);
        return -deviation * 0.3f;
    };

    // Efficiency: always beneficial, but costly at extremes
    traitOptimalFunctions[GeneType::EFFICIENCY] = [](float value, const EnvironmentContext& env) {
        // Higher efficiency is better when resources are scarce
        float benefit = (value - 1.0f) * (1.0f - env.resourceAvailability);
        return benefit * 0.4f;
    };

    // Vision range: depends on habitat openness and predation
    traitOptimalFunctions[GeneType::VISION_RANGE] = [](float value, const EnvironmentContext& env) {
        float optimalRange = 30.0f + env.predationPressure * 20.0f;
        float normalizedValue = value / 50.0f;
        float normalizedOptimal = optimalRange / 50.0f;
        float deviation = std::abs(normalizedValue - normalizedOptimal);
        return -deviation * 0.2f;
    };

    // Temperature tolerance traits
    traitOptimalFunctions[GeneType::HEAT_TOLERANCE] = [](float value, const EnvironmentContext& env) {
        // Higher heat tolerance better when temperature is high
        if (env.temperature > 0.6f) {
            return (value - 0.5f) * (env.temperature - 0.5f) * 0.5f;
        }
        return 0.0f;
    };

    traitOptimalFunctions[GeneType::COLD_TOLERANCE] = [](float value, const EnvironmentContext& env) {
        // Higher cold tolerance better when temperature is low
        if (env.temperature < 0.4f) {
            return (value - 0.5f) * (0.5f - env.temperature) * 0.5f;
        }
        return 0.0f;
    };

    // Aggression: depends on competition level
    traitOptimalFunctions[GeneType::AGGRESSION] = [](float value, const EnvironmentContext& env) {
        float optimalAggression = env.competitionLevel * 0.8f;
        float deviation = std::abs(value - optimalAggression);
        return -deviation * 0.2f;
    };
}

float MutationEffectCalculator::calculateFitnessEffect(const Mutation& mutation,
                                                         const EnvironmentContext& environment) const {
    float baseEffect = mutation.fitnessEffect;

    // Modify based on trait-specific functions
    auto it = traitOptimalFunctions.find(mutation.affectedGeneType);
    if (it != traitOptimalFunctions.end()) {
        float traitEffect = it->second(mutation.newValue, environment);
        baseEffect += traitEffect;
    }

    // Environmental stress can amplify effects
    float stressLevel = environment.getStressLevel();
    if (mutation.effect == MutationEffect::DELETERIOUS) {
        // Deleterious mutations are worse under stress
        baseEffect *= (1.0f + stressLevel * 0.5f);
    } else if (mutation.effect == MutationEffect::BENEFICIAL) {
        // Beneficial mutations may be more valuable under stress
        baseEffect *= (1.0f + stressLevel * 0.3f);
    }

    return std::max(-1.0f, std::min(1.0f, baseEffect));
}

bool MutationEffectCalculator::isPositiveInContext(const Mutation& mutation,
                                                    const EcologicalNiche& niche) const {
    // Create an environment context from the niche
    EnvironmentContext env;
    env.resourceAvailability = 1.0f - niche.dietSpecialization;  // Specialists need specific resources
    env.habitatStability = 0.5f;

    // Activity time affects optimal traits
    // Nocturnal creatures might benefit from different sensory mutations
    if (mutation.affectedGeneType == GeneType::VISION_ACUITY ||
        mutation.affectedGeneType == GeneType::VISION_RANGE) {
        // Diurnal creatures benefit more from vision improvements
        if (niche.activityTime > 0.5f) {
            return mutation.newValue > mutation.originalValue;
        }
    }

    if (mutation.affectedGeneType == GeneType::HEARING_RANGE ||
        mutation.affectedGeneType == GeneType::ECHOLOCATION_ABILITY) {
        // Nocturnal creatures benefit more from hearing improvements
        if (niche.activityTime < 0.5f) {
            return mutation.newValue > mutation.originalValue;
        }
    }

    // General case: check fitness effect
    float effect = calculateFitnessEffect(mutation, env);
    return effect > 0.0f;
}

MutationEffect MutationEffectCalculator::classifyEffect(const Mutation& mutation,
                                                          const EnvironmentContext& environment) const {
    float effect = calculateFitnessEffect(mutation, environment);

    if (effect <= -0.5f) return MutationEffect::LETHAL;
    if (effect < -0.01f) return MutationEffect::DELETERIOUS;
    if (effect > 0.01f) return MutationEffect::BENEFICIAL;
    return MutationEffect::NEUTRAL;
}

float MutationEffectCalculator::calculateEpistaticModifier(
    const Mutation& newMutation,
    const std::vector<Mutation>& existingMutations) const {

    float modifier = 1.0f;

    for (const auto& existing : existingMutations) {
        // Same gene type - may have diminishing returns
        if (existing.affectedGeneType == newMutation.affectedGeneType) {
            // Multiple mutations in same gene often have reduced combined effect
            modifier *= 0.8f;
        }

        // Opposite effects may partially cancel
        if (existing.effect == MutationEffect::BENEFICIAL &&
            newMutation.effect == MutationEffect::DELETERIOUS) {
            modifier *= 0.9f;  // Slight buffering
        }

        // Check for functional relationships
        // Size and speed mutations may interact
        if ((existing.affectedGeneType == GeneType::SIZE &&
             newMutation.affectedGeneType == GeneType::SPEED) ||
            (existing.affectedGeneType == GeneType::SPEED &&
             newMutation.affectedGeneType == GeneType::SIZE)) {
            // Size affects speed - epistatic interaction
            modifier *= 1.1f;  // Slight synergy or antagonism
        }

        // Metabolic rate interactions
        if (existing.affectedGeneType == GeneType::METABOLIC_RATE ||
            newMutation.affectedGeneType == GeneType::METABOLIC_RATE) {
            if (existing.affectedGeneType == GeneType::EFFICIENCY ||
                newMutation.affectedGeneType == GeneType::EFFICIENCY) {
                modifier *= 0.85f;  // Trade-off interaction
            }
        }
    }

    return modifier;
}

bool MutationEffectCalculator::checkSyntheticLethality(
    const std::vector<Mutation>& mutations) const {

    // Check for combinations that are lethal together
    int deleteriousCount = 0;
    bool hasMetabolicMutation = false;
    bool hasSizeMutation = false;

    for (const auto& mutation : mutations) {
        if (mutation.effect == MutationEffect::DELETERIOUS) {
            deleteriousCount++;
        }
        if (mutation.affectedGeneType == GeneType::METABOLIC_RATE) {
            hasMetabolicMutation = true;
        }
        if (mutation.affectedGeneType == GeneType::SIZE) {
            hasSizeMutation = true;
        }
    }

    // Multiple severe deleterious mutations
    if (deleteriousCount >= 3) {
        return Random::chance(0.3f);  // High chance of synthetic lethality
    }

    // Specific lethal combinations
    if (hasMetabolicMutation && hasSizeMutation) {
        // Check if both are extreme
        for (const auto& m1 : mutations) {
            if (m1.affectedGeneType == GeneType::METABOLIC_RATE && m1.newValue > 1.8f) {
                for (const auto& m2 : mutations) {
                    if (m2.affectedGeneType == GeneType::SIZE && m2.newValue > 1.8f) {
                        return true;  // High metabolism + large size = unsustainable
                    }
                }
            }
        }
    }

    return false;
}

float MutationEffectCalculator::calculateCompensation(const Mutation& deleterious,
                                                        const Mutation& compensatory) const {
    // Check if compensatory mutation can offset deleterious one

    // Same gene type - direct compensation possible
    if (deleterious.affectedGeneType == compensatory.affectedGeneType) {
        // Check if effects are opposite
        if (deleterious.newValue < deleterious.originalValue &&
            compensatory.newValue > compensatory.originalValue) {
            float compensation = std::min(1.0f,
                std::abs(compensatory.newValue - compensatory.originalValue) /
                std::abs(deleterious.originalValue - deleterious.newValue));
            return compensation;
        }
    }

    // Functional compensation - different genes that affect same pathway
    // Speed can compensate for reduced vision range (faster escape)
    if (deleterious.affectedGeneType == GeneType::VISION_RANGE &&
        compensatory.affectedGeneType == GeneType::SPEED) {
        if (compensatory.newValue > compensatory.originalValue) {
            return 0.5f;  // Partial compensation
        }
    }

    // Efficiency can compensate for metabolic rate changes
    if (deleterious.affectedGeneType == GeneType::METABOLIC_RATE &&
        compensatory.affectedGeneType == GeneType::EFFICIENCY) {
        if (compensatory.effect == MutationEffect::BENEFICIAL) {
            return 0.6f;
        }
    }

    return 0.0f;
}

float MutationEffectCalculator::getTraitFitnessEffect(GeneType trait, float oldValue,
                                                        float newValue,
                                                        const EnvironmentContext& environment) const {
    auto it = traitOptimalFunctions.find(trait);
    if (it != traitOptimalFunctions.end()) {
        float oldEffect = it->second(oldValue, environment);
        float newEffect = it->second(newValue, environment);
        return newEffect - oldEffect;
    }

    // Default: small changes are neutral, large changes may be deleterious
    float change = std::abs(newValue - oldValue);
    return -change * 0.1f;
}

void MutationEffectCalculator::setEffectDistribution(float beneficial, float neutral,
                                                       float deleterious) {
    float total = beneficial + neutral + deleterious;
    beneficialProb = beneficial / total;
    neutralProb = neutral / total;
    deleteriousProb = deleterious / total;
}

// =============================================================================
// MUTATION SYSTEM IMPLEMENTATION
// =============================================================================

MutationSystem::MutationSystem()
    : currentGeneration(0) {
    initializeDefaultHotspots();
}

MutationSystem::MutationSystem(const MutationConfig& cfg)
    : config(cfg),
      currentGeneration(0) {
    initializeDefaultHotspots();
}

void MutationSystem::initializeDefaultHotspots() {
    // Add some biologically-inspired default hotspots

    // Repetitive sequence hotspot in behavioral traits
    MutationHotspot behavioralHotspot(
        MutationLocation(2, 0),  // Chromosome 2 (behavioral)
        2.0f,  // 2x mutation rate
        MutationCategory::POINT_MUTATION,
        HotspotReason::REPETITIVE_SEQUENCE,
        4,  // Covers 4 genes
        "Behavioral trait repetitive region"
    );
    hotspots.push_back(behavioralHotspot);

    // Fragile site in sensory chromosome
    MutationHotspot sensoryHotspot(
        MutationLocation(7, 5),  // Chromosome 7 (sensory), middle region
        1.5f,
        MutationCategory::DUPLICATION,
        HotspotReason::FRAGILE_SITE,
        2,
        "Sensory gene fragile site"
    );
    hotspots.push_back(sensoryHotspot);

    // Transcription-coupled hotspot in frequently expressed genes
    MutationHotspot metabolicHotspot(
        MutationLocation(0, 4),  // Chromosome 0 (physical), metabolic genes
        1.3f,
        MutationCategory::REGULATORY,
        HotspotReason::TRANSCRIPTION_COUPLED,
        3,
        "High-expression metabolic region"
    );
    hotspots.push_back(metabolicHotspot);
}

std::vector<Mutation> MutationSystem::mutateWithTypes(DiploidGenome& genome,
                                                        const MutationRateModifiers& modifiers) {
    std::vector<Mutation> mutations;

    float effectiveRate = config.baseMutationRate * modifiers.getClampedModifier();

    // Iterate through all chromosome pairs
    for (size_t chromIdx = 0; chromIdx < genome.getChromosomeCount(); chromIdx++) {
        auto& pair = genome.getChromosomePair(chromIdx);

        // Process maternal chromosome
        for (size_t geneIdx = 0; geneIdx < pair.first.getGeneCount(); geneIdx++) {
            MutationLocation loc(static_cast<int>(chromIdx), static_cast<int>(geneIdx), 0, true);
            float localRate = getEffectiveMutationRate(loc, effectiveRate);

            if (Random::chance(localRate)) {
                MutationCategory type = selectMutationType();
                Mutation mutation = applyMutationOfType(type, pair.first, geneIdx, loc);

                if (mutation.id != 0) {  // Valid mutation occurred
                    mutation.generationOccurred = currentGeneration;
                    mutation.effect = determineMutationEffect(mutation);
                    mutation.fitnessEffect = generateFitnessEffect(mutation.effect);
                    mutations.push_back(mutation);
                }
            }
        }

        // Process paternal chromosome
        for (size_t geneIdx = 0; geneIdx < pair.second.getGeneCount(); geneIdx++) {
            MutationLocation loc(static_cast<int>(chromIdx), static_cast<int>(geneIdx), 0, false);
            float localRate = getEffectiveMutationRate(loc, effectiveRate);

            if (Random::chance(localRate)) {
                MutationCategory type = selectMutationType();
                Mutation mutation = applyMutationOfType(type, pair.second, geneIdx, loc);

                if (mutation.id != 0) {
                    mutation.generationOccurred = currentGeneration;
                    mutation.effect = determineMutationEffect(mutation);
                    mutation.fitnessEffect = generateFitnessEffect(mutation.effect);
                    mutations.push_back(mutation);
                }
            }
        }
    }

    // Record mutations if tracking enabled
    if (config.trackMutationFates) {
        tracker.recordMutations(mutations);
    }

    return mutations;
}

// Helper method to apply mutation of specific type
Mutation MutationSystem::applyMutationOfType(MutationCategory type, Chromosome& chromosome,
                                               size_t geneIdx, const MutationLocation& loc) {
    switch (type) {
        case MutationCategory::POINT_MUTATION:
            return applyPointMutation(chromosome.getGene(geneIdx));

        case MutationCategory::DUPLICATION:
            return applyDuplication(chromosome, geneIdx);

        case MutationCategory::DELETION:
            if (chromosome.getGeneCount() > 3) {  // Don't delete if too few genes
                return applyDeletion(chromosome, geneIdx);
            }
            break;

        case MutationCategory::INVERSION:
            if (geneIdx + 1 < chromosome.getGeneCount()) {
                size_t endIdx = std::min(geneIdx + 3, chromosome.getGeneCount() - 1);
                return applyInversion(chromosome, geneIdx, endIdx);
            }
            break;

        case MutationCategory::REGULATORY:
            return applyRegulatory(chromosome.getGene(geneIdx));

        case MutationCategory::WHOLE_GENE_DUPLICATION:
            return applyWholeGeneDuplication(chromosome, geneIdx);

        case MutationCategory::FRAMESHIFT:
            return applyFrameshift(chromosome, geneIdx);

        default:
            break;
    }

    return Mutation();  // Empty mutation if type not applicable
}

std::vector<Mutation> MutationSystem::mutate(DiploidGenome& genome) {
    return mutateWithTypes(genome, MutationRateModifiers());
}

Mutation MutationSystem::applyPointMutation(Gene& gene, float strength) {
    float mutationStrength = (strength < 0) ? config.pointMutationStrength : strength;

    Mutation mutation;
    mutation.type = MutationCategory::POINT_MUTATION;
    mutation.affectedGeneType = gene.getType();
    mutation.originalValue = gene.getAllele1().getValue();

    // Apply mutation to allele
    gene.mutate(mutationStrength);

    mutation.newValue = gene.getAllele1().getValue();
    mutation.magnitude = std::abs(mutation.newValue - mutation.originalValue);

    // Generate description
    std::stringstream ss;
    ss << "Point mutation in " << geneTypeToString(gene.getType())
       << ": " << mutation.originalValue << " -> " << mutation.newValue;
    mutation.description = ss.str();

    return mutation;
}

Mutation MutationSystem::applyDuplication(Chromosome& chromosome, size_t geneIndex) {
    if (geneIndex >= chromosome.getGeneCount()) return Mutation();

    const Gene& originalGene = chromosome.getGene(geneIndex);

    Mutation mutation;
    mutation.type = MutationCategory::DUPLICATION;
    mutation.affectedGeneType = originalGene.getType();
    mutation.originalValue = 1.0f;  // One copy
    mutation.magnitude = 1.0f;  // One additional copy

    // Apply duplication
    chromosome.applyDuplication(geneIndex);

    mutation.newValue = 2.0f;  // Now two copies

    std::stringstream ss;
    ss << "Gene duplication of " << geneTypeToString(originalGene.getType());
    mutation.description = ss.str();

    return mutation;
}

Mutation MutationSystem::applyDeletion(Chromosome& chromosome, size_t geneIndex) {
    if (geneIndex >= chromosome.getGeneCount()) return Mutation();
    if (chromosome.getGeneCount() <= 3) return Mutation();  // Safety check

    const Gene& deletedGene = chromosome.getGene(geneIndex);

    Mutation mutation;
    mutation.type = MutationCategory::DELETION;
    mutation.affectedGeneType = deletedGene.getType();
    mutation.originalValue = deletedGene.getPhenotype();
    mutation.magnitude = 1.0f;  // One gene deleted

    std::stringstream ss;
    ss << "Gene deletion of " << geneTypeToString(deletedGene.getType());
    mutation.description = ss.str();

    // Apply deletion
    chromosome.applyDeletion(geneIndex);

    mutation.newValue = 0.0f;  // Gene now absent

    return mutation;
}

Mutation MutationSystem::applyInversion(Chromosome& chromosome, size_t startIndex, size_t endIndex) {
    if (startIndex >= endIndex) return Mutation();
    if (endIndex >= chromosome.getGeneCount()) return Mutation();

    Mutation mutation;
    mutation.type = MutationCategory::INVERSION;
    mutation.magnitude = static_cast<float>(endIndex - startIndex + 1);

    // Get affected gene types for description
    std::stringstream ss;
    ss << "Inversion of genes " << startIndex << "-" << endIndex;
    mutation.description = ss.str();

    // Store first gene type as affected
    mutation.affectedGeneType = chromosome.getGene(startIndex).getType();

    // Apply inversion
    chromosome.applyInversion(startIndex, endIndex);

    return mutation;
}

Mutation MutationSystem::applyRegulatory(Gene& gene, float expressionChange) {
    float change = (expressionChange < 0) ?
        Random::range(-0.2f, 0.2f) : expressionChange;

    Mutation mutation;
    mutation.type = MutationCategory::REGULATORY;
    mutation.affectedGeneType = gene.getType();
    mutation.originalValue = gene.getExpressionLevel();

    // Modify expression level
    float newExpression = std::clamp(gene.getExpressionLevel() + change, 0.0f, 2.0f);
    gene.setExpressionLevel(newExpression);

    mutation.newValue = newExpression;
    mutation.magnitude = std::abs(change);

    std::stringstream ss;
    ss << "Regulatory mutation in " << geneTypeToString(gene.getType())
       << ": expression " << mutation.originalValue << " -> " << mutation.newValue;
    mutation.description = ss.str();

    return mutation;
}

Mutation MutationSystem::applyTranslocation(DiploidGenome& genome, size_t sourceChrom,
                                              size_t targetChrom, size_t geneIndex) {
    if (sourceChrom >= genome.getChromosomeCount()) return Mutation();
    if (targetChrom >= genome.getChromosomeCount()) return Mutation();
    if (sourceChrom == targetChrom) return Mutation();

    auto& sourcePair = genome.getChromosomePair(sourceChrom);
    auto& targetPair = genome.getChromosomePair(targetChrom);

    if (geneIndex >= sourcePair.first.getGeneCount()) return Mutation();

    const Gene& movedGene = sourcePair.first.getGene(geneIndex);

    Mutation mutation;
    mutation.type = MutationCategory::TRANSLOCATION;
    mutation.affectedGeneType = movedGene.getType();
    mutation.location = MutationLocation(static_cast<int>(sourceChrom),
                                          static_cast<int>(geneIndex));

    // Copy gene to target
    targetPair.first.addGene(movedGene);

    // Remove from source (if safe)
    if (sourcePair.first.getGeneCount() > 3) {
        sourcePair.first.applyDeletion(geneIndex);
        mutation.magnitude = 1.0f;
    }

    std::stringstream ss;
    ss << "Translocation of " << geneTypeToString(movedGene.getType())
       << " from chromosome " << sourceChrom << " to " << targetChrom;
    mutation.description = ss.str();

    return mutation;
}

Mutation MutationSystem::applyWholeGeneDuplication(Chromosome& chromosome, size_t geneIndex) {
    if (geneIndex >= chromosome.getGeneCount()) return Mutation();

    const Gene& originalGene = chromosome.getGene(geneIndex);

    Mutation mutation;
    mutation.type = MutationCategory::WHOLE_GENE_DUPLICATION;
    mutation.affectedGeneType = originalGene.getType();
    mutation.originalValue = 1.0f;

    // Create complete copy with regulatory elements intact
    Gene duplicatedGene = originalGene;

    // Apply small variation to distinguish copy
    float variation = Random::range(-0.05f, 0.05f);
    Allele modifiedAllele = duplicatedGene.getAllele1();
    modifiedAllele.setValue(modifiedAllele.getValue() + variation);
    duplicatedGene.setAllele1(modifiedAllele);

    chromosome.addGene(duplicatedGene);

    mutation.newValue = 2.0f;
    mutation.magnitude = 1.0f;

    std::stringstream ss;
    ss << "Whole gene duplication of " << geneTypeToString(originalGene.getType());
    mutation.description = ss.str();

    return mutation;
}

Mutation MutationSystem::applyFrameshift(Chromosome& chromosome, size_t geneIndex) {
    if (geneIndex >= chromosome.getGeneCount()) return Mutation();

    Mutation mutation;
    mutation.type = MutationCategory::FRAMESHIFT;

    // Frameshift affects multiple downstream genes
    int affectedCount = 0;
    for (size_t i = geneIndex; i < std::min(geneIndex + 5, chromosome.getGeneCount()); i++) {
        Gene& gene = chromosome.getGene(i);

        // Severe disruption to gene function
        float originalValue = gene.getAllele1().getValue();
        float disruption = Random::range(-0.5f, 0.5f);

        Allele disruptedAllele = gene.getAllele1();
        GeneValueRange range = getGeneValueRange(gene.getType());
        float newValue = std::clamp(originalValue + disruption, range.min, range.max);
        disruptedAllele.setValue(newValue);
        disruptedAllele.setDeleterious(true);
        gene.setAllele1(disruptedAllele);

        if (i == geneIndex) {
            mutation.affectedGeneType = gene.getType();
            mutation.originalValue = originalValue;
            mutation.newValue = newValue;
        }

        affectedCount++;
    }

    mutation.magnitude = static_cast<float>(affectedCount);

    std::stringstream ss;
    ss << "Frameshift mutation affecting " << affectedCount << " genes starting at position " << geneIndex;
    mutation.description = ss.str();

    return mutation;
}

void MutationSystem::addHotspot(const MutationHotspot& hotspot) {
    hotspots.push_back(hotspot);
}

void MutationSystem::removeHotspot(const MutationLocation& location) {
    hotspots.erase(
        std::remove_if(hotspots.begin(), hotspots.end(),
            [&location](const MutationHotspot& h) {
                return h.location == location;
            }),
        hotspots.end()
    );
}

const MutationHotspot* MutationSystem::getHotspotAt(const MutationLocation& location) const {
    for (const auto& hotspot : hotspots) {
        if (hotspot.isActive && hotspot.containsLocation(location)) {
            return &hotspot;
        }
    }
    return nullptr;
}

float MutationSystem::getEffectiveMutationRate(const MutationLocation& location,
                                                 float baseRate) const {
    if (!config.enableHotspots) return baseRate;

    const MutationHotspot* hotspot = getHotspotAt(location);
    if (hotspot) {
        return baseRate * hotspot->mutationRateMultiplier;
    }
    return baseRate;
}

float MutationSystem::calculateStressModifier(float stressLevel) const {
    if (!config.enableStressInducedMutagenesis) return 1.0f;

    // Stress-induced mutagenesis: mutation rate increases under stress
    // Based on SOS response in bacteria and similar mechanisms
    if (stressLevel < 0.3f) return 1.0f;  // Low stress - no effect
    if (stressLevel < 0.5f) return 1.0f + (stressLevel - 0.3f);  // Moderate increase
    if (stressLevel < 0.8f) return 1.2f + (stressLevel - 0.5f) * 2.0f;  // Higher increase
    return 2.0f + (stressLevel - 0.8f) * 5.0f;  // Severe stress - major increase
}

float MutationSystem::calculateDNARepairModifier(float repairEfficiency) const {
    // Better repair = lower mutation rate
    // repairEfficiency 0.0 = no repair, 1.0 = perfect repair
    return 2.0f - repairEfficiency * 1.5f;  // Range: 0.5 to 2.0
}

float MutationSystem::calculateMutatorModifier(float mutatorStrength) const {
    // Mutator alleles increase genome-wide mutation rate
    // mutatorStrength 0.0 = no mutator, 1.0 = strong mutator
    return 1.0f + mutatorStrength * 5.0f;  // Range: 1.0 to 6.0
}

float MutationSystem::calculateFitnessEffect(const Mutation& mutation,
                                               const EnvironmentContext& env) const {
    return effectCalculator.calculateFitnessEffect(mutation, env);
}

void MutationSystem::setCurrentGeneration(int generation) {
    currentGeneration = generation;
    tracker.setCurrentGeneration(generation);
}

MutationCategory MutationSystem::selectMutationType() const {
    float rand = Random::value();
    float cumulative = 0.0f;

    cumulative += config.pointMutationProb;
    if (rand < cumulative) return MutationCategory::POINT_MUTATION;

    cumulative += config.duplicationProb;
    if (rand < cumulative) return MutationCategory::DUPLICATION;

    cumulative += config.deletionProb;
    if (rand < cumulative) return MutationCategory::DELETION;

    cumulative += config.inversionProb;
    if (rand < cumulative) return MutationCategory::INVERSION;

    cumulative += config.translocationProb;
    if (rand < cumulative) return MutationCategory::TRANSLOCATION;

    cumulative += config.regulatoryProb;
    if (rand < cumulative) return MutationCategory::REGULATORY;

    cumulative += config.wholeGeneDupProb;
    if (rand < cumulative) return MutationCategory::WHOLE_GENE_DUPLICATION;

    cumulative += config.frameshiftProb;
    if (rand < cumulative) return MutationCategory::FRAMESHIFT;

    // Default
    return MutationCategory::POINT_MUTATION;
}

MutationEffect MutationSystem::determineMutationEffect(const Mutation& mutation) const {
    // Frameshift and deletion are usually deleterious
    if (mutation.type == MutationCategory::FRAMESHIFT) {
        return Random::chance(0.9f) ? MutationEffect::DELETERIOUS : MutationEffect::LETHAL;
    }

    if (mutation.type == MutationCategory::DELETION) {
        float roll = Random::value();
        if (roll < 0.7f) return MutationEffect::DELETERIOUS;
        if (roll < 0.8f) return MutationEffect::LETHAL;
        return MutationEffect::NEUTRAL;
    }

    // Duplications are often neutral or slightly deleterious
    if (mutation.type == MutationCategory::DUPLICATION ||
        mutation.type == MutationCategory::WHOLE_GENE_DUPLICATION) {
        float roll = Random::value();
        if (roll < 0.5f) return MutationEffect::NEUTRAL;
        if (roll < 0.8f) return MutationEffect::DELETERIOUS;
        return MutationEffect::BENEFICIAL;  // Rare beneficial duplication
    }

    // Standard distribution for other types
    float roll = Random::value();
    float cumulative = 0.0f;

    cumulative += config.lethalProb;
    if (roll < cumulative) return MutationEffect::LETHAL;

    cumulative += config.deleteriousProb;
    if (roll < cumulative) return MutationEffect::DELETERIOUS;

    cumulative += config.neutralProb;
    if (roll < cumulative) return MutationEffect::NEUTRAL;

    return MutationEffect::BENEFICIAL;
}

float MutationSystem::generateFitnessEffect(MutationEffect effect) const {
    switch (effect) {
        case MutationEffect::BENEFICIAL:
            return config.avgBeneficialEffect +
                   Random::range(-config.fitnessEffectVariance, config.fitnessEffectVariance);

        case MutationEffect::DELETERIOUS:
            return config.avgDeleteriousEffect +
                   Random::range(-config.fitnessEffectVariance, config.fitnessEffectVariance);

        case MutationEffect::LETHAL:
            return -1.0f;

        case MutationEffect::NEUTRAL:
        default:
            return Random::range(-0.01f, 0.01f);  // Very small effect
    }
}

} // namespace genetics
