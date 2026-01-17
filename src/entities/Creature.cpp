#include "Creature.h"
#include "SwimBehavior.h"
#include "SpeciesNameGenerator.h"
#include "../environment/Terrain.h"
#include "../utils/SpatialGrid.h"
#include "../utils/Random.h"
#include "../ai/NEATGenome.h"
#include "../ai/BrainModules.h"
#include "../ai/CreatureBrainInterface.h"
#include <cmath>
#include <algorithm>
#include <iostream>

// Thread-safe static ID counter - uses atomic for future multi-threading support
std::atomic<int> Creature::nextID{1};

// Helper function to create SensoryGenome from Genome
static SensoryGenome createSensoryGenome(const Genome& g) {
    SensoryGenome sg;
    sg.visionFOV = g.visionFOV;
    sg.visionRange = g.visionRange;
    sg.visionAcuity = g.visionAcuity;
    sg.colorPerception = g.colorPerception;
    sg.motionDetection = g.motionDetection;
    sg.hearingRange = g.hearingRange;
    sg.hearingDirectionality = g.hearingDirectionality;
    sg.echolocationAbility = g.echolocationAbility;
    sg.smellRange = g.smellRange;
    sg.smellSensitivity = g.smellSensitivity;
    sg.pheromoneProduction = g.pheromoneProduction;
    sg.touchRange = g.touchRange;
    sg.vibrationSensitivity = g.vibrationSensitivity;
    sg.camouflageLevel = g.camouflageLevel;
    sg.alarmCallVolume = g.alarmCallVolume;
    sg.displayIntensity = g.displayIntensity;
    sg.memoryCapacity = g.memoryCapacity;
    sg.memoryRetention = g.memoryRetention;
    return sg;
}

Creature::Creature(const glm::vec3& position, const Genome& genome, CreatureType type)
    : position(position), velocity(0.0f), rotation(0.0f), wanderTarget(1.0f, 0.0f, 0.0f),
      m_wanderAngle(0.0f),  // Per-instance wander angle (thread-safe)
      genome(genome), diploidGenome(),  // Initialize diploidGenome with default
      sensory(createSensoryGenome(genome)), type(type), currentTime(0.0f),
      energy(100.0f), age(0.0f), alive(true), sterile(false), fitnessModifier(1.0f),
      generation(0), id(nextID++),
      fitness(0.0f), foodEaten(0), distanceTraveled(0.0f),
      fear(0.0f), huntingCooldown(0.0f), killCount(0), beingHunted(false) {

    brain = std::make_unique<NeuralNetwork>(genome.neuralWeights);

    // Initialize NEAT brain by default - neural network drives behavior!
    initializeNEATBrain();

    // Configure steering behaviors based on genome (kept as fallback)
    SteeringBehaviors::Config config;
    config.maxSpeed = genome.speed;
    config.maxForce = genome.speed * 0.5f;
    config.fleeDistance = genome.visionRange * 0.8f;
    config.separationDistance = genome.size * 3.0f;
    config.alignmentDistance = genome.visionRange * 0.4f;
    config.cohesionDistance = genome.visionRange * 0.5f;
    steering.setConfig(config);

    // Generate species display name based on genome traits
    m_speciesDisplayName = naming::getNameGenerator().generateNameWithSeed(genome, type, static_cast<uint32_t>(id));
}

Creature::Creature(const glm::vec3& position, const Genome& parent1, const Genome& parent2, CreatureType type)
    : position(position), velocity(0.0f), rotation(0.0f), wanderTarget(1.0f, 0.0f, 0.0f),
      m_wanderAngle(0.0f),  // Per-instance wander angle (thread-safe)
      genome(parent1, parent2), diploidGenome(),  // Initialize diploidGenome with default
      sensory(createSensoryGenome(genome)), type(type), currentTime(0.0f),
      energy(100.0f), age(0.0f), alive(true), sterile(false), fitnessModifier(1.0f),
      generation(0), id(nextID++),
      fitness(0.0f), foodEaten(0), distanceTraveled(0.0f),
      fear(0.0f), huntingCooldown(0.0f), killCount(0), beingHunted(false) {

    brain = std::make_unique<NeuralNetwork>(genome.neuralWeights);

    // Initialize NEAT brain by default - neural network drives behavior!
    initializeNEATBrain();

    // Configure steering behaviors based on genome (kept as fallback)
    SteeringBehaviors::Config config;
    config.maxSpeed = genome.speed;
    config.maxForce = genome.speed * 0.5f;
    config.fleeDistance = genome.visionRange * 0.8f;
    config.separationDistance = genome.size * 3.0f;
    config.alignmentDistance = genome.visionRange * 0.4f;
    config.cohesionDistance = genome.visionRange * 0.5f;
    steering.setConfig(config);

    // Generate species display name based on genome traits
    m_speciesDisplayName = naming::getNameGenerator().generateNameWithSeed(genome, type, static_cast<uint32_t>(id));
}

// Helper to sync legacy Genome from DiploidGenome
static Genome syncGenomeFromDiploid(const genetics::DiploidGenome& dg) {
    Genome g;
    g.size = dg.getTrait(genetics::GeneType::SIZE);
    g.speed = dg.getTrait(genetics::GeneType::SPEED);
    g.visionRange = dg.getTrait(genetics::GeneType::VISION_RANGE);
    g.efficiency = dg.getTrait(genetics::GeneType::EFFICIENCY);
    g.color = dg.getColor();
    g.neuralWeights = dg.getNeuralWeights();

    // Fill in sensory traits with defaults
    g.visionFOV = 120.0f;
    g.visionAcuity = 0.7f;
    g.colorPerception = 0.5f;
    g.motionDetection = 0.6f;
    g.hearingRange = dg.getTrait(genetics::GeneType::VISION_RANGE) * 0.8f;
    g.hearingDirectionality = 0.5f;
    g.echolocationAbility = 0.0f;
    g.smellRange = 20.0f;
    g.smellSensitivity = 0.5f;
    g.pheromoneProduction = 0.3f;
    g.touchRange = 2.0f;
    g.vibrationSensitivity = 0.3f;
    g.camouflageLevel = 0.3f;
    g.alarmCallVolume = 0.5f;
    g.displayIntensity = dg.getTrait(genetics::GeneType::ORNAMENT_INTENSITY);
    g.memoryCapacity = 10;
    g.memoryRetention = 0.9f;

    return g;
}

Creature::Creature(const glm::vec3& position, const genetics::DiploidGenome& dg, CreatureType type)
    : position(position), velocity(0.0f), rotation(0.0f), wanderTarget(1.0f, 0.0f, 0.0f),
      m_wanderAngle(0.0f),  // Per-instance wander angle (thread-safe)
      genome(syncGenomeFromDiploid(dg)), diploidGenome(dg),
      sensory(createSensoryGenome(genome)), type(type), currentTime(0.0f),
      energy(100.0f), age(0.0f), alive(true), sterile(false), fitnessModifier(1.0f),
      generation(0), id(nextID++),
      fitness(0.0f), foodEaten(0), distanceTraveled(0.0f),
      fear(0.0f), huntingCooldown(0.0f), killCount(0), beingHunted(false) {

    brain = std::make_unique<NeuralNetwork>(genome.neuralWeights);

    // Initialize NEAT brain by default - neural network drives behavior!
    initializeNEATBrain();

    // Configure steering behaviors based on diploid genome (kept as fallback)
    SteeringBehaviors::Config config;
    config.maxSpeed = diploidGenome.getTrait(genetics::GeneType::SPEED);
    config.maxForce = config.maxSpeed * 0.5f;
    config.fleeDistance = diploidGenome.getTrait(genetics::GeneType::VISION_RANGE) * 0.8f;
    config.separationDistance = diploidGenome.getTrait(genetics::GeneType::SIZE) * 3.0f;
    config.alignmentDistance = diploidGenome.getTrait(genetics::GeneType::VISION_RANGE) * 0.4f;
    config.cohesionDistance = diploidGenome.getTrait(genetics::GeneType::VISION_RANGE) * 0.5f;
    steering.setConfig(config);

    // Apply genetic load as fitness modifier
    fitnessModifier = 1.0f - diploidGenome.getGeneticLoad() * 0.1f;

    // Generate species display name based on genome traits
    m_speciesDisplayName = naming::getNameGenerator().generateNameWithSeed(genome, type, static_cast<uint32_t>(id));
}

Creature::Creature(const glm::vec3& position, const genetics::DiploidGenome& parent1,
                   const genetics::DiploidGenome& parent2, CreatureType type)
    : position(position), velocity(0.0f), rotation(0.0f), wanderTarget(1.0f, 0.0f, 0.0f),
      m_wanderAngle(0.0f),  // Per-instance wander angle (thread-safe)
      genome(),  // Default-initialized, will be set in constructor body
      diploidGenome(parent1, parent2),  // Sexual reproduction
      sensory(),  // Default-initialized, will be set in constructor body
      type(type), currentTime(0.0f),
      energy(100.0f), age(0.0f), alive(true), sterile(false), fitnessModifier(1.0f),
      generation(0), id(nextID++),
      fitness(0.0f), foodEaten(0), distanceTraveled(0.0f),
      fear(0.0f), huntingCooldown(0.0f), killCount(0), beingHunted(false) {

    // Sync legacy genome from new diploid genome - MUST be first
    genome = syncGenomeFromDiploid(diploidGenome);
    sensory = SensorySystem(createSensoryGenome(genome));

    brain = std::make_unique<NeuralNetwork>(genome.neuralWeights);

    // Initialize NEAT brain by default - neural network drives behavior!
    initializeNEATBrain();

    // Configure steering behaviors (kept as fallback)
    SteeringBehaviors::Config config;
    config.maxSpeed = diploidGenome.getTrait(genetics::GeneType::SPEED);
    config.maxForce = config.maxSpeed * 0.5f;
    config.fleeDistance = diploidGenome.getTrait(genetics::GeneType::VISION_RANGE) * 0.8f;
    config.separationDistance = diploidGenome.getTrait(genetics::GeneType::SIZE) * 3.0f;
    config.alignmentDistance = diploidGenome.getTrait(genetics::GeneType::VISION_RANGE) * 0.4f;
    config.cohesionDistance = diploidGenome.getTrait(genetics::GeneType::VISION_RANGE) * 0.5f;
    steering.setConfig(config);

    // Apply genetic load and hybrid effects
    fitnessModifier = 1.0f - diploidGenome.getGeneticLoad() * 0.1f;

    // Inherit hybrid status
    if (parent1.isHybrid() || parent2.isHybrid() ||
        parent1.getSpeciesId() != parent2.getSpeciesId()) {
        diploidGenome.setHybrid(true);
    }

    // Generate species display name based on genome traits
    m_speciesDisplayName = naming::getNameGenerator().generateNameWithSeed(genome, type, static_cast<uint32_t>(id));
}

