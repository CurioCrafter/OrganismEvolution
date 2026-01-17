#include "EcosystemBehaviors.h"
#include "Creature.h"
#include "../environment/ProducerSystem.h"
#include "../environment/DecomposerSystem.h"
#include "../environment/EcosystemMetrics.h"
#include <algorithm>
#include <cmath>

std::vector<FoodSource> EcosystemBehaviors::getAvailableFood(
    CreatureType creatureType,
    const glm::vec3& position,
    float visionRange,
    const ProducerSystem* producers,
    const DecomposerSystem* decomposers)
{
    std::vector<FoodSource> sources;

    if (!producers && !decomposers) return sources;

    CreatureTraits traits = CreatureTraits::getTraitsFor(creatureType);

    // Grazers get grass
    if (traits.canDigest[0] && producers) {
        auto grassPositions = producers->getGrassPositions();
        for (const auto& pos : grassPositions) {
            float dist = glm::length(glm::vec2(pos.x - position.x, pos.z - position.z));
            if (dist < visionRange) {
                FoodSource source;
                source.position = pos;
                source.type = FoodSourceType::GRASS;
                source.energyValue = 10.0f;  // Approximate
                source.distance = dist;
                sources.push_back(source);
            }
        }
    }

    // Browsers get leaves
    if (traits.canDigest[1] && producers) {
        auto leafPositions = producers->getTreeLeafPositions();
        for (const auto& pos : leafPositions) {
            float dist = glm::length(glm::vec2(pos.x - position.x, pos.z - position.z));
            if (dist < visionRange) {
                FoodSource source;
                source.position = pos;
                source.type = FoodSourceType::TREE_LEAF;
                source.energyValue = 20.0f;
                source.distance = dist;
                sources.push_back(source);
            }
        }

        auto bushPositions = producers->getBushPositions();
        for (const auto& pos : bushPositions) {
            float dist = glm::length(glm::vec2(pos.x - position.x, pos.z - position.z));
            if (dist < visionRange) {
                FoodSource source;
                source.position = pos;
                source.type = FoodSourceType::BUSH_BERRY;
                source.energyValue = 25.0f;
                source.distance = dist;
                sources.push_back(source);
            }
        }
    }

    // Frugivores get fruit
    if (traits.canDigest[2] && producers) {
        auto fruitPositions = producers->getTreeFruitPositions();
        for (const auto& pos : fruitPositions) {
            float dist = glm::length(glm::vec2(pos.x - position.x, pos.z - position.z));
            if (dist < visionRange) {
                FoodSource source;
                source.position = pos;
                source.type = FoodSourceType::TREE_FRUIT;
                source.energyValue = 40.0f;
                source.distance = dist;
                sources.push_back(source);
            }
        }
    }

    // Scavengers get carrion
    if (traits.diet == DietType::CARRION && decomposers) {
        auto corpsePositions = decomposers->getCorpsePositions();
        for (const auto& pos : corpsePositions) {
            float dist = glm::length(glm::vec2(pos.x - position.x, pos.z - position.z));
            if (dist < visionRange) {
                FoodSource source;
                source.position = pos;
                source.type = FoodSourceType::CARRION;
                source.energyValue = 60.0f;
                source.distance = dist;
                sources.push_back(source);
            }
        }
    }

    // Sort by distance
    std::sort(sources.begin(), sources.end(),
        [](const FoodSource& a, const FoodSource& b) {
            return a.distance < b.distance;
        });

    return sources;
}

std::vector<Creature*> EcosystemBehaviors::getValidPrey(
    CreatureType predatorType,
    float predatorSize,
    const glm::vec3& position,
    float visionRange,
    const std::vector<Creature*>& creatures)
{
    std::vector<Creature*> prey;

    for (Creature* creature : creatures) {
        if (!creature || !creature->isAlive()) continue;

        float dist = glm::length(creature->getPosition() - position);
        if (dist > visionRange) continue;

        float preySize = creature->getGenome().size;

        if (canBeHuntedBy(creature->getType(), predatorType, preySize)) {
            prey.push_back(creature);
        }
    }

    // Sort by distance
    std::sort(prey.begin(), prey.end(),
        [&position](Creature* a, Creature* b) {
            return glm::length(a->getPosition() - position) <
                   glm::length(b->getPosition() - position);
        });

    return prey;
}

