// test_neural_network.cpp - Unit tests for NeuralNetwork class
// Tests forward pass, activation functions, and weight mutation

#include "entities/NeuralNetwork.h"
#include <cassert>
#include <iostream>
#include <cmath>
#include <vector>
#include <numeric>

// Helper function for float comparison
bool approxEqual(float a, float b, float epsilon = 0.01f) {
    return std::abs(a - b) < epsilon;
}

// Test neural network creation
void testNeuralNetworkCreation() {
    std::cout << "Testing NeuralNetwork creation..." << std::endl;

    // Create with default weights
    std::vector<float> weights(200, 0.5f);  // 200 weights initialized to 0.5
    NeuralNetwork nn(weights);

    // Check input/output counts
    assert(nn.getInputCount() == 8);
    assert(nn.getOutputCount() == 6);

    std::cout << "  NeuralNetwork creation test passed!" << std::endl;
}

// Test forward pass
void testForwardPass() {
    std::cout << "Testing forward pass..." << std::endl;

    std::vector<float> weights(200, 0.1f);
    NeuralNetwork nn(weights);

    // Create input vector
    std::vector<float> inputs = {
        0.5f,   // Food distance
        0.3f,   // Food angle
        0.8f,   // Threat distance
        -0.2f,  // Threat angle
        0.7f,   // Energy level
        0.4f,   // Speed
        0.5f,   // Allies nearby
        0.2f    // Fear level
    };

    NeuralOutputs outputs = nn.forward(inputs);

    // Check all outputs are in valid range (-1 to 1 for most)
    assert(outputs.turnAngle >= -1.0f && outputs.turnAngle <= 1.0f);
    assert(outputs.speedMultiplier >= -1.0f && outputs.speedMultiplier <= 1.0f);
    assert(outputs.aggressionMod >= -1.0f && outputs.aggressionMod <= 1.0f);
    assert(outputs.fearMod >= -1.0f && outputs.fearMod <= 1.0f);
    assert(outputs.socialMod >= -1.0f && outputs.socialMod <= 1.0f);
    assert(outputs.explorationMod >= -1.0f && outputs.explorationMod <= 1.0f);

    std::cout << "  Forward pass test passed!" << std::endl;
}

// Test deterministic behavior
void testDeterminism() {
    std::cout << "Testing deterministic behavior..." << std::endl;

    std::vector<float> weights(200, 0.2f);
    NeuralNetwork nn(weights);

    std::vector<float> inputs = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};

    // Same inputs should produce same outputs
    NeuralOutputs out1 = nn.forward(inputs);
    NeuralOutputs out2 = nn.forward(inputs);

    assert(approxEqual(out1.turnAngle, out2.turnAngle));
    assert(approxEqual(out1.speedMultiplier, out2.speedMultiplier));
    assert(approxEqual(out1.aggressionMod, out2.aggressionMod));
    assert(approxEqual(out1.fearMod, out2.fearMod));
    assert(approxEqual(out1.socialMod, out2.socialMod));
    assert(approxEqual(out1.explorationMod, out2.explorationMod));

    std::cout << "  Determinism test passed!" << std::endl;
}

// Test different weights produce different outputs
void testWeightSensitivity() {
    std::cout << "Testing weight sensitivity..." << std::endl;

    std::vector<float> weights1(200, 0.1f);
    std::vector<float> weights2(200, 0.9f);

    NeuralNetwork nn1(weights1);
    NeuralNetwork nn2(weights2);

    std::vector<float> inputs = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};

    NeuralOutputs out1 = nn1.forward(inputs);
    NeuralOutputs out2 = nn2.forward(inputs);

    // Different weights should produce different outputs
    bool allSame =
        approxEqual(out1.turnAngle, out2.turnAngle) &&
        approxEqual(out1.speedMultiplier, out2.speedMultiplier) &&
        approxEqual(out1.aggressionMod, out2.aggressionMod) &&
        approxEqual(out1.fearMod, out2.fearMod);

    assert(!allSame);

    std::cout << "  Weight sensitivity test passed!" << std::endl;
}

// Test input sensitivity
void testInputSensitivity() {
    std::cout << "Testing input sensitivity..." << std::endl;

    std::vector<float> weights(200, 0.3f);
    NeuralNetwork nn(weights);

    std::vector<float> inputs1 = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    std::vector<float> inputs2 = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

    NeuralOutputs out1 = nn.forward(inputs1);
    NeuralOutputs out2 = nn.forward(inputs2);

    // Different inputs should produce different outputs
    bool allSame =
        approxEqual(out1.turnAngle, out2.turnAngle) &&
        approxEqual(out1.aggressionMod, out2.aggressionMod);

    assert(!allSame);

    std::cout << "  Input sensitivity test passed!" << std::endl;
}

