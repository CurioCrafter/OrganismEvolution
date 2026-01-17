#pragma once

#include "Species.h"
#include "MateSelector.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

class Creature;
class Terrain;

namespace genetics {

// Zone type
enum class HybridZoneType {
    TENSION,           // Maintained by selection against hybrids + dispersal
    BOUNDED_HYBRID,    // Hybrids favored in intermediate environment
    MOSAIC,            // Patchy environment with parental types in different patches
    PARAPATRIC         // Along environmental gradient
};

// Hybrid zone statistics
struct HybridZoneStats {
    int totalHybrids;
    int generation1Hybrids;  // F1 hybrids
    int backCrosses;         // Backcrosses to parent species
    float averageHybridFitness;
    float introgression;     // Gene flow between species
    float zoneWidth;         // Geographic width of the zone

    HybridZoneStats()
        : totalHybrids(0), generation1Hybrids(0), backCrosses(0),
          averageHybridFitness(0.0f), introgression(0.0f), zoneWidth(0.0f) {}
};

// Represents a hybrid zone between two species
class HybridZone {
public:
    HybridZone();
    HybridZone(SpeciesId sp1, SpeciesId sp2, const glm::vec3& center, float radius);

    // Getters
    SpeciesId getSpecies1() const { return species1; }
    SpeciesId getSpecies2() const { return species2; }
    const glm::vec3& getCenter() const { return center; }
    float getRadius() const { return radius; }
    HybridZoneType getType() const { return type; }
    const HybridZoneStats& getStats() const { return stats; }
    int getCreatedGeneration() const { return createdGeneration; }
    bool isActive() const { return active; }

    // Setters
    void setType(HybridZoneType t) { type = t; }
    void setCreatedGeneration(int gen) { createdGeneration = gen; }
    void deactivate() { active = false; }

    // Check if a position is within the hybrid zone
    bool contains(const glm::vec3& position) const;

    // Update the zone based on current creature distribution
    void update(const std::vector<Creature*>& creatures, int generation);

    // Calculate hybrid fitness modifier based on position in zone
    float calculateHybridFitness(const glm::vec3& position, const Terrain* terrain = nullptr) const;

    // Get the probability of interspecific mating at a position
    float getInterbreedingProbability(const glm::vec3& position) const;

    // Check if zone should be deactivated (species merged or separated)
    bool shouldDeactivate() const;

private:
    SpeciesId species1, species2;
    glm::vec3 center;
    float radius;
    HybridZoneType type;
    HybridZoneStats stats;
    int createdGeneration;
    bool active;

    // Zone dynamics
    void updateCenter(const std::vector<Creature*>& creatures);
    void calculateZoneWidth(const std::vector<Creature*>& creatures);
    void countHybrids(const std::vector<Creature*>& creatures);
};

// Manager for tracking all hybrid zones
class HybridZoneManager {
public:
    HybridZoneManager();

    // Update all zones
    void update(const std::vector<Creature*>& creatures,
                const SpeciationTracker& specTracker,
                int generation);

    // Detect potential new hybrid zones
    void detectNewZones(const std::vector<Creature*>& creatures,
                        const SpeciationTracker& specTracker,
                        int generation);

    // Get zones
    const std::vector<std::unique_ptr<HybridZone>>& getZones() const { return zones; }
    HybridZone* getZone(SpeciesId sp1, SpeciesId sp2);

    // Check if position is in any hybrid zone
    bool isInHybridZone(const glm::vec3& position) const;

    // Get active zone count
    int getActiveZoneCount() const;

    // Handle hybrid creation
    std::unique_ptr<Creature> attemptHybridMating(
        Creature& parent1, Creature& parent2,
        const MateSelector& mateSelector,
        int generation);

    // Configuration
    void setZoneDetectionRadius(float radius) { zoneDetectionRadius = radius; }
    void setMinSpeciesOverlap(int overlap) { minSpeciesOverlap = overlap; }

private:
    std::vector<std::unique_ptr<HybridZone>> zones;
    float zoneDetectionRadius;
    int minSpeciesOverlap;

    // Find overlapping populations
    struct OverlapInfo {
        SpeciesId sp1, sp2;
        glm::vec3 center;
        float radius;
        int count;
    };

    std::vector<OverlapInfo> findOverlappingPopulations(
        const std::vector<Creature*>& creatures,
        const SpeciationTracker& specTracker);

    // Clean up inactive zones
    void removeInactiveZones();
};

} // namespace genetics
