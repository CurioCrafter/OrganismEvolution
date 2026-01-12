#pragma once

#include <vector>
#include <glm/glm.hpp>

class Genome {
public:
    // Physical traits
    float size;           // 0.5 to 2.0 (affects speed and energy)
    float speed;          // 5.0 to 20.0 (movement speed)
    float visionRange;    // 10.0 to 50.0 (detection distance)
    float efficiency;     // 0.5 to 1.5 (energy consumption multiplier)

    // Visual traits
    glm::vec3 color;      // RGB color

    // Neural network weights
    std::vector<float> neuralWeights;

    Genome();
    Genome(const Genome& parent1, const Genome& parent2); // Crossover constructor

    void mutate(float mutationRate, float mutationStrength);
    void randomize();

    static const int NEURAL_WEIGHT_COUNT = 24; // Will be used for simple neural network
};
