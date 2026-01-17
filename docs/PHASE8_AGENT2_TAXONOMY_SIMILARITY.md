# Phase 8: Species Similarity and Taxonomy System

## Overview

This document describes the species similarity clustering system that provides color-coding by similarity clusters and an enhanced lineage viewer. The system groups species into clusters based on multi-dimensional feature similarity, enabling visual identification of related species and ecological groupings.

## Architecture

### Components

1. **SpeciesSimilaritySystem** (`src/entities/genetics/SpeciesSimilarity.h/cpp`)
   - Main clustering engine
   - Feature extraction and normalization
   - UPGMA hierarchical clustering
   - Cluster color generation

2. **PhylogeneticTreeVisualizer** (`src/ui/PhylogeneticTreeVisualizer.h/cpp`)
   - Enhanced tree rendering with cluster colors
   - Filter controls for cluster, name, extinction status
   - Multiple color modes (Species ID, Cluster, Fitness, Age)
   - Interactive zoom/pan and tooltips

3. **SpeciesEvolutionPanel**
   - Similarity metrics display
   - Cluster legend and filter UI
   - Integration point for the similarity system

## Similarity Metric Design

### Feature Vector

Each species is characterized by an 8-dimensional feature vector:

```cpp
struct SpeciesFeatureVector {
    float normalizedSize;        // Population size (log-scaled)
    float normalizedSpeed;       // Average movement speed
    float dietSpecialization;    // From ecological niche [0, 1]
    float habitatPreference;     // From ecological niche [0, 1]
    float activityTime;          // From ecological niche [0, 1]
    float genomicComplexity;     // Heterozygosity as proxy
    float heterozygosity;        // Genetic diversity
    float fitness;               // Average fitness
};
```

### Feature Weights

Default weights emphasize ecological niche traits while balancing genetic and physical characteristics:

| Feature | Weight | Category |
|---------|--------|----------|
| normalizedSize | 0.15 | Physical |
| normalizedSpeed | 0.15 | Physical |
| dietSpecialization | 0.15 | Niche |
| habitatPreference | 0.15 | Niche |
| activityTime | 0.10 | Niche |
| genomicComplexity | 0.10 | Genetic |
| heterozygosity | 0.10 | Genetic |
| fitness | 0.10 | Genetic |

**Total weights:** Physical = 0.30, Niche = 0.40, Genetic = 0.30

### Distance Formula

Weighted Euclidean distance between feature vectors:

```
d(a, b) = sqrt(Σ w_i * (a_i - b_i)²) / Σ w_i
```

Where `w_i` are the feature weights and features are normalized to [0, 1] using min-max normalization across all species.

### Normalization

Features are normalized per-update using the observed min/max values:

- **Population Size**: Log-transformed before normalization (`log1p(size)`)
- **Niche Traits**: Already in [0, 1] range, no transformation needed
- **Heterozygosity/Fitness**: Normalized using observed min/max

## Clustering Method

### UPGMA Hierarchical Clustering

The system uses **Unweighted Pair Group Method with Arithmetic Mean (UPGMA)**, a standard phylogenetic clustering algorithm:

1. **Initialize**: Each species starts as its own cluster
2. **Distance Matrix**: Compute pairwise distances between all species
3. **Iterative Merging**:
   - Find the pair of clusters with minimum distance
   - Merge them into a new cluster
   - Update distances using the UPGMA formula:
     ```
     d(new, k) = (|A| * d(A, k) + |B| * d(B, k)) / (|A| + |B|)
     ```
   - Repeat until all species are in one cluster
4. **Dendrogram Cutting**: Cut the resulting dendrogram at the threshold distance

### Threshold Configuration

- **Recommended Range**: 0.25 - 0.35 (normalized distance units)
- **Default**: 0.30
- **Target Clusters**: 6 - 15 clusters for typical simulations

The system auto-tunes the threshold via binary search to achieve the target cluster range when the species list changes.

### Auto-Tuning Algorithm

