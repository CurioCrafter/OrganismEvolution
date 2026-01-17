#include "ColonyBehavior.h"
#include "SmallCreatures.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace small {

// Static member initialization
std::atomic<uint32_t> ColonyID::nextID{1};

// =============================================================================
// Colony Implementation
// =============================================================================

Colony::Colony(SmallCreatureType baseType, const XMFLOAT3& nestPosition)
    : id_(ColonyID::generate())
    , baseType_(baseType)
    , nestPosition_(nestPosition)
    , queen_(nullptr)
    , underAttack_(false)
    , reproductionCooldown_(0.0f)
    , decisionCooldown_(0.0f) {

    resources_.foodStored = 100.0f;
    resources_.buildingMaterial = 50.0f;
    resources_.larvae = 0;
    resources_.pupae = 0;
    resources_.eggs = 0;
    resources_.nestIntegrity = 100.0f;

    // Create initial nest structure
    createChamber(NestChamber::Type::ENTRANCE, nestPosition);
    createChamber(NestChamber::Type::QUEEN_CHAMBER,
                  { nestPosition.x, nestPosition.y - 0.5f, nestPosition.z });
    createChamber(NestChamber::Type::FOOD_STORAGE,
                  { nestPosition.x + 0.3f, nestPosition.y - 0.3f, nestPosition.z });
    createChamber(NestChamber::Type::BROOD_CHAMBER,
                  { nestPosition.x - 0.3f, nestPosition.y - 0.4f, nestPosition.z });
}

Colony::~Colony() = default;

void Colony::update(float deltaTime, SmallCreatureManager& manager) {
    // Update cooldowns
    reproductionCooldown_ = std::max(0.0f, reproductionCooldown_ - deltaTime);
    decisionCooldown_ = std::max(0.0f, decisionCooldown_ - deltaTime);

    // Colony decision making
    if (decisionCooldown_ <= 0.0f) {
        makeColonyDecisions(deltaTime, manager);
        decisionCooldown_ = 1.0f;  // Check every second
    }

    // Update tasks
    updateTaskAssignments(deltaTime, manager);

    // Queen reproduction
    updateReproduction(deltaTime, manager);

    // Nest maintenance
    updateNestMaintenance(deltaTime);

    // Consume food for colony
    float foodConsumption = members_.size() * 0.01f * deltaTime;
    consumeFood(foodConsumption);

    // Check for threats
    // (Would be done by individual creature AI detecting enemies)
}

void Colony::addMember(SmallCreature* creature, ColonyRole role) {
    if (!creature) return;

    creature->colonyID = id_;
    members_[creature->id] = { creature, role };

    // Assign role based on type
    if (creature->type == SmallCreatureType::ANT_QUEEN ||
        creature->type == SmallCreatureType::BEE_QUEEN ||
        creature->type == SmallCreatureType::TERMITE_QUEEN) {
        role = ColonyRole::QUEEN;
    } else if (creature->type == SmallCreatureType::ANT_SOLDIER ||
               creature->type == SmallCreatureType::TERMITE_SOLDIER) {
        role = ColonyRole::SOLDIER;
    }

    members_[creature->id].second = role;
}

void Colony::removeMember(uint32_t creatureID) {
    members_.erase(creatureID);
}

void Colony::setQueen(SmallCreature* queen) {
    queen_ = queen;
    if (queen) {
        addMember(queen, ColonyRole::QUEEN);
    }
}

