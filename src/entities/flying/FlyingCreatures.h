#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>
#include <memory>
#include <functional>

// Forward declarations
class Creature;
class Terrain;

namespace flying {

// ============================================================================
// Flying Creature Subtypes - More specific than CreatureType
// ============================================================================

enum class FlyingSubtype {
    // Birds - feathered wings, endothermic
    SONGBIRD,           // Small perching birds (sparrows, finches, robins)
    CORVID,             // Intelligent social birds (crows, ravens, jays)
    RAPTOR_SMALL,       // Small raptors (kestrel, sparrowhawk)
    RAPTOR_LARGE,       // Large raptors (eagle, hawk, osprey)
    OWL,                // Nocturnal hunters with silent flight
    SEABIRD,            // Oceanic gliders (albatross, gull)
    WATERFOWL,          // Duck, goose - can land on water
    HUMMINGBIRD,        // Tiny hovering birds
    VULTURE,            // Scavenging soarers

    // Insects - exoskeleton, compound eyes
    BUTTERFLY,          // Slow flapping, colorful
    MOTH,               // Nocturnal butterflies
    DRAGONFLY,          // Fast agile predators with 4 independent wings
    DAMSELFLY,          // Delicate dragonfly relatives
    BEE,                // Social pollinators
    WASP,               // Predatory bees
    BEETLE,             // Hard shell, clumsy flight
    FLY,                // Fast 2-wing flyers
    MOSQUITO,           // Blood feeders
    LOCUST,             // Swarm formers

    // Bats - membrane wings, echolocation
    MICROBAT,           // Small insectivorous bats
    FRUIT_BAT,          // Large frugivorous bats (flying foxes)
    VAMPIRE_BAT,        // Blood feeders

    // Fantastical (if enabled)
    PTEROSAUR,          // Prehistoric flying reptile
    DRAGON_SMALL,       // Small fantasy dragon
    DRAGON_LARGE,       // Large fantasy dragon
    PHOENIX,            // Mythical fire bird
    GRIFFIN,            // Lion-eagle hybrid

    // Swarm types (special collective behavior)
    SWARM_LOCUST,       // Massive locust swarms
    SWARM_MOSQUITO,     // Mosquito clouds
    SWARM_STARLING      // Murmuration flocks
};

// ============================================================================
// Wing Structure Types
// ============================================================================

enum class WingStructure {
    FEATHERED_BROAD,    // Eagle, hawk - soaring
    FEATHERED_POINTED,  // Swift, falcon - speed
    FEATHERED_ROUNDED,  // Owl - silent, maneuverable
    FEATHERED_LONG,     // Albatross - dynamic soaring
    MEMBRANE_BAT,       // Bat - flexible membrane
    MEMBRANE_PTEROSAUR, // Pterosaur - single finger support
    MEMBRANE_DRAGON,    // Dragon - multiple finger support
    INSECT_DIPTERA,     // 2-wing insects (flies, mosquitoes)
    INSECT_LEPIDOPTERA, // Scaled wings (butterflies, moths)
    INSECT_ODONATA,     // 4 independent wings (dragonflies)
    INSECT_HYMENOPTERA, // 4 coupled wings (bees, wasps)
    INSECT_COLEOPTERA   // Hardened forewings + flying hindwings (beetles)
};

// ============================================================================
// Flight Specializations
// ============================================================================

enum class FlightSpecialization {
    SOARING,            // Thermal/dynamic soaring (low energy)
    POWERED,            // Sustained flapping flight
    HOVERING,           // Stationary flight (hummingbirds, some insects)
    GLIDING,            // Unpowered descent
    DIVING,             // High-speed attack dives
    ACROBATIC,          // Highly maneuverable
    MIGRATORY,          // Long-distance endurance
    BURST,              // Short explosive flight (pheasant-like)
    SILENT              // Owl-like noise reduction
};

// ============================================================================
// Flying Creature Genome Extension
// ============================================================================

struct FlyingGenome {
    // Wing morphology
    float wingSpan;             // Total wingspan (relative to body)
    float wingChord;            // Wing width (affects area)
    float aspectRatio;          // Span^2 / Area (efficiency)
    float wingLoading;          // Mass / wing area (affects speed)
    float camber;               // Wing curvature (lift coefficient)
    float wingTaper;            // Tip-to-root chord ratio
    float wingTwist;            // Washout angle (stall protection)
    float dihedralAngle;        // Upward V-angle (stability)
    float sweepAngle;           // Backward sweep (high speed)

