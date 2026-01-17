#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <random>

namespace naming {

// Biome-themed phoneme table types
enum class PhonemeTableType {
    DRY,        // Desert, arid - harsh consonants, short vowels
    LUSH,       // Forest, jungle - soft sounds, flowing syllables
    OCEANIC,    // Coastal, deep sea - liquid sounds, rolling names
    FROZEN,     // Tundra, ice - crisp consonants, cold vowels
    VOLCANIC,   // Volcanic, fire - sharp sounds, explosive consonants
    ALIEN,      // Exotic, otherworldly - unusual combinations
    COUNT
};

// A weighted syllable entry
struct WeightedSyllable {
    std::string syllable;
    float weight;

    WeightedSyllable(const std::string& s, float w) : syllable(s), weight(w) {}
};

// Syllable position in name
enum class SyllablePosition {
    PREFIX,     // Start of name
    MIDDLE,     // Middle syllables
    SUFFIX      // End of name
};

// Collision resolution result
struct CollisionResolution {
    std::string resolvedName;
    int transformsApplied;
    bool wasCollision;
};

// Phoneme table set for a specific biome theme
struct PhonemeTableSet {
    PhonemeTableType type;
    std::vector<WeightedSyllable> prefixes;
    std::vector<WeightedSyllable> middles;
    std::vector<WeightedSyllable> suffixes;
    std::vector<std::string> connectors;     // Apostrophes, hyphens
    std::vector<std::string> rareSuffixes;   // For collision resolution

    // Compute total weight for a syllable list
    static float getTotalWeight(const std::vector<WeightedSyllable>& syllables);

    // Select a syllable using weighted random selection
    std::string selectSyllable(SyllablePosition pos, std::mt19937& rng) const;
};

// Main phoneme tables class
class NamePhonemeTables {
public:
    NamePhonemeTables();
    ~NamePhonemeTables() = default;

    // Generate a species name using phoneme tables
    // Deterministic: same seed produces same name
    std::string generateName(PhonemeTableType tableType, uint32_t seed, int minSyllables = 2, int maxSyllables = 3) const;

    // Generate a name with collision checking
    // Returns a unique name within the registered set
    CollisionResolution generateUniqueName(PhonemeTableType tableType, uint32_t seed,
                                           const std::unordered_set<std::string>& existingNames,
                                           int minSyllables = 2, int maxSyllables = 3) const;

    // Get table for a specific biome type
    const PhonemeTableSet& getTable(PhonemeTableType type) const;

    // Map biome string to table type
    static PhonemeTableType biomeToTableType(const std::string& biome);

    // Generate deterministic seed from planet seed and species ID
    static uint32_t computeNameSeed(uint32_t planetSeed, uint32_t speciesId, PhonemeTableType tableType);

    // Collision resolution transforms (applied in order)
    // 1) Swap last syllable
    // 2) Inject connector (apostrophe or hyphen)
    // 3) Add rare suffix
    // 4) Append roman numeral
    std::string applyCollisionTransform(const std::string& name, int transformIndex,
                                        PhonemeTableType tableType, uint32_t seed) const;

    // Get roman numeral string
    static std::string getRomanNumeral(int number);

    // Validation/debug
    void logTableStats() const;
    int validateTables() const;

private:
    std::unordered_map<PhonemeTableType, PhonemeTableSet> m_tables;

    void initializeDryTable();
    void initializeLushTable();
    void initializeOceanicTable();
    void initializeFrozenTable();
    void initializeVolcanicTable();
    void initializeAlienTable();

    // Helper to select from weighted list with seed
    static std::string selectWeighted(const std::vector<WeightedSyllable>& syllables,
                                      std::mt19937& rng);
};

// Global phoneme tables instance
NamePhonemeTables& getPhonemeTables();

} // namespace naming
