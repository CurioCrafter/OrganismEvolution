#include "ArchipelagoGenerator.h"
#include <algorithm>
#include <cmath>

// Name generation syllables for island names
const std::vector<std::string> ArchipelagoGenerator::NAME_PREFIXES = {
    "Ka", "Ma", "Ta", "Na", "Sa", "Ha", "La", "Ra", "Wa", "Ya",
    "Ko", "Mo", "To", "No", "So", "Ho", "Lo", "Ro", "Wo", "Yo",
    "Ai", "Ei", "Oi", "Ui", "Au", "Eu", "Ou"
};

const std::vector<std::string> ArchipelagoGenerator::NAME_ROOTS = {
    "lani", "nui", "kai", "hana", "moku", "pali", "wai", "lei",
    "ola", "mana", "ahi", "lua", "one", "iki", "loa", "kea",
    "ino", "ula", "ena", "ana", "onu", "ara", "ela", "ora"
};

const std::vector<std::string> ArchipelagoGenerator::NAME_SUFFIXES = {
    "ia", "ua", "a", "i", "u", "o", "e", "ni", "li", "ki",
    "ti", "ri", "na", "la", "ka", "ta", "ra", "", "", ""  // Empty for variation
};

// ============================================================================
// ArchipelagoData Helper Methods
// ============================================================================

const IslandConfig* ArchipelagoData::getNearestIsland(const glm::vec2& position) const {
    if (islands.empty()) return nullptr;

    const IslandConfig* nearest = nullptr;
    float minDist = std::numeric_limits<float>::max();

    for (const auto& island : islands) {
        float dist = glm::length(position - island.position);
        if (dist < minDist) {
            minDist = dist;
            nearest = &island;
        }
    }

    return nearest;
}

float ArchipelagoData::getDistanceBetween(uint32_t islandA, uint32_t islandB) const {
    if (islandA >= islands.size() || islandB >= islands.size()) {
        return std::numeric_limits<float>::max();
    }

    return glm::length(islands[islandA].position - islands[islandB].position);
}

const OceanCurrent* ArchipelagoData::getCurrentBetween(uint32_t fromIsland, uint32_t toIsland) const {
    for (const auto& current : currents) {
        if (current.fromIsland == fromIsland && current.toIsland == toIsland) {
            return &current;
        }
        if (current.bidirectional && current.fromIsland == toIsland && current.toIsland == fromIsland) {
            return &current;
        }
    }
    return nullptr;
}

std::vector<uint32_t> ArchipelagoData::getNeighborIslands(uint32_t islandIndex, float maxDistance) const {
    std::vector<uint32_t> neighbors;

    if (islandIndex >= islands.size()) return neighbors;

    const glm::vec2& pos = islands[islandIndex].position;

    for (uint32_t i = 0; i < static_cast<uint32_t>(islands.size()); ++i) {
        if (i == islandIndex) continue;

        float dist = glm::length(islands[i].position - pos);
        if (dist <= maxDistance) {
            neighbors.push_back(i);
        }
    }

    // Sort by distance
    std::sort(neighbors.begin(), neighbors.end(), [this, &pos](uint32_t a, uint32_t b) {
        return glm::length(islands[a].position - pos) < glm::length(islands[b].position - pos);
    });

    return neighbors;
}

// ============================================================================
// ArchipelagoGenerator Implementation
// ============================================================================

ArchipelagoGenerator::ArchipelagoGenerator()
    : m_baseSeed(12345)
    , m_sizeVariation(0.4f) {

    // Default shape distribution
    m_shapeWeights = {
        {IslandShape::IRREGULAR, 0.35f},
        {IslandShape::CIRCULAR, 0.20f},
        {IslandShape::VOLCANIC, 0.15f},
        {IslandShape::CRESCENT, 0.10f},
        {IslandShape::ATOLL, 0.10f},
        {IslandShape::CONTINENTAL, 0.10f}
    };

    initializeRng(m_baseSeed);
}

void ArchipelagoGenerator::initializeRng(uint32_t seed) {
    m_baseSeed = seed;
    m_rng.seed(seed);
}

