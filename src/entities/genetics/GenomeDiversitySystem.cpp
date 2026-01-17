#include "GenomeDiversitySystem.h"
#include "../../core/Random.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace Forge {

// ============================================================================
// DIVERSITY METRICS IMPLEMENTATION
// ============================================================================

void DiversityMetrics::calculateScore() {
    // Weighted score calculation
    float varianceScore = (sizeVariance + speedVariance + visionVariance + efficiencyVariance) / 4.0f;
    varianceScore = std::min(varianceScore * 10.0f, 100.0f);  // Scale to 0-100

    float morphologyScore = (distinctBodyPlans + distinctAppendages + distinctLocomotion) / 3.0f;
    morphologyScore = std::min(morphologyScore * 20.0f, 100.0f);

    int totalCreatures = landCreatures + aquaticCreatures + flyingCreatures;
    float domainBalance = 0.0f;
    if (totalCreatures > 0) {
        // Calculate how balanced the domain distribution is (Shannon entropy)
        float landRatio = static_cast<float>(landCreatures) / totalCreatures;
        float aquaticRatio = static_cast<float>(aquaticCreatures) / totalCreatures;
        float flyingRatio = static_cast<float>(flyingCreatures) / totalCreatures;

        float entropy = 0.0f;
        if (landRatio > 0) entropy -= landRatio * std::log2(landRatio);
        if (aquaticRatio > 0) entropy -= aquaticRatio * std::log2(aquaticRatio);
        if (flyingRatio > 0) entropy -= flyingRatio * std::log2(flyingRatio);

        // Max entropy for 3 categories is log2(3) ≈ 1.585
        domainBalance = (entropy / 1.585f) * 100.0f;
    }

    // Weighted average
    overallScore = (varianceScore * 0.4f) + (morphologyScore * 0.4f) + (domainBalance * 0.2f);
}

void DiversityMetrics::reset() {
    sizeVariance = 0.0f;
    speedVariance = 0.0f;
    visionVariance = 0.0f;
    efficiencyVariance = 0.0f;
    distinctBodyPlans = 0;
    distinctAppendages = 0;
    distinctLocomotion = 0;
    landCreatures = 0;
    aquaticCreatures = 0;
    flyingCreatures = 0;
    overallScore = 0.0f;
}

void DiversityMetrics::log() const {
    std::cout << "\n=== GENOME DIVERSITY METRICS ===" << std::endl;
    std::cout << "  Overall Diversity Score: " << overallScore << "/100" << std::endl;
    std::cout << "\n  Trait Variance:" << std::endl;
    std::cout << "    Size:       " << sizeVariance << std::endl;
    std::cout << "    Speed:      " << speedVariance << std::endl;
    std::cout << "    Vision:     " << visionVariance << std::endl;
    std::cout << "    Efficiency: " << efficiencyVariance << std::endl;
    std::cout << "\n  Morphology Diversity:" << std::endl;
    std::cout << "    Distinct Body Plans:  " << distinctBodyPlans << std::endl;
    std::cout << "    Distinct Appendages:  " << distinctAppendages << std::endl;
    std::cout << "    Distinct Locomotion:  " << distinctLocomotion << std::endl;
    std::cout << "\n  Domain Distribution:" << std::endl;
    std::cout << "    Land:    " << landCreatures << std::endl;
    std::cout << "    Aquatic: " << aquaticCreatures << std::endl;
    std::cout << "    Flying:  " << flyingCreatures << std::endl;
    std::cout << "================================\n" << std::endl;
}

// ============================================================================
// BIOME PROFILE SELECTION
// ============================================================================

