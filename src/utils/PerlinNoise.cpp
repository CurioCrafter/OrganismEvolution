#include "PerlinNoise.h"
#include <cmath>
#include <algorithm>
#include <random>

PerlinNoise::PerlinNoise() {
    p.resize(512);

    // Fill p with values from 0 to 255
    for (int i = 0; i < 256; i++) {
        p[i] = i;
    }

    // Shuffle using seed
    std::random_device rd;
    std::mt19937 engine(rd());
    std::shuffle(p.begin(), p.begin() + 256, engine);

    // Duplicate
    for (int i = 0; i < 256; i++) {
        p[256 + i] = p[i];
    }
}

PerlinNoise::PerlinNoise(unsigned int seed) {
    p.resize(512);

    for (int i = 0; i < 256; i++) {
        p[i] = i;
    }

    std::mt19937 engine(seed);
    std::shuffle(p.begin(), p.begin() + 256, engine);

    for (int i = 0; i < 256; i++) {
        p[256 + i] = p[i];
    }
}

float PerlinNoise::noise(float x, float y, float z) {
    int X = (int)floor(x) & 255;
    int Y = (int)floor(y) & 255;
    int Z = (int)floor(z) & 255;

    x -= floor(x);
    y -= floor(y);
    z -= floor(z);

    float u = fade(x);
    float v = fade(y);
    float w = fade(z);

    int A = p[X] + Y;
    int AA = p[A] + Z;
    int AB = p[A + 1] + Z;
    int B = p[X + 1] + Y;
    int BA = p[B] + Z;
    int BB = p[B + 1] + Z;

    float res = lerp(w,
        lerp(v,
            lerp(u, grad(p[AA], x, y, z), grad(p[BA], x - 1, y, z)),
            lerp(u, grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z))
        ),
        lerp(v,
            lerp(u, grad(p[AA + 1], x, y, z - 1), grad(p[BA + 1], x - 1, y, z - 1)),
            lerp(u, grad(p[AB + 1], x, y - 1, z - 1), grad(p[BB + 1], x - 1, y - 1, z - 1))
        )
    );

    return (res + 1.0f) / 2.0f;
}

float PerlinNoise::octaveNoise(float x, float y, int octaves, float persistence) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; i++) {
        total += noise(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }

    return total / maxValue;
}

float PerlinNoise::fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float PerlinNoise::lerp(float t, float a, float b) {
    return a + t * (b - a);
}

float PerlinNoise::grad(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}
