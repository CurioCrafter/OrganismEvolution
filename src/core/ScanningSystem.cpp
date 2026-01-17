#include "ScanningSystem.h"
#include "../graphics/Camera.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace discovery {

// ============================================================================
// Global Instance
// ============================================================================

static std::unique_ptr<ScanningSystem> g_scanner;

ScanningSystem& getScanner() {
    if (!g_scanner) {
        g_scanner = std::make_unique<ScanningSystem>();
    }
    return *g_scanner;
}

// ============================================================================
// ScanningSystem Implementation
// ============================================================================

ScanningSystem::ScanningSystem() = default;

void ScanningSystem::initialize(SpeciesCatalog* catalog) {
    m_catalog = catalog;
    m_initialized = true;
    m_scanningEnabled = true;  // Start enabled
}

void ScanningSystem::shutdown() {
    m_initialized = false;
    m_catalog = nullptr;
    m_visibleTargets.clear();
}

void ScanningSystem::update(float deltaTime,
                            const Camera& camera,
                            const std::vector<std::unique_ptr<Creature>>& creatures,
                            const BiomeSystem* biomeSystem,
                            float simulationTime,
                            float screenWidth,
                            float screenHeight) {
    if (!m_initialized || !m_scanningEnabled || !m_catalog) {
        return;
    }

    // Update cache
    m_cacheScreenWidth = screenWidth;
    m_cacheScreenHeight = screenHeight;

    // Find visible targets
    findVisibleTargets(camera, creatures, screenWidth, screenHeight);

    // Update auto-targeting if enabled
    if (m_autoTargetEnabled && m_lockedTargetId == -1 && !m_visibleTargets.empty()) {
        updateAutoTarget();
    }

    // Validate locked target still exists
    if (m_lockedTargetId != -1) {
        bool found = false;
        for (const auto& target : m_visibleTargets) {
            if (target.creatureId == m_lockedTargetId) {
                found = true;
                break;
            }
        }
        if (!found) {
            // Target lost (died or out of range)
            unlockTarget();
        }
    }

    // Update scan progress for locked target
    if (m_lockedTargetId != -1) {
        updateScanProgress(deltaTime, biomeSystem, simulationTime);
    }
}

void ScanningSystem::setScanning(bool enabled) {
    if (enabled != m_scanningEnabled) {
        m_scanningEnabled = enabled;
        if (!enabled) {
            unlockTarget();
            m_visibleTargets.clear();
        }
    }
}

bool ScanningSystem::lockTarget(int creatureId) {
    // Find the target in visible targets
    for (const auto& target : m_visibleTargets) {
        if (target.creatureId == creatureId && target.isTargetable) {
            int previousTarget = m_lockedTargetId;
            m_lockedTargetId = creatureId;
            m_lockedSpeciesId = target.speciesId;
            m_lockTime = 0.0f;

            // Update catalog active scan
            m_catalog->setActiveScanTarget(target.speciesId, creatureId);

            // Start new scan if not already discovered
            if (target.discoveryState != DiscoveryState::COMPLETE) {
                m_scansStarted++;
            }

            // Emit callback if target changed
            if (previousTarget != creatureId && m_targetChangedCallback) {
                m_targetChangedCallback(&target);
            }

            return true;
        }
    }
    return false;
}

void ScanningSystem::unlockTarget() {
    if (m_lockedTargetId != -1) {
        m_lockedTargetId = -1;
        m_lockedSpeciesId = 0;
        m_lockTime = 0.0f;
        m_catalog->clearActiveScanTarget();

        if (m_targetChangedCallback) {
            m_targetChangedCallback(nullptr);
        }
    }
}

const ScanTargetInfo* ScanningSystem::getCurrentTarget() const {
    if (m_lockedTargetId == -1) {
        return nullptr;
    }

    for (const auto& target : m_visibleTargets) {
        if (target.creatureId == m_lockedTargetId) {
            return &target;
        }
    }
    return nullptr;
}

const ScanTargetInfo* ScanningSystem::getTargetAtScreenPos(float screenX, float screenY, float tolerance) const {
    const ScanTargetInfo* closest = nullptr;
    float closestDist = tolerance * tolerance;

    for (const auto& target : m_visibleTargets) {
        if (!target.isTargetable) continue;

        float dx = target.screenX - screenX;
        float dy = target.screenY - screenY;
        float distSq = dx * dx + dy * dy;

        if (distSq < closestDist) {
            closestDist = distSq;
            closest = &target;
        }
    }
    return closest;
}

