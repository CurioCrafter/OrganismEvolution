#include "FishSchooling.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <execution>
#include <iostream>

#ifdef _WIN32
#include <d3d12.h>
#include <d3dcompiler.h>
#endif

namespace aquatic {

// ============================================================================
// Spatial Hash Grid Implementation
// ============================================================================

SpatialHashGrid::SpatialHashGrid(float cellSize, uint32_t gridSize)
    : m_cellSize(cellSize)
    , m_gridSize(gridSize)
    , m_gridMin(-500.0f)
    , m_gridMax(500.0f) {
    m_cells.resize(gridSize * gridSize * gridSize);
}

void SpatialHashGrid::clear() {
    for (auto& cell : m_cells) {
        cell.startIndex = 0;
        cell.count = 0;
    }
    m_indices.clear();
    m_sortBuffer.clear();
}

uint32_t SpatialHashGrid::getCellIndex(const glm::vec3& position) const {
    glm::vec3 normalized = (position - m_gridMin) / m_cellSize;
    uint32_t x = std::clamp(static_cast<uint32_t>(normalized.x), 0u, m_gridSize - 1);
    uint32_t y = std::clamp(static_cast<uint32_t>(normalized.y), 0u, m_gridSize - 1);
    uint32_t z = std::clamp(static_cast<uint32_t>(normalized.z), 0u, m_gridSize - 1);

    return x + y * m_gridSize + z * m_gridSize * m_gridSize;
}

void SpatialHashGrid::insert(uint32_t fishIndex, const glm::vec3& position) {
    uint32_t cellIndex = getCellIndex(position);
    m_sortBuffer.push_back({cellIndex, fishIndex});
}

void SpatialHashGrid::build(const std::vector<FishState>& fish) {
    clear();

    // Insert all fish
    for (uint32_t i = 0; i < fish.size(); ++i) {
        insert(i, fish[i].position);
    }

    // Sort by cell index
    std::sort(m_sortBuffer.begin(), m_sortBuffer.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    // Build cell start/count
    m_indices.resize(m_sortBuffer.size());

    uint32_t currentCell = UINT32_MAX;
    for (uint32_t i = 0; i < m_sortBuffer.size(); ++i) {
        uint32_t cellIndex = m_sortBuffer[i].first;
        m_indices[i] = m_sortBuffer[i].second;

        if (cellIndex != currentCell) {
            currentCell = cellIndex;
            m_cells[cellIndex].startIndex = i;
            m_cells[cellIndex].count = 1;
        } else {
            m_cells[cellIndex].count++;
        }
    }
}

std::vector<uint32_t> SpatialHashGrid::queryNeighbors(const glm::vec3& position, float radius) const {
    std::vector<uint32_t> result;
    queryNeighborsIntoBuffer(position, radius, result);
    return result;
}

void SpatialHashGrid::queryNeighborsIntoBuffer(const glm::vec3& position, float radius,
                                               std::vector<uint32_t>& outIndices) const {
    outIndices.clear();

    int cellRadius = static_cast<int>(std::ceil(radius / m_cellSize));

    glm::vec3 normalized = (position - m_gridMin) / m_cellSize;
    int baseX = static_cast<int>(normalized.x);
    int baseY = static_cast<int>(normalized.y);
    int baseZ = static_cast<int>(normalized.z);

    for (int dz = -cellRadius; dz <= cellRadius; ++dz) {
        for (int dy = -cellRadius; dy <= cellRadius; ++dy) {
            for (int dx = -cellRadius; dx <= cellRadius; ++dx) {
                int x = baseX + dx;
                int y = baseY + dy;
                int z = baseZ + dz;

                if (x < 0 || x >= static_cast<int>(m_gridSize) ||
                    y < 0 || y >= static_cast<int>(m_gridSize) ||
                    z < 0 || z >= static_cast<int>(m_gridSize)) {
                    continue;
                }

                uint32_t cellIndex = x + y * m_gridSize + z * m_gridSize * m_gridSize;
                const auto& cell = m_cells[cellIndex];

                for (uint32_t i = 0; i < cell.count; ++i) {
                    outIndices.push_back(m_indices[cell.startIndex + i]);
                }
            }
        }
    }
}

// ============================================================================
// Fish Schooling Manager Implementation
// ============================================================================

FishSchoolingManager::FishSchoolingManager()
    : m_spatialGrid(5.0f, 128) {
}

FishSchoolingManager::~FishSchoolingManager() {
    shutdown();
}

bool FishSchoolingManager::initialize(ID3D12Device* device, uint32_t maxFish) {
    m_device = device;
    m_maxFish = maxFish;
    m_fish.reserve(maxFish);

    if (device) {
        m_useGPU = createComputePipeline(device) && createBuffers(device);
    }

    return true;
}

void FishSchoolingManager::shutdown() {
#ifdef _WIN32
    if (m_fishBuffer) { m_fishBuffer->Release(); m_fishBuffer = nullptr; }
    if (m_fishReadbackBuffer) { m_fishReadbackBuffer->Release(); m_fishReadbackBuffer = nullptr; }
    if (m_predatorBuffer) { m_predatorBuffer->Release(); m_predatorBuffer = nullptr; }
    if (m_foodBuffer) { m_foodBuffer->Release(); m_foodBuffer = nullptr; }
    if (m_constantBuffer) { m_constantBuffer->Release(); m_constantBuffer = nullptr; }
    if (m_gridBuffer) { m_gridBuffer->Release(); m_gridBuffer = nullptr; }
    if (m_descriptorHeap) { m_descriptorHeap->Release(); m_descriptorHeap = nullptr; }
    if (m_computePipeline) { m_computePipeline->Release(); m_computePipeline = nullptr; }
    if (m_rootSignature) { m_rootSignature->Release(); m_rootSignature = nullptr; }
#endif

    m_fish.clear();
    m_schools.clear();
    m_predators.clear();
    m_foodSources.clear();
}

uint32_t FishSchoolingManager::addFish(const FishState& fish) {
    if (m_fish.size() >= m_maxFish) {
        return UINT32_MAX;
    }

    uint32_t index = static_cast<uint32_t>(m_fish.size());
    m_fish.push_back(fish);
    return index;
}

void FishSchoolingManager::removeFish(uint32_t index) {
    if (index >= m_fish.size()) return;

    // Swap with last and pop
    if (index != m_fish.size() - 1) {
        m_fish[index] = m_fish.back();
    }
    m_fish.pop_back();
}

void FishSchoolingManager::clearAllFish() {
    m_fish.clear();
    m_schools.clear();
    m_schoolMap.clear();
}

uint32_t FishSchoolingManager::createSchool(uint32_t speciesId, const glm::vec3& center) {
    static uint32_t nextSchoolId = 1;

    School school;
    school.schoolId = nextSchoolId++;
    school.speciesId = speciesId;
    school.centerOfMass = center;

    uint32_t index = static_cast<uint32_t>(m_schools.size());
    m_schoolMap[school.schoolId] = index;
    m_schools.push_back(school);

    return school.schoolId;
}

void FishSchoolingManager::dissolveSchool(uint32_t schoolId) {
    auto it = m_schoolMap.find(schoolId);
    if (it == m_schoolMap.end()) return;

    uint32_t index = it->second;

    // Remove fish association
    for (auto& fish : m_fish) {
        if (fish.schoolId == schoolId) {
            fish.schoolId = 0;
        }
    }

    // Swap and pop
    if (index != m_schools.size() - 1) {
        m_schools[index] = m_schools.back();
        m_schoolMap[m_schools[index].schoolId] = index;
    }
    m_schools.pop_back();
    m_schoolMap.erase(it);
}

void FishSchoolingManager::mergeSchools(uint32_t schoolA, uint32_t schoolB) {
    auto itA = m_schoolMap.find(schoolA);
    auto itB = m_schoolMap.find(schoolB);
    if (itA == m_schoolMap.end() || itB == m_schoolMap.end()) return;

    // Move all fish from B to A
    for (auto& fish : m_fish) {
        if (fish.schoolId == schoolB) {
            fish.schoolId = schoolA;
        }
    }

    dissolveSchool(schoolB);
}

void FishSchoolingManager::updateSchools() {
    // Reset school statistics
    for (auto& school : m_schools) {
        school.fishCount = 0;
        school.centerOfMass = glm::vec3(0.0f);
        school.averageVelocity = glm::vec3(0.0f);
    }

    // Accumulate fish data
    for (const auto& fish : m_fish) {
        auto it = m_schoolMap.find(fish.schoolId);
        if (it == m_schoolMap.end()) continue;

        School& school = m_schools[it->second];
        school.centerOfMass += fish.position;
        school.averageVelocity += fish.velocity;
        school.fishCount++;
    }

    // Finalize averages
    for (auto& school : m_schools) {
        if (school.fishCount > 0) {
            float inv = 1.0f / school.fishCount;
            school.centerOfMass *= inv;
            school.averageVelocity *= inv;

            if (glm::length(school.averageVelocity) > 0.001f) {
                school.averageForward = glm::normalize(school.averageVelocity);
            }
        }

        // Calculate school radius
        float maxDistSq = 0.0f;
        for (const auto& fish : m_fish) {
            if (fish.schoolId == school.schoolId) {
                float distSq = glm::dot(fish.position - school.centerOfMass,
                                        fish.position - school.centerOfMass);
                maxDistSq = std::max(maxDistSq, distSq);
            }
        }
        school.schoolRadius = std::sqrt(maxDistSq);

        // Calculate density
        float volume = (4.0f / 3.0f) * 3.14159f * school.schoolRadius * school.schoolRadius * school.schoolRadius;
        school.density = (volume > 0.001f) ? school.fishCount / volume : 0.0f;

        school.averageDepth = -school.centerOfMass.y; // Assuming Y is up, depth is down
    }
}

void FishSchoolingManager::addPredatorPosition(const glm::vec3& position, float threatLevel) {
    m_predators.push_back(glm::vec4(position, threatLevel));
}

void FishSchoolingManager::clearPredators() {
    m_predators.clear();
}

void FishSchoolingManager::addFoodSource(const glm::vec3& position, float value) {
    m_foodSources.push_back(glm::vec4(position, value));
}

void FishSchoolingManager::clearFoodSources() {
    m_foodSources.clear();
}

void FishSchoolingManager::update(float deltaTime) {
    m_time += deltaTime;
    m_frameNumber++;

    if (m_fish.empty()) return;

    // Always update CPU for now (GPU path needs more setup)
    updateCPU(deltaTime);

    updateSchools();
}

void FishSchoolingManager::updateCPU(float deltaTime) {
    // Rebuild spatial hash
    m_spatialGrid.build(m_fish);

    // Parallel update if enough fish
    if (m_fish.size() > 100) {
        std::vector<uint32_t> indices(m_fish.size());
        for (uint32_t i = 0; i < m_fish.size(); ++i) indices[i] = i;

        // Thread-local neighbor buffers
        std::for_each(std::execution::par_unseq, indices.begin(), indices.end(),
            [this, deltaTime](uint32_t i) {
                thread_local std::vector<uint32_t> neighbors;
                neighbors.clear();

                float maxRadius = std::max({m_config.separationRadius,
                                           m_config.alignmentRadius,
                                           m_config.cohesionRadius});

                m_spatialGrid.queryNeighborsIntoBuffer(m_fish[i].position, maxRadius, neighbors);

                glm::vec3 steering(0.0f);
                steering += calculateSeparation(i, neighbors);
                steering += calculateAlignment(i, neighbors);
                steering += calculateCohesion(i, neighbors);
                steering += calculatePredatorAvoidance(i);
                steering += calculateFoodAttraction(i);
                steering += calculateWander(i, deltaTime);
                steering += calculateBoundaryAvoidance(i);
                steering += calculateDepthCorrection(i);

                integrateMotion(i, steering, deltaTime);
                updateSwimAnimation(i, deltaTime);
            });
    } else {
        std::vector<uint32_t> neighbors;

        for (uint32_t i = 0; i < m_fish.size(); ++i) {
            neighbors.clear();

            float maxRadius = std::max({m_config.separationRadius,
                                       m_config.alignmentRadius,
                                       m_config.cohesionRadius});

            m_spatialGrid.queryNeighborsIntoBuffer(m_fish[i].position, maxRadius, neighbors);

            glm::vec3 steering(0.0f);
            steering += calculateSeparation(i, neighbors);
            steering += calculateAlignment(i, neighbors);
            steering += calculateCohesion(i, neighbors);
            steering += calculatePredatorAvoidance(i);
            steering += calculateFoodAttraction(i);
            steering += calculateWander(i, deltaTime);
            steering += calculateBoundaryAvoidance(i);
            steering += calculateDepthCorrection(i);

            integrateMotion(i, steering, deltaTime);
            updateSwimAnimation(i, deltaTime);
        }
    }

    propagatePanic(deltaTime);
    assignToSchools();
}

glm::vec3 FishSchoolingManager::calculateSeparation(uint32_t fishIndex,
                                                    const std::vector<uint32_t>& neighbors) {
    const FishState& fish = m_fish[fishIndex];
    glm::vec3 steering(0.0f);
    int count = 0;

    float radius = m_config.separationRadius;
    float radiusSq = radius * radius;

    for (uint32_t neighborIdx : neighbors) {
        if (neighborIdx == fishIndex) continue;

        const FishState& neighbor = m_fish[neighborIdx];
        glm::vec3 diff = fish.position - neighbor.position;
        float distSq = glm::dot(diff, diff);

        if (distSq < radiusSq && distSq > 0.0001f) {
            float dist = std::sqrt(distSq);
            // Weight by inverse distance (closer = stronger repulsion)
            steering += glm::normalize(diff) * (1.0f - dist / radius);
            count++;
        }
    }

    if (count > 0) {
        steering /= static_cast<float>(count);
        steering *= m_config.separationWeight * fish.separationWeight;

        // Limit force
        float len = glm::length(steering);
        if (len > m_config.separationMaxForce) {
            steering = steering / len * m_config.separationMaxForce;
        }
    }

    return steering;
}

glm::vec3 FishSchoolingManager::calculateAlignment(uint32_t fishIndex,
                                                   const std::vector<uint32_t>& neighbors) {
    const FishState& fish = m_fish[fishIndex];
    glm::vec3 averageVelocity(0.0f);
    int count = 0;

    float radius = m_config.alignmentRadius;
    float radiusSq = radius * radius;

    for (uint32_t neighborIdx : neighbors) {
        if (neighborIdx == fishIndex) continue;

        const FishState& neighbor = m_fish[neighborIdx];

        // Only align with same school
        if (neighbor.schoolId != fish.schoolId && fish.schoolId != 0) continue;

        glm::vec3 diff = fish.position - neighbor.position;
        float distSq = glm::dot(diff, diff);

        if (distSq < radiusSq) {
            averageVelocity += neighbor.velocity;
            count++;
        }
    }

    glm::vec3 steering(0.0f);
    if (count > 0) {
        averageVelocity /= static_cast<float>(count);
        steering = averageVelocity - fish.velocity;
        steering *= m_config.alignmentWeight * fish.alignmentWeight;

        float len = glm::length(steering);
        if (len > m_config.alignmentMaxForce) {
            steering = steering / len * m_config.alignmentMaxForce;
        }
    }

    return steering;
}

glm::vec3 FishSchoolingManager::calculateCohesion(uint32_t fishIndex,
                                                  const std::vector<uint32_t>& neighbors) {
    const FishState& fish = m_fish[fishIndex];
    glm::vec3 centerOfMass(0.0f);
    int count = 0;

    float radius = m_config.cohesionRadius;
    float radiusSq = radius * radius;

    for (uint32_t neighborIdx : neighbors) {
        if (neighborIdx == fishIndex) continue;

        const FishState& neighbor = m_fish[neighborIdx];

        // Only cohere with same school
        if (neighbor.schoolId != fish.schoolId && fish.schoolId != 0) continue;

        glm::vec3 diff = fish.position - neighbor.position;
        float distSq = glm::dot(diff, diff);

        if (distSq < radiusSq) {
            centerOfMass += neighbor.position;
            count++;
        }
    }

    glm::vec3 steering(0.0f);
    if (count > 0) {
        centerOfMass /= static_cast<float>(count);
        glm::vec3 desired = centerOfMass - fish.position;

        float dist = glm::length(desired);
        if (dist > 0.001f) {
            // Modulate by distance - farther away = stronger pull
            float factor = std::min(dist / radius, 1.0f);
            steering = glm::normalize(desired) * factor * m_config.cohesionWeight * fish.cohesionWeight;

            float len = glm::length(steering);
            if (len > m_config.cohesionMaxForce) {
                steering = steering / len * m_config.cohesionMaxForce;
            }
        }
    }

    return steering;
}

glm::vec3 FishSchoolingManager::calculatePredatorAvoidance(uint32_t fishIndex) {
    const FishState& fish = m_fish[fishIndex];
    glm::vec3 steering(0.0f);

    float detectionRange = m_config.predatorDetectionRange;
    float detectionRangeSq = detectionRange * detectionRange;

    for (const auto& predator : m_predators) {
        glm::vec3 predPos(predator);
        glm::vec3 diff = fish.position - predPos;
        float distSq = glm::dot(diff, diff);

        if (distSq < detectionRangeSq && distSq > 0.0001f) {
            float dist = std::sqrt(distSq);
            float urgency = 1.0f - (dist / detectionRange);
            urgency *= urgency; // Quadratic urgency - panic increases rapidly when close

            float threatLevel = predator.w;
            steering += glm::normalize(diff) * urgency * threatLevel * m_config.predatorFleeForce;

            // Increase panic level
            m_fish[fishIndex].panicLevel = std::min(m_fish[fishIndex].panicLevel + urgency * 0.5f, 1.0f);
        }
    }

    return steering;
}

glm::vec3 FishSchoolingManager::calculateFoodAttraction(uint32_t fishIndex) {
    const FishState& fish = m_fish[fishIndex];

    // Don't seek food when panicked
    if (fish.panicLevel > 0.5f) return glm::vec3(0.0f);

    glm::vec3 steering(0.0f);
    float nearestDistSq = FLT_MAX;
    glm::vec3 nearestFood(0.0f);

    for (const auto& food : m_foodSources) {
        glm::vec3 foodPos(food);
        glm::vec3 diff = foodPos - fish.position;
        float distSq = glm::dot(diff, diff);

        if (distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearestFood = foodPos;
        }
    }

    if (nearestDistSq < 100.0f * 100.0f) { // Within 100 units
        glm::vec3 desired = nearestFood - fish.position;
        float dist = std::sqrt(nearestDistSq);

        if (dist > 0.001f) {
            // Arrive behavior - slow down as approaching
            float desiredSpeed = m_config.maxSpeed;
            if (dist < 10.0f) {
                desiredSpeed *= dist / 10.0f;
            }

            steering = glm::normalize(desired) * desiredSpeed - fish.velocity;

            // Low priority compared to survival
            steering *= 0.3f * (1.0f - fish.panicLevel);
        }
    }

    return steering;
}

glm::vec3 FishSchoolingManager::calculateWander(uint32_t fishIndex, float deltaTime) {
    FishState& fish = m_fish[fishIndex];

    // Simple wander using noise
    float seed = fishIndex * 0.1f + m_time * 0.5f;
    float noiseX = std::sin(seed * 1.3f) * std::cos(seed * 0.7f);
    float noiseY = std::sin(seed * 0.8f) * std::cos(seed * 1.1f);
    float noiseZ = std::sin(seed * 1.1f) * std::cos(seed * 0.9f);

    glm::vec3 wander(noiseX, noiseY * 0.3f, noiseZ); // Less vertical wandering
    wander *= m_config.wanderStrength;

    return wander;
}

glm::vec3 FishSchoolingManager::calculateBoundaryAvoidance(uint32_t fishIndex) {
    const FishState& fish = m_fish[fishIndex];
    glm::vec3 steering(0.0f);

    float margin = 20.0f;
    float forceMult = 5.0f;

    // Water surface
    if (fish.position.y > m_waterSurfaceY - margin) {
        float dist = m_waterSurfaceY - margin - fish.position.y;
        steering.y -= forceMult * (1.0f - dist / margin);
    }

    // Sea floor
    if (fish.position.y < m_seaFloorY + margin) {
        float dist = fish.position.y - (m_seaFloorY + margin);
        steering.y += forceMult * (1.0f - dist / margin);
    }

    // World bounds (simple box)
    float worldBound = 400.0f;
    if (std::abs(fish.position.x) > worldBound - margin) {
        steering.x -= glm::sign(fish.position.x) * forceMult;
    }
    if (std::abs(fish.position.z) > worldBound - margin) {
        steering.z -= glm::sign(fish.position.z) * forceMult;
    }

    return steering;
}

glm::vec3 FishSchoolingManager::calculateDepthCorrection(uint32_t fishIndex) {
    FishState& fish = m_fish[fishIndex];
    glm::vec3 steering(0.0f);

    float currentDepth = -fish.position.y;
    float targetDepth = fish.targetDepth;

    float depthError = targetDepth - currentDepth;
    steering.y = -depthError * m_config.verticalCorrectionForce * 0.1f;

    // Limit vertical correction
    steering.y = std::clamp(steering.y, -2.0f, 2.0f);

    return steering;
}

void FishSchoolingManager::integrateMotion(uint32_t fishIndex, const glm::vec3& steering,
                                          float deltaTime) {
    FishState& fish = m_fish[fishIndex];

    // Apply acceleration limit
    glm::vec3 acceleration = steering;
    float accelLen = glm::length(acceleration);
    if (accelLen > m_config.maxAcceleration) {
        acceleration = acceleration / accelLen * m_config.maxAcceleration;
    }

    // Apply water current
    acceleration += m_currentDirection * m_currentStrength;

    fish.acceleration = acceleration;
    fish.velocity += acceleration * deltaTime;

    // Apply speed limits
    float speed = glm::length(fish.velocity);
    float maxSpd = fish.maxSpeed;

    // Burst speed when panicked
    if (fish.panicLevel > 0.5f) {
        maxSpd *= 1.0f + fish.panicLevel;
    }

    if (speed > maxSpd) {
        fish.velocity = fish.velocity / speed * maxSpd;
        speed = maxSpd;
    } else if (speed < m_config.minSpeed) {
        if (speed > 0.001f) {
            fish.velocity = fish.velocity / speed * m_config.minSpeed;
        } else {
            fish.velocity = fish.forward * m_config.minSpeed;
        }
        speed = m_config.minSpeed;
    }

    fish.speed = speed;

    // Update position
    fish.position += fish.velocity * deltaTime;

    // Update forward direction
    if (speed > 0.001f) {
        glm::vec3 newForward = fish.velocity / speed;

        // Smooth turn
        float turnAmount = m_config.turnRate * deltaTime;
        fish.forward = glm::normalize(glm::mix(fish.forward, newForward, std::min(turnAmount, 1.0f)));
    }

    // Decay panic
    fish.panicLevel = std::max(0.0f, fish.panicLevel - m_config.panicDecayRate * deltaTime);

    // Energy drain
    float energyDrain = 0.1f * deltaTime;
    if (fish.panicLevel > 0.1f) {
        energyDrain *= 1.0f + fish.panicLevel * 2.0f;
    }
    fish.energy = std::max(0.0f, fish.energy - energyDrain);

    // Age
    fish.age += deltaTime;
}

void FishSchoolingManager::updateSwimAnimation(uint32_t fishIndex, float deltaTime) {
    FishState& fish = m_fish[fishIndex];

    // Swim phase based on speed and genome
    float frequency = 2.0f + fish.speed * 0.3f;
    if (fish.panicLevel > 0.3f) {
        frequency *= 1.0f + fish.panicLevel;
    }

    fish.swimPhase += frequency * deltaTime * 6.28318f;
    if (fish.swimPhase > 6.28318f * 100.0f) {
        fish.swimPhase -= 6.28318f * 100.0f;
    }
}

void FishSchoolingManager::propagatePanic(float deltaTime) {
    // Fish can spread panic to nearby fish
    for (size_t i = 0; i < m_fish.size(); ++i) {
        if (m_fish[i].panicLevel < 0.3f) continue;

        std::vector<uint32_t> neighbors;
        m_spatialGrid.queryNeighborsIntoBuffer(m_fish[i].position,
                                               m_config.panicSpreadRadius, neighbors);

        for (uint32_t neighborIdx : neighbors) {
            if (neighborIdx == i) continue;

            FishState& neighbor = m_fish[neighborIdx];
            float panicSpread = m_fish[i].panicLevel * 0.2f * deltaTime;
            neighbor.panicLevel = std::min(neighbor.panicLevel + panicSpread, 1.0f);
        }
    }
}

void FishSchoolingManager::assignToSchools() {
    // Simple school assignment based on proximity and species
    // More sophisticated clustering could be added

    for (auto& fish : m_fish) {
        if (fish.schoolId != 0) continue; // Already in school

        // Find nearby fish of same species
        std::vector<uint32_t> neighbors;
        m_spatialGrid.queryNeighborsIntoBuffer(fish.position, m_config.schoolMergeDistance, neighbors);

        uint32_t bestSchool = 0;
        int bestCount = 0;

        for (uint32_t neighborIdx : neighbors) {
            const FishState& neighbor = m_fish[neighborIdx];
            if (neighbor.speciesId == fish.speciesId && neighbor.schoolId != 0) {
                // Count fish in this school nearby
                int count = 0;
                for (uint32_t n2 : neighbors) {
                    if (m_fish[n2].schoolId == neighbor.schoolId) count++;
                }
                if (count > bestCount) {
                    bestCount = count;
                    bestSchool = neighbor.schoolId;
                }
            }
        }

        if (bestSchool != 0 && bestCount >= 3) {
            fish.schoolId = bestSchool;
        }
    }
}

void FishSchoolingManager::setWaterBounds(float surfaceY, float floorY) {
    m_waterSurfaceY = surfaceY;
    m_seaFloorY = floorY;
}

void FishSchoolingManager::setCurrentDirection(const glm::vec3& direction, float strength) {
    m_currentDirection = glm::normalize(direction);
    m_currentStrength = strength;
}

FishState* FishSchoolingManager::getFishPtr(uint32_t index) {
    return (index < m_fish.size()) ? &m_fish[index] : nullptr;
}

const FishState* FishSchoolingManager::getFishPtr(uint32_t index) const {
    return (index < m_fish.size()) ? &m_fish[index] : nullptr;
}

void FishSchoolingManager::getSchoolStatistics(uint32_t schoolId, float& outDensity,
                                               float& outCohesion, float& outPanicLevel) const {
    auto it = m_schoolMap.find(schoolId);
    if (it == m_schoolMap.end()) {
        outDensity = 0.0f;
        outCohesion = 0.0f;
        outPanicLevel = 0.0f;
        return;
    }

    const School& school = m_schools[it->second];
    outDensity = school.density;
    outCohesion = school.cohesionLevel;
    outPanicLevel = school.panicLevel;
}

// GPU pipeline creation - NOT IMPLEMENTED, uses CPU fallback
// If you need GPU acceleration, implement full DX12 compute shader pipeline
bool FishSchoolingManager::createComputePipeline(ID3D12Device* device) {
    // GPU compute not implemented - using CPU fallback
    // To implement: compile HLSL compute shader and create PSO
    std::cerr << "FishSchoolingManager: GPU pipeline not implemented, using CPU fallback" << std::endl;
    return false;  // Indicates GPU unavailable, caller should use CPU path
}

bool FishSchoolingManager::createBuffers(ID3D12Device* device) {
    // GPU buffers not implemented - using CPU fallback
    // To implement: create UAV buffers for fish position/velocity data
    return false;  // Indicates GPU unavailable, caller should use CPU path
}

void FishSchoolingManager::updateGPU(ID3D12GraphicsCommandList* commandList, float deltaTime) {
    // GPU dispatch not implemented - fall back to CPU
    // This is the expected behavior when createComputePipeline returns false
    updateCPU(deltaTime);
}

void FishSchoolingManager::syncFromGPU(ID3D12GraphicsCommandList* commandList) {
    // No GPU sync needed when using CPU path
    // This is intentionally empty - not a bug
}

void FishSchoolingManager::updateConstantBuffer() {
    m_constants.separationRadius = m_config.separationRadius;
    m_constants.separationWeight = m_config.separationWeight;
    m_constants.alignmentRadius = m_config.alignmentRadius;
    m_constants.alignmentWeight = m_config.alignmentWeight;
    m_constants.cohesionRadius = m_config.cohesionRadius;
    m_constants.cohesionWeight = m_config.cohesionWeight;
    m_constants.predatorDetectionRange = m_config.predatorDetectionRange;
    m_constants.predatorFleeForce = m_config.predatorFleeForce;
    m_constants.maxSpeed = m_config.maxSpeed;
    m_constants.maxAcceleration = m_config.maxAcceleration;
    m_constants.minSpeed = m_config.minSpeed;
    m_constants.turnRate = m_config.turnRate;
    m_constants.wanderStrength = m_config.wanderStrength;
    m_constants.wanderRadius = m_config.wanderRadius;
    m_constants.panicDecayRate = m_config.panicDecayRate;
    m_constants.waterSurfaceY = m_waterSurfaceY;
    m_constants.seaFloorY = m_seaFloorY;
    m_constants.preferredDepth = m_config.preferredDepth;
    m_constants.depthVariation = m_config.depthVariation;
    m_constants.currentDirection = m_currentDirection;
    m_constants.currentStrength = m_currentStrength;
    m_constants.fishCount = static_cast<uint32_t>(m_fish.size());
    m_constants.predatorCount = static_cast<uint32_t>(m_predators.size());
    m_constants.foodSourceCount = static_cast<uint32_t>(m_foodSources.size());
    m_constants.randomSeed = m_frameNumber * 1103515245 + 12345;
    m_constants.frameNumber = m_frameNumber;
    m_constants.time = m_time;
}

// ============================================================================
// Schooling Utilities Implementation
// ============================================================================

namespace SchoolingUtils {

glm::vec3 calculateFormationOffset(FormationType type, uint32_t fishIndex,
                                   uint32_t totalFish, float baseRadius) {
    float angle = (fishIndex / static_cast<float>(totalFish)) * 6.28318f;
    float heightRatio = static_cast<float>(fishIndex % 10) / 10.0f;

    switch (type) {
        case FormationType::SPHERE: {
            // Golden spiral distribution
            float phi = 3.14159f * (3.0f - std::sqrt(5.0f)); // Golden angle
            float y = 1.0f - (fishIndex / static_cast<float>(totalFish - 1)) * 2.0f;
            float radiusAtY = std::sqrt(1.0f - y * y);
            float theta = phi * fishIndex;

            return glm::vec3(
                std::cos(theta) * radiusAtY * baseRadius,
                y * baseRadius,
                std::sin(theta) * radiusAtY * baseRadius
            );
        }

        case FormationType::TORNADO: {
            float height = (fishIndex / static_cast<float>(totalFish)) * baseRadius * 2.0f - baseRadius;
            float radius = baseRadius * (1.0f - std::abs(height) / baseRadius) * 0.5f + baseRadius * 0.5f;
            return glm::vec3(
                std::cos(angle * 3.0f + height * 0.5f) * radius,
                height,
                std::sin(angle * 3.0f + height * 0.5f) * radius
            );
        }

        case FormationType::FLAT: {
            float radius = std::sqrt(fishIndex / static_cast<float>(totalFish)) * baseRadius;
            return glm::vec3(
                std::cos(angle * 7.0f) * radius,
                (heightRatio - 0.5f) * baseRadius * 0.2f,
                std::sin(angle * 7.0f) * radius
            );
        }

        case FormationType::STREAM: {
            float streamPos = fishIndex / static_cast<float>(totalFish);
            float lateralOffset = std::sin(fishIndex * 0.5f) * baseRadius * 0.3f;
            return glm::vec3(
                lateralOffset,
                (heightRatio - 0.5f) * baseRadius * 0.3f,
                streamPos * baseRadius * 4.0f
            );
        }

        case FormationType::V_FORMATION: {
            uint32_t side = fishIndex % 2;
            uint32_t posInLine = fishIndex / 2;
            float offset = posInLine * 2.0f;
            return glm::vec3(
                (side == 0 ? -1.0f : 1.0f) * offset,
                0.0f,
                -offset * 0.5f
            );
        }

        default:
            return glm::vec3(0.0f);
    }
}

float calculatePanicFromPredator(const glm::vec3& fishPos, const glm::vec3& predatorPos,
                                 float detectionRange) {
    float dist = glm::length(fishPos - predatorPos);
    if (dist >= detectionRange) return 0.0f;

    float normalizedDist = dist / detectionRange;
    return (1.0f - normalizedDist) * (1.0f - normalizedDist);
}

bool shouldMergeSchools(const School& a, const School& b,
                       float mergeDistance, bool sameSpecies) {
    if (!sameSpecies && a.speciesId != b.speciesId) return false;

    float dist = glm::length(a.centerOfMass - b.centerOfMass);
    return dist < mergeDistance;
}

float calculateSwimmingEnergyCost(float speed, float maxSpeed, bool inSchool,
                                 float schoolEnergyBonus) {
    float speedRatio = speed / maxSpeed;
    float baseCost = speedRatio * speedRatio; // Quadratic cost

    if (inSchool) {
        baseCost *= (1.0f - schoolEnergyBonus);
    }

    return baseCost;
}

glm::vec3 calculateWanderTarget(const glm::vec3& currentTarget,
                               float wanderRadius, float jitter, uint32_t seed) {
    // Simple random displacement
    float s1 = ((seed * 1103515245 + 12345) & 0x7FFFFFFF) / static_cast<float>(0x7FFFFFFF);
    float s2 = ((seed * 1103515245 * 2 + 12345) & 0x7FFFFFFF) / static_cast<float>(0x7FFFFFFF);

    glm::vec3 displacement(
        (s1 - 0.5f) * 2.0f * jitter,
        (s2 - 0.5f) * jitter, // Less vertical
        (s1 * s2 - 0.25f) * 2.0f * jitter
    );

    glm::vec3 newTarget = currentTarget + displacement;

    // Keep on sphere surface
    float len = glm::length(newTarget);
    if (len > 0.001f) {
        newTarget = newTarget / len * wanderRadius;
    }

    return newTarget;
}

} // namespace SchoolingUtils

// ============================================================================
// Extended School Dynamics Implementation (Phase 7)
// ============================================================================

void SchoolDynamics::update(float deltaTime) {
    // Update panic wave propagation
    if (panicWaveRadius > 0.0f) {
        panicWaveRadius += panicWaveSpeed * deltaTime;

        // Decay wave after reaching certain size
        if (panicWaveRadius > 100.0f) {
            panicWaveRadius = 0.0f;
            if (state == SchoolBehaviorState::PANIC_SCATTER) {
                state = SchoolBehaviorState::REFORMING;
                stateStartTime = 0.0f;
                rejoinTimer = 5.0f;
            }
        }
    }

    // Update rejoin timer
    if (state == SchoolBehaviorState::REFORMING) {
        rejoinTimer -= deltaTime;
        if (rejoinTimer <= 0.0f) {
            state = SchoolBehaviorState::CRUISING;
            splitPositions.clear();
        }
    }

    // Update intensity based on state
    switch (state) {
        case SchoolBehaviorState::PANIC_SCATTER:
            intensityMultiplier = std::max(0.3f, intensityMultiplier - deltaTime * 0.5f);
            break;
        case SchoolBehaviorState::REFORMING:
            intensityMultiplier = std::min(1.0f, intensityMultiplier + deltaTime * 0.3f);
            break;
        case SchoolBehaviorState::FEEDING_FRENZY:
            intensityMultiplier = 0.7f;  // Looser formation while feeding
            break;
        case SchoolBehaviorState::LEADER_FOLLOWING:
            intensityMultiplier = 1.5f;  // Tighter formation following leader
            break;
        default:
            intensityMultiplier = 1.0f;
            break;
    }
}

void SchoolDynamics::triggerPanicWave(const glm::vec3& origin, float time) {
    panicOrigin = origin;
    panicWaveRadius = 1.0f;  // Start small
    state = SchoolBehaviorState::PANIC_SCATTER;
    stateStartTime = time;
}

void SchoolDynamics::requestSplit(int numGroups) {
    if (numGroups < 2) return;

    splitPositions.clear();

    // Generate split directions (radial)
    float angleStep = 6.28318f / numGroups;
    for (int i = 0; i < numGroups; ++i) {
        float angle = i * angleStep;
        splitPositions.push_back(glm::vec3(
            std::cos(angle) * splitDistance,
            0.0f,
            std::sin(angle) * splitDistance
        ));
    }
}

void SchoolDynamics::requestRejoin() {
    state = SchoolBehaviorState::REFORMING;
    rejoinTimer = 3.0f;
}

std::string SchoolDynamics::getStateName() const {
    switch (state) {
        case SchoolBehaviorState::CRUISING: return "Cruising";
        case SchoolBehaviorState::FEEDING_FRENZY: return "Feeding Frenzy";
        case SchoolBehaviorState::PANIC_SCATTER: return "Panic Scatter";
        case SchoolBehaviorState::REFORMING: return "Reforming";
        case SchoolBehaviorState::LEADER_FOLLOWING: return "Following Leader";
        case SchoolBehaviorState::DEPTH_MIGRATION: return "Depth Migration";
        default: return "Unknown";
    }
}

glm::vec3 calculateLeaderFollowForce(const glm::vec3& fishPos, const glm::vec3& leaderPos,
                                     const glm::vec3& leaderVel, float followDistance) {
    // Predict where leader will be
    glm::vec3 predictedPos = leaderPos + leaderVel * 0.5f;

    // Stay behind and to the side of leader
    glm::vec3 leaderDir = glm::length(leaderVel) > 0.01f ?
                          glm::normalize(leaderVel) : glm::vec3(1.0f, 0.0f, 0.0f);

    // Target position is behind leader at follow distance
    glm::vec3 targetPos = predictedPos - leaderDir * followDistance;

    glm::vec3 toTarget = targetPos - fishPos;
    float dist = glm::length(toTarget);

    if (dist < 0.5f) return glm::vec3(0.0f);

    // Stronger force when far from desired position
    float strength = std::min(dist / followDistance, 2.0f);
    return glm::normalize(toTarget) * strength;
}

glm::vec3 calculatePanicWaveForce(const glm::vec3& fishPos, const glm::vec3& panicOrigin,
                                  float waveRadius, float waveIntensity) {
    glm::vec3 awayDir = fishPos - panicOrigin;
    float dist = glm::length(awayDir);

    if (dist < 0.1f) {
        // At origin - flee in random direction
        return glm::vec3(1.0f, 0.0f, 0.0f);
    }

    // Wave front effect - strongest at wavefront
    float distFromWave = std::abs(dist - waveRadius);
    float waveFactor = std::exp(-distFromWave * 0.2f);

    // Only affect fish near the wavefront
    if (distFromWave > 10.0f) return glm::vec3(0.0f);

    return glm::normalize(awayDir) * waveFactor * waveIntensity;
}

float calculateLeaderScore(float age, float size, float energy, float survivalTime) {
    // Older, larger, healthier fish make better leaders
    float ageScore = std::min(age / 100.0f, 1.0f) * 0.3f;
    float sizeScore = std::min(size / 2.0f, 1.0f) * 0.3f;
    float energyScore = energy * 0.2f;
    float survivalScore = std::min(survivalTime / 300.0f, 1.0f) * 0.2f;

    return ageScore + sizeScore + energyScore + survivalScore;
}

} // namespace aquatic
