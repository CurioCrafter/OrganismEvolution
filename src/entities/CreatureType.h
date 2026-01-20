#pragma once

#include <string>

// Types of food sources from producers (shared between systems)
enum class FoodSourceType {
    GRASS,      // Fast regrowth, low energy
    BUSH_BERRY, // Medium regrowth, medium energy
    TREE_FRUIT, // Slow regrowth, high energy
    TREE_LEAF,  // Medium regrowth, medium energy (for browsers)
    CARRION,    // From decomposer system (for scavengers)

    // Aquatic food sources
    PLANKTON,   // Tiny floating organisms - filter feeders (fast regrowth)
    ALGAE,      // Underwater plant matter - herbivore fish (medium regrowth)
    SEAWEED,    // Larger underwater plants - larger herbivores (medium regrowth)
    KELP        // Kelp forest food - large herbivores (slow regrowth, high energy)
};

// Expanded creature types for multi-trophic ecosystem
enum class CreatureType {
    // Trophic Level 2: Primary Consumers (Herbivores)
    GRAZER,         // Eats grass only - cow/deer analog
    BROWSER,        // Eats tree leaves and bushes - giraffe analog
    FRUGIVORE,      // Eats fruits/berries - small mammal analog

    // Trophic Level 3: Secondary Consumers
    SMALL_PREDATOR, // Hunts small herbivores (frugivores) - fox analog
    OMNIVORE,       // Eats plants AND small creatures - bear analog

    // Trophic Level 4: Tertiary Consumers
    APEX_PREDATOR,  // Hunts all herbivores and small predators - wolf/lion analog
    SCAVENGER,      // Eats corpses - vulture analog

    // Special Types
    PARASITE,       // Attaches to hosts, drains energy
    CLEANER,        // Removes parasites from hosts (symbiont)

    // Aerial Types
    FLYING,         // Generic flying creature (legacy - omnivore, eats food + small herbivores)
    FLYING_BIRD,    // Bird - feathered wings, 2 legs, high glide ratio, soaring
    FLYING_INSECT,  // Insect - membrane/chitin wings, 4-6 legs, fast wing beats, agile
    AERIAL_PREDATOR, // Aerial apex predator - hawk/eagle analog, dives from above

    // Aquatic Types (water-dwelling)
    AQUATIC,              // Generic fish (legacy - schools together)
    AQUATIC_HERBIVORE,    // Small fish that eat algae/plants - minnow/guppy analog
    AQUATIC_PREDATOR,     // Predatory fish - bass/pike analog
    AQUATIC_APEX,         // Apex aquatic predator - shark analog
    AMPHIBIAN,            // Can survive both land and water - frog/salamander analog

    // Legacy types (for backward compatibility)
    HERBIVORE = GRAZER,
    CARNIVORE = APEX_PREDATOR
};

// Diet type for determining what a creature can eat
enum class DietType {
    GRASS_ONLY,         // Grazers
    BROWSE_ONLY,        // Browsers (leaves, twigs)
    FRUIT_ONLY,         // Frugivores
    PLANT_GENERALIST,   // Can eat any plant matter
    SMALL_PREY,         // Hunts small creatures only
    LARGE_PREY,         // Hunts large creatures
    ALL_PREY,           // Hunts any creature
    CARRION,            // Eats dead creatures only
    OMNIVORE_FLEX,      // Plants + small prey, switches based on need
    PARASITE_DRAIN,     // Drains host energy
    CLEANER_SERVICE,    // Gets energy from cleaning parasites
    AQUATIC_FILTER,     // Filter feeder (plankton/algae in water)
    AQUATIC_ALGAE,      // Eats underwater algae/plants
    AQUATIC_SMALL_PREY, // Hunts small aquatic creatures
    AQUATIC_ALL_PREY    // Hunts all aquatic creatures (apex)
};

// Trophic level for energy calculations
enum class TrophicLevel {
    PRODUCER = 1,           // Plants (not a creature type)
    PRIMARY_CONSUMER = 2,   // Herbivores
    SECONDARY_CONSUMER = 3, // Small predators, omnivores
    TERTIARY_CONSUMER = 4,  // Apex predators
    DECOMPOSER = 0          // Detritivores (special case)
};

