#include "AdaptiveRadiation.h"
#include "Species.h"
#include "../Creature.h"
#include "../../environment/Terrain.h"
#include "../../utils/Random.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>
#include <numeric>

namespace genetics {

// =============================================================================
// CONSTRUCTION AND INITIALIZATION
// =============================================================================

AdaptiveRadiationTracker::AdaptiveRadiationTracker()
    : speciationTracker_(nullptr)
    , statsCacheValid_(false)
    , radiationDetectionThreshold_(0.05f)
    , minSuccessfulRadiationSize_(3)
    , innovationThreshold_(0.3f)
    , rateCalculationWindow_(50)
    , nextRadiationId_(1)
    , nextLineageId_(1)
    , nextInnovationId_(1)
    , nextColonizationId_(1) {
}

AdaptiveRadiationTracker::~AdaptiveRadiationTracker() = default;

void AdaptiveRadiationTracker::initialize(SpeciationTracker* speciationTracker) {
    speciationTracker_ = speciationTracker;
    reset();
}

void AdaptiveRadiationTracker::reset() {
    radiationEvents_.clear();
    radiationById_.clear();
    lineages_.clear();
    keyInnovations_.clear();
    colonizationEvents_.clear();
    statsCacheValid_ = false;
    nextRadiationId_ = 1;
    nextLineageId_ = 1;
    nextInnovationId_ = 1;
    nextColonizationId_ = 1;
}

// =============================================================================
// RADIATION DETECTION
// =============================================================================

bool AdaptiveRadiationTracker::detectRadiationStart(
    const std::vector<SpeciationEvent>& speciationEvents,
    const std::map<std::string, float>& environmentData) {

    if (!speciationTracker_) return false;

    bool radiationDetected = false;

    // Calculate recent speciation rate
    float recentSpeciationRate = 0.0f;
    if (!speciationEvents.empty()) {
        int recentWindow = std::min(static_cast<int>(speciationEvents.size()), rateCalculationWindow_);
        recentSpeciationRate = static_cast<float>(recentWindow) / rateCalculationWindow_;
    }

    // Check for elevated speciation rate
    if (recentSpeciationRate > radiationDetectionThreshold_) {
        // Group speciation events by parent species to find potential radiations
        std::map<SpeciesId, std::vector<SpeciationEvent>> eventsByParent;
        for (const auto& event : speciationEvents) {
            eventsByParent[event.parentId].push_back(event);
        }

        for (const auto& [parentId, events] : eventsByParent) {
            if (events.size() >= 2) {
                // Potential radiation from this species
                std::vector<SpeciesId> relevantSpecies;
                for (const auto& event : events) {
                    relevantSpecies.push_back(event.childId);
                }

                RadiationEvent* existingRadiation = nullptr;
                for (auto& rad : radiationEvents_) {
                    if (rad->ancestorSpeciesId == parentId && rad->isOngoing) {
                        existingRadiation = rad.get();
                        break;
                    }
                }

                if (!existingRadiation && !events.empty()) {
                    // Create new radiation event
                    auto radiation = std::make_unique<RadiationEvent>();
                    radiation->radiationId = nextRadiationId_++;
                    radiation->startGeneration = events[0].generation;
                    radiation->ancestorSpeciesId = parentId;
                    radiation->triggerType = RadiationTrigger::UNKNOWN;
                    radiation->isOngoing = true;

                    // Set ancestor name
                    const Species* ancestorSpecies = speciationTracker_->getSpecies(parentId);
                    if (ancestorSpecies) {
                        radiation->ancestorSpeciesName = ancestorSpecies->getName();
                    }

                    // Add descendant species
                    for (const auto& event : events) {
                        radiation->descendantSpeciesIds.push_back(event.childId);
                        radiation->extantDescendants.push_back(event.childId);
                    }

                    radiation->timeToFirstSpeciation = 0;
                    radiation->diversificationRate = recentSpeciationRate;
                    radiation->speciationRate = recentSpeciationRate;

                    // Infer environment context
                    radiation->environmentContext = inferEnvironmentContext(environmentData);

                    // Determine trigger type
                    auto extinctionIt = environmentData.find("mass_extinction");
                    auto colonizationIt = environmentData.find("new_habitat");

                    if (extinctionIt != environmentData.end() && extinctionIt->second > 0.5f) {
                        radiation->triggerType = RadiationTrigger::MASS_EXTINCTION;
                        radiation->triggerDescription = "Post-extinction niche vacancy";
                    } else if (colonizationIt != environmentData.end() && colonizationIt->second > 0.5f) {
                        radiation->triggerType = RadiationTrigger::COLONIZATION;
                        radiation->triggerDescription = "New habitat colonization";
                    }

                    radiationById_[radiation->radiationId] = radiation.get();
                    radiationEvents_.push_back(std::move(radiation));
                    radiationDetected = true;

                    std::cout << "[ADAPTIVE RADIATION] New radiation detected from Species_"
                              << parentId << " (ID: " << radiationEvents_.back()->radiationId
                              << ")" << std::endl;
                }
            }
        }
    }

    // Check for mass extinction trigger
    auto extinctionRateIt = environmentData.find("extinction_rate");
    if (extinctionRateIt != environmentData.end() && extinctionRateIt->second > 0.3f) {
        // High extinction may create vacant niches
        // This will be picked up in future speciation bursts
    }

    if (radiationDetected) {
        invalidateStatsCache();
    }

    return radiationDetected;
}

RadiationEvent* AdaptiveRadiationTracker::detectRadiationByTrigger(
    RadiationTrigger trigger,
    const std::vector<SpeciesId>& relevantSpecies,
    int generation) {

    if (relevantSpecies.empty()) return nullptr;

    // Check if we already have an ongoing radiation for these species
    for (auto& rad : radiationEvents_) {
        if (!rad->isOngoing) continue;
        for (SpeciesId spId : relevantSpecies) {
            if (std::find(rad->descendantSpeciesIds.begin(),
                         rad->descendantSpeciesIds.end(), spId) != rad->descendantSpeciesIds.end()) {
                return rad.get();
            }
        }
    }

    // Create new radiation event
    auto radiation = std::make_unique<RadiationEvent>();
    radiation->radiationId = nextRadiationId_++;
    radiation->startGeneration = generation;
    radiation->ancestorSpeciesId = relevantSpecies[0];
    radiation->triggerType = trigger;
    radiation->isOngoing = true;

    // Set trigger description
    switch (trigger) {
        case RadiationTrigger::COLONIZATION:
            radiation->triggerDescription = "Island/habitat colonization event";
            break;
        case RadiationTrigger::MASS_EXTINCTION:
            radiation->triggerDescription = "Mass extinction creating vacant niches";
            break;
        case RadiationTrigger::KEY_INNOVATION:
            radiation->triggerDescription = "Key evolutionary innovation detected";
            break;
        case RadiationTrigger::NICHE_EXPANSION:
            radiation->triggerDescription = "New ecological opportunities available";
            break;
        case RadiationTrigger::GEOGRAPHIC_ISOLATION:
            radiation->triggerDescription = "Geographic fragmentation of population";
            break;
        default:
            radiation->triggerDescription = "Unknown trigger";
            break;
    }

    for (SpeciesId spId : relevantSpecies) {
        radiation->descendantSpeciesIds.push_back(spId);
        radiation->extantDescendants.push_back(spId);
    }

    RadiationEvent* ptr = radiation.get();
    radiationById_[radiation->radiationId] = ptr;
    radiationEvents_.push_back(std::move(radiation));

    invalidateStatsCache();
    return ptr;
}

// =============================================================================
// RADIATION PROGRESS TRACKING
// =============================================================================

void AdaptiveRadiationTracker::trackRadiationProgress(int generation) {
    for (auto& radiation : radiationEvents_) {
        if (radiation->isOngoing) {
            updateRadiationEvent(radiation->radiationId, generation);
        }
    }
}

void AdaptiveRadiationTracker::updateRadiationEvent(uint64_t radiationId, int generation) {
    RadiationEvent* radiation = getRadiationMutable(radiationId);
    if (!radiation || !radiation->isOngoing) return;

    // Update duration
    radiation->duration = generation - radiation->startGeneration;

    // Update descendant lists
    updateDescendantLists(radiation);

    // Update diversification metrics
    updateDiversificationMetrics(radiation, generation);

    // Update morphological disparity
    if (speciationTracker_) {
        std::vector<const Species*> descendants;
        for (SpeciesId spId : radiation->extantDescendants) {
            const Species* sp = speciationTracker_->getSpecies(spId);
            if (sp && !sp->isExtinct()) {
                descendants.push_back(sp);
            }
        }

        if (!descendants.empty()) {
            float currentDisparity = measureMorphologicalDisparity(descendants);
            radiation->morphologicalDisparity = currentDisparity;

            if (currentDisparity > radiation->maxMorphologicalDisparity) {
                radiation->maxMorphologicalDisparity = currentDisparity;
                radiation->maxDisparityGeneration = generation;
            }
        }
    }

    // Update niche exploitation
    std::vector<NicheType> niches = getNichesInRadiation(radiationId);
    radiation->nichesExploited = niches;
    radiation->nichePackingDensity = calculateNichePacking(radiationId);

    // Check for niche saturation
    if (!radiation->nicheSaturationReached && radiation->nichePackingDensity > 0.8f) {
        radiation->nicheSaturationReached = true;
        radiation->saturationGeneration = generation;
    }

    // Check if radiation should be completed
    if (shouldCompleteRadiation(radiation)) {
        completeRadiation(radiationId, generation, "Diversification slowdown detected");
    }

    invalidateStatsCache();
}

void AdaptiveRadiationTracker::completeRadiation(uint64_t radiationId, int generation,
                                                  const std::string& reason) {
    RadiationEvent* radiation = getRadiationMutable(radiationId);
    if (!radiation || !radiation->isOngoing) return;

    radiation->isOngoing = false;
    radiation->endGeneration = generation;
    radiation->duration = generation - radiation->startGeneration;

    std::cout << "[ADAPTIVE RADIATION] Radiation " << radiationId << " completed at generation "
              << generation << ". Descendants: " << radiation->getTotalDescendantCount()
              << ", Duration: " << radiation->duration << " generations";
    if (!reason.empty()) {
        std::cout << " (" << reason << ")";
    }
    std::cout << std::endl;

    invalidateStatsCache();
}

void AdaptiveRadiationTracker::updateDescendantLists(RadiationEvent* radiation) {
    if (!radiation || !speciationTracker_) return;

    radiation->extantDescendants.clear();
    radiation->extinctDescendants.clear();

    for (SpeciesId spId : radiation->descendantSpeciesIds) {
        const Species* sp = speciationTracker_->getSpecies(spId);
        if (sp) {
            if (sp->isExtinct()) {
                radiation->extinctDescendants.push_back(spId);
            } else {
                radiation->extantDescendants.push_back(spId);
            }
        }
    }

    // Check for new descendants from the phylogenetic tree
    const PhylogeneticTree& tree = speciationTracker_->getPhylogeneticTree();
    std::vector<SpeciesId> allDescendants = tree.getDescendants(radiation->ancestorSpeciesId);

    for (SpeciesId spId : allDescendants) {
        if (std::find(radiation->descendantSpeciesIds.begin(),
                     radiation->descendantSpeciesIds.end(), spId) == radiation->descendantSpeciesIds.end()) {
            radiation->descendantSpeciesIds.push_back(spId);

            const Species* sp = speciationTracker_->getSpecies(spId);
            if (sp && !sp->isExtinct()) {
                radiation->extantDescendants.push_back(spId);
            } else if (sp) {
                radiation->extinctDescendants.push_back(spId);
            }
        }
    }
}

void AdaptiveRadiationTracker::updateDiversificationMetrics(RadiationEvent* radiation, int generation) {
    if (!radiation) return;

    int extantCount = static_cast<int>(radiation->extantDescendants.size());
    int extinctCount = static_cast<int>(radiation->extinctDescendants.size());
    int totalCount = extantCount + extinctCount;

    if (radiation->duration > 0) {
        // Calculate speciation rate (new species per generation)
        radiation->speciationRate = static_cast<float>(totalCount) / radiation->duration;

        // Calculate extinction rate
        radiation->extinctionRate = static_cast<float>(extinctCount) / radiation->duration;

        // Net diversification rate = speciation - extinction
        radiation->diversificationRate = radiation->speciationRate - radiation->extinctionRate;

        // Track peak diversification
        if (radiation->diversificationRate > radiation->peakDiversificationRate) {
            radiation->peakDiversificationRate = radiation->diversificationRate;
            radiation->peakDiversificationGeneration = generation;
        }
    }
}

bool AdaptiveRadiationTracker::shouldCompleteRadiation(const RadiationEvent* radiation) const {
    if (!radiation || !radiation->isOngoing) return false;

    // Complete if diversification rate has dropped significantly from peak
    if (radiation->peakDiversificationRate > 0.0f) {
        float currentRatio = radiation->diversificationRate / radiation->peakDiversificationRate;
        if (currentRatio < 0.2f && radiation->duration > rateCalculationWindow_) {
            return true;
        }
    }

    // Complete if niche saturation reached and no new species for a while
    if (radiation->nicheSaturationReached && radiation->diversificationRate <= 0.0f) {
        return true;
    }

    // Complete if all extant descendants have gone extinct
    if (radiation->extantDescendants.empty() && !radiation->descendantSpeciesIds.empty()) {
        return true;
    }

    return false;
}

// =============================================================================
// DIVERSIFICATION RATE CALCULATION
// =============================================================================

float AdaptiveRadiationTracker::calculateDiversificationRate(SpeciesId speciesId) const {
    if (!speciationTracker_) return 0.0f;

    // Find the radiation this species is part of
    for (const auto& radiation : radiationEvents_) {
        if (std::find(radiation->descendantSpeciesIds.begin(),
                     radiation->descendantSpeciesIds.end(), speciesId) != radiation->descendantSpeciesIds.end()) {
            return radiation->diversificationRate;
        }
    }

    // Calculate based on lineage if no radiation found
    auto it = lineages_.find(speciesId);
    if (it != lineages_.end()) {
        return it->second.netDiversification;
    }

    return 0.0f;
}

float AdaptiveRadiationTracker::calculateRadiationDiversificationRate(uint64_t radiationId) const {
    const RadiationEvent* radiation = getRadiation(radiationId);
    if (!radiation) return 0.0f;

    return radiation->diversificationRate;
}

bool AdaptiveRadiationTracker::calculateBirthDeathRates(uint64_t lineageId,
                                                         float& outBirthRate,
                                                         float& outDeathRate) const {
    auto it = lineages_.find(lineageId);
    if (it == lineages_.end()) {
        outBirthRate = 0.0f;
        outDeathRate = 0.0f;
        return false;
    }

    outBirthRate = it->second.birthRate;
    outDeathRate = it->second.deathRate;
    return true;
}

// =============================================================================
// KEY INNOVATION DETECTION
// =============================================================================

KeyInnovation* AdaptiveRadiationTracker::detectKeyInnovation(
    const DiploidGenome& genome,
    const DiploidGenome& ancestralGenome) {

    // Compare trait values across all gene types
    static const std::vector<GeneType> importantTraits = {
        GeneType::AERIAL_APTITUDE,
        GeneType::AQUATIC_APTITUDE,
        GeneType::WING_SPAN,
        GeneType::FIN_SIZE,
        GeneType::ECHOLOCATION_ABILITY,
        GeneType::VISION_RANGE,
        GeneType::SIZE,
        GeneType::SPEED
    };

    for (GeneType type : importantTraits) {
        float currentValue = genome.getTrait(type);
        float ancestralValue = ancestralGenome.getTrait(type);

        float change = std::abs(currentValue - ancestralValue);
        float relativeChange = (ancestralValue != 0.0f) ?
                               change / std::abs(ancestralValue) : change;

        if (relativeChange > innovationThreshold_) {
            KeyInnovation innovation;
            innovation.innovationId = nextInnovationId_++;
            innovation.detectionGeneration = 0; // Will be set by caller
            innovation.originSpeciesId = genome.getSpeciesId();
            innovation.primaryGene = type;
            innovation.traitChange = change;
            innovation.ancestralValue = ancestralValue;
            innovation.derivedValue = currentValue;
            innovation.triggeredRadiation = false;

            // Determine description and niches unlocked based on trait type
            switch (type) {
                case GeneType::AERIAL_APTITUDE:
                case GeneType::WING_SPAN:
                    innovation.description = "Evolution of flight capability";
                    innovation.nicheUnlocked.push_back(NicheType::AERIAL);
                    break;
                case GeneType::AQUATIC_APTITUDE:
                case GeneType::FIN_SIZE:
                    innovation.description = "Aquatic adaptation";
                    innovation.nicheUnlocked.push_back(NicheType::AQUATIC_PELAGIC);
                    innovation.nicheUnlocked.push_back(NicheType::AQUATIC_SURFACE);
                    break;
                case GeneType::ECHOLOCATION_ABILITY:
                    innovation.description = "Evolution of echolocation";
                    innovation.nicheUnlocked.push_back(NicheType::NOCTURNAL);
                    innovation.nicheUnlocked.push_back(NicheType::CAVE_DWELLING);
                    break;
                case GeneType::SIZE:
                    if (currentValue > ancestralValue) {
                        innovation.description = "Gigantism evolution";
                        innovation.nicheUnlocked.push_back(NicheType::PREDATOR_LARGE);
                    } else {
                        innovation.description = "Dwarfism evolution";
                        innovation.nicheUnlocked.push_back(NicheType::FOSSORIAL);
                    }
                    break;
                default:
                    innovation.description = "Major trait innovation";
                    break;
            }

            keyInnovations_.push_back(innovation);
            return &keyInnovations_.back();
        }
    }

    return nullptr;
}

KeyInnovation* AdaptiveRadiationTracker::detectKeyInnovationFromPhenotype(
    const Phenotype& currentPhenotype,
    const Phenotype& ancestralPhenotype,
    SpeciesId speciesId,
    int generation) {

    // Check for major phenotypic changes
    float aerialChange = std::abs(currentPhenotype.aerialAptitude - ancestralPhenotype.aerialAptitude);
    float aquaticChange = std::abs(currentPhenotype.aquaticAptitude - ancestralPhenotype.aquaticAptitude);
    float sizeChange = std::abs(currentPhenotype.size - ancestralPhenotype.size);
    float visionChange = std::abs(currentPhenotype.visionRange - ancestralPhenotype.visionRange);

    KeyInnovation* innovation = nullptr;

    if (aerialChange > innovationThreshold_) {
        KeyInnovation newInnovation;
        newInnovation.innovationId = nextInnovationId_++;
        newInnovation.detectionGeneration = generation;
        newInnovation.originSpeciesId = speciesId;
        newInnovation.primaryGene = GeneType::AERIAL_APTITUDE;
        newInnovation.traitChange = aerialChange;
        newInnovation.ancestralValue = ancestralPhenotype.aerialAptitude;
        newInnovation.derivedValue = currentPhenotype.aerialAptitude;
        newInnovation.description = "Evolution of flight capability";
        newInnovation.nicheUnlocked.push_back(NicheType::AERIAL);
        newInnovation.triggeredRadiation = false;

        keyInnovations_.push_back(newInnovation);
        innovation = &keyInnovations_.back();
    }
    else if (aquaticChange > innovationThreshold_) {
        KeyInnovation newInnovation;
        newInnovation.innovationId = nextInnovationId_++;
        newInnovation.detectionGeneration = generation;
        newInnovation.originSpeciesId = speciesId;
        newInnovation.primaryGene = GeneType::AQUATIC_APTITUDE;
        newInnovation.traitChange = aquaticChange;
        newInnovation.ancestralValue = ancestralPhenotype.aquaticAptitude;
        newInnovation.derivedValue = currentPhenotype.aquaticAptitude;
        newInnovation.description = "Aquatic adaptation";
        newInnovation.nicheUnlocked.push_back(NicheType::AQUATIC_PELAGIC);
        newInnovation.triggeredRadiation = false;

        keyInnovations_.push_back(newInnovation);
        innovation = &keyInnovations_.back();
    }
    else if (sizeChange > innovationThreshold_ * 2.0f) {
        KeyInnovation newInnovation;
        newInnovation.innovationId = nextInnovationId_++;
        newInnovation.detectionGeneration = generation;
        newInnovation.originSpeciesId = speciesId;
        newInnovation.primaryGene = GeneType::SIZE;
        newInnovation.traitChange = sizeChange;
        newInnovation.ancestralValue = ancestralPhenotype.size;
        newInnovation.derivedValue = currentPhenotype.size;
        newInnovation.description = (currentPhenotype.size > ancestralPhenotype.size) ?
                                    "Gigantism evolution" : "Dwarfism evolution";
        newInnovation.triggeredRadiation = false;

        keyInnovations_.push_back(newInnovation);
        innovation = &keyInnovations_.back();
    }

    return innovation;
}

std::vector<KeyInnovation> AdaptiveRadiationTracker::getRadiationTriggeringInnovations() const {
    std::vector<KeyInnovation> result;
    for (const auto& innovation : keyInnovations_) {
        if (innovation.triggeredRadiation) {
            result.push_back(innovation);
        }
    }
    return result;
}

// =============================================================================
// ISLAND BIOGEOGRAPHY
// =============================================================================

IslandColonizationData AdaptiveRadiationTracker::islandColonizationEffect(
    const std::vector<const Species*>& colonizers,
    const std::string& islandId,
    const std::map<std::string, float>& islandProperties) {

    IslandColonizationData data;
    data.eventId = nextColonizationId_++;
    data.colonizationGeneration = 0; // Will be set by caller
    data.islandIdentifier = islandId;

    if (colonizers.empty()) return data;

    // Source species is the first colonizer
    data.sourceSpeciesId = colonizers[0]->getId();

    // Calculate founder population size
    int totalPopulation = 0;
    float totalDiversity = 0.0f;

    for (const Species* sp : colonizers) {
        totalPopulation += sp->getStats().size;
        totalDiversity += sp->getStats().averageHeterozygosity;
    }

    data.founderPopulation = totalPopulation;
    data.founderGeneticDiversity = colonizers.empty() ? 0.0f :
                                   totalDiversity / colonizers.size();

    // Extract island properties
    auto sizeIt = islandProperties.find("size");
    data.islandSize = (sizeIt != islandProperties.end()) ? sizeIt->second : 1.0f;

    auto distanceIt = islandProperties.find("distance");
    data.distanceFromSource = (distanceIt != islandProperties.end()) ? distanceIt->second : 0.0f;

    auto resourceIt = islandProperties.find("resources");
    data.resourceAvailability = (resourceIt != islandProperties.end()) ? resourceIt->second : 1.0f;

    auto nichesIt = islandProperties.find("available_niches");
    data.availableNiches = (nichesIt != islandProperties.end()) ?
                           static_cast<int>(nichesIt->second) : 10;

    // Founder effect analysis
    // Small founder populations lead to reduced genetic diversity
    if (data.hasSignificantFounderEffect()) {
        // Strong founder effect - reduced diversity but potential for rapid drift
        data.founderGeneticDiversity *= 0.5f;
    }

    // Island isolation bonus to speciation
    // More isolated islands have higher speciation rates
    float isolationBonus = std::min(1.0f, data.distanceFromSource / 100.0f);

    // Determine if radiation will likely occur
    // Conditions favorable for radiation:
    // 1. Many available niches
    // 2. Few competitors (founder effect)
    // 3. Isolated location
    // 4. Sufficient resources
    float radiationProbability = 0.0f;
    radiationProbability += (data.availableNiches > 5) ? 0.3f : 0.1f;
    radiationProbability += (data.founderPopulation < 50) ? 0.2f : 0.05f;
    radiationProbability += isolationBonus * 0.3f;
    radiationProbability += (data.resourceAvailability > 0.7f) ? 0.2f : 0.05f;

    data.triggeredRadiation = (radiationProbability > 0.5f);

    return data;
}

void AdaptiveRadiationTracker::recordColonization(const IslandColonizationData& data) {
    colonizationEvents_.push_back(data);

    std::cout << "[COLONIZATION] Island " << data.islandIdentifier
              << " colonized by Species_" << data.sourceSpeciesId
              << " (founders: " << data.founderPopulation
              << ", diversity: " << data.founderGeneticDiversity << ")" << std::endl;

    invalidateStatsCache();
}

bool AdaptiveRadiationTracker::hasIslandRadiation(const std::string& islandId) const {
    for (const auto& event : colonizationEvents_) {
        if (event.islandIdentifier == islandId && event.triggeredRadiation) {
            return true;
        }
    }
    return false;
}

// =============================================================================
// MORPHOLOGICAL DISPARITY
// =============================================================================

float AdaptiveRadiationTracker::measureMorphologicalDisparity(
    const std::vector<const Species*>& species) const {

    if (species.size() < 2) return 0.0f;

    // Extract trait vectors for each species
    std::vector<std::vector<float>> traitVectors;
    for (const Species* sp : species) {
        if (sp) {
            traitVectors.push_back(extractTraitVector(sp));
        }
    }

    if (traitVectors.size() < 2) return 0.0f;

    // Calculate disparity as variance of trait vectors
    return calculateTraitVariance(traitVectors);
}

float AdaptiveRadiationTracker::measureRadiationDisparity(uint64_t radiationId) const {
    const RadiationEvent* radiation = getRadiation(radiationId);
    if (!radiation || !speciationTracker_) return 0.0f;

    std::vector<const Species*> descendants;
    for (SpeciesId spId : radiation->extantDescendants) {
        const Species* sp = speciationTracker_->getSpecies(spId);
        if (sp && !sp->isExtinct()) {
            descendants.push_back(sp);
        }
    }

    return measureMorphologicalDisparity(descendants);
}

float AdaptiveRadiationTracker::calculateTraitDistance(const Species* species1,
                                                        const Species* species2) const {
    if (!species1 || !species2) return 0.0f;

    std::vector<float> traits1 = extractTraitVector(species1);
    std::vector<float> traits2 = extractTraitVector(species2);

    if (traits1.size() != traits2.size()) return 0.0f;

    // Euclidean distance
    float sumSquares = 0.0f;
    for (size_t i = 0; i < traits1.size(); i++) {
        float diff = traits1[i] - traits2[i];
        sumSquares += diff * diff;
    }

    return std::sqrt(sumSquares);
}

std::vector<std::pair<int, float>> AdaptiveRadiationTracker::getDisparityThroughTime(
    uint64_t radiationId) const {

    std::vector<std::pair<int, float>> result;

    const RadiationEvent* radiation = getRadiation(radiationId);
    if (!radiation) return result;

    // Return stored values
    result.push_back({radiation->startGeneration, radiation->initialMorphology});
    if (radiation->maxDisparityGeneration > radiation->startGeneration) {
        result.push_back({radiation->maxDisparityGeneration, radiation->maxMorphologicalDisparity});
    }
    if (!radiation->isOngoing && radiation->endGeneration > 0) {
        result.push_back({radiation->endGeneration, radiation->morphologicalDisparity});
    }

    return result;
}

std::vector<float> AdaptiveRadiationTracker::extractTraitVector(const Species* species) const {
    std::vector<float> traits;
    if (!species) return traits;

    // Get representative genome traits
    DiploidGenome genome = species->getRepresentativeGenome();
    Phenotype phenotype = genome.express();

    // Core morphological traits
    traits.push_back(phenotype.size);
    traits.push_back(phenotype.speed);
    traits.push_back(phenotype.visionRange);
    traits.push_back(phenotype.efficiency);
    traits.push_back(phenotype.aggression);
    traits.push_back(phenotype.terrestrialAptitude);
    traits.push_back(phenotype.aquaticAptitude);
    traits.push_back(phenotype.aerialAptitude);
    traits.push_back(phenotype.dietSpecialization);
    traits.push_back(phenotype.habitatPreference);

    return traits;
}

float AdaptiveRadiationTracker::calculateTraitVariance(
    const std::vector<std::vector<float>>& traitVectors) const {

    if (traitVectors.empty()) return 0.0f;

    size_t numTraits = traitVectors[0].size();
    size_t numSpecies = traitVectors.size();

    if (numTraits == 0 || numSpecies < 2) return 0.0f;

    // Calculate centroid
    std::vector<float> centroid(numTraits, 0.0f);
    for (const auto& traits : traitVectors) {
        for (size_t i = 0; i < numTraits && i < traits.size(); i++) {
            centroid[i] += traits[i];
        }
    }
    for (float& val : centroid) {
        val /= numSpecies;
    }

    // Calculate variance (sum of squared distances from centroid)
    float totalVariance = 0.0f;
    for (const auto& traits : traitVectors) {
        float distSq = 0.0f;
        for (size_t i = 0; i < numTraits && i < traits.size(); i++) {
            float diff = traits[i] - centroid[i];
            distSq += diff * diff;
        }
        totalVariance += distSq;
    }

    return totalVariance / numSpecies;
}

// =============================================================================
// RADIATION QUERIES
// =============================================================================

std::vector<const RadiationEvent*> AdaptiveRadiationTracker::getActiveRadiations() const {
    std::vector<const RadiationEvent*> active;
    for (const auto& radiation : radiationEvents_) {
        if (radiation->isOngoing) {
            active.push_back(radiation.get());
        }
    }
    return active;
}

std::vector<RadiationEvent*> AdaptiveRadiationTracker::getActiveRadiationsMutable() {
    std::vector<RadiationEvent*> active;
    for (auto& radiation : radiationEvents_) {
        if (radiation->isOngoing) {
            active.push_back(radiation.get());
        }
    }
    return active;
}

std::vector<const RadiationEvent*> AdaptiveRadiationTracker::getHistoricalRadiations() const {
    std::vector<const RadiationEvent*> historical;
    for (const auto& radiation : radiationEvents_) {
        if (!radiation->isOngoing) {
            historical.push_back(radiation.get());
        }
    }
    return historical;
}

const RadiationEvent* AdaptiveRadiationTracker::getRadiation(uint64_t radiationId) const {
    auto it = radiationById_.find(radiationId);
    return (it != radiationById_.end()) ? it->second : nullptr;
}

RadiationEvent* AdaptiveRadiationTracker::getRadiationMutable(uint64_t radiationId) {
    auto it = radiationById_.find(radiationId);
    return (it != radiationById_.end()) ? it->second : nullptr;
}

std::vector<const RadiationEvent*> AdaptiveRadiationTracker::getAllRadiations() const {
    std::vector<const RadiationEvent*> all;
    for (const auto& radiation : radiationEvents_) {
        all.push_back(radiation.get());
    }
    return all;
}

std::vector<const RadiationEvent*> AdaptiveRadiationTracker::getRadiationsForSpecies(
    SpeciesId speciesId) const {

    std::vector<const RadiationEvent*> result;
    for (const auto& radiation : radiationEvents_) {
        if (radiation->ancestorSpeciesId == speciesId ||
            std::find(radiation->descendantSpeciesIds.begin(),
                     radiation->descendantSpeciesIds.end(), speciesId) != radiation->descendantSpeciesIds.end()) {
            result.push_back(radiation.get());
        }
    }
    return result;
}

// =============================================================================
// LINEAGE TRACKING
// =============================================================================

const LineageDiversification* AdaptiveRadiationTracker::getLineageDiversification(
    uint64_t lineageId) const {

    auto it = lineages_.find(lineageId);
    return (it != lineages_.end()) ? &it->second : nullptr;
}

void AdaptiveRadiationTracker::updateLineage(uint64_t lineageId, int generation) {
    auto it = lineages_.find(lineageId);
    if (it == lineages_.end()) return;

    LineageDiversification& lineage = it->second;

    // Calculate rates from history
    int historySize = static_cast<int>(lineage.speciesCountHistory.size());
    if (historySize > 0) {
        int currentCount = lineage.currentSpeciesCount;
        int previousCount = lineage.speciesCountHistory.back();

        if (currentCount > previousCount) {
            lineage.branchingEvents++;
        } else if (currentCount < previousCount) {
            lineage.extinctionEvents++;
        }

        // Update peak
        if (currentCount > lineage.peakSpeciesCount) {
            lineage.peakSpeciesCount = currentCount;
            lineage.peakGeneration = generation;
        }

        // Calculate rates over window
        int windowSize = std::min(rateCalculationWindow_, historySize);
        int oldCount = (windowSize < historySize) ?
                       lineage.speciesCountHistory[historySize - windowSize - 1] :
                       lineage.speciesCountHistory[0];

        if (windowSize > 0) {
            lineage.birthRate = static_cast<float>(lineage.branchingEvents) / windowSize;
            lineage.deathRate = static_cast<float>(lineage.extinctionEvents) / windowSize;
            lineage.netDiversification = lineage.birthRate - lineage.deathRate;

            if (lineage.birthRate > 0) {
                lineage.turnoverRate = lineage.deathRate / lineage.birthRate;
            }
        }
    }

    // Store history
    lineage.speciesCountHistory.push_back(lineage.currentSpeciesCount);
    lineage.diversificationHistory.push_back(lineage.netDiversification);
    lineage.disparityHistory.push_back(lineage.morphologicalDisparity);
}

uint64_t AdaptiveRadiationTracker::registerLineage(SpeciesId rootSpeciesId, int generation) {
    LineageDiversification lineage;
    lineage.lineageId = nextLineageId_++;
    lineage.rootSpeciesId = rootSpeciesId;
    lineage.originGeneration = generation;
    lineage.currentSpeciesCount = 1;
    lineage.peakSpeciesCount = 1;
    lineage.peakGeneration = generation;

    lineages_[lineage.lineageId] = lineage;
    return lineage.lineageId;
}

// =============================================================================
// NICHE ANALYSIS
// =============================================================================

NicheType AdaptiveRadiationTracker::classifyNiche(const Species* species) const {
    if (!species) return NicheType::OMNIVORE;

    DiploidGenome genome = species->getRepresentativeGenome();
    Phenotype phenotype = genome.express();

    // Determine primary niche based on traits
    // Aerial
    if (phenotype.aerialAptitude > 0.6f) {
        if (phenotype.aggression > 0.6f) {
            return NicheType::AERIAL;
        }
        return NicheType::AERIAL;
    }

    // Aquatic
    if (phenotype.aquaticAptitude > 0.6f) {
        if (phenotype.preferredDepth > 0.7f) {
            return NicheType::AQUATIC_BENTHIC;
        } else if (phenotype.preferredDepth < 0.3f) {
            return NicheType::AQUATIC_SURFACE;
        }
        return NicheType::AQUATIC_PELAGIC;
    }

    // Terrestrial types based on behavior
    if (phenotype.aggression > 0.7f) {
        if (phenotype.size > 1.0f) {
            return NicheType::PREDATOR_LARGE;
        }
        return NicheType::PREDATOR_SMALL;
    }

    // Diet-based classification
    if (phenotype.dietSpecialization < 0.3f) {
        return NicheType::HERBIVORE_GRAZER;
    } else if (phenotype.dietSpecialization < 0.5f) {
        return NicheType::HERBIVORE_BROWSER;
    } else if (phenotype.dietSpecialization < 0.7f) {
        return NicheType::OMNIVORE;
    }

    // Activity pattern
    if (phenotype.activityTime < 0.3f) {
        return NicheType::NOCTURNAL;
    } else if (phenotype.activityTime > 0.7f) {
        return NicheType::DIURNAL;
    }

    return NicheType::OMNIVORE;
}

std::vector<NicheType> AdaptiveRadiationTracker::getNichesInRadiation(uint64_t radiationId) const {
    std::vector<NicheType> niches;

    const RadiationEvent* radiation = getRadiation(radiationId);
    if (!radiation || !speciationTracker_) return niches;

    std::set<NicheType> uniqueNiches;
    for (SpeciesId spId : radiation->extantDescendants) {
        const Species* sp = speciationTracker_->getSpecies(spId);
        if (sp && !sp->isExtinct()) {
            NicheType niche = classifyNiche(sp);
            uniqueNiches.insert(niche);
        }
    }

    niches.assign(uniqueNiches.begin(), uniqueNiches.end());
    return niches;
}

float AdaptiveRadiationTracker::calculateNichePacking(uint64_t radiationId) const {
    const RadiationEvent* radiation = getRadiation(radiationId);
    if (!radiation) return 0.0f;

    std::vector<NicheType> niches = getNichesInRadiation(radiationId);
    int numOccupied = static_cast<int>(niches.size());
    int totalPossible = static_cast<int>(NicheType::COUNT);

    if (totalPossible == 0) return 0.0f;

    return static_cast<float>(numOccupied) / totalPossible;
}

bool AdaptiveRadiationTracker::isNicheSaturated(uint64_t radiationId,
                                                 float saturationThreshold) const {
    float packing = calculateNichePacking(radiationId);
    return packing >= saturationThreshold;
}

// =============================================================================
// CLADE EXTINCTION RISK
// =============================================================================

float AdaptiveRadiationTracker::calculateCladeExtinctionRisk(uint64_t radiationId) const {
    const RadiationEvent* radiation = getRadiation(radiationId);
    if (!radiation || !speciationTracker_) return 0.0f;

    float totalRisk = 0.0f;
    int speciesCount = 0;

    float minPopulation = std::numeric_limits<float>::max();
    float minDiversity = std::numeric_limits<float>::max();

    for (SpeciesId spId : radiation->extantDescendants) {
        const Species* sp = speciationTracker_->getSpecies(spId);
        if (sp && !sp->isExtinct()) {
            // Population size component
            float popSize = static_cast<float>(sp->getStats().size);
            minPopulation = std::min(minPopulation, popSize);

            // Genetic diversity component
            float diversity = sp->getStats().averageHeterozygosity;
            minDiversity = std::min(minDiversity, diversity);

            speciesCount++;
        }
    }

    if (speciesCount == 0) return 1.0f; // All extinct

    // Risk factors:
    // 1. Low population sizes (higher weight)
    float populationRisk = 0.0f;
    if (minPopulation < 10.0f) {
        populationRisk = 0.8f;
    } else if (minPopulation < 50.0f) {
        populationRisk = 0.5f;
    } else if (minPopulation < 100.0f) {
        populationRisk = 0.2f;
    }

    // 2. Low genetic diversity
    float diversityRisk = 1.0f - std::min(1.0f, minDiversity);

    // 3. Few species in clade (less redundancy)
    float redundancyRisk = 0.0f;
    if (speciesCount == 1) {
        redundancyRisk = 0.8f;
    } else if (speciesCount <= 3) {
        redundancyRisk = 0.4f;
    } else if (speciesCount <= 10) {
        redundancyRisk = 0.1f;
    }

    // 4. Environmental instability (if tracking)
    // This could be passed in as a parameter

    // Weighted combination
    totalRisk = populationRisk * 0.4f +
                diversityRisk * 0.3f +
                redundancyRisk * 0.3f;

    return std::clamp(totalRisk, 0.0f, 1.0f);
}

std::vector<std::pair<SpeciesId, float>> AdaptiveRadiationTracker::getMostEndangeredDescendants(
    uint64_t radiationId, int topN) const {

    std::vector<std::pair<SpeciesId, float>> endangered;

    const RadiationEvent* radiation = getRadiation(radiationId);
    if (!radiation || !speciationTracker_) return endangered;

    for (SpeciesId spId : radiation->extantDescendants) {
        const Species* sp = speciationTracker_->getSpecies(spId);
        if (sp && !sp->isExtinct()) {
            ExtinctionRisk risk = sp->assessExtinctionRisk();
            endangered.push_back({spId, risk.riskScore});
        }
    }

    // Sort by risk (highest first)
    std::sort(endangered.begin(), endangered.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    // Return top N
    if (endangered.size() > static_cast<size_t>(topN)) {
        endangered.resize(topN);
    }

    return endangered;
}

// =============================================================================
// STATISTICS
// =============================================================================

RadiationStatistics AdaptiveRadiationTracker::getRadiationStatistics() const {
    if (!statsCacheValid_) {
        recalculateStats();
    }
    return cachedStats_;
}

void AdaptiveRadiationTracker::recalculateStats() const {
    cachedStats_ = RadiationStatistics();

    cachedStats_.totalRadiationEvents = static_cast<int>(radiationEvents_.size());

    float totalDuration = 0.0f;
    float totalDiversification = 0.0f;
    float totalPeakRate = 0.0f;
    float totalDescendants = 0.0f;
    float totalNiches = 0.0f;
    float totalDisparity = 0.0f;
    float totalTimeToFirst = 0.0f;
    float totalTimeToSaturation = 0.0f;
    int saturationCount = 0;

    for (const auto& radiation : radiationEvents_) {
        if (radiation->isOngoing) {
            cachedStats_.activeRadiations++;
        } else {
            cachedStats_.completedRadiations++;

            if (radiation->isSuccessful(minSuccessfulRadiationSize_)) {
                cachedStats_.successfulRadiations++;
            } else {
                cachedStats_.failedRadiations++;
            }
        }

        // Accumulate metrics
        totalDuration += radiation->duration;
        totalDiversification += radiation->diversificationRate;
        totalPeakRate += radiation->peakDiversificationRate;
        totalDescendants += radiation->getTotalDescendantCount();
        totalNiches += radiation->getNicheCount();
        totalDisparity += radiation->morphologicalDisparity;
        totalTimeToFirst += radiation->timeToFirstSpeciation;

        if (radiation->nicheSaturationReached) {
            totalTimeToSaturation += radiation->saturationGeneration - radiation->startGeneration;
            saturationCount++;
        }

        // Track max values
        cachedStats_.maxDiversificationRate = std::max(cachedStats_.maxDiversificationRate,
                                                       radiation->diversificationRate);
        cachedStats_.maxDescendantCount = std::max(cachedStats_.maxDescendantCount,
                                                   radiation->getTotalDescendantCount());
        cachedStats_.maxNicheCount = std::max(cachedStats_.maxNicheCount,
                                              radiation->getNicheCount());
        cachedStats_.maxMorphologicalDisparity = std::max(cachedStats_.maxMorphologicalDisparity,
                                                          radiation->morphologicalDisparity);

        // Count triggers
        cachedStats_.triggerCounts[radiation->triggerType]++;
        cachedStats_.contextCounts[radiation->environmentContext]++;
    }

    int total = cachedStats_.totalRadiationEvents;
    if (total > 0) {
        cachedStats_.averageRadiationDuration = totalDuration / total;
        cachedStats_.averageDiversificationRate = totalDiversification / total;
        cachedStats_.averagePeakRate = totalPeakRate / total;
        cachedStats_.averageDescendantCount = totalDescendants / total;
        cachedStats_.averageNicheCount = totalNiches / total;
        cachedStats_.averageMorphologicalDisparity = totalDisparity / total;
        cachedStats_.averageTimeToFirstSpeciation = totalTimeToFirst / total;

        if (saturationCount > 0) {
            cachedStats_.averageTimeToSaturation = totalTimeToSaturation / saturationCount;
        }
    }

    statsCacheValid_ = true;
}

int AdaptiveRadiationTracker::getTimeToFirstSpeciation(uint64_t radiationId) const {
    const RadiationEvent* radiation = getRadiation(radiationId);
    return radiation ? radiation->timeToFirstSpeciation : 0;
}

float AdaptiveRadiationTracker::getPeakDiversificationRate(uint64_t radiationId) const {
    const RadiationEvent* radiation = getRadiation(radiationId);
    return radiation ? radiation->peakDiversificationRate : 0.0f;
}

int AdaptiveRadiationTracker::getPeakDiversificationGeneration(uint64_t radiationId) const {
    const RadiationEvent* radiation = getRadiation(radiationId);
    return radiation ? radiation->peakDiversificationGeneration : 0;
}

int AdaptiveRadiationTracker::getSaturationPoint(uint64_t radiationId) const {
    const RadiationEvent* radiation = getRadiation(radiationId);
    if (!radiation || !radiation->nicheSaturationReached) return -1;
    return radiation->saturationGeneration;
}

// =============================================================================
// DATA EXPORT
// =============================================================================

bool AdaptiveRadiationTracker::exportRadiationData(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for radiation export: " << filename << std::endl;
        return false;
    }

    // Header
    file << "radiation_id,ancestor_species_id,ancestor_name,start_generation,"
         << "end_generation,duration,trigger_type,environment_context,"
         << "total_descendants,extant_descendants,extinct_descendants,"
         << "diversification_rate,peak_diversification_rate,"
         << "speciation_rate,extinction_rate,niche_count,niche_packing,"
         << "morphological_disparity,is_ongoing,is_successful\n";

    for (const auto& radiation : radiationEvents_) {
        file << radiation->radiationId << ","
             << radiation->ancestorSpeciesId << ","
             << "\"" << radiation->ancestorSpeciesName << "\","
             << radiation->startGeneration << ","
             << radiation->endGeneration << ","
             << radiation->duration << ","
             << radiationTriggerToString(radiation->triggerType) << ","
             << environmentContextToString(radiation->environmentContext) << ","
             << radiation->getTotalDescendantCount() << ","
             << radiation->getExtantDescendantCount() << ","
             << radiation->extinctDescendants.size() << ","
             << radiation->diversificationRate << ","
             << radiation->peakDiversificationRate << ","
             << radiation->speciationRate << ","
             << radiation->extinctionRate << ","
             << radiation->getNicheCount() << ","
             << radiation->nichePackingDensity << ","
             << radiation->morphologicalDisparity << ","
             << (radiation->isOngoing ? "true" : "false") << ","
             << (radiation->isSuccessful(minSuccessfulRadiationSize_) ? "true" : "false")
             << "\n";
    }

    file.close();
    return true;
}

bool AdaptiveRadiationTracker::exportLineageData(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for lineage export: " << filename << std::endl;
        return false;
    }

