#pragma once

#include "Species.h"
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

namespace genetics {

// =============================================================================
// SPECIES FEATURE VECTOR
// =============================================================================
// A normalized feature vector representing a species' characteristics for
// similarity computation. All features are normalized to [0, 1].

struct SpeciesFeatureVector {
    float normalizedSize;           // Population size (log-scaled, normalized)
    float normalizedSpeed;          // Average movement speed
    float dietSpecialization;       // From ecological niche [0, 1]
    float habitatPreference;        // From ecological niche [0, 1]
    float activityTime;             // From ecological niche [0, 1]
    float genomicComplexity;        // Proxy for genome distance to centroid
    float heterozygosity;           // Genetic diversity measure
    float fitness;                  // Average fitness

    SpeciesFeatureVector()
        : normalizedSize(0.0f), normalizedSpeed(0.5f),
          dietSpecialization(0.5f), habitatPreference(0.5f),
          activityTime(0.5f), genomicComplexity(0.5f),
          heterozygosity(0.5f), fitness(0.5f) {}

    // Weighted L2 distance between feature vectors
    float distanceTo(const SpeciesFeatureVector& other,
                     const std::vector<float>& weights) const;

    // Default weights for similarity computation
    static std::vector<float> getDefaultWeights();
};

// =============================================================================
// SIMILARITY CLUSTER
// =============================================================================
// Represents a cluster of similar species with shared coloring.

struct SimilarityCluster {
    int clusterId;
    std::vector<SpeciesId> members;
    glm::vec3 baseColor;            // Primary color for this cluster
    glm::vec3 lightVariant;         // Light UI variant
    glm::vec3 darkVariant;          // Dark UI variant
    SpeciesFeatureVector centroid;  // Average feature vector of members
    float intraClusterDistance;     // Average distance within cluster

    SimilarityCluster()
        : clusterId(-1), baseColor(1.0f), lightVariant(1.0f),
          darkVariant(0.5f), intraClusterDistance(0.0f) {}
};

// =============================================================================
// SIMILARITY CACHE ENTRY
// =============================================================================
// Cached pairwise similarity to avoid recomputation.

struct SimilarityCacheEntry {
    SpeciesId species1;
    SpeciesId species2;
    float similarity;               // 0 = identical, 1 = maximally different
    int computedGeneration;

    SimilarityCacheEntry()
        : species1(0), species2(0), similarity(0.0f), computedGeneration(0) {}
};

// =============================================================================
// CLUSTERING METRICS
// =============================================================================
// Debug and performance tracking for clustering operations.

struct ClusteringMetrics {
    int clusterCount;
    int speciesCount;
    float averageClusterSize;
    float averageIntraDistance;     // Cohesion: lower is better
    float averageInterDistance;     // Separation: higher is better
    float silhouetteScore;          // -1 to 1, higher is better
    double computeTimeMs;
    int recomputeCount;

    ClusteringMetrics()
        : clusterCount(0), speciesCount(0), averageClusterSize(0.0f),
          averageIntraDistance(0.0f), averageInterDistance(0.0f),
          silhouetteScore(0.0f), computeTimeMs(0.0), recomputeCount(0) {}
};

// =============================================================================
// COLOR PALETTE CONFIGURATION
// =============================================================================
// Configuration for cluster color generation.

struct ColorPaletteConfig {
    float minHueDelta;              // Minimum hue separation between clusters (degrees)
    float baseSaturation;           // Saturation for base colors [0, 1]
    float baseLightness;            // Lightness for base colors [0, 1]
    float lightVariantLightness;    // Lightness for light variants
    float darkVariantLightness;     // Lightness for dark variants
    uint64_t shuffleSeed;           // Seed for color order shuffling

    ColorPaletteConfig()
        : minHueDelta(35.0f), baseSaturation(0.7f), baseLightness(0.55f),
          lightVariantLightness(0.75f), darkVariantLightness(0.35f),
          shuffleSeed(42) {}
};

// =============================================================================
// SPECIES SIMILARITY SYSTEM
// =============================================================================
// Main system for computing species similarity, clustering, and color assignment.
// Provides read-only APIs for other agents to consume.

class SpeciesSimilaritySystem {
public:
    SpeciesSimilaritySystem();
    ~SpeciesSimilaritySystem();

    // =========================================================================
    // INITIALIZATION & UPDATE
    // =========================================================================

    // Initialize with planet seed for deterministic coloring
    void initialize(uint64_t planetSeed);

    // Update clustering when species change (speciation/extinction/periodic)
    void update(const SpeciationTracker& tracker, int currentGeneration);

    // Force recomputation of all clusters
    void forceRecompute();

    // =========================================================================
    // CONFIGURATION
    // =========================================================================

    // Set similarity distance threshold for clustering (0.25 - 0.35 recommended)
    void setClusterThreshold(float threshold);
    float getClusterThreshold() const { return m_clusterThreshold; }

    // Set feature weights for similarity computation
    void setFeatureWeights(const std::vector<float>& weights);
    const std::vector<float>& getFeatureWeights() const { return m_featureWeights; }