BiomeGenomeProfile GenomeDiversitySystem::getBaseProfileForBiome(BiomeType biome) const {
    BiomeGenomeProfile profile;

    switch (biome) {
        case BiomeType::TROPICAL_RAINFOREST:
            profile.preset = EvolutionStartPreset::COMPLEX;
            profile.bias = EvolutionGuidanceBias::NONE;
            profile.sizeModifier = 0.9f;       // Smaller on average
            profile.speedModifier = 1.1f;      // Faster
            profile.sensoryModifier = 1.2f;    // Rich sensory environment
            profile.primaryNiche = EcologicalNiche::ARBOREAL;
            profile.secondaryNiche = EcologicalNiche::GENERALIST;
            break;

        case BiomeType::DESERT:
            profile.preset = EvolutionStartPreset::EARLY_LIMB;
            profile.bias = EvolutionGuidanceBias::LAND;
            profile.sizeModifier = 0.85f;      // Smaller (heat regulation)
            profile.speedModifier = 1.15f;     // Fast (escape predators)
            profile.sensoryModifier = 1.1f;    // Sharp vision for open terrain
            profile.primaryNiche = EcologicalNiche::NOCTURNAL;
            profile.secondaryNiche = EcologicalNiche::BURROWER;
            break;

        case BiomeType::TUNDRA:
            profile.preset = EvolutionStartPreset::EARLY_LIMB;
            profile.bias = EvolutionGuidanceBias::LAND;
            profile.sizeModifier = 1.3f;       // Larger (cold adaptation)
            profile.speedModifier = 0.9f;      // Slower (energy conservation)
            profile.sensoryModifier = 1.0f;
            profile.primaryNiche = EcologicalNiche::GRAZER;
            profile.secondaryNiche = EcologicalNiche::PURSUIT_PREDATOR;
            break;

        case BiomeType::TEMPERATE_FOREST:
            profile.preset = EvolutionStartPreset::COMPLEX;
            profile.bias = EvolutionGuidanceBias::NONE;
            profile.sizeModifier = 1.0f;
            profile.speedModifier = 1.0f;
            profile.sensoryModifier = 1.1f;
            profile.primaryNiche = EcologicalNiche::GENERALIST;
            profile.secondaryNiche = EcologicalNiche::ARBOREAL;
            break;

        case BiomeType::GRASSLAND:
            profile.preset = EvolutionStartPreset::COMPLEX;
            profile.bias = EvolutionGuidanceBias::LAND;
            profile.sizeModifier = 1.2f;       // Larger grazers
            profile.speedModifier = 1.2f;      // Fast runners
            profile.sensoryModifier = 1.15f;   // Long-distance vision
            profile.primaryNiche = EcologicalNiche::GRAZER;
            profile.secondaryNiche = EcologicalNiche::PURSUIT_PREDATOR;
            break;

        case BiomeType::WETLAND:
            profile.preset = EvolutionStartPreset::COMPLEX;
            profile.bias = EvolutionGuidanceBias::AQUATIC;
            profile.sizeModifier = 1.1f;
            profile.speedModifier = 0.95f;
            profile.sensoryModifier = 1.2f;    // Murky water needs better senses
            profile.primaryNiche = EcologicalNiche::FILTER_FEEDER;
            profile.secondaryNiche = EcologicalNiche::AMBUSH_PREDATOR;
            break;

        case BiomeType::MOUNTAIN:
            profile.preset = EvolutionStartPreset::ADVANCED;
            profile.bias = EvolutionGuidanceBias::FLIGHT;
            profile.sizeModifier = 0.9f;
            profile.speedModifier = 1.1f;
            profile.sensoryModifier = 1.25f;   // High altitude vision
            profile.primaryNiche = EcologicalNiche::AERIAL_HUNTER;
            profile.secondaryNiche = EcologicalNiche::GENERALIST;
            break;

        case BiomeType::TAIGA:
            profile.preset = EvolutionStartPreset::COMPLEX;
            profile.bias = EvolutionGuidanceBias::LAND;
            profile.sizeModifier = 1.15f;      // Moderate size
            profile.speedModifier = 1.0f;
            profile.sensoryModifier = 1.1f;
            profile.primaryNiche = EcologicalNiche::PURSUIT_PREDATOR;
            profile.secondaryNiche = EcologicalNiche::GRAZER;
            break;

        case BiomeType::SAVANNA:
            profile.preset = EvolutionStartPreset::ADVANCED;
            profile.bias = EvolutionGuidanceBias::LAND;
            profile.sizeModifier = 1.25f;      // Large megafauna
            profile.speedModifier = 1.15f;     // Fast predators and prey
            profile.sensoryModifier = 1.2f;
            profile.primaryNiche = EcologicalNiche::GRAZER;
            profile.secondaryNiche = EcologicalNiche::PURSUIT_PREDATOR;
            break;

        case BiomeType::DEEP_OCEAN:
            profile.preset = EvolutionStartPreset::ADVANCED;
            profile.bias = EvolutionGuidanceBias::AQUATIC;
            profile.sizeModifier = 1.4f;       // Large aquatic creatures
            profile.speedModifier = 1.1f;
            profile.sensoryModifier = 0.8f;    // Low light
            profile.primaryNiche = EcologicalNiche::DEEP_DIVER;
            profile.secondaryNiche = EcologicalNiche::FILTER_FEEDER;
            profile.archetypeHint = BiomeGenomeProfile::ArchetypeHint::WHALE;
            break;

        case BiomeType::CORAL_REEF:
            profile.preset = EvolutionStartPreset::ADVANCED;
            profile.bias = EvolutionGuidanceBias::AQUATIC;
            profile.sizeModifier = 0.7f;       // Small reef fish
            profile.speedModifier = 1.2f;      // Agile
            profile.sensoryModifier = 1.3f;    // Colorful, complex environment
            profile.primaryNiche = EcologicalNiche::GENERALIST;
            profile.secondaryNiche = EcologicalNiche::FILTER_FEEDER;
            break;

        case BiomeType::VOLCANIC:
            profile.preset = EvolutionStartPreset::PROTO;
            profile.bias = EvolutionGuidanceBias::UNDERGROUND;
            profile.sizeModifier = 0.6f;       // Small, hardy
            profile.speedModifier = 0.8f;
            profile.sensoryModifier = 0.9f;
            profile.primaryNiche = EcologicalNiche::SCAVENGER;
            profile.secondaryNiche = EcologicalNiche::BURROWER;
            break;

        default:
            // Default to early limb generalist
            profile.preset = EvolutionStartPreset::EARLY_LIMB;
            profile.bias = EvolutionGuidanceBias::NONE;
            profile.sizeModifier = 1.0f;
            profile.speedModifier = 1.0f;
            profile.sensoryModifier = 1.0f;
            profile.primaryNiche = EcologicalNiche::GENERALIST;
            profile.secondaryNiche = EcologicalNiche::GENERALIST;
            break;
    }

    return profile;
}

