#include "SpeciesSimilarity.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <iostream>
#include <random>

namespace genetics {

// =============================================================================
// SPECIES FEATURE VECTOR IMPLEMENTATION
// =============================================================================

float SpeciesFeatureVector::distanceTo(const SpeciesFeatureVector& other,
                                        const std::vector<float>& weights) const {
    // Weighted Euclidean distance
    float sumSq = 0.0f;
    float totalWeight = 0.0f;

    // Feature array for iteration
    const float features1[] = {
        normalizedSize, normalizedSpeed, dietSpecialization,
        habitatPreference, activityTime, genomicComplexity,
        heterozygosity, fitness
    };
    const float features2[] = {
        other.normalizedSize, other.normalizedSpeed, other.dietSpecialization,
        other.habitatPreference, other.activityTime, other.genomicComplexity,
        other.heterozygosity, other.fitness
    };

    for (size_t i = 0; i < 8 && i < weights.size(); ++i) {
        float diff = features1[i] - features2[i];
        sumSq += weights[i] * diff * diff;
        totalWeight += weights[i];
    }

    if (totalWeight > 0) {
        sumSq /= totalWeight;  // Normalize by total weight
    }

    return std::sqrt(sumSq);
}

std::vector<float> SpeciesFeatureVector::getDefaultWeights() {
    // Weights: size, speed, diet, habitat, activity, genomic, heterozygosity, fitness
    // Higher weight on niche traits (diet, habitat, activity) = 0.4 total
    // Moderate weight on genomic/genetic = 0.3 total
    // Lower weight on physical traits = 0.3 total
    return {
        0.15f,  // normalizedSize
        0.15f,  // normalizedSpeed
        0.15f,  // dietSpecialization
        0.15f,  // habitatPreference
        0.10f,  // activityTime
        0.10f,  // genomicComplexity
        0.10f,  // heterozygosity
        0.10f   // fitness
    };
}

// =============================================================================
// SPECIES SIMILARITY SYSTEM IMPLEMENTATION
// =============================================================================

SpeciesSimilaritySystem::SpeciesSimilaritySystem()
    : m_clusterThreshold(0.30f)
    , m_featureWeights(SpeciesFeatureVector::getDefaultWeights())
    , m_targetMinClusters(6)
    , m_targetMaxClusters(15)
    , m_planetSeed(42)
    , m_lastUpdateGeneration(-1)
    , m_lastSpeciesCount(0)
    , m_needsRecompute(true)
    , m_debugLogging(false) {
}

SpeciesSimilaritySystem::~SpeciesSimilaritySystem() = default;

void SpeciesSimilaritySystem::initialize(uint64_t planetSeed) {
    m_planetSeed = planetSeed;
    m_paletteConfig.shuffleSeed = planetSeed;
    m_needsRecompute = true;
}

void SpeciesSimilaritySystem::update(const SpeciationTracker& tracker, int currentGeneration) {
    auto startTime = std::chrono::high_resolution_clock::now();

    // Check if recomputation needed
    int currentSpeciesCount = tracker.getTotalSpeciesCount();
    bool speciesChanged = (currentSpeciesCount != m_lastSpeciesCount);

    if (!m_needsRecompute && !speciesChanged) {
        return;  // No changes, skip update
    }

    // Extract features for all species
    std::vector<std::pair<SpeciesId, SpeciesFeatureVector>> features;

    auto activeSpecies = tracker.getActiveSpecies();
    for (const Species* sp : activeSpecies) {
        features.emplace_back(sp->getId(), extractFeatures(sp));
    }

    if (features.empty()) {
        m_clusters.clear();
        m_speciesToCluster.clear();
        m_featureVectors.clear();
        m_needsRecompute = false;
        return;
    }

    // Normalize features
    normalizeFeatures(features);

    // Store normalized features
    m_featureVectors.clear();
    for (const auto& [id, fv] : features) {
        m_featureVectors[id] = fv;
    }

    // Auto-tune threshold if needed
    if (speciesChanged && features.size() > 5) {
        autoTuneThreshold(features);
    }

    // Perform UPGMA clustering
    clusterUPGMA(features);

    // Generate colors for clusters
    generateClusterColors();

    // Compute quality metrics
    computeMetrics();

    // Update state
    m_lastUpdateGeneration = currentGeneration;
    m_lastSpeciesCount = currentSpeciesCount;
    m_needsRecompute = false;

    // Record timing
    auto endTime = std::chrono::high_resolution_clock::now();
    m_metrics.computeTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    m_metrics.recomputeCount++;

    if (m_debugLogging) {
        std::cout << "[SpeciesSimilarity] Updated: " << m_clusters.size()
                  << " clusters from " << features.size() << " species"
                  << " (threshold=" << m_clusterThreshold
                  << ", time=" << m_metrics.computeTimeMs << "ms)" << std::endl;
    }
}

void SpeciesSimilaritySystem::forceRecompute() {
    m_needsRecompute = true;
}

void SpeciesSimilaritySystem::setClusterThreshold(float threshold) {
    m_clusterThreshold = std::clamp(threshold, 0.1f, 0.8f);
    m_needsRecompute = true;
}

void SpeciesSimilaritySystem::setFeatureWeights(const std::vector<float>& weights) {
    if (weights.size() >= 8) {
        m_featureWeights = weights;
        m_needsRecompute = true;
        m_similarityCache.clear();
    }
}

void SpeciesSimilaritySystem::setColorPaletteConfig(const ColorPaletteConfig& config) {
    m_paletteConfig = config;
    generateClusterColors();  // Regenerate colors
}

void SpeciesSimilaritySystem::setTargetClusterRange(int minClusters, int maxClusters) {
    m_targetMinClusters = std::max(2, minClusters);
    m_targetMaxClusters = std::max(m_targetMinClusters + 1, maxClusters);
    m_needsRecompute = true;
}

// =============================================================================
// READ-ONLY QUERIES
// =============================================================================

glm::vec3 SpeciesSimilaritySystem::getClusterColor(SpeciesId speciesId) const {
    auto it = m_speciesToCluster.find(speciesId);
    if (it == m_speciesToCluster.end()) {
        return glm::vec3(0.7f);  // Default gray for unknown
    }

    int clusterId = it->second;
    if (clusterId >= 0 && clusterId < static_cast<int>(m_clusters.size())) {
        return m_clusters[clusterId].baseColor;
    }

    return glm::vec3(0.7f);
}

glm::vec3 SpeciesSimilaritySystem::getClusterColorLight(SpeciesId speciesId) const {
    auto it = m_speciesToCluster.find(speciesId);
    if (it == m_speciesToCluster.end()) {
        return glm::vec3(0.85f);
    }

    int clusterId = it->second;
    if (clusterId >= 0 && clusterId < static_cast<int>(m_clusters.size())) {
        return m_clusters[clusterId].lightVariant;
    }

    return glm::vec3(0.85f);
}

glm::vec3 SpeciesSimilaritySystem::getClusterColorDark(SpeciesId speciesId) const {
    auto it = m_speciesToCluster.find(speciesId);
    if (it == m_speciesToCluster.end()) {
        return glm::vec3(0.4f);
    }

    int clusterId = it->second;
    if (clusterId >= 0 && clusterId < static_cast<int>(m_clusters.size())) {
        return m_clusters[clusterId].darkVariant;
    }

    return glm::vec3(0.4f);
}

int SpeciesSimilaritySystem::getClusterId(SpeciesId speciesId) const {
    auto it = m_speciesToCluster.find(speciesId);
    return (it != m_speciesToCluster.end()) ? it->second : -1;
}

std::vector<SpeciesId> SpeciesSimilaritySystem::getClusterMembers(int clusterId) const {
    if (clusterId >= 0 && clusterId < static_cast<int>(m_clusters.size())) {
        return m_clusters[clusterId].members;
    }
    return {};
}

std::vector<SpeciesId> SpeciesSimilaritySystem::getRelatedSpecies(SpeciesId speciesId) const {
    int clusterId = getClusterId(speciesId);
    if (clusterId < 0) return {};

    auto members = getClusterMembers(clusterId);
    // Remove the queried species from the result
    members.erase(std::remove(members.begin(), members.end(), speciesId), members.end());
    return members;
}

float SpeciesSimilaritySystem::getSimilarity(SpeciesId sp1, SpeciesId sp2) const {
    if (sp1 == sp2) return 0.0f;

    // Normalize order for cache lookup
    if (sp1 > sp2) std::swap(sp1, sp2);
    auto key = std::make_pair(sp1, sp2);

    // Check cache
    auto it = m_similarityCache.find(key);
    if (it != m_similarityCache.end()) {
        return it->second;
    }

    // Compute similarity
    auto fv1It = m_featureVectors.find(sp1);
    auto fv2It = m_featureVectors.find(sp2);

    if (fv1It == m_featureVectors.end() || fv2It == m_featureVectors.end()) {
        return 1.0f;  // Maximum distance for unknown
    }

    float dist = fv1It->second.distanceTo(fv2It->second, m_featureWeights);

    // Cache result
    m_similarityCache[key] = dist;

    return dist;
}

const SpeciesFeatureVector* SpeciesSimilaritySystem::getFeatureVector(SpeciesId speciesId) const {
    auto it = m_featureVectors.find(speciesId);
    return (it != m_featureVectors.end()) ? &it->second : nullptr;
}

const SimilarityCluster* SpeciesSimilaritySystem::getCluster(int clusterId) const {
    if (clusterId >= 0 && clusterId < static_cast<int>(m_clusters.size())) {
        return &m_clusters[clusterId];
    }
    return nullptr;
}

// =============================================================================
// FEATURE EXTRACTION
// =============================================================================

SpeciesFeatureVector SpeciesSimilaritySystem::extractFeatures(const Species* species) const {
    SpeciesFeatureVector fv;

    const auto& stats = species->getStats();
    const auto& niche = species->getNiche();

    // Population size (will be normalized later)
    fv.normalizedSize = static_cast<float>(stats.size);

    // Speed - estimate from genome traits or use default
    // For now use a proxy based on heterozygosity (active species tend to be more diverse)
    fv.normalizedSpeed = 0.5f;  // Will be replaced if we have access to average speed

    // Ecological niche traits (already in [0, 1])
    fv.dietSpecialization = niche.dietSpecialization;
    fv.habitatPreference = niche.habitatPreference;
    fv.activityTime = niche.activityTime;

    // Genomic complexity proxy
    // Use heterozygosity as a stand-in for genomic distance
    fv.genomicComplexity = stats.averageHeterozygosity;

    // Direct genetic measures
    fv.heterozygosity = stats.averageHeterozygosity;
    fv.fitness = stats.averageFitness;

    return fv;
}

void SpeciesSimilaritySystem::normalizeFeatures(
    std::vector<std::pair<SpeciesId, SpeciesFeatureVector>>& features) {

    if (features.empty()) return;

    // Find min/max for each feature
    float minSize = std::numeric_limits<float>::max();
    float maxSize = std::numeric_limits<float>::lowest();
    float minSpeed = std::numeric_limits<float>::max();
    float maxSpeed = std::numeric_limits<float>::lowest();
    float minHet = std::numeric_limits<float>::max();
    float maxHet = std::numeric_limits<float>::lowest();
    float minFit = std::numeric_limits<float>::max();
    float maxFit = std::numeric_limits<float>::lowest();

    for (const auto& [id, fv] : features) {
        // Log scale for size
        float logSize = std::log1p(fv.normalizedSize);
        minSize = std::min(minSize, logSize);
        maxSize = std::max(maxSize, logSize);

        minSpeed = std::min(minSpeed, fv.normalizedSpeed);
        maxSpeed = std::max(maxSpeed, fv.normalizedSpeed);
        minHet = std::min(minHet, fv.heterozygosity);
        maxHet = std::max(maxHet, fv.heterozygosity);
        minFit = std::min(minFit, fv.fitness);
        maxFit = std::max(maxFit, fv.fitness);
    }

    // Normalize each feature to [0, 1]
    auto normalize = [](float val, float minVal, float maxVal) -> float {
        if (maxVal <= minVal) return 0.5f;
        return (val - minVal) / (maxVal - minVal);
    };

    for (auto& [id, fv] : features) {
        fv.normalizedSize = normalize(std::log1p(fv.normalizedSize), minSize, maxSize);
        fv.normalizedSpeed = normalize(fv.normalizedSpeed, minSpeed, maxSpeed);
        fv.genomicComplexity = normalize(fv.genomicComplexity, 0.0f, 1.0f);
        fv.heterozygosity = normalize(fv.heterozygosity, minHet, maxHet);
        fv.fitness = normalize(fv.fitness, minFit, maxFit);
        // Niche traits are already [0, 1]
    }
}

// =============================================================================
// CLUSTERING ALGORITHMS
// =============================================================================

std::vector<std::vector<float>> SpeciesSimilaritySystem::buildDistanceMatrix(
    const std::vector<std::pair<SpeciesId, SpeciesFeatureVector>>& features) const {

    size_t n = features.size();
    std::vector<std::vector<float>> matrix(n, std::vector<float>(n, 0.0f));

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            float dist = features[i].second.distanceTo(features[j].second, m_featureWeights);
            matrix[i][j] = dist;
            matrix[j][i] = dist;
        }
    }

    return matrix;
}

