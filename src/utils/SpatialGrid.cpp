#include "SpatialGrid.h"
#include "../entities/Creature.h"
#include "../entities/CreatureType.h"
#include <algorithm>
#include <cmath>

SpatialGrid::SpatialGrid(float worldWidth, float worldDepth, int gridSize)
    : worldWidth(worldWidth), worldDepth(worldDepth), gridSize(gridSize) {

    cellWidth = worldWidth / gridSize;
    cellDepth = worldDepth / gridSize;

    // Precompute inverse for faster division
    invCellWidth = 1.0f / cellWidth;
    invCellDepth = 1.0f / cellDepth;
    halfWorldWidth = worldWidth * 0.5f;
    halfWorldDepth = worldDepth * 0.5f;

    // Initialize flat grid (row-major order)
    cells.resize(gridSize * gridSize);

    // Pre-allocate query buffer
    m_queryBuffer.reserve(256);
}

void SpatialGrid::clear() {
    // Reset all cells without deallocating
    for (Cell& cell : cells) {
        cell.clear();
    }
    m_totalCreatures = 0;
    m_maxCellOccupancy = 0;
}

int SpatialGrid::worldToCellIndex(float x, float z) const {
    // Convert from world coords (centered at origin) to grid coords
    float normalizedX = (x + halfWorldWidth) * invCellWidth;
    float normalizedZ = (z + halfWorldDepth) * invCellDepth;

    int cellX = static_cast<int>(normalizedX);
    int cellZ = static_cast<int>(normalizedZ);

    // Clamp to valid range
    cellX = std::clamp(cellX, 0, gridSize - 1);
    cellZ = std::clamp(cellZ, 0, gridSize - 1);

    return cellZ * gridSize + cellX;
}

void SpatialGrid::worldToCell(float x, float z, int& cellX, int& cellZ) const {
    // Convert from world coords (centered at origin) to grid coords
    float normalizedX = (x + halfWorldWidth) * invCellWidth;
    float normalizedZ = (z + halfWorldDepth) * invCellDepth;

    cellX = static_cast<int>(normalizedX);
    cellZ = static_cast<int>(normalizedZ);

    // Clamp to valid range
    cellX = std::clamp(cellX, 0, gridSize - 1);
    cellZ = std::clamp(cellZ, 0, gridSize - 1);
}

void SpatialGrid::getCellsInRadius(float x, float z, float radius,
                                    int& minCellX, int& maxCellX,
                                    int& minCellZ, int& maxCellZ) const {
    worldToCell(x - radius, z - radius, minCellX, minCellZ);
    worldToCell(x + radius, z + radius, maxCellX, maxCellZ);
}

void SpatialGrid::insert(Creature* creature) {
    if (!creature || !creature->isAlive()) return;

    const glm::vec3& pos = creature->getPosition();
    int idx = worldToCellIndex(pos.x, pos.z);

    if (cells[idx].add(creature)) {
        m_totalCreatures++;
        m_maxCellOccupancy = std::max(m_maxCellOccupancy, static_cast<size_t>(cells[idx].count));
    }
}

const std::vector<Creature*>& SpatialGrid::query(const glm::vec3& position, float radius) const {
    m_queryBuffer.clear();
    m_queryCount++;

    int minCellX, maxCellX, minCellZ, maxCellZ;
    getCellsInRadius(position.x, position.z, radius, minCellX, maxCellX, minCellZ, maxCellZ);

    const float radiusSq = radius * radius;

    for (int cz = minCellZ; cz <= maxCellZ; cz++) {
        for (int cx = minCellX; cx <= maxCellX; cx++) {
            const Cell& cell = getCellAt(cx, cz);
            for (int i = 0; i < cell.count; i++) {
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

const std::vector<Creature*>& SpatialGrid::queryByType(const glm::vec3& position, float radius, int creatureType) const {
    m_queryBuffer.clear();
    m_queryCount++;

    int minCellX, maxCellX, minCellZ, maxCellZ;
    getCellsInRadius(position.x, position.z, radius, minCellX, maxCellX, minCellZ, maxCellZ);

    const float radiusSq = radius * radius;

    for (int cz = minCellZ; cz <= maxCellZ; cz++) {
        for (int cx = minCellX; cx <= maxCellX; cx++) {
            const Cell& cell = getCellAt(cx, cz);
            for (int i = 0; i < cell.count; i++) {
                Creature* creature = cell.creatures[i];
                if (!creature || !creature->isAlive()) continue;

                // Check type first (cheaper than distance)
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

Creature* SpatialGrid::findNearest(const glm::vec3& position, float maxRadius, int typeFilter) const {
    Creature* nearest = nullptr;
    float nearestDistSq = maxRadius * maxRadius;

    int minCellX, maxCellX, minCellZ, maxCellZ;
    getCellsInRadius(position.x, position.z, maxRadius, minCellX, maxCellX, minCellZ, maxCellZ);

    for (int cz = minCellZ; cz <= maxCellZ; cz++) {
        for (int cx = minCellX; cx <= maxCellX; cx++) {
            const Cell& cell = getCellAt(cx, cz);
            for (int i = 0; i < cell.count; i++) {
                Creature* creature = cell.creatures[i];
                if (!creature || !creature->isAlive()) continue;

                // Check type filter if specified
                if (typeFilter >= 0 && static_cast<int>(creature->getType()) != typeFilter) continue;

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

    return nearest;
}

int SpatialGrid::countNearby(const glm::vec3& position, float radius) const {
    int count = 0;

    int minCellX, maxCellX, minCellZ, maxCellZ;
    getCellsInRadius(position.x, position.z, radius, minCellX, maxCellX, minCellZ, maxCellZ);

    const float radiusSq = radius * radius;

    for (int cz = minCellZ; cz <= maxCellZ; cz++) {
        for (int cx = minCellX; cx <= maxCellX; cx++) {
            const Cell& cell = getCellAt(cx, cz);
            for (int i = 0; i < cell.count; i++) {
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
