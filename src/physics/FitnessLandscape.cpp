#include "FitnessLandscape.h"
#include <algorithm>
#include <cmath>

// =============================================================================
// FITNESS CALCULATOR IMPLEMENTATION
// =============================================================================

FitnessFactors FitnessCalculator::calculateFactors(
    const MorphologyGenes& genes,
    CreatureType type)
{
    FitnessFactors factors;

    // Movement
    factors.movementSpeed = calculateSpeedFactor(genes);
    factors.acceleration = calculateAccelerationFactor(genes);
    factors.maneuverability = calculateManeuverabilityFactor(genes);
    factors.stability = calculateStabilityFactor(genes);
    factors.terrainTraversal = calculateTerrainFactor(genes);

    // Energy
    factors.energyEfficiency = calculateEfficiencyFactor(genes);
    factors.metabolicEfficiency = calculateMetabolicFactor(genes);
    factors.staminaFactor = calculateStaminaFactor(genes);

    // Combat
    factors.attackReach = calculateAttackReach(genes);
    factors.attackPower = calculateAttackPower(genes);
    factors.attackSpeed = factors.movementSpeed * 0.5f + factors.maneuverability * 0.5f;
    factors.defensiveAbility = calculateDefense(genes);
    factors.intimidation = genes.baseMass * 0.3f + genes.featureSize * 0.5f +
                          (genes.primaryFeature == FeatureType::HORNS ? 0.3f : 0.0f);

    // Survival
    factors.predatorEvasion = calculateEvasion(genes);
    factors.camouflage = 1.0f - genes.featureSize; // Flashy features reduce camouflage
    factors.sensoryRange = calculateSensory(genes);
    factors.coldResistance = genes.baseMass * 0.3f + genes.armorCoverage * 0.5f;
    factors.heatResistance = 1.5f - genes.baseMass * 0.3f - genes.armorCoverage * 0.3f;

    // Resources
    factors.foodGathering = calculateGathering(genes);
    factors.reachCapability = genes.neckLength * 0.5f + genes.armLength * 0.5f;
    factors.carryingCapacity = genes.baseMass * 0.4f + genes.armPairs * 0.2f;

    // Reproduction
    factors.mateFinding = factors.movementSpeed * 0.4f + factors.sensoryRange * 0.6f;
    factors.displayQuality = genes.featureSize * 0.5f +
                            (genes.primaryFeature == FeatureType::CREST ? 0.3f : 0.0f) +
                            (genes.primaryFeature == FeatureType::ANTLERS ? 0.2f : 0.0f);
    factors.offspringCare = genes.armPairs > 0 ? 0.5f + genes.armPairs * 0.2f : 0.3f;

    return factors;
}

float FitnessCalculator::calculateOverallFitness(
    const FitnessFactors& factors,
    CreatureType type,
    EnvironmentType environment)
{
    float fitness = 0.0f;

    // Get environment modifiers
    float speedMod = EnvironmentModifiers::getSpeedModifier(environment);
    float manMod = EnvironmentModifiers::getManeuverabilityModifier(environment);
    float effMod = EnvironmentModifiers::getEfficiencyModifier(environment);
    float combatMod = EnvironmentModifiers::getCombatModifier(environment);
    float sensoryMod = EnvironmentModifiers::getSensoryModifier(environment);

    if (type == CreatureType::HERBIVORE) {
        // Herbivores prioritize: food gathering, evasion, efficiency
        fitness += factors.foodGathering * 0.25f;
        fitness += factors.predatorEvasion * 0.25f * speedMod;
        fitness += factors.energyEfficiency * 0.20f * effMod;
        fitness += factors.movementSpeed * 0.15f * speedMod;
        fitness += factors.sensoryRange * 0.15f * sensoryMod;
    } else {
        // Carnivores prioritize: attack, speed, sensory
        fitness += factors.attackPower * 0.25f * combatMod;
        fitness += factors.movementSpeed * 0.25f * speedMod;
        fitness += factors.attackReach * 0.15f;
        fitness += factors.sensoryRange * 0.20f * sensoryMod;
        fitness += factors.energyEfficiency * 0.15f * effMod;
    }

    return fitness;
}

