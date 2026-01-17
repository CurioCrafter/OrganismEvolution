#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <memory>
#include <cstdint>
#include <glm/glm.hpp>
#include "genetics/DiploidGenome.h"
#include "NamePhonemeTables.h"

namespace naming {

// Forward declarations
struct SpeciesName;
struct IndividualName;
struct TraitDescriptor;

// Taxonomic hierarchy levels
enum class TaxonomicRank {
    Kingdom,    // e.g., Animalia
    Phylum,     // e.g., Chordata
    Class,      // e.g., Mammalia
    Order,      // e.g., Carnivora
    Family,     // e.g., Felidae
    Genus,      // e.g., Panthera
    Species     // e.g., leo
};

// Creature trait categories for name generation
struct CreatureTraits {
    // Physical traits
    glm::vec3 primaryColor{0.5f};
    float size = 1.0f;
    float speed = 10.0f;

    // Morphological traits
    int legCount = 4;
    bool hasWings = false;
    bool hasFins = false;
    bool hasHorns = false;
    bool hasCrest = false;
    bool hasTail = true;
    float tailLength = 1.0f;

    // Behavioral traits
    bool isPredator = false;
    bool isNocturnal = false;
    bool isSocial = false;
    bool isAquatic = false;

    // Diet
    bool isHerbivore = true;
    bool isCarnivore = false;
    bool isOmnivore = false;

    // Environment
    bool livesInWater = false;
    bool canFly = false;
    bool burrows = false;
    bool isArboreal = false;   // Lives in trees
    bool isSubterranean = false; // Lives underground
};

// Trait-based descriptor (diet + locomotion/habitat)
// Format: "diet, behavior" e.g., "carnivore, aquatic"
// NO generic labels like "apex predator" allowed
struct TraitDescriptor {
    std::string diet;          // carnivore, herbivore, omnivore, scavenger, filter-feeder
    std::string locomotion;    // aquatic, arboreal, burrowing, terrestrial, aerial, amphibious

    std::string getFullDescriptor() const {
        if (diet.empty() && locomotion.empty()) return "";
        if (diet.empty()) return locomotion;
        if (locomotion.empty()) return diet;
        return diet + ", " + locomotion;
    }
};

// Name display mode
enum class NameDisplayMode {
    COMMON_NAME,      // e.g., "Sylvoria"
    BINOMIAL,         // e.g., "Sylvor sylvensis"
    FULL_SCIENTIFIC   // e.g., "Sylvor sylvensis (Family Sylvoridae)"
};

// Debug statistics for naming system
struct NamingStats {
    size_t totalNamesGenerated = 0;
    size_t uniqueNames = 0;
    size_t collisions = 0;
    float averageNameLength = 0.0f;
    std::unordered_map<int, int> collisionsByTransform; // transform index -> count
};

// Species name structure
struct SpeciesName {
    // Common name (e.g., "Sylvoria" - phoneme-generated)
    std::string commonName;

    // Scientific name parts
    std::string genus;      // e.g., "Sylvor"
    std::string species;    // e.g., "sylvensis"

    // Full scientific name (e.g., "Sylvor sylvensis")
    std::string scientificName;

    // Taxonomic hierarchy
    std::string family;     // e.g., "Sylvoridae"
    std::string order;      // e.g., "Herbivora"

    // Trait-based descriptor (NO generic labels like "apex predator")
    TraitDescriptor descriptor;

    // Generation info
    uint32_t speciesId = 0;
    int originGeneration = 0;

    // Genus cluster ID (for binomial consistency)
    uint32_t genusClusterId = 0;

    // Display helpers
    std::string getDisplayName() const { return commonName; }
    std::string getFullScientificName() const { return genus + " " + species; }
    std::string getAbbreviatedScientific() const {
        return genus.empty() ? "" : (genus.substr(0, 1) + ". " + species);
    }
    std::string getDescriptor() const { return descriptor.getFullDescriptor(); }

    // Get display name with descriptor
    std::string getDisplayWithDescriptor() const {
        std::string desc = descriptor.getFullDescriptor();
        if (desc.empty()) return commonName;
        return commonName;  // Descriptor shown separately in UI
    }
};

// Individual creature name
struct IndividualName {
    std::string firstName;      // e.g., "Rex"
    std::string suffix;         // e.g., "Jr.", "III"
    std::string title;          // e.g., "the Swift", "the Hunter"

    int generation = 0;         // Which generation (for suffix calculation)
    int ancestorCount = 0;      // How many ancestors with same name