void Creature::update(float deltaTime, const Terrain& terrain,
                      const std::vector<glm::vec3>& foodPositions,
                      const std::vector<Creature*>& otherCreatures,
                      const SpatialGrid* spatialGrid,
                      const EnvironmentConditions* envConditions,
                      const std::vector<SoundEvent>* sounds) {
    if (!alive) return;

    age += deltaTime;
    currentTime += deltaTime;
    m_timeSinceLastMeal += deltaTime;  // Track hunger duration

    // Reduce hunting cooldown
    if (huntingCooldown > 0.0f) {
        huntingCooldown -= deltaTime;
    }

    // Decay fear over time
    fear = std::max(0.0f, fear - deltaTime * 0.5f);

    // Reset being hunted flag (will be set by predators if needed)
    beingHunted = false;

    // Energy consumption (based on size and efficiency)
    float baseConsumption = (0.5f + genome.size * 0.3f) * genome.efficiency * deltaTime;
    energy -= baseConsumption;

    // Movement increases energy consumption
    float movementCost = glm::length(velocity) * 0.05f * deltaTime;
    energy -= movementCost;

    // Sensory system energy cost (based on evolved sensory capabilities)
    float sensoryCost = genome.calculateSensoryEnergyCost() * deltaTime;
    energy -= sensoryCost;

    // Carnivores have slightly higher base metabolism
    if (type == CreatureType::CARNIVORE) {
        energy -= 0.3f * deltaTime;
    }

    // Flying creatures have higher metabolism (flight is energetically expensive)
    if (type == CreatureType::FLYING) {
        // Wing flapping cost (based on flap frequency - faster = more energy)
        float flapCost = genome.flapFrequency * 0.05f * deltaTime;
        // Gliding reduces energy cost (higher glide ratio = less flapping)
        flapCost *= (1.0f - genome.glideRatio * 0.5f);
        energy -= flapCost;
    }

    // Die if out of energy
    if (energy <= 0.0f) {
        alive = false;
        return;
    }

    // Update sensory system
    EnvironmentConditions defaultEnv;
    std::vector<SoundEvent> emptySounds;
    const EnvironmentConditions& env = envConditions ? *envConditions : defaultEnv;
    const std::vector<SoundEvent>& soundEvents = sounds ? *sounds : emptySounds;

    sensory.sense(
        position,
        velocity,
        rotation,
        foodPositions,
        otherCreatures,
        spatialGrid,
        terrain,
        env,
        soundEvents,
        currentTime
    );

    // Update spatial memory based on sensory percepts
    sensory.updateMemory(deltaTime);

    // Update behavior influenced by sensory data
    updateSensoryBehavior(deltaTime, otherCreatures);

    // =========================================================================
    // NEURAL NETWORK BEHAVIOR MODULATION
    // This is where the evolved brain actually influences creature behavior!
    // The neural network outputs modulate aggression, fear, social, exploration
    // =========================================================================
    updateNeuralBehavior(foodPositions, otherCreatures);

    // Update behavior based on creature type
    if (type == CreatureType::HERBIVORE) {
        updateBehaviorHerbivore(deltaTime, foodPositions, otherCreatures, spatialGrid);
        updatePhysics(deltaTime, terrain);
    } else if (isAquatic(type)) {
        // All aquatic types use the aquatic behavior system
        updateBehaviorAquatic(deltaTime, otherCreatures, spatialGrid);
        // Aquatic creatures handle their own physics
    } else if (isFlying(type)) {
        updateBehaviorFlying(deltaTime, terrain, foodPositions, otherCreatures, spatialGrid);
        updateFlyingPhysics(deltaTime, terrain);
    } else {
        updateBehaviorCarnivore(deltaTime, otherCreatures, spatialGrid);
        updatePhysics(deltaTime, terrain);
    }

    // Calculate fitness
    calculateFitness();

    // Update physiological state for activity triggers
    updatePhysiologicalState(deltaTime);

    // Update activity system (eating, mating, sleeping, grooming, etc.)
    updateActivitySystem(deltaTime, foodPositions, otherCreatures);

    // Update animation system
    m_terrainPtr = &terrain;  // Store terrain pointer for ground raycasting
    updateAnimation(deltaTime);
}

void Creature::updateBehaviorHerbivore(float deltaTime,
                                        const std::vector<glm::vec3>& foodPositions,
                                        const std::vector<Creature*>& otherCreatures,
                                        const SpatialGrid* grid) {
    glm::vec3 steeringForce(0.0f);

    // =========================================================================
    // NEURAL NETWORK DRIVES BEHAVIOR (PRIMARY)
    // The NEAT brain outputs directly control movement and decisions
    // =========================================================================
    if (m_useNEATBrain && m_neatBrain) {
        // Get direct neural control values
        float neuralTurnAngle = m_neatMotorOutput.turnAngle * 3.14159f;  // -PI to PI
        float neuralSpeed = m_neatMotorOutput.speed;
        float fleeIntent = m_neatMotorOutput.fleeIntent;
        float eatIntent = m_neatMotorOutput.eatIntent;
        float socialAttraction = m_neatMotorOutput.socialAttraction;
        float restIntent = m_neatMotorOutput.restIntent;

        // === Primary movement from neural network ===
        // The neural network decides WHERE to go
        glm::vec3 neuralDirection(
            cos(rotation + neuralTurnAngle),
            0.0f,
            sin(rotation + neuralTurnAngle)
        );

        // Base movement from neural direction and speed
        steeringForce = neuralDirection * neuralSpeed * genome.speed;

        // === Flee behavior (enhanced when fleeIntent high) ===
        if (fleeIntent > 0.3f) {
            Creature* nearestPredator = findNearestCreature(otherCreatures, CreatureType::CARNIVORE, genome.visionRange, grid);
            if (nearestPredator != nullptr) {
                glm::vec3 evadeForce = steering.evasion(position, velocity,
                    nearestPredator->getPosition(), nearestPredator->getVelocity());
                // Neural flee intent scales the evasion response
                steeringForce += evadeForce * fleeIntent * 2.5f;
                fear = std::min(1.0f, fear + deltaTime * fleeIntent * 3.0f);

                // Speed boost proportional to flee intent
                SteeringBehaviors::Config config = steering.getConfig();
                config.maxSpeed = genome.speed * (1.0f + 0.4f * fleeIntent);
                steering.setConfig(config);
            }
        }

        // === Food seeking (when eatIntent high and not fleeing) ===
        if (eatIntent > 0.3f && fleeIntent < 0.5f && !foodPositions.empty()) {
            float nearestFoodDist = std::numeric_limits<float>::max();
            glm::vec3 nearestFoodPos;

            for (const auto& foodPos : foodPositions) {
                float dist = glm::length(foodPos - position);
                if (dist < nearestFoodDist && dist < genome.visionRange) {
                    nearestFoodDist = dist;
                    nearestFoodPos = foodPos;
                }
            }

            if (nearestFoodDist < genome.visionRange) {
                glm::vec3 arriveForce = steering.arrive(position, velocity, nearestFoodPos);
                steeringForce += arriveForce * eatIntent;
            }
        }

        // === Social behavior (flocking) scaled by neural social attraction ===
        if (std::abs(socialAttraction) > 0.1f) {
            std::vector<Creature*> herbivoreNeighbors = getNeighborsOfType(otherCreatures, CreatureType::HERBIVORE, genome.visionRange * 0.6f, grid);
            if (!herbivoreNeighbors.empty()) {
                if (socialAttraction > 0) {
                    // Positive = seek allies
                    glm::vec3 flockForce = steering.flock(position, velocity, herbivoreNeighbors,
                        1.0f, 0.5f * socialAttraction, 0.8f * socialAttraction);
                    steeringForce += flockForce * socialAttraction * 0.8f;
                } else {
                    // Negative = avoid allies (solitary)
                    glm::vec3 separateForce = steering.separate(position, velocity, herbivoreNeighbors);
                    steeringForce += separateForce * std::abs(socialAttraction) * 1.5f;
                }
            }
        }

        // === Rest behavior (slow down when rest intent high) ===
        if (restIntent > 0.5f) {
            steeringForce *= (1.0f - restIntent * 0.7f);  // Reduce movement
        }
    } else {
        // =========================================================================
        // FALLBACK: Legacy steering behaviors when NEAT brain not available
        // =========================================================================
        float fearModifier = 1.0f + m_neuralOutputs.fearMod * 0.5f;
        float socialModifier = 1.0f + m_neuralOutputs.socialMod * 0.5f;
        float exploreModifier = 1.0f + m_neuralOutputs.explorationMod * 0.5f;

        Creature* nearestPredator = findNearestCreature(otherCreatures, CreatureType::CARNIVORE, genome.visionRange, grid);
        if (nearestPredator != nullptr) {
            float predatorDist = glm::length(nearestPredator->getPosition() - position);
            if (predatorDist < genome.visionRange * 0.8f * fearModifier) {
                glm::vec3 evadeForce = steering.evasion(position, velocity,
                    nearestPredator->getPosition(), nearestPredator->getVelocity());
                steeringForce += evadeForce * 2.5f * fearModifier;
                fear = std::min(1.0f, fear + deltaTime * 3.0f * fearModifier);
            }
        }

        if (!foodPositions.empty() && fear < 0.5f) {
            float nearestFoodDist = std::numeric_limits<float>::max();
            glm::vec3 nearestFoodPos;
            for (const auto& foodPos : foodPositions) {
                float dist = glm::length(foodPos - position);
                if (dist < nearestFoodDist && dist < genome.visionRange) {
                    nearestFoodDist = dist;
                    nearestFoodPos = foodPos;
                }
            }
            if (nearestFoodDist < genome.visionRange) {
                glm::vec3 arriveForce = steering.arrive(position, velocity, nearestFoodPos);
                steeringForce += arriveForce;
            }
        }

        std::vector<Creature*> herbivoreNeighbors = getNeighborsOfType(otherCreatures, CreatureType::HERBIVORE, genome.visionRange * 0.6f, grid);
        if (!herbivoreNeighbors.empty()) {
            glm::vec3 flockForce = steering.flock(position, velocity, herbivoreNeighbors,
                1.5f * socialModifier, 0.8f * socialModifier, 0.8f * socialModifier);
            steeringForce += flockForce * 0.6f * socialModifier;
        }

        if (glm::length(steeringForce) < 0.1f) {
            glm::vec3 wanderForce = steering.wander(position, velocity, wanderTarget);
            steeringForce += wanderForce * 0.5f * exploreModifier;
        }
    }

    // Apply steering force to velocity
    velocity = steering.applyForce(velocity, steeringForce, deltaTime);

    // Update rotation to face movement direction
    if (glm::length(velocity) > 0.1f) {
        rotation = atan2(velocity.z, velocity.x);
    }

    // Reset speed boost
    SteeringBehaviors::Config config = steering.getConfig();
    config.maxSpeed = genome.speed;
    steering.setConfig(config);
}

void Creature::updateBehaviorCarnivore(float deltaTime,
                                        const std::vector<Creature*>& otherCreatures,
                                        const SpatialGrid* grid) {
    glm::vec3 steeringForce(0.0f);

    // =========================================================================
    // NEURAL NETWORK DRIVES BEHAVIOR (PRIMARY)
    // The NEAT brain outputs directly control hunting and movement decisions
    // =========================================================================
    if (m_useNEATBrain && m_neatBrain) {
        // Get direct neural control values
        float neuralTurnAngle = m_neatMotorOutput.turnAngle * 3.14159f;
        float neuralSpeed = m_neatMotorOutput.speed;
        float attackIntent = m_neatMotorOutput.attackIntent;
        float aggressionLevel = m_neatMotorOutput.aggressionLevel;
        float socialAttraction = m_neatMotorOutput.socialAttraction;
        float restIntent = m_neatMotorOutput.restIntent;

        // === Primary movement from neural network ===
        glm::vec3 neuralDirection(
            cos(rotation + neuralTurnAngle),
            0.0f,
            sin(rotation + neuralTurnAngle)
        );
        steeringForce = neuralDirection * neuralSpeed * genome.speed;

        // === Hunting behavior (driven by attackIntent and aggressionLevel) ===
        float huntingRange = genome.visionRange * (1.0f + aggressionLevel * 0.5f);
        Creature* nearestPrey = findNearestCreature(otherCreatures, CreatureType::HERBIVORE, huntingRange, grid);

        if (nearestPrey != nullptr && attackIntent > 0.3f) {
            float preyDist = glm::length(nearestPrey->getPosition() - position);
            nearestPrey->setBeingHunted(true);

            // Attack if close enough and attack intent is high
            if (preyDist < attackRange && huntingCooldown <= 0.0f && attackIntent > 0.5f) {
                attack(nearestPrey, deltaTime);
            } else {
                // Pursuit scaled by attack intent
                glm::vec3 pursuitForce = steering.pursuit(position, velocity,
                    nearestPrey->getPosition(), nearestPrey->getVelocity());
                float targetPriority = 1.0f + (1.0f - nearestPrey->getEnergy() / 200.0f) * 0.5f;
                steeringForce += pursuitForce * targetPriority * attackIntent * 2.0f;

                // Speed boost proportional to aggression
                SteeringBehaviors::Config config = steering.getConfig();
                config.maxSpeed = genome.speed * (1.0f + 0.3f * aggressionLevel);
                steering.setConfig(config);
            }
        }

        // === Territorial behavior (social attraction for carnivores usually negative) ===
        if (socialAttraction < -0.1f) {
            std::vector<Creature*> carnivoreNeighbors = getNeighborsOfType(otherCreatures, CreatureType::CARNIVORE, genome.visionRange * 0.5f, grid);
            if (!carnivoreNeighbors.empty()) {
                glm::vec3 separateForce = steering.separate(position, velocity, carnivoreNeighbors);
                steeringForce += separateForce * std::abs(socialAttraction) * 1.5f;
            }
        }

        // === Rest behavior ===
        if (restIntent > 0.5f) {
            steeringForce *= (1.0f - restIntent * 0.7f);
        }
    } else {
        // =========================================================================
        // FALLBACK: Legacy steering behaviors
        // =========================================================================
        float aggressionModifier = 1.0f + m_neuralOutputs.aggressionMod * 0.5f;
        float territorialModifier = 1.0f - m_neuralOutputs.socialMod * 0.3f;
        float exploreModifier = 1.0f + m_neuralOutputs.explorationMod * 0.5f;

        float huntingRange = genome.visionRange * 1.2f * aggressionModifier;
        Creature* nearestPrey = findNearestCreature(otherCreatures, CreatureType::HERBIVORE, huntingRange, grid);

        if (nearestPrey != nullptr) {
            float preyDist = glm::length(nearestPrey->getPosition() - position);
            nearestPrey->setBeingHunted(true);
            if (preyDist < attackRange && huntingCooldown <= 0.0f) {
                attack(nearestPrey, deltaTime);
            } else {
                glm::vec3 pursuitForce = steering.pursuit(position, velocity,
                    nearestPrey->getPosition(), nearestPrey->getVelocity());
                steeringForce += pursuitForce * 1.5f * aggressionModifier;
            }
        }

        std::vector<Creature*> carnivoreNeighbors = getNeighborsOfType(otherCreatures, CreatureType::CARNIVORE, genome.visionRange * 0.5f, grid);
        if (!carnivoreNeighbors.empty()) {
            glm::vec3 separateForce = steering.separate(position, velocity, carnivoreNeighbors);
            steeringForce += separateForce * 1.2f * territorialModifier;
        }

        if (nearestPrey == nullptr) {
            glm::vec3 wanderForce = steering.wander(position, velocity, wanderTarget);
            steeringForce += wanderForce * 0.8f * exploreModifier;
        }
    }

    // Apply steering force to velocity
    velocity = steering.applyForce(velocity, steeringForce, deltaTime);

    // Update rotation to face movement direction
    if (glm::length(velocity) > 0.1f) {
        rotation = atan2(velocity.z, velocity.x);
    }

    // Reset speed
    SteeringBehaviors::Config config = steering.getConfig();
    config.maxSpeed = genome.speed;
    steering.setConfig(config);
}