```cpp
float autoTuneThreshold() {
    for (10 iterations) {
        midThreshold = (low + high) / 2
        clusters = clusterAt(midThreshold)

        if (targetMin <= clusters.count <= targetMax) {
            return midThreshold  // Found good threshold
        }

        if (clusters.count < targetMin) {
            high = midThreshold  // Too few: decrease threshold
        } else {
            low = midThreshold   // Too many: increase threshold
        }
    }
    return bestThreshold
}
```

## Color Palette Generation

### HSL-Based Palette

Colors are generated using HSL color space for perceptual uniformity:

1. **Hue Distribution**: Evenly spaced hues across 360°
2. **Shuffling**: Deterministic shuffle based on planet seed
3. **Variants**:
   - **Base**: S=0.70, L=0.55 (vibrant, readable)
   - **Light**: S=0.56, L=0.75 (for light backgrounds)
   - **Dark**: S=0.63, L=0.35 (for dark backgrounds)

### Configuration

```cpp
struct ColorPaletteConfig {
    float minHueDelta = 35.0f;         // Minimum hue separation (degrees)
    float baseSaturation = 0.7f;       // Saturation for base colors
    float baseLightness = 0.55f;       // Lightness for base colors
    float lightVariantLightness = 0.75f;
    float darkVariantLightness = 0.35f;
    uint64_t shuffleSeed = 42;         // Planet seed for reproducibility
};
```

### Example Palette (8 clusters)

| Cluster | Hue | Base RGB | Description |
|---------|-----|----------|-------------|
| 0 | 0° | (0.89, 0.34, 0.34) | Red |
| 1 | 180° | (0.34, 0.89, 0.89) | Cyan |
| 2 | 90° | (0.61, 0.89, 0.34) | Lime |
| 3 | 270° | (0.61, 0.34, 0.89) | Purple |
| 4 | 45° | (0.89, 0.61, 0.34) | Orange |
| 5 | 225° | (0.34, 0.61, 0.89) | Sky Blue |
| 6 | 135° | (0.34, 0.89, 0.61) | Teal |
| 7 | 315° | (0.89, 0.34, 0.61) | Pink |

## UI Interaction Details

### Phylogenetic Tree Visualizer

#### Layout Modes
- **Vertical**: Root at top, descendants below (default)
- **Horizontal**: Root at left, descendants right
- **Radial**: Root at center, descendants radiate outward

#### Color Modes
1. **Species ID**: Original deterministic colors per species ID
2. **Cluster**: Species colored by similarity cluster (default)
3. **Fitness**: Green-to-red gradient based on divergence score
4. **Age**: Blue-to-yellow gradient based on founding generation

#### Navigation
- **Zoom**: Mouse scroll wheel (0.2x - 3.0x)
- **Pan**: Right-click drag
- **Select**: Left-click on node
- **Fit to View**: Button to auto-fit all nodes

### Filtering

The filter panel provides:

- **Extinction Status**: Show/hide extant and extinct species
- **Name Filter**: Case-insensitive substring search
- **Cluster Filter**: Show only species in selected cluster

Filtered nodes render as small, dimmed circles while maintaining tree structure.

### Tooltips

Enhanced tooltips show:
- Species name (colored)
- Species ID and population
- Founding generation
- Cluster ID and color
- Divergence score from cluster centroid
- Count of related species in same cluster
- Extinction status

### Cluster Legend

When in Cluster color mode:
- Shows up to 8 cluster colors with member counts
- Hover reveals full cluster info
- Colors match node coloring exactly

## Performance Notes

### Computational Complexity

- **Feature Extraction**: O(n) where n = species count
- **Distance Matrix**: O(n²) pairwise comparisons
- **UPGMA Clustering**: O(n² log n) standard implementation
- **Color Generation**: O(k) where k = cluster count

### Optimization Strategies

1. **Lazy Recomputation**: Only recompute when species list changes
2. **Similarity Caching**: Pairwise similarities cached between updates
3. **Auto-tuning Limits**: Maximum 10 iterations for threshold search

### Performance Targets

| Metric | Target | Typical |
|--------|--------|---------|
| Update Time | < 50ms | 5-20ms |
| Species Supported | 500+ | 50-200 |
| Cluster Count | 6-15 | 8-12 |

### Memory Usage

- Feature vectors: ~64 bytes per species
- Distance matrix: O(n²) temporary during clustering
- Cluster storage: O(k) where k = clusters

