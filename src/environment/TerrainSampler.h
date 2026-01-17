#pragma once

// Shared procedural terrain sampler to keep height queries consistent across systems.

namespace TerrainSampler {
    constexpr float WORLD_SIZE = 2048.0f;
    constexpr float HEIGHT_SCALE = 30.0f;
    constexpr float WATER_LEVEL = 0.35f; // Normalized
    constexpr float BEACH_LEVEL = 0.42f; // Normalized

    // Normalized height in [0, 1].
    float SampleHeightNormalized(float worldX, float worldZ);

    // World-space height in terrain units.
    float SampleHeight(float worldX, float worldZ);

    inline float GetWaterHeight() {
        return WATER_LEVEL * HEIGHT_SCALE;
    }

    inline bool IsWater(float worldX, float worldZ) {
        return SampleHeightNormalized(worldX, worldZ) < WATER_LEVEL;
    }
} // namespace TerrainSampler
