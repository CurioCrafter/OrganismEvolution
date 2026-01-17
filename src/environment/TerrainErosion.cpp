#include "TerrainErosion.h"
#include <cmath>
#include <algorithm>
#include <numeric>

// ============================================================================
// Heightmap Implementation
// ============================================================================

Heightmap::Heightmap(int w, int d)
    : width(w), depth(d), data(w * d, 0.0f) {}

Heightmap::Heightmap(const std::vector<float>& d, int w, int dp)
    : width(w), depth(dp), data(d) {
    if (data.size() != static_cast<size_t>(w * dp)) {
        data.resize(w * dp, 0.0f);
    }
}

float Heightmap::get(int x, int z) const {
    if (x < 0 || x >= width || z < 0 || z >= depth) {
        return 0.0f;
    }
    return data[z * width + x];
}

void Heightmap::set(int x, int z, float value) {
    if (x >= 0 && x < width && z >= 0 && z < depth) {
        data[z * width + x] = value;
    }
}

float Heightmap::getBilinear(float x, float z) const {
    int x0 = static_cast<int>(std::floor(x));
    int z0 = static_cast<int>(std::floor(z));
    int x1 = x0 + 1;
    int z1 = z0 + 1;

    float fx = x - x0;
    float fz = z - z0;

    float h00 = get(x0, z0);
    float h10 = get(x1, z0);
    float h01 = get(x0, z1);
    float h11 = get(x1, z1);

    float h0 = h00 * (1.0f - fx) + h10 * fx;
    float h1 = h01 * (1.0f - fx) + h11 * fx;

    return h0 * (1.0f - fz) + h1 * fz;
}

glm::vec2 Heightmap::getGradient(float x, float z) const {
    int x0 = static_cast<int>(std::floor(x));
    int z0 = static_cast<int>(std::floor(z));

    float fx = x - x0;
    float fz = z - z0;

    // Sample 4 corners
    float h00 = get(x0, z0);
    float h10 = get(x0 + 1, z0);
    float h01 = get(x0, z0 + 1);
    float h11 = get(x0 + 1, z0 + 1);

    // Interpolated gradient
    float gx = (h10 - h00) * (1.0f - fz) + (h11 - h01) * fz;
    float gz = (h01 - h00) * (1.0f - fx) + (h11 - h10) * fx;

    return {gx, gz};
}

float Heightmap::getMinHeight() const {
    if (data.empty()) return 0.0f;
    return *std::min_element(data.begin(), data.end());
}

float Heightmap::getMaxHeight() const {
    if (data.empty()) return 0.0f;
    return *std::max_element(data.begin(), data.end());
}

void Heightmap::normalize(float minH, float maxH) {
    float currentMin = getMinHeight();
    float currentMax = getMaxHeight();
    float range = currentMax - currentMin;

    if (range < 0.0001f) return;

    float targetRange = maxH - minH;
    for (float& h : data) {
        h = minH + ((h - currentMin) / range) * targetRange;
    }
}

// ============================================================================
// TerrainErosion Implementation
// ============================================================================

TerrainErosion::TerrainErosion()
    : rng(std::random_device{}()) {}

void TerrainErosion::setSeed(unsigned int seed) {
    rng.seed(seed);
}

void TerrainErosion::initializeErosionBrush(int radius, int mapWidth, int mapDepth) {
    erosionBrushWeights.clear();
    erosionBrushIndices.clear();

    int brushSize = (2 * radius + 1) * (2 * radius + 1);
    erosionBrushWeights.resize(mapWidth * mapDepth);
    erosionBrushIndices.resize(mapWidth * mapDepth);

    // Precompute weights and indices for each cell
    for (int z = 0; z < mapDepth; z++) {
        for (int x = 0; x < mapWidth; x++) {
            int cellIndex = z * mapWidth + x;

            std::vector<float>& weights = erosionBrushWeights[cellIndex];
            std::vector<glm::ivec2>& indices = erosionBrushIndices[cellIndex];

            float weightSum = 0.0f;

            for (int dz = -radius; dz <= radius; dz++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    int nx = x + dx;
                    int nz = z + dz;

                    // Check bounds
                    if (nx >= 0 && nx < mapWidth && nz >= 0 && nz < mapDepth) {
                        float dist = std::sqrt(static_cast<float>(dx * dx + dz * dz));
                        if (dist <= radius) {
                            float weight = 1.0f - dist / radius;
                            weight = weight * weight; // Quadratic falloff
                            weights.push_back(weight);
                            indices.push_back({nx, nz});
                            weightSum += weight;
                        }
                    }
                }
            }

            // Normalize weights
            if (weightSum > 0.0f) {
                for (float& w : weights) {
                    w /= weightSum;
                }
            }
        }
    }
}

void TerrainErosion::simulateHydraulicErosion(Heightmap& heightmap, int iterations) {
    HydraulicErosionParams params;
    params.numIterations = iterations;
    simulateHydraulicErosion(heightmap, params);
}

