#include "PlantCreatureInteraction.h"
#include "TreeGenerator.h"
#include "GrassSystem.h"
#include "AquaticPlants.h"
#include "FungiSystem.h"
#include "AlienVegetation.h"
#include "Terrain.h"
#include <algorithm>
#include <cmath>
#include <random>

// ============================================================
// FRUIT NUTRITION DATA
// ============================================================

FruitNutrition getFruitNutrition(FruitType type) {
    FruitNutrition nutrition = {};

    switch (type) {
        // Tree fruits - high calories, good hydration
        case FruitType::APPLE:
            nutrition = { 95.0f, 0.85f, 0.5f, 25.0f, 0.8f, 0.0f, 0.0f };
            break;
        case FruitType::PEAR:
            nutrition = { 100.0f, 0.84f, 0.6f, 27.0f, 0.7f, 0.0f, 0.0f };
            break;
        case FruitType::CHERRY:
            nutrition = { 50.0f, 0.82f, 1.0f, 12.0f, 0.9f, 0.0f, 0.0f };
            break;
        case FruitType::PLUM:
            nutrition = { 46.0f, 0.87f, 0.7f, 11.0f, 0.75f, 0.0f, 0.0f };
            break;
        case FruitType::PEACH:
            nutrition = { 59.0f, 0.89f, 1.4f, 14.0f, 0.85f, 0.0f, 0.0f };
            break;
        case FruitType::ORANGE:
            nutrition = { 62.0f, 0.87f, 1.2f, 15.0f, 0.95f, 0.0f, 0.0f };
            break;
        case FruitType::LEMON:
            nutrition = { 29.0f, 0.89f, 1.1f, 9.0f, 0.9f, 0.0f, 0.0f };
            break;
        case FruitType::MANGO:
            nutrition = { 135.0f, 0.84f, 1.1f, 35.0f, 0.95f, 0.0f, 0.0f };
            break;
        case FruitType::BANANA:
            nutrition = { 105.0f, 0.74f, 1.3f, 27.0f, 0.8f, 0.0f, 0.0f };
            break;
        case FruitType::COCONUT:
            nutrition = { 354.0f, 0.47f, 3.3f, 15.0f, 0.5f, 0.0f, 0.0f };
            break;
        case FruitType::FIG:
            nutrition = { 74.0f, 0.79f, 0.8f, 19.0f, 0.7f, 0.0f, 0.0f };
            break;
        case FruitType::DATE:
            nutrition = { 282.0f, 0.21f, 2.5f, 75.0f, 0.6f, 0.0f, 0.0f };
            break;
        case FruitType::OLIVE:
            nutrition = { 115.0f, 0.80f, 0.8f, 6.0f, 0.4f, 0.0f, 0.0f };
            break;
        case FruitType::AVOCADO:
            nutrition = { 240.0f, 0.73f, 3.0f, 12.0f, 0.7f, 0.0f, 0.0f };
            break;

        // Berries - moderate calories, high vitamins
        case FruitType::BERRY_RED:
            nutrition = { 32.0f, 0.87f, 0.7f, 7.0f, 0.95f, 0.0f, 0.0f };
            break;
        case FruitType::BERRY_BLUE:
            nutrition = { 57.0f, 0.84f, 0.7f, 14.0f, 0.98f, 0.0f, 0.0f };
            break;
        case FruitType::BERRY_BLACK:
            nutrition = { 43.0f, 0.88f, 1.4f, 10.0f, 0.9f, 0.0f, 0.0f };
            break;
        case FruitType::BERRY_PURPLE:
            nutrition = { 46.0f, 0.86f, 0.9f, 11.0f, 0.88f, 0.0f, 0.0f };
            break;
        case FruitType::BERRY_WHITE:
            nutrition = { 35.0f, 0.85f, 0.6f, 8.0f, 0.7f, 0.0f, 0.0f };
            break;
        case FruitType::BERRY_POISONOUS:
            nutrition = { 40.0f, 0.80f, 0.5f, 10.0f, 0.3f, 0.85f, 0.2f };
            break;

        // Nuts - high calories, high protein
        case FruitType::ACORN:
            nutrition = { 387.0f, 0.28f, 6.2f, 41.0f, 0.3f, 0.1f, 0.0f };
            break;
        case FruitType::WALNUT:
            nutrition = { 654.0f, 0.04f, 15.2f, 14.0f, 0.5f, 0.0f, 0.0f };
            break;
        case FruitType::CHESTNUT:
            nutrition = { 213.0f, 0.52f, 2.4f, 45.0f, 0.6f, 0.0f, 0.0f };
            break;
        case FruitType::PINE_NUT:
            nutrition = { 673.0f, 0.02f, 13.7f, 13.0f, 0.4f, 0.0f, 0.0f };
            break;
        case FruitType::HAZELNUT:
            nutrition = { 628.0f, 0.05f, 15.0f, 17.0f, 0.5f, 0.0f, 0.0f };
            break;

        // Seeds - low calories, good for dispersal
        case FruitType::SEED_SMALL:
            nutrition = { 15.0f, 0.10f, 2.0f, 3.0f, 0.2f, 0.0f, 0.0f };
            break;
        case FruitType::SEED_MEDIUM:
            nutrition = { 30.0f, 0.15f, 3.0f, 5.0f, 0.25f, 0.0f, 0.0f };
            break;
        case FruitType::SEED_LARGE:
            nutrition = { 50.0f, 0.20f, 4.0f, 8.0f, 0.3f, 0.0f, 0.0f };
            break;
        case FruitType::SEED_WINGED:
            nutrition = { 20.0f, 0.12f, 2.5f, 4.0f, 0.2f, 0.0f, 0.0f };
            break;
        case FruitType::SEED_FLUFFY:
            nutrition = { 10.0f, 0.08f, 1.5f, 2.0f, 0.15f, 0.0f, 0.0f };
            break;
        case FruitType::SEED_STICKY:
            nutrition = { 18.0f, 0.10f, 2.0f, 3.5f, 0.2f, 0.0f, 0.0f };
            break;
        case FruitType::SEED_FLOATING:
            nutrition = { 25.0f, 0.30f, 2.0f, 5.0f, 0.2f, 0.0f, 0.0f };
            break;

        // Alien fruits - exotic nutrition, some psychoactive
        case FruitType::GLOW_FRUIT:
            nutrition = { 80.0f, 0.75f, 2.0f, 20.0f, 0.5f, 0.0f, 0.3f };
            break;
        case FruitType::CRYSTAL_FRUIT:
            nutrition = { 50.0f, 0.60f, 1.0f, 15.0f, 0.8f, 0.1f, 0.2f };
            break;
        case FruitType::ENERGY_POD:
            nutrition = { 200.0f, 0.40f, 5.0f, 50.0f, 0.6f, 0.0f, 0.5f };
            break;
        case FruitType::VOID_BERRY:
            nutrition = { 30.0f, 0.50f, 3.0f, 8.0f, 0.3f, 0.3f, 0.8f };
            break;
        case FruitType::PLASMA_SEED:
            nutrition = { 150.0f, 0.30f, 8.0f, 30.0f, 0.4f, 0.2f, 0.6f };
            break;
        case FruitType::PSYCHIC_NUT:
            nutrition = { 100.0f, 0.20f, 10.0f, 20.0f, 0.5f, 0.1f, 0.95f };
            break;

        // Special
        case FruitType::SPORE_CLUSTER:
            nutrition = { 25.0f, 0.60f, 3.0f, 5.0f, 0.4f, 0.15f, 0.1f };
            break;
        case FruitType::NECTAR_DROP:
            nutrition = { 60.0f, 0.80f, 0.1f, 15.0f, 0.3f, 0.0f, 0.0f };
            break;

        default:
            nutrition = { 50.0f, 0.50f, 1.0f, 10.0f, 0.5f, 0.0f, 0.0f };
            break;
    }

    return nutrition;
}

