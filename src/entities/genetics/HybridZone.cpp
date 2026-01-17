#include "HybridZone.h"
#include "../Creature.h"
#include "../../environment/Terrain.h"
#include "../../utils/Random.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace genetics {

// HybridZone implementation
HybridZone::HybridZone()
    : species1(0), species2(0), center(0.0f), radius(20.0f),
      type(HybridZoneType::TENSION), createdGeneration(0), active(true) {
}

HybridZone::HybridZone(SpeciesId sp1, SpeciesId sp2, const glm::vec3& center, float radius)
    : species1(sp1), species2(sp2), center(center), radius(radius),
      type(HybridZoneType::TENSION), createdGeneration(0), active(true) {
}

bool HybridZone::contains(const glm::vec3& position) const {
    float dist = glm::length(glm::vec2(position.x - center.x, position.z - center.z));
    return dist <= radius;
}

void HybridZone::update(const std::vector<Creature*>& creatures, int generation) {
    if (!active) return;

    // Update zone center based on hybrid distribution
    updateCenter(creatures);

    // Calculate zone width
    calculateZoneWidth(creatures);

    // Count and categorize hybrids
    countHybrids(creatures);

    // Check if zone should be deactivated
    if (shouldDeactivate()) {
        active = false;
        std::cout << "[HYBRID ZONE] Zone between Species_" << species1
                  << " and Species_" << species2 << " dissolved" << std::endl;
    }
}

float HybridZone::calculateHybridFitness(const glm::vec3& position, const Terrain* terrain) const {
    if (!active) return 1.0f;

    float distFromCenter = glm::length(glm::vec2(position.x - center.x, position.z - center.z));
    float relativePos = distFromCenter / radius;

    switch (type) {
        case HybridZoneType::TENSION:
            // Hybrids have lower fitness in the zone
            return 0.7f + 0.3f * relativePos;  // Lower at center

        case HybridZoneType::BOUNDED_HYBRID:
            // Hybrids have higher fitness at center of zone
            return 0.8f + 0.4f * (1.0f - relativePos);  // Higher at center

        case HybridZoneType::MOSAIC:
            // Patchy - fitness depends on local environment
            if (terrain) {
                float elevation = terrain->getHeight(position.x, position.z);
                // Hybrids better at intermediate elevations
                float optimalElev = 10.0f;  // Middle elevation
                float elevDiff = std::abs(elevation - optimalElev) / 20.0f;
                return 0.6f + 0.4f * (1.0f - elevDiff);
            }
            return 0.8f;

        case HybridZoneType::PARAPATRIC:
            // Along gradient - fitness varies with position
            return 0.7f + 0.2f * std::sin(relativePos * 3.14159f);

        default:
            return 1.0f;
    }
}

float HybridZone::getInterbreedingProbability(const glm::vec3& position) const {
    if (!active || !contains(position)) return 0.0f;

    float distFromCenter = glm::length(glm::vec2(position.x - center.x, position.z - center.z));
    float relativePos = distFromCenter / radius;

    // Higher probability of interbreeding near center of zone
    return std::max(0.0f, 1.0f - relativePos);
}

bool HybridZone::shouldDeactivate() const {
    // Deactivate if no hybrids for a while
    if (stats.totalHybrids == 0 && createdGeneration > 0) {
        return true;
    }

    // Deactivate if zone has collapsed (too narrow)
    if (stats.zoneWidth < 5.0f) {
        return true;
    }

    return false;
}

void HybridZone::updateCenter(const std::vector<Creature*>& creatures) {
    glm::vec3 hybridCenter(0.0f);
    int hybridCount = 0;

    for (Creature* c : creatures) {
        if (!c || !c->isAlive()) continue;
        if (!c->getDiploidGenome().isHybrid()) continue;

        // Check if this hybrid is from our species pair
        // (simplified - in full implementation would track parent species)
        if (contains(c->getPosition())) {
            hybridCenter += c->getPosition();
            hybridCount++;
        }
    }

    if (hybridCount > 0) {
        // Smooth update of center
        glm::vec3 newCenter = hybridCenter / static_cast<float>(hybridCount);
        center = center * 0.8f + newCenter * 0.2f;
    }
}

void HybridZone::calculateZoneWidth(const std::vector<Creature*>& creatures) {
    float minDist = std::numeric_limits<float>::max();
    float maxDist = 0.0f;

    for (Creature* c : creatures) {
        if (!c || !c->isAlive()) continue;

        SpeciesId spId = c->getDiploidGenome().getSpeciesId();
        if (spId != species1 && spId != species2) continue;

        float dist = glm::length(glm::vec2(c->getPosition().x - center.x,
                                           c->getPosition().z - center.z));

        // Track distribution of each species
        if (spId == species1) {
            minDist = std::min(minDist, dist);
        } else {
            maxDist = std::max(maxDist, dist);
        }
    }

    // Zone width is the overlap region
    stats.zoneWidth = std::max(0.0f, maxDist - minDist);
}

