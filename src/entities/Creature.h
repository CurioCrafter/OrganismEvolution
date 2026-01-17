#pragma once

#include "Genome.h"
#include "genetics/DiploidGenome.h"
#include "NeuralNetwork.h"
#include "CreatureType.h"
#include "SteeringBehaviors.h"
#include "SensorySystem.h"
#include "../animation/Animation.h"
#include "../animation/ActivitySystem.h"
#include "../animation/ActivityAnimations.h"
#include "../ai/CreatureBrainInterface.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <atomic>

class Terrain;
class SpatialGrid;
class ClimateSystem;
struct ClimateData;

class Creature {
public:
    // Legacy constructors (for compatibility with existing code)
    Creature(const glm::vec3& position, const Genome& genome, CreatureType type = CreatureType::HERBIVORE);
    Creature(const glm::vec3& position, const Genome& parent1, const Genome& parent2, CreatureType type = CreatureType::HERBIVORE);

    // New diploid genome constructors
    Creature(const glm::vec3& position, const genetics::DiploidGenome& diploidGenome, CreatureType type = CreatureType::HERBIVORE);
    Creature(const glm::vec3& position, const genetics::DiploidGenome& parent1, const genetics::DiploidGenome& parent2, CreatureType type = CreatureType::HERBIVORE);

    void update(float deltaTime, const Terrain& terrain, const std::vector<glm::vec3>& foodPositions,
                const std::vector<Creature*>& otherCreatures, const SpatialGrid* spatialGrid = nullptr,
                const EnvironmentConditions* envConditions = nullptr,
                const std::vector<SoundEvent>* sounds = nullptr);
    void render(uint32_t vaoHandle);

    bool isAlive() const { return alive; }
    bool canReproduce() const;
    void consumeFood(float amount);
    void reproduce(float& energyCost);

    // ========================================
    // Mate preference evaluation (C-17 fix)
    // ========================================
    bool willMateWith(const Creature& other) const;
    float evaluateMateAttraction(const Creature& other) const;
    bool canMateWith(const Creature& other) const;

    // Combat methods for predator-prey
    void attack(Creature* target, float deltaTime);
    void takeDamage(float damage);

    const glm::vec3& getPosition() const { return position; }
    const glm::vec3& getVelocity() const { return velocity; }
    const Genome& getGenome() const { return genome; }
    const genetics::DiploidGenome& getDiploidGenome() const { return diploidGenome; }
    genetics::DiploidGenome& getDiploidGenome() { return diploidGenome; }
    float getEnergy() const { return energy; }
    float getAge() const { return age; }
    float getFitness() const { return fitness; }
    int getGeneration() const { return generation; }
    int getID() const { return id; }
    CreatureType getType() const { return type; }

    // Genome trait accessors (used by FoodChainManager, etc.)
    float getSpeed() const { return genome.speed; }
    float getSize() const { return genome.size; }
    float getVisionRange() const { return genome.visionRange; }
    float getFear() const { return fear; }
    int getKillCount() const { return killCount; }
    bool isBeingHunted() const { return beingHunted; }
    bool isSterile() const { return sterile; }
    void setSterile(bool s) { sterile = s; }
    float getFitnessModifier() const { return fitnessModifier; }
    void setFitnessModifier(float mod) { fitnessModifier = mod; }

    // Sensory system access
    SensorySystem& getSensory() { return sensory; }
    const SensorySystem& getSensory() const { return sensory; }
    float getCamouflageLevel() const { return genome.camouflageLevel; }

    // Bioluminescence (for night-time glow effects)
    bool hasBioluminescence() const { return genome.hasBioluminescence; }
    float getBiolumIntensity() const { return genome.biolumIntensity; }
    glm::vec3 getBiolumColor() const { return glm::vec3(genome.biolumRed, genome.biolumGreen, genome.biolumBlue); }
    uint32_t getId() const { return static_cast<uint32_t>(id); }

    // Species-based visual differentiation
    // Returns creature color mixed with species color for visual distinction
    glm::vec3 getSpeciesTintedColor() const;

