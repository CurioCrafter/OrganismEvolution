#include "CreatureBrainInterface.h"
#include <algorithm>
#include <cmath>

namespace ai {

// ============================================================================
// CreatureBrainInterface Implementation
// ============================================================================

CreatureBrainInterface::CreatureBrainInterface()
    : m_brainType(BrainType::LEGACY_STEERING)
    , m_brain(nullptr)
{
}

void CreatureBrainInterface::initialize(BrainType type, int inputSize, int outputSize) {
    m_brainType = type;

    if (type == BrainType::LEGACY_STEERING) {
        // No brain needed - will use steering behaviors
        m_brain = nullptr;
        return;
    }

    std::mt19937 rng(std::random_device{}());

    if (type == BrainType::MODULAR_BRAIN) {
        m_brain = std::make_unique<CreatureBrain>();
        m_brain->initialize(rng);
    } else if (type == BrainType::NEAT_EVOLVED) {
        // Create minimal NEAT genome
        m_genome.createMinimal(inputSize, outputSize, rng);
        m_brain = std::make_unique<CreatureBrain>();
        m_brain->initializeFromGenome(m_genome);
    }
}

void CreatureBrainInterface::initializeFromGenome(const NEATGenome& genome) {
    m_brainType = BrainType::NEAT_EVOLVED;
    m_genome = genome;
    m_brain = std::make_unique<CreatureBrain>();
    m_brain->initializeFromGenome(genome);
}

SensoryInput CreatureBrainInterface::buildSensoryInput(
    float nearestFoodDist, float nearestFoodAngle,
    float nearestPredatorDist, float nearestPredatorAngle,
    float nearestPreyDist, float nearestPreyAngle,
    float nearestAllyDist, float nearestAllyAngle,
    float energy, float health, float age,
    float terrainHeight, float waterProximity,
    bool wasRecentlyAttacked, bool recentlyAteFood)
{
    SensoryInput input;

    input.nearestFoodDistance = nearestFoodDist;
    input.nearestFoodAngle = nearestFoodAngle;
    input.nearestPredatorDistance = nearestPredatorDist;
    input.nearestPredatorAngle = nearestPredatorAngle;
    input.nearestPreyDistance = nearestPreyDist;
    input.nearestPreyAngle = nearestPreyAngle;
    input.nearestAllyDistance = nearestAllyDist;
    input.nearestAllyAngle = nearestAllyAngle;
    input.energy = energy;
    input.health = health;
    input.age = age;
    input.terrainHeight = terrainHeight;
    input.waterProximity = waterProximity;
    input.wasAttacked = wasRecentlyAttacked ? 1.0f : 0.0f;
    input.recentFoodEaten = recentlyAteFood ? 1.0f : 0.0f;

    return input;
}

MotorOutput CreatureBrainInterface::process(
    float nearestFoodDist, float nearestFoodAngle,
    float nearestPredatorDist, float nearestPredatorAngle,
    float nearestPreyDist, float nearestPreyAngle,
    float nearestAllyDist, float nearestAllyAngle,
    float energy, float health, float age,
    float terrainHeight, float waterProximity,
    bool wasRecentlyAttacked, bool recentlyAteFood,
    float deltaTime)
{
    // Build sensory input
    m_lastInput = buildSensoryInput(
        nearestFoodDist, nearestFoodAngle,
        nearestPredatorDist, nearestPredatorAngle,
        nearestPreyDist, nearestPreyAngle,
        nearestAllyDist, nearestAllyAngle,
        energy, health, age,
        terrainHeight, waterProximity,
        wasRecentlyAttacked, recentlyAteFood
    );

    if (m_brainType == BrainType::LEGACY_STEERING || !m_brain) {
        // Return default output for legacy mode
        // Actual behavior handled by existing steering code
        m_lastOutput = MotorOutput();
        return m_lastOutput;
    }

    // Process through brain
    m_lastOutput = m_brain->process(m_lastInput, deltaTime);

    // Update statistics
    m_stats.averageActivity = 0.9f * m_stats.averageActivity +
                              0.1f * m_brain->getAverageActivity();

    if (m_brain->getCurrentDrives().fear > 0.5f) {
        m_stats.fearTime += deltaTime;
    }

    return m_lastOutput;
}

MotorOutput CreatureBrainInterface::processExpanded(const SensoryInput& input, float deltaTime) {
    m_lastInput = input;

    if (m_brainType == BrainType::LEGACY_STEERING || !m_brain) {
        // Return default output for legacy mode
        m_lastOutput = MotorOutput();
        return m_lastOutput;
    }

    // Process through brain with expanded inputs (27 -> 10)
    m_lastOutput = m_brain->process(m_lastInput, deltaTime);

    // Update statistics
    m_stats.averageActivity = 0.9f * m_stats.averageActivity +
                              0.1f * m_brain->getAverageActivity();

    if (m_brain->getCurrentDrives().fear > 0.5f) {
        m_stats.fearTime += deltaTime;
    }

    return m_lastOutput;
}

CreatureBrainInterface::MovementCommand CreatureBrainInterface::getMovementCommand() const {
    MovementCommand cmd;
    cmd.turnAngle = m_lastOutput.turnAngle;
    cmd.speed = m_lastOutput.speed;
    cmd.attackIntent = m_lastOutput.attackIntent;
    cmd.fleeIntent = m_lastOutput.fleeIntent;
    return cmd;
}

void CreatureBrainInterface::onFoodEaten(float amount) {
    if (m_brain) {
        m_brain->onFoodEaten(amount);
    }
    m_stats.totalReward += amount;
}

void CreatureBrainInterface::onDamageTaken(float amount) {
    if (m_brain) {
        m_brain->onDamageTaken(amount);
    }
    m_stats.totalReward -= amount * 0.5f;
}

void CreatureBrainInterface::onThreatDetected(float level) {
    if (m_brain) {
        m_brain->onThreatDetected(level);
    }
}

void CreatureBrainInterface::onSuccessfulHunt() {
    if (m_brain) {
        m_brain->onSuccessfulHunt();
    }
    m_stats.totalReward += 2.0f;
}

void CreatureBrainInterface::onSuccessfulEscape() {
    // Small reward for escaping predators
    if (m_brain) {
        m_brain->learn(0.5f);
    }
    m_stats.totalReward += 0.5f;
}

void CreatureBrainInterface::learn(float reward) {
    if (m_brain) {
        m_brain->learn(reward);
    }
    m_stats.learningEvents++;
    m_stats.totalReward += reward;
}

void CreatureBrainInterface::reset() {
    if (m_brain) {
        m_brain->reset();
    }
    m_lastOutput = MotorOutput();
    m_stats = Statistics();
}

const NeuromodulatorState& CreatureBrainInterface::getNeuromodulators() const {
    static NeuromodulatorState defaultState;
    if (m_brain) {
        return m_brain->getNeuromodulators();
    }
    return defaultState;
}

const EmotionalModule::Drives& CreatureBrainInterface::getDrives() const {
    static EmotionalModule::Drives defaultDrives;
    if (m_brain) {
        return m_brain->getCurrentDrives();
    }
    return defaultDrives;
}

float CreatureBrainInterface::getBrainActivity() const {
    if (m_brain) {
        return m_brain->getAverageActivity();
    }
    return 0.0f;
}

float CreatureBrainInterface::getComplexity() const {
    if (m_brain) {
        return m_brain->getComplexity();
    }
    return 0.0f;
}

NEATGenome& CreatureBrainInterface::getGenome() {
    return m_genome;
}

const NEATGenome& CreatureBrainInterface::getGenome() const {
    return m_genome;
}

void CreatureBrainInterface::setGenome(const NEATGenome& genome) {
    m_genome = genome;
    if (m_brain && m_brainType == BrainType::NEAT_EVOLVED) {
        m_brain->setNEATGenome(genome);
    }
}

std::vector<float> CreatureBrainInterface::getWeights() const {
    if (m_brain) {
        return m_brain->getAllWeights();
    }
    return {};
}

void CreatureBrainInterface::setWeights(const std::vector<float>& weights) {
    if (m_brain) {
        m_brain->setAllWeights(weights);
    }
}

size_t CreatureBrainInterface::getWeightCount() const {
    if (m_brain) {
        return m_brain->getTotalWeightCount();
    }
    return 0;
}

// ============================================================================
// BrainEvolutionManager Implementation
// ============================================================================

BrainEvolutionManager::BrainEvolutionManager(int populationSize, int inputSize, int outputSize)
    : m_populationSize(populationSize)
    , m_inputSize(inputSize)
    , m_outputSize(outputSize)
    , m_rng(std::random_device{}())
{
    // Initialize with minimal genomes
    InnovationTracker::instance().reset();

    m_genomes.reserve(populationSize);
    for (int i = 0; i < populationSize; i++) {
        NEATGenome genome;
        genome.createMinimal(inputSize, outputSize, m_rng);
        m_genomes.push_back(std::move(genome));
    }

    m_bestGenome = m_genomes[0];
}

std::unique_ptr<CreatureBrainInterface> BrainEvolutionManager::createBrain() {
    auto brain = std::make_unique<CreatureBrainInterface>();

    if (m_genomes.empty()) {
        brain->initialize(CreatureBrainInterface::BrainType::NEAT_EVOLVED,
                         m_inputSize, m_outputSize);
    } else {
        // Select random genome from population
        std::uniform_int_distribution<size_t> dist(0, m_genomes.size() - 1);
        brain->initializeFromGenome(m_genomes[dist(m_rng)]);
    }

    return brain;
}

std::unique_ptr<CreatureBrainInterface> BrainEvolutionManager::createOffspringBrain(
    const CreatureBrainInterface& parent) {

    auto offspring = std::make_unique<CreatureBrainInterface>();

    // Copy parent genome and mutate
    NEATGenome childGenome = parent.getGenome();
    childGenome.mutate(m_rng, m_mutationParams);

    offspring->initializeFromGenome(childGenome);
    return offspring;
}

std::unique_ptr<CreatureBrainInterface> BrainEvolutionManager::createOffspringBrain(
    const CreatureBrainInterface& parent1,
    const CreatureBrainInterface& parent2) {

    auto offspring = std::make_unique<CreatureBrainInterface>();

    // Crossover based on fitness (assume parent1 is fitter if equal)
    const NEATGenome& fitter = parent1.getGenome();
    const NEATGenome& other = parent2.getGenome();

    NEATGenome childGenome = NEATGenome::crossover(fitter, other, m_rng);
    childGenome.mutate(m_rng, m_mutationParams);

    offspring->initializeFromGenome(childGenome);
    return offspring;
}

void BrainEvolutionManager::reportFitness(const NEATGenome& genome, float fitness) {
    // Find matching genome and update fitness
    for (auto& g : m_genomes) {
        // Simple check - could use genome ID for more robust matching
        if (g.getMaxInnovation() == genome.getMaxInnovation() &&
            g.getHiddenCount() == genome.getHiddenCount()) {
            g.setFitness(fitness);
            if (fitness > m_bestFitness) {
                m_bestFitness = fitness;
                m_bestGenome = g;
            }
            break;
        }
    }
}

void BrainEvolutionManager::speciate() {
    // Clear existing species memberships
    for (auto& species : m_species) {
        species.clear();
    }

    // Assign each genome to a species
    for (auto& genome : m_genomes) {
        bool found = false;

        for (auto& species : m_species) {
            float distance = genome.compatibilityDistance(
                species.representative, 1.0f, 1.0f, 0.4f);

            if (distance < m_compatibilityThreshold) {
                species.members.push_back(&genome);
                genome.setSpeciesId(species.id);
                found = true;
                break;
            }
        }

        if (!found) {
            static int nextSpeciesId = 0;
            Species newSpecies(nextSpeciesId++, genome);
            newSpecies.members.push_back(&genome);
            genome.setSpeciesId(newSpecies.id);
            m_species.push_back(newSpecies);
        }
    }

    // Remove empty species
    m_species.erase(
        std::remove_if(m_species.begin(), m_species.end(),
                       [](const Species& s) { return s.members.empty(); }),
        m_species.end());

    // Update representatives
    for (auto& species : m_species) {
        species.updateRepresentative(m_rng);
    }
}

NEATGenome BrainEvolutionManager::reproduce(Species& species) {
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);