void TerrainErosion::simulateHydraulicErosion(Heightmap& heightmap, const HydraulicErosionParams& params) {
    int mapWidth = heightmap.getWidth();
    int mapDepth = heightmap.getDepth();

    // Initialize erosion brush
    initializeErosionBrush(params.erosionRadius, mapWidth, mapDepth);

    reportProgress(0.0f, "Hydraulic erosion");

    std::uniform_real_distribution<float> distX(0.0f, static_cast<float>(mapWidth - 2));
    std::uniform_real_distribution<float> distZ(0.0f, static_cast<float>(mapDepth - 2));

    for (int i = 0; i < params.numIterations; i++) {
        // Random starting position
        float posX = distX(rng);
        float posZ = distZ(rng);

        float dirX = 0.0f;
        float dirZ = 0.0f;
        float speed = params.initialSpeed;
        float water = params.initialWaterVolume;
        float sediment = 0.0f;

        for (int lifetime = 0; lifetime < params.maxDropletLifetime; lifetime++) {
            int nodeX = static_cast<int>(posX);
            int nodeZ = static_cast<int>(posZ);
            int dropletIndex = nodeZ * mapWidth + nodeX;

            // Get height and gradient at current position
            float height = heightmap.getBilinear(posX, posZ);
            glm::vec2 gradient = heightmap.getGradient(posX, posZ);

            // Calculate new direction (blend old direction with gradient)
            dirX = dirX * params.inertia - gradient.x * (1.0f - params.inertia);
            dirZ = dirZ * params.inertia - gradient.y * (1.0f - params.inertia);

            // Normalize direction
            float len = std::sqrt(dirX * dirX + dirZ * dirZ);
            if (len > 0.0001f) {
                dirX /= len;
                dirZ /= len;
            } else {
                // Random direction if on flat terrain
                float angle = std::uniform_real_distribution<float>(0.0f, 6.28318f)(rng);
                dirX = std::cos(angle);
                dirZ = std::sin(angle);
            }

            // Move droplet
            posX += dirX;
            posZ += dirZ;

            // Check bounds
            if (posX < 0 || posX >= mapWidth - 1 || posZ < 0 || posZ >= mapDepth - 1) {
                break;
            }

            // Get new height
            float newHeight = heightmap.getBilinear(posX, posZ);
            float heightDiff = newHeight - height;

            // Calculate sediment capacity
            float sedimentCapacity = std::max(-heightDiff * speed * water * params.sedimentCapacityFactor,
                                               params.minSedimentCapacity);

            // Erode or deposit
            if (sediment > sedimentCapacity || heightDiff > 0) {
                // Deposit sediment
                float depositAmount = (heightDiff > 0) ?
                    std::min(heightDiff, sediment) :
                    (sediment - sedimentCapacity) * params.depositSpeed;

                sediment -= depositAmount;

                // Deposit at old position using brush
                if (nodeX >= 0 && nodeX < mapWidth && nodeZ >= 0 && nodeZ < mapDepth) {
                    depositAt(heightmap, static_cast<float>(nodeX), static_cast<float>(nodeZ), depositAmount);
                }
            } else {
                // Erode terrain
                float erodeAmount = std::min((sedimentCapacity - sediment) * params.erodeSpeed, -heightDiff);

                // Erode using brush
                if (dropletIndex >= 0 && static_cast<size_t>(dropletIndex) < erosionBrushWeights.size()) {
                    const auto& weights = erosionBrushWeights[dropletIndex];
                    const auto& indices = erosionBrushIndices[dropletIndex];

                    for (size_t j = 0; j < weights.size(); j++) {
                        float weightedErode = erodeAmount * weights[j];
                        int ex = indices[j].x;
                        int ez = indices[j].y;
                        float currentHeight = heightmap.get(ex, ez);

                        // Don't erode below 0
                        float actualErode = std::min(weightedErode, currentHeight);
                        heightmap.set(ex, ez, currentHeight - actualErode);
                        sediment += actualErode;
                    }
                }
            }

            // Update speed (accelerate downhill, decelerate uphill)
            speed = std::sqrt(std::max(0.0f, speed * speed + heightDiff * params.gravity));

            // Evaporate water
            water *= (1.0f - params.evaporateSpeed);

            // Stop if water depleted
            if (water < 0.01f) {
                break;
            }
        }

        // Progress update
        if (i % 5000 == 0) {
            reportProgress(static_cast<float>(i) / params.numIterations, "Hydraulic erosion");
        }
    }

    reportProgress(1.0f, "Hydraulic erosion complete");
}

void TerrainErosion::simulateThermalErosion(Heightmap& heightmap, float talusAngle) {
    ThermalErosionParams params;
    params.talusAngle = talusAngle;
    simulateThermalErosion(heightmap, params);
}

void TerrainErosion::simulateThermalErosion(Heightmap& heightmap, const ThermalErosionParams& params) {
    reportProgress(0.0f, "Thermal erosion");

    for (int iter = 0; iter < params.numIterations; iter++) {
        thermalErosionPass(heightmap, params.talusAngle, params.erosionRate);
        reportProgress(static_cast<float>(iter + 1) / params.numIterations, "Thermal erosion");
    }

    reportProgress(1.0f, "Thermal erosion complete");
}

