#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>
#include <cstdint>

namespace aquatic {

// Forward declarations
class AquaticCreature;
class SchoolingManager;

// ============================================================================
// Aquatic Species Classification
// ============================================================================

enum class AquaticSpecies : uint8_t {
    // Small fish (prey, schooling)
    SMALL_FISH,           // Generic schooling fish (sardines, anchovies)
    TROPICAL_FISH,        // Colorful reef fish
    MINNOW,               // Very small, fast breeding

    // Medium fish
    MEDIUM_FISH,          // Bass, trout-like
    REEF_FISH,            // Coral dwellers

    // Large predatory fish
    BARRACUDA,            // Fast ambush predator
    TUNA,                 // Endurance hunter
    SWORDFISH,            // High-speed striker

    // Apex predators
    SHARK_REEF,           // Small reef shark
    SHARK_HAMMERHEAD,     // Sensory specialist
    SHARK_GREAT_WHITE,    // Apex predator
    ORCA,                 // Intelligent pack hunter

    // Bottom dwellers (crustaceans & benthic)
    CRAB,                 // Scavenger, armored
    LOBSTER,              // Scavenger, territorial
    STARFISH,             // Slow, regenerative
    SEA_URCHIN,           // Defensive spines
    FLOUNDER,             // Camouflaged ambush
    RAY,                  // Bottom-feeding, venomous tail

    // Jellyfish & cnidarians
    JELLYFISH_SMALL,      // Moon jelly type
    JELLYFISH_LARGE,      // Lion's mane type
    JELLYFISH_BOX,        // Highly venomous
    PORTUGUESE_MAN_O_WAR, // Colonial organism
    SEA_ANEMONE,          // Sessile, symbiotic

    // Eels & serpentine
    MORAY_EEL,            // Ambush predator
    ELECTRIC_EEL,         // Stunning ability
    SEA_SNAKE,            // Venomous, air-breathing

    // Marine mammals
    DOLPHIN,              // Intelligent, social
    WHALE_HUMPBACK,       // Filter feeder, massive
    WHALE_SPERM,          // Deep diver, squid hunter
    SEAL,                 // Amphibious

    // Cephalopods
    OCTOPUS,              // Intelligent, camouflage master
    SQUID,                // Fast, schooling
    GIANT_SQUID,          // Deep water apex
    CUTTLEFISH,           // Color-changing

    // Filter feeders & plankton
    PLANKTON,             // Base of food chain
    KRILL,                // Small crustacean swarms
    WHALE_SHARK,          // Gentle giant filter feeder
    MANTA_RAY,            // Graceful filter feeder

    // Special
    ANGLERFISH,           // Deep water, bioluminescent lure
    PUFFERFISH,           // Defensive inflation
    SEAHORSE,             // Unique locomotion
    NAUTILUS,             // Ancient, spiral shell

    SPECIES_COUNT
};

// ============================================================================
// Aquatic Habitat Zones
// ============================================================================

enum class WaterZone : uint8_t {
    SURFACE,        // 0-5m - lots of light, air access
    EPIPELAGIC,     // 5-50m - sunlight zone
    MESOPELAGIC,    // 50-200m - twilight zone
    BATHYPELAGIC,   // 200-1000m - midnight zone
    ABYSSOPELAGIC,  // 1000m+ - abyss
    BENTHIC,        // Sea floor (any depth)
    REEF,           // Coral reef habitat
    COASTAL,        // Shallow coastal waters
    OPEN_OCEAN      // Pelagic/open water
};

// ============================================================================
// Aquatic Locomotion Types
// ============================================================================

enum class SwimLocomotion : uint8_t {
    CARANGIFORM,     // Body and tail undulation (most fish)
    THUNNIFORM,      // Tail-only propulsion (tuna, sharks)
    ANGUILLIFORM,    // Full-body serpentine (eels)
    LABRIFORM,       // Pectoral fin rowing (reef fish)
    OSTRACIFORM,     // Body rigid, tail/fins only (boxfish)
    RAJIFORM,        // Wing-like pectoral flapping (rays, mantas)
    MEDUSOID,        // Jet propulsion pulsing (jellyfish)
    JET_PROPULSION,  // Water jet (squid, octopus)
    WALKING,         // Leg-based (crabs, lobsters)
    DRIFTING,        // Passive drift (plankton)
    CETACEAN         // Vertical tail oscillation (whales, dolphins)
};

// ============================================================================
// Aquatic Sensory Capabilities
// ============================================================================

struct AquaticSenses {
    // Vision
    float visionRange = 30.0f;
    float visionAngle = 3.14f;      // radians (180 degrees default)
    float lowLightVision = 0.5f;    // 0-1, ability to see in darkness
    float uvVision = 0.0f;          // 0-1, UV light detection
    float polarizedVision = 0.0f;   // 0-1, polarization detection

