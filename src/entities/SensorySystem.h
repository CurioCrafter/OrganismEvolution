#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

class Creature;
class Terrain;
class SpatialGrid;

// Forward declarations for environment
struct EnvironmentConditions;
class PheromoneGrid;  // Changed from struct to class (matches definition)
struct SoundEvent;

// Types of sensory information
enum class SensoryType {
    VISION,
    HEARING,
    SMELL,
    TOUCH,
    ELECTRORECEPTION
};

// Types of detected entities
enum class DetectionType {
    FOOD,
    PREDATOR,
    PREY,
    CONSPECIFIC,  // Same species
    MATE,
    DANGER_ZONE,
    SHELTER,
    PHEROMONE_TRAIL,
    SOUND_SOURCE,
    MOVEMENT
};

// Types of pheromones
enum class PheromoneType {
    FOOD_TRAIL,      // "I found food this way"
    ALARM,           // "Danger here!"
    TERRITORY,       // "This is my area"
    MATING,          // "I'm available for reproduction"
    AGGREGATION      // "Gather here"
};

// Hash specialization for PheromoneType (required for std::unordered_map)
namespace std {
    template <>
    struct hash<PheromoneType> {
        size_t operator()(PheromoneType type) const {
            return hash<int>()(static_cast<int>(type));
        }
    };
}

// Types of sounds
enum class SoundType {
    MOVEMENT,        // Footsteps, rustling
    ALARM_CALL,      // Warning vocalization
    MATING_CALL,     // Attraction vocalization
    ECHOLOCATION,    // Active sonar ping
    FEEDING          // Eating sounds
};

// Memory types for spatial memory
enum class MemoryType {
    FOOD_LOCATION,
    DANGER_LOCATION,
    SHELTER_LOCATION,
    TERRITORY_BOUNDARY,
    CONSPECIFIC_SIGHTING
};

// A detected entity with sensory information
struct SensoryPercept {
    DetectionType type;
    glm::vec3 position;
    glm::vec3 velocity;
    float distance;
    float angle;              // Relative to creature facing
    float confidence;         // 0-1, how certain the detection is
    float strength;           // Signal strength (brightness, loudness, concentration)
    SensoryType sensedBy;     // Which sense detected this
    Creature* sourceCreature; // If detected entity is a creature
    float timestamp;          // When detected

    SensoryPercept() : type(DetectionType::FOOD), position(0.0f), velocity(0.0f),
                       distance(0.0f), angle(0.0f), confidence(1.0f), strength(1.0f),
                       sensedBy(SensoryType::VISION), sourceCreature(nullptr), timestamp(0.0f) {}
};

// Sound event in the environment
struct SoundEvent {
    glm::vec3 position;
    SoundType type;
    float intensity;          // 0-1 at source
    float frequency;          // Hz (affects propagation)
    float timestamp;
    Creature* source;         // nullptr for environmental sounds

    SoundEvent() : position(0.0f), type(SoundType::MOVEMENT), intensity(0.5f),
                   frequency(1000.0f), timestamp(0.0f), source(nullptr) {}
};

// Environment conditions affecting sensing
struct EnvironmentConditions {
    float visibility;         // 0-1, reduced by fog, darkness, murky water
    float ambientLight;       // 0-1, affects vision
    glm::vec3 windDirection;  // Normalized direction
    float windSpeed;          // Affects scent propagation
    float temperature;        // Affects sound speed
    bool isUnderwater;        // Changes sound/smell behavior

    EnvironmentConditions()
        : visibility(1.0f), ambientLight(1.0f), windDirection(1.0f, 0.0f, 0.0f),
          windSpeed(0.0f), temperature(20.0f), isUnderwater(false) {}
};

// Sensory genome traits - evolvable characteristics
struct SensoryGenome {
    // Vision traits
    float visionFOV;              // Field of view in radians (π/2 to 2π)
    float visionRange;            // Detection distance
    float visionAcuity;           // Detail perception (0-1)
    float colorPerception;        // Color sensitivity (0-1, 0=monochrome)
    float motionDetection;        // Motion sensitivity bonus (0-1)

