#pragma once

#include "DiploidGenome.h"
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include <deque>
#include <string>
#include <memory>

class Creature;

namespace genetics {
    // Forward declaration for similarity system
    class SpeciesSimilaritySystem;
}

namespace genetics {

// =============================================================================
// ISOLATION TYPES
// =============================================================================

enum class IsolationType {
    NONE,            // No isolation barrier

    // Pre-zygotic barriers
    BEHAVIORAL,      // Different mating behaviors/displays
    TEMPORAL,        // Different breeding times
    MECHANICAL,      // Physical incompatibility
    GAMETIC,         // Gamete incompatibility
    ECOLOGICAL,      // Different habitats/niches
    GEOGRAPHIC,      // Physical separation

    // Post-zygotic barriers
    HYBRID_INVIABILITY,   // Hybrids don't survive
    HYBRID_STERILITY,     // Hybrids can't reproduce
    HYBRID_BREAKDOWN      // F2 generation has problems
};

// =============================================================================
// SPECIATION CAUSES
// =============================================================================

enum class SpeciationCause {
    ALLOPATRIC,      // Geographic isolation
    SYMPATRIC,       // Niche differentiation without geographic isolation
    PARAPATRIC,      // Adjacent populations with gene flow
    POLYPLOIDY,      // Chromosome duplication
    HYBRID_SPECIATION,  // New species from hybrid zone
    UNKNOWN
};

// =============================================================================
// EXTINCTION CAUSES
// =============================================================================

enum class ExtinctionCause {
    POPULATION_DECLINE,   // Numbers fell too low
    GENETIC_COLLAPSE,     // Inbreeding/genetic load
    ENVIRONMENTAL,        // Climate/habitat change
    COMPETITION,          // Outcompeted by other species
    PREDATION,            // Excessive predation
    DISEASE,              // Epidemic
    HYBRIDIZATION,        // Genetic swamping
    MULTIPLE_FACTORS,     // Combination of causes
    UNKNOWN
};

// =============================================================================
// GENETIC DISTANCE METRICS
// =============================================================================

enum class DistanceMetric {
    SIMPLE,       // Euclidean trait distance
    NEI,          // Nei's genetic distance
    FST,          // Wright's FST
    WEIGHTED      // Weighted by gene importance
};

struct GeneticDistanceMetrics {
    int generation;
    float neiDistance;
    float fst;
    float weightedEuclidean;
    float identity;

    GeneticDistanceMetrics()
        : generation(0), neiDistance(0.0f), fst(0.0f),
          weightedEuclidean(0.0f), identity(1.0f) {}
};

// =============================================================================
// ISOLATION DATA
// =============================================================================

struct IsolationData {
    std::map<IsolationType, float> strengths;
    float totalIsolation;
    int generationsSinceStart;

    IsolationData() : totalIsolation(0.0f), generationsSinceStart(0) {}

    void updateTotal() {
        // Pre-zygotic isolation is multiplicative
        float preZygotic = 1.0f;
        for (const auto& [type, strength] : strengths) {
            if (type <= IsolationType::GEOGRAPHIC) {
                preZygotic *= (1.0f - strength);
            }
        }
        preZygotic = 1.0f - preZygotic;

        // Post-zygotic is additive
        float postZygotic = 0.0f;
        for (const auto& [type, strength] : strengths) {
            if (type > IsolationType::GEOGRAPHIC) {
                postZygotic += strength;
            }
        }
        postZygotic = std::min(1.0f, postZygotic / 3.0f);

        // Total isolation combines both
        totalIsolation = preZygotic + (1.0f - preZygotic) * postZygotic;
    }
};

// =============================================================================
// GEOGRAPHIC DATA
// =============================================================================

struct GeographicData {
    glm::vec3 centroid;
    float spatialVariance;
    float fragmentationIndex;
    std::vector<glm::vec3> subpopulationCentroids;

    GeographicData()
        : centroid(0.0f), spatialVariance(0.0f), fragmentationIndex(0.0f) {}
};

// =============================================================================
// HYBRID ZONE DATA
// =============================================================================

struct HybridData {
    SpeciesId species1;
    SpeciesId species2;
    std::vector<Creature*> hybrids;
    float averageFitness;
    float zoneWidth;
    glm::vec3 zoneCentroid;
    int generationsSinceFormation;

    HybridData()
        : species1(0), species2(0), averageFitness(0.0f),
          zoneWidth(0.0f), zoneCentroid(0.0f), generationsSinceFormation(0) {}
};

// =============================================================================
// EXTINCTION RISK ASSESSMENT
// =============================================================================

struct ExtinctionRisk {
    int populationSize;
    float geneticDiversity;
    float environmentalStress;
    float riskScore;
    std::string primaryThreat;
    std::string recommendation;