void TerrainErosion::thermalErosionPass(Heightmap& heightmap, float talusAngle, float erosionRate) {
    int mapWidth = heightmap.getWidth();
    int mapDepth = heightmap.getDepth();

    float talusTangent = std::tan(talusAngle);

    // Create copy to avoid read-write conflicts
    std::vector<float> heightChanges(mapWidth * mapDepth, 0.0f);

    // 8-connected neighbors
    const int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int dz[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    const float diag = 1.414f;
    const float dist[] = {diag, 1.0f, diag, 1.0f, 1.0f, diag, 1.0f, diag};

    for (int z = 1; z < mapDepth - 1; z++) {
        for (int x = 1; x < mapWidth - 1; x++) {
            float h = heightmap.get(x, z);
            float maxHeightDiff = 0.0f;
            float totalDiff = 0.0f;
            int steepestNeighbor = -1;

            // Find neighbors below talus angle threshold
            for (int i = 0; i < 8; i++) {
                int nx = x + dx[i];
                int nz = z + dz[i];
                float nh = heightmap.get(nx, nz);

                float heightDiff = h - nh;
                float slopeTangent = heightDiff / dist[i];

                if (slopeTangent > talusTangent) {
                    totalDiff += heightDiff - talusTangent * dist[i];
                    if (heightDiff > maxHeightDiff) {
                        maxHeightDiff = heightDiff;
                        steepestNeighbor = i;
                    }
                }
            }

            // Move material to neighbors if slope exceeds talus angle
            if (totalDiff > 0.0f && steepestNeighbor >= 0) {
                float moveAmount = (maxHeightDiff - talusTangent * dist[steepestNeighbor]) * erosionRate * 0.5f;

                // Remove from current cell
                heightChanges[z * mapWidth + x] -= moveAmount;

                // Add to steepest neighbor
                int nx = x + dx[steepestNeighbor];
                int nz = z + dz[steepestNeighbor];
                heightChanges[nz * mapWidth + nx] += moveAmount;
            }
        }
    }

    // Apply changes
    for (int z = 0; z < mapDepth; z++) {
        for (int x = 0; x < mapWidth; x++) {
            float h = heightmap.get(x, z);
            heightmap.set(x, z, h + heightChanges[z * mapWidth + x]);
        }
    }
}

void TerrainErosion::simulateFullErosion(Heightmap& heightmap, int hydraulicIterations, int thermalPasses) {
    // First do hydraulic erosion (creates valleys and rivers)
    HydraulicErosionParams hydraulicParams;
    hydraulicParams.numIterations = hydraulicIterations;
    simulateHydraulicErosion(heightmap, hydraulicParams);

    // Then thermal erosion to soften and create talus slopes
    ThermalErosionParams thermalParams;
    thermalParams.numIterations = thermalPasses;
    thermalParams.talusAngle = 0.55f; // ~31 degrees
    simulateThermalErosion(heightmap, thermalParams);
}

void TerrainErosion::depositAt(Heightmap& heightmap, float x, float z, float amount) {
    // Simple single-point deposition
    int ix = static_cast<int>(x);
    int iz = static_cast<int>(z);

    if (ix >= 0 && ix < heightmap.getWidth() && iz >= 0 && iz < heightmap.getDepth()) {
        float h = heightmap.get(ix, iz);
        heightmap.set(ix, iz, h + amount);
    }
}

void TerrainErosion::erodeAt(Heightmap& heightmap, float x, float z, float amount, int radius) {
    int cx = static_cast<int>(x);
    int cz = static_cast<int>(z);

    float totalWeight = 0.0f;

    // Calculate total weight first
    for (int dz = -radius; dz <= radius; dz++) {
        for (int dx = -radius; dx <= radius; dx++) {
            float dist = std::sqrt(static_cast<float>(dx * dx + dz * dz));
            if (dist <= radius) {
                float weight = 1.0f - dist / radius;
                totalWeight += weight * weight;
            }
        }
    }

    if (totalWeight < 0.0001f) return;

    // Apply erosion
    for (int dz = -radius; dz <= radius; dz++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int nx = cx + dx;
            int nz = cz + dz;

            if (nx >= 0 && nx < heightmap.getWidth() && nz >= 0 && nz < heightmap.getDepth()) {
                float dist = std::sqrt(static_cast<float>(dx * dx + dz * dz));
                if (dist <= radius) {
                    float weight = 1.0f - dist / radius;
                    weight = weight * weight / totalWeight;

                    float h = heightmap.get(nx, nz);
                    float erode = std::min(amount * weight, h);
                    heightmap.set(nx, nz, h - erode);
                }
            }
        }
    }
}

void TerrainErosion::reportProgress(float progress, const std::string& stage) {
    if (progressCallback) {
        progressCallback(progress, stage);
    }
}