void SpeciesSimilaritySystem::clusterUPGMA(
    const std::vector<std::pair<SpeciesId, SpeciesFeatureVector>>& features) {

    m_clusters.clear();
    m_speciesToCluster.clear();
    m_dendrogram.clear();

    size_t n = features.size();
    if (n == 0) return;
    if (n == 1) {
        // Single species = single cluster
        SimilarityCluster cluster;
        cluster.clusterId = 0;
        cluster.members.push_back(features[0].first);
        cluster.centroid = features[0].second;
        m_clusters.push_back(cluster);
        m_speciesToCluster[features[0].first] = 0;
        return;
    }

    // Build initial distance matrix
    auto distMatrix = buildDistanceMatrix(features);

    // Track active clusters (negative index = original species)
    std::vector<int> activeIndices;
    std::vector<std::vector<SpeciesId>> clusterMembers(n);
    std::vector<SpeciesFeatureVector> clusterCentroids(n);

    for (size_t i = 0; i < n; ++i) {
        activeIndices.push_back(static_cast<int>(i));
        clusterMembers[i].push_back(features[i].first);
        clusterCentroids[i] = features[i].second;
    }

    // UPGMA merging
    while (activeIndices.size() > 1) {
        // Find minimum distance pair
        float minDist = std::numeric_limits<float>::max();
        int minI = -1, minJ = -1;

        for (size_t i = 0; i < activeIndices.size(); ++i) {
            for (size_t j = i + 1; j < activeIndices.size(); ++j) {
                int ci = activeIndices[i];
                int cj = activeIndices[j];
                if (distMatrix[ci][cj] < minDist) {
                    minDist = distMatrix[ci][cj];
                    minI = static_cast<int>(i);
                    minJ = static_cast<int>(j);
                }
            }
        }

        if (minI < 0 || minJ < 0) break;

        int ci = activeIndices[minI];
        int cj = activeIndices[minJ];

        // Record dendrogram node
        DendrogramNode node;
        node.leftChild = ci;
        node.rightChild = cj;
        node.mergeDistance = minDist;
        node.members = clusterMembers[ci];
        node.members.insert(node.members.end(),
                            clusterMembers[cj].begin(),
                            clusterMembers[cj].end());
        m_dendrogram.push_back(node);

        // Create new cluster at index n + dendrogramIndex
        int newIdx = static_cast<int>(n + m_dendrogram.size() - 1);

        // Resize matrices if needed
        size_t newSize = newIdx + 1;
        if (distMatrix.size() < newSize) {
            distMatrix.resize(newSize);
            for (auto& row : distMatrix) row.resize(newSize, 0.0f);
            clusterMembers.resize(newSize);
            clusterCentroids.resize(newSize);
        }

        // Compute UPGMA distances to new cluster
        size_t sizeI = clusterMembers[ci].size();
        size_t sizeJ = clusterMembers[cj].size();
        size_t sizeNew = sizeI + sizeJ;

        for (int k : activeIndices) {
            if (k == ci || k == cj) continue;
            float dNew = (distMatrix[ci][k] * sizeI + distMatrix[cj][k] * sizeJ) / sizeNew;
            distMatrix[newIdx][k] = dNew;
            distMatrix[k][newIdx] = dNew;
        }

        // Store merged cluster data
        clusterMembers[newIdx] = node.members;

        // Compute centroid as average of member features
        SpeciesFeatureVector avgCentroid;
        for (SpeciesId memberId : node.members) {
            auto it = std::find_if(features.begin(), features.end(),
                                   [memberId](const auto& p) { return p.first == memberId; });
            if (it != features.end()) {
                avgCentroid.normalizedSize += it->second.normalizedSize;
                avgCentroid.normalizedSpeed += it->second.normalizedSpeed;
                avgCentroid.dietSpecialization += it->second.dietSpecialization;
                avgCentroid.habitatPreference += it->second.habitatPreference;
                avgCentroid.activityTime += it->second.activityTime;
                avgCentroid.genomicComplexity += it->second.genomicComplexity;
                avgCentroid.heterozygosity += it->second.heterozygosity;
                avgCentroid.fitness += it->second.fitness;
            }
        }
        float memberCount = static_cast<float>(node.members.size());
        avgCentroid.normalizedSize /= memberCount;
        avgCentroid.normalizedSpeed /= memberCount;
        avgCentroid.dietSpecialization /= memberCount;
        avgCentroid.habitatPreference /= memberCount;
        avgCentroid.activityTime /= memberCount;
        avgCentroid.genomicComplexity /= memberCount;
        avgCentroid.heterozygosity /= memberCount;
        avgCentroid.fitness /= memberCount;
        clusterCentroids[newIdx] = avgCentroid;

        // Update active indices
        activeIndices.erase(activeIndices.begin() + minJ);  // Remove higher index first
        activeIndices.erase(activeIndices.begin() + minI);
        activeIndices.push_back(newIdx);
    }

    // Cut dendrogram at threshold
    cutDendrogramAtThreshold(m_clusterThreshold);
}

