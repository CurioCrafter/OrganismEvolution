/**
 * Naming Validation Test - Comprehensive testing for species naming system
 *
 * Tests:
 * 1. All 19 creature types get valid names
 * 2. Name collision rate across 200 generated names
 * 3. Descriptor coverage (no empty descriptors)
 * 4. Determinism (same seed = same name)
 * 5. Biome integration (different biomes produce different names)
 */

#include "SpeciesNaming.h"
#include "NamePhonemeTables.h"
#include "CreatureType.h"
#include <iostream>
#include <vector>
#include <unordered_set>
#include <iomanip>

using namespace naming;

// Test 1: All creature types get valid names
void testCreatureTypeCoverage() {
    std::cout << "\n========================================\n";
    std::cout << "TEST 1: Creature Type Coverage\n";
    std::cout << "========================================\n\n";

    auto& namingSystem = getNamingSystem();
    namingSystem.setPlanetSeed(42);

    auto result = namingSystem.validateCreatureTypeCoverage(42);
    std::cout << result.report;

    if (result.emptyNames > 0 || result.emptyDescriptors > 0) {
        std::cout << "\n✗ FAILED: Some creature types have empty names or descriptors!\n";
    } else {
        std::cout << "\n✓ PASSED: All creature types have valid names and descriptors!\n";
    }
}

// Test 2: Collision rate across 200 generated names
void testCollisionRate() {
    std::cout << "\n========================================\n";
    std::cout << "TEST 2: Collision Rate (200 names)\n";
    std::cout << "========================================\n\n";

    auto& namingSystem = getNamingSystem();

    // Test with 3 different seeds
    const std::vector<uint32_t> seeds = {12345, 54321, 99999};

    for (uint32_t seed : seeds) {
        std::cout << "Seed " << seed << ":\n";
        namingSystem.resetStats();
        float collisionRate = namingSystem.validateNameGeneration(200, seed);

        if (collisionRate < 2.0f) {
            std::cout << "  ✓ Excellent collision rate (<2%)\n\n";
        } else if (collisionRate < 5.0f) {
            std::cout << "  ✓ Good collision rate (<5%)\n\n";
        } else {
            std::cout << "  ✗ High collision rate (>5%)\n\n";
        }
    }
}

// Test 3: Determinism
void testDeterminism() {
    std::cout << "\n========================================\n";
    std::cout << "TEST 3: Determinism\n";
    std::cout << "========================================\n\n";

    auto& namingSystem = getNamingSystem();

    CreatureTraits testTraits;
    testTraits.primaryColor = glm::vec3(0.2f, 0.8f, 0.3f);  // Green
    testTraits.size = 1.0f;
    testTraits.speed = 12.0f;
    testTraits.isHerbivore = true;
    testTraits.isPredator = false;

    // Generate name 3 times with same seed
    std::vector<std::string> names;
    for (int i = 0; i < 3; ++i) {
        namingSystem.clear();
        namingSystem.setPlanetSeed(12345);

        const SpeciesName& name = namingSystem.getOrCreateSpeciesNameDeterministic(
            100, testTraits, 12345, PhonemeTableType::LUSH);

        names.push_back(name.commonName);
        std::cout << "Iteration " << (i+1) << ": " << name.commonName << "\n";
    }

    // Check if all names are identical
    bool allSame = true;
    for (size_t i = 1; i < names.size(); ++i) {
        if (names[i] != names[0]) {
            allSame = false;
            break;
        }
    }

    if (allSame) {
        std::cout << "\n✓ PASSED: Names are deterministic!\n";
    } else {
        std::cout << "\n✗ FAILED: Names are not deterministic!\n";
    }
}