    // Header
    file << "lineage_id,root_species_id,origin_generation,"
         << "branching_events,extinction_events,current_species,"
         << "peak_species,peak_generation,birth_rate,death_rate,"
         << "net_diversification,turnover_rate,morphological_disparity\n";

    for (const auto& [id, lineage] : lineages_) {
        file << lineage.lineageId << ","
             << lineage.rootSpeciesId << ","
             << lineage.originGeneration << ","
             << lineage.branchingEvents << ","
             << lineage.extinctionEvents << ","
             << lineage.currentSpeciesCount << ","
             << lineage.peakSpeciesCount << ","
             << lineage.peakGeneration << ","
             << lineage.birthRate << ","
             << lineage.deathRate << ","
             << lineage.netDiversification << ","
             << lineage.turnoverRate << ","
             << lineage.morphologicalDisparity
             << "\n";
    }

    file.close();
    return true;
}

bool AdaptiveRadiationTracker::exportInnovationData(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for innovation export: " << filename << std::endl;
        return false;
    }

    // Header
    file << "innovation_id,detection_generation,origin_species_id,"
         << "primary_gene,trait_change,ancestral_value,derived_value,"
         << "description,triggered_radiation,associated_radiation_id\n";

    for (const auto& innovation : keyInnovations_) {
        file << innovation.innovationId << ","
             << innovation.detectionGeneration << ","
             << innovation.originSpeciesId << ","
             << static_cast<int>(innovation.primaryGene) << ","
             << innovation.traitChange << ","
             << innovation.ancestralValue << ","
             << innovation.derivedValue << ","
             << "\"" << innovation.description << "\","
             << (innovation.triggeredRadiation ? "true" : "false") << ","
             << innovation.associatedRadiationId
             << "\n";
    }

    file.close();
    return true;
}