void SpeciesSimilaritySystem::cutDendrogramAtThreshold(float threshold) {
    if (m_dendrogram.empty()) return;

    // Find clusters by walking up the dendrogram
    // Each species starts in its own cluster, merges with others if distance < threshold

    // Start from leaves and track which clusters are merged below threshold
    std::map<SpeciesId, int> speciesClusterMap;
    int nextClusterId = 0;

    // Process dendrogram from first (smallest distance) to last
    // Build a union-find structure essentially
    std::vector<int> clusterAssignment(m_dendrogram.size() + m_featureVectors.size(), -1);

    // Initially, each species gets a tentative cluster
    int speciesIdx = 0;
    for (const auto& [spId, fv] : m_featureVectors) {
        clusterAssignment[speciesIdx] = speciesIdx;  // Initially each in own cluster
        speciesClusterMap[spId] = speciesIdx;
        speciesIdx++;
    }

    // Process merges
    size_t baseOffset = m_featureVectors.size();
    for (size_t i = 0; i < m_dendrogram.size(); ++i) {
        const auto& node = m_dendrogram[i];

        if (node.mergeDistance <= threshold) {
            // Merge: assign all members to same cluster
            int targetCluster = -1;

            // Find existing cluster assignment or create new
            for (SpeciesId memberId : node.members) {
                int assigned = speciesClusterMap[memberId];
                if (targetCluster < 0) {
                    targetCluster = assigned;
                } else {
                    // Re-assign all with old cluster to target
                    for (auto& [sid, cid] : speciesClusterMap) {
                        if (cid == assigned) cid = targetCluster;
                    }
                }
            }
        }
    }

    // Compact cluster IDs
    std::map<int, int> clusterIdRemap;
    int finalClusterId = 0;
    for (auto& [spId, cid] : speciesClusterMap) {
        if (clusterIdRemap.find(cid) == clusterIdRemap.end()) {
            clusterIdRemap[cid] = finalClusterId++;
        }
        speciesClusterMap[spId] = clusterIdRemap[cid];
    }

    // Build final clusters
    m_clusters.resize(finalClusterId);
    m_speciesToCluster.clear();

    for (int i = 0; i < finalClusterId; ++i) {
        m_clusters[i].clusterId = i;
        m_clusters[i].members.clear();
    }

    for (const auto& [spId, cid] : speciesClusterMap) {
        m_clusters[cid].members.push_back(spId);
        m_speciesToCluster[spId] = cid;
    }

    // Compute cluster centroids and intra-cluster distances
    for (auto& cluster : m_clusters) {
        if (cluster.members.empty()) continue;

        // Centroid
        SpeciesFeatureVector centroid;
        for (SpeciesId memberId : cluster.members) {
            auto it = m_featureVectors.find(memberId);
            if (it != m_featureVectors.end()) {
                centroid.normalizedSize += it->second.normalizedSize;
                centroid.normalizedSpeed += it->second.normalizedSpeed;
                centroid.dietSpecialization += it->second.dietSpecialization;
                centroid.habitatPreference += it->second.habitatPreference;
                centroid.activityTime += it->second.activityTime;
                centroid.genomicComplexity += it->second.genomicComplexity;
                centroid.heterozygosity += it->second.heterozygosity;
                centroid.fitness += it->second.fitness;
            }
        }
        float memberCount = static_cast<float>(cluster.members.size());
        centroid.normalizedSize /= memberCount;
        centroid.normalizedSpeed /= memberCount;
        centroid.dietSpecialization /= memberCount;
        centroid.habitatPreference /= memberCount;
        centroid.activityTime /= memberCount;
        centroid.genomicComplexity /= memberCount;
        centroid.heterozygosity /= memberCount;
        centroid.fitness /= memberCount;
        cluster.centroid = centroid;

        // Intra-cluster distance (average pairwise distance)
        if (cluster.members.size() > 1) {
            float totalDist = 0.0f;
            int pairCount = 0;
            for (size_t i = 0; i < cluster.members.size(); ++i) {
                for (size_t j = i + 1; j < cluster.members.size(); ++j) {
                    totalDist += getSimilarity(cluster.members[i], cluster.members[j]);
                    pairCount++;
                }
            }
            cluster.intraClusterDistance = (pairCount > 0) ? totalDist / pairCount : 0.0f;
        }
    }
}

