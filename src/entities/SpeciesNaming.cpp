#include "SpeciesNaming.h"
#include "NamePhonemeTables.h"
#include <algorithm>
#include <sstream>
#include <cmath>
#include <chrono>
#include <iostream>
#include <cassert>

namespace naming {

// Global instance
static std::unique_ptr<SpeciesNamingSystem> g_namingSystem;

SpeciesNamingSystem& getNamingSystem() {
    if (!g_namingSystem) {
        g_namingSystem = std::make_unique<SpeciesNamingSystem>();
    }
    return *g_namingSystem;
}

SpeciesNamingSystem::SpeciesNamingSystem() {
    // Seed with current time
    auto seed = static_cast<uint32_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    m_rng.seed(seed);
    m_planetSeed = seed;

    initializeNameComponents();
    initializeArchetypeComponents();
}

void SpeciesNamingSystem::initializeNameComponents() {
    // Color descriptors for common names
    m_colorPrefixes = {
        "Red", "Crimson", "Scarlet", "Ruby",
        "Orange", "Amber", "Copper", "Rust",
        "Yellow", "Golden", "Honey", "Lemon",
        "Green", "Emerald", "Jade", "Olive",
        "Blue", "Azure", "Cobalt", "Sapphire",
        "Purple", "Violet", "Amethyst", "Plum",
        "White", "Silver", "Ivory", "Pearl",
        "Black", "Ebony", "Onyx", "Shadow",
        "Brown", "Chestnut", "Mahogany", "Tawny",
        "Gray", "Slate", "Ash", "Storm"
    };

    // Size descriptors
    m_sizePrefixes = {
        "Giant", "Great", "Large", "Big",
        "Common", "Medium", "Standard",
        "Small", "Lesser", "Little", "Tiny",
        "Dwarf", "Pygmy", "Miniature"
    };

    // Speed descriptors
    m_speedPrefixes = {
        "Swift", "Fleet", "Quick", "Rapid",
        "Nimble", "Agile", "Darting",
        "Steady", "Measured", "Patient",
        "Slow", "Plodding", "Lumbering"
    };

    // Morphology descriptors
    m_morphPrefixes = {
        "Crested", "Horned", "Tusked", "Fanged",
        "Winged", "Finned", "Tailed", "Spiked",
        "Long-necked", "Short-legged", "Heavy-bodied",
        "Slender", "Broad", "Striped", "Spotted",
        "Armored", "Scaled", "Feathered", "Furred"
    };

    // Habitat suffixes (for common names)
    m_habitatSuffixes = {
        "Walker", "Runner", "Crawler", "Stalker",
        "Swimmer", "Diver", "Glider", "Flyer",
        "Climber", "Burrower", "Hopper", "Leaper",
        "Dweller", "Wanderer", "Roamer", "Tracker"
    };

    // Behavior suffixes
    m_behaviorSuffixes = {
        "Hunter", "Grazer", "Forager", "Scavenger",
        "Predator", "Browser", "Gatherer", "Stalker",
        "Ambusher", "Chaser", "Pouncer", "Striker"
    };

    // Latin roots for scientific names
    m_latinRoots = {
        "Veloc", "Rapid", "Celer",           // Speed
        "Magn", "Grand", "Major",            // Size (large)
        "Parv", "Minor", "Minim",            // Size (small)
        "Aqu", "Fluvi", "Marin",             // Water
        "Terr", "Silv", "Agr",               // Land/forest
        "Aer", "Vol", "Ptero",               // Air/flight
        "Pred", "Carn", "Rhapt",             // Predator
        "Herb", "Phyt", "Botan",             // Plant-eating
        "Nox", "Noct", "Umbr",               // Night
        "Sol", "Heli", "Lux",                // Day/sun
        "Fer", "Sav", "Atroc",               // Fierce
        "Plac", "Mit", "Len",                // Gentle
        "Long", "Macro", "Dolicho",          // Long
        "Brev", "Brachy", "Micro",           // Short
        "Chrom", "Color", "Pigm",            // Color
        "Morph", "Form", "Fig"               // Shape
    };

    // Latin suffixes for genera
    m_latinSuffixes = {
        "us", "is", "a", "um", "ia",         // Standard endings
        "or", "ax", "ex", "ix", "ox",        // Active endings
        "ensis", "inus", "anus", "icus",     // Location/type
        "oides", "iformis", "atus"           // Similarity
    };

    // Male-sounding individual names
    m_maleNames = {
        "Rex", "Max", "Thor", "Blade", "Fang",
        "Storm", "Bolt", "Spike", "Claw", "Hunter",
        "Shadow", "Blaze", "Titan", "Crusher", "Striker",
        "Atlas", "Brutus", "Caesar", "Drago", "Goliath",
        "Hawk", "Iron", "Jaws", "Kong", "Leo",
        "Magnus", "Nero", "Odin", "Prowler", "Rage",
        "Scar", "Tank", "Venom", "Wolf", "Zeus"
    };

    // Female-sounding individual names
    m_femaleNames = {
        "Luna", "Aurora", "Stella", "Nova", "Ivy",
        "Willow", "Ember", "Jade", "Ruby", "Coral",
        "Pearl", "Siren", "Mystique", "Cleo", "Diana",
        "Echo", "Flora", "Gaia", "Iris", "Jewel",
        "Karma", "Lyra", "Misty", "Nyx", "Orchid",
        "Phoenix", "Quinn", "Raven", "Sage", "Tempest"
    };

    // Gender-neutral individual names
    m_neutralNames = {
        "Ash", "River", "Sky", "Rain", "Frost",
        "Moss", "Stone", "Brook", "Dawn", "Dusk",
        "Cloud", "Leaf", "Thorn", "Reed", "Vale",
        "Storm", "Wisp", "Shade", "Glen", "Ridge",
        "Flint", "Coral", "Marsh", "Peak", "Drift"
    };

    // Titles based on achievements/traits
    m_titles = {
        "the Swift", "the Strong", "the Wise", "the Bold",
        "the Hunter", "the Survivor", "the Elder", "the Young",
        "the Fierce", "the Gentle", "the Silent", "the Loud",
        "the Great", "the Small", "the Quick", "the Steady",
        "the Wanderer", "the Settler", "the Fighter", "the Peaceful",
        "the Ancient", "the Newborn", "the Cunning", "the Brave"
    };
}

void SpeciesNamingSystem::setSeed(uint32_t seed) {
    m_rng.seed(seed);
}

void SpeciesNamingSystem::clear() {
    m_speciesNames.clear();
    m_nameUsageCount.clear();
}

std::string SpeciesNamingSystem::getColorDescriptor(const glm::vec3& color) const {
    // Determine dominant color channel
    float r = color.r, g = color.g, b = color.b;
    float maxChannel = std::max({r, g, b});
    float minChannel = std::min({r, g, b});
    float brightness = (r + g + b) / 3.0f;
    float saturation = maxChannel > 0 ? (maxChannel - minChannel) / maxChannel : 0;

    // Low saturation = grayscale
    if (saturation < 0.2f) {
        if (brightness > 0.8f) return "White";
        if (brightness > 0.6f) return "Silver";
        if (brightness > 0.4f) return "Gray";
        if (brightness > 0.2f) return "Slate";
        return "Black";
    }

    // Determine hue
    float hue = 0;
    if (maxChannel == r) {
        hue = (g - b) / (maxChannel - minChannel);
    } else if (maxChannel == g) {
        hue = 2.0f + (b - r) / (maxChannel - minChannel);
    } else {
        hue = 4.0f + (r - g) / (maxChannel - minChannel);
    }
    hue *= 60.0f;
    if (hue < 0) hue += 360.0f;

    // Map hue to color name
    if (hue < 15 || hue >= 345) {
        return brightness > 0.5f ? "Scarlet" : "Crimson";
    } else if (hue < 45) {
        return brightness > 0.5f ? "Orange" : "Rust";
    } else if (hue < 75) {
        return brightness > 0.5f ? "Golden" : "Amber";
    } else if (hue < 150) {
        return brightness > 0.5f ? "Emerald" : "Jade";
    } else if (hue < 195) {
        return brightness > 0.5f ? "Teal" : "Cyan";
    } else if (hue < 255) {
        return brightness > 0.5f ? "Azure" : "Cobalt";
    } else if (hue < 285) {
        return brightness > 0.5f ? "Violet" : "Purple";
    } else if (hue < 345) {
        return brightness > 0.5f ? "Magenta" : "Plum";
    }

    return "Gray";
}

std::string SpeciesNamingSystem::getSizeDescriptor(float size) const {
    if (size > 1.8f) return "Giant";
    if (size > 1.5f) return "Great";
    if (size > 1.2f) return "Large";
    if (size > 0.8f) return "Common";
    if (size > 0.6f) return "Lesser";
    if (size > 0.4f) return "Small";
    return "Pygmy";
}

std::string SpeciesNamingSystem::getSpeedDescriptor(float speed) const {
    if (speed > 18.0f) return "Swift";
    if (speed > 15.0f) return "Fleet";
    if (speed > 12.0f) return "Quick";
    if (speed > 9.0f) return "Nimble";
    if (speed > 6.0f) return "Steady";
    return "Plodding";
}

std::string SpeciesNamingSystem::getMorphologyDescriptor(const CreatureTraits& traits) const {
    if (traits.hasWings) return "Winged";
    if (traits.hasFins) return "Finned";
    if (traits.hasHorns) return "Horned";
    if (traits.hasCrest) return "Crested";
    if (traits.legCount > 6) return "Many-legged";
    if (traits.legCount == 6) return "Six-legged";
    if (traits.legCount == 0) return "Legless";
    if (traits.tailLength > 1.5f) return "Long-tailed";
    return "";
}

std::string SpeciesNamingSystem::getHabitatDescriptor(const CreatureTraits& traits) const {
    if (traits.livesInWater) return "Swimmer";
    if (traits.canFly) return "Flyer";
    if (traits.burrows) return "Burrower";
    if (traits.speed > 15.0f) return "Runner";
    return "Walker";
}

std::string SpeciesNamingSystem::getBehaviorDescriptor(const CreatureTraits& traits) const {
    if (traits.isCarnivore) return "Hunter";
    if (traits.isPredator) return "Predator";
    if (traits.isHerbivore) return "Grazer";
    return "Forager";
}

std::string SpeciesNamingSystem::generateCommonName(const CreatureTraits& traits) {
    // 60% chance to use new archetype naming, 40% chance for classic naming
    std::uniform_int_distribution<int> styleChoice(0, 9);
    if (styleChoice(m_rng) < 6) {
        // Use new archetype-based naming (e.g., "MossNewt", "EmberShrike", "ReefManta")
        return generateArchetypeName(traits);
    }

    // Classic naming style
    std::uniform_int_distribution<int> coinFlip(0, 1);
    std::string name;

    // Pattern: [Color/Size]-[Morph] [Behavior/Habitat]
    // e.g., "Red-Crested Swift" or "Giant Horned Hunter"

    // First part: Color or Size prefix
    if (coinFlip(m_rng)) {
        name = getColorDescriptor(traits.primaryColor);
    } else {
        name = getSizeDescriptor(traits.size);
    }

    // Optional morphology descriptor
    std::string morph = getMorphologyDescriptor(traits);
    if (!morph.empty() && coinFlip(m_rng)) {
        name += "-" + morph;
    }

    // Add space and second part
    name += " ";

    // Second part: Speed, Habitat, or Behavior
    std::uniform_int_distribution<int> choice(0, 2);
    switch (choice(m_rng)) {
        case 0:
            name += getSpeedDescriptor(traits.speed);
            break;
        case 1:
            name += getHabitatDescriptor(traits);
            break;
        default:
            name += getBehaviorDescriptor(traits);
            break;
    }

    return name;
}

std::string SpeciesNamingSystem::latinize(const std::string& word) const {
    if (word.empty()) return "";

    std::string latin = word;
    // Convert to lowercase
    std::transform(latin.begin(), latin.end(), latin.begin(), ::tolower);

    // Simple latinization rules
    // Remove common English endings and add Latin ones
    if (latin.length() > 3) {
        if (latin.substr(latin.length() - 2) == "er") {
            latin = latin.substr(0, latin.length() - 2) + "or";
        } else if (latin.substr(latin.length() - 3) == "ing") {
            latin = latin.substr(0, latin.length() - 3) + "ens";
        } else if (latin.substr(latin.length() - 2) == "ed") {
            latin = latin.substr(0, latin.length() - 2) + "us";
        } else if (latin.back() == 'y') {
            latin = latin.substr(0, latin.length() - 1) + "ius";
        }
    }

    return latin;
}

std::string SpeciesNamingSystem::generateLatinRoot() const {
    std::uniform_int_distribution<size_t> dist(0, m_latinRoots.size() - 1);
    return m_latinRoots[dist(m_rng)];
}

std::string SpeciesNamingSystem::generateGenus(const CreatureTraits& traits) {
    // Generate genus based on primary trait
    std::string root;

    if (traits.canFly) {
        std::vector<std::string> flyingRoots = {"Ptero", "Aer", "Vol", "Avi"};
        std::uniform_int_distribution<size_t> dist(0, flyingRoots.size() - 1);
        root = flyingRoots[dist(m_rng)];
    } else if (traits.livesInWater) {
        std::vector<std::string> aquaticRoots = {"Aqu", "Ichthy", "Marin", "Fluvi"};
        std::uniform_int_distribution<size_t> dist(0, aquaticRoots.size() - 1);
        root = aquaticRoots[dist(m_rng)];
    } else if (traits.isPredator) {
        std::vector<std::string> predatorRoots = {"Pred", "Carn", "Rhapt", "Fer"};
        std::uniform_int_distribution<size_t> dist(0, predatorRoots.size() - 1);
        root = predatorRoots[dist(m_rng)];
    } else if (traits.speed > 15.0f) {
        std::vector<std::string> speedRoots = {"Veloc", "Celer", "Rapid", "Curs"};
        std::uniform_int_distribution<size_t> dist(0, speedRoots.size() - 1);
        root = speedRoots[dist(m_rng)];
    } else {
        root = generateLatinRoot();
    }

    // Add a suffix
    std::vector<std::string> genusSuffixes = {"us", "is", "a", "ia", "or", "ax"};
    std::uniform_int_distribution<size_t> suffixDist(0, genusSuffixes.size() - 1);

    return root + genusSuffixes[suffixDist(m_rng)];
}

std::string SpeciesNamingSystem::generateSpeciesEpithet(const CreatureTraits& traits) {
    // Species epithet often describes color, habitat, or discoverer
    std::string epithet;

    std::uniform_int_distribution<int> choice(0, 3);
    switch (choice(m_rng)) {
        case 0: {
            // Color-based
            std::string colorName = getColorDescriptor(traits.primaryColor);
            epithet = latinize(colorName);
            break;
        }
        case 1: {
            // Size-based
            if (traits.size > 1.5f) epithet = "magnus";
            else if (traits.size > 1.0f) epithet = "major";
            else if (traits.size > 0.5f) epithet = "minor";
            else epithet = "minimus";
            break;
        }
        case 2: {
            // Habitat-based
            if (traits.livesInWater) epithet = "aquaticus";
            else if (traits.canFly) epithet = "volans";
            else if (traits.burrows) epithet = "fossilis";
            else epithet = "terrestris";
            break;
        }
        default: {
            // Behavior-based
            if (traits.isPredator) epithet = "predator";
            else if (traits.isCarnivore) epithet = "carnivorans";
            else if (traits.isHerbivore) epithet = "herbivorus";
            else epithet = "communis";
            break;
        }
    }

    return epithet;
}

std::string SpeciesNamingSystem::generateFamily(const std::string& genus) {
    // Family names typically end in "-idae" for animals
    if (genus.empty()) return "Incertidae";

    std::string stem = genus;
    // Remove genus ending to get stem
    if (stem.length() > 2) {
        char lastTwo[3] = {stem[stem.length()-2], stem[stem.length()-1], '\0'};
        std::string ending(lastTwo);
        if (ending == "us" || ending == "is" || ending == "or" || ending == "ax") {
            stem = stem.substr(0, stem.length() - 2);
        } else if (stem.back() == 'a') {
            stem = stem.substr(0, stem.length() - 1);
        }
    }

    return stem + "idae";
}

std::string SpeciesNamingSystem::generateOrder(const CreatureTraits& traits) {
    // Order names typically end in "-a" for animals
    if (traits.canFly) return "Volantia";
    if (traits.livesInWater) return "Aquatia";
    if (traits.isPredator || traits.isCarnivore) return "Carnivora";
    if (traits.isHerbivore) return "Herbivora";
    if (traits.speed > 15.0f) return "Cursoria";
    return "Ambulatoria";
}

const SpeciesName& SpeciesNamingSystem::getOrCreateSpeciesName(
    genetics::SpeciesId speciesId,
    const CreatureTraits& traits)
{
    auto it = m_speciesNames.find(speciesId);
    if (it != m_speciesNames.end()) {
        return it->second;
    }

    // Use phoneme-based naming if enabled
    if (m_usePhonemeNaming) {
        return getOrCreateSpeciesNameDeterministic(speciesId, traits, m_planetSeed, m_defaultBiome);
    }

    // Legacy name generation
    SpeciesName name;
    name.speciesId = speciesId;
    name.commonName = generateCommonName(traits);
    name.genus = generateGenus(traits);
    name.species = generateSpeciesEpithet(traits);
    name.scientificName = name.genus + " " + name.species;
    name.family = generateFamily(name.genus);
    name.order = generateOrder(traits);
    name.descriptor = generateDescriptor(traits);

    // Store and return
    m_usedNames.insert(name.commonName);
    auto result = m_speciesNames.emplace(speciesId, std::move(name));
    m_stats.totalNamesGenerated++;
    m_stats.uniqueNames = m_usedNames.size();
    return result.first->second;
}

// =============================================================================
// NEW PHONEME-BASED NAMING SYSTEM
// =============================================================================

const SpeciesName& SpeciesNamingSystem::getOrCreateSpeciesNameDeterministic(
    genetics::SpeciesId speciesId,
    const CreatureTraits& traits,
    uint32_t planetSeed,
    PhonemeTableType biomeTable)
{
    auto it = m_speciesNames.find(speciesId);
    if (it != m_speciesNames.end()) {
        return it->second;
    }

    // Select phoneme table based on traits
    PhonemeTableType tableType = selectPhonemeTable(traits);

    // Generate unique common name using phoneme tables
    std::string commonName = generatePhonemeBasedName(speciesId, tableType);

    // Get or create genus cluster (use speciesId / 8 as default clustering)
    uint32_t clusterId = getGenusClusterId(speciesId);
    std::string genusName = getGenusForCluster(clusterId);

    // Generate species epithet
    std::string epithet = generateSpeciesEpithetFromTraits(traits, genusName);

    // Build the species name
    SpeciesName name;
    name.speciesId = speciesId;
    name.commonName = commonName;
    name.genus = genusName;
    name.species = epithet;
    name.scientificName = genusName + " " + epithet;
    name.family = generateFamily(genusName);
    name.order = generateOrder(traits);
    name.descriptor = generateDescriptor(traits);
    name.genusClusterId = clusterId;

    // Track for uniqueness
    m_usedNames.insert(commonName);

    // Update stats
    m_stats.totalNamesGenerated++;
    m_stats.uniqueNames = m_usedNames.size();

    // Calculate average name length
    size_t totalLen = 0;
    for (const auto& n : m_usedNames) {
        totalLen += n.length();
    }
    m_stats.averageNameLength = m_usedNames.empty() ? 0.0f :
        static_cast<float>(totalLen) / static_cast<float>(m_usedNames.size());

    // Store and return
    auto result = m_speciesNames.emplace(speciesId, std::move(name));
    return result.first->second;
}

std::string SpeciesNamingSystem::generatePhonemeBasedName(
    genetics::SpeciesId speciesId,
    PhonemeTableType tableType) const
{
    auto& phonemeTables = getPhonemeTables();

    // Compute deterministic seed
    uint32_t nameSeed = NamePhonemeTables::computeNameSeed(m_planetSeed, speciesId, tableType);

    // Generate with collision checking
    CollisionResolution result = phonemeTables.generateUniqueName(
        tableType, nameSeed, m_usedNames, 2, 3);

    // Track collision stats
    if (result.wasCollision) {
        m_stats.collisions++;
        m_stats.collisionsByTransform[result.transformsApplied]++;
    }

    return result.resolvedName;
}

PhonemeTableType SpeciesNamingSystem::selectPhonemeTable(const CreatureTraits& traits) const {
    // Select based on environment/traits
    if (traits.livesInWater || traits.hasFins) {
        return PhonemeTableType::OCEANIC;
    }
    if (traits.canFly || traits.hasWings) {
        // Flying creatures use lush or alien tables
        return (traits.isNocturnal) ? PhonemeTableType::ALIEN : PhonemeTableType::LUSH;
    }
    if (traits.burrows || traits.isSubterranean) {
        return PhonemeTableType::DRY;
    }
    if (traits.isArboreal) {
        return PhonemeTableType::LUSH;
    }

    // Use default biome
    return m_defaultBiome;
}

std::string SpeciesNamingSystem::generateGenusName(uint32_t clusterId, PhonemeTableType tableType) const {
    // Check if we already have a genus name for this cluster
    auto it = m_clusterGenusNames.find(clusterId);
    if (it != m_clusterGenusNames.end()) {
        return it->second;
    }

    auto& phonemeTables = getPhonemeTables();

    // Generate genus seed from cluster ID
    uint32_t genusSeed = NamePhonemeTables::computeNameSeed(m_planetSeed, clusterId * 1000, tableType);

    // Generate short name for genus (1-2 syllables, then truncate/modify)
    std::string baseName = phonemeTables.generateName(tableType, genusSeed, 1, 2);

    // Remove vowels at end if present and add genus-like ending
    if (!baseName.empty()) {
        char lastChar = baseName.back();
        if (lastChar == 'a' || lastChar == 'e' || lastChar == 'i' || lastChar == 'o' || lastChar == 'u') {
            baseName = baseName.substr(0, baseName.length() - 1);
        }
    }

    // Ensure first letter is uppercase
    if (!baseName.empty()) {
        baseName[0] = static_cast<char>(std::toupper(baseName[0]));
    }

    return baseName;
}

std::string SpeciesNamingSystem::generateSpeciesEpithetFromTraits(
    const CreatureTraits& traits,
    const std::string& genus) const
{
    // Generate epithet based on dominant trait
    std::vector<std::string> epithets;

    // Size-based
    if (traits.size > 1.5f) epithets.push_back("magnus");
    else if (traits.size > 1.2f) epithets.push_back("major");
    else if (traits.size < 0.5f) epithets.push_back("minimus");
    else if (traits.size < 0.8f) epithets.push_back("minor");

    // Speed-based
    if (traits.speed > 18.0f) epithets.push_back("velox");
    else if (traits.speed > 15.0f) epithets.push_back("celer");
    else if (traits.speed < 6.0f) epithets.push_back("tardus");

    // Habitat-based
    if (traits.livesInWater) epithets.push_back("aquatilis");
    else if (traits.canFly) epithets.push_back("volans");
    else if (traits.burrows) epithets.push_back("fossilis");
    else if (traits.isArboreal) epithets.push_back("arboreus");

    // Diet-based
    if (traits.isCarnivore) epithets.push_back("carnifex");
    else if (traits.isHerbivore) epithets.push_back("herbivorus");
    else if (traits.isOmnivore) epithets.push_back("omnivorus");

    // Behavior-based
    if (traits.isNocturnal) epithets.push_back("noctis");
    if (traits.isSocial) epithets.push_back("gregarius");

    // Color-based (fallback)
    std::string colorEpithet = latinize(getColorDescriptor(traits.primaryColor));
    if (!colorEpithet.empty()) {
        epithets.push_back(colorEpithet);
    }

    // Select from available epithets
    if (epithets.empty()) {
        epithets.push_back("communis");
    }

    // Use deterministic selection based on genus hash
    size_t hash = std::hash<std::string>{}(genus);
    size_t index = hash % epithets.size();

    return epithets[index];
}

// =============================================================================
// TRAIT DESCRIPTOR GENERATION (NO GENERIC LABELS)
// =============================================================================

TraitDescriptor SpeciesNamingSystem::generateDescriptor(const CreatureTraits& traits) const {
    TraitDescriptor descriptor;
    descriptor.diet = getDietString(traits);
    descriptor.locomotion = getLocomotionString(traits);
    return descriptor;
}

std::string SpeciesNamingSystem::getDietString(const CreatureTraits& traits) {
    // NO "apex", "predator", or other generic labels
    // Use specific diet only
    if (traits.isCarnivore && traits.livesInWater) {
        return "piscivore";  // Fish-eater
    }
    if (traits.isCarnivore) {
        return "carnivore";
    }
    if (traits.isHerbivore && traits.canFly) {
        return "nectarivore";  // Nectar-feeder
    }
    if (traits.isHerbivore && traits.livesInWater) {
        return "filter-feeder";
    }
    if (traits.isHerbivore) {
        return "herbivore";
    }
    if (traits.isOmnivore) {
        return "omnivore";
    }

    // Default based on predator flag
    if (traits.isPredator) {
        return "carnivore";
    }

    return "herbivore";
}

std::string SpeciesNamingSystem::getLocomotionString(const CreatureTraits& traits) {
    // Prioritize most specific habitat/locomotion
    if (traits.livesInWater && traits.canFly) {
        return "amphibious";
    }
    if (traits.livesInWater) {
        return "aquatic";
    }
    if (traits.canFly) {
        return "aerial";
    }
    if (traits.burrows || traits.isSubterranean) {
        return "burrowing";
    }
    if (traits.isArboreal) {
        return "arboreal";
    }

    // Ground-dwelling default
    return "terrestrial";
}

// =============================================================================
// BINOMIAL NAMING AND GENUS CLUSTERS
// =============================================================================

uint32_t SpeciesNamingSystem::getGenusClusterId(genetics::SpeciesId speciesId) const {
    // Check if we have a cluster assignment
    auto it = m_speciesGenusCluster.find(speciesId);
    if (it != m_speciesGenusCluster.end()) {
        return it->second;
    }

    // Default: cluster by dividing speciesId by 8
    // This groups similar species IDs together
    return speciesId / 8;
}

void SpeciesNamingSystem::setGenusCluster(genetics::SpeciesId speciesId, uint32_t clusterId) {
    m_speciesGenusCluster[speciesId] = clusterId;
}

std::string SpeciesNamingSystem::getGenusForCluster(uint32_t clusterId) {
    // Check if we already have a genus name for this cluster
    auto it = m_clusterGenusNames.find(clusterId);
    if (it != m_clusterGenusNames.end()) {
        return it->second;
    }

    // Generate new genus name
    std::string genusName = generateGenusName(clusterId, m_defaultBiome);

    // Store for future use
    m_clusterGenusNames[clusterId] = genusName;

    return genusName;
}

// =============================================================================
// DEBUG AND VALIDATION
// =============================================================================

void SpeciesNamingSystem::logStats() const {
    std::cout << "=== Species Naming System Statistics ===" << std::endl;
    std::cout << "Total names generated: " << m_stats.totalNamesGenerated << std::endl;
    std::cout << "Unique names: " << m_stats.uniqueNames << std::endl;
    std::cout << "Collisions: " << m_stats.collisions << std::endl;
    std::cout << "Average name length: " << m_stats.averageNameLength << std::endl;

    if (!m_stats.collisionsByTransform.empty()) {
        std::cout << "Collisions by transform:" << std::endl;
        for (const auto& [transform, count] : m_stats.collisionsByTransform) {
            std::cout << "  Transform " << transform << ": " << count << std::endl;
        }
    }

    float collisionRate = m_stats.totalNamesGenerated > 0 ?
        static_cast<float>(m_stats.collisions) / static_cast<float>(m_stats.totalNamesGenerated) * 100.0f : 0.0f;
    std::cout << "Collision rate: " << collisionRate << "%" << std::endl;
}

float SpeciesNamingSystem::validateNameGeneration(int count, uint32_t testSeed) const {
    std::unordered_set<std::string> testNames;
    int collisions = 0;

    auto& phonemeTables = getPhonemeTables();

    for (int i = 0; i < count; ++i) {
        uint32_t nameSeed = NamePhonemeTables::computeNameSeed(testSeed, i, PhonemeTableType::LUSH);
        auto result = phonemeTables.generateUniqueName(PhonemeTableType::LUSH, nameSeed, testNames);

        if (result.wasCollision) {
            collisions++;
        }

        testNames.insert(result.resolvedName);
    }

    float collisionRate = count > 0 ?
        static_cast<float>(collisions) / static_cast<float>(count) * 100.0f : 0.0f;

    std::cout << "Validation: Generated " << count << " names with " << collisions
              << " collisions (" << collisionRate << "% rate)" << std::endl;

    return collisionRate;
}

const SpeciesName* SpeciesNamingSystem::getSpeciesName(genetics::SpeciesId speciesId) const {
    auto it = m_speciesNames.find(speciesId);
    if (it != m_speciesNames.end()) {
        return &it->second;
    }
    return nullptr;
}

void SpeciesNamingSystem::updateSpeciesName(genetics::SpeciesId speciesId,
                                            const std::string& newName) {
    auto it = m_speciesNames.find(speciesId);
    if (it != m_speciesNames.end()) {
        it->second.commonName = newName;
    }
}

SpeciesName SpeciesNamingSystem::evolveSpeciesName(
    genetics::SpeciesId parentSpeciesId,
    genetics::SpeciesId newSpeciesId,
    const CreatureTraits& newTraits)
{
    // Get parent species name if exists
    const SpeciesName* parentName = getSpeciesName(parentSpeciesId);

    SpeciesName newName;
    newName.speciesId = newSpeciesId;

    if (parentName) {
        // Inherit genus from parent, generate new species epithet
        newName.genus = parentName->genus;
        newName.family = parentName->family;
        newName.order = parentName->order;
        newName.species = generateSpeciesEpithet(newTraits);

        // Modify common name to show relation
        std::string newPrefix;
        std::uniform_int_distribution<int> choice(0, 2);
        switch (choice(m_rng)) {
            case 0:
                newPrefix = getColorDescriptor(newTraits.primaryColor);
                break;
            case 1:
                newPrefix = getSizeDescriptor(newTraits.size);
                break;
            default:
                newPrefix = getMorphologyDescriptor(newTraits);
                if (newPrefix.empty()) {
                    newPrefix = getSpeedDescriptor(newTraits.speed);
                }
                break;
        }

        // Extract base name from parent
        std::string baseName = parentName->commonName;
        size_t spacePos = baseName.find(' ');
        if (spacePos != std::string::npos) {
            baseName = baseName.substr(spacePos + 1);
        }

        newName.commonName = newPrefix + " " + baseName;
    } else {
        // Generate completely new name
        newName = getOrCreateSpeciesName(newSpeciesId, newTraits);
    }

    newName.scientificName = newName.genus + " " + newName.species;

    // Store the new species name
    m_speciesNames[newSpeciesId] = newName;

    return newName;
}

std::string SpeciesNamingSystem::generateFirstName(bool isPredator) const {
    // Mix of all name types with weighted selection
    std::uniform_int_distribution<int> typeChoice(0, 9);
    int type = typeChoice(m_rng);

    const std::vector<std::string>* nameList;
    if (type < 3) {
        nameList = isPredator ? &m_maleNames : &m_femaleNames;
    } else if (type < 6) {
        nameList = isPredator ? &m_femaleNames : &m_maleNames;
    } else {
        nameList = &m_neutralNames;
    }

    std::uniform_int_distribution<size_t> dist(0, nameList->size() - 1);
    return (*nameList)[dist(m_rng)];
}

std::string SpeciesNamingSystem::calculateSuffix(int ancestorCount) const {
    if (ancestorCount == 0) return "";
    if (ancestorCount == 1) return "Jr.";
    if (ancestorCount == 2) return "III";
    if (ancestorCount == 3) return "IV";
    if (ancestorCount == 4) return "V";
    if (ancestorCount == 5) return "VI";

    // For very long lineages, use ordinal numbers
    return std::to_string(ancestorCount + 1) + "th";
}

std::string SpeciesNamingSystem::generateTitle(const CreatureTraits& traits) const {
    // Only some creatures get titles
    std::uniform_int_distribution<int> titleChance(0, 4);
    if (titleChance(m_rng) != 0) return "";

    std::uniform_int_distribution<size_t> dist(0, m_titles.size() - 1);
    return m_titles[dist(m_rng)];
}

IndividualName SpeciesNamingSystem::generateIndividualName(
    genetics::SpeciesId speciesId,
    int generation,
    int parentId,
    const std::string& parentName)
{
    IndividualName name;
    name.generation = generation;
    name.parentId = parentId;
    name.parentName = parentName;

    // Get species info for context
    const SpeciesName* speciesName = getSpeciesName(speciesId);
    bool isPredator = false;
    if (speciesName) {
        isPredator = (speciesName->order == "Carnivora");
    }

    // Generate first name
    name.firstName = generateFirstName(isPredator);

    // Check if this name has been used before
    std::string nameKey = name.firstName + "_" + std::to_string(speciesId);
    auto it = m_nameUsageCount.find(nameKey);
    if (it != m_nameUsageCount.end()) {
        name.ancestorCount = it->second;
        name.suffix = calculateSuffix(name.ancestorCount);
        it->second++;
    } else {
        m_nameUsageCount[nameKey] = 1;
        name.ancestorCount = 0;
    }

    // Optionally generate title for older/special creatures
    if (generation > 5) {
        CreatureTraits dummyTraits;
        name.title = generateTitle(dummyTraits);
    }

    return name;
}

std::string SpeciesNamingSystem::exportToJson() const {
    std::stringstream ss;
    ss << "{\n  \"species\": [\n";

    bool first = true;
    for (const auto& [id, name] : m_speciesNames) {
        if (!first) ss << ",\n";
        first = false;

        ss << "    {\n";
        ss << "      \"id\": " << id << ",\n";
        ss << "      \"commonName\": \"" << name.commonName << "\",\n";
        ss << "      \"genus\": \"" << name.genus << "\",\n";
        ss << "      \"species\": \"" << name.species << "\",\n";
        ss << "      \"family\": \"" << name.family << "\",\n";
        ss << "      \"order\": \"" << name.order << "\"\n";
        ss << "    }";
    }

    ss << "\n  ]\n}";
    return ss.str();
}

void SpeciesNamingSystem::importFromJson(const std::string& json) {
    // Minimal JSON parser for species data format
    // Expected format: {"species": [{"id": N, "commonName": "...", "genus": "...", "species": "...", "family": "...", "order": "..."}]}

    if (json.empty()) {
        std::cerr << "ERROR: SpeciesNamingSystem::importFromJson called with empty JSON!" << std::endl;
        assert(false && "importFromJson - empty JSON string");
        return;
    }

    // Helper to extract string value after a key
    auto extractString = [&json](const std::string& key, size_t startPos) -> std::pair<std::string, size_t> {
        std::string searchKey = "\"" + key + "\":";
        size_t keyPos = json.find(searchKey, startPos);
        if (keyPos == std::string::npos) return {"", std::string::npos};

        size_t valueStart = json.find('"', keyPos + searchKey.length());
        if (valueStart == std::string::npos) return {"", std::string::npos};
        valueStart++; // Skip opening quote

        size_t valueEnd = json.find('"', valueStart);
        if (valueEnd == std::string::npos) return {"", std::string::npos};

        return {json.substr(valueStart, valueEnd - valueStart), valueEnd};
    };

    // Helper to extract int value after a key
    auto extractInt = [&json](const std::string& key, size_t startPos) -> std::pair<int, size_t> {
        std::string searchKey = "\"" + key + "\":";
        size_t keyPos = json.find(searchKey, startPos);
        if (keyPos == std::string::npos) return {0, std::string::npos};

        size_t valueStart = keyPos + searchKey.length();
        while (valueStart < json.length() && (json[valueStart] == ' ' || json[valueStart] == '\t')) {
            valueStart++;
        }

        size_t valueEnd = valueStart;
        while (valueEnd < json.length() && (isdigit(json[valueEnd]) || json[valueEnd] == '-')) {
            valueEnd++;
        }

        if (valueStart == valueEnd) return {0, std::string::npos};
        return {std::stoi(json.substr(valueStart, valueEnd - valueStart)), valueEnd};
    };

    // Find species array
    size_t speciesArrayStart = json.find("\"species\":");
    if (speciesArrayStart == std::string::npos) {
        std::cerr << "ERROR: SpeciesNamingSystem::importFromJson - 'species' array not found in JSON!" << std::endl;
        assert(false && "importFromJson - invalid JSON format: no 'species' array");
        return;
    }

    // Parse each species object
    size_t pos = speciesArrayStart;
    int importCount = 0;
    int errorCount = 0;

    while ((pos = json.find('{', pos)) != std::string::npos) {
        size_t objEnd = json.find('}', pos);
        if (objEnd == std::string::npos) break;

        // Extract fields
        auto [id, idEnd] = extractInt("id", pos);
        auto [commonName, cnEnd] = extractString("commonName", pos);
        auto [genus, gEnd] = extractString("genus", pos);
        auto [species, sEnd] = extractString("species", pos);
        auto [family, fEnd] = extractString("family", pos);
        auto [order, oEnd] = extractString("order", pos);

        // Validate required fields
        if (idEnd == std::string::npos || commonName.empty()) {
            std::cerr << "WARNING: SpeciesNamingSystem::importFromJson - skipping malformed species entry at position "
                      << pos << std::endl;
            errorCount++;
            pos = objEnd + 1;
            continue;
        }

        // Create and store species name
        SpeciesName name;
        name.speciesId = static_cast<uint32_t>(id);
        name.commonName = commonName;
        name.genus = genus;
        name.species = species;
        name.family = family;
        name.order = order;
        name.scientificName = genus + " " + species;

        m_speciesNames[name.speciesId] = name;
        importCount++;

        pos = objEnd + 1;
    }

    if (importCount == 0 && errorCount > 0) {
        std::cerr << "ERROR: SpeciesNamingSystem::importFromJson - failed to import any species! "
                  << errorCount << " malformed entries found." << std::endl;
        assert(false && "importFromJson - failed to import any species data");
    } else if (errorCount > 0) {
        std::cerr << "WARNING: SpeciesNamingSystem::importFromJson - imported " << importCount
                  << " species with " << errorCount << " errors." << std::endl;
    }
}

// =============================================================================
// ARCHETYPE-BASED NAMING SYSTEM
// Generates unique names like "MossNewt", "EmberShrike", "ReefManta", "FrostGlider"
// =============================================================================

void SpeciesNamingSystem::initializeArchetypeComponents() {
    // Archetype prefixes - evocative environmental/characteristic prefixes
    // Categories: agile (fast+small), heavy (slow+large), aquatic, aerial,
    //            nocturnal, tropical, cold, desert, forest, coastal
    m_archetypePrefixes = {
        // Agile/Swift archetypes (fast + small creatures)
        "Swift", "Flash", "Dart", "Zephyr", "Nimble", "Quick", "Fleet",
        // Shadow/Nocturnal archetypes
        "Shadow", "Dusk", "Twilight", "Night", "Moon", "Shade", "Gloom",
        // Nature/Forest archetypes
        "Moss", "Fern", "Leaf", "Thorn", "Bramble", "Oak", "Willow", "Ivy",
        // Aquatic/Coastal archetypes
        "Coral", "Reef", "Tide", "Wave", "Kelp", "Pearl", "Lagoon", "Abyssal",
        // Sky/Aerial archetypes
        "Sky", "Cloud", "Storm", "Gale", "Wind", "Soar", "Aether", "Cirrus",
        // Time/Light archetypes
        "Dawn", "Dusk", "Solar", "Luna", "Aurora", "Starlit", "Radiant",
        // Temperature/Climate archetypes
        "Frost", "Ice", "Glacier", "Chill", "Ember", "Flame", "Blaze", "Scorch",
        // Desert/Arid archetypes
        "Sand", "Dune", "Mesa", "Dusty", "Copper", "Ochre", "Amber",
        // Heavy/Large archetypes (slow + large creatures)
        "Stone", "Iron", "Boulder", "Granite", "Thunder", "Titan", "Mammoth",
        // Exotic/Mysterious archetypes
        "Obsidian", "Crystal", "Opal", "Jade", "Onyx", "Sapphire", "Garnet"
    };

    // Locomotion suffixes - based on how the creature moves
    m_locomotionSuffixes = {
        // Ground predators
        "Stalker", "Hunter", "Prowler", "Tracker", "Chaser",
        // Aerial movement
        "Glider", "Soarer", "Diver", "Swooper", "Flitter", "Hover",
        // Aquatic movement
        "Swimmer", "Diver", "Drifter", "Surfer", "Dasher",
        // Ground herbivores
        "Crawler", "Hopper", "Leaper", "Jumper", "Bouncer",
        // Fast ground movement
        "Runner", "Sprinter", "Racer", "Dasher", "Bolter",
        // Slow/Methodical movement
        "Wanderer", "Roamer", "Treader", "Strider", "Pacer",
        // Climbing/Arboreal
        "Climber", "Scaler", "Vaulter", "Swinger",
        // Burrowing
        "Burrower", "Digger", "Tunneler"
    };

    // Species words - animal-inspired base names that evoke real creatures
    m_speciesWords = {
        // Bird-like
        "Finch", "Shrike", "Heron", "Crane", "Falcon", "Hawk", "Sparrow",
        "Wren", "Robin", "Jay", "Raven", "Owl", "Swift", "Martin", "Kite",
        // Aquatic
        "Manta", "Pike", "Perch", "Bass", "Eel", "Cod", "Trout", "Salmon",
        "Carp", "Gar", "Barb", "Guppy", "Tetra", "Betta", "Koi",
        // Reptile/Amphibian
        "Newt", "Gecko", "Skink", "Toad", "Frog", "Salamander", "Anole",
        "Iguana", "Monitor", "Basilisk", "Chameleon",
        // Insect-like
        "Beetle", "Moth", "Cicada", "Cricket", "Mantis", "Wasp", "Hornet",
        "Dragonfly", "Damsel", "Lacewing", "Firefly", "Weevil",
        // Mammal-like
        "Otter", "Mink", "Fox", "Wolf", "Lynx", "Vole", "Shrew", "Mole",
        "Badger", "Ferret", "Stoat", "Marten", "Hare", "Pika",
        // Exotic/Unique
        "Pangolin", "Sloth", "Lemur", "Loris", "Tarsier", "Kinkajou",
        // Fantasy-adjacent but natural-sounding
        "Wyrm", "Drake", "Wyvern", "Basilisk", "Hydra", "Chimera"
    };
}

std::string SpeciesNamingSystem::selectArchetypePrefix(const CreatureTraits& traits) const {
    // Select prefix based on creature characteristics
    std::vector<std::string> candidates;

    // Fast + small = agile prefixes
    if (traits.speed > 15.0f && traits.size < 0.8f) {
        candidates = {"Swift", "Flash", "Dart", "Zephyr", "Nimble", "Quick", "Fleet"};
    }
    // Slow + large = heavy prefixes
    else if (traits.speed < 8.0f && traits.size > 1.5f) {
        candidates = {"Stone", "Iron", "Boulder", "Granite", "Thunder", "Titan", "Mammoth"};
    }
    // Aquatic creatures
    else if (traits.livesInWater || traits.hasFins) {
        candidates = {"Coral", "Reef", "Tide", "Wave", "Kelp", "Pearl", "Lagoon", "Abyssal"};
    }
    // Flying creatures
    else if (traits.canFly || traits.hasWings) {
        candidates = {"Sky", "Cloud", "Storm", "Gale", "Wind", "Soar", "Aether", "Cirrus"};
    }
    // Nocturnal creatures
    else if (traits.isNocturnal) {
        candidates = {"Shadow", "Dusk", "Twilight", "Night", "Moon", "Shade", "Gloom"};
    }
    // Predators
    else if (traits.isPredator || traits.isCarnivore) {
        candidates = {"Ember", "Flame", "Blaze", "Obsidian", "Onyx", "Thorn", "Bramble"};
    }
    // Default - nature/forest prefixes
    else {
        candidates = {"Moss", "Fern", "Leaf", "Dawn", "Solar", "Willow", "Ivy", "Jade"};
    }

    // Add some variety by occasionally mixing in other prefixes
    std::uniform_int_distribution<int> mixChance(0, 4);
    if (mixChance(m_rng) == 0 && !m_archetypePrefixes.empty()) {
        std::uniform_int_distribution<size_t> dist(0, m_archetypePrefixes.size() - 1);
        return m_archetypePrefixes[dist(m_rng)];
    }

    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    return candidates[dist(m_rng)];
}

std::string SpeciesNamingSystem::selectLocomotionSuffix(const CreatureTraits& traits) const {
    std::vector<std::string> candidates;

    // Flying creatures
    if (traits.canFly || traits.hasWings) {
        if (traits.speed > 15.0f) {
            candidates = {"Glider", "Soarer", "Diver", "Swooper"};
        } else {
            candidates = {"Flitter", "Hover", "Drifter"};
        }
    }
    // Aquatic creatures
    else if (traits.livesInWater || traits.hasFins) {
        if (traits.speed > 12.0f) {
            candidates = {"Swimmer", "Dasher", "Surfer"};
        } else {
            candidates = {"Drifter", "Diver", "Glider"};
        }
    }
    // Fast ground creatures
    else if (traits.speed > 15.0f) {
        if (traits.isPredator) {
            candidates = {"Stalker", "Hunter", "Prowler", "Chaser"};
        } else {
            candidates = {"Runner", "Sprinter", "Racer", "Dasher", "Bolter"};
        }
    }
    // Jumping creatures (small + moderate speed)
    else if (traits.size < 0.7f && traits.speed > 10.0f) {
        candidates = {"Hopper", "Leaper", "Jumper", "Bouncer"};
    }
    // Slow creatures
    else if (traits.speed < 8.0f) {
        if (traits.burrows) {
            candidates = {"Burrower", "Digger", "Tunneler"};
        } else {
            candidates = {"Wanderer", "Roamer", "Treader", "Strider", "Pacer"};
        }
    }
    // Default moderate movement
    else {
        if (traits.isPredator) {
            candidates = {"Tracker", "Prowler", "Stalker"};
        } else {
            candidates = {"Walker", "Strider", "Roamer"};
        }
    }

    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    return candidates[dist(m_rng)];
}

std::string SpeciesNamingSystem::selectSpeciesWord(const CreatureTraits& traits) const {
    std::vector<std::string> candidates;

    // Flying creatures - bird names
    if (traits.canFly || traits.hasWings) {
        if (traits.isPredator) {
            candidates = {"Falcon", "Hawk", "Shrike", "Owl", "Kite", "Raven"};
        } else if (traits.size < 0.7f) {
            candidates = {"Finch", "Sparrow", "Wren", "Robin", "Swift", "Martin"};
        } else {
            candidates = {"Heron", "Crane", "Jay", "Stork", "Ibis"};
        }
    }
    // Aquatic creatures - fish names
    else if (traits.livesInWater || traits.hasFins) {
        if (traits.isPredator || traits.isCarnivore) {
            candidates = {"Pike", "Gar", "Barracuda", "Bass"};
        } else if (traits.size > 1.2f) {
            candidates = {"Manta", "Carp", "Koi", "Sturgeon"};
        } else {
            candidates = {"Perch", "Trout", "Guppy", "Tetra", "Betta", "Barb"};
        }
    }
    // Small creatures - insect/small animal names
    else if (traits.size < 0.6f) {
        if (traits.isPredator) {
            candidates = {"Mantis", "Wasp", "Hornet", "Spider"};
        } else {
            candidates = {"Beetle", "Moth", "Cicada", "Cricket", "Firefly", "Weevil"};
        }
    }
    // Medium ground creatures
    else if (traits.size < 1.2f) {
        if (traits.isPredator) {
            candidates = {"Fox", "Lynx", "Mink", "Stoat", "Ferret", "Marten"};
        } else {
            candidates = {"Newt", "Gecko", "Skink", "Vole", "Shrew", "Hare", "Pika"};
        }
    }
    // Large creatures
    else {
        if (traits.isPredator) {
            candidates = {"Wolf", "Badger", "Monitor", "Wyrm", "Drake"};
        } else {
            candidates = {"Otter", "Sloth", "Pangolin", "Iguana", "Salamander"};
        }
    }

    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    return candidates[dist(m_rng)];
}

std::string SpeciesNamingSystem::generateArchetypeName(const CreatureTraits& traits) const {
    // Generate name in format: [Prefix][SpeciesWord] or [Prefix][SpeciesWord][Suffix]
    // Examples: "MossNewt", "EmberShrike", "ReefManta", "FrostGlider"

    std::string prefix = selectArchetypePrefix(traits);
    std::string speciesWord = selectSpeciesWord(traits);

    // 50% chance to use species word alone, 50% to add locomotion suffix
    std::uniform_int_distribution<int> choice(0, 2);
    int format = choice(m_rng);

    if (format == 0) {
        // Format: PrefixSpecies (e.g., "MossNewt", "EmberShrike")
        return prefix + speciesWord;
    } else if (format == 1) {
        // Format: PrefixSuffix (e.g., "FrostGlider", "CoralSwimmer")
        std::string suffix = selectLocomotionSuffix(traits);
        return prefix + suffix;
    } else {
        // Format: Prefix Species (with space, e.g., "Moss Newt", "Ember Shrike")
        return prefix + " " + speciesWord;
    }
}

} // namespace naming