void ArchipelagoGenerator::generate(int islandCount, float spacing) {
    generate(islandCount, spacing, ArchipelagoPattern::RANDOM);
}

void ArchipelagoGenerator::generate(int islandCount, float spacing, ArchipelagoPattern pattern) {
    // Clamp island count
    islandCount = std::max(1, std::min(islandCount, MAX_ISLANDS));
    spacing = std::max(MIN_ISLAND_SPACING, spacing);

    // Clear previous data
    m_data.islands.clear();
    m_data.currents.clear();

    // Generate based on pattern
    switch (pattern) {
        case ArchipelagoPattern::LINEAR:
            generateLinear(islandCount, spacing);
            break;
        case ArchipelagoPattern::CIRCULAR:
            generateCircular(islandCount, spacing);
            break;
        case ArchipelagoPattern::CLUSTERED:
            generateClustered(islandCount, spacing);
            break;
        case ArchipelagoPattern::VOLCANIC_ARC:
            generateVolcanicArc(islandCount, spacing);
            break;
        case ArchipelagoPattern::SCATTERED:
            generateScattered(islandCount, spacing);
            break;
        case ArchipelagoPattern::RANDOM:
        default:
            generateRandom(islandCount, spacing);
            break;
    }

    // Post-generation processing
    assignIslandProperties();
    calculateBounds();
    generateOceanCurrents();
    generateWindPatterns();
}

void ArchipelagoGenerator::generateWithSeed(int islandCount, float spacing, uint32_t seed) {
    initializeRng(seed);
    generate(islandCount, spacing);
}

void ArchipelagoGenerator::generateRandom(int count, float spacing) {
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);
    std::uniform_real_distribution<float> radiusDist(0.3f, 1.0f);
    std::uniform_real_distribution<float> sizeDist(0.6f, 1.4f);

    float maxRadius = spacing * std::sqrt(static_cast<float>(count)) * 0.8f;

    for (int i = 0; i < count; ++i) {
        IslandConfig config;

        // Try to find valid placement
        if (i == 0) {
            // First island at center
            config.position = glm::vec2(0.0f);
        } else {
            config.position = findValidPosition(spacing * 0.7f, glm::vec2(0.0f), 200);
        }

        config.size = sizeDist(m_rng);
        config.seed = m_baseSeed + static_cast<uint32_t>(i * 1000);
        config.shape = selectRandomShape();
        config.name = generateIslandName(i, config.seed);

        m_data.islands.push_back(config);
    }
}

void ArchipelagoGenerator::generateLinear(int count, float spacing) {
    std::uniform_real_distribution<float> offsetDist(-0.2f, 0.2f);
    std::uniform_real_distribution<float> sizeDist(0.7f, 1.3f);

    // Random direction for the chain
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);
    float chainAngle = angleDist(m_rng);
    glm::vec2 direction(std::cos(chainAngle), std::sin(chainAngle));
    glm::vec2 perpendicular(-direction.y, direction.x);

    float totalLength = spacing * (count - 1);
    glm::vec2 startPos = -direction * (totalLength * 0.5f);

    for (int i = 0; i < count; ++i) {
        IslandConfig config;

        // Position along chain with slight perpendicular offset
        glm::vec2 basePos = startPos + direction * (spacing * static_cast<float>(i));
        float perpOffset = offsetDist(m_rng) * spacing;
        config.position = basePos + perpendicular * perpOffset;

        // Islands get smaller toward ends (volcanic chain pattern)
        float positionFactor = 1.0f - std::abs((i - (count - 1) / 2.0f) / (count * 0.5f)) * 0.3f;
        config.size = sizeDist(m_rng) * positionFactor;

        config.seed = m_baseSeed + static_cast<uint32_t>(i * 1000);
        config.shape = (i < count / 2) ? IslandShape::VOLCANIC : selectRandomShape();
        config.hasVolcano = (i < count / 3);
        config.name = generateIslandName(i, config.seed);

        m_data.islands.push_back(config);
    }
}