float FitnessCalculator::calculateNicheFitness(
    const FitnessFactors& factors,
    NicheType niche)
{
    auto weights = NicheWeights::getWeights(niche);

    float fitness = 0.0f;
    fitness += factors.movementSpeed * weights.speed;
    fitness += factors.maneuverability * weights.maneuverability;
    fitness += factors.energyEfficiency * weights.efficiency;
    fitness += factors.attackPower * weights.attackPower;
    fitness += factors.defensiveAbility * weights.defense;
    fitness += factors.sensoryRange * weights.sensory;
    fitness += factors.foodGathering * weights.gathering;
    fitness += factors.camouflage * weights.stealth;

    return fitness;
}

float FitnessCalculator::calculateCombatAdvantage(
    const FitnessFactors& attacker,
    const FitnessFactors& defender)
{
    // Attack score
    float attackScore = attacker.attackPower * 0.4f +
                        attacker.attackSpeed * 0.3f +
                        attacker.attackReach * 0.3f;

    // Defense score
    float defenseScore = defender.defensiveAbility * 0.5f +
                         defender.maneuverability * 0.3f +
                         defender.movementSpeed * 0.2f;

    // Advantage is ratio of attack to defense
    if (defenseScore < 0.001f) return 1.0f;
    return attackScore / (attackScore + defenseScore);
}

float FitnessCalculator::calculateChaseOutcome(
    const FitnessFactors& pursuer,
    const FitnessFactors& prey,
    float initialDistance)
{
    // Pursuer needs speed advantage to catch up
    float speedDiff = pursuer.movementSpeed - prey.movementSpeed;

    // But prey maneuverability helps escape
    float maneuverBonus = (prey.maneuverability - pursuer.maneuverability) * 0.5f;

    // Stamina determines who tires first
    float staminaRatio = pursuer.staminaFactor / std::max(0.1f, prey.staminaFactor);

    // Calculate catch probability
    float baseCatchChance = 0.5f;

    // Speed advantage
    baseCatchChance += speedDiff * 0.2f;

    // Maneuverability
    baseCatchChance -= maneuverBonus * 0.3f;

    // Stamina
    baseCatchChance += (staminaRatio - 1.0f) * 0.2f;

    // Distance penalty
    baseCatchChance -= initialDistance * 0.02f;

    return std::max(0.0f, std::min(1.0f, baseCatchChance));
}

// Movement calculations
float FitnessCalculator::calculateSpeedFactor(const MorphologyGenes& genes) {
    float baseSpeed = genes.getMaxSpeed();

    // Limb configuration affects speed
    float limbBonus = 1.0f;
    if (genes.legPairs == 2) limbBonus = 1.1f;      // Quadrupeds are fast
    else if (genes.legPairs == 1) limbBonus = 0.85f; // Bipeds less efficient
    else if (genes.legPairs == 0) limbBonus = 0.4f;  // No legs = slow on land
    else if (genes.legPairs >= 4) limbBonus = 0.75f; // Many legs = stable but slower

    // Wing bonus for flight
    if (genes.canFly && genes.wingPairs > 0) {
        limbBonus *= 1.4f;
    }

    // Tail for balance at speed
    if (genes.hasTail && genes.tailLength > 0.5f) {
        limbBonus *= 1.05f;
    }

    // Body streamlining
    float streamline = 1.0f - (genes.bodyWidth / genes.bodyLength) * 0.3f;
    streamline = std::max(0.7f, streamline);

    return baseSpeed * limbBonus * streamline / 10.0f; // Normalize to 0-1ish range
}

float FitnessCalculator::calculateAccelerationFactor(const MorphologyGenes& genes) {
    // Smaller creatures accelerate faster
    float mass = genes.getExpectedMass();
    float accel = Allometry::muscleForce(mass) / mass;

    // Leg strength
    accel *= genes.jointStrength;

    // Normalize
    return std::min(1.5f, accel / 50.0f);
}

float FitnessCalculator::calculateManeuverabilityFactor(const MorphologyGenes& genes) {
    // Smaller = more maneuverable
    float sizeFactor = 2.0f - genes.baseMass;
    sizeFactor = std::max(0.5f, sizeFactor);

    // Tail helps with balance and turning
    float tailBonus = genes.hasTail ? 1.0f + genes.tailLength * 0.2f : 1.0f;

    // Many legs = stable but less agile
    float legFactor = 1.0f;
    if (genes.legPairs <= 2) legFactor = 1.2f;
    else if (genes.legPairs >= 4) legFactor = 0.8f;

    // Wings for aerial maneuverability
    float wingBonus = genes.wingPairs > 0 ? 1.2f : 1.0f;

    return sizeFactor * tailBonus * legFactor * wingBonus * 0.5f;
}

