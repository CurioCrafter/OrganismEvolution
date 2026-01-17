#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "../entities/CreatureType.h"

class ProducerSystem;
class SeasonManager;

// Represents a dead creature being decomposed
struct Corpse {
    glm::vec3 position;
    float biomass;              // Remaining energy/mass to decompose
    float initialBiomass;       // Original biomass for progress tracking
    float age;                  // Time since death
    CreatureType sourceType;    // What type of creature died
    float size;                 // Size of the deceased creature

    // Scavenger interaction
    bool beingScavenged;
    float scavengedAmount;      // Amount removed by scavengers

    Corpse(const glm::vec3& pos, CreatureType type, float creatureSize, float energy)
        : position(pos)
        , biomass(energy * 0.5f)  // 50% of energy becomes decomposable biomass
        , initialBiomass(energy * 0.5f)
        , age(0.0f)
        , sourceType(type)
        , size(creatureSize)
        , beingScavenged(false)
        , scavengedAmount(0.0f)
    {}

    float getDecompositionProgress() const {
        return 1.0f - (biomass / initialBiomass);
    }

    bool isFullyDecomposed() const {
        return biomass <= 0.1f;
    }
};

class DecomposerSystem {
public:
    DecomposerSystem(ProducerSystem* producerSystem);

    void update(float deltaTime, const SeasonManager* seasonMgr);

    // Called when a creature dies
    void addCorpse(const glm::vec3& position, CreatureType type, float size, float energy);

    // Called by scavengers to consume corpse
    float scavengeCorpse(const glm::vec3& position, float amount);

    // Get corpse positions for scavengers
    std::vector<glm::vec3> getCorpsePositions() const;
    const std::vector<Corpse>& getCorpses() const { return corpses; }

    // Find nearest corpse within range
    Corpse* findNearestCorpse(const glm::vec3& position, float range);

    // Statistics
    int getCorpseCount() const { return static_cast<int>(corpses.size()); }
    float getTotalBiomass() const;
    float getDecompositionRate() const { return currentDecompositionRate; }

    // Enhanced scavenger loop - combines corpses + detritus for scavengers
    float getCarrionDensity(const glm::vec3& position, float radius) const;
    std::vector<glm::vec3> getScavengingTargets() const;  // All targets (corpses + detritus hotspots)
    float getTotalCarrionBiomass() const;  // Corpses + detritus for ecosystem metrics

    // Nutrient feedback rate (how effectively decomposition feeds producers)
    float getNutrientFeedbackRate() const { return nutrientFeedbackRate; }
    void setNutrientFeedbackRate(float rate) { nutrientFeedbackRate = rate; }

private:
    ProducerSystem* producerSystem;
    std::vector<Corpse> corpses;
    float nutrientFeedbackRate = 1.0f;  // Multiplier for nutrient release to soil

    // Base decomposition parameters
    float baseDecompositionRate;    // Biomass units decomposed per second
    float currentDecompositionRate; // Adjusted for season/conditions

    // Nutrient release ratios (what fraction of biomass becomes nutrients)
    static constexpr float nitrogenRatio = 0.3f;
    static constexpr float phosphorusRatio = 0.1f;
    static constexpr float organicMatterRatio = 0.5f;

    void decomposeCorpse(Corpse& corpse, float deltaTime, float seasonMult);
    void releaseNutrients(const glm::vec3& position, float decomposedAmount);
    void removeDecomposedCorpses();
};
