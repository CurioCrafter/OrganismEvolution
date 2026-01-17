#pragma once

#include "../Genome.h"
#include "../../core/CreatureManager.h"
#include "../../environment/BiomeSystem.h"
#include "../../environment/PlanetChemistry.h"
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>

// ============================================================================
// GENOME DIVERSITY SYSTEM
// ============================================================================
// Responsible for generating varied starting genomes that map to biome/climate
// characteristics and tracking population diversity metrics.
//
// Goals:
// - Avoid spawning too many near-identical bodies at start
// - Align genome presets to biomes and niches
// - Track diversity metrics (trait variance, morphology spread)
// - Ensure stable populations with viable locomotion and energy efficiency

namespace Forge {

// ============================================================================
// DIVERSITY METRICS
// ============================================================================

struct DiversityMetrics {
    // Trait variance
    float sizeVariance = 0.0f;
    float speedVariance = 0.0f;
    float visionVariance = 0.0f;
    float efficiencyVariance = 0.0f;

    // Morphology spread (count of distinct archetypes)
    int distinctBodyPlans = 0;      // Different segment counts
    int distinctAppendages = 0;     // Different limb/fin configurations
    int distinctLocomotion = 0;     // Different movement strategies

    // Domain distribution
    int landCreatures = 0;
    int aquaticCreatures = 0;
    int flyingCreatures = 0;

    // Overall diversity score (0-100)
    float overallScore = 0.0f;

    // Calculate diversity score from metrics
    void calculateScore();

    // Reset all metrics
    void reset();

    // Log metrics to console
    void log() const;
};

// ============================================================================
// BIOME-TO-PRESET MAPPING
// ============================================================================

// Niche specialization within a biome
enum class EcologicalNiche {
    GENERALIST,         // Jack of all trades
    AMBUSH_PREDATOR,    // Slow, strong, patient
    PURSUIT_PREDATOR,   // Fast, agile, high endurance
    GRAZER,             // Size, efficiency, herbivore
    SCAVENGER,          // Smell, opportunistic
    BURROWER,           // Underground, touch/vibration
    ARBOREAL,           // Climbing, agility (trees)
    NOCTURNAL,          // Night vision, hearing
    AERIAL_HUNTER,      // Flying predator
    FILTER_FEEDER,      // Aquatic, passive feeding
    DEEP_DIVER,         // Pressure resistance, dark adaptation
};

// Maps biome + niche → preset + bias
struct BiomeGenomeProfile {
    EvolutionStartPreset preset;
    EvolutionGuidanceBias bias;
    EcologicalNiche primaryNiche;
    EcologicalNiche secondaryNiche;

    // Trait modifiers (multipliers on preset ranges)
    float sizeModifier = 1.0f;
    float speedModifier = 1.0f;
    float sensoryModifier = 1.0f;

    // Archetype hint (for specialized randomizers)
    enum class ArchetypeHint {
        GENERIC,
        SHARK,
        BIRD,
        INSECT,
        PREDATOR,
        WHALE,
        OCTOPUS,
        EEL,
        CRUSTACEAN,
    } archetypeHint = ArchetypeHint::GENERIC;
};

// ============================================================================
// GENOME DIVERSITY SYSTEM
// ============================================================================

class GenomeDiversitySystem {
public:
    GenomeDiversitySystem() = default;
    ~GenomeDiversitySystem() = default;

    // ========================================================================
    // PRESET SELECTION
    // ========================================================================

    // Select genome profile based on biome and creature type
    // Uses deterministic selection to ensure consistent variety
    BiomeGenomeProfile selectProfile(
        BiomeType biome,
        CreatureType creatureType,
        const glm::vec3& spawnPosition,
        uint32_t worldSeed
    ) const;

    // Initialize genome using selected profile and planet chemistry
    void initializeGenome(
        Genome& genome,
        const BiomeGenomeProfile& profile,
        const PlanetChemistry& chemistry
    ) const;

    // ========================================================================
    // DIVERSITY TRACKING
    // ========================================================================

    // Add creature to diversity tracking
    void trackCreature(const Genome& genome, CreatureType type);

    // Calculate current diversity metrics
    DiversityMetrics calculateDiversityMetrics() const;

    // Reset diversity tracking
    void resetTracking();

    // Get diversity score (0-100, higher = more diverse)
    float getDiversityScore() const;

    // ========================================================================
    // DIVERSITY ENFORCEMENT
    // ========================================================================

    // Check if population needs more variety in a domain
    bool needsMoreVariety(CreatureDomain domain) const;

    // Get recommended archetype to increase diversity
    BiomeGenomeProfile::ArchetypeHint getRecommendedArchetype(CreatureDomain domain) const;

private:
    // ========================================================================
    // INTERNAL STATE
    // ========================================================================

    // Tracked genomes for diversity calculation
    struct TrackedCreature {
        Genome genome;
        CreatureType type;
    };
    std::vector<TrackedCreature> m_trackedCreatures;

    // Archetype counters
    std::unordered_map<int, int> m_archetypeCounts;  // archetype -> count

    // ========================================================================
    // PRESET SELECTION LOGIC
    // ========================================================================

    // Get base profile for biome type
    BiomeGenomeProfile getBaseProfileForBiome(BiomeType biome) const;

    // Apply niche specialization to profile
    void applyNicheModifiers(
        BiomeGenomeProfile& profile,
        EcologicalNiche niche
    ) const;

    // Select niche deterministically from position and seed
    EcologicalNiche selectNiche(
        BiomeType biome,
        CreatureType type,
        const glm::vec3& position,
        uint32_t seed
    ) const;

    // ========================================================================
    // DIVERSITY CALCULATION HELPERS
    // ========================================================================

    float calculateVariance(const std::vector<float>& values) const;
    int countDistinctValues(const std::vector<int>& values, int tolerance = 0) const;
};

} // namespace Forge