float FitnessCalculator::calculateStabilityFactor(const MorphologyGenes& genes) {
    // More legs = more stable
    float legStability = 0.3f + genes.legPairs * 0.15f;
    legStability = std::min(1.0f, legStability);

    // Wider stance = more stable
    float stanceFactor = genes.legSpread;

    // Lower center of gravity = more stable
    float heightFactor = 1.5f - genes.bodyHeight;
    heightFactor = std::max(0.5f, heightFactor);

    // Tail for balance
    float tailBonus = genes.hasTail ? 1.1f : 1.0f;

    return legStability * stanceFactor * heightFactor * tailBonus;
}

float FitnessCalculator::calculateTerrainFactor(const MorphologyGenes& genes) {
    float terrain = 0.5f;

    // Leg length helps with obstacles
    terrain += genes.legLength * 0.2f;

    // Climbing ability
    if (genes.primaryFeature == FeatureType::CLAWS || genes.hasHands) {
        terrain += 0.2f;
    }

    // Wings for crossing gaps
    if (genes.wingPairs > 0) {
        terrain += 0.3f;
    }

    // Fins for water
    if (genes.hasPectoralFins || genes.finCount > 0) {
        terrain += 0.1f;
    }

    return std::min(1.0f, terrain);
}

// Energy calculations
float FitnessCalculator::calculateEfficiencyFactor(const MorphologyGenes& genes) {
    // Base efficiency from metabolic rate
    float metabolic = genes.getMetabolicRate();
    float efficiency = 1.0f / std::max(0.1f, metabolic);

    // Streamlined body = less drag
    float streamline = genes.bodyLength / (genes.bodyWidth + genes.bodyHeight);
    efficiency *= 0.5f + streamline * 0.25f;

    // Efficient limb configuration
    if (genes.legPairs == 2) efficiency *= 1.1f; // Quadruped is efficient
    else if (genes.legPairs >= 4) efficiency *= 0.9f; // Many legs = more energy

    // Wings are energy-expensive unless gliding
    if (genes.wingPairs > 0 && genes.canFly) {
        efficiency *= 0.7f;
    }

    return std::min(1.5f, efficiency);
}

float FitnessCalculator::calculateMetabolicFactor(const MorphologyGenes& genes) {
    float metabolic = genes.getMetabolicRate();
    // Lower is better for this factor
    return 1.0f / (0.5f + metabolic * 0.5f);
}

float FitnessCalculator::calculateStaminaFactor(const MorphologyGenes& genes) {
    // Mass provides energy reserves
    float mass = genes.getExpectedMass();

    // But metabolic rate burns through it
    float metabolic = genes.getMetabolicRate();

    float stamina = mass / metabolic;

    // Efficient body plan helps
    if (genes.legPairs == 2) stamina *= 1.1f;

    return std::min(2.0f, stamina);
}

// Combat calculations
float FitnessCalculator::calculateAttackReach(const MorphologyGenes& genes) {
    float reach = 0.0f;

    // Neck extends reach
    reach += genes.neckLength * 0.3f;

    // Arms for reaching
    reach += genes.armLength * genes.armPairs * 0.2f;

    // Proboscis for distance attack
    if (genes.primaryFeature == FeatureType::PROBOSCIS) {
        reach += genes.featureSize * 0.3f;
    }

    // Tail can strike
    if (genes.hasTail && genes.tailLength > 0.5f) {
        reach += genes.tailLength * 0.15f;
    }

    return reach;
}

float FitnessCalculator::calculateAttackPower(const MorphologyGenes& genes) {
    float mass = genes.getExpectedMass();
    float basePower = Allometry::muscleForce(mass) / 100.0f;

    // Weapons multiply power
    float weaponBonus = 1.0f;
    switch (genes.primaryFeature) {
        case FeatureType::CLAWS:    weaponBonus = 1.5f; break;
        case FeatureType::HORNS:    weaponBonus = 1.4f; break;
        case FeatureType::ANTLERS:  weaponBonus = 1.3f; break;
        case FeatureType::MANDIBLES: weaponBonus = 1.6f; break;
        case FeatureType::SPIKES:   weaponBonus = 1.2f; break;
        default: break;
    }

    // Size of weapons
    weaponBonus *= (0.7f + genes.featureSize * 0.6f);

    // Jaw strength (head size)
    float jawBonus = 1.0f + genes.headSize * 0.3f;

    return basePower * weaponBonus * jawBonus;
}

