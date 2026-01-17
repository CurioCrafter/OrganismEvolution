/**
 * @file Coevolution.cpp
 * @brief Implementation of coevolutionary dynamics tracking system
 *
 * This file implements the CoevolutionTracker class and related functions
 * for monitoring and analyzing coevolutionary relationships between species.
 *
 * @author OrganismEvolution Development Team
 * @date 2025
 */

#include "Coevolution.h"
#include "Species.h"
#include "../Creature.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>
#include <numeric>

namespace genetics {

// =============================================================================
// UTILITY FUNCTIONS IMPLEMENTATION
// =============================================================================

const char* coevolutionTypeToString(CoevolutionType type) {
    switch (type) {
        case CoevolutionType::PREDATOR_PREY:    return "Predator-Prey";
        case CoevolutionType::POLLINATOR_PLANT: return "Pollinator-Plant";
        case CoevolutionType::PARASITE_HOST:    return "Parasite-Host";
        case CoevolutionType::MIMICRY:          return "Mimicry";
        case CoevolutionType::MUTUALISM:        return "Mutualism";
        case CoevolutionType::COMPETITION:      return "Competition";
        default:                                return "Unknown";
    }
}

const char* mimicryTypeToString(MimicryType type) {
    switch (type) {
        case MimicryType::BATESIAN:  return "Batesian";
        case MimicryType::MULLERIAN: return "Mullerian";
        default:                     return "Unknown";
    }
}

const char* advantageSideToString(AdvantageSide side) {
    switch (side) {
        case AdvantageSide::NEUTRAL:     return "Neutral";
        case AdvantageSide::SPECIES_1:   return "Species 1";
        case AdvantageSide::SPECIES_2:   return "Species 2";
        case AdvantageSide::OSCILLATING: return "Oscillating";
        default:                         return "Unknown";
    }
}

// =============================================================================
// COEVOLUTION TRACKER IMPLEMENTATION
// =============================================================================

CoevolutionTracker::CoevolutionTracker()
    : lastUpdateGeneration(0) {
}

CoevolutionTracker::CoevolutionTracker(const CoevolutionConfig& cfg)
    : config(cfg)
    , lastUpdateGeneration(0) {
}

// =============================================================================
// PRIVATE HELPER METHODS
// =============================================================================

std::pair<SpeciesId, SpeciesId> CoevolutionTracker::makeOrderedPair(SpeciesId s1, SpeciesId s2) const {
    return (s1 < s2) ? std::make_pair(s1, s2) : std::make_pair(s2, s1);
}

float CoevolutionTracker::calculatePearsonCorrelation(const std::deque<float>& x,
                                                       const std::deque<float>& y) const {
    if (x.size() != y.size() || x.size() < 3) {
        return 0.0f;
    }

    size_t n = x.size();

    // Calculate means
    float meanX = 0.0f, meanY = 0.0f;
    for (size_t i = 0; i < n; ++i) {
        meanX += x[i];
        meanY += y[i];
    }
    meanX /= n;
    meanY /= n;

    // Calculate correlation using Pearson formula:
    // r = sum((xi - x_mean)(yi - y_mean)) / sqrt(sum((xi - x_mean)^2) * sum((yi - y_mean)^2))
    float numerator = 0.0f;
    float sumSqX = 0.0f;
    float sumSqY = 0.0f;

    for (size_t i = 0; i < n; ++i) {
        float dx = x[i] - meanX;
        float dy = y[i] - meanY;
        numerator += dx * dy;
        sumSqX += dx * dx;
        sumSqY += dy * dy;
    }

    float denominator = std::sqrt(sumSqX * sumSqY);
    if (denominator < 1e-10f) {
        return 0.0f;
    }

    return numerator / denominator;
}

PredatorTraits CoevolutionTracker::extractPredatorTraits(const std::vector<Creature*>& creatures) const {
    PredatorTraits traits;

    if (creatures.empty()) {
        return traits;
    }

    float sumSpeed = 0.0f;
    float sumVenom = 0.0f;
    float sumStealth = 0.0f;
    float sumAttack = 0.0f;
    float sumSense = 0.0f;
    float sumStamina = 0.0f;

    for (const Creature* c : creatures) {
        if (!c || !c->isAlive()) continue;

        const DiploidGenome& genome = c->getDiploidGenome();
        Phenotype pheno = genome.express();

        sumSpeed += pheno.speed / 20.0f;  // Normalize to 0-1 range
        sumStealth += pheno.camouflageLevel;
        sumAttack += pheno.aggression;
        sumSense += pheno.visionAcuity;
        sumStamina += pheno.efficiency;

        // Venom approximated from aggression and toxicity-like traits
        sumVenom += pheno.aggression * 0.5f;
    }

    float count = static_cast<float>(creatures.size());
    traits.speed = sumSpeed / count;
    traits.venom = sumVenom / count;
    traits.stealth = sumStealth / count;
    traits.attackPower = sumAttack / count;
    traits.senseAcuity = sumSense / count;
    traits.stamina = sumStamina / count;

    return traits;
}

PreyTraits CoevolutionTracker::extractPreyTraits(const std::vector<Creature*>& creatures) const {
    PreyTraits traits;

    if (creatures.empty()) {
        return traits;
    }

    float sumSpeed = 0.0f;
    float sumArmor = 0.0f;
    float sumDetection = 0.0f;
    float sumCamo = 0.0f;
    float sumToxicity = 0.0f;
    float sumAgility = 0.0f;

    for (const Creature* c : creatures) {
        if (!c || !c->isAlive()) continue;

        const DiploidGenome& genome = c->getDiploidGenome();
        Phenotype pheno = genome.express();

        sumSpeed += pheno.speed / 20.0f;
        sumArmor += pheno.size * 0.3f;  // Larger creatures have more armor
        sumDetection += pheno.visionAcuity;
        sumCamo += pheno.camouflageLevel;
        sumAgility += (pheno.speed / 20.0f) * pheno.efficiency;

        // Toxicity approximated from ornament intensity (warning coloration)
        sumToxicity += pheno.ornamentIntensity * 0.5f;
    }

    float count = static_cast<float>(creatures.size());
    traits.speed = sumSpeed / count;
    traits.armor = sumArmor / count;
    traits.detection = sumDetection / count;
    traits.camouflage = sumCamo / count;
    traits.toxicity = sumToxicity / count;
    traits.agility = sumAgility / count;

    return traits;
}

void CoevolutionTracker::recordTraitValue(SpeciesId speciesId, GeneType trait, float value) {
    auto key = std::make_pair(speciesId, trait);
    auto& history = traitHistories[key];

    history.push_back(value);
    if (history.size() > static_cast<size_t>(config.historyLength)) {
        history.pop_front();
    }
}

void CoevolutionTracker::pruneHistory() {
    size_t maxSize = static_cast<size_t>(config.historyLength);

    for (auto& [key, history] : traitHistories) {
        while (history.size() > maxSize) {
            history.pop_front();
        }
    }
}

// =============================================================================
// PAIR DETECTION METHODS
// =============================================================================

bool CoevolutionTracker::detectCoevolutionaryPair(const Species* species1,
                                                   const Species* species2) {
    if (!species1 || !species2) {
        return false;
    }

    if (species1->isExtinct() || species2->isExtinct()) {
        return false;
    }

    SpeciesId id1 = species1->getId();
    SpeciesId id2 = species2->getId();

    // Check if already tracked
    auto pairKey = makeOrderedPair(id1, id2);
    if (coevolutionaryPairs.find(pairKey) != coevolutionaryPairs.end()) {
        return true;  // Already detected
    }

    // Check for correlated trait changes
    // We look at speed, size, and vision traits for correlation
    std::vector<GeneType> traitsToCheck = {
        GeneType::SPEED, GeneType::SIZE, GeneType::VISION_RANGE,
        GeneType::AGGRESSION, GeneType::CAMOUFLAGE_LEVEL
    };

    int significantCorrelations = 0;

    for (GeneType trait : traitsToCheck) {
        auto key1 = std::make_pair(id1, trait);
        auto key2 = std::make_pair(id2, trait);

        auto it1 = traitHistories.find(key1);
        auto it2 = traitHistories.find(key2);

        if (it1 != traitHistories.end() && it2 != traitHistories.end()) {
            float corr = std::abs(calculatePearsonCorrelation(it1->second, it2->second));
            if (corr > config.correlationThreshold) {
                significantCorrelations++;
            }
        }
    }

    // Need at least 2 correlated traits to consider coevolution
    return significantCorrelations >= 2;
}

CoevolutionType CoevolutionTracker::classifyInteraction(
    const Species* species1,
    const Species* species2,
    const std::map<std::pair<SpeciesId, SpeciesId>, float>& interactionMatrix) {

    if (!species1 || !species2) {
        return CoevolutionType::COMPETITION;
    }

    SpeciesId id1 = species1->getId();
    SpeciesId id2 = species2->getId();

    // Get interaction strength (positive = beneficial, negative = harmful)
    float interaction12 = 0.0f;
    float interaction21 = 0.0f;

    auto it12 = interactionMatrix.find(std::make_pair(id1, id2));
    auto it21 = interactionMatrix.find(std::make_pair(id2, id1));

    if (it12 != interactionMatrix.end()) interaction12 = it12->second;
    if (it21 != interactionMatrix.end()) interaction21 = it21->second;

    // Classify based on interaction signs
    if (interaction12 > 0.2f && interaction21 > 0.2f) {
        // Both benefit: mutualism
        // Check niche overlap for pollinator-plant vs general mutualism
        float nicheDistance = species1->getNiche().distanceTo(species2->getNiche());
        if (nicheDistance > 0.5f) {
            return CoevolutionType::POLLINATOR_PLANT;
        }
        return CoevolutionType::MUTUALISM;
    }
    else if (interaction12 > 0.2f && interaction21 < -0.2f) {
        // Species 1 benefits, species 2 harmed: parasite-host or predator-prey
        // Predator-prey usually has stronger negative effect
        if (interaction21 < -0.5f) {
            return CoevolutionType::PREDATOR_PREY;
        }
        return CoevolutionType::PARASITE_HOST;
    }
    else if (interaction12 < -0.2f && interaction21 > 0.2f) {
        // Species 2 is the predator/parasite
        if (interaction12 < -0.5f) {
            return CoevolutionType::PREDATOR_PREY;
        }
        return CoevolutionType::PARASITE_HOST;
    }
    else if (interaction12 < -0.2f && interaction21 < -0.2f) {
        // Both harmed: competition
        return CoevolutionType::COMPETITION;
    }

    // Default to competition for weak interactions
    return CoevolutionType::COMPETITION;
}

// =============================================================================
// ARMS RACE TRACKING
// =============================================================================

ArmsRace& CoevolutionTracker::trackArmsRace(const Species* predator,
                                             const Species* prey,
                                             int currentGeneration) {
    if (!predator || !prey) {
        static ArmsRace dummy;
        return dummy;
    }

    SpeciesId predId = predator->getId();
    SpeciesId preyId = prey->getId();
    auto key = std::make_pair(predId, preyId);

    // Create new arms race if doesn't exist
    if (armsRaces.find(key) == armsRaces.end()) {
        armsRaces[key] = ArmsRace(predId, preyId, currentGeneration);

        std::cout << "[COEVOLUTION] Arms race started between "
                  << predator->getName() << " (predator) and "
                  << prey->getName() << " (prey) at generation "
                  << currentGeneration << std::endl;
    }

    return armsRaces[key];
}

void CoevolutionTracker::updateArmsRace(SpeciesId predatorId,
                                         SpeciesId preyId,
                                         const std::vector<Creature*>& predatorCreatures,
                                         const std::vector<Creature*>& preyCreatures) {
    auto key = std::make_pair(predatorId, preyId);
    auto it = armsRaces.find(key);

    if (it == armsRaces.end()) {
        return;
    }

    ArmsRace& race = it->second;

    // Extract current traits
    race.predatorTraits = extractPredatorTraits(predatorCreatures);
    race.preyTraits = extractPreyTraits(preyCreatures);

    // Update advantage
    race.updateAdvantage();

    // Record state in history
    race.recordState();

    // Check for oscillating advantage
    if (race.escalationHistory.size() >= 10) {
        int recentOscillations = 0;
        AdvantageSide lastAdvantage = AdvantageSide::NEUTRAL;

        for (size_t i = race.escalationHistory.size() - 10; i < race.escalationHistory.size(); ++i) {
            float pred = race.predatorTraits.getEffectiveness();
            float prey = race.preyTraits.getDefenseScore();
            AdvantageSide current = (pred > prey + 0.05f) ? AdvantageSide::SPECIES_1 :
                                    (prey > pred + 0.05f) ? AdvantageSide::SPECIES_2 :
                                    AdvantageSide::NEUTRAL;

            if (current != lastAdvantage && current != AdvantageSide::NEUTRAL &&
                lastAdvantage != AdvantageSide::NEUTRAL) {
                recentOscillations++;
            }
            lastAdvantage = current;
        }

        if (recentOscillations >= 3) {
            race.currentAdvantage = AdvantageSide::OSCILLATING;
        }
    }
}

ArmsRace* CoevolutionTracker::getArmsRace(SpeciesId predatorId, SpeciesId preyId) {
    auto key = std::make_pair(predatorId, preyId);
    auto it = armsRaces.find(key);
    return (it != armsRaces.end()) ? &it->second : nullptr;
}

const ArmsRace* CoevolutionTracker::getArmsRace(SpeciesId predatorId, SpeciesId preyId) const {
    auto key = std::make_pair(predatorId, preyId);
    auto it = armsRaces.find(key);
    return (it != armsRaces.end()) ? &it->second : nullptr;
}

std::vector<const ArmsRace*> CoevolutionTracker::getAllArmsRaces() const {
    std::vector<const ArmsRace*> result;
    result.reserve(armsRaces.size());

    for (const auto& [key, race] : armsRaces) {
        result.push_back(&race);
    }

    return result;
}

// =============================================================================
// MUTUALISM TRACKING
// =============================================================================

CoevolutionaryPair& CoevolutionTracker::trackMutualism(const Species* species1,
                                                        const Species* species2,
                                                        CoevolutionType type,
                                                        int currentGeneration) {
    if (!species1 || !species2) {
        static CoevolutionaryPair dummy;
        return dummy;
    }

    auto pairKey = makeOrderedPair(species1->getId(), species2->getId());

    if (coevolutionaryPairs.find(pairKey) == coevolutionaryPairs.end()) {
        coevolutionaryPairs[pairKey] = CoevolutionaryPair(
            pairKey.first, pairKey.second, type, currentGeneration
        );

        std::cout << "[COEVOLUTION] Mutualistic relationship detected between "
                  << species1->getName() << " and " << species2->getName()
                  << " (" << coevolutionTypeToString(type) << ") at generation "
                  << currentGeneration << std::endl;
    }

    return coevolutionaryPairs[pairKey];
}

void CoevolutionTracker::calculateMutualismBenefits(
    const std::pair<SpeciesId, SpeciesId>& pairId,
    float& benefit1,
    float& benefit2) const {

    auto it = coevolutionaryPairs.find(pairId);
    if (it == coevolutionaryPairs.end()) {
        benefit1 = 0.0f;
        benefit2 = 0.0f;
        return;
    }

    const CoevolutionaryPair& pair = it->second;

    // Benefits scale with interaction strength and time linked
    float timeBonus = std::min(1.0f, pair.generationsLinked / 50.0f);
    float baseInteraction = pair.interactionStrength;

    // Mutualistic pairs have symmetric benefits
    if (pair.isMutualistic()) {
        benefit1 = baseInteraction * timeBonus * 0.5f;
        benefit2 = baseInteraction * timeBonus * 0.5f;

        // Pollinator-plant may have asymmetric benefits
        if (pair.type == CoevolutionType::POLLINATOR_PLANT) {
            benefit1 *= 0.8f;  // Pollinator gets slightly less
            benefit2 *= 1.2f;  // Plant gets more (reproduction)
        }
    } else {
        // Antagonistic pairs
        benefit1 = baseInteraction * timeBonus * 0.3f;
        benefit2 = -baseInteraction * timeBonus * 0.2f;
    }
}

// =============================================================================
// TRAIT CORRELATION METHODS
// =============================================================================

TraitCorrelation CoevolutionTracker::calculateTraitCorrelation(
    const Species* species1,
    const Species* species2,
    GeneType trait1,
    GeneType trait2,
    int windowSize) {

    TraitCorrelation result(trait1, trait2);

    if (!species1 || !species2) {
        return result;
    }

    auto key1 = std::make_pair(species1->getId(), trait1);
    auto key2 = std::make_pair(species2->getId(), trait2);

    auto it1 = traitHistories.find(key1);
    auto it2 = traitHistories.find(key2);

    if (it1 == traitHistories.end() || it2 == traitHistories.end()) {
        return result;
    }

    const std::deque<float>& history1 = it1->second;
    const std::deque<float>& history2 = it2->second;

    // Use the smaller of available data or requested window
    size_t actualWindow = std::min({
        static_cast<size_t>(windowSize),
        history1.size(),
        history2.size()
    });

    if (actualWindow < 3) {
        return result;
    }

    // Extract recent values
    std::deque<float> recent1, recent2;
    for (size_t i = history1.size() - actualWindow; i < history1.size(); ++i) {
        recent1.push_back(history1[i]);
    }
    for (size_t i = history2.size() - actualWindow; i < history2.size(); ++i) {
        recent2.push_back(history2[i]);
    }

    // Calculate Pearson correlation
    result.correlationCoefficient = calculatePearsonCorrelation(recent1, recent2);
    result.sampleSize = static_cast<int>(actualWindow);

    // Calculate approximate p-value using t-distribution approximation
    // t = r * sqrt((n-2) / (1-r^2))
    float r = result.correlationCoefficient;
    float r2 = r * r;
    float n = static_cast<float>(actualWindow);

    if (r2 < 0.9999f && n > 2) {
        float t = std::abs(r) * std::sqrt((n - 2.0f) / (1.0f - r2));
        // Simplified p-value approximation (two-tailed)
        // This is an approximation; for exact values, use a proper statistical library
        result.pValue = 2.0f * std::exp(-0.5f * t * t / (n - 2.0f));
    } else {
        result.pValue = r2 > 0.9999f ? 0.0f : 1.0f;
    }

    result.isSignificant = (result.pValue < 0.05f) &&
                           (std::abs(result.correlationCoefficient) > config.correlationThreshold);

    return result;
}

void CoevolutionTracker::updateAllTraitCorrelations(int currentGeneration) {
    std::vector<GeneType> trackedTraits = {
        GeneType::SPEED, GeneType::SIZE, GeneType::VISION_RANGE,
        GeneType::AGGRESSION, GeneType::CAMOUFLAGE_LEVEL, GeneType::EFFICIENCY
    };

    for (auto& [pairKey, pair] : coevolutionaryPairs) {
        pair.traitCorrelations.clear();

        for (GeneType t1 : trackedTraits) {
            for (GeneType t2 : trackedTraits) {
                // Create placeholder Species pointers for trait lookup
                auto key1 = std::make_pair(pair.species1Id, t1);
                auto key2 = std::make_pair(pair.species2Id, t2);

                auto it1 = traitHistories.find(key1);
                auto it2 = traitHistories.find(key2);

                if (it1 != traitHistories.end() && it2 != traitHistories.end()) {
                    float corr = calculatePearsonCorrelation(it1->second, it2->second);

                    if (std::abs(corr) > config.correlationThreshold) {
                        std::string name = std::string(geneTypeToString(t1)) + "_" +
                                          geneTypeToString(t2);
                        TraitCorrelation tc(t1, t2, corr);
                        tc.sampleSize = static_cast<int>(std::min(it1->second.size(),
                                                                  it2->second.size()));
                        tc.isSignificant = std::abs(corr) > config.correlationThreshold;
                        pair.addTraitCorrelation(name, tc);
                    }
                }
            }
        }

        pair.generationsLinked++;
    }
}

// =============================================================================
// ESCALATION MEASUREMENT
// =============================================================================

float CoevolutionTracker::measureEscalation(const CoevolutionaryPair& pair) const {
    if (pair.escalationHistory.empty()) {
        return 0.0f;
    }

    // For arms races, escalation is the sum of offensive and defensive investments
    auto armsRaceKey = std::make_pair(pair.species1Id, pair.species2Id);
    auto it = armsRaces.find(armsRaceKey);

    if (it != armsRaces.end()) {
        return it->second.getCurrentEscalation();
    }

    // For other relationships, use deviation from initial values
    if (pair.escalationHistory.size() < 2) {
        return pair.escalationLevel;
    }

    float initial = pair.escalationHistory.front();
    float current = pair.escalationHistory.back();

    return current - initial;
}

float CoevolutionTracker::calculateEscalationRate(const CoevolutionaryPair& pair,
                                                   int windowSize) const {
    if (pair.escalationHistory.size() < 2) {
        return 0.0f;
    }

    size_t actualWindow = std::min(
        static_cast<size_t>(windowSize),
        pair.escalationHistory.size()
    );

    if (actualWindow < 2) {
        return 0.0f;
    }

    // Calculate linear regression slope for escalation rate
    size_t startIdx = pair.escalationHistory.size() - actualWindow;

    float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumX2 = 0.0f;
    float n = static_cast<float>(actualWindow);

    for (size_t i = 0; i < actualWindow; ++i) {
        float x = static_cast<float>(i);
        float y = pair.escalationHistory[startIdx + i];
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumX2 += x * x;
    }

    float denominator = n * sumX2 - sumX * sumX;
    if (std::abs(denominator) < 1e-10f) {
        return 0.0f;
    }

    // Slope of linear regression = rate of change per generation
    return (n * sumXY - sumX * sumY) / denominator;
}

// =============================================================================
// MIMICRY DETECTION
// =============================================================================

float CoevolutionTracker::calculateVisualSimilarity(const Species* species1,
                                                     const Species* species2) const {
    if (!species1 || !species2) {
        return 0.0f;
    }

    // Get representative colors
    glm::vec3 color1 = species1->getColor();
    glm::vec3 color2 = species2->getColor();

    // Calculate color distance (inverse of similarity)
    float colorDist = glm::length(color1 - color2);

    // Normalize and invert to get similarity
    float colorSimilarity = std::max(0.0f, 1.0f - colorDist / 1.732f);  // Max dist is sqrt(3)

    // Get pattern similarity from genomes would require accessing creatures
    // For now, use color similarity as primary measure
    return colorSimilarity;
}

MimicryComplex* CoevolutionTracker::detectMimicry(const Species* species,
                                                   const std::vector<Species*>& allSpecies) {
    if (!species || !config.detectMimicry) {
        return nullptr;
    }

    SpeciesId speciesId = species->getId();

    // Check if already part of a mimicry complex
    for (auto& [modelId, complex] : mimicryComplexes) {
        if (modelId == speciesId) {
            return &complex;
        }
        for (SpeciesId mimicId : complex.mimicSpeciesIds) {
            if (mimicId == speciesId) {
                return &complex;
            }
        }
    }

    // Look for potential models (toxic/dangerous species)
    // A model should have high ornament intensity (warning coloration)
    const DiploidGenome& genome = species->getRepresentativeGenome();
    float ornamentIntensity = genome.getTrait(GeneType::ORNAMENT_INTENSITY);

    // If this species has high warning coloration, it could be a model
    if (ornamentIntensity > 0.7f) {
        // Find visually similar species
        for (Species* other : allSpecies) {
            if (!other || other->isExtinct() || other->getId() == speciesId) {
                continue;
            }

            float similarity = calculateVisualSimilarity(species, other);

            if (similarity > config.minMimicryAccuracy) {
                const DiploidGenome& otherGenome = other->getRepresentativeGenome();
                float otherOrnament = otherGenome.getTrait(GeneType::ORNAMENT_INTENSITY);

                // Determine mimicry type
                MimicryType type;
                if (otherOrnament < 0.4f) {
                    // Other species is not toxic: Batesian mimicry
                    type = MimicryType::BATESIAN;
                } else {
                    // Both species are toxic: Mullerian mimicry
                    type = MimicryType::MULLERIAN;
                }

                // Create or update mimicry complex
                MimicryComplex& complex = mimicryComplexes[speciesId];
                complex.modelSpeciesId = speciesId;
                complex.mimicryType = type;
                complex.mimicryAccuracy = similarity;
                complex.modelToxicity = ornamentIntensity;
                complex.addMimic(other->getId());

                if (type == MimicryType::MULLERIAN) {
                    complex.averageMimicToxicity = otherOrnament;
                }

                std::cout << "[COEVOLUTION] " << mimicryTypeToString(type)
                          << " mimicry detected: " << other->getName()
                          << " mimics " << species->getName()
                          << " (accuracy: " << (similarity * 100.0f) << "%)" << std::endl;

                return &complex;
            }
        }
    }

    return nullptr;
}

std::vector<const MimicryComplex*> CoevolutionTracker::getMimicryComplexes() const {
    std::vector<const MimicryComplex*> result;
    result.reserve(mimicryComplexes.size());

    for (const auto& [modelId, complex] : mimicryComplexes) {
        result.push_back(&complex);
    }

    return result;
}

void CoevolutionTracker::updateMimicryStability(MimicryComplex& complex,
                                                 int modelPopulation,
                                                 const std::map<SpeciesId, int>& mimicPopulations) {
    int totalMimics = 0;
    for (const auto& [mimicId, pop] : mimicPopulations) {
        totalMimics += pop;
    }

    if (totalMimics > 0) {
        complex.modelToMimicRatio = static_cast<float>(modelPopulation) /
                                    static_cast<float>(totalMimics);
    } else {
        complex.modelToMimicRatio = 1.0f;
    }

    // Update predator recognition based on model abundance
    // More models = better predator learning
    float abundanceBonus = std::min(1.0f, modelPopulation / 50.0f);
    complex.predatorRecognition = 0.5f + abundanceBonus * 0.5f;
}

// =============================================================================
// RED QUEEN DYNAMICS
// =============================================================================

void CoevolutionTracker::trackRedQueenDynamics(SpeciesId speciesId,
                                                float evolutionaryRate,
                                                float meanFitness) {
    if (!config.trackRedQueenDynamics) {
        return;
    }

    RedQueenMetrics& metrics = redQueenMetrics[speciesId];
    metrics.speciesId = speciesId;

    metrics.recordRate(evolutionaryRate, config.historyLength);
    metrics.recordFitness(meanFitness, config.historyLength);

    // Check for adaptation cycles
    if (metrics.fitnessHistory.size() >= 20) {
        // Look for peaks and troughs in fitness
        int peaks = 0;
        for (size_t i = 1; i < metrics.fitnessHistory.size() - 1; ++i) {
            if (metrics.fitnessHistory[i] > metrics.fitnessHistory[i-1] &&
                metrics.fitnessHistory[i] > metrics.fitnessHistory[i+1]) {
                peaks++;
            }
        }
        metrics.adaptationCycles = peaks;
    }
}

const RedQueenMetrics* CoevolutionTracker::getRedQueenMetrics(SpeciesId speciesId) const {
    auto it = redQueenMetrics.find(speciesId);
    return (it != redQueenMetrics.end()) ? &it->second : nullptr;
}

int CoevolutionTracker::detectAdaptationCycles(const CoevolutionaryPair& pair) const {
    // Count cycles by looking at advantage oscillations
    int cycles = 0;

    if (pair.escalationHistory.size() < 10) {
        return 0;
    }

    // Simple cycle detection: count direction changes
    bool increasing = true;
    for (size_t i = 1; i < pair.escalationHistory.size(); ++i) {
        bool nowIncreasing = pair.escalationHistory[i] > pair.escalationHistory[i-1];
        if (nowIncreasing != increasing) {
            cycles++;
            increasing = nowIncreasing;
        }
    }

    return cycles / 2;  // Each full cycle has 2 direction changes
}

float CoevolutionTracker::calculateResponseLag(const CoevolutionaryPair& pair) const {
    if (pair.escalationHistory.size() < 20) {
        return 0.0f;
    }

    // Estimate lag by cross-correlation analysis
    // This is a simplified version - full implementation would use proper
    // cross-correlation with lag shifting

    // For now, return average peak-to-peak distance
    std::vector<size_t> peakIndices;

    for (size_t i = 1; i < pair.escalationHistory.size() - 1; ++i) {
        if (pair.escalationHistory[i] > pair.escalationHistory[i-1] &&
            pair.escalationHistory[i] > pair.escalationHistory[i+1]) {
            peakIndices.push_back(i);
        }
    }

    if (peakIndices.size() < 2) {
        return 0.0f;
    }

    float totalLag = 0.0f;
    for (size_t i = 1; i < peakIndices.size(); ++i) {
        totalLag += static_cast<float>(peakIndices[i] - peakIndices[i-1]);
    }

    return totalLag / (peakIndices.size() - 1);
}

// =============================================================================
// MAIN UPDATE METHOD
// =============================================================================

void CoevolutionTracker::updateCoevolutionaryDynamics(
    int currentGeneration,
    const std::vector<Species*>& allSpecies,
    const std::vector<Creature*>& creatures,
    const std::map<std::pair<SpeciesId, SpeciesId>, float>& interactionMatrix) {

    // Check if update is needed based on frequency
    if (currentGeneration - lastUpdateGeneration < config.updateFrequency) {
        return;
    }
    lastUpdateGeneration = currentGeneration;

    // Group creatures by species
    std::map<SpeciesId, std::vector<Creature*>> creaturesBySpecies;
    for (Creature* c : creatures) {
        if (c && c->isAlive()) {
            creaturesBySpecies[c->getSpeciesId()].push_back(c);
        }
    }

    // Record trait values for all species
    for (const auto& [speciesId, speciesCreatures] : creaturesBySpecies) {
        if (speciesCreatures.empty()) continue;

        // Calculate mean trait values
        std::map<GeneType, float> traitSums;
        std::vector<GeneType> trackedTraits = {
            GeneType::SPEED, GeneType::SIZE, GeneType::VISION_RANGE,
            GeneType::AGGRESSION, GeneType::CAMOUFLAGE_LEVEL, GeneType::EFFICIENCY,
            GeneType::ORNAMENT_INTENSITY
        };

        for (GeneType trait : trackedTraits) {
            traitSums[trait] = 0.0f;
        }

        for (const Creature* c : speciesCreatures) {
            const DiploidGenome& genome = c->getDiploidGenome();
            for (GeneType trait : trackedTraits) {
                traitSums[trait] += genome.getTrait(trait);
            }
        }

        float count = static_cast<float>(speciesCreatures.size());
        for (GeneType trait : trackedTraits) {
            recordTraitValue(speciesId, trait, traitSums[trait] / count);
        }

        // Track Red Queen dynamics
        if (config.trackRedQueenDynamics) {
            // Calculate evolutionary rate as trait variance
            float totalVariance = 0.0f;
            for (GeneType trait : trackedTraits) {
                float mean = traitSums[trait] / count;
                float variance = 0.0f;
                for (const Creature* c : speciesCreatures) {
                    float val = c->getDiploidGenome().getTrait(trait);
                    variance += (val - mean) * (val - mean);
                }
                totalVariance += variance / count;
            }
            float evolutionaryRate = std::sqrt(totalVariance / trackedTraits.size());

            // Calculate mean fitness
            float meanFitness = 0.0f;
            for (const Creature* c : speciesCreatures) {
                meanFitness += c->getFitness();
            }
            meanFitness /= count;

            trackRedQueenDynamics(speciesId, evolutionaryRate, meanFitness);
        }
    }

    // Detect new coevolutionary pairs
    detectNewPairs(allSpecies, interactionMatrix, currentGeneration);

    // Update existing pairs
    updateExistingPairs(currentGeneration, creatures);

    // Update arms races
    if (config.trackArmsRaces) {
        for (auto& [key, race] : armsRaces) {
            auto predIt = creaturesBySpecies.find(race.predatorSpeciesId);
            auto preyIt = creaturesBySpecies.find(race.preySpeciesId);

            if (predIt != creaturesBySpecies.end() && preyIt != creaturesBySpecies.end()) {
                updateArmsRace(race.predatorSpeciesId, race.preySpeciesId,
                              predIt->second, preyIt->second);
            }
        }
    }

    // Detect mimicry relationships
    if (config.detectMimicry) {
        detectMimicryRelationships(allSpecies);
    }

    // Update Red Queen metrics
    if (config.trackRedQueenDynamics) {
        updateRedQueenMetrics(allSpecies, creatures);
    }

    // Prune old history
    pruneHistory();
}

void CoevolutionTracker::detectNewPairs(
    const std::vector<Species*>& allSpecies,
    const std::map<std::pair<SpeciesId, SpeciesId>, float>& interactionMatrix,
    int currentGeneration) {

    if (coevolutionaryPairs.size() >= static_cast<size_t>(config.maxTrackedPairs)) {
        return;
    }

    for (size_t i = 0; i < allSpecies.size(); ++i) {
        Species* sp1 = allSpecies[i];
        if (!sp1 || sp1->isExtinct()) continue;

        for (size_t j = i + 1; j < allSpecies.size(); ++j) {
            Species* sp2 = allSpecies[j];
            if (!sp2 || sp2->isExtinct()) continue;

            auto pairKey = makeOrderedPair(sp1->getId(), sp2->getId());
            if (coevolutionaryPairs.find(pairKey) != coevolutionaryPairs.end()) {
                continue;  // Already tracked
            }

            // Check interaction strength
            float interaction = 0.0f;
            auto it = interactionMatrix.find(std::make_pair(sp1->getId(), sp2->getId()));
            if (it != interactionMatrix.end()) {
                interaction = std::abs(it->second);
            }

            if (interaction < config.minInteractionStrength) {
                continue;
            }

            // Check for coevolution
            if (detectCoevolutionaryPair(sp1, sp2)) {
                CoevolutionType type = classifyInteraction(sp1, sp2, interactionMatrix);

                CoevolutionaryPair& newPair = coevolutionaryPairs[pairKey];
                newPair.species1Id = pairKey.first;
                newPair.species2Id = pairKey.second;
                newPair.type = type;
                newPair.discoveryGeneration = currentGeneration;
                newPair.interactionStrength = interaction;

                std::cout << "[COEVOLUTION] Detected " << coevolutionTypeToString(type)
                          << " relationship between " << sp1->getName()
                          << " and " << sp2->getName()
                          << " (generation " << currentGeneration << ")" << std::endl;

                // Start arms race tracking for predator-prey pairs
                if (type == CoevolutionType::PREDATOR_PREY && config.trackArmsRaces) {
                    // Determine which is predator based on interaction sign
                    auto it12 = interactionMatrix.find(std::make_pair(sp1->getId(), sp2->getId()));
                    if (it12 != interactionMatrix.end() && it12->second > 0) {
                        trackArmsRace(sp1, sp2, currentGeneration);
                    } else {
                        trackArmsRace(sp2, sp1, currentGeneration);
                    }
                }
            }
        }
    }
}

void CoevolutionTracker::updateExistingPairs(int currentGeneration,
                                              const std::vector<Creature*>& creatures) {
    for (auto& [pairKey, pair] : coevolutionaryPairs) {
        pair.generationsLinked++;

        // Update escalation
        float escalation = measureEscalation(pair);
        pair.escalationLevel = escalation;
        pair.recordEscalation(config.historyLength);
        pair.recordStrength(config.historyLength);
    }

    // Update trait correlations
    updateAllTraitCorrelations(currentGeneration);
}

void CoevolutionTracker::detectMimicryRelationships(const std::vector<Species*>& allSpecies) {
    for (Species* sp : allSpecies) {
        if (!sp || sp->isExtinct()) continue;

        // Only check species not already in a mimicry complex
        bool alreadyTracked = false;
        for (const auto& [modelId, complex] : mimicryComplexes) {
            if (modelId == sp->getId()) {
                alreadyTracked = true;
                break;
            }
            for (SpeciesId mimicId : complex.mimicSpeciesIds) {
                if (mimicId == sp->getId()) {
                    alreadyTracked = true;
                    break;
                }
            }
            if (alreadyTracked) break;
        }

        if (!alreadyTracked) {
            detectMimicry(sp, allSpecies);
        }
    }
}

void CoevolutionTracker::updateRedQueenMetrics(const std::vector<Species*>& allSpecies,
                                                const std::vector<Creature*>& creatures) {
    // Metrics are updated in the main update loop when traits are recorded
    // Here we can add additional analysis if needed

    for (auto& [speciesId, metrics] : redQueenMetrics) {
        // Update cycle detection
        if (metrics.fitnessHistory.size() >= 10) {
            metrics.updateOscillationMetrics();
        }
    }
}

// =============================================================================
// QUERY METHODS
// =============================================================================

std::vector<const CoevolutionaryPair*> CoevolutionTracker::getCoevolutionaryPairs() const {
    std::vector<const CoevolutionaryPair*> result;
    result.reserve(coevolutionaryPairs.size());

    for (const auto& [key, pair] : coevolutionaryPairs) {
        result.push_back(&pair);
    }

    return result;
}

std::vector<const CoevolutionaryPair*> CoevolutionTracker::getPairsForSpecies(SpeciesId speciesId) const {
    std::vector<const CoevolutionaryPair*> result;

    for (const auto& [key, pair] : coevolutionaryPairs) {
        if (pair.species1Id == speciesId || pair.species2Id == speciesId) {
            result.push_back(&pair);
        }
    }

    return result;
}

std::vector<const CoevolutionaryPair*> CoevolutionTracker::getPairsByType(CoevolutionType type) const {
    std::vector<const CoevolutionaryPair*> result;

    for (const auto& [key, pair] : coevolutionaryPairs) {
        if (pair.type == type) {
            result.push_back(&pair);
        }
    }

    return result;
}

const CoevolutionaryPair* CoevolutionTracker::getPair(SpeciesId species1Id,
                                                       SpeciesId species2Id) const {
    auto pairKey = makeOrderedPair(species1Id, species2Id);
    auto it = coevolutionaryPairs.find(pairKey);
    return (it != coevolutionaryPairs.end()) ? &it->second : nullptr;
}

CoevolutionStats CoevolutionTracker::getStats() const {
    CoevolutionStats stats;

    stats.totalPairs = static_cast<int>(coevolutionaryPairs.size());
    stats.activeArmsRaces = static_cast<int>(armsRaces.size());
    stats.mimicryComplexes = static_cast<int>(mimicryComplexes.size());

    float totalEscalation = 0.0f;
    float totalStrength = 0.0f;

    for (const auto& [key, pair] : coevolutionaryPairs) {
        switch (pair.type) {
            case CoevolutionType::PREDATOR_PREY:
                stats.predatorPreyPairs++;
                break;
            case CoevolutionType::POLLINATOR_PLANT:
            case CoevolutionType::MUTUALISM:
                stats.mutualisticPairs++;
                break;
            case CoevolutionType::PARASITE_HOST:
                stats.parasitePairs++;
                break;
            case CoevolutionType::COMPETITION:
                stats.competitivePairs++;
                break;
            default:
                break;
        }

        totalEscalation += pair.escalationLevel;
        totalStrength += pair.interactionStrength;
    }

    if (stats.totalPairs > 0) {
        stats.averageEscalation = totalEscalation / stats.totalPairs;
        stats.averageInteractionStrength = totalStrength / stats.totalPairs;
    }

    // Count species with Red Queen dynamics
    float totalRate = 0.0f;
    for (const auto& [speciesId, metrics] : redQueenMetrics) {
        if (metrics.isRunningInPlace) {
            stats.speciesWithRedQueenDynamics++;
        }
        totalRate += metrics.evolutionaryRate;
    }

    if (!redQueenMetrics.empty()) {
        stats.averageEvolutionaryRate = totalRate / redQueenMetrics.size();
    }

    return stats;
}

// =============================================================================
// DATA EXPORT
// =============================================================================

bool CoevolutionTracker::exportCoevolutionData(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[COEVOLUTION] Failed to open file for export: " << filename << std::endl;
        return false;
    }