// ============================================================
// PLANT CREATURE INTERACTION IMPLEMENTATION
// ============================================================

PlantCreatureInteraction::PlantCreatureInteraction() {
}

PlantCreatureInteraction::~PlantCreatureInteraction() {
}

void PlantCreatureInteraction::initialize(
    TreeGenerator* trees,
    GrassSystem* grass,
    AquaticPlantSystem* aquatic,
    FungiSystem* fungi,
    AlienVegetationSystem* alien,
    const Terrain* terrainRef
) {
    treeGenerator = trees;
    grassSystem = grass;
    aquaticSystem = aquatic;
    fungiSystem = fungi;
    alienSystem = alien;
    terrain = terrainRef;

    // Generate initial shelter zones and nectar sources
    generateShelterZones();
    generateNectarSources();

    // Spawn initial fruits from trees
    spawnTreeFruits();
}

void PlantCreatureInteraction::update(float deltaTime) {
    updateFruits(deltaTime);
    updateShelterZones(deltaTime);
    updatePollination(deltaTime);
    updateSeedDispersal(deltaTime);
    updateNectarSources(deltaTime);

    // Periodically spawn new fruits
    fruitSpawnTimer += deltaTime;
    if (fruitSpawnTimer >= fruitSpawnInterval) {
        fruitSpawnTimer = 0.0f;
        spawnTreeFruits();
    }

    // Periodically update shelter zones
    shelterUpdateTimer += deltaTime;
    if (shelterUpdateTimer >= shelterUpdateInterval) {
        shelterUpdateTimer = 0.0f;
        generateShelterZones();
    }
}

// ============================================================
// FRUIT SYSTEM
// ============================================================

void PlantCreatureInteraction::spawnFruit(const glm::vec3& position, FruitType type, int sourceTreeId) {
    FruitInstance fruit;
    fruit.position = position;
    fruit.type = type;
    fruit.size = 0.1f + (static_cast<float>(rand()) / RAND_MAX) * 0.2f;
    fruit.ripeness = 0.0f;
    fruit.age = 0.0f;

    // Set color based on type
    switch (type) {
        case FruitType::APPLE:
            fruit.color = glm::vec3(0.9f, 0.2f, 0.2f);
            break;
        case FruitType::ORANGE:
            fruit.color = glm::vec3(1.0f, 0.6f, 0.0f);
            break;
        case FruitType::BANANA:
            fruit.color = glm::vec3(1.0f, 0.9f, 0.1f);
            break;
        case FruitType::BERRY_BLUE:
            fruit.color = glm::vec3(0.2f, 0.3f, 0.9f);
            break;
        case FruitType::GLOW_FRUIT:
            fruit.color = glm::vec3(0.4f, 1.0f, 0.8f);
            fruit.glowIntensity = 0.8f;
            break;
        default:
            fruit.color = glm::vec3(0.6f, 0.8f, 0.3f);
            break;
    }

    fruit.glowIntensity = 0.0f;
    fruit.isOnTree = true;
    fruit.isOnGround = false;
    fruit.isInWater = false;
    fruit.isBeingCarried = false;
    fruit.carrierCreatureId = -1;
    fruit.velocity = glm::vec3(0.0f);
    fruit.bounceCount = 0;
    fruit.sourceTreeId = sourceTreeId;
    fruit.sourcePosition = position;
    fruit.hasSeed = true;

    // Set dispersal method based on fruit type
    if (type >= FruitType::SEED_WINGED && type <= FruitType::SEED_FLUFFY) {
        fruit.dispersalMethod = SeedDispersalMethod::WIND;
    } else if (type == FruitType::SEED_FLOATING) {
        fruit.dispersalMethod = SeedDispersalMethod::WATER;
    } else if (type == FruitType::SEED_STICKY) {
        fruit.dispersalMethod = SeedDispersalMethod::ANIMAL_CARRIED;
    } else if (type >= FruitType::ACORN && type <= FruitType::HAZELNUT) {
        fruit.dispersalMethod = SeedDispersalMethod::CREATURE_CACHED;
    } else if (type >= FruitType::GLOW_FRUIT && type <= FruitType::PSYCHIC_NUT) {
        fruit.dispersalMethod = SeedDispersalMethod::ALIEN_TELEPORT;
    } else {
        fruit.dispersalMethod = SeedDispersalMethod::ANIMAL_EATEN;
    }

    fruit.germinationChance = 0.1f + (static_cast<float>(rand()) / RAND_MAX) * 0.3f;

    fruits.push_back(fruit);
}

void PlantCreatureInteraction::dropFruit(int fruitId) {
    if (fruitId < 0 || fruitId >= static_cast<int>(fruits.size())) return;

    FruitInstance& fruit = fruits[fruitId];
    if (!fruit.isOnTree) return;

    fruit.isOnTree = false;
    fruit.velocity = glm::vec3(
        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f,
        -1.0f,
        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f
    );
}

FruitNutrition PlantCreatureInteraction::eatFruit(int fruitId, int creatureId) {
    if (fruitId < 0 || fruitId >= static_cast<int>(fruits.size())) {
        return FruitNutrition{};
    }

    FruitInstance& fruit = fruits[fruitId];
    FruitNutrition nutrition = getFruitNutrition(fruit.type);

    // Adjust nutrition based on ripeness
    float ripenessMultiplier = 1.0f;
    if (fruit.ripeness < 0.5f) {
        ripenessMultiplier = 0.5f + fruit.ripeness; // Unripe = less nutrition
    } else if (fruit.ripeness > 1.2f) {
        ripenessMultiplier = 1.5f - (fruit.ripeness - 1.0f) * 0.5f; // Overripe = slightly less
        nutrition.toxicity += (fruit.ripeness - 1.0f) * 0.1f; // Rotting = more toxic
    }

    nutrition.calories *= ripenessMultiplier;
    nutrition.hydration *= ripenessMultiplier;

    // Release seed if fruit has one and was eaten
    if (fruit.hasSeed && fruit.dispersalMethod == SeedDispersalMethod::ANIMAL_EATEN) {
        // Seed will be deposited when creature poops (handled elsewhere)
        // For now, just track that this seed was eaten
    }

    // Remove fruit from list
    fruits.erase(fruits.begin() + fruitId);
    totalFruitsEaten++;

    return nutrition;
}

std::vector<FruitInstance*> PlantCreatureInteraction::getFruitsInRadius(
    const glm::vec3& position, float radius
) {
    std::vector<FruitInstance*> result;
    float radiusSq = radius * radius;

    for (auto& fruit : fruits) {
        glm::vec3 diff = fruit.position - position;
        float distSq = glm::dot(diff, diff);
        if (distSq <= radiusSq) {
            result.push_back(&fruit);
        }
    }

    return result;
}

std::vector<FruitInstance*> PlantCreatureInteraction::getRipeFruitsInRadius(
    const glm::vec3& position, float radius
) {
    std::vector<FruitInstance*> result;
    float radiusSq = radius * radius;

    for (auto& fruit : fruits) {
        if (fruit.ripeness >= 0.8f && fruit.ripeness <= 1.3f) {
            glm::vec3 diff = fruit.position - position;
            float distSq = glm::dot(diff, diff);
            if (distSq <= radiusSq) {
                result.push_back(&fruit);
            }
        }
    }

    return result;
}

FruitInstance* PlantCreatureInteraction::findNearestFruit(
    const glm::vec3& position, FruitType type, float maxDistance
) {
    FruitInstance* nearest = nullptr;
    float nearestDistSq = maxDistance * maxDistance;

    for (auto& fruit : fruits) {
        if (fruit.type == type || type == FruitType::COUNT) {
            glm::vec3 diff = fruit.position - position;
            float distSq = glm::dot(diff, diff);
            if (distSq < nearestDistSq) {
                nearestDistSq = distSq;
                nearest = &fruit;
            }
        }
    }

    return nearest;
}