void Creature::updateBehaviorAquatic(float deltaTime, const std::vector<Creature*>& otherCreatures,
                                     const SpatialGrid* grid) {
    // Water level is at Y=10.5 (terrain waterLevel=0.35 * heightScale=30)
    // Fish swim BELOW this level (lower Y values)
    const float WATER_LEVEL = 10.5f;
    const float WATER_FLOOR = -10.0f;  // Approximate sea floor (below water)

    // Debug logging for aquatic creatures (once per 5 seconds)
    static float debugTimer = 0.0f;
    debugTimer += deltaTime;
    if (debugTimer > 5.0f && id % 10 == 0) {  // Log every 5 seconds for 1 in 10 fish
        std::cout << "[AQUATIC DEBUG] ID=" << id << " type=" << static_cast<int>(type)
                  << " pos=(" << position.x << "," << position.y << "," << position.z
                  << ") vel=" << glm::length(velocity) << " energy=" << energy << std::endl;
        debugTimer = 0.0f;
    }

    // =========================================================================
    // NEURAL NETWORK MODULATION FOR AQUATIC CREATURES
    // =========================================================================
    float fearModifier = 1.0f + m_neuralOutputs.fearMod * 0.5f;      // 0.5 to 1.5
    float socialModifier = 1.0f + m_neuralOutputs.socialMod * 0.5f;  // 0.5 to 1.5 (schooling)
    float aggressionModifier = 1.0f + m_neuralOutputs.aggressionMod * 0.5f;  // 0.5 to 1.5

    // Categorize nearby creatures
    std::vector<Creature*> nearbySchoolmates;  // Same species for schooling
    std::vector<Creature*> nearbyPredators;    // Threats to flee from
    std::vector<Creature*> nearbyPrey;         // Potential food

    float visionRange = genome.visionRange;
    float schoolRadius = visionRange * 0.6f * socialModifier;  // Neural modulates school size
    float predatorDetectRange = visionRange * 1.2f * fearModifier;  // Neural modulates detection
    float huntingRange = visionRange * 0.8f * aggressionModifier;  // Neural modulates hunting range

    // Determine if this creature is a predator
    bool isPredatorType = isAquaticPredator(type);

    for (Creature* other : otherCreatures) {
        if (other == this || !other->isAlive()) continue;

        float dist = glm::length(other->getPosition() - position);
        CreatureType otherType = other->getType();

        // Check for predators (aquatic predators that can hunt us)
        if (isAquatic(otherType) && canBeHuntedByAquatic(type, otherType, genome.size)) {
            if (dist < predatorDetectRange) {
                nearbyPredators.push_back(other);
            }
        }

        // Check for schoolmates (same type for schooling)
        if (isAquatic(otherType) && dist < schoolRadius) {
            // Only school with same sub-type or compatible types
            if (otherType == type ||
                (isAquaticPrey(type) && isAquaticPrey(otherType))) {
                nearbySchoolmates.push_back(other);
            }
        }

        // Check for prey (if we're a predator)
        if (isPredatorType && isAquatic(otherType)) {
            if (canBeHuntedByAquatic(otherType, type, other->getGenome().size)) {
                if (dist < huntingRange) {
                    nearbyPrey.push_back(other);
                }
            }
        }
    }

    glm::vec3 steeringForce(0.0f);
    bool isFleeing = false;
    bool isHunting = false;

    // === PRIORITY 1: FLEE FROM PREDATORS (NEURAL MODULATED) ===
    if (!nearbyPredators.empty() && !isPredatorType) {
        glm::vec3 fleeForce(0.0f);

        for (Creature* predator : nearbyPredators) {
            glm::vec3 toPredator = predator->getPosition() - position;
            float dist = glm::length(toPredator);

            if (dist > 0.01f) {
                // Flee strength inversely proportional to distance
                float urgency = 1.0f - (dist / predatorDetectRange);
                urgency *= urgency;  // Quadratic increase when close
                // Neural fear modifier affects flee intensity
                fleeForce -= glm::normalize(toPredator) * urgency * 3.0f * fearModifier;
            }
        }

        if (glm::length(fleeForce) > 0.01f) {
            // Neural fear modifier affects overall flee response
            steeringForce += glm::normalize(fleeForce) * 4.0f * fearModifier;
            isFleeing = true;
            fear = std::min(1.0f, fear + 0.3f * deltaTime * fearModifier);

            // Scatter effect - add some randomness when panicked
            float scatter = Random::range(-0.5f, 0.5f);
            steeringForce.x += scatter;
            steeringForce.z += scatter * 0.8f;
        }
    }

    // === PRIORITY 2: HUNT PREY (NEURAL MODULATED for predators) ===
    if (isPredatorType && !nearbyPrey.empty() && huntingCooldown <= 0.0f) {
        // Find closest/weakest prey
        Creature* targetPrey = nullptr;
        float bestScore = -1.0f;

        for (Creature* prey : nearbyPrey) {
            float dist = glm::length(prey->getPosition() - position);
            // Prefer closer and smaller prey
            float score = (huntingRange - dist) / huntingRange + (1.0f - prey->getGenome().size);
            if (score > bestScore) {
                bestScore = score;
                targetPrey = prey;
            }
        }

        if (targetPrey) {
            glm::vec3 toPrey = targetPrey->getPosition() - position;
            float dist = glm::length(toPrey);

            if (dist > 0.01f) {
                // Pursuit behavior
                glm::vec3 pursuitDir = glm::normalize(toPrey);

                // Predict prey position
                glm::vec3 preyVel = targetPrey->getVelocity();
                float predictionTime = dist / (genome.speed + 0.1f);
                glm::vec3 predictedPos = targetPrey->getPosition() + preyVel * predictionTime * 0.5f;
                glm::vec3 toPredicted = predictedPos - position;

                if (glm::length(toPredicted) > 0.01f) {
                    // Neural aggression modifier affects pursuit strength
                    steeringForce += glm::normalize(toPredicted) * 2.5f * aggressionModifier;
                } else {
                    steeringForce += pursuitDir * 2.5f * aggressionModifier;
                }

                isHunting = true;

                // Attack if close enough
                if (dist < attackRange) {
                    attack(targetPrey, deltaTime);
                }
            }
        }
    }

    // === SCHOOLING BEHAVIOR (NEURAL MODULATED - when not fleeing or hunting) ===
    if (!isFleeing && !isHunting && !nearbySchoolmates.empty()) {
        glm::vec3 separation(0.0f);
        glm::vec3 alignment(0.0f);
        glm::vec3 cohesion(0.0f);
        glm::vec3 centerOfMass(0.0f);

        float separationDist = genome.size * 3.0f;
        int alignmentCount = 0;

        for (Creature* fish : nearbySchoolmates) {
            glm::vec3 toFish = fish->getPosition() - position;
            float dist = glm::length(toFish);

            // Separation: avoid crowding neighbors
            if (dist < separationDist && dist > 0.01f) {
                separation -= glm::normalize(toFish) / dist;
            }

            // Alignment: steer toward average heading
            if (glm::length(fish->getVelocity()) > 0.1f) {
                alignment += fish->getVelocity();
                alignmentCount++;
            }

            // Cohesion: steer toward center of mass
            centerOfMass += fish->getPosition();
        }

        // Average the alignment and cohesion
        if (alignmentCount > 0) {
            alignment /= static_cast<float>(alignmentCount);
        }
        centerOfMass /= static_cast<float>(nearbySchoolmates.size());
        cohesion = centerOfMass - position;

        // Normalize and apply weights
        if (glm::length(separation) > 0.01f) separation = glm::normalize(separation);
        if (glm::length(alignment) > 0.01f) alignment = glm::normalize(alignment);
        if (glm::length(cohesion) > 0.01f) cohesion = glm::normalize(cohesion);

        // Neural network modulates schooling strength via socialModifier
        float schoolStrength = genome.schoolingStrength * socialModifier;

        // Predators school less strongly
        if (isPredatorType) {
            schoolStrength *= 0.3f;
        }

        // Apply neural-modulated schooling forces
        steeringForce += separation * 2.5f * schoolStrength;
        steeringForce += alignment * 1.5f * schoolStrength;
        steeringForce += cohesion * 1.0f * schoolStrength;
    }

    // === DEPTH CONTROL ===
    float targetY = WATER_LEVEL - genome.preferredDepth * 15.0f;

    // Adjust depth when fleeing (go deeper to escape)
    if (isFleeing) {
        targetY -= 3.0f;
    }

    float depthError = targetY - position.y;
    steeringForce.y += depthError * 5.0f;

    // Hard limits - avoid surface and floor
    if (position.y > WATER_LEVEL - 0.5f) {
        steeringForce.y -= 20.0f;
    }
    if (position.y < WATER_FLOOR + 0.5f) {
        steeringForce.y += 15.0f;
    }

    // === WANDER (when not doing other behaviors) ===
    if (!isFleeing && !isHunting) {
        m_wanderAngle += Random::range(-0.5f, 0.5f) * deltaTime * 3.0f;
        glm::vec3 wander(cos(m_wanderAngle), 0.0f, sin(m_wanderAngle));
        steeringForce += wander * 0.5f;
    }

    // === BOUNDARY AVOIDANCE ===
    float boundaryRange = 150.0f;
    if (std::abs(position.x) > boundaryRange) {
        steeringForce.x -= glm::sign(position.x) * 10.0f;
    }
    if (std::abs(position.z) > boundaryRange) {
        steeringForce.z -= glm::sign(position.z) * 10.0f;
    }

    // === APPLY PHYSICS ===
    float maxSpeed = genome.speed;

    // Speed boost when fleeing
    if (isFleeing) {
        maxSpeed *= 1.4f;
    } else if (isHunting) {
        maxSpeed *= 1.2f;
    }

    // Water drag (higher than air)
    float dragCoeff = 0.3f;
    velocity *= (1.0f - dragCoeff * deltaTime);

    // Apply steering force
    velocity += steeringForce * deltaTime;

    // Limit speed
    float currentSpeed = glm::length(velocity);
    if (currentSpeed > maxSpeed) {
        velocity = glm::normalize(velocity) * maxSpeed;
    }

    // Minimum movement (fish keep moving)
    if (currentSpeed < maxSpeed * 0.2f && !isFleeing) {
        glm::vec3 forward(sin(rotation), 0.0f, cos(rotation));
        velocity = forward * maxSpeed * 0.3f;
    }

    // Update position
    position += velocity * deltaTime;

    // Clamp to water bounds
    position.y = std::clamp(position.y, WATER_FLOOR + 0.5f, WATER_LEVEL - 0.5f);

    // Update rotation to face movement direction
    if (glm::length(glm::vec2(velocity.x, velocity.z)) > 0.1f) {
        rotation = atan2(velocity.x, velocity.z);
    }

    // === ENERGY CONSUMPTION ===
    float energyCost = 0.5f;  // Base cost
    energyCost += currentSpeed * 0.02f;  // Speed cost

    if (isFleeing) {
        energyCost *= 1.5f;  // Fleeing is exhausting
    } else if (isHunting) {
        energyCost *= 1.3f;  // Hunting is tiring
    }

    energy -= energyCost * genome.efficiency * deltaTime;

    // Decay fear over time
    fear = std::max(0.0f, fear - 0.5f * deltaTime);

    // Death check
    if (energy <= 0.0f) {
        energy = 0.0f;
        alive = false;
    }

    distanceTraveled += currentSpeed * deltaTime;
}

