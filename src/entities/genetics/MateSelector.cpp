#include "MateSelector.h"
#include "../Creature.h"
#include "../../utils/Random.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace genetics {

// ============================================
// Constructor and Core Methods
// ============================================

MateSelector::MateSelector()
    : defaultSearchRadius(30.0f),
      minimumAcceptance(0.3f),
      speciesDistanceThreshold(0.15f),
      choiceMode(ChoiceMode::BEST_OF_N),
      bestOfNSampleSize(5),
      assortativeStrength(0.3f) {
}

std::vector<Creature*> MateSelector::findPotentialMates(
    const Creature& seeker,
    const std::vector<Creature*>& candidates,
    float searchRadius) const {

    std::vector<Creature*> potentialMates;
    float radius = searchRadius > 0 ? searchRadius : defaultSearchRadius;

    for (Creature* candidate : candidates) {
        if (!candidate || candidate == &seeker) continue;
        if (!candidate->isAlive()) continue;

        // Must be same creature type (herbivore/carnivore)
        if (candidate->getType() != seeker.getType()) continue;

        // Must be within search radius
        float distance = glm::length(candidate->getPosition() - seeker.getPosition());
        if (distance > radius) continue;

        // Must be able to reproduce
        if (!candidate->canReproduce()) continue;

        // Check basic interbreeding compatibility
        if (!canInterbreed(seeker, *candidate)) continue;

        potentialMates.push_back(candidate);
    }

    return potentialMates;
}

MateEvaluation MateSelector::evaluateMate(const Creature& chooser,
                                          const Creature& candidate) const {
    MateEvaluation result(&const_cast<Creature&>(candidate));

    const DiploidGenome& chooserGenome = chooser.getDiploidGenome();
    const DiploidGenome& candidateGenome = candidate.getDiploidGenome();

    MatePreferences prefs = chooserGenome.getMatePreferences();

    // Assess based on preferences
    result.attractiveness = evaluateByPreferences(prefs, chooserGenome, candidateGenome);

    // Genetic compatibility (prefer dissimilar MHC, but not too different)
    result.geneticCompatibility = evaluateGeneticCompatibility(chooserGenome, candidateGenome);

    // Physical condition bonus
    float conditionBonus = estimateCondition(candidate);

    // Enhanced ornament evaluation
    OrnamentDisplay ornament = extractOrnamentDisplay(candidate);
    result.ornamentScore = ornament.calculateQuality() * prefs.ornamentPreference;

    // Display behavior evaluation
    result.displayScore = evaluateDisplayQuality(candidate);

    // Handicap principle scoring
    result.handicapScore = calculateZahavianScore(candidate);

    // Preference-trait matching
    result.preferenceMatch = evaluatePreferenceMatch(candidate, chooser);

    // Dominance and territory scores
    result.dominanceScore = getDominanceRank(candidate.getID());
    TerritoryQuality territory = evaluateTerritoryQuality(candidate);
    result.territoryScore = territory.overallQuality;

    // Calculate total score with weighted components
    result.totalScore = result.attractiveness * 0.20f +
                      result.geneticCompatibility * 0.15f +
                      conditionBonus * 0.10f +
                      result.ornamentScore * 0.15f +
                      result.displayScore * 0.10f +
                      result.handicapScore * 0.10f +
                      result.preferenceMatch * 0.10f +
                      result.dominanceScore * 0.05f +
                      result.territoryScore * 0.05f;

    return result;
}

Creature* MateSelector::selectMate(
    const Creature& chooser,
    const std::vector<Creature*>& potentialMates) const {

    // Use the configured choice mode
    return selectMateWithMode(chooser, potentialMates, choiceMode);
}

Creature* MateSelector::selectMateWithMode(
    const Creature& chooser,
    const std::vector<Creature*>& potentialMates,
    ChoiceMode mode) const {

    if (potentialMates.empty()) return nullptr;

    const DiploidGenome& chooserGenome = chooser.getDiploidGenome();
    MatePreferences prefs = chooserGenome.getMatePreferences();

    switch (mode) {
        case ChoiceMode::THRESHOLD: {
            // Accept first mate above threshold
            for (Creature* candidate : potentialMates) {
                MateEvaluation assessment = evaluateMate(chooser, *candidate);
                if (assessment.totalScore >= prefs.minimumAcceptable) {
                    return assessment.candidate;
                }
            }
            return nullptr;
        }

        case ChoiceMode::BEST_OF_N: {
            return selectBestOfN(chooser, potentialMates, bestOfNSampleSize);
        }

        case ChoiceMode::SEQUENTIAL: {
            // Evaluate sequentially, accept if better than previous best
            float bestScore = prefs.minimumAcceptable;
            Creature* bestMate = nullptr;

            for (Creature* candidate : potentialMates) {
                MateEvaluation assessment = evaluateMate(chooser, *candidate);
                if (assessment.totalScore > bestScore) {
                    // Probabilistic acceptance based on how much better
                    float improvement = assessment.totalScore - bestScore;
                    float acceptProb = std::min(1.0f, 0.5f + improvement * 2.0f);

                    if (Random::chance(acceptProb)) {
                        bestScore = assessment.totalScore;
                        bestMate = assessment.candidate;

                        // High choosiness may stop searching early
                        if (prefs.choosiness < 0.5f && bestScore > 0.7f) {
                            return bestMate;
                        }
                    }
                }
            }
            return bestMate;
        }
    }

    return nullptr;
}