    // Write header
    file << "Species1ID,Species2ID,Type,InteractionStrength,GenerationsLinked,"
         << "DiscoveryGeneration,EscalationLevel,CurrentAdvantage\n";

    // Write pairs
    for (const auto& [key, pair] : coevolutionaryPairs) {
        file << pair.species1Id << ","
             << pair.species2Id << ","
             << coevolutionTypeToString(pair.type) << ","
             << pair.interactionStrength << ","
             << pair.generationsLinked << ","
             << pair.discoveryGeneration << ","
             << pair.escalationLevel << ","
             << advantageSideToString(pair.currentAdvantage) << "\n";
    }

    file.close();
    std::cout << "[COEVOLUTION] Exported coevolution data to: " << filename << std::endl;
    return true;
}

bool CoevolutionTracker::exportCoevolutionNetwork(const std::string& filename,
                                                   const std::string& format) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[COEVOLUTION] Failed to open file for network export: " << filename << std::endl;
        return false;
    }

    if (format == "graphml") {
        // GraphML format
        file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        file << "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\">\n";
        file << "  <key id=\"type\" for=\"edge\" attr.name=\"type\" attr.type=\"string\"/>\n";
        file << "  <key id=\"strength\" for=\"edge\" attr.name=\"strength\" attr.type=\"double\"/>\n";
        file << "  <graph id=\"coevolution\" edgedefault=\"undirected\">\n";

        // Collect all species IDs
        std::set<SpeciesId> speciesIds;
        for (const auto& [key, pair] : coevolutionaryPairs) {
            speciesIds.insert(pair.species1Id);
            speciesIds.insert(pair.species2Id);
        }

        // Write nodes
        for (SpeciesId id : speciesIds) {
            file << "    <node id=\"" << id << "\"/>\n";
        }

        // Write edges
        int edgeId = 0;
        for (const auto& [key, pair] : coevolutionaryPairs) {
            file << "    <edge id=\"e" << edgeId++ << "\" source=\""
                 << pair.species1Id << "\" target=\"" << pair.species2Id << "\">\n";
            file << "      <data key=\"type\">" << coevolutionTypeToString(pair.type) << "</data>\n";
            file << "      <data key=\"strength\">" << pair.interactionStrength << "</data>\n";
            file << "    </edge>\n";
        }

        file << "  </graph>\n";
        file << "</graphml>\n";
    }
    else if (format == "dot") {
        // DOT format for Graphviz
        file << "graph coevolution {\n";
        file << "  node [shape=ellipse];\n";

        for (const auto& [key, pair] : coevolutionaryPairs) {
            std::string style;
            switch (pair.type) {
                case CoevolutionType::PREDATOR_PREY:
                    style = "color=red";
                    break;
                case CoevolutionType::MUTUALISM:
                case CoevolutionType::POLLINATOR_PLANT:
                    style = "color=green";
                    break;
                case CoevolutionType::PARASITE_HOST:
                    style = "color=orange";
                    break;
                case CoevolutionType::COMPETITION:
                    style = "color=blue";
                    break;
                default:
                    style = "color=gray";
            }

            file << "  " << pair.species1Id << " -- " << pair.species2Id
                 << " [" << style << ", label=\"" << coevolutionTypeToString(pair.type) << "\"];\n";
        }

        file << "}\n";
    }
    else if (format == "adjacency") {
        // Simple adjacency list
        for (const auto& [key, pair] : coevolutionaryPairs) {
            file << pair.species1Id << " " << pair.species2Id << " "
                 << pair.interactionStrength << "\n";
        }
    }
    else {
        std::cerr << "[COEVOLUTION] Unknown network format: " << format << std::endl;
        file.close();
        return false;
    }

    file.close();
    std::cout << "[COEVOLUTION] Exported coevolution network to: " << filename << std::endl;
    return true;
}