void ArchipelagoGenerator::generateCircular(int count, float spacing) {
    std::uniform_real_distribution<float> radiusVariation(0.85f, 1.15f);
    std::uniform_real_distribution<float> sizeDist(0.6f, 1.2f);

    // Calculate ring radius to achieve desired spacing
    float ringRadius = (spacing * count) / (2.0f * 3.14159f);

    for (int i = 0; i < count; ++i) {
        IslandConfig config;

        // Position around ring
        float angle = (2.0f * 3.14159f * i) / count;
        float r = ringRadius * radiusVariation(m_rng);

        config.position = glm::vec2(
            std::cos(angle) * r,
            std::sin(angle) * r
        );

        config.size = sizeDist(m_rng);
        config.seed = m_baseSeed + static_cast<uint32_t>(i * 1000);
        config.shape = (i % 3 == 0) ? IslandShape::ATOLL : selectRandomShape();
        config.name = generateIslandName(i, config.seed);

        m_data.islands.push_back(config);
    }
}

void ArchipelagoGenerator::generateClustered(int count, float spacing) {
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);
    std::uniform_real_distribution<float> sizeDist(0.5f, 1.5f);

    // Determine number of clusters (2-4)
    int numClusters = std::max(2, std::min(4, count / 3 + 1));
    int islandsPerCluster = count / numClusters;
    int extraIslands = count % numClusters;

    // Generate cluster centers
    std::vector<glm::vec2> clusterCenters;
    float clusterSpacing = spacing * 2.5f;

    for (int c = 0; c < numClusters; ++c) {
        glm::vec2 center;

        if (c == 0) {
            center = glm::vec2(0.0f);
        } else {
            // Place cluster centers with good separation
            float angle = (2.0f * 3.14159f * c) / numClusters + angleDist(m_rng) * 0.3f;
            float dist = clusterSpacing * (0.8f + angleDist(m_rng) * 0.2f / 6.28318f);
            center = glm::vec2(std::cos(angle) * dist, std::sin(angle) * dist);
        }

        clusterCenters.push_back(center);
    }

    // Generate islands within each cluster
    int islandIndex = 0;
    for (int c = 0; c < numClusters; ++c) {
        int clusterSize = islandsPerCluster + (c < extraIslands ? 1 : 0);
        float intraClusterSpacing = spacing * 0.5f;

        for (int i = 0; i < clusterSize; ++i) {
            IslandConfig config;

            if (i == 0) {
                // Main island of cluster
                config.position = clusterCenters[c];
                config.size = sizeDist(m_rng) * 1.3f;  // Larger main island
            } else {
                // Satellite islands
                float angle = angleDist(m_rng);
                float dist = intraClusterSpacing * (0.5f + i * 0.3f);
                config.position = clusterCenters[c] + glm::vec2(
                    std::cos(angle) * dist,
                    std::sin(angle) * dist
                );
                config.size = sizeDist(m_rng) * 0.8f;
            }

            config.seed = m_baseSeed + static_cast<uint32_t>(islandIndex * 1000);
            config.shape = selectRandomShape();
            config.name = generateIslandName(islandIndex, config.seed);

            m_data.islands.push_back(config);
            islandIndex++;
        }
    }
}

void ArchipelagoGenerator::generateVolcanicArc(int count, float spacing) {
    std::uniform_real_distribution<float> offsetDist(-0.15f, 0.15f);
    std::uniform_real_distribution<float> sizeDist(0.7f, 1.4f);

    // Arc parameters
    float arcAngle = 2.0f;  // radians (about 115 degrees)
    float arcRadius = (spacing * count) / arcAngle;

    // Random starting angle
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);
    float startAngle = angleDist(m_rng);

    for (int i = 0; i < count; ++i) {
        IslandConfig config;

        // Position along arc
        float t = static_cast<float>(i) / (count - 1);
        float angle = startAngle + t * arcAngle;
        float r = arcRadius * (1.0f + offsetDist(m_rng));

        config.position = glm::vec2(
            std::cos(angle) * r,
            std::sin(angle) * r
        );

        // Volcanic characteristics increase toward middle of arc
        float volcanism = 1.0f - 2.0f * std::abs(t - 0.5f);

        config.size = sizeDist(m_rng) * (0.8f + volcanism * 0.4f);
        config.seed = m_baseSeed + static_cast<uint32_t>(i * 1000);
        config.shape = (volcanism > 0.5f) ? IslandShape::VOLCANIC : IslandShape::IRREGULAR;
        config.hasVolcano = (volcanism > 0.3f);
        config.elevation = volcanism * 0.3f;  // Higher elevation near arc center
        config.name = generateIslandName(i, config.seed);

        m_data.islands.push_back(config);
    }
}