    // Get pattern type based on species (0=solid, 1=stripes, 2=spots, 3=gradient)
    int getSpeciesPatternType() const;

    // Get species ID for visualization
    genetics::SpeciesId getSpeciesId() const { return diploidGenome.getSpeciesId(); }

    // Species name stored per-creature for consistent display
    const std::string& getSpeciesDisplayName() const { return m_speciesDisplayName; }
    void setSpeciesDisplayName(const std::string& name) { m_speciesDisplayName = name; }

    void setGeneration(int gen) { generation = gen; }
    void setBeingHunted(bool hunted) { beingHunted = hunted; }

    // NEAT brain access (for evolved topology)
    void enableNEATBrain(bool enable = true) { m_useNEATBrain = enable; }
    bool isUsingNEATBrain() const { return m_useNEATBrain; }
    ai::CreatureBrainInterface* getNEATBrain() { return m_neatBrain.get(); }
    const ai::CreatureBrainInterface* getNEATBrain() const { return m_neatBrain.get(); }

    // Initialize NEAT brain from genome (called by evolution manager)
    void initializeNEATBrain(const ai::NEATGenome& genome);
    void initializeNEATBrain();  // Create minimal NEAT brain

    // Get NEAT genome for reproduction
    const ai::NEATGenome& getNEATGenome() const;
    void setNEATGenome(const ai::NEATGenome& genome);
    bool hasNEATBrain() const { return m_useNEATBrain && m_neatBrain != nullptr; }

    // Communication - emit sounds
    void emitAlarmCall(std::vector<SoundEvent>& soundBuffer);
    void emitMatingCall(std::vector<SoundEvent>& soundBuffer);

    // Animation system access
    animation::CreatureAnimator& getAnimator() { return m_animator; }
    const animation::CreatureAnimator& getAnimator() const { return m_animator; }
    bool isAnimationEnabled() const { return m_animationEnabled; }
    void setAnimationEnabled(bool enabled) { m_animationEnabled = enabled; }
    void initializeAnimation();

    // Get current animation phase for shader (0-1)
    float getAnimationPhase() const;

    // Get skinning matrices for GPU upload
    const std::vector<glm::mat4>& getSkinningMatrices() const { return m_animator.getSkinningMatrices(); }

    // ========================================
    // Activity System (Phase 8, Agent 10)
    // ========================================
    animation::ActivityStateMachine& getActivitySystem() { return m_activitySystem; }
    const animation::ActivityStateMachine& getActivitySystem() const { return m_activitySystem; }
    animation::ActivityType getCurrentActivity() const { return m_activitySystem.getCurrentActivity(); }
    std::string getCurrentActivityName() const { return m_activitySystem.getCurrentActivityName(); }
    float getActivityProgress() const { return m_activitySystem.getActivityProgress(); }
    bool isPerformingActivity() const { return m_activitySystem.isInActivity(); }

    // Activity triggers access (for UI inspection - Agent 8)
    const animation::ActivityTriggers& getActivityTriggers() const { return m_activityTriggers; }

    // Physiological state for activity triggers
    float getHungerLevel() const { return 1.0f - (energy / maxEnergy); }
    float getFatigueLevel() const { return m_fatigueLevel; }
    float getBladderFullness() const { return m_bladderFullness; }
    float getBowelFullness() const { return m_bowelFullness; }
    float getDirtyLevel() const { return m_dirtyLevel; }
    float getReproductionUrge() const;
    bool hasNearbyMate() const { return m_hasNearbyMate; }
    bool hasNearbyFood() const { return m_hasNearbyFood; }

    // Parental care state (Agent 10)
    bool hasNearbyOffspring() const { return m_hasOffspringNearby; }
    float getParentalUrge() const { return m_parentalUrge; }
    float getOffspringHungerLevel() const { return m_offspringHungerLevel; }