void HybridZone::countHybrids(const std::vector<Creature*>& creatures) {
    stats.totalHybrids = 0;
    stats.generation1Hybrids = 0;
    stats.backCrosses = 0;
    float totalFitness = 0.0f;

    for (Creature* c : creatures) {
        if (!c || !c->isAlive()) continue;
        if (!c->getDiploidGenome().isHybrid()) continue;

        if (contains(c->getPosition())) {
            stats.totalHybrids++;
            totalFitness += c->getFitness();

            // Categorize hybrid type (simplified)
            float inbreeding = c->getDiploidGenome().calculateInbreedingCoeff();
            if (inbreeding < 0.2f) {
                stats.generation1Hybrids++;  // F1 (high heterozygosity)
            } else {
                stats.backCrosses++;  // Backcross or later generation
            }
        }
    }

    if (stats.totalHybrids > 0) {
        stats.averageHybridFitness = totalFitness / stats.totalHybrids;
    } else {
        stats.averageHybridFitness = 0.0f;
    }
}

// HybridZoneManager implementation
HybridZoneManager::HybridZoneManager()
    : zoneDetectionRadius(30.0f), minSpeciesOverlap(5) {
}

void HybridZoneManager::update(const std::vector<Creature*>& creatures,
                                const SpeciationTracker& specTracker,
                                int generation) {
    // Update existing zones
    for (auto& zone : zones) {
        if (zone->isActive()) {
            zone->update(creatures, generation);
        }
    }

    // Detect new zones
    detectNewZones(creatures, specTracker, generation);

    // Remove inactive zones
    removeInactiveZones();
}

void HybridZoneManager::detectNewZones(const std::vector<Creature*>& creatures,
                                        const SpeciationTracker& specTracker,
                                        int generation) {
    auto overlaps = findOverlappingPopulations(creatures, specTracker);

    for (const auto& overlap : overlaps) {
        // Check if zone already exists
        if (getZone(overlap.sp1, overlap.sp2)) continue;

        // Check minimum overlap
        if (overlap.count < minSpeciesOverlap) continue;

        // Create new hybrid zone
        auto zone = std::make_unique<HybridZone>(
            overlap.sp1, overlap.sp2, overlap.center, overlap.radius);
        zone->setCreatedGeneration(generation);

        // Determine zone type based on environment
        // (simplified - could use terrain analysis)
        if (Random::chance(0.5f)) {
            zone->setType(HybridZoneType::TENSION);
        } else {
            zone->setType(HybridZoneType::BOUNDED_HYBRID);
        }

        std::cout << "[HYBRID ZONE] New zone detected between Species_"
                  << overlap.sp1 << " and Species_" << overlap.sp2
                  << " at (" << overlap.center.x << ", " << overlap.center.z
                  << ") radius " << overlap.radius << std::endl;

        zones.push_back(std::move(zone));
    }
}

HybridZone* HybridZoneManager::getZone(SpeciesId sp1, SpeciesId sp2) {
    for (auto& zone : zones) {
        if ((zone->getSpecies1() == sp1 && zone->getSpecies2() == sp2) ||
            (zone->getSpecies1() == sp2 && zone->getSpecies2() == sp1)) {
            return zone.get();
        }
    }
    return nullptr;
}

bool HybridZoneManager::isInHybridZone(const glm::vec3& position) const {
    for (const auto& zone : zones) {
        if (zone->isActive() && zone->contains(position)) {
            return true;
        }
    }
    return false;
}

int HybridZoneManager::getActiveZoneCount() const {
    int count = 0;
    for (const auto& zone : zones) {
        if (zone->isActive()) count++;
    }
    return count;
}

