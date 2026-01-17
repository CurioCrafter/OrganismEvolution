#include "BiochemistrySystem.h"
#include "../entities/Genome.h"
#include "../environment/PlanetChemistry.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <iomanip>

// ============================================================================
// PIGMENT HINT IMPLEMENTATION
// ============================================================================

glm::vec3 PigmentHint::blendWithGenome(const glm::vec3& genomeColor, float strength) const {
    // Blend primary pigment color with genome's natural color
    glm::vec3 blended = glm::mix(genomeColor, primaryColor, strength);

    // Apply saturation and brightness biases
    float avgBrightness = (blended.r + blended.g + blended.b) / 3.0f;

    // Saturation adjustment
    if (saturationBias > 0.0f) {
        // Increase saturation - push away from gray
        blended = glm::mix(glm::vec3(avgBrightness), blended, 1.0f + saturationBias * 0.5f);
    } else if (saturationBias < 0.0f) {
        // Decrease saturation - pull toward gray
        blended = glm::mix(blended, glm::vec3(avgBrightness), -saturationBias * 0.5f);
    }

    // Brightness adjustment
    blended += glm::vec3(brightnessBias * 0.2f);

    // Clamp to valid range
    return glm::clamp(blended, glm::vec3(0.0f), glm::vec3(1.0f));
}

// ============================================================================
// BIOCHEMISTRY SYSTEM SINGLETON
// ============================================================================

BiochemistrySystem& BiochemistrySystem::getInstance() {
    static BiochemistrySystem instance;
    return instance;
}

BiochemistrySystem::BiochemistrySystem()
    : m_currentFrame(0)
    , m_cacheLifetimeFrames(3600)  // Default: 60 seconds at 60 FPS
{
}

// ============================================================================
// CORE COMPUTATION METHODS
// ============================================================================

BiochemistryCompatibility BiochemistrySystem::computeCompatibility(const Genome& genome, const PlanetChemistry& chemistry) const {
    BiochemistryCompatibility result;

    // Compute individual components
    result.solventCompatibility = computeSolventCompatibility(genome, chemistry);
    result.oxygenCompatibility = computeOxygenCompatibility(genome, chemistry);
    result.temperatureCompatibility = computeTemperatureCompatibility(genome, chemistry);
    result.radiationCompatibility = computeRadiationCompatibility(genome, chemistry);
    result.acidityCompatibility = computeAcidityCompatibility(genome, chemistry);
    result.mineralCompatibility = computeMineralCompatibility(genome, chemistry);

    // Compute weighted overall score
    result.overall =
        result.solventCompatibility * SOLVENT_WEIGHT +
        result.oxygenCompatibility * OXYGEN_WEIGHT +
        result.temperatureCompatibility * TEMPERATURE_WEIGHT +
        result.radiationCompatibility * RADIATION_WEIGHT +
        result.acidityCompatibility * ACIDITY_WEIGHT +
        result.mineralCompatibility * MINERAL_WEIGHT;

    // Clamp to valid range
    result.overall = std::clamp(result.overall, 0.0f, 1.0f);

    // Compute gameplay penalties based on overall compatibility
    computePenalties(result);

    return result;
}

PigmentHint BiochemistrySystem::computePigmentHint(const Genome& genome, const PlanetChemistry& chemistry) const {
    PigmentHint hint;

    // Base color from pigment family
    hint.primaryColor = getPigmentFamilyColor(genome.biopigmentFamily);

    // Secondary color based on chemistry and complementary pigment
    uint8_t secondaryPigment = (genome.biopigmentFamily + 3) % 6;  // Offset for variety
    hint.secondaryColor = getPigmentFamilyColor(secondaryPigment);

    // Saturation based on radiation and mineral abundance
    // High radiation = more saturated (protection pigments)
    // Low minerals = less saturated (energy conservation)
    hint.saturationBias = (chemistry.radiationLevel - 1.0f) * 0.3f;
    hint.saturationBias -= (1.0f - chemistry.minerals.phosphorus) * 0.2f;
    hint.saturationBias = std::clamp(hint.saturationBias, -0.5f, 0.5f);

    // Brightness based on temperature and solvent
    // Cold = darker (heat absorption)
    // Hot = lighter (heat reflection)
    float tempNormalized = (chemistry.temperatureBase + 50.0f) / 150.0f;  // Normalize to 0-1 roughly
    hint.brightnessBias = (tempNormalized - 0.5f) * 0.4f;

    // Adjust for deep-sea/bioluminescent conditions (low light)
    if (chemistry.atmosphere.pressure > 3.0f || chemistry.solventType != SolventType::WATER) {
        hint.brightnessBias -= 0.2f;  // Darker in extreme conditions
        hint.saturationBias += 0.2f;   // But more saturated
    }

    hint.brightnessBias = std::clamp(hint.brightnessBias, -0.4f, 0.4f);

    return hint;
}