void ArchipelagoGenerator::generateScattered(int count, float spacing) {
    std::uniform_real_distribution<float> sizeDist(0.5f, 1.5f);

    // Much larger area for scattered islands
    float areaRadius = spacing * std::sqrt(static_cast<float>(count)) * 1.5f;

    for (int i = 0; i < count; ++i) {
        IslandConfig config;

        // Find position with maximum spacing
        config.position = findValidPosition(spacing * 1.2f, glm::vec2(0.0f), 300);

        config.size = sizeDist(m_rng);
        config.seed = m_baseSeed + static_cast<uint32_t>(i * 1000);
        config.shape = selectRandomShape();
        config.isolationFactor = 0.8f;  // High isolation for scattered islands
        config.name = generateIslandName(i, config.seed);

        m_data.islands.push_back(config);
    }
}

void ArchipelagoGenerator::assignIslandProperties() {
    std::uniform_real_distribution<float> biomeDist(0.0f, 1.0f);

    for (size_t i = 0; i < m_data.islands.size(); ++i) {
        auto& island = m_data.islands[i];

        // Set up full generation parameters
        island.genParams.seed = island.seed;
        island.genParams.shape = island.shape;

        // Size affects various parameters
        island.genParams.islandRadius = 0.35f + island.size * 0.1f;
        island.genParams.coastalIrregularity = 0.2f + biomeDist(m_rng) * 0.3f;
        island.genParams.mountainousness = 0.3f + island.elevation + biomeDist(m_rng) * 0.4f;
        island.genParams.riverDensity = biomeDist(m_rng) * 0.5f;
        island.genParams.lakeDensity = biomeDist(m_rng) * 0.3f;

        // Volcanic islands have specific settings
        if (island.hasVolcano || island.shape == IslandShape::VOLCANIC) {
            island.genParams.volcanoHeight = 1.2f + biomeDist(m_rng) * 0.6f;
            island.genParams.craterSize = 0.1f + biomeDist(m_rng) * 0.1f;
            island.genParams.hasLavaFlows = biomeDist(m_rng) > 0.4f;
        }

        // Atoll-specific
        if (island.shape == IslandShape::ATOLL) {
            island.genParams.lagoonDepth = 0.25f + biomeDist(m_rng) * 0.2f;
            island.genParams.reefWidth = 0.08f + biomeDist(m_rng) * 0.07f;
        }

        // Assign biome based on latitude-like position
        float latitudeProxy = island.position.y / 500.0f;  // Normalized position
        if (std::abs(latitudeProxy) < 0.2f) {
            island.biomeIndex = 0;  // Tropical
        } else if (std::abs(latitudeProxy) < 0.5f) {
            island.biomeIndex = 1;  // Subtropical
        } else {
            island.biomeIndex = 2;  // Temperate
        }

        // Calculate isolation factor based on neighbors
        float minNeighborDist = std::numeric_limits<float>::max();
        for (size_t j = 0; j < m_data.islands.size(); ++j) {
            if (i == j) continue;
            float dist = glm::length(island.position - m_data.islands[j].position);
            minNeighborDist = std::min(minNeighborDist, dist);
        }
        island.isolationFactor = std::min(1.0f, minNeighborDist / 300.0f);
    }
}