void SpeciesSimilaritySystem::autoTuneThreshold(
    const std::vector<std::pair<SpeciesId, SpeciesFeatureVector>>& features) {

    if (features.size() < 2) return;

    // Binary search for threshold that gives target cluster count
    float lowThreshold = 0.1f;
    float highThreshold = 0.8f;
    float bestThreshold = m_clusterThreshold;
    int bestClusterCount = -1;

    for (int iter = 0; iter < 10; ++iter) {
        float midThreshold = (lowThreshold + highThreshold) / 2.0f;

        // Temporarily set threshold and cluster
        float savedThreshold = m_clusterThreshold;
        m_clusterThreshold = midThreshold;
        clusterUPGMA(features);

        int clusterCount = static_cast<int>(m_clusters.size());

        if (clusterCount >= m_targetMinClusters && clusterCount <= m_targetMaxClusters) {
            bestThreshold = midThreshold;
            bestClusterCount = clusterCount;
            break;  // Found good threshold
        }

        if (clusterCount < m_targetMinClusters) {
            // Too few clusters, decrease threshold (tighter clustering)
            highThreshold = midThreshold;
        } else {
            // Too many clusters, increase threshold (looser clustering)
            lowThreshold = midThreshold;
        }

        if (bestClusterCount < 0 ||
            std::abs(clusterCount - (m_targetMinClusters + m_targetMaxClusters) / 2) <
            std::abs(bestClusterCount - (m_targetMinClusters + m_targetMaxClusters) / 2)) {
            bestThreshold = midThreshold;
            bestClusterCount = clusterCount;
        }

        m_clusterThreshold = savedThreshold;
    }

    m_clusterThreshold = bestThreshold;

    if (m_debugLogging) {
        std::cout << "[SpeciesSimilarity] Auto-tuned threshold to " << m_clusterThreshold
                  << " -> " << bestClusterCount << " clusters" << std::endl;
    }
}

