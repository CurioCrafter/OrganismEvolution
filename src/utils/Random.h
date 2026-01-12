#pragma once

#include <random>

class Random {
public:
    static void init();
    static float range(float min, float max);
    static int rangeInt(int min, int max);
    static float value(); // Returns 0.0 to 1.0
    static bool chance(float probability); // 0.0 to 1.0

private:
    static std::mt19937 s_RandomEngine;
    static std::uniform_real_distribution<float> s_Distribution;
};