void GenomeDiversitySystem::applyNicheModifiers(
    BiomeGenomeProfile& profile,
    EcologicalNiche niche
) const {
    switch (niche) {
        case EcologicalNiche::AMBUSH_PREDATOR:
            profile.sizeModifier *= 1.2f;
            profile.speedModifier *= 0.85f;
            profile.archetypeHint = BiomeGenomeProfile::ArchetypeHint::PREDATOR;
            break;

        case EcologicalNiche::PURSUIT_PREDATOR:
            profile.sizeModifier *= 0.95f;
            profile.speedModifier *= 1.3f;
            profile.sensoryModifier *= 1.15f;
            profile.archetypeHint = BiomeGenomeProfile::ArchetypeHint::PREDATOR;
            break;

        case EcologicalNiche::GRAZER:
            profile.sizeModifier *= 1.15f;
            profile.speedModifier *= 1.05f;
            break;

        case EcologicalNiche::SCAVENGER:
            profile.sizeModifier *= 0.9f;
            profile.speedModifier *= 0.95f;
            profile.sensoryModifier *= 1.25f;  // Good smell
            break;

        case EcologicalNiche::BURROWER:
            profile.sizeModifier *= 0.8f;
            profile.speedModifier *= 0.85f;
            profile.bias = EvolutionGuidanceBias::UNDERGROUND;
            break;

        case EcologicalNiche::ARBOREAL:
            profile.sizeModifier *= 0.85f;
            profile.speedModifier *= 1.1f;
            break;

        case EcologicalNiche::NOCTURNAL:
            profile.sensoryModifier *= 1.3f;  // Enhanced senses
            break;

        case EcologicalNiche::AERIAL_HUNTER:
            profile.bias = EvolutionGuidanceBias::FLIGHT;
            profile.speedModifier *= 1.2f;
            profile.sensoryModifier *= 1.2f;
            profile.archetypeHint = BiomeGenomeProfile::ArchetypeHint::BIRD;
            break;

        case EcologicalNiche::FILTER_FEEDER:
            profile.sizeModifier *= 1.1f;
            profile.speedModifier *= 0.9f;
            break;

        case EcologicalNiche::DEEP_DIVER:
            profile.sizeModifier *= 1.3f;
            profile.sensoryModifier *= 0.7f;
            profile.archetypeHint = BiomeGenomeProfile::ArchetypeHint::WHALE;
            break;

        case EcologicalNiche::GENERALIST:
        default:
            // No modifications
            break;
    }
}

