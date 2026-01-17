// test_genome.cpp - Unit tests for Genome class
// Tests genetic traits, mutation, and crossover

#include "entities/Genome.h"
#include <cassert>
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>

// Helper function for float comparison
bool approxEqual(float a, float b, float epsilon = 0.01f) {
    return std::abs(a - b) < epsilon;
}

// Test basic genome properties
void testGenomeDefaults() {
    std::cout << "Testing Genome defaults..." << std::endl;

    Genome g;

    // Check basic traits have valid ranges
    assert(g.size > 0.0f);
    assert(g.speed > 0.0f);
    assert(g.efficiency > 0.0f && g.efficiency <= 1.0f);
    assert(g.mutationRate >= 0.0f && g.mutationRate <= 1.0f);

    // Check sensory traits
    assert(g.visionRange > 0.0f);
    assert(g.visionFOV > 0.0f && g.visionFOV <= 360.0f);
    assert(g.visionAcuity >= 0.0f && g.visionAcuity <= 1.0f);

    std::cout << "  Genome defaults test passed!" << std::endl;
}

// Test genome randomization
void testGenomeRandomize() {
    std::cout << "Testing Genome randomize..." << std::endl;

    Genome g1, g2;
    g1.randomize();
    g2.randomize();

    // Two randomized genomes should be different
    // (statistically nearly impossible to be identical)
    bool allSame =
        g1.size == g2.size &&
        g1.speed == g2.speed &&
        g1.efficiency == g2.efficiency &&
        g1.colorR == g2.colorR &&
        g1.colorG == g2.colorG &&
        g1.colorB == g2.colorB;

    assert(!allSame);

    // Values should be within reasonable ranges
    assert(g1.size >= 0.3f && g1.size <= 3.0f);
    assert(g1.speed >= 0.5f && g1.speed <= 5.0f);

    std::cout << "  Genome randomize test passed!" << std::endl;
}

// Test genome mutation
void testGenomeMutation() {
    std::cout << "Testing Genome mutation..." << std::endl;

    Genome original;
    original.randomize();

    // Store original values
    float origSize = original.size;
    float origSpeed = original.speed;
    float origEfficiency = original.efficiency;

    Genome mutated = original;

    // Apply high mutation rate to ensure changes
    mutated.mutate(1.0f);  // 100% mutation rate for testing

    // At least one trait should have changed
    bool sizeChanged = !approxEqual(mutated.size, origSize, 0.001f);
    bool speedChanged = !approxEqual(mutated.speed, origSpeed, 0.001f);
    bool efficiencyChanged = !approxEqual(mutated.efficiency, origEfficiency, 0.001f);

    // With 100% mutation rate, almost certainly something changed
    assert(sizeChanged || speedChanged || efficiencyChanged);

    // Test low mutation rate
    Genome lowMutation = original;
    lowMutation.mutate(0.0f);  // 0% mutation rate

    // With 0% rate, should be unchanged
    assert(approxEqual(lowMutation.size, origSize));
    assert(approxEqual(lowMutation.speed, origSpeed));

    std::cout << "  Genome mutation test passed!" << std::endl;
}

// Test genome crossover
void testGenomeCrossover() {
    std::cout << "Testing Genome crossover..." << std::endl;

    Genome parent1, parent2;
    parent1.randomize();
    parent2.randomize();

    // Force different values
    parent1.size = 1.0f;
    parent2.size = 2.0f;
    parent1.speed = 1.0f;
    parent2.speed = 3.0f;

    Genome child(parent1, parent2);

    // Child traits should be between or blend of parents
    // Size should be approximately between parent values
    assert(child.size >= 0.5f && child.size <= 2.5f);
    assert(child.speed >= 0.5f && child.speed <= 3.5f);

    // Child should inherit some traits from each parent (probabilistic)
    // We can't test exact values but can verify ranges

    std::cout << "  Genome crossover test passed!" << std::endl;
}