// ============================================================
// SHELTER SYSTEM
// ============================================================

ShelterZone* PlantCreatureInteraction::findShelter(
    const glm::vec3& position, float searchRadius, float minQuality
) {
    float radiusSq = searchRadius * searchRadius;

    for (auto& shelter : shelterZones) {
        glm::vec3 diff = shelter.position - position;
        float distSq = glm::dot(diff, diff);
        if (distSq <= radiusSq) {
            float quality = shelter.quality.coveragePercent * 0.3f +
                           shelter.quality.concealment * 0.3f +
                           shelter.quality.predatorSafety * 0.4f;
            if (quality >= minQuality) {
                return &shelter;
            }
        }
    }

    return nullptr;
}

ShelterZone* PlantCreatureInteraction::findBestShelter(
    const glm::vec3& position, float searchRadius
) {
    ShelterZone* best = nullptr;
    float bestQuality = 0.0f;
    float radiusSq = searchRadius * searchRadius;

    for (auto& shelter : shelterZones) {
        glm::vec3 diff = shelter.position - position;
        float distSq = glm::dot(diff, diff);
        if (distSq <= radiusSq) {
            float quality = shelter.quality.coveragePercent * 0.25f +
                           shelter.quality.concealment * 0.25f +
                           shelter.quality.predatorSafety * 0.25f +
                           shelter.quality.comfortLevel * 0.25f;

            // Penalty for distance
            quality *= (1.0f - std::sqrt(distSq) / searchRadius * 0.3f);

            // Penalty for crowding
            float occupancyRatio = static_cast<float>(shelter.occupantCreatureIds.size()) /
                                  static_cast<float>(shelter.maxOccupants);
            quality *= (1.0f - occupancyRatio * 0.5f);

            if (quality > bestQuality) {
                bestQuality = quality;
                best = &shelter;
            }
        }
    }

    return best;
}

bool PlantCreatureInteraction::enterShelter(int shelterIndex, int creatureId) {
    if (shelterIndex < 0 || shelterIndex >= static_cast<int>(shelterZones.size())) {
        return false;
    }

    ShelterZone& shelter = shelterZones[shelterIndex];
    if (static_cast<int>(shelter.occupantCreatureIds.size()) >= shelter.maxOccupants) {
        return false;
    }

    // Check if already in shelter
    for (int id : shelter.occupantCreatureIds) {
        if (id == creatureId) return true;
    }

    shelter.occupantCreatureIds.push_back(creatureId);
    return true;
}

void PlantCreatureInteraction::leaveShelter(int shelterIndex, int creatureId) {
    if (shelterIndex < 0 || shelterIndex >= static_cast<int>(shelterZones.size())) {
        return;
    }

    ShelterZone& shelter = shelterZones[shelterIndex];
    auto it = std::find(shelter.occupantCreatureIds.begin(),
                        shelter.occupantCreatureIds.end(),
                        creatureId);
    if (it != shelter.occupantCreatureIds.end()) {
        shelter.occupantCreatureIds.erase(it);
    }
}

ShelterQuality PlantCreatureInteraction::getShelterQuality(
    const glm::vec3& position, float radius
) const {
    ShelterQuality result = {};
    result.type = ShelterType::NONE;
    result.center = position;
    result.radius = radius;

    float totalCoverage = 0.0f;
    float totalConcealment = 0.0f;
    float totalProtection = 0.0f;
    int contributorCount = 0;

    float radiusSq = radius * radius;

    for (const auto& shelter : shelterZones) {
        glm::vec3 diff = shelter.position - position;
        float distSq = glm::dot(diff, diff);
        if (distSq <= radiusSq) {
            float influence = 1.0f - std::sqrt(distSq) / radius;
            totalCoverage += shelter.quality.coveragePercent * influence;
            totalConcealment += shelter.quality.concealment * influence;
            totalProtection += shelter.quality.weatherProtection * influence;
            contributorCount++;
        }
    }

    if (contributorCount > 0) {
        result.coveragePercent = std::min(1.0f, totalCoverage / contributorCount);
        result.concealment = std::min(1.0f, totalConcealment / contributorCount);
        result.weatherProtection = std::min(1.0f, totalProtection / contributorCount);
        result.predatorSafety = result.concealment * 0.8f;
        result.comfortLevel = (result.coveragePercent + result.weatherProtection) * 0.5f;

        if (result.coveragePercent > 0.8f) {
            result.type = ShelterType::FULL;
        } else if (result.coveragePercent > 0.5f) {
            result.type = ShelterType::PARTIAL;
        } else if (result.coveragePercent > 0.2f) {
            result.type = ShelterType::MINIMAL;
        }
    }

    return result;
}

// ============================================================
// POLLINATION SYSTEM
// ============================================================

void PlantCreatureInteraction::creatureVisitsFlower(
    int creatureId, int flowerId, PollinatorType type
) {
    // Get pollen from flower
    PollenPacket pollen;
    pollen.sourceFlowerId = flowerId;
    pollen.sourcePosition = glm::vec3(0.0f); // Would get from flower
    pollen.flowerSpeciesId = flowerId % 25;  // Simplified
    pollen.viability = 1.0f;
    pollen.amount = 10.0f + static_cast<float>(rand()) / RAND_MAX * 20.0f;
    pollen.collectionTime = 0.0f;

    // Add to creature's pollen load
    creaturePollenCarried[creatureId].push_back(pollen);

    // Limit pollen carried
    if (creaturePollenCarried[creatureId].size() > 5) {
        creaturePollenCarried[creatureId].erase(
            creaturePollenCarried[creatureId].begin()
        );
    }

    // Try to pollinate with existing pollen
    for (auto& carriedPollen : creaturePollenCarried[creatureId]) {
        if (carriedPollen.sourceFlowerId != flowerId &&
            carriedPollen.viability > 0.3f) {
            // Pollination attempt
            PollinationEvent event;
            event.sourceFlowerId = carriedPollen.sourceFlowerId;
            event.targetFlowerId = flowerId;
            event.pollinatorCreatureId = creatureId;
            event.pollinatorType = type;
            event.timestamp = 0.0f;
            event.position = glm::vec3(0.0f);

            // Check compatibility (same species)
            if (carriedPollen.flowerSpeciesId == (flowerId % 25)) {
                event.successful = true;
                totalSuccessfulPollinations++;

                // Notify grass system if available
                if (grassSystem) {
                    grassSystem->pollinatorVisit(flowerId, static_cast<int>(type));
                }
            } else {
                event.successful = (static_cast<float>(rand()) / RAND_MAX) < 0.1f;
            }

            pollinationHistory.push_back(event);

            // Use up some pollen
            carriedPollen.amount -= 5.0f;
            if (carriedPollen.amount <= 0) {
                carriedPollen.viability = 0.0f;
            }

            break;
        }
    }
}

bool PlantCreatureInteraction::canCreaturePollinate(int creatureId, int targetFlowerId) const {
    auto it = creaturePollenCarried.find(creatureId);
    if (it == creaturePollenCarried.end()) return false;

    for (const auto& pollen : it->second) {
        if (pollen.viability > 0.3f && pollen.amount > 0) {
            return true;
        }
    }

    return false;
}