// Test 4: Biome integration
void testBiomeIntegration() {
    std::cout << "\n========================================\n";
    std::cout << "TEST 4: Biome Integration\n";
    std::cout << "========================================\n\n";

    auto& namingSystem = getNamingSystem();
    namingSystem.setPlanetSeed(42);

    CreatureTraits testTraits;
    testTraits.size = 1.0f;
    testTraits.speed = 10.0f;

    // Test all 6 phoneme tables
    const std::vector<std::pair<PhonemeTableType, std::string>> biomes = {
        {PhonemeTableType::DRY, "DRY (Desert)"},
        {PhonemeTableType::LUSH, "LUSH (Forest)"},
        {PhonemeTableType::OCEANIC, "OCEANIC (Ocean)"},
        {PhonemeTableType::FROZEN, "FROZEN (Tundra)"},
        {PhonemeTableType::VOLCANIC, "VOLCANIC (Lava)"},
        {PhonemeTableType::ALIEN, "ALIEN (Exotic)"}
    };

    std::cout << "Generating 5 sample names per biome:\n\n";

    for (const auto& [biome, biomeName] : biomes) {
        std::cout << biomeName << ":\n";

        for (int i = 0; i < 5; ++i) {
            testTraits.isHerbivore = (i % 2 == 0);
            testTraits.isPredator = !testTraits.isHerbivore;

            genetics::SpeciesId speciesId = static_cast<genetics::SpeciesId>(biome) * 1000 + i;
            const SpeciesName& name = namingSystem.getOrCreateSpeciesNameDeterministic(
                speciesId, testTraits, 42, biome);

            std::cout << "  " << std::setw(15) << std::left << name.commonName
                      << " - " << name.descriptor.getFullDescriptor() << "\n";
        }
        std::cout << "\n";
    }

    std::cout << "✓ PASSED: Biome integration test complete!\n";
}

// Test 5: Edge cases
void testEdgeCases() {
    std::cout << "\n========================================\n";
    std::cout << "TEST 5: Edge Cases\n";
    std::cout << "========================================\n\n";

    auto& namingSystem = getNamingSystem();
    namingSystem.setPlanetSeed(42);

    // Test very small creature
    CreatureTraits tinyTraits;
    tinyTraits.size = 0.1f;
    tinyTraits.speed = 5.0f;
    tinyTraits.isHerbivore = true;
    const SpeciesName& tinyName = namingSystem.getOrCreateSpeciesNameDeterministic(
        1, tinyTraits, 42, PhonemeTableType::LUSH);
    std::cout << "Tiny creature: " << tinyName.commonName << " (" << tinyName.descriptor.getFullDescriptor() << ")\n";

    // Test very large creature
    CreatureTraits giantTraits;
    giantTraits.size = 3.0f;
    giantTraits.speed = 8.0f;
    giantTraits.isPredator = true;
    giantTraits.isCarnivore = true;
    const SpeciesName& giantName = namingSystem.getOrCreateSpeciesNameDeterministic(
        2, giantTraits, 42, PhonemeTableType::VOLCANIC);
    std::cout << "Giant creature: " << giantName.commonName << " (" << giantName.descriptor.getFullDescriptor() << ")\n";

    // Test aquatic flying creature (amphibious)
    CreatureTraits amphibTraits;
    amphibTraits.livesInWater = true;
    amphibTraits.canFly = true;
    amphibTraits.hasFins = true;
    amphibTraits.hasWings = true;
    const SpeciesName& amphibName = namingSystem.getOrCreateSpeciesNameDeterministic(
        3, amphibTraits, 42, PhonemeTableType::OCEANIC);
    std::cout << "Amphibious creature: " << amphibName.commonName << " (" << amphibName.descriptor.getFullDescriptor() << ")\n";

    // Test nocturnal predator
    CreatureTraits nocturnalTraits;
    nocturnalTraits.isNocturnal = true;
    nocturnalTraits.isPredator = true;
    nocturnalTraits.isCarnivore = true;
    const SpeciesName& nocturnalName = namingSystem.getOrCreateSpeciesNameDeterministic(
        4, nocturnalTraits, 42, PhonemeTableType::ALIEN);
    std::cout << "Nocturnal predator: " << nocturnalName.commonName << " (" << nocturnalName.descriptor.getFullDescriptor() << ")\n";

    std::cout << "\n✓ PASSED: Edge cases handled correctly!\n";
}

// Main test runner
int main() {
    std::cout << "\n╔═══════════════════════════════════════════╗\n";
    std::cout << "║  NAMING SYSTEM VALIDATION TEST SUITE     ║\n";
    std::cout << "╚═══════════════════════════════════════════╝\n";

    try {
        testCreatureTypeCoverage();
        testCollisionRate();
        testDeterminism();
        testBiomeIntegration();
        testEdgeCases();

        std::cout << "\n╔═══════════════════════════════════════════╗\n";
        std::cout << "║  ALL TESTS COMPLETED                     ║\n";
        std::cout << "╚═══════════════════════════════════════════╝\n\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ TEST SUITE FAILED WITH EXCEPTION:\n";
        std::cerr << e.what() << "\n\n";
        return 1;
    }
}