    // Set color palette configuration
    void setColorPaletteConfig(const ColorPaletteConfig& config);
    const ColorPaletteConfig& getColorPaletteConfig() const { return m_paletteConfig; }

    // Set target cluster count range (system will auto-tune threshold)
    void setTargetClusterRange(int minClusters, int maxClusters);

    // =========================================================================
    // READ-ONLY QUERIES (for Agent 1 and Agent 8)
    // =========================================================================

    // Get cluster color for a species (primary API for other agents)
    glm::vec3 getClusterColor(SpeciesId speciesId) const;

    // Get light/dark color variants for UI
    glm::vec3 getClusterColorLight(SpeciesId speciesId) const;
    glm::vec3 getClusterColorDark(SpeciesId speciesId) const;

    // Get cluster ID for a species (-1 if not found)
    int getClusterId(SpeciesId speciesId) const;

    // Get all species in same cluster
    std::vector<SpeciesId> getClusterMembers(int clusterId) const;
    std::vector<SpeciesId> getRelatedSpecies(SpeciesId speciesId) const;

    // Get similarity score between two species (0 = identical, 1 = different)
    float getSimilarity(SpeciesId sp1, SpeciesId sp2) const;

    // Get feature vector for a species
    const SpeciesFeatureVector* getFeatureVector(SpeciesId speciesId) const;

    // =========================================================================
    // CLUSTER DATA ACCESS
    // =========================================================================

    // Get all clusters
    const std::vector<SimilarityCluster>& getClusters() const { return m_clusters; }

    // Get cluster by ID
    const SimilarityCluster* getCluster(int clusterId) const;

    // Get number of clusters
    int getClusterCount() const { return static_cast<int>(m_clusters.size()); }

    // =========================================================================
    // METRICS & DEBUGGING
    // =========================================================================

    // Get clustering quality metrics
    const ClusteringMetrics& getMetrics() const { return m_metrics; }

    // Get last update generation
    int getLastUpdateGeneration() const { return m_lastUpdateGeneration; }

    // Enable/disable debug logging
    void setDebugLogging(bool enabled) { m_debugLogging = enabled; }

private:
    // =========================================================================
    // FEATURE EXTRACTION
    // =========================================================================

    // Build feature vector from species data
    SpeciesFeatureVector extractFeatures(const Species* species) const;

    // Normalize features across all species (min-max normalization)
    void normalizeFeatures(std::vector<std::pair<SpeciesId, SpeciesFeatureVector>>& features);

    // =========================================================================
    // CLUSTERING ALGORITHMS
    // =========================================================================

    // UPGMA hierarchical clustering
    void clusterUPGMA(const std::vector<std::pair<SpeciesId, SpeciesFeatureVector>>& features);

    // K-medoids clustering as alternative
    void clusterKMedoids(const std::vector<std::pair<SpeciesId, SpeciesFeatureVector>>& features,
                         int targetK);

    // Build distance matrix for clustering
    std::vector<std::vector<float>> buildDistanceMatrix(
        const std::vector<std::pair<SpeciesId, SpeciesFeatureVector>>& features) const;

    // Cut dendrogram at threshold to form clusters
    void cutDendrogramAtThreshold(float threshold);

    // =========================================================================
    // COLOR GENERATION
    // =========================================================================

    // Generate deterministic color palette for clusters
    void generateClusterColors();

    // Convert HCL to RGB (perceptually uniform color space)
    glm::vec3 hclToRgb(float h, float c, float l) const;

    // Convert HSL to RGB
    glm::vec3 hslToRgb(float h, float s, float l) const;

    // =========================================================================
    // VALIDATION & METRICS
    // =========================================================================

    // Compute clustering quality metrics
    void computeMetrics();

    // Auto-tune threshold to achieve target cluster count
    void autoTuneThreshold(const std::vector<std::pair<SpeciesId, SpeciesFeatureVector>>& features);

    // =========================================================================
    // INTERNAL STATE
    // =========================================================================

    // Clusters
    std::vector<SimilarityCluster> m_clusters;

    // Maps species to cluster
    std::map<SpeciesId, int> m_speciesToCluster;

    // Feature vectors for all species
    std::map<SpeciesId, SpeciesFeatureVector> m_featureVectors;

    // Pairwise similarity cache
    mutable std::map<std::pair<SpeciesId, SpeciesId>, float> m_similarityCache;

    // Configuration
    float m_clusterThreshold;       // Distance threshold for clustering
    std::vector<float> m_featureWeights;
    ColorPaletteConfig m_paletteConfig;
    int m_targetMinClusters;
    int m_targetMaxClusters;

    // State tracking
    uint64_t m_planetSeed;
    int m_lastUpdateGeneration;
    int m_lastSpeciesCount;
    bool m_needsRecompute;
    bool m_debugLogging;

    // Metrics
    ClusteringMetrics m_metrics;

    // Dendrogram storage for UPGMA
    struct DendrogramNode {
        int leftChild;              // Index or -speciesId if leaf
        int rightChild;
        float mergeDistance;
        std::vector<SpeciesId> members;
    };
    std::vector<DendrogramNode> m_dendrogram;
};

} // namespace genetics
