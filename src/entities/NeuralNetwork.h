#pragma once

#include <vector>

class NeuralNetwork {
public:
    NeuralNetwork(const std::vector<float>& weights);

    // Returns movement direction (angle) and speed multiplier
    void process(const std::vector<float>& inputs, float& outAngle, float& outSpeed);

private:
    std::vector<float> weights;
    const int inputCount = 4;    // Distance to food, angle to food, energy, distance to threat
    const int hiddenCount = 4;   // Hidden layer neurons
    const int outputCount = 2;   // Movement angle, speed multiplier

    float sigmoid(float x);
    float tanh(float x);
};