Creature* MateSelector::selectBestOfN(
    const Creature& chooser,
    const std::vector<Creature*>& potentialMates,
    int sampleSize) const {

    if (potentialMates.empty()) return nullptr;

    const DiploidGenome& chooserGenome = chooser.getDiploidGenome();
    MatePreferences prefs = chooserGenome.getMatePreferences();

    // Sample N candidates (or all if fewer available)
    std::vector<Creature*> sample;
    if (static_cast<int>(potentialMates.size()) <= sampleSize) {
        sample = potentialMates;
    } else {
        // Random sampling without replacement
        std::vector<int> indices(potentialMates.size());
        std::iota(indices.begin(), indices.end(), 0);

        for (int i = 0; i < sampleSize; ++i) {
            int j = Random::rangeInt(i, static_cast<int>(indices.size()) - 1);
            std::swap(indices[i], indices[j]);
            sample.push_back(potentialMates[indices[i]]);
        }
    }

    // Evaluate all sampled candidates
    std::vector<MateEvaluation> assessments;
    for (Creature* candidate : sample) {
        assessments.push_back(evaluateMate(chooser, *candidate));
    }

    // Sort by total score
    std::sort(assessments.begin(), assessments.end(),
              [](const MateEvaluation& a, const MateEvaluation& b) {
                  return a.totalScore > b.totalScore;
              });

    // Return best if above threshold
    if (!assessments.empty() && assessments[0].totalScore >= prefs.minimumAcceptable) {
        return assessments[0].candidate;
    }

    // Very low choosiness might still accept best available
    if (prefs.choosiness < 0.3f && !assessments.empty()) {
        return assessments[0].candidate;
    }

    return nullptr;
}

bool MateSelector::canInterbreed(const Creature& c1, const Creature& c2) const {
    const DiploidGenome& g1 = c1.getDiploidGenome();
    const DiploidGenome& g2 = c2.getDiploidGenome();

    // Same species can always interbreed
    if (g1.getSpeciesId() == g2.getSpeciesId() && g1.getSpeciesId() != 0) {
        return true;
    }

    // Check genetic distance
    float distance = g1.distanceTo(g2);
    if (distance > speciesDistanceThreshold * 2.0f) {
        return false;  // Too divergent
    }

    // Check for isolation mechanisms
    IsolationType isolation = identifyIsolation(c1, c2);
    if (isolation == IsolationType::MECHANICAL ||
        isolation == IsolationType::GAMETIC) {
        return false;  // Hard barriers
    }

    // Temporal isolation reduces but doesn't prevent interbreeding
    if (isolation == IsolationType::TEMPORAL) {
        float temporalOverlap = evaluateTemporalCompatibility(g1, g2);
        return Random::chance(temporalOverlap);
    }

    // Behavioral isolation can be overcome
    if (isolation == IsolationType::BEHAVIORAL) {
        return Random::chance(0.3f);  // Reduced but possible
    }

    return true;
}

ReproductiveCompatibility MateSelector::calculateCompatibility(
    const DiploidGenome& g1, const DiploidGenome& g2) const {

    ReproductiveCompatibility compat;

    float geneticDistance = g1.distanceTo(g2);

    // Pre-mating barriers increase with genetic distance
    compat.preMatingBarrier = std::min(1.0f, geneticDistance * 3.0f);

    // Post-mating barriers (Dobzhansky-Muller incompatibilities)
    // Increases roughly quadratically with divergence
    compat.postMatingBarrier = std::min(1.0f, geneticDistance * geneticDistance * 10.0f);

    // Sterility (Haldane's rule - increases with divergence)
    compat.hybridSterility = std::min(1.0f, geneticDistance * 5.0f);

    // Hybrid vigor (heterosis) - highest at intermediate distances
    // Too similar = inbreeding, too different = incompatibility
    if (geneticDistance > 0.02f && geneticDistance < 0.1f) {
        compat.hybridVigor = 0.1f * (1.0f - std::abs(geneticDistance - 0.05f) / 0.05f);
    }

    return compat;
}

IsolationType MateSelector::identifyIsolation(const Creature& c1, const Creature& c2) const {
    const DiploidGenome& g1 = c1.getDiploidGenome();
    const DiploidGenome& g2 = c2.getDiploidGenome();

    // Check temporal isolation first (activity time)
    float temporalCompat = evaluateTemporalCompatibility(g1, g2);
    if (temporalCompat < 0.3f) {
        return IsolationType::TEMPORAL;
    }

    // Check ecological isolation (niche)
    float ecologicalCompat = evaluateEcologicalCompatibility(g1, g2);
    if (ecologicalCompat < 0.3f) {
        return IsolationType::ECOLOGICAL;
    }

    // Check behavioral isolation (display/courtship differences)
    float displayDiff = std::abs(g1.getTrait(GeneType::DISPLAY_FREQUENCY) -
                                  g2.getTrait(GeneType::DISPLAY_FREQUENCY));
    float ornamentDiff = std::abs(g1.getTrait(GeneType::ORNAMENT_INTENSITY) -
                                   g2.getTrait(GeneType::ORNAMENT_INTENSITY));
    if (displayDiff > 0.5f || ornamentDiff > 0.5f) {
        return IsolationType::BEHAVIORAL;
    }

    // Check physical/mechanical isolation (size differences)
    float physicalCompat = evaluatePhysicalCompatibility(g1, g2);
    if (physicalCompat < 0.2f) {
        return IsolationType::MECHANICAL;
    }

    // Check genetic distance for gametic isolation
    float geneticDistance = g1.distanceTo(g2);
    if (geneticDistance > speciesDistanceThreshold * 1.5f) {
        return IsolationType::GAMETIC;
    }

    return IsolationType::NONE;
}

// ============================================
// Runaway Selection Support
// ============================================

