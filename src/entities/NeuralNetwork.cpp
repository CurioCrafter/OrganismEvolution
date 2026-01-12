#include "NeuralNetwork.h"
#include <cmath>
#include <algorithm>

NeuralNetwork::NeuralNetwork(const std::vector<float>& weights)
    : weights(weights) {
}

void NeuralNetwork::process(const std::vector<float>& inputs, float& outAngle, float& outSpeed) {
    if (inputs.size() != inputCount) {
        outAngle = 0.0f;
        outSpeed = 0.5f;
        return;
    }

    // Input to hidden layer
    std::vector<float> hidden(hiddenCount, 0.0f);
    int weightIdx = 0;

    for (int h = 0; h < hiddenCount; h++) {
        float sum = 0.0f;
        for (int i = 0; i < inputCount; i++) {
            sum += inputs[i] * weights[weightIdx++];
        }
        hidden[h] = tanh(sum);
    }

    // Hidden to output layer
    std::vector<float> output(outputCount, 0.0f);

    for (int o = 0; o < outputCount; o++) {
        float sum = 0.0f;
        for (int h = 0; h < hiddenCount; h++) {
            sum += hidden[h] * weights[weightIdx++];
        }
        output[o] = tanh(sum);
    }

    // Interpret outputs
    outAngle = output[0] * 3.14159f; // -PI to PI
    outSpeed = (output[1] + 1.0f) * 0.5f; // 0 to 1
}

float NeuralNetwork::sigmoid(float x) {
    return 1.0f / (1.0f + exp(-x));
}

float NeuralNetwork::tanh(float x) {
    return std::tanh(x);
}