    ExtinctionRisk()
        : populationSize(0), geneticDiversity(0.0f),
          environmentalStress(0.0f), riskScore(0.0f) {}
};

// =============================================================================
// SPECIATION EVENT RECORD
// =============================================================================

struct SpeciationEvent {
    int generation;
    SpeciesId parentId;
    SpeciesId childId;
    SpeciationCause cause;
    float geneticDivergence;
    float geographicDistance;
    float nicheDivergence;
    int founderPopulation;
    std::string description;

    SpeciationEvent()
        : generation(0), parentId(0), childId(0),
          cause(SpeciationCause::UNKNOWN), geneticDivergence(0.0f),
          geographicDistance(0.0f), nicheDivergence(0.0f), founderPopulation(0) {}
};

// =============================================================================
// EXTINCTION EVENT RECORD
// =============================================================================

struct ExtinctionEvent {
    int generation;
    SpeciesId speciesId;
    std::string speciesName;
    ExtinctionCause cause;
    int finalPopulation;
    float finalDiversity;
    float finalFitness;
    int generationsExisted;
    std::string description;

    ExtinctionEvent()
        : generation(0), speciesId(0), cause(ExtinctionCause::UNKNOWN),
          finalPopulation(0), finalDiversity(0.0f), finalFitness(0.0f),
          generationsExisted(0) {}
};

// =============================================================================
// POPULATION STATISTICS
// =============================================================================

struct PopulationStats {
    int size;
    float averageHeterozygosity;
    float averageFitness;
    float averageGeneticLoad;
    float effectivePopulationSize;
    int generationsSinceBottleneck;
    int historicalMinimum;

    PopulationStats()
        : size(0), averageHeterozygosity(0.0f), averageFitness(0.0f),
          averageGeneticLoad(0.0f), effectivePopulationSize(0.0f),
          generationsSinceBottleneck(0), historicalMinimum(INT_MAX) {}
};

// =============================================================================
// SPECIES CLASS
// =============================================================================

class Species {
public:
    Species();
    Species(SpeciesId id, const std::string& name);

    // =========================================================================
    // BASIC GETTERS
    // =========================================================================

    SpeciesId getId() const { return id; }
    const std::string& getName() const { return name; }
    uint64_t getFoundingLineage() const { return foundingLineage; }
    int getFoundingGeneration() const { return foundingGeneration; }
    const PopulationStats& getStats() const { return stats; }
    const EcologicalNiche& getNiche() const { return niche; }
    bool isExtinct() const { return extinct; }
    int getExtinctionGeneration() const { return extinctionGeneration; }

    // =========================================================================
    // BASIC SETTERS
    // =========================================================================

    void setName(const std::string& n) { name = n; }
    void setFoundingLineage(uint64_t lineage) { foundingLineage = lineage; }
    void setFoundingGeneration(int gen) { foundingGeneration = gen; }
    void markExtinct(int generation);

    // =========================================================================
    // POPULATION MANAGEMENT
    // =========================================================================

    void addMember(Creature* creature);
    void removeMember(Creature* creature);
    void updateStatistics(const std::vector<Creature*>& members);

    // =========================================================================
    // ALLELE FREQUENCY TRACKING
    // =========================================================================

    float getAlleleFrequency(uint32_t alleleId) const;
    void updateAlleleFrequencies(const std::vector<Creature*>& members);

    // =========================================================================
    // ENHANCED GENETIC DISTANCE CALCULATION
    // =========================================================================

    float calculateGeneticDistance(const Species& other, DistanceMetric metric = DistanceMetric::WEIGHTED) const;
    float calculateNeisDistance(const Species& other) const;
    float calculateGeneticIdentity(const Species& other) const;
    float calculateFST(const Species& other) const;
    float calculateHeterozygosity() const;
    float calculateWeightedDistance(const Species& other) const;

    void trackDistanceTrend(SpeciesId otherId, float distance, int generation);
    std::vector<GeneticDistanceMetrics> getDistanceTrend(SpeciesId otherId, int lastNGenerations = 50) const;

    // =========================================================================
    // REPRODUCTIVE ISOLATION MECHANISMS
    // =========================================================================

    float getReproductiveIsolation(SpeciesId otherId) const;
    float getIsolationStrength(SpeciesId otherId, IsolationType type) const;
    void updateIsolation(SpeciesId otherId, IsolationType type, float strength);
    void accumulateIsolation(SpeciesId otherId, int generations);
    const IsolationData* getIsolationData(SpeciesId otherId) const;
    void setReproductiveIsolation(SpeciesId otherId, float isolation);
    bool canInterbreedWith(const Species& other) const;