    // Lateral line (water pressure/vibration)
    float lateralLineRange = 20.0f;
    float lateralLineSensitivity = 0.7f;

    // Electroreception (sharks, rays)
    float electroreceptionRange = 0.0f;  // 0 = none
    float electroreceptionSensitivity = 0.0f;

    // Echolocation (dolphins, whales)
    float echolocationRange = 0.0f;
    float echolocationPrecision = 0.0f;

    // Chemical senses
    float smellRange = 50.0f;
    float smellSensitivity = 0.6f;
    float tasteRange = 2.0f;

    // Magnetoreception (navigation)
    float magnetoreception = 0.0f;

    // Pressure sensing (depth)
    float pressureSensitivity = 0.5f;

    // Temperature sensing
    float thermoreception = 0.5f;
};

// ============================================================================
// Aquatic Physical Traits
// ============================================================================

struct AquaticPhysiology {
    // Body shape
    float bodyLength = 1.0f;
    float bodyWidth = 0.3f;         // Width/length ratio
    float bodyDepth = 0.2f;         // Depth/length ratio
    float streamlining = 0.7f;      // 0-1, affects drag

    // Fins
    float dorsalFinSize = 0.3f;
    float pectoralFinSize = 0.4f;
    float caudalFinSize = 0.5f;     // Tail fin
    float analFinSize = 0.2f;
    float pelvicFinSize = 0.2f;
    uint8_t finCount = 5;           // Number of distinct fins

    // Propulsion
    SwimLocomotion locomotion = SwimLocomotion::CARANGIFORM;
    float swimFrequency = 2.0f;     // Tail beats per second
    float swimAmplitude = 0.2f;     // Body wave amplitude
    float maxSpeed = 10.0f;
    float burstSpeedMultiplier = 2.5f;
    float acceleration = 5.0f;
    float turnRate = 2.0f;          // Radians per second

    // Buoyancy
    float neutralBuoyancyDepth = 10.0f;
    float swimbladderEfficiency = 0.8f;  // 0-1, 0 for sharks (must keep swimming)
    float oilLiverBuoyancy = 0.0f;       // Sharks use this instead

    // Respiration
    float gillEfficiency = 1.0f;
    float oxygenStorage = 1.0f;     // For breath-holding species
    bool ramVentilation = false;    // Must swim to breathe
    bool canBreathAir = false;      // Dolphins, whales
    float breathHoldDuration = 0.0f;// Seconds (0 = can't hold breath)

    // Depth tolerance
    float minDepth = 0.0f;
    float maxDepth = 100.0f;
    float preferredDepth = 20.0f;
    float pressureResistance = 1.0f;

    // Temperature tolerance
    float minTemp = 5.0f;
    float maxTemp = 30.0f;
    float preferredTemp = 20.0f;
    bool isEndothermic = false;     // Warm-blooded

    // Armor/defense
    float scaleArmor = 0.3f;
    float shellArmor = 0.0f;
    bool hasSpines = false;
    bool hasCamoflage = false;
    float toxicity = 0.0f;          // Venom/poison level
    float electricDischarge = 0.0f; // Electric eel

    // Size
    float mass = 1.0f;              // kg
    float metabolicRate = 1.0f;
};

// ============================================================================
// Aquatic Behavior Traits
// ============================================================================

struct AquaticBehavior {
    // Social
    float schoolingTendency = 0.0f;     // 0-1
    float preferredSchoolSize = 0.0f;   // 0 = solitary
    float schoolingDistance = 2.0f;     // Preferred distance to neighbors
    float territoriality = 0.0f;        // 0-1
    float territorySize = 0.0f;

    // Hunting
    float aggression = 0.5f;
    float huntingStyle = 0.0f;          // 0=ambush, 1=pursuit
    float packHuntingCoordination = 0.0f;
    float stalkingPatience = 0.5f;

    // Fleeing
    float flightResponse = 0.5f;
    float panicThreshold = 0.3f;
    float scatteringTendency = 0.5f;    // Break from school when threatened

    // Feeding
    float feedingFrequency = 1.0f;
    float filterFeedingRate = 0.0f;     // For filter feeders
    float grazingRate = 0.0f;           // For herbivores
    float predationRate = 0.0f;         // For predators