    // Hearing traits
    float hearingRange;           // Maximum hearing distance
    float hearingDirectionality;  // Directional accuracy (0-1)
    float echolocationAbility;    // 0=none, 1=full echolocation capability

    // Smell traits
    float smellRange;             // Detection distance
    float smellSensitivity;       // Detection threshold (0-1)
    float pheromoneProduction;    // Emission rate (0-1)

    // Touch/Vibration traits
    float touchRange;             // Very short range detection
    float vibrationSensitivity;   // Ground/water vibration detection (0-1)

    // Camouflage (reduces visual detection by others)
    float camouflageLevel;        // 0-1

    // Communication traits
    float alarmCallVolume;        // 0-1
    float displayIntensity;       // Mating display strength (0-1)

    // Memory capacity
    float memoryCapacity;         // Affects spatial memory size (0-1)
    float memoryRetention;        // How long memories last (0-1)

    // Constructor with default values
    SensoryGenome();

    // Mutation
    void mutate(float mutationRate, float mutationStrength);

    // Crossover
    static SensoryGenome crossover(const SensoryGenome& parent1, const SensoryGenome& parent2);

    // Calculate total energy cost of sensory systems
    float calculateEnergyCost() const;

    // Randomize for initial population
    void randomize();
};

// Memory entry for spatial memory
struct MemoryEntry {
    glm::vec3 location;
    MemoryType type;
    float strength;           // Decays over time
    float timestamp;          // When observed
    float importance;         // Affects decay rate

    MemoryEntry() : location(0.0f), type(MemoryType::FOOD_LOCATION),
                    strength(1.0f), timestamp(0.0f), importance(0.5f) {}

    MemoryEntry(glm::vec3 loc, MemoryType t, float str, float time, float imp = 0.5f)
        : location(loc), type(t), strength(str), timestamp(time), importance(imp) {}
};

// Spatial memory system
class SpatialMemory {
public:
    SpatialMemory(int maxCapacity = 20, float decayRate = 0.1f);

    void remember(const glm::vec3& position, MemoryType type, float importance = 0.5f);
    void update(float deltaTime);
    void clear();

    std::vector<MemoryEntry> recall(MemoryType type) const;
    std::vector<MemoryEntry> recallNearby(const glm::vec3& position, float radius) const;
    bool hasMemoryOf(MemoryType type) const;
    glm::vec3 getClosestMemory(const glm::vec3& position, MemoryType type) const;

    int getMemoryCount() const { return static_cast<int>(memories.size()); }
    void setCapacity(int capacity) { maxCapacity = capacity; }
    void setDecayRate(float rate) { decayRate = rate; }

private:
    std::vector<MemoryEntry> memories;
    int maxCapacity;
    float decayRate;
    float currentTime;

    void consolidate();  // Remove weak memories when at capacity
};

// Main sensory system class
class SensorySystem {
public:
    SensorySystem();  // Default constructor
    SensorySystem(const SensoryGenome& genome);

    // Main sensing function - gathers all percepts
    void sense(
        const glm::vec3& position,
        const glm::vec3& velocity,
        float facing,  // Current facing direction in radians
        const std::vector<glm::vec3>& foodPositions,
        const std::vector<Creature*>& creatures,
        const SpatialGrid* spatialGrid,
        const Terrain& terrain,
        const EnvironmentConditions& environment,
        const std::vector<SoundEvent>& sounds,
        float currentTime
    );

    // Get detected percepts by type
    std::vector<SensoryPercept> getPercepts() const { return currentPercepts; }
    std::vector<SensoryPercept> getPerceptsByType(DetectionType type) const;
    std::vector<SensoryPercept> getPerceptsBySense(SensoryType sense) const;