ColonyTask* Colony::assignTask(SmallCreature* creature) {
    if (!creature) return nullptr;

    // Find unassigned task with highest priority
    ColonyTask* bestTask = nullptr;
    float highestPriority = -1.0f;

    for (auto& task : taskQueue_) {
        if (task.assignedTo == 0 && task.priority > highestPriority) {
            highestPriority = task.priority;
            bestTask = &task;
        }
    }

    if (bestTask) {
        bestTask->assignedTo = creature->id;
        return bestTask;
    }

    // Create default task based on colony needs
    ColonyTask::Type type = decideNextPriority();

    ColonyTask newTask;
    newTask.type = type;
    newTask.priority = 0.5f;
    newTask.timeRemaining = 30.0f;
    newTask.assignedTo = creature->id;

    switch (type) {
        case ColonyTask::Type::FORAGE:
            // Random direction from nest
            {
                static std::mt19937 rng(std::random_device{}());
                std::uniform_real_distribution<float> dist(-10.0f, 10.0f);
                newTask.targetPosition = {
                    nestPosition_.x + dist(rng),
                    nestPosition_.y,
                    nestPosition_.z + dist(rng)
                };
            }
            break;
        case ColonyTask::Type::PATROL:
            // Circle around nest
            {
                static std::mt19937 rng(std::random_device{}());
                std::uniform_real_distribution<float> angle(0.0f, 6.28318f);
                float a = angle(rng);
                newTask.targetPosition = {
                    nestPosition_.x + cosf(a) * 5.0f,
                    nestPosition_.y,
                    nestPosition_.z + sinf(a) * 5.0f
                };
            }
            break;
        case ColonyTask::Type::RETURN_HOME:
            newTask.targetPosition = nestPosition_;
            break;
        default:
            newTask.targetPosition = nestPosition_;
            break;
    }

    taskQueue_.push_back(newTask);
    return &taskQueue_.back();
}

void Colony::completeTask(uint32_t creatureID) {
    for (auto it = taskQueue_.begin(); it != taskQueue_.end(); ++it) {
        if (it->assignedTo == creatureID) {
            taskQueue_.erase(it);
            break;
        }
    }
}

void Colony::addFood(float amount) {
    resources_.foodStored += amount;

    // Find food storage chamber and update
    auto* storage = getNearestChamber(nestPosition_, NestChamber::Type::FOOD_STORAGE);
    if (storage) {
        storage->currentOccupancy = std::min(storage->currentOccupancy + amount,
                                              storage->capacity);
    }
}

float Colony::consumeFood(float amount) {
    float consumed = std::min(amount, resources_.foodStored);
    resources_.foodStored -= consumed;
    return consumed;
}

void Colony::addBuildingMaterial(float amount) {
    resources_.buildingMaterial += amount;
}

NestChamber* Colony::createChamber(NestChamber::Type type, const XMFLOAT3& position) {
    static std::atomic<uint32_t> chamberID{1};

    auto chamber = std::make_unique<NestChamber>();
    chamber->id = chamberID.fetch_add(1);
    chamber->type = type;
    chamber->position = position;

    // Set size based on type
    switch (type) {
        case NestChamber::Type::ENTRANCE:
            chamber->size = { 0.2f, 0.1f, 0.2f };
            chamber->capacity = 0.0f;
            break;
        case NestChamber::Type::QUEEN_CHAMBER:
            chamber->size = { 0.5f, 0.3f, 0.5f };
            chamber->capacity = 1.0f;
            break;
        case NestChamber::Type::FOOD_STORAGE:
            chamber->size = { 0.4f, 0.3f, 0.4f };
            chamber->capacity = 500.0f;
            break;
        case NestChamber::Type::BROOD_CHAMBER:
            chamber->size = { 0.4f, 0.2f, 0.4f };
            chamber->capacity = 100.0f;  // Larvae capacity
            break;
        case NestChamber::Type::TUNNEL:
            chamber->size = { 0.1f, 0.1f, 0.3f };
            chamber->capacity = 0.0f;
            break;
        default:
            chamber->size = { 0.2f, 0.2f, 0.2f };
            chamber->capacity = 50.0f;
    }

    chamber->currentOccupancy = 0.0f;

    // Connect to nearby chambers
    for (auto& existing : chambers_) {
        float dx = existing->position.x - position.x;
        float dy = existing->position.y - position.y;
        float dz = existing->position.z - position.z;
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);

        if (dist < 0.5f) {
            chamber->connectedTo.push_back(existing->id);
            existing->connectedTo.push_back(chamber->id);
        }
    }

    chambers_.push_back(std::move(chamber));
    return chambers_.back().get();
}

NestChamber* Colony::getNearestChamber(const XMFLOAT3& position, NestChamber::Type type) {
    NestChamber* nearest = nullptr;
    float minDist = FLT_MAX;

    for (auto& chamber : chambers_) {
        if (type != static_cast<NestChamber::Type>(-1) && chamber->type != type) {
            continue;
        }

        float dx = chamber->position.x - position.x;
        float dy = chamber->position.y - position.y;
        float dz = chamber->position.z - position.z;
        float dist = dx * dx + dy * dy + dz * dz;

        if (dist < minDist) {
            minDist = dist;
            nearest = chamber.get();
        }
    }

    return nearest;
}

