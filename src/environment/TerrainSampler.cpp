#include "TerrainSampler.h"
#include <algorithm>
#include <cmath>

namespace TerrainSampler {
    namespace {
        inline float fade(float t) {
            return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
        }

        inline float lerp(float a, float b, float t) {
            return a + t * (b - a);
        }

        inline float grad(int hash, float x, float y) {
            int h = hash & 7;
            float u = h < 4 ? x : y;
            float v = h < 4 ? y : x;
            return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
        }

        static const int perm[512] = {
            151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
            8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
            35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
            134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
            55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
            18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
            250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
            189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
            172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
            228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
            107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
            138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
            151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
            8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
            35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
            134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
            55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
            18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
            250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
            189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
            172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
            228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
            107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
            138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
        };

        inline float perlin2D(float x, float y) {
            int X = static_cast<int>(std::floor(x)) & 255;
            int Y = static_cast<int>(std::floor(y)) & 255;

            x -= std::floor(x);
            y -= std::floor(y);

            float u = fade(x);
            float v = fade(y);

            int A = perm[X] + Y;
            int B = perm[X + 1] + Y;

            return lerp(
                lerp(grad(perm[A], x, y), grad(perm[B], x - 1, y), u),
                lerp(grad(perm[A + 1], x, y - 1), grad(perm[B + 1], x - 1, y - 1), u),
                v
            );
        }

        inline float octaveNoise(float x, float y, int octaves, float persistence) {
            float total = 0.0f;
            float frequency = 1.0f;
            float amplitude = 1.0f;
            float maxValue = 0.0f;

            for (int i = 0; i < octaves; ++i) {
                total += perlin2D(x * frequency, y * frequency) * amplitude;
                maxValue += amplitude;
                amplitude *= persistence;
                frequency *= 2.0f;
            }

            return (total / maxValue + 1.0f) * 0.5f;
        }

        inline float smoothstep(float edge0, float edge1, float x) {
            float t = std::max(0.0f, std::min(1.0f, (x - edge0) / (edge1 - edge0)));
            return t * t * (3.0f - 2.0f * t);
        }
    } // namespace

    float SampleHeightNormalized(float worldX, float worldZ) {
        float nx = worldX / WORLD_SIZE + 0.5f;
        float nz = worldZ / WORLD_SIZE + 0.5f;

        float dx = nx - 0.5f;
        float dz = nz - 0.5f;
        float distance = std::sqrt(dx * dx + dz * dz) * 2.0f;

        float continental = octaveNoise(nx * 2.0f, nz * 2.0f, 4, 0.6f);
        float mountains = octaveNoise(nx * 4.0f + 100.0f, nz * 4.0f + 100.0f, 6, 0.5f);
        mountains = std::pow(mountains, 1.5f);
        float hills = octaveNoise(nx * 8.0f + 50.0f, nz * 8.0f + 50.0f, 4, 0.5f);

        float ridgeNoise = octaveNoise(nx * 3.0f + 200.0f, nz * 3.0f + 200.0f, 4, 0.5f);
        float ridges = 1.0f - std::abs(ridgeNoise * 2.0f - 1.0f);
        ridges = std::pow(ridges, 2.0f) * 0.3f;

        float height = continental * 0.3f + mountains * 0.45f + hills * 0.15f + ridges;

        if (height < 0.35f) {
            height = height * 0.8f;
        } else if (height > 0.7f) {
            float excess = (height - 0.7f) / 0.3f;
            height = 0.7f + excess * excess * 0.3f;
        }

        float islandFactor = 1.0f - smoothstep(0.4f, 0.95f, distance);
        height = height * islandFactor;
        height = height * 1.1f - 0.05f;

        return std::max(0.0f, std::min(1.0f, height));
    }

    float SampleHeight(float worldX, float worldZ) {
        return SampleHeightNormalized(worldX, worldZ) * HEIGHT_SCALE;
    }
} // namespace TerrainSampler