// ============================================================================
// SPECIES CACHING
// ============================================================================

const SpeciesAffinity& BiochemistrySystem::getSpeciesAffinity(uint32_t speciesId, const Genome& representativeGenome, const PlanetChemistry& chemistry) {
    auto it = m_speciesCache.find(speciesId);

    // Check if cached and still valid
    if (it != m_speciesCache.end()) {
        if (it->second.isValid && (m_currentFrame - it->second.computedFrame) < m_cacheLifetimeFrames) {
            return it->second;
        }
    }

    // Compute and cache
    SpeciesAffinity affinity;
    affinity.speciesId = speciesId;
    affinity.compatibility = computeCompatibility(representativeGenome, chemistry);
    affinity.pigmentHint = computePigmentHint(representativeGenome, chemistry);
    affinity.computedFrame = m_currentFrame;
    affinity.isValid = true;

    m_speciesCache[speciesId] = affinity;
    return m_speciesCache[speciesId];
}

void BiochemistrySystem::invalidateSpeciesCache(uint32_t speciesId) {
    auto it = m_speciesCache.find(speciesId);
    if (it != m_speciesCache.end()) {
        it->second.isValid = false;
    }
}

void BiochemistrySystem::clearAllCaches() {
    m_speciesCache.clear();
}

// ============================================================================
// DIAGNOSTIC METHODS
// ============================================================================

float BiochemistrySystem::getAverageCompatibility() const {
    if (m_speciesCache.empty()) return 1.0f;

    float sum = 0.0f;
    int count = 0;
    for (const auto& pair : m_speciesCache) {
        if (pair.second.isValid) {
            sum += pair.second.compatibility.overall;
            count++;
        }
    }

    return count > 0 ? sum / count : 1.0f;
}

float BiochemistrySystem::getMinimumCompatibility() const {
    if (m_speciesCache.empty()) return 1.0f;

    float minVal = 1.0f;
    for (const auto& pair : m_speciesCache) {
        if (pair.second.isValid) {
            minVal = std::min(minVal, pair.second.compatibility.overall);
        }
    }

    return minVal;
}

void BiochemistrySystem::getCompatibilityDistribution(int& lethal, int& poor, int& moderate, int& good, int& excellent) const {
    lethal = poor = moderate = good = excellent = 0;

    for (const auto& pair : m_speciesCache) {
        if (!pair.second.isValid) continue;

        float compat = pair.second.compatibility.overall;
        if (compat < LETHAL_THRESHOLD) {
            lethal++;
        } else if (compat < POOR_THRESHOLD) {
            poor++;
        } else if (compat < MODERATE_THRESHOLD) {
            moderate++;
        } else if (compat < GOOD_THRESHOLD) {
            good++;
        } else {
            excellent++;
        }
    }
}

void BiochemistrySystem::logStatistics() const {
    int lethal, poor, moderate, good, excellent;
    getCompatibilityDistribution(lethal, poor, moderate, good, excellent);

    std::cout << "=== Biochemistry Compatibility Statistics ===" << std::endl;
    std::cout << "  Species cached: " << m_speciesCache.size() << std::endl;
    std::cout << "  Average compatibility: " << std::fixed << std::setprecision(3) << getAverageCompatibility() << std::endl;
    std::cout << "  Minimum compatibility: " << std::fixed << std::setprecision(3) << getMinimumCompatibility() << std::endl;
    std::cout << "  Distribution:" << std::endl;
    std::cout << "    Lethal (<0.2):    " << lethal << std::endl;
    std::cout << "    Poor (0.2-0.4):   " << poor << std::endl;
    std::cout << "    Moderate (0.4-0.6): " << moderate << std::endl;
    std::cout << "    Good (0.6-0.8):   " << good << std::endl;
    std::cout << "    Excellent (>0.8): " << excellent << std::endl;
    std::cout << "============================================" << std::endl;
}

// ============================================================================
// INTERNAL COMPUTATION HELPERS
// ============================================================================