EcologicalNiche GenomeDiversitySystem::selectNiche(
    BiomeType biome,
    CreatureType type,
    const glm::vec3& position,
    uint32_t seed
) const {
    // Use position and seed for deterministic but varied selection
    uint32_t hash = seed;
    hash ^= static_cast<uint32_t>(position.x * 1000.0f);
    hash ^= static_cast<uint32_t>(position.z * 1000.0f) << 16;
    hash ^= static_cast<uint32_t>(type) << 8;

    // Get base profile for biome to determine primary/secondary niches
    BiomeGenomeProfile baseProfile = getBaseProfileForBiome(biome);

    // 70% chance of primary niche, 30% chance of secondary
    float chance = static_cast<float>(hash % 100) / 100.0f;
    if (chance < 0.7f) {
        return baseProfile.primaryNiche;
    } else {
        return baseProfile.secondaryNiche;
    }
}

BiomeGenomeProfile GenomeDiversitySystem::selectProfile(
    BiomeType biome,
    CreatureType creatureType,
    const glm::vec3& spawnPosition,
    uint32_t worldSeed
) const {
    // Get base profile for biome
    BiomeGenomeProfile profile = getBaseProfileForBiome(biome);

    // Select niche based on position (creates spatial variation)
    EcologicalNiche selectedNiche = selectNiche(biome, creatureType, spawnPosition, worldSeed);

    // Apply niche modifiers
    applyNicheModifiers(profile, selectedNiche);

    // Override archetype hint based on creature type if needed
    switch (creatureType) {
        case CreatureType::AQUATIC_BASIC:
        case CreatureType::AQUATIC_SCHOOL:
            if (profile.archetypeHint == BiomeGenomeProfile::ArchetypeHint::GENERIC) {
                profile.archetypeHint = BiomeGenomeProfile::ArchetypeHint::GENERIC;
            }
            break;

        case CreatureType::AQUATIC_PREDATOR:
        case CreatureType::AQUATIC_APEX:
            profile.archetypeHint = BiomeGenomeProfile::ArchetypeHint::SHARK;
            break;

        case CreatureType::FLYING_BASIC:
        case CreatureType::FLYING_SMALL:
            profile.archetypeHint = BiomeGenomeProfile::ArchetypeHint::BIRD;
            break;

        case CreatureType::AERIAL_PREDATOR:
            profile.archetypeHint = BiomeGenomeProfile::ArchetypeHint::BIRD;
            profile.preset = EvolutionStartPreset::ADVANCED;
            break;

        default:
            break;
    }

    return profile;
}