float Colony::getColonyHealth() const {
    float health = 0.0f;

    // Factor in resources
    health += std::min(resources_.foodStored / 100.0f, 1.0f) * 25.0f;

    // Factor in population
    float targetPop = 50.0f;  // Ideal colony size
    health += std::min(static_cast<float>(members_.size()) / targetPop, 1.0f) * 25.0f;

    // Factor in queen presence
    if (queen_ && queen_->isAlive()) {
        health += 25.0f;
    }

    // Factor in nest integrity
    health += (resources_.nestIntegrity / 100.0f) * 25.0f;

    return health;
}

std::vector<SmallCreature*> Colony::getMembersByRole(ColonyRole role) const {
    std::vector<SmallCreature*> result;
    for (const auto& [id, pair] : members_) {
        if (pair.second == role && pair.first && pair.first->isAlive()) {
            result.push_back(pair.first);
        }
    }
    return result;
}

void Colony::updateTaskAssignments(float deltaTime, SmallCreatureManager& manager) {
    // Update task timers
    for (auto it = taskQueue_.begin(); it != taskQueue_.end(); ) {
        it->timeRemaining -= deltaTime;
        if (it->timeRemaining <= 0.0f) {
            it = taskQueue_.erase(it);
        } else {
            ++it;
        }
    }

    // Assign tasks to idle workers
    for (auto& [id, pair] : members_) {
        auto* creature = pair.first;
        if (!creature || !creature->isAlive()) continue;

        ColonyRole role = pair.second;
        if (role == ColonyRole::QUEEN) continue;

        // Check if already has task
        bool hasTask = false;
        for (const auto& task : taskQueue_) {
            if (task.assignedTo == id) {
                hasTask = true;
                break;
            }
        }

        if (!hasTask) {
            assignTask(creature);
        }
    }
}

void Colony::updateReproduction(float deltaTime, SmallCreatureManager& manager) {
    if (!queen_ || !queen_->isAlive()) return;
    if (reproductionCooldown_ > 0.0f) return;

    // Queen lays eggs based on food availability and colony size
    float foodRatio = resources_.foodStored / (members_.size() + 1.0f);
    if (foodRatio < 5.0f) return;  // Need enough food per member

    // Lay egg
    resources_.eggs++;
    resources_.foodStored -= 10.0f;

    reproductionCooldown_ = 5.0f;  // 5 seconds between eggs

    // Process eggs -> larvae -> pupae -> adults
    static float developmentTimer = 0.0f;
    developmentTimer += deltaTime;

    if (developmentTimer > 10.0f) {
        developmentTimer = 0.0f;

        // Eggs hatch to larvae
        if (resources_.eggs > 0) {
            resources_.eggs--;
            resources_.larvae++;
        }

        // Larvae become pupae
        if (resources_.larvae > 0 && resources_.foodStored > 20.0f) {
            resources_.larvae--;
            resources_.pupae++;
            resources_.foodStored -= 20.0f;
        }

        // Pupae become adults
        if (resources_.pupae > 0) {
            resources_.pupae--;

            // Spawn new worker
            SmallCreatureType workerType = baseType_;
            if (baseType_ == SmallCreatureType::ANT_QUEEN) {
                workerType = SmallCreatureType::ANT_WORKER;
            } else if (baseType_ == SmallCreatureType::BEE_QUEEN) {
                workerType = SmallCreatureType::BEE_WORKER;
            } else if (baseType_ == SmallCreatureType::TERMITE_QUEEN) {
                workerType = SmallCreatureType::TERMITE_WORKER;
            }

            // Spawn near brood chamber
            auto* broodChamber = getNearestChamber(nestPosition_, NestChamber::Type::BROOD_CHAMBER);
            XMFLOAT3 spawnPos = broodChamber ? broodChamber->position : nestPosition_;

            SmallCreature* newWorker = manager.spawn(workerType, spawnPos);
            if (newWorker) {
                addMember(newWorker, ColonyRole::WORKER);
            }
        }
    }
}

void Colony::updateNestMaintenance(float deltaTime) {
    // Nest slowly degrades
    resources_.nestIntegrity -= deltaTime * 0.1f;

    // Builders repair nest
    auto builders = getMembersByRole(ColonyRole::BUILDER);
    float repairRate = builders.size() * 0.5f * deltaTime;

    if (resources_.buildingMaterial > repairRate) {
        resources_.nestIntegrity = std::min(100.0f, resources_.nestIntegrity + repairRate);
        resources_.buildingMaterial -= repairRate;
    }
}