float FitnessCalculator::calculateDefense(const MorphologyGenes& genes) {
    float defense = 0.3f; // Base

    // Armor is major defensive factor
    defense += genes.armorCoverage * 0.5f;

    // Shell provides defense
    if (genes.primaryFeature == FeatureType::SHELL) {
        defense += 0.3f;
    }

    // Spikes deter attackers
    if (genes.primaryFeature == FeatureType::SPIKES ||
        genes.secondaryFeature == FeatureType::SPIKES) {
        defense += 0.15f;
    }

    // Size makes you harder to kill
    defense += genes.baseMass * 0.1f;

    return std::min(1.0f, defense);
}

// Survival calculations
float FitnessCalculator::calculateEvasion(const MorphologyGenes& genes) {
    float evasion = calculateSpeedFactor(genes) * 0.5f;
    evasion += calculateManeuverabilityFactor(genes) * 0.3f;
    evasion += calculateAccelerationFactor(genes) * 0.2f;

    // Small creatures evade better
    evasion += (2.0f - genes.baseMass) * 0.1f;

    return std::min(1.0f, evasion);
}

float FitnessCalculator::calculateSensory(const MorphologyGenes& genes) {
    float sensory = 0.3f; // Base

    // Eyes
    sensory += genes.eyeCount * 0.05f;
    sensory += genes.eyeSize * 0.3f;

    // Side-facing eyes = wider view
    if (genes.eyesSideFacing) {
        sensory += 0.1f;
    }

    // Antennae for smell/touch
    if (genes.primaryFeature == FeatureType::NONE) {
        // No special sensory organs
    }

    // Head size (brain)
    sensory += genes.headSize * 0.2f;

    return std::min(1.0f, sensory);
}

// Resource calculations
float FitnessCalculator::calculateGathering(const MorphologyGenes& genes) {
    float gathering = 0.3f;

    // Hands for manipulation
    if (genes.hasHands) {
        gathering += 0.3f;
    }
    gathering += genes.armPairs * 0.15f;

    // Neck for reaching
    gathering += genes.neckLength * 0.2f;

    // Proboscis for reaching into flowers/crevices
    if (genes.primaryFeature == FeatureType::PROBOSCIS) {
        gathering += 0.2f;
    }

    // Head/mouth size
    gathering += genes.headSize * 0.15f;

    return std::min(1.0f, gathering);
}

// =============================================================================
// SPECIALIZATION CALCULATOR IMPLEMENTATION
// =============================================================================

float SpecializationCalculator::calculateNicheMatch(
    const MorphologyGenes& genes,
    NicheType niche)
{
    switch (niche) {
        case NicheType::PURSUIT_PREDATOR: return matchPursuitPredator(genes);
        case NicheType::AMBUSH_PREDATOR:  return matchAmbushPredator(genes);
        case NicheType::GRAZER:           return matchGrazer(genes);
        case NicheType::CLIMBER:          return matchClimber(genes);
        case NicheType::SWIMMER:          return matchSwimmer(genes);
        case NicheType::FLYER:            return matchFlyer(genes);
        default: return 0.5f; // Generalist
    }
}

NicheType SpecializationCalculator::determineOptimalNiche(const MorphologyGenes& genes) {
    float bestMatch = 0.0f;
    NicheType bestNiche = NicheType::GENERALIST;

    // Test all niches
    for (int i = 0; i < 10; i++) {
        NicheType niche = static_cast<NicheType>(i);
        float match = calculateNicheMatch(genes, niche);
        if (match > bestMatch) {
            bestMatch = match;
            bestNiche = niche;
        }
    }

    return bestNiche;
}

float SpecializationCalculator::getSpecializationBonus(
    const MorphologyGenes& genes,
    NicheType attemptedNiche)
{
    float match = calculateNicheMatch(genes, attemptedNiche);

    // Strong match = bonus, poor match = penalty
    // 0.5 = neutral, 1.0 = 50% bonus, 0.0 = 50% penalty
    return 0.5f + match;
}

