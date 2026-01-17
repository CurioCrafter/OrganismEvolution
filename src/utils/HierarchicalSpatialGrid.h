#pragma once

// HierarchicalSpatialGrid - Multi-level spatial partitioning for 10,000+ creatures
// Implements a two-level grid hierarchy for efficient queries at multiple scales

#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

class Creature;

namespace Forge {

// ============================================================================
// Configuration
// ============================================================================

struct HierarchicalGridConfig {
    // World bounds
    float worldWidth = 500.0f;
    float worldDepth = 500.0f;

    // Coarse level (for large queries, region culling)
    int coarseGridSize = 8;      // 8x8 = 64 coarse cells

    // Fine level (for precise queries)
    int fineGridSize = 32;       // 32x32 = 1024 fine cells

    // Max creatures per cell
    int maxPerCoarseCell = 2048;
    int maxPerFineCell = 64;
};

// ============================================================================
// Grid Statistics
// ============================================================================

struct GridStats {
    size_t totalCreatures = 0;
    size_t coarseCellsUsed = 0;
    size_t fineCellsUsed = 0;
    size_t maxCoarseOccupancy = 0;
    size_t maxFineOccupancy = 0;
    size_t queryCount = 0;
    float avgQueryTimeUs = 0.0f;
};

// ============================================================================
// Cell Templates
// ============================================================================

template<int MaxPerCell>
struct GridCell {
    std::array<Creature*, MaxPerCell> creatures;
    int count = 0;

    // Bounding info for quick rejection
    glm::vec3 minBounds = glm::vec3(FLT_MAX);
    glm::vec3 maxBounds = glm::vec3(-FLT_MAX);

    void clear() {
        count = 0;
        minBounds = glm::vec3(FLT_MAX);
        maxBounds = glm::vec3(-FLT_MAX);
    }

    bool add(Creature* c, const glm::vec3& pos) {
        if (count >= MaxPerCell) return false;
        creatures[count++] = c;

        // Update bounds
        minBounds = glm::min(minBounds, pos);
        maxBounds = glm::max(maxBounds, pos);
        return true;
    }

    bool isEmpty() const { return count == 0; }

    // Quick check if a sphere could possibly intersect this cell
    bool sphereIntersects(const glm::vec3& center, float radius) const {
        if (isEmpty()) return false;

        // Expand bounds by radius
        glm::vec3 expandedMin = minBounds - glm::vec3(radius);
        glm::vec3 expandedMax = maxBounds + glm::vec3(radius);

        return center.x >= expandedMin.x && center.x <= expandedMax.x &&
               center.z >= expandedMin.z && center.z <= expandedMax.z;
    }
};

// ============================================================================
// HierarchicalSpatialGrid
// ============================================================================

class HierarchicalSpatialGrid {
public:
    static constexpr int MAX_COARSE_CELL = 2048;
    static constexpr int MAX_FINE_CELL = 64;

    using CoarseCell = GridCell<MAX_COARSE_CELL>;
    using FineCell = GridCell<MAX_FINE_CELL>;

    HierarchicalSpatialGrid();
    explicit HierarchicalSpatialGrid(const HierarchicalGridConfig& config);
    ~HierarchicalSpatialGrid() = default;

    // ========================================================================
    // Configuration
    // ========================================================================

    void configure(const HierarchicalGridConfig& config);
    const HierarchicalGridConfig& getConfig() const { return m_config; }

    // ========================================================================
    // Grid Management
    // ========================================================================

    // Clear both levels
    void clear();

    // Insert creature into both levels
    void insert(Creature* creature);

    // Rebuild from creature list (more efficient than individual inserts)
    void rebuild(const std::vector<Creature*>& creatures);

    // ========================================================================
    // Queries
    // ========================================================================

    // Query all creatures within radius (uses hierarchical culling)
    const std::vector<Creature*>& query(const glm::vec3& position, float radius) const;

    // Query by type
    const std::vector<Creature*>& queryByType(const glm::vec3& position,
                                                float radius, int creatureType) const;

    // Find nearest creature
    Creature* findNearest(const glm::vec3& position, float maxRadius,
                          int typeFilter = -1) const;

    // Count without allocation
    int countNearby(const glm::vec3& position, float radius) const;

    // ========================================================================
    // Optimized Queries
    // ========================================================================

    // Query with early termination (stops when limit reached)
    const std::vector<Creature*>& queryWithLimit(const glm::vec3& position,
                                                   float radius, int maxResults) const;

    // Query sorted by distance (for nearest-K queries)
    const std::vector<Creature*>& queryKNearest(const glm::vec3& position,
                                                  float maxRadius, int k) const;

    // Ray query (for line-of-sight)
    const std::vector<Creature*>& queryRay(const glm::vec3& start,
                                             const glm::vec3& direction,
                                             float maxDistance) const;

    // ========================================================================
    // Statistics
    // ========================================================================

    const GridStats& getStats() const { return m_stats; }
    void resetStats() { m_stats = GridStats{}; }

private:
    HierarchicalGridConfig m_config;

    // Two-level grid structure
    std::vector<CoarseCell> m_coarseGrid;
    std::vector<FineCell> m_fineGrid;

    // Precomputed values
    float m_coarseCellWidth;
    float m_coarseCellDepth;
    float m_fineCellWidth;
    float m_fineCellDepth;
    float m_invCoarseCellWidth;
    float m_invCoarseCellDepth;
    float m_invFineCellWidth;
    float m_invFineCellDepth;
    float m_halfWorldWidth;
    float m_halfWorldDepth;