    // Select parents from top performers
    int survivalCount = std::max(1, static_cast<int>(species.members.size() * 0.2f));
    std::uniform_int_distribution<int> parentDist(0, survivalCount - 1);

    NEATGenome* parent1 = species.members[parentDist(m_rng)];

    NEATGenome offspring;

    // 75% chance of crossover
    if (prob(m_rng) < 0.75f && species.members.size() > 1) {
        NEATGenome* parent2 = species.members[parentDist(m_rng)];
        while (parent2 == parent1 && survivalCount > 1) {
            parent2 = species.members[parentDist(m_rng)];
        }

        if (parent1->getFitness() >= parent2->getFitness()) {
            offspring = NEATGenome::crossover(*parent1, *parent2, m_rng);
        } else {
            offspring = NEATGenome::crossover(*parent2, *parent1, m_rng);
        }
    } else {
        offspring = *parent1;
    }

    offspring.mutate(m_rng, m_mutationParams);
    return offspring;
}

void BrainEvolutionManager::evolveGeneration() {
    m_generation++;

    // Speciate current population
    speciate();

    // Calculate adjusted fitness
    for (auto& species : m_species) {
        species.calculateAdjustedFitness();
        species.updateStagnation();
    }

    // Calculate total adjusted fitness
    float totalAdjustedFitness = 0.0f;
    for (const auto& species : m_species) {
        totalAdjustedFitness += species.totalAdjustedFitness;
    }

    if (totalAdjustedFitness <= 0.0f) {
        totalAdjustedFitness = 1.0f;
    }

    // Calculate offspring per species
    std::vector<int> offspringCounts;
    int totalOffspring = 0;

    for (const auto& species : m_species) {
        float proportion = species.totalAdjustedFitness / totalAdjustedFitness;
        int count = static_cast<int>(std::round(proportion * m_populationSize));
        count = std::max(1, count);
        offspringCounts.push_back(count);
        totalOffspring += count;
    }

    // Adjust to match population size
    while (totalOffspring > m_populationSize && !offspringCounts.empty()) {
        std::uniform_int_distribution<size_t> dist(0, offspringCounts.size() - 1);
        size_t idx = dist(m_rng);
        if (offspringCounts[idx] > 1) {
            offspringCounts[idx]--;
            totalOffspring--;
        }
    }
    while (totalOffspring < m_populationSize && !offspringCounts.empty()) {
        std::uniform_int_distribution<size_t> dist(0, offspringCounts.size() - 1);
        offspringCounts[dist(m_rng)]++;
        totalOffspring++;
    }

    // Generate new population
    std::vector<NEATGenome> newGenomes;
    newGenomes.reserve(m_populationSize);

    for (size_t speciesIdx = 0; speciesIdx < m_species.size(); speciesIdx++) {
        Species& species = m_species[speciesIdx];
        int count = offspringCounts[speciesIdx];

        if (species.members.empty()) continue;

        // Sort by fitness
        std::sort(species.members.begin(), species.members.end(),
                  [](const NEATGenome* a, const NEATGenome* b) {
                      return a->getFitness() > b->getFitness();
                  });

        // Keep champion unchanged
        if (count > 0 && species.members.size() >= 5) {
            newGenomes.push_back(*species.members[0]);
            count--;
        }

        // Produce offspring
        for (int i = 0; i < count; i++) {
            newGenomes.push_back(reproduce(species));
        }
    }

    m_genomes = std::move(newGenomes);

    // Clear species for next generation
    for (auto& species : m_species) {
        species.clear();
    }
}

const NEATGenome& BrainEvolutionManager::getBestGenome() const {
    return m_bestGenome;
}

float BrainEvolutionManager::getAverageFitness() const {
    if (m_genomes.empty()) return 0.0f;

    float sum = 0.0f;
    for (const auto& g : m_genomes) {
        sum += g.getFitness();
    }
    return sum / m_genomes.size();
}

float BrainEvolutionManager::getBestFitness() const {
    return m_bestFitness;
}

float BrainEvolutionManager::getAverageComplexity() const {
    if (m_genomes.empty()) return 0.0f;

    float sum = 0.0f;
    for (const auto& g : m_genomes) {
        sum += g.getComplexity();
    }
    return sum / m_genomes.size();
}

} // namespace ai
