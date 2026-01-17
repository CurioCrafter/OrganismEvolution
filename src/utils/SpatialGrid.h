#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

class Creature;

/**
 * @class SpatialGrid
 * @brief Optimized spatial partitioning grid for O(1) neighbor queries.
 *
 * Divides world into cells and maintains lists of creatures per cell.
 * Optimizations:
 * - Flat array layout for cache-friendly access
 * - Reusable query buffer to avoid per-query allocations
 * - Fixed-size cell arrays (no dynamic allocation in cells)
 * - Squared distance comparisons (avoid sqrt)
 */
class SpatialGrid {
public:
    /// Maximum creatures per cell (fixed allocation)
    static constexpr int MAX_PER_CELL = 64;

    SpatialGrid(float worldWidth, float worldDepth, int gridSize = 20);

    // Clear and rebuild the grid (call once per frame before queries)
    void clear();
    void insert(Creature* creature);

    // Query all creatures within radius of position
    // OPTIMIZED: Returns reference to internal buffer (no allocation)
    const std::vector<Creature*>& query(const glm::vec3& position, float radius) const;

    // Query creatures of a specific type within radius (optimization for predator-prey)
    // OPTIMIZED: Returns reference to internal buffer (no allocation)
    const std::vector<Creature*>& queryByType(const glm::vec3& position, float radius, int creatureType) const;

    // Get the nearest creature to position (optionally filtering by type)
    Creature* findNearest(const glm::vec3& position, float maxRadius, int typeFilter = -1) const;

    // Get count of creatures in nearby cells (no allocation)
    int countNearby(const glm::vec3& position, float radius) const;

    // Statistics for performance monitoring
    size_t getTotalCreatures() const { return m_totalCreatures; }
    size_t getMaxCellOccupancy() const { return m_maxCellOccupancy; }
    size_t getQueryCount() const { return m_queryCount; }
    void resetStats() { m_queryCount = 0; m_maxCellOccupancy = 0; }

private:
    float worldWidth;
    float worldDepth;
    int gridSize;
    float cellWidth;
    float cellDepth;
    float invCellWidth;   // Precomputed 1/cellWidth for faster division
    float invCellDepth;   // Precomputed 1/cellDepth for faster division
    float halfWorldWidth; // Precomputed worldWidth * 0.5f
    float halfWorldDepth; // Precomputed worldDepth * 0.5f

    /**
     * @struct Cell
     * @brief Fixed-size cell structure for cache-friendly storage.
     */
    struct Cell {
        Creature* creatures[MAX_PER_CELL];
        int count = 0;

        void clear() { count = 0; }

        bool add(Creature* c) {
            if (count >= MAX_PER_CELL) return false;
            creatures[count++] = c;
            return true;
        }
    };

    // Flat array of cells (row-major order for cache locality)
    std::vector<Cell> cells;

    // Reusable query result buffer (avoids per-query allocation)
    mutable std::vector<Creature*> m_queryBuffer;

    // Statistics
    mutable size_t m_totalCreatures = 0;
    mutable size_t m_maxCellOccupancy = 0;
    mutable size_t m_queryCount = 0;

    // Convert world position to grid cell index (flat array)
    int worldToCellIndex(float x, float z) const;

    // Convert world position to grid cell indices (x, z)
    void worldToCell(float x, float z, int& cellX, int& cellZ) const;

    // Get list of cell indices that overlap with a circle
    void getCellsInRadius(float x, float z, float radius,
                          int& minCellX, int& maxCellX,
                          int& minCellZ, int& maxCellZ) const;

    // Get cell at (x, z) grid coordinates
    Cell& getCellAt(int x, int z) { return cells[z * gridSize + x]; }
    const Cell& getCellAt(int x, int z) const { return cells[z * gridSize + x]; }
};