void MateSelector::trackOrnamentEvolution(SpeciesId speciesId,
                                          const std::vector<Creature*>& population) {
    if (population.empty()) return;

    // Filter to species members
    std::vector<const Creature*> speciesMembers;
    for (const Creature* c : population) {
        if (c && c->isAlive() && c->getDiploidGenome().getSpeciesId() == speciesId) {
            speciesMembers.push_back(c);
        }
    }

    if (speciesMembers.empty()) return;

    // Calculate population means
    float ornamentSum = 0.0f;
    float preferenceSum = 0.0f;

    for (const Creature* c : speciesMembers) {
        const DiploidGenome& genome = c->getDiploidGenome();
        ornamentSum += genome.getTrait(GeneType::ORNAMENT_INTENSITY);
        preferenceSum += genome.getMatePreferences().ornamentPreference;
    }

    float n = static_cast<float>(speciesMembers.size());
    float ornamentMean = ornamentSum / n;
    float preferenceMean = preferenceSum / n;

    // Calculate covariance
    float covariance = 0.0f;
    for (const Creature* c : speciesMembers) {
        const DiploidGenome& genome = c->getDiploidGenome();
        float ornamentDev = genome.getTrait(GeneType::ORNAMENT_INTENSITY) - ornamentMean;
        float prefDev = genome.getMatePreferences().ornamentPreference - preferenceMean;
        covariance += ornamentDev * prefDev;
    }
    covariance /= n;

    // Update tracking data
    RunawaySelectionData& data = runawayData[speciesId];
    data.generationsTracked++;

    // Store history for gradient calculation
    ornamentHistory[speciesId].push_back(ornamentMean);
    preferenceHistory[speciesId].push_back(preferenceMean);

    // Limit history length
    const size_t maxHistory = 50;
    if (ornamentHistory[speciesId].size() > maxHistory) {
        ornamentHistory[speciesId].erase(ornamentHistory[speciesId].begin());
        preferenceHistory[speciesId].erase(preferenceHistory[speciesId].begin());
    }

    // Calculate selection gradient (rate of change)
    float selectionGradient = 0.0f;
    if (ornamentHistory[speciesId].size() >= 2) {
        size_t histSize = ornamentHistory[speciesId].size();
        float recentOrnament = ornamentHistory[speciesId][histSize - 1];
        float olderOrnament = ornamentHistory[speciesId][histSize - 2];
        selectionGradient = recentOrnament - olderOrnament;
    }

    // Calculate runaway strength
    // Fisher runaway occurs when positive covariance AND both traits increasing
    float runawayStrength = 0.0f;
    if (covariance > 0.0f && selectionGradient > 0.0f) {
        // Runaway strength proportional to covariance and rate of change
        runawayStrength = std::sqrt(covariance) * (1.0f + selectionGradient * 10.0f);
        runawayStrength = std::clamp(runawayStrength, 0.0f, 1.0f);
    }

    // Update data
    data.ornamentTraitMean = ornamentMean;
    data.preferenceTraitMean = preferenceMean;
    data.covariance = covariance;
    data.selectionGradient = selectionGradient;
    data.runawayStrength = runawayStrength;
}

float MateSelector::calculateRunawayStrength(SpeciesId speciesId) const {
    auto it = runawayData.find(speciesId);
    if (it == runawayData.end()) return 0.0f;
    return it->second.runawayStrength;
}

const RunawaySelectionData* MateSelector::getRunawayData(SpeciesId speciesId) const {
    auto it = runawayData.find(speciesId);
    if (it == runawayData.end()) return nullptr;
    return &(it->second);
}

float MateSelector::calculatePreferenceOrnamentCovariance(
    const std::vector<Creature*>& population) const {

    if (population.size() < 2) return 0.0f;

    // Calculate means
    float ornamentSum = 0.0f;
    float preferenceSum = 0.0f;
    int count = 0;

    for (const Creature* c : population) {
        if (!c || !c->isAlive()) continue;
        const DiploidGenome& genome = c->getDiploidGenome();
        ornamentSum += genome.getTrait(GeneType::ORNAMENT_INTENSITY);
        preferenceSum += genome.getMatePreferences().ornamentPreference;
        count++;
    }

    if (count < 2) return 0.0f;

    float ornamentMean = ornamentSum / count;
    float preferenceMean = preferenceSum / count;

    // Calculate covariance
    float covariance = 0.0f;
    for (const Creature* c : population) {
        if (!c || !c->isAlive()) continue;
        const DiploidGenome& genome = c->getDiploidGenome();
        float ornamentDev = genome.getTrait(GeneType::ORNAMENT_INTENSITY) - ornamentMean;
        float prefDev = genome.getMatePreferences().ornamentPreference - preferenceMean;
        covariance += ornamentDev * prefDev;
    }

    return covariance / count;
}

// ============================================
// Handicap Principle Traits
// ============================================

float MateSelector::calculateHandicapCost(const OrnamentDisplay& ornament) const {
    // Metabolic cost increases with ornament intensity and complexity
    // Condition-dependent ornaments have higher costs
    float baseCost = ornament.intensity * 0.3f + ornament.complexity * 0.2f;
    float conditionCost = ornament.conditionDependence * ornament.intensity * 0.3f;

    // Symmetry maintenance has its own cost
    float symmetryCost = ornament.symmetry * 0.1f;

    return std::clamp(baseCost + conditionCost + symmetryCost, 0.0f, 1.0f);
}

float MateSelector::evaluateHonesty(const MateQualitySignal& signal,
                                    const Creature& signaler) const {
    // Honest signals have high correlation between signal and actual condition
    float actualCondition = estimateCondition(signaler);

    // Calculate expected signal given condition
    float expectedSignal = actualCondition * signal.conditionDependence;

    // Deviation from expected indicates dishonesty
    float deviation = std::abs(signal.signalStrength - expectedSignal);

    // Costly signals are more likely honest (Zahavian handicap)
    float costFactor = signal.signalCost;

    // Honesty score
    float honesty = (1.0f - deviation) * (0.5f + 0.5f * costFactor);

    return std::clamp(honesty, 0.0f, 1.0f);
}

float MateSelector::calculateZahavianScore(const Creature& male) const {
    OrnamentDisplay ornament = extractOrnamentDisplay(male);
    MateQualitySignal signal = extractQualitySignal(male);

    // Calculate the handicap cost
    float handicapCost = calculateHandicapCost(ornament);

    // Evaluate signal honesty
    float honesty = evaluateHonesty(signal, male);

    // Condition of the male
    float condition = estimateCondition(male);

    // Zahavian handicap: high-cost honest signals from good-condition males
    // are the most attractive
    float zahavianScore = 0.0f;

    if (signal.isHonestSignal && honesty > 0.5f) {
        // Survival despite handicap indicates quality
        zahavianScore = handicapCost * condition * honesty;
    } else {
        // Dishonest signals penalized
        zahavianScore = condition * 0.3f * (1.0f - honesty);
    }

    return std::clamp(zahavianScore, 0.0f, 1.0f);
}