void ArchipelagoGenerator::calculateBounds() {
    if (m_data.islands.empty()) {
        m_data.minBounds = glm::vec2(0.0f);
        m_data.maxBounds = glm::vec2(0.0f);
        m_data.center = glm::vec2(0.0f);
        m_data.totalArea = 0.0f;
        return;
    }

    m_data.minBounds = glm::vec2(std::numeric_limits<float>::max());
    m_data.maxBounds = glm::vec2(std::numeric_limits<float>::lowest());
    m_data.totalArea = 0.0f;

    for (const auto& island : m_data.islands) {
        // Account for island size when calculating bounds
        float islandRadius = island.size * 100.0f;  // Approximate world units

        m_data.minBounds.x = std::min(m_data.minBounds.x, island.position.x - islandRadius);
        m_data.minBounds.y = std::min(m_data.minBounds.y, island.position.y - islandRadius);
        m_data.maxBounds.x = std::max(m_data.maxBounds.x, island.position.x + islandRadius);
        m_data.maxBounds.y = std::max(m_data.maxBounds.y, island.position.y + islandRadius);

        // Approximate island area
        m_data.totalArea += 3.14159f * islandRadius * islandRadius;
    }

    m_data.center = (m_data.minBounds + m_data.maxBounds) * 0.5f;
}

void ArchipelagoGenerator::generateOceanCurrents() {
    m_data.currents.clear();

    if (m_data.islands.size() < 2) return;

    std::uniform_real_distribution<float> strengthDist(0.3f, 0.9f);

    // Generate currents between nearby islands
    for (size_t i = 0; i < m_data.islands.size(); ++i) {
        // Find nearest neighbors
        auto neighbors = m_data.getNeighborIslands(static_cast<uint32_t>(i), 400.0f);

        for (uint32_t neighborIdx : neighbors) {
            // Avoid duplicate currents
            bool alreadyExists = false;
            for (const auto& existing : m_data.currents) {
                if ((existing.fromIsland == i && existing.toIsland == neighborIdx) ||
                    (existing.fromIsland == neighborIdx && existing.toIsland == i)) {
                    alreadyExists = true;
                    break;
                }
            }

            if (!alreadyExists) {
                OceanCurrent current;
                current.fromIsland = static_cast<uint32_t>(i);
                current.toIsland = neighborIdx;

                glm::vec2 diff = m_data.islands[neighborIdx].position - m_data.islands[i].position;
                current.distance = glm::length(diff);
                current.direction = diff / current.distance;

                // Strength decreases with distance
                float distanceFactor = 1.0f - std::min(1.0f, current.distance / 400.0f);
                current.strength = strengthDist(m_rng) * distanceFactor;

                // Shorter distances are more likely to be bidirectional
                current.bidirectional = (current.distance < 200.0f) || (strengthDist(m_rng) > 0.5f);

                m_data.currents.push_back(current);
            }
        }
    }
}

void ArchipelagoGenerator::generateWindPatterns() {
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);
    std::uniform_real_distribution<float> strengthDist(0.3f, 0.7f);

    // Prevailing wind direction (trade winds typically east to west)
    float windAngle = angleDist(m_rng);
    m_data.wind.prevailingDirection = glm::vec2(std::cos(windAngle), std::sin(windAngle));
    m_data.wind.strength = strengthDist(m_rng);
    m_data.wind.seasonalVariation = 0.2f + strengthDist(m_rng) * 0.3f;
}

bool ArchipelagoGenerator::isValidPlacement(const glm::vec2& position, float minDist) const {
    for (const auto& island : m_data.islands) {
        if (glm::length(position - island.position) < minDist) {
            return false;
        }
    }
    return true;
}

glm::vec2 ArchipelagoGenerator::findValidPosition(float minDist, const glm::vec2& hint, int maxAttempts) {
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);
    std::uniform_real_distribution<float> radiusDist(0.0f, 1.0f);

    float searchRadius = minDist * 2.0f;

    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        // Expand search radius over attempts
        float currentRadius = searchRadius * (1.0f + attempt * 0.1f);

        float angle = angleDist(m_rng);
        float r = std::sqrt(radiusDist(m_rng)) * currentRadius;  // sqrt for uniform distribution

        glm::vec2 pos = hint + glm::vec2(std::cos(angle) * r, std::sin(angle) * r);

        if (isValidPlacement(pos, minDist)) {
            return pos;
        }
    }

    // Fallback: return position with some offset from hint
    float angle = angleDist(m_rng);
    return hint + glm::vec2(std::cos(angle), std::sin(angle)) * searchRadius * 3.0f;
}

