#include "CreatureUpdateScheduler.h"
#include "../entities/Creature.h"
#include "../entities/CreatureType.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Forge {

// ============================================================================
// Schedule Updates
// ============================================================================

void CreatureUpdateScheduler::scheduleUpdates(const std::vector<Creature*>& creatures,
                                               const glm::vec3& cameraPosition,
                                               const glm::mat4& viewProjection,
                                               size_t selectedIndex) {
    m_frameNumber++;

    // Clear previous frame data
    m_updateList.clear();
    for (auto& batch : m_batches) {
        batch.clear();
    }
    m_stats.reset();

    // Ensure scheduled creature array is sized
    if (m_scheduledCreatures.size() < creatures.size()) {
        m_scheduledCreatures.resize(creatures.size());
    }

    // Calculate budget scale based on last frame performance
    if (m_config.adaptiveScheduling) {
        updateAdaptiveBudget();
    }

    // Classify all creatures
    for (size_t i = 0; i < creatures.size(); ++i) {
        Creature* creature = creatures[i];
        if (!creature || !creature->isActive()) {
            m_scheduledCreatures[i].creature = nullptr;
            m_scheduledCreatures[i].needsUpdate = false;
            continue;
        }

        ScheduledCreature& sc = m_scheduledCreatures[i];
        sc.creature = creature;
        sc.index = i;

        // Calculate distance to camera
        glm::vec3 pos = creature->getPosition();
        glm::vec3 diff = pos - cameraPosition;
        sc.distance = glm::length(diff);

        // Check visibility
        float boundingRadius = creature->getSize() * 2.0f;
        sc.isVisible = isInFrustum(pos, boundingRadius, viewProjection);

        // Calculate importance
        sc.importance = calculateImportance(creature, selectedIndex);

        // Determine tier
        sc.tier = calculateTier(sc.distance, sc.importance, sc.isVisible);

        // Check if should update this frame
        sc.needsUpdate = shouldUpdateThisFrame(sc.tier);

        // Accumulate delta time for interpolation
        if (!sc.needsUpdate) {
            sc.accumulatedDeltaTime += m_lastFrameTimeMs / 1000.0f;
        } else {
            sc.accumulatedDeltaTime = 0.0f;
        }

        // Add to appropriate batch
        m_batches[static_cast<size_t>(sc.tier)].creatures.push_back(&sc);
        m_stats.countByTier[static_cast<size_t>(sc.tier)]++;
        m_stats.totalCreatures++;

        if (sc.needsUpdate) {
            m_updateList.push_back(&sc);
            m_stats.updatesThisFrame[static_cast<size_t>(sc.tier)]++;
            m_stats.totalUpdates++;
        }
    }

    // Sort update list by tier (critical first) then by distance (closest first)
    std::sort(m_updateList.begin(), m_updateList.end(),
        [](const ScheduledCreature* a, const ScheduledCreature* b) {
            if (a->tier != b->tier) {
                return static_cast<int>(a->tier) < static_cast<int>(b->tier);
            }
            return a->distance < b->distance;
        });

    // Set batch budgets
    m_batches[static_cast<size_t>(UpdateTier::CRITICAL)].budgetMs =
        m_config.criticalBudgetMs * m_budgetScale;
    m_batches[static_cast<size_t>(UpdateTier::HIGH)].budgetMs =
        m_config.highBudgetMs * m_budgetScale;
    m_batches[static_cast<size_t>(UpdateTier::MEDIUM)].budgetMs =
        m_config.mediumBudgetMs * m_budgetScale;
    m_batches[static_cast<size_t>(UpdateTier::LOW)].budgetMs =
        m_config.lowBudgetMs * m_budgetScale;
    m_batches[static_cast<size_t>(UpdateTier::MINIMAL)].budgetMs =
        0.5f * m_budgetScale;
    m_batches[static_cast<size_t>(UpdateTier::DORMANT)].budgetMs =
        0.25f * m_budgetScale;

    m_stats.budgetScale = m_budgetScale;
}

// ============================================================================
// Execute Updates
// ============================================================================