OrnamentDisplay MateSelector::extractOrnamentDisplay(const Creature& creature) const {
    const DiploidGenome& genome = creature.getDiploidGenome();

    OrnamentDisplay ornament;
    ornament.intensity = genome.getTrait(GeneType::ORNAMENT_INTENSITY);
    ornament.complexity = genome.getTrait(GeneType::PATTERN_TYPE);

    // Symmetry derived from genetic quality (heterozygosity as proxy)
    ornament.symmetry = 0.5f + 0.5f * genome.getHeterozygosity();

    // Condition dependence based on metabolic traits
    float metabolicRate = genome.getTrait(GeneType::METABOLIC_RATE);
    ornament.conditionDependence = metabolicRate * ornament.intensity;

    return ornament;
}

MateQualitySignal MateSelector::extractQualitySignal(const Creature& creature) const {
    const DiploidGenome& genome = creature.getDiploidGenome();

    MateQualitySignal signal;

    // Signal strength from ornament intensity and display frequency
    signal.signalStrength = genome.getTrait(GeneType::ORNAMENT_INTENSITY) *
                            genome.getTrait(GeneType::DISPLAY_FREQUENCY);

    // Cost based on metabolic investment
    signal.signalCost = genome.getTrait(GeneType::METABOLIC_RATE) *
                        signal.signalStrength;

    // Condition dependence based on metabolic rate and ornament intensity
    // Higher metabolic rate means ornaments are more condition-dependent
    float metabolicRate = genome.getTrait(GeneType::METABOLIC_RATE);
    float ornamentIntensity = genome.getTrait(GeneType::ORNAMENT_INTENSITY);
    signal.conditionDependence = metabolicRate * 0.5f + ornamentIntensity * 0.5f;

    // Honesty based on condition-dependent expression
    signal.honestyLevel = calculateSignalReliability(creature);

    // Handicap magnitude
    signal.handicapMagnitude = signal.signalCost * signal.signalStrength;

    // Determine if signal is honest
    signal.isHonestSignal = signal.honestyLevel > 0.5f &&
                            signal.signalCost > 0.2f;

    return signal;
}

// ============================================
// Display Behavior Evaluation
// ============================================

float MateSelector::evaluateDisplayQuality(const Creature& creature) const {
    DisplayBehavior display = extractDisplayBehavior(creature);

    // Base attractiveness from display components
    float baseScore = display.calculateAttractiveness();

    // Creativity bonus
    float creativityBonus = evaluateDisplayCreativity(display) * 0.2f;

    // Symmetry bonus
    float symmetryBonus = evaluateDisplaySymmetry(creature) * 0.2f;

    return std::clamp(baseScore + creativityBonus + symmetryBonus, 0.0f, 1.0f);
}

float MateSelector::evaluateDisplayCreativity(const DisplayBehavior& display) const {
    // Creativity based on unpredictability and novelty
    // High creativity = varied timing and intensity

    // Base creativity from the display's creativity trait
    float baseCreativity = display.creativity;

    // Bonus for non-standard display patterns
    float noveltyBonus = 0.0f;
    if (display.frequency > 0.7f || display.frequency < 0.3f) {
        noveltyBonus += 0.1f;  // Unusual frequency
    }
    if (display.duration > 8.0f || display.duration < 3.0f) {
        noveltyBonus += 0.1f;  // Unusual duration
    }
    if (display.vigor > 0.8f) {
        noveltyBonus += 0.15f; // Exceptional vigor
    }

    return std::clamp(baseCreativity + noveltyBonus, 0.0f, 1.0f);
}

float MateSelector::evaluateDisplaySymmetry(const Creature& creature) const {
    const DiploidGenome& genome = creature.getDiploidGenome();

    // Symmetry is a developmental stability indicator
    // High heterozygosity and low genetic load indicate good symmetry
    float heterozygosity = genome.getHeterozygosity();
    float geneticLoad = genome.getGeneticLoad();
    float inbreeding = genome.calculateInbreedingCoeff();

    // Symmetry score
    float symmetry = 0.8f;  // Base symmetry
    symmetry += heterozygosity * 0.1f;           // Heterozygosity bonus
    symmetry -= geneticLoad * 0.2f;              // Genetic load penalty
    symmetry -= inbreeding * 0.15f;              // Inbreeding penalty

    // Condition also affects developmental stability
    float condition = estimateCondition(creature);
    symmetry += (condition - 0.5f) * 0.1f;

    return std::clamp(symmetry, 0.0f, 1.0f);
}

DisplayBehavior MateSelector::extractDisplayBehavior(const Creature& creature) const {
    const DiploidGenome& genome = creature.getDiploidGenome();

    DisplayBehavior display;

    // Duration based on energy and display frequency gene
    float displayFreq = genome.getTrait(GeneType::DISPLAY_FREQUENCY);
    display.duration = 3.0f + displayFreq * 7.0f;  // 3-10 seconds
    display.frequency = displayFreq;

    // Vigor based on current condition and aggression
    float condition = estimateCondition(creature);
    float aggression = genome.getTrait(GeneType::AGGRESSION);
    display.vigor = condition * (0.6f + aggression * 0.4f);

    // Creativity based on curiosity and pattern complexity
    float curiosity = genome.getTrait(GeneType::CURIOSITY);
    float pattern = genome.getTrait(GeneType::PATTERN_TYPE);
    display.creativity = curiosity * 0.5f + pattern * 0.5f;

    return display;
}

// ============================================
// Female Choice Mechanics
// ============================================

float MateSelector::calculateChoiceStrength(const Creature& female) const {
    const DiploidGenome& genome = female.getDiploidGenome();
    MatePreferences prefs = genome.getMatePreferences();

    // Choice strength based on choosiness and condition
    float condition = estimateCondition(female);

    // High-condition females can afford to be choosier
    float choiceStrength = prefs.choosiness * (0.7f + condition * 0.3f);

    // Ornament preference contributes to selectivity
    choiceStrength += prefs.ornamentPreference * 0.2f;

    return std::clamp(choiceStrength, 0.0f, 1.0f);
}

