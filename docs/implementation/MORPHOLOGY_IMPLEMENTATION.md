# Morphology System Implementation Guide

This document describes how to integrate and use the advanced morphology evolution system.

## System Overview

The morphology system consists of several interconnected components:

```
MorphologyGenes  →  BodyPlan  →  MorphologyBuilder  →  MetaballSystem  →  Mesh
      ↓               ↓                                       ↓
  Evolution     Locomotion                              Rendering
      ↓               ↓
FitnessLandscape   PhysicsBody
```

## Core Components

### 1. MorphologyGenes (`src/physics/Morphology.h`)

The complete genetic encoding for creature body shape:

```cpp
MorphologyGenes genes;

// Body organization
genes.segmentCount = 4;        // 2-8 body segments
genes.bodyLength = 1.5f;       // Body length multiplier
genes.bodyWidth = 0.5f;        // Width relative to length
genes.bodyHeight = 0.4f;       // Height relative to length

// Limbs
genes.legPairs = 2;            // 0-4 pairs (0-8 total legs)
genes.armPairs = 1;            // 0-2 pairs
genes.wingPairs = 0;           // 0-1 pairs

// Special features
genes.primaryFeature = FeatureType::HORNS;
genes.hasTail = true;
genes.hasMetamorphosis = false;

// Evolution operations
genes.randomize();
genes.mutate(0.1f, 0.15f);
MorphologyGenes child = MorphologyGenes::crossover(parent1, parent2);
```

### 2. BodyPlan (`src/physics/Morphology.h`)

Converts genes into a hierarchical body structure:

```cpp
BodyPlan bodyPlan;
bodyPlan.buildFromGenes(genes);

// Access segments
const auto& segments = bodyPlan.getSegments();
int rootIdx = bodyPlan.findRootSegment();
std::vector<int> limbRoots = bodyPlan.findLimbRoots();

// Calculate properties
float mass = bodyPlan.getTotalMass();
glm::vec3 com = bodyPlan.getCenterOfMass();
```

### 3. FitnessLandscape (`src/physics/FitnessLandscape.h`)

Calculates how morphology affects survival:

```cpp
// Get all fitness factors
FitnessFactors factors = FitnessCalculator::calculateFactors(genes, CreatureType::HERBIVORE);

// Overall fitness
float fitness = FitnessCalculator::calculateOverallFitness(
    factors,
    CreatureType::HERBIVORE,
    EnvironmentType::PLAINS
);

// Niche-specific fitness
float nicheFitness = FitnessCalculator::calculateNicheFitness(factors, NicheType::GRAZER);

// Combat calculations
float advantage = FitnessCalculator::calculateCombatAdvantage(attackerFactors, defenderFactors);
```

### 4. Locomotion (`src/physics/Locomotion.h`)

Physics-based movement system:

```cpp
LocomotionController locomotion;
locomotion.initialize(bodyPlan, genes);

// In update loop
glm::vec3 desiredVelocity = glm::vec3(0, 0, 1) * maxSpeed;
locomotion.update(deltaTime, desiredVelocity, groundHeight);

// Get results
const auto& jointStates = locomotion.getJointStates();
float energyUsed = locomotion.getEnergyExpenditure();
bool stable = locomotion.isStable();
```

### 5. Visual Indicators (`src/physics/VisualIndicators.h`)

Visual state based on health/fitness:

```cpp
VisualState visual;
VisualStateCalculator::update(
    visual,
    energy, maxEnergy,
    health,
    fear,
    age,
    fitness,
    genes,
    lifeStage
);

// Get final color
glm::vec3 color = VisualStateCalculator::getFinalColor(visual);

// Get posture transformation
glm::mat4 posture = VisualStateCalculator::getPostureTransform(visual);
```

### 6. Metamorphosis (`src/physics/Metamorphosis.h`)

Life stage transitions:

```cpp
LifeStageController lifeStage;
lifeStage.initialize(genes);

// Update each frame
lifeStage.update(deltaTime, energy, health);

// Check state
if (lifeStage.isMetamorphosing()) {
    float progress = lifeStage.getTransformationProgress();
}

// Get current morphology (changes during metamorphosis)
MorphologyGenes currentMorph = lifeStage.getCurrentMorphology();
```

### 7. MorphologyBuilder (`src/graphics/procedural/MorphologyBuilder.h`)

Converts to metaballs for rendering:

```cpp
MetaballSystem metaballs;
MorphologyBuilder::buildFromMorphology(metaballs, genes, type, &visualState);

// Or generate mesh directly
auto mesh = CreatureMeshGenerator::generate(genes, type, 24);
```

