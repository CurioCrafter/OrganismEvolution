#pragma once

#include <cstdint>
#include <string>

namespace small {

// Size categories for small creatures (in meters)
enum class SizeCategory : uint8_t {
    MICROSCOPIC,  // < 1mm (mites, small insects)
    TINY,         // 1mm - 1cm (ants, beetles, spiders)
    SMALL,        // 1cm - 10cm (mice, frogs, large insects)
    MEDIUM        // 10cm - 30cm (squirrels, rabbits, snakes)
};

// Locomotion types
enum class LocomotionType : uint8_t {
    WALKING,      // Legs on ground
    CRAWLING,     // Body contact with ground (worms, snakes)
    CLIMBING,     // Vertical surface movement
    JUMPING,      // Hop-based movement (frogs, grasshoppers)
    BURROWING,    // Underground movement
    FLYING,       // Winged flight
    SWIMMING,     // Aquatic movement
    GLIDING       // Passive aerial descent
};

// Habitat types
enum class HabitatType : uint8_t {
    UNDERGROUND,   // Burrows, tunnels
    GROUND_SURFACE,// On ground
    GRASS,         // In grass/low vegetation
    BUSH,          // In bushes/shrubs
    TREE_TRUNK,    // On tree trunks
    CANOPY,        // In tree branches/leaves
    WATER_SURFACE, // On water surface
    UNDERWATER,    // Below water surface
    AERIAL         // Flying in air
};

// Social structure
enum class SocialType : uint8_t {
    SOLITARY,     // Lives alone
    PAIR,         // Lives in pairs
    FAMILY,       // Small family groups
    COLONY,       // Large organized colonies (ants, bees)
    SWARM,        // Loose aggregations (flies, locusts)
    HERD          // Moving groups (rabbits)
};

// Main small creature type enumeration
enum class SmallCreatureType : uint16_t {
    // ============ INSECTS - GROUND ============
    ANT_WORKER = 0,
    ANT_SOLDIER,
    ANT_QUEEN,
    BEETLE_GROUND,
    BEETLE_DUNG,
    BEETLE_CARRION,
    EARTHWORM,
    CENTIPEDE,
    MILLIPEDE,
    CRICKET,
    GRASSHOPPER,
    COCKROACH,
    TERMITE_WORKER,
    TERMITE_SOLDIER,
    TERMITE_QUEEN,

    // ============ INSECTS - FLYING ============
    FLY = 100,
    MOSQUITO,
    GNAT,
    BUTTERFLY,
    MOTH,
    BEE_WORKER,
    BEE_DRONE,
    BEE_QUEEN,
    WASP,
    HORNET,
    DRAGONFLY,
    DAMSELFLY,
    FIREFLY,
    CICADA,
    LOCUST,

    // ============ ARACHNIDS ============
    SPIDER_ORB_WEAVER = 200,
    SPIDER_JUMPING,
    SPIDER_WOLF,
    SPIDER_TRAPDOOR,
    SCORPION,
    TICK,
    MITE,
    HARVESTMAN,

    // ============ SMALL MAMMALS ============
    MOUSE = 300,
    RAT,
    SHREW,
    VOLE,
    MOLE,
    SQUIRREL_GROUND,
    SQUIRREL_TREE,
    CHIPMUNK,
    RABBIT,
    HARE,
    BAT_SMALL,
    BAT_LARGE,
    HEDGEHOG,

    // ============ REPTILES ============
    LIZARD_SMALL = 400,
    GECKO,
    SKINK,
    CHAMELEON,
    SNAKE_SMALL,
    SNAKE_MEDIUM,
    TURTLE_SMALL,

    // ============ AMPHIBIANS ============
    FROG = 500,
    TOAD,
    TREE_FROG,
    SALAMANDER,
    NEWT,

    // ============ OTHER INVERTEBRATES ============
    SNAIL = 600,
    SLUG,
    CRAB_SMALL,
    CRAYFISH,

