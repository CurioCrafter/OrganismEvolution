#include "RenderingOptimizer.h"
#include "Frustum.h"
#include "../entities/Creature.h"
#include "../entities/CreatureType.h"
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

namespace Forge {

// ============================================================================
// Cull and Sort
// ============================================================================

void RenderingOptimizer::cullAndSort(const std::vector<Creature*>& creatures,
                                      const Frustum& frustum,
                                      const glm::vec3& cameraPosition,
                                      const glm::mat4& viewProjection,
                                      float screenWidth, float screenHeight) {
    auto startTime = std::chrono::high_resolution_clock::now();

    m_visibleCreatures.clear();
    m_stats.reset();

    // Ensure LOD array is sized
    if (m_creatureLODs.size() < creatures.size()) {
        m_creatureLODs.resize(creatures.size(), MeshLOD::CULLED);
    }

    // Get FOV for screen-space calculations (assuming typical FOV)
    float fovY = glm::radians(60.0f);

    // Process all creatures
    for (size_t i = 0; i < creatures.size(); ++i) {
        Creature* creature = creatures[i];
        if (!creature || !creature->isActive()) {
            m_creatureLODs[i] = MeshLOD::CULLED;
            continue;
        }

        m_stats.totalCreatures++;

        glm::vec3 pos = creature->getPosition();
        float size = creature->getSize() * 2.0f;  // Bounding sphere

        // Calculate distance
        glm::vec3 diff = pos - cameraPosition;
        float distance = glm::length(diff);

        // Distance culling (early out)
        if (m_config.enableDistanceCulling &&
            distance > m_config.pointDistance * m_config.qualityScale) {
            m_creatureLODs[i] = MeshLOD::CULLED;
            m_stats.culledByDistance++;
            continue;
        }

        // Frustum culling
        if (m_config.enableFrustumCulling) {
            if (!frustum.isSphereVisible(pos, size)) {
                m_creatureLODs[i] = MeshLOD::CULLED;
                m_stats.culledByFrustum++;
                continue;
            }
        }

        // Calculate screen size
        float screenSize = calculateScreenSize(size, distance, fovY, screenHeight);

        // Screen-space culling (too small to see)
        if (m_config.enableScreenSpaceLOD && screenSize < 0.5f) {
            m_creatureLODs[i] = MeshLOD::CULLED;
            m_stats.culledByScreenSize++;
            continue;
        }

        // Calculate LOD
        MeshLOD lod = calculateLOD(distance, screenSize);
        m_creatureLODs[i] = lod;

        if (lod == MeshLOD::CULLED) {
            continue;
        }

        // Add to visible list
        VisibleCreature vc;
        vc.creature = creature;
        vc.index = i;
        vc.lod = lod;
        vc.distance = distance;
        vc.screenSize = screenSize;
        vc.isOccluded = false;
        vc.worldMatrix = buildWorldMatrix(creature);
        vc.color = getCreatureColor(creature);
        vc.animationTime = creature->getAge();

        m_visibleCreatures.push_back(vc);
        m_stats.visibleCreatures++;
        m_stats.countByLOD[static_cast<size_t>(lod)]++;
    }

    auto cullEnd = std::chrono::high_resolution_clock::now();
    auto cullDuration = std::chrono::duration_cast<std::chrono::microseconds>(cullEnd - startTime);
    m_stats.cullTimeMs = cullDuration.count() / 1000.0f;

    // Sort visible creatures
    auto sortStart = std::chrono::high_resolution_clock::now();

    if (m_config.sortByDistance) {
        std::sort(m_visibleCreatures.begin(), m_visibleCreatures.end(),
            [](const VisibleCreature& a, const VisibleCreature& b) {
                // Primary: LOD (lower LOD = higher detail = render first for better z-buffer)
                if (a.lod != b.lod) {
                    return static_cast<int>(a.lod) < static_cast<int>(b.lod);
                }
                // Secondary: distance (front-to-back for opaque, back-to-front for transparent)
                return a.distance < b.distance;
            });
    }

    auto sortEnd = std::chrono::high_resolution_clock::now();
    auto sortDuration = std::chrono::duration_cast<std::chrono::microseconds>(sortEnd - sortStart);
    m_stats.sortTimeMs = sortDuration.count() / 1000.0f;
}

// ============================================================================
// Build Batches
// ============================================================================

void RenderingOptimizer::buildBatches() {
    auto startTime = std::chrono::high_resolution_clock::now();

    m_batches.clear();

    if (m_config.batchByCreatureType) {
        // Advanced batching: group by LOD AND creature type for minimal state changes
        // Key: (LOD << 16) | creatureTypeID
        std::unordered_map<uint32_t, InstanceBatch> batchMap;

        for (auto& vc : m_visibleCreatures) {
            if (vc.lod == MeshLOD::CULLED) continue;

            int creatureType = vc.creature->getCreatureType();
            uint32_t key = (static_cast<uint32_t>(vc.lod) << 16) | (creatureType & 0xFFFF);

            auto& batch = batchMap[key];
            if (batch.instances.empty()) {
                batch.lod = vc.lod;
                batch.creatureTypeID = creatureType;
                batch.reserve(256);  // Pre-allocate for efficiency
            }
            batch.instances.push_back(&vc);
        }

        // Convert map to vector and split large batches
        size_t maxBatch = static_cast<size_t>(m_config.maxInstancesPerBatch);

        for (auto& pair : batchMap) {
            auto& batch = pair.second;
            if (batch.instances.empty()) continue;

            // Split large batches for better load balancing
            for (size_t start = 0; start < batch.instances.size(); start += maxBatch) {
                size_t end = std::min(start + maxBatch, batch.instances.size());

                InstanceBatch subBatch;
                subBatch.lod = batch.lod;
                subBatch.creatureTypeID = batch.creatureTypeID;
                subBatch.instances.assign(batch.instances.begin() + start,
                                          batch.instances.begin() + end);
                subBatch.buildGPUData();
                subBatch.generateSortKey();

                m_batches.push_back(std::move(subBatch));
                m_stats.totalBatches++;
                m_stats.totalInstances += static_cast<int>(end - start);
            }
        }

        // Sort batches for optimal rendering order (minimize state changes)
        std::sort(m_batches.begin(), m_batches.end());

    } else {
        // Simple batching: group by LOD only
        std::array<InstanceBatch, static_cast<size_t>(MeshLOD::COUNT)> lodBatches;

        for (size_t i = 0; i < static_cast<size_t>(MeshLOD::COUNT) - 1; ++i) {
            lodBatches[i].lod = static_cast<MeshLOD>(i);
            lodBatches[i].reserve(m_stats.countByLOD[i]);
        }

        for (auto& vc : m_visibleCreatures) {
            if (vc.lod == MeshLOD::CULLED) continue;
            lodBatches[static_cast<size_t>(vc.lod)].instances.push_back(&vc);
        }

        size_t maxBatch = static_cast<size_t>(m_config.maxInstancesPerBatch);

        for (auto& batch : lodBatches) {
            if (batch.instances.empty()) continue;

            for (size_t start = 0; start < batch.instances.size(); start += maxBatch) {
                size_t end = std::min(start + maxBatch, batch.instances.size());

                InstanceBatch subBatch;
                subBatch.lod = batch.lod;
                subBatch.instances.assign(batch.instances.begin() + start,
                                          batch.instances.begin() + end);
                subBatch.buildGPUData();

                m_batches.push_back(std::move(subBatch));
                m_stats.totalBatches++;
                m_stats.totalInstances += static_cast<int>(end - start);
            }
        }
    }

    // Mega-batching: merge small same-type batches for fewer draw calls
    if (m_config.enableMegaBatching && m_batches.size() > 10) {
        mergSmallBatches();
    }

    m_stats.drawCalls = static_cast<int>(m_batches.size());

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    m_stats.batchTimeMs = duration.count() / 1000.0f;
}

void RenderingOptimizer::mergSmallBatches() {
    // Merge batches with same LOD and type that are below threshold
    constexpr size_t MERGE_THRESHOLD = 256;
    constexpr size_t MAX_MERGED_SIZE = 2048;

    std::vector<InstanceBatch> mergedBatches;
    mergedBatches.reserve(m_batches.size());

    InstanceBatch currentMerge;
    currentMerge.lod = MeshLOD::CULLED;

    for (auto& batch : m_batches) {
        // Check if we can merge with current
        bool canMerge = (currentMerge.lod == batch.lod &&
                        currentMerge.creatureTypeID == batch.creatureTypeID &&
                        currentMerge.instances.size() + batch.instances.size() <= MAX_MERGED_SIZE);

        if (canMerge && !currentMerge.instances.empty()) {
            // Merge into current batch
            currentMerge.instances.insert(currentMerge.instances.end(),
                                          batch.instances.begin(),
                                          batch.instances.end());
            currentMerge.gpuData.insert(currentMerge.gpuData.end(),
                                        batch.gpuData.begin(),
                                        batch.gpuData.end());
        } else {
            // Finish current merge and start new one
            if (!currentMerge.instances.empty()) {
                mergedBatches.push_back(std::move(currentMerge));
            }
            currentMerge = std::move(batch);
        }
    }

    // Don't forget the last batch
    if (!currentMerge.instances.empty()) {
        mergedBatches.push_back(std::move(currentMerge));
    }

    m_batches = std::move(mergedBatches);
}

// ============================================================================
// Query Methods
// ============================================================================

std::vector<const VisibleCreature*> RenderingOptimizer::getCreaturesAtLOD(MeshLOD lod) const {
    std::vector<const VisibleCreature*> result;
    result.reserve(m_stats.countByLOD[static_cast<size_t>(lod)]);

    for (const auto& vc : m_visibleCreatures) {
        if (vc.lod == lod) {
            result.push_back(&vc);
        }
    }

    return result;
}

// ============================================================================
// Helper Methods
// ============================================================================

bool RenderingOptimizer::frustumCull(const glm::vec3& position, float radius,
                                      const Frustum& frustum) const {
    return frustum.isSphereVisible(position, radius);
}

glm::mat4 RenderingOptimizer::buildWorldMatrix(const Creature* creature) const {
    glm::mat4 world = glm::mat4(1.0f);

    // Translation
    world = glm::translate(world, creature->getPosition());

    // Rotation (creature's facing direction)
    float rotation = creature->getRotation();
    world = glm::rotate(world, rotation, glm::vec3(0.0f, 1.0f, 0.0f));

    // Scale based on creature size
    float scale = creature->getSize();
    world = glm::scale(world, glm::vec3(scale));

    return world;
}

glm::vec4 RenderingOptimizer::getCreatureColor(const Creature* creature) const {
    // Get base color from creature's genome
    const Genome& genome = creature->getGenome();
    glm::vec3 baseColor(genome.color.r, genome.color.g, genome.color.b);

    // Modulate based on health/energy
    float energy = creature->getEnergy();
    float maxEnergy = 200.0f;
    float healthFactor = std::min(1.0f, energy / maxEnergy);

    // Desaturate when low energy
    if (healthFactor < 0.3f) {
        glm::vec3 gray(0.5f);
        baseColor = glm::mix(gray, baseColor, healthFactor / 0.3f);
    }

    // Flash red when being hunted
    if (creature->isBeingHunted()) {
        float flash = std::sin(creature->getAge() * 10.0f) * 0.5f + 0.5f;
        baseColor = glm::mix(baseColor, glm::vec3(1.0f, 0.2f, 0.2f), flash * 0.3f);
    }

    return glm::vec4(baseColor, 1.0f);
}

} // namespace Forge