void Colony::assignRoles() {
    // Dynamically assign roles based on colony needs
    int workerCount = 0;
    int soldierCount = 0;
    int foragerCount = 0;
    int nurseCount = 0;

    for (const auto& [id, pair] : members_) {
        switch (pair.second) {
            case ColonyRole::WORKER: workerCount++; break;
            case ColonyRole::SOLDIER: soldierCount++; break;
            case ColonyRole::FORAGER: foragerCount++; break;
            case ColonyRole::NURSE: nurseCount++; break;
            default: break;
        }
    }

    // Target ratios: 50% foragers, 30% workers, 15% soldiers, 5% nurses
    int total = members_.size();
    int targetForagers = total * 0.5f;
    int targetSoldiers = total * 0.15f;
    int targetNurses = std::max(1, (int)(total * 0.05f));

    // Reassign excess workers
    for (auto& [id, pair] : members_) {
        if (pair.second != ColonyRole::WORKER) continue;

        if (foragerCount < targetForagers) {
            pair.second = ColonyRole::FORAGER;
            foragerCount++;
            workerCount--;
        } else if (soldierCount < targetSoldiers) {
            pair.second = ColonyRole::SOLDIER;
            soldierCount++;
            workerCount--;
        } else if (nurseCount < targetNurses) {
            pair.second = ColonyRole::NURSE;
            nurseCount++;
            workerCount--;
        }
    }
}

void Colony::makeColonyDecisions(float deltaTime, SmallCreatureManager& manager) {
    assignRoles();

    // Decide on colony-wide priorities
    float foodUrgency = 1.0f - (resources_.foodStored / (members_.size() * 10.0f + 1.0f));
    float defenseUrgency = underAttack_ ? 1.0f : 0.0f;
    float buildUrgency = 1.0f - (resources_.nestIntegrity / 100.0f);

    // Clear old tasks and create new priority tasks
    if (foodUrgency > 0.7f) {
        // Emergency foraging
        for (int i = 0; i < 5; ++i) {
            ColonyTask task;
            task.type = ColonyTask::Type::FORAGE;
            task.priority = foodUrgency;
            task.timeRemaining = 60.0f;
            task.assignedTo = 0;

            static std::mt19937 rng(std::random_device{}());
            std::uniform_real_distribution<float> dist(-15.0f, 15.0f);
            task.targetPosition = {
                nestPosition_.x + dist(rng),
                nestPosition_.y,
                nestPosition_.z + dist(rng)
            };

            taskQueue_.push_back(task);
        }
    }

    if (defenseUrgency > 0.5f) {
        // Call soldiers to defend
        ColonyTask task;
        task.type = ColonyTask::Type::DEFEND;
        task.priority = defenseUrgency;
        task.timeRemaining = 30.0f;
        task.assignedTo = 0;
        task.targetPosition = nestPosition_;
        taskQueue_.push_back(task);
    }
}

ColonyTask::Type Colony::decideNextPriority() {
    // Calculate needs
    float foodNeed = 1.0f - std::min(resources_.foodStored / 200.0f, 1.0f);
    float buildNeed = 1.0f - resources_.nestIntegrity / 100.0f;
    float defenseNeed = underAttack_ ? 1.0f : 0.0f;
    float patrolNeed = 0.3f;  // Always some patrol

    float total = foodNeed + buildNeed + defenseNeed + patrolNeed;
    if (total < 0.01f) return ColonyTask::Type::IDLE;

    // Weighted random selection
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(0.0f, total);
    float roll = dist(rng);

    if (roll < foodNeed) return ColonyTask::Type::FORAGE;
    roll -= foodNeed;
    if (roll < buildNeed) return ColonyTask::Type::BUILD;
    roll -= buildNeed;
    if (roll < defenseNeed) return ColonyTask::Type::DEFEND;
    return ColonyTask::Type::PATROL;
}

// =============================================================================
// PheromoneSystem Implementation
// =============================================================================

PheromoneSystem::PheromoneSystem(float worldSize)
    : worldSize_(worldSize) {
    points_.reserve(MAX_POINTS);
}

PheromoneSystem::~PheromoneSystem() = default;

