#include "SpeciesNaming.h"
#include "SpeciesNameGenerator.h"
#include "Genome.h"
#include "CreatureType.h"
#include <iostream>
#include <unordered_set>
#include <vector>
#include <iomanip>

namespace naming {

// Test helper to generate names for all creature types
struct NamingCoverageReport {
    struct TypeReport {
        CreatureType type;
        std::string typeName;
        int nameCount = 0;
        int emptyNames = 0;
        int uniqueNames = 0;
        float avgNameLength = 0.0f;
        std::vector<std::string> sampleNames;
        std::unordered_set<std::string> allNames;
    };

    std::vector<TypeReport> reports;
    int totalNames = 0;
    int totalEmpty = 0;
    int totalUnique = 0;
    float overallCollisionRate = 0.0f;
};

// Generate test genome with specific traits
Genome generateTestGenome(float size, float speed, const glm::vec3& color,
                          bool hasWings = false, bool hasFins = false) {
    Genome genome;
    genome.size = size;
    genome.speed = speed;
    genome.color = color;
    genome.visionRange = 30.0f + size * 10.0f;
    genome.metabolicRate = 0.8f + size * 0.2f;
    genome.camouflageLevel = 0.3f;
    genome.reproductionThreshold = 80.0f;
    genome.mutationRate = 0.05f;

    // Set morphology based on type hints
    if (hasWings) {
        // Flying creature traits
        genome.glideRatio = 0.7f;
    }
    if (hasFins) {
        // Aquatic creature traits
        genome.preferredDepth = 0.5f;
    }

    return genome;
}

// Test all 18 creature types with multiple variations
NamingCoverageReport testAllCreatureTypes(int namesPerType = 20) {
    NamingCoverageReport report;

    // All 18 creature types from CreatureType.h
    std::vector<CreatureType> allTypes = {
        // Herbivores
        CreatureType::GRAZER,
        CreatureType::BROWSER,
        CreatureType::FRUGIVORE,

        // Predators
        CreatureType::SMALL_PREDATOR,
        CreatureType::OMNIVORE,
        CreatureType::APEX_PREDATOR,

        // Special types
        CreatureType::SCAVENGER,
        CreatureType::PARASITE,
        CreatureType::CLEANER,

        // Flying types
        CreatureType::FLYING,
        CreatureType::FLYING_BIRD,
        CreatureType::FLYING_INSECT,
        CreatureType::AERIAL_PREDATOR,

        // Aquatic types
        CreatureType::AQUATIC,
        CreatureType::AQUATIC_HERBIVORE,
        CreatureType::AQUATIC_PREDATOR,
        CreatureType::AQUATIC_APEX,
        CreatureType::AMPHIBIAN
    };

    auto& nameGen = getNameGenerator();
    auto& namingSystem = getNamingSystem();

    // Test each type
    for (CreatureType type : allTypes) {
        NamingCoverageReport::TypeReport typeReport;
        typeReport.type = type;
        typeReport.typeName = getCreatureTypeName(type);

        // Generate multiple names for this type
        for (int i = 0; i < namesPerType; ++i) {
            // Vary the genome parameters to get different names
            float sizeVariation = 0.5f + (i % 10) * 0.15f;
            float speedVariation = 8.0f + (i % 8) * 2.0f;
            glm::vec3 colorVariation(
                (i % 3) * 0.3f + 0.2f,
                ((i + 1) % 3) * 0.3f + 0.2f,
                ((i + 2) % 3) * 0.3f + 0.2f
            );

            // Generate genome appropriate for type
            bool hasWings = isFlying(type);
            bool hasFins = isAquatic(type);
            Genome genome = generateTestGenome(sizeVariation, speedVariation, colorVariation, hasWings, hasFins);

            // Test both naming systems
            uint32_t seed = static_cast<uint32_t>(i * 1000 + static_cast<int>(type) * 100);

            // Test SpeciesNameGenerator
            std::string nameGen1 = nameGen.generateNameWithSeed(genome, type, seed);

            // Test SpeciesNamingSystem (phoneme-based)
            CreatureTraits traits;
            traits.primaryColor = colorVariation;
            traits.size = sizeVariation;
            traits.speed = speedVariation;
            traits.hasWings = hasWings;
            traits.hasFins = hasFins;
            traits.livesInWater = hasFins;
            traits.canFly = hasWings;
            traits.isPredator = isPredator(type);
            traits.isCarnivore = isPredator(type) && !isHerbivore(type);
            traits.isHerbivore = isHerbivore(type);

            genetics::SpeciesId speciesId = seed;
            const auto& speciesName = namingSystem.getOrCreateSpeciesNameDeterministic(
                speciesId, traits, seed % 100, PhonemeTableType::LUSH);

            // Track names
            typeReport.nameCount++;

            if (nameGen1.empty() || speciesName.commonName.empty()) {
                typeReport.emptyNames++;
            }

            // Store both names
            typeReport.allNames.insert(nameGen1);
            typeReport.allNames.insert(speciesName.commonName);

            // Save first 5 as samples
            if (typeReport.sampleNames.size() < 5) {
                typeReport.sampleNames.push_back(nameGen1 + " / " + speciesName.commonName);
            }
        }

        // Calculate statistics
        typeReport.uniqueNames = typeReport.allNames.size();

        float totalLen = 0.0f;
        for (const auto& name : typeReport.allNames) {
            totalLen += name.length();
        }
        typeReport.avgNameLength = typeReport.allNames.empty() ? 0.0f :
            totalLen / typeReport.allNames.size();

        report.reports.push_back(typeReport);
        report.totalNames += typeReport.nameCount * 2; // Two systems tested
        report.totalEmpty += typeReport.emptyNames;
        report.totalUnique += typeReport.uniqueNames;
    }

    // Calculate overall collision rate
    float expectedUnique = static_cast<float>(report.totalNames);
    float actualUnique = static_cast<float>(report.totalUnique);
    report.overallCollisionRate = expectedUnique > 0 ?
        (1.0f - actualUnique / expectedUnique) * 100.0f : 0.0f;

    return report;
}

// Print detailed coverage report
void printCoverageReport(const NamingCoverageReport& report) {
    std::cout << "\n===========================================\n";
    std::cout << "NAMING SYSTEM COVERAGE REPORT\n";
    std::cout << "===========================================\n\n";

    std::cout << "Testing " << report.reports.size() << " creature types\n";
    std::cout << "Total names generated: " << report.totalNames << "\n";
    std::cout << "Total empty names: " << report.totalEmpty << "\n";
    std::cout << "Total unique names: " << report.totalUnique << "\n";
    std::cout << "Overall collision rate: " << std::fixed << std::setprecision(2)
              << report.overallCollisionRate << "%\n\n";

    // Print per-type breakdown
    std::cout << std::setw(20) << std::left << "TYPE"
              << std::setw(10) << "COUNT"
              << std::setw(10) << "UNIQUE"
              << std::setw(10) << "EMPTY"
              << std::setw(10) << "AVG LEN"
              << "SAMPLES\n";
    std::cout << std::string(100, '-') << "\n";

    for (const auto& typeReport : report.reports) {
        std::cout << std::setw(20) << std::left << typeReport.typeName
                  << std::setw(10) << typeReport.nameCount * 2
                  << std::setw(10) << typeReport.uniqueNames
                  << std::setw(10) << typeReport.emptyNames
                  << std::setw(10) << std::fixed << std::setprecision(1) << typeReport.avgNameLength;

        // Print first sample
        if (!typeReport.sampleNames.empty()) {
            std::cout << typeReport.sampleNames[0];
        }
        std::cout << "\n";

        // Print additional samples indented
        for (size_t i = 1; i < typeReport.sampleNames.size(); ++i) {
            std::cout << std::string(60, ' ') << typeReport.sampleNames[i] << "\n";
        }
    }

    std::cout << "\n===========================================\n";

    // Flag any issues
    if (report.totalEmpty > 0) {
        std::cout << "WARNING: " << report.totalEmpty << " empty names detected!\n";
    }

    if (report.overallCollisionRate > 30.0f) {
        std::cout << "WARNING: High collision rate (" << std::fixed << std::setprecision(1)
                  << report.overallCollisionRate << "%)! Consider expanding name pools.\n";
    }

    std::cout << "===========================================\n\n";
}

// Focused test for special types (SCAVENGER, PARASITE, CLEANER)
void testSpecialTypes() {
    std::cout << "\n===========================================\n";
    std::cout << "SPECIAL TYPES FOCUSED TEST\n";
    std::cout << "===========================================\n\n";

    std::vector<CreatureType> specialTypes = {
        CreatureType::SCAVENGER,
        CreatureType::PARASITE,
        CreatureType::CLEANER
    };

    auto& nameGen = getNameGenerator();

    for (CreatureType type : specialTypes) {
        std::cout << "\nType: " << getCreatureTypeName(type) << "\n";
        std::cout << std::string(40, '-') << "\n";

        for (int i = 0; i < 10; ++i) {
            Genome genome = generateTestGenome(0.5f + i * 0.1f, 10.0f + i,
                glm::vec3(i * 0.1f, 0.5f, 1.0f - i * 0.1f));

            uint32_t seed = i * 100 + static_cast<int>(type);
            std::string name = nameGen.generateNameWithSeed(genome, type, seed);

            std::cout << "  " << std::setw(3) << i << ": " << name;
            if (name.empty()) {
                std::cout << " [EMPTY!]";
            }
            std::cout << "\n";
        }
    }

    std::cout << "\n===========================================\n\n";
}

} // namespace naming

// Entry point for standalone test
int main() {
    std::cout << "Testing naming system coverage...\n";

    // Run comprehensive coverage test
    auto report = naming::testAllCreatureTypes(20);
    naming::printCoverageReport(report);

    // Run focused test on special types
    naming::testSpecialTypes();

    // Print final verdict
    if (report.totalEmpty == 0 && report.overallCollisionRate < 25.0f) {
        std::cout << "\n✓ PASS: All creature types have deterministic naming coverage!\n";
        return 0;
    } else {
        std::cout << "\n✗ FAIL: Naming coverage issues detected!\n";
        return 1;
    }
}