void Creature::attack(Creature* target, float deltaTime) {
    if (!target || !target->isAlive()) return;

    float damage = attackDamage * deltaTime;
    target->takeDamage(damage);

    // Small energy cost for attacking
    energy -= 1.0f * deltaTime;

    // Check if we killed the target
    if (!target->isAlive()) {
        energy = std::min(energy + killEnergyGain, maxEnergy);
        killCount++;
        std::cout << "[Carnivore " << id << "] Killed herbivore! Kills: " << killCount << std::endl;

        // Notify NEAT brain of successful hunt
        if (m_useNEATBrain && m_neatBrain) {
            m_neatBrain->onSuccessfulHunt();
        }
    }

    huntingCooldown = attackCooldown;
}

void Creature::takeDamage(float damage) {
    energy -= damage;
    fear = 1.0f;  // Maximum fear when taking damage

    // Notify NEAT brain for learning
    if (m_useNEATBrain && m_neatBrain) {
        m_neatBrain->onDamageTaken(damage / 50.0f);  // Normalized penalty
    }

    if (energy <= 0.0f) {
        alive = false;
    }
}

void Creature::updatePhysics(float deltaTime, const Terrain& terrain) {
    glm::vec3 oldPos = position;

    // Update position
    position += velocity * deltaTime;

    // Track distance traveled
    distanceTraveled += glm::length(position - oldPos);

    // Terrain boundaries
    float halfWidth = terrain.getWidth() * terrain.getScale() * 0.5f;
    float halfDepth = terrain.getDepth() * terrain.getScale() * 0.5f;

    // Apply boundary avoidance force
    glm::vec3 boundaryForce = steering.avoidBoundary(position, velocity,
        terrain.getWidth() * terrain.getScale(), terrain.getDepth() * terrain.getScale());
    velocity += boundaryForce * deltaTime;

    position.x = std::clamp(position.x, -halfWidth + 1.0f, halfWidth - 1.0f);
    position.z = std::clamp(position.z, -halfDepth + 1.0f, halfDepth - 1.0f);

    // Stay on terrain (avoid water)
    if (terrain.isWater(position.x, position.z)) {
        position = oldPos;
        velocity *= -0.5f;  // Bounce back
    } else {
        position.y = terrain.getHeight(position.x, position.z) + genome.size;
    }
}

bool Creature::canReproduce() const {
    // Sterile individuals cannot reproduce
    if (sterile) return false;

    if (type == CreatureType::HERBIVORE) {
        return energy > herbivoreReproductionThreshold;
    } else if (type == CreatureType::FLYING) {
        // Flying creatures need energy and at least 1 kill (omnivore)
        return energy > flyingReproductionThreshold && killCount >= minKillsToReproduceFlying;
    } else {
        // Carnivores need both energy and kills to reproduce
        return energy > carnivoreReproductionThreshold && killCount >= minKillsToReproduce;
    }
}

void Creature::consumeFood(float amount) {
    energy = std::min(energy + amount, maxEnergy);
    foodEaten++;
    m_timeSinceLastMeal = 0.0f;  // Reset time since last meal

    // Notify NEAT brain for learning
    if (m_useNEATBrain && m_neatBrain) {
        m_neatBrain->onFoodEaten(amount / 50.0f);  // Normalized reward
    }
}

void Creature::reproduce(float& energyCost) {
    if (type == CreatureType::HERBIVORE) {
        energy -= herbivoreReproductionCost;
        energyCost = herbivoreReproductionCost;
    } else if (type == CreatureType::FLYING) {
        energy -= flyingReproductionCost;
        energyCost = flyingReproductionCost;
        killCount = 0;  // Reset kill count after reproduction
    } else {
        energy -= carnivoreReproductionCost;
        energyCost = carnivoreReproductionCost;
        killCount = 0;  // Reset kill count after reproduction
    }
}

// ============================================
// Mate Preference Methods (C-17 fix)
// ============================================

bool Creature::canMateWith(const Creature& other) const {
    // Basic compatibility checks
    if (!other.isAlive()) return false;
    if (other.getID() == id) return false;  // Can't mate with self

    // Must be same general type category
    // Flying with flying, aquatic with aquatic, terrestrial with terrestrial
    if (isFlying(type) != isFlying(other.type)) return false;
    if (isAquatic(type) != isAquatic(other.type)) return false;

    // Both must be able to reproduce
    if (!canReproduce() || !other.canReproduce()) return false;

    // Species compatibility check
    float geneticDistance = diploidGenome.distanceTo(other.diploidGenome);
    if (geneticDistance > 0.8f) {
        return false;  // Too genetically different
    }

    return true;
}

float Creature::evaluateMateAttraction(const Creature& other) const {
    // Get mate preferences from genome
    genetics::MatePreferences prefs = diploidGenome.getMatePreferences();
    float attraction = 0.0f;

    // Get phenotypes for comparison
    genetics::Phenotype myPhenotype = diploidGenome.express();
    genetics::Phenotype otherPhenotype = other.diploidGenome.express();

    // ====================================
    // Size preference (-1 to 1: smaller to larger)
    // ====================================
    float sizeDiff = otherPhenotype.size - myPhenotype.size;
    float sizeMatch = 1.0f - std::abs(sizeDiff - prefs.sizePreference);
    sizeMatch = std::max(0.0f, sizeMatch);
    attraction += sizeMatch * 0.25f;

    // ====================================
    // Ornament preference
    // ====================================
    float ornamentScore = otherPhenotype.ornamentIntensity;
    if (prefs.ornamentPreference > 0) {
        // Prefers more ornamented mates
        attraction += ornamentScore * prefs.ornamentPreference * 0.25f;
    } else if (prefs.ornamentPreference < 0) {
        // Prefers less ornamented mates
        attraction += (1.0f - ornamentScore) * std::abs(prefs.ornamentPreference) * 0.25f;
    }

    // ====================================
    // Similarity preference
    // ====================================
    float colorDistance = glm::length(myPhenotype.color - otherPhenotype.color);
    colorDistance = std::min(colorDistance, 1.0f);  // Normalize

    if (prefs.similarityPreference > 0) {
        // Prefers similar mates (assortative mating)
        attraction += (1.0f - colorDistance) * prefs.similarityPreference * 0.2f;
    } else if (prefs.similarityPreference < 0) {
        // Prefers different mates (disassortative mating)
        attraction += colorDistance * std::abs(prefs.similarityPreference) * 0.2f;
    }

    // ====================================
    // Fitness preference (general quality indicator)
    // ====================================
    float otherFitness = other.getFitness();
    float normalizedFitness = std::min(otherFitness / 1000.0f, 1.0f);  // Normalize
    if (normalizedFitness > prefs.minimumAcceptable) {
        attraction += 0.15f;  // Base acceptance
        attraction += (normalizedFitness - prefs.minimumAcceptable) * 0.15f;  // Quality bonus
    }

    // ====================================
    // Genetic distance preference (avoid inbreeding)
    // ====================================
    float geneticDistance = diploidGenome.distanceTo(other.diploidGenome);
    // Sweet spot: not too similar (inbreeding), not too different (incompatibility)
    float distanceScore = 1.0f - std::abs(geneticDistance - 0.3f) * 2.0f;
    distanceScore = std::max(0.0f, distanceScore);
    attraction += distanceScore * 0.1f;

    return std::clamp(attraction, 0.0f, 1.0f);
}

bool Creature::willMateWith(const Creature& other) const {
    // First check basic compatibility
    if (!canMateWith(other)) return false;

    // Get mate preferences
    genetics::MatePreferences prefs = diploidGenome.getMatePreferences();

    // Calculate attraction score
    float attraction = evaluateMateAttraction(other);

    // Apply choosiness threshold
    float acceptanceThreshold = prefs.choosiness * 0.5f;

    // Probabilistic acceptance based on attraction vs threshold
    // Higher attraction = higher chance of acceptance
    // Higher choosiness = higher threshold to meet
    if (attraction < acceptanceThreshold) {
        return false;  // Below minimum acceptable
    }

    // Probabilistic acceptance for marginal cases
    float acceptanceChance = (attraction - acceptanceThreshold) / (1.0f - acceptanceThreshold);
    acceptanceChance = std::clamp(acceptanceChance, 0.0f, 1.0f);

    // Use a deterministic but pseudo-random decision based on IDs
    // This ensures consistency if called multiple times for same pair
    unsigned int seed = static_cast<unsigned int>(id * 31 + other.getID());
    float randomValue = static_cast<float>(seed % 1000) / 1000.0f;

    return randomValue < acceptanceChance;
}

void Creature::calculateFitness() {
    // =========================================================================
    // FITNESS CALCULATION - Evolution pressure for neural network learning
    // Higher fitness = more likely to reproduce and pass on brain weights
    // =========================================================================

    // === BASE FITNESS: Survival is paramount ===
    // Creatures that survive longer are doing SOMETHING right
    float survivalFitness = age * 1.0f;  // 1 point per time unit survived

    // === ENERGY EFFICIENCY: Energy management shows smart behavior ===
    // High energy = found food, low energy expenditure
    float energyFitness = (energy / maxEnergy) * 20.0f;

    // === FOOD ACQUISITION: Primary evolutionary pressure ===
    // Food eaten is direct evidence of successful foraging behavior
    float foodFitness = foodEaten * 15.0f;  // Increased from 10

    // === EXPLORATION: Finding new resources ===
    float explorationFitness = std::min(distanceTraveled * 0.02f, 50.0f);  // Capped to prevent wandering

    // Combine base fitness
    fitness = survivalFitness + energyFitness + foodFitness + explorationFitness;

    // === TYPE-SPECIFIC FITNESS BONUSES ===
    if (type == CreatureType::HERBIVORE) {
        // Herbivores: reward for avoiding predators (surviving with predators nearby)
        // If they're alive and have eaten, they've successfully avoided predators
        if (age > 20.0f && foodEaten > 5) {
            fitness += 30.0f;  // Bonus for long-term survival with food acquisition
        }

        // Reward for energy efficiency (not wasting energy fleeing constantly)
        if (energy > maxEnergy * 0.6f) {
            fitness += 15.0f;  // Maintaining high energy is hard while foraging
        }
    }

    if (type == CreatureType::CARNIVORE) {
        // Carnivores: hunting success is THE metric
        fitness += killCount * 75.0f;  // Increased from 50 - kills are HARD

        // Bonus for efficiency - kills per time survived
        if (age > 10.0f && killCount > 0) {
            float huntingEfficiency = (float)killCount / (age * 0.1f);
            fitness += huntingEfficiency * 50.0f;
        }
    }

    if (type == CreatureType::FLYING) {
        // Flying: kills + aerial efficiency
        fitness += killCount * 60.0f;
        fitness += distanceTraveled * 0.03f;  // Extra exploration bonus

        // Bonus for using altitude effectively (energy-efficient flight)
        if (age > 15.0f) {
            fitness += 20.0f;
        }
    }

    // === OFFSPRING BONUS: Reproductive success is ultimate fitness ===
    // Creatures that reproduce successfully have proven their survival strategy
    // Note: offspringCount would need to be tracked - add if not present

    // === PENALTIES ===
    // Penalize creatures that died young (didn't figure out survival)
    if (!alive && age < 10.0f) {
        fitness *= 0.5f;  // Halve fitness for early death
    }

    // Apply genetic modifiers (inbreeding depression, hybrid vigor, etc.)
    fitness *= fitnessModifier;

    // Ensure minimum fitness for evolution to work
    fitness = std::max(0.1f, fitness);

    // =========================================================================
    // UPDATE NEAT BRAIN WITH FITNESS
    // =========================================================================
    if (m_useNEATBrain && m_neatBrain) {
        m_neatBrain->getGenome().setFitness(fitness);

        // Give immediate reward signal for online learning
        // Scale reward by how much fitness improved
        static float lastFitness = 0.0f;
        float fitnessGain = fitness - lastFitness;
        if (fitnessGain > 0) {
            m_neatBrain->learn(fitnessGain * 0.01f);
        }
        lastFitness = fitness;
    }
}

