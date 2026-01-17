#pragma once

// Shared procedural terrain sampler to keep height queries consistent across systems.

#include <vector>

namespace TerrainSampler {
    inline float WORLD_SIZE = 2048.0f;
    inline float HEIGHT_SCALE = 30.0f;
    inline float WATER_LEVEL = 0.35f; // Normalized
    inline float BEACH_LEVEL = 0.42f; // Normalized

    // Override terrain sampling with a heightmap.
    void SetHeightmap(const std::vector<float>* heightmap, int width, int height);
    void ClearHeightmap();

    // Update world scale parameters (used by heightmap sampling and water tests).
    void SetWorldParams(float worldSize, float heightScale, float waterLevel, float beachLevel);

    // Normalized height in [0, 1].
    float SampleHeightNormalized(float worldX, float worldZ);

    // World-space height in terrain units.
    float SampleHeight(float worldX, float worldZ);

    inline float GetWaterHeight() { return WATER_LEVEL * HEIGHT_SCALE; }

    inline bool IsWater(float worldX, float worldZ) {
        return SampleHeightNormalized(worldX, worldZ) < WATER_LEVEL;
    }
} // namespace TerrainSampler