## Integration with Existing Creature System

### Converting Old Genome to MorphologyGenes

```cpp
// In Creature constructor or initialization
MorphologyGenes morphology = MorphologyBuilder::genomeToMorphology(genome, type);
```

### Updated Creature Class

```cpp
class Creature {
    // Existing members...
    MorphologyGenes morphology;
    VisualState visualState;
    LifeStageController lifeStage;
    LocomotionController locomotion;

    void initialize() {
        morphology = MorphologyBuilder::genomeToMorphology(genome, type);
        lifeStage.initialize(morphology);

        BodyPlan bodyPlan;
        bodyPlan.buildFromGenes(morphology);
        locomotion.initialize(bodyPlan, morphology);
    }

    void update(float dt) {
        // Update life stage
        lifeStage.update(dt, energy, health);

        // Get current morphology (may change during metamorphosis)
        MorphologyGenes current = lifeStage.getCurrentMorphology();

        // Update locomotion
        locomotion.update(dt, velocity, terrain.getHeight(position));

        // Update visual state
        VisualStateCalculator::update(
            visualState, energy, maxEnergy, health, fear,
            age, fitness, current, lifeStage.getCurrentStage()
        );

        // Calculate fitness
        FitnessFactors factors = FitnessCalculator::calculateFactors(current, type);
        float fitnessScore = FitnessCalculator::calculateOverallFitness(factors, type);
    }
};
```

## Evolutionary Pressure

### Speed vs. Efficiency Trade-off

```cpp
// Large creatures: slower but stronger
float speed = Allometry::maxSpeed(mass);
float force = Allometry::muscleForce(mass);

// High leg count: stable but slower
if (genes.legPairs >= 3) {
    speed *= 0.85f;  // Penalty
    stability *= 1.3f;  // Bonus
}
```

### Niche Specialization

```cpp
// Determine optimal niche for morphology
NicheType bestNiche = SpecializationCalculator::determineOptimalNiche(genes);

// Get bonus/penalty for attempting a niche
float bonus = SpecializationCalculator::getSpecializationBonus(genes, attemptedNiche);
```

### Combat Fitness

```cpp
// Weapons increase attack power
if (genes.primaryFeature == FeatureType::CLAWS) {
    attackPower *= 1.5f;
}

// Armor increases defense
defense += genes.armorCoverage * 0.5f;

// Size affects both
attackPower *= genes.baseMass;
defense += genes.baseMass * 0.1f;
```

## Gait Selection

The system automatically selects appropriate gaits:

| Legs | Low Speed | Medium Speed | High Speed |
|------|-----------|--------------|------------|
| 0    | Slither   | Slither      | Slither    |
| 2    | Walk      | Walk         | Hop        |
| 4    | Walk      | Trot         | Gallop     |
| 6    | Wave      | Tripod       | Tripod     |
| 8+   | Wave      | Wave         | Wave       |

## Metamorphosis Types

```cpp
enum class MetamorphosisType {
    NONE,           // Direct development (mammals)
    GRADUAL,        // Nymph stages (grasshoppers)
    COMPLETE,       // Larva → pupa → adult (butterflies)
    AQUATIC_TO_LAND // Tadpole → frog
};
```

## File Structure

```
src/physics/
├── Morphology.h/.cpp          # MorphologyGenes, BodyPlan, Allometry
├── Locomotion.h/.cpp          # CPG, IK, LocomotionController, PhysicsBody
├── FitnessLandscape.h/.cpp    # FitnessCalculator, SpecializationCalculator
├── VisualIndicators.h/.cpp    # VisualState, VisualStateCalculator
└── Metamorphosis.h/.cpp       # LifeStageController, MorphologyInterpolator

src/graphics/procedural/
└── MorphologyBuilder.h/.cpp   # MorphologyBuilder, CreatureMeshGenerator

docs/
├── MORPHOLOGY_EVOLUTION.md    # Research documentation
└── MORPHOLOGY_IMPLEMENTATION.md  # This file
```

## Performance Considerations

1. **Mesh Generation**: Cache meshes by morphology hash
2. **Fitness Calculation**: Cache and update only when genes change
3. **IK Solving**: Limit iterations (10 is usually enough)
4. **CPG Updates**: Simple phase advancement, very fast

## Future Extensions

1. **Procedural Textures**: Generate scale/fur patterns from genes
2. **Behavioral Traits**: Link morphology to neural network structure
3. **Environmental Adaptation**: Morphology changes within lifetime
4. **Sexual Dimorphism**: Different morphologies for males/females
5. **Symbiosis**: Morphological adaptations for mutualism
