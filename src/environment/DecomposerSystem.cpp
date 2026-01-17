#include "DecomposerSystem.h"
#include "ProducerSystem.h"
#include "SeasonManager.h"
#include <algorithm>
#include <cmath>

DecomposerSystem::DecomposerSystem(ProducerSystem* producerSystem)
    : producerSystem(producerSystem)
    , baseDecompositionRate(2.0f)  // 2 biomass units per second
    , currentDecompositionRate(2.0f)
{
}

void DecomposerSystem::update(float deltaTime, const SeasonManager* seasonMgr) {
    float seasonMult = 1.0f;
    if (seasonMgr) {
        seasonMult = seasonMgr->getDecompositionMultiplier();
    }
    currentDecompositionRate = baseDecompositionRate * seasonMult;

    // Update each corpse
    for (auto& corpse : corpses) {
        corpse.age += deltaTime;
        decomposeCorpse(corpse, deltaTime, seasonMult);
    }

    // Remove fully decomposed corpses
    removeDecomposedCorpses();
}

void DecomposerSystem::decomposeCorpse(Corpse& corpse, float deltaTime, float seasonMult) {
    if (corpse.isFullyDecomposed()) return;

    // Decomposition rate affected by:
    // 1. Season (temperature/moisture)
    // 2. Corpse size (larger = slower per unit)
    // 3. Soil moisture at location

    float sizeFactor = 1.0f / (0.5f + corpse.size * 0.5f);  // Smaller corpses decompose faster

    // Get local soil moisture if producer system available
    float moistureFactor = 1.0f;
    if (producerSystem) {
        const SoilTile& soil = producerSystem->getSoilAt(corpse.position);
        moistureFactor = 0.5f + soil.moisture / 200.0f;  // 0.5-1.0 based on moisture
    }

    float effectiveRate = baseDecompositionRate * seasonMult * sizeFactor * moistureFactor;
    float decomposed = effectiveRate * deltaTime;

    // Don't decompose more than available
    decomposed = std::min(decomposed, corpse.biomass);
    corpse.biomass -= decomposed;

    // Release nutrients to soil
    if (decomposed > 0.0f) {
        releaseNutrients(corpse.position, decomposed);
    }
}

void DecomposerSystem::releaseNutrients(const glm::vec3& position, float decomposedAmount) {
    if (!producerSystem) return;

    // Apply nutrient feedback rate for stronger/weaker nutrient cycling
    float feedbackAmount = decomposedAmount * nutrientFeedbackRate;

    float nitrogen = feedbackAmount * nitrogenRatio;
    float phosphorus = feedbackAmount * phosphorusRatio;
    float organicMatter = feedbackAmount * organicMatterRatio;

    producerSystem->addNutrients(position, nitrogen, phosphorus, organicMatter);

    // Also add some detritus (partially decomposed material feeds the detritus pool)
    float detritusContribution = feedbackAmount * 0.15f;
    producerSystem->addDetritus(position, detritusContribution);
}

void DecomposerSystem::removeDecomposedCorpses() {
    corpses.erase(
        std::remove_if(corpses.begin(), corpses.end(),
            [](const Corpse& c) { return c.isFullyDecomposed(); }),
        corpses.end()
    );
}

void DecomposerSystem::addCorpse(const glm::vec3& position, CreatureType type, float size, float energy) {
    // Minimum energy threshold for creating a corpse
    if (energy < 10.0f) return;

    corpses.emplace_back(position, type, size, energy);
}

float DecomposerSystem::scavengeCorpse(const glm::vec3& position, float amount) {
    Corpse* corpse = findNearestCorpse(position, 5.0f);

    if (!corpse || corpse->biomass < 0.1f) {
        return 0.0f;
    }

    corpse->beingScavenged = true;
    float scavenged = std::min(amount, corpse->biomass);
    corpse->biomass -= scavenged;
    corpse->scavengedAmount += scavenged;

    // Return energy gained (scavengers get more energy per biomass than decomposition)
    return scavenged * 3.0f;  // 3 energy per biomass unit scavenged
}

Corpse* DecomposerSystem::findNearestCorpse(const glm::vec3& position, float range) {
    Corpse* nearest = nullptr;
    float nearestDist = range;

    for (auto& corpse : corpses) {
        if (corpse.isFullyDecomposed()) continue;

        float dist = glm::length(glm::vec2(corpse.position.x - position.x,
                                           corpse.position.z - position.z));
        if (dist < nearestDist) {
            nearestDist = dist;
            nearest = &corpse;
        }
    }

    return nearest;
}

std::vector<glm::vec3> DecomposerSystem::getCorpsePositions() const {
    std::vector<glm::vec3> positions;
    for (const auto& corpse : corpses) {
        if (!corpse.isFullyDecomposed()) {
            positions.push_back(corpse.position);
        }
    }
    return positions;
}

float DecomposerSystem::getTotalBiomass() const {
    float total = 0.0f;
    for (const auto& corpse : corpses) {
        total += corpse.biomass;
    }
    return total;
}

// ============================================================================
// Enhanced Scavenger Loop
// ============================================================================

float DecomposerSystem::getCarrionDensity(const glm::vec3& position, float radius) const {
    float corpseBiomass = 0.0f;
    float radiusSq = radius * radius;

    // Sum nearby corpse biomass
    for (const auto& corpse : corpses) {
        if (corpse.isFullyDecomposed()) continue;
        float dx = corpse.position.x - position.x;
        float dz = corpse.position.z - position.z;
        float distSq = dx * dx + dz * dz;
        if (distSq <= radiusSq) {
            // Weight by distance (closer = more dense)
            float influence = 1.0f - std::sqrt(distSq) / radius;
            corpseBiomass += corpse.biomass * influence;
        }
    }

    // Add detritus contribution if producer system available
    float detritusValue = 0.0f;
    if (producerSystem) {
        detritusValue = producerSystem->getDetritusAt(position, radius) * 0.5f;  // Detritus worth less than fresh carrion
    }

    return corpseBiomass + detritusValue;
}

std::vector<glm::vec3> DecomposerSystem::getScavengingTargets() const {
    std::vector<glm::vec3> targets;

    // Add all corpse positions
    for (const auto& corpse : corpses) {
        if (!corpse.isFullyDecomposed() && corpse.biomass > 1.0f) {
            targets.push_back(corpse.position);
        }
    }

    // Add detritus hotspots from producer system
    if (producerSystem) {
        auto hotspots = producerSystem->getDetritusHotspots();
        targets.insert(targets.end(), hotspots.begin(), hotspots.end());
    }

    return targets;
}

float DecomposerSystem::getTotalCarrionBiomass() const {
    float total = getTotalBiomass();

    // Add average detritus (rough estimate)
    if (producerSystem) {
        // Sample a few points to estimate total detritus
        // In practice this would be cached for performance
        total += producerSystem->getDetritusAt(glm::vec3(0, 0, 0), 100.0f) * 10.0f;
    }

    return total;
}