float BiochemistrySystem::computeSolventCompatibility(const Genome& genome, const PlanetChemistry& chemistry) const {
    // Map solvent type to expected affinity range
    float expectedAffinity;
    switch (chemistry.solventType) {
        case SolventType::WATER:
            expectedAffinity = 0.5f;  // Water-adapted creatures have mid-range affinity
            break;
        case SolventType::AMMONIA:
        case SolventType::METHANE:
            expectedAffinity = 0.15f;  // Cold solvent creatures have low affinity value
            break;
        case SolventType::SULFURIC_ACID:
        case SolventType::ETHANOL:
            expectedAffinity = 0.85f;  // Extreme solvent creatures have high affinity value
            break;
        default:
            expectedAffinity = 0.5f;
    }

    // Calculate how close genome's solvent affinity is to expected
    float affinityDiff = std::abs(genome.solventAffinity - expectedAffinity);

    // Tolerance is affected by membrane fluidity (more fluid = more adaptable)
    float tolerance = 0.2f + genome.membraneFluidity * 0.2f;

    if (affinityDiff <= tolerance) {
        return 1.0f;  // Perfect compatibility within tolerance
    } else {
        // Gradual falloff outside tolerance
        float excess = affinityDiff - tolerance;
        return std::max(0.0f, 1.0f - excess * 2.5f);
    }
}

float BiochemistrySystem::computeOxygenCompatibility(const Genome& genome, const PlanetChemistry& chemistry) const {
    float atmosphericOxygen = chemistry.atmosphere.oxygen;

    // Genome's oxygen tolerance determines preferred oxygen level
    // Low tolerance (0) = prefers anaerobic conditions
    // High tolerance (1) = prefers oxygen-rich conditions
    float preferredOxygen = genome.oxygenTolerance * 0.3f;  // Scale to realistic oxygen range (0-30%)

    float oxygenDiff = std::abs(atmosphericOxygen - preferredOxygen);

    // Metabolic pathway affects tolerance
    float tolerance = 0.05f;  // Base tolerance
    if (genome.metabolicPathway == 0) {
        // Aerobic - more sensitive to low oxygen
        if (atmosphericOxygen < preferredOxygen) {
            tolerance = 0.03f;
        } else {
            tolerance = 0.1f;  // More tolerant of excess oxygen
        }
    } else if (genome.metabolicPathway == 1) {
        // Anaerobic - more tolerant across the board
        tolerance = 0.15f;
    } else {
        // Chemosynthesis/photosynthesis - moderate tolerance
        tolerance = 0.08f;
    }

    if (oxygenDiff <= tolerance) {
        return 1.0f;
    } else {
        float excess = oxygenDiff - tolerance;
        return std::max(0.0f, 1.0f - excess * 4.0f);
    }
}

float BiochemistrySystem::computeTemperatureCompatibility(const Genome& genome, const PlanetChemistry& chemistry) const {
    // Membrane fluidity determines optimal temperature
    // Low fluidity (rigid) = cold-adapted
    // High fluidity (fluid) = warm-adapted
    float optimalTemp = -50.0f + genome.membraneFluidity * 200.0f;  // Range: -50 to +150°C

    float tempDiff = std::abs(chemistry.temperatureBase - optimalTemp);

    // Temperature tolerance from genome
    float tolerance = genome.temperatureTolerance;

    if (tempDiff <= tolerance) {
        return 1.0f;
    } else {
        float excess = tempDiff - tolerance;
        // Temperature mismatches are serious
        return std::max(0.0f, 1.0f - excess / 50.0f);
    }
}

float BiochemistrySystem::computeRadiationCompatibility(const Genome& genome, const PlanetChemistry& chemistry) const {
    // If radiation is low, even non-resistant creatures are fine
    if (chemistry.radiationLevel <= 1.0f) {
        return 1.0f;
    }

    // High radiation requires resistance
    float excessRadiation = chemistry.radiationLevel - 1.0f;
    float protectionNeeded = excessRadiation;
    float protectionAvailable = genome.radiationResistance;

    if (protectionAvailable >= protectionNeeded) {
        return 1.0f;
    } else {
        float deficit = protectionNeeded - protectionAvailable;
        return std::max(0.0f, 1.0f - deficit * 1.5f);
    }
}

float BiochemistrySystem::computeAcidityCompatibility(const Genome& genome, const PlanetChemistry& chemistry) const {
    // pH preference mapping:
    // 0.0 = acidophile (prefers pH 0-4)
    // 0.5 = neutrophile (prefers pH 6-8)
    // 1.0 = alkaliphile (prefers pH 10-14)

    float preferredpH;
    if (genome.pHPreference < 0.33f) {
        // Acidophile
        preferredpH = 2.0f + genome.pHPreference * 6.0f;  // pH 2-4
    } else if (genome.pHPreference < 0.67f) {
        // Neutrophile
        preferredpH = 6.0f + (genome.pHPreference - 0.33f) * 6.0f;  // pH 6-8
    } else {
        // Alkaliphile
        preferredpH = 10.0f + (genome.pHPreference - 0.67f) * 6.0f;  // pH 10-12
    }

    float pHDiff = std::abs(chemistry.acidity - preferredpH);

    // pH tolerance (most organisms have ~2 pH unit tolerance)
    float tolerance = 2.0f;

    if (pHDiff <= tolerance) {
        return 1.0f;
    } else {
        float excess = pHDiff - tolerance;
        return std::max(0.0f, 1.0f - excess * 0.3f);
    }
}