    // Migration
    float migrationTendency = 0.0f;
    float homeRange = 50.0f;
    float explorationTendency = 0.5f;

    // Reproduction
    float matingDisplayIntensity = 0.5f;
    float nestBuildingBehavior = 0.0f;
    float parentalCare = 0.0f;

    // Circadian
    float nocturnalTendency = 0.0f;     // 0=diurnal, 1=nocturnal
    float crepuscularActivity = 0.0f;   // Dawn/dusk activity

    // Special behaviors
    float cleaningBehavior = 0.0f;      // Cleaner fish
    float symbioticTendency = 0.0f;
    float mimicryAbility = 0.0f;
};

// ============================================================================
// Bioluminescence
// ============================================================================

struct Bioluminescence {
    bool enabled = false;
    glm::vec3 color = glm::vec3(0.0f, 0.8f, 1.0f);  // Default blue-green
    float intensity = 0.5f;
    float flashFrequency = 0.0f;    // 0 = constant glow

    enum class Pattern : uint8_t {
        NONE,
        CONSTANT_GLOW,
        PULSING,
        FLASHING,
        COUNTER_ILLUMINATION,   // Belly glow for camouflage
        LURE,                   // Anglerfish-style
        WARNING,                // Startling flash
        COMMUNICATION           // Signaling
    } pattern = Pattern::NONE;

    // Body regions
    bool bellyGlow = false;         // Counter-illumination
    bool eyeGlow = false;
    bool finEdgeGlow = false;
    bool spotPattern = false;
    bool lureAppendage = false;     // Anglerfish
};

// ============================================================================
// Aquatic Species Configuration
// ============================================================================

struct AquaticSpeciesConfig {
    AquaticSpecies species;
    std::string name;

    // Trophic level
    uint8_t trophicLevel = 2;       // 1=producer, 2=primary, 3=secondary, 4=apex

    // Habitat
    WaterZone primaryZone = WaterZone::EPIPELAGIC;
    std::vector<WaterZone> validZones;

    // Traits
    AquaticPhysiology physiology;
    AquaticSenses senses;
    AquaticBehavior behavior;
    Bioluminescence bioluminescence;

    // Diet
    std::vector<AquaticSpecies> preySpecies;
    std::vector<AquaticSpecies> predatorSpecies;
    bool isFilterFeeder = false;
    bool isHerbivore = false;
    bool isDetritivore = false;

    // Reproduction
    float reproductionThreshold = 150.0f;
    float reproductionCost = 60.0f;
    uint32_t offspringCount = 10;
    float offspringSurvivalRate = 0.1f;

    // Energy
    float baseEnergyDrain = 0.5f;
    float maxEnergy = 200.0f;
    float foodValue = 50.0f;        // Energy gained when eaten

    // Mesh generation hints
    glm::vec3 baseColor = glm::vec3(0.5f, 0.5f, 0.6f);
    float colorVariation = 0.2f;
    bool hasStripes = false;
    bool hasSpots = false;
    bool hasPatterns = true;
};

// ============================================================================
// Species Factory - Creates default configurations
// ============================================================================

class AquaticSpeciesFactory {
public:
    static AquaticSpeciesConfig createSpeciesConfig(AquaticSpecies species);

    // Individual species configs
    static AquaticSpeciesConfig createSmallFish();
    static AquaticSpeciesConfig createTropicalFish();
    static AquaticSpeciesConfig createMinnow();

    static AquaticSpeciesConfig createBarracuda();
    static AquaticSpeciesConfig createTuna();
    static AquaticSpeciesConfig createSwordfish();

    static AquaticSpeciesConfig createReefShark();
    static AquaticSpeciesConfig createHammerhead();
    static AquaticSpeciesConfig createGreatWhite();
    static AquaticSpeciesConfig createOrca();

    static AquaticSpeciesConfig createCrab();
    static AquaticSpeciesConfig createLobster();
    static AquaticSpeciesConfig createStarfish();
    static AquaticSpeciesConfig createRay();

    static AquaticSpeciesConfig createSmallJellyfish();
    static AquaticSpeciesConfig createLargeJellyfish();
    static AquaticSpeciesConfig createBoxJellyfish();

    static AquaticSpeciesConfig createMorayEel();
    static AquaticSpeciesConfig createElectricEel();

    static AquaticSpeciesConfig createDolphin();
    static AquaticSpeciesConfig createHumpbackWhale();
    static AquaticSpeciesConfig createSpermWhale();