std::vector<Creature*> EcosystemBehaviors::getThreats(
    CreatureType creatureType,
    float creatureSize,
    const glm::vec3& position,
    float visionRange,
    const std::vector<Creature*>& creatures)
{
    std::vector<Creature*> threats;

    for (Creature* creature : creatures) {
        if (!creature || !creature->isAlive()) continue;

        float dist = glm::length(creature->getPosition() - position);
        if (dist > visionRange) continue;

        // Check if this creature can hunt us
        if (canBeHuntedBy(creatureType, creature->getType(), creatureSize)) {
            threats.push_back(creature);
        }
    }

    // Sort by distance (closest threat first)
    std::sort(threats.begin(), threats.end(),
        [&position](Creature* a, Creature* b) {
            return glm::length(a->getPosition() - position) <
                   glm::length(b->getPosition() - position);
        });

    return threats;
}

float EcosystemBehaviors::consumeProducerFood(
    CreatureType creatureType,
    const glm::vec3& position,
    float consumeRate,
    ProducerSystem* producers,
    EcosystemMetrics* metrics)
{
    if (!producers) return 0.0f;

    FoodSourceType preferredType = getPreferredFoodType(creatureType);
    float energyGained = producers->consumeAt(position, preferredType, consumeRate);

    // Apply trophic efficiency (herbivores get ~10-15% of plant energy)
    energyGained *= 0.12f;

    if (metrics && energyGained > 0) {
        metrics->recordEnergyToHerbivore(energyGained);
    }

    return energyGained;
}

float EcosystemBehaviors::consumeCarrion(
    const glm::vec3& position,
    float consumeRate,
    DecomposerSystem* decomposers,
    EcosystemMetrics* metrics)
{
    if (!decomposers) return 0.0f;

    float energyGained = decomposers->scavengeCorpse(position, consumeRate);

    if (metrics && energyGained > 0) {
        metrics->recordEnergyToDecomposer(energyGained);
    }

    return energyGained;
}

void EcosystemBehaviors::getReproductionRequirements(
    CreatureType type,
    float& energyThreshold,
    float& energyCost,
    int& requiredKills)
{
    switch (type) {
        case CreatureType::GRAZER:
        case CreatureType::BROWSER:
        case CreatureType::FRUGIVORE:
            energyThreshold = 180.0f;
            energyCost = 80.0f;
            requiredKills = 0;
            break;

        case CreatureType::SMALL_PREDATOR:
            energyThreshold = 170.0f;
            energyCost = 90.0f;
            requiredKills = 1;
            break;

        case CreatureType::OMNIVORE:
            energyThreshold = 175.0f;
            energyCost = 85.0f;
            requiredKills = 0;  // Can reproduce without kills
            break;

        case CreatureType::APEX_PREDATOR:
            energyThreshold = 170.0f;
            energyCost = 100.0f;
            requiredKills = 2;
            break;

        case CreatureType::SCAVENGER:
            energyThreshold = 160.0f;
            energyCost = 70.0f;
            requiredKills = 0;
            break;

        case CreatureType::PARASITE:
            energyThreshold = 150.0f;
            energyCost = 50.0f;
            requiredKills = 0;
            break;

        case CreatureType::CLEANER:
            energyThreshold = 160.0f;
            energyCost = 60.0f;
            requiredKills = 0;
            break;

        default:
            energyThreshold = 180.0f;
            energyCost = 80.0f;
            requiredKills = 0;
            break;
    }
}

float EcosystemBehaviors::getMetabolismRate(CreatureType type, float size, float efficiency) {
    float baseRate = 0.5f + size * 0.3f;

    // Predators have higher metabolism
    if (isPredator(type)) {
        baseRate *= 1.3f;
    }

    // Parasites have very low metabolism
    if (type == CreatureType::PARASITE) {
        baseRate *= 0.5f;
    }

    return baseRate * efficiency;
}

bool EcosystemBehaviors::isPreyFor(CreatureType prey, CreatureType predator) {
    // Use the helper function from CreatureType.h
    return canBeHuntedBy(prey, predator, 1.0f);  // Default size
}

bool EcosystemBehaviors::shouldFormHerd(CreatureType type) {
    return type == CreatureType::GRAZER ||
           type == CreatureType::BROWSER;
}

bool EcosystemBehaviors::shouldFormPack(CreatureType type) {
    return type == CreatureType::APEX_PREDATOR;
}