    // Tail configuration
    float tailLength;           // Relative to body
    float tailSpan;             // Horizontal tail width
    float tailFork;             // 0=square, 1=deeply forked
    bool hasTailFeathers;       // Birds vs bats/insects

    // Body shape for aerodynamics
    float bodyStreamlining;     // 0=round, 1=torpedo
    float neckLength;           // For reach/vision
    float legLength;            // For landing gear
    bool retractableLegs;       // Tuck legs during flight

    // Flight muscles
    float breastMuscleRatio;    // Power-to-weight
    float supracoracoideus;     // Upstroke muscle (for hovering)
    float fastTwitchRatio;      // Fast vs slow muscle fibers

    // Sensory adaptations
    float visualAcuity;         // Detail detection range
    float motionSensitivity;    // Movement detection
    float uvVision;             // UV perception (flowers, trails)
    float nightVision;          // Low-light adaptation
    float echolocationStrength; // For bats
    float magneticSense;        // Migration navigation
    float pressureSense;        // Weather prediction

    // Behavioral traits
    float flockingStrength;     // How strongly follows flock
    float territorialRadius;    // Defense zone size
    float migratoryUrge;        // Seasonal movement drive
    float nocturnality;         // 0=day, 1=night active
    float aggressionLevel;      // Combat readiness
    float curiosity;            // Exploration tendency

    // Coloration (for display, camouflage, thermoregulation)
    glm::vec3 primaryColor;
    glm::vec3 secondaryColor;
    glm::vec3 accentColor;
    float iridescence;          // Angle-dependent color
    float patternComplexity;    // Markings detail

    // Initialize with defaults
    FlyingGenome();

    // Initialize for specific subtypes
    static FlyingGenome forSubtype(FlyingSubtype subtype);

    // Crossover and mutation
    static FlyingGenome crossover(const FlyingGenome& a, const FlyingGenome& b);
    void mutate(float rate, float strength);

    // Calculate derived stats
    float calculateLiftCoefficient() const;
    float calculateDragCoefficient() const;
    float calculateStallSpeed(float mass) const;
    float calculateMaxSpeed() const;
    float calculateTurnRadius(float speed) const;
    float calculateClimbRate() const;
    float calculateGlideRatio() const;
};

// ============================================================================
// Flying Creature State
// ============================================================================

struct FlyingState {
    // Position and orientation
    glm::vec3 position;
    glm::vec3 velocity;
    glm::quat orientation;
    float altitude;             // Height above terrain
    float groundClearance;      // Immediate ground distance

    // Flight dynamics
    float airSpeed;
    float groundSpeed;
    float verticalSpeed;
    float angleOfAttack;
    float sideslipAngle;
    float bankAngle;
    float pitchAngle;
    float yawRate;

    // Control inputs (0-1 or -1 to 1)
    float throttle;             // Flap intensity
    float pitch;                // Nose up/down
    float roll;                 // Bank left/right
    float yaw;                  // Rudder/tail

    // Energy and stamina
    float flightEnergy;         // Current flight energy pool
    float maxFlightEnergy;
    float energyRegenRate;      // Recovery when resting
    float currentStamina;       // Burst flight availability

    // Awareness
    std::vector<glm::vec3> nearbyFlock;
    std::vector<glm::vec3> predatorPositions;
    std::vector<glm::vec3> preyPositions;
    std::vector<glm::vec3> foodPositions;
    glm::vec3 nestPosition;
    bool hasNest;

    // Behavior timers
    float timeSinceTakeoff;
    float timeSinceLastMeal;
    float timeInCurrentState;

    FlyingState();
};

// ============================================================================
// Aerial Behavior Patterns
// ============================================================================

enum class AerialBehavior {
    // Basic states
    PERCHED,            // Resting on surface
    TAKING_OFF,         // Launching into air
    CRUISING,           // Normal flight
    GLIDING,            // Energy-saving descent
    LANDING,            // Approaching landing spot