// =============================================================================
// COLOR GENERATION
// =============================================================================

void SpeciesSimilaritySystem::generateClusterColors() {
    if (m_clusters.empty()) return;

    int n = static_cast<int>(m_clusters.size());

    // Generate evenly-spaced hues
    std::vector<float> hues(n);
    float hueStep = 360.0f / n;

    for (int i = 0; i < n; ++i) {
        hues[i] = fmod(i * hueStep, 360.0f);
    }

    // Shuffle hues deterministically based on planet seed
    std::mt19937 rng(static_cast<unsigned>(m_paletteConfig.shuffleSeed));
    std::shuffle(hues.begin(), hues.end(), rng);

    // Assign colors to clusters
    for (int i = 0; i < n; ++i) {
        float h = hues[i] / 360.0f;  // Normalize to [0, 1]
        float s = m_paletteConfig.baseSaturation;
        float l = m_paletteConfig.baseLightness;

        m_clusters[i].baseColor = hslToRgb(h, s, l);
        m_clusters[i].lightVariant = hslToRgb(h, s * 0.8f, m_paletteConfig.lightVariantLightness);
        m_clusters[i].darkVariant = hslToRgb(h, s * 0.9f, m_paletteConfig.darkVariantLightness);
    }
}

glm::vec3 SpeciesSimilaritySystem::hslToRgb(float h, float s, float l) const {
    // HSL to RGB conversion
    float c = (1.0f - std::abs(2.0f * l - 1.0f)) * s;
    float x = c * (1.0f - std::abs(fmod(h * 6.0f, 2.0f) - 1.0f));
    float m = l - c / 2.0f;

    glm::vec3 rgb;

    if (h < 1.0f/6.0f)      rgb = glm::vec3(c, x, 0);
    else if (h < 2.0f/6.0f) rgb = glm::vec3(x, c, 0);
    else if (h < 3.0f/6.0f) rgb = glm::vec3(0, c, x);
    else if (h < 4.0f/6.0f) rgb = glm::vec3(0, x, c);
    else if (h < 5.0f/6.0f) rgb = glm::vec3(x, 0, c);
    else                    rgb = glm::vec3(c, 0, x);

    return glm::clamp(rgb + m, 0.0f, 1.0f);
}