bool AdaptiveRadiationTracker::exportDisparityThroughTime(uint64_t radiationId,
                                                          const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for disparity export: " << filename << std::endl;
        return false;
    }

    file << "generation,disparity\n";

    auto dtt = getDisparityThroughTime(radiationId);
    for (const auto& [gen, disparity] : dtt) {
        file << gen << "," << disparity << "\n";
    }

    file.close();
    return true;
}

std::string AdaptiveRadiationTracker::generateSummaryReport() const {
    std::stringstream ss;

    RadiationStatistics stats = getRadiationStatistics();

    ss << "=== ADAPTIVE RADIATION SUMMARY REPORT ===\n\n";

    ss << "OVERVIEW:\n";
    ss << "  Total radiation events: " << stats.totalRadiationEvents << "\n";
    ss << "  Active radiations: " << stats.activeRadiations << "\n";
    ss << "  Completed radiations: " << stats.completedRadiations << "\n";
    ss << "  Successful radiations: " << stats.successfulRadiations << "\n";
    ss << "  Failed radiations: " << stats.failedRadiations << "\n";
    ss << "  Success rate: " << (stats.getSuccessRate() * 100.0f) << "%\n\n";

    ss << "DIVERSIFICATION METRICS:\n";
    ss << "  Average diversification rate: " << stats.averageDiversificationRate << "\n";
    ss << "  Maximum diversification rate: " << stats.maxDiversificationRate << "\n";
    ss << "  Average peak rate: " << stats.averagePeakRate << "\n\n";

    ss << "DESCENDANT METRICS:\n";
    ss << "  Average descendants per radiation: " << stats.averageDescendantCount << "\n";
    ss << "  Maximum descendants: " << stats.maxDescendantCount << "\n";
    ss << "  Average niches exploited: " << stats.averageNicheCount << "\n";
    ss << "  Maximum niches: " << stats.maxNicheCount << "\n\n";

    ss << "TIMING METRICS:\n";
    ss << "  Average radiation duration: " << stats.averageRadiationDuration << " generations\n";
    ss << "  Average time to first speciation: " << stats.averageTimeToFirstSpeciation << " generations\n";
    ss << "  Average time to saturation: " << stats.averageTimeToSaturation << " generations\n\n";

    ss << "MORPHOLOGICAL DISPARITY:\n";
    ss << "  Average disparity: " << stats.averageMorphologicalDisparity << "\n";
    ss << "  Maximum disparity: " << stats.maxMorphologicalDisparity << "\n\n";

    ss << "TRIGGER BREAKDOWN:\n";
    for (const auto& [trigger, count] : stats.triggerCounts) {
        ss << "  " << radiationTriggerToString(trigger) << ": " << count << "\n";
    }
    ss << "\n";

    ss << "ENVIRONMENT CONTEXT BREAKDOWN:\n";
    for (const auto& [context, count] : stats.contextCounts) {
        ss << "  " << environmentContextToString(context) << ": " << count << "\n";
    }
    ss << "\n";

    ss << "KEY INNOVATIONS:\n";
    ss << "  Total detected: " << keyInnovations_.size() << "\n";
    int triggeringInnovations = 0;
    for (const auto& innovation : keyInnovations_) {
        if (innovation.triggeredRadiation) triggeringInnovations++;
    }
    ss << "  Triggering radiations: " << triggeringInnovations << "\n\n";

    ss << "COLONIZATION EVENTS:\n";
    ss << "  Total colonizations: " << colonizationEvents_.size() << "\n";
    int successfulColonizations = 0;
    for (const auto& event : colonizationEvents_) {
        if (event.triggeredRadiation) successfulColonizations++;
    }
    ss << "  Triggering radiations: " << successfulColonizations << "\n";

    return ss.str();
}