    // Climate response methods
    void updateClimateResponse(const ClimateData& climate, const ClimateSystem* climateSystem, float deltaTime);
    float getOptimalTemperature() const;
    float getClimateStress() const { return m_climateStress; }
    bool isMigrating() const { return m_isMigrating; }
    glm::vec3 getMigrationDirection() const { return m_migrationDirection; }
    void setMigrating(bool migrating) { m_isMigrating = migrating; }
    void setMigrationDirection(const glm::vec3& dir) { m_migrationDirection = dir; }

    // Energy manipulation
    void addEnergy(float amount) { energy = glm::clamp(energy + amount, 0.0f, maxEnergy); }
    void setEnergy(float e) { energy = glm::clamp(e, 0.0f, maxEnergy); }
    float getMaxEnergy() const { return maxEnergy; }

    // Active check (creature manager pool compatibility)
    bool isActive() const { return alive; }

private:
    glm::vec3 position;
    glm::vec3 velocity;
    float rotation;
    glm::vec3 wanderTarget;  // For wander behavior persistence
    float m_wanderAngle;     // Per-instance wander angle for aquatic behavior (thread-safe)

    Genome genome;
    genetics::DiploidGenome diploidGenome;  // New sophisticated genetic system
    std::unique_ptr<NeuralNetwork> brain;
    SteeringBehaviors steering;
    SensorySystem sensory;
    CreatureType type;
    float currentTime;  // Accumulated time for sensory system

    float energy;
    float age;
    bool alive;
    bool sterile;           // For hybrid sterility
    float fitnessModifier;  // For hybrid fitness effects
    int generation;
    int id;

    float fitness;
    int foodEaten;
    float distanceTraveled;

    // Predator-prey state
    float fear;              // 0-1 fear level (affects behavior)
    float huntingCooldown;   // Time between attacks
    int killCount;           // For carnivore reproduction requirement
    bool beingHunted;        // Set by predators targeting this creature

    // Climate response state
    float m_climateStress = 0.0f;     // 0-1 stress level from temperature
    float m_optimalTemp = 0.5f;       // Creature's preferred temperature (0-1 normalized)
    bool m_seekingCooling = false;    // Behavioral flag: seek water/shade
    bool m_seekingWarmth = false;     // Behavioral flag: seek shelter/huddle
    bool m_isMigrating = false;       // Currently migrating to better climate
    glm::vec3 m_migrationDirection = glm::vec3(0.0f);
    float m_migrationCooldown = 0.0f; // Prevent constant migration checks

    // Animation system
    animation::CreatureAnimator m_animator;
    bool m_animationEnabled = true;
    const Terrain* m_terrainPtr = nullptr;  // For ground raycasting in animation

    // Activity system (Phase 8, Agent 10)
    animation::ActivityStateMachine m_activitySystem;
    animation::ActivityAnimationDriver m_activityAnimDriver;
    animation::SecondaryMotionLayer m_secondaryMotion;
    animation::ActivityTriggers m_activityTriggers;

    // Physiological state for activity triggers
    float m_fatigueLevel = 0.0f;          // 0 = rested, 1 = exhausted
    float m_bladderFullness = 0.0f;       // 0 = empty, 1 = needs to excrete
    float m_bowelFullness = 0.0f;         // 0 = empty, 1 = needs to excrete
    float m_dirtyLevel = 0.0f;            // 0 = clean, 1 = needs grooming
    float m_lastMealTime = 0.0f;          // Time since last meal (for digestion)
    bool m_hasNearbyMate = false;         // Potential mate nearby
    bool m_hasNearbyFood = false;         // Food nearby
    glm::vec3 m_nearestFoodPos{0.0f};     // For eating animation target
    glm::vec3 m_nearestMatePos{0.0f};     // For mating animation target

    // Parental care state (Agent 10)
    bool m_hasOffspringNearby = false;    // Own offspring nearby
    float m_parentalUrge = 0.0f;          // 0 = none, 1 = strong
    float m_offspringHungerLevel = 0.0f;  // 0 = fed, 1 = hungry