    // =========================================================================
    // GEOGRAPHIC DISTRIBUTION
    // =========================================================================

    void updateGeographicDistribution(const std::vector<Creature*>& members);
    const GeographicData& getGeographicData() const { return geographicData; }
    float calculateGeographicOverlap(const Species& other) const;

    // =========================================================================
    // HYBRID INTERACTIONS
    // =========================================================================

    float getHybridFitness(const Species& other) const;
    void trackHybrid(Creature* hybrid, SpeciesId otherSpeciesId);
    int getHybridCount(SpeciesId otherId) const;

    // =========================================================================
    // EXTINCTION RISK
    // =========================================================================

    ExtinctionRisk assessExtinctionRisk(float environmentalStress = 0.0f) const;

    // =========================================================================
    // LEGACY METHODS
    // =========================================================================

    DiploidGenome getRepresentativeGenome() const;
    float distanceTo(const Species& other) const;
    glm::vec3 getColor() const;

private:
    SpeciesId id;
    std::string name;
    uint64_t foundingLineage;
    int foundingGeneration;

    PopulationStats stats;
    EcologicalNiche niche;

    std::map<uint32_t, float> alleleFrequencies;
    std::map<SpeciesId, IsolationData> reproductiveIsolation;

    bool extinct;
    int extinctionGeneration;

    std::vector<Creature*> members;

    // Geographic tracking
    GeographicData geographicData;

    // Hybrid tracking
    std::map<SpeciesId, std::vector<Creature*>> hybridsByOtherSpecies;

    // Distance trends
    static const size_t MAX_DISTANCE_HISTORY = 100;
    std::map<SpeciesId, std::deque<GeneticDistanceMetrics>> distanceTrends;

    static SpeciesId nextId;
};

// =============================================================================
// PHYLOGENETIC TREE NODE
// =============================================================================

struct PhyloNode {
    uint64_t id;
    uint64_t parentId;
    SpeciesId speciesId;
    int generation;
    float branchLength;
    std::vector<uint64_t> childrenIds;
    bool isExtant;
    glm::vec3 color;

    PhyloNode()
        : id(0), parentId(0), speciesId(0), generation(0),
          branchLength(0.0f), isExtant(true), color(1.0f) {}
};

// =============================================================================
// PHYLOGENETIC TREE
// =============================================================================

class PhylogeneticTree {
public:
    PhylogeneticTree();

    // Tree construction
    uint64_t addRoot(SpeciesId speciesId, int generation);
    uint64_t addSpeciation(SpeciesId parentSpecies, SpeciesId childSpecies, int generation);
    void markExtinction(SpeciesId species, int generation);

    // Tree queries
    SpeciesId getMostRecentCommonAncestor(SpeciesId sp1, SpeciesId sp2) const;
    float getEvolutionaryDistance(SpeciesId sp1, SpeciesId sp2) const;
    std::vector<SpeciesId> getDescendants(SpeciesId ancestor) const;
    std::vector<SpeciesId> getExtantSpecies() const;
    int getSpeciationCount() const;
    int getExtinctionCount() const;

    // Node access
    const PhyloNode* getNode(uint64_t nodeId) const;
    const PhyloNode* getNodeForSpecies(SpeciesId speciesId) const;

    // Export
    std::string toNewick() const;
    void exportNewick(const std::string& filename) const;

    // Statistics
    float getTreeDepth() const;
    float getAverageBranchLength() const;

private:
    std::map<uint64_t, PhyloNode> nodes;
    std::map<SpeciesId, uint64_t> speciesToNode;
    uint64_t rootId;
    uint64_t nextNodeId;

    std::string nodeToNewick(uint64_t nodeId) const;
    void collectDescendants(uint64_t nodeId, std::vector<SpeciesId>& result) const;
};

// =============================================================================
// SPECIATION TRACKER
// =============================================================================

class SpeciationTracker {
public:
    SpeciationTracker();

    // =========================================================================
    // CORE UPDATE
    // =========================================================================

    void update(std::vector<Creature*>& creatures, int currentGeneration);

    // =========================================================================
    // SPECIES QUERIES
    // =========================================================================

    Species* getSpecies(SpeciesId id);
    const Species* getSpecies(SpeciesId id) const;
    std::vector<Species*> getActiveSpecies();
    std::vector<const Species*> getActiveSpecies() const;
    std::vector<const Species*> getExtinctSpecies() const;

    // =========================================================================
    // TREE ACCESS
    // =========================================================================

    PhylogeneticTree& getPhylogeneticTree() { return tree; }
    const PhylogeneticTree& getPhylogeneticTree() const { return tree; }

    // =========================================================================
    // STATISTICS
    // =========================================================================