std::vector<PollenPacket>* PlantCreatureInteraction::getCreaturePollen(int creatureId) {
    auto it = creaturePollenCarried.find(creatureId);
    if (it != creaturePollenCarried.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<NectarSource> PlantCreatureInteraction::findNectarSources(
    const glm::vec3& position, float radius
) const {
    std::vector<NectarSource> result;
    float radiusSq = radius * radius;

    for (const auto& nectar : nectarSources) {
        glm::vec3 diff = nectar.position - position;
        float distSq = glm::dot(diff, diff);
        if (distSq <= radiusSq && nectar.nectarAmount > 0.1f) {
            result.push_back(nectar);
        }
    }

    // Sort by amount (most nectar first)
    std::sort(result.begin(), result.end(),
        [](const NectarSource& a, const NectarSource& b) {
            return a.nectarAmount > b.nectarAmount;
        });

    return result;
}

NectarSource* PlantCreatureInteraction::findBestNectarSource(
    const glm::vec3& position, float maxDistance
) {
    NectarSource* best = nullptr;
    float bestScore = 0.0f;
    float maxDistSq = maxDistance * maxDistance;

    for (auto& nectar : nectarSources) {
        if (nectar.nectarAmount < 0.1f) continue;

        glm::vec3 diff = nectar.position - position;
        float distSq = glm::dot(diff, diff);
        if (distSq > maxDistSq) continue;

        float distance = std::sqrt(distSq);
        float score = nectar.nectarAmount * nectar.sugarContent *
                     (1.0f - distance / maxDistance);

        if (score > bestScore) {
            bestScore = score;
            best = &nectar;
        }
    }

    return best;
}

float PlantCreatureInteraction::consumeNectar(int nectarSourceIndex, float amount, int creatureId) {
    if (nectarSourceIndex < 0 || nectarSourceIndex >= static_cast<int>(nectarSources.size())) {
        return 0.0f;
    }

    NectarSource& nectar = nectarSources[nectarSourceIndex];
    float consumed = std::min(amount, nectar.nectarAmount);
    nectar.nectarAmount -= consumed;

    // Trigger flower visit for pollination
    creatureVisitsFlower(creatureId, nectar.flowerId, PollinatorType::BEE);

    return consumed * nectar.sugarContent;
}

// ============================================================
// SEED DISPERSAL
// ============================================================

void PlantCreatureInteraction::releaseSeed(const FruitInstance& fruit, const glm::vec3& position) {
    DispersingSeed seed;
    seed.position = position;
    seed.fruitType = fruit.type;
    seed.method = fruit.dispersalMethod;
    seed.originPosition = fruit.sourcePosition;
    seed.originPlantId = fruit.sourceTreeId;
    seed.age = 0.0f;
    seed.viability = fruit.germinationChance;
    seed.isAttachedToCreature = false;
    seed.carrierCreatureId = -1;

    // Set physics based on dispersal method
    switch (fruit.dispersalMethod) {
        case SeedDispersalMethod::WIND:
            seed.velocity = glm::vec3(0.0f, 0.5f, 0.0f);
            seed.windResistance = 0.9f;
            seed.liftCoefficient = 0.3f;
            seed.buoyancy = 0.0f;
            break;

        case SeedDispersalMethod::WATER:
            seed.velocity = glm::vec3(0.0f, -0.1f, 0.0f);
            seed.windResistance = 0.3f;
            seed.liftCoefficient = 0.0f;
            seed.buoyancy = 0.8f;
            break;

        case SeedDispersalMethod::EXPLOSIVE:
            seed.velocity = glm::vec3(
                (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 10.0f,
                5.0f + static_cast<float>(rand()) / RAND_MAX * 5.0f,
                (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 10.0f
            );
            seed.windResistance = 0.2f;
            seed.liftCoefficient = 0.0f;
            seed.buoyancy = 0.0f;
            break;

        case SeedDispersalMethod::GRAVITY:
        default:
            seed.velocity = glm::vec3(0.0f, -2.0f, 0.0f);
            seed.windResistance = 0.1f;
            seed.liftCoefficient = 0.0f;
            seed.buoyancy = 0.3f;
            break;
    }

    seed.germinationChance = fruit.germinationChance;
    seed.dormancyTimer = 0.0f;
    seed.requiresColdPeriod = (fruit.type >= FruitType::ACORN && fruit.type <= FruitType::HAZELNUT);
    seed.hasExperiencedCold = false;

    dispersingSeeds.push_back(seed);
}

void PlantCreatureInteraction::attachSeedToCreature(
    int seedIndex, int creatureId, const glm::vec3& attachPoint
) {
    if (seedIndex < 0 || seedIndex >= static_cast<int>(dispersingSeeds.size())) return;

    DispersingSeed& seed = dispersingSeeds[seedIndex];
    seed.isAttachedToCreature = true;
    seed.carrierCreatureId = creatureId;
    seed.attachmentPoint = attachPoint;
    seed.velocity = glm::vec3(0.0f);

    if (onSeedAttach) {
        onSeedAttach(creatureId, seed);
    }
}

void PlantCreatureInteraction::detachSeedFromCreature(int seedIndex) {
    if (seedIndex < 0 || seedIndex >= static_cast<int>(dispersingSeeds.size())) return;

    DispersingSeed& seed = dispersingSeeds[seedIndex];
    seed.isAttachedToCreature = false;
    seed.carrierCreatureId = -1;
    seed.velocity = glm::vec3(0.0f, -1.0f, 0.0f);
}

void PlantCreatureInteraction::cacheSeed(
    int creatureId, FruitType seedType, const glm::vec3& position
) {
    SeedCache cache;
    cache.position = position;
    cache.seeds.push_back(seedType);
    cache.creatorCreatureId = creatureId;
    cache.burialTime = 0.0f;
    cache.depth = 0.05f + static_cast<float>(rand()) / RAND_MAX * 0.1f;
    cache.isRetrieved = false;
    cache.hasSprouted = false;

    seedCaches.push_back(cache);
}

std::vector<FruitType> PlantCreatureInteraction::retrieveCache(int cacheIndex, int creatureId) {
    if (cacheIndex < 0 || cacheIndex >= static_cast<int>(seedCaches.size())) {
        return {};
    }

    SeedCache& cache = seedCaches[cacheIndex];
    if (cache.isRetrieved || cache.hasSprouted) {
        return {};
    }

    cache.isRetrieved = true;
    return cache.seeds;
}

std::vector<DispersingSeed*> PlantCreatureInteraction::getSeedsOnCreature(int creatureId) {
    std::vector<DispersingSeed*> result;
    for (auto& seed : dispersingSeeds) {
        if (seed.isAttachedToCreature && seed.carrierCreatureId == creatureId) {
            result.push_back(&seed);
        }
    }
    return result;
}

void PlantCreatureInteraction::checkGermination(float deltaTime) {
    // Check dispersing seeds that have landed
    for (auto it = dispersingSeeds.begin(); it != dispersingSeeds.end(); ) {
        if (!it->isAttachedToCreature && it->age > 5.0f) {
            // Check if conditions are right for germination
            if (checkGerminationConditions(*it, it->position)) {
                if (static_cast<float>(rand()) / RAND_MAX < it->germinationChance * deltaTime) {
                    spawnNewPlant(it->fruitType, it->position);
                    totalGerminatedSeeds++;
                    it = dispersingSeeds.erase(it);
                    continue;
                }
            }
        }
        ++it;
    }

    // Check cached seeds
    for (auto& cache : seedCaches) {
        if (!cache.isRetrieved && !cache.hasSprouted) {
            cache.burialTime += deltaTime;

            // Seeds that have been buried long enough may sprout
            if (cache.burialTime > 30.0f) {
                for (const auto& seedType : cache.seeds) {
                    if (static_cast<float>(rand()) / RAND_MAX < 0.01f * deltaTime) {
                        spawnNewPlant(seedType, cache.position);
                        cache.hasSprouted = true;
                        totalGerminatedSeeds++;
                        break;
                    }
                }
            }
        }
    }
}

// ============================================================
// PLANT EFFECTS
// ============================================================

std::vector<PlantEffect> PlantCreatureInteraction::getPlantEffectsAt(
    const glm::vec3& position, float radius
) const {
    std::vector<PlantEffect> effects;
    float radiusSq = radius * radius;

    // Check alien plants for effects
    if (alienSystem) {
        const auto& aliens = alienSystem->getAllInstances();
        for (size_t i = 0; i < aliens.size(); ++i) {
            const auto& plant = aliens[i];
            glm::vec3 diff = plant.position - position;
            float distSq = glm::dot(diff, diff);

            if (distSq <= radiusSq) {
                float distance = std::sqrt(distSq);

                // Predatory plants cause damage
                if (plant.isPredatory && distance < plant.dangerRadius) {
                    PlantEffect effect;
                    effect.type = PlantEffectType::POISON;
                    effect.strength = 0.5f * (1.0f - distance / plant.dangerRadius);
                    effect.duration = 5.0f;
                    effect.elapsed = 0.0f;
                    effect.sourcePlantId = static_cast<int>(i);
                    effect.sourcePosition = plant.position;
                    effects.push_back(effect);
                }

                // Psychic plants cause hallucinations
                if (plant.psychicRange > 0 && distance < plant.psychicRange) {
                    PlantEffect effect;
                    effect.type = PlantEffectType::HALLUCINOGEN;
                    effect.strength = 0.3f * (1.0f - distance / plant.psychicRange);
                    effect.duration = 10.0f;
                    effect.elapsed = 0.0f;
                    effect.sourcePlantId = static_cast<int>(i);
                    effect.sourcePosition = plant.position;
                    effects.push_back(effect);
                }

                // Glowing plants provide energy (alien symbiosis)
                if (plant.glowIntensity > 0.5f && distance < 5.0f) {
                    PlantEffect effect;
                    effect.type = PlantEffectType::ENERGY_BOOST;
                    effect.strength = plant.glowIntensity * 0.2f;
                    effect.duration = 3.0f;
                    effect.elapsed = 0.0f;
                    effect.sourcePlantId = static_cast<int>(i);
                    effect.sourcePosition = plant.position;
                    effects.push_back(effect);
                }
            }
        }
    }

    // Check fungi for effects
    if (fungiSystem) {
        const auto& fungi = fungiSystem->getAllInstances();
        for (size_t i = 0; i < fungi.size(); ++i) {
            const auto& fungus = fungi[i];
            glm::vec3 diff = fungus.position - position;
            float distSq = glm::dot(diff, diff);

            if (distSq <= radiusSq) {
                float distance = std::sqrt(distSq);

                // Some fungi are poisonous
                if (fungus.toxicity > 0.5f && distance < 2.0f) {
                    PlantEffect effect;
                    effect.type = PlantEffectType::POISON;
                    effect.strength = fungus.toxicity * 0.3f;
                    effect.duration = 8.0f;
                    effect.elapsed = 0.0f;
                    effect.sourcePlantId = static_cast<int>(i);
                    effect.sourcePosition = fungus.position;
                    effects.push_back(effect);
                }

                // Bioluminescent fungi may have psychoactive effects
                if (fungus.isBioluminescent && distance < 3.0f) {
                    PlantEffect effect;
                    effect.type = PlantEffectType::HALLUCINOGEN;
                    effect.strength = 0.2f;
                    effect.duration = 15.0f;
                    effect.elapsed = 0.0f;
                    effect.sourcePlantId = static_cast<int>(i);
                    effect.sourcePosition = fungus.position;
                    effects.push_back(effect);
                }
            }
        }
    }

    return effects;
}

void PlantCreatureInteraction::applyPlantEffects(
    int creatureId, const glm::vec3& position, float deltaTime
) {
    auto effects = getPlantEffectsAt(position, 10.0f);

    for (const auto& effect : effects) {
        if (onPlantEffect) {
            onPlantEffect(creatureId, effect);
        }
    }
}

bool PlantCreatureInteraction::isDangerousPlantNearby(
    const glm::vec3& position, float radius
) const {
    return getPlantDangerLevel(position, radius) > 0.3f;
}

float PlantCreatureInteraction::getPlantDangerLevel(
    const glm::vec3& position, float radius
) const {
    float dangerLevel = 0.0f;

    // Check alien plants
    if (alienSystem) {
        dangerLevel += alienSystem->getDangerLevel(position, radius);
    }

    // Check fungi toxicity
    if (fungiSystem) {
        const auto& fungi = fungiSystem->getAllInstances();
        float radiusSq = radius * radius;

        for (const auto& fungus : fungi) {
            glm::vec3 diff = fungus.position - position;
            float distSq = glm::dot(diff, diff);
            if (distSq <= radiusSq && fungus.toxicity > 0.5f) {
                float distance = std::sqrt(distSq);
                dangerLevel += fungus.toxicity * (1.0f - distance / radius) * 0.2f;
            }
        }
    }

    return std::min(1.0f, dangerLevel);
}

// ============================================================
// CREATURE QUERIES
// ============================================================

PlantCreatureInteraction::FoodScanResult PlantCreatureInteraction::scanForFood(
    const glm::vec3& position, float radius
) const {
    FoodScanResult result;
    result.closestFoodDistance = radius;
    result.closestFoodPosition = position;

    float radiusSq = radius * radius;

    // Scan fruits
    for (const auto& fruit : fruits) {
        if (fruit.ripeness >= 0.5f && fruit.ripeness <= 1.5f) {
            glm::vec3 diff = fruit.position - position;
            float distSq = glm::dot(diff, diff);
            if (distSq <= radiusSq) {
                result.fruits.push_back(const_cast<FruitInstance*>(&fruit));
                float dist = std::sqrt(distSq);
                if (dist < result.closestFoodDistance) {
                    result.closestFoodDistance = dist;
                    result.closestFoodPosition = fruit.position;
                }
            }
        }
    }

    // Scan nectar
    for (const auto& nectar : nectarSources) {
        if (nectar.nectarAmount > 0.1f) {
            glm::vec3 diff = nectar.position - position;
            float distSq = glm::dot(diff, diff);
            if (distSq <= radiusSq) {
                result.nectarSources.push_back(nectar);
                float dist = std::sqrt(distSq);
                if (dist < result.closestFoodDistance) {
                    result.closestFoodDistance = dist;
                    result.closestFoodPosition = nectar.position;
                }
            }
        }
    }

    return result;
}

PlantCreatureInteraction::ShelterScanResult PlantCreatureInteraction::scanForShelter(
    const glm::vec3& position, float radius
) {
    ShelterScanResult result;
    result.bestShelter = nullptr;
    result.closestShelterDistance = radius;

    float radiusSq = radius * radius;
    float bestQuality = 0.0f;

    for (auto& shelter : shelterZones) {
        glm::vec3 diff = shelter.position - position;
        float distSq = glm::dot(diff, diff);
        if (distSq <= radiusSq) {
            result.shelters.push_back(&shelter);

            float dist = std::sqrt(distSq);
            if (dist < result.closestShelterDistance) {
                result.closestShelterDistance = dist;
            }

            float quality = shelter.quality.coveragePercent * 0.3f +
                           shelter.quality.predatorSafety * 0.4f +
                           shelter.quality.comfortLevel * 0.3f;
            quality *= (1.0f - dist / radius * 0.5f);

            if (quality > bestQuality) {
                bestQuality = quality;
                result.bestShelter = &shelter;
            }
        }
    }

    return result;
}

PlantCreatureInteraction::DangerScanResult PlantCreatureInteraction::scanForDanger(
    const glm::vec3& position, float radius
) const {
    DangerScanResult result;
    result.overallDangerLevel = getPlantDangerLevel(position, radius);

    auto effects = getPlantEffectsAt(position, radius);
    for (const auto& effect : effects) {
        if (effect.type == PlantEffectType::POISON ||
            effect.type == PlantEffectType::PARASITIC ||
            effect.type == PlantEffectType::MIND_CONTROL) {
            result.dangerousPlantPositions.push_back(effect.sourcePosition);
            result.activeThreats.push_back(effect.type);
        }
    }

    return result;
}

// ============================================================
// STATISTICS
// ============================================================

PlantCreatureInteraction::InteractionStats PlantCreatureInteraction::getStats() const {
    InteractionStats stats = {};

    stats.totalFruits = static_cast<int>(fruits.size());

    for (const auto& fruit : fruits) {
        if (fruit.ripeness >= 0.8f && fruit.ripeness <= 1.2f) {
            stats.ripeFruits++;
        }
        if (!fruit.isOnTree) {
            stats.fallenFruits++;
        }
    }

    stats.fruitsEaten = totalFruitsEaten;
    stats.shelterZoneCount = static_cast<int>(shelterZones.size());

    for (const auto& shelter : shelterZones) {
        if (!shelter.occupantCreatureIds.empty()) {
            stats.occupiedShelters++;
        }
    }

    stats.pollinationEvents = static_cast<int>(pollinationHistory.size());
    stats.successfulPollinations = totalSuccessfulPollinations;
    stats.dispersingSeeds = static_cast<int>(dispersingSeeds.size());
    stats.seedCaches = static_cast<int>(seedCaches.size());
    stats.germinatedSeeds = totalGerminatedSeeds;

    return stats;
}

// ============================================================
// UPDATE FUNCTIONS
// ============================================================

void PlantCreatureInteraction::updateFruits(float deltaTime) {
    const float gravity = 9.8f;
    const float groundFriction = 0.8f;

    for (auto it = fruits.begin(); it != fruits.end(); ) {
        FruitInstance& fruit = *it;

        // Age and ripen
        fruit.age += deltaTime;
        if (fruit.isOnTree) {
            fruit.ripeness += deltaTime * 0.01f; // Ripen slowly on tree

            // Drop when overripe
            if (fruit.ripeness > 1.3f) {
                fruit.isOnTree = false;
                fruit.velocity = glm::vec3(0.0f, -1.0f, 0.0f);
            }
        }

        // Physics for falling/fallen fruit
        if (!fruit.isOnTree && !fruit.isBeingCarried) {
            // Apply gravity
            fruit.velocity.y -= gravity * deltaTime;

            // Update position
            fruit.position += fruit.velocity * deltaTime;

            // Check ground collision
            float groundHeight = 0.0f;
            if (terrain) {
                groundHeight = terrain->getHeightAt(fruit.position.x, fruit.position.z);
            }

            if (fruit.position.y <= groundHeight + fruit.size * 0.5f) {
                fruit.position.y = groundHeight + fruit.size * 0.5f;
                fruit.isOnGround = true;

                // Bounce
                if (fruit.velocity.y < -1.0f && fruit.bounceCount < 3) {
                    fruit.velocity.y = -fruit.velocity.y * 0.3f;
                    fruit.velocity.x *= groundFriction;
                    fruit.velocity.z *= groundFriction;
                    fruit.bounceCount++;
                } else {
                    fruit.velocity = glm::vec3(0.0f);
                }
            }

            // Decay on ground
            if (fruit.isOnGround) {
                fruit.ripeness += deltaTime * 0.05f; // Rot faster on ground
            }
        }

        // Remove rotted fruits
        if (fruit.ripeness > 2.0f) {
            // Release seed before removing
            if (fruit.hasSeed) {
                releaseSeed(fruit, fruit.position);
            }
            it = fruits.erase(it);
        } else {
            ++it;
        }
    }
}

void PlantCreatureInteraction::updateShelterZones(float deltaTime) {
    // Shelter zones are regenerated periodically
    // This just updates occupancy and quality based on time of day
}

void PlantCreatureInteraction::updatePollination(float deltaTime) {
    // Decay pollen viability
    for (auto& pair : creaturePollenCarried) {
        for (auto& pollen : pair.second) {
            pollen.viability -= deltaTime * 0.01f;
            pollen.collectionTime += deltaTime;
        }

        // Remove dead pollen
        pair.second.erase(
            std::remove_if(pair.second.begin(), pair.second.end(),
                [](const PollenPacket& p) { return p.viability <= 0.0f; }),
            pair.second.end()
        );
    }
}

void PlantCreatureInteraction::updateSeedDispersal(float deltaTime) {
    const float gravity = 9.8f;

    for (auto it = dispersingSeeds.begin(); it != dispersingSeeds.end(); ) {
        DispersingSeed& seed = *it;

        if (!seed.isAttachedToCreature) {
            seed.age += deltaTime;

            // Apply physics based on dispersal method
            glm::vec3 windVelocity = calculateWindDispersal(seed, deltaTime);

            if (seed.method == SeedDispersalMethod::WIND) {
                seed.velocity += windVelocity * deltaTime;
                seed.velocity.y -= gravity * (1.0f - seed.liftCoefficient) * deltaTime;
            } else if (seed.method == SeedDispersalMethod::WATER) {
                // Float in water
                if (seed.position.y < 0) { // Underwater
                    seed.velocity.y += seed.buoyancy * gravity * deltaTime;
                }
            } else {
                seed.velocity.y -= gravity * deltaTime;
            }

            // Damping
            seed.velocity *= (1.0f - seed.windResistance * deltaTime);

            seed.position += seed.velocity * deltaTime;

            // Ground check
            float groundHeight = 0.0f;
            if (terrain) {
                groundHeight = terrain->getHeightAt(seed.position.x, seed.position.z);
            }

            if (seed.position.y <= groundHeight) {
                seed.position.y = groundHeight;
                seed.velocity = glm::vec3(0.0f);
            }

            // Seed viability decreases over time
            seed.viability -= deltaTime * 0.001f;

            // Remove dead seeds
            if (seed.viability <= 0.0f || seed.age > 300.0f) {
                it = dispersingSeeds.erase(it);
                continue;
            }
        }

        ++it;
    }

    // Check for germination
    checkGermination(deltaTime);
}

void PlantCreatureInteraction::updateNectarSources(float deltaTime) {
    for (auto& nectar : nectarSources) {
        // Refill nectar over time
        if (nectar.nectarAmount < nectar.maxNectar) {
            nectar.nectarAmount += nectar.nectarRefillRate * deltaTime;
            nectar.nectarAmount = std::min(nectar.nectarAmount, nectar.maxNectar);
        }
    }
}

// ============================================================
// GENERATION FUNCTIONS
// ============================================================

void PlantCreatureInteraction::generateShelterZones() {
    shelterZones.clear();

    // Generate shelter from trees
    if (treeGenerator) {
        const auto& trees = treeGenerator->getTreeInstances();
        for (size_t i = 0; i < trees.size(); ++i) {
            const auto& tree = trees[i];

            ShelterZone shelter;
            shelter.position = tree.position;
            shelter.radius = tree.scale * 3.0f;
            shelter.sourceType = ShelterZone::SourceType::TREE;
            shelter.sourceId = static_cast<int>(i);
            shelter.maxOccupants = static_cast<int>(tree.scale * 2.0f);

            shelter.quality.type = ShelterType::CANOPY;
            shelter.quality.coveragePercent = 0.6f + tree.health * 0.3f;
            shelter.quality.concealment = 0.5f + tree.scale * 0.1f;
            shelter.quality.weatherProtection = 0.7f;
            shelter.quality.predatorSafety = 0.4f;
            shelter.quality.comfortLevel = 0.5f;
            shelter.quality.capacity = static_cast<float>(shelter.maxOccupants);
            shelter.quality.center = shelter.position;
            shelter.quality.radius = shelter.radius;

            shelterZones.push_back(shelter);
        }
    }

    // Generate shelter from kelp forests
    if (aquaticSystem) {
        const auto& kelpForests = aquaticSystem->getKelpForests();
        for (size_t i = 0; i < kelpForests.size(); ++i) {
            const auto& forest = kelpForests[i];

            ShelterZone shelter;
            shelter.position = forest.center;
            shelter.radius = forest.radius;
            shelter.sourceType = ShelterZone::SourceType::KELP_FOREST;
            shelter.sourceId = static_cast<int>(i);
            shelter.maxOccupants = static_cast<int>(forest.radius * 0.5f);

            shelter.quality.type = ShelterType::AQUATIC;
            shelter.quality.coveragePercent = 0.8f;
            shelter.quality.concealment = 0.7f;
            shelter.quality.weatherProtection = 0.3f;
            shelter.quality.predatorSafety = 0.6f;
            shelter.quality.comfortLevel = 0.4f;
            shelter.quality.capacity = static_cast<float>(shelter.maxOccupants);
            shelter.quality.center = shelter.position;
            shelter.quality.radius = shelter.radius;

            shelterZones.push_back(shelter);
        }

        // Coral reefs
        const auto& reefs = aquaticSystem->getCoralReefs();
        for (size_t i = 0; i < reefs.size(); ++i) {
            const auto& reef = reefs[i];

            ShelterZone shelter;
            shelter.position = reef.center;
            shelter.radius = reef.radius;
            shelter.sourceType = ShelterZone::SourceType::CORAL_REEF;
            shelter.sourceId = static_cast<int>(i);
            shelter.maxOccupants = static_cast<int>(reef.radius);

            shelter.quality.type = ShelterType::AQUATIC;
            shelter.quality.coveragePercent = 0.7f * reef.overallHealth;
            shelter.quality.concealment = 0.8f;
            shelter.quality.weatherProtection = 0.2f;
            shelter.quality.predatorSafety = 0.7f;
            shelter.quality.comfortLevel = 0.6f;
            shelter.quality.capacity = static_cast<float>(shelter.maxOccupants);
            shelter.quality.center = shelter.position;
            shelter.quality.radius = shelter.radius;

            shelterZones.push_back(shelter);
        }
    }

    // Generate shelter from alien colonies
    if (alienSystem) {
        const auto& colonies = alienSystem->getColonies();
        for (size_t i = 0; i < colonies.size(); ++i) {
            const auto& colony = colonies[i];

            // Only non-dangerous colonies provide shelter
            if (colony.areaDanger < 0.3f) {
                ShelterZone shelter;
                shelter.position = colony.center;
                shelter.radius = colony.radius * 0.5f;
                shelter.sourceType = ShelterZone::SourceType::ALIEN_COLONY;
                shelter.sourceId = static_cast<int>(i);
                shelter.maxOccupants = 3;

                shelter.quality.type = ShelterType::PARTIAL;
                shelter.quality.coveragePercent = 0.4f;
                shelter.quality.concealment = 0.6f + colony.areaWeirdness * 0.2f;
                shelter.quality.weatherProtection = 0.3f;
                shelter.quality.predatorSafety = 0.3f;
                shelter.quality.comfortLevel = 0.2f;
                shelter.quality.capacity = static_cast<float>(shelter.maxOccupants);
                shelter.quality.center = shelter.position;
                shelter.quality.radius = shelter.radius;

                shelterZones.push_back(shelter);
            }
        }
    }
}

void PlantCreatureInteraction::generateNectarSources() {
    nectarSources.clear();

    // Generate from grass system flowers
    if (grassSystem) {
        const auto& flowers = grassSystem->getFlowerInstances();
        for (size_t i = 0; i < flowers.size(); ++i) {
            const auto& flower = flowers[i];

            // Only blooming flowers produce nectar
            if (flower.pollinationState == PollinationState::BLOOMING) {
                NectarSource nectar;
                nectar.position = flower.position;
                nectar.flowerId = static_cast<int>(i);
                nectar.maxNectar = flower.nectarProduction;
                nectar.nectarAmount = nectar.maxNectar * 0.5f;
                nectar.nectarRefillRate = flower.nectarProduction * 0.1f;
                nectar.sugarContent = 0.5f + static_cast<float>(rand()) / RAND_MAX * 0.3f;
                nectar.flowerColor = flower.color;
                nectar.scentStrength = 0.3f + static_cast<float>(rand()) / RAND_MAX * 0.5f;
                nectar.isAlien = false;

                nectarSources.push_back(nectar);
            }
        }
    }

    // Generate from alien plants with flowers
    if (alienSystem) {
        const auto& aliens = alienSystem->getAllInstances();
        for (size_t i = 0; i < aliens.size(); ++i) {
            const auto& plant = aliens[i];

            // Some alien plants produce nectar
            if (plant.type == AlienPlantType::PHOTON_FLOWER ||
                plant.type == AlienPlantType::VOID_BLOSSOM ||
                plant.type == AlienPlantType::HOVER_BLOOM ||
                plant.type == AlienPlantType::HARMONIC_FLOWER) {

                NectarSource nectar;
                nectar.position = plant.position;
                nectar.flowerId = static_cast<int>(i) + 100000; // Offset for alien flowers
                nectar.maxNectar = plant.energy * 0.5f;
                nectar.nectarAmount = nectar.maxNectar;
                nectar.nectarRefillRate = 0.1f;
                nectar.sugarContent = 0.8f;
                nectar.flowerColor = plant.glowColor;
                nectar.scentStrength = 0.8f;
                nectar.isAlien = true;

                nectarSources.push_back(nectar);
            }
        }
    }
}

void PlantCreatureInteraction::spawnTreeFruits() {
    if (!treeGenerator) return;

    const auto& trees = treeGenerator->getTreeInstances();

    for (size_t i = 0; i < trees.size(); ++i) {
        const auto& tree = trees[i];

        // Only healthy mature trees produce fruit
        if (tree.health < 0.5f || tree.growthStage < 0.8f) continue;

        // Determine fruit type based on tree type
        FruitType fruitType = getTreeFruitType(static_cast<int>(tree.type));
        if (fruitType == FruitType::COUNT) continue;

        // Spawn probability based on season (would check season manager)
        float spawnChance = 0.1f * tree.health;

        if (static_cast<float>(rand()) / RAND_MAX < spawnChance) {
            // Random position in tree canopy
            float angle = static_cast<float>(rand()) / RAND_MAX * 6.28318f;
            float radius = tree.scale * (0.5f + static_cast<float>(rand()) / RAND_MAX * 0.5f);
            float height = tree.scale * (0.6f + static_cast<float>(rand()) / RAND_MAX * 0.3f);

            glm::vec3 fruitPos = tree.position + glm::vec3(
                std::cos(angle) * radius,
                height,
                std::sin(angle) * radius
            );

            spawnFruit(fruitPos, fruitType, static_cast<int>(i));
        }
    }
}

// ============================================================
// HELPER FUNCTIONS
// ============================================================

FruitType PlantCreatureInteraction::getTreeFruitType(int treeType) const {
    // Map tree types to fruit types
    switch (treeType) {
        case 0: return FruitType::APPLE;      // Oak-like
        case 1: return FruitType::CHERRY;     // Cherry tree
        case 2: return FruitType::ACORN;      // Oak
        case 3: return FruitType::PINE_NUT;   // Pine
        case 4: return FruitType::WALNUT;     // Walnut tree
        case 5: return FruitType::ORANGE;     // Citrus
        case 6: return FruitType::MANGO;      // Tropical
        case 7: return FruitType::COCONUT;    // Palm
        case 8: return FruitType::FIG;        // Fig tree
        case 9: return FruitType::DATE;       // Date palm
        case 10: return FruitType::BERRY_RED; // Berry bush
        case 11: return FruitType::GLOW_FRUIT; // Alien tree
        case 12: return FruitType::CRYSTAL_FRUIT; // Crystal tree
        default: return FruitType::SEED_MEDIUM;
    }
}

float PlantCreatureInteraction::calculateShelterQuality(
    const glm::vec3& position, float radius
) const {
    auto quality = getShelterQuality(position, radius);
    return quality.coveragePercent * 0.3f +
           quality.concealment * 0.3f +
           quality.predatorSafety * 0.4f;
}

glm::vec3 PlantCreatureInteraction::calculateWindDispersal(
    const DispersingSeed& seed, float deltaTime
) const {
    // Simple wind simulation
    float windStrength = 2.0f + std::sin(seed.age * 0.5f) * 1.0f;
    float windAngle = seed.age * 0.1f;

    return glm::vec3(
        std::cos(windAngle) * windStrength * seed.windResistance,
        std::sin(seed.age * 2.0f) * 0.5f * seed.liftCoefficient,
        std::sin(windAngle) * windStrength * seed.windResistance
    );
}

bool PlantCreatureInteraction::checkGerminationConditions(
    const DispersingSeed& seed, const glm::vec3& position
) const {
    // Check if seed is on ground
    if (seed.velocity.y != 0.0f) return false;

    // Check stratification requirement
    if (seed.requiresColdPeriod && !seed.hasExperiencedCold) return false;

    // Check terrain suitability (would query terrain for soil type)
    if (terrain) {
        float height = terrain->getHeightAt(position.x, position.z);
        // Don't germinate underwater or on steep slopes
        if (position.y < height - 0.1f) return false;
    }

    return true;
}

void PlantCreatureInteraction::spawnNewPlant(FruitType seedType, const glm::vec3& position) {
    // This would add a new plant to the appropriate system
    // For now, just log that germination occurred

    // In a full implementation, this would:
    // - Determine plant type from seed type
    // - Add to TreeGenerator, GrassSystem, etc.
    // - Start the plant at growth stage 0
}

// ============================================================
// VEGETATION MANAGER IMPLEMENTATION
// ============================================================

VegetationManager::VegetationManager() {
}

VegetationManager::~VegetationManager() {
}

void VegetationManager::initialize(DX12Device* device, const Terrain* terrainRef) {
    terrain = terrainRef;

    // Create all subsystems
    treeGenerator = std::make_unique<TreeGenerator>();
    grassSystem = std::make_unique<GrassSystem>();
    aquaticSystem = std::make_unique<AquaticPlantSystem>();
    fungiSystem = std::make_unique<FungiSystem>();
    alienSystem = std::make_unique<AlienVegetationSystem>();
    interaction = std::make_unique<PlantCreatureInteraction>();

    // Initialize each system
    treeGenerator->initialize(device, terrain);
    grassSystem->initialize(device, terrain);
    aquaticSystem->initialize(device, terrain);
    fungiSystem->initialize(device, terrain);
    alienSystem->initialize(device, terrain);

    // Generate initial vegetation
    unsigned int seed = 12345;
    treeGenerator->generate(seed);
    grassSystem->generate(seed + 1);
    aquaticSystem->generate(seed + 2);
    fungiSystem->generate(seed + 3);
    alienSystem->generate(seed + 4);

    // Initialize interaction system last (needs other systems)
    interaction->initialize(
        treeGenerator.get(),
        grassSystem.get(),
        aquaticSystem.get(),
        fungiSystem.get(),
        alienSystem.get(),
        terrain
    );
}

void VegetationManager::update(float deltaTime, const glm::vec3& cameraPos) {
    if (treeGenerator) treeGenerator->update(deltaTime, cameraPos);
    if (grassSystem) grassSystem->update(deltaTime, cameraPos);
    if (aquaticSystem) aquaticSystem->update(deltaTime, cameraPos);
    if (fungiSystem) fungiSystem->update(deltaTime, cameraPos);
    if (alienSystem) alienSystem->update(deltaTime, cameraPos);
    if (interaction) interaction->update(deltaTime);
}

void VegetationManager::render(ID3D12GraphicsCommandList* commandList) {
    if (treeGenerator) treeGenerator->render(commandList);
    if (grassSystem) grassSystem->render(commandList);
    if (aquaticSystem) aquaticSystem->render(commandList);
    if (fungiSystem) fungiSystem->render(commandList);
    if (alienSystem) alienSystem->render(commandList);
}

float VegetationManager::getVegetationDensity(const glm::vec3& position, float radius) const {
    float density = 0.0f;

    if (treeGenerator) {
        // Count trees in radius
        const auto& trees = treeGenerator->getTreeInstances();
        float radiusSq = radius * radius;
        int count = 0;
        for (const auto& tree : trees) {
            glm::vec3 diff = tree.position - position;
            if (glm::dot(diff, diff) <= radiusSq) count++;
        }
        density += count * 0.1f;
    }

    if (grassSystem) {
        density += grassSystem->getGrassDensity(position, radius) * 0.3f;
    }

    return std::min(1.0f, density);
}

float VegetationManager::getFoodAvailability(const glm::vec3& position, float radius) const {
    if (!interaction) return 0.0f;

    auto foodScan = interaction->scanForFood(position, radius);
    float availability = static_cast<float>(foodScan.fruits.size()) * 0.1f +
                        static_cast<float>(foodScan.nectarSources.size()) * 0.05f;

    return std::min(1.0f, availability);
}

float VegetationManager::getShelterAvailability(const glm::vec3& position, float radius) const {
    if (!interaction) return 0.0f;

    auto quality = interaction->getShelterQuality(position, radius);
    return quality.coveragePercent;
}

glm::vec3 VegetationManager::getBiomeAmbientColor(const glm::vec3& position) const {
    glm::vec3 color(0.1f, 0.15f, 0.1f); // Default forest green

    // Add alien glow
    if (alienSystem) {
        glm::vec3 alienColor = alienSystem->getAmbientAlienColor(position, 50.0f);
        color += alienColor * 0.3f;
    }

    // Add bioluminescent fungi
    if (fungiSystem) {
        const auto& fungi = fungiSystem->getAllInstances();
        float radiusSq = 50.0f * 50.0f;
        for (const auto& f : fungi) {
            if (f.isBioluminescent) {
                glm::vec3 diff = f.position - position;
                float distSq = glm::dot(diff, diff);
                if (distSq < radiusSq) {
                    float influence = 1.0f - std::sqrt(distSq) / 50.0f;
                    color += f.glowColor * influence * 0.1f;
                }
            }
        }
    }

    return color;
}

float VegetationManager::getBiomeAlienness(const glm::vec3& position) const {
    if (!alienSystem) return 0.0f;
    return alienSystem->getAliennessLevel(position, 100.0f);
}

VegetationManager::VegetationStats VegetationManager::getStats() const {
    VegetationStats stats = {};

    if (treeGenerator) {
        stats.totalTrees = static_cast<int>(treeGenerator->getTreeInstances().size());
    }

    if (grassSystem) {
        auto grassStats = grassSystem->getStats();
        stats.totalGrassBlades = grassStats.totalBlades;
        stats.totalFlowers = grassStats.bloomingFlowers;
    }

    if (aquaticSystem) {
        auto aquaticStats = aquaticSystem->getStats();
        stats.totalAquaticPlants = aquaticStats.totalPlantCount;
    }

    if (fungiSystem) {
        auto fungiStats = fungiSystem->getStats();
        stats.totalFungi = fungiStats.totalFungi;
    }

    if (alienSystem) {
        auto alienStats = alienSystem->getStats();
        stats.totalAlienPlants = alienStats.totalPlants;
    }

    return stats;
}