// Test edge case inputs
void testEdgeCases() {
    std::cout << "Testing edge cases..." << std::endl;

    std::vector<float> weights(200, 0.5f);
    NeuralNetwork nn(weights);

    // Test all zeros
    std::vector<float> zeros(8, 0.0f);
    NeuralOutputs outZero = nn.forward(zeros);
    assert(!std::isnan(outZero.turnAngle));
    assert(!std::isnan(outZero.aggressionMod));

    // Test all ones
    std::vector<float> ones(8, 1.0f);
    NeuralOutputs outOne = nn.forward(ones);
    assert(!std::isnan(outOne.turnAngle));
    assert(!std::isnan(outOne.aggressionMod));

    // Test negative values
    std::vector<float> negatives(8, -1.0f);
    NeuralOutputs outNeg = nn.forward(negatives);
    assert(!std::isnan(outNeg.turnAngle));
    assert(!std::isnan(outNeg.aggressionMod));

    // Test extreme values
    std::vector<float> extreme = {100.0f, -100.0f, 0.0f, 1000.0f, -1000.0f, 0.5f, 0.5f, 0.5f};
    NeuralOutputs outExtreme = nn.forward(extreme);
    assert(!std::isnan(outExtreme.turnAngle));
    // Outputs should still be bounded due to tanh
    assert(outExtreme.turnAngle >= -1.0f && outExtreme.turnAngle <= 1.0f);

    std::cout << "  Edge cases test passed!" << std::endl;
}

// Test empty/short input handling
void testInputPadding() {
    std::cout << "Testing input padding..." << std::endl;

    std::vector<float> weights(200, 0.3f);
    NeuralNetwork nn(weights);

    // Test with fewer inputs than expected
    std::vector<float> shortInput = {0.5f, 0.5f, 0.5f};  // Only 3 inputs
    NeuralOutputs out = nn.forward(shortInput);

    // Should handle gracefully (padding with zeros)
    assert(!std::isnan(out.turnAngle));
    assert(!std::isnan(out.aggressionMod));

    std::cout << "  Input padding test passed!" << std::endl;
}

// Test weight variations
void testWeightVariations() {
    std::cout << "Testing weight variations..." << std::endl;

    // Test with zero weights
    std::vector<float> zeroWeights(200, 0.0f);
    NeuralNetwork nnZero(zeroWeights);
    std::vector<float> inputs = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
    NeuralOutputs outZero = nnZero.forward(inputs);
    // With zero weights, output should be near zero (biased by activation)
    assert(!std::isnan(outZero.turnAngle));

    // Test with alternating weights
    std::vector<float> altWeights(200);
    for (size_t i = 0; i < altWeights.size(); i++) {
        altWeights[i] = (i % 2 == 0) ? 0.5f : -0.5f;
    }
    NeuralNetwork nnAlt(altWeights);
    NeuralOutputs outAlt = nnAlt.forward(inputs);
    assert(!std::isnan(outAlt.turnAngle));

    std::cout << "  Weight variations test passed!" << std::endl;
}

// Test process() method (legacy interface)
void testProcessMethod() {
    std::cout << "Testing process() method..." << std::endl;

    std::vector<float> weights(200, 0.3f);
    NeuralNetwork nn(weights);

    // process() should work with 4 legacy inputs
    std::vector<float> legacyInputs = {0.5f, 0.3f, 0.8f, 0.2f};
    float outAngle = 0.0f, outSpeed = 0.0f;
    nn.process(legacyInputs, outAngle, outSpeed);

    // Should return valid outputs
    assert(!std::isnan(outAngle));
    assert(!std::isnan(outSpeed));

    std::cout << "  process() method test passed!" << std::endl;
}

// Test behavior modulation ranges
void testBehaviorModulation() {
    std::cout << "Testing behavior modulation ranges..." << std::endl;

    std::vector<float> weights(200, 0.4f);
    NeuralNetwork nn(weights);

    // Run many random inputs
    for (int i = 0; i < 100; i++) {
        std::vector<float> inputs(8);
        for (int j = 0; j < 8; j++) {
            inputs[j] = (float)rand() / RAND_MAX;
        }

        NeuralOutputs out = nn.forward(inputs);

        // All outputs should be bounded
        assert(out.turnAngle >= -1.0f && out.turnAngle <= 1.0f);
        assert(out.speedMultiplier >= -1.0f && out.speedMultiplier <= 1.0f);
        assert(out.aggressionMod >= -1.0f && out.aggressionMod <= 1.0f);
        assert(out.fearMod >= -1.0f && out.fearMod <= 1.0f);
        assert(out.socialMod >= -1.0f && out.socialMod <= 1.0f);
        assert(out.explorationMod >= -1.0f && out.explorationMod <= 1.0f);
    }

    std::cout << "  Behavior modulation test passed!" << std::endl;
}

int main() {
    std::cout << "=== NeuralNetwork Unit Tests ===" << std::endl;

    testNeuralNetworkCreation();
    testForwardPass();
    testDeterminism();
    testWeightSensitivity();
    testInputSensitivity();
    testEdgeCases();
    testInputPadding();
    testWeightVariations();
    testProcessMethod();
    testBehaviorModulation();

    std::cout << "\n=== All NeuralNetwork tests passed! ===" << std::endl;
    return 0;
}
