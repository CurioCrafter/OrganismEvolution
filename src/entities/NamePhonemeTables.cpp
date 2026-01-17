#include "NamePhonemeTables.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <functional>

namespace naming {

// Global instance
static std::unique_ptr<NamePhonemeTables> g_phonemeTables;

NamePhonemeTables& getPhonemeTables() {
    if (!g_phonemeTables) {
        g_phonemeTables = std::make_unique<NamePhonemeTables>();
    }
    return *g_phonemeTables;
}

// =============================================================================
// PhonemeTableSet implementation
// =============================================================================

float PhonemeTableSet::getTotalWeight(const std::vector<WeightedSyllable>& syllables) {
    float total = 0.0f;
    for (const auto& s : syllables) {
        total += s.weight;
    }
    return total;
}

std::string PhonemeTableSet::selectSyllable(SyllablePosition pos, std::mt19937& rng) const {
    const std::vector<WeightedSyllable>* syllables = nullptr;

    switch (pos) {
        case SyllablePosition::PREFIX:
            syllables = &prefixes;
            break;
        case SyllablePosition::MIDDLE:
            syllables = &middles;
            break;
        case SyllablePosition::SUFFIX:
            syllables = &suffixes;
            break;
    }

    if (!syllables || syllables->empty()) {
        return "";
    }

    float totalWeight = getTotalWeight(*syllables);
    std::uniform_real_distribution<float> dist(0.0f, totalWeight);
    float target = dist(rng);

    float cumulative = 0.0f;
    for (const auto& s : *syllables) {
        cumulative += s.weight;
        if (target <= cumulative) {
            return s.syllable;
        }
    }

    return syllables->back().syllable;
}

// =============================================================================
// NamePhonemeTables implementation
// =============================================================================

NamePhonemeTables::NamePhonemeTables() {
    initializeDryTable();
    initializeLushTable();
    initializeOceanicTable();
    initializeFrozenTable();
    initializeVolcanicTable();
    initializeAlienTable();
}

uint32_t NamePhonemeTables::computeNameSeed(uint32_t planetSeed, uint32_t speciesId, PhonemeTableType tableType) {
    // Combine seeds using hash mixing
    uint32_t seed = planetSeed;
    seed ^= speciesId + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= static_cast<uint32_t>(tableType) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}

std::string NamePhonemeTables::generateName(PhonemeTableType tableType, uint32_t seed,
                                             int minSyllables, int maxSyllables) const {
    auto it = m_tables.find(tableType);
    if (it == m_tables.end()) {
        return "Unknown";
    }

    const PhonemeTableSet& table = it->second;
    std::mt19937 rng(seed);

    // Determine syllable count
    std::uniform_int_distribution<int> syllableDist(minSyllables, maxSyllables);
    int syllableCount = syllableDist(rng);

    std::string name;

    // Always start with a prefix
    name += table.selectSyllable(SyllablePosition::PREFIX, rng);

    // Add middle syllables if needed
    for (int i = 1; i < syllableCount - 1; ++i) {
        name += table.selectSyllable(SyllablePosition::MIDDLE, rng);
    }

    // End with suffix if more than 1 syllable
    if (syllableCount > 1) {
        name += table.selectSyllable(SyllablePosition::SUFFIX, rng);
    }

    // Capitalize first letter
    if (!name.empty()) {
        name[0] = static_cast<char>(std::toupper(name[0]));
    }

    return name;
}

CollisionResolution NamePhonemeTables::generateUniqueName(
    PhonemeTableType tableType, uint32_t seed,
    const std::unordered_set<std::string>& existingNames,
    int minSyllables, int maxSyllables) const
{
    CollisionResolution result;
    result.transformsApplied = 0;
    result.wasCollision = false;

    std::string baseName = generateName(tableType, seed, minSyllables, maxSyllables);
    result.resolvedName = baseName;

    // Check for collision
    if (existingNames.find(baseName) == existingNames.end()) {
        return result;
    }

    result.wasCollision = true;

    // Apply transforms in order until unique
    for (int transform = 0; transform < 10; ++transform) {
        std::string transformed = applyCollisionTransform(baseName, transform, tableType, seed);
        result.transformsApplied = transform + 1;

        if (existingNames.find(transformed) == existingNames.end()) {
            result.resolvedName = transformed;
            return result;
        }
    }

    // Fallback: append numeric ID
    result.resolvedName = baseName + "-" + std::to_string(seed % 10000);
    result.transformsApplied = 11;
    return result;
}

std::string NamePhonemeTables::applyCollisionTransform(
    const std::string& name, int transformIndex,
    PhonemeTableType tableType, uint32_t seed) const
{
    auto it = m_tables.find(tableType);
    if (it == m_tables.end()) {
        return name + std::to_string(transformIndex);
    }

    const PhonemeTableSet& table = it->second;
    std::mt19937 rng(seed + transformIndex * 12345);

    switch (transformIndex) {
        case 0: {
            // Transform 1: Swap last syllable
            if (name.length() > 3) {
                std::string swapped = name.substr(0, name.length() - 2);
                swapped += table.selectSyllable(SyllablePosition::SUFFIX, rng);
                if (!swapped.empty()) {
                    swapped[0] = static_cast<char>(std::toupper(swapped[0]));
                }
                return swapped;
            }
            break;
        }
        case 1:
        case 2: {
            // Transform 2-3: Inject connector (apostrophe or hyphen)
            if (name.length() > 4 && !table.connectors.empty()) {
                size_t insertPos = name.length() / 2;
                std::uniform_int_distribution<size_t> connDist(0, table.connectors.size() - 1);
                std::string connector = table.connectors[connDist(rng)];
                return name.substr(0, insertPos) + connector + name.substr(insertPos);
            }
            break;
        }
        case 3:
        case 4:
        case 5: {
            // Transform 4-6: Add rare suffix
            if (!table.rareSuffixes.empty()) {
                std::uniform_int_distribution<size_t> rareDist(0, table.rareSuffixes.size() - 1);
                return name + table.rareSuffixes[rareDist(rng)];
            }
            break;
        }
        default: {
            // Transform 7+: Append roman numeral
            return name + " " + getRomanNumeral(transformIndex - 5);
        }
    }

    // Fallback to roman numeral
    return name + " " + getRomanNumeral(transformIndex + 1);
}

std::string NamePhonemeTables::getRomanNumeral(int number) {
    if (number <= 0 || number > 20) {
        return std::to_string(number);
    }

    static const std::vector<std::string> numerals = {
        "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X",
        "XI", "XII", "XIII", "XIV", "XV", "XVI", "XVII", "XVIII", "XIX", "XX"
    };

    return numerals[number - 1];
}

const PhonemeTableSet& NamePhonemeTables::getTable(PhonemeTableType type) const {
    static PhonemeTableSet emptyTable;
    auto it = m_tables.find(type);
    if (it != m_tables.end()) {
        return it->second;
    }
    return emptyTable;
}

PhonemeTableType NamePhonemeTables::biomeToTableType(const std::string& biome) {
    // Map biome names to table types
    static const std::unordered_map<std::string, PhonemeTableType> biomeMap = {
        {"desert", PhonemeTableType::DRY},
        {"arid", PhonemeTableType::DRY},
        {"savanna", PhonemeTableType::DRY},
        {"grassland", PhonemeTableType::DRY},
        {"mesa", PhonemeTableType::DRY},
        {"forest", PhonemeTableType::LUSH},
        {"jungle", PhonemeTableType::LUSH},
        {"rainforest", PhonemeTableType::LUSH},
        {"swamp", PhonemeTableType::LUSH},
        {"wetland", PhonemeTableType::LUSH},
        {"ocean", PhonemeTableType::OCEANIC},
        {"coastal", PhonemeTableType::OCEANIC},
        {"reef", PhonemeTableType::OCEANIC},
        {"lake", PhonemeTableType::OCEANIC},
        {"river", PhonemeTableType::OCEANIC},
        {"tundra", PhonemeTableType::FROZEN},
        {"ice", PhonemeTableType::FROZEN},
        {"glacier", PhonemeTableType::FROZEN},
        {"arctic", PhonemeTableType::FROZEN},
        {"boreal", PhonemeTableType::FROZEN},
        {"volcanic", PhonemeTableType::VOLCANIC},
        {"lava", PhonemeTableType::VOLCANIC},
        {"magma", PhonemeTableType::VOLCANIC},
        {"alien", PhonemeTableType::ALIEN},
        {"exotic", PhonemeTableType::ALIEN},
        {"crystal", PhonemeTableType::ALIEN},
        {"bioluminescent", PhonemeTableType::ALIEN}
    };

    std::string lowerBiome = biome;
    std::transform(lowerBiome.begin(), lowerBiome.end(), lowerBiome.begin(), ::tolower);

    auto it = biomeMap.find(lowerBiome);
    if (it != biomeMap.end()) {
        return it->second;
    }

    // Default to lush for unknown biomes
    return PhonemeTableType::LUSH;
}

void NamePhonemeTables::logTableStats() const {
    std::cout << "=== Phoneme Tables Statistics ===" << std::endl;
    for (const auto& [type, table] : m_tables) {
        std::cout << "Table " << static_cast<int>(type) << ": "
                  << table.prefixes.size() << " prefixes, "
                  << table.middles.size() << " middles, "
                  << table.suffixes.size() << " suffixes, "
                  << table.connectors.size() << " connectors, "
                  << table.rareSuffixes.size() << " rare suffixes" << std::endl;
    }
}

int NamePhonemeTables::validateTables() const {
    int errors = 0;
    for (const auto& [type, table] : m_tables) {
        if (table.prefixes.empty()) {
            std::cerr << "ERROR: Table " << static_cast<int>(type) << " has no prefixes!" << std::endl;
            errors++;
        }
        if (table.suffixes.empty()) {
            std::cerr << "ERROR: Table " << static_cast<int>(type) << " has no suffixes!" << std::endl;
            errors++;
        }
    }
    return errors;
}

std::string NamePhonemeTables::selectWeighted(const std::vector<WeightedSyllable>& syllables,
                                               std::mt19937& rng) {
    if (syllables.empty()) return "";

    float totalWeight = PhonemeTableSet::getTotalWeight(syllables);
    std::uniform_real_distribution<float> dist(0.0f, totalWeight);
    float target = dist(rng);

    float cumulative = 0.0f;
    for (const auto& s : syllables) {
        cumulative += s.weight;
        if (target <= cumulative) {
            return s.syllable;
        }
    }

    return syllables.back().syllable;
}

// =============================================================================
// Table Initialization - DRY (Desert, Arid)
// =============================================================================

void NamePhonemeTables::initializeDryTable() {
    PhonemeTableSet table;
    table.type = PhonemeTableType::DRY;

    // Prefixes - harsh consonants, short sounds
    table.prefixes = {
        {"Ka", 1.5f}, {"Kra", 1.2f}, {"Tar", 1.5f}, {"Zar", 1.0f},
        {"Rak", 1.3f}, {"Drak", 1.0f}, {"Sha", 1.4f}, {"Ska", 1.1f},
        {"Gra", 1.2f}, {"Khor", 1.0f}, {"Sar", 1.3f}, {"Bar", 1.2f},
        {"Jax", 0.8f}, {"Thrax", 0.7f}, {"Vex", 0.9f}, {"Dax", 1.0f},
        {"Zek", 0.9f}, {"Kez", 0.8f}, {"Rah", 1.1f}, {"Nah", 1.0f}
    };

    // Middles - transitional sounds
    table.middles = {
        {"ar", 1.5f}, {"ak", 1.3f}, {"ek", 1.2f}, {"or", 1.0f},
        {"ir", 1.1f}, {"az", 1.0f}, {"ox", 0.8f}, {"ax", 0.9f},
        {"ra", 1.2f}, {"za", 1.0f}, {"ka", 1.1f}, {"ta", 1.0f}
    };

    // Suffixes - strong endings
    table.suffixes = {
        {"ax", 1.5f}, {"ek", 1.3f}, {"uk", 1.2f}, {"os", 1.1f},
        {"us", 1.4f}, {"is", 1.2f}, {"ar", 1.3f}, {"or", 1.0f},
        {"ix", 1.1f}, {"ak", 1.2f}, {"az", 1.0f}, {"oz", 0.9f},
        {"rath", 0.8f}, {"kan", 1.0f}, {"dar", 0.9f}, {"zan", 0.8f}
    };

    table.connectors = {"'", "-"};
    table.rareSuffixes = {"-akh", "-zar", "-kha", "'dar", "-rex"};

    m_tables[PhonemeTableType::DRY] = std::move(table);
}

// =============================================================================
// Table Initialization - LUSH (Forest, Jungle)
// =============================================================================

void NamePhonemeTables::initializeLushTable() {
    PhonemeTableSet table;
    table.type = PhonemeTableType::LUSH;

    // Prefixes - soft, flowing sounds
    table.prefixes = {
        {"Syl", 1.5f}, {"Fen", 1.4f}, {"Wil", 1.3f}, {"Ael", 1.2f},
        {"Lor", 1.4f}, {"Mel", 1.3f}, {"Vir", 1.2f}, {"Fae", 1.1f},
        {"Elm", 1.2f}, {"Ash", 1.3f}, {"Ivy", 1.1f}, {"Wyn", 1.0f},
        {"Lin", 1.2f}, {"Bri", 1.1f}, {"Flo", 1.0f}, {"Gal", 0.9f},
        {"Tha", 1.1f}, {"Lea", 1.2f}, {"Myr", 1.0f}, {"Ner", 0.9f}
    };

    // Middles - melodic transitions
    table.middles = {
        {"an", 1.5f}, {"el", 1.4f}, {"ia", 1.3f}, {"or", 1.2f},
        {"al", 1.3f}, {"en", 1.2f}, {"il", 1.1f}, {"ea", 1.0f},
        {"ae", 1.1f}, {"yl", 1.0f}, {"ar", 0.9f}, {"ir", 0.8f}
    };

    // Suffixes - gentle endings
    table.suffixes = {
        {"ia", 1.5f}, {"on", 1.4f}, {"en", 1.3f}, {"is", 1.2f},
        {"ara", 1.3f}, {"iel", 1.2f}, {"wyn", 1.1f}, {"ath", 1.0f},
        {"or", 1.1f}, {"il", 1.0f}, {"ae", 0.9f}, {"a", 1.2f},
        {"ris", 0.9f}, {"las", 0.8f}, {"nis", 0.8f}, {"ven", 0.9f}
    };

    table.connectors = {"'", "-", "ae"};
    table.rareSuffixes = {"-bloom", "-leaf", "-whisper", "'shade", "-dell"};

    m_tables[PhonemeTableType::LUSH] = std::move(table);
}

// =============================================================================
// Table Initialization - OCEANIC (Coastal, Deep Sea)
// =============================================================================

void NamePhonemeTables::initializeOceanicTable() {
    PhonemeTableSet table;
    table.type = PhonemeTableType::OCEANIC;

    // Prefixes - liquid, flowing sounds
    table.prefixes = {
        {"Mer", 1.5f}, {"Nal", 1.4f}, {"Kal", 1.3f}, {"Pel", 1.2f},
        {"Thal", 1.4f}, {"Ner", 1.3f}, {"Cor", 1.2f}, {"Del", 1.1f},
        {"Mar", 1.3f}, {"Ael", 1.2f}, {"Bry", 1.1f}, {"Wav", 1.0f},
        {"Rip", 1.0f}, {"Tid", 1.1f}, {"Cur", 1.0f}, {"Sal", 0.9f},
        {"Nau", 1.1f}, {"Oce", 1.0f}, {"Aqu", 0.9f}, {"Sel", 0.8f}
    };

    // Middles - rolling transitions
    table.middles = {
        {"al", 1.5f}, {"el", 1.4f}, {"an", 1.3f}, {"on", 1.2f},
        {"er", 1.2f}, {"ar", 1.1f}, {"or", 1.0f}, {"ul", 0.9f},
        {"ae", 1.1f}, {"il", 1.0f}, {"en", 0.9f}, {"in", 0.8f}
    };

    // Suffixes - wave-like endings
    table.suffixes = {
        {"aris", 1.5f}, {"on", 1.4f}, {"us", 1.3f}, {"a", 1.2f},
        {"eos", 1.3f}, {"ene", 1.2f}, {"yl", 1.1f}, {"al", 1.0f},
        {"is", 1.2f}, {"os", 1.1f}, {"an", 1.0f}, {"or", 0.9f},
        {"tide", 0.8f}, {"wave", 0.7f}, {"fin", 0.8f}, {"kel", 0.7f}
    };

    table.connectors = {"'", "-", "o"};
    table.rareSuffixes = {"-tide", "-wave", "-fin", "'deep", "-coral"};

    m_tables[PhonemeTableType::OCEANIC] = std::move(table);
}

// =============================================================================
// Table Initialization - FROZEN (Tundra, Ice)
// =============================================================================

void NamePhonemeTables::initializeFrozenTable() {
    PhonemeTableSet table;
    table.type = PhonemeTableType::FROZEN;

    // Prefixes - crisp, cold sounds
    table.prefixes = {
        {"Fro", 1.5f}, {"Kri", 1.4f}, {"Gla", 1.3f}, {"Bor", 1.2f},
        {"Nor", 1.4f}, {"Sno", 1.3f}, {"Ice", 1.2f}, {"Vin", 1.1f},
        {"Win", 1.3f}, {"Fri", 1.2f}, {"Kel", 1.1f}, {"Yal", 1.0f},
        {"Hri", 1.0f}, {"Ski", 1.1f}, {"Tun", 1.0f}, {"Ark", 0.9f},
        {"Pol", 1.0f}, {"Cry", 0.9f}, {"Hail", 0.8f}, {"Rime", 0.8f}
    };

    // Middles - sharp transitions
    table.middles = {
        {"ir", 1.5f}, {"or", 1.4f}, {"al", 1.3f}, {"el", 1.2f},
        {"ik", 1.2f}, {"ok", 1.1f}, {"ar", 1.0f}, {"er", 0.9f},
        {"in", 1.1f}, {"on", 1.0f}, {"il", 0.9f}, {"ol", 0.8f}
    };

    // Suffixes - icy endings
    table.suffixes = {
        {"ir", 1.5f}, {"or", 1.4f}, {"en", 1.3f}, {"in", 1.2f},
        {"ik", 1.3f}, {"ok", 1.2f}, {"ar", 1.1f}, {"er", 1.0f},
        {"is", 1.2f}, {"os", 1.1f}, {"ax", 1.0f}, {"ex", 0.9f},
        {"frost", 0.7f}, {"ice", 0.6f}, {"rim", 0.7f}, {"keld", 0.6f}
    };

    table.connectors = {"'", "-"};
    table.rareSuffixes = {"-frost", "-ice", "-keld", "'rim", "-berg"};

    m_tables[PhonemeTableType::FROZEN] = std::move(table);
}

// =============================================================================
// Table Initialization - VOLCANIC (Volcanic, Fire)
// =============================================================================

void NamePhonemeTables::initializeVolcanicTable() {
    PhonemeTableSet table;
    table.type = PhonemeTableType::VOLCANIC;

    // Prefixes - explosive, fiery sounds
    table.prefixes = {
        {"Pyr", 1.5f}, {"Mag", 1.4f}, {"Vol", 1.3f}, {"Bla", 1.2f},
        {"Ash", 1.4f}, {"Emb", 1.3f}, {"Cin", 1.2f}, {"Sco", 1.1f},
        {"Mol", 1.2f}, {"Sul", 1.1f}, {"Ign", 1.0f}, {"Cal", 0.9f},
        {"Fla", 1.1f}, {"Bur", 1.0f}, {"Sear", 0.9f}, {"Char", 0.8f},
        {"Kra", 1.1f}, {"Vul", 1.0f}, {"Fer", 0.9f}, {"Tor", 0.8f}
    };

    // Middles - crackling transitions
    table.middles = {
        {"ar", 1.5f}, {"or", 1.4f}, {"ur", 1.3f}, {"er", 1.2f},
        {"ax", 1.2f}, {"ox", 1.1f}, {"ix", 1.0f}, {"ex", 0.9f},
        {"ra", 1.1f}, {"ro", 1.0f}, {"ru", 0.9f}, {"re", 0.8f}
    };

    // Suffixes - fiery endings
    table.suffixes = {
        {"or", 1.5f}, {"ax", 1.4f}, {"us", 1.3f}, {"os", 1.2f},
        {"ix", 1.3f}, {"ex", 1.2f}, {"ar", 1.1f}, {"ur", 1.0f},
        {"an", 1.1f}, {"on", 1.0f}, {"is", 0.9f}, {"as", 0.8f},
        {"burn", 0.7f}, {"cind", 0.6f}, {"ite", 0.8f}, {"ite", 0.7f}
    };

    table.connectors = {"'", "-"};
    table.rareSuffixes = {"-burn", "-cinder", "-ash", "'flame", "-scorch"};

    m_tables[PhonemeTableType::VOLCANIC] = std::move(table);
}

// =============================================================================
// Table Initialization - ALIEN (Exotic, Otherworldly)
// =============================================================================

void NamePhonemeTables::initializeAlienTable() {
    PhonemeTableSet table;
    table.type = PhonemeTableType::ALIEN;

    // Prefixes - unusual, exotic sounds
    table.prefixes = {
        {"Xyl", 1.5f}, {"Zyx", 1.4f}, {"Qua", 1.3f}, {"Vex", 1.2f},
        {"Nyx", 1.4f}, {"Pho", 1.3f}, {"Xen", 1.2f}, {"Kry", 1.1f},
        {"Zyg", 1.2f}, {"Psi", 1.1f}, {"Omi", 1.0f}, {"Chi", 0.9f},
        {"Aeth", 1.1f}, {"Neo", 1.0f}, {"Lux", 0.9f}, {"Orb", 0.8f},
        {"Voi", 1.0f}, {"Abs", 0.9f}, {"Nth", 0.7f}, {"Qel", 0.8f}
    };

    // Middles - alien transitions
    table.middles = {
        {"yx", 1.5f}, {"ex", 1.4f}, {"on", 1.3f}, {"ar", 1.2f},
        {"ax", 1.2f}, {"or", 1.1f}, {"ix", 1.0f}, {"us", 0.9f},
        {"ae", 1.1f}, {"oi", 1.0f}, {"uu", 0.8f}, {"ii", 0.7f}
    };

    // Suffixes - otherworldly endings
    table.suffixes = {
        {"on", 1.5f}, {"yx", 1.4f}, {"ax", 1.3f}, {"ix", 1.2f},
        {"or", 1.3f}, {"ar", 1.2f}, {"is", 1.1f}, {"us", 1.0f},
        {"ae", 1.1f}, {"oi", 1.0f}, {"ex", 0.9f}, {"ox", 0.8f},
        {"prime", 0.6f}, {"void", 0.5f}, {"lux", 0.7f}, {"nex", 0.6f}
    };

    table.connectors = {"'", "-", "x", "z"};
    table.rareSuffixes = {"-prime", "-void", "-nexus", "'zenith", "-omega"};

    m_tables[PhonemeTableType::ALIEN] = std::move(table);
}

} // namespace naming