void PheromoneSystem::addPheromone(const XMFLOAT3& position, uint32_t colonyID,
                                    PheromonePoint::Type type, float strength) {
    // Check for nearby point to merge with
    for (auto& point : points_) {
        if (point.colonyID != colonyID || point.type != type) continue;

        float dx = point.position.x - position.x;
        float dy = point.position.y - position.y;
        float dz = point.position.z - position.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < MERGE_DISTANCE * MERGE_DISTANCE) {
            // Merge: add strength
            point.strength = std::min(point.strength + strength, 2.0f);
            point.maxStrength = std::max(point.maxStrength, point.strength);
            return;
        }
    }

    // Create new point
    if (points_.size() < MAX_POINTS) {
        PheromonePoint point;
        point.position = position;
        point.strength = strength;
        point.maxStrength = strength;
        point.colonyID = colonyID;
        point.type = type;
        point.age = 0.0f;
        points_.push_back(point);
    }
}

void PheromoneSystem::update(float deltaTime) {
    // Decay all pheromones
    for (auto& point : points_) {
        point.strength -= DECAY_RATE * deltaTime;
        point.age += deltaTime;
    }

    // Cleanup weak pheromones
    cleanup();
}

std::vector<PheromonePoint*> PheromoneSystem::queryNearby(const XMFLOAT3& position, float radius,
                                                          uint32_t colonyID,
                                                          PheromonePoint::Type type) {
    std::vector<PheromonePoint*> result;
    float radiusSq = radius * radius;

    for (auto& point : points_) {
        if (colonyID != 0 && point.colonyID != colonyID) continue;
        if (type != static_cast<PheromonePoint::Type>(-1) && point.type != type) continue;

        float dx = point.position.x - position.x;
        float dy = point.position.y - position.y;
        float dz = point.position.z - position.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq <= radiusSq) {
            result.push_back(&point);
        }
    }

    return result;
}

PheromonePoint* PheromoneSystem::getStrongestInDirection(const XMFLOAT3& position,
                                                          const XMFLOAT3& direction,
                                                          float coneAngle,
                                                          uint32_t colonyID,
                                                          PheromonePoint::Type type) {
    PheromonePoint* strongest = nullptr;
    float maxStrength = 0.0f;
    float cosConeAngle = cosf(coneAngle);

    // Normalize direction
    float dirLen = sqrtf(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
    XMFLOAT3 normDir = direction;
    if (dirLen > 0.001f) {
        normDir.x /= dirLen;
        normDir.y /= dirLen;
        normDir.z /= dirLen;
    }

    for (auto& point : points_) {
        if (point.colonyID != colonyID || point.type != type) continue;

        float dx = point.position.x - position.x;
        float dy = point.position.y - position.y;
        float dz = point.position.z - position.z;
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);

        if (dist < 0.01f) continue;

        // Check if within cone
        float dot = (dx * normDir.x + dy * normDir.y + dz * normDir.z) / dist;
        if (dot < cosConeAngle) continue;

        // Weight by strength and distance
        float score = point.strength / (1.0f + dist * 0.5f);
        if (score > maxStrength) {
            maxStrength = score;
            strongest = &point;
        }
    }

    return strongest;
}

XMFLOAT3 PheromoneSystem::getGradientDirection(const XMFLOAT3& position, float radius,
                                                uint32_t colonyID, PheromonePoint::Type type) {
    XMFLOAT3 gradient = { 0, 0, 0 };
    float totalWeight = 0.0f;

    auto nearby = queryNearby(position, radius, colonyID, type);

    for (auto* point : nearby) {
        float dx = point->position.x - position.x;
        float dy = point->position.y - position.y;
        float dz = point->position.z - position.z;
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);

        if (dist < 0.01f) continue;

        // Weight by strength
        float weight = point->strength;
        gradient.x += (dx / dist) * weight;
        gradient.y += (dy / dist) * weight;
        gradient.z += (dz / dist) * weight;
        totalWeight += weight;
    }

    if (totalWeight > 0.01f) {
        gradient.x /= totalWeight;
        gradient.y /= totalWeight;
        gradient.z /= totalWeight;
    }

    return gradient;
}

void PheromoneSystem::clear() {
    points_.clear();
}

void PheromoneSystem::cleanup() {
    points_.erase(
        std::remove_if(points_.begin(), points_.end(),
            [](const PheromonePoint& p) { return p.strength < MIN_STRENGTH; }),
        points_.end()
    );
}