float BiochemistrySystem::computeMineralCompatibility(const Genome& genome, const PlanetChemistry& chemistry) const {
    // High mineralization bias requires high mineral availability
    float mineralDemand = genome.mineralizationBias;

    // Average mineral availability
    float mineralSupply = (chemistry.minerals.iron + chemistry.minerals.calcium +
                           chemistry.minerals.silicon + chemistry.minerals.phosphorus) / 4.0f;

    // Also consider specific minerals based on pigment family
    switch (genome.biopigmentFamily) {
        case 0:  // Chlorophyll - needs magnesium
            mineralSupply = mineralSupply * 0.7f + chemistry.minerals.magnesium * 0.3f;
            break;
        case 2:  // Phycocyanin - needs copper
            mineralSupply = mineralSupply * 0.7f + chemistry.minerals.copper * 0.3f;
            break;
        case 5:  // Flavin - needs sulfur
            mineralSupply = mineralSupply * 0.7f + chemistry.minerals.sulfur * 0.3f;
            break;
        default:
            break;
    }

    if (mineralSupply >= mineralDemand) {
        return 1.0f;
    } else {
        float deficit = mineralDemand - mineralSupply;
        return std::max(0.2f, 1.0f - deficit * 1.5f);  // Mineral deficiency is survivable
    }
}

void BiochemistrySystem::computePenalties(BiochemistryCompatibility& compat) const {
    if (compat.overall >= GOOD_THRESHOLD) {
        // Excellent or good compatibility - no penalties
        compat.energyPenaltyMultiplier = 1.0f;
        compat.healthPenaltyRate = 0.0f;
        compat.reproductionPenalty = 1.0f;
    } else if (compat.overall >= MODERATE_THRESHOLD) {
        // Moderate compatibility - minor energy penalty
        float severity = (GOOD_THRESHOLD - compat.overall) / (GOOD_THRESHOLD - MODERATE_THRESHOLD);
        compat.energyPenaltyMultiplier = 1.0f + severity * 0.2f;  // Up to 20% more energy use
        compat.healthPenaltyRate = 0.0f;
        compat.reproductionPenalty = 1.0f - severity * 0.1f;  // Up to 10% reproduction penalty
    } else if (compat.overall >= POOR_THRESHOLD) {
        // Poor compatibility - significant penalties
        float severity = (MODERATE_THRESHOLD - compat.overall) / (MODERATE_THRESHOLD - POOR_THRESHOLD);
        compat.energyPenaltyMultiplier = 1.2f + severity * 0.3f;  // 20-50% more energy use
        compat.healthPenaltyRate = severity * 0.5f;  // Up to 0.5 health/second loss
        compat.reproductionPenalty = 0.9f - severity * 0.3f;  // 10-40% reproduction penalty
    } else if (compat.overall >= LETHAL_THRESHOLD) {
        // Very poor - serious penalties
        float severity = (POOR_THRESHOLD - compat.overall) / (POOR_THRESHOLD - LETHAL_THRESHOLD);
        compat.energyPenaltyMultiplier = 1.5f + severity * 0.5f;  // 50-100% more energy use
        compat.healthPenaltyRate = 0.5f + severity * 1.5f;  // 0.5-2.0 health/second loss
        compat.reproductionPenalty = 0.6f - severity * 0.4f;  // 40-80% reproduction penalty
    } else {
        // Lethal - rapid death
        compat.energyPenaltyMultiplier = 2.0f + (LETHAL_THRESHOLD - compat.overall) * 5.0f;
        compat.healthPenaltyRate = 2.0f + (LETHAL_THRESHOLD - compat.overall) * 10.0f;
        compat.reproductionPenalty = 0.0f;  // Cannot reproduce
    }
}

glm::vec3 BiochemistrySystem::getPigmentFamilyColor(uint8_t pigmentFamily) const {
    switch (pigmentFamily) {
        case 0:  // Chlorophyll - green
            return glm::vec3(0.2f, 0.6f, 0.2f);
        case 1:  // Carotenoid - orange/red
            return glm::vec3(0.8f, 0.4f, 0.1f);
        case 2:  // Phycocyanin - blue
            return glm::vec3(0.1f, 0.3f, 0.7f);
        case 3:  // Bacteriorhodopsin - purple
            return glm::vec3(0.5f, 0.2f, 0.6f);
        case 4:  // Melanin - brown/black
            return glm::vec3(0.25f, 0.2f, 0.15f);
        case 5:  // Flavin - yellow
            return glm::vec3(0.8f, 0.7f, 0.1f);
        default:
            return glm::vec3(0.5f, 0.5f, 0.5f);  // Gray default
    }
}