float SpecializationCalculator::matchPursuitPredator(const MorphologyGenes& genes) {
    float match = 0.0f;

    // High speed is essential
    float speed = genes.getMaxSpeed();
    match += (speed > 12.0f) ? 0.3f : speed / 40.0f;

    // Long legs for running
    match += genes.legLength * 0.15f;

    // Quadruped is ideal
    if (genes.legPairs == 2) match += 0.2f;
    else if (genes.legPairs == 1) match += 0.1f;

    // Forward-facing eyes for tracking prey
    if (!genes.eyesSideFacing) match += 0.1f;

    // Claws for catching
    if (genes.primaryFeature == FeatureType::CLAWS) match += 0.15f;

    // Streamlined body
    match += (genes.bodyLength / genes.bodyWidth) * 0.05f;

    return std::min(1.0f, match);
}

float SpecializationCalculator::matchAmbushPredator(const MorphologyGenes& genes) {
    float match = 0.0f;

    // Compact body for hiding
    if (genes.bodyLength < 1.5f) match += 0.15f;

    // Powerful for quick strike
    match += genes.jointStrength * 0.2f;

    // Weapons for quick kill
    if (genes.primaryFeature == FeatureType::CLAWS) match += 0.2f;
    if (genes.primaryFeature == FeatureType::MANDIBLES) match += 0.15f;

    // Large eyes for low light
    match += genes.eyeSize * 0.2f;

    // Forward-facing eyes
    if (!genes.eyesSideFacing) match += 0.1f;

    // Low profile
    if (genes.bodyHeight < 0.5f) match += 0.1f;

    return std::min(1.0f, match);
}

float SpecializationCalculator::matchGrazer(const MorphologyGenes& genes) {
    float match = 0.0f;

    // Large body for processing plant matter
    match += genes.baseMass * 0.15f;

    // Wide mouth
    match += genes.headSize * 0.15f;

    // Long neck for reaching
    match += genes.neckLength * 0.2f;

    // Side-facing eyes for predator detection
    if (genes.eyesSideFacing) match += 0.15f;

    // Stable stance
    if (genes.legPairs >= 2) match += 0.1f;

    // Defensive features
    if (genes.armorCoverage > 0) match += 0.1f;
    if (genes.primaryFeature == FeatureType::HORNS) match += 0.1f;

    return std::min(1.0f, match);
}

float SpecializationCalculator::matchClimber(const MorphologyGenes& genes) {
    float match = 0.0f;

    // Light body
    match += (2.0f - genes.baseMass) * 0.15f;

    // Strong grip (claws or hands)
    if (genes.primaryFeature == FeatureType::CLAWS) match += 0.2f;
    if (genes.hasHands) match += 0.2f;

    // Prehensile tail
    if (genes.tailPrehensile) match += 0.2f;

    // Long limbs for reaching
    match += genes.armLength * 0.1f;
    match += genes.legLength * 0.1f;

    // Flexible joints
    match += genes.jointFlexibility * 0.1f;

    return std::min(1.0f, match);
}

float SpecializationCalculator::matchSwimmer(const MorphologyGenes& genes) {
    float match = 0.0f;

    // Fins are essential
    match += genes.finCount * 0.1f;
    if (genes.hasCaudalFin) match += 0.2f;
    if (genes.hasPectoralFins) match += 0.15f;
    if (genes.hasDorsalFin) match += 0.05f;

    // Streamlined body
    float streamline = genes.bodyLength / (genes.bodyWidth + genes.bodyHeight);
    match += streamline * 0.1f;

    // No legs (or reduced)
    if (genes.legPairs == 0) match += 0.2f;
    else if (genes.legPairs == 1) match += 0.05f;

    // Long tail for propulsion
    if (genes.hasTail && genes.tailLength > 0.5f) match += 0.1f;

    return std::min(1.0f, match);
}

float SpecializationCalculator::matchFlyer(const MorphologyGenes& genes) {
    float match = 0.0f;

    // Wings are essential
    if (genes.wingPairs > 0) match += 0.3f;
    else return 0.0f; // Can't fly without wings

    // Can actually fly (not just glide)
    if (genes.canFly) match += 0.2f;

    // Light body (wing loading)
    float mass = genes.getExpectedMass();
    float wingArea = genes.wingSpan * genes.wingChord;
    if (Allometry::canFly(mass, wingArea)) match += 0.2f;

    // Light bones (low density)
    match += (1.5f - genes.densityMultiplier) * 0.15f;

    // Streamlined
    if (genes.bodyWidth < genes.bodyLength) match += 0.1f;

    return std::min(1.0f, match);
}