    // Convenience accessors
    bool hasThreatNearby() const;
    bool hasFoodNearby() const;
    SensoryPercept getNearestThreat() const;
    SensoryPercept getNearestFood() const;
    SensoryPercept getNearestMate() const;

    // Neural network input generation
    std::vector<float> generateNeuralInputs() const;

    // Spatial memory access
    SpatialMemory& getMemory() { return memory; }
    const SpatialMemory& getMemory() const { return memory; }

    // Update memory with current percepts
    void updateMemory(float deltaTime);

    // Communication - emits signals (timestamp should be current simulation time)
    void emitAlarmCall(std::vector<SoundEvent>& soundBuffer, const glm::vec3& position, float timestamp = 0.0f);
    void emitMatingCall(std::vector<SoundEvent>& soundBuffer, const glm::vec3& position, float timestamp = 0.0f);

    // Getters
    const SensoryGenome& getGenome() const { return genome; }
    float getEnergyCost() const { return genome.calculateEnergyCost(); }

    // Update genome (for evolution)
    void setGenome(const SensoryGenome& newGenome) { genome = newGenome; }

private:
    SensoryGenome genome;
    std::vector<SensoryPercept> currentPercepts;
    SpatialMemory memory;

    // Individual sensing functions
    void senseVision(
        const glm::vec3& position,
        float facing,
        const std::vector<glm::vec3>& foodPositions,
        const std::vector<Creature*>& creatures,
        const SpatialGrid* spatialGrid,
        const EnvironmentConditions& environment,
        float currentTime
    );

    void senseHearing(
        const glm::vec3& position,
        const std::vector<SoundEvent>& sounds,
        const std::vector<Creature*>& creatures,
        const EnvironmentConditions& environment,
        float currentTime
    );

    void senseSmell(
        const glm::vec3& position,
        const std::vector<glm::vec3>& foodPositions,
        const std::vector<Creature*>& creatures,
        const EnvironmentConditions& environment,
        float currentTime
    );

    void senseTouch(
        const glm::vec3& position,
        const std::vector<Creature*>& creatures,
        const EnvironmentConditions& environment,
        float currentTime
    );

    // Helper functions
    float calculateDetectionProbability(
        float distance,
        float maxRange,
        float targetCamouflage,
        float targetSpeed,
        const EnvironmentConditions& environment
    ) const;

    float normalizeAngle(float angle) const;
    bool isInFieldOfView(float angleToTarget, float facing, float fov) const;
    float calculateSoundAttenuation(float distance, float frequency, const EnvironmentConditions& env) const;
    float calculateScentStrength(float distance, const glm::vec3& windDir, float windSpeed,
                                  const glm::vec3& toTarget) const;
};

// Global pheromone grid for environment-based communication
class PheromoneGrid {
public:
    PheromoneGrid(float worldSize, float cellSize);

    void deposit(const glm::vec3& position, PheromoneType type, float strength);
    float sample(const glm::vec3& position, PheromoneType type) const;
    glm::vec3 getGradient(const glm::vec3& position, PheromoneType type) const;

    void update(float deltaTime);  // Evaporation and diffusion
    void clear();

private:
    struct PheromoneCell {
        std::unordered_map<PheromoneType, float> concentrations;
    };

    float worldSize;
    float cellSize;
    int gridSize;
    std::vector<PheromoneCell> grid;
    float evaporationRate;
    float diffusionRate;

    int positionToIndex(const glm::vec3& position) const;
    glm::vec3 indexToPosition(int index) const;
};

// Sound propagation manager
class SoundManager {
public:
    SoundManager(float maxRange = 100.0f);

    void addSound(const SoundEvent& sound);
    void update(float deltaTime);
    void clear();

    std::vector<SoundEvent> getSoundsInRange(const glm::vec3& position, float range) const;
    const std::vector<SoundEvent>& getAllSounds() const { return activeSounds; }

private:
    std::vector<SoundEvent> activeSounds;
    float maxSoundDuration;
    float maxRange;
};