// =============================================================================
// PRIVATE HELPER METHODS
// =============================================================================

EnvironmentContext AdaptiveRadiationTracker::inferEnvironmentContext(
    const std::map<std::string, float>& environmentData) const {

    auto islandIt = environmentData.find("island");
    if (islandIt != environmentData.end() && islandIt->second > 0.5f) {
        return EnvironmentContext::ISLAND_ARCHIPELAGO;
    }

    auto lakeIt = environmentData.find("isolated_lake");
    if (lakeIt != environmentData.end() && lakeIt->second > 0.5f) {
        return EnvironmentContext::ISOLATED_LAKE;
    }

    auto mountainIt = environmentData.find("mountain");
    if (mountainIt != environmentData.end() && mountainIt->second > 0.5f) {
        return EnvironmentContext::MOUNTAIN_RANGE;
    }

    auto extinctionIt = environmentData.find("post_extinction");
    if (extinctionIt != environmentData.end() && extinctionIt->second > 0.5f) {
        return EnvironmentContext::POST_EXTINCTION;
    }

    auto fragmentationIt = environmentData.find("fragmentation");
    if (fragmentationIt != environmentData.end() && fragmentationIt->second > 0.5f) {
        return EnvironmentContext::HABITAT_FRAGMENTATION;
    }

    auto caveIt = environmentData.find("cave");
    if (caveIt != environmentData.end() && caveIt->second > 0.5f) {
        return EnvironmentContext::CAVE_SYSTEM;
    }

    return EnvironmentContext::CONTINENTAL;
}

} // namespace genetics