float EcosystemBehaviors::getTerritoryRadius(CreatureType type, float size) {
    CreatureTraits traits = CreatureTraits::getTraitsFor(type);
    if (!traits.isTerritorial) return 0.0f;

    // Territory size scales with creature size
    return 20.0f + size * 15.0f;
}

bool EcosystemBehaviors::shouldHuntInsteadOfGraze(
    CreatureType type,
    float energy,
    float maxEnergy,
    int nearbyPrey,
    int nearbyPlants)
{
    if (type != CreatureType::OMNIVORE) return false;

    // Type III functional response - hunt more when prey is abundant
    float energyRatio = energy / maxEnergy;

    // Low energy: prefer safe plant food
    if (energyRatio < 0.3f) {
        return false;
    }

    // High energy and abundant prey: hunt
    if (energyRatio > 0.5f && nearbyPrey >= 3) {
        return true;
    }

    // Few plants available: hunt
    if (nearbyPlants < 2 && nearbyPrey > 0) {
        return true;
    }

    return false;
}

Creature* EcosystemBehaviors::findBestHost(
    const glm::vec3& position,
    float visionRange,
    const std::vector<Creature*>& creatures)
{
    Creature* bestHost = nullptr;
    float bestScore = 0.0f;

    for (Creature* creature : creatures) {
        if (!creature || !creature->isAlive()) continue;
        if (creature->getType() == CreatureType::PARASITE) continue;
        if (creature->getType() == CreatureType::CLEANER) continue;

        float dist = glm::length(creature->getPosition() - position);
        if (dist > visionRange) continue;

        // Score based on: energy (more = better), size (larger = more energy), distance
        float score = creature->getEnergy() * creature->getGenome().size / (dist + 1.0f);

        // Prefer hosts with low parasite resistance
        CreatureTraits traits = CreatureTraits::getTraitsFor(creature->getType());
        score *= (1.0f - traits.parasiteResistance * 0.5f);

        if (score > bestScore) {
            bestScore = score;
            bestHost = creature;
        }
    }

    return bestHost;
}

float EcosystemBehaviors::drainHost(Creature* host, float drainRate, float deltaTime) {
    if (!host || !host->isAlive()) return 0.0f;

    float drained = std::min(drainRate * deltaTime, host->getEnergy() * 0.1f);
    host->takeDamage(drained);

    return drained * 0.8f;  // 80% efficiency
}

Creature* EcosystemBehaviors::findParasitizedHost(
    const glm::vec3& position,
    float visionRange,
    const std::vector<Creature*>& creatures)
{
    // Cleaners look for creatures with parasites
    // Since we track parasites in EcosystemState, this would need integration
    // For now, find any larger creature that might need cleaning

    Creature* bestTarget = nullptr;
    float nearestDist = visionRange;

    for (Creature* creature : creatures) {
        if (!creature || !creature->isAlive()) continue;
        if (creature->getType() == CreatureType::PARASITE) continue;
        if (creature->getType() == CreatureType::CLEANER) continue;

        // Prefer larger creatures (more likely to have parasites)
        if (creature->getGenome().size < 1.0f) continue;

        float dist = glm::length(creature->getPosition() - position);
        if (dist < nearestDist) {
            nearestDist = dist;
            bestTarget = creature;
        }
    }

    return bestTarget;
}

float EcosystemBehaviors::cleanHost(Creature* host, float cleanRate, float deltaTime) {
    if (!host || !host->isAlive()) return 0.0f;

    // Cleaning provides energy to cleaner and small health boost to host
    float energyGained = cleanRate * deltaTime * 2.0f;

    // Host gets tiny energy boost from cleaning service
    host->consumeFood(cleanRate * deltaTime * 0.5f);

    return energyGained;
}

FoodSourceType EcosystemBehaviors::getPreferredFoodType(CreatureType type) {
    switch (type) {
        case CreatureType::GRAZER:
            return FoodSourceType::GRASS;
        case CreatureType::BROWSER:
            return FoodSourceType::TREE_LEAF;
        case CreatureType::FRUGIVORE:
            return FoodSourceType::TREE_FRUIT;
        case CreatureType::OMNIVORE:
            return FoodSourceType::BUSH_BERRY;  // Default plant food for omnivores
        default:
            return FoodSourceType::GRASS;
    }
}
