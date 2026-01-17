#pragma once

#include <vector>
#include <random>
#include <glm/glm.hpp>

// Erosion parameters for fine-tuning
struct HydraulicErosionParams {
    int numIterations = 50000;       // Number of water droplets to simulate
    int maxDropletLifetime = 64;     // Max steps per droplet

    float inertia = 0.05f;           // How much droplet direction persists
    float sedimentCapacityFactor = 4.0f;  // Multiplier for carrying capacity
    float minSedimentCapacity = 0.01f;
    float erodeSpeed = 0.3f;         // Erosion rate
    float depositSpeed = 0.3f;       // Deposition rate
    float evaporateSpeed = 0.01f;    // Water evaporation rate
    float gravity = 4.0f;            // Affects erosion based on slope

    int erosionRadius = 3;           // Brush radius for erosion/deposition
    float initialWaterVolume = 1.0f;
    float initialSpeed = 1.0f;
};

struct ThermalErosionParams {
    int numIterations = 5;           // Passes over entire terrain
    float talusAngle = 0.5f;         // Angle of repose (radians, ~30 degrees)
    float erosionRate = 0.5f;        // How much material moves per iteration
};

// Heightmap wrapper for erosion operations
class Heightmap {
public:
    Heightmap(int width, int depth);
    Heightmap(const std::vector<float>& data, int width, int depth);

    // Access
    float get(int x, int z) const;
    void set(int x, int z, float value);
    float getBilinear(float x, float z) const;

    // Gradient calculation
    glm::vec2 getGradient(float x, float z) const;

    // Dimensions
    int getWidth() const { return width; }
    int getDepth() const { return depth; }

    // Raw data access
    std::vector<float>& getData() { return data; }
    const std::vector<float>& getData() const { return data; }

    // Statistics
    float getMinHeight() const;
    float getMaxHeight() const;
    void normalize(float minH = 0.0f, float maxH = 1.0f);

private:
    std::vector<float> data;
    int width;
    int depth;
};

class TerrainErosion {
public:
    TerrainErosion();
    ~TerrainErosion() = default;

    // Set random seed for reproducibility
    void setSeed(unsigned int seed);

    // Hydraulic erosion (water droplet simulation)
    // Creates realistic river valleys and terrain features
    void simulateHydraulicErosion(Heightmap& heightmap, const HydraulicErosionParams& params);

    // Simplified hydraulic erosion with default params
    void simulateHydraulicErosion(Heightmap& heightmap, int iterations = 50000);

    // Thermal erosion (talus slope collapse)
    // Creates scree slopes and softens terrain
    void simulateThermalErosion(Heightmap& heightmap, const ThermalErosionParams& params);

    // Simplified thermal erosion with default params
    void simulateThermalErosion(Heightmap& heightmap, float talusAngle = 0.5f);

    // Combined erosion (recommended for realistic results)
    void simulateFullErosion(Heightmap& heightmap,
                              int hydraulicIterations = 50000,
                              int thermalPasses = 3);

    // Progress callback for long operations
    using ProgressCallback = std::function<void(float progress, const std::string& stage)>;
    void setProgressCallback(ProgressCallback callback) { progressCallback = callback; }

private:
    std::mt19937 rng;
    ProgressCallback progressCallback;

    // Precomputed erosion/deposition brush weights
    std::vector<std::vector<float>> erosionBrushWeights;
    std::vector<std::vector<glm::ivec2>> erosionBrushIndices;

    // Initialize brush for given radius
    void initializeErosionBrush(int radius, int mapWidth, int mapDepth);

    // Single droplet simulation step
    void simulateDroplet(Heightmap& heightmap, const HydraulicErosionParams& params);

    // Erode/deposit at position using brush
    void erodeAt(Heightmap& heightmap, float x, float z, float amount, int radius);
    void depositAt(Heightmap& heightmap, float x, float z, float amount);

    // Thermal erosion helpers
    void thermalErosionPass(Heightmap& heightmap, float talusAngle, float erosionRate);

    // Report progress
    void reportProgress(float progress, const std::string& stage);
};