    // Hunting behaviors
    HUNTING_SEARCH,     // Looking for prey
    HUNTING_STALK,      // Following prey
    HUNTING_DIVE,       // Attack dive
    HUNTING_PURSUE,     // Chasing prey
    HUNTING_STRIKE,     // Contact attack

    // Foraging behaviors
    FORAGING_SEARCH,    // Looking for food
    FORAGING_APPROACH,  // Moving to food
    FORAGING_FEED,      // Eating (may be in air or landed)
    FORAGING_CACHE,     // Storing food (corvids)

    // Social behaviors
    FLOCKING,           // Following flock
    TERRITORIAL_PATROL, // Defending area
    TERRITORIAL_DISPLAY,// Warning intruders
    TERRITORIAL_CHASE,  // Pursuing intruders
    COURTSHIP_DISPLAY,  // Mating displays
    COURTSHIP_PURSUIT,  // Chasing mate

    // Nest behaviors
    NEST_BUILDING,      // Constructing nest
    NEST_GUARDING,      // Protecting nest
    NEST_FEEDING,       // Feeding young

    // Emergency behaviors
    EVADING,            // Fleeing predator
    MOBBING,            // Group attack on predator
    ALARM_CALLING,      // Warning others

    // Special behaviors
    THERMAL_RIDING,     // Circling in thermal
    DYNAMIC_SOARING,    // Using wind gradients
    HOVERING,           // Stationary flight
    MIGRATION,          // Long-distance travel
    ROOSTING_SEARCH,    // Finding night shelter
    BATHING,            // Water bathing (birds)
    PREENING,           // Self-grooming
    DUST_BATHING        // Dust/sand bathing
};

// ============================================================================
// Thermal and Wind Systems Interface
// ============================================================================

struct ThermalInfo {
    glm::vec3 center;
    float radius;
    float strength;         // Vertical velocity m/s
    float maxAltitude;      // Top of thermal
    float age;              // Time since formation
    bool isActive;

    float getStrengthAt(const glm::vec3& pos) const;
};

struct WindInfo {
    glm::vec3 direction;    // Normalized wind direction
    float speed;            // Wind speed m/s
    float gustiness;        // Variability factor
    float turbulence;       // Random perturbation

    glm::vec3 getWindAt(const glm::vec3& pos, float time) const;
};

// ============================================================================
// Flying Creature Controller
// ============================================================================

class FlyingCreatureController {
public:
    FlyingCreatureController();
    ~FlyingCreatureController() = default;

    // Initialize with genome
    void initialize(const FlyingGenome& genome, FlyingSubtype subtype);

    // Update flight physics and behavior
    void update(float deltaTime, const Terrain& terrain,
                const std::vector<ThermalInfo>& thermals,
                const WindInfo& wind);

    // Set external inputs
    void setTargetPosition(const glm::vec3& target);
    void setFlockmates(const std::vector<glm::vec3>& positions);
    void setPredators(const std::vector<glm::vec3>& positions);
    void setPrey(const std::vector<glm::vec3>& positions);
    void setFoodSources(const std::vector<glm::vec3>& positions);

    // Commands
    void commandTakeoff();
    void commandLand(const glm::vec3& landingSpot);
    void commandDive(const glm::vec3& target);
    void commandFlock();
    void commandEvade(const glm::vec3& threat);
    void commandHunt(const glm::vec3& prey);
    void commandReturn(); // Return to nest

    // State access
    const FlyingState& getState() const { return m_state; }
    FlyingState& getState() { return m_state; }
    const FlyingGenome& getGenome() const { return m_genome; }
    FlyingSubtype getSubtype() const { return m_subtype; }
    AerialBehavior getCurrentBehavior() const { return m_currentBehavior; }
    WingStructure getWingStructure() const { return m_wingStructure; }

    // Animation interface
    float getWingPhase() const { return m_wingPhase; }
    float getFlapFrequency() const { return m_flapFrequency; }
    float getTailSpread() const { return m_tailSpread; }
    float getFeatherSpread() const { return m_featherSpread; }