    static AquaticSpeciesConfig createOctopus();
    static AquaticSpeciesConfig createSquid();
    static AquaticSpeciesConfig createGiantSquid();

    static AquaticSpeciesConfig createPlankton();
    static AquaticSpeciesConfig createKrill();
    static AquaticSpeciesConfig createWhaleShark();
    static AquaticSpeciesConfig createMantaRay();

    static AquaticSpeciesConfig createAnglerfish();
    static AquaticSpeciesConfig createPufferfish();
    static AquaticSpeciesConfig createSeahorse();

private:
    // Helper methods
    static void setupPredatorPreyRelations(AquaticSpeciesConfig& config, AquaticSpecies species);
    static void applyZoneDefaults(AquaticSpeciesConfig& config, WaterZone zone);
};

// ============================================================================
// Aquatic Creature State
// ============================================================================

struct AquaticCreatureState {
    // Position and movement
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 acceleration = glm::vec3(0.0f);
    glm::vec3 forward = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    float currentDepth = 0.0f;
    float targetDepth = 0.0f;
    float currentSpeed = 0.0f;

    // Swimming animation
    float swimPhase = 0.0f;
    float bodyWavePhase = 0.0f;
    float finPhase = 0.0f;
    bool isBursting = false;

    // Schooling
    uint32_t schoolId = 0;
    glm::vec3 schoolCenter = glm::vec3(0.0f);
    glm::vec3 schoolVelocity = glm::vec3(0.0f);
    uint32_t nearbyCount = 0;

    // Behavior state
    enum class BehaviorState : uint8_t {
        IDLE,
        CRUISING,
        SCHOOLING,
        HUNTING,
        FLEEING,
        FEEDING,
        MATING,
        MIGRATING,
        RESTING,
        SURFACING,
        DIVING,
        HIDING,
        DEFENDING_TERRITORY
    } behaviorState = BehaviorState::CRUISING;

    // Energy and health
    float energy = 100.0f;
    float health = 100.0f;
    float oxygenLevel = 1.0f;   // For air-breathers
    float stressLevel = 0.0f;

    // Timers
    float behaviorTimer = 0.0f;
    float breathTimer = 0.0f;
    float huntCooldown = 0.0f;
    float matingCooldown = 0.0f;

    // Statistics
    uint32_t preyEaten = 0;
    uint32_t timesEscaped = 0;
    float distanceTraveled = 0.0f;
    float age = 0.0f;

    // Visual state
    float biolumIntensity = 0.0f;
    glm::vec3 currentColor = glm::vec3(0.5f);
    float camoflageLevel = 0.0f;
    float inflationLevel = 0.0f;  // Pufferfish
    float inkCooldown = 0.0f;     // Cephalopods
};

// ============================================================================
// Aquatic Environment Info (passed to creatures)
// ============================================================================

struct AquaticEnvironment {
    // Water properties at position
    float waterSurfaceY = 0.0f;
    float seaFloorY = -100.0f;
    float currentDepth = 0.0f;

    // Currents
    glm::vec3 currentVelocity = glm::vec3(0.0f);
    float currentStrength = 0.0f;

    // Temperature
    float waterTemperature = 20.0f;

    // Light
    float lightLevel = 1.0f;
    float visibilityRange = 50.0f;

    // Oxygen
    float oxygenLevel = 1.0f;

    // Pressure (increases with depth)
    float pressure = 1.0f;

    // Nearby entities
    std::vector<AquaticCreature*> nearbyCreatures;
    std::vector<glm::vec3> foodSources;
    std::vector<glm::vec3> threats;
    std::vector<glm::vec3> potentialMates;

    // Terrain info
    glm::vec3 nearestReef;
    glm::vec3 nearestCave;
    float distanceToSurface = 0.0f;
    float distanceToFloor = 0.0f;
};

// ============================================================================
// Utility Functions
// ============================================================================

const char* getSpeciesName(AquaticSpecies species);
WaterZone getDepthZone(float depth);
float getZoneMinDepth(WaterZone zone);
float getZoneMaxDepth(WaterZone zone);
bool isSpeciesCompatibleWithZone(AquaticSpecies species, WaterZone zone);
bool canSpeciesEat(AquaticSpecies predator, AquaticSpecies prey);
float getSpeciesBaseSize(AquaticSpecies species);
glm::vec3 getSpeciesBaseColor(AquaticSpecies species);

} // namespace aquatic