void CreatureUpdateScheduler::executeUpdates(float deltaTime) {
    auto startTime = std::chrono::high_resolution_clock::now();

    // Execute each tier
    for (size_t i = 0; i < static_cast<size_t>(UpdateTier::COUNT); ++i) {
        executeTier(static_cast<UpdateTier>(i), deltaTime);
    }

    // Calculate total time
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    m_lastFrameTimeMs = duration.count() / 1000.0f;
    m_stats.totalTimeMs = m_lastFrameTimeMs;

    // Calculate efficiency metrics
    if (m_stats.totalUpdates > 0) {
        m_stats.updateRate = static_cast<float>(m_stats.totalUpdates) / m_stats.totalCreatures;
        m_stats.avgUpdateTimeUs = (m_stats.totalTimeMs * 1000.0f) / m_stats.totalUpdates;
    }
}

void CreatureUpdateScheduler::executeTier(UpdateTier tier, float deltaTime) {
    size_t tierIndex = static_cast<size_t>(tier);
    UpdateBatch& batch = m_batches[tierIndex];

    if (batch.creatures.empty()) return;

    m_tierStart = std::chrono::high_resolution_clock::now();

    // Calculate effective delta time based on tier
    // Lower tiers get multiplied delta to compensate for skipped frames
    float effectiveDelta = deltaTime;
    switch (tier) {
        case UpdateTier::CRITICAL:
        case UpdateTier::HIGH:
            effectiveDelta = deltaTime;
            break;
        case UpdateTier::MEDIUM:
            effectiveDelta = deltaTime * m_config.mediumInterval;
            break;
        case UpdateTier::LOW:
            effectiveDelta = deltaTime * m_config.lowInterval;
            break;
        case UpdateTier::MINIMAL:
            effectiveDelta = deltaTime * m_config.minimalInterval;
            break;
        case UpdateTier::DORMANT:
            effectiveDelta = deltaTime * m_config.dormantInterval;
            break;
        default:
            break;
    }

    // Process creatures in this tier
    for (ScheduledCreature* sc : batch.creatures) {
        if (!sc->needsUpdate) continue;
        if (!sc->creature || !sc->creature->isActive()) continue;

        // Add accumulated time to effective delta
        float totalDelta = effectiveDelta + sc->accumulatedDeltaTime;

        // Perform the update (creature's own update method handles the details)
        // This is a placeholder - actual update happens in CreatureManager
        // We just mark it as updated here
        sc->lastUpdateTime = static_cast<float>(m_frameNumber) / 60.0f;
        sc->accumulatedDeltaTime = 0.0f;

        batch.actualCount++;

        // Check time budget (only for lower priority tiers)
        if (tier > UpdateTier::HIGH && m_config.adaptiveScheduling) {
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - m_tierStart);
            float elapsedMs = elapsed.count() / 1000.0f;

            if (elapsedMs > batch.budgetMs) {
                break;  // Budget exceeded, stop processing this tier
            }
        }
    }

    // Record tier time
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - m_tierStart);
    batch.usedMs = duration.count() / 1000.0f;
    m_stats.timeByTier[tierIndex] = batch.usedMs;
}

// ============================================================================
// Importance Calculation
// ============================================================================

float CreatureUpdateScheduler::calculateImportance(const Creature* creature,
                                                    size_t selectedIndex) const {
    float importance = 1.0f;

    // Selected creature is always critical
    if (creature->getID() == static_cast<int>(selectedIndex)) {
        importance *= m_config.selectedImportanceBoost * 10.0f;  // Always critical
    }

    // Predators are more important (interesting behavior)
    CreatureType type = creature->getType();
    if (isPredator(type)) {
        importance *= m_config.predatorImportanceBoost;
    }

    // Creatures ready to reproduce
    if (creature->getEnergy() > 150.0f && creature->getAge() > 20.0f) {
        importance *= m_config.reproducingBoost;
    }

    // Low energy creatures (about to die - dramatic)
    if (creature->getEnergy() < 30.0f) {
        importance *= 1.2f;
    }

    // Flying creatures are more visible
    if (isFlying(type)) {
        importance *= 1.1f;
    }

    return importance;
}

// ============================================================================
// Adaptive Budget
// ============================================================================

void CreatureUpdateScheduler::updateAdaptiveBudget() {
    // Scale budget based on last frame time vs target
    float ratio = m_lastFrameTimeMs / m_config.targetFrameTimeMs;

    if (ratio > 1.1f) {
        // Exceeded budget by >10%, reduce scale
        m_budgetScale = std::max(m_config.budgetScaleMin, m_budgetScale * 0.95f);
    } else if (ratio < 0.8f) {
        // Under budget by >20%, can increase
        m_budgetScale = std::min(m_config.budgetScaleMax, m_budgetScale * 1.02f);
    }
}

} // namespace Forge