float MateSelector::evaluatePreferenceMatch(const Creature& male,
                                            const Creature& female) const {
    const DiploidGenome& maleGenome = male.getDiploidGenome();
    const DiploidGenome& femaleGenome = female.getDiploidGenome();
    MatePreferences prefs = femaleGenome.getMatePreferences();

    float matchScore = 0.0f;

    // Size preference match
    float maleSize = maleGenome.getTrait(GeneType::SIZE);
    float femaleSize = femaleGenome.getTrait(GeneType::SIZE);
    float sizeRatio = maleSize / femaleSize;

    if (prefs.sizePreference > 0) {
        // Prefers larger: ratio > 1 is good
        matchScore += std::min(1.0f, sizeRatio - 1.0f + 0.5f) * prefs.sizePreference * 0.3f;
    } else if (prefs.sizePreference < 0) {
        // Prefers smaller: ratio < 1 is good
        matchScore += std::min(1.0f, 1.0f - sizeRatio + 0.5f) * (-prefs.sizePreference) * 0.3f;
    } else {
        // No preference: similar size is fine
        matchScore += (1.0f - std::abs(1.0f - sizeRatio)) * 0.15f;
    }

    // Ornament preference match
    float maleOrnament = maleGenome.getTrait(GeneType::ORNAMENT_INTENSITY);
    matchScore += maleOrnament * prefs.ornamentPreference * 0.4f;

    // Similarity preference match
    float geneticDistance = maleGenome.distanceTo(femaleGenome);
    if (prefs.similarityPreference > 0) {
        // Prefers similar
        matchScore += (1.0f - geneticDistance) * prefs.similarityPreference * 0.3f;
    } else if (prefs.similarityPreference < 0) {
        // Prefers different (outbreeding)
        matchScore += geneticDistance * (-prefs.similarityPreference) * 0.3f;
    }

    return std::clamp(matchScore, 0.0f, 1.0f);
}

// ============================================
// Male Competition
// ============================================

CombatResult MateSelector::evaluateCombatSuccess(const Creature& male1,
                                                  const Creature& male2) const {
    CombatResult result;

    float ability1 = calculateFightingAbility(male1);
    float ability2 = calculateFightingAbility(male2);

    // Add randomness to combat outcome
    float noise = Random::range(-0.15f, 0.15f);
    float effectiveAbility1 = ability1 + noise;
    float effectiveAbility2 = ability2 - noise;

    // Determine winner
    result.winner1 = effectiveAbility1 > effectiveAbility2;

    // Calculate damage based on ability difference
    float abilityDiff = std::abs(ability1 - ability2);
    float baseDamage = 0.1f + abilityDiff * 0.3f;

    if (result.winner1) {
        result.winnerDamage = baseDamage * (1.0f - abilityDiff);
        result.loserDamage = baseDamage * (1.0f + abilityDiff);
    } else {
        result.loserDamage = baseDamage * (1.0f + abilityDiff);
        result.winnerDamage = baseDamage * (1.0f - abilityDiff);
    }

    // Dominance change proportional to ability difference
    result.dominanceChange = 0.05f + abilityDiff * 0.1f;

    return result;
}

TerritoryQuality MateSelector::evaluateTerritoryQuality(const Creature& male) const {
    TerritoryQuality territory;

    const DiploidGenome& genome = male.getDiploidGenome();

    // Territory quality based on male traits and current condition
    float condition = estimateCondition(male);
    float size = genome.getTrait(GeneType::SIZE);
    float aggression = genome.getTrait(GeneType::AGGRESSION);

    // Larger, more aggressive males hold better territories
    territory.size = 0.3f + size * 0.4f + aggression * 0.3f;

    // Resource density based on foraging success (energy proxy)
    territory.resourceDensity = 0.3f + condition * 0.5f + size * 0.2f;

    // Safety based on size and awareness
    float visionRange = genome.getTrait(GeneType::VISION_RANGE);
    territory.safetyLevel = 0.4f + size * 0.3f + visionRange * 0.3f;

    // Display visibility based on ornament and territory location
    float ornament = genome.getTrait(GeneType::ORNAMENT_INTENSITY);
    territory.displayVisibility = 0.5f + ornament * 0.3f + territory.size * 0.2f;

    // Overall quality
    territory.overallQuality = territory.resourceDensity * 0.3f +
                               territory.safetyLevel * 0.25f +
                               territory.displayVisibility * 0.25f +
                               territory.size * 0.2f;

    return territory;
}

float MateSelector::evaluateDominanceRank(const Creature& male,
                                          const std::vector<Creature*>& population) const {
    // Calculate rank based on fighting ability relative to population
    float ability = calculateFightingAbility(male);

    int betterCount = 0;
    int totalMales = 0;

    for (const Creature* c : population) {
        if (!c || !c->isAlive()) continue;
        if (c->getType() != male.getType()) continue;
        if (c == &male) continue;

        totalMales++;
        if (calculateFightingAbility(*c) > ability) {
            betterCount++;
        }
    }

    if (totalMales == 0) return 1.0f;  // Only male = top rank

    return 1.0f - (static_cast<float>(betterCount) / totalMales);
}

float MateSelector::calculateFightingAbility(const Creature& male) const {
    const DiploidGenome& genome = male.getDiploidGenome();

    float size = genome.getTrait(GeneType::SIZE);
    float aggression = genome.getTrait(GeneType::AGGRESSION);
    float speed = genome.getTrait(GeneType::SPEED);
    float condition = estimateCondition(male);

    // Fighting ability formula
    float ability = size * 0.35f +          // Size matters most
                   aggression * 0.25f +     // Willingness to fight
                   speed * 0.15f +          // Agility
                   condition * 0.25f;       // Current condition

    return std::clamp(ability, 0.0f, 1.0f);
}