IslandShape ArchipelagoGenerator::selectRandomShape() {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float roll = dist(m_rng);

    float cumulative = 0.0f;
    for (const auto& [shape, weight] : m_shapeWeights) {
        cumulative += weight;
        if (roll <= cumulative) {
            return shape;
        }
    }

    return IslandShape::IRREGULAR;  // Fallback
}

const IslandConfig* ArchipelagoGenerator::getIsland(uint32_t index) const {
    if (index >= m_data.islands.size()) return nullptr;
    return &m_data.islands[index];
}

float ArchipelagoGenerator::getMigrationDifficulty(uint32_t fromIsland, uint32_t toIsland) const {
    if (fromIsland >= m_data.islands.size() || toIsland >= m_data.islands.size()) {
        return 1.0f;  // Maximum difficulty
    }

    float distance = glm::length(
        m_data.islands[toIsland].position - m_data.islands[fromIsland].position
    );

    // Base difficulty from distance
    float distanceDifficulty = std::min(1.0f, distance / 500.0f);

    // Check for favorable current
    const OceanCurrent* current = m_data.getCurrentBetween(fromIsland, toIsland);
    float currentBonus = 0.0f;

    if (current) {
        if (current->fromIsland == fromIsland) {
            // Going with the current
            currentBonus = current->strength * 0.3f;
        } else if (current->bidirectional) {
            // Going against but still possible
            currentBonus = current->strength * 0.1f;
        }
    }

    // Island isolation factors
    float isolation = (m_data.islands[fromIsland].isolationFactor +
                       m_data.islands[toIsland].isolationFactor) * 0.5f;

    return std::max(0.1f, distanceDifficulty + isolation * 0.2f - currentBonus);
}

void ArchipelagoGenerator::setShapeDistribution(
    const std::vector<std::pair<IslandShape, float>>& distribution) {
    m_shapeWeights = distribution;

    // Normalize weights
    float total = 0.0f;
    for (const auto& [shape, weight] : m_shapeWeights) {
        total += weight;
    }

    if (total > 0.0f) {
        for (auto& [shape, weight] : m_shapeWeights) {
            weight /= total;
        }
    }
}

std::string ArchipelagoGenerator::generateIslandName(uint32_t index, uint32_t seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<size_t> prefixDist(0, NAME_PREFIXES.size() - 1);
    std::uniform_int_distribution<size_t> rootDist(0, NAME_ROOTS.size() - 1);
    std::uniform_int_distribution<size_t> suffixDist(0, NAME_SUFFIXES.size() - 1);

    std::string name = NAME_PREFIXES[prefixDist(rng)] +
                       NAME_ROOTS[rootDist(rng)] +
                       NAME_SUFFIXES[suffixDist(rng)];

    // Capitalize first letter
    if (!name.empty()) {
        name[0] = static_cast<char>(std::toupper(name[0]));
    }

    return name;
}

glm::vec2 ArchipelagoGenerator::worldToArchipelagoCoord(const glm::vec3& worldPos) const {
    return glm::vec2(worldPos.x, worldPos.z);
}

int ArchipelagoGenerator::findNearestIsland(const glm::vec3& worldPos) const {
    glm::vec2 pos2D = worldToArchipelagoCoord(worldPos);

    int nearestIdx = -1;
    float minDist = std::numeric_limits<float>::max();

    for (size_t i = 0; i < m_data.islands.size(); ++i) {
        float dist = glm::length(m_data.islands[i].position - pos2D);
        if (dist < minDist) {
            minDist = dist;
            nearestIdx = static_cast<int>(i);
        }
    }

    return nearestIdx;
}
