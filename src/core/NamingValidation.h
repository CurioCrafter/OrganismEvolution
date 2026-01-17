#pragma once

#include "../entities/SpeciesNameGenerator.h"
#include "../entities/SpeciesNaming.h"
#include "../entities/NamePhonemeTables.h"
#include "../entities/CreatureType.h"
#include "../entities/Genome.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace naming {

/**
 * NamingValidation - Comprehensive testing and validation for the naming system
 *
 * This class provides utilities to test naming coverage, collision rates,
 * and determinism across all creature types and biomes.
 */
class NamingValidation {
public:
    struct ValidationResult {
        int totalGenerated = 0;
        int uniqueNames = 0;
        int collisions = 0;
        float collisionRate = 0.0f;
        float averageNameLength = 0.0f;

        // Per creature type statistics
        std::unordered_map<CreatureType, int> namesByType;
        std::unordered_map<CreatureType, std::vector<std::string>> examplesByType;

        // Missing or failed names (should always be empty!)
        std::vector<std::string> missingTypes;
        std::vector<std::string> emptyNames;
    };

    /**
     * Validate naming coverage for all creature types
     * Generates names for every CreatureType enum value and checks for gaps
     *
     * @param seed Planet seed for deterministic testing
     * @param namesPerType How many names to generate per creature type
     * @return Validation results with coverage statistics
     */
    static ValidationResult validateAllCreatureTypes(uint32_t seed = 42, int namesPerType = 10);

    /**
     * Validate collision rates across multiple seeds
     *
     * @param seeds Vector of planet seeds to test
     * @param namesPerSeed How many names to generate per seed
     * @return Aggregate validation results
     */
    static ValidationResult validateMultipleSeeds(const std::vector<uint32_t>& seeds, int namesPerSeed = 200);

    /**
     * Validate biome-specific naming
     * Tests that all phoneme tables produce unique names
     *
     * @param seed Planet seed
     * @param namesPerTable How many names to generate per phoneme table
     * @return Validation results by biome
     */
    static ValidationResult validateBiomeNaming(uint32_t seed = 42, int namesPerTable = 50);

    /**
     * Test determinism - same seed should produce same name
     *
     * @param iterations How many times to regenerate names
     * @return True if all names are consistent, false if any mismatch
     */
    static bool validateDeterminism(int iterations = 10);

    /**
     * Print validation results in human-readable format
     */
    static void printValidationResults(const ValidationResult& result);

    /**
     * Generate a comprehensive test report and write to file
     *
     * @param outputPath Path to write the test report
     * @param seeds Seeds to test with
     */
    static void generateTestReport(const std::string& outputPath,
                                   const std::vector<uint32_t>& seeds = {42, 12345, 99999});

private:
    static Genome createTestGenome(uint32_t seed, float sizeVariation = 1.0f, float speedVariation = 1.0f);
    static std::string creatureTypeToString(CreatureType type);
};

} // namespace naming