    // Physics queries
    float getCurrentLift() const { return m_currentLift; }
    float getCurrentDrag() const { return m_currentDrag; }
    float getCurrentThrust() const { return m_currentThrust; }
    bool isStalling() const { return m_isStalling; }
    bool isThermalRiding() const { return m_currentBehavior == AerialBehavior::THERMAL_RIDING; }

private:
    // Configuration
    FlyingGenome m_genome;
    FlyingSubtype m_subtype;
    WingStructure m_wingStructure;
    FlightSpecialization m_specialization;

    // Current state
    FlyingState m_state;
    AerialBehavior m_currentBehavior;
    AerialBehavior m_previousBehavior;

    // Physics state
    float m_currentLift;
    float m_currentDrag;
    float m_currentThrust;
    bool m_isStalling;
    float m_stallTimer;

    // Animation state
    float m_wingPhase;
    float m_flapFrequency;
    float m_tailSpread;
    float m_featherSpread;
    float m_wingFoldAmount;

    // Targets and navigation
    glm::vec3 m_targetPosition;
    glm::vec3 m_landingSpot;
    bool m_hasTarget;
    bool m_isLanding;

    // Behavior timers
    float m_behaviorTimer;
    float m_decisionCooldown;

    // Internal methods - Physics
    void calculateAerodynamics(float deltaTime, const WindInfo& wind);
    void applyLift(float deltaTime);
    void applyDrag(float deltaTime);
    void applyThrust(float deltaTime);
    void applyGravity(float deltaTime);
    void applyThermalForce(const std::vector<ThermalInfo>& thermals, float deltaTime);
    void applyWindForce(const WindInfo& wind, float deltaTime);
    void updateOrientation(float deltaTime);
    void enforceFlightEnvelope(float deltaTime, const Terrain& terrain);

    // Internal methods - Behavior
    void updateBehavior(float deltaTime, const Terrain& terrain);
    void selectBehavior();
    void executeBehavior(float deltaTime, const Terrain& terrain);
    void transitionBehavior(AerialBehavior newBehavior);

    // Specific behavior implementations
    void updatePerched(float deltaTime, const Terrain& terrain);
    void updateTakingOff(float deltaTime);
    void updateCruising(float deltaTime);
    void updateGliding(float deltaTime);
    void updateLanding(float deltaTime, const Terrain& terrain);
    void updateHovering(float deltaTime);
    void updateThermalRiding(float deltaTime, const std::vector<ThermalInfo>& thermals);
    void updateFlocking(float deltaTime);
    void updateEvading(float deltaTime);
    void updateHunting(float deltaTime);
    void updateDiving(float deltaTime);

    // Navigation helpers
    glm::vec3 calculateSteeringForce();
    glm::vec3 calculateSeparation(float radius);
    glm::vec3 calculateAlignment();
    glm::vec3 calculateCohesion();
    glm::vec3 findNearestThermal(const std::vector<ThermalInfo>& thermals);
    glm::vec3 findLandingSpot(const Terrain& terrain);

    // Animation helpers
    void updateAnimation(float deltaTime);
    float calculateOptimalFlapFrequency() const;
};

// ============================================================================
// Flying Creature Factory
// ============================================================================

class FlyingCreatureFactory {
public:
    // Create specific flying creature types
    static FlyingGenome createSongbird(float size = 0.1f);
    static FlyingGenome createRaptor(float size = 0.8f, bool isLarge = true);
    static FlyingGenome createOwl(float size = 0.4f);
    static FlyingGenome createSeabird(float size = 0.6f);
    static FlyingGenome createHummingbird(float size = 0.03f);
    static FlyingGenome createVulture(float size = 1.0f);

    static FlyingGenome createButterfly(float size = 0.02f);
    static FlyingGenome createDragonfly(float size = 0.05f);
    static FlyingGenome createBee(float size = 0.015f);
    static FlyingGenome createBeetle(float size = 0.03f);
    static FlyingGenome createMosquito(float size = 0.005f);
    static FlyingGenome createLocust(float size = 0.04f);