    // Query buffer (reused to avoid allocations)
    mutable std::vector<Creature*> m_queryBuffer;
    mutable std::vector<std::pair<Creature*, float>> m_distanceBuffer;

    // Statistics
    mutable GridStats m_stats;

    // Helper methods
    int worldToCoarseCell(float x, float z) const;
    int worldToFineCell(float x, float z) const;
    void worldToCoarseCellXZ(float x, float z, int& cellX, int& cellZ) const;
    void worldToFineCellXZ(float x, float z, int& cellX, int& cellZ) const;

    void getCellsInRadius(float x, float z, float radius,
                          int gridSize, float invCellWidth, float invCellDepth,
                          int& minX, int& maxX, int& minZ, int& maxZ) const;
};

// ============================================================================
// Inline Implementations
// ============================================================================

inline HierarchicalSpatialGrid::HierarchicalSpatialGrid() {
    configure(HierarchicalGridConfig{});
}

inline HierarchicalSpatialGrid::HierarchicalSpatialGrid(const HierarchicalGridConfig& config) {
    configure(config);
}

inline void HierarchicalSpatialGrid::configure(const HierarchicalGridConfig& config) {
    m_config = config;

    // Coarse grid
    m_coarseCellWidth = config.worldWidth / config.coarseGridSize;
    m_coarseCellDepth = config.worldDepth / config.coarseGridSize;
    m_invCoarseCellWidth = 1.0f / m_coarseCellWidth;
    m_invCoarseCellDepth = 1.0f / m_coarseCellDepth;
    m_coarseGrid.resize(config.coarseGridSize * config.coarseGridSize);

    // Fine grid
    m_fineCellWidth = config.worldWidth / config.fineGridSize;
    m_fineCellDepth = config.worldDepth / config.fineGridSize;
    m_invFineCellWidth = 1.0f / m_fineCellWidth;
    m_invFineCellDepth = 1.0f / m_fineCellDepth;
    m_fineGrid.resize(config.fineGridSize * config.fineGridSize);

    // Precomputed
    m_halfWorldWidth = config.worldWidth * 0.5f;
    m_halfWorldDepth = config.worldDepth * 0.5f;

    // Reserve query buffers
    m_queryBuffer.reserve(4096);
    m_distanceBuffer.reserve(1024);
}

inline void HierarchicalSpatialGrid::clear() {
    for (auto& cell : m_coarseGrid) cell.clear();
    for (auto& cell : m_fineGrid) cell.clear();
    m_stats.totalCreatures = 0;
    m_stats.coarseCellsUsed = 0;
    m_stats.fineCellsUsed = 0;
}

inline int HierarchicalSpatialGrid::worldToCoarseCell(float x, float z) const {
    float normX = (x + m_halfWorldWidth) * m_invCoarseCellWidth;
    float normZ = (z + m_halfWorldDepth) * m_invCoarseCellDepth;
    int cx = std::clamp(static_cast<int>(normX), 0, m_config.coarseGridSize - 1);
    int cz = std::clamp(static_cast<int>(normZ), 0, m_config.coarseGridSize - 1);
    return cz * m_config.coarseGridSize + cx;
}

inline int HierarchicalSpatialGrid::worldToFineCell(float x, float z) const {
    float normX = (x + m_halfWorldWidth) * m_invFineCellWidth;
    float normZ = (z + m_halfWorldDepth) * m_invFineCellDepth;
    int cx = std::clamp(static_cast<int>(normX), 0, m_config.fineGridSize - 1);
    int cz = std::clamp(static_cast<int>(normZ), 0, m_config.fineGridSize - 1);
    return cz * m_config.fineGridSize + cx;
}

inline void HierarchicalSpatialGrid::worldToCoarseCellXZ(float x, float z,
                                                          int& cellX, int& cellZ) const {
    float normX = (x + m_halfWorldWidth) * m_invCoarseCellWidth;
    float normZ = (z + m_halfWorldDepth) * m_invCoarseCellDepth;
    cellX = std::clamp(static_cast<int>(normX), 0, m_config.coarseGridSize - 1);
    cellZ = std::clamp(static_cast<int>(normZ), 0, m_config.coarseGridSize - 1);
}

inline void HierarchicalSpatialGrid::worldToFineCellXZ(float x, float z,
                                                         int& cellX, int& cellZ) const {
    float normX = (x + m_halfWorldWidth) * m_invFineCellWidth;
    float normZ = (z + m_halfWorldDepth) * m_invFineCellDepth;
    cellX = std::clamp(static_cast<int>(normX), 0, m_config.fineGridSize - 1);
    cellZ = std::clamp(static_cast<int>(normZ), 0, m_config.fineGridSize - 1);
}

inline void HierarchicalSpatialGrid::getCellsInRadius(float x, float z, float radius,
                                                        int gridSize,
                                                        float invCellWidth, float invCellDepth,
                                                        int& minX, int& maxX,
                                                        int& minZ, int& maxZ) const {
    float normMinX = (x - radius + m_halfWorldWidth) * invCellWidth;
    float normMaxX = (x + radius + m_halfWorldWidth) * invCellWidth;
    float normMinZ = (z - radius + m_halfWorldDepth) * invCellDepth;
    float normMaxZ = (z + radius + m_halfWorldDepth) * invCellDepth;

    minX = std::clamp(static_cast<int>(normMinX), 0, gridSize - 1);
    maxX = std::clamp(static_cast<int>(normMaxX), 0, gridSize - 1);
    minZ = std::clamp(static_cast<int>(normMinZ), 0, gridSize - 1);
    maxZ = std::clamp(static_cast<int>(normMaxZ), 0, gridSize - 1);
}

} // namespace Forge