const ScanTargetInfo* ScanningSystem::getNearestTargetable() const {
    const ScanTargetInfo* nearest = nullptr;
    float nearestDist = std::numeric_limits<float>::max();

    for (const auto& target : m_visibleTargets) {
        if (!target.isTargetable) continue;

        if (target.distance < nearestDist) {
            nearestDist = target.distance;
            nearest = &target;
        }
    }
    return nearest;
}

void ScanningSystem::findVisibleTargets(const Camera& camera,
                                        const std::vector<std::unique_ptr<Creature>>& creatures,
                                        float screenWidth,
                                        float screenHeight) {
    m_visibleTargets.clear();

    float aspectRatio = screenWidth / screenHeight;
    glm::mat4 view = const_cast<Camera&>(camera).getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix(aspectRatio);
    m_cachedViewProjection = proj * view;

    for (const auto& creature : creatures) {
        if (!creature || !creature->isAlive()) {
            continue;
        }

        ScanTargetInfo info = createTargetInfo(creature.get(), camera, m_cachedViewProjection, screenWidth, screenHeight);

        if (info.isInView || info.distance < m_maxScanDistance) {
            m_visibleTargets.push_back(info);
        }
    }

    // Sort by distance (nearest first)
    std::sort(m_visibleTargets.begin(), m_visibleTargets.end(),
        [](const ScanTargetInfo& a, const ScanTargetInfo& b) {
            return a.distance < b.distance;
        });
}

ScanTargetInfo ScanningSystem::createTargetInfo(const Creature* creature,
                                                 const Camera& camera,
                                                 const glm::mat4& viewProjection,
                                                 float screenWidth,
                                                 float screenHeight) {
    ScanTargetInfo info;
    info.creature = creature;
    info.creatureId = creature->getId();
    info.speciesId = creature->getSpeciesId();
    info.position = creature->getPosition();

    // Calculate distance
    info.distance = glm::length(creature->getPosition() - camera.Position);

    // Calculate screen position
    info.isInView = worldToScreen(creature->getPosition(), camera, viewProjection,
                                   screenWidth, screenHeight, info.screenX, info.screenY);

    // Check if in scan cone
    info.isTargetable = info.isInView &&
                        isInScanCone(camera.Position, camera.Front, creature->getPosition(),
                                    m_maxScanDistance, m_scanAngle);

    // Get discovery state from catalog
    if (m_catalog) {
        info.discoveryState = m_catalog->getDiscoveryState(info.speciesId);

        const auto* entry = m_catalog->getEntry(info.speciesId);
        if (entry) {
            info.rarity = entry->rarity;
            info.scanProgress = entry->getDiscoveryProgress();

            // Only show name if partially discovered
            if (info.discoveryState >= DiscoveryState::PARTIAL) {
                info.displayName = entry->commonName;
            } else if (info.discoveryState == DiscoveryState::DETECTED) {
                info.displayName = "Unknown " + std::string(getCreatureTypeName(creature->getType()));
            } else {
                info.displayName = "???";
            }
        } else {
            info.displayName = "???";
        }
    }

    // Check if being scanned
    info.isBeingScanned = (info.creatureId == m_lockedTargetId);

    return info;
}

void ScanningSystem::updateScanProgress(float deltaTime,
                                        const BiomeSystem* biomeSystem,
                                        float simulationTime) {
    const ScanTargetInfo* target = getCurrentTarget();
    if (!target || !target->creature) {
        unlockTarget();
        return;
    }

    m_lockTime += deltaTime;
    m_totalScanTime += deltaTime;

    // Record sighting in catalog
    BiomeType biome = BiomeType::GRASSLAND;
    if (biomeSystem) {
        auto query = biomeSystem->queryBiome(target->position.x, target->position.z);
        biome = query.biome;
    }

    m_catalog->recordSighting(
        target->speciesId,
        target->creature->getGenome(),
        target->creature->getType(),
        biome,
        target->position,
        target->creatureId,
        target->creature->getGeneration(),
        simulationTime
    );

    // Update scan progress
    bool wasComplete = target->discoveryState == DiscoveryState::COMPLETE;
    bool stateChanged = m_catalog->updateScan(
        target->speciesId,
        deltaTime,
        target->distance,
        true  // Is actively targeted
    );

    // Check for completion
    if (stateChanged && !wasComplete) {
        DiscoveryState newState = m_catalog->getDiscoveryState(target->speciesId);
        if (newState == DiscoveryState::COMPLETE) {
            m_scansCompleted++;

            // Emit completion callback
            if (m_scanCompleteCallback) {
                // Update target info with new state
                ScanTargetInfo updatedInfo = *target;
                updatedInfo.discoveryState = newState;
                updatedInfo.scanProgress = 1.0f;

                const auto* entry = m_catalog->getEntry(target->speciesId);
                if (entry) {
                    updatedInfo.displayName = entry->commonName;
                    updatedInfo.rarity = entry->rarity;
                }

                m_scanCompleteCallback(updatedInfo);
            }
        }
    }
}