    static FlyingGenome createMicrobat(float size = 0.05f);
    static FlyingGenome createFruitBat(float size = 0.3f);

    static FlyingGenome createPterosaur(float size = 2.0f);
    static FlyingGenome createDragon(float size = 3.0f, bool isLarge = true);

    // Random within category
    static FlyingGenome createRandomBird();
    static FlyingGenome createRandomInsect();
    static FlyingGenome createRandomBat();
    static FlyingGenome createRandomFantasy();

    // Get appropriate wing structure for subtype
    static WingStructure getWingStructure(FlyingSubtype subtype);

    // Get appropriate flight specialization
    static FlightSpecialization getSpecialization(FlyingSubtype subtype);
};

// ============================================================================
// Swarm System for Insects and Flocking Birds
// ============================================================================

class SwarmSystem {
public:
    SwarmSystem();
    ~SwarmSystem() = default;

    // Configuration
    void setSwarmType(FlyingSubtype type);
    void setSwarmSize(int count);
    void setSwarmCenter(const glm::vec3& center);
    void setSwarmRadius(float radius);

    // Behavior parameters
    void setSeparationWeight(float weight);
    void setAlignmentWeight(float weight);
    void setCohesionWeight(float weight);
    void setGoalWeight(float weight);
    void setNoiseAmount(float noise);

    // Update
    void update(float deltaTime, const Terrain& terrain);

    // Access
    const std::vector<glm::vec3>& getPositions() const { return m_positions; }
    const std::vector<glm::vec3>& getVelocities() const { return m_velocities; }
    const std::vector<glm::quat>& getOrientations() const { return m_orientations; }
    int getSwarmSize() const { return static_cast<int>(m_positions.size()); }

    // Goals
    void setTargetPosition(const glm::vec3& target);
    void addAttractor(const glm::vec3& pos, float strength);
    void addRepeller(const glm::vec3& pos, float strength);
    void clearForces();

private:
    FlyingSubtype m_swarmType;
    int m_swarmSize;
    glm::vec3 m_swarmCenter;
    float m_swarmRadius;

    // Individual states
    std::vector<glm::vec3> m_positions;
    std::vector<glm::vec3> m_velocities;
    std::vector<glm::quat> m_orientations;

    // Behavior weights
    float m_separationWeight;
    float m_alignmentWeight;
    float m_cohesionWeight;
    float m_goalWeight;
    float m_noiseAmount;

    // External forces
    std::vector<std::pair<glm::vec3, float>> m_attractors;
    std::vector<std::pair<glm::vec3, float>> m_repellers;
    glm::vec3 m_targetPosition;
    bool m_hasTarget;

    // Swarm parameters (based on type)
    float m_maxSpeed;
    float m_neighborRadius;
    float m_separationRadius;
    float m_wingBeatFrequency;

    // Update individual
    void updateIndividual(int index, float deltaTime);
    glm::vec3 calculateFlockingForce(int index);
};

// ============================================================================
// Murmuration System (Specialized for starling-like flocks)
// ============================================================================

class MurmurationSystem {
public:
    MurmurationSystem();

    void initialize(int birdCount, const glm::vec3& center);
    void update(float deltaTime, const Terrain& terrain);

    // Predator response
    void addPredatorThreat(const glm::vec3& position);
    void clearThreats();

    // Access
    const std::vector<glm::vec3>& getPositions() const { return m_positions; }
    const std::vector<glm::vec3>& getVelocities() const { return m_velocities; }
    glm::vec3 getFlockCenter() const { return m_flockCenter; }
    float getFlockRadius() const { return m_flockRadius; }

private:
    std::vector<glm::vec3> m_positions;
    std::vector<glm::vec3> m_velocities;
    std::vector<float> m_phases;        // Wing beat phases

    glm::vec3 m_flockCenter;
    float m_flockRadius;

    std::vector<glm::vec3> m_predatorThreats;

    // Topological distance (nearest N neighbors rather than radius)
    int m_topologicalNeighbors;
    std::vector<std::vector<int>> m_neighborCache;

    void rebuildNeighborCache();
    void updateBird(int index, float deltaTime);
    glm::vec3 calculateMurmurationForce(int index);
};

} // namespace flying