Creature* Creature::findNearestCreature(const std::vector<Creature*>& creatures,
                                         CreatureType targetType, float maxRange,
                                         const SpatialGrid* grid) const {
    // Use SpatialGrid for O(1) average-case performance when available
    if (grid) {
        Creature* nearest = grid->findNearest(position, maxRange, static_cast<int>(targetType));
        // Ensure we don't return ourselves
        if (nearest == this) {
            return nullptr;
        }
        return nearest;
    }

    // Fallback to O(n) linear scan when SpatialGrid is not available
    Creature* nearest = nullptr;
    float nearestDist = maxRange;

    for (Creature* other : creatures) {
        if (other == this || !other->isAlive() || other->getType() != targetType) continue;

        float dist = glm::length(other->getPosition() - position);
        if (dist < nearestDist) {
            nearestDist = dist;
            nearest = other;
        }
    }

    return nearest;
}

std::vector<Creature*> Creature::getNeighborsOfType(const std::vector<Creature*>& creatures,
                                                      CreatureType targetType, float range,
                                                      const SpatialGrid* grid) const {
    // Use SpatialGrid for O(1) average-case performance when available
    if (grid) {
        std::vector<Creature*> neighbors = grid->queryByType(position, range, static_cast<int>(targetType));
        // Remove ourselves from the result if present
        neighbors.erase(
            std::remove(neighbors.begin(), neighbors.end(), const_cast<Creature*>(this)),
            neighbors.end()
        );
        return neighbors;
    }

    // Fallback to O(n) linear scan when SpatialGrid is not available
    std::vector<Creature*> neighbors;

    for (Creature* other : creatures) {
        if (other == this || !other->isAlive() || other->getType() != targetType) continue;

        float dist = glm::length(other->getPosition() - position);
        if (dist < range) {
            neighbors.push_back(other);
        }
    }

    return neighbors;
}

void Creature::render(uint32_t vaoHandle) {
    // Rendering handled by main simulation
}

// =============================================================================
// SPECIES-BASED VISUAL DIFFERENTIATION
// =============================================================================

glm::vec3 Creature::getSpeciesTintedColor() const {
    // Get base color from genome
    glm::vec3 baseColor = genome.color;

    // Get species ID
    genetics::SpeciesId speciesId = diploidGenome.getSpeciesId();
    if (speciesId == 0) {
        // No species assigned, return base color
        return baseColor;
    }

    // Generate species color from species ID (HSV to RGB)
    // Use golden ratio to spread colors evenly
    float hue = fmod(speciesId * 0.618033988749895f, 1.0f);
    float saturation = 0.7f;
    float value = 0.9f;

    // HSV to RGB conversion
    int hi = static_cast<int>(hue * 6.0f);
    float f = hue * 6.0f - hi;
    float p = value * (1.0f - saturation);
    float q = value * (1.0f - f * saturation);
    float t = value * (1.0f - (1.0f - f) * saturation);

    glm::vec3 speciesColor;
    switch (hi % 6) {
        case 0: speciesColor = glm::vec3(value, t, p); break;
        case 1: speciesColor = glm::vec3(q, value, p); break;
        case 2: speciesColor = glm::vec3(p, value, t); break;
        case 3: speciesColor = glm::vec3(p, q, value); break;
        case 4: speciesColor = glm::vec3(t, p, value); break;
        case 5: speciesColor = glm::vec3(value, p, q); break;
        default: speciesColor = glm::vec3(1.0f); break;
    }

    // Mix base color with species color (30% species tint)
    float mixFactor = 0.3f;
    return glm::mix(baseColor, speciesColor, mixFactor);
}

int Creature::getSpeciesPatternType() const {
    // Get species ID to determine pattern
    genetics::SpeciesId speciesId = diploidGenome.getSpeciesId();

    // 0=solid, 1=stripes, 2=spots, 3=gradient
    return static_cast<int>(speciesId) % 4;
}

// =============================================================================
// NEURAL NETWORK INTEGRATION - This is where the brain actually gets USED!
// =============================================================================

std::vector<float> Creature::gatherNeuralInputs(const std::vector<glm::vec3>& foodPositions,
                                                 const std::vector<Creature*>& otherCreatures) const {
    std::vector<float> inputs;
    inputs.reserve(8);

    // Input 0: Distance to nearest food (normalized 0-1, 0 = at food, 1 = far/none)
    float nearestFoodDist = genome.visionRange;
    float nearestFoodAngle = 0.0f;
    for (const auto& foodPos : foodPositions) {
        float dist = glm::length(foodPos - position);
        if (dist < nearestFoodDist) {
            nearestFoodDist = dist;
            glm::vec3 toFood = foodPos - position;
            nearestFoodAngle = atan2(toFood.z, toFood.x) - rotation;
            // Normalize angle to [-PI, PI]
            while (nearestFoodAngle > 3.14159f) nearestFoodAngle -= 6.28318f;
            while (nearestFoodAngle < -3.14159f) nearestFoodAngle += 6.28318f;
        }
    }
    inputs.push_back(1.0f - (nearestFoodDist / genome.visionRange));  // Input 0: Food proximity

    // Input 1: Angle to nearest food (normalized -1 to 1)
    inputs.push_back(nearestFoodAngle / 3.14159f);  // Input 1: Food direction

    // Input 2-3: Nearest threat (predator for herbivores, larger creature for carnivores)
    float nearestThreatDist = genome.visionRange;
    float nearestThreatAngle = 0.0f;

    for (Creature* other : otherCreatures) {
        if (other == this || !other->isAlive()) continue;

        // For herbivores: carnivores are threats
        // For carnivores: other larger carnivores could be threats
        bool isThreat = false;
        if (type == CreatureType::HERBIVORE && other->getType() == CreatureType::CARNIVORE) {
            isThreat = true;
        } else if (type == CreatureType::CARNIVORE && other->getType() == CreatureType::CARNIVORE) {
            if (other->getGenome().size > genome.size * 1.2f) {
                isThreat = true;  // Larger carnivore is a threat
            }
        }

        if (isThreat) {
            float dist = glm::length(other->getPosition() - position);
            if (dist < nearestThreatDist) {
                nearestThreatDist = dist;
                glm::vec3 toThreat = other->getPosition() - position;
                nearestThreatAngle = atan2(toThreat.z, toThreat.x) - rotation;
                while (nearestThreatAngle > 3.14159f) nearestThreatAngle -= 6.28318f;
                while (nearestThreatAngle < -3.14159f) nearestThreatAngle += 6.28318f;
            }
        }
    }
    inputs.push_back(1.0f - (nearestThreatDist / genome.visionRange));  // Input 2: Threat proximity
    inputs.push_back(nearestThreatAngle / 3.14159f);  // Input 3: Threat direction

    // Input 4: Current energy level (normalized 0-1)
    inputs.push_back(energy / maxEnergy);

    // Input 5: Current speed (normalized 0-1)
    float currentSpeed = glm::length(velocity);
    inputs.push_back(std::min(1.0f, currentSpeed / (genome.speed + 0.001f)));

    // Input 6: Number of allies nearby (same type, normalized)
    int allyCount = 0;
    for (Creature* other : otherCreatures) {
        if (other == this || !other->isAlive()) continue;
        if (other->getType() == type) {
            float dist = glm::length(other->getPosition() - position);
            if (dist < genome.visionRange * 0.5f) {
                allyCount++;
            }
        }
    }
    inputs.push_back(std::min(1.0f, allyCount / 5.0f));  // Input 6: Ally density (capped at 5)

    // Input 7: Current fear level
    inputs.push_back(fear);  // Input 7: Fear level

    return inputs;
}