glm::vec3 SpeciesSimilaritySystem::hclToRgb(float h, float c, float l) const {
    // HCL (Hue-Chroma-Luminance) to RGB via Lab color space
    // Simplified implementation using HSL as approximation
    float saturation = c / (l < 0.5f ? (2.0f * l) : (2.0f - 2.0f * l));
    saturation = std::clamp(saturation, 0.0f, 1.0f);
    return hslToRgb(h, saturation, l);
}

// =============================================================================
// METRICS
// =============================================================================

void SpeciesSimilaritySystem::computeMetrics() {
    m_metrics.clusterCount = static_cast<int>(m_clusters.size());
    m_metrics.speciesCount = static_cast<int>(m_featureVectors.size());

    if (m_clusters.empty()) {
        m_metrics.averageClusterSize = 0;
        m_metrics.averageIntraDistance = 0;
        m_metrics.averageInterDistance = 0;
        m_metrics.silhouetteScore = 0;
        return;
    }

    // Average cluster size
    float totalSize = 0;
    for (const auto& cluster : m_clusters) {
        totalSize += cluster.members.size();
    }
    m_metrics.averageClusterSize = totalSize / m_clusters.size();

    // Average intra-cluster distance
    float totalIntra = 0;
    for (const auto& cluster : m_clusters) {
        totalIntra += cluster.intraClusterDistance;
    }
    m_metrics.averageIntraDistance = totalIntra / m_clusters.size();

    // Average inter-cluster distance (between centroids)
    float totalInter = 0;
    int interPairs = 0;
    for (size_t i = 0; i < m_clusters.size(); ++i) {
        for (size_t j = i + 1; j < m_clusters.size(); ++j) {
            totalInter += m_clusters[i].centroid.distanceTo(
                m_clusters[j].centroid, m_featureWeights);
            interPairs++;
        }
    }
    m_metrics.averageInterDistance = (interPairs > 0) ? totalInter / interPairs : 0;

    // Simplified silhouette score
    // (inter - intra) / max(inter, intra)
    float maxDist = std::max(m_metrics.averageInterDistance, m_metrics.averageIntraDistance);
    if (maxDist > 0) {
        m_metrics.silhouetteScore = (m_metrics.averageInterDistance - m_metrics.averageIntraDistance) / maxDist;
    } else {
        m_metrics.silhouetteScore = 0;
    }
}

} // namespace genetics