struct CreatureTraits {
    CreatureType type;
    DietType diet;
    TrophicLevel trophicLevel;

    // Combat/interaction parameters
    float attackRange;      // Melee range for predators
    float attackDamage;     // Damage per second
    float fleeDistance;     // Distance to start fleeing
    float huntingEfficiency; // How well this creature converts prey to energy (0.05-0.15)

    // Size constraints (affects what can hunt/be hunted)
    float minPreySize;      // Minimum size of prey this creature can hunt
    float maxPreySize;      // Maximum size of prey this creature can hunt

    // Social parameters
    bool isPackHunter;      // Hunts in groups
    bool isHerdAnimal;      // Forms herds for protection
    bool isTerritorial;     // Defends territory

    // Special abilities
    bool canClimb;          // Can reach tree food
    bool canDigest[3];      // Can digest: [grass, leaves, fruit]
    float parasiteResistance; // Resistance to parasite infection (0-1)

    CreatureTraits()
        : type(CreatureType::GRAZER)
        , diet(DietType::GRASS_ONLY)
        , trophicLevel(TrophicLevel::PRIMARY_CONSUMER)
        , attackRange(2.5f)
        , attackDamage(15.0f)
        , fleeDistance(35.0f)
        , huntingEfficiency(0.10f)
        , minPreySize(0.0f)
        , maxPreySize(0.0f)
        , isPackHunter(false)
        , isHerdAnimal(true)
        , isTerritorial(false)
        , canClimb(false)
        , canDigest{true, false, false}
        , parasiteResistance(0.5f)
    {}

    // Factory methods for each creature type
    static CreatureTraits getTraitsFor(CreatureType type);
};

// Helper function to get readable name
inline const char* getCreatureTypeName(CreatureType type) {
    switch (type) {
        case CreatureType::GRAZER: return "Grazer";
        case CreatureType::BROWSER: return "Browser";
        case CreatureType::FRUGIVORE: return "Frugivore";
        case CreatureType::SMALL_PREDATOR: return "Small Predator";
        case CreatureType::OMNIVORE: return "Omnivore";
        case CreatureType::APEX_PREDATOR: return "Apex Predator";
        case CreatureType::SCAVENGER: return "Scavenger";
        case CreatureType::PARASITE: return "Parasite";
        case CreatureType::CLEANER: return "Cleaner";
        case CreatureType::FLYING: return "Flying";
        case CreatureType::FLYING_BIRD: return "Bird";
        case CreatureType::FLYING_INSECT: return "Insect";
        case CreatureType::AERIAL_PREDATOR: return "Aerial Predator";
        case CreatureType::AQUATIC: return "Fish";
        case CreatureType::AQUATIC_HERBIVORE: return "Small Fish";
        case CreatureType::AQUATIC_PREDATOR: return "Predator Fish";
        case CreatureType::AQUATIC_APEX: return "Shark";
        case CreatureType::AMPHIBIAN: return "Amphibian";
        default: return "Unknown";
    }
}

// Helper to check if type is a herbivore
inline bool isHerbivore(CreatureType type) {
    return type == CreatureType::GRAZER ||
           type == CreatureType::BROWSER ||
           type == CreatureType::FRUGIVORE;
}

// Helper to check if type is a predator
inline bool isPredator(CreatureType type) {
    return type == CreatureType::SMALL_PREDATOR ||
           type == CreatureType::APEX_PREDATOR ||
           type == CreatureType::OMNIVORE ||
           type == CreatureType::FLYING ||           // Flying creatures can hunt small prey
           type == CreatureType::AERIAL_PREDATOR;    // Aerial predators are hunters
}

// Helper to check if type is a flying creature
inline bool isFlying(CreatureType type) {
    return type == CreatureType::FLYING ||
           type == CreatureType::FLYING_BIRD ||
           type == CreatureType::FLYING_INSECT ||
           type == CreatureType::AERIAL_PREDATOR;
}