// =============================================================================
// SwarmBehavior Implementation
// =============================================================================

XMFLOAT3 SwarmBehavior::calculateSwarmForce(SmallCreature* creature,
                                             const std::vector<SmallCreature*>& neighbors,
                                             float separationWeight,
                                             float alignmentWeight,
                                             float cohesionWeight) {
    if (!creature || neighbors.empty()) return { 0, 0, 0 };

    XMFLOAT3 separation = { 0, 0, 0 };
    XMFLOAT3 alignment = { 0, 0, 0 };
    XMFLOAT3 cohesion = { 0, 0, 0 };

    int count = 0;
    for (auto* other : neighbors) {
        if (!other || other->id == creature->id) continue;

        float dx = other->position.x - creature->position.x;
        float dy = other->position.y - creature->position.y;
        float dz = other->position.z - creature->position.z;
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);

        if (dist < 0.01f) continue;

        // Separation - avoid crowding
        float separationDist = 0.5f;  // Personal space
        if (dist < separationDist) {
            float strength = (separationDist - dist) / separationDist;
            separation.x -= (dx / dist) * strength;
            separation.y -= (dy / dist) * strength;
            separation.z -= (dz / dist) * strength;
        }

        // Alignment - match velocity
        alignment.x += other->velocity.x;
        alignment.y += other->velocity.y;
        alignment.z += other->velocity.z;

        // Cohesion - move towards center
        cohesion.x += dx;
        cohesion.y += dy;
        cohesion.z += dz;

        ++count;
    }

    if (count > 0) {
        alignment.x /= count;
        alignment.y /= count;
        alignment.z /= count;

        cohesion.x /= count;
        cohesion.y /= count;
        cohesion.z /= count;
    }

    // Combine forces
    XMFLOAT3 force;
    force.x = separation.x * separationWeight +
              alignment.x * alignmentWeight +
              cohesion.x * cohesionWeight;
    force.y = separation.y * separationWeight +
              alignment.y * alignmentWeight +
              cohesion.y * cohesionWeight;
    force.z = separation.z * separationWeight +
              alignment.z * alignmentWeight +
              cohesion.z * cohesionWeight;

    return force;
}

XMFLOAT3 SwarmBehavior::calculateLocustSwarmForce(SmallCreature* creature,
                                                   const std::vector<SmallCreature*>& neighbors,
                                                   const XMFLOAT3& foodDirection) {
    // Locust swarms are more aggressive in movement
    XMFLOAT3 swarmForce = calculateSwarmForce(creature, neighbors, 1.0f, 2.0f, 1.5f);

    // Add strong bias towards food
    swarmForce.x += foodDirection.x * 3.0f;
    swarmForce.y += foodDirection.y * 0.5f;
    swarmForce.z += foodDirection.z * 3.0f;

    return swarmForce;
}

XMFLOAT3 SwarmBehavior::calculateFlySwarmForce(SmallCreature* creature,
                                                const std::vector<SmallCreature*>& neighbors,
                                                const XMFLOAT3& swarmCenter) {
    // Flies loosely aggregate
    XMFLOAT3 swarmForce = calculateSwarmForce(creature, neighbors, 0.5f, 0.3f, 0.8f);

    // Tendency towards swarm center
    float dx = swarmCenter.x - creature->position.x;
    float dy = swarmCenter.y - creature->position.y;
    float dz = swarmCenter.z - creature->position.z;

    swarmForce.x += dx * 0.1f;
    swarmForce.y += dy * 0.1f;
    swarmForce.z += dz * 0.1f;

    // Add randomness
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> noise(-0.5f, 0.5f);
    swarmForce.x += noise(rng);
    swarmForce.y += noise(rng) * 0.3f;
    swarmForce.z += noise(rng);

    return swarmForce;
}

XMFLOAT3 SwarmBehavior::calculateMosquitoCloudForce(SmallCreature* creature,
                                                     const std::vector<SmallCreature*>& neighbors,
                                                     const XMFLOAT3& targetPosition) {
    XMFLOAT3 swarmForce = calculateSwarmForce(creature, neighbors, 0.3f, 0.2f, 0.5f);

    // Mosquitoes are attracted to targets (hosts)
    float dx = targetPosition.x - creature->position.x;
    float dy = targetPosition.y - creature->position.y;
    float dz = targetPosition.z - creature->position.z;
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);

    if (dist > 0.1f) {
        swarmForce.x += (dx / dist) * 0.5f;
        swarmForce.y += (dy / dist) * 0.5f;
        swarmForce.z += (dz / dist) * 0.5f;
    }

    return swarmForce;
}