bool CoevolutionTracker::exportArmsRaceHistory(const ArmsRace& armsRace,
                                                const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[COEVOLUTION] Failed to open file for arms race export: " << filename << std::endl;
        return false;
    }

    file << "Generation,Escalation,PredatorEffectiveness,PreyDefense,Advantage\n";

    for (size_t i = 0; i < armsRace.escalationHistory.size(); ++i) {
        int gen = armsRace.startGeneration + static_cast<int>(i);
        file << gen << ","
             << armsRace.escalationHistory[i] << ","
             << armsRace.predatorTraits.getEffectiveness() << ","
             << armsRace.preyTraits.getDefenseScore() << ","
             << advantageSideToString(armsRace.currentAdvantage) << "\n";
    }

    file.close();
    std::cout << "[COEVOLUTION] Exported arms race history to: " << filename << std::endl;
    return true;
}

// =============================================================================
// CONFIGURATION
// =============================================================================

void CoevolutionTracker::setConfig(const CoevolutionConfig& cfg) {
    config = cfg;
}

// =============================================================================
// CLEANUP
// =============================================================================

void CoevolutionTracker::handleExtinction(SpeciesId extinctSpeciesId) {
    // Remove pairs involving extinct species
    std::vector<std::pair<SpeciesId, SpeciesId>> pairsToRemove;

    for (const auto& [key, pair] : coevolutionaryPairs) {
        if (pair.species1Id == extinctSpeciesId || pair.species2Id == extinctSpeciesId) {
            pairsToRemove.push_back(key);
        }
    }

    for (const auto& key : pairsToRemove) {
        coevolutionaryPairs.erase(key);
    }

    // Remove arms races
    std::vector<std::pair<SpeciesId, SpeciesId>> racesToRemove;

    for (const auto& [key, race] : armsRaces) {
        if (race.predatorSpeciesId == extinctSpeciesId ||
            race.preySpeciesId == extinctSpeciesId) {
            racesToRemove.push_back(key);
        }
    }

    for (const auto& key : racesToRemove) {
        armsRaces.erase(key);
    }

    // Remove from mimicry complexes
    mimicryComplexes.erase(extinctSpeciesId);

    for (auto& [modelId, complex] : mimicryComplexes) {
        complex.removeMimic(extinctSpeciesId);
    }

    // Remove Red Queen metrics
    redQueenMetrics.erase(extinctSpeciesId);

    // Remove trait histories
    std::vector<std::pair<SpeciesId, GeneType>> historiesToRemove;

    for (const auto& [key, history] : traitHistories) {
        if (key.first == extinctSpeciesId) {
            historiesToRemove.push_back(key);
        }
    }

    for (const auto& key : historiesToRemove) {
        traitHistories.erase(key);
    }

    std::cout << "[COEVOLUTION] Cleaned up coevolution data for extinct species "
              << extinctSpeciesId << std::endl;
}

void CoevolutionTracker::clear() {
    coevolutionaryPairs.clear();
    armsRaces.clear();
    mimicryComplexes.clear();
    redQueenMetrics.clear();
    traitHistories.clear();
    lastUpdateGeneration = 0;

    std::cout << "[COEVOLUTION] Cleared all coevolution tracking data" << std::endl;
}

} // namespace genetics