void Creature::updateNeuralBehavior(const std::vector<glm::vec3>& foodPositions,
                                     const std::vector<Creature*>& otherCreatures) {
    if (!m_useNeuralBehavior) return;

    // =========================================================================
    // BUILD EXPANDED SENSORY INPUT (27 inputs for neural network)
    // The neural network is now the PRIMARY decision maker!
    // =========================================================================
    ai::SensoryInput sensoryInput;

    // === Vision inputs: distances and angles to key targets ===
    float nearestFoodDist = genome.visionRange;
    float nearestFoodAngle = 0.0f;
    for (const auto& foodPos : foodPositions) {
        float dist = glm::length(foodPos - position);
        if (dist < nearestFoodDist) {
            nearestFoodDist = dist;
            glm::vec3 toFood = foodPos - position;
            nearestFoodAngle = atan2(toFood.z, toFood.x) - rotation;
            // Normalize angle to [-PI, PI]
            while (nearestFoodAngle > 3.14159f) nearestFoodAngle -= 6.28318f;
            while (nearestFoodAngle < -3.14159f) nearestFoodAngle += 6.28318f;
        }
    }
    sensoryInput.nearestFoodDistance = 1.0f - nearestFoodDist / genome.visionRange;
    sensoryInput.nearestFoodAngle = nearestFoodAngle / 3.14159f;

    // Scan nearby creatures for predators, prey, allies, and potential mates
    float nearestPredatorDist = genome.visionRange;
    float nearestPredatorAngle = 0.0f;
    float nearestPreyDist = genome.visionRange;
    float nearestPreyAngle = 0.0f;
    float nearestAllyDist = genome.visionRange;
    float nearestAllyAngle = 0.0f;
    float nearestMateDist = genome.visionRange;
    float nearestMateAngle = 0.0f;

    int nearbyCreatureCount = 0;
    int nearbyPredatorCount = 0;
    int nearbyPreyCount = 0;
    int nearbyAllyCount = 0;

    for (Creature* other : otherCreatures) {
        if (other == this || !other->isAlive()) continue;

        float dist = glm::length(other->getPosition() - position);
        if (dist > genome.visionRange) continue;

        nearbyCreatureCount++;

        glm::vec3 toOther = other->getPosition() - position;
        float angle = atan2(toOther.z, toOther.x) - rotation;
        while (angle > 3.14159f) angle -= 6.28318f;
        while (angle < -3.14159f) angle += 6.28318f;

        // Check for predators
        bool isPredator = false;
        if (type == CreatureType::HERBIVORE && other->getType() == CreatureType::CARNIVORE) {
            isPredator = true;
        } else if (type == CreatureType::CARNIVORE && other->getType() == CreatureType::CARNIVORE) {
            if (other->getGenome().size > genome.size * 1.2f) {
                isPredator = true;
            }
        }
        if (isPredator) {
            nearbyPredatorCount++;
            if (dist < nearestPredatorDist) {
                nearestPredatorDist = dist;
                nearestPredatorAngle = angle;
            }
        }

        // Check for prey (carnivores hunting herbivores/smaller creatures)
        bool isPrey = false;
        if (type == CreatureType::CARNIVORE && other->getType() == CreatureType::HERBIVORE) {
            isPrey = true;
        } else if (type == CreatureType::FLYING && other->getType() == CreatureType::HERBIVORE) {
            isPrey = true;
        }
        if (isPrey) {
            nearbyPreyCount++;
            if (dist < nearestPreyDist) {
                nearestPreyDist = dist;
                nearestPreyAngle = angle;
            }
        }

        // Check for allies (same type)
        if (other->getType() == type) {
            nearbyAllyCount++;
            if (dist < nearestAllyDist) {
                nearestAllyDist = dist;
                nearestAllyAngle = angle;
            }

            // Check for potential mates
            if (canMateWith(*other) && dist < nearestMateDist) {
                nearestMateDist = dist;
                nearestMateAngle = angle;
            }
        }
    }

    sensoryInput.nearestPredatorDistance = 1.0f - nearestPredatorDist / genome.visionRange;
    sensoryInput.nearestPredatorAngle = nearestPredatorAngle / 3.14159f;
    sensoryInput.nearestPreyDistance = 1.0f - nearestPreyDist / genome.visionRange;
    sensoryInput.nearestPreyAngle = nearestPreyAngle / 3.14159f;
    sensoryInput.nearestAllyDistance = 1.0f - nearestAllyDist / genome.visionRange;
    sensoryInput.nearestAllyAngle = nearestAllyAngle / 3.14159f;

    // === Internal state inputs ===
    sensoryInput.energy = energy / maxEnergy;
    sensoryInput.health = 1.0f;  // Currently creatures don't have separate health
    sensoryInput.age = std::min(1.0f, age / 100.0f);
    sensoryInput.currentSpeed = std::min(1.0f, glm::length(velocity) / (genome.speed + 0.001f));
    sensoryInput.hungerLevel = 1.0f - (energy / maxEnergy);

    // === Environmental inputs ===
    sensoryInput.terrainHeight = 0.5f;  // Could be filled from terrain data
    sensoryInput.waterProximity = 0.0f;  // Could be filled from terrain
    sensoryInput.temperature = 0.5f;  // Could be filled from climate system
    sensoryInput.dayNightCycle = 0.5f;  // Could be filled from day/night cycle

    // === Social/situational awareness inputs ===
    sensoryInput.nearbyCreatureCount = std::min(1.0f, nearbyCreatureCount / 10.0f);
    sensoryInput.nearbyPredatorCount = std::min(1.0f, nearbyPredatorCount / 5.0f);
    sensoryInput.nearbyPreyCount = std::min(1.0f, nearbyPreyCount / 10.0f);
    sensoryInput.nearbyAllyCount = std::min(1.0f, nearbyAllyCount / 10.0f);

    // === Memory/state inputs ===
    sensoryInput.wasAttacked = fear > 0.5f ? 1.0f : 0.0f;
    sensoryInput.recentFoodEaten = foodEaten > 0 ? 1.0f : 0.0f;
    sensoryInput.fear = fear;
    sensoryInput.timeSinceLastMeal = std::min(1.0f, m_timeSinceLastMeal / 30.0f);

    // === Mate detection inputs ===
    sensoryInput.nearestMateDistance = 1.0f - nearestMateDist / genome.visionRange;
    sensoryInput.nearestMateAngle = nearestMateAngle / 3.14159f;

    // =========================================================================
    // PROCESS THROUGH NEAT BRAIN - PRIMARY DECISION MAKER
    // =========================================================================
    if (m_useNEATBrain && m_neatBrain) {
        // Use the expanded input/output NEAT brain
        m_neatMotorOutput = m_neatBrain->processExpanded(sensoryInput, 0.016f);

        // Also update legacy neural outputs for compatibility with existing behavior code
        m_neuralOutputs.turnAngle = m_neatMotorOutput.turnAngle;
        m_neuralOutputs.speedMultiplier = m_neatMotorOutput.speed;
        m_neuralOutputs.aggressionMod = m_neatMotorOutput.aggressionLevel * 2.0f - 1.0f;
        m_neuralOutputs.fearMod = m_neatMotorOutput.fleeIntent * 2.0f - 1.0f;
        m_neuralOutputs.socialMod = m_neatMotorOutput.socialAttraction;
        m_neuralOutputs.explorationMod = (1.0f - m_neatMotorOutput.restIntent) * 2.0f - 1.0f;

        // Report events to brain for learning
        if (fear > 0.3f) {
            m_neatBrain->onThreatDetected(fear);
        }
    } else if (brain) {
        // Fallback: Use simple fixed-topology neural network
        std::vector<float> inputs = gatherNeuralInputs(foodPositions, otherCreatures);
        m_neuralOutputs = brain->forward(inputs);
    }

    // Neural outputs are now cached and will DRIVE behavior (not just modulate)
}

void Creature::updateSensoryBehavior(float deltaTime, const std::vector<Creature*>& otherCreatures) {
    // Use sensory percepts to modulate behavior

    // Check for threats detected by any sense
    if (sensory.hasThreatNearby()) {
        auto threat = sensory.getNearestThreat();

        // Increase fear based on threat proximity and confidence
        float threatIntensity = threat.confidence * (1.0f - threat.distance / genome.visionRange);
        fear = std::min(1.0f, fear + threatIntensity * deltaTime * 3.0f);

        // If we detected a predator by smell alone (not seen), be more cautious
        if (threat.sensedBy == SensoryType::SMELL && threat.confidence > 0.3f) {
            // Predator is nearby but not visible - increase alertness
            fear = std::min(1.0f, fear + 0.5f * deltaTime);
        }

        // If we heard an alarm call, react to it
        auto percepts = sensory.getPerceptsByType(DetectionType::DANGER_ZONE);
        for (const auto& p : percepts) {
            if (p.sensedBy == SensoryType::HEARING) {
                // Another creature sounded an alarm - remember the danger zone
                fear = std::min(1.0f, fear + 0.3f * deltaTime);
            }
        }
    }

    // Check spatial memory for remembered danger zones
    if (sensory.getMemory().hasMemoryOf(MemoryType::DANGER_LOCATION)) {
        auto dangerMemories = sensory.getMemory().recall(MemoryType::DANGER_LOCATION);
        for (const auto& mem : dangerMemories) {
            float dist = glm::length(mem.location - position);
            if (dist < genome.visionRange * 0.5f && mem.strength > 0.3f) {
                // We're near a remembered danger zone - slight fear increase
                fear = std::min(1.0f, fear + 0.1f * mem.strength * deltaTime);
            }
        }
    }

    // For herbivores: Use memory to seek food when hungry
    if (type == CreatureType::HERBIVORE && energy < 100.0f) {
        if (sensory.getMemory().hasMemoryOf(MemoryType::FOOD_LOCATION)) {
            // We remember where food was - this will influence pathfinding
            // (The actual movement is handled by the behavior functions)
        }
    }
}

void Creature::emitAlarmCall(std::vector<SoundEvent>& soundBuffer) {
    if (genome.alarmCallVolume < 0.1f) return;

    SoundEvent alarm;
    alarm.position = position;
    alarm.type = SoundType::ALARM_CALL;
    alarm.intensity = genome.alarmCallVolume;
    alarm.frequency = 2000.0f;
    alarm.timestamp = currentTime;  // Use creature's current time
    alarm.source = this;

    soundBuffer.push_back(alarm);
}

void Creature::emitMatingCall(std::vector<SoundEvent>& soundBuffer) {
    if (genome.displayIntensity < 0.1f) return;

    SoundEvent call;
    call.position = position;
    call.type = SoundType::MATING_CALL;
    call.intensity = genome.displayIntensity;
    call.frequency = 500.0f;
    call.timestamp = currentTime;  // Use creature's current time
    call.source = this;

    soundBuffer.push_back(call);
}

void Creature::updateBehaviorFlying(float deltaTime, const Terrain& terrain,
                                    const std::vector<glm::vec3>& foodPositions,
                                    const std::vector<Creature*>& otherCreatures,
                                    const SpatialGrid* grid) {
    glm::vec3 steeringForce(0.0f);

    // =========================================================================
    // NEURAL NETWORK MODULATION FOR FLYING CREATURES
    // =========================================================================
    float aggressionModifier = 1.0f + m_neuralOutputs.aggressionMod * 0.5f;  // 0.5 to 1.5
    float socialModifier = 1.0f + m_neuralOutputs.socialMod * 0.5f;          // 0.5 to 1.5
    float exploreModifier = 1.0f + m_neuralOutputs.explorationMod * 0.5f;    // 0.5 to 1.5

    // Get terrain height at current position
    float terrainHeight = terrain.getHeight(position.x, position.z);
    float targetAltitude = terrainHeight + genome.preferredAltitude;

    // === ALTITUDE MAINTENANCE ===
    float altitudeError = targetAltitude - position.y;
    // Smooth altitude adjustment with damping
    float altitudeForce = altitudeError * 2.0f;
    steeringForce.y += altitudeForce;

    // === HUNTING BEHAVIOR (can hunt small herbivores) - NEURAL MODULATED ===
    Creature* nearestPrey = nullptr;
    // Neural aggression affects hunting range
    float nearestPreyDist = genome.visionRange * aggressionModifier;

    for (Creature* other : otherCreatures) {
        if (other == this || !other->isAlive()) continue;
        // Flying creatures can hunt small frugivores
        if (other->getType() != CreatureType::FRUGIVORE) continue;
        if (other->getGenome().size > 0.8f) continue;  // Only small prey

        float dist = glm::length(other->getPosition() - position);
        if (dist < nearestPreyDist) {
            nearestPreyDist = dist;
            nearestPrey = other;
        }
    }

    // === FOOD SEEKING (omnivore - can eat plants too) ===
    glm::vec3 nearestFoodPos;
    float nearestFoodDist = genome.visionRange;
    bool foodFound = false;

    for (const auto& foodPos : foodPositions) {
        float dist = glm::length(foodPos - position);
        if (dist < nearestFoodDist) {
            nearestFoodDist = dist;
            nearestFoodPos = foodPos;
            foodFound = true;
        }
    }

    // Prioritize hunting when hungry, otherwise prefer plant food (less energy cost)
    bool shouldHunt = (energy < 80.0f && nearestPrey != nullptr) ||
                      (nearestPrey != nullptr && nearestPreyDist < 15.0f && energy < 120.0f);

    if (shouldHunt && nearestPrey != nullptr) {
        // HUNTING BEHAVIOR: Circle above and dive to attack
        float preyDist = glm::length(nearestPrey->getPosition() - position);
        glm::vec3 toPrey = nearestPrey->getPosition() - position;

        nearestPrey->setBeingHunted(true);

        if (preyDist < 5.0f && position.y > nearestPrey->getPosition().y - 2.0f) {
            // DIVE TO ATTACK
            glm::vec3 diveTarget = nearestPrey->getPosition();
            steeringForce += glm::normalize(diveTarget - position) * 3.0f;
            steeringForce.y -= 15.0f;  // Strong downward force for diving

            // Attack if close enough
            if (preyDist < attackRange && huntingCooldown <= 0.0f) {
                attack(nearestPrey, deltaTime);
            }
        } else if (preyDist < 20.0f) {
            // CIRCLE ABOVE PREY (predatory circling behavior)
            glm::vec3 preyPos = nearestPrey->getPosition();
            float circleRadius = 12.0f;

            // Calculate current angle around prey
            float currentAngle = atan2(position.z - preyPos.z, position.x - preyPos.x);
            float rotationSpeed = 0.8f;  // Radians per second
            currentAngle += rotationSpeed * deltaTime;

            // Target point on circle
            glm::vec3 circleTarget;
            circleTarget.x = preyPos.x + cos(currentAngle) * circleRadius;
            circleTarget.z = preyPos.z + sin(currentAngle) * circleRadius;
            circleTarget.y = preyPos.y + genome.preferredAltitude * 0.5f;  // Circle above prey

            glm::vec3 seekForce = steering.seek(position, velocity, circleTarget);
            steeringForce += seekForce * 1.5f;
        } else {
            // Approach prey from a distance
            glm::vec3 pursuitForce = steering.pursuit(position, velocity,
                nearestPrey->getPosition(), nearestPrey->getVelocity());
            steeringForce += pursuitForce * 1.2f;
        }
    } else if (foodFound && energy < 150.0f) {
        // FORAGING BEHAVIOR: Circle around food source
        float foodDist = glm::length(nearestFoodPos - position);

        if (foodDist < 3.0f && position.y < nearestFoodPos.y + 5.0f) {
            // Descend to eat
            glm::vec3 arriveForce = steering.arrive(position, velocity, nearestFoodPos);
            steeringForce += arriveForce * 2.0f;
            steeringForce.y -= 5.0f;  // Descend toward food
        } else if (foodDist < 15.0f) {
            // Circle above food before landing
            float circleAngle = atan2(position.z - nearestFoodPos.z, position.x - nearestFoodPos.x);
            circleAngle += 0.5f * deltaTime;

            glm::vec3 circlePoint;
            circlePoint.x = nearestFoodPos.x + cos(circleAngle) * 10.0f;
            circlePoint.z = nearestFoodPos.z + sin(circleAngle) * 10.0f;
            circlePoint.y = position.y;

            glm::vec3 seekForce = steering.seek(position, velocity, circlePoint);
            steeringForce += seekForce * 1.0f;
        } else {
            // Fly toward food area
            glm::vec3 arriveForce = steering.arrive(position, velocity, nearestFoodPos);
            steeringForce += arriveForce * 0.8f;
        }
    } else {
        // SOARING/GLIDING BEHAVIOR (no immediate goal) - NEURAL MODULATED
        // Use glide ratio to determine how much active flight vs passive soaring
        float soarInfluence = genome.glideRatio;

        // Wander in the air - neural exploration modifier affects this
        glm::vec3 wanderForce = steering.wander(position, velocity, wanderTarget);
        steeringForce += wanderForce * (1.0f - soarInfluence * 0.5f) * exploreModifier;

        // Occasional lazy circles (soaring)
        float soarPhase = currentTime * 0.3f;
        steeringForce.x += sin(soarPhase) * soarInfluence * 3.0f;
        steeringForce.z += cos(soarPhase) * soarInfluence * 3.0f;
    }

    // === GROUND AVOIDANCE (critical!) ===
    if (position.y < terrainHeight + 10.0f) {
        // Too close to ground - pull up!
        float emergencyFactor = 1.0f - (position.y - terrainHeight) / 10.0f;
        emergencyFactor = std::clamp(emergencyFactor, 0.0f, 1.0f);
        steeringForce.y += 20.0f * emergencyFactor;
    }

    // === WATER AVOIDANCE ===
    if (terrain.isWater(position.x, position.z) && position.y < 15.0f) {
        // Over water and too low - definitely pull up
        steeringForce.y += 30.0f;
    }

    // === FLOCKING WITH OTHER FLYING CREATURES (NEURAL MODULATED) ===
    std::vector<Creature*> flyingNeighbors = getNeighborsOfType(otherCreatures, CreatureType::FLYING,
                                                                genome.visionRange * 0.5f * socialModifier, grid);
    if (!flyingNeighbors.empty()) {
        // Light flocking (birds don't flock as tightly as fish) - neural social modifier
        glm::vec3 flockForce = steering.flock(position, velocity, flyingNeighbors,
            2.0f * socialModifier,   // Separation weight
            0.5f * socialModifier,   // Alignment weight
            0.3f * socialModifier    // Cohesion weight
        );
        steeringForce += flockForce * 0.4f * socialModifier;
    }

    // === BOUNDARY AVOIDANCE ===
    float halfWidth = 150.0f;
    float halfDepth = 150.0f;
    float boundaryMargin = 30.0f;

    if (position.x > halfWidth - boundaryMargin) {
        steeringForce.x -= (position.x - (halfWidth - boundaryMargin)) * 0.5f;
    } else if (position.x < -halfWidth + boundaryMargin) {
        steeringForce.x -= (position.x + (halfWidth - boundaryMargin)) * 0.5f;
    }

    if (position.z > halfDepth - boundaryMargin) {
        steeringForce.z -= (position.z - (halfDepth - boundaryMargin)) * 0.5f;
    } else if (position.z < -halfDepth + boundaryMargin) {
        steeringForce.z -= (position.z + (halfDepth - boundaryMargin)) * 0.5f;
    }

    // Apply steering force to velocity
    velocity = steering.applyForce(velocity, steeringForce, deltaTime);

    // Flying creatures are faster
    float maxSpeed = genome.speed * 1.3f;
    float speed = glm::length(velocity);
    if (speed > maxSpeed) {
        velocity = glm::normalize(velocity) * maxSpeed;
    }

    // Minimum forward speed (birds need air flow for lift)
    float minSpeed = genome.speed * 0.3f;
    if (speed < minSpeed) {
        glm::vec3 forward(cos(rotation), 0.0f, sin(rotation));
        velocity += forward * (minSpeed - speed);
    }

    // Update rotation to face movement direction (bank into turns)
    if (glm::length(glm::vec2(velocity.x, velocity.z)) > 0.5f) {
        float targetRotation = atan2(velocity.z, velocity.x);
        // Smooth rotation (birds bank into turns)
        float rotationDiff = targetRotation - rotation;
        // Normalize angle difference
        while (rotationDiff > 3.14159f) rotationDiff -= 6.28318f;
        while (rotationDiff < -3.14159f) rotationDiff += 6.28318f;
        rotation += rotationDiff * deltaTime * 3.0f;
    }
}