// =============================================================================
// AntBehavior Implementation
// =============================================================================

XMFLOAT3 AntBehavior::followTrail(SmallCreature* creature, PheromoneSystem& pheromones) {
    // Get gradient direction for food trails
    XMFLOAT3 gradient = pheromones.getGradientDirection(
        creature->position, creature->genome->smellRange,
        creature->colonyID, PheromonePoint::Type::FOOD_TRAIL);

    // Normalize if significant
    float gradMag = sqrtf(gradient.x * gradient.x + gradient.y * gradient.y + gradient.z * gradient.z);
    if (gradMag > 0.1f) {
        gradient.x /= gradMag;
        gradient.y /= gradMag;
        gradient.z /= gradMag;
    } else {
        // No trail found - random walk
        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        gradient.x = dist(rng);
        gradient.y = 0.0f;
        gradient.z = dist(rng);
    }

    auto props = getProperties(creature->type);
    float speed = props.baseSpeed * creature->genome->speed;

    return {
        gradient.x * speed,
        gradient.y * speed * 0.1f,
        gradient.z * speed
    };
}

void AntBehavior::layTrail(SmallCreature* creature, PheromoneSystem& pheromones,
                           PheromonePoint::Type type, float strength) {
    pheromones.addPheromone(creature->position, creature->colonyID, type, strength);
}

void AntBehavior::recruitNearbyAnts(SmallCreature* leader, Colony& colony,
                                     SmallCreatureManager& manager) {
    // Recruitment pheromone
    manager.getPheromoneSystem()->addPheromone(
        leader->position, leader->colonyID,
        PheromonePoint::Type::RECRUITMENT, 2.0f);
}

XMFLOAT3 AntBehavior::defendFormation(SmallCreature* soldier,
                                       const std::vector<SmallCreature*>& enemies,
                                       const XMFLOAT3& nestEntrance) {
    if (enemies.empty()) {
        // Return to entrance
        float dx = nestEntrance.x - soldier->position.x;
        float dz = nestEntrance.z - soldier->position.z;
        float dist = sqrtf(dx * dx + dz * dz);
        if (dist > 0.1f) {
            return { dx / dist * 0.1f, 0, dz / dist * 0.1f };
        }
        return { 0, 0, 0 };
    }

    // Find nearest enemy
    SmallCreature* nearestEnemy = nullptr;
    float minDist = FLT_MAX;
    for (auto* enemy : enemies) {
        float dx = enemy->position.x - soldier->position.x;
        float dz = enemy->position.z - soldier->position.z;
        float dist = dx * dx + dz * dz;
        if (dist < minDist) {
            minDist = dist;
            nearestEnemy = enemy;
        }
    }

    if (nearestEnemy) {
        // Attack nearest enemy
        float dx = nearestEnemy->position.x - soldier->position.x;
        float dz = nearestEnemy->position.z - soldier->position.z;
        float dist = sqrtf(dx * dx + dz * dz);
        if (dist > 0.01f) {
            return { dx / dist * 0.15f, 0, dz / dist * 0.15f };
        }
    }

    return { 0, 0, 0 };
}

bool AntBehavior::canCarryFood(SmallCreature* ant, float foodWeight) {
    return foodWeight <= getCarryCapacity(ant);
}

float AntBehavior::getCarryCapacity(SmallCreature* ant) {
    // Ants can carry 10-50x their body weight
    auto props = getProperties(ant->type);
    return props.minSize * 50.0f * ant->genome->size;
}

// =============================================================================
// BeeBehavior Implementation
// =============================================================================

void BeeBehavior::performWaggleDance(SmallCreature* bee, const DanceInfo& info,
                                      PheromoneSystem& pheromones) {
    // Waggle dance encodes direction and distance through pheromones
    // Direction is encoded relative to sun position (simplified here)

    float strength = info.quality;

    // Multiple points in the direction
    for (int i = 0; i < 5; ++i) {
        XMFLOAT3 trailPos = bee->position;
        trailPos.x += info.foodDirection.x * (i + 1) * 0.1f;
        trailPos.z += info.foodDirection.z * (i + 1) * 0.1f;

        pheromones.addPheromone(trailPos, bee->colonyID,
                                PheromonePoint::Type::FOOD_TRAIL, strength);
    }
}