    // Parent lineage
    int parentId = -1;
    std::string parentName;

    std::string getDisplayName() const {
        std::string result = firstName;
        if (!suffix.empty()) result += " " + suffix;
        if (!title.empty()) result += " " + title;
        return result;
    }

    std::string getShortName() const {
        return firstName + (suffix.empty() ? "" : " " + suffix);
    }
};

// Combined creature identity
struct CreatureIdentity {
    IndividualName individualName;
    const SpeciesName* speciesName = nullptr;  // Pointer to cached species name

    int creatureId = 0;
    int generation = 0;

    std::string getFullIdentity() const {
        std::string result = individualName.getDisplayName();
        if (speciesName) {
            result += " (" + speciesName->commonName + ")";
        }
        return result;
    }
};

// Main naming system class
class SpeciesNamingSystem {
public:
    SpeciesNamingSystem();
    ~SpeciesNamingSystem() = default;

    // ========================================
    // PRIMARY API - Species naming
    // ========================================

    // Get or create a species name with phoneme-based generation
    const SpeciesName& getOrCreateSpeciesName(genetics::SpeciesId speciesId,
                                               const CreatureTraits& traits);

    // Get or create with planet seed for deterministic naming
    const SpeciesName& getOrCreateSpeciesNameDeterministic(
        genetics::SpeciesId speciesId,
        const CreatureTraits& traits,
        uint32_t planetSeed,
        PhonemeTableType biomeTable = PhonemeTableType::LUSH);

    const SpeciesName* getSpeciesName(genetics::SpeciesId speciesId) const;
    void updateSpeciesName(genetics::SpeciesId speciesId, const std::string& newName);

    // Individual naming
    IndividualName generateIndividualName(genetics::SpeciesId speciesId,
                                          int generation,
                                          int parentId = -1,
                                          const std::string& parentName = "");

    // Name evolution (on speciation events)
    SpeciesName evolveSpeciesName(genetics::SpeciesId parentSpeciesId,
                                  genetics::SpeciesId newSpeciesId,
                                  const CreatureTraits& newTraits);

    // ========================================
    // PLANET AND BIOME CONFIGURATION
    // ========================================

    // Set planet seed for deterministic naming
    void setPlanetSeed(uint32_t seed) { m_planetSeed = seed; }
    uint32_t getPlanetSeed() const { return m_planetSeed; }

    // Set default biome table type
    void setDefaultBiome(PhonemeTableType biome) { m_defaultBiome = biome; }
    PhonemeTableType getDefaultBiome() const { return m_defaultBiome; }

    // ========================================
    // TRAIT DESCRIPTOR GENERATION (NO GENERIC LABELS)
    // ========================================

    // Generate descriptor from traits (e.g., "carnivore, aquatic")
    TraitDescriptor generateDescriptor(const CreatureTraits& traits) const;

    // Get diet string (NO "apex", "predator" - use specific diet only)
    static std::string getDietString(const CreatureTraits& traits);

    // Get locomotion/habitat string
    static std::string getLocomotionString(const CreatureTraits& traits);

    // ========================================
    // BINOMIAL NAMING AND GENUS CLUSTERS
    // ========================================

    // Get or assign genus cluster ID (for similarity clustering)
    uint32_t getGenusClusterId(genetics::SpeciesId speciesId) const;

    // Set genus cluster from similarity system (Agent 2 integration)
    void setGenusCluster(genetics::SpeciesId speciesId, uint32_t clusterId);

    // Get shared genus name for a cluster
    std::string getGenusForCluster(uint32_t clusterId);

    // ========================================
    // DISPLAY MODE AND TOGGLES
    // ========================================

    void setDisplayMode(NameDisplayMode mode) { m_displayMode = mode; }
    NameDisplayMode getDisplayMode() const { return m_displayMode; }

    void setShowDescriptor(bool show) { m_showDescriptor = show; }
    bool getShowDescriptor() const { return m_showDescriptor; }

    // ========================================
    // UTILITY
    // ========================================

    void setSeed(uint32_t seed);
    void clear();
    size_t getSpeciesCount() const { return m_speciesNames.size(); }

    // Debug and validation
    const NamingStats& getStats() const { return m_stats; }
    void resetStats() { m_stats = NamingStats(); }
    void logStats() const;

    // Validate names: generate N names and report collision rate
    float validateNameGeneration(int count, uint32_t testSeed) const;

    // Export/import
    std::string exportToJson() const;
    void importFromJson(const std::string& json);

private:
    // ========================================
    // PHONEME-BASED NAME GENERATION (NEW)
    // ========================================