void MateSelector::updateDominanceHierarchy(const std::vector<Creature*>& population) {
    // Clear old ranks
    dominanceRanks.clear();

    // Calculate abilities for all males
    std::vector<std::pair<int, float>> abilities;

    for (const Creature* c : population) {
        if (!c || !c->isAlive()) continue;
        abilities.push_back({c->getID(), calculateFightingAbility(*c)});
    }

    // Sort by ability descending
    std::sort(abilities.begin(), abilities.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    // Assign ranks
    int n = static_cast<int>(abilities.size());
    for (int i = 0; i < n; ++i) {
        float rank = 1.0f - static_cast<float>(i) / std::max(1, n - 1);
        dominanceRanks[abilities[i].first] = rank;
    }
}

float MateSelector::getDominanceRank(int creatureId) const {
    auto it = dominanceRanks.find(creatureId);
    if (it == dominanceRanks.end()) return 0.5f;  // Default middle rank
    return it->second;
}

// ============================================
// Assortative Mating
// ============================================

float MateSelector::calculateAssortativeIndex(const std::vector<Creature*>& population) const {
    if (population.size() < 4) return 0.0f;

    // Calculate correlation between mated pairs' phenotypes
    // Using size as the assortment trait

    std::vector<float> trait1, trait2;

    // Sample pairs (using consecutive creatures as "mated pairs" approximation)
    for (size_t i = 0; i + 1 < population.size(); i += 2) {
        if (!population[i] || !population[i+1]) continue;
        if (!population[i]->isAlive() || !population[i+1]->isAlive()) continue;

        trait1.push_back(population[i]->getDiploidGenome().getTrait(GeneType::SIZE));
        trait2.push_back(population[i+1]->getDiploidGenome().getTrait(GeneType::SIZE));
    }

    if (trait1.size() < 2) return 0.0f;

    // Calculate Pearson correlation
    float n = static_cast<float>(trait1.size());
    float sum1 = std::accumulate(trait1.begin(), trait1.end(), 0.0f);
    float sum2 = std::accumulate(trait2.begin(), trait2.end(), 0.0f);
    float mean1 = sum1 / n;
    float mean2 = sum2 / n;

    float cov = 0.0f, var1 = 0.0f, var2 = 0.0f;
    for (size_t i = 0; i < trait1.size(); ++i) {
        float d1 = trait1[i] - mean1;
        float d2 = trait2[i] - mean2;
        cov += d1 * d2;
        var1 += d1 * d1;
        var2 += d2 * d2;
    }

    float denom = std::sqrt(var1 * var2);
    if (denom < 0.0001f) return 0.0f;

    return cov / denom;  // Pearson r, ranges -1 to 1
}

std::vector<Creature*> MateSelector::enforceAssortativeMating(
    const Creature& chooser,
    const std::vector<Creature*>& candidates,
    float strength) const {

    if (candidates.empty() || strength <= 0.0f) return candidates;

    std::vector<std::pair<Creature*, float>> scored;

    for (Creature* c : candidates) {
        if (!c) continue;
        float similarity = calculatePhenotypicSimilarity(chooser, *c);
        scored.push_back({c, similarity});
    }

    // Sort by similarity descending
    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    // Keep top fraction based on strength
    size_t keepCount = std::max(1ULL,
        static_cast<size_t>(candidates.size() * (1.0f - strength * 0.5f)));

    std::vector<Creature*> filtered;
    for (size_t i = 0; i < keepCount && i < scored.size(); ++i) {
        filtered.push_back(scored[i].first);
    }

    return filtered;
}

std::vector<Creature*> MateSelector::enforceDisassortativeMating(
    const Creature& chooser,
    const std::vector<Creature*>& candidates,
    float strength) const {

    if (candidates.empty() || strength <= 0.0f) return candidates;

    std::vector<std::pair<Creature*, float>> scored;

    for (Creature* c : candidates) {
        if (!c) continue;
        float dissimilarity = 1.0f - calculatePhenotypicSimilarity(chooser, *c);
        scored.push_back({c, dissimilarity});
    }

    // Sort by dissimilarity descending
    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    // Keep top fraction based on strength
    size_t keepCount = std::max(1ULL,
        static_cast<size_t>(candidates.size() * (1.0f - strength * 0.5f)));

    std::vector<Creature*> filtered;
    for (size_t i = 0; i < keepCount && i < scored.size(); ++i) {
        filtered.push_back(scored[i].first);
    }

    return filtered;
}

bool MateSelector::detectSympatricSpeciation(const std::vector<Creature*>& population) const {
    if (population.size() < 20) return false;

    // Calculate assortative mating index
    float assortIndex = calculateAssortativeIndex(population);

    // Check for bimodal distribution in key traits
    std::vector<float> sizes;
    for (const Creature* c : population) {
        if (c && c->isAlive()) {
            sizes.push_back(c->getDiploidGenome().getTrait(GeneType::SIZE));
        }
    }

    if (sizes.size() < 20) return false;

    // Sort and look for gap in middle
    std::sort(sizes.begin(), sizes.end());

    float totalRange = sizes.back() - sizes.front();
    if (totalRange < 0.2f) return false;  // Not enough variation

    // Look for largest gap
    float maxGap = 0.0f;
    size_t gapIndex = 0;
    for (size_t i = 1; i < sizes.size(); ++i) {
        float gap = sizes[i] - sizes[i-1];
        if (gap > maxGap) {
            maxGap = gap;
            gapIndex = i;
        }
    }

    // Sympatric speciation if:
    // 1. High assortative mating
    // 2. Bimodal distribution (large gap in middle)
    bool highAssortment = assortIndex > 0.5f;
    bool bimodal = maxGap > totalRange * 0.3f &&
                   gapIndex > sizes.size() * 0.25f &&
                   gapIndex < sizes.size() * 0.75f;

    return highAssortment && bimodal;
}

// ============================================
// Sexual Conflict
// ============================================

SexualConflictData MateSelector::detectSexualConflict(
    SpeciesId speciesId,
    const std::vector<Creature*>& population) const {

    SexualConflictData conflict;

    // Collect trait values and fitness proxies
    std::vector<float> ornaments;
    std::vector<float> conditions;

    for (const Creature* c : population) {
        if (!c || !c->isAlive()) continue;
        if (c->getDiploidGenome().getSpeciesId() != speciesId) continue;

        float ornament = c->getDiploidGenome().getTrait(GeneType::ORNAMENT_INTENSITY);
        float condition = estimateCondition(*c);

        ornaments.push_back(ornament);
        conditions.push_back(condition);
    }

    if (ornaments.size() < 10) return conflict;

    // Calculate correlation between ornaments and condition
    float n = static_cast<float>(ornaments.size());
    float sumO = std::accumulate(ornaments.begin(), ornaments.end(), 0.0f);
    float sumC = std::accumulate(conditions.begin(), conditions.end(), 0.0f);
    float meanO = sumO / n;
    float meanC = sumC / n;

    float covOC = 0.0f, varO = 0.0f;
    for (size_t i = 0; i < ornaments.size(); ++i) {
        float devO = ornaments[i] - meanO;
        float devC = conditions[i] - meanC;
        covOC += devO * devC;
        varO += devO * devO;
    }

    // Negative covariance suggests sexual conflict
    // (ornaments costly for survival but selected by females)
    if (varO > 0.0001f) {
        float correlation = covOC / std::sqrt(varO * varO);
        if (correlation < -0.2f) {
            conflict.isAntagonistic = true;
            conflict.conflictIntensity = -correlation;
        }
    }

    // Estimate optima
    conflict.maleFitnessOptimum = meanO + 0.2f;   // Males benefit from higher ornaments
    conflict.femaleFitnessOptimum = meanO - 0.1f; // Females prefer moderate (honest signals)

    return conflict;
}

void MateSelector::trackChaseAwaySelection(SpeciesId speciesId,
                                           const std::vector<Creature*>& population) {
    SexualConflictData detected = detectSexualConflict(speciesId, population);

    // Update tracking
    SexualConflictData& stored = conflictData[speciesId];

    if (detected.isAntagonistic) {
        // Chase-away strength increases with sustained conflict
        float newStrength = detected.conflictIntensity * 0.3f + stored.chaseAwayStrength * 0.7f;
        stored.chaseAwayStrength = std::min(1.0f, newStrength + 0.02f);
        stored.isAntagonistic = true;
    } else {
        // Conflict diminishing
        stored.chaseAwayStrength *= 0.95f;
        if (stored.chaseAwayStrength < 0.05f) {
            stored.isAntagonistic = false;
        }
    }

    stored.conflictIntensity = detected.conflictIntensity;
    stored.maleFitnessOptimum = detected.maleFitnessOptimum;
    stored.femaleFitnessOptimum = detected.femaleFitnessOptimum;
}

float MateSelector::getChaseAwayStrength(SpeciesId speciesId) const {
    auto it = conflictData.find(speciesId);
    if (it == conflictData.end()) return 0.0f;
    return it->second.chaseAwayStrength;
}

float MateSelector::calculateSexualConflictCost(const Creature& creature,
                                                 const SexualConflictData& conflict) const {
    if (!conflict.isAntagonistic) return 0.0f;

    float ornament = creature.getDiploidGenome().getTrait(GeneType::ORNAMENT_INTENSITY);

    // Cost depends on deviation from optimum
    // For simplicity, assume creature is male (ornament bearer)
    float deviation = std::abs(ornament - conflict.maleFitnessOptimum);

    return deviation * conflict.conflictIntensity * 0.5f;
}

// ============================================
// Utility Methods
// ============================================

const char* MateSelector::isolationTypeToString(IsolationType type) {
    switch (type) {
        case IsolationType::NONE: return "None";
        case IsolationType::BEHAVIORAL: return "Behavioral";
        case IsolationType::TEMPORAL: return "Temporal";
        case IsolationType::MECHANICAL: return "Mechanical";
        case IsolationType::GAMETIC: return "Gametic";
        case IsolationType::ECOLOGICAL: return "Ecological";
        case IsolationType::GEOGRAPHIC: return "Geographic";
        default: return "Unknown";
    }
}

const char* MateSelector::choiceModeToString(ChoiceMode mode) {
    switch (mode) {
        case ChoiceMode::THRESHOLD: return "Threshold";
        case ChoiceMode::BEST_OF_N: return "BestOfN";
        case ChoiceMode::SEQUENTIAL: return "Sequential";
        default: return "Unknown";
    }
}

void MateSelector::reset() {
    runawayData.clear();
    conflictData.clear();
    dominanceRanks.clear();
    ornamentHistory.clear();
    preferenceHistory.clear();
}

void MateSelector::updateGeneration(const std::vector<Creature*>& population) {
    // Group by species
    std::unordered_map<SpeciesId, std::vector<Creature*>> bySpecies;

    for (Creature* c : population) {
        if (c && c->isAlive()) {
            bySpecies[c->getDiploidGenome().getSpeciesId()].push_back(c);
        }
    }

    // Update tracking for each species
    for (auto& [speciesId, members] : bySpecies) {
        if (members.size() >= 5) {
            trackOrnamentEvolution(speciesId, members);
            trackChaseAwaySelection(speciesId, members);
        }
    }

    // Update dominance hierarchy
    updateDominanceHierarchy(population);
}

// ============================================
// Private Helper Methods (existing)
// ============================================

float MateSelector::evaluateByPreferences(const MatePreferences& prefs,
                                          const DiploidGenome& chooser,
                                          const DiploidGenome& candidate) const {
    float score = 0.0f;

    // Size preference
    float candidateSize = candidate.getTrait(GeneType::SIZE);
    float chooserSize = chooser.getTrait(GeneType::SIZE);
    float sizeRatio = candidateSize / chooserSize;

    if (prefs.sizePreference > 0) {
        // Prefers larger
        score += std::min(1.0f, sizeRatio) * prefs.sizePreference;
    } else if (prefs.sizePreference < 0) {
        // Prefers smaller
        score += std::min(1.0f, 1.0f / sizeRatio) * (-prefs.sizePreference);
    }

    // Ornament preference
    float ornamentIntensity = candidate.getTrait(GeneType::ORNAMENT_INTENSITY);
    score += ornamentIntensity * prefs.ornamentPreference;

    // Similarity preference
    float geneticDistance = chooser.distanceTo(candidate);
    if (prefs.similarityPreference > 0) {
        // Prefers similar (assortative mating)
        score += (1.0f - geneticDistance) * prefs.similarityPreference;
    } else if (prefs.similarityPreference < 0) {
        // Prefers different (disassortative mating)
        score += geneticDistance * (-prefs.similarityPreference);
    }

    // Normalize
    return std::clamp(score / 2.0f, 0.0f, 1.0f);
}

float MateSelector::evaluateGeneticCompatibility(const DiploidGenome& g1,
                                                 const DiploidGenome& g2) const {
    float distance = g1.distanceTo(g2);

    // Optimal distance is moderate - not too similar (inbreeding)
    // and not too different (incompatibility)
    const float optimalDistance = 0.05f;
    const float tolerance = 0.1f;

    float deviation = std::abs(distance - optimalDistance);
    float compatibility = 1.0f - (deviation / tolerance);

    return std::clamp(compatibility, 0.0f, 1.0f);
}

float MateSelector::evaluatePhysicalCompatibility(const DiploidGenome& g1,
                                                  const DiploidGenome& g2) const {
    float size1 = g1.getTrait(GeneType::SIZE);
    float size2 = g2.getTrait(GeneType::SIZE);

    // Size ratio shouldn't be too extreme
    float ratio = std::max(size1, size2) / std::min(size1, size2);

    // Perfect compatibility at ratio 1, decreases as ratio increases
    float compatibility = 1.0f - std::min(1.0f, (ratio - 1.0f) / 2.0f);

    return compatibility;
}

float MateSelector::evaluateTemporalCompatibility(const DiploidGenome& g1,
                                                  const DiploidGenome& g2) const {
    float activity1 = g1.getTrait(GeneType::ACTIVITY_TIME);
    float activity2 = g2.getTrait(GeneType::ACTIVITY_TIME);

    // 0 = nocturnal, 1 = diurnal
    // Overlap is highest when both are similar
    float diff = std::abs(activity1 - activity2);

    return 1.0f - diff;
}

float MateSelector::evaluateEcologicalCompatibility(const DiploidGenome& g1,
                                                    const DiploidGenome& g2) const {
    EcologicalNiche niche1 = g1.getEcologicalNiche();
    EcologicalNiche niche2 = g2.getEcologicalNiche();

    float nicheDistance = niche1.distanceTo(niche2);

    return 1.0f - nicheDistance;
}

// ============================================
// Private Helper Methods (new)
// ============================================

float MateSelector::calculateGeneticSimilarity(const Creature& c1, const Creature& c2) const {
    float distance = c1.getDiploidGenome().distanceTo(c2.getDiploidGenome());
    return 1.0f - distance;
}

float MateSelector::calculatePhenotypicSimilarity(const Creature& c1, const Creature& c2) const {
    const DiploidGenome& g1 = c1.getDiploidGenome();
    const DiploidGenome& g2 = c2.getDiploidGenome();

    // Compare multiple phenotypic traits
    float sizeDiff = std::abs(g1.getTrait(GeneType::SIZE) - g2.getTrait(GeneType::SIZE));
    float colorDiff = glm::length(g1.getColor() - g2.getColor()) / std::sqrt(3.0f);
    float ornamentDiff = std::abs(g1.getTrait(GeneType::ORNAMENT_INTENSITY) -
                                   g2.getTrait(GeneType::ORNAMENT_INTENSITY));
    float speedDiff = std::abs(g1.getTrait(GeneType::SPEED) - g2.getTrait(GeneType::SPEED));

    float avgDiff = (sizeDiff + colorDiff + ornamentDiff + speedDiff) / 4.0f;

    return 1.0f - std::clamp(avgDiff, 0.0f, 1.0f);
}

float MateSelector::estimateCondition(const Creature& creature) const {
    // Condition based on current energy and health proxies
    float energy = creature.getEnergy();
    float maxEnergy = 200.0f;  // Typical max energy

    float energyRatio = std::clamp(energy / maxEnergy, 0.0f, 1.0f);

    // Age penalty (very young or very old = lower condition)
    float age = creature.getAge();
    float ageFactor = 1.0f;
    if (age < 5.0f) {
        ageFactor = age / 5.0f;  // Young penalty
    } else if (age > 100.0f) {
        ageFactor = std::max(0.0f, 1.0f - (age - 100.0f) / 200.0f);  // Old penalty
    }

    return energyRatio * ageFactor;
}

float MateSelector::calculateSignalReliability(const Creature& signaler) const {
    const DiploidGenome& genome = signaler.getDiploidGenome();

    // Signals are more reliable when:
    // 1. Condition-dependent (vary with health)
    // 2. Costly to produce
    // 3. Low genetic load (developmental stability)

    float condition = estimateCondition(signaler);
    float ornament = genome.getTrait(GeneType::ORNAMENT_INTENSITY);
    float metabolic = genome.getTrait(GeneType::METABOLIC_RATE);
    float geneticLoad = genome.getGeneticLoad();

    // Cost factor (higher metabolic + ornament = more costly signal)
    float costFactor = ornament * metabolic;

    // Condition-dependence (signal should track condition)
    float expectedOrnament = condition * 0.7f + 0.3f;  // Baseline + condition component
    float trackingAccuracy = 1.0f - std::abs(ornament - expectedOrnament);

    // Genetic quality
    float geneticQuality = 1.0f - geneticLoad * 2.0f;

    float reliability = costFactor * 0.3f +
                       trackingAccuracy * 0.4f +
                       geneticQuality * 0.3f;

    return std::clamp(reliability, 0.0f, 1.0f);
}

} // namespace genetics