// Helper to check if flying creature is a bird type
inline bool isBirdType(CreatureType type) {
    return type == CreatureType::FLYING_BIRD ||
           type == CreatureType::AERIAL_PREDATOR;  // Raptors are birds
}

// Helper to check if flying creature is an insect type
inline bool isInsectType(CreatureType type) {
    return type == CreatureType::FLYING_INSECT;
}

// Helper to check if type is an aquatic creature
inline bool isAquatic(CreatureType type) {
    return type == CreatureType::AQUATIC ||
           type == CreatureType::AQUATIC_HERBIVORE ||
           type == CreatureType::AQUATIC_PREDATOR ||
           type == CreatureType::AQUATIC_APEX ||
           type == CreatureType::AMPHIBIAN;
}

// Helper to check if aquatic creature is a predator
inline bool isAquaticPredator(CreatureType type) {
    return type == CreatureType::AQUATIC_PREDATOR ||
           type == CreatureType::AQUATIC_APEX;
}

// Helper to check if aquatic creature is prey
inline bool isAquaticPrey(CreatureType type) {
    return type == CreatureType::AQUATIC ||
           type == CreatureType::AQUATIC_HERBIVORE;
}

// Helper to check if creature can survive on land
inline bool canSurviveOnLand(CreatureType type) {
    return type == CreatureType::AMPHIBIAN ||
           !isAquatic(type);  // All non-aquatic can survive on land
}

// Helper to check if creature can survive in water
inline bool canSurviveInWater(CreatureType type) {
    return isAquatic(type) ||
           type == CreatureType::AMPHIBIAN;
}

// Helper to check if type can be hunted by another
inline bool canBeHuntedBy(CreatureType prey, CreatureType predator, float preySize) {
    switch (predator) {
        case CreatureType::SMALL_PREDATOR:
            // Can hunt small herbivores and opportunistically snatch small flyers
            return (prey == CreatureType::FRUGIVORE && preySize < 1.0f) ||
                   (prey == CreatureType::FLYING_INSECT) ||
                   (prey == CreatureType::FLYING_BIRD && preySize < 0.9f);

        case CreatureType::APEX_PREDATOR:
            // Can hunt all herbivores, small predators, and aquatic creatures (when near surface)
            return isHerbivore(prey) || prey == CreatureType::SMALL_PREDATOR || prey == CreatureType::AQUATIC ||
                   (prey == CreatureType::FLYING_BIRD && preySize < 1.2f) ||
                   (prey == CreatureType::FLYING_INSECT);

        case CreatureType::OMNIVORE:
            // Can hunt small creatures when in predator mode
            return (prey == CreatureType::FRUGIVORE && preySize < 1.2f) ||
                   (prey == CreatureType::FLYING_INSECT) ||
                   (prey == CreatureType::FLYING_BIRD && preySize < 0.7f);

        case CreatureType::FLYING:
        case CreatureType::FLYING_BIRD:
            // Flying creatures can hunt small herbivores from above
            return (prey == CreatureType::FRUGIVORE && preySize < 0.8f) ||
                   (prey == CreatureType::FLYING_INSECT);

        case CreatureType::FLYING_INSECT:
            // Predatory insects target smaller insects
            return (prey == CreatureType::FLYING_INSECT && preySize < 0.3f);

        case CreatureType::AERIAL_PREDATOR:
            // Aerial predators (hawks, eagles) can hunt all small creatures
            // Can hunt small herbivores, small predators, other flying creatures, and insects
            return (prey == CreatureType::FRUGIVORE && preySize < 1.2f) ||
                   (prey == CreatureType::SMALL_PREDATOR && preySize < 0.8f) ||
                   (prey == CreatureType::FLYING_BIRD && preySize < 0.9f) ||
                   (prey == CreatureType::FLYING_INSECT) ||  // Can always catch insects
                   (prey == CreatureType::FLYING && preySize < 0.9f);

        case CreatureType::AQUATIC_PREDATOR:
            // Predatory fish can hunt small fish
            return (prey == CreatureType::AQUATIC_HERBIVORE && preySize < 0.8f) ||
                   (prey == CreatureType::AQUATIC && preySize < 0.7f);

        case CreatureType::AQUATIC_APEX:
            // Sharks can hunt all smaller aquatic creatures
            return (isAquatic(prey) && prey != CreatureType::AQUATIC_APEX && preySize < 1.5f) ||
                   (prey == CreatureType::AMPHIBIAN && preySize < 1.0f);

        default:
            return false;
    }
}