void Creature::updateFlyingPhysics(float deltaTime, const Terrain& terrain) {
    glm::vec3 oldPos = position;

    // Update position
    position += velocity * deltaTime;

    // Track distance traveled (3D distance for flying)
    distanceTraveled += glm::length(position - oldPos);

    // Apply velocity damping (air resistance)
    velocity.y *= 0.98f;  // Vertical damping

    // Enforce altitude constraints
    float terrainHeight = terrain.getHeight(position.x, position.z);

    // Minimum altitude above terrain
    float minAltitude = terrainHeight + 8.0f;
    if (position.y < minAltitude) {
        position.y = minAltitude;
        velocity.y = std::max(velocity.y, 2.0f);  // Bounce up
    }

    // Maximum altitude (don't fly too high)
    float maxAltitude = terrainHeight + genome.preferredAltitude * 2.0f;
    if (position.y > maxAltitude) {
        position.y = maxAltitude;
        velocity.y = std::min(velocity.y, 0.0f);  // Stop climbing
    }

    // Boundary constraints
    float halfWidth = 150.0f;
    float halfDepth = 150.0f;

    position.x = std::clamp(position.x, -halfWidth + 5.0f, halfWidth - 5.0f);
    position.z = std::clamp(position.z, -halfDepth + 5.0f, halfDepth - 5.0f);

    // If clamped to boundary, reflect velocity
    if (std::abs(position.x) >= halfWidth - 5.0f) {
        velocity.x *= -0.5f;
    }
    if (std::abs(position.z) >= halfDepth - 5.0f) {
        velocity.z *= -0.5f;
    }
}

// =============================================================================
// ANIMATION SYSTEM INTEGRATION
// =============================================================================

void Creature::initializeAnimation() {
    // Initialize animation based on creature type
    switch (type) {
        case CreatureType::HERBIVORE:
        case CreatureType::CARNIVORE:
            // Most land creatures are quadrupeds
            m_animator.initializeQuadruped(genome.size, genome.size * 0.5f);
            break;

        case CreatureType::FLYING:
        case CreatureType::FLYING_BIRD:
        case CreatureType::FLYING_INSECT:
        case CreatureType::AERIAL_PREDATOR:
            m_animator.initializeFlying(genome.wingSpan);
            break;

        case CreatureType::AQUATIC:
        case CreatureType::AQUATIC_HERBIVORE:
        case CreatureType::AQUATIC_PREDATOR:
        case CreatureType::AQUATIC_APEX:
        case CreatureType::AMPHIBIAN:
            m_animator.initializeAquatic(genome.size);
            break;

        default:
            m_animator.initializeQuadruped(genome.size, genome.size * 0.5f);
            break;
    }

    // Set up ground raycast callback for foot IK
    if (m_terrainPtr) {
        m_animator.getLocomotion().setGroundCallback(
            [this](const glm::vec3& origin, const glm::vec3& dir,
                   float maxDist, glm::vec3& hit, glm::vec3& normal) -> bool {
                if (!m_terrainPtr) return false;

                // Simple ground plane intersection at terrain height
                float terrainY = m_terrainPtr->getHeight(origin.x, origin.z);
                float t = (terrainY - origin.y) / dir.y;

                if (t > 0.0f && t < maxDist) {
                    hit = origin + dir * t;
                    normal = m_terrainPtr->getNormal(hit.x, hit.z);
                    return true;
                }
                return false;
            }
        );
    }
}

void Creature::updateAnimation(float deltaTime) {
    if (!m_animationEnabled) return;

    // If animator not initialized, initialize it now
    if (m_animator.getBoneCount() == 0) {
        initializeAnimation();
    }

    // Sync animation with creature movement state
    m_animator.setPosition(position);
    m_animator.setVelocity(velocity);

    // Convert 2D rotation to quaternion
    glm::quat bodyRotation = glm::quat(glm::vec3(0.0f, rotation, 0.0f));
    m_animator.setRotation(bodyRotation);

    // Set angular velocity (approximated from rotation change)
    m_animator.setAngularVelocity(0.0f);  // Could track rotation delta if needed

    // Update gait based on speed
    auto& loco = m_animator.getLocomotion();
    float speed = glm::length(velocity);
    float normalizedSpeed = speed / genome.speed;

    if (isFlying(type)) {
        loco.setGaitType(animation::GaitType::Fly);
    } else if (isAquatic(type)) {
        loco.setGaitType(animation::GaitType::Swim);
    } else {
        // Land creatures: walk, trot, or gallop based on speed
        if (normalizedSpeed < 0.3f) {
            loco.setGaitType(animation::GaitType::Walk);
        } else if (normalizedSpeed < 0.7f) {
            loco.setGaitType(animation::GaitType::Trot);
        } else {
            loco.setGaitType(animation::GaitType::Gallop);
        }
    }

    // Update animation
    m_animator.update(deltaTime);
}

float Creature::getAnimationPhase() const {
    if (!m_animationEnabled || m_animator.getBoneCount() == 0) {
        return 0.0f;
    }
    return m_animator.getLocomotion().getGaitPhase();
}

// =============================================================================
// ACTIVITY SYSTEM - Phase 8, Agent 10
// Handles creature activities: eating, mating, sleeping, grooming, etc.
// =============================================================================

void Creature::updateActivitySystem(float deltaTime,
                                    const std::vector<glm::vec3>& foodPositions,
                                    const std::vector<Creature*>& otherCreatures) {
    // Update activity triggers from creature state
    m_activityTriggers.hungerLevel = getHungerLevel();
    m_activityTriggers.energyLevel = energy / maxEnergy;
    m_activityTriggers.fatigueLevel = m_fatigueLevel;
    m_activityTriggers.bladderFullness = m_bladderFullness;
    m_activityTriggers.bowelFullness = m_bowelFullness;
    m_activityTriggers.dirtyLevel = m_dirtyLevel;
    m_activityTriggers.reproductionUrge = getReproductionUrge();
    m_activityTriggers.threatLevel = fear;
    m_activityTriggers.isJuvenile = (age < 10.0f);
    m_activityTriggers.playUrge = m_activityTriggers.isJuvenile ?
        (1.0f - m_fatigueLevel) * (1.0f - getHungerLevel()) : 0.1f;

    // Check for nearby food
    m_hasNearbyFood = false;
    float nearestFoodDist = genome.visionRange;
    for (const auto& foodPos : foodPositions) {
        float dist = glm::length(foodPos - position);
        if (dist < nearestFoodDist) {
            nearestFoodDist = dist;
            m_nearestFoodPos = foodPos;
            m_hasNearbyFood = true;
        }
    }
    m_activityTriggers.foodNearby = m_hasNearbyFood;
    m_activityTriggers.foodDistance = nearestFoodDist;

    // Check for nearby mates
    m_hasNearbyMate = false;
    float nearestMateDist = genome.visionRange;
    for (const auto* other : otherCreatures) {
        if (other == this || !other->isAlive()) continue;
        if (other->getType() != type) continue;  // Same species only

        float dist = glm::length(other->getPosition() - position);
        if (dist < nearestMateDist && canMateWith(*other)) {
            nearestMateDist = dist;
            m_nearestMatePos = other->getPosition();
            m_hasNearbyMate = true;
        }
    }
    m_activityTriggers.potentialMateNearby = m_hasNearbyMate;
    m_activityTriggers.mateDistance = nearestMateDist;

    // Check for territory intruders (same type, different species)
    m_activityTriggers.territoryIntruder = false;
    for (const auto* other : otherCreatures) {
        if (other == this || !other->isAlive()) continue;
        if (other->getType() == type && other->getSpeciesId() != getSpeciesId()) {
            float dist = glm::length(other->getPosition() - position);
            if (dist < genome.visionRange * 0.5f) {
                m_activityTriggers.territoryIntruder = true;
                break;
            }
        }
    }

    // Check for nearby offspring (parental care - Agent 10)
    // Simplified: young creatures of same species within close range are considered offspring
    m_hasOffspringNearby = false;
    m_offspringHungerLevel = 0.0f;
    float parentalRange = genome.visionRange * 0.4f;  // Close range for parental care
    for (const auto* other : otherCreatures) {
        if (other == this || !other->isAlive()) continue;
        if (other->getSpeciesId() != getSpeciesId()) continue;

        // Check if creature is young (juvenile) and within range
        if (other->getAge() < 10.0f) {  // Juvenile threshold
            float dist = glm::length(other->getPosition() - position);
            if (dist < parentalRange) {
                m_hasOffspringNearby = true;
                // Track hunger of nearby offspring
                float offspringHunger = other->getHungerLevel();
                if (offspringHunger > m_offspringHungerLevel) {
                    m_offspringHungerLevel = offspringHunger;
                }
            }
        }
    }

    // Parental urge increases after recent reproduction, decays over time
    // For simplicity, adults with juveniles nearby have parental urge
    if (m_hasOffspringNearby && age > 15.0f) {  // Must be adult
        m_parentalUrge = std::min(1.0f, m_parentalUrge + deltaTime * 0.1f);
    } else {
        m_parentalUrge = std::max(0.0f, m_parentalUrge - deltaTime * 0.02f);
    }

    m_activityTriggers.hasOffspringNearby = m_hasOffspringNearby;
    m_activityTriggers.offspringHungerLevel = m_offspringHungerLevel;
    m_activityTriggers.parentalUrge = m_parentalUrge;

    // Update activity state machine
    m_activitySystem.setTriggers(m_activityTriggers);
    m_activitySystem.update(deltaTime);

    // Update activity animation driver
    m_activityAnimDriver.setStateMachine(&m_activitySystem);
    m_activityAnimDriver.setFoodPosition(m_nearestFoodPos);
    m_activityAnimDriver.setMatePosition(m_nearestMatePos);
    m_activityAnimDriver.setGroundPosition(position);
    m_activityAnimDriver.setBodySize(genome.size);
    m_activityAnimDriver.setHasTail(genome.hasBioluminescence || true); // Simplified
    m_activityAnimDriver.update(deltaTime);

    // Update secondary motion layer
    m_secondaryMotion.setActivityState(m_activitySystem.getCurrentActivity());
    m_secondaryMotion.setMovementSpeed(glm::length(velocity) / genome.speed);
    m_secondaryMotion.setArousalLevel(fear + m_activityTriggers.reproductionUrge * 0.5f);
    m_secondaryMotion.update(deltaTime);
}