    // Neural network behavior modulation (cached outputs from brain->forward())
    NeuralOutputs m_neuralOutputs;
    bool m_useNeuralBehavior = true;  // Enable/disable neural modulation

    // Expanded motor output from NEAT brain (drives PRIMARY behavior)
    ai::MotorOutput m_neatMotorOutput;
    float m_timeSinceLastMeal = 0.0f;  // For neural input

    // Species display name (stored per-creature for consistent UI display)
    std::string m_speciesDisplayName;

    // NEAT-evolved brain (topology evolves, not just weights)
    std::unique_ptr<ai::CreatureBrainInterface> m_neatBrain;
    bool m_useNEATBrain = true;  // ENABLED BY DEFAULT: Use NEAT brain for evolved behavior

    // Thread-safe ID generator for potential future multi-threading support
    static std::atomic<int> nextID;

    // Herbivore energy settings
    static constexpr float maxEnergy = 200.0f;
    static constexpr float herbivoreReproductionThreshold = 180.0f;
    static constexpr float herbivoreReproductionCost = 80.0f;

    // Carnivore energy settings (higher threshold, needs kills)
    static constexpr float carnivoreReproductionThreshold = 170.0f;
    static constexpr float carnivoreReproductionCost = 100.0f;
    static constexpr int minKillsToReproduce = 2;

    // Flying creature energy settings (omnivore - can eat food and hunt)
    static constexpr float flyingReproductionThreshold = 160.0f;
    static constexpr float flyingReproductionCost = 70.0f;
    static constexpr int minKillsToReproduceFlying = 1;

    // Combat settings
    static constexpr float attackRange = 2.5f;
    static constexpr float attackDamage = 15.0f;
    static constexpr float attackCooldown = 0.5f;
    static constexpr float killEnergyGain = 120.0f;

    void updatePhysics(float deltaTime, const Terrain& terrain);
    void updateBehaviorHerbivore(float deltaTime, const std::vector<glm::vec3>& foodPositions,
                                  const std::vector<Creature*>& otherCreatures, const SpatialGrid* grid);
    void updateBehaviorCarnivore(float deltaTime, const std::vector<Creature*>& otherCreatures,
                                  const SpatialGrid* grid);
    void updateBehaviorAquatic(float deltaTime, const std::vector<Creature*>& otherCreatures,
                               const SpatialGrid* grid);
    void updateBehaviorFlying(float deltaTime, const Terrain& terrain,
                              const std::vector<glm::vec3>& foodPositions,
                              const std::vector<Creature*>& otherCreatures, const SpatialGrid* grid);
    void updateFlyingPhysics(float deltaTime, const Terrain& terrain);
    void updateSensoryBehavior(float deltaTime, const std::vector<Creature*>& otherCreatures);
    void updateAnimation(float deltaTime);
    void updateActivitySystem(float deltaTime, const std::vector<glm::vec3>& foodPositions,
                              const std::vector<Creature*>& otherCreatures);
    void updatePhysiologicalState(float deltaTime);
    void calculateFitness();

    // Neural network integration
    std::vector<float> gatherNeuralInputs(const std::vector<glm::vec3>& foodPositions,
                                           const std::vector<Creature*>& otherCreatures) const;
    void updateNeuralBehavior(const std::vector<glm::vec3>& foodPositions,
                              const std::vector<Creature*>& otherCreatures);

    // Find nearest creature of specified type (uses SpatialGrid for O(1) when available)
    Creature* findNearestCreature(const std::vector<Creature*>& creatures, CreatureType targetType, float maxRange, const SpatialGrid* grid = nullptr) const;
    std::vector<Creature*> getNeighborsOfType(const std::vector<Creature*>& creatures, CreatureType targetType, float range, const SpatialGrid* grid = nullptr) const;

    // Climate response helpers
    void considerMigration(const ClimateData& currentClimate, const ClimateSystem* climateSystem);
    float calculateClimateSuitability(const ClimateData& climate) const;
    void initializeOptimalTemperature();
};