void GenomeDiversitySystem::initializeGenome(
    Genome& genome,
    const BiomeGenomeProfile& profile,
    const PlanetChemistry& chemistry
) const {
    // First, initialize with preset and bias
    genome.initializeForPreset(profile.preset, profile.bias, chemistry);

    // Apply archetype-specific initialization if needed
    switch (profile.archetypeHint) {
        case BiomeGenomeProfile::ArchetypeHint::SHARK:
            genome.randomizeShark();
            break;
        case BiomeGenomeProfile::ArchetypeHint::BIRD:
            genome.randomizeBird();
            break;
        case BiomeGenomeProfile::ArchetypeHint::INSECT:
            genome.randomizeInsect();
            break;
        case BiomeGenomeProfile::ArchetypeHint::PREDATOR:
            genome.randomizeAquaticPredator();
            break;
        case BiomeGenomeProfile::ArchetypeHint::WHALE:
            genome.randomizeWhale();
            break;
        case BiomeGenomeProfile::ArchetypeHint::OCTOPUS:
            genome.randomizeOctopus();
            break;
        case BiomeGenomeProfile::ArchetypeHint::EEL:
            genome.randomizeEel();
            break;
        case BiomeGenomeProfile::ArchetypeHint::CRUSTACEAN:
            genome.randomizeCrustacean();
            break;
        case BiomeGenomeProfile::ArchetypeHint::GENERIC:
        default:
            // Already initialized by initializeForPreset
            break;
    }

    // Apply profile modifiers
    genome.size *= profile.sizeModifier;
    genome.speed *= profile.speedModifier;
    genome.visionRange *= profile.sensoryModifier;
    genome.visionAcuity *= profile.sensoryModifier;
    genome.hearingRange *= profile.sensoryModifier;
    genome.smellRange *= profile.sensoryModifier;

    // Clamp to valid ranges
    genome.size = std::clamp(genome.size, 0.5f, 2.0f);
    genome.speed = std::clamp(genome.speed, 5.0f, 20.0f);
    genome.efficiency = std::clamp(genome.efficiency, 0.5f, 1.5f);

    // Re-adapt to chemistry after modifications
    genome.adaptToChemistry(chemistry);

    // ========================================================================
    // BALANCE AND SAFETY CHECKS
    // ========================================================================

    // Ensure viable energy efficiency
    // Creatures that are too inefficient will die immediately
    if (genome.efficiency > 1.3f) {
        genome.efficiency = 1.0f + (genome.efficiency - 1.3f) * 0.5f;  // Soften extreme inefficiency
    }

    // Ensure viable locomotion
    // Land creatures need reasonable speed
    if (genome.wingSpan < 0.5f && genome.finSize < 0.3f) {
        // Land creature
        if (genome.speed < 6.0f) {
            genome.speed = 6.0f + Random::range(0.0f, 2.0f);  // Minimum viable speed
        }
    }

    // Flying creatures need adequate wing size and low body density
    if (genome.wingSpan > 0.5f) {
        if (genome.bodyDensity > 1.1f) {
            genome.bodyDensity = 0.9f + Random::range(0.0f, 0.15f);  // Lighter for flight
        }
        if (genome.wingSpan < 0.7f) {
            genome.wingSpan = 0.7f + Random::range(0.0f, 0.3f);  // Minimum wing size
        }
    }

    // Aquatic creatures need adequate swimming traits
    if (genome.finSize > 0.3f || genome.tailSize > 0.5f) {
        if (genome.swimFrequency < 1.0f) {
            genome.swimFrequency = 1.5f + Random::range(0.0f, 0.5f);  // Minimum swim frequency
        }
        if (genome.gillEfficiency < 0.5f) {
            genome.gillEfficiency = 0.7f + Random::range(0.0f, 0.3f);  // Minimum gill efficiency
        }
    }

    // Ensure sensory systems are not completely disabled
    if (genome.visionAcuity < 0.1f && genome.hearingRange < 10.0f && genome.smellRange < 10.0f) {
        // At least one sense must be functional
        float sensoryBoost = Random::range(0.3f, 0.5f);
        genome.visionAcuity = std::max(genome.visionAcuity, sensoryBoost);
        genome.visionRange = std::max(genome.visionRange, 20.0f);
    }

    // Prevent extreme combinations that would cause instant death
    // Large + slow + inefficient = death sentence
    if (genome.size > 1.5f && genome.speed < 8.0f && genome.efficiency > 1.2f) {
        // Improve one attribute
        if (Random::chance(0.5f)) {
            genome.speed += 2.0f;  // Make faster
        } else {
            genome.efficiency -= 0.2f;  // Make more efficient
        }
    }

    // Small + low sensory + slow = easy prey
    if (genome.size < 0.7f && genome.speed < 10.0f &&
        genome.visionRange < 20.0f && genome.hearingRange < 30.0f) {
        // Boost either speed or senses
        if (Random::chance(0.5f)) {
            genome.speed += 3.0f;
        } else {
            genome.visionRange *= 1.5f;
            genome.hearingRange *= 1.5f;
        }
    }

    // Final safety clamps
    genome.size = std::clamp(genome.size, 0.5f, 2.0f);
    genome.speed = std::clamp(genome.speed, 5.0f, 20.0f);
    genome.efficiency = std::clamp(genome.efficiency, 0.5f, 1.5f);
    genome.visionRange = std::clamp(genome.visionRange, 10.0f, 50.0f);
}

// ============================================================================
// DIVERSITY TRACKING
// ============================================================================

void GenomeDiversitySystem::trackCreature(const Genome& genome, CreatureType type) {
    TrackedCreature tracked;
    tracked.genome = genome;
    tracked.type = type;
    m_trackedCreatures.push_back(tracked);

    // Update archetype counters
    int archetypeKey = static_cast<int>(genome.segmentCount) * 1000 +
                       static_cast<int>(genome.limbSegments) * 100 +
                       static_cast<int>(genome.finCount);
    m_archetypeCounts[archetypeKey]++;
}