    COUNT  // For iteration
};

// Structure to hold creature type properties
struct SmallCreatureProperties {
    SizeCategory sizeCategory;
    LocomotionType primaryLocomotion;
    LocomotionType secondaryLocomotion;  // Alternative movement
    HabitatType primaryHabitat;
    SocialType socialType;

    float minSize;        // Minimum size in meters
    float maxSize;        // Maximum size in meters
    float baseSpeed;      // Base movement speed m/s
    float energyRate;     // Energy consumption rate
    float reproductionRate; // Reproduction frequency

    bool canFly;
    bool canSwim;
    bool canClimb;
    bool canBurrow;
    bool canJump;
    bool isVenomous;
    bool isPoisonous;     // Passive defense
    bool hasExoskeleton;
    bool undergoesMorphosis;
    bool isNocturnal;
    bool isColonial;
    bool isPollinator;
    bool isDecomposer;
    bool isPredator;
    bool isParasite;
};

// Get properties for a creature type
const SmallCreatureProperties& getProperties(SmallCreatureType type);

// Helper functions
inline bool isInsect(SmallCreatureType type) {
    uint16_t t = static_cast<uint16_t>(type);
    return t < 200;
}

inline bool isGroundInsect(SmallCreatureType type) {
    uint16_t t = static_cast<uint16_t>(type);
    return t < 100;
}

inline bool isFlyingInsect(SmallCreatureType type) {
    uint16_t t = static_cast<uint16_t>(type);
    return t >= 100 && t < 200;
}

inline bool isArachnid(SmallCreatureType type) {
    uint16_t t = static_cast<uint16_t>(type);
    return t >= 200 && t < 300;
}

inline bool isSmallMammal(SmallCreatureType type) {
    uint16_t t = static_cast<uint16_t>(type);
    return t >= 300 && t < 400;
}

inline bool isReptile(SmallCreatureType type) {
    uint16_t t = static_cast<uint16_t>(type);
    return t >= 400 && t < 500;
}

inline bool isAmphibian(SmallCreatureType type) {
    uint16_t t = static_cast<uint16_t>(type);
    return t >= 500 && t < 600;
}

inline bool isInvertebrate(SmallCreatureType type) {
    uint16_t t = static_cast<uint16_t>(type);
    return t < 300 || t >= 600;
}

inline bool isAnt(SmallCreatureType type) {
    return type == SmallCreatureType::ANT_WORKER ||
           type == SmallCreatureType::ANT_SOLDIER ||
           type == SmallCreatureType::ANT_QUEEN;
}

inline bool isBee(SmallCreatureType type) {
    return type == SmallCreatureType::BEE_WORKER ||
           type == SmallCreatureType::BEE_DRONE ||
           type == SmallCreatureType::BEE_QUEEN;
}

inline bool isTermite(SmallCreatureType type) {
    return type == SmallCreatureType::TERMITE_WORKER ||
           type == SmallCreatureType::TERMITE_SOLDIER ||
           type == SmallCreatureType::TERMITE_QUEEN;
}

inline bool isSpider(SmallCreatureType type) {
    uint16_t t = static_cast<uint16_t>(type);
    return t >= 200 && t < 205;
}

inline bool isColonialInsect(SmallCreatureType type) {
    return isAnt(type) || isBee(type) || isTermite(type);
}

inline bool canFormSwarm(SmallCreatureType type) {
    return type == SmallCreatureType::LOCUST ||
           type == SmallCreatureType::FLY ||
           type == SmallCreatureType::GNAT ||
           type == SmallCreatureType::MOSQUITO;
}

// Get display name for a creature type
std::string getTypeName(SmallCreatureType type);

// Get trophic level (0 = producer equivalent, 1 = primary consumer, 2 = secondary, 3 = tertiary)
int getTrophicLevel(SmallCreatureType type);

// Check if type A can eat type B
bool canEat(SmallCreatureType predator, SmallCreatureType prey);

// Check if type can be eaten by large creatures (for ecosystem integration)
bool isPreyForLargeCreatures(SmallCreatureType type);

} // namespace small