bool ScanningSystem::worldToScreen(const glm::vec3& worldPos,
                                    const Camera& camera,
                                    const glm::mat4& viewProjection,
                                    float screenWidth,
                                    float screenHeight,
                                    float& outScreenX,
                                    float& outScreenY) const {
    glm::vec4 clipPos = viewProjection * glm::vec4(worldPos, 1.0f);

    // Behind camera check
    if (clipPos.w <= 0.0f) {
        return false;
    }

    // Perspective divide
    glm::vec3 ndcPos = glm::vec3(clipPos) / clipPos.w;

    // Convert from NDC (-1 to 1) to screen coordinates
    outScreenX = (ndcPos.x + 1.0f) * 0.5f * screenWidth;
    outScreenY = (1.0f - ndcPos.y) * 0.5f * screenHeight;  // Flip Y for screen coordinates

    // Check if within screen bounds (with some margin)
    bool inScreen = ndcPos.x >= -1.2f && ndcPos.x <= 1.2f &&
                    ndcPos.y >= -1.2f && ndcPos.y <= 1.2f &&
                    ndcPos.z >= 0.0f && ndcPos.z <= 1.0f;

    return inScreen;
}

bool ScanningSystem::isInScanCone(const glm::vec3& cameraPos,
                                   const glm::vec3& cameraForward,
                                   const glm::vec3& targetPos,
                                   float maxDistance,
                                   float coneAngleDegrees) const {
    glm::vec3 toTarget = targetPos - cameraPos;
    float distance = glm::length(toTarget);

    if (distance > maxDistance || distance < 0.1f) {
        return false;
    }

    // Normalize direction to target
    glm::vec3 dirToTarget = toTarget / distance;

    // Calculate angle between forward and target direction
    float dotProduct = glm::dot(cameraForward, dirToTarget);
    float angleDegrees = glm::degrees(std::acos(glm::clamp(dotProduct, -1.0f, 1.0f)));

    return angleDegrees <= coneAngleDegrees;
}

void ScanningSystem::updateAutoTarget() {
    // Find best target based on:
    // 1. Not yet fully discovered (priority)
    // 2. Distance (closer is better)
    // 3. Rarity (rarer is better for tiebreaker)

    const ScanTargetInfo* bestTarget = nullptr;
    float bestScore = -1.0f;

    for (const auto& target : m_visibleTargets) {
        if (!target.isTargetable) continue;

        // Calculate score
        float score = 0.0f;

        // Undiscovered species get big bonus
        if (target.discoveryState == DiscoveryState::UNDISCOVERED) {
            score += 1000.0f;
        } else if (target.discoveryState == DiscoveryState::DETECTED) {
            score += 500.0f;
        } else if (target.discoveryState == DiscoveryState::PARTIAL) {
            score += 200.0f;
        }
        // Already complete species get no bonus (score stays 0 base)

        // Distance factor (inverse, closer is better)
        // Normalize to 0-100 based on max scan distance
        float distanceFactor = 1.0f - (target.distance / m_maxScanDistance);
        score += distanceFactor * 100.0f;

        // Rarity bonus (for tiebreaker)
        score += static_cast<float>(target.rarity) * 10.0f;

        if (score > bestScore) {
            bestScore = score;
            bestTarget = &target;
        }
    }

    // Lock the best target
    if (bestTarget && bestTarget->discoveryState != DiscoveryState::COMPLETE) {
        lockTarget(bestTarget->creatureId);
    }
}

} // namespace discovery
