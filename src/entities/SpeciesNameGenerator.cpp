#include "SpeciesNameGenerator.h"
#include "../environment/PlanetTheme.h"
#include <chrono>
#include <cmath>

namespace naming {

// Global instance
static std::unique_ptr<SpeciesNameGenerator> g_nameGenerator;

SpeciesNameGenerator& getNameGenerator() {
    if (!g_nameGenerator) {
        g_nameGenerator = std::make_unique<SpeciesNameGenerator>();
    }
    return *g_nameGenerator;
}

SpeciesNameGenerator::SpeciesNameGenerator() {
    // Seed with current time
    auto seed = static_cast<uint32_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    m_rng.seed(seed);

    initializePrefixes();
    initializeSuffixes();
    initializeSpeciesWords();
}

void SpeciesNameGenerator::setSeed(uint32_t seed) {
    m_rng.seed(seed);
}

void SpeciesNameGenerator::initializePrefixes() {
    // Agile prefixes (fast + small creatures)
    m_agilePrefixes = {
        "Swift", "Flash", "Dart", "Zephyr", "Nimble", "Quick", "Fleet",
        "Spark", "Dash", "Blur", "Whisk", "Zip"
    };

    // Heavy prefixes (slow + large creatures)
    m_heavyPrefixes = {
        "Stone", "Iron", "Boulder", "Granite", "Thunder", "Titan", "Mammoth",
        "Brute", "Colossal", "Massive", "Tank", "Goliath"
    };

    // Shadow/Nocturnal prefixes
    m_shadowPrefixes = {
        "Shadow", "Dusk", "Twilight", "Night", "Moon", "Shade", "Gloom",
        "Phantom", "Umbra", "Murk", "Shroud", "Void"
    };

    // Nature/Forest prefixes (moss-like)
    m_mossPrefixes = {
        "Moss", "Fern", "Leaf", "Willow", "Ivy", "Vine", "Lichen",
        "Grove", "Meadow", "Bark", "Root", "Sage"
    };

    // Aquatic/Coastal prefixes (coral-like)
    m_coralPrefixes = {
        "Coral", "Tide", "Wave", "Kelp", "Pearl", "Lagoon", "Shoal",
        "Brine", "Salt", "Spray", "Foam", "Cove"
    };

    // Sky/Aerial prefixes
    m_skyPrefixes = {
        "Sky", "Cloud", "Storm", "Gale", "Wind", "Aether", "Cirrus",
        "Breeze", "Tempest", "Squall", "Draft", "Azure"
    };

    // Time/Light prefixes (dawn-like)
    m_dawnPrefixes = {
        "Dawn", "Dusk", "Solar", "Luna", "Aurora", "Radiant", "Starlit",
        "Gleam", "Shimmer", "Bright", "Glow", "Beam"
    };

    // Cold/Ice prefixes (frost-like)
    m_frostPrefixes = {
        "Frost", "Ice", "Glacier", "Chill", "Snow", "Winter", "Tundra",
        "Frozen", "Arctic", "Boreal", "Frigid", "Crystal"
    };

    // Fire/Heat prefixes (ember-like)
    m_emberPrefixes = {
        "Ember", "Flame", "Blaze", "Scorch", "Cinder", "Spark", "Ash",
        "Inferno", "Char", "Smolder", "Furnace", "Molten"
    };

    // Thorny/Defensive prefixes
    m_thornPrefixes = {
        "Thorn", "Bramble", "Spine", "Barb", "Spike", "Nettle", "Prickle",
        "Bristle", "Quill", "Needle", "Talon", "Fang"
    };

    // Reef/Deep water prefixes
    m_reefPrefixes = {
        "Reef", "Abyssal", "Deep", "Abyss", "Trench", "Fathom", "Hadal",
        "Midnight", "Pelagic", "Benthic", "Nautical", "Oceanic"
    };
}

void SpeciesNameGenerator::initializeSuffixes() {
    // Ground predator suffixes
    m_stalkerSuffixes = {
        "Stalker", "Hunter", "Prowler", "Tracker", "Chaser", "Predator",
        "Ambusher", "Striker", "Slayer", "Ravager"
    };

    // Aerial movement suffixes
    m_gliderSuffixes = {
        "Glider", "Soarer", "Flitter", "Hover", "Drifter", "Floater",
        "Swoop", "Swooper", "Wing", "Flutter"
    };

    // Aquatic movement suffixes
    m_swimmerSuffixes = {
        "Swimmer", "Dasher", "Surfer", "Streamer", "Torpedo", "Flash",
        "Dart", "Glide", "Cruise", "Rush"
    };

    // Slow ground movement suffixes
    m_crawlerSuffixes = {
        "Crawler", "Creeper", "Trudger", "Plodder", "Lumber", "Shambler",
        "Waddle", "Trundler", "Mover", "Shuffler"
    };

    // Jumping creature suffixes
    m_hopperSuffixes = {
        "Hopper", "Leaper", "Jumper", "Bouncer", "Springer", "Vaulter",
        "Skip", "Bound", "Pouncer", "Skipper"
    };

    // Diving creature suffixes
    m_diverSuffixes = {
        "Diver", "Plunger", "Splasher", "Submerger", "Sinker", "Delver",
        "Drop", "Swooper", "Pierce", "Plummet"
    };

    // Soaring/gliding suffixes
    m_soarerSuffixes = {
        "Soarer", "Rider", "Sailor", "Coaster", "Glide", "Drift",
        "Float", "Waft", "Cruise", "Hover"
    };

    // Fast ground movement suffixes
    m_runnerSuffixes = {
        "Runner", "Sprinter", "Racer", "Dasher", "Bolter", "Speedster",
        "Zip", "Streak", "Blur", "Flash"
    };
}

void SpeciesNameGenerator::initializeSpeciesWords() {
    // Bird-like species words
    m_birdSpecies = {
        "Finch", "Shrike", "Heron", "Crane", "Falcon", "Hawk", "Sparrow",
        "Wren", "Robin", "Jay", "Raven", "Owl", "Swift", "Martin", "Kite",
        "Lark", "Thrush", "Warbler", "Starling", "Bunting"
    };

    // Fish-like species words
    m_fishSpecies = {
        "Manta", "Pike", "Perch", "Bass", "Eel", "Cod", "Trout", "Salmon",
        "Carp", "Gar", "Barb", "Guppy", "Tetra", "Betta", "Koi",
        "Grouper", "Snapper", "Mullet", "Anchovy", "Herring"
    };

    // Reptile/Amphibian species words
    m_reptileSpecies = {
        "Newt", "Gecko", "Skink", "Toad", "Frog", "Salamander", "Anole",
        "Iguana", "Monitor", "Basilisk", "Chameleon", "Agama", "Slider",
        "Caecilian", "Axolotl", "Siren", "Hellbender", "Mudpuppy"
    };

    // Insect-like species words
    m_insectSpecies = {
        "Beetle", "Moth", "Cicada", "Cricket", "Mantis", "Wasp", "Hornet",
        "Dragonfly", "Damsel", "Lacewing", "Firefly", "Weevil", "Leafhopper",
        "Katydid", "Stonefly", "Mayfly", "Caddis", "Sawfly"
    };

    // Mammal-like species words
    m_mammalSpecies = {
        "Otter", "Mink", "Fox", "Wolf", "Lynx", "Vole", "Shrew", "Mole",
        "Badger", "Ferret", "Stoat", "Marten", "Hare", "Pika", "Weasel",
        "Mongoose", "Civet", "Genet", "Dormouse", "Lemming"
    };
}

float SpeciesNameGenerator::genomeHash(const Genome& genome, int index) const {
    // Generate deterministic value from genome traits
    float val = 0.0f;
    if (!genome.neuralWeights.empty()) {
        val = genome.neuralWeights[index % genome.neuralWeights.size()];
    }
    val = std::fmod(val * 12.9898f + genome.size * 78.233f + genome.speed * 43.758f + index * 17.291f, 1.0f);
    return std::abs(val);
}

bool SpeciesNameGenerator::isAgile(const Genome& genome) const {
    return genome.speed > 15.0f && genome.size < 0.8f;
}

bool SpeciesNameGenerator::isHeavy(const Genome& genome) const {
    return genome.speed < 8.0f && genome.size > 1.5f;
}

bool SpeciesNameGenerator::isNocturnal(const Genome& genome) const {
    // Use camouflage level and color darkness as proxy for nocturnal behavior
    float brightness = (genome.color.r + genome.color.g + genome.color.b) / 3.0f;
    return genome.camouflageLevel > 0.6f || brightness < 0.3f;
}

bool SpeciesNameGenerator::isAquatic(CreatureType type) const {
    return type == CreatureType::AQUATIC ||
           type == CreatureType::AQUATIC_HERBIVORE ||
           type == CreatureType::AQUATIC_PREDATOR ||
           type == CreatureType::AQUATIC_APEX ||
           type == CreatureType::AMPHIBIAN;
}

bool SpeciesNameGenerator::isFlying(CreatureType type) const {
    return type == CreatureType::FLYING ||
           type == CreatureType::FLYING_BIRD ||
           type == CreatureType::FLYING_INSECT ||
           type == CreatureType::AERIAL_PREDATOR;
}

bool SpeciesNameGenerator::isPredator(CreatureType type) const {
    return type == CreatureType::CARNIVORE ||
           type == CreatureType::APEX_PREDATOR ||
           type == CreatureType::SMALL_PREDATOR ||
           type == CreatureType::AERIAL_PREDATOR ||
           type == CreatureType::AQUATIC_PREDATOR ||
           type == CreatureType::AQUATIC_APEX;
}

std::string SpeciesNameGenerator::selectPrefix(const Genome& genome, CreatureType type) const {
    const std::vector<std::string>* prefixList = nullptr;

    // Handle special types first (SCAVENGER, PARASITE, CLEANER)
    if (type == CreatureType::SCAVENGER) {
        // Scavengers get shadow/dark prefixes (often nocturnal, opportunistic)
        prefixList = &m_shadowPrefixes;
    } else if (type == CreatureType::PARASITE) {
        // Parasites get shadow/thorn prefixes (small, invasive)
        if (genomeHash(genome, 10) > 0.5f) {
            prefixList = &m_shadowPrefixes;
        } else {
            prefixList = &m_thornPrefixes;
        }
    } else if (type == CreatureType::CLEANER) {
        // Cleaners get bright/light prefixes (symbiotic, visible)
        prefixList = &m_dawnPrefixes;
    }
    // Determine which prefix list to use based on traits
    else if (isAgile(genome)) {
        prefixList = &m_agilePrefixes;
    } else if (isHeavy(genome)) {
        prefixList = &m_heavyPrefixes;
    } else if (isAquatic(type)) {
        // Deep vs shallow water based on depth tolerance
        if (genome.preferredDepth > 0.3f) {
            prefixList = &m_reefPrefixes;
        } else {
            prefixList = &m_coralPrefixes;
        }
    } else if (isFlying(type)) {
        prefixList = &m_skyPrefixes;
    } else if (isNocturnal(genome)) {
        prefixList = &m_shadowPrefixes;
    } else if (isPredator(type)) {
        // Predators get ember/thorn prefixes based on aggression
        if (genomeHash(genome, 10) > 0.5f) {
            prefixList = &m_emberPrefixes;
        } else {
            prefixList = &m_thornPrefixes;
        }
    } else {
        // Default to nature/moss prefixes for herbivores
        float selector = genomeHash(genome, 11);
        if (selector < 0.25f) {
            prefixList = &m_mossPrefixes;
        } else if (selector < 0.5f) {
            prefixList = &m_dawnPrefixes;
        } else if (selector < 0.75f) {
            prefixList = &m_frostPrefixes;
        } else {
            prefixList = &m_thornPrefixes;
        }
    }

    // Select from the list using genome hash
    if (prefixList && !prefixList->empty()) {
        size_t index = static_cast<size_t>(genomeHash(genome, 0) * prefixList->size()) % prefixList->size();
        return (*prefixList)[index];
    }

    return "Common"; // Fallback
}

std::string SpeciesNameGenerator::selectSuffix(const Genome& genome, CreatureType type) const {
    const std::vector<std::string>* suffixList = nullptr;

    // Handle special types first
    if (type == CreatureType::SCAVENGER) {
        // Scavengers are slow, methodical
        suffixList = &m_crawlerSuffixes;
    } else if (type == CreatureType::PARASITE) {
        // Parasites are small and creeping
        suffixList = &m_crawlerSuffixes;
    } else if (type == CreatureType::CLEANER) {
        // Cleaners are small, agile swimmers or hoppers
        if (genome.speed > 12.0f) {
            suffixList = &m_hopperSuffixes;
        } else {
            suffixList = &m_swimmerSuffixes;
        }
    }
    // Flying types
    else if (isFlying(type)) {
        if (isPredator(type)) {
            // Aerial predators dive
            suffixList = &m_diverSuffixes;
        } else if (genome.glideRatio > 0.6f) {
            suffixList = &m_soarerSuffixes;
        } else {
            suffixList = &m_gliderSuffixes;
        }
    }
    // Aquatic types
    else if (isAquatic(type)) {
        if (isPredator(type)) {
            suffixList = &m_stalkerSuffixes;
        } else {
            suffixList = &m_swimmerSuffixes;
        }
    }
    // Terrestrial predators
    else if (isPredator(type)) {
        if (genome.speed > 15.0f) {
            suffixList = &m_runnerSuffixes;
        } else {
            suffixList = &m_stalkerSuffixes;
        }
    }
    // Fast herbivores
    else if (genome.speed > 15.0f) {
        suffixList = &m_runnerSuffixes;
    }
    // Small hoppers
    else if (genome.size < 0.6f && genome.speed > 10.0f) {
        suffixList = &m_hopperSuffixes;
    }
    // Slow creatures
    else if (genome.speed < 8.0f) {
        suffixList = &m_crawlerSuffixes;
    }
    // Default
    else {
        float selector = genomeHash(genome, 12);
        if (selector < 0.5f) {
            suffixList = &m_runnerSuffixes;
        } else {
            suffixList = &m_crawlerSuffixes;
        }
    }

    // Select from the list
    if (suffixList && !suffixList->empty()) {
        size_t index = static_cast<size_t>(genomeHash(genome, 1) * suffixList->size()) % suffixList->size();
        return (*suffixList)[index];
    }

    return "Walker"; // Fallback
}

std::string SpeciesNameGenerator::selectSpeciesWord(const Genome& genome, CreatureType type) const {
    const std::vector<std::string>* speciesList = nullptr;

    // Handle special types first
    if (type == CreatureType::SCAVENGER) {
        // Scavengers are bird-like (vulture) or mammal-like (hyena)
        if (genomeHash(genome, 13) > 0.5f) {
            speciesList = &m_birdSpecies;
        } else {
            speciesList = &m_mammalSpecies;
        }
    } else if (type == CreatureType::PARASITE) {
        // Parasites are insect-like or small creatures
        speciesList = &m_insectSpecies;
    } else if (type == CreatureType::CLEANER) {
        // Cleaners are fish-like (cleaner wrasse) or small creatures
        if (genomeHash(genome, 13) > 0.6f) {
            speciesList = &m_fishSpecies;
        } else {
            speciesList = &m_insectSpecies;
        }
    }
    // Flying types
    else if (isFlying(type)) {
        if (type == CreatureType::FLYING_INSECT) {
            speciesList = &m_insectSpecies;
        } else {
            speciesList = &m_birdSpecies;
        }
    }
    // Aquatic types
    else if (isAquatic(type)) {
        speciesList = &m_fishSpecies;
    }
    // Small creatures
    else if (genome.size < 0.6f) {
        // Small creatures can be insects or small mammals
        if (genomeHash(genome, 13) > 0.5f) {
            speciesList = &m_insectSpecies;
        } else {
            speciesList = &m_mammalSpecies;
        }
    }
    // Predators
    else if (isPredator(type)) {
        // Predators are mammal-like
        speciesList = &m_mammalSpecies;
    }
    // Herbivores
    else {
        // Herbivores can be any type
        float selector = genomeHash(genome, 14);
        if (selector < 0.35f) {
            speciesList = &m_reptileSpecies;
        } else if (selector < 0.7f) {
            speciesList = &m_mammalSpecies;
        } else {
            speciesList = &m_birdSpecies;
        }
    }

    // Select from the list
    if (speciesList && !speciesList->empty()) {
        size_t index = static_cast<size_t>(genomeHash(genome, 2) * speciesList->size()) % speciesList->size();
        return (*speciesList)[index];
    }

    return "Beast"; // Fallback
}

std::string SpeciesNameGenerator::generateName(const Genome& genome, CreatureType type) const {
    std::string prefix = selectPrefix(genome, type);
    std::string speciesWord = selectSpeciesWord(genome, type);
    std::string suffix = selectSuffix(genome, type);

    // Choose name format based on genome hash (variety in naming style)
    float format = genomeHash(genome, 20);

    if (format < 0.45f) {
        // Format: PrefixSpecies (e.g., "MossNewt", "EmberShrike")
        return prefix + speciesWord;
    } else if (format < 0.75f) {
        // Format: PrefixSuffix (e.g., "FrostGlider", "CoralSwimmer")
        return prefix + suffix;
    } else {
        // Format: Prefix Species (with space, e.g., "Moss Newt", "Ember Shrike")
        return prefix + " " + speciesWord;
    }
}

std::string SpeciesNameGenerator::generateNameWithSeed(const Genome& genome, CreatureType type, uint32_t seed) const {
    // Temporarily set seed, generate name, then restore
    std::mt19937 savedRng = m_rng;
    m_rng.seed(seed);
    std::string name = generateName(genome, type);
    m_rng = savedRng;
    return name;
}

// ============================================================================
// Biome and Theme Aware Naming
// ============================================================================

std::string SpeciesNameGenerator::generateNameWithBiome(const Genome& genome, CreatureType type, BiomeType biome) const {
    // Get biome-specific prefix
    const auto& biomePrefixes = getBiomePrefixes(biome);

    std::string prefix;
    if (!biomePrefixes.empty()) {
        // 40% chance to use biome prefix, 60% chance to use trait-based prefix
        float choice = genomeHash(genome, 30);
        if (choice < 0.4f) {
            size_t index = static_cast<size_t>(genomeHash(genome, 31) * biomePrefixes.size()) % biomePrefixes.size();
            prefix = biomePrefixes[index];
        } else {
            prefix = selectPrefix(genome, type);
        }
    } else {
        prefix = selectPrefix(genome, type);
    }

    std::string speciesWord = selectSpeciesWord(genome, type);
    std::string suffix = selectSuffix(genome, type);

    // Choose format
    float format = genomeHash(genome, 32);
    if (format < 0.5f) {
        return prefix + speciesWord;
    } else if (format < 0.8f) {
        return prefix + suffix;
    } else {
        return prefix + " " + speciesWord;
    }
}

std::string SpeciesNameGenerator::generateNameWithTheme(const Genome& genome, CreatureType type, BiomeType biome,
                                                         const PlanetTheme* theme) const {
    if (!theme) {
        return generateNameWithBiome(genome, type, biome);
    }

    // Get theme-specific prefixes based on planet preset
    auto themePrefixes = getThemePrefixes(theme->getData().preset);

    std::string prefix;
    float choice = genomeHash(genome, 33);

    // 30% theme prefix, 30% biome prefix, 40% trait prefix
    if (choice < 0.3f && !themePrefixes.empty()) {
        size_t index = static_cast<size_t>(genomeHash(genome, 34) * themePrefixes.size()) % themePrefixes.size();
        prefix = themePrefixes[index];
    } else if (choice < 0.6f) {
        const auto& biomePrefixes = getBiomePrefixes(biome);
        if (!biomePrefixes.empty()) {
            size_t index = static_cast<size_t>(genomeHash(genome, 35) * biomePrefixes.size()) % biomePrefixes.size();
            prefix = biomePrefixes[index];
        } else {
            prefix = selectPrefix(genome, type);
        }
    } else {
        prefix = selectPrefix(genome, type);
    }

    std::string speciesWord = selectSpeciesWord(genome, type);
    std::string suffix = selectSuffix(genome, type);

    // Choose format
    float format = genomeHash(genome, 36);
    if (format < 0.45f) {
        return prefix + speciesWord;
    } else if (format < 0.75f) {
        return prefix + suffix;
    } else {
        return prefix + " " + speciesWord;
    }
}

const std::vector<std::string>& SpeciesNameGenerator::getBiomePrefixes(BiomeType biome) const {
    // Map biomes to appropriate prefix lists
    switch (biome) {
        // Water biomes
        case BiomeType::DEEP_OCEAN:
        case BiomeType::OCEAN:
            return m_reefPrefixes;

        case BiomeType::SHALLOW_WATER:
        case BiomeType::CORAL_REEF:
        case BiomeType::KELP_FOREST:
            return m_coralPrefixes;

        // Coastal biomes
        case BiomeType::BEACH_SANDY:
        case BiomeType::BEACH_ROCKY:
        case BiomeType::TIDAL_POOL:
        case BiomeType::MANGROVE:
        case BiomeType::SALT_MARSH:
            return m_coralPrefixes;

        // Forest biomes
        case BiomeType::TROPICAL_RAINFOREST:
        case BiomeType::TEMPERATE_FOREST:
        case BiomeType::BOREAL_FOREST:
        case BiomeType::MOUNTAIN_FOREST:
            return m_mossPrefixes;

        // Grassland biomes
        case BiomeType::GRASSLAND:
        case BiomeType::SAVANNA:
            return m_dawnPrefixes;

        // Wet biomes
        case BiomeType::SWAMP:
        case BiomeType::WETLAND:
            return m_mossPrefixes;

        // Highland biomes
        case BiomeType::SHRUBLAND:
        case BiomeType::ALPINE_MEADOW:
        case BiomeType::ROCKY_HIGHLANDS:
            return m_thornPrefixes;

        // Cold biomes
        case BiomeType::TUNDRA:
        case BiomeType::GLACIER:
        case BiomeType::DESERT_COLD:
            return m_frostPrefixes;

        // Hot biomes
        case BiomeType::DESERT_HOT:
            return m_emberPrefixes;

        // Volcanic biomes
        case BiomeType::VOLCANIC:
        case BiomeType::LAVA_FIELD:
            return m_emberPrefixes;

        // Special biomes
        case BiomeType::CAVE_ENTRANCE:
            return m_shadowPrefixes;

        case BiomeType::RIVER_BANK:
        case BiomeType::LAKE_SHORE:
        case BiomeType::CRATER_LAKE:
            return m_coralPrefixes;

        default:
            return m_mossPrefixes;
    }
}

std::vector<std::string> SpeciesNameGenerator::getThemePrefixes(PlanetPreset preset) const {
    std::vector<std::string> prefixes;

    switch (preset) {
        case PlanetPreset::EARTH_LIKE:
            // Standard prefixes, use trait-based
            break;

        case PlanetPreset::ALIEN_PURPLE:
            prefixes = {"Violet", "Amethyst", "Lavender", "Plum", "Orchid", "Mauve", "Heliotrope", "Wisteria"};
            break;

        case PlanetPreset::ALIEN_RED:
            prefixes = {"Crimson", "Scarlet", "Vermillion", "Rust", "Cardinal", "Garnet", "Ruby", "Sanguine"};
            break;

        case PlanetPreset::ALIEN_BLUE:
            prefixes = {"Cerulean", "Sapphire", "Indigo", "Cobalt", "Lapis", "Ultramarine", "Teal", "Cyan"};
            break;

        case PlanetPreset::FROZEN_WORLD:
            prefixes = {"Glacier", "Permafrost", "Boreal", "Arctic", "Polar", "Cryogenic", "Frigid", "Hoarfrost"};
            break;

        case PlanetPreset::DESERT_WORLD:
            prefixes = {"Dune", "Arid", "Parched", "Sirocco", "Mesa", "Oasis", "Scorched", "Sandstorm"};
            break;

        case PlanetPreset::OCEAN_WORLD:
            prefixes = {"Pelagic", "Abyssal", "Benthic", "Tidal", "Nautical", "Brine", "Atoll", "Lagoon"};
            break;

        case PlanetPreset::VOLCANIC_WORLD:
            prefixes = {"Magma", "Obsidian", "Volcanic", "Basalt", "Pyroclastic", "Sulfuric", "Igneous", "Caldera"};
            break;

        case PlanetPreset::BIOLUMINESCENT:
            prefixes = {"Glow", "Lumina", "Phosphor", "Radiant", "Stellar", "Aurora", "Neon", "Prism"};
            break;

        case PlanetPreset::CRYSTAL_WORLD:
            prefixes = {"Crystal", "Quartz", "Geode", "Facet", "Prism", "Opal", "Diamond", "Lattice"};
            break;

        case PlanetPreset::TOXIC_WORLD:
            prefixes = {"Miasma", "Blight", "Caustic", "Venom", "Corrosive", "Noxious", "Acrid", "Murk"};
            break;

        case PlanetPreset::ANCIENT_WORLD:
            prefixes = {"Ancient", "Elder", "Primeval", "Archaic", "Fossil", "Relic", "Ancestral", "Epoch"};
            break;

        case PlanetPreset::CUSTOM:
            // Use standard trait-based naming
            break;
    }

    return prefixes;
}

} // namespace naming
