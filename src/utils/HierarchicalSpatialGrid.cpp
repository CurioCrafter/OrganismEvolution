#include "HierarchicalSpatialGrid.h"
#include "../entities/Creature.h"
#include "../entities/CreatureType.h"
#include <algorithm>
#include <chrono>

namespace Forge {

// ============================================================================
// Insert
// ============================================================================

void HierarchicalSpatialGrid::insert(Creature* creature) {
    if (!creature || !creature->isAlive()) return;

    const glm::vec3& pos = creature->getPosition();

    // Insert into coarse grid
    int coarseIdx = worldToCoarseCell(pos.x, pos.z);
    if (m_coarseGrid[coarseIdx].isEmpty()) {
        m_stats.coarseCellsUsed++;
    }
    m_coarseGrid[coarseIdx].add(creature, pos);

    // Insert into fine grid
    int fineIdx = worldToFineCell(pos.x, pos.z);
    if (m_fineGrid[fineIdx].isEmpty()) {
        m_stats.fineCellsUsed++;
    }
    m_fineGrid[fineIdx].add(creature, pos);

    m_stats.totalCreatures++;

    // Track max occupancy
    m_stats.maxCoarseOccupancy = std::max(m_stats.maxCoarseOccupancy,
                                           static_cast<size_t>(m_coarseGrid[coarseIdx].count));
    m_stats.maxFineOccupancy = std::max(m_stats.maxFineOccupancy,
                                         static_cast<size_t>(m_fineGrid[fineIdx].count));
}

// ============================================================================
// Rebuild
// ============================================================================

void HierarchicalSpatialGrid::rebuild(const std::vector<Creature*>& creatures) {
    clear();

    for (Creature* c : creatures) {
        if (c && c->isAlive()) {
            insert(c);
        }
    }
}

// ============================================================================
// Query (Hierarchical)
// ============================================================================

const std::vector<Creature*>& HierarchicalSpatialGrid::query(
    const glm::vec3& position, float radius) const {

    auto startTime = std::chrono::high_resolution_clock::now();

    m_queryBuffer.clear();
    m_stats.queryCount++;

    const float radiusSq = radius * radius;

    // Determine which grid level to use based on query radius
    // Use coarse grid for large radii to reduce cell iteration
    bool useCoarseGrid = radius > m_fineCellWidth * 2.0f;

    if (useCoarseGrid) {
        // First pass: coarse grid cells (for region rejection)
        int minCoarseX, maxCoarseX, minCoarseZ, maxCoarseZ;
        getCellsInRadius(position.x, position.z, radius,
                         m_config.coarseGridSize,
                         m_invCoarseCellWidth, m_invCoarseCellDepth,
                         minCoarseX, maxCoarseX, minCoarseZ, maxCoarseZ);

        for (int cz = minCoarseZ; cz <= maxCoarseZ; ++cz) {
            for (int cx = minCoarseX; cx <= maxCoarseX; ++cx) {
                const CoarseCell& cell = m_coarseGrid[cz * m_config.coarseGridSize + cx];

                // Quick bounds check
                if (!cell.sphereIntersects(position, radius)) continue;

                // Check all creatures in coarse cell
                for (int i = 0; i < cell.count; ++i) {
                    Creature* creature = cell.creatures[i];
                    if (!creature || !creature->isAlive()) continue;

                    const glm::vec3& cpos = creature->getPosition();
                    float dx = cpos.x - position.x;
                    float dz = cpos.z - position.z;
                    float distSq = dx * dx + dz * dz;

                    if (distSq <= radiusSq) {
                        m_queryBuffer.push_back(creature);
                    }
                }
            }
        }
    } else {
        // Use fine grid for small radii
        int minFineX, maxFineX, minFineZ, maxFineZ;
        getCellsInRadius(position.x, position.z, radius,
                         m_config.fineGridSize,
                         m_invFineCellWidth, m_invFineCellDepth,
                         minFineX, maxFineX, minFineZ, maxFineZ);

        for (int cz = minFineZ; cz <= maxFineZ; ++cz) {
            for (int cx = minFineX; cx <= maxFineX; ++cx) {
                const FineCell& cell = m_fineGrid[cz * m_config.fineGridSize + cx];

                for (int i = 0; i < cell.count; ++i) {
                    Creature* creature = cell.creatures[i];
                    if (!creature || !creature->isAlive()) continue;

                    const glm::vec3& cpos = creature->getPosition();
                    float dx = cpos.x - position.x;
                    float dz = cpos.z - position.z;
                    float distSq = dx * dx + dz * dz;

                    if (distSq <= radiusSq) {
                        m_queryBuffer.push_back(creature);
                    }
                }
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    float queryTimeUs = static_cast<float>(duration.count());

    // Update average (exponential moving average)
    m_stats.avgQueryTimeUs = m_stats.avgQueryTimeUs * 0.95f + queryTimeUs * 0.05f;

    return m_queryBuffer;
}

// ============================================================================
// Query By Type
// ============================================================================

const std::vector<Creature*>& HierarchicalSpatialGrid::queryByType(
    const glm::vec3& position, float radius, int creatureType) const {

    m_queryBuffer.clear();
    m_stats.queryCount++;

    const float radiusSq = radius * radius;

    // Always use fine grid for type queries (typically smaller)
    int minFineX, maxFineX, minFineZ, maxFineZ;
    getCellsInRadius(position.x, position.z, radius,
                     m_config.fineGridSize,
                     m_invFineCellWidth, m_invFineCellDepth,
                     minFineX, maxFineX, minFineZ, maxFineZ);

    for (int cz = minFineZ; cz <= maxFineZ; ++cz) {
        for (int cx = minFineX; cx <= maxFineX; ++cx) {
            const FineCell& cell = m_fineGrid[cz * m_config.fineGridSize + cx];

            for (int i = 0; i < cell.count; ++i) {
                Creature* creature = cell.creatures[i];
                if (!creature || !creature->isAlive()) continue;

                // Type check first (cheaper than distance)
                if (static_cast<int>(creature->getType()) != creatureType) continue;

                const glm::vec3& cpos = creature->getPosition();
                float dx = cpos.x - position.x;
                float dz = cpos.z - position.z;
                float distSq = dx * dx + dz * dz;

                if (distSq <= radiusSq) {
                    m_queryBuffer.push_back(creature);
                }
            }
        }
    }

    return m_queryBuffer;
}

// ============================================================================
// Find Nearest
// ============================================================================

Creature* HierarchicalSpatialGrid::findNearest(const glm::vec3& position,
                                                 float maxRadius, int typeFilter) const {
    Creature* nearest = nullptr;
    float nearestDistSq = maxRadius * maxRadius;

    // Start with fine grid cells near position
    int minFineX, maxFineX, minFineZ, maxFineZ;
    getCellsInRadius(position.x, position.z, maxRadius,
                     m_config.fineGridSize,
                     m_invFineCellWidth, m_invFineCellDepth,
                     minFineX, maxFineX, minFineZ, maxFineZ);

    // Search in expanding rings from center for early termination
    int centerX, centerZ;
    worldToFineCellXZ(position.x, position.z, centerX, centerZ);

    // Process cells in order of distance from center
    for (int ring = 0; ring <= std::max(maxFineX - minFineX, maxFineZ - minFineZ); ++ring) {
        for (int cz = std::max(minFineZ, centerZ - ring);
             cz <= std::min(maxFineZ, centerZ + ring); ++cz) {
            for (int cx = std::max(minFineX, centerX - ring);
                 cx <= std::min(maxFineX, centerX + ring); ++cx) {

                // Only process cells on the current ring
                if (ring > 0 &&
                    cx > centerX - ring && cx < centerX + ring &&
                    cz > centerZ - ring && cz < centerZ + ring) {
                    continue;
                }

                const FineCell& cell = m_fineGrid[cz * m_config.fineGridSize + cx];

                for (int i = 0; i < cell.count; ++i) {
                    Creature* creature = cell.creatures[i];
                    if (!creature || !creature->isAlive()) continue;

                    if (typeFilter >= 0 &&
                        static_cast<int>(creature->getType()) != typeFilter) continue;

                    const glm::vec3& cpos = creature->getPosition();
                    float dx = cpos.x - position.x;
                    float dz = cpos.z - position.z;
                    float distSq = dx * dx + dz * dz;

                    if (distSq < nearestDistSq && distSq > 0.001f) {
                        nearestDistSq = distSq;
                        nearest = creature;
                    }
                }
            }
        }

        // Early termination: if we found something and all remaining cells
        // are farther than our current best, we're done
        if (nearest) {
            float ringDist = ring * std::min(m_fineCellWidth, m_fineCellDepth);
            if (ringDist * ringDist > nearestDistSq) {
                return nearest;
            }
        }
    }

    return nearest;
}

// ============================================================================
// Count Nearby
// ============================================================================

int HierarchicalSpatialGrid::countNearby(const glm::vec3& position, float radius) const {
    int count = 0;
    const float radiusSq = radius * radius;

    int minFineX, maxFineX, minFineZ, maxFineZ;
    getCellsInRadius(position.x, position.z, radius,
                     m_config.fineGridSize,
                     m_invFineCellWidth, m_invFineCellDepth,
                     minFineX, maxFineX, minFineZ, maxFineZ);

    for (int cz = minFineZ; cz <= maxFineZ; ++cz) {
        for (int cx = minFineX; cx <= maxFineX; ++cx) {
            const FineCell& cell = m_fineGrid[cz * m_config.fineGridSize + cx];

            for (int i = 0; i < cell.count; ++i) {
                Creature* creature = cell.creatures[i];
                if (!creature || !creature->isAlive()) continue;

                const glm::vec3& cpos = creature->getPosition();
                float dx = cpos.x - position.x;
                float dz = cpos.z - position.z;
                float distSq = dx * dx + dz * dz;

                if (distSq <= radiusSq) {
                    count++;
                }
            }
        }
    }

    return count;
}

// ============================================================================
// Query With Limit
// ============================================================================

const std::vector<Creature*>& HierarchicalSpatialGrid::queryWithLimit(
    const glm::vec3& position, float radius, int maxResults) const {

    m_queryBuffer.clear();
    m_stats.queryCount++;

    const float radiusSq = radius * radius;

    int minFineX, maxFineX, minFineZ, maxFineZ;
    getCellsInRadius(position.x, position.z, radius,
                     m_config.fineGridSize,
                     m_invFineCellWidth, m_invFineCellDepth,
                     minFineX, maxFineX, minFineZ, maxFineZ);

    for (int cz = minFineZ; cz <= maxFineZ && static_cast<int>(m_queryBuffer.size()) < maxResults; ++cz) {
        for (int cx = minFineX; cx <= maxFineX && static_cast<int>(m_queryBuffer.size()) < maxResults; ++cx) {
            const FineCell& cell = m_fineGrid[cz * m_config.fineGridSize + cx];

            for (int i = 0; i < cell.count && static_cast<int>(m_queryBuffer.size()) < maxResults; ++i) {
                Creature* creature = cell.creatures[i];
                if (!creature || !creature->isAlive()) continue;

                const glm::vec3& cpos = creature->getPosition();
                float dx = cpos.x - position.x;
                float dz = cpos.z - position.z;
                float distSq = dx * dx + dz * dz;

                if (distSq <= radiusSq) {
                    m_queryBuffer.push_back(creature);
                }
            }
        }
    }

    return m_queryBuffer;
}

// ============================================================================
// Query K Nearest
// ============================================================================

const std::vector<Creature*>& HierarchicalSpatialGrid::queryKNearest(
    const glm::vec3& position, float maxRadius, int k) const {

    m_distanceBuffer.clear();
    m_stats.queryCount++;

    const float radiusSq = maxRadius * maxRadius;

    int minFineX, maxFineX, minFineZ, maxFineZ;
    getCellsInRadius(position.x, position.z, maxRadius,
                     m_config.fineGridSize,
                     m_invFineCellWidth, m_invFineCellDepth,
                     minFineX, maxFineX, minFineZ, maxFineZ);

    for (int cz = minFineZ; cz <= maxFineZ; ++cz) {
        for (int cx = minFineX; cx <= maxFineX; ++cx) {
            const FineCell& cell = m_fineGrid[cz * m_config.fineGridSize + cx];

            for (int i = 0; i < cell.count; ++i) {
                Creature* creature = cell.creatures[i];
                if (!creature || !creature->isAlive()) continue;

                const glm::vec3& cpos = creature->getPosition();
                float dx = cpos.x - position.x;
                float dz = cpos.z - position.z;
                float distSq = dx * dx + dz * dz;

                if (distSq <= radiusSq && distSq > 0.001f) {
                    m_distanceBuffer.push_back({creature, distSq});
                }
            }
        }
    }

    // Sort by distance
    std::partial_sort(m_distanceBuffer.begin(),
                      m_distanceBuffer.begin() + std::min(k, static_cast<int>(m_distanceBuffer.size())),
                      m_distanceBuffer.end(),
                      [](const auto& a, const auto& b) { return a.second < b.second; });

    // Build result
    m_queryBuffer.clear();
    int resultCount = std::min(k, static_cast<int>(m_distanceBuffer.size()));
    m_queryBuffer.reserve(resultCount);

    for (int i = 0; i < resultCount; ++i) {
        m_queryBuffer.push_back(m_distanceBuffer[i].first);
    }

    return m_queryBuffer;
}

// ============================================================================
// Ray Query
// ============================================================================

const std::vector<Creature*>& HierarchicalSpatialGrid::queryRay(
    const glm::vec3& start, const glm::vec3& direction, float maxDistance) const {

    m_queryBuffer.clear();
    m_stats.queryCount++;

    // Normalize direction
    glm::vec3 dir = glm::normalize(direction);

    // Use DDA-like traversal through fine grid
    float tDeltaX = std::abs(m_fineCellWidth / (dir.x + 0.0001f));
    float tDeltaZ = std::abs(m_fineCellDepth / (dir.z + 0.0001f));

    int stepX = dir.x > 0 ? 1 : -1;
    int stepZ = dir.z > 0 ? 1 : -1;

    int cellX, cellZ;
    worldToFineCellXZ(start.x, start.z, cellX, cellZ);

    // Calculate initial t values
    float cellMinX = -m_halfWorldWidth + cellX * m_fineCellWidth;
    float cellMinZ = -m_halfWorldDepth + cellZ * m_fineCellDepth;

    float tMaxX = (stepX > 0) ?
        (cellMinX + m_fineCellWidth - start.x) / (dir.x + 0.0001f) :
        (start.x - cellMinX) / (-dir.x + 0.0001f);
    float tMaxZ = (stepZ > 0) ?
        (cellMinZ + m_fineCellDepth - start.z) / (dir.z + 0.0001f) :
        (start.z - cellMinZ) / (-dir.z + 0.0001f);

    float t = 0.0f;

    while (t < maxDistance) {
        // Check current cell
        if (cellX >= 0 && cellX < m_config.fineGridSize &&
            cellZ >= 0 && cellZ < m_config.fineGridSize) {

            const FineCell& cell = m_fineGrid[cellZ * m_config.fineGridSize + cellX];

            for (int i = 0; i < cell.count; ++i) {
                Creature* creature = cell.creatures[i];
                if (!creature || !creature->isAlive()) continue;

                // Check if creature intersects ray
                const glm::vec3& cpos = creature->getPosition();
                float radius = creature->getSize();

                // Simplified ray-sphere test (project point onto ray)
                glm::vec3 oc = cpos - start;
                float tClosest = glm::dot(oc, dir);
                if (tClosest < 0 || tClosest > maxDistance) continue;

                glm::vec3 closest = start + dir * tClosest;
                float distSq = glm::dot(cpos - closest, cpos - closest);

                if (distSq <= radius * radius) {
                    m_queryBuffer.push_back(creature);
                }
            }
        }

        // Step to next cell
        if (tMaxX < tMaxZ) {
            t = tMaxX;
            tMaxX += tDeltaX;
            cellX += stepX;
        } else {
            t = tMaxZ;
            tMaxZ += tDeltaZ;
            cellZ += stepZ;
        }
    }

    return m_queryBuffer;
}

} // namespace Forge