DiversityMetrics GenomeDiversitySystem::calculateDiversityMetrics() const {
    DiversityMetrics metrics;

    if (m_trackedCreatures.empty()) {
        return metrics;
    }

    // Extract trait values
    std::vector<float> sizes, speeds, visions, efficiencies;
    std::vector<int> segments, limbs, fins;

    for (const auto& tracked : m_trackedCreatures) {
        const Genome& g = tracked.genome;

        sizes.push_back(g.size);
        speeds.push_back(g.speed);
        visions.push_back(g.visionRange);
        efficiencies.push_back(g.efficiency);

        segments.push_back(g.segmentCount);
        limbs.push_back(g.limbSegments);
        fins.push_back(g.finCount);

        // Count by domain (simplified - based on fin presence)
        if (g.wingSpan > 0.5f) {
            metrics.flyingCreatures++;
        } else if (g.finSize > 0.3f || g.tailSize > 0.5f) {
            metrics.aquaticCreatures++;
        } else {
            metrics.landCreatures++;
        }
    }

    // Calculate variances
    metrics.sizeVariance = calculateVariance(sizes);
    metrics.speedVariance = calculateVariance(speeds);
    metrics.visionVariance = calculateVariance(visions);
    metrics.efficiencyVariance = calculateVariance(efficiencies);

    // Count distinct morphologies
    metrics.distinctBodyPlans = countDistinctValues(segments, 0);
    metrics.distinctAppendages = countDistinctValues(limbs, 0);
    metrics.distinctLocomotion = countDistinctValues(fins, 1);

    // Calculate overall score
    metrics.calculateScore();

    return metrics;
}

void GenomeDiversitySystem::resetTracking() {
    m_trackedCreatures.clear();
    m_archetypeCounts.clear();
}

float GenomeDiversitySystem::getDiversityScore() const {
    DiversityMetrics metrics = calculateDiversityMetrics();
    return metrics.overallScore;
}

bool GenomeDiversitySystem::needsMoreVariety(CreatureDomain domain) const {
    DiversityMetrics metrics = calculateDiversityMetrics();

    int totalCreatures = metrics.landCreatures + metrics.aquaticCreatures + metrics.flyingCreatures;
    if (totalCreatures == 0) return false;

    float domainRatio = 0.0f;
    switch (domain) {
        case CreatureDomain::LAND:
            domainRatio = static_cast<float>(metrics.landCreatures) / totalCreatures;
            break;
        case CreatureDomain::WATER:
            domainRatio = static_cast<float>(metrics.aquaticCreatures) / totalCreatures;
            break;
        case CreatureDomain::AIR:
            domainRatio = static_cast<float>(metrics.flyingCreatures) / totalCreatures;
            break;
    }

    // Need more variety if domain is underrepresented (<20% of population)
    return domainRatio < 0.2f;
}

BiomeGenomeProfile::ArchetypeHint GenomeDiversitySystem::getRecommendedArchetype(CreatureDomain domain) const {
    // Find least common archetype in this domain
    // Simplified: return different archetypes based on domain
    switch (domain) {
        case CreatureDomain::WATER:
            // Cycle through aquatic archetypes
            if (m_archetypeCounts.size() % 4 == 0) return BiomeGenomeProfile::ArchetypeHint::SHARK;
            if (m_archetypeCounts.size() % 4 == 1) return BiomeGenomeProfile::ArchetypeHint::WHALE;
            if (m_archetypeCounts.size() % 4 == 2) return BiomeGenomeProfile::ArchetypeHint::OCTOPUS;
            return BiomeGenomeProfile::ArchetypeHint::EEL;

        case CreatureDomain::AIR:
            return BiomeGenomeProfile::ArchetypeHint::BIRD;

        case CreatureDomain::LAND:
        default:
            if (m_archetypeCounts.size() % 2 == 0) return BiomeGenomeProfile::ArchetypeHint::PREDATOR;
            return BiomeGenomeProfile::ArchetypeHint::GENERIC;
    }
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

float GenomeDiversitySystem::calculateVariance(const std::vector<float>& values) const {
    if (values.empty()) return 0.0f;

    float mean = 0.0f;
    for (float v : values) mean += v;
    mean /= values.size();

    float variance = 0.0f;
    for (float v : values) {
        float diff = v - mean;
        variance += diff * diff;
    }
    variance /= values.size();

    return variance;
}

int GenomeDiversitySystem::countDistinctValues(const std::vector<int>& values, int tolerance) const {
    if (values.empty()) return 0;

    std::vector<int> uniqueValues;
    for (int v : values) {
        bool found = false;
        for (int u : uniqueValues) {
            if (std::abs(v - u) <= tolerance) {
                found = true;
                break;
            }
        }
        if (!found) {
            uniqueValues.push_back(v);
        }
    }

    return static_cast<int>(uniqueValues.size());
}

} // namespace Forge
