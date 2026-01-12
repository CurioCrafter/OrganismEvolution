#pragma once

#include <vector>

class PerlinNoise {
public:
    PerlinNoise();
    PerlinNoise(unsigned int seed);

    float noise(float x, float y, float z = 0.0f);
    float octaveNoise(float x, float y, int octaves, float persistence = 0.5f);

private:
    std::vector<int> p;

    float fade(float t);
    float lerp(float t, float a, float b);
    float grad(int hash, float x, float y, float z);
};