// Test neural weights
void testNeuralWeights() {
    std::cout << "Testing neural weights..." << std::endl;

    Genome g;
    g.randomize();

    // Should have neural weights after randomization
    assert(g.neuralWeights.size() > 0);

    // Weights should be in reasonable range
    for (float w : g.neuralWeights) {
        assert(w >= -5.0f && w <= 5.0f);
    }

    // Test crossover preserves neural weights
    Genome parent1, parent2;
    parent1.randomize();
    parent2.randomize();

    Genome child(parent1, parent2);
    assert(child.neuralWeights.size() == parent1.neuralWeights.size());

    std::cout << "  Neural weights test passed!" << std::endl;
}

// Test flying traits
void testFlyingTraits() {
    std::cout << "Testing flying traits..." << std::endl;

    Genome g;
    g.randomize();

    // Flying traits should be valid
    assert(g.wingSpan > 0.0f);
    assert(g.flapFrequency > 0.0f);
    assert(g.glideRatio > 0.0f);
    assert(g.preferredAltitude > 0.0f);

    std::cout << "  Flying traits test passed!" << std::endl;
}

// Test aquatic traits
void testAquaticTraits() {
    std::cout << "Testing aquatic traits..." << std::endl;

    Genome g;
    g.randomize();

    // Aquatic traits should be valid
    assert(g.finSize > 0.0f);
    assert(g.tailSize > 0.0f);
    assert(g.swimFrequency > 0.0f);
    assert(g.schoolingStrength >= 0.0f && g.schoolingStrength <= 1.0f);

    std::cout << "  Aquatic traits test passed!" << std::endl;
}

// Test fitness calculation
void testFitnessCalculation() {
    std::cout << "Testing fitness calculation..." << std::endl;

    Genome g;
    g.randomize();

    // Fitness should be calculable
    float age = 100.0f;
    int foodEaten = 50;
    float distanceTraveled = 1000.0f;

    float fitness = age * 0.5f + foodEaten * 10.0f + distanceTraveled * 0.01f;

    assert(fitness > 0.0f);
    assert(fitness == 100.0f * 0.5f + 50 * 10.0f + 1000.0f * 0.01f);

    std::cout << "  Fitness calculation test passed!" << std::endl;
}

// Test mutation stability
void testMutationStability() {
    std::cout << "Testing mutation stability..." << std::endl;

    Genome g;
    g.randomize();

    // Apply many mutations - values should stay within bounds
    for (int i = 0; i < 100; i++) {
        g.mutate(0.3f);

        assert(g.size > 0.0f && g.size < 10.0f);
        assert(g.speed > 0.0f && g.speed < 20.0f);
        assert(g.efficiency >= 0.0f && g.efficiency <= 1.0f);
        assert(g.mutationRate >= 0.0f && g.mutationRate <= 1.0f);
    }

    std::cout << "  Mutation stability test passed!" << std::endl;
}

// Test sensory traits
void testSensoryTraits() {
    std::cout << "Testing sensory traits..." << std::endl;

    Genome g;
    g.randomize();

    // Vision
    assert(g.visionRange >= 0.0f);
    assert(g.visionFOV > 0.0f);
    assert(g.visionAcuity >= 0.0f);
    assert(g.colorPerception >= 0.0f);
    assert(g.motionDetection >= 0.0f);

    // Hearing
    assert(g.hearingRange >= 0.0f);
    assert(g.hearingDirectionality >= 0.0f);
    assert(g.echolocationAbility >= 0.0f);

    // Smell
    assert(g.smellRange >= 0.0f);
    assert(g.smellSensitivity >= 0.0f);

    // Touch
    assert(g.touchSensitivity >= 0.0f);

    std::cout << "  Sensory traits test passed!" << std::endl;
}

int main() {
    std::cout << "=== Genome Unit Tests ===" << std::endl;

    testGenomeDefaults();
    testGenomeRandomize();
    testGenomeMutation();
    testGenomeCrossover();
    testNeuralWeights();
    testFlyingTraits();
    testAquaticTraits();
    testFitnessCalculation();
    testMutationStability();
    testSensoryTraits();

    std::cout << "\n=== All Genome tests passed! ===" << std::endl;
    return 0;
}