    int getActiveSpeciesCount() const;
    int getTotalSpeciesCount() const;
    int getSpeciationEventCount() const;
    int getExtinctionEventCount() const;

    // =========================================================================
    // CONFIGURATION
    // =========================================================================

    void setSpeciesThreshold(float threshold) { speciesThreshold = threshold; }
    void setMinPopulationForSpecies(int minPop) { minPopulationForSpecies = minPop; }

    // =========================================================================
    // ALLOPATRIC SPECIATION
    // =========================================================================

    bool detectGeographicIsolation(const std::vector<Creature*>& population, SpeciesId speciesId);
    std::vector<std::vector<Creature*>> detectSubpopulations(const std::vector<Creature*>& population, float maxDistance);
    float calculateGeneFlow(const std::vector<Creature*>& pop1, const std::vector<Creature*>& pop2);
    float calculateSpatialDistance(const std::vector<Creature*>& group1, const std::vector<Creature*>& group2);

    // =========================================================================
    // SYMPATRIC SPECIATION
    // =========================================================================

    bool detectSympatricSpeciation(const std::vector<Creature*>& population, SpeciesId speciesId);
    float calculateNicheDivergence(const std::vector<Creature*>& group1, const std::vector<Creature*>& group2);
    bool detectDisruptiveSelection(const std::vector<Creature*>& population);
    float calculateAssortativeMating(const std::vector<Creature*>& population);

    // =========================================================================
    // HYBRID ZONE TRACKING
    // =========================================================================

    void trackHybridZone(SpeciesId species1, SpeciesId species2, const std::vector<Creature*>& hybrids);
    HybridData* getHybridZone(SpeciesId species1, SpeciesId species2);
    const HybridData* getHybridZone(SpeciesId species1, SpeciesId species2) const;
    float calculateIntrogression(SpeciesId species1, SpeciesId species2);
    float calculateHybridZoneWidth(const std::vector<Creature*>& hybrids);
    bool detectHybridSwarm(SpeciesId species1, SpeciesId species2);

    // =========================================================================
    // EXTINCTION TRACKING
    // =========================================================================

    float getBackgroundExtinctionRate(int generationWindow = 100) const;
    std::vector<ExtinctionEvent> getMassExtinctionEvents(float threshold = 0.3f) const;

    // =========================================================================
    // EVENT LOGGING
    // =========================================================================

    void logSpeciationEvent(int generation, SpeciesId parentId, SpeciesId childId,
                           SpeciationCause cause, float divergence,
                           float geoDist, float nicheDist, int founderPop,
                           const std::string& description);
    void logExtinctionEvent(int generation, SpeciesId speciesId,
                           const std::string& speciesName,
                           ExtinctionCause cause, int finalPop, float finalDiv,
                           float finalFit, int genExisted,
                           const std::string& description);

    std::vector<SpeciationEvent> getSpeciationEvents(int sinceGeneration = 0) const;
    std::vector<ExtinctionEvent> getExtinctionEvents(int sinceGeneration = 0) const;
    float getSpeciationRate(int generationWindow = 100) const;
    void exportEventsToCSV(const std::string& filename) const;
    const SpeciationEvent* getSpeciationEventForSpecies(SpeciesId speciesId) const;

    // =========================================================================
    // UTILITY
    // =========================================================================

    static std::string generateSpeciesName(int index);

private:
    std::vector<std::unique_ptr<Species>> species;
    PhylogeneticTree tree;

    float speciesThreshold;
    int minPopulationForSpecies;
    int generationsForSpeciation;
    int speciationEvents;
    int extinctionEvents;

    // Event logs
    std::vector<SpeciationEvent> speciationEventLog;
    std::vector<ExtinctionEvent> extinctionEventLog;

    // Hybrid zones
    std::map<std::pair<SpeciesId, SpeciesId>, HybridData> hybridZones;

    // Clustering algorithm
    std::vector<std::vector<float>> buildDistanceMatrix(const std::vector<Creature*>& creatures);
    std::vector<int> clusterByDistance(const std::vector<std::vector<float>>& distances);

    // Detect speciation
    void checkForSpeciation(std::vector<Creature*>& creatures, int generation);

    // Detect extinction
    void checkForExtinction(int generation);
    ExtinctionCause determineExtinctionCause(const Species* sp, int generation);

    // Assign creatures to species
    void assignToSpecies(Creature* creature);

    // Create new species
    Species* createSpecies(const std::vector<Creature*>& founders, int generation,
                          SpeciesId parentId = 0, SpeciationCause cause = SpeciationCause::UNKNOWN);

    // Helper for ordered species pair key
    std::pair<SpeciesId, SpeciesId> makeSpeciesPair(SpeciesId s1, SpeciesId s2) const;
};

} // namespace genetics