// Helper to check if aquatic creature can be hunted by another aquatic creature
inline bool canBeHuntedByAquatic(CreatureType prey, CreatureType predator, float preySize) {
    if (!isAquatic(predator)) return false;

    switch (predator) {
        case CreatureType::AQUATIC_PREDATOR:
            return (prey == CreatureType::AQUATIC_HERBIVORE && preySize < 0.8f) ||
                   (prey == CreatureType::AQUATIC && preySize < 0.7f);

        case CreatureType::AQUATIC_APEX:
            return isAquatic(prey) && prey != CreatureType::AQUATIC_APEX && preySize < 1.5f;

        default:
            return false;
    }
}

// =============================================================================
// AQUATIC SPAWN DEPTHS - Preferred depth bands for each aquatic creature type
// =============================================================================

// Forward declare DepthBand (defined in SwimBehavior.h)
enum class DepthBand;

// Get preferred depth band for aquatic creature spawning
// Returns the primary depth band where this species should spawn
inline int getPreferredDepthBandIndex(CreatureType type) {
    switch (type) {
        // Herbivore fish - shallow/mid-water for algae and plants
        case CreatureType::AQUATIC:
        case CreatureType::AQUATIC_HERBIVORE:
            return 2;  // MID_WATER (5-25m) - common schooling fish

        // Predatory fish - mid-water to hunt schooling fish
        case CreatureType::AQUATIC_PREDATOR:
            return 2;  // MID_WATER (5-25m) - bass, pike

        // Apex predators - deep water with occasional mid-water hunting
        case CreatureType::AQUATIC_APEX:
            return 3;  // DEEP (25-50m) - sharks

        // Amphibians - surface/shallow for air access
        case CreatureType::AMPHIBIAN:
            return 1;  // SHALLOW (2-5m) - frogs, salamanders

        default:
            return 2;  // Default to mid-water
    }
}

// Get spawn depth range for aquatic creature type
inline void getAquaticSpawnDepthRange(CreatureType type, float& minDepth, float& maxDepth) {
    switch (type) {
        case CreatureType::AQUATIC:
        case CreatureType::AQUATIC_HERBIVORE:
            minDepth = 3.0f;  maxDepth = 15.0f;  // Small fish, upper mid-water
            break;

        case CreatureType::AQUATIC_PREDATOR:
            minDepth = 5.0f;  maxDepth = 25.0f;  // Hunting fish, full mid-water
            break;

        case CreatureType::AQUATIC_APEX:
            minDepth = 10.0f; maxDepth = 40.0f;  // Large predators, deep
            break;

        case CreatureType::AMPHIBIAN:
            minDepth = 0.5f;  maxDepth = 4.0f;   // Surface-dwellers
            break;

        default:
            minDepth = 5.0f;  maxDepth = 20.0f;  // Generic mid-water
            break;
    }
}

// Calculate spawn depth for a creature given water depth and terrain
inline float calculateAquaticSpawnDepth(CreatureType type, float availableWaterDepth, float randomValue01) {
    float minDepth, maxDepth;
    getAquaticSpawnDepthRange(type, minDepth, maxDepth);

    // Clamp to available water depth (leave 0.5m buffer from floor)
    float floorBuffer = 0.5f;
    float surfaceBuffer = 0.3f;
    maxDepth = std::min(maxDepth, availableWaterDepth - floorBuffer);
    minDepth = std::min(minDepth, maxDepth - 1.0f);
    minDepth = std::max(minDepth, surfaceBuffer);

    if (maxDepth <= minDepth) {
        return (minDepth + maxDepth) * 0.5f;  // Not enough depth, spawn in middle
    }

    return minDepth + randomValue01 * (maxDepth - minDepth);
}