std::unique_ptr<Creature> HybridZoneManager::attemptHybridMating(
    Creature& parent1, Creature& parent2,
    const MateSelector& mateSelector,
    int generation) {

    const DiploidGenome& g1 = parent1.getDiploidGenome();
    const DiploidGenome& g2 = parent2.getDiploidGenome();

    // Check if different species
    if (g1.getSpeciesId() == g2.getSpeciesId()) {
        return nullptr;  // Same species, normal mating
    }

    // Calculate compatibility
    ReproductiveCompatibility compat = mateSelector.calculateCompatibility(g1, g2);

    // Check pre-mating barrier
    if (Random::value() < compat.preMatingBarrier) {
        return nullptr;  // Mating rejected
    }

    // Create hybrid genome
    DiploidGenome hybridGenome(g1, g2, true);  // isHybrid = true

    // Calculate spawn position
    glm::vec3 spawnPos = (parent1.getPosition() + parent2.getPosition()) * 0.5f;
    spawnPos.x += Random::range(-3.0f, 3.0f);
    spawnPos.z += Random::range(-3.0f, 3.0f);

    // Check for hybrid zone fitness modification
    float fitnessModifier = 1.0f - compat.postMatingBarrier;
    for (const auto& zone : zones) {
        if (zone->isActive() && zone->contains(spawnPos)) {
            float zoneMod = zone->calculateHybridFitness(spawnPos, nullptr);
            fitnessModifier *= zoneMod;

            // Potential hybrid vigor
            if (compat.hybridVigor > 0) {
                fitnessModifier += compat.hybridVigor;
            }
        }
    }

    // Create hybrid creature
    auto hybrid = std::make_unique<Creature>(spawnPos, hybridGenome, parent1.getType());

    // Set generation to max of parents + 1 (essential for evolution tracking)
    hybrid->setGeneration(std::max(parent1.getGeneration(), parent2.getGeneration()) + 1);

    // Apply fitness modifier to hybrid
    hybrid->setFitnessModifier(hybrid->getFitnessModifier() * fitnessModifier);

    // Check sterility - hybrids may be sterile depending on genetic compatibility
    if (Random::value() < compat.hybridSterility) {
        hybrid->setSterile(true);
    }

    std::cout << "[HYBRID] F1 hybrid created from Species_" << g1.getSpeciesId()
              << " x Species_" << g2.getSpeciesId()
              << " (fitness mod: " << fitnessModifier
              << ", sterile: " << (hybrid->isSterile() ? "yes" : "no") << ")" << std::endl;

    return hybrid;
}

std::vector<HybridZoneManager::OverlapInfo> HybridZoneManager::findOverlappingPopulations(
    const std::vector<Creature*>& creatures,
    const SpeciationTracker& specTracker) {

    std::vector<OverlapInfo> overlaps;

    // Group creatures by species and location
    std::map<SpeciesId, std::vector<Creature*>> bySpecies;
    for (Creature* c : creatures) {
        if (c && c->isAlive()) {
            bySpecies[c->getDiploidGenome().getSpeciesId()].push_back(c);
        }
    }

    // Check each pair of species for overlap
    std::vector<SpeciesId> speciesIds;
    for (const auto& [spId, members] : bySpecies) {
        if (spId > 0 && members.size() >= static_cast<size_t>(minSpeciesOverlap)) {
            speciesIds.push_back(spId);
        }
    }

    for (size_t i = 0; i < speciesIds.size(); i++) {
        for (size_t j = i + 1; j < speciesIds.size(); j++) {
            SpeciesId sp1 = speciesIds[i];
            SpeciesId sp2 = speciesIds[j];

            const auto& members1 = bySpecies[sp1];
            const auto& members2 = bySpecies[sp2];

            // Find overlapping individuals
            glm::vec3 overlapCenter(0.0f);
            int overlapCount = 0;

            for (Creature* c1 : members1) {
                for (Creature* c2 : members2) {
                    float dist = glm::length(c1->getPosition() - c2->getPosition());
                    if (dist < zoneDetectionRadius) {
                        overlapCenter += (c1->getPosition() + c2->getPosition()) * 0.5f;
                        overlapCount++;
                    }
                }
            }

            if (overlapCount >= minSpeciesOverlap) {
                OverlapInfo info;
                info.sp1 = sp1;
                info.sp2 = sp2;
                info.center = overlapCenter / static_cast<float>(overlapCount);
                info.count = overlapCount;

                // Calculate radius
                float maxDist = 0.0f;
                for (Creature* c1 : members1) {
                    float dist = glm::length(c1->getPosition() - info.center);
                    if (dist < zoneDetectionRadius) {
                        maxDist = std::max(maxDist, dist);
                    }
                }
                for (Creature* c2 : members2) {
                    float dist = glm::length(c2->getPosition() - info.center);
                    if (dist < zoneDetectionRadius) {
                        maxDist = std::max(maxDist, dist);
                    }
                }
                info.radius = maxDist * 1.2f;  // Add buffer

                overlaps.push_back(info);
            }
        }
    }

    return overlaps;
}

void HybridZoneManager::removeInactiveZones() {
    zones.erase(
        std::remove_if(zones.begin(), zones.end(),
            [](const std::unique_ptr<HybridZone>& zone) {
                return !zone->isActive();
            }),
        zones.end()
    );
}

} // namespace genetics