BeeBehavior::DanceInfo BeeBehavior::interpretWaggleDance(const XMFLOAT3& dancePosition,
                                                          PheromoneSystem& pheromones) {
    DanceInfo info;
    info.foodDirection = pheromones.getGradientDirection(
        dancePosition, 1.0f, 0, PheromonePoint::Type::FOOD_TRAIL);
    info.distance = 0.0f;  // Would need more complex encoding
    info.quality = 1.0f;
    return info;
}

XMFLOAT3 BeeBehavior::calculateForagingPath(SmallCreature* bee,
                                             const std::vector<XMFLOAT3>& flowerPositions) {
    if (flowerPositions.empty()) {
        // Random search pattern
        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> dist(-10.0f, 10.0f);
        return { dist(rng), 0.0f, dist(rng) };
    }

    // Find nearest flower
    XMFLOAT3 nearestFlower = flowerPositions[0];
    float minDist = FLT_MAX;

    for (const auto& flower : flowerPositions) {
        float dx = flower.x - bee->position.x;
        float dy = flower.y - bee->position.y;
        float dz = flower.z - bee->position.z;
        float dist = dx * dx + dy * dy + dz * dz;

        if (dist < minDist) {
            minDist = dist;
            nearestFlower = flower;
        }
    }

    return nearestFlower;
}

void BeeBehavior::regulateTemperature(SmallCreature* bee, float currentTemp,
                                       float targetTemp) {
    // Bees fan wings to cool or cluster to warm
    if (currentTemp > targetTemp + 2.0f) {
        // Fan wings - increase animation speed
        bee->animationSpeed = 2.0f;
    } else if (currentTemp < targetTemp - 2.0f) {
        // Cluster - reduce movement
        bee->velocity.x *= 0.5f;
        bee->velocity.z *= 0.5f;
    }
}

bool BeeBehavior::shouldSwarm(Colony& colony) {
    // Colony swarms when too large and has multiple potential queens
    return colony.getMemberCount() > 200 && colony.getResources().foodStored > 500.0f;
}

XMFLOAT3 BeeBehavior::findNewNestLocation(SmallCreature* scout,
                                           const XMFLOAT3& currentNest) {
    // Scout searches for new nest location
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(10.0f, 50.0f);
    std::uniform_real_distribution<float> angle(0.0f, 6.28318f);

    float a = angle(rng);
    float d = dist(rng);

    return {
        currentNest.x + cosf(a) * d,
        currentNest.y + 5.0f,  // Prefer elevated locations
        currentNest.z + sinf(a) * d
    };
}

// =============================================================================
// TermiteBehavior Implementation
// =============================================================================

XMFLOAT3 TermiteBehavior::getMoundBuildPosition(SmallCreature* termite,
                                                 const XMFLOAT3& moundCenter,
                                                 float currentHeight) {
    // Build upward in spiral pattern
    static float spiralAngle = 0.0f;
    spiralAngle += 0.1f;

    float radius = 0.5f + currentHeight * 0.1f;

    return {
        moundCenter.x + cosf(spiralAngle) * radius,
        currentHeight + 0.05f,
        moundCenter.z + sinf(spiralAngle) * radius
    };
}

XMFLOAT3 TermiteBehavior::navigateTunnel(SmallCreature* termite,
                                          const std::vector<NestChamber*>& chambers) {
    if (chambers.empty()) return termite->position;

    // Find nearest chamber entrance
    NestChamber* target = nullptr;
    float minDist = FLT_MAX;

    for (auto* chamber : chambers) {
        float dx = chamber->position.x - termite->position.x;
        float dy = chamber->position.y - termite->position.y;
        float dz = chamber->position.z - termite->position.z;
        float dist = dx * dx + dy * dy + dz * dz;

        if (dist < minDist) {
            minDist = dist;
            target = chamber;
        }
    }

    if (target) {
        return target->position;
    }

    return termite->position;
}

float TermiteBehavior::calculateDigestionRate(SmallCreature* termite) {
    // Termites digest wood slowly with gut bacteria
    return 0.01f * termite->genome->metabolism;
}

} // namespace small