    // Generate common name using phoneme tables
    std::string generatePhonemeBasedName(genetics::SpeciesId speciesId,
                                          PhonemeTableType tableType) const;

    // Select appropriate phoneme table based on traits
    PhonemeTableType selectPhonemeTable(const CreatureTraits& traits) const;

    // Generate genus name (shared within clusters)
    std::string generateGenusName(uint32_t clusterId, PhonemeTableType tableType) const;

    // Generate species epithet (unique within genus)
    std::string generateSpeciesEpithetFromTraits(const CreatureTraits& traits,
                                                  const std::string& genus) const;

    // ========================================
    // LEGACY NAME GENERATION (kept for compatibility)
    // ========================================

    std::string generateCommonName(const CreatureTraits& traits);
    std::string generateGenus(const CreatureTraits& traits);
    std::string generateSpeciesEpithet(const CreatureTraits& traits);
    std::string generateFamily(const std::string& genus);
    std::string generateOrder(const CreatureTraits& traits);

    // Component generators
    std::string getColorDescriptor(const glm::vec3& color) const;
    std::string getSizeDescriptor(float size) const;
    std::string getSpeedDescriptor(float speed) const;
    std::string getMorphologyDescriptor(const CreatureTraits& traits) const;
    std::string getHabitatDescriptor(const CreatureTraits& traits) const;
    std::string getBehaviorDescriptor(const CreatureTraits& traits) const;

    // Latin-style name generation
    std::string latinize(const std::string& word) const;
    std::string generateLatinRoot() const;

    // Individual name generation
    std::string generateFirstName(bool isPredator) const;
    std::string calculateSuffix(int ancestorCount) const;
    std::string generateTitle(const CreatureTraits& traits) const;

    // ========================================
    // DATA MEMBERS
    // ========================================

    // Primary data storage
    std::unordered_map<genetics::SpeciesId, SpeciesName> m_speciesNames;
    std::unordered_map<std::string, int> m_nameUsageCount;  // Track name frequency

    // Uniqueness tracking per planet
    std::unordered_set<std::string> m_usedNames;

    // Genus cluster mappings
    std::unordered_map<genetics::SpeciesId, uint32_t> m_speciesGenusCluster;
    std::unordered_map<uint32_t, std::string> m_clusterGenusNames;

    // Configuration
    uint32_t m_planetSeed = 42;
    PhonemeTableType m_defaultBiome = PhonemeTableType::LUSH;
    NameDisplayMode m_displayMode = NameDisplayMode::COMMON_NAME;
    bool m_showDescriptor = true;
    bool m_usePhonemeNaming = true;  // Use new phoneme system vs legacy

    // Statistics
    mutable NamingStats m_stats;

    // Random generation
    mutable std::mt19937 m_rng;

    // Name component lists (loaded at construction)
    std::vector<std::string> m_colorPrefixes;
    std::vector<std::string> m_sizePrefixes;
    std::vector<std::string> m_speedPrefixes;
    std::vector<std::string> m_morphPrefixes;
    std::vector<std::string> m_habitatSuffixes;
    std::vector<std::string> m_behaviorSuffixes;
    std::vector<std::string> m_latinRoots;
    std::vector<std::string> m_latinSuffixes;
    std::vector<std::string> m_maleNames;
    std::vector<std::string> m_femaleNames;
    std::vector<std::string> m_neutralNames;
    std::vector<std::string> m_titles;

    // Archetype-based naming for visual diversity (Task 1)
    // Prefixes based on creature archetype/environment
    std::vector<std::string> m_archetypePrefixes;  // swift/shadow/moss/coral/dawn/frost/ember/thorn/reef/sky
    // Suffixes based on locomotion style
    std::vector<std::string> m_locomotionSuffixes; // stalker/glider/swimmer/crawler/hopper/diver/soarer/runner
    // Species words (animal-inspired base names)
    std::vector<std::string> m_speciesWords;       // finch/manta/beetle/newt/gecko/heron/otter/shrike/pike/moth

    void initializeNameComponents();
    void initializeArchetypeComponents();

    // Enhanced name generation using archetype system
    std::string generateArchetypeName(const CreatureTraits& traits) const;
    std::string selectArchetypePrefix(const CreatureTraits& traits) const;
    std::string selectLocomotionSuffix(const CreatureTraits& traits) const;
    std::string selectSpeciesWord(const CreatureTraits& traits) const;
};

// Global naming system instance
SpeciesNamingSystem& getNamingSystem();

} // namespace naming