## Metrics and Debugging

### Clustering Quality Metrics

```cpp
struct ClusteringMetrics {
    int clusterCount;           // Number of clusters formed
    int speciesCount;           // Total species clustered
    float averageClusterSize;   // Mean species per cluster
    float averageIntraDistance; // Cohesion (lower = better)
    float averageInterDistance; // Separation (higher = better)
    float silhouetteScore;      // -1 to +1 (higher = better)
    double computeTimeMs;       // Clustering computation time
    int recomputeCount;         // Total recomputes since init
};
```

### Silhouette Score Interpretation

| Score Range | Quality | Description |
|-------------|---------|-------------|
| 0.71 - 1.00 | Excellent | Strong cluster structure |
| 0.51 - 0.70 | Good | Reasonable cluster structure |
| 0.26 - 0.50 | Fair | Clusters may be artificial |
| < 0.25 | Poor | No substantial structure |

### Debug Logging

Enable with:
```cpp
similarity.setDebugLogging(true);
```

Logs:
- Cluster count changes
- Threshold auto-tuning results
- Recomputation timing

## API Reference

### SpeciesSimilaritySystem

#### Initialization
```cpp
void initialize(uint64_t planetSeed);
void update(const SpeciationTracker& tracker, int currentGeneration);
void forceRecompute();
```

#### Configuration
```cpp
void setClusterThreshold(float threshold);       // 0.1 - 0.8
void setFeatureWeights(const std::vector<float>& weights);
void setColorPaletteConfig(const ColorPaletteConfig& config);
void setTargetClusterRange(int minClusters, int maxClusters);
```

#### Read-Only Queries (for Agent 1 and Agent 8)
```cpp
glm::vec3 getClusterColor(SpeciesId speciesId) const;
glm::vec3 getClusterColorLight(SpeciesId speciesId) const;
glm::vec3 getClusterColorDark(SpeciesId speciesId) const;
int getClusterId(SpeciesId speciesId) const;
std::vector<SpeciesId> getClusterMembers(int clusterId) const;
std::vector<SpeciesId> getRelatedSpecies(SpeciesId speciesId) const;
float getSimilarity(SpeciesId sp1, SpeciesId sp2) const;
const SpeciesFeatureVector* getFeatureVector(SpeciesId speciesId) const;
```

### PhylogeneticTreeVisualizer

#### Setup
```cpp
void buildFromTracker(const SpeciationTracker& tracker);
void setSimilaritySystem(const SpeciesSimilaritySystem* similarity);
```

#### View Configuration
```cpp
void setLayoutStyle(LayoutStyle style);   // VERTICAL, HORIZONTAL, RADIAL
void setColorMode(ColorMode mode);        // SPECIES_ID, CLUSTER, FITNESS, AGE
void setFilter(const TreeFilterOptions& filter);
void setZoom(float zoom);
void setPan(const glm::vec2& pan);
void fitToCanvas(const ImVec2& canvasSize);
```

## Integration Example

```cpp
// In simulation initialization
genetics::SpeciesSimilaritySystem similarity;
similarity.initialize(planetSeed);
similarity.setTargetClusterRange(6, 15);

// In main loop
similarity.update(speciationTracker, currentGeneration);

// In UI code
speciesPanel.setSimilaritySystem(&similarity);
speciesPanel.render();

// For nametag coloring (Agent 1 integration)
glm::vec3 nameColor = similarity.getClusterColor(creature.getSpeciesId());
```

## Owned Files

Files exclusively owned by this agent:
- `src/entities/genetics/SpeciesSimilarity.h`
- `src/entities/genetics/SpeciesSimilarity.cpp`
- `src/ui/PhylogeneticTreeVisualizer.h`
- `src/ui/PhylogeneticTreeVisualizer.cpp`

Files with read-only modifications (forward declarations):
- `src/entities/genetics/Species.h`

## Dependencies

Required headers:
- `<glm/glm.hpp>` - Vector math
- `<imgui.h>` - UI rendering
- `<vector>`, `<map>`, `<set>` - STL containers
- `<chrono>` - Performance timing

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | Phase 8 | Initial implementation with UPGMA clustering, HSL colors, filter UI |