void Creature::updatePhysiologicalState(float deltaTime) {
    // Fatigue increases over time, decreases when sleeping
    if (m_activitySystem.getCurrentActivity() == animation::ActivityType::SLEEPING) {
        m_fatigueLevel = std::max(0.0f, m_fatigueLevel - deltaTime * 0.1f);
    } else {
        // Fatigue increases based on activity level
        float activityLevel = glm::length(velocity) / genome.speed;
        m_fatigueLevel = std::min(1.0f, m_fatigueLevel + deltaTime * 0.005f * (1.0f + activityLevel));
    }

    // Bladder/bowel fullness increases after eating
    if (m_activitySystem.getCurrentActivity() == animation::ActivityType::EATING) {
        m_lastMealTime = 0.0f;
    }
    m_lastMealTime += deltaTime;

    // Digestion: bladder/bowel fill up after eating
    if (m_lastMealTime > 5.0f && m_lastMealTime < 60.0f) {
        float digestionRate = 0.01f * deltaTime;
        m_bladderFullness = std::min(1.0f, m_bladderFullness + digestionRate);
        m_bowelFullness = std::min(1.0f, m_bowelFullness + digestionRate * 0.5f);
    }

    // Excreting empties bladder/bowel
    if (m_activitySystem.getCurrentActivity() == animation::ActivityType::EXCRETING) {
        if (m_activitySystem.getExcretionType() == animation::ExcretionType::URINATE) {
            m_bladderFullness = std::max(0.0f, m_bladderFullness - deltaTime * 0.5f);
        } else {
            m_bowelFullness = std::max(0.0f, m_bowelFullness - deltaTime * 0.3f);
        }
    }

    // Dirtiness increases over time, decreases when grooming
    if (m_activitySystem.getCurrentActivity() == animation::ActivityType::GROOMING) {
        m_dirtyLevel = std::max(0.0f, m_dirtyLevel - deltaTime * 0.2f);
    } else {
        m_dirtyLevel = std::min(1.0f, m_dirtyLevel + deltaTime * 0.002f);
    }
}

float Creature::getReproductionUrge() const {
    if (!canReproduce()) return 0.0f;

    // Reproduction urge based on energy, age, and time
    float energyFactor = (energy - 100.0f) / 100.0f;
    float ageFactor = std::min(1.0f, age / 20.0f);  // Mature after age 20

    return std::max(0.0f, energyFactor * ageFactor);
}

// =============================================================================
// NEAT BRAIN INTEGRATION - Evolved topology neural networks
// =============================================================================

void Creature::initializeNEATBrain() {
    m_neatBrain = std::make_unique<ai::CreatureBrainInterface>();
    // 27 inputs (expanded sensory) and 10 outputs (expanded motor control)
    // This creates a neural network that actually DRIVES creature behavior
    m_neatBrain->initialize(ai::CreatureBrainInterface::BrainType::NEAT_EVOLVED,
                           ai::SensoryInput::size(),   // 27 inputs
                           ai::MotorOutput::size());   // 10 outputs
    m_useNEATBrain = true;
}

void Creature::initializeNEATBrain(const ai::NEATGenome& genome) {
    m_neatBrain = std::make_unique<ai::CreatureBrainInterface>();
    m_neatBrain->initializeFromGenome(genome);
    m_useNEATBrain = true;
}

const ai::NEATGenome& Creature::getNEATGenome() const {
    static ai::NEATGenome emptyGenome;
    if (m_neatBrain) {
        return m_neatBrain->getGenome();
    }
    return emptyGenome;
}

void Creature::setNEATGenome(const ai::NEATGenome& genome) {
    if (!m_neatBrain) {
        m_neatBrain = std::make_unique<ai::CreatureBrainInterface>();
    }
    m_neatBrain->setGenome(genome);
    m_useNEATBrain = true;
}

// =============================================================================
// CLIMATE RESPONSE SYSTEM - Creatures adapt to changing climate
// =============================================================================

#include "../environment/ClimateSystem.h"

void Creature::initializeOptimalTemperature() {
    // Derive optimal temperature from creature type and genome
    // Base temperature preferences by creature type
    switch (type) {
        case CreatureType::HERBIVORE:
            m_optimalTemp = 0.5f;  // Temperate preference
            break;
        case CreatureType::CARNIVORE:
            m_optimalTemp = 0.45f;  // Slightly cooler (thick fur)
            break;
        case CreatureType::AQUATIC:
            m_optimalTemp = 0.4f;  // Cool water preference
            break;
        case CreatureType::FLYING:
            m_optimalTemp = 0.55f;  // Slightly warmer (high metabolism)
            break;
        default:
            m_optimalTemp = 0.5f;
    }

    // Modify based on genome traits (size affects temperature tolerance)
    // Larger creatures handle cold better, smaller handle heat better
    float sizeEffect = (genome.size - 1.0f) * 0.1f;  // Size 1.0 is average
    m_optimalTemp -= sizeEffect;  // Larger = lower optimal temp

    // Clamp to valid range
    m_optimalTemp = std::clamp(m_optimalTemp, 0.1f, 0.9f);
}

float Creature::getOptimalTemperature() const {
    return m_optimalTemp;
}

float Creature::calculateClimateSuitability(const ClimateData& climate) const {
    // Calculate how suitable a climate is for this creature
    // Returns 0-1 where 1 is perfect and 0 is uninhabitable

    float tempDiff = std::abs(climate.temperature - m_optimalTemp);

    // Temperature suitability (bell curve centered on optimal)
    float tempSuit = std::exp(-tempDiff * tempDiff * 20.0f);

    // Moisture suitability (most creatures like moderate moisture)
    float moistureDiff = std::abs(climate.moisture - 0.5f);
    float moistSuit = 1.0f - moistureDiff * 0.5f;

    // Aquatic creatures need high moisture
    if (type == CreatureType::AQUATIC) {
        moistSuit = climate.moisture;  // Directly proportional to moisture
    }

    // Combine factors
    return tempSuit * 0.7f + moistSuit * 0.3f;
}

void Creature::updateClimateResponse(const ClimateData& climate, const ClimateSystem* climateSystem, float deltaTime) {
    // Initialize optimal temperature on first call
    static bool initialized = false;
    if (!initialized) {
        initializeOptimalTemperature();
        initialized = true;
    }

    float currentTemp = climate.temperature;
    float tempDiff = std::abs(currentTemp - m_optimalTemp);

    // Reset behavioral flags
    m_seekingCooling = false;
    m_seekingWarmth = false;

    // Calculate climate stress based on temperature difference
    // Stress starts at 0 when in comfort zone, increases with distance from optimal
    float comfortZone = 0.1f;  // 10% normalized temperature range is comfortable
    if (tempDiff > comfortZone) {
        m_climateStress = std::clamp((tempDiff - comfortZone) / 0.3f, 0.0f, 1.0f);
    } else {
        m_climateStress = 0.0f;
    }

    // Apply energy drain from climate stress
    if (m_climateStress > 0.0f) {
        // Severe stress drains energy (0.5-2 energy per second at max stress)
        float stressDrain = m_climateStress * 2.0f * deltaTime;
        energy -= stressDrain;
    }

    // Behavioral adaptations based on temperature
    if (currentTemp > m_optimalTemp + 0.1f) {
        // Too hot - seek cooling (water, shade)
        m_seekingCooling = true;
    } else if (currentTemp < m_optimalTemp - 0.1f) {
        // Too cold - seek warmth (huddle with others)
        m_seekingWarmth = true;

        // Reduce social distance when cold (huddling behavior)
        // This is applied in steering behaviors
    }

    // Update migration cooldown
    if (m_migrationCooldown > 0.0f) {
        m_migrationCooldown -= deltaTime;
    }

    // Consider migration if stressed and cooldown elapsed
    if (m_climateStress > 0.3f && m_migrationCooldown <= 0.0f && climateSystem) {
        considerMigration(climate, climateSystem);
    }

    // If migrating, apply migration direction to movement
    if (m_isMigrating) {
        // Migration will be incorporated into steering in updatePhysics
        // Clear migration after some time
        static float migrationTimer = 0.0f;
        migrationTimer += deltaTime;
        if (migrationTimer > 30.0f) {  // Migrate for 30 seconds then reassess
            m_isMigrating = false;
            m_migrationCooldown = 60.0f;  // Wait before considering migration again
            migrationTimer = 0.0f;
        }
    }
}

void Creature::considerMigration(const ClimateData& currentClimate, const ClimateSystem* climateSystem) {
    if (!climateSystem) return;

    // Look for better climate in sensing range
    glm::vec3 bestDirection = glm::vec3(0.0f);
    float currentSuitability = calculateClimateSuitability(currentClimate);
    float bestSuitability = currentSuitability;

    // Sample in 8 directions
    float senseRange = genome.visionRange * 2.0f;  // Look further for migration
    for (int angle = 0; angle < 360; angle += 45) {
        float rad = glm::radians(static_cast<float>(angle));
        glm::vec3 dir = glm::vec3(std::cos(rad), 0.0f, std::sin(rad));
        glm::vec3 samplePos = position + dir * senseRange;

        ClimateData sampleClimate = climateSystem->getClimateAt(samplePos);
        float suitability = calculateClimateSuitability(sampleClimate);

        if (suitability > bestSuitability) {
            bestSuitability = suitability;
            bestDirection = dir;
        }
    }

    // Only migrate if significantly better climate found
    float improvementThreshold = 0.15f;
    if (bestSuitability > currentSuitability + improvementThreshold) {
        m_migrationDirection = glm::normalize(bestDirection);
        m_isMigrating = true;
        m_migrationCooldown = 60.0f;  // Reset cooldown
    } else {
        // No good migration target found
        m_isMigrating = false;
        m_migrationCooldown = 30.0f;  // Shorter cooldown to try again
    }
}
