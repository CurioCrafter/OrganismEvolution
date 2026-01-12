#include "Genome.h"
#include "../utils/Random.h"
#include <algorithm>

Genome::Genome() {
    randomize();
}

Genome::Genome(const Genome& parent1, const Genome& parent2) {
    // Crossover - blend traits from both parents
    if (Random::chance(0.5f)) {
        size = parent1.size;
        speed = parent2.speed;
    } else {
        size = parent2.size;
        speed = parent1.speed;
    }

    if (Random::chance(0.5f)) {
        visionRange = parent1.visionRange;
        efficiency = parent2.efficiency;
    } else {
        visionRange = parent2.visionRange;
        efficiency = parent1.efficiency;
    }

    // Blend colors
    color = (parent1.color + parent2.color) * 0.5f;

    // Uniform crossover for neural weights
    neuralWeights.resize(NEURAL_WEIGHT_COUNT);
    for (int i = 0; i < NEURAL_WEIGHT_COUNT; i++) {
        if (Random::chance(0.5f)) {
            neuralWeights[i] = parent1.neuralWeights[i];
        } else {
            neuralWeights[i] = parent2.neuralWeights[i];
        }
    }
}

void Genome::mutate(float mutationRate, float mutationStrength) {
    // Mutate physical traits
    if (Random::chance(mutationRate)) {
        size = std::clamp(size + Random::range(-mutationStrength, mutationStrength), 0.5f, 2.0f);
    }

    if (Random::chance(mutationRate)) {
        speed = std::clamp(speed + Random::range(-mutationStrength * 3.0f, mutationStrength * 3.0f), 5.0f, 20.0f);
    }

    if (Random::chance(mutationRate)) {
        visionRange = std::clamp(visionRange + Random::range(-mutationStrength * 10.0f, mutationStrength * 10.0f), 10.0f, 50.0f);
    }

    if (Random::chance(mutationRate)) {
        efficiency = std::clamp(efficiency + Random::range(-mutationStrength * 0.2f, mutationStrength * 0.2f), 0.5f, 1.5f);
    }

    // Mutate color slightly
    if (Random::chance(mutationRate)) {
        color.r = std::clamp(color.r + Random::range(-0.1f, 0.1f), 0.0f, 1.0f);
        color.g = std::clamp(color.g + Random::range(-0.1f, 0.1f), 0.0f, 1.0f);
        color.b = std::clamp(color.b + Random::range(-0.1f, 0.1f), 0.0f, 1.0f);
    }

    // Mutate neural weights
    for (int i = 0; i < NEURAL_WEIGHT_COUNT; i++) {
        if (Random::chance(mutationRate)) {
            neuralWeights[i] = std::clamp(
                neuralWeights[i] + Random::range(-mutationStrength, mutationStrength),
                -1.0f, 1.0f
            );
        }
    }
}

void Genome::randomize() {
    size = Random::range(0.5f, 2.0f);
    speed = Random::range(5.0f, 20.0f);
    visionRange = Random::range(10.0f, 50.0f);
    efficiency = Random::range(0.5f, 1.5f);

    color = glm::vec3(Random::value(), Random::value(), Random::value());

    neuralWeights.resize(NEURAL_WEIGHT_COUNT);
    for (int i = 0; i < NEURAL_WEIGHT_COUNT; i++) {
        neuralWeights[i] = Random::range(-1.0f, 1.0f);
    }
}
