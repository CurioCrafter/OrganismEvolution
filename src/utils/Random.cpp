#include "Random.h"
#include <chrono>

std::mt19937 Random::s_RandomEngine;
std::uniform_real_distribution<float> Random::s_Distribution(0.0f, 1.0f);

void Random::init() {
    s_RandomEngine.seed(std::chrono::system_clock::now().time_since_epoch().count());
}

float Random::range(float min, float max) {
    return min + (max - min) * s_Distribution(s_RandomEngine);
}

int Random::rangeInt(int min, int max) {
    return min + (int)(s_Distribution(s_RandomEngine) * (max - min + 1));
}

float Random::value() {
    return s_Distribution(s_RandomEngine);
}

bool Random::chance(float probability) {
    return s_Distribution(s_RandomEngine) < probability;
}
