/**
 * @file EvolutionaryHistory.cpp
 * @brief Implementation of the comprehensive evolutionary history tracking system.
 *
 * This file implements the EvolutionaryHistoryTracker class and related functionality
 * for tracking complete evolutionary histories, including lineage records, trait changes,
 * speciation events, and phylogenetic relationships.
 */

#include "EvolutionaryHistory.h"
#include "../Creature.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>
#include <numeric>
#include <queue>
#include <unordered_set>

namespace genetics {

// =============================================================================
// STATIC MEMBER INITIALIZATION
// =============================================================================

LineageId EvolutionaryHistoryTracker::s_nextLineageId = 1;

// =============================================================================
// CONSTRUCTION & DESTRUCTION
// =============================================================================

EvolutionaryHistoryTracker::EvolutionaryHistoryTracker()
    : m_currentGeneration(0)
    , m_traitChangeThreshold(0.01f)
    , m_detailedGenomicTracking(true)
    , m_maxLineageRecords(0)
    , m_maxSpeciesRecords(0)
    , m_totalBirths(0)
    , m_totalDeaths(0)
    , m_totalSpeciations(0)
    , m_totalExtinctions(0)
{
}

EvolutionaryHistoryTracker::~EvolutionaryHistoryTracker() = default;

// =============================================================================
// EVENT RECORDING - LIFE CYCLE
// =============================================================================

void EvolutionaryHistoryTracker::recordBirth(const Creature* creature,
                                              const Creature* parent1,
                                              const Creature* parent2)
{
    if (!creature) return;

    m_totalBirths++;

    const DiploidGenome& genome = creature->getDiploidGenome();
    LineageId lineageId = genome.getLineageId();
    LineageId parentLineageId = 0;

    // Determine parent lineage (inherit from primary parent)
    if (parent1) {
        parentLineageId = parent1->getDiploidGenome().getLineageId();
    }

    // Check if this lineage already exists
    auto it = m_lineageRecords.find(lineageId);
    if (it != m_lineageRecords.end()) {
        // Update existing lineage record
        LineageRecord& record = it->second;
        record.totalDescendants++;
        record.survivingDescendants++;

        // Update peak population if needed
        if (record.survivingDescendants > record.peakPopulation) {
            record.peakPopulation = record.survivingDescendants;
            record.peakPopulationGen = m_currentGeneration;
        }

        // Update species history if species changed
        SpeciesId currentSpecies = genome.getSpeciesId();
        if (!record.speciesHistory.empty() &&
            record.speciesHistory.back().second != currentSpecies) {
            record.speciesHistory.emplace_back(m_currentGeneration, currentSpecies);
        }
    } else {
        // Create new lineage record
        lineageId = createLineageRecord(creature, parentLineageId);
    }

    // Update lineage tree structure
    auto treeIt = m_lineageTree.find(lineageId);
    if (treeIt == m_lineageTree.end()) {
        LineageTreeNode node;
        node.lineageId = lineageId;
        node.parentId = parentLineageId;
        node.birthGeneration = m_currentGeneration;
        node.deathGeneration = -1;
        node.depth = 0;
        node.branchLength = 0.0f;

        // Calculate depth from parent
        if (parentLineageId != 0) {
            auto parentTreeIt = m_lineageTree.find(parentLineageId);
            if (parentTreeIt != m_lineageTree.end()) {
                node.depth = parentTreeIt->second.depth + 1;
                node.branchLength = static_cast<float>(m_currentGeneration -
                                    parentTreeIt->second.birthGeneration);

                // Add this as a child of parent
                parentTreeIt->second.childrenIds.push_back(lineageId);
            }
        }

        m_lineageTree[lineageId] = node;
    }

    // Update parent lineage's child list
    if (parentLineageId != 0) {
        auto parentIt = m_lineageRecords.find(parentLineageId);
        if (parentIt != m_lineageRecords.end()) {
            auto& childLineages = parentIt->second.childLineages;
            if (std::find(childLineages.begin(), childLineages.end(), lineageId) ==
                childLineages.end()) {
                childLineages.push_back(lineageId);
            }
        }
    }

    // Invalidate ancestor cache for related lineages
    invalidateAncestorCache(lineageId);

    // Auto-prune if limits are set
    autoPruneIfNeeded();
}

void EvolutionaryHistoryTracker::recordDeath(const Creature* creature)
{
    if (!creature) return;

    m_totalDeaths++;

    const DiploidGenome& genome = creature->getDiploidGenome();
    LineageId lineageId = genome.getLineageId();

    // Update lineage record
    updateLineageOnDeath(lineageId);

    // Check for lineage extinction
    auto it = m_lineageRecords.find(lineageId);
    if (it != m_lineageRecords.end()) {
        LineageRecord& record = it->second;

        // If no surviving descendants, mark lineage as extinct
        if (record.survivingDescendants <= 0) {
            record.extinctionGeneration = m_currentGeneration;

            // Update tree node
            auto treeIt = m_lineageTree.find(lineageId);
            if (treeIt != m_lineageTree.end()) {
                treeIt->second.deathGeneration = m_currentGeneration;
            }
        }
    }
}

// =============================================================================
// EVENT RECORDING - SPECIATION & EXTINCTION
// =============================================================================

void EvolutionaryHistoryTracker::recordSpeciation(SpeciesId parentSpecies,
                                                   SpeciesId childSpecies,
                                                   SpeciationCause cause,
                                                   const std::vector<DiploidGenome>& founderGenomes)
{
    m_totalSpeciations++;
    m_speciationsByGeneration[m_currentGeneration]++;

    // Create phylogenetic record for the new species
    PhylogeneticRecord record(childSpecies, parentSpecies, m_currentGeneration);

    // Store founder genomes (up to a reasonable limit)
    const size_t maxFounderGenomes = 10;
    for (size_t i = 0; i < std::min(founderGenomes.size(), maxFounderGenomes); i++) {
        record.founderGenomes.push_back(founderGenomes[i]);
    }

    record.founderPopulationSize = static_cast<int>(founderGenomes.size());

    // Calculate initial genetic diversity
    if (!founderGenomes.empty()) {
        float totalHeterozygosity = 0.0f;
        for (const auto& genome : founderGenomes) {
            totalHeterozygosity += genome.getHeterozygosity();
        }
        record.founderGeneticDiversity = totalHeterozygosity / founderGenomes.size();
    }

    m_phylogeneticRecords[childSpecies] = record;

    // Update parent species record
    auto parentIt = m_phylogeneticRecords.find(parentSpecies);
    if (parentIt != m_phylogeneticRecords.end()) {
        parentIt->second.descendantCount++;
        parentIt->second.daughterSpecies.push_back(childSpecies);
    }

    // Auto-prune if limits are set
    autoPruneIfNeeded();
}

void EvolutionaryHistoryTracker::recordExtinction(SpeciesId species,
                                                   ExtinctionCause cause,
                                                   const std::string& notes)
{
    m_totalExtinctions++;
    m_extinctionsByGeneration[m_currentGeneration]++;

    auto it = m_phylogeneticRecords.find(species);
    if (it != m_phylogeneticRecords.end()) {
        PhylogeneticRecord& record = it->second;
        record.extinctionGeneration = m_currentGeneration;
        record.extinctionCause = cause;
        record.extinctionNotes = notes;

        // Record final population stats if available
        if (!record.populationHistory.empty()) {
            record.finalPopulation = record.populationHistory.back().second;
        }
    }
}

// =============================================================================
// EVENT RECORDING - TRAIT CHANGES
// =============================================================================

void EvolutionaryHistoryTracker::recordTraitChange(SpeciesId species,
                                                    GeneType trait,
                                                    float oldValue,
                                                    float newValue,
                                                    ChangeType changeType,
                                                    SelectionPressure pressure)
{
    // Check if change exceeds threshold
    float magnitude = std::abs(newValue - oldValue);
    if (magnitude < m_traitChangeThreshold) {
        return;
    }

    TraitChange change(m_currentGeneration, species, trait, oldValue, newValue,
                       changeType, pressure);

    // Calculate effect size
    change.effectSize = magnitude;

    // Store by species
    m_traitChangesBySpecies[species].push_back(change);

    // Also update lineage records for this species
    for (auto& [lineageId, record] : m_lineageRecords) {
        if (record.getCurrentSpecies() == species && record.isExtant()) {
            record.majorTraitChanges.push_back(change);
        }
    }
}

void EvolutionaryHistoryTracker::recordGenomicChange(LineageId lineage,
                                                      const GenomicChange& change)
{
    if (!m_detailedGenomicTracking) return;

    auto it = m_lineageRecords.find(lineage);
    if (it != m_lineageRecords.end()) {
        it->second.genomicChanges.push_back(change);
    }
}

// =============================================================================
// ANCESTRY QUERIES
// =============================================================================

LineageId EvolutionaryHistoryTracker::getMostRecentCommonAncestor(LineageId lineage1,
                                                                   LineageId lineage2) const
{
    if (lineage1 == 0 || lineage2 == 0) return 0;
    if (lineage1 == lineage2) return lineage1;

    // Check cache first
    uint64_t cacheKey = makeAncestorCacheKey(lineage1, lineage2);
    auto cacheIt = m_ancestorCache.find(cacheKey);
    if (cacheIt != m_ancestorCache.end()) {
        return cacheIt->second;
    }

    // Build ancestor set for lineage1
    std::unordered_set<LineageId> ancestors1 = buildAncestorSet(lineage1);
    ancestors1.insert(lineage1);

    // Walk up lineage2's ancestry and find first intersection
    LineageId current = lineage2;
    while (current != 0) {
        if (ancestors1.count(current) > 0) {
            // Cache the result
            m_ancestorCache[cacheKey] = current;
            return current;
        }

        auto it = m_lineageRecords.find(current);
        if (it == m_lineageRecords.end()) break;
        current = it->second.ancestorLineageId;
    }

    // No common ancestor found
    m_ancestorCache[cacheKey] = 0;
    return 0;
}

int EvolutionaryHistoryTracker::getEvolutionaryDistance(LineageId lineage1,
                                                         LineageId lineage2) const
{
    if (lineage1 == 0 || lineage2 == 0) return -1;
    if (lineage1 == lineage2) return 0;

    LineageId mrca = getMostRecentCommonAncestor(lineage1, lineage2);
    if (mrca == 0) return -1;

    // Count generations from lineage1 to MRCA
    int dist1 = 0;
    LineageId current = lineage1;
    while (current != mrca && current != 0) {
        auto it = m_lineageRecords.find(current);
        if (it == m_lineageRecords.end()) break;

        // Use founding generation difference as distance
        auto parentIt = m_lineageRecords.find(it->second.ancestorLineageId);
        if (parentIt != m_lineageRecords.end()) {
            dist1 += std::abs(it->second.foundingGeneration -
                             parentIt->second.foundingGeneration);
        } else {
            dist1++;
        }
        current = it->second.ancestorLineageId;
    }

    // Count generations from lineage2 to MRCA
    int dist2 = 0;
    current = lineage2;
    while (current != mrca && current != 0) {
        auto it = m_lineageRecords.find(current);
        if (it == m_lineageRecords.end()) break;

        auto parentIt = m_lineageRecords.find(it->second.ancestorLineageId);
        if (parentIt != m_lineageRecords.end()) {
            dist2 += std::abs(it->second.foundingGeneration -
                             parentIt->second.foundingGeneration);
        } else {
            dist2++;
        }
        current = it->second.ancestorLineageId;
    }

    return dist1 + dist2;
}

std::vector<LineageId> EvolutionaryHistoryTracker::getAncestry(LineageId lineage) const
{
    std::vector<LineageId> ancestry;

    LineageId current = lineage;
    while (current != 0) {
        auto it = m_lineageRecords.find(current);
        if (it == m_lineageRecords.end()) break;

        LineageId parent = it->second.ancestorLineageId;
        if (parent != 0) {
            ancestry.push_back(parent);
        }
        current = parent;
    }

    return ancestry;
}

std::vector<LineageId> EvolutionaryHistoryTracker::getDescendants(LineageId lineage,
                                                                   bool extantOnly) const
{
    std::vector<LineageId> descendants;

    // BFS traversal of descendants
    std::queue<LineageId> toProcess;
    toProcess.push(lineage);

    while (!toProcess.empty()) {
        LineageId current = toProcess.front();
        toProcess.pop();

        auto it = m_lineageRecords.find(current);
        if (it == m_lineageRecords.end()) continue;

        for (LineageId childId : it->second.childLineages) {
            auto childIt = m_lineageRecords.find(childId);
            if (childIt != m_lineageRecords.end()) {
                if (!extantOnly || childIt->second.isExtant()) {
                    descendants.push_back(childId);
                }
                toProcess.push(childId);
            }
        }
    }

    return descendants;
}

// =============================================================================
// TRAIT HISTORY QUERIES
// =============================================================================

std::vector<TraitChange> EvolutionaryHistoryTracker::getTraitHistory(SpeciesId species,
                                                                      GeneType trait) const
{
    std::vector<TraitChange> history;

    auto it = m_traitChangesBySpecies.find(species);
    if (it != m_traitChangesBySpecies.end()) {
        for (const TraitChange& change : it->second) {
            if (change.traitType == trait) {
                history.push_back(change);
            }
        }
    }

    // Sort by generation
    std::sort(history.begin(), history.end(),
              [](const TraitChange& a, const TraitChange& b) {
                  return a.generation < b.generation;
              });

    return history;
}

float EvolutionaryHistoryTracker::getHistoricalTraitValue(SpeciesId species,
                                                          GeneType trait,
                                                          Generation generation) const
{
    auto history = getTraitHistory(species, trait);
    if (history.empty()) return 0.0f;

    // Find the trait value at or before the target generation
    float value = history[0].oldValue;  // Start with initial value

    for (const TraitChange& change : history) {
        if (change.generation <= generation) {
            value = change.newValue;
        } else {
            break;
        }
    }

    return value;
}

// =============================================================================
// TREE ACCESS
// =============================================================================

std::vector<LineageId> EvolutionaryHistoryTracker::getRootLineages() const
{
    std::vector<LineageId> roots;

    for (const auto& [lineageId, record] : m_lineageRecords) {
        if (record.ancestorLineageId == 0) {
            roots.push_back(lineageId);
        }
    }

    return roots;
}

int EvolutionaryHistoryTracker::getMaxTreeDepth() const
{
    int maxDepth = 0;

    for (const auto& [lineageId, node] : m_lineageTree) {
        maxDepth = std::max(maxDepth, node.depth);
    }

    return maxDepth;
}

// =============================================================================
// RECORD ACCESS
// =============================================================================

const LineageRecord* EvolutionaryHistoryTracker::getLineageRecord(LineageId lineage) const
{
    auto it = m_lineageRecords.find(lineage);
    return (it != m_lineageRecords.end()) ? &it->second : nullptr;
}

const PhylogeneticRecord* EvolutionaryHistoryTracker::getPhylogeneticRecord(SpeciesId species) const
{
    auto it = m_phylogeneticRecords.find(species);
    return (it != m_phylogeneticRecords.end()) ? &it->second : nullptr;
}

// =============================================================================
// RATE CALCULATIONS
// =============================================================================

float EvolutionaryHistoryTracker::getExtinctionRate(int generationWindow) const
{
    if (m_currentGeneration == 0) return 0.0f;

    Generation startGen = std::max(0, m_currentGeneration - generationWindow);
    int extinctionCount = 0;

    for (const auto& [gen, count] : m_extinctionsByGeneration) {
        if (gen >= startGen && gen <= m_currentGeneration) {
            extinctionCount += count;
        }
    }

    int actualWindow = m_currentGeneration - startGen;
    return actualWindow > 0 ? static_cast<float>(extinctionCount) / actualWindow : 0.0f;
}

float EvolutionaryHistoryTracker::getSpeciationRate(int generationWindow) const
{
    if (m_currentGeneration == 0) return 0.0f;

    Generation startGen = std::max(0, m_currentGeneration - generationWindow);
    int speciationCount = 0;

    for (const auto& [gen, count] : m_speciationsByGeneration) {
        if (gen >= startGen && gen <= m_currentGeneration) {
            speciationCount += count;
        }
    }

    int actualWindow = m_currentGeneration - startGen;
    return actualWindow > 0 ? static_cast<float>(speciationCount) / actualWindow : 0.0f;
}

// =============================================================================
// STATISTICS - LINEAGE ANALYSIS
// =============================================================================

float EvolutionaryHistoryTracker::getAverageLineageLifespan(bool extinctOnly) const
{
    if (m_lineageRecords.empty()) return 0.0f;

    float totalLifespan = 0.0f;
    int count = 0;

    for (const auto& [lineageId, record] : m_lineageRecords) {
        if (extinctOnly && record.isExtant()) {
            continue;
        }

        int lifespan = record.getLifespan(m_currentGeneration);
        totalLifespan += static_cast<float>(lifespan);
        count++;
    }

    return count > 0 ? totalLifespan / count : 0.0f;
}

std::vector<LineageSuccessMetrics> EvolutionaryHistoryTracker::getMostSuccessfulLineages(int count) const
{
    std::vector<LineageSuccessMetrics> allMetrics;

    for (const auto& [lineageId, record] : m_lineageRecords) {
        LineageSuccessMetrics metrics;
        metrics.lineageId = lineageId;
        metrics.totalDescendants = record.totalDescendants;
        metrics.currentDescendants = record.survivingDescendants;
        metrics.averageFitness = record.averageFitness;
        metrics.generationsSurvived = record.getLifespan(m_currentGeneration);

        // Count species generated (count unique species in history)
        std::unordered_set<SpeciesId> uniqueSpecies;
        for (const auto& [gen, speciesId] : record.speciesHistory) {
            uniqueSpecies.insert(speciesId);
        }
        metrics.speciesGenerated = static_cast<int>(uniqueSpecies.size());

        // Calculate genetic contribution (proportion of total population)
        int totalPopulation = 0;
        for (const auto& [id, rec] : m_lineageRecords) {
            totalPopulation += rec.survivingDescendants;
        }
        metrics.geneticContribution = totalPopulation > 0 ?
            static_cast<float>(record.survivingDescendants) / totalPopulation : 0.0f;

        // Calculate composite success score
        metrics.compositeScore = calculateSuccessScore(record);

        allMetrics.push_back(metrics);
    }

    // Sort by composite score (descending)
    std::sort(allMetrics.begin(), allMetrics.end());

    // Return top N
    if (static_cast<int>(allMetrics.size()) > count) {
        allMetrics.resize(count);
    }

    return allMetrics;
}

std::vector<LineageId> EvolutionaryHistoryTracker::getLongestSurvivingLineages(int count) const
{
    std::vector<std::pair<LineageId, int>> lifespans;

    for (const auto& [lineageId, record] : m_lineageRecords) {
        lifespans.emplace_back(lineageId, record.getLifespan(m_currentGeneration));
    }

    // Sort by lifespan (descending)
    std::sort(lifespans.begin(), lifespans.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    std::vector<LineageId> result;
    for (int i = 0; i < count && i < static_cast<int>(lifespans.size()); i++) {
        result.push_back(lifespans[i].first);
    }

    return result;
}

std::vector<LineageId> EvolutionaryHistoryTracker::getMostProlificLineages(int count,
                                                                            bool extantOnly) const
{
    std::vector<std::pair<LineageId, int>> descendants;

    for (const auto& [lineageId, record] : m_lineageRecords) {
        int descCount = extantOnly ? record.survivingDescendants : record.totalDescendants;
        descendants.emplace_back(lineageId, descCount);
    }

    // Sort by descendant count (descending)
    std::sort(descendants.begin(), descendants.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    std::vector<LineageId> result;
    for (int i = 0; i < count && i < static_cast<int>(descendants.size()); i++) {
        result.push_back(descendants[i].first);
    }

    return result;
}

// =============================================================================
// STATISTICS - EVOLUTIONARY TRENDS
// =============================================================================

EvolutionaryTrendAnalysis EvolutionaryHistoryTracker::getEvolutionaryTrends(GeneType trait) const
{
    EvolutionaryTrendAnalysis analysis;
    analysis.trait = trait;

    // Collect all trait changes across species
    std::vector<std::pair<Generation, float>> allChanges;

    for (const auto& [speciesId, changes] : m_traitChangesBySpecies) {
        for (const TraitChange& change : changes) {
            if (change.traitType == trait) {
                allChanges.emplace_back(change.generation, change.newValue);
            }
        }
    }

    if (allChanges.empty()) {
        analysis.overallTrend = EvolutionaryTrend::STABLE;
        return analysis;
    }

    // Sort by generation
    std::sort(allChanges.begin(), allChanges.end());

    // Store trajectory
    analysis.trajectory = allChanges;

    // Calculate statistics
    analysis.ancestralValue = allChanges.front().second;
    analysis.currentValue = allChanges.back().second;
    analysis.totalChange = analysis.currentValue - analysis.ancestralValue;

    // Calculate mean change per generation
    int totalGenerations = allChanges.back().first - allChanges.front().first;
    analysis.meanChangePerGeneration = totalGenerations > 0 ?
        analysis.totalChange / totalGenerations : 0.0f;

    // Calculate volatility (variance of changes)
    if (allChanges.size() > 1) {
        std::vector<float> deltas;
        for (size_t i = 1; i < allChanges.size(); i++) {
            deltas.push_back(allChanges[i].second - allChanges[i - 1].second);
        }

        float meanDelta = std::accumulate(deltas.begin(), deltas.end(), 0.0f) / deltas.size();
        float variance = 0.0f;
        for (float d : deltas) {
            variance += (d - meanDelta) * (d - meanDelta);
        }
        analysis.volatility = std::sqrt(variance / deltas.size());
    }

    // Determine overall trend
    if (std::abs(analysis.totalChange) < m_traitChangeThreshold * 10) {
        analysis.overallTrend = EvolutionaryTrend::STABLE;
    } else if (analysis.volatility > std::abs(analysis.meanChangePerGeneration) * 2) {
        analysis.overallTrend = EvolutionaryTrend::OSCILLATING;
    } else if (analysis.totalChange > 0) {
        analysis.overallTrend = EvolutionaryTrend::INCREASING;
    } else {
        analysis.overallTrend = EvolutionaryTrend::DECREASING;
    }

    // Calculate trend strength (simple linear regression R^2)
    if (allChanges.size() > 2) {
        float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0, sumY2 = 0;
        int n = static_cast<int>(allChanges.size());

        for (const auto& [gen, val] : allChanges) {
            float x = static_cast<float>(gen);
            sumX += x;
            sumY += val;
            sumXY += x * val;
            sumX2 += x * x;
            sumY2 += val * val;
        }

        float denominator = (n * sumX2 - sumX * sumX) * (n * sumY2 - sumY * sumY);
        if (denominator > 0) {
            float r = (n * sumXY - sumX * sumY) / std::sqrt(denominator);
            analysis.trendStrength = r * r;  // R^2
        }
    }

    return analysis;
}

std::map<GeneType, EvolutionaryTrendAnalysis> EvolutionaryHistoryTracker::getAllTraitTrends() const
{
    std::map<GeneType, EvolutionaryTrendAnalysis> allTrends;

    // Get unique traits that have been tracked
    std::unordered_set<GeneType> trackedTraits;
    for (const auto& [speciesId, changes] : m_traitChangesBySpecies) {
        for (const TraitChange& change : changes) {
            trackedTraits.insert(change.traitType);
        }
    }

    for (GeneType trait : trackedTraits) {
        allTrends[trait] = getEvolutionaryTrends(trait);
    }

    return allTrends;
}

std::vector<GeneType> EvolutionaryHistoryTracker::getFastestEvolvingTraits(int count) const
{
    auto allTrends = getAllTraitTrends();

    std::vector<std::pair<GeneType, float>> traitRates;
    for (const auto& [trait, analysis] : allTrends) {
        float rate = std::abs(analysis.meanChangePerGeneration);
        traitRates.emplace_back(trait, rate);
    }

    // Sort by rate (descending)
    std::sort(traitRates.begin(), traitRates.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    std::vector<GeneType> result;
    for (int i = 0; i < count && i < static_cast<int>(traitRates.size()); i++) {
        result.push_back(traitRates[i].first);
    }

    return result;
}

// =============================================================================
// STATISTICS - GENETIC DIVERSITY
// =============================================================================

std::vector<GeneticDiversitySnapshot> EvolutionaryHistoryTracker::getGeneticDiversityHistory(
    int interval) const
{
    if (interval <= 0 || m_diversityHistory.empty()) {
        return std::vector<GeneticDiversitySnapshot>(m_diversityHistory.begin(),
                                                      m_diversityHistory.end());
    }

    // Filter by interval
    std::vector<GeneticDiversitySnapshot> filtered;
    Generation lastGen = -interval;

    for (const auto& snapshot : m_diversityHistory) {
        if (snapshot.generation - lastGen >= interval) {
            filtered.push_back(snapshot);
            lastGen = snapshot.generation;
        }
    }

    return filtered;
}

GeneticDiversitySnapshot EvolutionaryHistoryTracker::getCurrentDiversity() const
{
    if (m_diversityHistory.empty()) {
        GeneticDiversitySnapshot empty;
        empty.generation = m_currentGeneration;
        return empty;
    }

    return m_diversityHistory.back();
}

void EvolutionaryHistoryTracker::recordDiversitySnapshot(const GeneticDiversitySnapshot& snapshot)
{
    m_diversityHistory.push_back(snapshot);

    // Limit history size
    const size_t maxDiversityHistory = 10000;
    while (m_diversityHistory.size() > maxDiversityHistory) {
        m_diversityHistory.pop_front();
    }
}

// =============================================================================
// EXPORT FUNCTIONS
// =============================================================================

std::string EvolutionaryHistoryTracker::exportToNewick() const
{
    // Find root species
    std::vector<SpeciesId> roots;
    for (const auto& [speciesId, record] : m_phylogeneticRecords) {
        if (record.parentSpeciesId == 0) {
            roots.push_back(speciesId);
        }
    }

    if (roots.empty()) {
        return ";";
    }

    // Build Newick string from each root
    std::string newick;
    if (roots.size() == 1) {
        newick = newickForSpecies(roots[0]);
    } else {
        // Multiple roots - create a polytomy
        newick = "(";
        for (size_t i = 0; i < roots.size(); i++) {
            if (i > 0) newick += ",";
            newick += newickForSpecies(roots[i]);
        }
        newick += ")";
    }

    return newick + ";";
}

bool EvolutionaryHistoryTracker::exportNewickToFile(const std::string& filename) const
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    file << exportToNewick() << std::endl;
    file.close();
    return true;
}

bool EvolutionaryHistoryTracker::exportToCSV(const std::string& baseFilename) const
{
    bool success = true;

    success &= exportDataToCSV(baseFilename + "_lineages.csv", "lineages");
    success &= exportDataToCSV(baseFilename + "_species.csv", "species");
    success &= exportDataToCSV(baseFilename + "_traits.csv", "traits");
    success &= exportDataToCSV(baseFilename + "_diversity.csv", "diversity");

    return success;
}

bool EvolutionaryHistoryTracker::exportDataToCSV(const std::string& filename,
                                                  const std::string& dataType) const
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    if (dataType == "lineages") {
        // Header
        file << "LineageId,AncestorId,FoundingGeneration,ExtinctionGeneration,"
             << "PeakPopulation,TotalDescendants,SurvivingDescendants,AverageFitness\n";

        for (const auto& [lineageId, record] : m_lineageRecords) {
            file << lineageId << ","
                 << record.ancestorLineageId << ","
                 << record.foundingGeneration << ","
                 << record.extinctionGeneration << ","
                 << record.peakPopulation << ","
                 << record.totalDescendants << ","
                 << record.survivingDescendants << ","
                 << record.averageFitness << "\n";
        }
    } else if (dataType == "species") {
        // Header
        file << "SpeciesId,ParentSpeciesId,FoundingGeneration,ExtinctionGeneration,"
             << "FounderPopulation,PeakPopulation,DescendantCount,ExtinctionCause\n";

        for (const auto& [speciesId, record] : m_phylogeneticRecords) {
            file << speciesId << ","
                 << record.parentSpeciesId << ","
                 << record.foundingGeneration << ","
                 << record.extinctionGeneration << ","
                 << record.founderPopulationSize << ","
                 << record.peakPopulation << ","
                 << record.descendantCount << ","
                 << static_cast<int>(record.extinctionCause) << "\n";
        }
    } else if (dataType == "traits") {
        // Header
        file << "Generation,SpeciesId,TraitType,OldValue,NewValue,ChangeType,SelectionPressure\n";

        for (const auto& [speciesId, changes] : m_traitChangesBySpecies) {
            for (const TraitChange& change : changes) {
                file << change.generation << ","
                     << speciesId << ","
                     << static_cast<int>(change.traitType) << ","
                     << change.oldValue << ","
                     << change.newValue << ","
                     << static_cast<int>(change.changeType) << ","
                     << static_cast<int>(change.selectionPressure) << "\n";
            }
        }
    } else if (dataType == "diversity") {
        // Header
        file << "Generation,OverallHeterozygosity,NucleotideDiversity,"
             << "NumberOfAlleles,EffectivePopulationSize\n";

        for (const auto& snapshot : m_diversityHistory) {
            file << snapshot.generation << ","
                 << snapshot.overallHeterozygosity << ","
                 << snapshot.nucleotideDiversity << ","
                 << snapshot.numberOfAlleles << ","
                 << snapshot.effectivePopulationSize << "\n";
        }
    } else {
        file.close();
        return false;
    }

    file.close();
    return true;
}

// =============================================================================
// MEMORY MANAGEMENT
// =============================================================================

int EvolutionaryHistoryTracker::pruneOldRecords(Generation generationThreshold,
                                                 bool preserveAncestry)
{
    int prunedCount = 0;

    // Build set of lineages that must be preserved (ancestry of extant lineages)
    std::unordered_set<LineageId> preservedLineages;

    if (preserveAncestry) {
        for (const auto& [lineageId, record] : m_lineageRecords) {
            if (record.isExtant()) {
                // Keep this lineage and all its ancestors
                preservedLineages.insert(lineageId);
                auto ancestry = getAncestry(lineageId);
                for (LineageId ancestorId : ancestry) {
                    preservedLineages.insert(ancestorId);
                }
            }
        }
    }

    // Prune old lineage records
    std::vector<LineageId> toRemove;
    for (const auto& [lineageId, record] : m_lineageRecords) {
        // Check if record is old and extinct
        if (!record.isExtant() &&
            record.extinctionGeneration < generationThreshold &&
            preservedLineages.count(lineageId) == 0) {
            toRemove.push_back(lineageId);
        }
    }

    for (LineageId id : toRemove) {
        m_lineageRecords.erase(id);
        m_lineageTree.erase(id);
        prunedCount++;

        // Update parent's child list
        for (auto& [parentId, record] : m_lineageRecords) {
            auto& children = record.childLineages;
            children.erase(std::remove(children.begin(), children.end(), id),
                          children.end());
        }

        // Update tree parent's child list
        for (auto& [parentId, node] : m_lineageTree) {
            auto& children = node.childrenIds;
            children.erase(std::remove(children.begin(), children.end(), id),
                          children.end());
        }
    }

    // Prune old trait changes
    for (auto& [speciesId, changes] : m_traitChangesBySpecies) {
        auto newEnd = std::remove_if(changes.begin(), changes.end(),
            [generationThreshold](const TraitChange& change) {
                return change.generation < generationThreshold;
            });
        changes.erase(newEnd, changes.end());
    }

    // Clear ancestor cache (may contain stale entries)
    m_ancestorCache.clear();

    return prunedCount;
}

size_t EvolutionaryHistoryTracker::getMemoryUsage() const
{
    size_t usage = 0;

    // Lineage records
    usage += m_lineageRecords.size() * (sizeof(LineageId) + sizeof(LineageRecord));
    for (const auto& [id, record] : m_lineageRecords) {
        usage += record.speciesHistory.capacity() * sizeof(std::pair<Generation, SpeciesId>);
        usage += record.majorTraitChanges.capacity() * sizeof(TraitChange);
        usage += record.genomicChanges.capacity() * sizeof(GenomicChange);
        usage += record.childLineages.capacity() * sizeof(LineageId);
        if (record.founderGenome.has_value()) {
            usage += sizeof(DiploidGenome);
        }
    }

    // Phylogenetic records
    usage += m_phylogeneticRecords.size() * (sizeof(SpeciesId) + sizeof(PhylogeneticRecord));

    // Lineage tree
    usage += m_lineageTree.size() * (sizeof(LineageId) + sizeof(LineageTreeNode));

    // Trait changes
    for (const auto& [id, changes] : m_traitChangesBySpecies) {
        usage += changes.capacity() * sizeof(TraitChange);
    }

    // Diversity history
    usage += m_diversityHistory.size() * sizeof(GeneticDiversitySnapshot);

    // Generation-indexed events
    usage += m_speciationsByGeneration.size() * (sizeof(Generation) + sizeof(int));
    usage += m_extinctionsByGeneration.size() * (sizeof(Generation) + sizeof(int));

    // Ancestor cache
    usage += m_ancestorCache.size() * (sizeof(uint64_t) + sizeof(LineageId));

    return usage;
}

void EvolutionaryHistoryTracker::setMaxRecords(size_t maxLineages, size_t maxSpecies)
{
    m_maxLineageRecords = maxLineages;
    m_maxSpeciesRecords = maxSpecies;
}

void EvolutionaryHistoryTracker::clear()
{
    m_lineageRecords.clear();
    m_phylogeneticRecords.clear();
    m_lineageTree.clear();
    m_traitChangesBySpecies.clear();
    m_diversityHistory.clear();
    m_speciationsByGeneration.clear();
    m_extinctionsByGeneration.clear();
    m_ancestorCache.clear();

    m_currentGeneration = 0;
    m_totalBirths = 0;
    m_totalDeaths = 0;
    m_totalSpeciations = 0;
    m_totalExtinctions = 0;
}

// =============================================================================
// VALIDATION & DEBUGGING
// =============================================================================

bool EvolutionaryHistoryTracker::validateRecords() const
{
    bool valid = true;

    // Check lineage record consistency
    for (const auto& [lineageId, record] : m_lineageRecords) {
        // Check that ancestor exists (if specified)
        if (record.ancestorLineageId != 0) {
            if (m_lineageRecords.count(record.ancestorLineageId) == 0) {
                std::cerr << "Lineage " << lineageId << " references non-existent ancestor "
                          << record.ancestorLineageId << std::endl;
                valid = false;
            }
        }

        // Check that child lineages exist
        for (LineageId childId : record.childLineages) {
            if (m_lineageRecords.count(childId) == 0) {
                std::cerr << "Lineage " << lineageId << " references non-existent child "
                          << childId << std::endl;
                valid = false;
            }
        }

        // Check logical consistency
        if (record.extinctionGeneration >= 0 &&
            record.extinctionGeneration < record.foundingGeneration) {
            std::cerr << "Lineage " << lineageId << " has extinction before founding" << std::endl;
            valid = false;
        }
    }

    // Check phylogenetic record consistency
    for (const auto& [speciesId, record] : m_phylogeneticRecords) {
        if (record.parentSpeciesId != 0) {
            if (m_phylogeneticRecords.count(record.parentSpeciesId) == 0) {
                std::cerr << "Species " << speciesId << " references non-existent parent "
                          << record.parentSpeciesId << std::endl;
                valid = false;
            }
        }
    }

    return valid;
}

std::string EvolutionaryHistoryTracker::getDebugSummary() const
{
    std::stringstream ss;

    ss << "=== Evolutionary History Summary ===" << std::endl;
    ss << "Current generation: " << m_currentGeneration << std::endl;
    ss << "Total births: " << m_totalBirths << std::endl;
    ss << "Total deaths: " << m_totalDeaths << std::endl;
    ss << "Total speciations: " << m_totalSpeciations << std::endl;
    ss << "Total extinctions: " << m_totalExtinctions << std::endl;
    ss << std::endl;

    ss << "Record counts:" << std::endl;
    ss << "  Lineage records: " << m_lineageRecords.size() << std::endl;
    ss << "  Phylogenetic records: " << m_phylogeneticRecords.size() << std::endl;
    ss << "  Tree nodes: " << m_lineageTree.size() << std::endl;
    ss << "  Diversity snapshots: " << m_diversityHistory.size() << std::endl;
    ss << std::endl;

    // Count extant vs extinct lineages
    int extantLineages = 0;
    int extinctLineages = 0;
    for (const auto& [id, record] : m_lineageRecords) {
        if (record.isExtant()) extantLineages++;
        else extinctLineages++;
    }

    ss << "Lineage status:" << std::endl;
    ss << "  Extant: " << extantLineages << std::endl;
    ss << "  Extinct: " << extinctLineages << std::endl;
    ss << "  Max tree depth: " << getMaxTreeDepth() << std::endl;
    ss << std::endl;

    ss << "Memory usage: " << getMemoryUsage() / 1024 << " KB" << std::endl;

    return ss.str();
}

// =============================================================================
// INTERNAL HELPER METHODS
// =============================================================================

LineageId EvolutionaryHistoryTracker::createLineageRecord(const Creature* creature,
                                                           LineageId parentLineage)
{
    LineageId newId = s_nextLineageId++;

    LineageRecord record(newId, parentLineage, m_currentGeneration);

    const DiploidGenome& genome = creature->getDiploidGenome();

    // Store founder genome
    record.founderGenome = genome;
    record.founderFitness = creature->getFitness();

    // Initialize species history
    SpeciesId currentSpecies = genome.getSpeciesId();
    if (currentSpecies != 0) {
        record.speciesHistory.emplace_back(m_currentGeneration, currentSpecies);
    }

    // Set initial statistics
    record.totalDescendants = 1;
    record.survivingDescendants = 1;
    record.peakPopulation = 1;
    record.peakPopulationGen = m_currentGeneration;
    record.averageFitness = creature->getFitness();

    m_lineageRecords[newId] = std::move(record);

    return newId;
}

void EvolutionaryHistoryTracker::updateLineageOnDeath(LineageId lineageId)
{
    auto it = m_lineageRecords.find(lineageId);
    if (it == m_lineageRecords.end()) return;

    LineageRecord& record = it->second;
    record.survivingDescendants = std::max(0, record.survivingDescendants - 1);
}

std::unordered_set<LineageId> EvolutionaryHistoryTracker::buildAncestorSet(LineageId lineage) const
{
    std::unordered_set<LineageId> ancestors;

    LineageId current = lineage;
    while (current != 0) {
        auto it = m_lineageRecords.find(current);
        if (it == m_lineageRecords.end()) break;

        LineageId parent = it->second.ancestorLineageId;
        if (parent != 0) {
            ancestors.insert(parent);
        }
        current = parent;
    }

    return ancestors;
}

std::string EvolutionaryHistoryTracker::newickForSpecies(SpeciesId speciesId) const
{
    auto it = m_phylogeneticRecords.find(speciesId);
    if (it == m_phylogeneticRecords.end()) {
        return "Unknown_" + std::to_string(speciesId);
    }

    const PhylogeneticRecord& record = it->second;
    std::stringstream ss;

    // Check if this species has descendants
    if (!record.daughterSpecies.empty()) {
        ss << "(";
        for (size_t i = 0; i < record.daughterSpecies.size(); i++) {
            if (i > 0) ss << ",";
            ss << newickForSpecies(record.daughterSpecies[i]);
        }
        ss << ")";
    }

    // Add species name
    if (!record.speciesName.empty()) {
        ss << record.speciesName;
    } else {
        ss << "Species_" << speciesId;
    }

    // Add branch length (duration if extinct, time since founding if extant)
    float branchLength = 0.0f;
    if (record.parentSpeciesId != 0) {
        auto parentIt = m_phylogeneticRecords.find(record.parentSpeciesId);
        if (parentIt != m_phylogeneticRecords.end()) {
            branchLength = static_cast<float>(record.foundingGeneration -
                                               parentIt->second.foundingGeneration);
        }
    }
    if (branchLength > 0) {
        ss << ":" << branchLength;
    }

    return ss.str();
}

float EvolutionaryHistoryTracker::calculateSuccessScore(const LineageRecord& record) const
{
    // Weighted composite score for lineage success
    const float descendantWeight = 0.3f;
    const float survivalWeight = 0.2f;
    const float fitnessWeight = 0.2f;
    const float longevityWeight = 0.2f;
    const float speciesWeight = 0.1f;

    float score = 0.0f;

    // Normalize descendant count (log scale for large numbers)
    float descendantScore = record.totalDescendants > 0 ?
        std::log10(static_cast<float>(record.totalDescendants) + 1) / 5.0f : 0.0f;
    score += descendantWeight * std::min(1.0f, descendantScore);

    // Survival score (current descendants relative to total)
    float survivalScore = record.totalDescendants > 0 ?
        static_cast<float>(record.survivingDescendants) / record.totalDescendants : 0.0f;
    score += survivalWeight * survivalScore;

    // Fitness score (normalized)
    score += fitnessWeight * std::min(1.0f, record.averageFitness / 100.0f);

    // Longevity score (normalized by current generation)
    float longevityScore = m_currentGeneration > 0 ?
        static_cast<float>(record.getLifespan(m_currentGeneration)) / m_currentGeneration : 0.0f;
    score += longevityWeight * std::min(1.0f, longevityScore);

    // Species contribution
    float speciesScore = static_cast<float>(record.speciesHistory.size()) / 10.0f;
    score += speciesWeight * std::min(1.0f, speciesScore);

    return score;
}

void EvolutionaryHistoryTracker::autoPruneIfNeeded()
{
    // Check if we need to auto-prune lineage records
    if (m_maxLineageRecords > 0 && m_lineageRecords.size() > m_maxLineageRecords) {
        // Find a threshold that removes about 20% of old records
        std::vector<Generation> extinctionGens;
        for (const auto& [id, record] : m_lineageRecords) {
            if (!record.isExtant()) {
                extinctionGens.push_back(record.extinctionGeneration);
            }
        }

        if (!extinctionGens.empty()) {
            std::sort(extinctionGens.begin(), extinctionGens.end());
            size_t targetIndex = extinctionGens.size() / 5;  // 20%
            Generation threshold = extinctionGens[targetIndex];
            pruneOldRecords(threshold, true);
        }
    }

    // Check species records (less aggressive pruning)
    if (m_maxSpeciesRecords > 0 && m_phylogeneticRecords.size() > m_maxSpeciesRecords) {
        // Remove oldest extinct species with no extant descendants
        std::vector<std::pair<Generation, SpeciesId>> extinctSpecies;
        for (const auto& [speciesId, record] : m_phylogeneticRecords) {
            if (!record.isExtant() && record.descendantCount == 0) {
                extinctSpecies.emplace_back(record.extinctionGeneration, speciesId);
            }
        }

        std::sort(extinctSpecies.begin(), extinctSpecies.end());

        // Remove oldest 10%
        size_t toRemove = extinctSpecies.size() / 10;
        for (size_t i = 0; i < toRemove && i < extinctSpecies.size(); i++) {
            m_phylogeneticRecords.erase(extinctSpecies[i].second);
        }
    }
}

uint64_t EvolutionaryHistoryTracker::makeAncestorCacheKey(LineageId l1, LineageId l2) const
{
    // Ensure consistent ordering for cache key
    if (l1 > l2) std::swap(l1, l2);
    return (static_cast<uint64_t>(l1) << 32) | static_cast<uint64_t>(l2);
}

void EvolutionaryHistoryTracker::invalidateAncestorCache(LineageId lineage)
{
    // Simple approach: clear entries involving this lineage
    // For a more efficient approach, we could track cache dependencies
    std::vector<uint64_t> keysToRemove;

    for (const auto& [key, value] : m_ancestorCache) {
        LineageId l1 = static_cast<LineageId>(key >> 32);
        LineageId l2 = static_cast<LineageId>(key & 0xFFFFFFFF);
        if (l1 == lineage || l2 == lineage) {
            keysToRemove.push_back(key);
        }
    }

    for (uint64_t key : keysToRemove) {
        m_ancestorCache.erase(key);
    }
}

// =============================================================================
// ADDITIONAL METHODS FROM HEADER
// =============================================================================

float Species::getReproductiveIsolation(SpeciesId otherId) const {
    auto it = reproductiveIsolation.find(otherId);
    if (it != reproductiveIsolation.end()) {
        return it->second.totalIsolation;
    }
    return 0.0f;
}

float Species::getIsolationStrength(SpeciesId otherId, IsolationType type) const {
    auto it = reproductiveIsolation.find(otherId);
    if (it != reproductiveIsolation.end()) {
        auto typeIt = it->second.strengths.find(type);
        if (typeIt != it->second.strengths.end()) {
            return typeIt->second;
        }
    }
    return 0.0f;
}

void Species::updateIsolation(SpeciesId otherId, IsolationType type, float strength) {
    auto& isolationData = reproductiveIsolation[otherId];
    isolationData.strengths[type] = std::clamp(strength, 0.0f, 1.0f);
    isolationData.updateTotal();
}

void Species::accumulateIsolation(SpeciesId otherId, int generations) {
    auto& isolationData = reproductiveIsolation[otherId];
    isolationData.generationsSinceStart += generations;

    // Natural isolation accumulation over time
    for (auto& [type, strength] : isolationData.strengths) {
        // Slowly increase isolation barriers over time
        float increment = 0.001f * generations;
        isolationData.strengths[type] = std::min(1.0f, strength + increment);
    }
    isolationData.updateTotal();
}

const IsolationData* Species::getIsolationData(SpeciesId otherId) const {
    auto it = reproductiveIsolation.find(otherId);
    return (it != reproductiveIsolation.end()) ? &it->second : nullptr;
}

void Species::updateGeographicDistribution(const std::vector<Creature*>& memberList) {
    if (memberList.empty()) {
        geographicData = GeographicData();
        return;
    }

    // Calculate centroid
    glm::vec3 sum(0.0f);
    for (Creature* c : memberList) {
        sum += c->getPosition();
    }
    geographicData.centroid = sum / static_cast<float>(memberList.size());

    // Calculate spatial variance
    float variance = 0.0f;
    for (Creature* c : memberList) {
        glm::vec3 diff = c->getPosition() - geographicData.centroid;
        variance += glm::dot(diff, diff);
    }
    geographicData.spatialVariance = std::sqrt(variance / memberList.size());
}

float Species::calculateGeographicOverlap(const Species& other) const {
    // Simple overlap based on distance between centroids and spatial variances
    float distance = glm::length(geographicData.centroid - other.geographicData.centroid);
    float combinedVariance = geographicData.spatialVariance + other.geographicData.spatialVariance;

    if (combinedVariance <= 0.0f) return 0.0f;

    // Overlap decreases with distance, increases with combined variance
    float overlap = 1.0f - (distance / (combinedVariance * 3.0f));
    return std::clamp(overlap, 0.0f, 1.0f);
}

float Species::getHybridFitness(const Species& other) const {
    // Base hybrid fitness on genetic distance
    float distance = distanceTo(other);

    // Hybrids between close species have higher fitness
    // Very distant species produce inviable hybrids
    if (distance > 0.5f) return 0.0f;  // Too divergent

    return 1.0f - (distance * 2.0f);  // Linear decrease
}

void Species::trackHybrid(Creature* hybrid, SpeciesId otherSpeciesId) {
    hybridsByOtherSpecies[otherSpeciesId].push_back(hybrid);
}

int Species::getHybridCount(SpeciesId otherId) const {
    auto it = hybridsByOtherSpecies.find(otherId);
    return (it != hybridsByOtherSpecies.end()) ? static_cast<int>(it->second.size()) : 0;
}

ExtinctionRisk Species::assessExtinctionRisk(float environmentalStress) const {
    ExtinctionRisk risk;
    risk.populationSize = stats.size;
    risk.geneticDiversity = stats.averageHeterozygosity;
    risk.environmentalStress = environmentalStress;

    // Calculate risk score based on multiple factors
    float populationRisk = 0.0f;
    if (stats.size < 10) populationRisk = 1.0f;
    else if (stats.size < 50) populationRisk = 0.7f;
    else if (stats.size < 100) populationRisk = 0.4f;
    else populationRisk = 0.1f;

    float diversityRisk = 1.0f - stats.averageHeterozygosity;

    float stressRisk = environmentalStress;

    // Weighted combination
    risk.riskScore = (populationRisk * 0.4f) +
                     (diversityRisk * 0.3f) +
                     (stressRisk * 0.3f);

    // Identify primary threat
    if (populationRisk >= diversityRisk && populationRisk >= stressRisk) {
        risk.primaryThreat = "Low population size";
        risk.recommendation = "Increase population through protection or supplementation";
    } else if (diversityRisk >= stressRisk) {
        risk.primaryThreat = "Low genetic diversity";
        risk.recommendation = "Introduce genetic variation through managed breeding";
    } else {
        risk.primaryThreat = "Environmental stress";
        risk.recommendation = "Reduce environmental pressures or relocate population";
    }

    return risk;
}

float Species::calculateGeneticDistance(const Species& other, DistanceMetric metric) const {
    switch (metric) {
        case DistanceMetric::SIMPLE:
            return distanceTo(other);
        case DistanceMetric::NEI:
            return calculateNeisDistance(other);
        case DistanceMetric::FST:
            return calculateFST(other);
        case DistanceMetric::WEIGHTED:
        default:
            return calculateWeightedDistance(other);
    }
}

float Species::calculateNeisDistance(const Species& other) const {
    // Nei's genetic distance based on allele frequencies
    float identity = calculateGeneticIdentity(other);
    if (identity <= 0.0f) return std::numeric_limits<float>::max();
    return -std::log(identity);
}

float Species::calculateGeneticIdentity(const Species& other) const {
    // I = sum(sqrt(pi * qi)) / sqrt(sum(pi^2) * sum(qi^2))
    float sumProduct = 0.0f;
    float sumP2 = 0.0f;
    float sumQ2 = 0.0f;

    for (const auto& [alleleId, freqP] : alleleFrequencies) {
        float freqQ = other.getAlleleFrequency(alleleId);
        sumProduct += std::sqrt(freqP * freqQ);
        sumP2 += freqP * freqP;
        sumQ2 += freqQ * freqQ;
    }

    // Also consider alleles only in the other species
    for (const auto& [alleleId, freqQ] : other.alleleFrequencies) {
        if (alleleFrequencies.count(alleleId) == 0) {
            sumQ2 += freqQ * freqQ;
        }
    }

    float denominator = std::sqrt(sumP2 * sumQ2);
    return denominator > 0.0f ? sumProduct / denominator : 0.0f;
}

float Species::calculateFST(const Species& other) const {
    // Wright's FST = (Ht - Hs) / Ht
    // Ht = total heterozygosity, Hs = subpopulation heterozygosity

    float hs = (calculateHeterozygosity() + other.calculateHeterozygosity()) / 2.0f;

    // Calculate total (combined) heterozygosity
    std::map<uint32_t, float> combinedFreq;
    for (const auto& [id, freq] : alleleFrequencies) {
        combinedFreq[id] += freq / 2.0f;
    }
    for (const auto& [id, freq] : other.alleleFrequencies) {
        combinedFreq[id] += freq / 2.0f;
    }

    float ht = 0.0f;
    for (const auto& [id, freq] : combinedFreq) {
        ht += freq * freq;
    }
    ht = 1.0f - ht;

    if (ht <= 0.0f) return 0.0f;
    return (ht - hs) / ht;
}

float Species::calculateHeterozygosity() const {
    // Expected heterozygosity = 1 - sum(pi^2)
    float sumP2 = 0.0f;
    for (const auto& [id, freq] : alleleFrequencies) {
        sumP2 += freq * freq;
    }
    return 1.0f - sumP2;
}

float Species::calculateWeightedDistance(const Species& other) const {
    // Weighted combination of multiple distance metrics
    float simple = distanceTo(other);
    float nei = calculateNeisDistance(other);
    float fst = calculateFST(other);

    // Normalize Nei's distance (can be very large)
    nei = std::min(1.0f, nei / 5.0f);

    return (simple * 0.4f) + (nei * 0.3f) + (fst * 0.3f);
}

void Species::trackDistanceTrend(SpeciesId otherId, float distance, int generation) {
    GeneticDistanceMetrics metrics;
    metrics.generation = generation;
    metrics.weightedEuclidean = distance;

    auto& trend = distanceTrends[otherId];
    trend.push_back(metrics);

    // Limit history size
    while (trend.size() > MAX_DISTANCE_HISTORY) {
        trend.pop_front();
    }
}

std::vector<GeneticDistanceMetrics> Species::getDistanceTrend(SpeciesId otherId,
                                                               int lastNGenerations) const {
    auto it = distanceTrends.find(otherId);
    if (it == distanceTrends.end()) {
        return {};
    }

    const auto& trend = it->second;
    std::vector<GeneticDistanceMetrics> result;

    for (const auto& metrics : trend) {
        result.push_back(metrics);
    }

    // Filter to last N generations if specified
    if (lastNGenerations > 0 && !result.empty()) {
        int cutoffGen = result.back().generation - lastNGenerations;
        auto newEnd = std::remove_if(result.begin(), result.end(),
            [cutoffGen](const GeneticDistanceMetrics& m) {
                return m.generation < cutoffGen;
            });
        result.erase(newEnd, result.end());
    }

    return result;
}

// SpeciationTracker additional methods
bool SpeciationTracker::detectGeographicIsolation(const std::vector<Creature*>& population,
                                                   SpeciesId speciesId) {
    auto subpops = detectSubpopulations(population, 50.0f);

    if (subpops.size() < 2) return false;

    // Check if gene flow is restricted between subpopulations
    for (size_t i = 0; i < subpops.size(); i++) {
        for (size_t j = i + 1; j < subpops.size(); j++) {
            float geneFlow = calculateGeneFlow(subpops[i], subpops[j]);
            if (geneFlow < 0.05f) {  // Very low gene flow
                return true;
            }
        }
    }

    return false;
}

std::vector<std::vector<Creature*>> SpeciationTracker::detectSubpopulations(
    const std::vector<Creature*>& population, float maxDistance) {

    std::vector<std::vector<Creature*>> subpops;
    std::vector<bool> assigned(population.size(), false);

    for (size_t i = 0; i < population.size(); i++) {
        if (assigned[i]) continue;

        std::vector<Creature*> group;
        std::queue<size_t> toProcess;
        toProcess.push(i);
        assigned[i] = true;

        while (!toProcess.empty()) {
            size_t idx = toProcess.front();
            toProcess.pop();
            group.push_back(population[idx]);

            for (size_t j = 0; j < population.size(); j++) {
                if (!assigned[j]) {
                    float dist = glm::length(population[idx]->getPosition() -
                                            population[j]->getPosition());
                    if (dist < maxDistance) {
                        assigned[j] = true;
                        toProcess.push(j);
                    }
                }
            }
        }

        if (!group.empty()) {
            subpops.push_back(group);
        }
    }

    return subpops;
}

float SpeciationTracker::calculateGeneFlow(const std::vector<Creature*>& pop1,
                                            const std::vector<Creature*>& pop2) {
    if (pop1.empty() || pop2.empty()) return 0.0f;

    // Simple estimate based on spatial distance and population overlap
    float distance = calculateSpatialDistance(pop1, pop2);

    // Gene flow decreases exponentially with distance
    float dispersalScale = 100.0f;  // Typical dispersal distance
    return std::exp(-distance / dispersalScale);
}

float SpeciationTracker::calculateSpatialDistance(const std::vector<Creature*>& group1,
                                                   const std::vector<Creature*>& group2) {
    // Calculate centroid distance
    glm::vec3 centroid1(0.0f), centroid2(0.0f);

    for (Creature* c : group1) {
        centroid1 += c->getPosition();
    }
    centroid1 /= static_cast<float>(group1.size());

    for (Creature* c : group2) {
        centroid2 += c->getPosition();
    }
    centroid2 /= static_cast<float>(group2.size());

    return glm::length(centroid1 - centroid2);
}

bool SpeciationTracker::detectSympatricSpeciation(const std::vector<Creature*>& population,
                                                   SpeciesId speciesId) {
    if (population.size() < static_cast<size_t>(minPopulationForSpecies * 2)) {
        return false;
    }

    // Check for disruptive selection
    if (detectDisruptiveSelection(population)) {
        return true;
    }

    // Check for strong assortative mating
    float assortative = calculateAssortativeMating(population);
    return assortative > 0.8f;
}

float SpeciationTracker::calculateNicheDivergence(const std::vector<Creature*>& group1,
                                                   const std::vector<Creature*>& group2) {
    if (group1.empty() || group2.empty()) return 0.0f;

    // Calculate average niche for each group
    EcologicalNiche niche1, niche2;
    niche1.dietSpecialization = 0;
    niche1.habitatPreference = 0;
    niche1.activityTime = 0;
    niche2 = niche1;

    for (Creature* c : group1) {
        auto n = c->getDiploidGenome().getEcologicalNiche();
        niche1.dietSpecialization += n.dietSpecialization;
        niche1.habitatPreference += n.habitatPreference;
        niche1.activityTime += n.activityTime;
    }
    niche1.dietSpecialization /= group1.size();
    niche1.habitatPreference /= group1.size();
    niche1.activityTime /= group1.size();

    for (Creature* c : group2) {
        auto n = c->getDiploidGenome().getEcologicalNiche();
        niche2.dietSpecialization += n.dietSpecialization;
        niche2.habitatPreference += n.habitatPreference;
        niche2.activityTime += n.activityTime;
    }
    niche2.dietSpecialization /= group2.size();
    niche2.habitatPreference /= group2.size();
    niche2.activityTime /= group2.size();

    return niche1.distanceTo(niche2);
}

bool SpeciationTracker::detectDisruptiveSelection(const std::vector<Creature*>& population) {
    if (population.size() < 20) return false;

    // Check for bimodal distribution in key traits
    std::vector<float> sizes;
    for (Creature* c : population) {
        sizes.push_back(c->getDiploidGenome().getTrait(GeneType::SIZE));
    }

    // Simple bimodality check: compare variance of halves to whole
    std::sort(sizes.begin(), sizes.end());
    size_t mid = sizes.size() / 2;

    float meanAll = std::accumulate(sizes.begin(), sizes.end(), 0.0f) / sizes.size();
    float varAll = 0.0f;
    for (float s : sizes) {
        varAll += (s - meanAll) * (s - meanAll);
    }
    varAll /= sizes.size();

    float meanLow = std::accumulate(sizes.begin(), sizes.begin() + mid, 0.0f) / mid;
    float meanHigh = std::accumulate(sizes.begin() + mid, sizes.end(), 0.0f) / (sizes.size() - mid);

    // If means are far apart and variance is high, likely disruptive selection
    float meanDiff = std::abs(meanHigh - meanLow);
    return (meanDiff > 0.3f) && (varAll > 0.1f);
}

float SpeciationTracker::calculateAssortativeMating(const std::vector<Creature*>& population) {
    // Estimate assortative mating from similarity preferences
    float totalSimilarityPref = 0.0f;
    int count = 0;

    for (Creature* c : population) {
        MatePreferences prefs = c->getDiploidGenome().getMatePreferences();
        totalSimilarityPref += prefs.similarityPreference;
        count++;
    }

    if (count == 0) return 0.0f;

    float avgPref = totalSimilarityPref / count;

    // Convert to 0-1 scale (preferences are typically -1 to 1)
    return (avgPref + 1.0f) / 2.0f;
}

void SpeciationTracker::trackHybridZone(SpeciesId species1, SpeciesId species2,
                                         const std::vector<Creature*>& hybrids) {
    auto key = makeSpeciesPair(species1, species2);
    HybridData& data = hybridZones[key];

    data.species1 = key.first;
    data.species2 = key.second;
    data.hybrids = hybrids;
    data.generationsSinceFormation++;

    if (!hybrids.empty()) {
        // Calculate average fitness
        float totalFitness = 0.0f;
        glm::vec3 centroid(0.0f);

        for (Creature* h : hybrids) {
            totalFitness += h->getFitness();
            centroid += h->getPosition();
        }

        data.averageFitness = totalFitness / hybrids.size();
        data.zoneCentroid = centroid / static_cast<float>(hybrids.size());
        data.zoneWidth = calculateHybridZoneWidth(hybrids);
    }
}

HybridData* SpeciationTracker::getHybridZone(SpeciesId species1, SpeciesId species2) {
    auto key = makeSpeciesPair(species1, species2);
    auto it = hybridZones.find(key);
    return (it != hybridZones.end()) ? &it->second : nullptr;
}

const HybridData* SpeciationTracker::getHybridZone(SpeciesId species1, SpeciesId species2) const {
    auto key = makeSpeciesPair(species1, species2);
    auto it = hybridZones.find(key);
    return (it != hybridZones.end()) ? &it->second : nullptr;
}

float SpeciationTracker::calculateIntrogression(SpeciesId species1, SpeciesId species2) {
    const HybridData* zone = getHybridZone(species1, species2);
    if (!zone || zone->hybrids.empty()) return 0.0f;

    Species* sp1 = getSpecies(species1);
    Species* sp2 = getSpecies(species2);
    if (!sp1 || !sp2) return 0.0f;

    // Estimate introgression as ratio of hybrids to total population
    int totalPop = sp1->getStats().size + sp2->getStats().size + zone->hybrids.size();
    if (totalPop == 0) return 0.0f;

    return static_cast<float>(zone->hybrids.size()) / totalPop;
}

float SpeciationTracker::calculateHybridZoneWidth(const std::vector<Creature*>& hybrids) {
    if (hybrids.size() < 2) return 0.0f;

    // Calculate spatial extent
    glm::vec3 minPos = hybrids[0]->getPosition();
    glm::vec3 maxPos = minPos;

    for (Creature* h : hybrids) {
        glm::vec3 pos = h->getPosition();
        minPos = glm::min(minPos, pos);
        maxPos = glm::max(maxPos, pos);
    }

    return glm::length(maxPos - minPos);
}

bool SpeciationTracker::detectHybridSwarm(SpeciesId species1, SpeciesId species2) {
    const HybridData* zone = getHybridZone(species1, species2);
    if (!zone) return false;

    // Hybrid swarm if:
    // 1. High proportion of hybrids
    // 2. Hybrids have decent fitness
    float introgression = calculateIntrogression(species1, species2);
    return (introgression > 0.3f) && (zone->averageFitness > 0.5f);
}

float SpeciationTracker::getBackgroundExtinctionRate(int generationWindow) const {
    int extinctions = 0;

    for (const auto& event : extinctionEventLog) {
        if (event.generation >= (speciationEventLog.empty() ? 0 :
            speciationEventLog.back().generation - generationWindow)) {
            extinctions++;
        }
    }

    return generationWindow > 0 ?
        static_cast<float>(extinctions) / generationWindow : 0.0f;
}

std::vector<ExtinctionEvent> SpeciationTracker::getMassExtinctionEvents(float threshold) const {
    std::vector<ExtinctionEvent> massExtinctions;

    // Group extinctions by generation
    std::map<int, std::vector<const ExtinctionEvent*>> byGeneration;
    for (const auto& event : extinctionEventLog) {
        byGeneration[event.generation].push_back(&event);
    }

    // Find generations with high extinction rates
    for (const auto& [gen, events] : byGeneration) {
        float rate = static_cast<float>(events.size()) / getTotalSpeciesCount();
        if (rate >= threshold) {
            for (const ExtinctionEvent* event : events) {
                massExtinctions.push_back(*event);
            }
        }
    }

    return massExtinctions;
}

void SpeciationTracker::logSpeciationEvent(int generation, SpeciesId parentId, SpeciesId childId,
                                            SpeciationCause cause, float divergence,
                                            float geoDist, float nicheDist, int founderPop,
                                            const std::string& description) {
    SpeciationEvent event;
    event.generation = generation;
    event.parentId = parentId;
    event.childId = childId;
    event.cause = cause;
    event.geneticDivergence = divergence;
    event.geographicDistance = geoDist;
    event.nicheDivergence = nicheDist;
    event.founderPopulation = founderPop;
    event.description = description;

    speciationEventLog.push_back(event);
}

void SpeciationTracker::logExtinctionEvent(int generation, SpeciesId speciesId,
                                            const std::string& speciesName,
                                            ExtinctionCause cause, int finalPop, float finalDiv,
                                            float finalFit, int genExisted,
                                            const std::string& description) {
    ExtinctionEvent event;
    event.generation = generation;
    event.speciesId = speciesId;
    event.speciesName = speciesName;
    event.cause = cause;
    event.finalPopulation = finalPop;
    event.finalDiversity = finalDiv;
    event.finalFitness = finalFit;
    event.generationsExisted = genExisted;
    event.description = description;

    extinctionEventLog.push_back(event);
}

std::vector<SpeciationEvent> SpeciationTracker::getSpeciationEvents(int sinceGeneration) const {
    std::vector<SpeciationEvent> events;
    for (const auto& event : speciationEventLog) {
        if (event.generation >= sinceGeneration) {
            events.push_back(event);
        }
    }
    return events;
}

std::vector<ExtinctionEvent> SpeciationTracker::getExtinctionEvents(int sinceGeneration) const {
    std::vector<ExtinctionEvent> events;
    for (const auto& event : extinctionEventLog) {
        if (event.generation >= sinceGeneration) {
            events.push_back(event);
        }
    }
    return events;
}

float SpeciationTracker::getSpeciationRate(int generationWindow) const {
    int events = 0;
    int latestGen = speciationEventLog.empty() ? 0 : speciationEventLog.back().generation;

    for (const auto& event : speciationEventLog) {
        if (event.generation >= latestGen - generationWindow) {
            events++;
        }
    }

    return generationWindow > 0 ?
        static_cast<float>(events) / generationWindow : 0.0f;
}

void SpeciationTracker::exportEventsToCSV(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) return;

    // Header
    file << "EventType,Generation,ParentId,ChildId,Cause,Divergence,Description\n";

    // Speciation events
    for (const auto& event : speciationEventLog) {
        file << "SPECIATION," << event.generation << ","
             << event.parentId << "," << event.childId << ","
             << static_cast<int>(event.cause) << ","
             << event.geneticDivergence << ","
             << "\"" << event.description << "\"\n";
    }

    // Extinction events
    for (const auto& event : extinctionEventLog) {
        file << "EXTINCTION," << event.generation << ","
             << event.speciesId << ",0,"
             << static_cast<int>(event.cause) << ","
             << event.finalDiversity << ","
             << "\"" << event.description << "\"\n";
    }

    file.close();
}

const SpeciationEvent* SpeciationTracker::getSpeciationEventForSpecies(SpeciesId speciesId) const {
    for (const auto& event : speciationEventLog) {
        if (event.childId == speciesId) {
            return &event;
        }
    }
    return nullptr;
}

std::pair<SpeciesId, SpeciesId> SpeciationTracker::makeSpeciesPair(SpeciesId s1, SpeciesId s2) const {
    return s1 < s2 ? std::make_pair(s1, s2) : std::make_pair(s2, s1);
}

Species* SpeciationTracker::createSpecies(const std::vector<Creature*>& founders, int generation,
                                           SpeciesId parentId, SpeciationCause cause) {
    if (founders.empty()) return nullptr;

    auto newSpecies = std::make_unique<Species>();
    newSpecies->setName(generateSpeciesName(species.size()));
    newSpecies->setFoundingGeneration(generation);

    if (!founders.empty()) {
        newSpecies->setFoundingLineage(founders[0]->getDiploidGenome().getLineageId());
    }

    SpeciesId newId = newSpecies->getId();

    // Assign founders to new species
    for (Creature* c : founders) {
        DiploidGenome& genome = const_cast<DiploidGenome&>(c->getDiploidGenome());
        genome.setSpeciesId(newId);
        newSpecies->addMember(c);
    }

    newSpecies->updateStatistics(founders);

    Species* ptr = newSpecies.get();
    species.push_back(std::move(newSpecies));

    // Add to tree
    if (parentId != 0) {
        tree.addSpeciation(parentId, newId, generation);
    } else if (species.size() == 1) {
        tree.addRoot(newId, generation);
    }

    return ptr;
}

ExtinctionCause SpeciationTracker::determineExtinctionCause(const Species* sp, int generation) {
    if (!sp) return ExtinctionCause::UNKNOWN;

    const PopulationStats& stats = sp->getStats();

    // Check various factors
    if (stats.historicalMinimum < 10 && stats.generationsSinceBottleneck < 20) {
        return ExtinctionCause::POPULATION_DECLINE;
    }

    if (stats.averageGeneticLoad > 0.5f) {
        return ExtinctionCause::GENETIC_COLLAPSE;
    }

    if (stats.averageHeterozygosity < 0.1f) {
        return ExtinctionCause::GENETIC_COLLAPSE;
    }

    // Default
    return ExtinctionCause::MULTIPLE_FACTORS;
}

} // namespace genetics
